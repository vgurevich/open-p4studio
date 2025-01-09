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


#ifndef S3_BFDD_H__
#define S3_BFDD_H__

#include <pthread.h>

#include "bf_switch/bf_switch_types.h"
#include "bfd_timer.h"
#include "switch_utils.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** BFD hashtable random seed value **/
#define SWITCH_BFD_HASH_SEED 0x123456

/** BFD hashtable entry hash key size **/
#define SWITCH_BFD_HASH_KEY_SIZE sizeof(switch_ip_address_t) * 2

/** BFD hash table size **/
#define SWITCH_BFD_TABLE_SIZE 4096

#define SWITCH_BFD_VERSION 1
#define SWITCH_BFD_PKT_LEN 24
#define SWITCH_BFD_GET_VER(ver_diag) ((ver_diag >> 5) & 0x03)
#define SWITCH_BFD_GET_DIAG(ver_diag) (ver_diag & 0x01f)
#define SWITCH_BFD_GET_STATE(flags) ((flags >> 6) & 0x3)
#define SWITCH_BFD_GET_POLL(flags) ((flags >> 5) & 0x1)
#define SWITCH_BFD_GET_FINAL(flags) ((flags >> 4) & 0x1)
#define SWITCH_BFD_SET_VER(arg, val) ((arg) |= (val & 0x3) << 5)
#define SWITCH_BFD_SET_STATE(arg, val) ((arg) |= (val & 0x3) << 6)
#define SWITCH_BFD_SET_POLL(arg, val) ((arg) |= (val & 0x1) << 5)
#define SWITCH_BFD_SET_FINAL(arg, val) ((arg) |= (val & 0x1) << 4)
#define SWITCH_BFD_DESIRED_MIN_TX_INTVL (1000 * 1000)   // microseconds
#define SWITCH_BFD_REQUIRED_MIN_TX_INTVL (1000 * 1000)  // microseconds
#define SWITCH_BFD_UDP_DSTPORT 3784
#define SWITCH_BFD_SRC_PORT_START 49152
#define SWITCH_BFD_SRC_PORT_MAX 65535
#define SWITCH_BFD_DIAG_MASK 0x1F

// From the RFC, the applied jitter is between 0-25% of the interval and average
// interval is 12.5% less than negotiated interval
#define BFD_TX_JITTER 0.875

/** BFD Diagnostics **/
typedef enum switch_bfd_diag_e {
  SWITCH_BFD_DIAG_NO_DIAG = 0,
  SWITCH_BFD_DIAG_CTRL_DET_TIMER_EXPIRED = 1,
  SWITCH_BFD_DIAG_ECHO_FUNC_FAILED = 2,
  SWITCH_BFD_DIAG_NEIGH_SIG_SESS_DOWN = 3,
  SWITCH_BFD_DIAG_FWD_PLANE_RESET = 4,
  SWITCH_BFD_DIAG_PATH_DOWN = 5,
  SWITCH_BFD_DIAG_CONCAT_PATH_DOWN = 6,
  SWITCH_BFD_DIAG_ADMIN_DOWN = 7,
  SWITCH_BFD_DIAG_REV_CONCAT_PATH_DOWN = 8
} switch_bfd_diag_t;

/** BFD states **/
typedef enum switch_bfd_state_e {
  SWITCH_BFD_ADMIN_DOWN,
  SWITCH_BFD_DOWN,
  SWITCH_BFD_INIT,
  SWITCH_BFD_UP
} switch_bfd_state_t;

/** bfd hash key **/
typedef struct switch_bfd_key_s {
  switch_ip_address_t local_ip;
  switch_ip_address_t peer_ip;
} switch_bfd_key_t;

typedef struct switch_bfd_discrs_s {
  uint32_t local_discr;
  uint32_t remote_discr;
} switch_bfd_discrs_t;

// below timers are in microseconds
typedef struct switch_bfd_timers_s {
  uint32_t desired_min_tx_intvl;
  uint32_t required_min_rx_intvl;
  uint32_t required_min_echo_intvl;
} switch_bfd_timers_t;

/** bfd session parameters **/
typedef struct switch_bfd_session_s {
  /**
   * bfd_key shall act as a key for bfd hashtable.
   * This should be the first entry in this
   * struct for hashing
   */
  switch_bfd_key_t bfd_key;
  switch_bfd_state_t session_state;
  switch_hashnode_t node;
  uint8_t local_diag;
  uint8_t remote_diag;
  uint8_t detect_mult;
  switch_bfd_discrs_t discriminators;
  switch_bfd_timers_t local_timers;
  bfd_timer_t sys_tx_timer;
  bfd_timer_t sys_rx_timer;
  bfd_timer_t dummy_timer;
  uint16_t udp_src_port;
  uint64_t session_handle;
  switch_bfd_session_type_t session_type;
  switch_bfd_timers_t remote_timers;
  uint8_t remote_detect_mult;
  uint32_t negotiated_tx_intvl;
  uint32_t negotiated_rx_intvl;
} switch_bfd_session_t;

/* BFD pkt as per rfc */
typedef struct switch_bfd_header_s {
  uint8_t ver_diag;
  uint8_t state;
  uint8_t detect_mult;
  uint8_t length;
  switch_bfd_discrs_t discriminators;
  switch_bfd_timers_t timers;
} switch_bfd_header_t;

typedef struct PACKED switch_ip_header_s {
#if defined(__LITTLE_ENDIAN_BITFIELD)
  uint8_t ihl : 4;
  uint8_t version : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
  uint8_t version : 4;
  uint8_t ihl : 4;
#else
#error "Please fix <asym/byteorder.h>"
#endif
  uint8_t tos;
  uint16_t tot_len;
  uint16_t id;
  uint16_t frag_off;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t checksum;
  uint32_t src_addr;
  uint32_t dst_addr;
} switch_ip_header_t;

typedef struct switch_udp_header_s {
  uint16_t src_port;
  uint16_t dst_port;
  uint16_t len;
  uint16_t checksum;
} switch_udp_header_t;

typedef struct PACKED switch_bfd_pkt_s {
  switch_ethernet_header_t ether_hdr;
  switch_ip_header_t ip_hdr;
  switch_udp_header_t udp_hdr;
  switch_bfd_header_t bfd_hdr;
} switch_bfd_pkt_t;

typedef struct PACKED switch_ip6_header_s {
#if defined(__LITTLE_ENDIAN_BITFIELD)
  uint8_t priority : 4;
  uint8_t version : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
  uint8_t version : 4;
  uint8_t priority : 4;
#else
#error "Please fix <asm/byteorder.h>"
#endif
  uint8_t flow_lbl[3];

  uint16_t payload_len;
  uint8_t nexthdr;
  uint8_t hop_limit;

  uint8_t src_addr[16];
  uint8_t dst_addr[16];
} switch_ip6_header_t;

switch_status_t switch_bfdd_recv_pkt(void *buf,
                                     switch_ip_address_t *local_ip,
                                     switch_ip_address_t *peer_ip);
void switch_bfdd_get_bfd_key(switch_bfd_key_t *bfd_key,
                             switch_ip_address_t *local_ip,
                             switch_ip_address_t *peer_ip);
switch_status_t switch_bfdd_session_down(switch_bfd_session_t *bfd,
                                         switch_bfd_diag_t diag);
switch_bfd_session_t *switch_bfdd_get_bfd_session(switch_bfd_key_t *bfd_key);

void switch_bfdd_create_bfd_pkt(switch_bfd_session_t *bfd,
                                switch_bfd_pkt_t *pkt);

// BMAI object handle
typedef void (*switch_bfdd_session_state_cb)(
    uint64_t session_id, switch_bfdd_session_params_t params);

switch_status_t switch_register_bfdd_session_state_cb(
    switch_bfdd_session_state_cb cb);

/* bfdd context */
typedef struct switch_bfdd_context_s {
  // pktdriver rx filter id
  uint64_t filter_id;

  // session state cb
  switch_bfdd_session_state_cb state_cb;

  // bfdd state machine thread id
  pthread_t bfdd_thread;

  // opaque event loop userdata
  void *userdata;

  // packet trace enable
  bool pkt_trace_enable;

} switch_bfdd_context_t;

#ifdef __cplusplus
}
#endif

#endif  // S3_BFDD_H__
