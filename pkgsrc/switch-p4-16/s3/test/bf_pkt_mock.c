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

extern int tx_count;

// Mock bf_pkt APIs
int bf_pkt_alloc(bf_dev_id_t id, bf_pkt **pkt, size_t size, int dr) {
  bf_pkt *t_pkt = (bf_pkt *)bf_sys_calloc(1, sizeof(bf_pkt));
  *pkt = t_pkt;
  return 0;
}
int bf_pkt_data_copy(bf_pkt *pkt, uint8_t *pkt_buf, uint16_t size) {
  pkt->pkt_data = pkt_buf;
  pkt->pkt_size = size;
  return 0;
}
int bf_pkt_free(bf_dev_id_t id, bf_pkt *pkt) {
  bf_sys_free(pkt);
  pkt = NULL;
  return 0;
}

// Each packet based test should implement pkt_tx_passthrough
int bf_pkt_tx(bf_dev_id_t dev_id,
              bf_pkt *pkt,
              bf_pkt_tx_ring_t tx_ring,
              void *tx_cookie) {
  pkt_tx_passthrough(dev_id, pkt, tx_ring, tx_cookie);
  tx_count++;
  return 0;
}

// Each packet based test should implement pkt_rx_callback_passthrough
int bf_pkt_rx_register(bf_dev_id_t dev_id,
                       bf_pkt_rx_callback cb,
                       bf_pkt_rx_ring_t rx_ring,
                       void *rx_cookie) {
  pkt_rx_callback_passthrough(cb);
  return 0;
}
bool bf_pkt_is_inited(bf_dev_id_t dev_id) { return true; }

// Mock bf_pal APIs
int bf_pal_pltfm_type_get(bf_dev_id_t dev_id, bool *is_sw_model) { return 0; }
int bf_pal_cpuif_netdev_name_get(bf_dev_id_t dev_id,
                                 char *cpuif_netdev_name,
                                 size_t cpuif_name_size) {
  return 0;
}

// Mock bf_knet APIs
bool bf_knet_module_is_inited() { return 0; }
int bf_knet_cpuif_ndev_add(const char *cpuif_netdev_name,
                           char *cpuif_knetdev_name,
                           bf_knet_cpuif_t *knet_cpuif_id) {
  return 0;
}
int bf_knet_cpuif_ndev_delete(const bf_knet_cpuif_t knet_cpuif_id) { return 0; }
int bf_knet_hostif_kndev_add(const bf_knet_cpuif_t knet_cpuif_id,
                             bf_knet_hostif_knetdev_t *hostif_knetdev) {
  return 0;
}
int bf_knet_hostif_kndev_delete(const bf_knet_cpuif_t knet_cpuif_id,
                                const bf_knet_hostif_t knet_hostif_id) {
  return 0;
}
int bf_knet_tx_action_add(const bf_knet_cpuif_t knet_cpuif_id,
                          const bf_knet_hostif_t knet_hostif_id,
                          bf_knet_tx_action_t *const tx_action) {
  return 0;
}
int bf_knet_tx_action_delete(const bf_knet_cpuif_t knet_cpuif_id,
                             const bf_knet_hostif_t knet_hostif_id) {
  return 0;
}
int bf_knet_rx_filter_add(const bf_knet_cpuif_t knet_cpuif_id,
                          bf_knet_rx_filter_t *rx_filter) {
  return 0;
}
int bf_knet_rx_filter_delete(const bf_knet_cpuif_t knet_cpuif_id,
                             const bf_knet_filter_t filter_id) {
  return 0;
}
