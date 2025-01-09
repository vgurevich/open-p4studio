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

#include <stdint.h>
#include <bf_types/bf_types.h>
#include "aw_if.h"

bf_status_t bf_tof3_serdes_pmd_anlt_logical_lane_num_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t logical_lane, uint32_t an_no_attached);
bf_status_t bf_tof3_serdes_pmd_anlt_logical_lane_num_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * logical_lane, uint32_t * an_no_attached);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_adv_ability_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * adv_ability);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_adv_ability_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * adv_ability);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_config_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ms_per_ck, uint32_t status_check_disable, uint32_t next_page_en, uint32_t an_no_nonce_check);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_config_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * ms_per_ck, uint32_t * status_check_disable, uint32_t * next_page_en, uint32_t * an_no_nonce_check);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_start_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t start);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_start_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * start);
bf_status_t bf_tof3_serdes_pmd_anlt_auto_neg_status_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * link_good);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_en_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_en_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * en);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_config_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ms_per_ck, uint32_t width, uint32_t clause, uint32_t preset_check, uint32_t mod);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_config_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * ms_per_ck, uint32_t * width, uint32_t * clause, uint32_t * preset_check, uint32_t * mod);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_init_preset_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t clause, uint32_t init[], uint32_t preset[], uint32_t preset2[]);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_init_preset_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t clause, uint32_t init[], uint32_t preset[], uint32_t preset2[]);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_min_max_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t pre1_max, uint32_t main_max, uint32_t post1_max, uint32_t pre1_min, uint32_t main_min, uint32_t post1_min);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_min_max_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * pre1_max, uint32_t * main_max, uint32_t * post1_max, uint32_t * pre1_min, uint32_t * main_min, uint32_t * post1_min);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_start_set (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t start);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_start_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * start);
bf_status_t bf_tof3_serdes_pmd_anlt_link_training_status_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * lt_running, uint32_t * lt_done, uint32_t * lt_training_failure, uint32_t * lt_rx_ready);
bf_status_t bf_tof3_serdes_pmd_refclk_termination_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_refclk_term_mode_t lsrefbuf_term_mode);
bf_status_t bf_tof3_serdes_pmd_refclk_termination_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_refclk_term_mode_t *lsrefbuf_term_mode);
bf_status_t bf_tof3_serdes_pmd_rx_termination_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_acc_term_mode_t acc_term_mode);
bf_status_t bf_tof3_serdes_pmd_rx_termination_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_acc_term_mode_t *acc_term_mode);
bf_status_t bf_tof3_serdes_pmd_force_signal_detect_config_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_force_sigdet_mode_t sigdet_mode);
bf_status_t bf_tof3_serdes_pmd_force_signal_detect_config_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_force_sigdet_mode_t *sigdet_mode);
bf_status_t bf_tof3_serdes_pmd_tx_disable_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t tx_disable);
bf_status_t bf_tof3_serdes_pmd_tx_disable_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *tx_disable);
bf_status_t bf_tof3_serdes_pmd_txfir_config_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_txfir_config_t txfir_cfg);
bf_status_t bf_tof3_serdes_pmd_txfir_config_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_txfir_config_t *txfir_cfg);
bf_status_t bf_tof3_serdes_pmd_tx_tap_mode_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *max_rng_cm3, uint32_t *max_rng_cm2, uint32_t *max_rng_cm1, uint32_t *max_rng_c1, uint32_t *max_rng_c0);
bf_status_t bf_tof3_serdes_pmd_tx_pam4_precoder_override_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_tx_pam4_precoder_override_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *en);
bf_status_t bf_tof3_serdes_pmd_tx_pam4_precoder_enable_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t gray_en, uint32_t plusd_en);
bf_status_t bf_tof3_serdes_pmd_tx_pam4_precoder_enable_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *gray_en, uint32_t *plusd_en);
bf_status_t bf_tof3_serdes_pmd_rx_pam4_precoder_override_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_rx_pam4_precoder_override_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *en);
bf_status_t bf_tof3_serdes_pmd_rx_pam4_precoder_enable_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t gray_en, uint32_t plusd_en);
bf_status_t bf_tof3_serdes_pmd_rx_pam4_precoder_enable_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *gray_en, uint32_t *plusd_en);
bf_status_t bf_tof3_serdes_pmd_remote_loopback_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t remote_loopback_enable);
bf_status_t bf_tof3_serdes_pmd_remote_loopback_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *remote_loopback_enable);
bf_status_t bf_tof3_serdes_pmd_analog_loopback_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t analog_loopback_enable);
bf_status_t bf_tof3_serdes_pmd_analog_loopback_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *analog_loopback_enable);
bf_status_t bf_tof3_serdes_pmd_tx_polarity_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t tx_pol_flip);
bf_status_t bf_tof3_serdes_pmd_tx_polarity_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *tx_pol_flip);
bf_status_t bf_tof3_serdes_pmd_rx_polarity_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t rx_pol_flip);
bf_status_t bf_tof3_serdes_pmd_rx_polarity_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *rx_pol_flip);
bf_status_t bf_tof3_serdes_pmd_tx_hbridge_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t msb, uint32_t lsb);
bf_status_t bf_tof3_serdes_pmd_tx_hbridge_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *msb, uint32_t *lsb);
bf_status_t bf_tof3_serdes_pmd_rx_dfe_adapt_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t dfe_adapt_enable);
bf_status_t bf_tof3_serdes_pmd_rx_dfe_adapt_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *dfe_adapt_enable);
bf_status_t bf_tof3_serdes_pmd_rx_ctle_adapt_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ctle_adapt_enable, uint32_t ctle_boost_a);
bf_status_t bf_tof3_serdes_pmd_rx_ctle_adapt_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *ctle_adapt_enable, uint32_t *ctle_boost_a);
bf_status_t bf_tof3_serdes_pmd_rx_background_adapt_enable_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t rx_bkgrnd_adapt_enable);
bf_status_t bf_tof3_serdes_pmd_rx_background_adapt_enable_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *rx_bkgrnd_adapt_enable);
bf_status_t bf_tof3_serdes_pmd_rx_autoeq_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t rx_autoeq_enable);
bf_status_t bf_tof3_serdes_pmd_rx_autoeq_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *rx_autoeq_enable);
bf_status_t bf_tof3_serdes_pmd_rx_signal_detect_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *signal_detect);
bf_status_t bf_tof3_serdes_pmd_rx_signal_detect_check(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t signal_detect_expected);
bf_status_t bf_tof3_serdes_pmd_tx_ppm_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t timing_window, uint32_t timeout_us, double *tx_ppm);
bf_status_t bf_tof3_serdes_pmd_rx_ppm_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t timing_window, uint32_t timeout_us, double *rx_ppm);
bf_status_t bf_tof3_serdes_pmd_rx_lock_status_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *pmd_rx_lock);
bf_status_t bf_tof3_serdes_pmd_rx_dcdiq_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_dcdiq_data_t *rx_dcdiq_data);
bf_status_t bf_tof3_serdes_pmd_tx_dcdiq_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_dcdiq_data_t *tx_dcdiq_data);
bf_status_t bf_tof3_serdes_pmd_rx_afe_get (bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_afe_data_t *rx_afe_data);
bf_status_t bf_tof3_serdes_pmd_rx_chk_config_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_bist_pattern_t pattern, aw_bist_mode_t mode, uint64_t udp, uint32_t lock_thresh, uint32_t timer_thresh);
bf_status_t bf_tof3_serdes_pmd_rx_chk_config_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_bist_pattern_t *pattern, aw_bist_mode_t *mode, uint64_t *udp, uint32_t *lock_thresh, uint32_t *timer_thresh);
bf_status_t bf_tof3_serdes_pmd_rx_chk_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t enable);
bf_status_t bf_tof3_serdes_pmd_rx_chk_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *enable);
bf_status_t bf_tof3_serdes_pmd_rx_chk_lock_state_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *rx_bist_lock);
bf_status_t bf_tof3_serdes_pmd_rx_chk_err_count_state_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint64_t *err_count, uint32_t *err_count_done, uint32_t *err_count_overflown);
bf_status_t bf_tof3_serdes_pmd_rx_chk_err_count_state_clear(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_tof3_serdes_pmd_tx_gen_config_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_bist_pattern_t pattern, uint64_t user_defined_pattern);
bf_status_t bf_tof3_serdes_pmd_tx_gen_config_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_bist_pattern_t *pattern, uint64_t *user_defined_pattern);
bf_status_t bf_tof3_serdes_pmd_tx_gen_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t enable);
bf_status_t bf_tof3_serdes_pmd_tx_gen_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *enable);
bf_status_t bf_tof3_serdes_pmd_tx_gen_err_inject_config_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint64_t err_pattern, uint32_t err_rate);
bf_status_t bf_tof3_serdes_pmd_tx_gen_err_inject_config_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint64_t *err_pattern, uint32_t *err_rate);
bf_status_t bf_tof3_serdes_pmd_tx_gen_err_inject_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t enable);
bf_status_t bf_tof3_serdes_pmd_tx_gen_err_inject_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *enable);
bf_status_t bf_tof3_serdes_pmd_rx_sweep_demapper(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t npam4_nrz, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_uc_ucode_load(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t (*ucode_arr)[2], uint32_t size);
bf_status_t bf_tof3_serdes_pmd_uc_diag_reg_dump(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_uc_diag_regs_t *uc_diag);
bf_status_t bf_tof3_serdes_pmd_uc_diag_logging_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t uc_log_cmn_en, uint32_t uc_log_tx_en, uint32_t uc_log_rx_en);
bf_status_t bf_tof3_serdes_pmd_uc_diag_logging_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *uc_log_cmn_en, uint32_t *uc_log_tx_en, uint32_t *uc_log_rx_en);
bf_status_t bf_tof3_serdes_pmd_iso_cmn_pstate_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_cmn_pstate_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_cmn_state_req_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_cmn_state_ack_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *cmn_state_ack);
bf_status_t bf_tof3_serdes_pmd_iso_tx_reset_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_reset_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_reset_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_reset_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_rate_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_rate_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_rate_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_rate_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_pstate_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_pstate_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_pstate_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_pstate_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_width_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_width_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_width_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_width_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_state_req_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_rx_state_req_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_iso_tx_state_ack_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *tx_state_ack);
bf_status_t bf_tof3_serdes_pmd_iso_rx_state_ack_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *rx_state_ack);
bf_status_t bf_tof3_serdes_pmd_isolate_cmn_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_isolate_cmn_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *en);
bf_status_t bf_tof3_serdes_pmd_isolate_lane_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_isolate_lane_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *en);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l_hsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l0_lsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l1_lsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r_hsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r0_lsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r1_lsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t sel);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l_hsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l0_lsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_r2l1_lsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r_hsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r0_lsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_l2r1_lsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *sel);
bf_status_t bf_tof3_serdes_pmd_cmn_lsref_sel_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ref_sel);
bf_status_t bf_tof3_serdes_pmd_cmn_lsref_sel_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *ref_sel);
bf_status_t bf_tof3_serdes_pmd_gen_tx_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_gen_tx_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_rx_error_cnt_done_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *err_count_done);
bf_status_t bf_tof3_serdes_pmd_iso_request_cmn_state_change(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_cmn_pstate_t cmn_pstate, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_iso_request_tx_state_change(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_pstate_t tx_pstate, uint32_t tx_rate, uint32_t tx_width, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_iso_request_rx_state_change(bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_pstate_t rx_pstate, uint32_t rx_rate, uint32_t rx_width, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_rx_check_cdr_lock(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_rx_check_bist (bf_dev_id_t dev_id, bf_dev_port_t dev_port, aw_bist_mode_t bist_mode, uint32_t timer_threshold, uint32_t rx_width, uint32_t timeout_us, int32_t expected_errors);
bf_status_t bf_tof3_serdes_pmd_eqeval_type_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t eq_type);
bf_status_t bf_tof3_serdes_pmd_eqeval_req_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_eqeval_ack_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *eqeval_ack );
bf_status_t bf_tof3_serdes_pmd_eqeval_incdec_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * incdec);
bf_status_t bf_tof3_serdes_pmd_rx_equalize(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t eq_type, uint32_t timeout_us );
bf_status_t bf_tof3_serdes_pmd_tx_rxdet_req_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_tx_rxdet(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t rxdet_expected, uint32_t timeout_us);
bf_status_t bf_tof3_serdes_pmd_tx_beacon_en_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t value);
bf_status_t bf_tof3_serdes_pmd_tx_beacon_en_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t *value);
bf_status_t bf_tof3_serdes_pmd_tx_pll_fine_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * tx_pll_fine_code, uint32_t center_code, int tolerance);
bf_status_t bf_tof3_serdes_pmd_tx_pll_coarse_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * tx_pll_coarse_code);
bf_status_t bf_tof3_serdes_pmd_rx_pll_fine_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * rx_pll_fine_code, uint32_t center_code, int tolerance);
bf_status_t bf_tof3_serdes_pmd_rx_pll_coarse_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * rx_pll_coarse_code);
bf_status_t bf_tof3_serdes_pmd_cmn_pll_fine_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * cmn_pll_fine_code, uint32_t center_code, int tolerance);
bf_status_t bf_tof3_serdes_pmd_cmn_pll_coarse_code_get(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t * cmn_pll_coarse_code);
bf_status_t bf_tof3_serdes_pmd_measure_pmon(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t pmon_sel, uint32_t pvt_measure_timing_window, uint32_t timeout_us, uint32_t * pvt_measure_result);
bf_status_t bf_tof3_serdes_pmd_atest_en(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t en);
bf_status_t bf_tof3_serdes_pmd_atest_cmn_capture(bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t atest_addr, uint32_t * atest_adc_val);
bf_status_t bf_tof3_serdes_pmd_read_status(bf_dev_id_t dev_id, bf_dev_port_t dev_port, int branch);
#endif // BF_TOF3_SERDES_IF_INCLUDED
