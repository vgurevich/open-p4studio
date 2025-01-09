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


#ifndef port_mgr_tof2_serdes_h
#define port_mgr_tof2_serdes_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_mgr/bf_tof2_serdes_if.h"
#include "port_mgr_tof2_serdes_defs.h"

bf_status_t port_mgr_tof2_serdes_tx_eq_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           int32_t pre2,
                                           int32_t pre1,
                                           int32_t main,
                                           int32_t post1,
                                           int32_t post2,
                                           bool apply);
bf_status_t port_mgr_tof2_serdes_tx_eq_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           int32_t *pre2,
                                           int32_t *pre1,
                                           int32_t *main,
                                           int32_t *post1,
                                           int32_t *post2);

// apply tap settings directly to HW, do not cache
// used to squelch Tx output
bf_status_t port_mgr_tof2_serdes_tx_taps_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             int32_t pre2,
                                             int32_t pre1,
                                             int32_t main,
                                             int32_t post1,
                                             int32_t post2);
// used to verify hw settings
bf_status_t port_mgr_tof2_serdes_tx_taps_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             int32_t *pre2,
                                             int32_t *pre1,
                                             int32_t *main,
                                             int32_t *post1,
                                             int32_t *post2);
uint32_t port_mgr_tof2_serdes_tile_rd(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ofs);
void port_mgr_tof2_serdes_tile_wr(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ofs,
                                  uint32_t val);
bool port_mgr_tof2_serdes_mode_is_nrz(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln);
bool port_mgr_tof2_serdes_mode_is_pam4(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln);
bf_status_t port_mgr_tof2_serdes_lane_map_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t phys_tx_ln[8],
                                              uint32_t phys_rx_ln[8]);
bf_status_t port_mgr_tof2_serdes_lane_map_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t phys_tx_ln[8],
                                              uint32_t phys_rx_ln[8]);
bf_status_t port_mgr_tof2_serdes_cpu_reset(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int ln);
bf_status_t port_mgr_tof2_serdes_soft_reset(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            int ln);
bf_status_t port_mgr_tof2_serdes_logic_reset(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int ln);
bf_status_t port_mgr_tof2_serdes_group_reset(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int ln,
                                             int phase);
bf_status_t port_mgr_tof2_serdes_lane_reset_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                int ln);

// FW
bf_status_t port_mgr_tof2_serdes_fw_file_info_get(unsigned char *buf_ptr,
                                                  uint32_t *hash_code,
                                                  uint32_t *crc);
bf_status_t port_mgr_tof2_serdes_fw_running_info_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t *running_hash_code,
    uint32_t *running_crc);
bf_status_t port_mgr_tof2_serdes_fw_check_load_reqd(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t fw_hash_code,
                                                    uint32_t fw_crc,
                                                    bool *load_reqd);
bf_status_t port_mgr_tof2_serdes_fw_load_to_buffer(char *fw_file_name,
                                                   uint8_t **fw_buffer_p,
                                                   uint32_t *fw_len,
                                                   uint32_t *fw_hash_code,
                                                   uint32_t *fw_crc);
bf_status_t port_mgr_tof2_serdes_fw_load_from_buffer(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     unsigned char *buf_ptr);

bf_status_t port_mgr_tof2_serdes_rx_checker_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *en);
bf_status_t port_mgr_tof2_serdes_rx_checker_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool en);
bf_status_t port_mgr_tof2_serdes_fw_hash_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *hash_code);
bf_status_t port_mgr_tof2_serdes_fw_crc_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *crc);
bf_status_t port_mgr_tof2_serdes_fw_debug_cmd(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              int ln,
                                              uint32_t section,
                                              uint32_t index,
                                              uint32_t *result);
bf_status_t port_mgr_tof2_serdes_lane_mode_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               bf_serdes_encoding_mode_t mode);
bf_status_t port_mgr_tof2_serdes_lane_mode_nrz_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln);
bf_status_t port_mgr_tof2_serdes_lane_mode_pam4_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln);
bf_status_t port_mgr_tof2_serdes_sig_detect_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *sig_detect,
                                                bool *phy_ready);
bf_status_t port_mgr_tof2_ctle_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t ctle_over_val);
bf_status_t port_mgr_tof2_serdes_ppm_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         int32_t *ppm);
bf_status_t port_mgr_tof2_serdes_an_done_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             bool *an_done);
bf_status_t port_mgr_tof2_serdes_adapt_done_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *adapt_done);
bf_status_t port_mgr_tof2_serdes_fw_adapt_cnt_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  int ln,
                                                  uint32_t *cnt);
bf_status_t port_mgr_tof2_serdes_fw_readapt_cnt_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    int ln,
                                                    uint32_t *cnt);
bf_status_t port_mgr_tof2_serdes_fw_link_lost_cnt_get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      int ln,
                                                      uint32_t *cnt);
bf_status_t port_mgr_tof2_serdes_fw_lane_speed_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    int ln,
    uint32_t *G,
    bf_serdes_encoding_mode_t *enc_mode);
bf_status_t port_mgr_tof2_serdes_tx_pol_inv_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool inv,
                                                bool apply);
bf_status_t port_mgr_tof2_serdes_tx_pol_inv_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *inv);
bf_status_t port_mgr_tof2_serdes_tx_polarity_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 bool *inv);
bf_status_t port_mgr_tof2_serdes_rx_pol_inv_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool inv,
                                                bool apply);
bf_status_t port_mgr_tof2_serdes_rx_pol_inv_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *inv);
bf_status_t port_mgr_tof2_serdes_rx_polarity_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 bool *inv);
bf_status_t port_mgr_tof2_serdes_sram_bist_grp(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_serdes_rom_bist_grp(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_serdes_power_on_self_test(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t n_lanes);
bf_status_t port_mgr_tof2_serdes_un_config_ln(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln);
bf_status_t port_mgr_tof2_serdes_config_ln_an(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint64_t basepage,
                                              uint32_t consortium_np_47_16,
                                              bool is_loop);
bf_status_t port_mgr_tof2_serdes_config_ln_nrz(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    port_mgr_serdes_nrz_speed_t speed,
    port_mgr_tof2_prbs_mode_t tx_pat,
    port_mgr_tof2_prbs_mode_t rx_pat);
bf_status_t port_mgr_tof2_serdes_config_ln_pam4(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    port_mgr_serdes_pam4_speed_t speed,
    port_mgr_tof2_prbs_mode_t tx_pat,
    port_mgr_tof2_prbs_mode_t rx_pat);
bf_status_t port_mgr_tof2_serdes_power_dn_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              bool rx_off,
                                              bool tx_off,
                                              bool rx_bg_off,
                                              bool tx_bg_off);
bf_status_t port_mgr_tof2_serdes_prbs_rst_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln);
bf_status_t port_mgr_tof2_serdes_rx_prbs_err_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t *err_cnt);
bf_status_t port_mgr_tof2_serdes_tx_prbs_cfg_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t *tx_cfg);
bf_status_t port_mgr_tof2_serdes_eye_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         float *eye_1,
                                         float *eye_2,
                                         float *eye_3);
bf_status_t port_mgr_tof2_serdes_of_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *of);
bf_status_t port_mgr_tof2_serdes_hf_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *hf);
bf_status_t port_mgr_tof2_serdes_delta_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *delta);
bf_status_t port_mgr_tof2_serdes_edge_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *edge1,
                                          uint32_t *edge2,
                                          uint32_t *edge3,
                                          uint32_t *edge4);
bf_status_t port_mgr_tof2_serdes_dfe_nrz_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *tap1,
                                             uint32_t *tap2,
                                             uint32_t *tap3);
bf_status_t port_mgr_tof2_serdes_dfe_pam4_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              float *f0,
                                              float *f1,
                                              float *ratio);
bf_status_t port_mgr_tof2_serdes_skef_val_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *skef_val);
bf_status_t port_mgr_tof2_serdes_dac_val_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *dac_val);
bf_status_t port_mgr_tof2_serdes_ctle_val_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t ctle_sel,
                                              uint32_t *ctle_map_0,
                                              uint32_t *ctle_map_1);
bf_status_t port_mgr_tof2_serdes_ctle_over_val_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *ctle_over_val);
bf_status_t port_mgr_tof2_serdes_agcgain_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t ctle_gain_1,
                                             uint32_t ctle_gain_2);
bf_status_t port_mgr_tof2_serdes_ctle_gain_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *ctle_gain_1,
                                               uint32_t *ctle_gain_2);
bf_status_t port_mgr_tof2_serdes_ffe_taps_pam4_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   int32_t *k1,
                                                   int32_t *k2,
                                                   int32_t *k3,
                                                   int32_t *k4,
                                                   int32_t *s1,
                                                   int32_t *s2);
bf_status_t port_mgr_tof2_serdes_f13_val_pam4_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t *f13_val);
bf_status_t port_mgr_tof2_serdes_fw_loaded_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *loaded);
bf_status_t port_mgr_tof2_serdes_fec_analyzer_tei_get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t *tei);
bf_status_t port_mgr_tof2_serdes_fec_analyzer_teo_get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t *teo);
bf_status_t port_mgr_tof2_serdes_fec_analyzer_init_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t err_type,
                                                       uint32_t T,
                                                       uint32_t M,
                                                       uint32_t N);
bf_status_t port_mgr_tof2_serdes_clkobs_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_clkobs_pad_t pad,
                                            bf_sds_clkobs_clksel_t clk_src,
                                            int divider,
                                            int daisy_sel);
bf_status_t port_mgr_tof2_clkobs_drive_strength_set(bf_dev_id_t dev_id,
                                                    int drive_strength);
bf_status_t port_mgr_tof2_serdes_init_log_to_phy_reg_range(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_serdes_an_status_get(bf_dev_id_t dev_id,
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
bf_status_t port_mgr_tof2_serdes_an_lp_base_page_get(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint64_t *lp_basepage);
bf_status_t port_mgr_tof2_serdes_an_hcd_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *hcd,
                                            bool *base_r_fec,
                                            bool *rs_fec);
bf_status_t port_mgr_tof2_serdes_an_lp_pages_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint64_t *lp_basepage,
                                                 uint64_t *lp_nextpage1,
                                                 uint64_t *lp_nextpage2);
bf_status_t port_mgr_tof2_serdes_lt_status_get(bf_dev_id_t dev_id,
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
bf_status_t port_mgr_tof2_serdes_config_ln_mode(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t ln_mode,
                                                bool an_enabled);
bf_status_t port_mgr_tof2_serdes_tx_bandgap_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *tx_bg);
bf_status_t port_mgr_tof2_serdes_tx_bandgap_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t tx_bg,
                                                bool apply);
bf_status_t port_mgr_tof2_serdes_rx_bandgap_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *rx_bg);
bf_status_t port_mgr_tof2_serdes_rx_bandgap_hw_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *rx_bg);
bf_status_t port_mgr_tof2_serdes_rx_bandgap_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t rx_bg,
                                                bool apply);
bf_status_t port_mgr_tof2_serdes_rx_bandgap_hw_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t rx_bg_val);
bf_status_t port_mgr_tof2_serdes_tx_bandgap_hw_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t tx_bg_val);
bf_status_t port_mgr_tof2_serdes_tx_bandgap_hw_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *tx_bg);
bf_status_t port_mgr_tof2_serdes_isi_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t isi_vals[16]);
void port_mgr_tof2_bandgap_load(void);
void port_mgr_tof2_serdes_fw_cmd_lock_init(void);
bf_status_t port_mgr_tof2_serdes_fw_cmd_w_detail(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 int ln,
                                                 uint32_t cmd,
                                                 uint32_t *rsp,
                                                 uint32_t detail);
bf_status_t port_mgr_tof2_serdes_fw_reg_wr(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           int ln,
                                           uint32_t addr,
                                           uint32_t data);
bf_status_t port_mgr_tof2_serdes_fw_reg_section_wr(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   int ln,
                                                   uint32_t addr,
                                                   uint32_t data,
                                                   uint32_t section);
bf_status_t port_mgr_tof2_serdes_fw_reg_section_rd(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   int ln,
                                                   uint32_t addr,
                                                   uint32_t *data,
                                                   uint32_t section);
bf_status_t port_mgr_tof2_serdes_fw_dfe_pam4_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t dfe_val[12]);
bf_status_t port_mgr_tof2_serdes_fw_ver_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *fw_ver);
bf_status_t port_mgr_tof2_serdes_error_inject_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t n_errs);
bf_status_t port_mgr_tof2_serdes_temperature_start_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool auto_);
bf_status_t port_mgr_tof2_serdes_temperature_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bool auto_,
                                                 float *temp);
bf_status_t port_mgr_tof2_serdes_fw_temperature_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    float *temp);
bf_status_t port_mgr_tof2_serdes_init_lane_for_fw(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    bf_serdes_encoding_mode_t enc_mode,
    port_mgr_tof2_prbs_mode_t tx_pat,
    port_mgr_tof2_prbs_mode_t rx_pat);
bf_status_t port_mgr_tof2_serdes_init_lane_for_an(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln);
bf_status_t port_mgr_tof2_serdes_disable_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln);
bf_status_t port_mgr_tof2_serdes_precode_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             bool *tx_en,
                                             bool *rx_en);
bf_status_t port_mgr_tof2_serdes_precode_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             bool tx_en,
                                             bool rx_en);
bf_status_t port_mgr_tof2_serdes_fw_precode_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool phy_mode_tx_en,
                                                bool phy_mode_rx_en,
                                                bool anlt_mode_tx_en,
                                                bool anlt_mode_rx_en);
bf_status_t port_mgr_tof2_serdes_fw_precode_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                bool *phy_mode_tx_en,
                                                bool *phy_mode_rx_en,
                                                bool *anlt_mode_tx_en,
                                                bool *anlt_mode_rx_en);
bf_status_t port_mgr_tof2_serdes_pll_info_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              pll_info_t *tx_pll_info,
                                              pll_info_t *rx_pll_info);
bf_status_t port_mgr_tof2_serdes_eye_plot_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint8_t **plot_data);
bf_status_t port_mgr_tof2_serdes_ac_couple_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               bool *ac_en);
bf_status_t port_mgr_tof2_serdes_ac_couple_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               bool ac_en);
uint32_t port_mgr_tof2_serdes_tile_efuse_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             int bank);
void port_mgr_tof2_serdes_a0_upper_bits_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t x3_bits);
bf_status_t port_mgr_tof2_serdes_known_value_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_serdes_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info);
#ifdef __cplusplus
}
#endif /* C++ */

#endif
