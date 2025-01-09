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

#ifdef __CPU_PROFILER__
#include "gperftools/profiler.h"
#endif

int tx_count = 0;

const uint16_t device = 0;

bf_pkt_rx_callback packet_inject;

void pkt_rx_callback_passthrough(bf_pkt_rx_callback cb) { packet_inject = cb; }

int pkt_tx_passthrough(bf_dev_id_t dev_id,
                       bf_pkt *pkt,
                       bf_pkt_tx_ring_t tx_ring,
                       void *tx_cookie) {
  (void)dev_id;
  (void)pkt;
  (void)tx_ring;
  (void)tx_cookie;
  return 0;
}

void test_rx_filter() {
  printf("%s\n", __func__);
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  uint64_t flags = 0, filter_id = 0;
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MIN;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT;
  rx_nf_key.dev_port = 10;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB;

  status = switch_pktdriver_rx_filter_create(0, 0, 0, NULL, NULL, NULL);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, NULL, NULL, NULL);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, NULL, NULL);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, NULL);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_pktdriver_rx_filter_delete(0, NULL, 0);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, 0);
  assert(status == SWITCH_STATUS_INVALID_PARAMETER);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
}

// clang-format off
uint8_t test_pkt[SWITCH_PACKET_MAX_BUFFER_SIZE] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x90, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x0a, 0x00, 0x06, 0x10, 0x08, 0x00, 0x17, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00,
    0x06, 0x04, 0x00, 0x01, 0x00, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01, 0x01, 0x02, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x0b, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// clang-format on
int test_pkt_size = 64;
int cb_counter = 0;
int cb_rx_filter_counter = 0;

bf_pkt *make_pkt() {
  bf_pkt *pkt = NULL;
  bf_pkt_alloc(0, &pkt, 0, 0);
  bf_pkt_data_copy(pkt, test_pkt, test_pkt_size);
  // pkt.pkt_data = test_pkt;
  // pkt.pkt_size = test_pkt_size;
  return pkt;
}
void test_cb() {
  printf("%s\n", __func__);
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch_pkt_dump_enable(true);
  switch_pkt_dev_port_to_port_handle_set(10, 0x11111111);

  uint64_t flags = 0, filter_id = 0;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MIN;

  flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT;
  rx_nf_key.dev_port = 10;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  // packet matching on ingress dev port
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_counter == 1);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  flags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE;
  rx_nf_key.dev_port = 0;
  rx_nf_key.reason_code = 23;
  rx_nf_key.reason_code_mask = SWITCH_REASON_CODE_VALUE_MASK;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  // packet matching on reason code
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_counter == 2);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  rx_nf_key.reason_code = 24;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  // no packet received since no match on reason code
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_counter == 2);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  rx_nf_key.dev_port = 10;
  rx_nf_key.reason_code = 23;
  flags = flags | SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  // packet matching on reason code and dev_port
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_counter == 3);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_GENL;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, flags, &rx_nf_key, &rx_nf_action, &filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);

  // no packet received cuz destination is not cb
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_counter == 3);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
}

// test function to test the API's
void test_fd() {
  printf("%s\n", __func__);
  switch_status_t status;

  uint16_t bd = 1, vlan = 1;

  status = switch_pktdriver_fd_add(device, 2);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet fd add ***\n");

  switch_pktdriver_fd_delete(device, 2);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** fd del ***\n");

  status = switch_pktdriver_bd_to_vlan_mapping_add(device, bd, vlan);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** vlan mapping ***\n");

  status = switch_pktdriver_bd_to_vlan_mapping_delete(device, bd, vlan);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** vlan mapping del ***\n");

  /*
  status = switch_pktdriver_knet_device_add(device);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** knet dev add ***\n");

  status = switch_pktdriver_knet_device_delete(device);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** knet dev del ***\n");
  */
}

void setup_rx_filter_netdev(uint64_t *pkt_hostif_handle,
                            int *fd,
                            uint64_t *rx_filter_id,
                            uint64_t *tx_filter_id) {
  uint64_t flags = 0;
  const char *intf_name = "test_intf";
  switch_pkt_hostif_info_t hostif_info = {};
  switch_status_t status;

  // switch_pkt_dump_enable(true);
  switch_pkt_dev_port_to_port_handle_set(10, 0x11111111);

  strncpy(hostif_info.intf_name.text,
          "test_intf",
          sizeof(hostif_info.intf_name.text));
  hostif_info.admin_state = true;
  flags = SWITCH_PKT_HOSTIF_ATTR_INTERFACE_NAME;
  switch_mac_addr_t hostif_mac = {.mac = {0x0, 0x1, 0x2, 0x3, 0x4, 0x5}};
  memcpy(hostif_info.mac.mac, hostif_mac.mac, ETH_LEN);
  flags |= SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS;
  hostif_info.v4addr.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  hostif_info.v4addr.addr.ip4 = 0xb010101;
  flags |= SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS;
  status = switch_pkt_hostif_create(
      device, &hostif_info, flags, fd, pkt_hostif_handle);
  assert(status == SWITCH_STATUS_SUCCESS);
  assert(*fd > 0);
  printf("*** packet hostif create ***, fd=%d\n", *fd);

  status = switch_pkt_hostif_update(device, &hostif_info, flags);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet hostif update ***\n");

  status = switch_pkt_hostif_set_interface_admin_state(device, intf_name, true);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet hostif set admin ***\n");

  status = switch_pkt_hostif_set_interface_oper_state(device, intf_name, true);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** hostif oper state ***\n");

  uint64_t fflags = 0;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MIN;

  fflags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT;
  rx_nf_key.dev_port = 10;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV;
  rx_nf_action.fd = *fd;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, fflags, &rx_nf_key, &rx_nf_action, rx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet rx filter add ***\n");

  fflags = 0;
  switch_pktdriver_tx_filter_key_t tx_nf_key = {};
  switch_pktdriver_tx_filter_action_t tx_nf_action = {};
  switch_pktdriver_tx_filter_priority_t tx_nf_priority =
      SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_MIN;

  tx_nf_action.dev_port = 10;
  tx_nf_action.bypass_flags = SWITCH_BYPASS_ALL;
  fflags |= SWITCH_PKTDRIVER_TX_FILTER_ATTR_HOSTIF_FD;
  tx_nf_key.hostif_fd = *fd;
  tx_nf_key.knet_hostif_handle = *pkt_hostif_handle;
  tx_nf_priority = SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_HOSTIF;
  status = switch_pktdriver_tx_filter_create(
      0, tx_nf_priority, fflags, &tx_nf_key, &tx_nf_action, tx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet tx filter add ***\n");
}

void teardown_rx_filter_netdev(uint64_t pkt_hostif_handle,
                               int fd,
                               uint64_t rx_filter_id,
                               uint64_t tx_filter_id) {
  switch_status_t status;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_tx_filter_key_t tx_nf_key = {};
  status = switch_pktdriver_tx_filter_delete(0, &tx_nf_key, tx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet tx filter del ***\n");

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, rx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet rx filter del ***\n");

  status = switch_pkt_hostif_delete(device, fd, pkt_hostif_handle);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet hostif delete ***\n");

  printf("\n");
}

void test_netdev() {
  printf("%s\n", __func__);
  uint64_t pkt_hostif_handle = 0;
  int fd = 0;
  uint64_t rx_filter_id = 0, tx_filter_id = 0;

  setup_rx_filter_netdev(&pkt_hostif_handle, &fd, &rx_filter_id, &tx_filter_id);

#ifdef __CPU_PROFILER__
  ProfilerStart("./packet.txt");
  ProfilerFlush();
#endif

  int n = 30000;
  tx_count = 0;
  clock_t t = clock();
  for (int i = 0; i < n; i++) packet_inject(0, make_pkt(), NULL, 0);

#ifdef __CPU_PROFILER__
  ProfilerStop();
#endif

  t = clock() - t;
  double time_taken = (((double)t) / CLOCKS_PER_SEC) * 1000.0;
  printf("Time to rx %d packets: %.3f msecs\n", n, time_taken);
  usleep(2000000);
  // assert(tx_count == n);

  teardown_rx_filter_netdev(pkt_hostif_handle, fd, rx_filter_id, tx_filter_id);

  printf("\n");
}

void test_trap_rx_cb(char *pkt, int pkt_size, uint16_t reason_code) {
  (void)pkt;
  (void)pkt_size;
  assert(reason_code == 23);
  cb_rx_filter_counter++;
}

void test_cb_and_netdev() {
  printf("%s\n", __func__);
  switch_status_t status;
  uint64_t pkt_hostif_handle = 0;
  int fd = 0;
  uint64_t rx_filter_id = 0, tx_filter_id = 0, cb_rx_filter_id = 0;

  setup_rx_filter_netdev(&pkt_hostif_handle, &fd, &rx_filter_id, &tx_filter_id);
  usleep(9000000);

  // no cb rx filter. rx count is 0 in callback
  packet_inject(0, make_pkt(), NULL, 0);
  usleep(1000000);
  assert(cb_rx_filter_counter == 0);

  uint64_t fflags = SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE;
  switch_pktdriver_rx_filter_key_t rx_nf_key = {};
  switch_pktdriver_rx_filter_action_t rx_nf_action = {};
  switch_pktdriver_rx_filter_priority_t rx_nf_priority =
      SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB_AND_NETDEV;

  rx_nf_key.dev_port = 0;
  rx_nf_key.reason_code = 23;
  rx_nf_key.reason_code_mask = SWITCH_REASON_CODE_VALUE_MASK;
  rx_nf_action.channel_type = SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV;
  rx_nf_action.cb = test_trap_rx_cb;
  status = switch_pktdriver_rx_filter_create(
      0, rx_nf_priority, fflags, &rx_nf_key, &rx_nf_action, &cb_rx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet cb rx filter add ***\n");

  // cb rx filter has higher priority
  // count should be 1
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_rx_filter_counter == 1);

  // count should be 2
  packet_inject(0, make_pkt(), NULL, 0);
  assert(cb_rx_filter_counter == 2);

  status = switch_pktdriver_rx_filter_delete(0, &rx_nf_key, cb_rx_filter_id);
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** packet rx filter del ***\n");

  // remove cb rx filter, rx count is still 2 in callback
  packet_inject(0, make_pkt(), NULL, 0);
  usleep(1000000);
  assert(cb_rx_filter_counter == 2);

  teardown_rx_filter_netdev(pkt_hostif_handle, fd, rx_filter_id, tx_filter_id);

  printf("\n");
}

void test_switch_packet_cb(char *pkt,
                           int pkt_size,
                           uint64_t port_lag_handle,
                           uint64_t hostif_trap_handle) {
  // 14 is BFN header size
  assert(pkt_size == (test_pkt_size - 14));
  assert(port_lag_handle == 0x11111111);
  cb_counter++;
  // pkt_event.hostif_trap_handle.data = hostif_trap_handle;
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
  printf("*** packet driver start ***\n");

  switch_register_callback_rx(test_switch_packet_cb);

  test_netdev();
  test_fd();
  test_rx_filter();
  test_cb();
  test_cb_and_netdev();

  status = stop_bf_switch_api_packet_driver();
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** stop pktdrv ***\n");

  status = switch_packet_clean();
  assert(status == SWITCH_STATUS_SUCCESS);
  printf("*** pkt clean ***\n");

  printf("\n\nAll tests passed!\n");
  return 0;
}
