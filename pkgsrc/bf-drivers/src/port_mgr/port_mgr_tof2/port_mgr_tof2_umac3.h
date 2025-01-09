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


#ifndef port_mgr_tof2_umac3_h
#define port_mgr_tof2_umac3_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************
  glbl.chXmode
***************************************************************/
typedef enum {
  UMAC3_MODE_RESET = 0,  // Disabled (Channel held in reset)
  UMAC3_MODE_1GBASE_X = 3,
  UMAC3_MODE_10GBASE_R = 4,
  UMAC3_MODE_25GBASE_R1 = 6,
  UMAC3_MODE_40GBASE_R4 = 7,
  UMAC3_MODE_50GBASE_R2 = 8,
  UMAC3_MODE_100GBASE_R4 = 9,
} umac3_mode_t;

/**************************************************************
  hsmcpcs fec mode
***************************************************************/
typedef enum {
  UMAC3_FEC_NONE = 0,  // no FEC
  UMAC3_FEC_FC = 1,    // base-r (Firecode) FEC
  UMAC3_FEC_RS = 2,    // Reed-Solomon FEC
} umac3_fec_mode_t;

/**************************************************************
  hsmcpcs fec mode
***************************************************************/
typedef enum {
  UMAC3_PMA_BASE_R1 = 3,
  UMAC3_PMA_BASE_R2 = 4,
  UMAC3_PMA_BASE_R4 = 5,
  UMAC3_PMA_MLG = 15,
} umac3_pma_mode_t;

void port_mgr_tof2_umac3_init(bf_dev_id_t dev_id,
                              uint32_t umac3,
                              uint32_t clk_div);

void port_mgr_tof2_umac3_config(bf_dev_id_t dev_id,
                                uint32_t umac3,
                                uint32_t ch,
                                bf_port_speed_t speed,
                                bf_fec_types_t fec);

void port_mgr_tof2_umac3_de_config(bf_dev_id_t dev_id,
                                   uint32_t umac3,
                                   uint32_t ch);

void port_mgr_tof2_umac3_enable(bf_dev_id_t dev_id,
                                uint32_t umac3,
                                uint32_t ch);

void port_mgr_tof2_umac3_disable(bf_dev_id_t dev_id,
                                 uint32_t umac3,
                                 uint32_t ch);

void port_mgr_tof2_umac3_link_state_get(bf_dev_id_t dev_id,
                                        uint32_t umac3,
                                        uint32_t ch,
                                        bool *up);
void port_mgr_tof2_umac3_link_fault_get(bf_dev_id_t dev_id,
                                        uint32_t umac,
                                        uint32_t ch,
                                        bool *pcs_ready,
                                        bool *local_fault,
                                        bool *remote_fault);
void port_mgr_tof2_umac3_get_pcs_status(bf_dev_id_t dev_id,
                                        int umac3,
                                        int ch,
                                        bool *pcs_status,
                                        uint32_t *block_lock_per_pcs_lane,
                                        uint32_t *alig_marker_lock_per_pcs_lane,
                                        bool *hi_ber,
                                        bool *block_lock_all,
                                        bool *alignment_marker_lock_all);
void port_mgr_tof2_umac3_sw_reset_tx_set(bf_dev_id_t dev_id,
                                         uint32_t umac3,
                                         uint32_t ch,
                                         bool assert_reset);

void port_mgr_tof2_umac3_sw_reset_rx_set(bf_dev_id_t dev_id,
                                         uint32_t umac3,
                                         uint32_t ch,
                                         bool assert_reset);

bf_status_t port_mgr_tof2_umac3_status_get(bf_dev_id_t dev_id,
                                           uint32_t umac3,
                                           uint32_t ch,
                                           uint32_t *reg32,
                                           uint32_t *ch0_link_sts,
                                           uint32_t *ch1_link_sts,
                                           uint32_t *ch2_link_sts,
                                           uint32_t *ch3_link_sts,
                                           uint32_t *ch0_rx_fault,
                                           uint32_t *ch1_rx_fault,
                                           uint32_t *ch2_rx_fault,
                                           uint32_t *ch3_rx_fault,
                                           uint32_t *ch0_sig_ok,
                                           uint32_t *ch1_sig_ok,
                                           uint32_t *ch2_sig_ok,
                                           uint32_t *ch3_sig_ok,
                                           uint32_t *ch0_tx_idle,
                                           uint32_t *ch1_tx_idle,
                                           uint32_t *ch2_tx_idle,
                                           uint32_t *ch3_tx_idle);

uint32_t port_mgr_tof2_umac3_num_chnls_for_speed_calc(bf_port_speed_t speed);

bf_status_t port_mgr_tof2_umac3_read_counter(bf_dev_id_t dev_id,
                                             uint32_t umac3,
                                             uint32_t ch,
                                             bf_rmon_counter_t ctr_id,
                                             uint64_t *ctr_value);
bf_status_t port_mgr_tof2_umac3_clear_counter(bf_dev_id_t dev_id,
                                              uint32_t umac3,
                                              uint32_t ch);
void port_mgr_tof2_umac3_lane_map_set(bf_dev_id_t dev_id,
                                      uint32_t umac3,
                                      uint32_t phys_tx_ln[8],
                                      uint32_t phys_rx_ln[8]);
void port_mgr_tof2_umac3_force_sigok_low_set(bf_dev_id_t dev_id,
                                             uint32_t umac3,
                                             uint32_t ch,
                                             uint32_t n_ch);
void port_mgr_tof2_umac3_force_sigok_hi_set(bf_dev_id_t dev_id,
                                            uint32_t umac3,
                                            uint32_t ch,
                                            uint32_t n_ch);
void port_mgr_tof2_umac3_clear_forced_sigok_set(bf_dev_id_t dev_id,
                                                uint32_t umac3,
                                                uint32_t ch,
                                                uint32_t n_ch);
void port_mgr_tof2_umac3_loopback_set(bf_dev_id_t dev_id,
                                      uint32_t umac3,
                                      uint32_t ch,
                                      bf_loopback_mode_e mode);
void port_mgr_tof2_umac3_forced_sigok_get(bf_dev_id_t dev_id,
                                          uint32_t umac3,
                                          uint32_t ch,
                                          uint32_t n_lanes,
                                          uint32_t *force_hi_raw_val,
                                          uint32_t *force_lo_raw_val,
                                          uint32_t *force_hi,
                                          uint32_t *force_lo);
void port_mgr_tof2_umac3_rs_fec_status_and_counters_get(
    bf_dev_id_t dev_id,
    uint32_t umac3,
    uint32_t ch,
    bool *hi_ser,
    bool *fec_align_status,
    uint32_t *fec_corr_cnt,
    uint32_t *fec_uncorr_cnt,
    uint32_t *fec_ser_lane_0,
    uint32_t *fec_ser_lane_1,
    uint32_t *fec_ser_lane_2,
    uint32_t *fec_ser_lane_3);
void port_mgr_tof2_umac3_int_dis_all_set(bf_dev_id_t dev_id, uint32_t umac3);
void port_mgr_tof2_umac3_int_en_set(bf_dev_id_t dev_id,
                                    uint32_t umac3,
                                    bool on);
void port_mgr_tof2_umac3_local_fault_int_en_set(bf_dev_id_t dev_id,
                                                uint32_t umac3,
                                                uint32_t ch,
                                                bool en);
void port_mgr_tof2_umac3_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                 uint32_t umac3,
                                                 uint32_t ch,
                                                 bool en);
void port_mgr_tof2_umac3_tx_local_fault_set(bf_dev_id_t dev_id,
                                            uint32_t umac3,
                                            uint32_t ch,
                                            bool on);
void port_mgr_tof2_umac3_tx_remote_fault_set(bf_dev_id_t dev_id,
                                             uint32_t umac3,
                                             uint32_t ch,
                                             bool on);
void port_mgr_tof2_umac3_tx_idle_set(bf_dev_id_t dev_id,
                                     uint32_t umac3,
                                     uint32_t ch,
                                     bool on);
void port_mgr_tof2_umac3_rx_enable_set(bf_dev_id_t dev_id,
                                       uint32_t umac3,
                                       uint32_t ch,
                                       bool rx_en);
void port_mgr_tof2_umac3_txdrain_set(bf_dev_id_t dev_id,
                                     uint32_t umac3,
                                     uint32_t ch,
                                     bool en);
void port_mgr_tof2_umac3_flowcontrol_config_set(bf_dev_id_t dev_id,
                                                uint32_t umac3,
                                                uint32_t ch);
void port_mgr_tof2_umac3_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                          uint32_t umac3,
                                          uint32_t ch,
                                          bool en);
uint32_t port_mgr_tof2_umac3_config_get(bf_dev_id_t dev_id,
                                        uint32_t umac3,
                                        uint32_t ch);
void port_mgr_tof2_umac3_channel_add(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch,
                                     uint32_t n_ch);
bf_status_t port_mgr_tof2_umac3_handle_interrupts(bf_dev_id_t dev_id,
                                                  uint32_t umac3,
                                                  bool *possible_state_chg);
void port_mgr_tof2_umac3_get_1588_timestamp_tx(bf_dev_id_t dev_id,
                                               uint32_t mac_blk,
                                               uint32_t ch,
                                               uint64_t *ts,
                                               bool *ts_valid,
                                               int *ts_id);
void port_mgr_tof2_umac3_set_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint32_t ch,
                                                     uint16_t delta);
void port_mgr_tof2_umac3_get_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint32_t ch,
                                                     uint16_t *delta);
void port_mgr_tof2_umac3_set_1588_timestamp_delta_rx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint32_t ch,
                                                     uint16_t delta);
void port_mgr_tof2_umac3_get_1588_timestamp_delta_rx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint32_t ch,
                                                     uint16_t *delta);
#ifdef __cplusplus
}
#endif /* C++ */

#endif
