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


#ifndef S3_SWITCH_UTILS_H__
#define S3_SWITCH_UTILS_H__

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

#include <knet_mgr/bf_knet_if.h>
#include <tofino/bf_pal/dev_intf.h>
#include <target-utils/third-party/tommyds/tommyds/tommyhashtbl.h>
#include <target-utils/third-party/tommyds/tommyds/tommylist.h>
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>

#include "packet_utils.h"
#include "bf_switch/bf_switch_types.h"
#include "./log.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SWITCH_ASSERT(x) CHECK_RET(!(x), SWITCH_STATUS_INVALID_PARAMETER);

#define SWITCH_PACKET_MAX_BUFFER_SIZE 10000

#define SWITCH_PACKET_HEADER_OFFSET 2 * ETH_LEN

#define SWITCH_FABRIC_HEADER_ETHTYPE 0x9000

#define ETHERTYPE_BF_PKTGEN 0x9001
#define ETHERTYPE_DOT1Q 0x8100

#define SWITCH_FD_INVALID -1

#define SWITCH_CPU_ETH_INTF_DEFAULT "veth251"

#define SWITCH_CPU_ETH_INTF_DEFAULT_LEN strlen(SWITCH_CPU_ETH_INTF_DEFAULT)

#define SWITCH_PACKET_TX_HASH_TABLE_SIZE 1024

#define SWITCH_PACKET_RX_HASH_TABLE_SIZE 1024

#define SWITCH_PACKET_TX_HASH_KEY_SIZE sizeof(switch_packet_tx_hash_entry_t)

#define SWITCH_PACKET_RX_HASH_KEY_SIZE sizeof(switch_packet_rx_hash_entry_t)

#define SWITCH_PKTDRIVER_RX_FILTER_SIZE 4096

#define SWITCH_PKTDRIVER_TX_FILTER_SIZE 4096

#define SWITCH_ETH_HEADER_SIZE sizeof(switch_ethernet_header_t)

#define SWITCH_VLAN_HEADER_SIZE sizeof(switch_vlan_header_t)

#define SWITCH_IPV4_COMPUTE_MASK(_len) (0xFFFFFFFF << (32 - _len))

// Max sflow sessions on switch layer 256
// 64 reserved for exclusive session bind to port(in that case sflow_id is
// calculated as local_port) Also that session are programmed asymmetrically, so
// we should reserver 64 for each pipe.
#define SWITCH_MAX_SFLOW_ID 256
#define SWITCH_MAX_SFLOW_EXCLUSIVE_ID 64
#define SWITCH_SFLOW_SHARED_SESSIONS_OFFSET \
  (SWITCH_MAX_SFLOW_EXCLUSIVE_ID * BF_PIPE_COUNT)
#define SWITCH_MAX_SFLOW_SESSIONS                        \
  (SWITCH_MAX_SFLOW_ID - SWITCH_MAX_SFLOW_EXCLUSIVE_ID + \
   SWITCH_SFLOW_SHARED_SESSIONS_OFFSET)

typedef char switch_int8_t;
typedef uint8_t switch_uint8_t;
typedef int switch_int32_t;
typedef uint16_t switch_uint16_t;
typedef uint32_t switch_uint32_t;
typedef uint64_t switch_uint64_t;

#define switch_pipe pipe
#define switch_fcntl fcntl
#define switch_fd_read read
#define switch_fd_write write
#define switch_fd_close close
#define switch_fd_send sendto
#define switch_ntohs ntohs
#define switch_htons htons
#define switch_socket socket
#define switch_bind bind
#define switch_ioctl ioctl
#define switch_open open
#define switch_fd_set fd_set
#define switch_select select

#define SWITCH_MEMSET memset
#define SWITCH_MEMCPY memcpy
#define SWITCH_MEMMOVE memmove
#define SWITCH_MALLOC(x, c) bf_sys_malloc((x) * (c))
#define SWITCH_FREE(x)     \
  do {                     \
    if (x) bf_sys_free(x); \
  } while (0)

typedef uint16_t switch_device_t;
typedef int switch_fd_t;
typedef uint64_t switch_handle_t;
typedef uint32_t switch_size_t;

#define UNUSED(x) (void)x;
#define PACKED __attribute__((__packed__))

typedef bf_knet_hostif_t switch_knet_hostif_t;
typedef bf_knet_cpuif_t switch_knet_cpuif_t;
typedef bf_knet_hostif_knetdev_t switch_knet_hostif_knetdev_t;
typedef bf_knet_rx_filter_t switch_knet_rx_filter_t;
typedef bf_knet_tx_action_t switch_knet_tx_action_t;
typedef bf_knet_packet_mutation_t switch_knet_packet_mutation_t;

typedef tommy_node switch_node_t;
typedef tommy_hashtable_node switch_hashnode_t;

#define SWITCH_HOSTIF_NAME_SIZE 512
#define SWITCH_HOSTIF_MAX_REASON_CODE 100

typedef switch_status_t (*switch_key_func_t)(void *args,
                                             switch_uint8_t *key,
                                             switch_uint32_t *len);

typedef switch_int32_t (*switch_hash_compare_func_t)(const void *key1,
                                                     const void *key2);

typedef switch_int32_t (*switch_list_compare_func_t)(const void *key1,
                                                     const void *key2);

typedef struct switch_array_ {
  void *array;
  switch_size_t num_entries;
} switch_array_t;

typedef struct switch_hashtable_ {
  tommy_hashtable table;
  switch_hash_compare_func_t compare_func;
  switch_key_func_t key_func;
  switch_size_t num_entries;
  switch_size_t size;
  switch_size_t hash_seed;
} switch_hashtable_t;

typedef struct switch_list_ {
  tommy_list list;
  switch_size_t num_entries;
} switch_list_t;

#define FOR_EACH_IN_ARRAY(__index, __array, __type, __entry)            \
  {                                                                     \
    Word_t *__pvalue = NULL;                                            \
    Word_t *__pvalue_next = NULL;                                       \
    Word_t __index_tmp = __index;                                       \
    JLF(__pvalue, __array.array, __index_tmp);                          \
    __index = __index_tmp;                                              \
    for (; __pvalue; __pvalue = __pvalue_next, __index = __index_tmp) { \
      JLN(__pvalue_next, __array.array, __index_tmp);                   \
      __entry = (__type *)(*__pvalue);

#define FOR_EACH_IN_ARRAY_END() \
  }                             \
  }

#define FOR_EACH_IN_LIST(__list, __node)   \
  {                                        \
    node = tommy_list_head(&__list.list);  \
    switch_node_t *__next_node = NULL;     \
    for (; __node; __node = __next_node) { \
      __next_node = node->next;

#define FOR_EACH_IN_LIST_END() \
  }                            \
  }

#define SWITCH_PACKET_HEADER_NTOH(_pkt_header)               \
  do {                                                       \
    _pkt_header.fabric_header.ether_type =                   \
        switch_ntohs(_pkt_header.fabric_header.ether_type);  \
    _pkt_header.cpu_header.reason_code =                     \
        switch_ntohs(_pkt_header.cpu_header.reason_code);    \
    _pkt_header.cpu_header.ingress_port =                    \
        switch_ntohs(_pkt_header.cpu_header.ingress_port);   \
    _pkt_header.cpu_header.port_lag_index =                  \
        switch_ntohs(_pkt_header.cpu_header.port_lag_index); \
    _pkt_header.cpu_header.ingress_bd =                      \
        switch_ntohs(_pkt_header.cpu_header.ingress_bd);     \
  } while (0);

#define SWITCH_PACKET_HEADER_HTON(_pkt_header)               \
  do {                                                       \
    _pkt_header.fabric_header.ether_type =                   \
        switch_htons(_pkt_header.fabric_header.ether_type);  \
    _pkt_header.cpu_header.reason_code =                     \
        switch_htons(_pkt_header.cpu_header.reason_code);    \
    _pkt_header.cpu_header.ingress_port =                    \
        switch_htons(_pkt_header.cpu_header.ingress_port);   \
    _pkt_header.cpu_header.port_lag_index =                  \
        switch_htons(_pkt_header.cpu_header.port_lag_index); \
    _pkt_header.cpu_header.ingress_bd =                      \
        switch_htons(_pkt_header.cpu_header.ingress_bd);     \
  } while (0);

typedef enum switch_pktdriver_genl_family_s {
  SWITCH_PKTDRIVER_GENL_FAMILY_PSAMPLE = 0,
  SWITCH_PKTDRIVER_GENL_FAMILY_PACKET = 1,
  SWITCH_PKTDRIVER_GENL_FAMILY_MAX
} switch_pktdriver_genl_family_t;

typedef enum switch_fabric_header_type_s {
  SWITCH_FABRIC_HEADER_TYPE_NONE = 0,
  SWITCH_FABRIC_HEADER_TYPE_UNICAST = 1,
  SWITCH_FABRIC_HEADER_TYPE_MULTICAST = 2,
  SWITCH_FABRIC_HEADER_TYPE_MIRROR = 3,
  SWITCH_FABRIC_HEADER_TYPE_CONTROL = 4,
  SWITCH_FABRIC_HEADER_TYPE_CPU = 5
} switch_fabric_header_type_t;

typedef enum switch_pktdriver_packet_type_s {
  SWITCH_PKTDRIVER_PACKET_TYPE_TX_CB = 1,
  SWITCH_PKTDRIVER_PACKET_TYPE_TX_NETDEV = 2,
  SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_ETH = 3,
  SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_PCIE = 4,
  SWITCH_PKTDRIVER_PACKET_TYPE_RX_CPU_KNET = 5,
} switch_pktdriver_packet_type_t;

typedef struct switch_pktdriver_rx_filter_info_s {
  /** rx filter key */
  switch_pktdriver_rx_filter_key_t rx_key;

  /** rx filter action */
  switch_pktdriver_rx_filter_action_t rx_action;

  /** rx filter priority */
  switch_pktdriver_rx_filter_priority_t priority;

  /** number of packets */
  uint64_t num_packets;

  /** rx filter flags */
  uint64_t flags;

  /** list node */
  switch_node_t node;
} switch_pktdriver_rx_filter_info_t;

typedef struct switch_pktdriver_tx_filter_info_s {
  /** tx filter key */
  switch_pktdriver_tx_filter_key_t tx_key;

  /** tx filter action */
  switch_pktdriver_tx_filter_action_t tx_action;

  /** tx filter priority */
  switch_pktdriver_tx_filter_priority_t priority;

  /** number of packets */
  uint64_t num_packets;

  /** tx filter flags */
  uint64_t flags;

  /** list node */
  switch_node_t node;
} switch_pktdriver_tx_filter_info_t;

/** cpu timestamp header */
typedef struct PACKED switch_cpu_timestamp_header_s {
  /** Arrival Time */
  uint32_t arrival_time;
} switch_cpu_timestamp_header_t;

#define SWITCH_PKTINFO_TX_DEV_PORT(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.port_lag_index

#define SWITCH_PKTINFO_RX_DEV_PORT(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.ingress_port

#define SWITCH_PKTINFO_REASON_CODE(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.reason_code

#define SWITCH_PKTINFO_BYPASS_FLAGS(_pkt_info) \
  SWITCH_PKTINFO_REASON_CODE(_pkt_info)

#define SWITCH_PKTINFO_TX_BYPASS(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.tx_bypass

#define SWITCH_PKTINFO_PACKET_TYPE(_pkt_info) \
  _pkt_info->pkt_header.fabric_header.packet_type

#define SWITCH_PKTINFO_ETHER_TYPE(_pkt_info) \
  _pkt_info->pkt_header.fabric_header.ether_type

#define SWITCH_PKTINFO_INGRESS_PORT_LAG_INDEX(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.port_lag_index

#define SWITCH_PKTINFO_INGRESS_BD(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.ingress_bd

#define SWITCH_PKTINFO_TX_EGRESS_QUEUE(_pkt_info) \
  _pkt_info->pkt_header.cpu_header.egress_queue

typedef struct switch_packet_info_s {
  /** device id */
  switch_device_t device;

  /** packet type */
  switch_pktdriver_packet_type_t pkt_type;

  /** packet header tx/rx */
  switch_packet_header_t pkt_header;

  /** hostif fd */
  switch_fd_t fd;

  /** packet */
  switch_int8_t *pkt;

  /** packet size */
  switch_int32_t pkt_size;
} switch_packet_info_t;

typedef struct switch_kern_info_s {
  switch_knet_cpuif_t knet_cpuif_id;

  /* bf_knet interface for kernel Tx and RX */
  char cpuif_knetdev_name[IFNAMSIZ];

  struct sockaddr_ll s_addr;

  switch_fd_t sock_fd;
} switch_knet_info_t;

typedef struct switch_pktdriver_genl_s {
  /** genetlink family id */
  int family_id;

  /** genetlink mcgrp id */
  int mcgrp_id;

  /** genetlink socket */
  struct nl_sock *nlsock;

  /** netlink family referencies */
  switch_int32_t ref_cnt;
} switch_pktdriver_genl_t;

typedef void (*pkt_free)(uint16_t device, char *pkt);

/** packet driver context */
typedef struct switch_pktdriver_context_s {
  /** Is kernel packet processing enabled */
  bool knet_pkt_driver;

  /* Stores socket information when device uses bf_knet */
  switch_knet_info_t switch_kern_info;

  /** cpu interface name */
  switch_int8_t intf_name[SWITCH_HOSTIF_NAME_SIZE];

  /** use pcie */
  bool use_pcie;

  /** is kernel packet processing enabled */
  bool use_kpkt;

  /** cpu ifindex */
  uint32_t cpu_ifindex;

  /** cpu fd */
  switch_fd_t cpu_fd;

  /** dummy pipe fd */
  switch_fd_t pipe_fd[2];

  switch_array_t fd_array;

  /** list of tx filters */
  switch_list_t tx_filter;

  /** list of rx filters */
  switch_list_t rx_filter;

  /** total rx packets */
  uint64_t num_rx_packets;

  /** total tx packets */
  uint64_t num_tx_packets;

  /** total tx netdev packets */
  uint64_t num_tx_netdev_packets;

  /** total rx netdev packets */
  uint64_t num_rx_netdev_packets;

  /** total tx callback packets */
  uint64_t num_tx_cb_packets;

  /** total rx callback packets */
  uint64_t num_rx_cb_packets;

  /** rx pkt counters by reason code */
  switch_counter_t rx_rc_pkts[SWITCH_HOSTIF_MAX_REASON_CODE];

  /** rx byte counters by reason code */
  switch_counter_t rx_rc_bytes[SWITCH_HOSTIF_MAX_REASON_CODE];

  /** rx pkt counters by ingress port */
  switch_counter_t rx_port_pkts[SWITCH_MAX_PORTS];

  /** rx byte counters by ingress port */
  switch_counter_t rx_port_bytes[SWITCH_MAX_PORTS];

  /** rx packet trace enable */
  bool rx_pkt_trace_enable;

  /** tx packet trace enable */
  bool tx_pkt_trace_enable;

  /** vlan to bd mapping table */
  uint16_t bd_mapping[SWITCH_MAX_VLANS];

  /** dev_port to fd map */
  switch_fd_t dev_port_to_fd_map[SWITCH_MAX_PORTS];

  /** dev_port to linux ifindex map for sflow */
  int dev_port_to_ifindex_map[SWITCH_MAX_PORTS];

  /** dev_port to smi port handle for callback reporting */
  uint64_t dev_port_to_port_handle_map[SWITCH_MAX_PORTS];

  /** sflow session id to rate */
  uint32_t sflow_id_to_rate[SWITCH_MAX_SFLOW_SESSIONS];

  /** genetlink families */
  switch_pktdriver_genl_t genl[SWITCH_PKTDRIVER_GENL_FAMILY_MAX];

  /** sflow group sequence */
  uint32_t group_seq;

  /** rx_callback_function **/
  switch_internal_callback_rx rx_cb_internal;

  /* packet free cb */
  pkt_free pkt_free_cb;

} switch_pktdriver_context_t;

static inline switch_status_t switch_bf_status_to_switch_status(
    bf_status_t pd_status) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;

  switch (pd_status) {
    case BF_SUCCESS:
      status = SWITCH_STATUS_SUCCESS;
      break;

    case BF_NO_SYS_RESOURCES:
      status = SWITCH_STATUS_INSUFFICIENT_RESOURCES;
      break;

    case BF_ALREADY_EXISTS:
      status = SWITCH_STATUS_ITEM_ALREADY_EXISTS;
      break;

    case BF_IN_USE:
      status = SWITCH_STATUS_RESOURCE_IN_USE;
      break;

    case BF_HW_COMM_FAIL:
      status = SWITCH_STATUS_HW_FAILURE;
      break;

    case BF_OBJECT_NOT_FOUND:
      status = SWITCH_STATUS_ITEM_NOT_FOUND;
      break;

    case BF_NOT_IMPLEMENTED:
      status = SWITCH_STATUS_NOT_IMPLEMENTED;
      break;

    case BF_INVALID_ARG:
      status = SWITCH_STATUS_INVALID_PARAMETER;
      break;

    case BF_NO_SPACE:
      status = SWITCH_STATUS_TABLE_FULL;
      break;

    default:
      status = SWITCH_STATUS_PD_FAILURE;
      break;
  }

  return status;
}

switch_status_t SWITCH_ARRAY_INIT(switch_array_t *array);

switch_uint32_t SWITCH_ARRAY_COUNT(switch_array_t *array);

switch_status_t SWITCH_ARRAY_INSERT(switch_array_t *array,
                                    switch_uint64_t index,
                                    void *ptr);

switch_status_t SWITCH_ARRAY_GET(switch_array_t *array,
                                 switch_uint64_t index,
                                 void **ptr);

switch_status_t SWITCH_ARRAY_DELETE(switch_array_t *array,
                                    switch_uint64_t index);

switch_status_t SWITCH_LIST_INIT(switch_list_t *list);

switch_size_t SWITCH_LIST_COUNT(switch_list_t *list);

bool SWITCH_LIST_EMPTY(switch_list_t *list);

switch_status_t SWITCH_LIST_SORT(switch_list_t *list,
                                 switch_list_compare_func_t compare_func);

switch_status_t SWITCH_LIST_INSERT(switch_list_t *list,
                                   switch_node_t *node,
                                   void *ptr);

switch_status_t SWITCH_LIST_DELETE(switch_list_t *list, switch_node_t *node);

switch_status_t SWITCH_HASHTABLE_INIT(switch_hashtable_t *hashtable);

switch_size_t SWITCH_HASHTABLE_COUNT(switch_hashtable_t *hashtable);

switch_status_t SWITCH_HASHTABLE_INSERT(switch_hashtable_t *hashtable,
                                        switch_hashnode_t *node,
                                        void *key,
                                        void *data);

switch_status_t SWITCH_HASHTABLE_DELETE(switch_hashtable_t *hashtable,
                                        void *key,
                                        void **data);

switch_status_t SWITCH_HASHTABLE_DELETE_NODE(switch_hashtable_t *hashtable,
                                             switch_hashnode_t *node);

switch_status_t SWITCH_HASHTABLE_SEARCH(switch_hashtable_t *hashtable,
                                        void *key,
                                        void **data);

switch_status_t SWITCH_HASHTABLE_FOREACH_ARG(switch_hashtable_t *hashtable,
                                             void *func,
                                             void *arg);

switch_status_t SWITCH_HASHTABLE_DONE(switch_hashtable_t *hashtable);

switch_status_t switch_pktdriver_pkt_tx(switch_packet_info_t *pkt_info);
void switch_register_callback_pkt_free(pkt_free p_free_cb);

#ifdef __cplusplus
}
#endif

#endif  // S3_SWITCH_UTILS_H__
