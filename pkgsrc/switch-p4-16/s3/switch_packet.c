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

/* Local header includes */
#include "s3/switch_packet.h"
#include "switch_utils.h"
#include "psample.h"
#include "genl-packet.h"

#ifdef TESTING
#include "test/test_packet.h"
#else
#include "pkt_mgr/pkt_mgr_intf.h"
#include "tofino/bf_pal/pltfm_intf.h"
#endif
#include "target-sys/bf_sal/bf_sys_intf.h"

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#undef __MODULE__
#define __MODULE__ SWITCH_API_TYPE_PACKET_DRIVER

#undef __MODULE__
#define __MODULE__ SWITCH_API_TYPE_PACKET_DRIVER

#ifdef TESTING
#define SWITCH_PKT_ERROR(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_PKT_WARN(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_PKT_INFO(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_PKT_DEBUG(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#define SWITCH_PKT_DUMP(fmt, arg...) fprintf(stderr, fmt "\n", ##arg);
#else
#define SWITCH_PKT_ERROR(fmt, arg...)    \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_ERR,       \
                       "%s:%d: " fmt,    \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_PKT_WARN(fmt, arg...)     \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_WARN,      \
                       "%s:%d: " fmt,    \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_PKT_INFO(fmt, arg...)     \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_INFO,      \
                       "%s:%d " fmt,     \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_PKT_DEBUG(fmt, arg...)    \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, \
                       BF_LOG_DBG,       \
                       "%s:%d " fmt,     \
                       __FUNCTION__,     \
                       __LINE__,         \
                       ##arg)
#define SWITCH_PKT_DUMP(fmt, arg...) \
  bf_sys_log_and_trace(BF_MOD_SWITCHAPI, BF_LOG_INFO, fmt, ##arg)
#endif

#define PKTDRV_CMD_NONE '\0'
#define PKTDRV_CMD_WAKE 'w'
#define PKTDRV_CMD_EXIT 'e'

pthread_t packet_driver_thread;
static pthread_mutex_t cookie_mutex;
static pthread_cond_t cookie_cv;
static volatile int cookie = 0;

switch_pktdriver_context_t *pktdriver_ctx = NULL;
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;

static switch_status_t pktdriver_cmd_send(char cmd) {
  int rc = switch_fd_write(pktdriver_ctx->pipe_fd[1], &cmd, 1);
  if (rc == 1) return SWITCH_STATUS_SUCCESS;
  return SWITCH_STATUS_FAILURE;
}

static switch_status_t pktdriver_cmd_recv(char *cmd) {
  int rc = switch_fd_read(pktdriver_ctx->pipe_fd[0], cmd, 1);
  if (rc == 1) return SWITCH_STATUS_SUCCESS;
  return SWITCH_STATUS_FAILURE;
}

void switch_pkt_dump_enable(bool enable) {
  if (!pktdriver_ctx) return;
  pktdriver_ctx->tx_pkt_trace_enable = enable;
  pktdriver_ctx->rx_pkt_trace_enable = enable;
}

switch_status_t switch_pkt_sflow_id_to_rate_set(uint8_t sflow_id,
                                                uint32_t rate,
                                                uint32_t pipe_id) {
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (sflow_id < SWITCH_MAX_SFLOW_EXCLUSIVE_ID) {
    // Exclusive sflow session bind to port
    // Originally in that case sflow_id is the pipe_port,
    // so use an offset for each pipe to set the rate in sflow_id_to_rate
    sflow_id = sflow_id + (pipe_id * SWITCH_MAX_PORT_PER_PIPE);
  } else {
    // For shared session the rate is save with some offset
    sflow_id += SWITCH_SFLOW_SHARED_SESSIONS_OFFSET;
  }

  pktdriver_ctx->sflow_id_to_rate[sflow_id] = rate;

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pkt_dev_port_to_ifindex_set(uint16_t dev_port,
                                                   const char *intf_name) {
  switch_int32_t rc = 0;
  struct ifreq ifr;
  switch_fd_t sock_fd = SWITCH_FD_INVALID;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;
  if (dev_port >= SWITCH_MAX_PORTS) {
    SWITCH_PKT_ERROR("dev_port value %d exceeded SWITCH_MAX_PORTS %d: \n",
                     dev_port,
                     SWITCH_MAX_PORTS);
    return SWITCH_STATUS_FAILURE;
  }

  sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock_fd < 0) {
    SWITCH_PKT_ERROR(
        "dev_port_to_ifindex_map update failed, dev_port %d interface %s: \n",
        dev_port,
        intf_name);
    return SWITCH_STATUS_FAILURE;
  }

  SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
  rc = switch_ioctl(sock_fd, SIOCGIFINDEX, (void *)&ifr);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "dev_port_to_ifindex_map update failed, dev_port %d interface %s: \n",
        dev_port,
        intf_name);
    switch_fd_close(sock_fd);
    return SWITCH_STATUS_FAILURE;
  }

  pktdriver_ctx->dev_port_to_ifindex_map[dev_port] = ifr.ifr_ifindex;

  switch_fd_close(sock_fd);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pkt_dev_port_to_port_handle_set(uint16_t dev_port,
                                                       uint64_t port_handle) {
  if (dev_port >= SWITCH_MAX_PORTS) {
    SWITCH_PKT_ERROR("dev_port value %d exceeded SWITCH_MAX_PORTS %d: \n",
                     dev_port,
                     SWITCH_MAX_PORTS);
    return SWITCH_STATUS_FAILURE;
  }
  pktdriver_ctx->dev_port_to_port_handle_map[dev_port] = port_handle;
  return SWITCH_STATUS_SUCCESS;
}

static switch_pktdriver_genl_family_t switch_pkt_genl_family_by_name(
    const char *const name) {
  if (strcmp(name, PSAMPLE_GENL_NAME) == 0) {
    return SWITCH_PKTDRIVER_GENL_FAMILY_PSAMPLE;
  } else if (strcmp(name, GENL_PACKET_NAME) == 0) {
    return SWITCH_PKTDRIVER_GENL_FAMILY_PACKET;
  }
  return SWITCH_PKTDRIVER_GENL_FAMILY_MAX;
}

switch_status_t switch_pkt_genetlink_create(
    switch_device_t device, const switch_pkt_hostif_info_t *hostif_info) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  int family_id, grp_id;
  struct nl_sock *nlsock = NULL;
  const char *intf_name = hostif_info->intf_name.text;
  const char *mcgrp_name = hostif_info->genetlink_mcgrp_name.text;
  switch_pktdriver_genl_family_t genl_family;
  switch_pktdriver_genl_t *genl = NULL;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  genl_family = switch_pkt_genl_family_by_name(intf_name);
  if (genl_family == SWITCH_PKTDRIVER_GENL_FAMILY_MAX) {
    SWITCH_PKT_ERROR("unknown genetlink family name %s", intf_name);
    return SWITCH_STATUS_FAILURE;
  }

  genl = &pktdriver_ctx->genl[genl_family];

  if (genl->nlsock) {
    genl->ref_cnt++;
    return SWITCH_STATUS_SUCCESS;
  }

  nlsock = nl_socket_alloc();
  if (!nlsock) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink create failed on device %d family %s:"
        " socket alloc failed:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(status));
    return status;
  }

  // disable seq checks on multicast sockets
  nl_socket_disable_seq_check(nlsock);
  nl_socket_disable_auto_ack(nlsock);

  // connect to genl */
  if (genl_connect(nlsock)) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink create failed on device %d family %s:"
        " genl_connect failed:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(status));
    goto cleanup;
  }

  // resolve the generic nl family id
  family_id = genl_ctrl_resolve(nlsock, intf_name);
  if (family_id < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink create failed on device %d family %s:"
        " genl family resolve failed:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(status));
    goto cleanup;
  }

  // resolve the generic nl mc grp id
  grp_id = genl_ctrl_resolve_grp(nlsock, intf_name, mcgrp_name);
  if (grp_id < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink create failed on device %d family %s:"
        " genl mc group %s, resolve failed:(%s)\n",
        device,
        intf_name,
        mcgrp_name,
        switch_error_to_string(status));
    goto cleanup;
  }

  // add this socket as a member of the mc grp
  if (nl_socket_add_membership(nlsock, grp_id)) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink create failed on device %d family %s:"
        " genl mc group %s, member add failed:(%s)\n",
        device,
        intf_name,
        mcgrp_name,
        switch_error_to_string(status));
    goto cleanup;
  }

  // set the groups where we want to mc sample packets
  genl->nlsock = nlsock;
  genl->family_id = family_id;
  genl->mcgrp_id = grp_id;
  genl->ref_cnt = 1;
  return status;

cleanup:
  nl_socket_free(nlsock);
  return status;
}

switch_status_t switch_pkt_genetlink_delete(
    switch_device_t device, const switch_pkt_hostif_info_t *hostif_info) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  struct nl_sock *nlsock = NULL;
  const char *intf_name = hostif_info->intf_name.text;
  const char *mcgrp_name = hostif_info->genetlink_mcgrp_name.text;
  switch_pktdriver_genl_family_t genl_family;
  switch_pktdriver_genl_t *genl = NULL;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  genl_family = switch_pkt_genl_family_by_name(intf_name);
  if (genl_family == SWITCH_PKTDRIVER_GENL_FAMILY_MAX) {
    SWITCH_PKT_ERROR("unknown genetlink family name %s", intf_name);
    return SWITCH_STATUS_FAILURE;
  }
  genl = &pktdriver_ctx->genl[genl_family];

  if (!genl->nlsock) return SWITCH_STATUS_SUCCESS;

  if (genl->ref_cnt > 1) {
    genl->ref_cnt--;
    return SWITCH_STATUS_SUCCESS;
  }

  nlsock = genl->nlsock;

  // remove this socket as a member of the mc grp
  if (nl_socket_drop_membership(nlsock, genl->mcgrp_id)) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink delete failed on device %d family %s:"
        " genl mc group %s, member drop failed:(%s)\n",
        device,
        intf_name,
        mcgrp_name,
        switch_error_to_string(status));
    goto cleanup;
  }

cleanup:
  nl_socket_free(genl->nlsock);
  genl->nlsock = NULL;
  genl->family_id = 0;
  genl->mcgrp_id = 0;
  genl->ref_cnt = 0;
  return status;
}

switch_status_t switch_pkt_hostif_create(
    switch_device_t device,
    const switch_pkt_hostif_info_t *hostif_info,
    const uint64_t flags,
    switch_fd_t *fd,
    uint64_t *knet_hostif_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t rc = 0;
  switch_fd_t hostif_fd = SWITCH_FD_INVALID;
  switch_int32_t fdflags = 0;
  struct ifreq ifr;
  switch_fd_t sock_fd = SWITCH_FD_INVALID;
  switch_knet_hostif_knetdev_t hostif_knetdev;
  switch_knet_info_t *knet_info = NULL;
  const char *intf_name = hostif_info->intf_name.text;
  switch_uint64_t ifindex = hostif_info->ifindex;
  bool use_ifindex = hostif_info->use_ifindex;

  UNUSED(knet_info);
  UNUSED(hostif_knetdev);

  if (!pktdriver_ctx) {
    SWITCH_PKT_ERROR(
        "hostif create failed on device %d interface %s:"
        "pktdriver ctx not ready:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(SWITCH_STATUS_FAILURE));
    return status;
  }

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    switch_strncpy(hostif_knetdev.name, intf_name, IFNAMSIZ);
    status =
        bf_knet_hostif_kndev_add(knet_info->knet_cpuif_id, &hostif_knetdev);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "knet hostif create failed:(%s)\n",
          device,
          intf_name,
          switch_error_to_string(status));
      return status;
    }
    *knet_hostif_handle = hostif_knetdev.knet_hostif_id;
  } else {
    hostif_fd = switch_open("/dev/net/bf_tun", O_RDWR);
    if (hostif_fd < 0) {
      // fallback on linux native tuntap driver
      hostif_fd = switch_open("/dev/net/tun", O_RDWR);
      if (hostif_fd < 0) {
        SWITCH_PKT_ERROR(
            "hostif create failed on device %d interface %s:"
            "netdev create failed:(%s) errno:(%s)\n",
            device,
            intf_name,
            switch_error_to_string(status),
            strerror(errno));
        return SWITCH_STATUS_FAILURE;
      }
    }

    if (use_ifindex) {
      rc = switch_ioctl(hostif_fd, TUNSETIFINDEX, &ifindex);
      if (rc < 0) {
        status = SWITCH_STATUS_FAILURE;
        SWITCH_PKT_ERROR(
            "hostif create failed on device %d interface %s:"
            "netdev flags get failed:(%s) errno:(%s) not able to set ifindex "
            "\n",
            device,
            intf_name,
            switch_error_to_string(status),
            strerror(errno));
        goto cleanup;
      }
    }

    SWITCH_MEMSET(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
    rc = switch_ioctl(hostif_fd, TUNSETIFF, (void *)&ifr);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev ioctl failed:(%s) errno:(%s)\n",
          device,
          intf_name,
          switch_error_to_string(status),
          strerror(errno));
      goto cleanup;
    }

    rc = switch_fcntl(hostif_fd, F_GETFL, fdflags);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev flags get failed:(%s) errno:(%s)\n",
          device,
          intf_name,
          switch_error_to_string(status),
          strerror(errno));
      goto cleanup;
    }
    fdflags |= O_NONBLOCK;
    rc = switch_fcntl(hostif_fd, F_SETFL, fdflags);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev flags set failed:(%s) errno:(%s)\n",
          device,
          intf_name,
          switch_error_to_string(status),
          strerror(errno));
      goto cleanup;
    }
  }

  sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock_fd < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif create failed on device %d interface %s:"
        "netdev socket create failed:(%s) errno:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(status),
        strerror(errno));
    goto cleanup;
  }

  if (flags & SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS) {
    struct sockaddr_in sin;
    SWITCH_MEMSET(&sin, 0x0, sizeof(struct sockaddr));
    SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
    switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ntohl(hostif_info->v4addr.addr.ip4);
    SWITCH_MEMCPY(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    rc = switch_ioctl(sock_fd, SIOCSIFADDR, &ifr);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev socket ip addr conf failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
    sin.sin_addr.s_addr =
        ntohl(SWITCH_IPV4_COMPUTE_MASK(hostif_info->v4addr.len));
    SWITCH_MEMCPY(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    rc = switch_ioctl(sock_fd, SIOCSIFNETMASK, &ifr);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev socket ip netmask conf failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
  }

  if (flags & SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS) {
    SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
    SWITCH_MEMCPY(ifr.ifr_hwaddr.sa_data, hostif_info->mac.mac, ETH_LEN);
    switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    rc = switch_ioctl(sock_fd, SIOCSIFHWADDR, &ifr);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "netdev socket mac addr conf failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
  }

  if (flags & SWITCH_PKT_HOSTIF_ATTR_OPER_STATUS) {
    // Always bring up the tun interface in oper down state. It will be
    // finally brought up/down by control plane.
    // The only exception is a warm-reboot mode. To avoid TeamD FSM flap,
    // this code will be executed only in case the port should stay
    // in oper down state afterwards.
    rc = switch_pkt_hostif_set_interface_oper_state(0, intf_name, false);
    if (rc < 0) {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "failed to set initial oper state to down:(%d)\n",
          device,
          intf_name,
          rc);
      goto cleanup;
    }
  }

  SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
  ifr.ifr_flags &= ~IFF_UP;
  if (hostif_info->admin_state) {
    ifr.ifr_flags |= IFF_UP;
  } else {
    ifr.ifr_flags &= ~IFF_UP;
  }

  rc = switch_ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
  if (rc < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif create failed on device %d interface %s:"
        "netdev socket SIOCSIFFLAGS failed:(%d) errno:(%s)\n",
        device,
        intf_name,
        rc,
        strerror(errno));
    goto cleanup;
  }

  if (!pktdriver_ctx->knet_pkt_driver) {
    status = switch_pktdriver_fd_add(device, hostif_fd);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "hostif create failed on device %d interface %s:"
          "hostif fd add failed:(%s)\n",
          device,
          intf_name,
          switch_error_to_string(status));
      goto cleanup;
    }
  }

  switch_fd_close(sock_fd);
  *fd = hostif_fd;

  return status;

cleanup:
  if (pktdriver_ctx->knet_pkt_driver) {
    if (knet_info) {
      switch_status_t tmp_status = SWITCH_STATUS_SUCCESS;
      tmp_status = bf_knet_hostif_kndev_delete(knet_info->knet_cpuif_id,
                                               *knet_hostif_handle);
      if (tmp_status != SWITCH_STATUS_SUCCESS) {
        SWITCH_PKT_ERROR(
            "hostif create failed on device %d interface %s:"
            "knetdev delete failed:(%s)\n",
            device,
            intf_name,
            switch_error_to_string(tmp_status));
      }
    }
  }
  if (sock_fd >= 0) {
    switch_fd_close(sock_fd);
  }
  if (hostif_fd >= 0) {
    switch_fd_close(hostif_fd);
    *fd = SWITCH_FD_INVALID;
  }
  return status;
}

switch_status_t switch_pkt_hostif_update(
    switch_device_t device,
    const switch_pkt_hostif_info_t *hostif_info,
    const uint64_t flags) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t rc = 0;
  struct ifreq ifr;
  const char *intf_name = hostif_info->intf_name.text;
  switch_fd_t sock_fd = SWITCH_FD_INVALID;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock_fd < 0) {
    SWITCH_PKT_ERROR(
        "hostif update failed on device %d interface %s:"
        "netdev socket create failed:(%s)\n",
        device,
        intf_name,
        switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  if (flags & SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS) {
    struct sockaddr_in sin;
    SWITCH_MEMSET(&sin, 0x0, sizeof(struct sockaddr));
    SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
    switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ntohl(hostif_info->v4addr.addr.ip4);
    SWITCH_MEMCPY(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    rc = switch_ioctl(sock_fd, SIOCSIFADDR, &ifr);
    if (rc < 0) {
      SWITCH_PKT_ERROR(
          "hostif update failed on device %d interface %s:"
          "netdev socket ip address failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
    sin.sin_addr.s_addr =
        ntohl(SWITCH_IPV4_COMPUTE_MASK(hostif_info->v4addr.len));
    SWITCH_MEMCPY(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));
    rc = switch_ioctl(sock_fd, SIOCSIFNETMASK, &ifr);
    if (rc < 0) {
      SWITCH_PKT_ERROR(
          "hostif update failed on device %d interface %s:"
          "netdev socket ip netmask failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
  }

  if (flags & SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS) {
    /*
     * for mac address change, flap the interface
     */
    if (hostif_info->admin_state) {
      SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
      switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
      ifr.ifr_flags &= ~IFF_UP;
      rc = switch_ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
      if (rc < 0) {
        SWITCH_PKT_ERROR(
            "hostif update failed on device %d interface %s:"
            "netdev socket ip netmask failed:(%d) errno:(%s)\n",
            device,
            intf_name,
            rc,
            strerror(errno));
        goto cleanup;
      }
    }

    SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
    SWITCH_MEMCPY(ifr.ifr_hwaddr.sa_data, hostif_info->mac.mac, ETH_LEN);
    switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    rc = switch_ioctl(sock_fd, SIOCSIFHWADDR, &ifr);
    if (rc < 0) {
      SWITCH_PKT_ERROR(
          "hostif update failed on device %d interface %s:"
          "netdev socket mac address failed:(%d) errno:(%s)\n",
          device,
          intf_name,
          rc,
          strerror(errno));
      goto cleanup;
    }
    if (hostif_info->admin_state) {
      SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
      switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
      ifr.ifr_flags |= IFF_UP;
      rc = switch_ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
      if (rc < 0) {
        SWITCH_PKT_ERROR(
            "hostif update failed on device %d interface %s:"
            "netdev socket ifflags failed:(%d) errno:(%s)\n",
            device,
            intf_name,
            rc,
            strerror(errno));
        goto cleanup;
      }
    }
  }

cleanup:
  switch_fd_close(sock_fd);
  return status;
}

switch_status_t switch_pkt_hostif_set_interface_admin_state(
    switch_device_t device, const char *intf_name, bool state) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t rc = 0;
  struct ifreq ifr;
  switch_fd_t sock_fd = SWITCH_FD_INVALID;

  sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (sock_fd < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR("hostif update failed on device %d interface %s: \n",
                     device,
                     intf_name);
    return SWITCH_STATUS_FAILURE;
  }

  SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, intf_name, IFNAMSIZ);
  if (state) {
    ifr.ifr_flags |= IFF_UP;
  } else {
    ifr.ifr_flags &= ~IFF_UP;
  }

  rc = switch_ioctl(sock_fd, SIOCSIFFLAGS, &ifr);
  if (rc < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif update failed on device %d interface %s: errno:(%s)\n",
        device,
        intf_name,
        strerror(errno));
    goto cleanup;
  }

cleanup:
  switch_fd_close(sock_fd);
  return status;
}

switch_status_t switch_pkt_hostif_delete(switch_device_t device,
                                         switch_fd_t fd,
                                         uint64_t knet_hostif_handle) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_knet_hostif_knetdev_t hostif_knetdev;
  switch_knet_info_t *knet_info = NULL;

  UNUSED(knet_info);
  UNUSED(hostif_knetdev);

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (!pktdriver_ctx->knet_pkt_driver) {
    status = switch_pktdriver_fd_delete(device, fd);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "hostif delete failed on device %d "
          "pkt driver fd delete failed:(%s)\n",
          device,
          switch_error_to_string(status));
      return status;
    }
    switch_fd_close(fd);
  } else {
    knet_info = &pktdriver_ctx->switch_kern_info;
    status = bf_knet_hostif_kndev_delete(knet_info->knet_cpuif_id,
                                         knet_hostif_handle);
    SWITCH_ASSERT(status == SWITCH_STATUS_SUCCESS);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "hostif delete failed on device %d "
          "knet hostif delete failed:(%s)\n",
          device,
          switch_error_to_string(status));
      return status;
    }
  }

  return status;
}

int switch_pkt_hostif_set_interface_oper_state(const switch_device_t device,
                                               const char *intf_name,
                                               bool state) {
  int err = 0;
  struct nl_sock *sock;
  struct rtnl_link *p_rtnl_link, *change;
  unsigned char opstate = 0;

  SWITCH_PKT_INFO(
      "%s set operational status %s", intf_name, state ? "UP" : "DOWN");

  sock = nl_socket_alloc();
  if (!sock) {
    SWITCH_PKT_ERROR("could not allocate netlink socket.\n");
    return -1;
  }
  // connect to socket
  if ((err = nl_connect(sock, NETLINK_ROUTE))) {
    SWITCH_PKT_ERROR("netlink error: %s\n", nl_geterror(err));
    nl_socket_free(sock);
    return -1;
  }

  nl_socket_disable_seq_check(sock);
  err = rtnl_link_get_kernel(sock, 0, intf_name, &p_rtnl_link);
  if (err < 0) {
    SWITCH_PKT_ERROR(
        "Cannot get link by name %s: %s\n", intf_name, nl_geterror(err));
    return err;
  }

  change = rtnl_link_alloc();
  opstate = rtnl_link_get_operstate(p_rtnl_link);
  rtnl_link_set_operstate(change, opstate);
  if (state == 0) {
    rtnl_link_set_carrier(change, 0);
    rtnl_link_set_operstate(change, IF_OPER_LOWERLAYERDOWN);
  } else {
    rtnl_link_set_carrier(change, 1);
    rtnl_link_set_operstate(change, IF_OPER_UP);
  }

  err = rtnl_link_change(sock, p_rtnl_link, change, 0);
  if (err < 0) {
    SWITCH_PKT_DEBUG("%s set operational status %s failed: %s",
                     intf_name,
                     state ? "UP" : "DOWN",
                     nl_geterror(err));
  }

  rtnl_link_put(p_rtnl_link);
  rtnl_link_put(change);

  nl_close(sock);
  nl_socket_free(sock);

  bool sw_model = true;
  bf_pal_pltfm_type_get(device, &sw_model);
  if (sw_model) return 0;

  return err;
}

bool switch_pktdriver_mode_is_kernel_internal() {
  return bf_knet_module_is_inited();
}

switch_status_t switch_pktdriver_knet_device_add_internal(
    const switch_device_t device) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_knet_info_t *knet_info = NULL;
  char cpuif_netdev_name[IFNAMSIZ] = "";

  if (pktdriver_ctx->knet_pkt_driver) {
    return status;
  }
  status = switch_bf_status_to_switch_status(
      bf_pal_cpuif_netdev_name_get(device, cpuif_netdev_name, IFNAMSIZ));
  SWITCH_ASSERT(status == SWITCH_STATUS_SUCCESS);

  knet_info = &pktdriver_ctx->switch_kern_info;
  SWITCH_MEMSET(knet_info, 0x0, sizeof(switch_knet_info_t));
  status = switch_bf_status_to_switch_status(
      bf_knet_cpuif_ndev_add(cpuif_netdev_name,
                             knet_info->cpuif_knetdev_name,
                             &knet_info->knet_cpuif_id));
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "device add failed for device %d:"
        "knet cpuif add failed for %s:(%s)",
        device,
        cpuif_netdev_name,
        switch_error_to_string(status));

    return SWITCH_STATUS_FAILURE;
  }
  pktdriver_ctx->knet_pkt_driver = true;
  return status;
}

switch_status_t switch_pktdriver_knet_device_delete_internal(
    const switch_device_t device) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_knet_info_t *knet_info = NULL;

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    status = switch_bf_status_to_switch_status(
        bf_knet_cpuif_ndev_delete(knet_info->knet_cpuif_id));
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "device delete failed on device %d"
          "knet delete failed for cpuif id %" PRIu64 ": %s",
          device,
          knet_info->knet_cpuif_id,
          switch_error_to_string(status));
      return status;
    }
  }
  return status;
}

bool switch_pktdriver_mode_is_kernel() {
  /*fixme: do we need to thread-safe?? */
  return (switch_pktdriver_mode_is_kernel_internal());
}

switch_status_t switch_pktdriver_knet_device_add(const switch_device_t device) {
  return (switch_pktdriver_knet_device_add_internal(device));
}

switch_status_t switch_pktdriver_knet_device_delete(
    const switch_device_t device) {
  return (switch_pktdriver_knet_device_delete_internal(device));
}

switch_status_t switch_packet_knet_cpuif_bind(switch_device_t device) {
  switch_knet_info_t *knet_info = NULL;
  struct ifreq ifr;
  struct sockaddr_ll *addr;
  switch_int32_t knet_cpuif_fd = 0;
  switch_int32_t flags = 0;
  switch_int32_t rc = 0;
  char *intf_name = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  knet_info = &pktdriver_ctx->switch_kern_info;
  intf_name = knet_info->cpuif_knetdev_name;
  knet_info->sock_fd = SWITCH_FD_INVALID;

  knet_cpuif_fd = switch_socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
  if (knet_cpuif_fd < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "socket creation failed for %s",
        intf_name);
    return SWITCH_STATUS_FAILURE;
  }

  // set cpu port to be non-blocking
  flags = switch_fcntl(knet_cpuif_fd, F_GETFL, 0);
  if (flags < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "Get flag for interface %s failed",
        intf_name);
    switch_fd_close(knet_cpuif_fd);
    return SWITCH_STATUS_FAILURE;
  }

  flags |= O_NONBLOCK;
  rc = switch_fcntl(knet_cpuif_fd, F_SETFL, flags);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "Set flag for interface %s failed",
        intf_name);
    switch_fd_close(knet_cpuif_fd);
    return SWITCH_STATUS_FAILURE;
  }

  SWITCH_MEMSET(&ifr, 0x0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, knet_info->cpuif_knetdev_name, IFNAMSIZ);
  ifr.ifr_flags |= IFF_UP;

  rc = switch_ioctl(knet_cpuif_fd, SIOCSIFFLAGS, &ifr);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "Failed to bring up interface %s",
        intf_name);
    switch_fd_close(knet_cpuif_fd);
    return SWITCH_STATUS_FAILURE;
  }

  SWITCH_MEMSET(&ifr, 0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, knet_info->cpuif_knetdev_name, IFNAMSIZ);
  rc = switch_ioctl(knet_cpuif_fd, SIOCGIFINDEX, (void *)&ifr);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "IOCTL on %s failed",
        intf_name);
    switch_fd_close(knet_cpuif_fd);
    return SWITCH_STATUS_FAILURE;
  }

  addr = &(knet_info->s_addr);
  // bind to cpu port
  SWITCH_MEMSET(addr, 0, sizeof(struct sockaddr_ll));
  addr->sll_family = AF_PACKET;
  addr->sll_ifindex = ifr.ifr_ifindex;
  addr->sll_protocol = switch_htons(ETH_P_ALL);
  rc = switch_bind(
      knet_cpuif_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_ll));
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "knet cpu interface bind failed for %s",
        intf_name);
    switch_fd_close(knet_cpuif_fd);
    return SWITCH_STATUS_FAILURE;
  }

  status = switch_pktdriver_fd_add(device, knet_cpuif_fd);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "knet cpuif bind failed."
        "knet cpu interface bind failed : %s, rc=%u",
        switch_error_to_string(status),
        rc);
    switch_fd_close(knet_cpuif_fd);
    return status;
  }
  knet_info->sock_fd = knet_cpuif_fd;

  return status;
}

switch_status_t switch_packet_hostif_bind(switch_device_t device) {
  struct ifreq ifr;
  struct sockaddr_ll addr;
  switch_int32_t cpu_fd = 0;
  switch_int32_t flags = 0;
  switch_int32_t rc = 0;
  char *intf_name = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  intf_name = pktdriver_ctx->intf_name;
  // initialize raw socket
  cpu_fd = switch_socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW);
  if (cpu_fd < 0) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "socket creation failed for %s.",
        intf_name);
    return SWITCH_STATUS_FAILURE;
  }

  // set cpu port to be non-blocking
  flags = switch_fcntl(cpu_fd, F_GETFL, 0);
  if (flags < 0) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "socket getfl failed for %s.",
        intf_name);
    switch_fd_close(cpu_fd);
    return SWITCH_STATUS_FAILURE;
  }

  flags |= O_NONBLOCK;
  rc = switch_fcntl(cpu_fd, F_SETFL, flags);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "Set flag for interface %s failed",
        intf_name);
    switch_fd_close(cpu_fd);
    return SWITCH_STATUS_FAILURE;
  }

  // initialize cpu port
  SWITCH_MEMSET(&ifr, 0, sizeof(ifr));
  switch_strncpy(ifr.ifr_name, pktdriver_ctx->intf_name, IFNAMSIZ);
  rc = switch_ioctl(cpu_fd, SIOCGIFINDEX, (void *)&ifr);
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "IOCTL on %s failed",
        intf_name);
    switch_fd_close(cpu_fd);
    return SWITCH_STATUS_FAILURE;
  }

  // bind to cpu port
  SWITCH_MEMSET(&addr, 0, sizeof(addr));
  addr.sll_family = AF_PACKET;
  addr.sll_ifindex = ifr.ifr_ifindex;
  addr.sll_protocol = switch_htons(ETH_P_ALL);
  rc = switch_bind(cpu_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (rc < 0) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "cpu interface bind failed for %s",
        intf_name);
    switch_fd_close(cpu_fd);
    return SWITCH_STATUS_FAILURE;
  }

  pktdriver_ctx->cpu_ifindex = ifr.ifr_ifindex;
  pktdriver_ctx->cpu_fd = cpu_fd;

  status = switch_pktdriver_fd_add(device, cpu_fd);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "hostif bind failed."
        "cpu interface bind failed : %s, rc=%u",
        switch_error_to_string(status),
        rc);
    return status;
  }

  return status;
}

bf_status_t switch_pktdriver_tx_complete(bf_dev_id_t device,
                                         bf_pkt_tx_ring_t tx_ring,
                                         uint64_t tx_cookie,
                                         uint32_t status) {
  /* free the packet */
  UNUSED(tx_ring);
  UNUSED(status);
  bf_pkt *pkt = (bf_pkt *)(uintptr_t)tx_cookie;
  if (pktdriver_ctx->pkt_free_cb)
    pktdriver_ctx->pkt_free_cb(device, (char *)pkt);
  else
    bf_pkt_free(device, pkt);

  return 0;
}

static inline char *switch_pktdriver_pkttype_to_string(
    switch_pktdriver_packet_type_t pkt_type) {
  switch (pkt_type) {
    case SWITCH_PKTDRIVER_PACKET_TYPE_TX_CB:
      return "tx callback";
    case SWITCH_PKTDRIVER_PACKET_TYPE_TX_NETDEV:
      return "tx netdev";
    case SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_ETH:
      return "rx ethernet";
    case SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_PCIE:
      return "rx pcie";
    case SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_KNET:
      return "rx knet";
    default:
      return "unknown";
  }
}

switch_status_t switch_pktdriver_packet_dump(switch_packet_info_t *pkt_info,
                                             bool rx_tx) {
  switch_uint16_t index = 0;
  const int NUM_BYTES = 16;

  if (!pkt_info) {
    return SWITCH_STATUS_SUCCESS;
  }

  SWITCH_PKT_DUMP("++++++++++PACKET %s START++++++++++", rx_tx ? "RX" : "TX");
  SWITCH_PKT_DUMP("type: %s, size: %d, fd: %d",
                  switch_pktdriver_pkttype_to_string(pkt_info->pkt_type),
                  pkt_info->pkt_size,
                  pkt_info->fd);
  SWITCH_PKT_DUMP("FABRIC HEADER");
  SWITCH_PKT_DUMP("pkt version: %d, hdr version: %d, pkt type: %d",
                  pkt_info->pkt_header.fabric_header.packet_version,
                  pkt_info->pkt_header.fabric_header.header_version,
                  pkt_info->pkt_header.fabric_header.packet_type);
  SWITCH_PKT_DUMP("eth type: 0x%x color: %d, qos: %d, device: %d",
                  pkt_info->pkt_header.fabric_header.ether_type,
                  pkt_info->pkt_header.fabric_header.fabric_color,
                  pkt_info->pkt_header.fabric_header.fabric_qos,
                  pkt_info->pkt_header.fabric_header.dst_device);
  SWITCH_PKT_DUMP("CPU HEADER");
  SWITCH_PKT_DUMP("tx_bypass: %d, egress queue: %d",
                  pkt_info->pkt_header.cpu_header.tx_bypass,
                  pkt_info->pkt_header.cpu_header.egress_queue);
  SWITCH_PKT_DUMP("port: %d, port_lag_index: %d, bd: 0x%x, rc: %d",
                  pkt_info->pkt_header.cpu_header.ingress_port,
                  pkt_info->pkt_header.cpu_header.port_lag_index,
                  pkt_info->pkt_header.cpu_header.ingress_bd,
                  pkt_info->pkt_header.cpu_header.reason_code);
  SWITCH_PKT_DUMP("PACKET");
  for (index = 0; index < (pkt_info->pkt_size / NUM_BYTES) * NUM_BYTES;
       index += NUM_BYTES) {
    SWITCH_PKT_DUMP(
        "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
        "%02x %02x",
        pkt_info->pkt[index] & 0xFF,
        pkt_info->pkt[index + 1] & 0xFF,
        pkt_info->pkt[index + 2] & 0xFF,
        pkt_info->pkt[index + 3] & 0xFF,
        pkt_info->pkt[index + 4] & 0xFF,
        pkt_info->pkt[index + 5] & 0xFF,
        pkt_info->pkt[index + 6] & 0xFF,
        pkt_info->pkt[index + 7] & 0xFF,
        pkt_info->pkt[index + 8] & 0xFF,
        pkt_info->pkt[index + 9] & 0xFF,
        pkt_info->pkt[index + 10] & 0xFF,
        pkt_info->pkt[index + 11] & 0xFF,
        pkt_info->pkt[index + 12] & 0xFF,
        pkt_info->pkt[index + 13] & 0xFF,
        pkt_info->pkt[index + 14] & 0xFF,
        pkt_info->pkt[index + 15] & 0xFF);
  }
  char buffer[50];
  for (int i = 0; index < pkt_info->pkt_size; index++, i += 3)
    snprintf(&buffer[i], 50, "%02x ", pkt_info->pkt[index]);
  SWITCH_PKT_DUMP("%s", buffer);

  SWITCH_PKT_DUMP("++++++++++PACKET %s END++++++++++", rx_tx ? "RX" : "TX");
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_packet_init(const char *cpu_port,
                                   bool use_pcie,
                                   bool use_kpkt) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  pktdriver_ctx = bf_sys_calloc(1, sizeof(switch_pktdriver_context_t));
  if (!pktdriver_ctx) {
    return SWITCH_STATUS_FAILURE;
  }

  pktdriver_ctx->pipe_fd[0] = SWITCH_FD_INVALID;
  pktdriver_ctx->pipe_fd[1] = SWITCH_FD_INVALID;
  pktdriver_ctx->cpu_fd = SWITCH_FD_INVALID;
  pktdriver_ctx->knet_pkt_driver = false;
  pktdriver_ctx->use_pcie = use_pcie;
  pktdriver_ctx->use_kpkt = use_kpkt;
  if (cpu_port)
    switch_strncpy(pktdriver_ctx->intf_name, cpu_port, SWITCH_HOSTIF_NAME_SIZE);

  status = switch_pktdriver_init();
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR("packet driver hostif init failed:(%s)",
                     switch_error_to_string(status));
  }
  return status;
}

switch_status_t switch_packet_clean() {
  if (pktdriver_ctx) bf_sys_free(pktdriver_ctx);
  cookie = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pktdriver_init() {
  switch_device_t device = 0;
  switch_int32_t rc = 0;
  switch_int32_t flags = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_status_t tmp_status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) {
    return SWITCH_STATUS_FAILURE;
  }

  rc = switch_pipe(pktdriver_ctx->pipe_fd);
  if (rc != 0) {
    SWITCH_PKT_ERROR("pktdriver init failed. pipe cmd failed:(%s)\n",
                     switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  rc = switch_fcntl(pktdriver_ctx->pipe_fd[0], F_GETFL, 0);
  if (rc != 0) {
    SWITCH_PKT_ERROR("pktdriver init failed. F_GETFL failed:(%s)\n",
                     switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  flags = flags | O_NONBLOCK;
  rc = switch_fcntl(pktdriver_ctx->pipe_fd[0], F_SETFL, flags);
  if (rc != 0) {
    SWITCH_PKT_ERROR("pktdriver init failed. F_SETFL failed:(%s)\n",
                     switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  status = SWITCH_ARRAY_INIT(&pktdriver_ctx->fd_array);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver init failed."
        "fd array list init failed:(%s)\n",
        switch_error_to_string(status));
    goto cleanup;
  }

  status = switch_pktdriver_fd_add(device, pktdriver_ctx->pipe_fd[0]);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR("pktdriver init failed. fd add failed:(%s)\n",
                     switch_error_to_string(status));
    return SWITCH_STATUS_FAILURE;
  }

  if (pktdriver_ctx->knet_pkt_driver) {
    status = switch_packet_knet_cpuif_bind(device);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver init failed."
          "knet cpuif bind failed:(%s)\n",
          switch_error_to_string(status));
      goto cleanup;
    }
  } else {
    pktdriver_ctx->cpu_ifindex = 0;

    if (!pktdriver_ctx->use_pcie) {
      status = switch_packet_hostif_bind(device);
      if (status != SWITCH_STATUS_SUCCESS) {
        SWITCH_PKT_ERROR(
            "pktdriver init failed."
            "hostif bind failed:(%s)\n",
            switch_error_to_string(status));
        goto cleanup;
      }
    }

    status = SWITCH_LIST_INIT(&pktdriver_ctx->rx_filter);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver init failed."
          "rx filter list init failed:(%s)\n",
          switch_error_to_string(status));
      goto cleanup;
    }

    status = SWITCH_LIST_INIT(&pktdriver_ctx->tx_filter);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver init failed."
          "rx filter list init failed:(%s)\n",
          switch_error_to_string(status));
      goto cleanup;
    }
  }

  return status;

cleanup:
  tmp_status = switch_pktdriver_free();
  SWITCH_ASSERT(tmp_status == SWITCH_STATUS_SUCCESS);
  return status;
}

switch_status_t switch_pktdriver_free(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_device_t device = {0};

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (pktdriver_ctx->cpu_fd != SWITCH_FD_INVALID) {
    switch_pktdriver_fd_delete(device, pktdriver_ctx->cpu_fd);
    switch_fd_close(pktdriver_ctx->cpu_fd);
  }

  if (pktdriver_ctx->pipe_fd[0] != SWITCH_FD_INVALID) {
    switch_pktdriver_fd_delete(device, pktdriver_ctx->pipe_fd[0]);
    switch_fd_close(pktdriver_ctx->pipe_fd[0]);
  }

  if (pktdriver_ctx->pipe_fd[1] != SWITCH_FD_INVALID) {
    switch_fd_close(pktdriver_ctx->pipe_fd[1]);
  }

  return status;
}

static bool switch_packet_driver_tx_filter_match(
    const uint64_t flags,
    const switch_pktdriver_tx_filter_key_t *tx_key1,
    const switch_pktdriver_tx_filter_key_t *tx_key2) {
  SWITCH_ASSERT(tx_key1 && tx_key2);

  if (flags & SWITCH_PKTDRIVER_TX_FILTER_ATTR_HOSTIF_FD) {
    if (tx_key1->hostif_fd != tx_key2->hostif_fd) {
      return false;
    }
  }

  return true;
}

switch_int32_t switch_packet_driver_tx_filter_priority_compare(
    const void *key1, const void *key2) {
  switch_pktdriver_tx_filter_info_t *tx_info1 = NULL;
  switch_pktdriver_tx_filter_info_t *tx_info2 = NULL;

  SWITCH_ASSERT(key1 && key2);
  if (!key1 || !key2) {
    SWITCH_PKT_ERROR("tx filter priority compare failed: %s",
                     switch_error_to_string(SWITCH_STATUS_INVALID_PARAMETER));
    return -1;
  }

  tx_info1 = (switch_pktdriver_tx_filter_info_t *)key1;
  tx_info2 = (switch_pktdriver_tx_filter_info_t *)key2;

  return (switch_int32_t)tx_info1->priority -
         (switch_int32_t)tx_info2->priority;
}

static bool switch_packet_driver_rx_filter_match(
    const uint64_t flags,
    const switch_pktdriver_rx_filter_key_t *rx_key1,
    const switch_pktdriver_rx_filter_key_t *rx_key2) {
  SWITCH_ASSERT(rx_key1 && rx_key2);

  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT) {
    if (rx_key1->dev_port != rx_key2->dev_port) {
      return false;
    }
  }

  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_PORT_LAG_INDEX) {
    if (rx_key1->port_lag_index != rx_key2->port_lag_index) {
      return false;
    }
  }

  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_BD) {
    if (rx_key1->bd != rx_key2->bd) {
      return false;
    }
  }

  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE) {
    if ((rx_key1->reason_code & rx_key1->reason_code_mask) !=
        (rx_key2->reason_code & rx_key1->reason_code_mask)) {
      return false;
    }
  }

  return true;
}

switch_int32_t switch_packet_driver_rx_filter_priority_compare(
    const void *key1, const void *key2) {
  switch_pktdriver_rx_filter_info_t *rx_info1 = NULL;
  switch_pktdriver_rx_filter_info_t *rx_info2 = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!key1 || !key2) {
    SWITCH_PKT_ERROR("packet driver rx filter priority compare failed:(%s)\n",
                     switch_error_to_string(status));
    return -1;
  }

  rx_info1 = (switch_pktdriver_rx_filter_info_t *)key1;
  rx_info2 = (switch_pktdriver_rx_filter_info_t *)key2;

  return (switch_int32_t)rx_info2->priority -
         (switch_int32_t)rx_info1->priority;
}

switch_status_t switch_pktdriver_rx_filter_info_get(
    switch_pktdriver_rx_filter_key_t *rx_key,
    switch_pktdriver_rx_filter_info_t **rx_info) {
  switch_pktdriver_rx_filter_info_t *tmp_rx_info = NULL;
  switch_node_t *node = NULL;
  bool matched = false;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(rx_key && rx_info);
  if (!rx_key || !rx_info) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR("rx filter get failed: (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  *rx_info = NULL;

  status = SWITCH_STATUS_ITEM_NOT_FOUND;
  FOR_EACH_IN_LIST(pktdriver_ctx->rx_filter, node) {
    tmp_rx_info = (switch_pktdriver_rx_filter_info_t *)node->data;
    matched = switch_packet_driver_rx_filter_match(
        tmp_rx_info->flags, &tmp_rx_info->rx_key, rx_key);

    if (matched) {
      *rx_info = tmp_rx_info;
      return SWITCH_STATUS_SUCCESS;
    }
  }
  FOR_EACH_IN_LIST_END();

  return status;
}

switch_status_t switch_pktdriver_tx_filter_info_get(
    switch_pktdriver_tx_filter_key_t *tx_key,
    switch_pktdriver_tx_filter_info_t **tx_info) {
  switch_pktdriver_tx_filter_info_t *tmp_tx_info = NULL;
  switch_node_t *node = NULL;
  bool matched = false;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(tx_key && tx_info);
  if (!tx_key || !tx_info) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR("tx filter get failed: (%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  *tx_info = NULL;

  status = SWITCH_STATUS_ITEM_NOT_FOUND;
  FOR_EACH_IN_LIST(pktdriver_ctx->tx_filter, node) {
    tmp_tx_info = (switch_pktdriver_tx_filter_info_t *)node->data;
    matched = switch_packet_driver_tx_filter_match(
        tmp_tx_info->flags, &tmp_tx_info->tx_key, tx_key);

    if (matched) {
      *tx_info = tmp_tx_info;
      return SWITCH_STATUS_SUCCESS;
    }
  }
  FOR_EACH_IN_LIST_END();

  return status;
}

switch_status_t switch_pktdriver_cpu_tx(switch_device_t device,
                                        switch_int8_t *out_packet,
                                        switch_int32_t pkt_size) {
  switch_knet_info_t *knet_info = NULL;
  UNUSED(knet_info);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t rc = 0;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    rc = switch_fd_send(knet_info->sock_fd,
                        out_packet,
                        pkt_size,
                        0x0,
                        (struct sockaddr *)&knet_info->s_addr,
                        sizeof(struct sockaddr_ll));
    if (rc < 0) {
      SWITCH_PKT_ERROR(
          "pktdriver cp tx failed: fd=%d "
          "pkt netdev write failed: errno=%s\n",
          knet_info->sock_fd,
          strerror(errno));
      return SWITCH_STATUS_FAILURE;
    }
  } else if (pktdriver_ctx->use_pcie) {
    bf_pkt *pkt = NULL;
    bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;

    if (bf_pkt_alloc(device, &pkt, pkt_size, BF_DMA_CPU_PKT_TRANSMIT_0) != 0) {
      SWITCH_PKT_ERROR("pktdriver bf_pkt_alloc failed: pkt_size=%d\n",
                       pkt_size);
      return SWITCH_STATUS_NO_MEMORY;
    }

    /* copy the packet buffer and send it */
    if (bf_pkt_data_copy(pkt, (uint8_t *)out_packet, pkt_size) != 0) {
      SWITCH_PKT_ERROR("bf_pkt_data_copy failed: pkt_size=%d\n", pkt_size);
      bf_pkt_free(device, pkt);
      return SWITCH_STATUS_FAILURE;
    }

    if (bf_pkt_tx(device, pkt, tx_ring, (void *)pkt) != BF_SUCCESS) {
      bf_pkt_free(device, pkt);
    }
  } else {
    struct sockaddr_ll addr;
    SWITCH_MEMSET(&addr, 0x0, sizeof(addr));
    addr.sll_ifindex = pktdriver_ctx->cpu_ifindex;

    for (int retry = 2; retry > 0; retry--) {
      fd_set write_set;
      struct timeval timeout;

      /* Initialize the file descriptor set. */
      FD_ZERO(&write_set);
      FD_SET(pktdriver_ctx->cpu_fd, &write_set);

      /* Initialize the timeout data structure. */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      /* select returns 0 if timeout, 1 if input available, -1 if error. */
      rc = select(FD_SETSIZE, NULL, &write_set, NULL, &timeout);
      if (rc < 0) {
        SWITCH_PKT_ERROR("cpu tx select failed: %s", strerror(errno));
        continue;
      }

      if (!FD_ISSET((pktdriver_ctx->cpu_fd), &write_set)) {
        SWITCH_PKT_DEBUG(
            "pktdriver cp tx failed: cpu_fd is not ready to write\n");
        continue;
      }

      rc = switch_fd_send(pktdriver_ctx->cpu_fd,
                          out_packet,
                          pkt_size,
                          0x0,
                          (struct sockaddr *)&addr,
                          sizeof(addr));
      if (rc >= 0) {
        break;
      }

      SWITCH_PKT_ERROR("pktdriver cpu tx failed: errno=%s\n", strerror(errno));
      return SWITCH_STATUS_FAILURE;
    }
  }

  return status;
}

switch_status_t switch_pktdriver_tx(switch_packet_info_t *pkt_info) {
  switch_int8_t out_packet[SWITCH_PACKET_MAX_BUFFER_SIZE +
                           sizeof(switch_packet_header_t)];
  uint16_t bd = 0;
  uint16_t in_offset = 0;
  uint16_t out_offset = 0;
  uint32_t pkt_size = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (!pktdriver_ctx->use_pcie && pktdriver_ctx->cpu_fd == SWITCH_FD_INVALID) {
    SWITCH_PKT_ERROR(
        "packet tx failed: "
        "cpu fd not initialized:(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  pkt_size = pkt_info->pkt_size;

  if (pkt_info->pkt_type == SWITCH_PKTDRIVER_PACKET_TYPE_TX_NETDEV) {
    switch_pktdriver_tx_filter_key_t tx_key;
    switch_pktdriver_tx_filter_info_t *tx_info = NULL;

    SWITCH_ASSERT(pkt_info->fd != SWITCH_FD_INVALID);
    if (pkt_info->fd == SWITCH_FD_INVALID) {
      status = SWITCH_STATUS_INVALID_PARAMETER;
      SWITCH_PKT_ERROR(
          "packet tx failed: "
          "hostif fd invalid:(%s)\n",
          switch_error_to_string(status));
      return status;
    }

    tx_key.hostif_fd = pkt_info->fd;
    status = switch_pktdriver_tx_filter_info_get(&tx_key, &tx_info);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_WARN(
          "pktdriver tx failed for fd %d: "
          "tx filter not found:(%s)\n",
          pkt_info->fd,
          switch_error_to_string(status));
      return status;
    }

    if (tx_info->tx_action.bypass_flags == SWITCH_BYPASS_ALL) {
      SWITCH_PKTINFO_TX_DEV_PORT(pkt_info) = tx_info->tx_action.port_lag_index;
      SWITCH_PKTINFO_TX_BYPASS(pkt_info) = true;
    } else {
      bd = tx_info->tx_action.bd;
      SWITCH_PKTINFO_INGRESS_BD(pkt_info) = bd;
      SWITCH_PKTINFO_TX_DEV_PORT(pkt_info) = SWITCH_INVALID_HW_PORT;
    }
    SWITCH_PKTINFO_BYPASS_FLAGS(pkt_info) = tx_info->tx_action.bypass_flags;
    pktdriver_ctx->num_tx_netdev_packets++;
    tx_info->num_packets++;
  } else {
    SWITCH_PKTINFO_BYPASS_FLAGS(pkt_info) =
        SWITCH_BYPASS_NONE | SWITCH_BYPASS_SYSTEM_ACL;
    SWITCH_PKTINFO_TX_DEV_PORT(pkt_info) = SWITCH_INVALID_HW_PORT;
    pktdriver_ctx->num_tx_cb_packets++;
  }

  SWITCH_PKTINFO_PACKET_TYPE(pkt_info) = SWITCH_FABRIC_HEADER_TYPE_CPU;
  SWITCH_PKTINFO_ETHER_TYPE(pkt_info) = SWITCH_FABRIC_HEADER_ETHTYPE;
  SWITCH_PKTINFO_TX_EGRESS_QUEUE(pkt_info) =
      SWITCH_PKTDRIVER_TX_EGRESS_QUEUE_DEFAULT;

  if (pktdriver_ctx->tx_pkt_trace_enable) {
    switch_pktdriver_packet_dump(pkt_info, false);
  }

  SWITCH_MEMSET(out_packet, 0x0, sizeof(out_packet));

  SWITCH_MEMCPY(
      (out_packet + out_offset), pkt_info->pkt, SWITCH_PACKET_HEADER_OFFSET);
  out_offset += SWITCH_PACKET_HEADER_OFFSET;
  in_offset += SWITCH_PACKET_HEADER_OFFSET;

  SWITCH_PACKET_HEADER_HTON(pkt_info->pkt_header);

  SWITCH_MEMCPY((out_packet + out_offset),
                &pkt_info->pkt_header,
                sizeof(switch_packet_header_t));
  out_offset += sizeof(switch_packet_header_t);
  pkt_size += sizeof(switch_packet_header_t);

  SWITCH_MEMCPY((out_packet + out_offset),
                (pkt_info->pkt + in_offset),
                pkt_info->pkt_size - in_offset);

  status = switch_pktdriver_cpu_tx(pkt_info->device, out_packet, pkt_size);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver tx failed: "
        "pktdriver cpu tx failed:(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  pktdriver_ctx->num_tx_packets++;

  return status;
}

static switch_status_t send_msg_to_genl_psample(switch_packet_info_t *pkt_info,
                                                uint16_t sflow_id) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  struct nl_msg *msg;
  int err = 0;
  uint16_t dev_port = 0;
  switch_pktdriver_genl_t *genl = NULL;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  genl = &pktdriver_ctx->genl[SWITCH_PKTDRIVER_GENL_FAMILY_PSAMPLE];
  if (!genl->nlsock) return SWITCH_STATUS_FAILURE;

  if (sflow_id >= SWITCH_MAX_SFLOW_ID) return SWITCH_STATUS_INVALID_PARAMETER;

  msg = nlmsg_alloc();
  if (!msg) {
    // fprintf(stderr, "failed to allocate netlink message\n");
    goto cleanup;
  }

  if (!genlmsg_put(msg,
                   NL_AUTO_PID,
                   NL_AUTO_SEQ,
                   genl->family_id,
                   0,
                   NLM_F_REQUEST,
                   PSAMPLE_CMD_SAMPLE,
                   0)) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to put nl header\n",
        genl->family_id);
    goto cleanup;
  }

  // ingress ifindex if applicable
  int ifindex = 0;
  dev_port = SWITCH_PKTINFO_RX_DEV_PORT(pkt_info);
  if (dev_port < SWITCH_MAX_PORTS) {
    ifindex = pktdriver_ctx->dev_port_to_ifindex_map[dev_port];
  }
  err = nla_put_u16(msg, PSAMPLE_ATTR_IIFINDEX, ifindex);

  // add when egress sflow is supported
  err = nla_put_u16(msg, PSAMPLE_ATTR_OIFINDEX, 0);

  if (sflow_id < SWITCH_MAX_SFLOW_EXCLUSIVE_ID) {
    // Eclusive sflow session bind to port
    // Originally in that case sflow_id is the pipe_port,
    // so use the pipe * sflow_id to get the right rate from sflow_id_to_rate.
    // Also local port for normal port starts from 8 (the first 8 are recirc)
    sflow_id =
        sflow_id + (DEV_PORT_TO_PIPE(dev_port) * SWITCH_MAX_PORT_PER_PIPE);
  } else {
    // In that case we have shared session, for which the rate is saved with
    // offest.
    sflow_id +=
        SWITCH_SFLOW_SHARED_SESSIONS_OFFSET - SWITCH_MAX_PORT_PER_PIPE - 1;
  }

  err = nla_put_u32(
      msg, PSAMPLE_ATTR_SAMPLE_RATE, pktdriver_ctx->sflow_id_to_rate[sflow_id]);
  err = nla_put_u32(msg, PSAMPLE_ATTR_ORIGSIZE, pkt_info->pkt_size);
  err = nla_put_u32(msg, PSAMPLE_ATTR_SAMPLE_GROUP, 1);
  err = nla_put_u32(msg, PSAMPLE_ATTR_GROUP_SEQ, ++pktdriver_ctx->group_seq);

  // add the packet data
  err = nla_put(msg, PSAMPLE_ATTR_DATA, pkt_info->pkt_size, pkt_info->pkt);
  if (err) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to setup psample header, err: %d\n",
        genl->family_id,
        err);
    goto cleanup;
  }

  // an alternative way to set the dest group is using
  // nl_socket_set_peer_groups()
  struct sockaddr_nl dst_addr;
  memset(&dst_addr, 0, sizeof(dst_addr));
  dst_addr.nl_family = AF_NETLINK;
  dst_addr.nl_groups = 1 << (genl->mcgrp_id - 1);
  nlmsg_set_dst(msg, &dst_addr);

  err = nl_send(genl->nlsock, msg);
  if (err < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to send psample packet, err: %d\n",
        genl->family_id,
        err);
    goto cleanup;
  }

cleanup:
  nlmsg_free(msg);
  return status;
}

static switch_status_t send_msg_to_genl_packet(switch_packet_info_t *pkt_info,
                                               uint16_t context) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  struct nl_msg *msg;
  int err = 0;
  uint16_t dev_port = 0;
  switch_pktdriver_genl_t *genl = NULL;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  genl = &pktdriver_ctx->genl[SWITCH_PKTDRIVER_GENL_FAMILY_PACKET];
  if (!genl->nlsock) return SWITCH_STATUS_FAILURE;

  if (context >= SWITCH_MAX_GENL_PACKET_CONTEXTS)
    return SWITCH_STATUS_INVALID_PARAMETER;

  // GENL_PACKET_ATTR_IIFINDEX(2 bytes) + GENL_PACKET_ATTR_OIFINDEX(2 bytes) +
  // GENL_PACKET_ATTR_CONTEXT (4 bytes)
  const int genl_metadata_size =
      nla_total_size(2) + nla_total_size(2) + nla_total_size(4);
  const int msg_size =
      nlmsg_total_size(pkt_info->pkt_size + genl_metadata_size);
  msg = nlmsg_alloc_size(msg_size);
  if (!msg) {
    // fprintf(stderr, "failed to allocate netlink message\n");
    goto cleanup;
  }

  if (!genlmsg_put(msg,
                   NL_AUTO_PID,
                   NL_AUTO_SEQ,
                   genl->family_id,
                   0,
                   NLM_F_REQUEST,
                   GENL_PACKET_CMD_PACKET,
                   0)) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to put nl header\n",
        genl->family_id);
    goto cleanup;
  }

  // ingress ifindex if applicable
  int ifindex = 0;
  dev_port = SWITCH_PKTINFO_RX_DEV_PORT(pkt_info);
  if (dev_port < SWITCH_MAX_PORTS) {
    ifindex = pktdriver_ctx->dev_port_to_ifindex_map[dev_port];
  }
  err = nla_put_u16(msg, GENL_PACKET_ATTR_IIFINDEX, ifindex);
  err = nla_put_u16(msg, GENL_PACKET_ATTR_OIFINDEX, 0);
  err = nla_put_u32(msg, GENL_PACKET_ATTR_CONTEXT, context);

  // add the packet data
  err = nla_put(msg, GENL_PACKET_ATTR_DATA, pkt_info->pkt_size, pkt_info->pkt);
  if (err) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to setup genl packet header, err: %s\n",
        genl->family_id,
        nl_geterror(err));
    goto cleanup;
  }

  // an alternative way to set the dest group is using
  // nl_socket_set_peer_groups()
  struct sockaddr_nl dst_addr;
  memset(&dst_addr, 0, sizeof(dst_addr));
  dst_addr.nl_family = AF_NETLINK;
  dst_addr.nl_groups = 1 << (genl->mcgrp_id - 1);
  nlmsg_set_dst(msg, &dst_addr);

  err = nl_send(genl->nlsock, msg);
  if (err < 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "hostif genetlink send msg failed: family %d:"
        " failed to send genl packet, err: %s\n",
        genl->family_id,
        nl_geterror(err));
    goto cleanup;
  }

cleanup:
  nlmsg_free(msg);
  return status;
}

static switch_status_t send_msg_to_genl(
    switch_packet_info_t *pkt_info, switch_pktdriver_rx_filter_key_t *rx_key) {
  uint16_t reason_code_value =
      rx_key->reason_code & SWITCH_REASON_CODE_VALUE_MASK;

  switch (rx_key->reason_code & SWITCH_REASON_CODE_TYPE_MASK) {
    case SWITCH_SFLOW_REASON_CODE:
      return send_msg_to_genl_psample(pkt_info, reason_code_value);
    case SWITCH_UDT_REASON_CODE:
      return send_msg_to_genl_packet(pkt_info, reason_code_value);
  }
  return SWITCH_STATUS_NOT_SUPPORTED;
}

switch_status_t switch_pktdriver_netdev_rx(switch_packet_info_t *pkt_info) {
  switch_int32_t rc = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pkt_info || pkt_info->fd == SWITCH_FD_INVALID ||
      pkt_info->fd == STDIN_FILENO) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver netdev rx failed: "
        "pkt info invalid:(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  rc = switch_fd_write(pkt_info->fd, pkt_info->pkt, pkt_info->pkt_size);
  if (rc < 0) {
    if (errno == EIO) {
      SWITCH_PKT_DEBUG(
          "pktdriver netdev rx failed: fd=%d "
          "pkt netdev write failed: errno=%s\n",
          pkt_info->fd,
          strerror(errno));
    } else {
      status = SWITCH_STATUS_FAILURE;
      SWITCH_PKT_ERROR(
          "pktdriver netdev rx failed: fd=%d "
          "pkt netdev write failed: errno=%s\n",
          pkt_info->fd,
          strerror(errno));
      return status;
    }
  }

  return status;
}

switch_status_t switch_pktdriver_bd_to_vlan_mapping_add(switch_device_t device,
                                                        uint16_t bd,
                                                        uint16_t vlan) {
  UNUSED(device);
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  SWITCH_ASSERT(vlan < SWITCH_MAX_VLANS);
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  pktdriver_ctx->bd_mapping[vlan] = bd;

  return status;
}

switch_status_t switch_pktdriver_bd_to_vlan_mapping_delete(
    switch_device_t device, uint16_t bd, uint16_t vlan) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  UNUSED(device);
  UNUSED(bd);

  SWITCH_ASSERT(vlan < SWITCH_MAX_VLANS);
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  pktdriver_ctx->bd_mapping[vlan] = 0;

  return status;
}

switch_status_t switch_pktdriver_bd_to_vlan_mapping_get(uint16_t bd,
                                                        uint16_t *vlan) {
  uint16_t index = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  *vlan = 0;
  status = SWITCH_STATUS_ITEM_NOT_FOUND;

  if (bd == 0) return status;

  for (index = 1; index < SWITCH_MAX_VLANS; index++) {
    if (pktdriver_ctx->bd_mapping[index] == bd) {
      *vlan = index;
      return SWITCH_STATUS_SUCCESS;
    }
  }

  return status;
}

switch_status_t switch_pktdriver_rx_filter_action_transform(
    switch_pktdriver_rx_filter_key_t *rx_key,
    switch_pktdriver_rx_filter_info_t *rx_info,
    switch_packet_info_t *pkt_info) {
  switch_pktdriver_rx_filter_action_t *rx_action = NULL;
  switch_ethernet_header_t *eth_header = NULL;
  switch_vlan_header_t *vlan_header = NULL;
  uint16_t vlan_id = 0;
  uint16_t ether_type = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  SWITCH_ASSERT(rx_info && pkt_info);

  rx_action = &rx_info->rx_action;

  if (rx_action->vlan_action == SWITCH_PACKET_VLAN_ACTION_NONE) {
    return status;
  }

  eth_header = (switch_ethernet_header_t *)(pkt_info->pkt);
  ether_type = switch_htons(eth_header->ether_type);

  if (rx_action->vlan_action == SWITCH_PACKET_VLAN_ACTION_ADD) {
    if (ether_type == ETHERTYPE_DOT1Q) {
      return status;
    }

    status = switch_pktdriver_bd_to_vlan_mapping_get(rx_key->bd, &vlan_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      return status;
    }

    SWITCH_MEMMOVE(
        pkt_info->pkt + SWITCH_ETH_HEADER_SIZE + SWITCH_VLAN_HEADER_SIZE,
        pkt_info->pkt + SWITCH_ETH_HEADER_SIZE,
        pkt_info->pkt_size - SWITCH_ETH_HEADER_SIZE);
    pkt_info->pkt_size += SWITCH_VLAN_HEADER_SIZE;
    vlan_header = (switch_vlan_header_t *)((switch_uint8_t *)(pkt_info->pkt) +
                                           SWITCH_ETH_HEADER_SIZE);

    vlan_header->tpid = eth_header->ether_type;
    vlan_id = switch_htons(vlan_id);
    SWITCH_MEMCPY((void *)vlan_header, (void *)(&vlan_id), sizeof(vlan_id));
    eth_header->ether_type = switch_htons(ETHERTYPE_DOT1Q);

  } else if (rx_action->vlan_action == SWITCH_PACKET_VLAN_ACTION_REMOVE) {
    if (ether_type != ETHERTYPE_DOT1Q) {
      return status;
    }

    vlan_header = (switch_vlan_header_t *)((switch_uint8_t *)(pkt_info->pkt) +
                                           SWITCH_ETH_HEADER_SIZE);
    eth_header->ether_type = vlan_header->tpid;

    SWITCH_MEMMOVE(
        pkt_info->pkt + SWITCH_ETH_HEADER_SIZE,
        pkt_info->pkt + SWITCH_ETH_HEADER_SIZE + SWITCH_VLAN_HEADER_SIZE,
        pkt_info->pkt_size - SWITCH_ETH_HEADER_SIZE - SWITCH_VLAN_HEADER_SIZE);
    pkt_info->pkt_size -= SWITCH_VLAN_HEADER_SIZE;
  }

  return status;
}

void switch_register_callback_rx(switch_internal_callback_rx rx) {
  if (!pktdriver_ctx) {
    SWITCH_PKT_ERROR("callback register fail, pktdriver ctx not ready\n");
    return;
  }
  pktdriver_ctx->rx_cb_internal = rx;
}

void switch_register_callback_pkt_free(pkt_free p_free_cb) {
  pktdriver_ctx->pkt_free_cb = p_free_cb;
}

/*
 * ARP packet with fabric and cpu headers prepended
 * {     DMAC      } {     SMAC      } {BFN} { FAB
 * ff ff ff ff ff ff 00 06 07 08 09 0a 90 00 00 00
 *
 *  } EQ { IP} {PLI} { BD} { RC} {  APR pkt start
 * 00 00 00 00 00 06 10 08 00 17 08 06 00 01 08 00
 * 06 04 00 01 00 06 07 08 09 0a c0 a8 00 01 00 00
 * 00 00 00 00 c0 a8 00 02 00 00 00 00 00 00 00 00
 *
 * BFN - BFN Ethertype (9000)
 * FAB - Fabric header
 * EQ  - Egress Queue
 * IP  - Ingress port
 * PLI - Ingress port_lag_index
 * BD  - Ingress BD
 * RC  - reason code
 *
 */
switch_status_t switch_pktdriver_rx(switch_packet_info_t *pkt_info) {
  switch_pktdriver_rx_filter_key_t rx_key;
  switch_pktdriver_rx_filter_info_t *rx_info = NULL;
  uint16_t dev_port = 0;
  uint64_t fp_port = 0;
  uint16_t reason_code = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  // copy packet header to pkt_header beyond DMAC/SMAC
  SWITCH_MEMCPY(&pkt_info->pkt_header,
                pkt_info->pkt + SWITCH_PACKET_HEADER_OFFSET,
                sizeof(switch_packet_header_t));
  // overwrite DMAC/SMAC over pkt_header
  SWITCH_MEMCPY(pkt_info->pkt + sizeof(switch_packet_header_t),
                pkt_info->pkt,
                SWITCH_PACKET_HEADER_OFFSET);

  // move packet offset to DMAC
  pkt_info->pkt += sizeof(switch_packet_header_t);
  pkt_info->pkt_size = pkt_info->pkt_size - sizeof(switch_packet_header_t);

  SWITCH_PACKET_HEADER_NTOH(pkt_info->pkt_header);

  if (pktdriver_ctx->rx_pkt_trace_enable) {
    switch_pktdriver_packet_dump(pkt_info, true);
  }

  if (SWITCH_PKTINFO_ETHER_TYPE(pkt_info) != SWITCH_FABRIC_HEADER_ETHTYPE) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver rx failed: "
        "fabric header ethertype invalid:(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  reason_code = SWITCH_PKTINFO_REASON_CODE(pkt_info);
  if (reason_code < SWITCH_HOSTIF_MAX_REASON_CODE) {
    pktdriver_ctx->rx_rc_pkts[reason_code].count++;
    pktdriver_ctx->rx_rc_bytes[reason_code].count += pkt_info->pkt_size;
  }

  dev_port = SWITCH_PKTINFO_RX_DEV_PORT(pkt_info);
  if (dev_port < SWITCH_MAX_PORTS) {
    // status = switch_device_front_port_get(pkt_info->device, dev_port,
    // &fp_port);
    if (status == SWITCH_STATUS_SUCCESS) {
      pktdriver_ctx->rx_port_pkts[fp_port].count++;
      pktdriver_ctx->rx_port_bytes[fp_port].count += pkt_info->pkt_size;
    }
  }

  if (!pktdriver_ctx->knet_pkt_driver) {
    SWITCH_MEMSET(&rx_key, 0x0, sizeof(rx_key));
    rx_key.dev_port = SWITCH_PKTINFO_RX_DEV_PORT(pkt_info);
    rx_key.port_lag_index = SWITCH_PKTINFO_INGRESS_PORT_LAG_INDEX(pkt_info);
    rx_key.bd = SWITCH_PKTINFO_INGRESS_BD(pkt_info);
    rx_key.reason_code = SWITCH_PKTINFO_REASON_CODE(pkt_info);
    status = switch_pktdriver_rx_filter_info_get(&rx_key, &rx_info);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver rx failed to get rx filter"
          "dev port: %d ifindex: 0x%x "
          "bd %d rc 0x%x\n",
          rx_key.dev_port,
          rx_key.port_lag_index,
          rx_key.bd,
          rx_key.reason_code);
      return status;
    }
    switch_pktdriver_rx_filter_action_transform(&rx_key, rx_info, pkt_info);
    switch (rx_info->rx_action.channel_type) {
      case SWITCH_PKTDRIVER_CHANNEL_TYPE_GENL:
        status = send_msg_to_genl(pkt_info, &rx_key);
        if (status != SWITCH_STATUS_SUCCESS) {
          SWITCH_PKT_ERROR(
              "pktdriver rx failed for genl:"
              "dev port: %d ifindex: 0x%x "
              "bd %d rc 0x%x\n",
              rx_key.dev_port,
              rx_key.port_lag_index,
              rx_key.bd,
              rx_key.reason_code);
          return status;
        }
        rx_info->num_packets++;
        break;
      case SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV:
      case SWITCH_PKTDRIVER_CHANNEL_TYPE_FD:
      case SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV:
        if (rx_info->rx_action.channel_type ==
            SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV) {
          if (rx_info->rx_action.cb) {
            rx_info->rx_action.cb(
                pkt_info->pkt, pkt_info->pkt_size, rx_key.reason_code);
          }
          pkt_info->fd = pktdriver_ctx->dev_port_to_fd_map[rx_key.dev_port];
        }
        if (rx_info->rx_action.channel_type ==
            SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV) {
          pkt_info->fd = rx_info->rx_action.fd;
        }
        if (rx_info->rx_action.channel_type ==
            SWITCH_PKTDRIVER_CHANNEL_TYPE_FD) {
          pkt_info->fd = rx_info->rx_action.fd;
        }
        status = switch_pktdriver_netdev_rx(pkt_info);
        if (status != SWITCH_STATUS_SUCCESS) {
          SWITCH_PKT_ERROR(
              "pktdriver rx failed for rx key:"
              "dev port: %d ifindex: 0x%x "
              "bd %d rc 0x%x\n",
              rx_key.dev_port,
              rx_key.port_lag_index,
              rx_key.bd,
              rx_key.reason_code);
          return status;
        }
        rx_info->num_packets++;
        pktdriver_ctx->num_rx_netdev_packets++;
        break;
      case SWITCH_PKTDRIVER_CHANNEL_TYPE_CB:
        if (pktdriver_ctx->rx_cb_internal) {
          pktdriver_ctx->rx_cb_internal(
              pkt_info->pkt,
              pkt_info->pkt_size,
              pktdriver_ctx->dev_port_to_port_handle_map[dev_port],
              rx_info->rx_action.hostif_trap_handle);
          pktdriver_ctx->num_rx_cb_packets++;
        }
        rx_info->num_packets++;
        break;
      default:
        break;
    }
  }

  pktdriver_ctx->num_rx_packets++;

  return status;
}

switch_status_t switch_pktdriver_knet_cpu_rx(switch_fd_t knet_fd) {
  switch_packet_info_t pkt_info;
  switch_int8_t in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  switch_int32_t pkt_size = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  pkt_size = switch_fd_read(knet_fd, in_packet, sizeof(in_packet));
  if (pkt_size <= 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR("packet knet cpu rx failed: packet size < 0\n");
    return status;
  }

  SWITCH_MEMSET(&pkt_info, 0x0, sizeof(pkt_info));
  pkt_info.pkt_type = SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_KNET;
  pkt_info.pkt = in_packet;
  pkt_info.pkt_size = pkt_size;

  status = switch_pktdriver_rx(&pkt_info);
  return status;
}

switch_status_t switch_pktdriver_cpu_eth_rx(switch_fd_t cpu_fd) {
  switch_packet_info_t pkt_info;
  switch_int8_t in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  switch_int32_t pkt_size = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(pktdriver_ctx->cpu_fd == cpu_fd);

  pkt_size = switch_fd_read(cpu_fd, in_packet, sizeof(in_packet));
  if (pkt_size <= 0) {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR("packet cpu eth rx failed: packet size < 0\n");
    return status;
  }

  SWITCH_MEMSET(&pkt_info, 0x0, sizeof(pkt_info));
  pkt_info.pkt_type = SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_ETH;
  pkt_info.pkt = in_packet;
  pkt_info.pkt_size = pkt_size;

  status = switch_pktdriver_rx(&pkt_info);
  return status;
}

switch_status_t switch_pktdriver_netdev_tx(switch_fd_t hostif_fd) {
  switch_packet_info_t pkt_info;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t pkt_size = 0;
  switch_int8_t in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];

  SWITCH_ASSERT(hostif_fd != SWITCH_FD_INVALID);

  pkt_size = switch_fd_read(hostif_fd, in_packet, sizeof(in_packet));
  if (pkt_size <= 0) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver netdev tx failed for fd %d: "
        "pkt size is less than 0:(%s)\n",
        hostif_fd,
        switch_error_to_string(status));
    return status;
  }

  SWITCH_MEMSET(&pkt_info, 0x0, sizeof(pkt_info));
  pkt_info.fd = hostif_fd;
  pkt_info.pkt = in_packet;
  pkt_info.pkt_size = pkt_size;
  pkt_info.pkt_type = SWITCH_PKTDRIVER_PACKET_TYPE_TX_NETDEV;

  status = switch_pktdriver_tx(&pkt_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_WARN(
        "pktdriver netdev tx failed for fd %d: "
        "pktdriver cpu tx failed:(%s)\n",
        hostif_fd,
        switch_error_to_string(status));
    return status;
  }

  return status;
}

// public API
switch_status_t switch_pkt_xmit(char *pkt, int pkt_size) {
  switch_packet_info_t pkt_info;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  SWITCH_MEMSET(&pkt_info, 0x0, sizeof(pkt_info));
  pkt_info.pkt = pkt;
  pkt_info.pkt_size = pkt_size;
  pkt_info.pkt_type = SWITCH_PKTDRIVER_PACKET_TYPE_TX_CB;

  status = switch_pktdriver_tx(&pkt_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR("pktdriver xmit failed:(%s)\n",
                     switch_error_to_string(status));
    return status;
  }

  return status;
}

switch_status_t switch_pktdriver_fd_add(const switch_device_t device,
                                        const switch_fd_t fd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(fd != SWITCH_FD_INVALID);

  pthread_mutex_lock(&fd_mutex);
  status = SWITCH_ARRAY_INSERT(
      &pktdriver_ctx->fd_array, fd, (void *)((uintptr_t)fd));
  pthread_mutex_unlock(&fd_mutex);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet fd add failed on device %d fd 0x%x: "
        "fd array insert failed(%s)\n",
        device,
        fd,
        switch_error_to_string(status));
    return status;
  }

  status = pktdriver_cmd_send(PKTDRV_CMD_WAKE);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_DEBUG("notifying bf_switch_pkdrv thread failed %d fd 0x%x: %s",
                     device,
                     fd,
                     switch_error_to_string(status));
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pktdriver_fd_delete(const switch_device_t device,
                                           const switch_fd_t fd) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(fd != SWITCH_FD_INVALID);

  pthread_mutex_lock(&fd_mutex);
  status = SWITCH_ARRAY_DELETE(&pktdriver_ctx->fd_array, fd);
  pthread_mutex_unlock(&fd_mutex);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet fd delete failed on device %d fd 0x%x: "
        "fd array delete failed(%s)\n",
        device,
        fd,
        switch_error_to_string(status));
    return status;
  }

  status = pktdriver_cmd_send(PKTDRV_CMD_WAKE);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_DEBUG("notifying bf_switch_pkdrv thread failed %d fd 0x%x: %s",
                     device,
                     fd,
                     switch_error_to_string(status));
  }

  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_packet_driver_fd_update(switch_fd_set *read_fds,
                                               switch_int32_t *nfds) {
  switch_fd_t hostif_fd = 0;
  switch_fd_t *hostif_tmp_fd = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t high_fd = 0;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(read_fds != NULL);
  SWITCH_ASSERT(nfds != NULL);
  if (!read_fds || !nfds) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR("packet fd update failed %s",
                     switch_error_to_string(status));
    return status;
  }

  *nfds = 0;
  FD_ZERO(read_fds);

  pthread_mutex_lock(&fd_mutex);
  FOR_EACH_IN_ARRAY(
      hostif_fd, pktdriver_ctx->fd_array, switch_fd_t, hostif_tmp_fd) {
    UNUSED(hostif_tmp_fd);

    FD_SET(hostif_fd, read_fds);
    high_fd = (high_fd > hostif_fd) ? high_fd : hostif_fd;
  }
  FOR_EACH_IN_ARRAY_END();
  pthread_mutex_unlock(&fd_mutex);

  *nfds = high_fd + 1;
  return status;
}

static inline int is_knet_fd(switch_fd_t fd) {
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (pktdriver_ctx->knet_pkt_driver) {
    if (pktdriver_ctx->switch_kern_info.sock_fd == fd) return 1;
  }
  return 0;
}

switch_status_t switch_packet_demux(switch_fd_set *read_fds) {
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  switch_fd_t *tmp_fd = NULL;
  switch_fd_t fd = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  SWITCH_ASSERT(read_fds != NULL);
  if (!read_fds) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR("packet demux failed: %s", switch_error_to_string(status));
    return status;
  }

  pthread_mutex_lock(&fd_mutex);
  FOR_EACH_IN_ARRAY(fd, pktdriver_ctx->fd_array, switch_fd_t, tmp_fd) {
    UNUSED(tmp_fd);

    if (pktdriver_ctx->cpu_fd == fd &&
        FD_ISSET(pktdriver_ctx->cpu_fd, read_fds)) {
      status = switch_pktdriver_cpu_eth_rx(pktdriver_ctx->cpu_fd);
      if (status != SWITCH_STATUS_SUCCESS) {
        SWITCH_PKT_ERROR("packet demux failed from cpu fd 0x%d: %s\n",
                         pktdriver_ctx->cpu_fd,
                         switch_error_to_string(status));
        pthread_mutex_unlock(&fd_mutex);
        return status;
      }
    } else if (is_knet_fd(fd) && FD_ISSET(fd, read_fds)) {
      status = switch_pktdriver_knet_cpu_rx(fd);
      if (status != SWITCH_STATUS_SUCCESS) {
        SWITCH_PKT_ERROR("packet demux failed from knet cpu fd %d: %s\n",
                         fd,
                         switch_error_to_string(status));
        pthread_mutex_unlock(&fd_mutex);
        return status;
      }
    } else if (FD_ISSET(fd, read_fds)) {
      status = switch_pktdriver_netdev_tx(fd);
      if (status != SWITCH_STATUS_SUCCESS) {
        SWITCH_PKT_WARN("packet demux failed from fd %d: %s\n",
                        fd,
                        switch_error_to_string(status));
        pthread_mutex_unlock(&fd_mutex);
        return status;
      }
    }
  }
  FOR_EACH_IN_ARRAY_END();
  pthread_mutex_unlock(&fd_mutex);

  return status;
}

static void *switch_packet_driver(void *args) {
  UNUSED(args);
  switch_fd_set read_fds;
  switch_int32_t num_fd = 0;
  switch_int32_t rc = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  pthread_mutex_lock(&cookie_mutex);
  cookie = 1;
  pthread_cond_signal(&cookie_cv);
  pthread_mutex_unlock(&cookie_mutex);

  while (true) {
    char cmd = PKTDRV_CMD_NONE;

    status = switch_packet_driver_fd_update(&read_fds, &num_fd);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR("packet driver fd update failed: %s",
                       switch_error_to_string(status));
      goto cleanup;
    }

    if (!num_fd) {
      continue;
    }

    rc = switch_select(num_fd, &read_fds, NULL, NULL, NULL);
    if (rc < 0) {
      SWITCH_PKT_ERROR("packet driver select failed: %s",
                       switch_error_to_string(status));
      goto cleanup;
    }

    if (rc == 0) {
      SWITCH_PKT_DEBUG("packet driver without fds. ignoring");
      continue;
    }

    if (FD_ISSET(pktdriver_ctx->pipe_fd[0], &read_fds)) {
      FD_CLR(pktdriver_ctx->pipe_fd[0], &read_fds);
      pktdriver_cmd_recv(&cmd);
      if (cmd == PKTDRV_CMD_EXIT) {
        SWITCH_PKT_DEBUG("packet driver exit");
        goto cleanup;
      }
    }

    switch_packet_demux(&read_fds);
  }

cleanup:
  return NULL;
}

static inline uint64_t bit_mask(uint64_t x) {
  uint64_t temp = 1;
  return (x >= sizeof(uint64_t) * CHAR_BIT) ? (uint64_t)-1 : (temp << x) - 1;
}

static inline switch_status_t switch_knet_rx_filter_create(
    const switch_device_t device,
    const switch_pktdriver_rx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_rx_filter_key_t *rx_key,
    const switch_pktdriver_rx_filter_action_t *rx_action,
    uint64_t *filter_id) {
  switch_knet_info_t *knet_info = NULL;
  switch_knet_rx_filter_t rx_filter;
  switch_cpu_header_t *filter_packet_hdr = NULL;
  switch_cpu_header_t *mask_packet_hdr = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  size_t cpu_hdr_offset = 0;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_MEMSET(&rx_filter, 0, sizeof(switch_knet_rx_filter_t));
  rx_filter.spec.priority = priority;
  /* We index into an array hence -1 */
  cpu_hdr_offset =
      sizeof(switch_ethernet_header_t) + sizeof(switch_fabric_header_t) - 2;
  // Check filter size vs packet size
  filter_packet_hdr =
      (switch_cpu_header_t *)(rx_filter.spec.filter + cpu_hdr_offset);
  mask_packet_hdr =
      (switch_cpu_header_t *)(rx_filter.spec.mask + cpu_hdr_offset);

  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT) {
    filter_packet_hdr->ingress_port = switch_htons(rx_key->dev_port);
    mask_packet_hdr->ingress_port =
        bit_mask(8 * sizeof(filter_packet_hdr->ingress_port));
  }
  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_BD) {
    filter_packet_hdr->ingress_bd = switch_htons(rx_key->bd);
    mask_packet_hdr->ingress_bd =
        bit_mask(8 * sizeof(filter_packet_hdr->ingress_bd));
  }
  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE) {
    filter_packet_hdr->reason_code = switch_htons(rx_key->reason_code);
    mask_packet_hdr->reason_code =
        bit_mask(8 * sizeof(filter_packet_hdr->reason_code));
  }
  if (flags & SWITCH_PKTDRIVER_RX_FILTER_ATTR_PORT_LAG_INDEX) {
    filter_packet_hdr->port_lag_index = switch_htons(rx_key->port_lag_index);
    mask_packet_hdr->port_lag_index =
        bit_mask(8 * sizeof(filter_packet_hdr->port_lag_index));
  }
  rx_filter.spec.filter_size = sizeof(switch_ethernet_header_t) +
                               sizeof(switch_cpu_header_t) +
                               sizeof(switch_fabric_header_t);

  rx_filter.action.dest_proto = 0;
  if (rx_action->knet_hostif_handle) {
    rx_filter.action.dest_type = BF_KNET_FILTER_DESTINATION_HOSTIF;
    rx_filter.action.knet_hostif_id = rx_action->knet_hostif_handle;
    rx_filter.action.count = 1;
    rx_filter.action.pkt_mutation =
        calloc(sizeof(switch_knet_packet_mutation_t), 1);
    if (rx_filter.action.pkt_mutation == NULL) {
      status = SWITCH_STATUS_NO_MEMORY;
      SWITCH_PKT_ERROR(
          "knet hostif rx filter create failed on device %d: "
          "knet action mutation malloc failed:(%s)\n",
          device,
          switch_error_to_string(status));
      return status;
    }
    rx_filter.action.pkt_mutation[0].mutation_type = BF_KNET_RX_MUT_STRIP;
    rx_filter.action.pkt_mutation[0].offset =
        offsetof(switch_ethernet_header_t, ether_type);
    rx_filter.action.pkt_mutation[0].len = sizeof(switch_packet_header_t);
  } else {
    rx_filter.action.dest_type = BF_KNET_FILTER_DESTINATION_CPUIF;
    rx_filter.action.count = 0;
  }

  knet_info = &pktdriver_ctx->switch_kern_info;
  status = bf_knet_rx_filter_add(knet_info->knet_cpuif_id, &rx_filter);
  if (status != SWITCH_STATUS_SUCCESS) goto ret;

  *filter_id = rx_filter.spec.filter_id;
  status = SWITCH_STATUS_SUCCESS;
ret:
  if (rx_filter.action.count > 0) free(rx_filter.action.pkt_mutation);
  return status;
}

switch_status_t switch_pktdriver_rx_filter_create(
    const switch_device_t device,
    const switch_pktdriver_rx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_rx_filter_key_t *rx_key,
    const switch_pktdriver_rx_filter_action_t *rx_action,
    uint64_t *filter_id) {
  switch_pktdriver_rx_filter_info_t *rx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (!rx_key || !rx_action || !filter_id) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver rx filter create failed on device %d: "
        "parameters null:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  if (pktdriver_ctx->knet_pkt_driver) {
    status = switch_knet_rx_filter_create(
        device, priority, flags, rx_key, rx_action, filter_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver rx filter create failed on device %d: "
          "knet rx filter add failed:(%s)\n",
          device,
          switch_error_to_string(status));
      return status;
    }
    return status;
  }

  rx_info = SWITCH_MALLOC(sizeof(switch_pktdriver_rx_filter_info_t), 0x01);
  if (!rx_info) {
    status = SWITCH_STATUS_NO_MEMORY;
    SWITCH_PKT_ERROR(
        "pktdriver rx filter create failed on device %d: "
        "parameters null:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  SWITCH_MEMSET(rx_info, 0, sizeof(switch_pktdriver_rx_filter_info_t));
  SWITCH_MEMCPY(&rx_info->rx_key, rx_key, sizeof(*rx_key));
  SWITCH_MEMCPY(&rx_info->rx_action, rx_action, sizeof(*rx_action));
  rx_info->flags = flags;
  rx_info->priority = priority;

  status =
      SWITCH_LIST_INSERT(&pktdriver_ctx->rx_filter, &rx_info->node, rx_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver rx filter create failed on device %d: "
        "rx filter list insert failed:(%s)\n",
        device,
        switch_error_to_string(status));
    SWITCH_FREE(rx_info);
    return status;
  }

  status = SWITCH_LIST_SORT(&pktdriver_ctx->rx_filter,
                            switch_packet_driver_rx_filter_priority_compare);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver rx filter create failed on device %d: "
        "rx filter sort failed:(%s)\n",
        device,
        switch_error_to_string(status));
    SWITCH_FREE(rx_info);
    return status;
  }

  *filter_id = (uintptr_t)rx_info;

  if (rx_action->channel_type == SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV) {
    pktdriver_ctx->dev_port_to_fd_map[rx_key->dev_port] = rx_action->fd;
  }

  SWITCH_PKT_DEBUG(
      "packet driver rx filter created on device %d "
      "handle 0x%" PRIx64 "\n",
      device,
      *filter_id);

  return status;
}

switch_status_t switch_pktdriver_rx_filter_delete(
    const switch_device_t device,
    switch_pktdriver_rx_filter_key_t *rx_key,
    const uint64_t filter_id) {
  UNUSED(rx_key);
  switch_pktdriver_rx_filter_info_t *rx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (filter_id == 0 || !rx_key) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver rx filter delete failed on device %d: "
        "parameters null:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }
  rx_info = (switch_pktdriver_rx_filter_info_t *)((uintptr_t)filter_id);
  if (pktdriver_ctx->knet_pkt_driver) {
    status = bf_knet_rx_filter_delete(
        pktdriver_ctx->switch_kern_info.knet_cpuif_id, filter_id);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver rx filter delete failed on device %d: "
          "knet filter delete failed:(%s)\n",
          device,
          switch_error_to_string(status));
      return status;
    }
    return status;
  }

  if (rx_info->rx_action.channel_type == SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV) {
    pktdriver_ctx->dev_port_to_fd_map[rx_key->dev_port] = -1;
  }

#if 0
  status = switch_pktdriver_rx_filter_info_get(rx_key, &rx_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet driver rx filter delete failed on device %d "
        "rx filter list delete failed:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }
#endif

  status = SWITCH_LIST_DELETE(&pktdriver_ctx->rx_filter, &rx_info->node);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet driver rx filter delete failed on device %d "
        "rx filter list delete failed:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  SWITCH_PKT_DEBUG("packet driver rx filter deleted on device %d ", device);
  SWITCH_FREE(rx_info);
  return status;
}

switch_status_t switch_pktdriver_rx_filter_num_packets_get(
    const switch_device_t device, const uint64_t filter_id, uint64_t *counter) {
  UNUSED(device);
  switch_pktdriver_rx_filter_info_t *rx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  rx_info = (switch_pktdriver_rx_filter_info_t *)((uintptr_t)filter_id);
  if (rx_info) *counter = rx_info->num_packets;

  return status;
}

switch_status_t switch_pktdriver_rx_filter_num_packets_clear(
    const switch_device_t device, const uint64_t filter_id) {
  UNUSED(device);
  switch_pktdriver_rx_filter_info_t *rx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  rx_info = (switch_pktdriver_rx_filter_info_t *)((uintptr_t)filter_id);
  if (rx_info) rx_info->num_packets = 0;

  return status;
}

static switch_status_t switch_knet_tx_filter_create(
    const switch_device_t device,
    const switch_pktdriver_tx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_tx_filter_key_t *tx_key,
    const switch_pktdriver_tx_filter_action_t *tx_action,
    switch_handle_t *tx_filter_handle) {
  switch_knet_info_t *knet_info = NULL;
  switch_knet_tx_action_t knet_tx_action;
  switch_packet_header_t *packet_hdr = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  UNUSED(tx_filter_handle);
  UNUSED(tx_key);
  UNUSED(priority);
  UNUSED(flags);

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_MEMSET(&knet_tx_action, 0, sizeof(switch_knet_tx_action_t));
  knet_tx_action.count = 1;
  knet_tx_action.pkt_mutation =
      calloc(sizeof(switch_knet_packet_mutation_t), 1);
  if (knet_tx_action.pkt_mutation == NULL) {
    status = SWITCH_STATUS_NO_MEMORY;
    SWITCH_PKT_ERROR(
        "knet hostif tx filter create failed on device %d: "
        "knet action mutation malloc failed:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }
  knet_tx_action.pkt_mutation[0].mutation_type = BF_KNET_RX_MUT_INSERT;
  knet_tx_action.pkt_mutation[0].offset =
      offsetof(switch_ethernet_header_t, ether_type);
  knet_tx_action.pkt_mutation[0].len = sizeof(switch_packet_header_t);
  if (knet_tx_action.pkt_mutation[0].offset +
          knet_tx_action.pkt_mutation[0].len <
      BF_KNET_DATA_BYTES_MAX) {
    packet_hdr =
        (switch_packet_header_t *)(knet_tx_action.pkt_mutation[0].data);
  } else {
    status = SWITCH_STATUS_FAILURE;
    SWITCH_PKT_ERROR(
        "knet hostif tx filter create failed on device %d: "
        "knet action mutation index %d is greater than knet max muation index "
        "%d\n",
        device,
        knet_tx_action.pkt_mutation[0].offset +
            knet_tx_action.pkt_mutation[0].len,
        BF_KNET_DATA_BYTES_MAX);
    free(knet_tx_action.pkt_mutation);
    return status;
  }

  SWITCH_MEMSET(knet_tx_action.pkt_mutation[0].data, 0, BF_KNET_DATA_BYTES_MAX);
  if (tx_action->bypass_flags == SWITCH_BYPASS_ALL) {
    packet_hdr->cpu_header.port_lag_index =
        switch_htons(tx_action->port_lag_index);
    packet_hdr->cpu_header.tx_bypass = true;
  } else {
    packet_hdr->cpu_header.port_lag_index = SWITCH_INVALID_HW_PORT;
    packet_hdr->cpu_header.ingress_bd = switch_htons(tx_action->bd);
  }
  packet_hdr->cpu_header.reason_code = switch_htons(tx_action->bypass_flags);
  packet_hdr->fabric_header.packet_type = SWITCH_FABRIC_HEADER_TYPE_CPU;
  packet_hdr->fabric_header.ether_type =
      switch_htons(SWITCH_FABRIC_HEADER_ETHTYPE);
  packet_hdr->cpu_header.egress_queue =
      SWITCH_PKTDRIVER_TX_EGRESS_QUEUE_DEFAULT;

  knet_info = &pktdriver_ctx->switch_kern_info;
  status = bf_knet_tx_action_add(
      knet_info->knet_cpuif_id, tx_key->knet_hostif_handle, &knet_tx_action);
  return status;
}

switch_status_t switch_pktdriver_tx_filter_create(
    const switch_device_t device,
    const switch_pktdriver_tx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_tx_filter_key_t *tx_key,
    const switch_pktdriver_tx_filter_action_t *tx_action,
    switch_handle_t *tx_filter_handle) {
  switch_pktdriver_tx_filter_info_t *tx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  SWITCH_ASSERT(tx_key && tx_action);
  if (!tx_key || !tx_action) {
    status = SWITCH_STATUS_INVALID_PARAMETER;
    SWITCH_PKT_ERROR(
        "pktdriver tx filter create failed on device %d: "
        "parameters null:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  if (pktdriver_ctx->knet_pkt_driver) {
    status = switch_knet_tx_filter_create(
        device, priority, flags, tx_key, tx_action, tx_filter_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver tx filter create failed on device %d handle 0x%" PRIx64
          ": "
          "knet tx filter add failed:(%s)\n",
          device,
          tx_key->knet_hostif_handle,
          switch_error_to_string(status));
      return status;
    }
    return status;
  }

  tx_info = SWITCH_MALLOC(sizeof(switch_pktdriver_tx_filter_info_t), 0x01);
  if (!tx_info) {
    status = SWITCH_STATUS_NO_MEMORY;
    SWITCH_PKT_ERROR(
        "pktdriver tx filter create failed on device %d: "
        "parameters null:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  SWITCH_MEMSET(tx_info, 0, sizeof(switch_pktdriver_tx_filter_info_t));
  SWITCH_MEMCPY(&tx_info->tx_key, tx_key, sizeof(*tx_key));
  SWITCH_MEMCPY(&tx_info->tx_action, tx_action, sizeof(*tx_action));

  status =
      SWITCH_LIST_INSERT(&pktdriver_ctx->tx_filter, &tx_info->node, tx_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver tx filter create failed on device %d: "
        "tx filter list insert failed:(%s)\n",
        device,
        switch_error_to_string(status));
    SWITCH_FREE(tx_info);
    return status;
  }

  status = SWITCH_LIST_SORT(&pktdriver_ctx->tx_filter,
                            switch_packet_driver_tx_filter_priority_compare);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver tx filter create failed on device %d: "
        "tx filter sort failed:(%s)\n",
        device,
        switch_error_to_string(status));
    SWITCH_FREE(tx_info);
    return status;
  }

  tx_info->flags = flags;
  tx_info->priority = priority;

  *tx_filter_handle = (uintptr_t)tx_info;
  SWITCH_PKT_DEBUG("packet driver tx filter created on device %d ", device);

  return status;
}

switch_status_t switch_pktdriver_tx_filter_delete(
    const switch_device_t device,
    switch_pktdriver_tx_filter_key_t *tx_key,
    uint64_t tx_filter_handle) {
  switch_pktdriver_tx_filter_info_t *tx_info = NULL;
  switch_knet_info_t *knet_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  UNUSED(knet_info);
  UNUSED(tx_key);
  UNUSED(device);

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    status =
        bf_knet_tx_action_delete(knet_info->knet_cpuif_id, tx_filter_handle);
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR(
          "pktdriver tx filter delete failed on device %d handle 0x%" PRIx64
          ": "
          "knet tx action delete failed: (%s)\n",
          device,
          tx_filter_handle,
          switch_error_to_string(status));
      return status;
    }
    return status;
  }

  tx_info = (switch_pktdriver_tx_filter_info_t *)((uintptr_t)tx_filter_handle);

#if 0
  status = switch_pktdriver_tx_filter_info_get(tx_key, &tx_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet driver tx filter delete failed on device %d "
        "tx filter get failed:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }
#endif

  status = SWITCH_LIST_DELETE(&pktdriver_ctx->tx_filter, &tx_info->node);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "packet driver tx filter delete failed on device %d "
        "tx filter list delete failed:(%s)\n",
        device,
        switch_error_to_string(status));
    return status;
  }

  SWITCH_PKT_DEBUG(
      "packet driver tx filter deleted on device %d "
      "handle 0x%" PRIx64 "\n",
      device,
      tx_filter_handle);

  SWITCH_FREE(tx_info);
  return status;
}

switch_status_t switch_pktdriver_tx_filter_num_packets_get(
    const switch_device_t device, const uint64_t filter_id, uint64_t *counter) {
  UNUSED(device);
  switch_pktdriver_tx_filter_info_t *tx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  tx_info = (switch_pktdriver_tx_filter_info_t *)((uintptr_t)filter_id);
  if (tx_info) *counter = tx_info->num_packets;

  return status;
}

switch_status_t switch_pktdriver_tx_filter_num_packets_clear(
    const switch_device_t device, const uint64_t filter_id) {
  UNUSED(device);
  switch_pktdriver_tx_filter_info_t *tx_info = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  tx_info = (switch_pktdriver_tx_filter_info_t *)((uintptr_t)filter_id);
  if (tx_info) tx_info->num_packets = 0;

  return status;
}

switch_status_t switch_pktdriver_reason_code_stats_get(
    const switch_device_t device,
    const uint16_t reason_code,
    uint64_t *pkts,
    uint64_t *bytes) {
  UNUSED(device);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  *pkts = pktdriver_ctx->rx_rc_pkts[reason_code].count;
  *bytes = pktdriver_ctx->rx_rc_bytes[reason_code].count;
  return status;
}

switch_status_t switch_pktdriver_reason_code_stats_clear(
    const switch_device_t device, const uint16_t reason_code) {
  UNUSED(device);
  pktdriver_ctx->rx_rc_pkts[reason_code].count = 0;
  pktdriver_ctx->rx_rc_bytes[reason_code].count = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pktdriver_reason_code_pkts_clear(
    const switch_device_t device, const uint16_t reason_code) {
  UNUSED(device);
  pktdriver_ctx->rx_rc_pkts[reason_code].count = 0;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pktdriver_reason_code_bytes_clear(
    const switch_device_t device, const uint16_t reason_code) {
  UNUSED(device);
  pktdriver_ctx->rx_rc_bytes[reason_code].count = 0;
  return SWITCH_STATUS_SUCCESS;
}

bf_status_t switch_pktdriver_cpu_pcie_rx(bf_dev_id_t device,
                                         bf_pkt *pkt,
                                         void *data,
                                         bf_pkt_rx_ring_t rx_ring) {
  UNUSED(data);
  UNUSED(rx_ring);
  bf_pkt *orig_pkt = NULL;
  char in_packet[SWITCH_PACKET_MAX_BUFFER_SIZE];
  switch_packet_info_t pkt_info;
  char *pkt_buf = NULL;
  char *bufp = NULL;
  uint32_t packet_size = 0;
  switch_int32_t pkt_len = 0;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  /* save a copy of the original packet */
  orig_pkt = pkt;

  /* assemble the received packet */
  bufp = &in_packet[0];
  do {
    pkt_buf = (char *)bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);
    if ((packet_size + pkt_len) > SWITCH_PACKET_MAX_BUFFER_SIZE) {
      SWITCH_PKT_ERROR("Packet too large to Transmit - Skipping\n");
      break;
    }
    SWITCH_MEMCPY(bufp, pkt_buf, pkt_len);
    bufp += pkt_len;
    packet_size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  /* free the packet */
  bf_pkt_free(device, orig_pkt);

  /* process the received packet buffer */
  SWITCH_MEMSET(&pkt_info, 0x0, sizeof(pkt_info));
  pkt_info.pkt_type = SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_PCIE;
  pkt_info.pkt = in_packet;
  pkt_info.pkt_size = packet_size;

  status = switch_pktdriver_rx(&pkt_info);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "%s.%d: pktdriver cpu pcie rx failed: "
        "pktdriver rx failed:(%s)\n",
        __func__,
        __LINE__,
        switch_error_to_string(status));
    return 0;
  }

  return 0;
}

static void switch_pktdriver_callback_register(switch_device_t device) {
#ifndef TESTING
  /* register callback for TX complete */
  for (bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;
       tx_ring < BF_PKT_TX_RING_MAX;
       tx_ring++) {
    bf_pkt_tx_done_notif_register(
        device, switch_pktdriver_tx_complete, tx_ring);
  }
#endif

  /* register callback for RX */
  for (bf_pkt_rx_ring_t rx_ring = BF_PKT_RX_RING_0;
       rx_ring < BF_PKT_RX_RING_MAX;
       rx_ring++) {
    bf_pkt_rx_register(device, switch_pktdriver_cpu_pcie_rx, rx_ring, 0);
  }
}

static void switch_pktdriver_callback_deregister(switch_device_t device) {
  (void)device;
#ifndef TESTING
  /* register callback for TX complete */
  for (bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;
       tx_ring < BF_PKT_TX_RING_MAX;
       tx_ring++) {
    bf_pkt_tx_done_notif_deregister(device, tx_ring);
  }

  /* register callback for RX */
  for (bf_pkt_rx_ring_t rx_ring = BF_PKT_RX_RING_0;
       rx_ring < BF_PKT_RX_RING_MAX;
       rx_ring++) {
    bf_pkt_rx_deregister(device, rx_ring);
  }
#endif
}

switch_status_t start_bf_switch_api_packet_driver(void) {
  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;
  if (!pktdriver_ctx->use_kpkt && pktdriver_ctx->use_pcie) {
    if (bf_pkt_is_inited(0)) {
      switch_pktdriver_callback_register(0);
    } else {
      SWITCH_PKT_ERROR(
          "Failed to start packet driver in kdrv mode. Pkt mgr is "
          "unitialized\n");
      return SWITCH_STATUS_FAILURE;
    }
  }
  pthread_mutex_init(&cookie_mutex, NULL);
  pthread_cond_init(&cookie_cv, NULL);
  int status =
      pthread_create(&packet_driver_thread, NULL, switch_packet_driver, NULL);
  if (status) return status;
  pthread_setname_np(packet_driver_thread, "bf_switch_pkdrv");
  pthread_mutex_lock(&cookie_mutex);
  while (!cookie) {
    pthread_cond_wait(&cookie_cv, &cookie_mutex);
  }
  pthread_mutex_unlock(&cookie_mutex);
  pthread_mutex_destroy(&cookie_mutex);
  pthread_cond_destroy(&cookie_cv);
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t stop_bf_switch_api_packet_driver(void) {
  switch_knet_info_t *knet_info = NULL;
  switch_status_t status = pktdriver_cmd_send(PKTDRV_CMD_EXIT);

  if (status == SWITCH_STATUS_SUCCESS) {
    pthread_join(packet_driver_thread, NULL);
  }

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    if (knet_info->sock_fd != SWITCH_FD_INVALID) {
      switch_fd_close(knet_info->sock_fd);
    }
  } else {
    status = switch_pktdriver_free();
    if (status != SWITCH_STATUS_SUCCESS) {
      SWITCH_PKT_ERROR("packet driver cleanup failed!");
    }
  }

  if (!pktdriver_ctx->use_kpkt && pktdriver_ctx->use_pcie) {
    if (bf_pkt_is_inited(0)) {
      switch_pktdriver_callback_deregister(0);
    } else {
      SWITCH_PKT_ERROR(
          "Failed to stop packet driver in kdrv mode. Pkt mgr is "
          "unitialized\n");
      return SWITCH_STATUS_FAILURE;
    }
  }
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t switch_pkt_alloc(switch_device_t device,
                                 char **pkt,
                                 int pkt_size) {
  if (!pkt) return SWITCH_STATUS_FAILURE;
  pkt_size = pkt_size + sizeof(switch_packet_header_t);
  if (pktdriver_ctx->use_pcie) {
    if (bf_pkt_alloc(
            device, (bf_pkt **)&pkt, pkt_size, BF_DMA_CPU_PKT_TRANSMIT_0) !=
        0) {
      return SWITCH_STATUS_FAILURE;
    }
  } else {
    *pkt = SWITCH_MALLOC(pkt_size, 0x01);
    if (!(*pkt)) return SWITCH_STATUS_NO_MEMORY;
  }
  *pkt = *pkt + sizeof(switch_packet_header_t);
  return SWITCH_STATUS_SUCCESS;
}

void switch_pkt_free(switch_device_t device, char *pkt) {
  if (pktdriver_ctx->use_pcie) {
    bf_pkt_free(device, (bf_pkt *)pkt);
  } else {
    SWITCH_FREE(pkt);
  }
}

switch_status_t switch_pktdriver_cpu_pkt_noalloc_tx(switch_device_t device,
                                                    switch_int8_t *out_packet,
                                                    switch_int32_t pkt_size) {
  switch_knet_info_t *knet_info = NULL;
  UNUSED(knet_info);
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_int32_t rc = 0;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  if (pktdriver_ctx->knet_pkt_driver) {
    knet_info = &pktdriver_ctx->switch_kern_info;
    rc = switch_fd_send(knet_info->sock_fd,
                        out_packet,
                        pkt_size,
                        0x0,
                        (struct sockaddr *)&knet_info->s_addr,
                        sizeof(struct sockaddr_ll));
    if (rc < 0) {
      SWITCH_PKT_ERROR(
          "pktdriver cp tx failed: fd=%d "
          "pkt netdev write failed: errno=%s\n",
          knet_info->sock_fd,
          strerror(errno));
      return SWITCH_STATUS_FAILURE;
    }
  } else if (pktdriver_ctx->use_pcie) {
    bf_pkt_tx_ring_t tx_ring = BF_PKT_TX_RING_0;

    if (bf_pkt_tx(device, (bf_pkt *)out_packet, tx_ring, (void *)out_packet) !=
        BF_SUCCESS) {
      bf_pkt_free(device, (bf_pkt *)out_packet);
    }
    // return failure;
  } else {
    struct sockaddr_ll addr;
    SWITCH_MEMSET(&addr, 0x0, sizeof(addr));
    addr.sll_ifindex = pktdriver_ctx->cpu_ifindex;

    for (int retry = 2; retry > 0; retry--) {
      fd_set write_set;
      struct timeval timeout;

      /* Initialize the file descriptor set. */
      FD_ZERO(&write_set);
      FD_SET(pktdriver_ctx->cpu_fd, &write_set);

      /* Initialize the timeout data structure. */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      /* select returns 0 if timeout, 1 if input available, -1 if error. */
      rc = select(FD_SETSIZE, NULL, &write_set, NULL, &timeout);
      if (rc < 0) {
        SWITCH_PKT_ERROR("cpu tx select failed: %s", strerror(errno));
        continue;
      }

      if (!FD_ISSET((pktdriver_ctx->cpu_fd), &write_set)) {
        SWITCH_PKT_DEBUG(
            "pktdriver cp tx failed: cpu_fd is not ready to write\n");
        continue;
      }

      rc = switch_fd_send(pktdriver_ctx->cpu_fd,
                          out_packet,
                          pkt_size,
                          0x0,
                          (struct sockaddr *)&addr,
                          sizeof(addr));
      if (rc >= 0) {
        break;
      }

      SWITCH_PKT_ERROR("pktdriver cpu tx failed: errno=%s\n", strerror(errno));
      return SWITCH_STATUS_FAILURE;
    }
  }

  return status;
}

/**
 * @brief This function differs from switch_pktdriver_tx that we do a memmove
 *and not memcpy here
 *        and insert the packet header for the packet already allocated.
 *        We only deal with the user hostif packet here and not the netdev path.
 *
 * @retval SWITCH_STATUS_SUCCESS Initialization success
 * @retval SWITCH_STATUS_FAILURE failed to allocate memory.
 */

switch_status_t switch_pktdriver_pkt_tx(switch_packet_info_t *pkt_info) {
  uint16_t out_offset = 0;
  uint32_t pkt_size = 0;
  char *start = NULL;
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  if (!pktdriver_ctx) return SWITCH_STATUS_FAILURE;

  pktdriver_ctx->num_tx_cb_packets++;
  SWITCH_PKTINFO_PACKET_TYPE(pkt_info) = SWITCH_FABRIC_HEADER_TYPE_CPU;
  SWITCH_PKTINFO_ETHER_TYPE(pkt_info) = SWITCH_FABRIC_HEADER_ETHTYPE;

  if (pktdriver_ctx->tx_pkt_trace_enable) {
    switch_pktdriver_packet_dump(pkt_info, false);
  }

  start = pkt_info->pkt;
  start = start - sizeof(switch_packet_header_t);
  SWITCH_MEMMOVE(start, pkt_info->pkt, SWITCH_PACKET_HEADER_OFFSET);
  out_offset += SWITCH_PACKET_HEADER_OFFSET;

  SWITCH_PACKET_HEADER_HTON(pkt_info->pkt_header);
  SWITCH_MEMCPY((start + out_offset),
                &pkt_info->pkt_header,
                sizeof(switch_packet_header_t));
  pkt_info->pkt_size = pkt_info->pkt_size + sizeof(switch_packet_header_t);
  pkt_size = pkt_info->pkt_size;
  pkt_info->pkt = start;

  status = switch_pktdriver_cpu_pkt_noalloc_tx(
      pkt_info->device, pkt_info->pkt, pkt_size);
  if (status != SWITCH_STATUS_SUCCESS) {
    SWITCH_PKT_ERROR(
        "pktdriver_pkt tx failed: "
        "pktdriver_cpu_noalloc tx failed:(%s)\n",
        switch_error_to_string(status));
    return status;
  }

  pktdriver_ctx->num_tx_packets++;

  return status;
}
