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


#ifndef port_mgr_tof2_umac4_h
#define port_mgr_tof2_umac4_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************
6'd00 : Disabled (Channel is shut down and held in reset)
6'd01 : 10M SGMII
6'd02 - 03: Reserved
6'd04 : 100M SGMII
6'd05 - 06: Reserved
6'd07 : 1G SGMII
6'd08 : Reserved
6'd09 : 1GBASE-X
6'd10 - 14: Reserved
6'd15 : 10GBASE-R
6'd16 : 10GBASE-R + FCFEC
6'd17 - 20: Reserved
6'd21 : 25GBASE-R1
6'd22 : 25GBASE-R1 + FCFEC
6'd23 : 25GBASE-R1 + RSFEC
6'd24 : 25GBASE-R1 + RSFEC r1.5
6'd25 : 25GBASE-R1 + RSFEC r1.6
6'd26 : 40GBASE-R4
6'd27 : 40GBASE-R4 + FCFEC
6'd28 - 36: Reserved
6'd37 : 50GBASE-R2
6'd38 : 50GBASE-R2 + FCFEC
6'd39 : 50GBASE-R2 + RSFEC
6'd40 : 50GBASE-R1
6'd41 - 42: Reserved
6'd43 : 50GBASE-R1 + RSFEC (KP)
6'd44 - 45: Reserved
6'd46 : 100GBASE-R4
6'd47 : 100GBASE-R4 + RSFEC
6'd48 : 100GBASE-R2
6'd49 : Reserved
6'd50 : 100GBASE-R2 + RSFEC (KP)
6'd51 : 100GBASE-R1 + RSFEC (KP)
6'd52 : 200GBASE-R8 + RSFEC (KP)
6'd53 : 200GBASE-R4 + RSFEC (KP)
6'd54 : 200GBASE-R2 + RSFEC (KP)
6'd55 : 400GBASE-R16 + RSFEC (KP)
6'd56 : 400GBASE-R8 + RSFEC (KP)
6'd57 : 400GBASE-R4 + RSFEC (KP)
6'd58 - 6'd63 : Reserved

  Enum skips unsupported modes
***************************************************************/
typedef enum {
  UMAC4_MODE_RESET =
      0,  // 6'd00 : Disabled (Channel is shut down and held in reset)
  UMAC4_MODE_10M_SGMII = 1,               // 6'd01 : 10M SGMII
  UMAC4_MODE_100M_SGMII = 4,              // 6'd04 : 100M SGMII
  UMAC4_MODE_1G_SGMII = 7,                // 6'd07 : 1G SGMII
  UMAC4_MODE_1GBASE_X = 9,                // 6'd09 : 1GBASE-X
  UMAC4_MODE_10GBASE_R = 15,              // 6'd15 : 10GBASE-R
  UMAC4_MODE_10GBASE_R_FCFEC = 16,        // 10GBASE-R + FCFEC
  UMAC4_MODE_25GBASE_R1 = 21,             // 6'd21 : 25GBASE-R1
  UMAC4_MODE_25GBASE_R1_FCFEC = 22,       // 6'd16 : 10GBASE-R + FCFEC
  UMAC4_MODE_25GBASE_R1_RSFEC = 23,       // 6'd23 : 25GBASE-R1 + RSFEC
  UMAC4_MODE_25GBASE_R1_RSFEC_r1_5 = 24,  // 6'd24 : 25GBASE-R1 + RSFEC r1.5
  UMAC4_MODE_25GBASE_R1_RSFEC_r1_6 = 25,  // 6'd25 : 25GBASE-R1 + RSFEC r1.6
  UMAC4_MODE_40GBASE_R4 = 26,             // 6'd26 : 40GBASE-R4
  UMAC4_MODE_40GBASE_R4_FCFEC = 27,       // 6'd27 : 40GBASE-R4 + FCFEC
                                          // non standard mode 40GBASE_R2 uses
                                          // same settings as for 50GBASE_R2:
  UMAC4_MODE_40GBASE_R2 = 37,             // 6'd37 : 50GBASE-R2
  UMAC4_MODE_40GBASE_R2_FCFEC = 38,       // 6'd38 : 50GBASE-R2 + FCFEC
  UMAC4_MODE_40GBASE_R2_RSFEC = 39,       // 6'd39 : 50GBASE-R2 + RSFEC
  UMAC4_MODE_50GBASE_R2 = 37,             // 6'd37 : 50GBASE-R2
  UMAC4_MODE_50GBASE_R2_FCFEC = 38,       // 6'd38 : 50GBASE-R2 + FCFEC
  UMAC4_MODE_50GBASE_R2_RSFEC = 39,       // 6'd39 : 50GBASE-R2 + RSFEC

  UMAC4_MODE_50GBASE_R1_RSFEC = 43,   // 6'd43 : 50GBASE-R1 + RSFEC (KP)
  UMAC4_MODE_100GBASE_R4 = 46,        // 6'd46 : 100GBASE-R4
  UMAC4_MODE_100GBASE_R4_RSFEC = 47,  // 6'd47 : 100GBASE-R4 + RSFEC
  UMAC4_MODE_100GBASE_R2 = 48,        // 6'd48 : 100GBASE-R2
  UMAC4_MODE_100GBASE_R2_RSFEC = 50,  // 6'd50 : 100GBASE-R2 + RSFEC (KP)
  UMAC4_MODE_200GBASE_R8_RSFEC = 52,  // 6'd52 : 200GBASE-R8 + RSFEC (KP)
  UMAC4_MODE_200GBASE_R4_RSFEC = 53,  // 6'd53 : 200GBASE-R4 + RSFEC (KP)
  UMAC4_MODE_400GBASE_R8_RSFEC = 56,  // 6'd56 : 400GBASE-R8 + RSFEC (KP)

} umac4_mode_t;

void port_mgr_tof2_umac4_init(bf_dev_id_t dev_id,
                              uint32_t umac4,
                              uint32_t clk_div);

void port_mgr_tof2_umac4_config(bf_dev_id_t dev_id,
                                uint32_t umac4,
                                uint32_t ch,
                                bf_port_speed_t speed,
                                bf_fec_types_t fec,
                                uint32_t n_lanes);

void port_mgr_tof2_umac4_de_config(bf_dev_id_t dev_id,
                                   uint32_t umac4,
                                   uint32_t ch);

void port_mgr_tof2_umac4_enable(bf_dev_id_t dev_id,
                                uint32_t umac4,
                                uint32_t ch);

void port_mgr_tof2_umac4_disable(bf_dev_id_t dev_id,
                                 uint32_t umac4,
                                 uint32_t ch);

void port_mgr_tof2_umac4_link_state_get(bf_dev_id_t dev_id,
                                        uint32_t umac4,
                                        uint32_t ch,
                                        bool *up);

void port_mgr_tof2_umac4_link_fault_get(bf_dev_id_t dev_id,
                                        uint32_t umac4,
                                        uint32_t ch,
                                        bool *pcs_ready,
                                        bool *local_fault,
                                        bool *remote_fault);
void port_mgr_tof2_umac4_get_pcs_status(bf_dev_id_t dev_id,
                                        int umac4,
                                        int ch,
                                        bool *pcs_status,
                                        uint32_t *block_lock_per_pcs_lane,
                                        uint32_t *alig_marker_lock_per_pcs_lane,
                                        bool *hi_ber,
                                        bool *block_lock_all,
                                        bool *alignment_marker_lock_all);
void port_mgr_tof2_umac4_sw_reset_tx_set(bf_dev_id_t dev_id,
                                         uint32_t umac4,
                                         uint32_t ch,
                                         bool assert_reset);

void port_mgr_tof2_umac4_sw_reset_rx_set(bf_dev_id_t dev_id,
                                         uint32_t umac4,
                                         uint32_t ch,
                                         bool assert_reset);
bf_status_t port_mgr_tof2_umac4_interrupt_get(bf_dev_id_t dev_id,
                                              uint32_t umac,
                                              uint32_t channel,
                                              uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_interrupt_dn_up_get(bf_dev_id_t dev_id,
                                                    uint32_t umac,
                                                    uint32_t channel,
                                                    uint64_t *dn,
                                                    uint64_t *up);
bf_status_t port_mgr_tof2_umac4_chmode_get(bf_dev_id_t dev_id,
                                           uint32_t umac4,
                                           uint32_t ch,
                                           uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_maccfg_get(bf_dev_id_t dev_id,
                                           uint32_t umac4,
                                           uint32_t ch,
                                           uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_chconfig30_get(bf_dev_id_t dev_id,
                                               uint32_t umac4,
                                               uint32_t ch,
                                               uint64_t *reg64);
bf_status_t port_mgr_tof2_eth400g_mac_txff_ctrl_get(bf_dev_id_t dev_id,
                                                    uint32_t umac4,
                                                    uint32_t ch,
                                                    uint32_t *reg32);
bf_status_t port_mgr_tof2_eth400g_mac_chnl_seq_get(bf_dev_id_t dev_id,
                                                   uint32_t umac4,
                                                   uint32_t ch,
                                                   uint32_t *reg32);
bf_status_t port_mgr_tof2_umac4_read_counter(bf_dev_id_t dev_id,
                                             uint32_t umac4,
                                             uint32_t ch,
                                             bf_rmon_counter_t ctr_id,
                                             uint64_t *ctr_value);
bf_status_t port_mgr_tof2_umac4_clear_counter(bf_dev_id_t dev_id,
                                              uint32_t umac4,
                                              uint32_t ch);
void port_mgr_tof2_umac4_lane_map_set(bf_dev_id_t dev_id,
                                      uint32_t umac4,
                                      uint32_t phys_tx_ln[8],
                                      uint32_t phys_rx_ln[8]);
// rxsigok forces
void port_mgr_tof2_umac4_force_sigok_hi_set(bf_dev_id_t dev_id,
                                            uint32_t umac4,
                                            uint32_t ch,
                                            uint32_t n_lanes);
void port_mgr_tof2_umac4_force_sigok_low_set(bf_dev_id_t dev_id,
                                             uint32_t umac4,
                                             uint32_t ch,
                                             uint32_t n_lanes);
void port_mgr_tof2_umac4_clear_forced_sigok_set(bf_dev_id_t dev_id,
                                                uint32_t umac4,
                                                uint32_t ch,
                                                uint32_t n_lanes);
bf_status_t port_mgr_tof2_umac4_to_umac3_ctr_copy(uint64_t *umac4_ctr_array,
                                                  uint64_t *umac3_ctr_array);
void port_mgr_tof2_umac4_loopback_set(bf_dev_id_t dev_id,
                                      uint32_t umac4,
                                      uint32_t ch,
                                      bf_loopback_mode_e mode);
void port_mgr_tof2_umac4_txdrain_set(bf_dev_id_t dev_id,
                                     uint32_t umac4,
                                     uint32_t ch,
                                     bool en);
void port_mgr_tof2_umac4_txdrain_get(bf_dev_id_t dev_id,
                                     uint32_t umac4,
                                     uint32_t ch,
                                     bool *en);
bf_status_t port_mgr_tof2_umac4_status_get(bf_dev_id_t dev_id,
                                           uint32_t umac,
                                           uint32_t channel,
                                           uint64_t *reg64,
                                           uint64_t *txclkpresentall,
                                           uint64_t *rxclkpresentall,
                                           uint64_t *rxsigokall,
                                           uint64_t *blocklockall,
                                           uint64_t *amlockall,
                                           uint64_t *aligned,
                                           uint64_t *nohiber,
                                           uint64_t *nolocalfault,
                                           uint64_t *noremotefault,
                                           uint64_t *linkup,
                                           uint64_t *hiser,
                                           uint64_t *fecdegser,
                                           uint64_t *rxamsf);
void port_mgr_tof2_umac4_forced_sigok_get(bf_dev_id_t dev_id,
                                          uint32_t umac4,
                                          uint32_t ch,
                                          uint32_t n_lanes,
                                          uint32_t *force_hi_raw_val,
                                          uint32_t *force_lo_raw_val,
                                          uint32_t *force_hi,
                                          uint32_t *force_lo);
void port_mgr_tof2_umac4_rs_fec_status_and_counters_get(
    bf_dev_id_t dev_id,
    uint32_t umac4,
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
void port_mgr_tof2_umac4_int_en_set(bf_dev_id_t dev_id,
                                    uint32_t umac4,
                                    bool on);
void port_mgr_tof2_umac4_ch_int_en_all_set(bf_dev_id_t dev_id,
                                           uint32_t umac4,
                                           uint32_t ch);
void port_mgr_tof2_umac4_ch_int_dis_all_set(bf_dev_id_t dev_id,
                                            uint32_t umac4,
                                            uint32_t ch);
void port_mgr_tof2_umac4_local_fault_int_en_set(bf_dev_id_t dev_id,
                                                uint32_t umac4,
                                                uint32_t ch,
                                                bool en);
void port_mgr_tof2_umac4_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                 uint32_t umac4,
                                                 uint32_t ch,
                                                 bool en);
void port_mgr_tof2_umac4_link_gain_int_en_set(bf_dev_id_t dev_id,
                                              uint32_t umac4,
                                              uint32_t ch,
                                              bool en);
void port_mgr_tof2_umac4_tx_local_fault_set(bf_dev_id_t dev_id,
                                            uint32_t umac4,
                                            uint32_t ch,
                                            bool on);
void port_mgr_tof2_umac4_tx_remote_fault_set(bf_dev_id_t dev_id,
                                             uint32_t umac4,
                                             uint32_t ch,
                                             bool on);
void port_mgr_tof2_umac4_tx_idle_set(bf_dev_id_t dev_id,
                                     uint32_t umac4,
                                     uint32_t ch,
                                     bool on);
bf_status_t port_mgr_tof2_eth400g_serdes_mode_get(bf_dev_id_t dev_id,
                                                  uint32_t umac4,
                                                  uint32_t ch,
                                                  uint32_t *tx_sds_mode,
                                                  bool *tx_is_pam4,
                                                  uint32_t *tx_phys_ln,
                                                  uint32_t *rx_sds_mode,
                                                  bool *rx_is_pam4,
                                                  uint32_t *rx_phys_ln);
bf_status_t port_mgr_tof2_umac4_sdcfg_get(bf_dev_id_t dev_id,
                                          uint32_t umac4,
                                          uint32_t ch,
                                          uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_sdcfg_detail_get(bf_dev_id_t dev_id,
                                                 uint32_t umac4,
                                                 uint32_t ch,
                                                 uint64_t *serdeslpbk,
                                                 uint64_t *txremap,
                                                 uint64_t *sigokoverride,
                                                 uint64_t *rxremap,
                                                 uint64_t *txinv,
                                                 uint64_t *rxinv,
                                                 uint64_t *paceren,
                                                 uint64_t *pacerdiv,
                                                 uint64_t *txprbssel,
                                                 uint64_t *rxprbssel);
bf_status_t port_mgr_tof2_umac4_sderrcfg_get(bf_dev_id_t dev_id,
                                             uint32_t umac4,
                                             uint32_t ch,
                                             uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_sderrcfg_detail_get(bf_dev_id_t dev_id,
                                                    uint32_t umac4,
                                                    uint32_t ch,
                                                    uint64_t *txerrperiod,
                                                    uint64_t *txerrburst);
bf_status_t port_mgr_tof2_umac4_sdsts_get(bf_dev_id_t dev_id,
                                          uint32_t umac4,
                                          uint32_t ch,
                                          uint64_t *reg64);
bf_status_t port_mgr_tof2_umac4_sdsts_detail_get(bf_dev_id_t dev_id,
                                                 uint32_t umac4,
                                                 uint32_t ch,
                                                 uint64_t *txclkpresent,
                                                 uint64_t *txclkrate,
                                                 uint64_t *rxclkpresent,
                                                 uint64_t *rxclkrate,
                                                 uint64_t *sigok,
                                                 uint64_t *rxprbserrcnt);

void port_mgr_tof2_umac4_rx_enable_set(bf_dev_id_t dev_id,
                                       uint32_t umac4,
                                       uint32_t ch,
                                       bool rx_en);
void port_mgr_tof2_umac4_flowcontrol_config_set(bf_dev_id_t dev_id,
                                                uint32_t umac3,
                                                uint32_t ch);
bf_status_t port_mgr_tof2_umac4_handle_interrupts(bf_dev_id_t dev_id,
                                                  uint32_t umac4,
                                                  uint32_t ch,
                                                  bool *possible_state_chg);
void port_mgr_tof2_umac4_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                          uint32_t umac4,
                                          uint32_t ch,
                                          bool en);
void port_mgr_tof2_umac4_config_get(bf_dev_id_t dev_id,
                                    uint32_t umac4,
                                    uint32_t ch);
void port_mgr_tof2_umac4_channel_add(bf_dev_id_t dev_id,
                                     uint32_t umac,
                                     uint32_t ch,
                                     uint32_t n_ch);
void port_mgr_tof2_umac4_get_1588_timestamp_tx(bf_dev_id_t dev_id,
                                               uint32_t mac_blk,
                                               uint32_t ch,
                                               uint64_t *ts,
                                               bool *ts_valid,
                                               int *ts_id);
void port_mgr_tof2_umac4_set_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint16_t delta);
void port_mgr_tof2_umac4_get_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     uint32_t mac_blk,
                                                     uint16_t *delta);
#ifdef __cplusplus
}
#endif /* C++ */

#endif
