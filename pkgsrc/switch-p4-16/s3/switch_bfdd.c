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


#include <arpa/inet.h>

#include <unistd.h>
#include "s3/switch_bfdd.h"
#include "s3/switch_packet.h"
#include "switch_utils.h"
#include "bfd_timer.h"
#include "bfdd.h"

#ifdef TESTING
#define SWITCH_BFD_ERROR(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_BFD_WARN(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_BFD_INFO(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_BFD_DEBUG(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_BFD_DUMP(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_BFD_ENTER() fprintf(stderr, "%s: Enter\n", __FUNCTION__);
#else
#define SWITCH_BFD_ERROR(fmt, arg...)    \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_ERR,       \
                       "%s:%d: " fmt,    \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_BFD_WARN(fmt, arg...)     \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_WARN,      \
                       "%s:%d: " fmt,    \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_BFD_INFO(fmt, arg...)     \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_INFO,      \
                       "%s:%d " fmt,     \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_BFD_DEBUG(fmt, arg...)    \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_DBG,       \
                       "%s:%d " fmt,     \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_BFD_DUMP(fmt, arg...) \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, BF_LOG_INFO, fmt, ##arg)
#define SWITCH_BFD_ENTER() \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, BF_LOG_DBG, "%s: Enter", __FUNCTION__)
#endif

static uint32_t switch_bfdd_slow_tx_intvl(switch_bfd_session_t *bfd) {
  if (bfd->local_timers.desired_min_tx_intvl < 1000000) {
    return 1000000;
  }
  return bfd->local_timers.desired_min_tx_intvl;
}

static inline char *switch_bfd_diag_to_string(switch_bfd_diag_t bfd_diag) {
  switch (bfd_diag) {
    case SWITCH_BFD_DIAG_NO_DIAG:
      return "No Diagnostic";
      break;
    case SWITCH_BFD_DIAG_CTRL_DET_TIMER_EXPIRED:
      return "Control Detection Time Expired";
      break;
    case SWITCH_BFD_DIAG_ECHO_FUNC_FAILED:
      return "Echo Function Failed";
      break;
    case SWITCH_BFD_DIAG_NEIGH_SIG_SESS_DOWN:
      return "Neighbor Signaled Session Down";
      break;
    case SWITCH_BFD_DIAG_FWD_PLANE_RESET:
      return "Forwarding Plane Reset";
      break;
    case SWITCH_BFD_DIAG_PATH_DOWN:
      return "Path Down";
      break;
    case SWITCH_BFD_DIAG_CONCAT_PATH_DOWN:
      return "Concatenated Path Down";
      break;
    case SWITCH_BFD_DIAG_ADMIN_DOWN:
      return "Administratively Down";
      break;
    case SWITCH_BFD_DIAG_REV_CONCAT_PATH_DOWN:
      return "Reverse Concatenated Path Down";
      break;
    default:
      return "Reserved or unsupported";
      break;
  }
}

static inline char *switch_bfd_state_to_string(switch_bfd_state_t state) {
  switch (state) {
    case SWITCH_BFD_ADMIN_DOWN:
      return "AdminDown";
    case SWITCH_BFD_DOWN:
      return "Down";
    case SWITCH_BFD_INIT:
      return "Init";
    case SWITCH_BFD_UP:
      return "Up";
    default:
      return "Invalid state";
  }
}

// TBD move this into bfdd_ctx
switch_hashtable_t bfd_hashtable;

switch_bfdd_context_t *bfdd_ctx = NULL;
switch_status_t switch_bfdd_timer_handler(switch_bfd_session_t *bfd);

void switch_bfdd_dump_enable(bool enable) {
  if (!bfdd_ctx) return;
  bfdd_ctx->pkt_trace_enable = enable;
}

/*
 * Routine Description:
 *   @brief bfd session entry hash compare
 *
 * Arguments:
 *   @param[in] key1 - hash key
 *   @param[in] key2 - bfd session struct
 *
 * Return Values:
 *    @return 0 if key matches
 *            -1 o +1 depending which key is greater
 */
switch_int32_t switch_bfdd_entry_hash_compare(const void *key1,
                                              const void *key2) {
  return memcmp(key1, key2, SWITCH_BFD_HASH_KEY_SIZE);
}

/*
 * Routine Description:
 *   @brief compute bfd table entry hash
 *
 * Arguments:
 *   @param[in] args - bfd_key
 *   @param[out] key - hash key
 *   @param[out] len - hash key length
 *
 * Return Values:
 *    @return  SWITCH_STATUS_SUCCESS on success
 *             Failure status code on error
 */
switch_status_t switch_bfdd_table_entry_key_init(void *args,
                                                 switch_uint8_t *key,
                                                 switch_uint32_t *len) {
  switch_bfd_key_t *bfd_key = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  SWITCH_ASSERT(args && key && len);
  if (!args || !key || !len) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_BFD_ERROR(
        "bfd hash entry key init failed"
        "invalid parameters(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  *len = 0;
  bfd_key = (switch_bfd_key_t *)args;

  memcpy(key, &bfd_key->local_ip, sizeof(switch_ip_address_t));
  *len += sizeof(switch_ip_address_t);

  memcpy((key + *len), &bfd_key->peer_ip, sizeof(switch_ip_address_t));
  *len += sizeof(switch_ip_address_t);

  SWITCH_ASSERT(*len == SWITCH_BFD_HASH_KEY_SIZE);

  return status;
}

/**
 * Routine Description:
 *   @brief Initialize bfd hash table
 *
 * Return  Values:
 *    @retval SWITCH_STATUS_SUCCESS Initialization success
 *    @retval SWITCH_STATUS_FAILURE failed to Initialize
 */
switch_status_t switch_bfd_hashtable_initialize() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  // Initialize bfd hashtable
  bfd_hashtable.size = SWITCH_BFD_TABLE_SIZE;
  bfd_hashtable.compare_func = switch_bfdd_entry_hash_compare;
  bfd_hashtable.key_func = switch_bfdd_table_entry_key_init;
  bfd_hashtable.hash_seed = SWITCH_BFD_HASH_SEED;

  status = SWITCH_HASHTABLE_INIT(&bfd_hashtable);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("bfd hash table init failed : %s\n",
                     switch_error_to_string(status));
    return status;
  }
  return status;
}

/**
 * Routine Description:
 *   @brief Delete bfd hash table
 *
 * Return Values:
 *   @retval SWITCH_STATUS_SUCCESS Deletion success
 *   @retval SWITCH_STATUS_FAILURE failed to delete
 */
switch_status_t switch_bfd_hashtable_delete() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  status = SWITCH_HASHTABLE_DONE(&bfd_hashtable);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("bfd hashtable done failed(%s)\n",
                     switch_error_to_string(status));
  }
  return status;
}

void switch_bfdd_packet_dump(void *pkt, bool tx) {
  switch_bfd_pkt_t *in_pkt = (switch_bfd_pkt_t *)pkt;

  if (bfdd_ctx && !bfdd_ctx->pkt_trace_enable) {
    return;
  }

  SWITCH_BFD_DUMP("**** BFD packet %s ****", (tx ? "Tx" : "Rx"));
  switch_ethernet_header_t *ether_hdr = &in_pkt->ether_hdr;
  SWITCH_BFD_DUMP(
      "dmac: %02x:%02x:%02x:%02x:%02x:%02x smac: "
      "%02x:%02x:%02x:%02x:%02x:%02x etype: 0x%x",
      ether_hdr->dst_mac[0],
      ether_hdr->dst_mac[1],
      ether_hdr->dst_mac[2],
      ether_hdr->dst_mac[3],
      ether_hdr->dst_mac[4],
      ether_hdr->dst_mac[5],
      ether_hdr->src_mac[0],
      ether_hdr->src_mac[1],
      ether_hdr->src_mac[2],
      ether_hdr->src_mac[3],
      ether_hdr->src_mac[4],
      ether_hdr->src_mac[5],
      ntohs(ether_hdr->ether_type));

  switch_ip_header_t ip_hdr = {};
  memcpy(&ip_hdr, &in_pkt->ip_hdr, 20);
  struct in_addr sip_addr;
  sip_addr.s_addr = ip_hdr.src_addr;
  SWITCH_BFD_DUMP("%s: %s", tx ? "local" : "peer", inet_ntoa(sip_addr));
  struct in_addr dip_addr;
  dip_addr.s_addr = ip_hdr.dst_addr;
  SWITCH_BFD_DUMP("%s: %s", tx ? "peer" : "local", inet_ntoa(dip_addr));

  switch_bfd_header_t bfd_hdr = {};
  memcpy(&bfd_hdr, &in_pkt->bfd_hdr, 24);
  SWITCH_BFD_DUMP(
      "State: %s, Flags: [none], Diagnostic: %s (0x%x)",
      switch_bfd_state_to_string(SWITCH_BFD_GET_STATE(bfd_hdr.state)),
      switch_bfd_diag_to_string(SWITCH_BFD_GET_DIAG(bfd_hdr.ver_diag)),
      SWITCH_BFD_GET_DIAG(bfd_hdr.ver_diag));
  SWITCH_BFD_DUMP("Multi: %d, Length: %d, myDisc: %d, yourDisc: %d",
                  bfd_hdr.detect_mult,
                  bfd_hdr.length,
                  ntohl(bfd_hdr.discriminators.local_discr),
                  ntohl(bfd_hdr.discriminators.remote_discr));
  SWITCH_BFD_DUMP("minTx: %d us, minRx: %d us, EchoRx: %d us\n",
                  ntohl(bfd_hdr.timers.desired_min_tx_intvl),
                  ntohl(bfd_hdr.timers.required_min_rx_intvl),
                  ntohl(bfd_hdr.timers.required_min_echo_intvl));
}

uint16_t compute_udp_checksum(uint8_t *udp_hdr,
                              size_t len,
                              uint32_t src_addr,
                              uint32_t dest_addr) {
  const uint16_t *buf = (const uint16_t *)udp_hdr;
  uint16_t *ip_src = (void *)&src_addr, *ip_dst = (void *)&dest_addr;
  uint32_t sum;
  size_t length = len;

  // Calculate the sum
  sum = 0;
  while (len > 1) {
    sum += *buf++;
    if (sum & 0x80000000) sum = (sum & 0xFFFF) + (sum >> 16);
    len -= 2;
  }

  if (len & 1)
    // Add the padding if the packet lenght is odd
    sum += *((uint8_t *)buf);

  // Add the pseudo-header
  sum += *(ip_src++);
  sum += *ip_src;

  sum += *(ip_dst++);
  sum += *ip_dst;

  sum += htons(IPPROTO_UDP);
  sum += htons(length);

  // Add the carries
  while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);

  // Return the one's complement of sum
  return (uint16_t)~sum;
}

uint16_t compute_ip_checksum(uint8_t *ptr, int size) {
  int cksum = 0;
  int index = 0;
  *(ptr + 10) = 0;
  *(ptr + 11) = 0;
  if (size % 2 != 0) return 0;
  while (index < size) {
    cksum += *(ptr + index + 1);
    cksum += *(ptr + index) << 8;
    index += 2;
  }
  while (cksum > 0xffff) {
    cksum = (cksum >> 16) + (cksum & 0xffff);
  }
  return ~cksum;
}

static void switch_bfdd_create_bfd_pkt_ctlbit(switch_bfd_session_t *bfd,
                                              switch_bfd_pkt_t *pkt,
                                              int poll,
                                              int final) {
  SWITCH_BFD_SET_VER(pkt->bfd_hdr.ver_diag, SWITCH_BFD_VERSION);
  SWITCH_BFD_SET_STATE(pkt->bfd_hdr.state, bfd->session_state);
  pkt->bfd_hdr.ver_diag = pkt->bfd_hdr.ver_diag | bfd->local_diag;

  pkt->bfd_hdr.detect_mult = bfd->detect_mult;
  pkt->bfd_hdr.length = SWITCH_BFD_PKT_LEN;

  pkt->bfd_hdr.discriminators.local_discr =
      htonl(bfd->discriminators.local_discr);
  pkt->bfd_hdr.discriminators.remote_discr =
      htonl(bfd->discriminators.remote_discr);

  if (bfd->session_state == SWITCH_BFD_UP) {
    pkt->bfd_hdr.timers.desired_min_tx_intvl =
        htonl(bfd->local_timers.desired_min_tx_intvl);
  } else {
    pkt->bfd_hdr.timers.desired_min_tx_intvl =
        htonl(switch_bfdd_slow_tx_intvl(bfd));
  }

  pkt->bfd_hdr.timers.required_min_rx_intvl =
      htonl(bfd->local_timers.required_min_rx_intvl);
  pkt->bfd_hdr.timers.required_min_echo_intvl =
      htonl(bfd->local_timers.required_min_echo_intvl);

  SWITCH_BFD_SET_POLL(pkt->bfd_hdr.state, !!poll);
  SWITCH_BFD_SET_FINAL(pkt->bfd_hdr.state, !!final);

  // Ethernet
  // dst 0x000102030405
  // src 0x00ba7ef00000
  pkt->ether_hdr.dst_mac[0] = 0x00;
  pkt->ether_hdr.dst_mac[1] = 0x01;
  pkt->ether_hdr.dst_mac[2] = 0x02;
  pkt->ether_hdr.dst_mac[3] = 0x03;
  pkt->ether_hdr.dst_mac[4] = 0x04;
  pkt->ether_hdr.dst_mac[5] = 0x05;
  pkt->ether_hdr.src_mac[0] = 0x00;
  pkt->ether_hdr.src_mac[1] = 0xba;
  pkt->ether_hdr.src_mac[2] = 0x7e;
  pkt->ether_hdr.src_mac[3] = 0xf0;
  pkt->ether_hdr.src_mac[4] = 0x00;
  pkt->ether_hdr.src_mac[5] = 0x00;
  pkt->ether_hdr.ether_type = htons(0x0800);

  // IP
  pkt->ip_hdr.version = 0x4;
  pkt->ip_hdr.ihl = 0x5;
  pkt->ip_hdr.tos = 0x0;
  // ip(20) + udp(8) + bfd(24)
  pkt->ip_hdr.tot_len = htons(0x34);
  pkt->ip_hdr.id = 0x1;
  pkt->ip_hdr.frag_off = 0x0;
  pkt->ip_hdr.ttl = 0x0;
  pkt->ip_hdr.protocol = 0x11;
  pkt->ip_hdr.dst_addr = htonl(bfd->bfd_key.peer_ip.ip4);
  pkt->ip_hdr.src_addr = htonl(bfd->bfd_key.local_ip.ip4);
  pkt->ip_hdr.checksum =
      htons(compute_ip_checksum((uint8_t *)&pkt->ip_hdr, 20));

  // UDP
  pkt->udp_hdr.src_port = htons(bfd->udp_src_port);
  pkt->udp_hdr.dst_port = htons(SWITCH_BFD_UDP_DSTPORT);
  // udp(8) + bfd(24)
  pkt->udp_hdr.len = htons(0x20);
  uint8_t buf[32];
  memcpy(buf, &pkt->udp_hdr, 32);
  pkt->udp_hdr.checksum = compute_udp_checksum(buf,
                                               32,
                                               htonl(bfd->bfd_key.local_ip.ip4),
                                               htonl(bfd->bfd_key.peer_ip.ip4));

  return;
}

void switch_bfdd_create_bfd_pkt(switch_bfd_session_t *bfd,
                                switch_bfd_pkt_t *pkt) {
  return switch_bfdd_create_bfd_pkt_ctlbit(bfd, pkt, 0, 0);
}

switch_status_t switch_bfdd_send_bfd_pkt_ctlbit(switch_bfd_session_t *bfd,
                                                int poll,
                                                int final) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfd_pkt_t pkt = {};

  switch_bfdd_create_bfd_pkt_ctlbit(bfd, &pkt, poll, final);

  switch_bfdd_packet_dump((char *)&pkt, true);

  status = switch_pkt_xmit((char *)&pkt, sizeof(switch_bfd_pkt_t));
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("switch_pkt_xmit failed \n");
    return status;
  }

  return status;
}

switch_status_t switch_bfdd_send_bfd_pkt(switch_bfd_session_t *bfd) {
  return switch_bfdd_send_bfd_pkt_ctlbit(bfd, 0, 0);
}

static switch_status_t switch_bfdd_send_bfd_pkt_final(
    switch_bfd_session_t *bfd) {
  return switch_bfdd_send_bfd_pkt_ctlbit(bfd, 0, 1);
}

/*
 * Routine Description:
 *   @brief Tx timer call back
 *          This API shall trigger when tx timer expires.
 *          Send the bfd pkt again as tx timer expired.
 *
 * Arguments:
 *   @param[in] args - timer
 *   @param[out] key - Pointer to switch_bfd_session_t
 *
 * Return Values:
 *    @return  void
 */

void switch_bfdd_session_txtimer_cb(bfd_timer_t *timer, void *data) {
  switch_bfd_session_t *bfd = (switch_bfd_session_t *)data;
  (void)timer;

  // txtimer expired, send the pkt.
  // ignore if session is ASYNC_PASSIVE & state is DOWN
  if (!(bfd->session_type == SWITCH_BFD_ASYNC_PASSIVE &&
        bfd->session_state == SWITCH_BFD_DOWN)) {
    switch_bfdd_send_bfd_pkt(bfd);
  }
}

static void bfdd_session_timeout(switch_bfd_session_t *bfd) {
  switch (bfd->session_state) {
    case SWITCH_BFD_INIT:
    case SWITCH_BFD_UP:
      switch_bfdd_session_down(bfd, SWITCH_BFD_DIAG_CTRL_DET_TIMER_EXPIRED);
      break;
    default:
      break;
  }
}

void switch_bfdd_session_dummytimer_cb(bfd_timer_t *timer, void *data) {
  (void)timer;
  (void)data;
}

/*
 * Routine Description:
 *   @brief Rx timer call back
 *          This API shall trigger when rx timer expires.
 *          Timer is vlaid for INIT or UP state.
 *          Move the bfd state to DOWN if current status is INIT/UP
 *
 * Arguments:
 *   @param[in] args - timer
 *   @param[out] key - Pointer to switch_bfd_session_t
 *
 * Return Values:
 *    @return  void
 */
void switch_bfdd_session_rxtimer_cb(bfd_timer_t *timer, void *data) {
  SWITCH_BFD_ENTER();
  (void)timer;
  bfdd_session_timeout(data);
}

switch_status_t switch_bfdd_session_txtimer_update(switch_bfd_session_t *bfd) {
  SWITCH_BFD_ENTER();
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t final_min_tx_ms =
      (uint32_t)(switch_bfdd_slow_tx_intvl(bfd) / 1000) * BFD_TX_JITTER;
  status = bfd_timer_update(&bfd->sys_tx_timer, 0, final_min_tx_ms);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD tx timer update failed \n");
    return status;
  }

  return status;
}

// This timer never repeats. It is always a 1-time event triggered from Init
// If this timer expires, then state changes to Down
// Else, the Up packet from peer will stop this timer
switch_status_t switch_bfdd_session_rxtimer_update(switch_bfd_session_t *bfd) {
  SWITCH_BFD_ENTER();
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  uint32_t final_rx_intvl_ms = bfd->negotiated_rx_intvl / 1000;
  status = bfd_timer_update(&bfd->sys_rx_timer, final_rx_intvl_ms, 0);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD rx timer deletion failed \n");
    return status;
  }

  return status;
}

static switch_bfd_session_t *switch_bfdd_session_alloc(
    switch_ip_address_t local_ip, switch_ip_address_t peer_ip) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfd_session_t *bfd = SWITCH_MALLOC(sizeof(switch_bfd_session_t), 0x01);
  if (!bfd) {
    SWITCH_BFD_ERROR("BFD session alloc failed \n");
    return NULL;
  }

  SWITCH_MEMSET(bfd, 0, sizeof(switch_bfd_session_t));

  bfd->bfd_key.local_ip = local_ip;
  bfd->bfd_key.peer_ip = peer_ip;
  bfd->session_state = SWITCH_BFD_DOWN;
  bfd->discriminators.local_discr = rand() % 4096;
  bfd->detect_mult = 0x3;
  bfd->local_timers.desired_min_tx_intvl = SWITCH_BFD_DESIRED_MIN_TX_INTVL;
  bfd->local_timers.required_min_rx_intvl = SWITCH_BFD_REQUIRED_MIN_TX_INTVL;
  bfd->local_timers.required_min_echo_intvl = 0;
  bfd->remote_timers.required_min_rx_intvl = 1;
  bfd->udp_src_port =
      (rand() % (SWITCH_BFD_SRC_PORT_MAX - SWITCH_BFD_SRC_PORT_START + 1)) +
      SWITCH_BFD_SRC_PORT_START;
  bfd->local_diag = SWITCH_BFD_DIAG_NO_DIAG;

  // update neg_tx same as local min tx as we did not recv any bfd pkt yet
  bfd->negotiated_tx_intvl = bfd->local_timers.desired_min_tx_intvl;

  // Create tx timer, only deleted during session delete
  status = bfd_timer_create(
      &bfd->sys_tx_timer, 1100, 0, switch_bfdd_session_txtimer_cb, (void *)bfd);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("Failed to create tx timer %s",
                     switch_error_to_string(status));
    SWITCH_FREE(bfd);
    return NULL;
  }
  bfd->sys_tx_timer.userdata = bfdd_ctx->userdata;

  // Create rx timer, only deleted during session delete
  status = bfd_timer_create(
      &bfd->sys_rx_timer, 1100, 0, switch_bfdd_session_rxtimer_cb, (void *)bfd);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("Failed to create rx timer %s",
                     switch_error_to_string(status));
    SWITCH_FREE(bfd);
    return NULL;
  }
  bfd->sys_rx_timer.userdata = bfdd_ctx->userdata;

  status = bfd_timer_create(&bfd->dummy_timer,
                            1100,
                            0,
                            switch_bfdd_session_dummytimer_cb,
                            (void *)bfd);
  bfd->dummy_timer.userdata = bfdd_ctx->userdata;
  bfd_timer_update(&bfd->dummy_timer, 100, 500);

  return bfd;
}

static void switch_bfdd_session_free(switch_bfd_session_t *bfd) {
  SWITCH_FREE(bfd);
}

switch_status_t switch_bfdd_session_create(
    uint64_t bfd_session_handle,
    switch_bfd_session_type_t session_type,
    uint32_t local_discriminator,
    uint32_t min_tx,
    uint32_t min_rx,
    uint32_t multiplier,
    uint16_t udp_src_port,
    switch_ip_address_t local_ip,
    switch_ip_address_t peer_ip) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch_bfd_session_t *bfd = switch_bfdd_session_alloc(local_ip, peer_ip);
  if (!bfd) {
    status = SWITCH_STATUS_NO_MEMORY;
    SWITCH_BFD_ERROR("BFD session alloc failed with error:(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  bfd->discriminators.local_discr = local_discriminator;
  bfd->local_timers.desired_min_tx_intvl = min_tx;
  bfd->local_timers.required_min_rx_intvl = min_rx;
  bfd->detect_mult = multiplier;
  bfd->udp_src_port = udp_src_port;
  bfd->session_type = session_type;
  bfd->session_handle = bfd_session_handle;

  status = SWITCH_HASHTABLE_INSERT(
      &bfd_hashtable, &(bfd->node), (void *)(&bfd->bfd_key), (void *)(bfd));
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("bfd session hashtable insert failed(%s)\n",
                     switch_error_to_string(status));
    SWITCH_FREE(bfd);
    return status;
  }

  if (bfd->session_type == SWITCH_BFD_ASYNC_PASSIVE) return status;

  switch_bfdd_timer_handler(bfd);

  return status;
}

switch_status_t switch_bfdd_session_delete(uint32_t local_discriminator,
                                           switch_ip_address_t local_ip,
                                           switch_ip_address_t peer_ip) {
  (void)local_discriminator;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfd_key_t bfd_key;
  switch_bfd_session_t *bfd = NULL;

  switch_bfdd_get_bfd_key(&bfd_key, &local_ip, &peer_ip);

  status = SWITCH_HASHTABLE_DELETE(
      &bfd_hashtable, (void *)(&bfd_key), (void **)&bfd);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("bfd session from  hashtable remove failed(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  status = bfd_timer_del(&bfd->sys_tx_timer);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD tx timer deletion failed \n");
    return SWITCH_STATUS_FAILURE;
  }

  status = bfd_timer_del(&bfd->sys_rx_timer);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD rx timer deletion failed \n");
    return SWITCH_STATUS_FAILURE;
  }

  status = bfd_timer_del(&bfd->dummy_timer);
  if (status != SWITCH_STATUS_SUCCESS) {
    return SWITCH_STATUS_FAILURE;
  }

  switch_bfdd_session_free(bfd);

  return status;
}

switch_status_t switch_bfdd_notify(switch_bfd_session_t *bfd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfdd_session_params_t params = {};

  // update params
  params.remote_min_rx = bfd->remote_timers.required_min_rx_intvl;
  params.remote_min_tx = bfd->remote_timers.desired_min_tx_intvl;
  params.negotiated_rx = bfd->negotiated_rx_intvl;
  params.negotiated_tx = bfd->negotiated_tx_intvl;
  params.remote_discriminator = bfd->discriminators.remote_discr;
  params.local_diag = bfd->local_diag;
  params.remote_diag = bfd->remote_diag;
  params.state = (switch_bfd_session_state_t)bfd->session_state;
  params.remote_multiplier = bfd->remote_detect_mult;

  if (bfdd_ctx->state_cb) {
    bfdd_ctx->state_cb(bfd->session_handle, params);
  }
  return status;
}

switch_status_t switch_bfdd_session_down(switch_bfd_session_t *bfd,
                                         switch_bfd_diag_t diag) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bfd->session_state = SWITCH_BFD_DOWN;
  bfd->local_diag = diag;

  bfd->discriminators.remote_discr = 0;
  bfd->remote_timers.required_min_rx_intvl = 1;

  // notify update to SAI
  switch_bfdd_notify(bfd);

  return status;
}

switch_status_t switch_bfdd_delete_timers(switch_bfd_session_t *bfd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  // stop Tx and Rx timer as session is offloading to HW
  status = bfd_timer_stop(&bfd->sys_tx_timer);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD tx timer deletion failed \n");
    return SWITCH_STATUS_FAILURE;
  }

  status = bfd_timer_stop(&bfd->sys_rx_timer);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("BFD rx timer deletion failed \n");
    return SWITCH_STATUS_FAILURE;
  }
  return status;
}

switch_status_t switch_bfdd_session_up(switch_bfd_session_t *bfd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bfd->session_state = SWITCH_BFD_UP;

  /* notify update to SAI  */
  switch_bfdd_notify(bfd);

  /* stop Tx and Rx timer as
     session is offloading to HW */
  status = switch_bfdd_delete_timers(bfd);
  return status;
}

switch_status_t switch_bfdd_session_init(switch_bfd_session_t *bfd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bfd->session_state = SWITCH_BFD_INIT;

  // notify update to SAI
  switch_bfdd_notify(bfd);

  return status;
}

void switch_bfdd_get_bfd_key(switch_bfd_key_t *bfd_key,
                             switch_ip_address_t *local_ip,
                             switch_ip_address_t *peer_ip) {
  bfd_key->local_ip = *local_ip;
  bfd_key->peer_ip = *peer_ip;
}

switch_bfd_session_t *switch_bfdd_get_bfd_session(switch_bfd_key_t *bfd_key) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfd_session_t *bfd;
  status =
      SWITCH_HASHTABLE_SEARCH(&bfd_hashtable, (void *)bfd_key, (void **)&bfd);

  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("bfd hash search fail: %s\n",
                     switch_error_to_string(status));
    return NULL;
  }

  return bfd;
}

void switch_bfdd_down_handler(switch_bfd_session_t *bfd,
                              switch_bfd_state_t bfd_state) {
  switch (bfd_state) {
    case SWITCH_BFD_ADMIN_DOWN:
    case SWITCH_BFD_UP:
      break;
    case SWITCH_BFD_DOWN:
      switch_bfdd_session_init(bfd);
      break;
    case SWITCH_BFD_INIT:
      switch_bfdd_session_up(bfd);
      break;
    default:
      break;
  }
}
void switch_bfdd_init_handler(switch_bfd_session_t *bfd,
                              switch_bfd_state_t bfd_state) {
  switch (bfd_state) {
    case SWITCH_BFD_ADMIN_DOWN:
      switch_bfdd_session_down(bfd, SWITCH_BFD_DIAG_ADMIN_DOWN);
      break;
    case SWITCH_BFD_DOWN:
      break;
    case SWITCH_BFD_INIT:
    case SWITCH_BFD_UP:
      switch_bfdd_session_up(bfd);
      break;
    default:
      break;
  }
}

void switch_bfdd_up_handler(switch_bfd_session_t *bfd,
                            switch_bfd_state_t bfd_state) {
  switch (bfd_state) {
    case SWITCH_BFD_ADMIN_DOWN:
      switch_bfdd_session_down(bfd, SWITCH_BFD_DIAG_ADMIN_DOWN);
      break;
    case SWITCH_BFD_DOWN:
      switch_bfdd_session_down(bfd, SWITCH_BFD_DIAG_NEIGH_SIG_SESS_DOWN);
      break;
    case SWITCH_BFD_INIT:
      break;
    case SWITCH_BFD_UP:
      switch_bfdd_notify(bfd);
      break;
    default:
      break;
  }
}

void switch_bfdd_state_handler(switch_bfd_session_t *bfd,
                               switch_bfd_state_t bfd_state) {
  switch (bfd->session_state) {
    case SWITCH_BFD_ADMIN_DOWN:
      break;
    case SWITCH_BFD_DOWN:
      switch_bfdd_down_handler(bfd, bfd_state);
      break;
    case SWITCH_BFD_INIT:
      switch_bfdd_init_handler(bfd, bfd_state);
      break;
    case SWITCH_BFD_UP:
      switch_bfdd_up_handler(bfd, bfd_state);
      break;
    default:
      break;
  }
}

switch_status_t switch_bfdd_timer_handler(switch_bfd_session_t *bfd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch (bfd->session_state) {
    case SWITCH_BFD_ADMIN_DOWN:
      // delete the timers
      status = switch_bfdd_delete_timers(bfd);
      break;
    case SWITCH_BFD_DOWN:
      if (bfd->session_type == SWITCH_BFD_ASYNC_PASSIVE) {
        // Stop both Rx and Tx timers in passive mode
        status = switch_bfdd_delete_timers(bfd);
      } else {
        // Initiate tx timer for pkt Tx
        switch_bfdd_session_txtimer_update(bfd);

        // stop Rx timer if any
        status = bfd_timer_stop(&bfd->sys_rx_timer);
        if (status != SWITCH_STATUS_SUCCESS) {
          SWITCH_BFD_ERROR("BFD rx timer deletion failed \n");
          return SWITCH_STATUS_FAILURE;
        }
      }
      break;

    case SWITCH_BFD_INIT:
      // Initiate tx timer for pkt Tx
      switch_bfdd_session_txtimer_update(bfd);
      // Initiate Rx timer
      switch_bfdd_session_rxtimer_update(bfd);
      break;
    case SWITCH_BFD_UP:
      break;
    default:
      break;
  }
  return status;
}

switch_status_t switch_bfdd_recv_pkt(void *buf,
                                     switch_ip_address_t *local_ip,
                                     switch_ip_address_t *peer_ip) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_bfd_header_t *bfd_hdr;
  switch_bfd_key_t bfd_key;
  switch_bfd_session_t *bfd;
  switch_bfd_state_t bfd_state;

  bfd_hdr = (switch_bfd_header_t *)buf;

  if (SWITCH_BFD_GET_VER(bfd_hdr->ver_diag) != SWITCH_BFD_VERSION) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd version mismatch (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  if (bfd_hdr->length < SWITCH_BFD_PKT_LEN) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd pkt len too short (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  if (bfd_hdr->detect_mult == 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd detect multiplier is set to 0 (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  if (bfd_hdr->discriminators.local_discr == 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd local discriminator is set to 0 (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  switch_bfdd_get_bfd_key(&bfd_key, local_ip, peer_ip);
  bfd = switch_bfdd_get_bfd_session(&bfd_key);

  if (!bfd) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd session not found (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  // update remote discriminator
  bfd->discriminators.remote_discr = ntohl(bfd_hdr->discriminators.local_discr);

  // update remote diag
  bfd->remote_diag = bfd_hdr->ver_diag & SWITCH_BFD_DIAG_MASK;

  // update remote timers
  bfd->remote_timers.desired_min_tx_intvl =
      ntohl(bfd_hdr->timers.desired_min_tx_intvl);
  bfd->remote_timers.required_min_rx_intvl =
      ntohl(bfd_hdr->timers.required_min_rx_intvl);
  bfd->remote_timers.required_min_echo_intvl =
      ntohl(bfd_hdr->timers.required_min_echo_intvl);
  bfd->remote_detect_mult = bfd_hdr->detect_mult;

  // update negotiated time intervals
  // Tx timer
  if (bfd->local_timers.desired_min_tx_intvl >
      bfd->remote_timers.required_min_rx_intvl) {
    bfd->negotiated_tx_intvl = bfd->local_timers.desired_min_tx_intvl;
  } else {
    bfd->negotiated_tx_intvl = bfd->remote_timers.required_min_rx_intvl;
  }

  // Rx detection timer
  if (bfd->remote_timers.desired_min_tx_intvl >
      bfd->local_timers.required_min_rx_intvl) {
    bfd->negotiated_rx_intvl =
        bfd->remote_detect_mult * bfd->remote_timers.desired_min_tx_intvl;
  } else {
    bfd->negotiated_rx_intvl =
        bfd->remote_detect_mult * bfd->local_timers.required_min_rx_intvl;
  }

  bfd_state = SWITCH_BFD_GET_STATE(bfd_hdr->state);
  switch_bfdd_state_handler(bfd, bfd_state);

  if (SWITCH_BFD_GET_POLL(bfd_hdr->state)) {
    switch_bfdd_send_bfd_pkt_final(bfd);
  }

  status = switch_bfdd_timer_handler(bfd);

  return status;
}

switch_status_t switch_register_bfdd_session_state_cb(
    switch_bfdd_session_state_cb cb) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!bfdd_ctx) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_BFD_ERROR("bfd context not found (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  bfdd_ctx->state_cb = cb;

  return status;
}

switch_status_t switch_bfdd_ctx_init() {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  bfdd_ctx = SWITCH_MALLOC(sizeof(switch_bfdd_context_t), 0x01);
  if (!bfdd_ctx) {
    SWITCH_BFD_ERROR("BFD context alloc failed \n");
    status = SWITCH_STATUS_FAILURE;
    return status;
  }

  SWITCH_MEMSET(bfdd_ctx, 0, sizeof(switch_bfdd_context_t));
  return status;
}

void switch_bfdd_ctx_clean() { SWITCH_FREE(bfdd_ctx); }

static void *switch_bfdd() {
  bfd_timer_init(&bfdd_ctx->userdata);
  return NULL;
}

// This callback has to be non-blocking
void switch_bfdd_trap_rx_cb(char *pkt, int pkt_size, uint16_t reason_code) {
  (void)pkt_size;
  (void)reason_code;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch_bfd_pkt_t *pkt_rx = (switch_bfd_pkt_t *)pkt;
  switch_ip_address_t local_ip = {};
  switch_ip_address_t peer_ip = {};

  switch_bfdd_packet_dump(pkt, false);
  local_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  local_ip.ip4 = ntohl(pkt_rx->ip_hdr.dst_addr);

  peer_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  peer_ip.ip4 = ntohl(pkt_rx->ip_hdr.src_addr);

  status = switch_bfdd_recv_pkt((void *)&pkt_rx->bfd_hdr, &local_ip, &peer_ip);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_BFD_ERROR("switch_bfdd_recv_pkt() failed, status = %d ", status);
  }
}

static void switch_bfdd_rx_timeout_expired_cb(char *buf,
                                              int buf_size,
                                              uint16_t reason_code) {
  switch_bfd_pkt_t *pkt;
  switch_bfd_session_t *bfd;
  switch_bfd_key_t key = {0};

  if ((size_t)buf_size < sizeof *pkt) {
    SWITCH_BFD_ERROR(
        "got expiration packet of invalid size from dataplane, size %d "
        "reason_code 0x%04" PRIX16,
        buf_size,
        reason_code);
    return;
  }

  pkt = (switch_bfd_pkt_t *)buf;
  switch_bfdd_packet_dump(pkt, false);
  key.peer_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  key.local_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  key.peer_ip.ip4 = ntohl(pkt->ip_hdr.dst_addr);
  key.local_ip.ip4 = ntohl(pkt->ip_hdr.src_addr);

  bfd = switch_bfdd_get_bfd_session(&key);

  SWITCH_BFD_DEBUG(
      "got timeout expired notification from dataplane for session id "
      "0x%" PRIX64 " reason_code 0x%04" PRIX16 " local_ip 0x%08" PRIX32
      " peer_ip 0x%08" PRIX32,
      (bfd ? bfd->session_handle : 0),
      reason_code,
      key.local_ip.ip4,
      key.peer_ip.ip4);

  if (bfd) {
    bfdd_session_timeout(bfd);
    switch_bfdd_timer_handler(bfd);
  }
}

switch_status_t start_bf_switch_bfdd(uint16_t reason_code) {
  uint64_t expire_pkt_filter_id;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch_bfdd_ctx_init();
  switch_bfd_hashtable_initialize();

  if (pthread_create(&bfdd_ctx->bfdd_thread, NULL, switch_bfdd, NULL) != 0) {
    SWITCH_BFD_ERROR("Failed to initialize BFD state machine \n");
    status = SWITCH_STATUS_FAILURE;
    return status;
  }
  pthread_setname_np(bfdd_ctx->bfdd_thread, "bf_switch_bfdd");

  uint64_t flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB_AND_NETDEV;

  rx_nf_key.reason_code = reason_code;
  rx_nf_key.reason_code_mask = 0xFFFF;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV;
  rx_nf_action.cb = switch_bfdd_trap_rx_cb;

  status = switch_pktdriver_rx_filter_create(0,
                                             rx_nf_priority,
                                             flags,
                                             &rx_nf_key,
                                             &rx_nf_action,
                                             &bfdd_ctx->filter_id);
  if (status != SWITCH_STATUS_SUCCESS) {
    return status;
  }

  rx_nf_priority = SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB_AND_NETDEV;
  flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE;
  rx_nf_key = (switch_pktdriver_rx_filter_key_t){
      .reason_code = SWITCH_BFD_REASON_CODE,
      .reason_code_mask = 0xF000,
  };
  rx_nf_action = (switch_pktdriver_rx_filter_action_t){
      .channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV,
      .cb = switch_bfdd_rx_timeout_expired_cb,
  };
  status = switch_pktdriver_rx_filter_create(0,
                                             rx_nf_priority,
                                             flags,
                                             &rx_nf_key,
                                             &rx_nf_action,
                                             &expire_pkt_filter_id);

  // TBD requires another for IPv6
  return status;
}

switch_status_t stop_bf_switch_bfdd(void) {
  switch_bfd_hashtable_delete();
  switch_bfdd_ctx_clean();

  return SWITCH_STATUS_SUCCESS;
}
