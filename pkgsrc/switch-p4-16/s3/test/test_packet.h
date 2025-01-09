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

#ifndef S3_TEST_PACKET_H__
#define S3_TEST_PACKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#define __USE_GNU 1
#include <pthread.h>
#include <asm/byteorder.h>
#include <linux/if_tun.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <signal.h>
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS 1
#include <libnl3/netlink/netlink.h>
#include <libnl3/netlink/route/link.h>
#include <libnl3/netlink/route/addr.h>
#include <errno.h>

typedef int bf_dev_id_t;
typedef uint64_t bf_knet_cpuif_t;
typedef uint64_t bf_knet_hostif_t;
typedef uint64_t bf_knet_filter_t;

typedef enum bf_pkt_tx_ring_e {
  BF_PKT_TX_RING_0,
  BF_PKT_TX_RING_1,
  BF_PKT_TX_RING_2,
  BF_PKT_TX_RING_3,
  BF_PKT_TX_RING_MAX
} bf_pkt_tx_ring_t;

typedef enum bf_pkt_rx_ring_e {
  BF_PKT_RX_RING_0,
  BF_PKT_RX_RING_1,
  BF_PKT_RX_RING_2,
  BF_PKT_RX_RING_3,
  BF_PKT_RX_RING_4,
  BF_PKT_RX_RING_5,
  BF_PKT_RX_RING_6,
  BF_PKT_RX_RING_7,
  BF_PKT_RX_RING_MAX
} bf_pkt_rx_ring_t;

typedef struct bf_pkt_s {
  // fields of interest to applications
  struct bf_pkt_s *next;  // pointer to a chained bf_pkt
  uint16_t pkt_size;      // current size of bf_pkt payload
  uint8_t *pkt_data;      // current pointer to bf_pkt payload
  int ref_cnt;            // for multi consumer of bf_pkt (future use)
  bf_dev_id_t dev;        // device the packet is allocated for
} bf_pkt;

typedef int (*bf_pkt_tx_done_notif_cb)(bf_dev_id_t dev_id,
                                       bf_pkt_tx_ring_t tx_ring,
                                       uint64_t tx_cookie,
                                       uint32_t status);
typedef int (*bf_pkt_rx_callback)(bf_dev_id_t dev_id,
                                  bf_pkt *pkt,
                                  void *cookie,
                                  bf_pkt_rx_ring_t rx_ring);

static inline uint16_t bf_pkt_get_pkt_size(bf_pkt *p) { return (p->pkt_size); }
static inline uint8_t *bf_pkt_get_pkt_data(bf_pkt *p) { return (p->pkt_data); }

static inline bf_pkt *bf_pkt_get_nextseg(bf_pkt *p) {
  bf_pkt *p2 = NULL;
  if (p) {
    p2 = p->next;
  }
  return p2;
}

int bf_pkt_alloc(bf_dev_id_t id, bf_pkt **pkt, size_t size, int dr);
int bf_pkt_data_copy(bf_pkt *pkt, uint8_t *pkt_buf, uint16_t size);
int bf_pkt_free(bf_dev_id_t id, bf_pkt *pkt);
int pkt_tx_passthrough(bf_dev_id_t dev_id,
                       bf_pkt *pkt,
                       bf_pkt_tx_ring_t tx_ring,
                       void *tx_cookie);
int bf_pkt_tx(bf_dev_id_t dev_id,
              bf_pkt *pkt,
              bf_pkt_tx_ring_t tx_ring,
              void *tx_cookie);

int bf_knet_cpuif_ndev_add(const char *cpuif_netdev_name,
                           char *cpuif_knetdev_name,
                           bf_knet_cpuif_t *knet_cpuif_id);
int bf_knet_cpuif_ndev_delete(const bf_knet_cpuif_t knet_cpuif_id);
int bf_knet_hostif_kndev_add(const bf_knet_cpuif_t knet_cpuif_id,
                             bf_knet_hostif_knetdev_t *hostif_knetdev);
int bf_knet_hostif_kndev_delete(const bf_knet_cpuif_t knet_cpuif_id,
                                const bf_knet_hostif_t knet_hostif_id);

bool bf_knet_module_is_inited();
int bf_pal_pltfm_type_get(bf_dev_id_t dev_id, bool *is_sw_model);
int bf_pal_cpuif_netdev_name_get(bf_dev_id_t dev_id,
                                 char *cpuif_netdev_name,
                                 size_t cpuif_name_size);

int bf_knet_tx_action_add(const bf_knet_cpuif_t knet_cpuif_id,
                          const bf_knet_hostif_t knet_hostif_id,
                          bf_knet_tx_action_t *const tx_action);
int bf_knet_tx_action_delete(const bf_knet_cpuif_t knet_cpuif_id,
                             const bf_knet_hostif_t knet_hostif_id);
int bf_knet_rx_filter_add(const bf_knet_cpuif_t knet_cpuif_id,
                          bf_knet_rx_filter_t *rx_filter);
int bf_knet_rx_filter_delete(const bf_knet_cpuif_t knet_cpuif_id,
                             const bf_knet_filter_t filter_id);

int bf_pkt_tx_done_notif_register(bf_dev_id_t dev_id,
                                  bf_pkt_tx_done_notif_cb cb,
                                  bf_pkt_tx_ring_t tx_ring);
void pkt_rx_callback_passthrough(bf_pkt_rx_callback cb);
int bf_pkt_rx_register(bf_dev_id_t dev_id,
                       bf_pkt_rx_callback cb,
                       bf_pkt_rx_ring_t rx_ring,
                       void *rx_cookie);
int bf_pkt_tx_done_notif_deregister(bf_dev_id_t dev_id,
                                    bf_pkt_tx_ring_t tx_ring);
int bf_pkt_rx_deregister(bf_dev_id_t dev_id, bf_pkt_rx_ring_t rx_ring);

bool bf_pkt_is_inited(bf_dev_id_t dev_id);

#endif
