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


#ifndef S3_SWITCH_PACKET_H__
#define S3_SWITCH_PACKET_H__

#include "bf_switch/bf_switch_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SWITCH_PKTDRIVER_TX_EGRESS_QUEUE_DEFAULT 7

#define SWITCH_MAX_GENL_PACKET_CONTEXTS 0x1000 /* 4096 */

/* The reason code is 16-bits value. Four MSBs are used to identify
 * the reason code type. At this moment the following types are defined:
 *   - Generic reason code type                 - 0
 *   - sFlow reason code type                   - 1
 *   - Generic netlink packet reason code type  - 2
 *   - User defined trap reason code type       - 4
 * */
#define SWITCH_REASON_CODE_TYPE_MASK 0xF000
#define SWITCH_REASON_CODE_VALUE_MASK 0x0FFF

#define SWITCH_GENERIC_REASON_CODE 0x0000
#define SWITCH_SFLOW_REASON_CODE 0x1000
#define SWITCH_UDT_REASON_CODE 0x2000
#define SWITCH_BFD_REASON_CODE 0x4000

#define BFD_SESSION_ID_WIDTH 12

/** ARP Opcodes */
typedef enum switch_arp_opcode_s {
  SWITCH_ARP_OPCODE_NONE = 0,
  SWITCH_ARP_OPCODE_REQ = 1,
  SWITCH_ARP_OPCODE_RES = 2
} switch_arp_opcode_t;

typedef enum switch_tx_bypass_flags_s {
  SWITCH_BYPASS_NONE = 0x0,
  SWITCH_BYPASS_L2 = (1 << 0),
  SWITCH_BYPASS_L3 = (1 << 1),
  SWITCH_BYPASS_ACL = (1 << 2),
  SWITCH_BYPASS_SYSTEM_ACL = (1 << 3),
  SWITCH_BYPASS_QOS = (1 << 4),
  SWITCH_BYPASS_METER = (1 << 5),
  SWITCH_BYPASS_STORM_CONTROL = (1 << 6),
  SWITCH_BYPASS_STP = (1 << 7),
  SWITCH_BYPASS_SMAC = (1 << 8),
  SWITCH_BYPASS_NAT = (1 << 9),
  SWITCH_BYPASS_ROUTING_CHECK = (1 << 10),
  SWITCH_BYPASS_PV = (1 << 11),
  SWITCH_BYPASS_ALL = 0xFFFF
} switch_tx_bypass_flags_t;

typedef enum switch_pktdriver_channel_type_s {
  SWITCH_PKTDRIVER_CHANNEL_TYPE_CB = 1,
  SWITCH_PKTDRIVER_CHANNEL_TYPE_NETDEV = 2,
  SWITCH_PKTDRIVER_CHANNEL_TYPE_FD = 3,
  SWITCH_PKTDRIVER_CHANNEL_TYPE_GENL = 4,
  SWITCH_PKTDRIVER_CHANNEL_TYPE_CB_AND_NETDEV = 5,
} switch_pktdriver_channel_type_t;

typedef enum switch_pktdriver_rx_filter_attr_s {
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_DEV_PORT = (1 << 0),
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_PORT_LAG_INDEX = (1 << 1),
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_BD = (1 << 2),
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_REASON_CODE = (1 << 3),
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_ETHER_TYPE = (1 << 4),
  SWITCH_PKTDRIVER_RX_FILTER_ATTR_GLOBAL = (1 << 5)
} switch_pktdriver_rx_filter_attr_t;

typedef enum switch_pktdriver_tx_filter_attr_s {
  SWITCH_PKTDRIVER_TX_FILTER_ATTR_HOSTIF_FD = (1 << 0),
  SWITCH_PKTDRIVER_TX_FILTER_ATTR_VLAN_ID = (1 << 1)
} switch_pktdriver_tx_filter_attr_t;

typedef enum switch_pktdriver_vlan_action_s {
  SWITCH_PACKET_VLAN_ACTION_NONE = 0x0,
  SWITCH_PACKET_VLAN_ACTION_ADD = 0x1,
  SWITCH_PACKET_VLAN_ACTION_REMOVE = 0x2,
} switch_pktdriver_vlan_action_t;

typedef enum switch_pktdriver_rx_filter_priority_s {
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MIN = 0x0,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PORT = 0x1,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_INTERFACE = 0x2,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_VLAN = 0x3,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_RIF = 0x4,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_TRAP = 0x5,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB = 0x6,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_CB_AND_NETDEV = 0x7,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PRIO1 = 0x10,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PRIO2 = 0x11,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PRIO3 = 0x12,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_PRIO4 = 0x13,
  SWITCH_PKTDRIVER_RX_FILTER_PRIORITY_MAX = 0x14
} switch_pktdriver_rx_filter_priority_t;

typedef enum switch_pktdriver_tx_filter_priority_s {
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_MIN = 0x0,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_HOSTIF = 0x1,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_PRIO1 = 0x4,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_PRIO2 = 0x5,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_PRIO3 = 0x6,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_PRIO4 = 0x7,
  SWITCH_PKTDRIVER_TX_FILTER_PRIORITY_MAX = 0x8
} switch_pktdriver_tx_filter_priority_t;

typedef enum switch_pkt_hostif_attr_s {
  SWITCH_PKT_HOSTIF_ATTR_MAC_ADDRESS = (1 << 0),
  SWITCH_PKT_HOSTIF_ATTR_IPV4_ADDRESS = (1 << 1),
  SWITCH_PKT_HOSTIF_ATTR_IPV6_ADDRESS = (1 << 2),
  SWITCH_PKT_HOSTIF_ATTR_INTERFACE_NAME = (1 << 3),
  SWITCH_PKT_HOSTIF_ATTR_ADMIN_STATE = (1 << 4),
  SWITCH_PKT_HOSTIF_ATTR_OPER_STATUS = (1 << 5),
  SWITCH_PKT_HOSTIF_ATTR_VLAN_ACTION = (1 << 6),
  SWITCH_PKT_HOSTIF_ATTR_GENL_MCGRP_NAME = (1 << 7),
  SWITCH_PKT_HOSTIF_ATTR_IFINDEX = (1 << 8),
  SWITCH_PKT_HOSTIF_ATTR_USE_IFINDEX = (1 << 9),
} switch_pkt_hostif_attr_t;

/** host interface */
typedef struct switch_pkt_hostif_info_s {
  /** netdev interface name */
  switch_string_t intf_name;

  /** netdev interface index */
  uint64_t ifindex;

  /** to use netdev interface index */
  bool use_ifindex;

  /** genetlink mcgroup name */
  switch_string_t genetlink_mcgrp_name;

  /** hostif mac address */
  switch_mac_addr_t mac;

  /** hostif v4 ip address */
  switch_ip_prefix_t v4addr;

  /** hostif v6 ip address */
  switch_ip_address_t v6addr;

  /** @todo: vlan action */
  // switch_hostif_vlan_action_t vlan_action;

  /** oper status */
  bool operstatus;

  /** admin state */
  bool admin_state;
} switch_pkt_hostif_info_t;

typedef struct switch_pktdriver_rx_filter_key_s {
  /** port number */
  uint16_t dev_port;

  /** port_lag_index */
  uint32_t port_lag_index;

  /** bridge domain */
  uint16_t bd;

  /** reason code */
  uint16_t reason_code;

  /** reason code mask */
  uint16_t reason_code_mask;
} switch_pktdriver_rx_filter_key_t;

/** Per rx filter callback for channel type CB_AND_NETDEV */
typedef void (*switch_pktdriver_rx_filter_callback)(char *pkt,
                                                    int pkt_size,
                                                    uint16_t reason_code);

typedef struct switch_pktdriver_rx_filter_action_s {
  /** channel type */
  switch_pktdriver_channel_type_t channel_type;

  /** hostif fd */
  int fd;

  /** knet hostif handle - used with kernel packet processing */
  uint64_t knet_hostif_handle;

  /** vlan action */
  switch_pktdriver_vlan_action_t vlan_action;

  /** s3 hostif_trap_handle */
  uint64_t hostif_trap_handle;

  /** callback when channel_type is CB_AND_NETDEV only */
  switch_pktdriver_rx_filter_callback cb;
} switch_pktdriver_rx_filter_action_t;

typedef struct switch_pktdriver_tx_filter_key_s {
  /** netdev fd */
  int hostif_fd;

  /** knet hostif handle - used with kernel packet processing */
  uint64_t knet_hostif_handle;
} switch_pktdriver_tx_filter_key_t;

typedef struct switch_pktdriver_tx_filter_action_s {
  /** tx bypass flags */
  uint16_t bypass_flags;

  /** bd if tx bypass is false */
  uint16_t bd;

  /** dev port if tx bypass is true */
  uint16_t dev_port;

  /** port_lag_index if tx bypass is true */
  uint16_t port_lag_index;
} switch_pktdriver_tx_filter_action_t;

bool switch_pktdriver_mode_is_kernel();
void switch_pkt_dump_enable(bool enable);

switch_status_t switch_pkt_dev_port_to_ifindex_set(uint16_t dev_port,
                                                   const char *intf_name);

switch_status_t switch_pkt_dev_port_to_port_handle_set(uint16_t dev_port,
                                                       uint64_t port_handle);

switch_status_t switch_pkt_sflow_id_to_rate_set(uint8_t sflow_id,
                                                uint32_t rate,
                                                uint32_t pipe_id);

switch_status_t switch_pktdriver_knet_device_add(const uint16_t device);

switch_status_t switch_pktdriver_knet_device_delete(const uint16_t device);

switch_status_t start_bf_switch_api_packet_driver();
switch_status_t stop_bf_switch_api_packet_driver(void);

switch_status_t switch_packet_init(const char *cpu_port,
                                   bool use_pcie,
                                   bool use_kpkt);

switch_status_t switch_packet_clean();

switch_status_t switch_pktdriver_init();

switch_status_t switch_pktdriver_free(void);

switch_status_t switch_pktdriver_fd_add(const uint16_t device, const int fd);

switch_status_t switch_pktdriver_fd_delete(const uint16_t device, const int fd);

switch_status_t switch_pktdriver_rx_filter_create(
    const uint16_t device,
    const switch_pktdriver_rx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_rx_filter_key_t *rx_key,
    const switch_pktdriver_rx_filter_action_t *rx_action,
    uint64_t *filter_id);

switch_status_t switch_pktdriver_rx_filter_delete(
    const uint16_t device,
    switch_pktdriver_rx_filter_key_t *rx_key,
    uint64_t filter_id);

switch_status_t switch_pktdriver_rx_filter_num_packets_get(
    const uint16_t device, const uint64_t filter_id, uint64_t *counter);
switch_status_t switch_pktdriver_rx_filter_num_packets_clear(
    const uint16_t device, const uint64_t filter_id);

switch_status_t switch_pktdriver_tx_filter_create(
    const uint16_t device,
    const switch_pktdriver_tx_filter_priority_t priority,
    const uint64_t flags,
    const switch_pktdriver_tx_filter_key_t *tx_key,
    const switch_pktdriver_tx_filter_action_t *tx_action,
    uint64_t *tx_filter_handle);

switch_status_t switch_pktdriver_tx_filter_delete(
    const uint16_t device,
    switch_pktdriver_tx_filter_key_t *tx_key,
    uint64_t tx_filter_handle);

switch_status_t switch_pktdriver_tx_filter_num_packets_get(
    const uint16_t device, const uint64_t filter_id, uint64_t *counter);
switch_status_t switch_pktdriver_tx_filter_num_packets_clear(
    const uint16_t device, const uint64_t filter_id);

switch_status_t switch_pktdriver_reason_code_stats_get(
    const uint16_t device,
    const uint16_t reason_code,
    uint64_t *pkts,
    uint64_t *bytes);
switch_status_t switch_pktdriver_reason_code_stats_clear(
    const uint16_t device, const uint16_t reason_code);

switch_status_t switch_pktdriver_reason_code_pkts_clear(
    const uint16_t device, const uint16_t reason_code);

switch_status_t switch_pktdriver_reason_code_bytes_clear(
    const uint16_t device, const uint16_t reason_code);

switch_status_t switch_pktdriver_bd_to_vlan_mapping_add(uint16_t device,
                                                        uint16_t bd,
                                                        uint16_t vlan);

switch_status_t switch_pktdriver_bd_to_vlan_mapping_delete(uint16_t device,
                                                           uint16_t bd,
                                                           uint16_t vlan);

switch_status_t switch_pktdriver_knet_device_add(const uint16_t device);

switch_status_t switch_pktdriver_knet_device_delete(const uint16_t device);

switch_status_t switch_pkt_genetlink_create(
    uint16_t device, const switch_pkt_hostif_info_t *hostif_info);

switch_status_t switch_pkt_genetlink_delete(
    uint16_t device, const switch_pkt_hostif_info_t *hostif_info);

switch_status_t switch_pkt_hostif_create(
    uint16_t device,
    const switch_pkt_hostif_info_t *hostif_info,
    const uint64_t flags,
    int *fd,
    uint64_t *knet_hostif_handle);

switch_status_t switch_pkt_hostif_update(
    uint16_t device,
    const switch_pkt_hostif_info_t *hostif_info,
    const uint64_t flags);

switch_status_t switch_pkt_hostif_delete(uint16_t device,
                                         int fd,
                                         uint64_t knet_hostif_handle);
switch_status_t switch_pkt_hostif_set_interface_admin_state(
    uint16_t device, const char *intf_name, bool state);

int switch_pkt_hostif_set_interface_oper_state(const uint16_t device,
                                               const char *intf_name,
                                               bool state);

char *switch_strncpy(char *dest, const char *src, size_t n);

switch_status_t switch_pkt_alloc(const uint16_t device,
                                 char **pkt,
                                 int pkt_size);
void switch_pkt_free(const uint16_t device, char *pkt);

switch_status_t switch_pkt_xmit(char *pkt, int pkt_size);

typedef void (*switch_internal_callback_rx)(char *pkt,
                                            int pkt_size,
                                            uint64_t port_lag_handle,
                                            uint64_t hostif_trap_handle);
void switch_register_callback_rx(switch_internal_callback_rx rx);

#ifdef __cplusplus
}
#endif

#endif  // S3_SWITCH_PACKET_H__
