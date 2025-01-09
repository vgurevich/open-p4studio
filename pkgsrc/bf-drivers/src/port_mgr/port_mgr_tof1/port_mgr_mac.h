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


#ifndef PORT_MGR_MAC_H_INCLUDED
#define PORT_MGR_MAC_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  FEC_TYP_NONE = 0,
  FEC_TYP_FIRECODE = 1,
  FEC_TYP_REED_SOLOMON = 2
} port_mgr_fec_type_e;

typedef enum {
  CMRA_CH_SPD_NONE = 0,
  CMRA_CH_SPD_1G = 3,
  CMRA_CH_SPD_10G = 4,
  CMRA_CH_SPD_25G = 6,
  CMRA_CH_SPD_40G = 7,
  CMRA_CH_SPD_50G = 8,
  CMRA_CH_SPD_100G = 9,
} port_mgr_comira_ch_speed_e;

typedef enum {
  CMRA_PMA_BASE_R1 = 3,
  CMRA_PMA_BASE_R2 = 4,
  CMRA_PMA_BASE_R4 = 5,
  CMRA_PMA_MLG = 15,
} port_mgr_comira_pma_e;

typedef void (*mac_int_dump_cb)(bf_dev_id_t dev_id,
                                int mac_block,
                                int ch,
                                uint32_t int_reg,
                                int bit,
                                uint32_t total,
                                uint32_t shown);
void port_mgr_mac_interrupt_dump(mac_int_dump_cb fn, bf_dev_id_t dev_id);
void port_mgr_mac_interrupt_poll(bf_dev_id_t dev_id, int mac_block, int ch);

void port_mgr_mac_enable_ch(bf_dev_id_t dev_id, int mac_block, int ch);
void port_mgr_mac_disable_ch(bf_dev_id_t dev_id, int mac_block, int ch);
void port_mgr_mac_delete_ch(bf_dev_id_t dev_id, int mac_block, int ch);

void port_mgr_mac_set_speed_100g(bf_dev_id_t dev_id,
                                 int mac_block,
                                 int ch,
                                 int pma,
                                 int fec,
                                 int loopback_en);
void port_mgr_mac_set_speed_50g(bf_dev_id_t dev_id,
                                int mac_block,
                                int ch,
                                int pma,
                                int fec,
                                int loopback_en);
void port_mgr_mac_set_speed_40g(bf_dev_id_t dev_id,
                                int mac_block,
                                int ch,
                                int pma,
                                int fec,
                                int loopback_en);
void port_mgr_mac_set_speed_25g(bf_dev_id_t dev_id,
                                int mac_block,
                                int ch,
                                int pma,
                                int fec,
                                int loopback_en);
void port_mgr_mac_set_speed_10g(bf_dev_id_t dev_id,
                                int mac_block,
                                int ch,
                                int pma,
                                int fec,
                                int loopback_en);
void port_mgr_mac_set_speed_1g(bf_dev_id_t dev_id,
                               int mac_block,
                               int ch,
                               int pma,
                               int fec,
                               int loopback_en);
void port_mgr_mac_unconfigure(bf_dev_id_t dev_id, int mac_block, int ch);
void port_mgr_mac_int_fault_get(bf_dev_id_t dev_id,
                                int mac_blk,
                                int ch,
                                int *remote_fault,
                                int *local_fault);
bf_status_t port_mgr_mac_int_fault_cache_get(bf_dev_id_t dev_id,
                                             int mac_blk,
                                             int ch,
                                             int *remote_fault,
                                             int *local_fault);
bf_status_t port_mgr_mac_int_fault_clr(bf_dev_id_t dev_id,
                                       int mac_blk,
                                       int ch,
                                       int clear_all);
int port_mgr_mac_live_link_state(bf_dev_id_t dev_id,
                                 int mac_block,
                                 int channel);
int port_mgr_mac_link_state(bf_dev_id_t dev_id,
                            int mac_block,
                            int channel,
                            bool oper_state);
bf_status_t port_mgr_mac_link_state_v2(bf_dev_id_t dev_id,
                                       int mac_block,
                                       int channel,
                                       int *state,
                                       int *local_fault,
                                       int *remote_fault);
void port_mgr_mac_basic_setup(bf_dev_id_t dev_id, int mac_block, int ch);
void port_mgr_mac_basic_interrupt_setup(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);
void port_mgr_mac_basic_interrupt_teardown(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port);
void port_mgr_mac_register_default_handler_for_mac_ints(bf_dev_id_t dev_id);

// mac control
void port_mgr_mac_set_tx_enable(bf_dev_id_t chip,
                                bf_dev_port_t dev_port,
                                bool tx_en);
void port_mgr_mac_set_rx_enable(bf_dev_id_t chip,
                                bf_dev_port_t dev_port,
                                bool rx_en);
void port_mgr_mac_set_drain(bf_dev_id_t dev_id,
                            bf_dev_port_t dev_port,
                            bool en);
void port_mgr_mac_set_promiscuous_mode(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       int en);
void port_mgr_mac_set_loopback_mode(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    bf_loopback_mode_e mode);
void port_mgr_mac_force_pcs_near_loopback(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          bool enable);
void port_mgr_mac_set_flow_control(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
void port_mgr_mac_force_disable_rx_flow_control(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port);
void port_mgr_mac_enable_rx_flow_control(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port);
void port_mgr_mac_set_src_mac(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              uint8_t *mac_addr);
void port_mgr_mac_set_fc_src_mac(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint8_t *mac_addr);
void port_mgr_mac_get_fc_src_mac(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint8_t *mac_addr);
void port_mgr_mac_set_fc_dst_mac(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint8_t *mac_addr);
void port_mgr_mac_get_fc_dst_mac(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint8_t *mac_addr);
void port_mgr_mac_set_fc_pause_time(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    uint32_t pause_time);
void port_mgr_mac_set_xoff_pause_time(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t pause_time);
void port_mgr_mac_get_xoff_pause_time(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      uint32_t *pause_time);
void port_mgr_mac_set_xon_pause_time(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t pause_time);
void port_mgr_mac_get_xon_pause_time(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t *pause_time);
void port_mgr_mac_set_rtestmode(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                bool rtestmode);
void port_mgr_mac_set_force_local_fault(bf_dev_id_t chip,
                                        bf_dev_port_t dev_port,
                                        bool force_val);
void port_mgr_mac_set_force_remote_fault(bf_dev_id_t chip,
                                         bf_dev_port_t dev_port,
                                         bool force_val);
void port_mgr_mac_set_force_idle(bf_dev_id_t chip,
                                 bf_dev_port_t dev_port,
                                 bool force_val);
void port_mgr_mac_set_tx_auto_drain_on_disable(bf_dev_id_t chip,
                                               bf_dev_port_t dev_port,
                                               bool force_val);
void port_mgr_mac_set_rs_fec_control(bf_dev_id_t chip,
                                     bf_dev_port_t dev_port,
                                     bool byp_corr_en,
                                     bool byp_ind_en);
void port_mgr_mac_set_fc_fec_control(bf_dev_id_t chip,
                                     bf_dev_port_t dev_port,
                                     bool corr_en,
                                     bool ind_en);
void port_mgr_mac_get_fc_fec_control(bf_dev_id_t chip,
                                     bf_dev_port_t dev_port,
                                     bool *corr_en,
                                     bool *ind_en);
void port_mgr_mac_get_rs_fec_status_and_counters(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bool *hi_ser,
                                                 bool *fec_align_status,
                                                 uint32_t *fec_corr_cnt,
                                                 uint32_t *fec_uncorr_cnt,
                                                 uint32_t *fec_ser_lane_0,
                                                 uint32_t *fec_ser_lane_1,
                                                 uint32_t *fec_ser_lane_2,
                                                 uint32_t *fec_ser_lane_3);
bf_status_t port_mgr_mac_get_fc_fec_status_and_counters_per_vl(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    uint32_t vl,
    bool *block_lock_status,
    uint32_t *fec_corr_blk_cnt,
    uint32_t *fec_uncorr_blk_cnt);
void port_mgr_mac_pgm_txff_ctrl(bf_dev_id_t dev_id, int mac_block);
bf_status_t port_mgr_mac_reset_txff_ctrl_chnl_enable(bf_dev_id_t dev_id,
                                                     int mac_block);
void port_mgr_mac_get_txff_ctrl(bf_dev_id_t dev_id,
                                int mac_block,
                                bool *txff_ctrl_pad_disable,
                                bool *txff_ctrl_fcs_insert_disable,
                                bool *txff_ctrl_crc_removal_disable,
                                bool *txff_ctrl_crc_check_disable);
void port_mgr_mac_get_loopback_mode(bf_dev_id_t dev_id,
                                    int mac_block,
                                    int ch,
                                    bool *loopback_enable,
                                    bf_loopback_mode_e *mode);
void port_mgr_mac_pgm_force_pfc_flush(bf_dev_id_t dev_id,
                                      int mac_block,
                                      int ch);
void port_mgr_mac_pgm_txff_trunc_ctrl(
    bf_dev_id_t dev_id, int mac_block, int ch, int trunc_sz, int trunc_en);
void port_mgr_mac_get_txff_trunc_ctrl(bf_dev_id_t dev_id,
                                      int mac_block,
                                      int ch,
                                      uint32_t *trunc_sz,
                                      bool *trunc_en);

// read/write
uint32_t mac_read(bf_dev_id_t dev_id, int mac_block, uint32_t offset);
void mac_write(bf_dev_id_t dev_id,
               int mac_block,
               uint32_t offset,
               uint32_t val);

// interrupts
int port_mgr_mac_bind_interrupt_callback(bf_dev_id_t dev_id,
                                         int mac_block,
                                         int ch,
                                         bf_port_int_callback_t fn,
                                         void *userdata);
int port_mgr_mac_interrupt_registration(
    bf_dev_id_t dev_id, int mac_block, int ch, uint32_t reg, uint32_t mask);
int port_mgr_mac_interrupt_unregistration(
    bf_dev_id_t dev_id, int mac_block, int ch, uint32_t reg, uint32_t mask);

// 1588
bf_status_t port_mgr_mac_set_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint16_t delta);
bf_status_t port_mgr_mac_get_1588_timestamp_delta_tx(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint16_t *delta);
bf_status_t port_mgr_mac_set_1588_timestamp_delta_rx(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint16_t delta);
bf_status_t port_mgr_mac_get_1588_timestamp_delta_rx(bf_dev_id_t dev_id,
                                                     bf_dev_port_t dev_port,
                                                     uint16_t *delta);
bf_status_t port_mgr_mac_get_1588_timestamp_tx(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               uint64_t *ts,
                                               bool *ts_valid,
                                               int *ts_id);

// debug
int port_mgr_mac_get_pcs_status(bf_dev_id_t dev_id,
                                int mac_block,
                                int mac,
                                bool *pcs_status,
                                uint32_t *block_lock_per_pcs_lane,
                                uint32_t *alignment_marker_lock_per_pcs_lane,
                                bool *hi_ber,
                                bool *block_lock_all,
                                bool *alignment_marker_lock_all);
int port_mgr_mac_get_pcs_status_v2(bf_dev_id_t dev_id,
                                   int mac_block,
                                   int mac,
                                   bool *pcs_status,
                                   bool *hi_ber,
                                   bool *block_lock_all);
void port_mgr_mac_get_pcs_counters(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t *ber_cnt,
                                   uint32_t *errored_blk_cnt,
                                   uint32_t *sync_loss,
                                   uint32_t *block_lock_loss,
                                   uint32_t *hi_ber_cnt,
                                   uint32_t *valid_error_cnt,
                                   uint32_t *unknown_error_cnt,
                                   uint32_t *invalid_error_cnt,
                                   uint32_t *bip_errors_per_pcs_lane);
void port_mgr_mac_get_pcs_cumulative_counters(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t *ber_cnt,
                                              uint32_t *errored_blk_cnt);
void port_mgr_mac_clr_pcs_cumulative_counters(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port);
int port_mgr_mac_read_counter(bf_dev_id_t chip,
                              bf_dev_port_t dev_port,
                              int ctr,
                              uint64_t *ctr_value);

uint32_t port_mgr_mac_get_glb_mode(bf_dev_id_t chip, bf_dev_port_t dev_port);
uint32_t port_mgr_mac_get_slot2ch_map(bf_dev_id_t chip, bf_dev_port_t dev_port);
void port_mgr_mac_set_lane_remap(bf_dev_id_t chip,
                                 bf_dev_port_t dev_port,
                                 int rx_ln0,
                                 int rx_ln1,
                                 int rx_ln2,
                                 int rx_ln3,
                                 int tx_ln0,
                                 int tx_ln1,
                                 int tx_ln2,
                                 int tx_ln3);

void port_mgr_mac_get_lane_remap(bf_dev_id_t chip,
                                 bf_dev_port_t dev_port,
                                 int *rx_ln0,
                                 int *rx_ln1,
                                 int *rx_ln2,
                                 int *rx_ln3,
                                 int *tx_ln0,
                                 int *tx_ln1,
                                 int *tx_ln2,
                                 int *tx_ln3);

void port_mgr_mac_hw_cfg_get(bf_dev_id_t dev_id, int mac_block, int ch);
int port_mgr_mac_kr_backchannel_mux_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        int tx_ln,
                                        int rx_ln);
bf_status_t port_mgr_mac_block_init(bf_dev_id_t dev_id);
void port_mgr_mac_cfg_txfifo_ctrl(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  uint32_t val);
void port_mgr_mac_cfg_fifo_ctrl1(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t aful_1ch_mode,
                                 uint32_t aful_2ch_mode,
                                 uint32_t aful_4ch_mode);
void port_mgr_mac_stats_clear(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
void port_mgr_mac_rs_fec_reset_set(bf_dev_id_t dev_id,
                                   int mac_block,
                                   int ch,
                                   bool assert_reset);
void port_mgr_mac_rs_fec_25g_am_fix(bf_dev_id_t dev_id,
                                    int mac_block,
                                    int ch,
                                    bool en);
void port_mgr_mac_disable_mac_ints(bf_dev_id_t dev_id);
void port_mgr_mac_get_lf_rf_interrupts(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bool *latched_lf,
                                       bool *latched_rf);
void port_mgr_mac_local_fault_int_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool en);
void port_mgr_mac_remote_fault_int_set(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port,
                                       bool en);
void port_mgr_mac_int_en_get(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             bool *en);
void port_mgr_mac_int_en_set(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             bool en);
void port_mgr_mac_pgm_rxsigok_ctrl(bf_dev_id_t dev_id, int mac_block);
int port_mgr_mac_rxsigok_ctrl_get(bf_dev_id_t dev_id, int mac_block, int ch);
int port_mgr_mac_rxsigok_ctrl_unmapped_get(bf_dev_id_t dev_id,
                                           int mac_block,
                                           int ch);
void port_mgr_mac_sigovrd_set(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              int num_lanes,
                              bf_sigovrd_fld_t sig_ovrd_val);
void port_mgr_mac_rs_fec_scrambler_en_set(bf_dev_id_t dev_id,
                                          int mac_block,
                                          int ch,
                                          bool en);
bf_status_t port_mgr_mac_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port,
                                          bool en);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_MAC_H_INCLUDED
