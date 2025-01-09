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


/*!
 * @file bf_diag_api.h
 * @date
 *
 * Contains definitions of barefoot diag APIs
 *
 */
#ifndef _BF_DIAG_API_H
#define _BF_DIAG_API_H

#include <bf_types/bf_types.h>
#include <tofino/pdfixed/pd_common.h>
#include <mc_mgr/mc_mgr_types.h>

#define BF_DIAG_MAX_PORTS (BF_PIPE_COUNT * 128)
#define BF_DIAG_MAX_VLANS 4096
#define BF_DIAG_TCP_DSTPORT_MIN 0
#define BF_DIAG_TCP_DSTPORT_MAX 65535
#define BF_DIAG_TCP_DSTPORT_MID 32768

#define BF_DIAG_PORT_GROUP_ALL (0x7fff)
#define BF_DIAG_PORT_GROUP_ALLI (0x7fff - 1)
#define BF_DIAG_PORT_GROUP_ALL_MESH (0x7fff - 2)

typedef uint32_t bf_diag_sess_hdl_t;

/* Diag loopback mode */
typedef enum {
  BF_DIAG_PORT_LPBK_NONE = 0,
  BF_DIAG_PORT_LPBK_MAC,
  BF_DIAG_PORT_LPBK_PHY,
  BF_DIAG_PORT_LPBK_EXT,
  BF_DIAG_PORT_LPBK_PCS,
  /* pipe loopback is only supported on tofino2 */
  BF_DIAG_PORT_LPBK_PIPE,
} bf_diag_port_lpbk_mode_e;

/* Diag test result */
typedef enum {
  BF_DIAG_TEST_STATUS_UNKNOWN = 0,
  BF_DIAG_TEST_STATUS_FAIL,
  BF_DIAG_TEST_STATUS_PASS,
  BF_DIAG_TEST_STATUS_IN_PROGRESS,
} bf_diag_test_status_e;

typedef struct bf_diag_loopback_pair_port_ {
  uint32_t tx;
  uint32_t rx;
} bf_diag_loopback_pair_port_t;

#define BF_DIAG_MAX_LOOPBACK_TEST_PAIRS (BF_DIAG_MAX_PORTS / 2 + 1)
typedef struct bf_diag_loopback_pair_ {
  bf_dev_port_t port1;
  bf_dev_port_t port2;
  bf_diag_loopback_pair_port_t port_stats[2];
  uint32_t tx_total;
  uint32_t rx_total;
  uint32_t rx_good;
  uint32_t rx_bad;
  bf_diag_test_status_e test_status;
} bf_diag_loopback_pair_t;

typedef struct bf_diag_loopback_pair_test_stats_ {
  uint32_t num_pairs;
  bf_diag_loopback_pair_t pairs[BF_DIAG_MAX_LOOPBACK_TEST_PAIRS];
  uint64_t total_bytes_with_bit_flip_detected;
  uint64_t total_bits_with_bit_flip_detected;
  uint64_t total_1_to_0_flips;
  uint64_t total_0_to_1_flips;
  uint64_t total_weak_suspect_for_setup;
  uint64_t total_strong_suspect_for_setup;
  uint64_t total_weak_suspect_for_hold;
  uint64_t total_strong_suspect_for_hold;
  uint64_t total_unknown_failures;
  uint64_t total_payload_setup_failures;
  uint64_t total_mixed_failures;
} bf_diag_loopback_pair_test_stats_t;

typedef struct bf_diag_port_stats_ {
  uint32_t tx_total;
  uint32_t rx_total;
  uint32_t rx_good;
  uint32_t rx_bad;
  uint32_t rx_origin;  // Rx count of pkts that originated from this port
  uint64_t total_bytes_with_bit_flip_detected;
  uint64_t total_bits_with_bit_flip_detected;
  uint64_t total_1_to_0_flips;
  uint64_t total_0_to_1_flips;
  uint64_t total_weak_suspect_for_setup;
  uint64_t total_strong_suspect_for_setup;
  uint64_t total_weak_suspect_for_hold;
  uint64_t total_strong_suspect_for_hold;
  uint64_t total_unknown_failures;
  uint64_t total_payload_setup_failures;
  uint64_t total_mixed_failures;
} bf_diag_port_stats_t;

typedef enum {
  BF_DIAG_DATA_PATTERN_RANDOM = 0,
  BF_DIAG_DATA_PATTERN_FIXED,
} bf_diag_data_pattern_t;

typedef enum {
  BF_DIAG_PACKET_PAYLOAD_RANDOM = 0,
  BF_DIAG_PACKET_PAYLOAD_FIXED,
  /* 16 bytes of random data followed by 16 bytes of the flipped random data */
  BF_DIAG_PACKET_PAYLOAD_RANDOM_FLIP,
} bf_diag_packet_payload_t;

typedef enum {
  BF_DIAG_PACKET_FULL_RANDOM = 0,
  BF_DIAG_PACKET_FULL_FIXED,
} bf_diag_packet_full_t;

/* --- Vlan commands --- */
bf_status_t bf_diag_vlan_create(bf_dev_id_t dev_id, int vlan_id);
bf_status_t bf_diag_vlan_get_first(bf_dev_id_t dev_id, int *vlan_id);
bf_status_t bf_diag_vlan_get_next(bf_dev_id_t dev_id,
                                  int vlan_id,
                                  int num_entries,
                                  int *next_vlan_ids);
bf_status_t bf_diag_port_vlan_add(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  int vlan_id);
bf_status_t bf_diag_port_vlan_del(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  int vlan_id);
bf_status_t bf_diag_vlan_port_get_first(bf_dev_id_t dev_id,
                                        int vlan_id,
                                        bf_dev_port_t *first_port);
bf_status_t bf_diag_vlan_port_get_next(bf_dev_id_t dev_id,
                                       int vlan_id,
                                       bf_dev_port_t port,
                                       int num_entries,
                                       bf_dev_port_t *next_ports);
bf_status_t bf_diag_port_default_vlan_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          int vlan_id);
bf_status_t bf_diag_port_default_vlan_reset(bf_dev_id_t dev_id,
                                            bf_dev_port_t port);
bf_status_t bf_diag_port_default_vlan_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          int *vlan);
bf_status_t bf_diag_default_vlan_port_get_first(bf_dev_id_t dev_id,
                                                int vlan_id,
                                                bf_dev_port_t *first_port);
bf_status_t bf_diag_default_vlan_port_get_next(bf_dev_id_t dev_id,
                                               int vlan_id,
                                               bf_dev_port_t port,
                                               int num_entries,
                                               bf_dev_port_t *next_ports);
bf_status_t bf_diag_vlan_destroy(bf_dev_id_t dev_id, int vlan_id);

/* ----- Mac aging ----- */
bf_status_t bf_diag_mac_aging_set(bf_dev_id_t dev_id, uint32_t ttl_in_msec);
bf_status_t bf_diag_mac_aging_reset(bf_dev_id_t dev_id);
bf_status_t bf_diag_mac_aging_get(bf_dev_id_t dev_id, uint32_t *ttl_in_msec);
bf_status_t bf_diag_learning_timeout_set(bf_dev_id_t dev_id,
                                         uint32_t timeout_usec);

/* --- Forwarding rules --- */
bf_status_t bf_diag_forwarding_rule_add(bf_dev_id_t dev_id,
                                        bf_dev_port_t ig_port,
                                        bf_dev_port_t eg_port,
                                        uint32_t tcp_dstPort_start,
                                        uint32_t tcp_dstPort_end,
                                        int priority,
                                        p4_pd_entry_hdl_t *entry_hdl);
bf_status_t bf_diag_mc_forwarding_rule_add(bf_dev_id_t dev_id,
                                           bf_dev_port_t ig_port,
                                           bf_mc_grp_id_t mc_grp_id_a,
                                           bf_mc_grp_id_t mc_grp_id_b,
                                           uint32_t tcp_dstPort_start,
                                           uint32_t tcp_dstPort_end,
                                           int priority,
                                           p4_pd_entry_hdl_t *entry_hdl);
bf_status_t bf_diag_forwarding_rule_del(bf_dev_id_t dev_id,
                                        p4_pd_entry_hdl_t entry_hdl);

/* Port loopback mode set */
bf_status_t bf_diag_port_loopback_mode_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_diag_port_lpbk_mode_e diag_loop_mode);

/* Port loopback mode get */
bf_status_t bf_diag_port_loopback_mode_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_diag_port_lpbk_mode_e *diag_loop_mode);

/* --- Loopback test --- */
bf_status_t bf_diag_loopback_test_setup(bf_dev_id_t dev_id,
                                        bf_dev_port_t *port_list,
                                        int num_ports,
                                        bf_diag_port_lpbk_mode_e diag_loop_mode,
                                        bf_diag_sess_hdl_t *sess_hdl);
bf_status_t bf_diag_loopback_test_start(bf_diag_sess_hdl_t sess_hdl,
                                        uint32_t num_packet,
                                        uint32_t pkt_size);
bf_status_t bf_diag_loopback_test_abort(bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_loopback_test_status_get(
    bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_loopback_test_port_status_get(
    bf_diag_sess_hdl_t sess_hdl,
    bf_dev_port_t port,
    bf_diag_port_stats_t *stats);
bf_status_t bf_diag_loopback_test_cleanup(bf_diag_sess_hdl_t sess_hdl);

/* --- Snake test --- */
bf_status_t bf_diag_loopback_snake_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl);
bf_status_t bf_diag_loopback_snake_test_start(bf_diag_sess_hdl_t sess_hdl,
                                              uint32_t num_packet,
                                              uint32_t pkt_size,
                                              bool bidir);
bf_status_t bf_diag_loopback_snake_test_stop(bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_loopback_snake_test_status_get(
    bf_diag_sess_hdl_t sess_hdl, bf_diag_port_stats_t *stats);
bf_status_t bf_diag_loopback_snake_test_cleanup(bf_diag_sess_hdl_t sess_hdl);

/* --- Pair test --- */
bf_status_t bf_diag_loopback_pair_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl);
bf_status_t bf_diag_loopback_pair_test_start(bf_diag_sess_hdl_t sess_hdl,
                                             uint32_t num_packet,
                                             uint32_t pkt_size,
                                             bool bidir);
bf_status_t bf_diag_loopback_pair_test_stop(bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_loopback_pair_test_status_get(
    bf_diag_sess_hdl_t sess_hdl, bf_diag_loopback_pair_test_stats_t *stats);
bf_status_t bf_diag_loopback_pair_test_cleanup(bf_diag_sess_hdl_t sess_hdl);

/* --- Multicast loopback test --- */
bf_status_t bf_diag_multicast_loopback_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl);
bf_status_t bf_diag_multicast_loopback_test_start(bf_diag_sess_hdl_t sess_hdl,
                                                  uint32_t num_packet,
                                                  uint32_t pkt_size);
bf_status_t bf_diag_multicast_loopback_test_stop(bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_multicast_loopback_test_status_get(
    bf_diag_sess_hdl_t sess_hdl);
bf_diag_test_status_e bf_diag_multicast_loopback_test_port_status_get(
    bf_diag_sess_hdl_t sess_hdl,
    bf_dev_port_t port,
    bf_diag_port_stats_t *stats);
bf_status_t bf_diag_multicast_loopback_test_cleanup(
    bf_diag_sess_hdl_t sess_hdl);

/*  ---- Session APIs ---- */
bool bf_diag_session_valid(bf_diag_sess_hdl_t sess_hdl);

/*  ---- CPU APIs ---- */
/* Packet inject */
bf_status_t bf_diag_packet_inject_from_cpu(bf_dev_id_t dev_id,
                                           bf_dev_port_t *port_list,
                                           int num_ports,
                                           uint32_t num_packet,
                                           uint32_t pkt_size);
bf_status_t bf_diag_cpu_port_get(bf_dev_id_t dev_id, bf_dev_port_t *cpu_port);
bf_status_t bf_diag_cpu_stats_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  bf_diag_port_stats_t *stats);
bf_status_t bf_diag_cpu_stats_clear(bf_dev_id_t dev_id,
                                    bf_dev_port_t port,
                                    bool all_ports);

/*  ---- Version APIs ---- */
const char *bf_diag_get_version();
const char *bf_diag_get_internal_version();

/* ---- Data Pattern ---- */
bf_status_t bf_diag_data_pattern_set(bf_diag_sess_hdl_t sess_hdl,
                                     bf_diag_data_pattern_t mode,
                                     uint8_t start_pat,
                                     uint32_t start_pat_len,
                                     uint8_t pat_a,
                                     uint8_t pat_b,
                                     uint32_t pattern_len);
/* ---- Packet payload ---- */
bf_status_t bf_diag_packet_payload_set(bf_diag_sess_hdl_t sess_hdl,
                                       bf_diag_packet_payload_t mode,
                                       char *payload_str,
                                       char *payload_file_path);
/* ---- Packet full ---- */
bf_status_t bf_diag_packet_full_set(bf_diag_sess_hdl_t sess_hdl,
                                    bf_diag_packet_full_t mode,
                                    char *pkt_str,
                                    char *pkt_file_path);
/* ---- Change max allowed sessions ----- */
bf_status_t bf_diag_sessions_max_set(bf_dev_id_t dev_id, uint32_t max_sessions);

/* Allow min pkt size of 64 for diag pkts */
bf_status_t bf_diag_min_packet_size_enable(bool enable);

/* --- Stream APIs start ------ */
bf_status_t bf_diag_stream_setup(bf_dev_id_t dev_id,
                                 uint32_t num_pkts,
                                 uint32_t pkt_size,
                                 bf_dev_port_t dst_port,
                                 bf_diag_sess_hdl_t *sess_hdl);
bf_status_t bf_diag_stream_start(bf_diag_sess_hdl_t sess_hdl);
bf_status_t bf_diag_stream_adjust(bf_diag_sess_hdl_t sess_hdl,
                                  uint32_t num_pkts,
                                  uint32_t pkt_size);
bf_status_t bf_diag_stream_stop(bf_diag_sess_hdl_t sess_hdl);
bf_status_t bf_diag_stream_counter_get(bf_diag_sess_hdl_t sess_hdl,
                                       uint64_t *counter);
bf_status_t bf_diag_stream_cleanup(bf_diag_sess_hdl_t sess_hdl);
/* --- Stream APIs end ------ */
bf_status_t bf_diag_gfm_pattern(bf_dev_id_t dev_id, int mode);

#endif
