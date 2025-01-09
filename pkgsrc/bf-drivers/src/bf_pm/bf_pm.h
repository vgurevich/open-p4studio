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


#ifndef _BF_PM_H
#define _BF_PM_H

#include <tofino/bf_pal/bf_pal_types.h>
#include <target-sys/bf_sal/bf_sys_sem.h>
#include <sys/time.h>

#define PORT_STATS_POLL_TMR_PERIOD_MS 1000
#define PORT_STATS_POLL_MIN_TMR_PERIOD_MS 50
#define BF_PM_FSM_LINK_UP_THRSHLD_DEFAULT 0
#define BF_PM_FSM_LINK_UP_THRSHLD_MAX 100

#define PORT_MGR_LOCK(x)                                           \
  do {                                                             \
    int A_A = bf_sys_rmutex_lock(x);                               \
    if (A_A) {                                                     \
      char errStr[32] = {0};                                       \
      strerror_r(A_A, errStr, 32);                                 \
      PM_ERROR("LOCK: Error \"%s\" (%d) taking \"%s\" from %s:%d", \
               errStr,                                             \
               A_A,                                                \
               #x,                                                 \
               __func__,                                           \
               __LINE__);                                          \
    }                                                              \
  } while (0);

#define PORT_MGR_UNLOCK(x)                                              \
  do {                                                                  \
    int A_A = bf_sys_rmutex_unlock(x);                                  \
    if (A_A) {                                                          \
      char errStr[32] = {0};                                            \
      strerror_r(A_A, errStr, 32);                                      \
      PM_ERROR("UNLOCK: Error \"%s\" (%d) releasing \"%s\" from %s:%d", \
               errStr,                                                  \
               A_A,                                                     \
               #x,                                                      \
               __func__,                                                \
               __LINE__);                                               \
    }                                                                   \
  } while (0);

/**
 * Accumulated FEC information of a port in the system
 */
typedef struct bf_pm_port_fec_info_ {
  /* RS FEC info */
  bool hi_ser;
  bool fec_align_status;
  uint32_t fec_corr_cnt;
  uint32_t fec_uncorr_cnt;
  uint32_t fec_ser_lane_0;
  uint32_t fec_ser_lane_1;
  uint32_t fec_ser_lane_2;
  uint32_t fec_ser_lane_3;
  uint32_t fec_ser_lane_4;
  uint32_t fec_ser_lane_5;
  uint32_t fec_ser_lane_6;
  uint32_t fec_ser_lane_7;

  /* FC FEC info */
  bool fc_block_lock_status[4];
  uint32_t fc_fec_corr_blk_cnt[4];
  uint32_t fc_fec_uncorr_blk_cnt[4];
} bf_pm_port_fec_info_t;

//#include <port_mgr/bf_tof2_serdes_if.h>
typedef struct info_t {
  struct timeval tv;
  bool st;  // false=dn true=up
  // int info
  uint64_t int_reg;
  struct {
    // PCS info
    uint64_t txclkpresentall;
    uint64_t rxclkpresentall;
    uint64_t rxsigokall;
    uint64_t blocklockall;
    uint64_t amlockall;
    uint64_t aligned;
    uint64_t nohiber;
    uint64_t nolocalfault;
    uint64_t noremotefault;
    uint64_t linkup;
    uint64_t hiser;
    uint64_t fecdegser;
    uint64_t rxamsf;
  } pcs;
  struct {
    // serdes info
    uint32_t G, adapt, readapt, link_lost, of, hf;
    uint32_t skef_val, dac_val;
    uint32_t ctle_over_val, ctle_map_0, ctle_map_1, ctle_1, ctle_2, dc_gain_1,
        dc_gain_2;
    uint32_t delta, edge1, edge2, edge3, edge4;
    uint32_t tap1, tap2, tap3, f13_val;
    int signed_ppm, signed7_delta;
    bf_serdes_encoding_mode_t enc_mode;
    bool sig_detect, phy_ready, adapt_done;
    int32_t ppm;
    float chan_est, eye_1, eye_2, eye_3;
    float f0, f1, ratio;
    int32_t k1, k2, k3, k4, s1, s2;
  } serdes[MAX_LANES_PER_PORT];
} info_t;

/**
 * Identifies the information of a port in the system
 */
typedef struct bf_pm_port_info_t {
  bool is_added;     // Indicates if this port is added or not
  bool is_internal;  // Indicates if this port has serdes or not
  bool
      is_iport_user_created;  // Indicates if internal port user-created or auto
  bf_dev_port_t dev_port;     // Device port number

  bf_pal_front_port_info_t pltfm_port_info;  // Information of the port
  // retrieved from the platforms module

  bf_sys_rmutex_t port_mtx;              // Per port recursive mutex
  bf_pm_port_admin_state_e admin_state;  // Indicates the admin state of a port
  bf_pm_port_oper_status_e oper_status;  // Indicates the oper status of a port
  bf_port_speed_t speed;                 // Port speed configured by the user
  uint32_t n_lanes;                      // Number of lanes in the port
  bf_fec_type_t fec_type;                // Port FEC type configured by the user
  bool is_ready_for_bringup;  // Indicates if FSM should be started on a port
                              // or not
  bool is_an_eligible;        // Indicates if a port is eligible for AN. This is
                              // reported by the platforms module.
  bf_pm_port_precoding_policy_e pc_tx_policy[MAX_LANES_PER_PORT];  // Port Tx
                                                                   // PRECODING
                                                                   // policy
                                                                   // desired by
                                                                   // the user
  bf_pm_port_precoding_policy_e pc_rx_policy[MAX_LANES_PER_PORT];  // Port Rx
                                                                   // PRECODING
                                                                   // policy
                                                                   // desired by
                                                                   // the user
  bf_pm_port_autoneg_policy_e an_policy;  // Port AN policy desired by the user
                                          /*
                                           If AN policy == PM_AN_DEFAULT && is_an_eligible == true, port will AN
                                           If AN policy == PM_AN_DEFAULT && is_an_eligible == false, port will not AN
                                           If AN policy == PM_AN_FORCE_ENABLE, port will AN irrespective of
                                           is_an_eligible
                                           If AN policy == PM_AN_FORCE_DISABLE, port will not AN irrespective of
                                           is_an_eligible
                                           */
  bf_pm_port_kr_mode_policy_e
      kr_policy;  // Port KR mode policy desired by the user
                  //
  bf_pm_port_term_mode_e term_mode[MAX_LANES_PER_PORT];  // Port termination
                                                         // mode (determined by
                                                         // board layout)
                                                         //
  // If KR policy == PM_KR_DEFAULT port will negotiate KR iff CPU port, others
  // CR
  // If KR policy == PM_KR_FORCE_ENABLE, port will negotiate KR
  // If KR policy == PM_KR_FORCE_DISABLE, port will not negotiate KR
  bf_rmon_counter_array_t curr_stats;  // Current RMON stats
  bf_rmon_counter_array_t old_stats;   // Old RMON stats
  uint64_t stats_timestamp_sec;        // timestamp sec for curr_stats
  uint64_t stats_timestamp_nsec;       // timestamp nsec for curr_stats
  uint64_t prev_stats_timestamp_sec;   // timestamp sec for prev_stats
  uint64_t prev_stats_timestamp_nsec;  // timestamp nsec for prev_stats
  uint64_t dma_timestamp_nsec;         // timestamp nsec for dma
  int64_t rate_timestamp_sec;          // timestamp in sec for rate calculation
  int64_t rate_timestamp_nsec;         // timestamp in nsec for rate calculation
  uint64_t rx_octets_good;  // last OctetsReceivedinGoodFrames for rate
  uint64_t tx_octets_good;  // last OctetsTransmittedwithouterror for rate
  uint64_t rx_frame_good;   // FramesReceivedOK
  uint64_t tx_frame_good;   // FramesTransmittedOK
  long rx_rate;             // latest data rate for rate display
  long tx_rate;             // latest data rate for rate display
  uint32_t rx_pps;          // latest pps for rate display
  uint32_t tx_pps;          // latest pps for rate display

  int link_bring_up_time;  // Time period to try to bring the port UP internally
  int last_dwn_time;       // Relative time for which the link became DOWN
  bool enb_req;  // Indicates if the port needs to be explicitly enabled
                 // by the PM internally
  bf_loopback_mode_e lpbk_mode;  // Indicates the loopback mode set for the port
  int disable_time;  // Time after which the port is forcefully disabled

  bool async_stats_update_request_issued;  // Indicates if a async stats update
                                           // request is already in progress for
                                           // this port
  bf_pm_port_fec_info_t fec_info;          // FEC Info to be gathered
  bool there_is_traffic;      // Indicates if there is traffic via this intf
  bf_pm_port_dir_e port_dir;  // Port direction set by user
  uint32_t actv_chnl_mask;    // for channelizinf FSM operations
  uint32_t history_next;      // first index below (ln is second index)
  info_t history[8];          // fw_serdes_param info last 8 times port came up

  bool serdes_rx_ready;          // flag set from port-fsm. Cumulative of all
                                 // serdes-lanes
  bool serdes_tx_ready;          // flag set from port-fsm. Cumulative of all
                                 // serdes-lanes
  bool serdes_rx_ready_for_dfe;  // set from platform when AN=off.
  bool module_ready_for_link;    // set from platform to indicate moule is ready
                                 // for link status processing
                                 // flag is only used for tx-mode operation
  bf_port_prbs_mode_t prbs_mode;
  bool serdes_tx_eq_override;  // user has applied his/her own settings
  bool serdes_lane_tx_eq_override[MAX_LANES_PER_PORT];  // per-lane Tx Eq
                                                        // overrides
  uint32_t fsm_debounce_cnt;       // Debounce counter used by fsm.
  uint32_t fsm_debounce_thr;       // Debounce threshold.
  bf_port_speeds_t adv_speed_map;  // The map of advertised speeds
  bf_fec_types_t adv_fec_map;      // The map of advertised fec types
} bf_pm_port_info_t;

int pm_port_info_get_copy(bf_dev_id_t dev_id,
                          bf_dev_port_t dev_port,
                          bf_pm_port_info_t *port_info);
int pm_port_info_get_first_copy(bf_dev_id_t dev_id,
                                bf_pm_port_info_t *port_info,
                                bf_dev_id_t *dev_id_of_port);
int pm_port_info_get_next_copy(bf_dev_id_t dev_id,
                               bf_pm_port_info_t *port_info,
                               bf_pm_port_info_t *next_port_info,
                               bf_dev_id_t *dev_id_of_port);
bool pm_port_valid_speed_and_channel(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     bf_port_speed_t speed,
                                     uint32_t n_lanes,
                                     bf_fec_type_t fec);
uint32_t pm_port_info_db_size_get(bf_dev_id_t dev_id);
void pm_link_poll_start(bf_dev_id_t dev_id);
void pm_link_poll_stop(bf_dev_id_t dev_id);
void pm_port_stats_poll_start(bf_dev_id_t dev_id);
void pm_port_stats_poll_stop(bf_dev_id_t dev_id);
uint32_t pm_num_of_internal_ports_get(bf_dev_id_t dev_id);
uint32_t pm_num_of_front_ports_get(bf_dev_id_t dev_id);
uint32_t pm_num_fp_all_get(void);

bf_status_t pm_port_serdes_rx_ready_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bool rx_ready);
bf_status_t pm_port_serdes_tx_ready_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bool tx_ready);
bf_status_t pm_port_serdes_rx_ready_for_bringup_get(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    bool *do_dfe);
bf_status_t bf_pm_port_module_ready_for_link_handling_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, bool *update_link);
bf_status_t bf_pm_port_loopback_mode_from_dev_port_get(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, bf_loopback_mode_e *mode);
bf_status_t bf_pm_actv_chnl_mask_get(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t *msk);
bf_status_t bf_pm_actv_chnl_mask_set(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port,
                                     uint32_t msk);

uint32_t bf_pm_port_get_num_of_lanes(bf_dev_id_t dev_id,
                                     bf_pal_front_port_handle_t *port_hdl);
int pm_port_fsm_stop_set(bf_dev_id_t dev_id, bf_dev_port_t dev_port, bool stop);
bf_status_t bf_pm_port_info_all_get(const char *caller,
                                    bf_dev_id_t exp_dev_id,
                                    bf_pal_front_port_handle_t *port_hdl,
                                    bf_pm_port_info_t **port_info,
                                    bf_dev_id_t *dev_id_of_port,
                                    bf_dev_port_t *dev_port);
bf_status_t bf_pm_is_port_added(bf_dev_id_t dev_id,
                                bf_pal_front_port_handle_t *port_hdl,
                                bool *is_added);
bf_status_t bf_pm_serdes_tx_eq_override_set(
    bf_dev_id_t dev_id, bf_pal_front_port_handle_t *port_hdl, bool override);
bf_status_t bf_pm_serdes_lane_tx_eq_override_set(
    bf_dev_id_t dev_id,
    bf_pal_front_port_handle_t *port_hdl,
    uint32_t ln,
    bool override);
bf_status_t bf_pm_port_precoding_rx_clear(
    bf_dev_id_t dev_id,
    bf_pal_front_port_handle_t *port_hdl_list,
    uint32_t len);
bf_status_t bf_pm_port_precoding_tx_clear(
    bf_dev_id_t dev_id,
    bf_pal_front_port_handle_t *port_hdl_list,
    uint32_t len);
uint32_t pm_num_of_internal_ports_all_get(void);
bf_status_t bf_pm_port_debounce_restore(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);
bool bf_pm_port_debounce_adj_chk(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
uint32_t bf_pm_port_num_serdes_per_lane_get(
    bf_dev_id_t dev_id, bf_pal_front_port_handle_t *port_hdl);
#endif
