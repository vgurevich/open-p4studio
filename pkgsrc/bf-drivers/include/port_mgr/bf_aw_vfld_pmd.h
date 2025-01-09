#ifndef BF_AW_VFLD_PMD_INCLUDED
#define BF_AW_VFLD_PMD_INCLUDED

#include <stdint.h>
#include <bf_types/bf_types.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplusa
extern "C" {
#endif
bf_status_t bf_aw_pmd_vfld_lsref_bypass_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t val);
bf_status_t bf_aw_pmd_vfld_lsref_bypass_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *val);
bf_status_t bf_aw_pmd_vfld_cmn_sris_enable_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t val);
bf_status_t bf_aw_pmd_vfld_cmn_sris_enable_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *val);
bf_status_t bf_aw_pmd_vfld_fast_sram_clk_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t val);
bf_status_t bf_aw_pmd_vfld_fast_sram_clk_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *val);
bf_status_t bf_aw_pmd_vfld_tx_spare_0_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t val);
bf_status_t bf_aw_pmd_vfld_tx_spare_0_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *val);
bf_status_t bf_aw_pmd_vfld_tx_spare_1_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t val);
bf_status_t bf_aw_pmd_vfld_tx_spare_1_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq0_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq0_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq1_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq1_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq2_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq2_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq3_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq3_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq4_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq4_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nyq5_set(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nyq5_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t ln,
                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_cm1c1_takeover_ratio_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_cm1c1_takeover_ratio_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_c0_takeover_code_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t val);
bf_status_t bf_aw_pmd_vfld_c0_takeover_code_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqbk_counter_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqbk_counter_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *val);
bf_status_t bf_aw_pmd_vfld_temp1_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t val);
bf_status_t bf_aw_pmd_vfld_temp1_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *val);
bf_status_t bf_aw_pmd_vfld_temp2_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t val);
bf_status_t bf_aw_pmd_vfld_temp2_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t ln,
                                     uint32_t *val);
bf_status_t bf_aw_pmd_vfld_sigdet_offset_cal_valid_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t val);
bf_status_t bf_aw_pmd_vfld_sigdet_offset_cal_valid_get(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t *val);
bf_status_t bf_aw_pmd_vfld_sigdet_offset_cal_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t val);
bf_status_t bf_aw_pmd_vfld_sigdet_offset_cal_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t *val);
bf_status_t bf_aw_pmd_vfld_fg_done_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t val);
bf_status_t bf_aw_pmd_vfld_fg_done_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       uint32_t ln,
                                       uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqbk_first_iter_done_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqbk_first_iter_done_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_post1_npre1_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t val);
bf_status_t bf_aw_pmd_vfld_post1_npre1_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *val);
bf_status_t bf_aw_pmd_vfld_linkeval_state_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_linkeval_state_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_c0_dec_counter_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_c0_dec_counter_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_cm1_inc_counter_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t val);
bf_status_t bf_aw_pmd_vfld_cm1_inc_counter_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *val);
bf_status_t bf_aw_pmd_vfld_c1_inc_counter_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_c1_inc_counter_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_c0_iter_remain_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_c0_iter_remain_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_channel_type_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t val);
bf_status_t bf_aw_pmd_vfld_channel_type_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *val);
bf_status_t bf_aw_pmd_vfld_autoeq_disable_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_autoeq_disable_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqstore_valid_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqstore_valid_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *val);
bf_status_t bf_aw_pmd_vfld_disable_vga_cap_adapt_set(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln,
                                                     uint32_t val);
bf_status_t bf_aw_pmd_vfld_disable_vga_cap_adapt_get(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln,
                                                     uint32_t *val);
bf_status_t bf_aw_pmd_vfld_disable_ctle_adapt_set(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t val);
bf_status_t bf_aw_pmd_vfld_disable_ctle_adapt_get(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t ln,
                                                  uint32_t *val);
bf_status_t bf_aw_pmd_vfld_disable_c0_adapt_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t val);
bf_status_t bf_aw_pmd_vfld_disable_c0_adapt_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *val);
bf_status_t bf_aw_pmd_vfld_disable_cm1c1_adapt_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t val);
bf_status_t bf_aw_pmd_vfld_disable_cm1c1_adapt_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_vga_cap_takeover_set(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_vga_cap_takeover_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_ctle_takeover_set(bf_dev_id_t dev_id,
                                                        bf_dev_port_t dev_port,
                                                        uint32_t ln,
                                                        uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_ctle_takeover_get(bf_dev_id_t dev_id,
                                                        bf_dev_port_t dev_port,
                                                        uint32_t ln,
                                                        uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_c0_takeover_set(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_c0_takeover_get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_cm1c1_dz_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_cm1c1_dz_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_cdr_offset_set(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln,
                                                     uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_cdr_offset_get(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint32_t ln,
                                                     uint32_t *val);
bf_status_t bf_aw_pmd_vfld_skip_wait_lt_done_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t val);
bf_status_t bf_aw_pmd_vfld_skip_wait_lt_done_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t *val);
bf_status_t bf_aw_pmd_vfld_target_cma_bypass_set(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t val);
bf_status_t bf_aw_pmd_vfld_target_cma_bypass_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t ln,
                                                 uint32_t *val);
bf_status_t bf_aw_pmd_vfld_zero_small_taps_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t val);
bf_status_t bf_aw_pmd_vfld_zero_small_taps_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nes_mode_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t val);
bf_status_t bf_aw_pmd_vfld_nes_mode_get(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        uint32_t ln,
                                        uint32_t *val);
bf_status_t bf_aw_pmd_vfld_pick_c162_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t val);
bf_status_t bf_aw_pmd_vfld_pick_c162_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *val);
bf_status_t bf_aw_pmd_vfld_vga_cap_takeover_ratio_set(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t val);
bf_status_t bf_aw_pmd_vfld_vga_cap_takeover_ratio_get(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      uint32_t ln,
                                                      uint32_t *val);
bf_status_t bf_aw_pmd_vfld_ctle_takeover_ratio_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t val);
bf_status_t bf_aw_pmd_vfld_ctle_takeover_ratio_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *val);
bf_status_t bf_aw_pmd_vfld_dz_pre1_fw_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t val);
bf_status_t bf_aw_pmd_vfld_dz_pre1_fw_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          uint32_t ln,
                                          uint32_t *val);
bf_status_t bf_aw_pmd_vfld_dz_post1_fw_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t val);
bf_status_t bf_aw_pmd_vfld_dz_post1_fw_get(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t ln,
                                           uint32_t *val);
bf_status_t bf_aw_pmd_vfld_cdr_offset_cfg_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_cdr_offset_cfg_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_rxeq_prbs_set(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t val);
bf_status_t bf_aw_pmd_vfld_rxeq_prbs_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t ln,
                                         uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqbk_skip_delay_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqbk_skip_delay_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqbk_hold_req_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqbk_hold_req_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *val);
bf_status_t bf_aw_pmd_vfld_eqbk_hold_ack_set(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t val);
bf_status_t bf_aw_pmd_vfld_eqbk_hold_ack_get(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t ln,
                                             uint32_t *val);
bf_status_t bf_aw_pmd_vfld_ffe_tap_disable_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t val);
bf_status_t bf_aw_pmd_vfld_ffe_tap_disable_get(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint32_t ln,
                                               uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_ffe_tap_disable_set(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_ffe_tap_disable_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t *val);
bf_status_t bf_aw_pmd_vfld_rx_sris_enable_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_rx_sris_enable_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_enable_roaming_adapt_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_enable_roaming_adapt_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_custom_roaming_windows_set(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_custom_roaming_windows_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln, uint32_t *val);
bf_status_t bf_aw_pmd_vfld_first_fom_done_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t val);
bf_status_t bf_aw_pmd_vfld_first_fom_done_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t ln,
                                              uint32_t *val);
bf_status_t bf_aw_pmd_vfld_roaming_windows_cfg_set(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t val);
bf_status_t bf_aw_pmd_vfld_roaming_windows_cfg_get(bf_dev_id_t dev_id,
                                                   bf_dev_port_t dev_port,
                                                   uint32_t ln,
                                                   uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_reduced_taps_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_reduced_taps_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *val);
bf_status_t bf_aw_pmd_vfld_reduced_taps_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t val);
bf_status_t bf_aw_pmd_vfld_reduced_taps_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t ln,
                                            uint32_t *val);
bf_status_t bf_aw_pmd_vfld_use_auto_lookup_dfe_ratio_set(bf_dev_id_t dev_id,
                                                         bf_dev_port_t dev_port,
                                                         uint32_t ln,
                                                         uint32_t val);
bf_status_t bf_aw_pmd_vfld_use_auto_lookup_dfe_ratio_get(bf_dev_id_t dev_id,
                                                         bf_dev_port_t dev_port,
                                                         uint32_t ln,
                                                         uint32_t *val);
bf_status_t bf_aw_pmd_vfld_enable_dfe_ratio_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t val);
bf_status_t bf_aw_pmd_vfld_enable_dfe_ratio_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *val);
bf_status_t bf_aw_pmd_vfld_nes_dfe_bypass_value_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t val);
bf_status_t bf_aw_pmd_vfld_nes_dfe_bypass_value_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln,
                                                    uint32_t *val);
bf_status_t bf_aw_pmd_vfld_target_cma_bypass_value_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t val);
bf_status_t bf_aw_pmd_vfld_target_cma_bypass_value_get(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       uint32_t ln,
                                                       uint32_t *val);
bf_status_t bf_aw_pmd_vfld_custom_dfe_ratio_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t val);
bf_status_t bf_aw_pmd_vfld_custom_dfe_ratio_get(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                uint32_t ln,
                                                uint32_t *val);
#ifdef __cplusplus
}
#endif  /* C++ */
#endif  // BF_AW_VFLD_PMD_INCLUDED
