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


#ifndef port_mgr_tof2_umac_h
#define port_mgr_tof2_umac_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

void port_mgr_tof2_umac_init(bf_dev_id_t dev_id, uint32_t clk_div);

void port_mgr_tof2_umac_config(bf_dev_id_t dev_id,
                               uint32_t umac,
                               uint32_t ch,
                               bf_port_speed_t speed,
                               bf_fec_types_t fec,
                               uint32_t n_lanes);
void port_mgr_tof2_umac_de_config(bf_dev_id_t dev_id,
                                  uint32_t umac,
                                  uint32_t ch);
void port_mgr_tof2_umac_enable(bf_dev_id_t dev_id, uint32_t umac, uint32_t ch);
void port_mgr_tof2_umac_disable(bf_dev_id_t dev_id, uint32_t umac, uint32_t ch);
void port_mgr_tof2_umac_link_state_get(bf_dev_id_t dev_id,
                                       uint32_t umac,
                                       uint32_t ch,
                                       bool *up);
void port_mgr_tof2_umac_link_fault_get(bf_dev_id_t dev_id,
                                       uint32_t umac,
                                       uint32_t ch,
                                       bool *pcs_ready,
                                       bool *local_fault,
                                       bool *remote_fault);
void port_mgr_tof2_umac_get_pcs_status(bf_dev_id_t dev_id,
                                       int umac,
                                       int ch,
                                       bool *pcs_status,
                                       uint32_t *block_lock_per_pcs_lane,
                                       uint32_t *alig_marker_lock_per_pcs_lane,
                                       bool *hi_ber,
                                       bool *block_lock_all,
                                       bool *alignment_marker_lock_all);
void port_mgr_tof2_umac_sw_reset_tx_set(bf_dev_id_t dev_id,
                                        uint32_t umac,
                                        uint32_t ch,
                                        bool assert_reset);

void port_mgr_tof2_umac_sw_reset_rx_set(bf_dev_id_t dev_id,
                                        uint32_t umac,
                                        uint32_t ch,
                                        bool assert_reset);
void port_mgr_tof2_umac_sw_reset_set(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch,
                                     bool assert_reset);
bf_status_t port_mgr_tof2_umac_read_counter(bf_dev_id_t dev_id,
                                            uint32_t umac,
                                            uint32_t ch,
                                            bf_rmon_counter_t ctr_id,
                                            uint64_t *ctr_value);
bf_status_t port_mgr_tof2_umac_clear_counter(bf_dev_id_t dev_id,
                                             uint32_t umac,
                                             uint32_t ch);
bf_status_t umac_ll_rd(bf_dev_id_t dev_id,
                       uint32_t umac,
                       uint32_t offset,
                       uint32_t *r_data);
bf_status_t umac_ll_wr(bf_dev_id_t dev_id,
                       uint32_t umac,
                       uint32_t offset,
                       uint32_t w_data);
void port_mgr_tof2_umac_lane_map_set(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t phys_tx_ln[8],
                                     uint32_t phys_rx_ln[8]);
void port_mgr_tof2_umac_sigovrd_set(bf_dev_id_t dev_id,
                                    uint32_t umac,
                                    uint32_t ch,
                                    uint32_t n_lanes,
                                    bf_sigovrd_fld_t ovrd_val);
bf_status_t port_mgr_tof2_umac_loopback_set(bf_dev_id_t dev_id,
                                            uint32_t umac,
                                            uint32_t ch,
                                            bf_loopback_mode_e mode);
void port_mgr_tof2_umac_forced_sigok_get(bf_dev_id_t dev_id,
                                         uint32_t umac,
                                         uint32_t ch,
                                         uint32_t n_lanes,
                                         uint32_t *force_hi_raw_val,
                                         uint32_t *force_lo_raw_val,
                                         uint32_t *force_hi,
                                         uint32_t *force_lo);
void port_mgr_tof2_umac_rs_fec_status_and_counters_get(
    bf_dev_id_t dev_id,
    uint32_t umac,
    uint32_t ch,
    bool *hi_ser,
    bool *fec_align_status,
    uint32_t *fec_corr_cnt,
    uint32_t *fec_uncorr_cnt,
    uint32_t *fec_ser_lane_0,
    uint32_t *fec_ser_lane_1,
    uint32_t *fec_ser_lane_2,
    uint32_t *fec_ser_lane_3,
    uint32_t *fec_ser_lane_4,
    uint32_t *fec_ser_lane_5,
    uint32_t *fec_ser_lane_6,
    uint32_t *fec_ser_lane_7);
void port_mgr_tof2_umac_dis_all_set(bf_dev_id_t dev_id);
void port_mgr_tof2_umac_int_en_set(bf_dev_id_t dev_id, uint32_t umac, bool on);
void port_mgr_tof2_umac_local_fault_int_en_set(bf_dev_id_t dev_id,
                                               uint32_t umac,
                                               uint32_t ch,
                                               bool en);
void port_mgr_tof2_umac_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                uint32_t umac,
                                                uint32_t ch,
                                                bool en);
void port_mgr_tof2_umac_link_gain_int_en_set(bf_dev_id_t dev_id,
                                             uint32_t umac,
                                             uint32_t ch,
                                             bool en);
void port_mgr_tof2_umac_tx_local_fault_set(bf_dev_id_t dev_id,
                                           uint32_t umac,
                                           uint32_t ch,
                                           bool on);
void port_mgr_tof2_umac_tx_remote_fault_set(bf_dev_id_t dev_id,
                                            uint32_t umac,
                                            uint32_t ch,
                                            bool on);
void port_mgr_tof2_umac_tx_idle_set(bf_dev_id_t dev_id,
                                    uint32_t umac,
                                    uint32_t ch,
                                    bool on);
void port_mgr_tof2_umac_rx_enable(bf_dev_id_t dev_id,
                                  uint32_t umac,
                                  uint32_t ch,
                                  bool rx_en);
void port_mgr_tof2_umac_tx_drain_set(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch,
                                     bool en);
void port_mgr_tof2_umac_tx_reset_set(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch);
void port_mgr_tof2_umac_rx_reset_set(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch);
bf_status_t port_mgr_tof2_umac_flowcontrol_set(bf_dev_id_t dev_id,
                                               uint32_t umac,
                                               uint32_t ch);
bf_status_t port_mgr_tof2_umac_handle_interrupts(bf_dev_id_t dev_id,
                                                 uint32_t umac,
                                                 uint32_t ch,
                                                 bool *possible_state_chg);
bf_status_t port_mgr_tof2_umac_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                                uint32_t umac,
                                                uint32_t ch,
                                                bool en);
bf_status_t port_mgr_tof2_umac_config_get(bf_dev_id_t dev_id,
                                          uint32_t umac,
                                          uint32_t ch);
void port_mgr_tof2_umac_channel_add(bf_dev_id_t dev_id,
                                    uint32_t umac,
                                    uint32_t ch,
                                    uint32_t n_ch);
void port_mgr_tof2_umac_hw_cfg_get(bf_dev_id_t dev_id,
                                   uint32_t umac,
                                   uint32_t ch);
bf_status_t port_mgr_tof2_umac_get_1588_timestamp_tx(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint64_t *ts,
                                                     bool *ts_valid,
                                                     int *ts_id);
bf_status_t port_mgr_tof2_umac_set_1588_timestamp_delta_tx(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint16_t delta);
bf_status_t port_mgr_tof2_umac_get_1588_timestamp_delta_tx(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint16_t *delta);
bf_status_t port_mgr_tof2_umac_set_1588_timestamp_delta_rx(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint16_t delta);
bf_status_t port_mgr_tof2_umac_get_1588_timestamp_delta_rx(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint16_t *delta);
#ifdef __cplusplus
}
#endif /* C++ */

#endif
