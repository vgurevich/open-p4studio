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
 * @file diag_common.h
 * @date
 *
 * Contains definitions of diag
 *
 */
#ifndef _DIAG_COMMON_H
#define _DIAG_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <linux/if_packet.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <target-sys/bf_sal/bf_sys_sem.h>
#include <target-utils/map/map.h>
#include <bf_types/bf_types.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include "diags/bf_diag_api.h"

// Temporarily override the format checker so we can indent
// the nested preprocessor directives.
// clang-format off


/* Module header includes */
#if defined(DIAG_POWER_MAX_ENABLE)
  #define DIAG_POWER_ENABLE
#endif

#if defined(DIAG_PHV_PARDE_HOLD_STRESS_ENABLE)
  #define DIAG_SINGLE_STAGE
  #define DIAG_PHV_STRESS_ENABLE
  #define DIAG_PARDE_STRESS_POWER
  #define DIAG_HOLD_STRESS
#endif

#if defined(DIAG_PHV_PARDE_STRESS_ENABLE)
  #define DIAG_SINGLE_STAGE
  #define DIAG_PHV_STRESS_ENABLE
  #define DIAG_PARDE_STRESS_POWER
#endif

#if defined(DIAG_PHV_FLOP_TEST)
  #if DIAG_PHV_FLOP_TEST == 1
    #define DIAG_PHV_FLOP_CONFIG_1
  #elif DIAG_PHV_FLOP_TEST == 2
    #define DIAG_PHV_FLOP_CONFIG_2
  #elif DIAG_PHV_FLOP_TEST == 3
    #define DIAG_PHV_FLOP_CONFIG_3
  #elif DIAG_PHV_FLOP_TEST == 4
    #define DIAG_PHV_FLOP_CONFIG_4
  #elif DIAG_PHV_FLOP_TEST == 5
    #define DIAG_PHV_FLOP_CONFIG_5
  #else
    #error "DIAG_PHV_FLOP_TEST must equal 1,2,3,4 or 5"
  #endif
#elif defined(DIAG_PHV_FLOP_MATCH)
  #define DIAG_PHV_FLOP_TEST
  #if DIAG_PHV_FLOP_MATCH == 1
    #define DIAG_PHV_FLOP_CONFIG_1
  #elif DIAG_PHV_FLOP_MATCH == 2
    #define DIAG_PHV_FLOP_CONFIG_2
  #else
    #error "DIAG_PHV_FLOP_MATCH must equal 1 or 2"
  #endif
#endif

// Reenable format checking.
// clang-format on

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE) && \
    !defined(DIAG_PARDE_STRAIN) && !defined(DIAG_PHV_FLOP_TEST)
#define DIAG_ADVANCED_FEATURES
#endif

#define DIAG_ASSERT assert

#define DIAG_MAX_PKT_SIZE 16000
#define DIAG_MAX_FIXED_PAYLOAD_CNT 64
#define DIAG_UCLI_BUFFER_SIZE 64 * 1024
#define DIAG_DEF_TTL 120000
#define DIAG_MIN_TTL 5000
#define DIAG_MAX_TTL 9000000
#define DIAG_MAC_SIZE 6
#define DIAG_LEARN_TIMEOUT 2000000
#define DIAG_PRINT printf
#define DIAG_MAKE_72_PIPE_PORT(pipe, l_port) (72 * pipe + l_port)
#define DIAG_SNAKE_STOP_MAX_HDLS 500
#define DIAG_MIN_CALC(a, b) ((a < b) ? a : b)
#define DIAG_MAX_CALC(a, b) ((a > b) ? a : b)
#define DIAG_PKT_SEND_BATCH_SIZE 1000
#define DIAG_KERNEL_PKT_SEND_BATCH_SIZE 1000
#define DIAG_PKT_SEND_INTERVAL_US 1000000
#define DIAG_KERNEL_PKT_SEND_INTERVAL_US 3000000
#define DIAG_SNAKE_STOP_TCP_DSTPORT_RANGE 350
#define DIAG_SNAKE_STOP_TCP_DSTPORT_RANGE_ETH 200
#define DIAG_PKT_RCV_INTERVAL_US 1700000
#define DIAG_PAIR_PKT_RCV_INTERVAL_US 120000
#define DIAG_MULTICAST_LOOPBACK_PKT_RCV_INTERVAL_US 140000
#define DIAG_PAIR_KERNEL_PKT_RCV_INTERVAL_US 100000
#define DIAG_SNAKE_STOP_TCP_DSTPORT_MAX 1600
#define DIAG_TEST_MAX_PKT_SIZES 9216
#define DIAG_SESSIONS_MAX_LIMIT 128
#define DIAG_SESSIONS_MAX_DEFAULT 32
#define DIAG_MALLOC bf_sys_malloc
#define DIAG_CALLOC bf_sys_calloc
#define DIAG_FREE bf_sys_free
#define DIAG_MUTEX_LOCK bf_sys_mutex_lock
#define DIAG_MUTEX_UNLOCK bf_sys_mutex_unlock
#define DIAG_MUTEX_INIT bf_sys_mutex_init
#define DIAG_MUTEX_DEINIT bf_sys_mutex_del
#define DIAG_PAIR_TEST_CPU_MAX_PKT 5
#define DIAG_PAIR_BIDIR_TEST_CPU_MAX_PKT 2
#define DIAG_PGEN_APPS_MAX_LIMIT 16
#define DIAG_PKTGEN_PKT_BUF_SIZE (16 * 1024)
#define DIAG_SUBDEV_PIPE_COUNT BF_SUBDEV_PIPE_COUNT
#define DIAG_MC_TEST_LARGE_TREE_CNT 2
#define DIAG_MC_TEST_MAX_COPIES_PER_PORT 500

typedef enum {
  DIAG_SLT_FAILURE_TYPE_S_SETUP = 0,
  DIAG_SLT_FAILURE_TYPE_SS_SETUP = 1,
  DIAG_SLT_FAILURE_TYPE_S_HOLD = 2,
  DIAG_SLT_FAILURE_TYPE_SS_HOLD = 3,
  DIAG_SLT_FAILURE_TYPE_UNKNOWN = 4,  // Even if 1 byte w/ multi bit corruptions
  DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP = 5,
  DIAG_SLT_FAILURE_TYPE_MIXED = 6,
  DIAG_SLT_FAILURE_TYPE_NO_FAILURE = 7,
  DIAG_SLT_FAILURE_TYPE_MAX = 8
} diag_slt_failure_type_e;

typedef enum {
  DIAG_TEST_NONE = 0,
  DIAG_TEST_LOOPBACK,
  DIAG_TEST_SNAKE,
  DIAG_TEST_PAIRED_LOOPBACK,
  DIAG_TEST_MULTICAST_LOOPBACK,
  DIAG_TEST_STREAM
} diag_test_type_e;

/* Linespeed init command params */
typedef struct diag_loopback_test_params_s {
  bf_dev_id_t dev_id;
  bf_diag_sess_hdl_t sess_hdl;
  bf_dev_port_t port_list[BF_DIAG_MAX_PORTS];
  int num_ports;
  bf_diag_port_lpbk_mode_e loop_mode;
  uint32_t pkt_size;
  uint32_t pkt_sizes_sent[DIAG_TEST_MAX_PKT_SIZES];
  uint32_t num_packet;
  uint32_t tcp_dstPort_start;              // for snake test only
  uint32_t tcp_dstPort_end;                // for snake test only
  p4_pd_entry_hdl_t snake_stop_entry_hdl;  // for snake test only
  diag_test_type_e test_type;
  bool bidir;         /* bi-directional traffic */
  bool bidir_sess_en; /* bi-directional traffic run at least once on session */
  bool valid;
} diag_loopback_test_params_t;

/* Multicast info for this port */
typedef struct diag_port_mc_info_s {
  p4_pd_entry_hdl_t mc_loopback_stop_override_hdl;
  bf_mc_grp_id_t mc_grp_id_a;                      // Group-id a
  bf_mc_grp_id_t mc_grp_id_b;                      // Group-id b
  bf_mc_mgrp_hdl_t mgrp_hdl_a;                     // multicast group hdl a
  bf_mc_mgrp_hdl_t mgrp_hdl_b;                     // multicast group hdl b
  bf_mc_node_hdl_t node_hdl_a;                     // l2 node hdl
  bf_mc_node_hdl_t node_hdl_b[BF_DIAG_MAX_PORTS];  // l2 node hdl
  uint32_t lag_id;                                 // LAG ID
  bf_mc_ecmp_hdl_t ecmp_hdl;                       // ecmp hdl
  bf_mc_node_hdl_t ecmp_node_hdl;                  // ecmp node
  /* l2 node hdl for max copies to be made per port */
  bf_mc_node_hdl_t copies_node_hdl_b[DIAG_MC_TEST_MAX_COPIES_PER_PORT];
} diag_port_mc_info_t;

/* Port info */
typedef struct diag_port_s {
  int default_vlan;
  p4_pd_entry_hdl_t def_vlan_entry_hdl;
  p4_pd_entry_hdl_t from_src_override_hdl[DIAG_SESSIONS_MAX_LIMIT];
  p4_pd_entry_hdl_t to_dst_override_hdl[DIAG_SESSIONS_MAX_LIMIT];
  p4_pd_entry_hdl_t rev_from_src_override_hdl[DIAG_SESSIONS_MAX_LIMIT];
  p4_pd_entry_hdl_t rev_to_dst_override_hdl[DIAG_SESSIONS_MAX_LIMIT];
  p4_pd_entry_hdl_t
      pair_stop_override_hdl[DIAG_SESSIONS_MAX_LIMIT];  // for pair test only
  p4_pd_entry_hdl_t
      pair_stop_override_hdl2[DIAG_SESSIONS_MAX_LIMIT];  // for pair test only
  // Multicast info
  diag_port_mc_info_t mc[DIAG_SESSIONS_MAX_LIMIT];
  bf_diag_port_lpbk_mode_e loopback_mode;
} diag_port_t;

typedef struct diag_vlan_port_s {
  bool valid;
  p4_pd_entry_hdl_t dmac_entry_hdl;
  p4_pd_entry_hdl_t port_vlan_entry_hdl;
} diag_vlan_port_t;

/* Vlan info */
typedef struct diag_vlan_s {
  bool valid;
  int mc_index;
  bf_mc_rid_t rid;
  diag_vlan_port_t untagged[BF_DIAG_MAX_PORTS];  // untagged
  diag_vlan_port_t tagged[BF_DIAG_MAX_PORTS];    // tagged
  p4_pd_entry_hdl_t bd_flood_entry_hdl;
  p4_pd_entry_hdl_t mc_grp_hdl;
  p4_pd_entry_hdl_t mc_node_hdl;
} diag_vlan_t;

/* Packet-gen params */
typedef struct diag_pgen_params_s {
  bf_dev_id_t dev_id;
  uint32_t app_id;
  bf_dev_pipe_t pipe;
  bf_dev_port_t pktgen_port;
  bf_dev_port_t src_port;
  bf_dev_port_t dst_port;
  p4_pd_sess_hdl_t fwd_hdl;
  uint32_t pkt_buf_offset;
  uint32_t pkt_size;
  uint32_t aligned_pkt_size;
  uint32_t num_pkts;
  uint32_t timer_nsec;
  bool enabled;
} diag_pgen_params_t;

typedef struct diag_session_info_s {
  bf_diag_sess_hdl_t sess_hdl;
  bf_dev_id_t dev_id;
  diag_loopback_test_params_t loop_params; /* Loop test params */
  bool use_fixed_pattern;
  uint8_t start_pattern; /* Start data pattern */
  uint32_t start_pattern_len;
  uint8_t data_pattern_a;
  uint8_t data_pattern_b;
  uint32_t pattern_len;
  bool use_fixed_pkt_contents;
  bf_diag_packet_payload_t payload_type;
  char *fixed_payload[DIAG_MAX_FIXED_PAYLOAD_CNT];
  uint32_t fixed_payload_cnt;
  bf_diag_packet_full_t pkt_full_type;
  char *pkt_full;
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
  uint64_t pkt_id_corrupt_cnt;
  diag_pgen_params_t pgen_params; /* Pkt-gen params */
  bf_sys_mutex_t session_mtx;
} diag_session_info_t;

typedef struct diag_session_s {
  uint8_t sess_hdls[DIAG_SESSIONS_MAX_LIMIT];
  uint32_t num_hdls_alloced;
  bf_map_t sess_map;
} diag_session_t;

#define DIAG_ETH_CPU_PORT_NAME_LEN 200
typedef struct diag_eth_cpu_info_s {
  int eth_cpu_port;
  bool use_eth_cpu_port;  // comes from bf-switchd which reads the conf file
  char eth_cpu_port_name[DIAG_ETH_CPU_PORT_NAME_LEN];
  uint32_t cpu_ifindex;
  int cpu_fd;
  struct sockaddr_ll s_addr;
  pthread_t eth_cpu_driver_thread;
} diag_eth_cpu_info_t;

/* Device info */
typedef struct diag_dev_info_s {
  bf_dev_id_t dev_id;
  bf_dev_family_t chip_family;
  int num_active_pipes;
  int cpu_port;
  int cpu_port2;
  diag_eth_cpu_info_t eth_cpu_info;
  bf_sku_chip_part_rev_t part_rev;
  /* State info */
  bf_diag_port_stats_t cpu_stats[BF_DIAG_MAX_PORTS + 1]
                                [DIAG_SESSIONS_MAX_LIMIT]; /* cpu stats */
  diag_vlan_t vlans[BF_DIAG_MAX_VLANS + 1];                /* vlan info */
  diag_port_t ports[BF_DIAG_MAX_PORTS + 1];                /* port info */
  p4_pd_sess_hdl_t learn_sess_hdl;
  uint32_t mac_aging_ttl;  // in milli-sec
  uint32_t lrn_timeout;    // in micro-sec
  uint32_t tx_pkt_completions;
  bool kernel_pkt;
  diag_session_t session;
  uint64_t pkt_id_corrupt_cnt; /* Number of pkts rcvd with corrupt pkt-id */
  /* Drain all pkts (entire tcp range, 0-65535) in pair-stop */
  bool drain_full_tcp_port_range;
  bool is_sw_model;
  bool pkt_rx_disabled; /* Do not process any pkts in the rx path */
  /* Track usage of pktgen apps */
  uint8_t pgen_app_used[BF_PIPE_COUNT][DIAG_PGEN_APPS_MAX_LIMIT];
  uint32_t num_pgen_app_used[BF_PIPE_COUNT];
  uint32_t pgen_global_pkt_buf_offset[BF_PIPE_COUNT];
} diag_dev_info_t;

/* Diag info */
typedef struct diag_info_s {
  diag_dev_info_t *info[BF_MAX_DEV_COUNT];
  int pkt_id_replication_factor; /* pkt id replication count */
  bool min_pkt_size;             /* Allow user to send 64 byte packets */
} diag_info_t;

extern diag_info_t diag_info;
extern uint32_t diag_sessions_current_max;
#define DIAG_DEV_INFO(_dev) (diag_info.info[_dev])
#define DIAG_DEV_VALID(_dev) \
  ((_dev < BF_MAX_DEV_COUNT) && (diag_info.info[_dev]))
#define DIAG_SESSION_INFO(_dev) (diag_info.info[_dev]->session)
#define DIAG_PGEN_APP_USED(_dev, _pipe, _app_id) \
  (diag_info.info[_dev]->pgen_app_used[_pipe][_app_id])
#define DIAG_PGEN_APP_USED_CNT(_dev, _pipe) \
  (diag_info.info[_dev]->num_pgen_app_used[_pipe])

#define DIAG_PIPE_PORT_VALID(_dev, _pipe, _port)      \
  ((_pipe < DIAG_DEV_INFO(_dev)->num_active_pipes) && \
   (_port < BF_PIPE_PORT_COUNT))

#define DIAG_DEVPORT_VALID(_dev, _p)                     \
  ((DIAG_DEV_VALID(_dev)) && (_p < BF_DIAG_MAX_PORTS) && \
   (DIAG_PIPE_PORT_VALID(                                \
       _dev, DEV_PORT_TO_PIPE(_p), DEV_PORT_TO_LOCAL_PORT(_p))))
#define DIAG_DEV_IS_SW_MODEL(_dev) (diag_info.info[_dev]->is_sw_model)

#define DIAG_GET_PGEN_PARAMS(_sess_info) (_sess_info->pgen_params)
#define DIAG_PGEN_PARAMS_CLEAR(_sess_info) \
  memset(&(_sess_info->pgen_params), 0, sizeof(diag_pgen_params_t))
#define DIAG_GET_LOOPBACK_PARAMS(_sess_info) (_sess_info->loop_params)
#define DIAG_LOOPBACK_PARAMS_CLEAR(_sess_info) \
  memset(&(_sess_info->loop_params), 0, sizeof(diag_loopback_test_params_t))
#define DIAG_GET_CPU_STATS(_dev, _port, _sess_hdl) \
  (DIAG_DEV_INFO(_dev)->cpu_stats[_port][_sess_hdl])
#define DIAG_PKT_ID_CORRUPT_CNT(_dev) (DIAG_DEV_INFO(_dev)->pkt_id_corrupt_cnt)
#define DIAG_DRAIN_FULL_TCP_PORT_RANGE(_dev) \
  (DIAG_DEV_INFO(_dev)->drain_full_tcp_port_range)
#define DIAG_TX_PKT_COMPLETIONS(_dev) (DIAG_DEV_INFO(_dev)->tx_pkt_completions)
/* session tcp values */
#define DIAG_SESS_DSTPORT_RANGE \
  (BF_DIAG_TCP_DSTPORT_MID / diag_sessions_current_max)
#define DIAG_SESS_TCP_DSTPORT_START(_sess_hdl) \
  ((DIAG_SESS_HANDLE_TO_HDL(_sess_hdl) - 1) * DIAG_SESS_DSTPORT_RANGE)
#define DIAG_SESS_TCP_DSTPORT_END(_sess_hdl) \
  (DIAG_SESS_TCP_DSTPORT_START(_sess_hdl) + DIAG_SESS_DSTPORT_RANGE - 1)
#define DIAG_SESS_TCP_DSTPORT_REV_START(_sess_hdl)                        \
  (((DIAG_SESS_HANDLE_TO_HDL(_sess_hdl) - 1) * DIAG_SESS_DSTPORT_RANGE) + \
   BF_DIAG_TCP_DSTPORT_MID)
#define DIAG_SESS_TCP_DSTPORT_REV_END(_sess_hdl) \
  (DIAG_SESS_TCP_DSTPORT_REV_START(_sess_hdl) + DIAG_SESS_DSTPORT_RANGE - 1)

#define DIAG_PORT_DSTPORT_RANGE \
  ((BF_DIAG_TCP_DSTPORT_MAX + 1) / BF_DIAG_MAX_PORTS)
#define DIAG_PORT_DSTPORT_SESS_RANGE \
  (DIAG_PORT_DSTPORT_RANGE / diag_sessions_current_max)
#define DIAG_PORT_TCP_DSTPORT_START(_port, _sess_hdl) \
  ((_port * DIAG_PORT_DSTPORT_RANGE) +                \
   ((_sess_hdl - 1) * DIAG_PORT_DSTPORT_SESS_RANGE))
#define DIAG_PORT_TCP_DSTPORT_END(_port, _sess_hdl) \
  (DIAG_PORT_TCP_DSTPORT_START(_port, _sess_hdl) +  \
   DIAG_PORT_DSTPORT_SESS_RANGE - 1)

#define DIAG_PKT_RX_PROCESSING_DISABLED(_dev) \
  (DIAG_DEV_INFO(_dev)->pkt_rx_disabled)
#define DIAG_MIN_PKT_SIZE_ENABLED (diag_info.min_pkt_size)
#define DIAG_PKT_ID_REPLICATION_FACTOR (diag_info.pkt_id_replication_factor)
#define DIAG_RANDOM_PKT_SIZE_VAL 1

#endif
