#ifndef BF_AW_PMD_INCLUDED
#define BF_AW_PMD_INCLUDED

#include <stdint.h>
#include <bf_types/bf_types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplusa
extern "C" {
#endif
bf_status_t bf_aw_pmd_cmn_clkgen_refdiv_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t div);
bf_status_t bf_aw_pmd_cmn_clkgen_refdiv_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *div);
bf_status_t bf_aw_pmd_anlt_logical_lane_num_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t logical_lane,
                                                uint32_t an_no_attached);
bf_status_t bf_aw_pmd_anlt_logical_lane_num_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *logical_lane,
                                                uint32_t *an_no_attached);
bf_status_t bf_aw_pmd_anlt_auto_neg_adv_ability_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *adv_ability,
                                                    uint32_t *fec_ability,
                                                    uint32_t nonce);
bf_status_t bf_aw_pmd_anlt_auto_neg_adv_ability_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *adv_ability,
                                                    uint32_t *fec_ability);
bf_status_t bf_aw_pmd_anlt_auto_neg_config_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t status_check_disable,
                                               uint32_t next_page_en,
                                               uint32_t an_no_nonce_check);
bf_status_t bf_aw_pmd_anlt_auto_neg_config_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *status_check_disable,
                                               uint32_t *next_page_en,
                                               uint32_t *an_no_nonce_check);
bf_status_t bf_aw_pmd_anlt_auto_neg_start_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t start);
bf_status_t bf_aw_pmd_anlt_auto_neg_start_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *start);
bf_status_t bf_aw_pmd_anlt_auto_neg_status_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *link_good);
bf_status_t bf_aw_pmd_anlt_auto_neg_result_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *an_result);
bf_status_t bf_aw_pmd_anlt_auto_neg_page_rx_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *an_mr_page_rx,
                                                uint64_t *an_rx_link_code_word);
bf_status_t bf_aw_pmd_anlt_auto_neg_next_page_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint64_t an_tx_np);
bf_status_t bf_aw_pmd_anlt_auto_neg_newdef_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               aw_an_spec_t *newdef_get);
bf_status_t bf_aw_pmd_anlt_auto_neg_rs_fec_int_ena_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t *an_rs_fec_int_ena);
bf_status_t bf_aw_pmd_anlt_auto_neg_next_page_oui_compare_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t np_expected_oui);
bf_status_t bf_aw_pmd_anlt_link_training_en_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t en);
bf_status_t bf_aw_pmd_anlt_link_training_en_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *en);
bf_status_t bf_aw_pmd_anlt_link_training_config_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t width,
                                                    uint32_t clause);
bf_status_t bf_aw_pmd_anlt_link_training_config_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *width,
                                                    uint32_t *clause);
bf_status_t bf_aw_pmd_anlt_link_training_prbs_seed_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t clause,
                                                       uint32_t logical_lane);
bf_status_t bf_aw_pmd_anlt_link_training_start_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t start);
bf_status_t bf_aw_pmd_anlt_link_training_start_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *start);
bf_status_t bf_aw_pmd_anlt_link_training_status_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t *lt_running,
    uint32_t *lt_done,
    uint32_t *lt_training_failure,
    uint32_t *lt_rx_ready);
bf_status_t bf_aw_pmd_anlt_link_training_timeout_enable_set(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t enable);
bf_status_t bf_aw_pmd_ctrl_map_en_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t anlt_ctrl_map_en);
bf_status_t bf_aw_pmd_refclk_termination_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    aw_refclk_term_mode_t lsrefbuf_term_mode);
bf_status_t bf_aw_pmd_refclk_termination_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    aw_refclk_term_mode_t *lsrefbuf_term_mode);
bf_status_t bf_aw_pmd_rx_termination_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         aw_acc_term_mode_t acc_term_mode);
bf_status_t bf_aw_pmd_rx_termination_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         aw_acc_term_mode_t *acc_term_mode);
bf_status_t bf_aw_pmd_force_signal_detect_config_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    aw_force_sigdet_mode_t sigdet_mode);
bf_status_t bf_aw_pmd_force_signal_detect_config_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    aw_force_sigdet_mode_t *sigdet_mode);
bf_status_t bf_aw_pmd_tx_disable_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t tx_disable);
bf_status_t bf_aw_pmd_tx_disable_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *tx_disable);
bf_status_t bf_aw_pmd_txfir_config_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       aw_txfir_config_t txfir_cfg,
                                       uint32_t fir_ovr_enable);
bf_status_t bf_aw_pmd_txfir_config_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       aw_txfir_config_t *txfir_cfg);
bf_status_t bf_aw_pmd_tx_tap_mode_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *max_rng_cm3,
                                      uint32_t *max_rng_cm2,
                                      uint32_t *max_rng_cm1,
                                      uint32_t *max_rng_c1,
                                      uint32_t *max_rng_c0);
bf_status_t bf_aw_pmd_tx_pam4_precoder_override_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t en);
bf_status_t bf_aw_pmd_tx_pam4_precoder_override_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *en);
bf_status_t bf_aw_pmd_tx_pam4_precoder_enable_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t gray_en,
                                                  uint32_t plusd_en);
bf_status_t bf_aw_pmd_tx_pam4_precoder_enable_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t *gray_en,
                                                  uint32_t *plusd_en);
bf_status_t bf_aw_pmd_rx_pam4_precoder_override_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t en);
bf_status_t bf_aw_pmd_rx_pam4_precoder_override_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *en);
bf_status_t bf_aw_pmd_rx_pam4_precoder_enable_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t gray_en,
                                                  uint32_t plusd_en);
bf_status_t bf_aw_pmd_rx_pam4_precoder_enable_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t *gray_en,
                                                  uint32_t *plusd_en);
bf_status_t bf_aw_pmd_remote_loopback_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t remote_loopback_enable);
bf_status_t bf_aw_pmd_remote_loopback_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *remote_loopback_enable);
bf_status_t bf_aw_pmd_fep_data_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t datapath_en);
bf_status_t bf_aw_pmd_fep_data_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t *datapath_en);
bf_status_t bf_aw_pmd_tx_dcd_iq_cal(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t enable_d);
bf_status_t bf_aw_pmd_fep_clock_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint8_t clock_en);
bf_status_t bf_aw_pmd_tx_postdiv_loopback_ena_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint8_t postdiv_loopback_ena);
bf_status_t bf_aw_pmd_tx_postdiv_loopback_ena_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint8_t *postdiv_loopback_ena);
bf_status_t bf_aw_pmd_fep_clock_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *clock_en);
bf_status_t bf_aw_pmd_analog_loopback_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t analog_loopback_enable);
bf_status_t bf_aw_pmd_analog_loopback_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *analog_loopback_enable);
bf_status_t bf_aw_pmd_fes_loopback_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t fes_loopback_enable);
bf_status_t bf_aw_pmd_fes_loopback_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *fes_loopback_enable);
bf_status_t bf_aw_pmd_nep_loopback_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t nep_loopback_enable);
bf_status_t bf_aw_pmd_nep_loopback_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *nep_loopback_enable);
bf_status_t bf_aw_pmd_tx_polarity_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t tx_pol_flip);
bf_status_t bf_aw_pmd_tx_polarity_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *tx_pol_flip);
bf_status_t bf_aw_pmd_rx_polarity_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t rx_pol_flip);
bf_status_t bf_aw_pmd_rx_polarity_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *rx_pol_flip);
bf_status_t bf_aw_pmd_tx_hbridge_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     tx_hbridge_t *tx_hbridge_st);
bf_status_t bf_aw_pmd_tx_hbridge_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *msb,
                                     uint32_t *lsb);
bf_status_t bf_aw_pmd_rx_dfe_adapt_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t dfe_adapt_enable);
bf_status_t bf_aw_pmd_rx_dfe_adapt_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *dfe_adapt_enable);
bf_status_t bf_aw_pmd_rx_ctle_adapt_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t ctle_adapt_enable,
                                        uint32_t ctle_boost_a);
bf_status_t bf_aw_pmd_rx_ctle_adapt_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *ctle_adapt_enable,
                                        uint32_t *ctle_boost_a);
bf_status_t bf_aw_pmd_rx_background_adapt_enable_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint8_t rx_background_adapt);
bf_status_t bf_aw_pmd_rx_background_adapt_enable_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t *rx_bkgrnd_adapt_enable);
bf_status_t bf_aw_pmd_rx_autoeq_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t rx_autoeq_enable);
bf_status_t bf_aw_pmd_rx_autoeq_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *rx_autoeq_enable);
bf_status_t bf_aw_pmd_rx_ffe_tap_count_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t tap_count);
bf_status_t bf_aw_pmd_rx_ffe_tap_count_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *tap_count);
bf_status_t bf_aw_pmd_rx_roaming_windows_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             aw_rx_roaming_mode_t mode,
                                             uint8_t window_select1,
                                             uint8_t window_select2);
bf_status_t bf_aw_pmd_rx_roaming_windows_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             aw_rx_roaming_mode_t *mode,
                                             uint8_t *window_select1,
                                             uint8_t *window_select2);
bf_status_t bf_aw_pmd_rx_vga_cap_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t vga_cap);
bf_status_t bf_aw_pmd_rx_vga_cap_adapt_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           vga_opt_t *opts);
bf_status_t bf_aw_pmd_rx_vga_cap_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *vga_cap);
bf_status_t bf_aw_pmd_rx_vga_cap_adapt_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           vga_opt_t *opts);
bf_status_t bf_aw_pmd_rxeq_prbs_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t prbs_en);
bf_status_t bf_aw_pmd_rxeq_prbs_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *prbs_en);
bf_status_t bf_aw_pmd_rx_c0_adapt_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t en);
bf_status_t bf_aw_pmd_rx_cdr_offset_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t use_custom_cdr_offset,
                                        uint32_t cdr_offset,
                                        uint32_t cdr_dir);
bf_status_t bf_aw_pmd_rx_signal_detect_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *signal_detect);
bf_status_t bf_aw_pmd_rx_signal_detect_check(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t signal_detect_expected);
bf_status_t bf_aw_pmd_tx_ppm_get(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t ln,
                                 uint32_t timing_window,
                                 uint32_t timeout_us,
                                 double *tx_ppm,
                                 double *vco_freq,
                                 double refclk_freq);
bf_status_t bf_aw_pmd_rx_ppm_get(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t ln,
                                 uint32_t timing_window,
                                 uint32_t timeout_us,
                                 double *rx_ppm,
                                 double *vco_freq,
                                 double refclk_freq);
bf_status_t bf_aw_pmd_rx_lock_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *pmd_rx_lock);
bf_status_t bf_aw_pmd_rx_dcdiq_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   aw_dcdiq_data_t *rx_dcdiq_data);
bf_status_t bf_aw_pmd_tx_dcdiq_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   aw_dcdiq_data_t *tx_dcdiq_data);
bf_status_t bf_aw_pmd_rx_afe_get(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t ln,
                                 aw_afe_data_t *rx_afe_data);
bf_status_t bf_aw_pmd_rx_invert_datapath(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         char modulation_mode[],
                                         uint32_t gray_code_en,
                                         uint32_t invert_en);
bf_status_t bf_aw_pmd_tx_invert_datapath(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         char modulation_mode[],
                                         uint32_t gray_code_en,
                                         uint32_t invert_en);
bf_status_t bf_aw_pmd_rx_chk_config_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        aw_bist_pattern_t pattern,
                                        aw_bist_mode_t mode,
                                        uint64_t udp,
                                        uint32_t lock_thresh,
                                        uint32_t timer_thresh);
bf_status_t bf_aw_pmd_rx_chk_config_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        aw_bist_pattern_t *pattern,
                                        aw_bist_mode_t *mode,
                                        uint64_t *udp,
                                        uint32_t *lock_thresh,
                                        uint32_t *timer_thresh);
bf_status_t bf_aw_pmd_rx_chk_en_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t enable);
bf_status_t bf_aw_pmd_rx_chk_en_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *enable);
bf_status_t bf_aw_pmd_rx_chk_lock_state_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *rx_bist_lock);
bf_status_t bf_aw_pmd_rx_chk_err_count_state_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint64_t *err_count,
                                                 uint32_t *err_count_done,
                                                 uint32_t *err_count_overflown);
bf_status_t bf_aw_pmd_rx_chk_err_count_state_clear(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln);
bf_status_t bf_aw_pmd_tx_gen_config_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        aw_bist_pattern_t pattern,
                                        uint64_t udp);
bf_status_t bf_aw_pmd_tx_gen_config_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        aw_bist_pattern_t *pattern,
                                        uint64_t *udp);
bf_status_t bf_aw_pmd_tx_gen_en_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t enable);
bf_status_t bf_aw_pmd_tx_gen_en_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *enable);
bf_status_t bf_aw_pmd_tx_gen_err_inject_config_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint64_t err_pattern,
                                                   uint32_t err_rate);
bf_status_t bf_aw_pmd_tx_gen_err_inject_config_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint64_t *err_pattern,
                                                   uint32_t *err_rate);
bf_status_t bf_aw_pmd_tx_gen_err_inject_en_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t enable);
bf_status_t bf_aw_pmd_tx_gen_err_inject_en_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *enable);
bf_status_t bf_aw_pmd_rx_sweep_demapper(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t npam4_nrz,
                                        uint32_t timeout_us);
bf_status_t bf_aw_pmd_gen_tx_swap_msb_lsb_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t en);
bf_status_t bf_aw_pmd_gen_tx_swap_msb_lsb_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *en);
bf_status_t bf_aw_pmd_gen_rx_swap_msb_lsb_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t en);
bf_status_t bf_aw_pmd_gen_rx_swap_msb_lsb_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *en);
bf_status_t bf_aw_pmd_uc_ucode_load(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    aw_ucode_t *ucode_arr,
                                    uint32_t size);
bf_status_t bf_aw_pmd_pll_lock_max_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t val);
bf_status_t bf_aw_pmd_pll_lock_min_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t val);
bf_status_t bf_aw_pmd_pll_lock_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t *pll_lock,
                                   uint32_t check_en,
                                   uint32_t expected_val);
bf_status_t bf_aw_pmd_uc_diag_reg_dump(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       aw_uc_diag_regs_t *uc_diag);
bf_status_t bf_aw_pmd_uc_diag_logging_en_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t uc_log_cmn_en,
                                             uint32_t uc_log_tx_en,
                                             uint32_t uc_log_rx_en);
bf_status_t bf_aw_pmd_uc_diag_logging_en_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *uc_log_cmn_en,
                                             uint32_t *uc_log_tx_en,
                                             uint32_t *uc_log_rx_en);
bf_status_t bf_aw_pmd_iso_ref_ls_en_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t value);
bf_status_t bf_aw_pmd_iso_cmn_pstate_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t value);
bf_status_t bf_aw_pmd_iso_cmn_pstate_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *value);
bf_status_t bf_aw_pmd_iso_cmn_state_req_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t value);
bf_status_t bf_aw_pmd_iso_cmn_state_ack_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *cmn_state_ack);
bf_status_t bf_aw_pmd_iso_tx_reset_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_iso_tx_reset_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *value);
bf_status_t bf_aw_pmd_iso_rx_reset_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_iso_rx_reset_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *value);
bf_status_t bf_aw_pmd_iso_tx_rate_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t value);
bf_status_t bf_aw_pmd_iso_tx_rate_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *value);
bf_status_t bf_aw_pmd_iso_rx_rate_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t value);
bf_status_t bf_aw_pmd_iso_rx_rate_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *value);
bf_status_t bf_aw_pmd_iso_tx_pstate_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t value);
bf_status_t bf_aw_pmd_iso_tx_pstate_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *value);
bf_status_t bf_aw_pmd_iso_rx_pstate_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t value);
bf_status_t bf_aw_pmd_iso_rx_pstate_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *value);
bf_status_t bf_aw_pmd_iso_tx_width_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_iso_tx_width_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *value);
bf_status_t bf_aw_pmd_iso_rx_width_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_iso_rx_width_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *value);
bf_status_t bf_aw_pmd_iso_tx_state_req_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t value);
bf_status_t bf_aw_pmd_iso_rx_state_req_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t value);
bf_status_t bf_aw_pmd_iso_tx_state_ack_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *tx_state_ack);
bf_status_t bf_aw_pmd_iso_rx_state_ack_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *rx_state_ack);
bf_status_t bf_aw_pmd_isolate_cmn_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t en);
bf_status_t bf_aw_pmd_isolate_cmn_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *en);
bf_status_t bf_aw_pmd_isolate_lane_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t en);
bf_status_t bf_aw_pmd_isolate_lane_tx_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t en);
bf_status_t bf_aw_pmd_isolate_lane_rx_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t en);
bf_status_t bf_aw_pmd_isolate_lane_txrx_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t en);
bf_status_t bf_aw_pmd_isolate_lane_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *en);
bf_status_t bf_aw_pmd_cmn_r2l_hsref_sel_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t sel);
bf_status_t bf_aw_pmd_cmn_r2l0_lsref_sel_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t sel);
bf_status_t bf_aw_pmd_cmn_r2l1_lsref_sel_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t sel);
bf_status_t bf_aw_pmd_cmn_l2r_hsref_sel_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t sel);
bf_status_t bf_aw_pmd_cmn_l2r0_lsref_sel_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t sel);
bf_status_t bf_aw_pmd_cmn_l2r1_lsref_sel_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t sel);
bf_status_t bf_aw_pmd_cmn_r2l_hsref_sel_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_r2l0_lsref_sel_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_r2l1_lsref_sel_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_l2r_hsref_sel_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_l2r0_lsref_sel_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_l2r1_lsref_sel_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *sel);
bf_status_t bf_aw_pmd_cmn_lsref_sel_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t ref_sel);
bf_status_t bf_aw_pmd_cmn_lsref_sel_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *ref_sel);
bf_status_t bf_aw_pmd_pcie_cmn_lsref_25m_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t lsref_25m);
bf_status_t bf_aw_pmd_pcie_cmn_lsref_25m_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *lsref_25m);
bf_status_t bf_aw_pmd_gen_tx_en_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t value);
bf_status_t bf_aw_pmd_gen_tx_en_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *value);
bf_status_t bf_aw_pmd_rx_error_cnt_done_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *err_count_done);
bf_status_t bf_aw_pmd_iso_request_cmn_state_change(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   aw_cmn_pstate_t cmn_pstate,
                                                   uint32_t timeout_us);
bf_status_t bf_aw_pmd_iso_request_tx_state_change(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  aw_pstate_t tx_pstate,
                                                  uint32_t tx_rate,
                                                  uint32_t tx_width,
                                                  uint32_t timeout_us);
bf_status_t bf_aw_pmd_iso_request_rx_state_change(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  aw_pstate_t rx_pstate,
                                                  uint32_t rx_rate,
                                                  uint32_t rx_width,
                                                  uint32_t timeout_us);
bf_status_t bf_aw_pmd_rx_check_cdr_lock(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t timeout_us);
bf_status_t bf_aw_pmd_rx_check_bist(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    aw_bist_mode_t bist_mode,
                                    uint32_t timer_threshold,
                                    uint32_t rx_width,
                                    uint32_t timeout_us,
                                    int32_t expected_errors);
bf_status_t bf_aw_pmd_rx_prefec_clear(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln);
bf_status_t bf_aw_pmd_rx_prefec_enable_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *enable);
bf_status_t bf_aw_pmd_rx_prefec_enable_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t enable);
bf_status_t bf_aw_pmd_rx_prefec_poll(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t timeout_us);
bf_status_t bf_aw_pmd_rx_prefec_config_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *corr_num_syms,
                                           uint32_t *symbol_size,
                                           uint32_t *wall_mode,
                                           uint32_t *sym_per_cw,
                                           uint32_t *skip_syms,
                                           uint32_t *timer_num_cw);
bf_status_t bf_aw_pmd_rx_prefec_config_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t corr_num_syms,
                                           uint32_t symbol_size,
                                           uint32_t wall_mode,
                                           uint32_t sym_per_cw,
                                           uint32_t skip_syms,
                                           uint32_t timer_num_cw);
bf_status_t bf_aw_pmd_rx_prefec_get_results(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *hist);
bf_status_t bf_aw_pmd_eqeval_type_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t eq_type);
bf_status_t bf_aw_pmd_eqeval_req_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t value);
bf_status_t bf_aw_pmd_eqeval_ack_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *eqeval_ack);
bf_status_t bf_aw_pmd_eqeval_incdec_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *incdec);
bf_status_t bf_aw_pmd_rx_equalize(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  aw_eq_type_t eq_type,
                                  uint32_t timeout_us);
bf_status_t bf_aw_pmd_tx_rxdet_req_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_tx_rxdet(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               uint32_t ln,
                               uint32_t rxdet_expected,
                               uint32_t timeout_us);
bf_status_t bf_aw_pmd_tx_beacon_en_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t value);
bf_status_t bf_aw_pmd_tx_beacon_en_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *value);
bf_status_t bf_aw_pmd_tx_pll_fine_code_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *tx_pll_fine_code,
                                           uint32_t center_code,
                                           int tolerance);
bf_status_t bf_aw_pmd_tx_pll_coarse_code_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *tx_pll_coarse_code,
                                             uint32_t center_code,
                                             int tolerance);
bf_status_t bf_aw_pmd_rx_pll_fine_code_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *rx_pll_fine_code,
                                           uint32_t center_code,
                                           int tolerance);
bf_status_t bf_aw_pmd_rx_pll_coarse_code_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *rx_pll_coarse_code,
                                             uint32_t center_code,
                                             int tolerance);
bf_status_t bf_aw_pmd_cmn_pll_fine_code_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *cmn_pll_fine_code,
                                            uint32_t center_code,
                                            int tolerance);
bf_status_t bf_aw_pmd_cmn_pll_coarse_code_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *cmn_pll_coarse_code,
                                              uint32_t center_code,
                                              int tolerance);
bf_status_t bf_aw_pmd_rd_data_pipeline_stages_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t stages);
bf_status_t bf_aw_pmd_rd_data_pipeline_stages_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t *stages);
bf_status_t bf_aw_pmd_measure_pmon(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   uint32_t pmon_sel,
                                   uint32_t pvt_measure_timing_window,
                                   uint32_t timeout_us,
                                   uint32_t *pvt_measure_result);
bf_status_t bf_aw_pmd_atest_en(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               uint32_t ln,
                               uint32_t en);
bf_status_t bf_aw_pmd_atest_cmn_capture(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t atest_addr,
                                        uint32_t atest_term,
                                        uint32_t *atest_adc_val);
bf_status_t bf_aw_pmd_atest_tx_capture(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t atest_addr,
                                       uint32_t atest_term,
                                       uint32_t *atest_adc_val);
bf_status_t bf_aw_pmd_atest_rx_a_capture(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t atest_addr,
                                         uint32_t atest_term,
                                         uint32_t *atest_adc_val);
bf_status_t bf_aw_pmd_atest_rx_b_capture(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t atest_addr,
                                         uint32_t atest_term,
                                         uint32_t *atest_adc_val);
bf_status_t bf_aw_pmd_atest_adc_power(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t en);
bf_status_t bf_aw_pmd_atest_adc_temp_capture(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t iterations,
                                             aw_adc_temp_data_t *mean_data);
bf_status_t bf_aw_pmd_atest_adc_temp_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         aw_adc_temp_calibration_t *calibration,
                                         aw_adc_temp_method_t method,
                                         uint32_t iterations,
                                         aw_adc_temp_data_t *measured_data,
                                         float *result_temp);
bf_status_t bf_aw_pmd_rx_cdr_lock_get(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t ln,
                                      uint32_t *rx_cdr_lock);
bf_status_t bf_aw_pmd_lcpll_vco_counter_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t timing_window,
                                            double *lcpll_ppm,
                                            double *vco_freq,
                                            double refclk_freq);
bf_status_t bf_aw_pmd_rx_cdr_offset_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *use_custom_cdr_offset,
                                        uint32_t *cdr_offset,
                                        uint32_t *cdr_dir);
bf_status_t bf_aw_pmd_read_status(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  int branch);
bf_status_t bf_aw_pmd_pause_background(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t pause_enable);
bf_status_t bf_aw_pmd_tracebuffer_config_enable(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t clk_sel,
                                                uint32_t enable);
bf_status_t bf_aw_pmd_tracebuffer_config_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             aw_pmd_tracebuffer_mode_t *tbmode,
                                             uint32_t *samples_per_cycle);
bf_status_t bf_aw_pmd_tracebuffer_config_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             aw_pmd_tracebuffer_mode_t tbmode,
                                             uint32_t samples_per_cycle);
bf_status_t bf_aw_pmd_read_tracebuffer(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *tb_data,
                                       int tb_size);
bf_status_t bf_aw_pmd_convert_data_signed(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t data);
bf_status_t bf_aw_pmd_read_tracebuffer_adc(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    int num_samples,
    int adc_data[num_samples][AW_NUM_BRANCHES]);
bf_status_t bf_aw_pmd_tbus_client_config_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *block_id,
                                             uint32_t *signal_id,
                                             uint32_t *trigger,
                                             uint32_t *continuous_sample);
bf_status_t bf_aw_pmd_tbus_client_config_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t block_id,
                                             uint32_t signal_id,
                                             uint32_t trigger,
                                             uint32_t continuous_sample);
bf_status_t bf_aw_pmd_read_tracebuffer_demapper(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    int32_t demapper_data[2][AW_TBUS_NUM_SAMPLES],
    uint32_t branch_id,
    uint32_t is_ffe);
bf_status_t bf_aw_pmd_read_tracebuffer_general(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    int32_t tb_data_out[AW_TBUS_NUM_SAMPLES],
    int tbus_block_id,
    uint32_t signal_id,
    int fp_lsb,
    int fp_msb,
    int fp_si);
bf_status_t bf_aw_pmd_read_tracebuffer_ffe(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           int num_samples,
                                           int32_t *ffe_data,
                                           int branch_id);
bf_status_t bf_aw_pmd_read_tracebuffer_itr_dlpf_int(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    int num_samples,
                                                    int32_t *itr_dlpf_int);
bf_status_t bf_aw_pmd_read_tracebuffer_quantizer_err(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln,
                                                     int num_samples,
                                                     int32_t *qztr_err_data,
                                                     int branch_id);
bf_status_t bf_aw_pmd_cmn_ssc_config(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     double lsref_mhz,
                                     double lcpll_mhz,
                                     int ppm_downspread);
bf_status_t bf_aw_pmd_cmn_ssc_en_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t en);
bf_status_t bf_aw_pmd_sram_clk_div_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t en);
bf_status_t bf_aw_pmd_sram_clk_div_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *en);
bf_status_t bf_aw_pmd_rx_burst_mode_config_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t pam_mode,
                                               uint32_t burst_threshold,
                                               uint32_t burst_mode);
bf_status_t bf_aw_pmd_rx_burst_mode_config_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *pam_mode,
                                               uint32_t *burst_threshold,
                                               uint32_t *burst_mode);
bf_status_t bf_aw_pmd_rx_burst_err_cnt_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *burst_err_cnt);
bf_status_t bf_aw_pmd_rx_burst_mode_stats_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t exp_data[],
                                              uint32_t rec_data[],
                                              uint32_t xor_data[]);
bf_status_t bf_aw_pmd_rx_gray_code_mapping_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint8_t *gray_code_map);
bf_status_t bf_aw_pmd_rx_gray_code_mapping_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint8_t gray_code_map);
bf_status_t bf_aw_pmd_tx_gray_code_mapping_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint8_t *gray_code_map);
bf_status_t bf_aw_pmd_tx_gray_code_mapping_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint8_t gray_code_map);
bf_status_t bf_aw_pmd_tx_perf_settings(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint8_t perf_mode);
bf_status_t bf_aw_pmd_cmn_bias_bandgap_force_startup(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln);
bf_status_t bf_aw_pmd_sris_set(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               uint32_t ln,
                               uint32_t en);
bf_status_t bf_aw_pmd_sris_get(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               uint32_t ln,
                               uint32_t *en);
bf_status_t bf_aw_pmd_rx_active_branches_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *active_branches);
bf_status_t bf_aw_pmd_fw_version_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     aw_version_t *version_st);
bf_status_t bf_aw_pmd_rx_dfe_ratio_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t enable,
                                       uint32_t use_auto_lookup_dfe_ratio,
                                       double custom_dfe_ratio);
bf_status_t bf_aw_pmd_rx_dfe_ratio_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *enable,
                                       uint32_t *use_auto_lookup_dfe_ratio,
                                       double *custom_dfe_ratio);
bf_status_t bf_aw_pmd_rd_csr(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             uint32_t ln,
                             uint32_t addr,
                             uint32_t *rdata);
bf_status_t bf_aw_pmd_wr_csr(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             uint32_t ln,
                             uint32_t addr,
                             uint32_t wdata);
bf_status_t bf_aw_pmd_mss_reset(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                uint32_t ln);
bf_status_t bf_aw_pmd_fw_load(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              uint32_t ln,
                              char *path);
bf_status_t bf_aw_pmd_one_time_pgm(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln);
bf_status_t bf_aw_pmd_cur_rate_width_pstate(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *tx_rate,
                                            uint32_t *tx_width,
                                            uint32_t *tx_pstate,
                                            uint32_t *rx_rate,
                                            uint32_t *rx_width,
                                            uint32_t *rx_pstate);
bf_status_t bf_aw_pmd_rxmfsm_eq_check_rxdisable_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t val);
bf_status_t bf_aw_pmd_lt_info_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t ln,
                                  uint32_t *lt_fsm_st,
                                  uint32_t *frame_lock);
bf_status_t bf_aw_pmd_reg_defs_get(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t ln,
                                   aw_reg_defs_t **regs,
                                   aw_fld_defs_t **flds,
                                   char **cmnts,
                                   aw_reg_defs_t **vregs,
                                   aw_fld_defs_t **vflds,
                                   char **vcmnts);
bf_status_t bf_aw_pmd_speed_to_rate_and_width(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t serdes_speed,
                                              bool is_pam4,
                                              uint32_t *rate,
                                              uint32_t *width);
bf_status_t bf_aw_pmd_rx_dig_pwr_det_threshold_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t *adc_valid_thresh_nt,
    uint32_t *adc_invalid_thresh_nt);
bf_status_t bf_aw_pmd_rx_dig_pwr_det_threshold_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t ln,
    uint32_t adc_valid_thresh_nt,
    uint32_t adc_invalid_thresh_nt);
#ifdef __cplusplus
}
#endif  /* C++ */
#endif  // BF_AW_PMD_INCLUDED
