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

#ifndef BF_TOF3_SERDES_IF_INCLUDED
#define BF_TOF3_SERDES_IF_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <bf_types/bf_types.h>
#include <port_mgr/port_mgr_tof3/aw_lane_cfg.h>

int bf_tof3_serdes_num_lanes_per_ch(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bool bf_tof3_serdes_sppt(bf_dev_id_t dev_id);
bf_status_t bf_tof3_serdes_lane_bcast_set(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          uint32_t macro,
                                          uint32_t en);
bf_status_t bf_tof3_serdes_anlt_lane_status_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *signal_detect,
                                                uint32_t *pmd_rx_lock);
bf_status_t bf_tof3_serdes_anlt_link_up_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t val);
bf_status_t bf_tof3_serdes_anlt_mux_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t num_lanes);
bf_status_t bf_tof3_serdes_anlt_mux_reset(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t num_lanes);
bf_status_t bf_tof3_serdes_an_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *an_done);
bf_status_t bf_tof3_serdes_lt_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *link_training_done);
bf_status_t bf_tof3_serdes_anlt_lane_status_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *signal_detect,
                                                uint32_t *pmd_rx_lock);
bf_status_t bf_tof3_serdes_anlt_link_training_status_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t *lt_running,
    uint32_t *lt_done,
    uint32_t *lt_training_failure,
    uint32_t *lt_rx_ready);
bf_status_t bf_tof3_serdes_addr_range(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *subdev_id,
                                      uint32_t *macro,
                                      uint32_t *phys_tx_ln,
                                      uint32_t *phys_rx_ln,
                                      uint32_t *cmn_ofs,
                                      uint32_t *tx_ofs,
                                      uint32_t *rx_ofs,
                                      uint32_t *sram0,
                                      uint32_t *sram1);
bf_status_t bf_tof3_serdes_init(bf_dev_id_t dev_id,
                                bool skip_post,
                                bool force_fw_dnld,
                                bool skip_power_dn);

bf_status_t bf_tof3_serdes_mss_reset(uint32_t dev_id, uint32_t dev_port);
bf_status_t bf_tof3_serdes_fw_load(uint32_t dev_id,
                                   uint32_t dev_port,
                                   char *fw_path);

// Top level init
bf_status_t bf_tof3_serdes_top_init(bf_dev_id_t dev_id, uint32_t run_post);

// CMN block init
bf_status_t bf_tof3_serdes_cmn_init(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    bool is_4ln);

// per-lane init
bf_status_t bf_tof3_serdes_lane_init(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln);

bf_status_t bf_tof3_serdes_csr_rd_specific_side(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t section,
                                                uint32_t csr,
                                                uint32_t *val);

bf_status_t bf_tof3_serdes_csr_rd(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t csr,
                                  uint32_t *val);

bf_status_t bf_tof3_serdes_csr_wr_specific_side(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t section,
                                                uint32_t csr,
                                                uint32_t val);

bf_status_t bf_tof3_serdes_csr_wr(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t csr,
                                  uint32_t val);
// legacy
bf_status_t bf_tof3_serdes_term_mode_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t term_mode);
bf_status_t bf_tof3_serdes_term_mode_adv_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t term_mode);
bf_status_t bf_tof3_serdes_tx_polarity_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool *inv);
bf_status_t bf_tof3_serdes_tx_polarity_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool inv,
                                           bool apply);
bf_status_t bf_tof3_serdes_rx_polarity_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool *inv);
bf_status_t bf_tof3_serdes_rx_polarity_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           bool inv,
                                           bool apply);
bf_status_t bf_tof3_serdes_txfir_config_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t cm3,
                                            uint32_t cm2,
                                            uint32_t cm1,
                                            uint32_t c0,
                                            uint32_t c1);

bf_status_t bf_tof3_serdes_txfir_config_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *cm3,
                                            uint32_t *cm2,
                                            uint32_t *cm1,
                                            uint32_t *c0,
                                            uint32_t *c1);
bf_status_t bf_tof3_serdes_txfir_range_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *cm3,
                                           uint32_t *cm2,
                                           uint32_t *cm1,
                                           uint32_t *c0,
                                           uint32_t *c1);
bf_status_t bf_tof3_serdes_loopback_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t en);
bf_status_t bf_tof3_serdes_precode_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *tx_en,
                                       bool *rx_en);
bf_status_t bf_tof3_serdes_precode_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool tx_en,
                                       bool rx_en);

bf_status_t bf_tof3_serdes_config_lane(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t rate_gb,
                                       uint32_t is_pam4,
                                       uint32_t prbs_mode,
                                       uint32_t loopback_mode);
bf_status_t bf_tof3_serdes_power_down(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln);
bf_status_t bf_tof3_serdes_squelch_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t en);
bf_status_t bf_tof3_serdes_rx_eq(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t ln,
                                 uint32_t eq_type,
                                 uint32_t tout);
bf_status_t bf_tof3_serdes_pstate_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t tx_pstate,
                                      uint32_t rx_pstate);
bf_status_t bf_tof3_serdes_ctle_adapt_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t rx_ctle_adapt_en,
                                          uint32_t rx_ctle_adapt_boost);
bf_status_t bf_tof3_serdes_ctle_adapt_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *rx_ctle_adapt_en,
                                          uint32_t *rx_ctle_adapt_boost);
bf_status_t bf_tof3_serdes_check_bist(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t timer_threshold,
                                      uint32_t timeout_us,
                                      uint64_t *err_count,
                                      uint32_t *err_count_done,
                                      uint32_t *err_count_overflown,
                                      double *ber);
bf_status_t bf_tof3_serdes_status_get(uint32_t dev_id,
                                      uint32_t dev_port,
                                      uint32_t ln,
                                      lane_cfg_t *rtnd_cfg);

bf_status_t bf_tof3_serdes_config_ln_autoneg(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint64_t basepage,
                                             uint32_t consortium_np_47_16,
                                             bool is_loop);

bf_status_t bf_tof3_serdes_an_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *an_done);

bf_status_t bf_tof3_serdes_lt_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       bool *link_training__done);

bf_status_t bf_tof3_serdes_config_ln(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     bf_port_speed_t speed,
                                     uint32_t num_lanes,
                                     bf_port_prbs_mode_t tx_pat,
                                     bf_port_prbs_mode_t rx_pat);
bf_status_t bf_tof3_serdes_apply_tx_rate(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln);
bf_status_t bf_tof3_serdes_wait_tx_rate_change_done(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln);
bf_status_t bf_tof3_serdes_tx_rate_change_done(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln);
bf_status_t bf_tof3_serdes_apply_rx_rate(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln);
bf_status_t bf_tof3_serdes_wait_rx_rate_change_done(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln);
bf_status_t bf_tof3_serdes_rx_rate_change_done(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln);
bf_status_t bf_tof3_serdes_config_mode(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln);
bf_status_t bf_tof3_serdes_wait_sig_ok(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln);
bf_status_t bf_tof3_serdes_eq_start(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln);
bf_status_t bf_tof3_serdes_check_eq_done(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln);
bf_status_t bf_tof3_serdes_check_cdr_lock(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln);
bf_status_t bf_tof3_serdes_check_bist_lock(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln);
bf_status_t bf_tof3_serdes_prbs_rst_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln);
bf_status_t bf_tof3_serdes_rx_prbs_err_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *err_cnt);
bf_status_t bf_tof3_serdes_serdes_speed_to_rate_and_width(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t serdes_speed,
    bool is_pam4,
    uint32_t *rate,
    uint32_t *width);
bf_status_t bf_tof3_serdes_an_result_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t *an_mr_page_rx,
                                         uint32_t *an_result,
                                         uint64_t *lp_basepage);
bf_status_t bf_tof3_serdes_clk_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   int32_t *tx_ppm,
                                   int32_t *rx_ppm,
                                   double *tx_vco_freq,
                                   double *rx_vco_freq);
bf_status_t bf_tof3_serdes_port_speed_to_serdes_speed(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      bf_port_speed_t speed,
                                                      uint32_t num_lanes,
                                                      uint32_t *serdes_gb,
                                                      bool *is_pam4);
bf_status_t bf_tof3_serdes_cdr_lock_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *cdr_lock);
bf_status_t bf_tof3_serdes_an_construct_advertisement(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint64_t *base_pg,
                                                      uint64_t *cons_np);
bf_status_t bf_tof3_serdes_lt_info_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *lt_fsm_st,
                                       uint32_t *frame_lock);
bf_status_t bf_tof3_serdes_cleanup(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln);
bf_status_t bf_tof3_serdes_glue_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *rx_sts__rx_valid,
                                    uint32_t *anlt_ctrl__link_status,
                                    uint32_t *anlt_mux__group_master,
                                    uint32_t *anlt_mux__group_sel,
                                    uint32_t *anlt_mux__rxsel,
                                    uint32_t *anlt_stat__an_done,
                                    uint32_t *anlt_stat__an_fec_ena,
                                    uint32_t *anlt_stat__an_link_good,
                                    uint32_t *anlt_stat__an_new_page,
                                    uint32_t *anlt_stat__an_rsfec_ena,
                                    uint32_t *anlt_stat__an_tr_disable,
                                    uint32_t *anlt_stat__an_link_control);
bf_status_t bf_tof3_serdes_rx_vga_cap_adapt_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *en,
                                                uint32_t *vga_cap,
                                                bool *use_custom_takeover_ratio,
                                                uint32_t *custom_takeover_ratio,
                                                uint32_t *custom_nyq_mask);
bf_status_t bf_tof3_serdes_rx_vga_cap_adapt_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t en,
                                                uint32_t vga_cap,
                                                bool use_custom_takeover_ratio,
                                                uint32_t custom_takeover_ratio,
                                                uint32_t custom_nyq_mask);
bf_status_t bf_tof3_serdes_clkobs_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_clkobs_pad_t pad,
                                      bf_sds_clkobs_clksel_t clk_src,
                                      int divider);
#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_TOF3_SERDES_IF_INCLUDED
