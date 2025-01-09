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


#ifndef port_mgr_logical_port_h_included
#define port_mgr_logical_port_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>

typedef struct port_cfg_settings_t {
  int assigned;  // 1=in-use, 0=available for assignment
  bool enabled;
  bool oper_state;
  uint32_t n_lanes;  // varies by speed and enc mode (NRZ/PAM4)
  bf_port_speed_t speed;
  bf_fec_type_t fec;
  bf_port_prbs_mode_t prbs_mode;
  uint32_t single_lane_error_thresh;  // max PCS errors before considering
                                      // link down (0=no limit)
  // Firecode values which must be programmed with umac in reset
  bool fc_corr_en;
  bool fc_ind_en;
  int tx_mtu;
  int rx_mtu;
  int rx_max_jab_sz;
  int ifg;
  int ipg;
  uint8_t preamble[7];  // custom preamble
  int preamble_length;
  int promiscuous_mode;
  uint8_t mac_addr[6];
  bool loopback_enabled;
  bf_dfe_type_e dfe_type;
  // flow control settings
  bool link_pause_tx;
  bool link_pause_rx;
  uint8_t pfc_pause_tx;
  uint8_t pfc_pause_rx;
  uint32_t xoff_pause_time;
  uint32_t xon_pause_time;
  uint8_t fc_src_mac_addr[6];
  uint8_t fc_dst_mac_addr[6];
  // txff settings
  uint32_t txff_trunc_ctrl_size;
  bool txff_trunc_ctrl_en;
  bool txff_ctrl_crc_check_disable;
  bool txff_ctrl_crc_removal_disable;
  bool txff_ctrl_fcs_insert_disable;
  bool txff_ctrl_pad_disable;
  bf_loopback_mode_e lpbk_mode;
  bf_port_dir_e port_dir;
} port_cfg_settings_t;

typedef struct port_link_state_t {
  int local_fault;
  int remote_fault;
  bool pcs_rdy;
  bool hi_ber;
  bool blocklockall;
  uint32_t lnk_int_fcnt;
  bool adative_dfe_enabled;
} port_link_state_t;

/** \typedef port_mgr_fsm_ext_t:
 *  FSM extensions.
 */
typedef struct port_mgr_fsm_ext_t {
  uint32_t debounce_cnt;                      // Debounce counter used by fsm.
  uint32_t debounce_thr;                      // Debounce threshold.
  bf_port_link_fault_st_t link_fault_status;  // Link fault status associted to
                                              // current fsm state.
  bf_port_link_fault_st_t alt_link_fault_st;  // Alt link fault status to be
                                              // used when interrupt based link
                                              // management is enabled
  bool port_was_up;                           // indicates the link was up in
                                              // current config sequence.
} port_mgr_fsm_ext_t;

/** \typedef port_mgr_an_stats_t:
 *  Auto-neg stats
 */
typedef struct port_mgr_an_stats_t {
  // link training: start timestamp and duration
  uint64_t an_lt_start_ns;
  uint64_t an_lt_start_s;
  uint64_t an_lt_dur_us;
  uint64_t an_lt_dur_us_cache;
  uint32_t an_try_count;
} port_mgr_an_stats_t;

/** \typedef port_mgr_pcs_ctrs:
 *  PCS cumulative error counters.
 */
typedef struct port_mgr_pcs_ctrs_t {
  uint32_t block_err;
  uint32_t bercount;
} port_mgr_pcs_ctrs_t;

/** \typedef port_mgr_port_t:
 */
typedef struct port_mgr_port_s {
  // from APIs
  port_cfg_settings_t sw;

  // from current hw setting (warm-init)
  port_cfg_settings_t hw;

  // link state related variables.
  port_link_state_t lstate;

  // front port id (from lane map)
  uint32_t fp_conn_id;
  uint32_t fp_chnl_id;

  // our AN adverts
  bool an_enabled;
  uint32_t av_hcd;  // raw hcd value for use by assert link status
  int hcd_ndx;      // generic raw hcd index
  uint32_t cur_next_pg;
  uint64_t an_base_page;
  uint32_t an_num_next_pages;
  uint64_t an_next_page[BF_MAX_AN_NEXT_PAGES];

  // link partners AN adverts
  bool an_log_ena;
  uint64_t an_lp_base_page;
  uint32_t an_lp_num_next_pages;
  uint64_t an_lp_next_page[BF_MAX_AN_NEXT_PAGES];

  // Temp LP rx pages: Used to save received pages for an ongoing negotiation
  uint64_t an_lp_tmp_base_page;
  uint32_t an_lp_tmp_num_next_pages;
  uint64_t an_lp_tmp_next_page[BF_MAX_AN_NEXT_PAGES];

  // link training: start timestamp and duration
  port_mgr_an_stats_t an_stats;

  // disable Link-training. Do AN only then just switch to HCD speed
  //(used for Credo retimer host-side)
  bool lt_disabled;

  // # cycles spent in current FSM state (for FSM state timeout)
  uint32_t time_in_state;

  // callback related info
  bf_port_callback_t sts_chg_cb;
  void *sts_chg_userdata;
  bf_port_int_callback_t mac_int_cb;
  void *mac_int_userdata;

  // MAC stat DMA info
  bf_port_mac_stat_callback_t mac_stat_user_cb;
  void *mac_stat_user_data;
  bf_rmon_counter_array_t mac_stat_cache;
  bf_rmon_counter_array_t mac_stat_historical;  // counts since last port-dis
  bf_rmon_counter_array_t *mac_stat_dma_vaddr;

  // PCS cumulative error counters
  port_mgr_pcs_ctrs_t pcs_ctrs;

  uint64_t mac_stat_dma_paddr;
  bf_sys_dma_pool_handle_t mac_stat_hndl;
  uint32_t mac_stat_buf_size;

  // PCS near-loopback override info (used by tx-only mode)
  bool forced_pcs_loopback;

  // Serdes upgrade info
  bool serdes_upgrade_required;

  // QSFP info (needed for optical xcvers)
  bool is_optical;
  bool optical_xcvr_ready;
  bool loss_of_optical_signal;

  struct timeval last_up_time;
  struct timeval last_dn_time;
  struct timeval last_en_time;  // time last enabled
  struct timeval last_sig_detect_time;
  bool issue_callbacks;  // Indicates if we need to issue callbacks for oper
                         // state
  bf_sys_mutex_t port_mtx;

  bool fec_reset_inp;

  struct port_mgr_fsm_ext_t fsm_ext;

  bool force_link_down_cb;
} port_mgr_port_t;

void port_mgr_set_default_port_state(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
