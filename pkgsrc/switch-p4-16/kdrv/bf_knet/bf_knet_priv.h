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

#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/spinlock_types.h>
#include <stdbool.h>
#include <linux/version.h>
#include "bf_knet_ioctl.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)
/* integer equivalents of KERN_<LEVEL> */
#define LOGLEVEL_SCHED          -2      /* Deferred messages from sched code
                                         * are set to this special level */
#define LOGLEVEL_DEFAULT        -1      /* default (or last) loglevel */
#define LOGLEVEL_EMERG          0       /* system is unusable */
#define LOGLEVEL_ALERT          1       /* action must be taken immediately */
#define LOGLEVEL_CRIT           2       /* critical conditions */
#define LOGLEVEL_ERR            3       /* error conditions */
#define LOGLEVEL_WARNING        4       /* warning conditions */
#define LOGLEVEL_NOTICE         5       /* normal but significant condition */
#define LOGLEVEL_INFO           6       /* informational */
#define LOGLEVEL_DEBUG          7       /* debug-level messages */
#endif

#define BF_KNET_CPU_INTF_PREFIX "k_"
#define DRV_NAME "bf_knet"
#define DRV_VERSION "1.0"
#define DRV_DESCRIPTION "Intel(R) Packet Filtering Linux Driver"
#define DRV_COPYRIGHT "Copyright (c) 2015-2024 Intel Corporation."

typedef bool bf_knet_tx_status_t;

typedef struct bf_knet_rx_filter_info_s {
	/* List Node */
	struct list_head list;

	/* Number of filter matches */
	unsigned long hits;

	/*Dest when dest_type BF_FILTER_DESTINATION_HOSTIF */
	struct net_device *hostif_knetdev;

	/* Filter key and action, programmed by user */
	bf_knet_rx_filter_t rx_filter;

	/* Net # of bytes that will be added/remove
	as a result of this filter action */
	int delta_net;

	int pkt_min_len;

	struct hlist_node hash_bucket;

} bf_knet_rx_filter_info_t;

/* Private struct used to store interface information by the driver per netdev
 */
typedef struct bf_knet_priv_s {
	/* hostif_knetdev list node */
	struct list_head list;

	/* netdev id allocated to hostif by this module. */
	uint64_t id;

	/* physical interface corresponding to to this interface */
	struct net_device *cpuif_netdev;

	/* Back Pointer to own netdev */
	struct net_device *hostif_knetdev;

	/* For storing device specific stats */
	struct net_device_stats stats;

	/* If true Tx actions are applied */
	// bf_knet_tx_status_t action_active;
	bf_knet_tx_action_t *tx_action;

	/* Net # of bytes that will be added/remove
	as a result of tx action */
	int delta_net;

	uint32_t debug_level;

	int pkt_min_len;

	struct ethtool_link_ksettings link_ksettings;
} bf_knet_priv_t;

typedef struct bf_cpuif_info_s {

	/* cpuif_list node */
	struct list_head list;

	uint64_t cpuif_id;

	/* Needed for protocol handler registration */
	struct packet_type device_proto;

	/* Corresponding netdev interface for the device */
	struct net_device *cpuif_netdev;

	/* Corresponding knetdev interface for the device */
	struct net_device *cpuif_knetdev;

	/* list of rx packet filters for this cpuif_netdev */
	struct list_head bf_knet_rx_pf_list;

	/* List of hostif_knetdev (mapped to device
	port/interface/vlan/ln) */
	struct list_head hostif_kndev_list;

	/* Main lock for device */
	rwlock_t cpuif_lock;
} bf_cpuif_info_t;
