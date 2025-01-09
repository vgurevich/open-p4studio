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

#include <stdio.h>
#include <assert.h>
#include <time.h>
#include "bf_switch/bf_switch_types.h"
#include "s3/switch_packet.h"
#include "../switch_utils.h"
#include "test_packet.h"
#include "../../include/s3/switch_bfdd.h"
#include "../bfdd.h"

int tx_count = 0;
uint8_t local_packet[80] = {};

bf_pkt *make_pkt(switch_bfd_pkt_t *bfd_pkt) {
  int shift = SWITCH_PACKET_HEADER_OFFSET + sizeof(switch_packet_header_t);
  uint8_t cpu_fabric[14] = {0x90,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x0a,
                            0x00,
                            0x06,
                            0x10,
                            0x08,
                            0x00,
                            0x2a};
  uint8_t *tpkt = (uint8_t *)bfd_pkt;
  memcpy(local_packet, tpkt, SWITCH_PACKET_HEADER_OFFSET);
  memcpy(local_packet + SWITCH_PACKET_HEADER_OFFSET, &cpu_fabric, 14);
  memcpy(local_packet + shift, tpkt + SWITCH_PACKET_HEADER_OFFSET, 54);
  bf_pkt *pkt = NULL;
  bf_pkt_alloc(0, &pkt, 0, 0);
  bf_pkt_data_copy(pkt, local_packet, 80);

  return pkt;
}

bf_pkt *make_pkt2(switch_bfd_pkt_t *bfd_pkt) {
  int shift = SWITCH_PACKET_HEADER_OFFSET + sizeof(switch_packet_header_t);
  uint8_t cpu_fabric[14] = {0x90,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x00,
                            0x0a,
                            0x00,
                            0x06,
                            0x10,
                            0x08,
                            0x40,
                            0x00};
  uint8_t *tpkt = (uint8_t *)bfd_pkt;
  memcpy(local_packet, tpkt, SWITCH_PACKET_HEADER_OFFSET);
  memcpy(local_packet + SWITCH_PACKET_HEADER_OFFSET, &cpu_fabric, 14);
  memcpy(local_packet + shift, tpkt + SWITCH_PACKET_HEADER_OFFSET, 54);
  bf_pkt *pkt = NULL;
  bf_pkt_alloc(0, &pkt, 0, 0);
  bf_pkt_data_copy(pkt, local_packet, 80);

  return pkt;
}

switch_bfd_header_t test_bfd_hdr = {};
switch_bfd_header_t test_bfd_hdr1 = {};
switch_bfd_key_t test_bfd_key = {};
switch_bfd_session_t test_bfd_session = {};
switch_ip_address_t test_local_ip = {};
switch_ip_address_t test_peer_ip = {};
switch_bfd_session_t *test_bfd_session_p;
uint32_t test_local_discriminator = 10;
uint32_t test_remote_discriminator = 20;

// this simulates packet rx via callback
bf_pkt_rx_callback bfdd_inject;
void pkt_rx_callback_passthrough(bf_pkt_rx_callback cb) { bfdd_inject = cb; }

int pkt_tx_passthrough(bf_dev_id_t dev_id,
                       bf_pkt *pkt,
                       bf_pkt_tx_ring_t tx_ring,
                       void *tx_cookie) {
  (void)dev_id;
  (void)tx_ring;
  (void)tx_cookie;

  // make a copy of original
  uint8_t local_pkt[100];
  memset(local_pkt, 0, sizeof(local_pkt));
  memcpy(local_pkt, pkt->pkt_data, pkt->pkt_size);

  // now get rid of fabric and cpu header
  int shift = SWITCH_PACKET_HEADER_OFFSET + sizeof(switch_packet_header_t);
  memcpy(local_pkt + SWITCH_PACKET_HEADER_OFFSET,
         local_pkt + shift,
         pkt->pkt_size - shift);

  switch_bfd_pkt_t *bfd_pkt = (switch_bfd_pkt_t *)local_pkt;

  memcpy(&test_bfd_hdr, &bfd_pkt->bfd_hdr, sizeof(switch_bfd_header_t));

  switch_ip_header_t *ip_hdr = &bfd_pkt->ip_hdr;
  test_local_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  test_local_ip.ip4 = ntohl(ip_hdr->src_addr);
  test_peer_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  test_peer_ip.ip4 = ntohl(ip_hdr->dst_addr);

  return 0;
}

void test_session_initiate() {
  printf(
      "\n================= test_session_initiate =========================\n");

  switch_ip_address_t local_ip = {};
  switch_ip_address_t peer_ip = {};
  switch_status_t status;

  local_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  local_ip.ip4 = 0x0A000005;

  peer_ip.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  peer_ip.ip4 = 0x0A000006;

  memset(&test_bfd_hdr, 0, sizeof(switch_bfd_header_t));
  memset(&test_local_ip, 0, sizeof(switch_ip_address_t));
  memset(&test_peer_ip, 0, sizeof(switch_ip_address_t));

  status = switch_bfdd_session_create(0,
                                      SWITCH_BFD_ASYNC_ACTIVE,
                                      test_local_discriminator,
                                      1000000,
                                      1000000,
                                      3,
                                      46657,
                                      local_ip,
                                      peer_ip);
  assert(status == SWITCH_STATUS_SUCCESS);
  sleep(1);
  assert(SWITCH_BFD_GET_STATE(test_bfd_hdr.state) == SWITCH_BFD_DOWN);
  assert(ntohl(test_bfd_hdr.discriminators.local_discr) ==
         test_local_discriminator);
  assert(test_local_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_peer_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_local_ip.ip4 == 0x0A000005);
  assert(test_peer_ip.ip4 == 0x0A000006);
  printf(" #bfd session initiated#\n");
}

void test_session_up_and_session_timeout() {
  printf(
      "\n================ test_session_up_and_session_timeout "
      "===================\n");

  switch_bfd_session_t bfd = {};
  bfd.discriminators.local_discr = test_remote_discriminator;
  bfd.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd.session_state = SWITCH_BFD_INIT;
  bfd.detect_mult = 0x3;
  bfd.local_timers.desired_min_tx_intvl =
      ntohl(test_bfd_hdr.timers.desired_min_tx_intvl);
  bfd.local_timers.required_min_rx_intvl =
      ntohl(test_bfd_hdr.timers.required_min_rx_intvl);
  bfd.bfd_key.local_ip = test_peer_ip;
  bfd.bfd_key.peer_ip = test_local_ip;

  switch_bfd_pkt_t pkt = {};
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  bf_pkt *spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);
  sleep(1);

  memcpy(&test_bfd_key.local_ip, &test_local_ip, sizeof(switch_ip_address_t));
  memcpy(&test_bfd_key.peer_ip, &test_peer_ip, sizeof(switch_ip_address_t));

  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_UP);
  printf(" #bfd session is UP#\n");

  /* Inject pkt with session timeout reasoncode*/
  bfd.session_state = SWITCH_BFD_DOWN;
  bfd.bfd_key.local_ip = test_local_ip;
  bfd.bfd_key.peer_ip = test_peer_ip;

  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  spkt = make_pkt2(&pkt);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_DOWN);
  printf(" #bfd session is in DOWN state#\n");
}

void test_session_up_and_session_down() {
  printf(
      "\n===================== test_session_up_and_session_down "
      "===========================\n");
  switch_bfd_session_t bfd = {};
  bfd.discriminators.local_discr = test_remote_discriminator;
  bfd.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd.session_state = SWITCH_BFD_INIT;
  bfd.detect_mult = 0x3;
  bfd.local_timers.desired_min_tx_intvl =
      ntohl(test_bfd_hdr.timers.desired_min_tx_intvl);
  bfd.local_timers.required_min_rx_intvl =
      ntohl(test_bfd_hdr.timers.required_min_rx_intvl);
  bfd.bfd_key.local_ip = test_peer_ip;
  bfd.bfd_key.peer_ip = test_local_ip;

  switch_bfd_pkt_t pkt = {};
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  bf_pkt *spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);
  sleep(1);

  memcpy(&test_bfd_key.local_ip, &test_local_ip, sizeof(switch_ip_address_t));
  memcpy(&test_bfd_key.peer_ip, &test_peer_ip, sizeof(switch_ip_address_t));

  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_UP);
  printf(" #bfd session is UP#\n");

  /* inject pkt with session down */
  switch_bfd_session_t bfd1 = {};
  bfd1.discriminators.local_discr = test_remote_discriminator;
  bfd1.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd1.session_state = SWITCH_BFD_DOWN;
  bfd1.detect_mult = 0x3;
  bfd1.local_timers.desired_min_tx_intvl =
      ntohl(test_bfd_hdr.timers.desired_min_tx_intvl);
  bfd1.local_timers.required_min_rx_intvl =
      ntohl(test_bfd_hdr.timers.required_min_rx_intvl);
  bfd1.bfd_key.local_ip = test_peer_ip;
  bfd1.bfd_key.peer_ip = test_local_ip;
  bfd1.session_state = SWITCH_BFD_DOWN;
  switch_bfd_pkt_t pkt1 = {};
  switch_bfdd_create_bfd_pkt(&bfd1, &pkt1);

  // clear test params
  memset(&test_bfd_hdr, 0, sizeof(switch_bfd_header_t));
  memset(&test_local_ip, 0, sizeof(switch_ip_address_t));
  memset(&test_peer_ip, 0, sizeof(switch_ip_address_t));

  spkt = make_pkt(&pkt1);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  assert(SWITCH_BFD_GET_STATE(test_bfd_hdr.state) == SWITCH_BFD_DOWN);
  assert(ntohl(test_bfd_hdr.discriminators.local_discr) ==
         test_local_discriminator);
  assert(test_local_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_peer_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_local_ip.ip4 == 0x0A000005);
  assert(test_peer_ip.ip4 == 0x0A000006);

  printf(" #bfd session is DOWN#\n");
}

void test_session_init_and_session_timeout() {
  printf(
      "\n================= test_session_init_and_session_timeout "
      "====================\n");
  switch_bfd_session_t bfd = {};
  bfd.discriminators.local_discr = test_remote_discriminator;
  bfd.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd.session_state = SWITCH_BFD_DOWN;
  bfd.detect_mult = 0x3;
  bfd.local_timers.desired_min_tx_intvl =
      ntohl(test_bfd_hdr.timers.desired_min_tx_intvl);
  bfd.local_timers.required_min_rx_intvl =
      ntohl(test_bfd_hdr.timers.required_min_rx_intvl);

  bfd.bfd_key.local_ip = test_peer_ip;
  bfd.bfd_key.peer_ip = test_local_ip;

  switch_bfd_pkt_t pkt = {};
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  // clear test params
  memset(&test_bfd_hdr, 0, sizeof(switch_bfd_header_t));
  memset(&test_local_ip, 0, sizeof(switch_ip_address_t));
  memset(&test_peer_ip, 0, sizeof(switch_ip_address_t));

  bf_pkt *spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  assert(SWITCH_BFD_GET_STATE(test_bfd_hdr.state) == SWITCH_BFD_INIT);
  assert(ntohl(test_bfd_hdr.discriminators.local_discr) ==
         test_local_discriminator);
  assert(test_local_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_peer_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_local_ip.ip4 == 0x0A000005);
  assert(test_peer_ip.ip4 == 0x0A000006);
  printf(" #bfd session is in INIT state#\n");

  printf(" timer started for 4s \n");
  usleep(4000000);
  memcpy(&test_bfd_key.local_ip, &test_local_ip, sizeof(switch_ip_address_t));
  memcpy(&test_bfd_key.peer_ip, &test_peer_ip, sizeof(switch_ip_address_t));
  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_DOWN);
  printf(" #bfd session is DOWN#\n");
}

void test_session_with_different_remote_timers() {
  printf(
      "\n================= test_session_with_different_remote_timers "
      "====================\n");
  switch_bfd_session_t bfd = {};
  bfd.discriminators.local_discr = test_remote_discriminator;
  bfd.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd.session_state = SWITCH_BFD_DOWN;
  bfd.detect_mult = 0x4;
  bfd.local_timers.desired_min_tx_intvl = 2000000;
  bfd.local_timers.required_min_rx_intvl = 2000000;
  bfd.bfd_key.local_ip = test_peer_ip;
  bfd.bfd_key.peer_ip = test_local_ip;

  switch_bfd_pkt_t pkt = {};
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  // clear test params
  memset(&test_bfd_hdr, 0, sizeof(switch_bfd_header_t));
  memset(&test_local_ip, 0, sizeof(switch_ip_address_t));
  memset(&test_peer_ip, 0, sizeof(switch_ip_address_t));

  bf_pkt *spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  assert(SWITCH_BFD_GET_STATE(test_bfd_hdr.state) == SWITCH_BFD_INIT);
  assert(ntohl(test_bfd_hdr.discriminators.local_discr) ==
         test_local_discriminator);
  assert(test_local_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_peer_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_local_ip.ip4 == 0x0A000005);
  assert(test_peer_ip.ip4 == 0x0A000006);
  printf(" #bfd session is in INIT state#\n");

  printf(" timer started for 4s \n");
  usleep(4000000);
  memcpy(&test_bfd_key.local_ip, &test_local_ip, sizeof(switch_ip_address_t));
  memcpy(&test_bfd_key.peer_ip, &test_peer_ip, sizeof(switch_ip_address_t));
  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state != SWITCH_BFD_DOWN);
  printf(" #bfd session is not DOWN yet#\n");

  printf(" timer started for another 4s \n");
  usleep(4000000);
  memcpy(&test_bfd_key.local_ip, &test_local_ip, sizeof(switch_ip_address_t));
  memcpy(&test_bfd_key.peer_ip, &test_peer_ip, sizeof(switch_ip_address_t));
  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_DOWN);
  printf(" #bfd session is DOWN#\n");
}

void test_session_init_and_session_up() {
  printf(
      "\n================ test_session_init_and_session_up "
      "=========================\n");
  switch_bfd_session_t bfd = {};
  bfd.discriminators.local_discr = test_remote_discriminator;
  bfd.discriminators.remote_discr =
      ntohl(test_bfd_hdr.discriminators.local_discr);
  bfd.session_state = SWITCH_BFD_DOWN;
  bfd.detect_mult = 0x3;
  bfd.local_timers.desired_min_tx_intvl =
      ntohl(test_bfd_hdr.timers.desired_min_tx_intvl);
  bfd.local_timers.required_min_rx_intvl =
      ntohl(test_bfd_hdr.timers.required_min_rx_intvl);
  bfd.bfd_key.local_ip = test_peer_ip;
  bfd.bfd_key.peer_ip = test_local_ip;

  switch_bfd_pkt_t pkt = {};
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  // clear test params
  memset(&test_bfd_hdr, 0, sizeof(switch_bfd_header_t));
  memset(&test_local_ip, 0, sizeof(switch_ip_address_t));
  memset(&test_peer_ip, 0, sizeof(switch_ip_address_t));

  bf_pkt *spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  assert(SWITCH_BFD_GET_STATE(test_bfd_hdr.state) == SWITCH_BFD_INIT);
  assert(ntohl(test_bfd_hdr.discriminators.local_discr) ==
         test_local_discriminator);
  assert(test_local_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_peer_ip.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
  assert(test_local_ip.ip4 == 0x0A000005);
  assert(test_peer_ip.ip4 == 0x0A000006);
  printf(" #bfd session is in INIT state#\n");

  bfd.session_state = SWITCH_BFD_UP;
  switch_bfdd_create_bfd_pkt(&bfd, &pkt);

  spkt = make_pkt(&pkt);
  bfdd_inject(0, spkt, NULL, 0);

  sleep(1);
  test_bfd_session_p = switch_bfdd_get_bfd_session(&test_bfd_key);
  assert(test_bfd_session_p);
  assert(test_bfd_session_p->session_state == SWITCH_BFD_UP);
  printf(" #bfd session is in UP state#\n");
}

void test_session_destroy() {
  printf("\n============= test_session_destroy ========================\n");
  switch_status_t status;

  status = switch_bfdd_session_delete(
      test_local_discriminator, test_local_ip, test_peer_ip);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf(" #bfd session is Destroyed#\n");
}

/* main function*/
int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  const char *cpu_port = NULL;
  status = switch_packet_init(cpu_port, true, false);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet init ***\n");

  status = start_bf_switch_api_packet_driver();
  assert(status == SWITCH_STATUS_SUCCESS);

  printf("*** bfdd start ***\n");
  status = start_bf_switch_bfdd(42);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** bfdd started ***\n");

  sleep(1);

  /** NOTE: please do not change
     sequence of below APIs **/
  test_session_initiate();
  test_session_up_and_session_timeout();
  test_session_up_and_session_down();
  test_session_init_and_session_timeout();
  test_session_with_different_remote_timers();
  test_session_init_and_session_up();
  test_session_destroy();

  printf("*** stopping bfdd ***\n");
  status = stop_bf_switch_bfdd();
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** stopped bfdd ***\n");

  printf("\n\nAll tests passed!\n");
  return 0;
}
