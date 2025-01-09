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
 * @file diag_pkt_kernel.c
 * @date
 *
 * Contains implementation of diag pkt send thread, recv callback for kernel
 * packet interface.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <errno.h>
#include <tofino/bf_pal/dev_intf.h>
#include "diag_common.h"
#include "diag_pkt.h"

typedef struct diag_kernel_info_s {
  struct sockaddr_ll s_addr;
  int sock_fd;
  pthread_t tid;
  uint8_t rx_pkt[4096];
} diag_kernel_info_t;

static diag_kernel_info_t diag_kern_info[BF_MAX_DEV_COUNT];

static void *diag_kernel_pkt_rx(void *arg) {
  struct sockaddr_ll from;
  int sock_fd;
  int len;
  uint8_t *rx_pkt;
  socklen_t fromlen = sizeof(from);
  bf_dev_id_t dev_id = (bf_dev_id_t)(intptr_t)arg;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    DIAG_PRINT("bad dev_id in pkt_rx\n");
    return NULL;
  }

  sock_fd = diag_kern_info[dev_id].sock_fd;
  rx_pkt = diag_kern_info[dev_id].rx_pkt;

  while (1) {
    len =
        recvfrom(sock_fd, rx_pkt, 4096, 0, (struct sockaddr *)&from, &fromlen);
    if (len == -1) {
      DIAG_PRINT(
          "error in recvfrom from kernel socket interface, errno %d %s\n",
          errno,
          strerror(errno));
    } else {
      /* disconsider outgoing packet as they are always received in SOCKET_RAW
       */
      if (from.sll_pkttype != PACKET_OUTGOING) {
        diag_kernel_cb_pkt_rx(dev_id, rx_pkt, len);
      }
    }
  }
}

int diag_kernel_pkt_tx(bf_dev_id_t dev_id,
                       uint8_t *pkt_buf,
                       uint32_t pkt_size) {
  if (dev_id >= BF_MAX_DEV_COUNT) {
    DIAG_PRINT("bad dev_id in pkt_tx\n");
    return -1;
  }

  if (sendto(diag_kern_info[dev_id].sock_fd,
             pkt_buf,
             pkt_size,
             0,
             (struct sockaddr *)&diag_kern_info[dev_id].s_addr,
             sizeof(struct sockaddr_ll)) == -1) {
    DIAG_PRINT("error in sendto errno %d %s\n", errno, strerror(errno));
    return -1;
  }
  return 0;
}

bf_status_t diag_kernel_setup_pkt_intf(bf_dev_id_t dev_id) {
  struct ifreq ifr;
  int fd, ifindex;
  struct sockaddr_ll *saddr;
  pthread_t *tid_p;
  struct packet_mreq mreq;
  bf_status_t status;

  if (dev_id >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  memset(&diag_kern_info[dev_id], 0, sizeof(diag_kernel_info_t));
  tid_p = &diag_kern_info[dev_id].tid;

  fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  diag_kern_info[dev_id].sock_fd = fd;
  if (fd == -1) {
    DIAG_PRINT("error opening socket errno %d %s\n", errno, strerror(errno));
    return BF_OBJECT_NOT_FOUND;
  }

  memset(&ifr, 0, sizeof(struct ifreq));
  status = bf_pal_cpuif_netdev_name_get(dev_id, (char *)ifr.ifr_name, IFNAMSIZ);
  if (status != BF_SUCCESS) {
    return status;
  }

  if (ioctl(fd, SIOCGIFINDEX, &ifr) == -1) {
    DIAG_PRINT("error config socket errno %d %s\n", errno, strerror(errno));
    return BF_OBJECT_NOT_FOUND;
  }
  ifindex = ifr.ifr_ifindex;

  saddr = &(diag_kern_info[dev_id].s_addr);
  saddr->sll_family = AF_PACKET;
  saddr->sll_ifindex = ifindex;
  saddr->sll_halen = ETHER_ADDR_LEN;
  saddr->sll_protocol = htons(ETH_P_ALL);
  if ((bind(fd, (struct sockaddr *)saddr, sizeof(struct sockaddr_ll))) == -1) {
    DIAG_PRINT("error binding socket errno %d %s\n", errno, strerror(errno));
    return BF_UNEXPECTED;
  }

  memset(&mreq, 0, sizeof(struct packet_mreq));
  mreq.mr_ifindex = ifindex;
  mreq.mr_type = PACKET_MR_PROMISC;
  mreq.mr_alen = 6;
  if (setsockopt(fd,
                 SOL_PACKET,
                 PACKET_ADD_MEMBERSHIP,
                 (void *)&mreq,
                 (socklen_t)sizeof(mreq)) < 0) {
    DIAG_PRINT("error set socket option errno %d %s\n", errno, strerror(errno));
    return BF_OBJECT_NOT_FOUND;
  }

  if (pthread_create(
          tid_p, NULL, diag_kernel_pkt_rx, (void *)(uintptr_t)dev_id) != 0) {
    DIAG_PRINT("\ncan't create thread :%d %s", errno, strerror(errno));
    exit(1);
  } else {
    DIAG_PRINT("\n diag rx pkt thread created successfully\n");
  }

  return BF_SUCCESS;
}
