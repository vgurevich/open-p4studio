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


#ifndef port_mgr_av_sd_h_included
#define port_mgr_av_sd_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/* Well-known Sbus node ids */
#define sbm 0xfd
#define sbc 0xfe

/* PMRO (temperature/voltage sensor) IP requires a 2Mhz clock. It provives
 * a divider to divide down whatever frequency the core is feeding it to
 * 2Mhz, but it needs to know what the core frequency is.
 * For Tofino, the core frequency is supplying a 100Mhz clk
 */
#define PMRO_FREQ (100000000 /*100Mhz*/)

void *port_mgr_av_sd_get_aapl(bf_dev_id_t dev_id);
void port_mgr_av_sd_set_aapl(bf_dev_id_t dev_id);
int port_mgr_av_sd_access_fn_set(bf_dev_id_t dev_id,
                                 bf_serdes_access_method_e method);

uint32_t port_mgr_av_sd_encode_sbus_addr(bf_dev_id_t dev_id,
                                         int ring,
                                         int node);
void port_mgr_av_sd_decode_sbus_addr(uint32_t sbus_addr,
                                     bf_dev_id_t *dev_id,
                                     int *ring,
                                     int *node);
uint32_t port_mgr_av_sd_fw_ver_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_fw_build_id_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_load_firmware(
    bf_dev_id_t dev_id, int ring, int sd, uint32_t fw_ver, char *fw_path);
void port_mgr_av_sd_init_aapl(bf_dev_id_t dev_id, int tcp_mode);
void port_mgr_av_sd_identify_serdes_node_types(bf_dev_id_t dev_id);
void port_mgr_av_sd_set_initial_state(bf_dev_id_t dev_id);
uint32_t port_mgr_av_sd_sbus_rd(bf_dev_id_t dev_id,
                                int ring,
                                int sd,
                                uint32_t reg);
void port_mgr_av_sd_sbus_wr(
    bf_dev_id_t dev_id, int ring, int sd, uint32_t reg, uint32_t data);
int port_mgr_av_sd_spico_int(
    bf_dev_id_t dev_id, int ring, int sd, int interrupt, uint32_t int_data);
int port_mgr_av_sd_init_serdes(bf_dev_id_t dev_id,
                               int ring,
                               int sd,
                               bf_sds_line_rate_mode_t line_rate,
                               bool init_rx,
                               bool init_tx,
                               bool tx_drv_en,
                               bool phase_cal);
int port_mgr_av_sd_init(bf_dev_id_t dev_id,
                        int ring,
                        int sd,
                        int reset,
                        int init_mode,
                        int divider,
                        int data_width,
                        int phase_cal,
                        int output_en);
int port_mgr_av_sd_check_tx_pll_state(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      uint32_t expected_divider);
int port_mgr_av_sd_check_rx_pll_state(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      uint32_t expected_divider);
int port_mgr_av_sd_get_tx_output_en(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_set_tx_output_en(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    int en);
int port_mgr_av_sd_set_rx_tx_and_tx_output_en(bf_dev_id_t dev_id,
                                              int ring,
                                              int sd,
                                              int rx_en,
                                              int tx_en,
                                              int tx_output_en);
int port_mgr_av_sd_elec_idle_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_signal_ok_en_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_signal_ok_thresh_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_signal_ok_thresh_set(bf_dev_id_t dev_id,
                                        int ring,
                                        int sd,
                                        int thresh);
int port_mgr_av_sd_signal_ok_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_los_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_get_error_count(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_start_dfe_ical_w_pcal(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_start_dfe_ical_no_pcal(bf_dev_id_t dev_id,
                                           int ring,
                                           int sd);
void port_mgr_av_sd_start_dfe_ical_no_pcal_fixed_hf(bf_dev_id_t dev_id,
                                                    int ring,
                                                    int sd,
                                                    int hf);
void port_mgr_av_sd_start_dfe_ical_no_pcal_fixed_lf(bf_dev_id_t dev_id,
                                                    int ring,
                                                    int sd,
                                                    int lf);
void port_mgr_av_sd_start_dfe_ical_no_pcal_fixed_hf_seeded_dc(
    bf_dev_id_t dev_id, int ring, int sd, int hf, int dc);
void port_mgr_av_sd_start_dfe_ical(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_start_dfe_pcal(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_start_dfe_pi_cal(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_start_dfe_adaptive(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_stop_dfe_adaptive(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_stop_dfe(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_check_dfe_running(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_log_dfe_st(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_dump_dfe(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_dump_eye(
    bf_dev_id_t dev_id, int ring, int sd, int plot, ucli_context_t *uc);
int port_mgr_av_sd_tx_invert_set(bf_dev_id_t dev_id, int ring, int sd, int inv);
int port_mgr_av_sd_rx_invert_set(bf_dev_id_t dev_id, int ring, int sd, int inv);
int port_mgr_av_sd_tx_invert_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_rx_invert_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_tx_error_inject_set(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       int num_bits);
int port_mgr_av_sd_rx_error_inject_set(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       int num_bits);
int port_mgr_av_sd_tx_data_sel_set(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   int sel);
int port_mgr_av_sd_tx_data_sel_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_set_rx_cmp_sel(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  int sel);
int port_mgr_av_sd_get_rx_cmp_sel(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_get_rx_cmp_mode(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_set_rx_cmp_mode(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   int mode);
#if 0
hw_serdes_rx_data_qual_t port_mgr_av_sd_get_rx_data_qual(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_set_rx_data_qual(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    hw_serdes_rx_data_qual_t qual);
#endif  // 0
int port_mgr_av_sd_rx_term_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_rx_term_set(bf_dev_id_t dev_id, int ring, int sd, int term);
int port_mgr_av_sd_tx_pll_clk_source_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_tx_pll_clk_source_set(bf_dev_id_t dev_id,
                                         int ring,
                                         int sd,
                                         int clk);
int port_mgr_av_sd_spico_clk_source_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_spico_clk_source_set(bf_dev_id_t dev_id,
                                        int ring,
                                        int sd,
                                        int clk);
int port_mgr_av_sd_dfe_param_set(
    bf_dev_id_t dev_id, int ring, int sd, int row, int col, int value);
int port_mgr_av_sd_dfe_param_get(
    bf_dev_id_t dev_id, int ring, int sd, int row, int col, int *value);
int port_mgr_av_sd_get_tx_eq(
    bf_dev_id_t dev_id, int ring, int sd, int *pre, int *atten, int *post);
int port_mgr_av_sd_set_tx_eq(
    bf_dev_id_t dev_id, int ring, int sd, int pre, int atten, int post);
int port_mgr_av_sd_get_tx_eq_limits(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    int *pre_min,
                                    int *atten_min,
                                    int *post_min,
                                    int *pre_max,
                                    int *atten_max,
                                    int *post_max);
int port_mgr_av_sd_check_crc(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_autoneg_en_set(bf_dev_id_t dev_id, int ring, int sd, int en);
int port_mgr_av_sd_autoneg_advert_set(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      uint64_t base_pg,
                                      int num_next_pg,
                                      uint64_t *next_pg);
int port_mgr_av_sd_autoneg_start(bf_dev_id_t dev_id,
                                 int ring,
                                 int sd,
                                 uint64_t base_pg,
                                 int num_next_pg,
                                 bool disable_nonce_match,
                                 bool disable_link_inhibit_timer);
int port_mgr_av_sd_autoneg_stop(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_autoneg_st_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_autoneg_hcd_fec_get(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       bf_port_speed_t *resolved_hcd,
                                       bf_fec_type_t *resolved_fec,
                                       uint32_t *av_hcd);
int port_mgr_av_sd_hcd_speed_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 int tx_sd,
                                 int rx_sd,
                                 bf_port_speed_t speed);
int port_mgr_av_sd_link_training_st_get(bf_dev_id_t dev_id,
                                        int ring,
                                        int sd,
                                        bf_lt_state_e *lt_st);
void port_mgr_av_sd_link_training_st_extended_get(bf_dev_id_t dev_id,
                                                  int ring,
                                                  int sd,
                                                  int *failed,
                                                  int *in_prg,
                                                  int *rx_trnd,
                                                  int *frm_lk,
                                                  int *rmt_rq,
                                                  int *lcl_rq,
                                                  int *rmt_rcvr_rdy);
int port_mgr_av_sd_pll_lock_get(bf_dev_id_t dev_id,
                                int ring,
                                int tx_sd,
                                int rx_sd,
                                bool *tx_sd_ready,
                                bool *rx_sd_ready);
int port_mgr_av_sd_tx_pll_state_get(
    bf_dev_id_t dev_id, int ring, int sd, bool *locked, int *div, int *freq);
int port_mgr_av_sd_rx_pll_state_get(
    bf_dev_id_t dev_id, int ring, int sd, bool *locked, int *div, int *freq);
int port_mgr_av_sd_near_loopback_set(bf_dev_id_t dev_id,
                                     int ring,
                                     int sd,
                                     bool en);
int port_mgr_av_sd_far_loopback_set(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    bool en);
int port_mgr_av_sd_near_loopback_get(bf_dev_id_t dev_id,
                                     int ring,
                                     int sd,
                                     bool *en);
int port_mgr_av_sd_tx_drv_en_set(bf_dev_id_t dev_id,
                                 int ring,
                                 int sd,
                                 bool tx_drv_en);
int port_mgr_av_sd_tx_drv_en_get(bf_dev_id_t dev_id,
                                 int ring,
                                 int sd,
                                 bool *tx_drv_en);
int port_mgr_av_sd_dfe_running_get(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   bool *running);
int port_mgr_av_sd_rx_eq_ctle_set(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  int ctle_dc,
                                  int ctle_lf,
                                  int ctle_hf,
                                  int ctle_bw);
int port_mgr_av_sd_rx_eq_ctle_get(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  int *ctle_dc,
                                  int *ctle_lf,
                                  int *ctle_hf,
                                  int *ctle_bw);
int port_mgr_av_sd_dfe_tap_set(
    bf_dev_id_t dev_id, int ring, int sd, int dfe_tap_num, int dfe_tap_val);
int port_mgr_av_sd_dfe_tap_get(
    bf_dev_id_t dev_id, int ring, int sd, int dfe_tap_num, int *dfe_tap_val);
int port_mgr_av_sd_dfe_gain_set(bf_dev_id_t dev_id,
                                int ring,
                                int sd,
                                uint32_t dfe_gain);
int port_mgr_av_sd_dfe_gain_get(bf_dev_id_t dev_id,
                                int ring,
                                int sd,
                                uint32_t *dfe_gain);
int port_mgr_av_sd_rx_eq_status_get(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    int *dfe_status);
int port_mgr_av_sd_rx_eq_vos_done_get(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      bool *vos_done);
int port_mgr_av_sd_rx_eq_cal_param_set(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       int ctle_dc_hint,
                                       int dfe_gain_range,
                                       int pcal_loop_cnt);
int port_mgr_av_sd_rx_eq_cal_param_get(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       int *ctle_dc_hint,
                                       int *dfe_gain_range,
                                       int *pcal_loop_cnt);
int port_mgr_av_sd_rx_eq_cal_adv_run(bf_dev_id_t dev_id,
                                     int ring,
                                     int sd,
                                     bf_sds_rx_cal_mode_t cal_cmd,
                                     int ctle_cal_cfg,
                                     int dfe_fixed);
int port_mgr_av_sd_rx_eq_cal_eye_get(bf_dev_id_t dev_id,
                                     int ring,
                                     int sd,
                                     int *cal_eye);
int port_mgr_av_sd_tx_fixed_pat_set(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    int fixed_pat[4]);
int port_mgr_av_sd_tx_fixed_pat_get(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    int fixed_pat[4]);
int port_mgr_av_sd_rx_cmp_mode_set(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   int rx_patsel);
int port_mgr_av_sd_rx_patsel_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_rx_data_get(bf_dev_id_t dev_id,
                               int ring,
                               int sd,
                               int rx_data[4]);
int port_mgr_av_sd_rx_eye_get(bf_dev_id_t dev_id,
                              int ring,
                              int sd,
                              bf_sds_rx_eye_meas_mode_t mode,
                              bf_sds_rx_eye_meas_ber_t ber,
                              int *meas_eye);
int port_mgr_av_sd_rx_full_eye_get(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   bf_sds_rx_eye_meas_ber_t ber,
                                   char *meas_eye,
                                   int eye_data_max_len);
int port_mgr_av_sd_offset_set(
    bf_dev_id_t dev_id, int ring, int sd, int x, int y);
int port_mgr_av_sd_offset_step_set(
    bf_dev_id_t dev_id, int ring, int sd, int x, int y);
void port_mgr_av_sd_set_ignore_bcast(bf_dev_id_t dev_id,
                                     int ring,
                                     int sd,
                                     int ignore);
void port_mgr_av_sd_temp_read_start(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    uint32_t channel);
int port_mgr_av_sd_temp_read_get(bf_dev_id_t dev_id,
                                 int ring,
                                 int sd,
                                 uint32_t channel);
void port_mgr_av_sd_voltage_read_start(bf_dev_id_t dev_id,
                                       int ring,
                                       int sd,
                                       uint32_t channel);
int port_mgr_av_sd_voltage_read_get(bf_dev_id_t dev_id,
                                    int ring,
                                    int sd,
                                    uint32_t channel);
void port_mgr_av_sd_reset(
    bf_dev_id_t dev_id, int ring, int sd, bool node_reset, bool microp_reset);
void port_mgr_av_sd_pgm_symmetric(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  int speed,
                                  bool tx_output_en,
                                  float pll_ovrclk);
void port_mgr_av_sd_pgm_asymmetric_tx(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      int speed,
                                      int rx_speed,
                                      int rx_en,
                                      bool tx_output_en,
                                      float pll_ovrclk);
void port_mgr_av_sd_pgm_asymmetric_rx(bf_dev_id_t dev_id,
                                      int ring,
                                      int sd,
                                      int speed,
                                      int tx_speed,
                                      int tx_en,
                                      float pll_ovrclk);
void port_mgr_av_sd_tx_rx_en_get(
    bf_dev_id_t dev_id, int ring, int sd, bool *tx_en, bool *rx_en);
void port_mgr_av_sd_tx_rx_en_set(
    bf_dev_id_t dev_id, int ring, int sd, bool tx_en, bool rx_en);
uint32_t port_mgr_av_sd_o_core_status_get(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_an_status_get(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  bool *lp_base_pg_rdy,
                                  bool *lp_next_pg_rdy,
                                  bool *an_good,
                                  bool *an_complete,
                                  bool *an_failed);
uint32_t port_mgr_av_sd_lp_np_rdy_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_lp_base_rdy_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_an_cmplt_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_an_good_get(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_fc_fec_resolution_get(bf_dev_id_t dev_id,
                                              int ring,
                                              int sd);
uint32_t port_mgr_av_sd_rs_fec_resolution_get(bf_dev_id_t dev_id,
                                              int ring,
                                              int sd);
uint64_t port_mgr_av_sd_lp_base_pg_get(bf_dev_id_t dev_id, int ring, int sd);
uint64_t port_mgr_av_sd_lp_next_pg_get(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_next_pg_set(bf_dev_id_t dev_id,
                                int ring,
                                int sd,
                                uint64_t next_pg);
uint32_t port_mgr_av_sd_pmd_training_start(bf_dev_id_t dev_id,
                                           int ring,
                                           int sd,
                                           bool hi_spd);
uint32_t port_mgr_av_sd_pmd_training_stop(bf_dev_id_t dev_id, int ring, int sd);
uint32_t port_mgr_av_sd_pmd_training_restart(bf_dev_id_t dev_id,
                                             int ring,
                                             int sd);
void port_mgr_av_sd_clause_72_training_set(bf_dev_id_t dev_id,
                                           int ring,
                                           int sd,
                                           uint32_t pcs_lane);
void port_mgr_av_sd_clause_92_training_set(bf_dev_id_t dev_id,
                                           int ring,
                                           int sd,
                                           uint32_t pcs_lane);
int port_mgr_av_sd_assert_hcd_link_status(
    bf_dev_id_t dev_id, int ring, int tx_sd, int rx_sd, uint32_t hcd);
int port_mgr_av_sd_eye_metric_get(bf_dev_id_t dev_id,
                                  int ring,
                                  int sd,
                                  uint32_t *eye_metric);
int port_mgr_av_sd_delay_cal(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_pll_bbgain_set(bf_dev_id_t dev_id,
                                   int ring,
                                   int sd,
                                   bool is_10g);
void port_mgr_av_sd_pll_bbgain_set_these(bf_dev_id_t dev_id,
                                         int ring,
                                         int sd,
                                         uint32_t tx_pll_setting,
                                         uint32_t rx_pll_setting);
int port_mgr_av_sd_dump_vbtc(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_vbtc_get(bf_dev_id_t dev_id,
                            int ring,
                            int sd,
                            int *eye_ht_1e06,
                            int *eye_ht_1e10,
                            int *eye_ht_1e12,
                            int *eye_ht_1e15,
                            int *eye_ht_1e17);
int port_mgr_av_sd_map_eye(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_map_eye_quick(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_unlock_pcie(void);
void port_mgr_av_sd_lock_pcie(void);
int port_mgr_av_sd_signal_ok_live_get(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_av_sd_frequency_lock_get(bf_dev_id_t dev_id, int ring, int sd);

void port_mgr_av_sd_tx_loop_bandwidth_set(bf_dev_id_t dev_id,
                                          int ring,
                                          int sd,
                                          uint32_t tx_pll_setting);
void port_mgr_av_sd_rx_loop_bandwidth_set(bf_dev_id_t dev_id,
                                          int ring,
                                          int sd,
                                          uint32_t rx_pll_setting);
void port_mgr_av_sd_tx_loop_bandwidth_get(bf_dev_id_t dev_id,
                                          int ring,
                                          int sd,
                                          uint32_t *tx_pll_setting);
void port_mgr_av_sd_rx_loop_bandwidth_get(bf_dev_id_t dev_id,
                                          int ring,
                                          int sd,
                                          uint32_t *rx_pll_setting);
int port_mgr_av_sd_calibration_status_get(bf_dev_id_t dev_id, int ring, int sd);
void port_mgr_av_sd_encode_bitrate_and_width(int speed,
                                             uint32_t *bit_rate_code,
                                             uint32_t *data_width);
#ifdef __cplusplus
}
#endif /* C++ */

#endif  // port_mgr_av_sd_h_included
