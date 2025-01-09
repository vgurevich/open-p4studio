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
 * @file diag_pkt_eth_cpu.c
 * @date
 *
 * Contains implementation of diag pkt send thread, recv callback for eth cpu
 * packet interface.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <signal.h>
#define __USE_GNU 1
#include <linux/if_tun.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <errno.h>
#include "diag_util.h"
#include "diag_common.h"
#include "diag_pkt.h"
#include "diag_pkt_eth_cpu.h"
#include "tofino/bf_pal/bf_pal_port_intf.h"
#include "tofino/bf_pal/dev_intf.h"

static bf_status_t diag_eth_cpu_hostif_bind(bf_dev_id_t dev_id) {
  struct ifreq ifr;
  struct sockaddr_ll *s_addr;
  int cpu_fd = 0, rc = 0;
  char *intf_name = NULL;
  bf_status_t status = BF_SUCCESS;
  struct packet_mreq mreq;

  if (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    return BF_SUCCESS;
  }

  intf_name = DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name;
  // initialize raw socket
  cpu_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (cpu_fd < 0) {
    DIAG_PRINT("hostif bind failed. socket creation failed for %s.", intf_name);
    return BF_UNEXPECTED;
  }

  // initialize cpu port
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, intf_name, IFNAMSIZ - 1);
  rc = ioctl(cpu_fd, SIOCGIFINDEX, (void *)&ifr);
  if (rc < 0) {
    DIAG_PRINT("hostif bind failed.IOCTL on %s failed", intf_name);
    close(cpu_fd);
    return BF_UNEXPECTED;
  }

  // bind to cpu port
  s_addr = &(DIAG_DEV_INFO(dev_id)->eth_cpu_info.s_addr);
  s_addr->sll_family = AF_PACKET;
  s_addr->sll_ifindex = ifr.ifr_ifindex;
  s_addr->sll_halen = ETHER_ADDR_LEN;
  s_addr->sll_protocol = htons(ETH_P_ALL);
  rc = bind(cpu_fd, (struct sockaddr *)s_addr, sizeof(*s_addr));
  if (rc < 0) {
    DIAG_PRINT("hostif bind failed.cpu interface bind failed for %s",
               intf_name);
    close(cpu_fd);
    return BF_UNEXPECTED;
  }

  memset(&mreq, 0, sizeof(struct packet_mreq));
  mreq.mr_ifindex = ifr.ifr_ifindex;
  mreq.mr_type = PACKET_MR_PROMISC;
  mreq.mr_alen = 6;
  if (setsockopt(cpu_fd,
                 SOL_PACKET,
                 PACKET_ADD_MEMBERSHIP,
                 (void *)&mreq,
                 (socklen_t)sizeof(mreq)) < 0) {
    DIAG_PRINT("error set socket option errno %d %s\n", errno, strerror(errno));
    close(cpu_fd);
    return BF_UNEXPECTED;
  }

  DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_ifindex = ifr.ifr_ifindex;
  DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_fd = cpu_fd;

  return status;
}

static void *diag_eth_cpu_packet_driver(void *args) {
  bf_dev_id_t dev_id = 0, *dev_id_p = (bf_dev_id_t *)args;
  uint8_t in_packet[DIAG_MAX_PKT_SIZE];
  int pkt_size = 0;
  int socket_fd = 0;
  struct sockaddr_ll from;
  socklen_t fromlen = sizeof(from);

  dev_id = *dev_id_p;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

  /* Diag socket bind */
  diag_eth_cpu_hostif_bind(dev_id);

  DIAG_PRINT("Diag: Started ETH-CPU Rx thread \n");

  socket_fd = DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_fd;
  while (1) {
    pkt_size = recvfrom(socket_fd,
                        in_packet,
                        sizeof(in_packet),
                        0,
                        (struct sockaddr *)&from,
                        &fromlen);
    if (pkt_size == -1) {
      DIAG_PRINT("error in recvfrom from Eth-cpu interface, errno %d %s\n",
                 errno,
                 strerror(errno));
    } else {
      /* disconsider outgoing packet as they are always received in SOCKET_RAW
       */
      if (from.sll_pkttype != PACKET_OUTGOING) {
        /* Process the packet */
        diag_eth_cpu_cb_pkt_rx(dev_id, in_packet, pkt_size);
      }
    }
  }

  return NULL;
}

static bf_status_t diag_eth_cpu_rx_init(bf_dev_id_t dev_id) {
  if (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    return BF_SUCCESS;
  }
  int status = pthread_create(
      &(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_driver_thread),
      NULL,
      diag_eth_cpu_packet_driver,
      &(DIAG_DEV_INFO(dev_id)->dev_id));
  if (status) {
    DIAG_PRINT("Eth CPU thread create failed\n");
    return status;
  }
  pthread_setname_np(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_driver_thread,
                     "bf_diag_eth_cpu");
  return BF_SUCCESS;
}

static bf_status_t diag_eth_cpu_rx_deinit(bf_dev_id_t dev_id) {
  int status =
      pthread_cancel(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_driver_thread);
  if (status == 0) {
    pthread_join(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_driver_thread,
                 NULL);
  }
  /* Close fd */
  if (DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_fd) {
    close(DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_fd);
  }

  return BF_SUCCESS;
}

#define DIAG_LOCAL_CMD_NAME_LEN 300
static bf_status_t diag_eth_cpu_intf_setup(bf_dev_id_t dev_id) {
  char cmd[DIAG_LOCAL_CMD_NAME_LEN];
  int ret = 0;

  if (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    return BF_SUCCESS;
  }

  /* Set the link down */
  memset(cmd, 0, sizeof(cmd));
  snprintf(cmd,
           DIAG_LOCAL_CMD_NAME_LEN,
           "ip link set %s down",
           DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name);

  ret = system(cmd);
  if (ret != 0) {
    return BF_UNEXPECTED;
  }

  /* Set the mtu, promsicuous mode and link up */
  memset(cmd, 0, sizeof(cmd));
  snprintf(cmd,
           DIAG_LOCAL_CMD_NAME_LEN,
           "ip link set %s promisc on mtu 9260 up",
           DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name);

  ret = system(cmd);
  if (ret != 0) {
    return BF_UNEXPECTED;
  }

  /* Increase the socket buffer size otherwise pkts will get dropped */
  memset(cmd, 0, sizeof(cmd));
  snprintf(
      cmd, DIAG_LOCAL_CMD_NAME_LEN, "sysctl -w net.core.rmem_default=962144");

  ret = system(cmd);
  if (ret != 0) {
    return BF_UNEXPECTED;
  }

  return BF_SUCCESS;
}

/* Eth CPU port init */
bf_status_t diag_eth_cpu_port_init(bf_dev_id_t dev_id,
                                   const char *eth_cpu_port_name) {
  bf_status_t status = BF_SUCCESS;
  int eth_cpu_port = DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port;
  uint32_t num_lanes = 1;
  /* hardcoded value for tofino1 and tofino2 */
  char pcie_bus_dev[] = "/sys/bus/pci/devices/0000:04:00";
  int instance = 1; /* instance 1 for eth2 */
  int test_fd = 0, rc = 0;
  struct ifreq ifr;

  DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port = false;
  /* Do not use eth cpu port on sw model */
  if (DIAG_DEV_INFO(dev_id)->is_sw_model) {
    return BF_SUCCESS;
  }

  /* Does conf file want us to use eth cpu port */
  if (!eth_cpu_port_name) {
    return BF_SUCCESS;
  }

  test_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (test_fd < 0) {
    DIAG_PRINT("Eth Interface check: socket creation failed");
    return BF_UNEXPECTED;
  }

  // Eth cpu interface name
  DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port = true;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, eth_cpu_port_name, IFNAMSIZ - 1);
  rc = ioctl(test_fd, SIOCGIFINDEX, (void *)&ifr);
  if (rc < 0) {
    DIAG_PRINT("Diag: Eth Interface in conf file does not exist: %s\n",
               eth_cpu_port_name);
    /* If we could not use interface from conf file, try to discover it */
    DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name[0] = '\0';
    status = bf_pal_cpuif_10g_netdev_name_get(
        dev_id,
        pcie_bus_dev,
        instance,
        &(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name[0]),
        DIAG_ETH_CPU_PORT_NAME_LEN);
    if (status != BF_SUCCESS) {
      DIAG_PRINT("Diag: Failed to Auto-discover Eth Interface name \n");
      close(test_fd);
      return status;
    } else {
      DIAG_PRINT("Diag: Auto-discovered Eth Interface name: %s\n",
                 DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name);
    }
  } else {
    strncpy(&(DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port_name[0]),
            eth_cpu_port_name,
            DIAG_ETH_CPU_PORT_NAME_LEN);
  }
  close(test_fd);

  status = diag_eth_cpu_intf_setup(dev_id);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("eth cpu interface physical config set failed");
    return status;
  }

  status = bf_pal_port_add_with_lanes(
      dev_id, eth_cpu_port, BF_SPEED_10G, num_lanes, BF_FEC_TYP_NONE);
  if (status != BF_SUCCESS) {
    DIAG_PRINT(
        "port-add failure for eth cpu port %d status %d", eth_cpu_port, status);
    return status;
  }

  status = bf_pal_port_enable(dev_id, eth_cpu_port);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("port-enable failure for eth cpu port %d status %d",
               eth_cpu_port,
               status);
    return status;
  }

  /* Create RX thread */
  diag_eth_cpu_rx_init(dev_id);

  return BF_SUCCESS;
}

/* Eth CPU port deinit */
bf_status_t diag_eth_cpu_port_deinit(bf_dev_id_t dev_id) {
  if (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    return BF_SUCCESS;
  }
  return diag_eth_cpu_rx_deinit(dev_id);
}

bf_status_t diag_eth_cpu_pkt_tx(bf_dev_id_t dev_id,
                                const uint8_t *buf,
                                uint32_t size) {
  struct sockaddr_ll s_addr;
  memset(&s_addr, 0, sizeof(s_addr));
  s_addr.sll_ifindex = DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_ifindex;
  if (sendto(DIAG_DEV_INFO(dev_id)->eth_cpu_info.cpu_fd,
             buf,
             size,
             0,
             (struct sockaddr *)&s_addr,
             sizeof(struct sockaddr_ll)) == -1) {
    DIAG_PRINT("error in sendto errno %d %s\n", errno, strerror(errno));
    return -1;
  }
  return BF_SUCCESS;
}
