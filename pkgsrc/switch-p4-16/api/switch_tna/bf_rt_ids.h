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


#ifndef __BF_RT_IDS_H__
#define __BF_RT_IDS_H__

#include "bf_switch/bf_switch_types.h"
#include "s3/bf_rt_backend.h"

/**
 * How to add entries here?
 * The entries are organized by filenames in switch.p4
 * In each file area, the entries for each table and actions are
 * grouped together
 * Same grouping is maintained in the cpp file also
 */

/* Notation:
 * T  -> table
 * F  -> match field in a table
 * A  -> action id for a match action direct or action profile table
 * P  -> action parameter of an action
 * AP -> action profile
 * SG -> selector group
 * D  -> data field in match action indirect table
 */

namespace smi {
namespace bf_rt {

class smi_id {
 public:
  static const BfRtInfo *rt_info;
  static void init_bf_rt_ids();
  /* Grouping is based on p4 src file */
  static bf_rt_action_id_t A_NO_ACTION;

  // pd fixed
  static bf_rt_table_id_t T_PRE_MGID;
  static bf_rt_field_id_t F_PRE_MGID_MGID;
  static bf_rt_field_id_t D_PRE_MGID_MULTICAST_NODE_ID;
  static bf_rt_field_id_t D_PRE_MGID_MULTICAST_NODE_L1_XID;
  static bf_rt_field_id_t D_PRE_MGID_MULTICAST_NODE_L1_XID_VALID;

  static bf_rt_table_id_t T_PRE_NODE;
  static bf_rt_field_id_t F_PRE_NODE_MULTICAST_NODE_ID;
  static bf_rt_field_id_t D_PRE_NODE_MULTICAST_RID;
  static bf_rt_field_id_t D_PRE_NODE_MULTICAST_LAG_ID;
  static bf_rt_field_id_t D_PRE_NODE_DEV_PORT;

  static bf_rt_table_id_t T_PRE_LAG;
  static bf_rt_field_id_t F_PRE_LAG_LAG_ID;
  static bf_rt_field_id_t D_PRE_LAG_DEV_PORT;

  static bf_rt_table_id_t T_PRE_PRUNE;
  static bf_rt_field_id_t F_PRE_PRUNE_YID;
  static bf_rt_field_id_t D_PRE_PRUNE_DEV_PORT;

  // bfrt
  static bf_rt_table_id_t T_TM_CFG;
  static bf_rt_field_id_t D_TM_CFG_CELL_SIZE;
  static bf_rt_field_id_t D_TM_CFG_TOTAL_CELLS;

  static bf_rt_table_id_t T_TM_PPG_CFG;
  static bf_rt_field_id_t D_TM_PPG_CFG_GUARANTEED_CELLS;

  static bf_rt_table_id_t T_TM_POOL_CFG;
  static bf_rt_field_id_t F_TM_POOL_CFG_POOL;
  static bf_rt_field_id_t D_TM_POOL_CFG_SIZE_CELLS;
  // tm.counter.ig.port
  static bf_rt_table_id_t T_TM_COUNTER_IG_PORT;
  static bf_rt_field_id_t F_TM_COUNTER_IG_PORT_DEV_PORT;
  static bf_rt_field_id_t D_TM_COUNTER_IG_PORT_DROP_COUNT;
  static bf_rt_field_id_t D_TM_COUNTER_IG_PORT_USAGE_CELLS;
  static bf_rt_field_id_t D_TM_COUNTER_IG_PORT_WATERMARK_CELLS;

  // tm.counter.eg.port
  static bf_rt_table_id_t T_TM_COUNTER_EG_PORT;
  static bf_rt_field_id_t F_TM_COUNTER_EG_PORT_DEV_PORT;
  static bf_rt_field_id_t D_TM_COUNTER_EG_PORT_DROP_COUNT;
  static bf_rt_field_id_t D_TM_COUNTER_EG_PORT_USAGE_CELLS;
  static bf_rt_field_id_t D_TM_COUNTER_EG_PORT_WATERMARK_CELLS;

  static bf_rt_table_id_t T_TM_POOL_APP_PFC;
  static bf_rt_field_id_t F_TM_POOL_APP_PFC_POOL;
  static bf_rt_field_id_t F_TM_POOL_APP_PFC_COS;
  static bf_rt_field_id_t D_TM_POOL_APP_PFC_LIMIT_CELLS;

  static bf_rt_table_id_t T_TM_COUNTER_POOL;
  static bf_rt_field_id_t F_TM_COUNTER_POOL;
  static bf_rt_field_id_t D_TM_COUNTER_POOL_WATERMARK_CELLS;
  static bf_rt_field_id_t D_TM_COUNTER_POOL_USAGE_CELLS;

  // TM port buffer
  static bf_rt_table_id_t T_TM_PORT_BUFFER;
  static bf_rt_field_id_t F_TM_PORT_BUFFER_DEV_PORT;
  static bf_rt_field_id_t D_TM_PORT_BUFFER_SKID_LIMIT_CELLS;

  // tm.port.sched_shaping
  static bf_rt_table_id_t T_TM_PORT_SCHED_SHAPING;
  static bf_rt_field_id_t F_TM_PORT_SCHED_SHAPING_DEV_PORT;
  static bf_rt_field_id_t D_TM_PORT_SCHED_SHAPING_UNIT;
  static bf_rt_field_id_t D_TM_PORT_SCHED_SHAPING_PROVISIONING;
  static bf_rt_field_id_t D_TM_PORT_SCHED_SHAPING_MAX_RATE;
  static bf_rt_field_id_t D_TM_PORT_SCHED_SHAPING_MAX_BURST_SIZE;

  // tm.port.flowcontrol
  static bf_rt_table_id_t T_TM_PORT_FLOWCONTROL;
  static bf_rt_field_id_t F_TM_PORT_FLOWCONTROL_DEV_PORT;
  static bf_rt_field_id_t D_TM_PORT_FLOWCONTROL_MODE_TX;
  static bf_rt_field_id_t D_TM_PORT_FLOWCONTROL_MODE_RX;
  static bf_rt_field_id_t D_TM_PORT_FLOWCONTROL_COS_TO_ICOS;

  // tm.port.sched_cfg
  static bf_rt_table_id_t T_TM_PORT_SCHED_CFG;
  static bf_rt_field_id_t F_TM_PORT_SCHED_CFG_DEV_PORT;
  static bf_rt_field_id_t D_TM_PORT_SCHED_CFG_MAX_RATE_ENABLE;

  /* pvs */
  static bf_rt_table_id_t T_ING_UDP_PORT_VXLAN;
  static bf_rt_field_id_t F_ING_UDP_PORT_VXLAN_F1;
  static bf_rt_table_id_t T_EG_UDP_PORT_VXLAN;
  static bf_rt_field_id_t F_EG_UDP_PORT_VXLAN_F1;

  // for folded pipeline only
  static bf_rt_table_id_t T_ING1_UDP_PORT_VXLAN;
  static bf_rt_field_id_t F_ING1_UDP_PORT_VXLAN_F1;
  static bf_rt_table_id_t T_EG1_UDP_PORT_VXLAN;
  static bf_rt_field_id_t F_EG1_UDP_PORT_VXLAN_F1;

  static bf_rt_table_id_t T_INGRESS_CPU_PORT;
  static bf_rt_field_id_t F_INGRESS_CPU_PORT_ETHER_TYPE;
  static bf_rt_field_id_t F_INGRESS_CPU_PORT_PORT;
  static bf_rt_table_id_t T_EGRESS_CPU_PORT;
  static bf_rt_field_id_t F_EGRESS_CPU_PORT_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_CPU_PORT_PORT;
  static bf_rt_table_id_t T_INTERNAL_PIPE_CPU_PORT;
  static bf_rt_field_id_t F_INTERNAL_PIPE_CPU_PORT_ETHER_TYPE;
  static bf_rt_field_id_t F_INTERNAL_PIPE_CPU_PORT_PORT;
  static bf_rt_table_id_t T_INGRESS_PIPE_CPU_PORT;
  static bf_rt_field_id_t F_INGRESS_PIPE_CPU_PORT_ETHER_TYPE;
  static bf_rt_field_id_t F_INGRESS_PIPE_CPU_PORT_PORT;

  static bf_rt_table_id_t T_INGRESS_NVGRE_ST_KEY;
  static bf_rt_table_id_t F_INGRESS_NVGRE_ST_KEY_VSID_FLOWID;
  static bf_rt_table_id_t T_EGRESS_NVGRE_ST_KEY;
  static bf_rt_table_id_t F_EGRESS_NVGRE_ST_KEY_VSID_FLOWID;

  /* ipv4_hash */
  static bf_rt_table_id_t T_IPV4_HASH;
  static bf_rt_field_id_t P_IPV4_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_IPV4_HASH_DST_ADDR;
  static bf_rt_field_id_t P_IPV4_HASH_IP_PROTO;
  static bf_rt_field_id_t P_IPV4_HASH_SRC_PORT;
  static bf_rt_field_id_t P_IPV4_HASH_DST_PORT;

  /* ipv6 hash */
  static bf_rt_table_id_t T_IPV6_HASH;
  static bf_rt_field_id_t P_IPV6_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_IPV6_HASH_DST_ADDR;
  static bf_rt_field_id_t P_IPV6_HASH_IP_PROTO;
  static bf_rt_field_id_t P_IPV6_HASH_SRC_PORT;
  static bf_rt_field_id_t P_IPV6_HASH_DST_PORT;
  static bf_rt_field_id_t P_IPV6_HASH_IPV6_FLOW_LABEL;

  /* non-ip hash */
  static bf_rt_table_id_t T_NON_IP_HASH;
  static bf_rt_field_id_t P_NONIP_HASH_ING_PORT;
  static bf_rt_field_id_t P_NONIP_HASH_MAC_TYPE;
  static bf_rt_field_id_t P_NONIP_HASH_SRC_MAC;
  static bf_rt_field_id_t P_NONIP_HASH_DST_MAC;

  /* lag v4 hash */
  static bf_rt_table_id_t T_LAG_V4_HASH;
  static bf_rt_field_id_t P_LAG_V4_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_LAG_V4_HASH_DST_ADDR;
  static bf_rt_field_id_t P_LAG_V4_HASH_IP_PROTO;
  static bf_rt_field_id_t P_LAG_V4_HASH_DST_PORT;
  static bf_rt_field_id_t P_LAG_V4_HASH_SRC_PORT;

  /* lag v6 hash */
  static bf_rt_table_id_t T_LAG_V6_HASH;
  static bf_rt_field_id_t P_LAG_V6_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_LAG_V6_HASH_DST_ADDR;
  static bf_rt_field_id_t P_LAG_V6_HASH_IP_PROTO;
  static bf_rt_field_id_t P_LAG_V6_HASH_DST_PORT;
  static bf_rt_field_id_t P_LAG_V6_HASH_SRC_PORT;
  static bf_rt_field_id_t P_LAG_V6_HASH_FLOW_LABEL;

  /* inner dtel v4 hash */
  static bf_rt_table_id_t T_INNER_DTEL_V4_HASH;
  static bf_rt_field_id_t P_INNER_DTEL_V4_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_INNER_DTEL_V4_HASH_DST_ADDR;
  static bf_rt_field_id_t P_INNER_DTEL_V4_HASH_IP_PROTO;
  static bf_rt_field_id_t P_INNER_DTEL_V4_HASH_DST_PORT;
  static bf_rt_field_id_t P_INNER_DTEL_V4_HASH_SRC_PORT;

  /* inner dtel v6 hash */
  static bf_rt_table_id_t T_INNER_DTEL_V6_HASH;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_SRC_ADDR;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_DST_ADDR;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_IP_PROTO;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_DST_PORT;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_SRC_PORT;
  static bf_rt_field_id_t P_INNER_DTEL_V6_HASH_FLOW_LABEL;

  /* Hash container inner fields ids */
  static bf_rt_field_id_t P_HASH_CONTAINER_START_BIT;
  static bf_rt_field_id_t P_HASH_CONTAINER_LENGTH;
  static bf_rt_field_id_t P_HASH_CONTAINER_ORDER;

  /*ipv4 dynamic hash algorithm */
  static bf_rt_table_id_t T_IPV4_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_IPV4_HASH_PREDEFINED;
  static bf_rt_table_id_t A_IPV4_HASH_USERDEFINED;
  static bf_rt_table_id_t P_IPV4_HASH_SEED;
  static bf_rt_table_id_t P_IPV4_HASH_ALG;
  static bf_rt_table_id_t P_IPV4_HASH_REV;
  static bf_rt_table_id_t P_IPV4_HASH_POLY;
  static bf_rt_table_id_t P_IPV4_HASH_INIT;
  static bf_rt_table_id_t P_IPV4_HASH_FXOR;
  static bf_rt_table_id_t P_IPV4_HASH_HBW;

  /*ipv6 dynamic hash algorithm */
  static bf_rt_table_id_t T_IPV6_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_IPV6_HASH_PREDEFINED;
  static bf_rt_table_id_t P_IPV6_HASH_SEED;
  static bf_rt_table_id_t P_IPV6_HASH_ALG;
  static bf_rt_table_id_t A_IPV6_HASH_USERDEFINED;
  static bf_rt_table_id_t P_IPV6_HASH_REV;
  static bf_rt_table_id_t P_IPV6_HASH_POLY;
  static bf_rt_table_id_t P_IPV6_HASH_INIT;
  static bf_rt_table_id_t P_IPV6_HASH_FXOR;
  static bf_rt_table_id_t P_IPV6_HASH_HBW;

  // Outer ecmp
  static bf_rt_table_id_t T_OUTER_IPV4_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_OUTER_IPV4_HASH_PREDEFINED;
  static bf_rt_table_id_t T_OUTER_IPV6_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_OUTER_IPV6_HASH_PREDEFINED;

  /*non-ip dynamic hash algorithm */
  static bf_rt_table_id_t T_NONIP_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_NONIP_HASH_PREDEFINED;
  static bf_rt_table_id_t P_NONIP_HASH_SEED;
  static bf_rt_table_id_t P_NONIP_HASH_ALG;
  static bf_rt_table_id_t A_NONIP_HASH_USERDEFINED;
  static bf_rt_table_id_t P_NONIP_HASH_REV;
  static bf_rt_table_id_t P_NONIP_HASH_POLY;
  static bf_rt_table_id_t P_NONIP_HASH_INIT;
  static bf_rt_table_id_t P_NONIP_HASH_FXOR;
  static bf_rt_table_id_t P_NONIP_HASH_HBW;

  /*LAG V4 dynamic hash algorithm */
  static bf_rt_table_id_t T_LAGV4_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_LAGV4_HASH_PREDEFINED;
  static bf_rt_table_id_t P_LAGV4_HASH_SEED;
  static bf_rt_table_id_t P_LAGV4_HASH_ALG;
  static bf_rt_table_id_t A_LAGV4_HASH_USERDEFINED;
  static bf_rt_table_id_t P_LAGV4_HASH_REV;
  static bf_rt_table_id_t P_LAGV4_HASH_POLY;
  static bf_rt_table_id_t P_LAGV4_HASH_INIT;
  static bf_rt_table_id_t P_LAGV4_HASH_FXOR;
  static bf_rt_table_id_t P_LAGV4_HASH_HBW;

  /* LAG V6 dynamic hash algorithm */
  static bf_rt_table_id_t T_LAGV6_DYN_HASH_ALGORITHM;
  static bf_rt_table_id_t A_LAGV6_HASH_PREDEFINED;
  static bf_rt_table_id_t P_LAGV6_HASH_SEED;
  static bf_rt_table_id_t P_LAGV6_HASH_ALG;
  static bf_rt_table_id_t A_LAGV6_HASH_USERDEFINED;
  static bf_rt_table_id_t P_LAGV6_HASH_REV;
  static bf_rt_table_id_t P_LAGV6_HASH_POLY;
  static bf_rt_table_id_t P_LAGV6_HASH_INIT;
  static bf_rt_table_id_t P_LAGV6_HASH_FXOR;
  static bf_rt_table_id_t P_LAGV6_HASH_HBW;

  /* rmac.p4 */
  static bf_rt_action_id_t A_RMAC_MISS;
  static bf_rt_action_id_t A_RMAC_HIT;

  static bf_rt_table_id_t T_INGRESS_PV_RMAC;
  static bf_rt_field_id_t F_INGRESS_PV_RMAC_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VALID;
  static bf_rt_field_id_t F_INGRESS_PV_RMAC_HDR_VLAN_TAG0_VID;
  static bf_rt_field_id_t F_INGRESS_PV_RMAC_HDR_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_PV_RMAC_MATCH_PRIORITY;

  static bf_rt_table_id_t T_INGRESS_VLAN_RMAC;
  static bf_rt_field_id_t F_INGRESS_VLAN_RMAC_HDR_VLAN_TAG0_VID;
  static bf_rt_field_id_t F_INGRESS_VLAN_RMAC_HDR_ETHERNET_DST_ADDR;

  /* validation.p4 */
  static bf_rt_table_id_t T_VALIDATE_ETHERNET;
  static bf_rt_field_id_t F_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR;
  static bf_rt_field_id_t F_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_VALIDATE_ETHERNET_VLAN_0_VALID;
  static bf_rt_field_id_t F_VALIDATE_ETHERNET_VLAN_1_VALID;
  static bf_rt_field_id_t F_VALIDATE_ETHERNET_PRIORITY;
  static bf_rt_action_id_t A_MALFORMED_ETH_PKT;
  static bf_rt_field_id_t P_MALFORMED_ETH_PKT_REASON;
  static bf_rt_action_id_t A_VALID_PKT_UNTAGGED;
  static bf_rt_field_id_t P_VALID_PKT_UNTAGGED_PKT_TYPE;
  static bf_rt_action_id_t A_VALID_PKT_TAGGED;
  static bf_rt_field_id_t P_VALID_PKT_TAGGED_PKT_TYPE;
  static bf_rt_action_id_t A_VALID_PKT_DOUBLE_TAGGED;
  static bf_rt_field_id_t P_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE;

  static bf_rt_table_id_t T_VALIDATE_IP;
  static bf_rt_field_id_t F_VALIDATE_IP_ARP_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_CHKSUM_ERR;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_VERSION;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_TTL;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_IHL;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_FLAGS;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_FRAG_OFFSET;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV4_SRC_ADDR;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV6_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV6_VERSION;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV6_HOP_LIMIT;
  static bf_rt_field_id_t F_VALIDATE_IP_IPV6_SRC_ADDR;
  static bf_rt_field_id_t F_VALIDATE_IP_MPLS_0_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_MPLS_0_LABEL;
  static bf_rt_field_id_t F_VALIDATE_IP_MPLS_1_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_INNER_IPV4_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_INNER_IPV6_VALID;
  static bf_rt_field_id_t F_VALIDATE_IP_PRIORITY;
  static bf_rt_action_id_t A_MALFORMED_IPV4_PKT;
  static bf_rt_field_id_t P_MALFORMED_IPV4_PKT_REASON;
  static bf_rt_action_id_t A_MALFORMED_IPV6_PKT;
  static bf_rt_field_id_t P_MALFORMED_IPV6_PKT_REASON;
  static bf_rt_action_id_t A_VALID_ARP_PKT;
  static bf_rt_action_id_t A_VALID_IPV4_PKT;
  static bf_rt_field_id_t P_VALID_IPV4_PKT_IP_FRAG;
  static bf_rt_field_id_t P_VALID_IPV4_PKT_IS_LINK_LOCAL;
  static bf_rt_action_id_t A_VALID_IPV6_PKT;
  static bf_rt_action_id_t A_VALID_MPLS_PKT;
  static bf_rt_action_id_t A_VALID_MPLS_NULL_PKT;
  static bf_rt_action_id_t A_VALID_MPLS_NULL_IPV4_PKT;
  static bf_rt_action_id_t A_VALID_MPLS_NULL_IPV6_PKT;
  static bf_rt_action_id_t A_VALID_MPLS_ROUTER_ALERT_LABEL;

  static bf_rt_table_id_t T_INNER_VALIDATE_ETHERNET;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_ETHERNET_VALID;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_PRIORITY;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV4_VALID;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV4_CHKSUM_ERR;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV4_VERSION;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV4_TTL;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV4_IHL;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV6_VALID;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV6_VERSION;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_IPV6_HOP_LIMIT;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_TCP_VALID;
  static bf_rt_field_id_t F_INNER_VALIDATE_ETHERNET_UDP_VALID;
  static bf_rt_action_id_t A_INNER_L2_MALFORMED_PKT;
  static bf_rt_field_id_t P_INNER_L2_MALFORMED_PKT_REASON;
  static bf_rt_action_id_t A_INNER_L3_MALFORMED_PKT;
  static bf_rt_field_id_t P_INNER_L3_MALFORMED_PKT_REASON;
  static bf_rt_action_id_t A_INNER_VALID_ETHERNET_PKT;
  static bf_rt_field_id_t P_INNER_VALID_ETHERNET_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV4_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV4_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV6_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV6_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV4_TCP_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV4_TCP_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV4_UDP_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV4_UDP_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV6_TCP_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV6_TCP_PKT_TYPE;
  static bf_rt_action_id_t A_INNER_VALID_IPV6_UDP_PKT;
  static bf_rt_field_id_t P_INNER_VALID_IPV6_UDP_PKT_TYPE;

  static bf_rt_table_id_t T_INGRESS_SAME_MAC_CHECK;
  static bf_rt_field_id_t F_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC;
  static bf_rt_action_id_t A_COMPUTE_SAME_MAC_CHECK;

  /* validation_fp.p4 */
  static bf_rt_table_id_t T_FP_VALIDATE_ETHERNET;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_ETHERNET_SRC_ADDR;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_VLAN_0_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_VLAN_1_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_PRIORITY;
  static bf_rt_action_id_t A_FP_MALFORMED_ETH_PKT;
  static bf_rt_field_id_t P_FP_MALFORMED_ETH_PKT_REASON;
  static bf_rt_action_id_t A_FP_VALID_PKT_UNTAGGED;
  static bf_rt_field_id_t P_FP_VALID_PKT_UNTAGGED_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_PKT_TAGGED;
  static bf_rt_field_id_t P_FP_VALID_PKT_TAGGED_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_PKT_DOUBLE_TAGGED;
  static bf_rt_field_id_t P_FP_VALID_PKT_DOUBLE_TAGGED_PKT_TYPE;

  static bf_rt_table_id_t T_FP_VALIDATE_IP;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_ARP_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_CHKSUM_ERR;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_VERSION;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_TTL;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_IHL;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_FLAGS;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_FRAG_OFFSET;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV4_SRC_ADDR;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV6_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV6_VERSION;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV6_HOP_LIMIT;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_IPV6_SRC_ADDR;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_MPLS_0_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_MPLS_0_LABEL;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_MPLS_1_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_INNER_IPV4_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_INNER_IPV6_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_IP_PRIORITY;
  static bf_rt_action_id_t A_FP_MALFORMED_IPV4_PKT;
  static bf_rt_field_id_t P_FP_MALFORMED_IPV4_PKT_REASON;
  static bf_rt_action_id_t A_FP_MALFORMED_IPV6_PKT;
  static bf_rt_field_id_t P_FP_MALFORMED_IPV6_PKT_REASON;
  static bf_rt_action_id_t A_FP_VALID_ARP_PKT;
  static bf_rt_action_id_t A_FP_VALID_IPV4_PKT;
  static bf_rt_field_id_t P_FP_VALID_IPV4_PKT_IP_FRAG;
  static bf_rt_field_id_t P_FP_VALID_IPV4_PKT_IS_LINK_LOCAL;
  static bf_rt_action_id_t A_FP_VALID_IPV6_PKT;
  static bf_rt_action_id_t A_FP_VALID_MPLS_PKT;
  static bf_rt_action_id_t A_FP_VALID_MPLS_NULL_PKT;
  static bf_rt_action_id_t A_FP_VALID_MPLS_NULL_IPV4_PKT;
  static bf_rt_action_id_t A_FP_VALID_MPLS_NULL_IPV6_PKT;
  static bf_rt_action_id_t A_FP_VALID_MPLS_ROUTER_ALERT_LABEL;

  static bf_rt_table_id_t T_FP_INGRESS_SAME_MAC_CHECK;
  static bf_rt_field_id_t F_FP_INGRESS_SAME_MAC_CHECK_LOCAL_MD_SAME_MAC;
  static bf_rt_action_id_t A_FP_COMPUTE_SAME_MAC_CHECK;

  /* NOTE: IDs from section below are not in use
     as long as constant entries of validate_ethernet table in control
     PktValidation1 in p4 code are sufficient */
  static bf_rt_table_id_t T_FP_VALIDATE_ETHERNET_1;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_PRIORITY;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV6_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV6_VERSION;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV6_HOP_LIMIT;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV4_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV4_CHKSUM_ERR;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV4_VERSION;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV4_IHL;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_IPV4_TTL;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_TCP_VALID;
  static bf_rt_field_id_t F_FP_VALIDATE_ETHERNET_1_UDP_VALID;
  static bf_rt_action_id_t A_FP_VALID_ETHERNET_PACKET_1;
  static bf_rt_field_id_t P_FP_VALID_IPV4_ETHERNET_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV4_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV4_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV6_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV6_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV4_TCP_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV4_TCP_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV4_UDP_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV4_UDP_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV6_TCP_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV6_TCP_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_VALID_IPV6_UDP_PKT_1;
  static bf_rt_field_id_t P_FP_VALID_IPV6_UDP_PKT_1_PKT_TYPE;
  static bf_rt_action_id_t A_FP_MALFORMED_PKT_1;
  static bf_rt_field_id_t P_FP_MALFORMED_PKT_1_REASON;
  /*
   * Egress packet validation table.
   */
  static bf_rt_table_id_t T_EGRESS_PKT_VALIDATION;

  /* port.p4 */
  static bf_rt_table_id_t T_PORT_METADATA;
  static bf_rt_field_id_t F_PORT_METADATA_PORT;
  static bf_rt_field_id_t P_PORT_METADATA_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_PORT_METADATA_PORT_LAG_LABEL;
  static bf_rt_field_id_t P_PORT_METADATA_EXT_INGRESS_PORT;

  static bf_rt_table_id_t T_INGRESS_PORT_MIRROR;
  static bf_rt_field_id_t F_INGRESS_PORT_MIRROR_PORT;
  static bf_rt_action_id_t A_INGRESS_PORT_MIRROR_SET_MIRROR_ID;
  static bf_rt_field_id_t P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID;
  static bf_rt_field_id_t P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID;
  static bf_rt_field_id_t P_INGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC;

  static bf_rt_table_id_t T_EGRESS_PORT_MIRROR;
  static bf_rt_field_id_t F_EGRESS_PORT_MIRROR_PORT;
  static bf_rt_action_id_t A_EGRESS_PORT_MIRROR_SET_MIRROR_ID;
  static bf_rt_field_id_t P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SESSION_ID;
  static bf_rt_field_id_t P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_METER_ID;
  static bf_rt_field_id_t P_EGRESS_PORT_MIRROR_SET_MIRROR_ID_SRC;

  static bf_rt_table_id_t T_INGRESS_PORT_MAPPING;
  static bf_rt_field_id_t F_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_INGRESS_PORT_MAPPING_HDR_CPU_VALID;
  static bf_rt_field_id_t F_INGRESS_PORT_MAPPING_HDR_CPU_INGRESS_PORT;
  static bf_rt_action_id_t A_SET_PORT_PROPERTIES;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_EXCLUSION_ID;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_LEARNING_MODE;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_COLOR;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_TC;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV4;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_IPV6;
  static bf_rt_field_id_t P_SET_PORT_PROPERTIES_IN_PORTS_GROUP_LABEL_MIRROR;
  static bf_rt_action_id_t A_SET_CPU_PORT_PROPERTIES;
  static bf_rt_field_id_t P_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
  static bf_rt_field_id_t P_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
  static bf_rt_field_id_t P_SET_CPU_PORT_PROPERTIES_COLOR;
  static bf_rt_field_id_t P_SET_CPU_PORT_PROPERTIES_TC;

  static bf_rt_table_id_t T_PORT_VLAN_TO_BD_MAPPING;
  static bf_rt_field_id_t
      F_PORT_VLAN_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID;
  static bf_rt_field_id_t F_PORT_VLAN_TO_BD_MAPPING_VLAN_0_VID_VALID;
  static bf_rt_field_id_t F_PORT_VLAN_TO_BD_MAPPING_SRC_MAC_ADDRESS;
  static bf_rt_field_id_t F_PORT_VLAN_TO_BD_MAPPING_PRIORITY;
  static bf_rt_field_id_t D_PORT_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID;

  static bf_rt_table_id_t T_PORT_DOUBLE_TAG_TO_BD_MAPPING;
  static bf_rt_field_id_t
      F_PORT_DOUBLE_TAG_TO_BD_MAPPING_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VID;
  static bf_rt_field_id_t F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_0_VALID;
  static bf_rt_field_id_t F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VID;
  static bf_rt_field_id_t F_PORT_DOUBLE_TAG_TO_BD_MAPPING_VLAN_1_VALID;
  static bf_rt_field_id_t D_PORT_DOUBLE_TAG_TO_BD_MAPPING_ACTION_MEMBER_ID;

  static bf_rt_table_id_t T_VLAN_TO_BD_MAPPING;
  static bf_rt_field_id_t F_VLAN_TO_BD_MAPPING_VLAN_0_VID;
  static bf_rt_field_id_t D_VLAN_TO_BD_MAPPING_ACTION_MEMBER_ID;

  static bf_rt_table_id_t T_VLAN_MEMBERSHIP;
  static bf_rt_field_id_t F_VLAN_MEMBERSHIP_REGISTER_INDEX;
  static bf_rt_field_id_t D_VLAN_MEMBERSHIP_REGISTER_DATA;

  static bf_rt_table_id_t T_CPU_TO_BD_MAPPING;
  static bf_rt_field_id_t F_CPU_TO_BD_MAPPING_HDR_CPU_INGRESS_BD;
  static bf_rt_field_id_t D_CPU_TO_BD_MAPPING_ACTION_MEMBER_ID;

  // bd_action_profile stuff
  static bf_rt_table_id_t AP_BD_ACTION_PROFILE;
  static bf_rt_field_id_t F_BD_ACTION_PROFILE_ACTION_MEMBER_ID;
  static bf_rt_action_id_t A_PORT_VLAN_MISS;
  static bf_rt_action_id_t A_INGRESS_SET_BD_PROPERTIES;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_BD;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t
      P_INGRESS_SET_BD_PROPERTIES_VRF_UNKNOWN_L3_MULTICAST_TRAP;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_VRF;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_BD_LABEL;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_STP_GROUP;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_LEARNING_MODE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_MPLS_ENABLE;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_MRPF_GROUP;
  static bf_rt_field_id_t P_INGRESS_SET_BD_PROPERTIES_NAT_ZONE;

  static bf_rt_table_id_t T_LAG;
  static bf_rt_field_id_t F_LAG_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_LAG_PRIORITY;
  static bf_rt_field_id_t D_LAG_ACTION_MEMBER_ID;
  static bf_rt_field_id_t D_LAG_SELECTOR_GROUP_ID;

  static bf_rt_table_id_t T_EGRESS_PORT_MAPPING;
  static bf_rt_field_id_t F_EGRESS_PORT_MAPPING_PORT;
  static bf_rt_action_id_t A_PORT_NORMAL;
  static bf_rt_field_id_t P_PORT_NORMAL_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_PORT_NORMAL_PORT_LAG_LABEL;
  static bf_rt_field_id_t P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV4;
  static bf_rt_field_id_t P_PORT_NORMAL_OUT_PORTS_GROUP_LABEL_IPV6;
  static bf_rt_action_id_t A_PORT_CPU;

  static bf_rt_table_id_t T_EGRESS_EGRESS_INGRESS_PORT_MAPPING;
  static bf_rt_field_id_t
      F_EGRESS_EGRESS_INGRESS_PORT_MAPPING_LOCAL_MD_INGRESS_PORT;
  static bf_rt_action_id_t A_SET_EGRESS_INGRESS_PORT_PROPERTIES;
  static bf_rt_field_id_t D_SET_INGRESS_PORT_PROPERTIES_PORT_ISOLATION_GROUP;
  static bf_rt_field_id_t
      D_SET_INGRESS_PORT_PROPERTIES_BRIDGE_PORT_ISOLATION_GROUP;

  static bf_rt_table_id_t T_EGRESS_EGRESS_PORT_ISOLATION;
  static bf_rt_field_id_t F_EGRESS_EGRESS_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT;
  static bf_rt_field_id_t
      F_EGRESS_EGRESS_PORT_ISOLATION_LOCAL_MD_PORT_ISOLATION_GROUP;
  static bf_rt_action_id_t A_ISOLATE_PACKET_PORT;
  static bf_rt_field_id_t D_ISOLATE_PACKET_PORT_DROP;

  static bf_rt_table_id_t T_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION;
  static bf_rt_field_id_t
      F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_EG_INTR_MD_EGRESS_PORT;
  static bf_rt_field_id_t
      F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t
      F_EGRESS_EGRESS_BRIDGE_PORT_ISOLATION_LOCAL_MD_BRIDGE_PORT_ISOLATION_GROUP;
  static bf_rt_action_id_t A_ISOLATE_PACKET_BPORT;
  static bf_rt_field_id_t D_ISOLATE_PACKET_BPORT_DROP;

  static bf_rt_table_id_t T_EGRESS_PEER_LINK_TUNNEL_ISOLATION;
  static bf_rt_field_id_t
      F_EGRESS_PEER_LINK_TUNNEL_ISOLATION_LOCAL_MD_BPORT_ISOLATION_GROUP;
  static bf_rt_action_id_t A_PEER_LINK_ISOLATE;
  static bf_rt_field_id_t D_PEER_LINK_ISOLATE_DROP;

  static bf_rt_table_id_t T_SNAKE;
  static bf_rt_field_id_t F_SNAKE_INGRESS_PORT;
  static bf_rt_action_id_t A_SNAKE_SET_EGRESS_PORT;
  static bf_rt_field_id_t P_SNAKE_SET_EGRESS_PORT_EGRESS_PORT;

  // lag_selector stuff
  static bf_rt_table_id_t AP_LAG_SELECTOR;
  static bf_rt_field_id_t F_LAG_SELECTOR_ACTION_MEMBER_ID;
  static bf_rt_action_id_t A_LAG_MISS;
  static bf_rt_action_id_t A_SET_LAG_PORT;
  static bf_rt_field_id_t P_SET_LAG_PORT_PORT;

  // group table id
  static bf_rt_table_id_t SG_LAG_SELECTOR_GROUP;
  // key fields
  static bf_rt_field_id_t F_LAG_SELECTOR_GROUP_ID;
  // data fields
  static bf_rt_field_id_t P_LAG_SELECTOR_GROUP_MAX_GROUP_SIZE;
  static bf_rt_field_id_t P_LAG_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
  static bf_rt_field_id_t P_LAG_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

  static bf_rt_table_id_t T_EGRESS_CPU_PORT_REWRITE;
  static bf_rt_field_id_t F_EGRESS_CPU_PORT_REWRITE_PORT;
  static bf_rt_action_id_t A_CPU_REWRITE;

  static bf_rt_table_id_t T_INGRESS_PORT_STATE_EG_1;
  static bf_rt_field_id_t F_INGRESS_PORT_STATE_EG_1_INGRESS_PORT;
  static bf_rt_table_id_t T_INGRESS_PORT_STATE_IG_1;
  static bf_rt_field_id_t F_INGRESS_PORT_STATE_IG_1_INGRESS_PORT;

  static bf_rt_action_id_t A_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_EXCLUSION_ID;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_METER_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_IG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID;

  static bf_rt_action_id_t A_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_PORT_LAG_LABEL;
  //  static bf_rt_field_id_t
  //  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_EXCLUSION_ID; static
  //  bf_rt_field_id_t
  //  P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_METER_INDEX; static
  //  bf_rt_field_id_t
  //      P_INGRESS_PORT_STATE_EG_1_SET_PORT_PROPERTIES_SFLOW_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t
      P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_PORT_LAG_LABEL;
  // static bf_rt_field_id_t
  //     P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_EXCLUSION_ID;
  // static bf_rt_field_id_t
  //     P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_METER_INDEX;
  // static bf_rt_field_id_t
  //     P_INGRESS_PORT_STATE_EG_1_SET_CPU_PORT_PROPERTIES_SFLOW_SESSION_ID;

  //  static bf_rt_table_id_t T_INGRESS_BD_STATE_EG_1;
  //  static bf_rt_field_id_t F_INGRESS_BD_STATE_EG_1_BD;
  static bf_rt_table_id_t T_INGRESS_BD_STATE_IG_1;
  static bf_rt_field_id_t F_INGRESS_BD_STATE_IG_1_BD;

  static bf_rt_action_id_t A_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_BD_LABEL;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_MPLS_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_IG_1_SET_BD_PROPERTIES_NAT_ZONE;

  /*
  static bf_rt_action_id_t A_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_BD_LABEL;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV4_MULTICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IGMP_SNOOPING_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_UNICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_IPV6_MULTICAST_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MLD_SNOOPING_ENABLE;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_MPLS_ENABLE;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VLAN_ARP_SUPPRESS;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t
      P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t P_INGRESS_BD_STATE_EG_1_SET_BD_PROPERTIES_NAT_ZONE;
  */
  /* l2.p4 */
  static bf_rt_table_id_t T_INGRESS_STP_GROUP;
  static bf_rt_field_id_t F_INGRESS_STP_GROUP_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_STP_GROUP;
  static bf_rt_table_id_t T_EGRESS_STP_GROUP;
  static bf_rt_field_id_t F_EGRESS_STP_GROUP_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_STP_GROUP;
  static bf_rt_table_id_t T_INGRESS_STP0_CHECK;
  static bf_rt_field_id_t F_INGRESS_STP0_CHECK_REGISTER_INDEX;
  static bf_rt_field_id_t D_INGRESS_STP0_CHECK_REGISTER_DATA;
  static bf_rt_table_id_t T_INGRESS_STP1_CHECK;
  static bf_rt_field_id_t F_INGRESS_STP1_CHECK_REGISTER_INDEX;
  static bf_rt_field_id_t D_INGRESS_STP1_CHECK_REGISTER_DATA;
  static bf_rt_table_id_t T_EGRESS_STP_CHECK;
  static bf_rt_field_id_t F_EGRESS_STP_CHECK_REGISTER_INDEX;
  static bf_rt_field_id_t D_EGRESS_STP_CHECK_REGISTER_DATA;
  static bf_rt_action_id_t A_INGRESS_STP_SET_STP_STATE;
  static bf_rt_field_id_t P_INGRESS_STP_SET_STP_STATE_STP_STATE;
  static bf_rt_action_id_t A_EGRESS_STP_SET_STP_STATE;
  static bf_rt_field_id_t P_EGRESS_STP_SET_STP_STATE_STP_STATE;

  static bf_rt_table_id_t T_SMAC;
  static bf_rt_field_id_t F_SMAC_LOCAL_MD_BD;
  static bf_rt_field_id_t F_SMAC_SRC_ADDR;
  static bf_rt_action_id_t A_SMAC_HIT;
  static bf_rt_field_id_t P_SMAC_HIT_PORT_LAG_INDEX;
  static bf_rt_field_id_t D_SMAC_TTL;

  static bf_rt_table_id_t T_DMAC;
  static bf_rt_field_id_t F_DMAC_LOCAL_MD_BD;
  static bf_rt_field_id_t F_DMAC_DST_ADDR;
  static bf_rt_action_id_t A_DMAC_MISS;
  static bf_rt_action_id_t A_DMAC_HIT;
  static bf_rt_field_id_t P_DMAC_HIT_PORT_LAG_INDEX;
  static bf_rt_action_id_t A_DMAC_MULTICAST;
  static bf_rt_field_id_t P_DMAC_MULTICAST_INDEX;
  static bf_rt_action_id_t A_DMAC_REDIRECT;
  static bf_rt_field_id_t P_DMAC_REDIRECT_NEXTHOP_INDEX;

  static bf_rt_table_id_t T_INGRESS_BD_STATS;
  static bf_rt_field_id_t F_INGRESS_BD_STATS_BD;
  static bf_rt_field_id_t F_INGRESS_BD_STATS_PKT_TYPE;
  static bf_rt_action_id_t A_INGRESS_BD_STATS_COUNT;
  static bf_rt_field_id_t P_INGRESS_BD_STATS_BYTES;
  static bf_rt_field_id_t P_INGRESS_BD_STATS_PKTS;

  static bf_rt_table_id_t T_EGRESS_BD_STATS;
  static bf_rt_field_id_t F_EGRESS_BD_STATS_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_BD_STATS_LOCAL_MD_PKT_TYPE;
  static bf_rt_action_id_t A_EGRESS_BD_STATS_COUNT;
  static bf_rt_field_id_t P_EGRESS_BD_STATS_BYTES;
  static bf_rt_field_id_t P_EGRESS_BD_STATS_PKTS;

  static bf_rt_table_id_t T_EGRESS_BD_MAPPING;
  static bf_rt_field_id_t F_EGRESS_BD_MAPPING_LOCAL_MD_BD;
  static bf_rt_action_id_t A_EGRESS_SET_BD_PROPERTIES;
  static bf_rt_field_id_t P_EGRESS_SET_BD_PROPERTIES_SMAC;
  static bf_rt_field_id_t P_EGRESS_SET_BD_PROPERTIES_MTU;
  static bf_rt_field_id_t P_EGRESS_SET_BD_PROPERTIES_BD_LABEL;

  static bf_rt_table_id_t T_VLAN_DECAP;
  static bf_rt_field_id_t F_VLAN_DECAP_PORT;
  static bf_rt_field_id_t F_VLAN_DECAP_VLAN_0_VALID;
  static bf_rt_field_id_t F_VLAN_DECAP_PRIORITY;
  static bf_rt_action_id_t A_REMOVE_VLAN_TAG;
  static bf_rt_action_id_t A_REMOVE_DOUBLE_TAG;
  static bf_rt_field_id_t F_VLAN_DECAP_VLAN_1_VALID;

  static bf_rt_table_id_t T_PORT_BD_TO_VLAN_MAPPING;
  static bf_rt_field_id_t F_PORT_BD_TO_VLAN_MAPPING_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_PORT_BD_TO_VLAN_MAPPING_BD;
  static bf_rt_action_id_t A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED;
  static bf_rt_action_id_t A_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED;
  static bf_rt_field_id_t P_PORT_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID;
  static bf_rt_action_id_t A_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED;
  static bf_rt_field_id_t P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID0;
  static bf_rt_field_id_t P_PORT_BD_TO_VLAN_MAPPING_SET_DOUBLE_TAGGED_VID1;

  static bf_rt_table_id_t T_BD_TO_VLAN_MAPPING;
  static bf_rt_field_id_t F_BD_TO_VLAN_MAPPING_BD;
  static bf_rt_action_id_t A_BD_TO_VLAN_MAPPING_SET_VLAN_UNTAGGED;
  static bf_rt_action_id_t A_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED;
  static bf_rt_field_id_t P_BD_TO_VLAN_MAPPING_SET_VLAN_TAGGED_VID;

  /* l3.p4 */
  static bf_rt_action_id_t A_FIB_HIT;
  static bf_rt_field_id_t P_FIB_HIT_NEXTHOP_INDEX;
  static bf_rt_field_id_t P_FIB_HIT_FIB_LABEL;
  static bf_rt_action_id_t A_FIB_DROP;
  static bf_rt_action_id_t A_FIB_MYIP;
  static bf_rt_field_id_t P_FIB_MYIP_MYIP;

  static bf_rt_table_id_t T_IPV4_FIB_HOST;
  static bf_rt_field_id_t F_IPV4_FIB_VRF;
  static bf_rt_field_id_t F_IPV4_FIB_VRF_SIZE;
  static bf_rt_field_id_t F_IPV4_FIB_DST_ADDR;
  static bf_rt_table_id_t T_IPV4_FIB_LOCAL_HOST;
  static bf_rt_field_id_t F_IPV4_FIB_LOCAL_HOST_VRF;
  static bf_rt_field_id_t F_IPV4_FIB_LOCAL_HOST_DST_ADDR;

  static bf_rt_table_id_t T_IPV4_FIB_LPM;
  static bf_rt_field_id_t F_IPV4_FIB_LPM_VRF;
  static bf_rt_field_id_t F_IPV4_FIB_LPM_DST_ADDR;

  static bf_rt_table_id_t T_IPV6_FIB_HOST;
  static bf_rt_field_id_t F_IPV6_FIB_VRF;
  static bf_rt_field_id_t F_IPV6_FIB_DST_ADDR;

  static bf_rt_table_id_t T_IPV6_FIB_HOST64;
  static bf_rt_field_id_t F_IPV6_FIB_HOST64_VRF;
  static bf_rt_field_id_t F_IPV6_FIB_HOST64_DST_ADDR;

  static bf_rt_table_id_t T_IPV6_FIB_LPM;
  static bf_rt_field_id_t F_IPV6_FIB_LPM_VRF;
  static bf_rt_field_id_t F_IPV6_FIB_LPM_DST_ADDR;

  static bf_rt_table_id_t T_IPV6_FIB_LPM64;
  static bf_rt_field_id_t F_IPV6_FIB_LPM64_VRF;
  static bf_rt_field_id_t F_IPV6_FIB_LPM64_DST_ADDR;

  static bf_rt_table_id_t T_IPV6_FIB_LPM_TCAM;
  static bf_rt_field_id_t F_IPV6_FIB_LPM_TCAM_VRF;
  static bf_rt_field_id_t F_IPV6_FIB_LPM_TCAM_DST_ADDR;

  static bf_rt_table_id_t T_IP_FIB_LPM64;
  static bf_rt_field_id_t F_IP_FIB_LPM64_VRF;
  static bf_rt_field_id_t F_IP_FIB_LPM64_DST_ADDR;

  /* nexthop.p4 */
  static bf_rt_table_id_t T_NEXTHOP;
  static bf_rt_field_id_t F_NEXTHOP_LOCAL_MD_NEXTHOP;
  static size_t F_NEXTHOP_INDEX_WIDTH;
  static bf_rt_action_id_t A_SET_NEXTHOP_PROPERTIES;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_BD;
  static bf_rt_action_id_t A_SET_NEXTHOP_PROPERTIES_PR_FLOOD;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_BD;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_MGID;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_PR_FLOOD_NAT_ZONE;
  static bf_rt_action_id_t A_SET_NEXTHOP_PROPERTIES_GLEAN;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_GLEAN_TRAP_ID;
  static bf_rt_action_id_t A_SET_NEXTHOP_PROPERTIES_DROP;
  static bf_rt_action_id_t P_SET_NEXTHOP_PROPERTIES_DROP_DROP_REASON;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_NAT_ZONE;
  static bf_rt_action_id_t A_SET_NEXTHOP_PROPERTIES_TUNNEL;
  static bf_rt_field_id_t P_SET_NEXTHOP_PROPERTIES_TUNNEL_DIP_INDEX;

  // outer ecmp
  // ecmp table
  static bf_rt_table_id_t T_OUTER_FIB;
  static bf_rt_field_id_t F_OUTER_FIB_LOCAL_MD_TUNNEL_DIP_INDEX;
  static bf_rt_field_id_t D_OUTER_FIB_ACTION_MEMBER_ID;
  static bf_rt_field_id_t D_OUTER_FIB_SELECTOR_GROUP_ID;
  static bf_rt_action_id_t A_SET_OUTER_NEXTHOP_PROPERTIES;
  static bf_rt_field_id_t P_SET_OUTER_NEXTHOP_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_SET_OUTER_NEXTHOP_PROPERTIES_NEXTHOP_INDEX;

  // ecmp_selector stuff
  static bf_rt_table_id_t AP_OUTER_ECMP_SELECTOR;
  static bf_rt_field_id_t F_OUTER_ECMP_SELECTOR_ACTION_MEMBER_ID;

  // group table id
  static bf_rt_table_id_t SG_OUTER_ECMP_SELECTOR_GROUP;
  // key fields
  static bf_rt_field_id_t F_OUTER_ECMP_SELECTOR_GROUP_ID;
  // data fields
  static bf_rt_field_id_t P_OUTER_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE;
  static bf_rt_field_id_t P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
  static bf_rt_field_id_t P_OUTER_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

  // inner ecmp
  // ecmp table
  static bf_rt_table_id_t T_ECMP;
  static bf_rt_field_id_t F_ECMP_LOCAL_MD_NEXTHOP;
  static bf_rt_field_id_t D_ECMP_ACTION_MEMBER_ID;
  static bf_rt_field_id_t D_ECMP_SELECTOR_GROUP_ID;

  // ecmp_selector stuff
  static bf_rt_table_id_t AP_ECMP_SELECTOR;
  static bf_rt_field_id_t F_ECMP_SELECTOR_ACTION_MEMBER_ID;
  static bf_rt_action_id_t A_SET_ECMP_PROPERTIES;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_BD;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_NEXTHOP_INDEX;
  static bf_rt_action_id_t A_SET_ECMP_PROPERTIES_DROP;
  static bf_rt_action_id_t A_SET_ECMP_PROPERTIES_TUNNEL;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_TUNNEL_DIP_INDEX;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_TUNNEL_NEXTHOP_INDEX;
  static bf_rt_action_id_t A_SET_ECMP_PROPERTIES_GLEAN;
  static bf_rt_field_id_t P_SET_ECMP_PROPERTIES_GLEAN_TRAP_ID;

  // group table id
  static bf_rt_table_id_t SG_ECMP_SELECTOR_GROUP;
  // key fields
  static bf_rt_field_id_t F_ECMP_SELECTOR_GROUP_ID;
  // data fields
  static bf_rt_field_id_t P_ECMP_SELECTOR_GROUP_MAX_GROUP_SIZE;
  static bf_rt_field_id_t P_ECMP_SELECTOR_GROUP_MAX_MEMBER_ARRAY;
  static bf_rt_field_id_t P_ECMP_SELECTOR_GROUP_MAX_MEMBER_STATUS_ARRAY;

  static bf_rt_table_id_t T_NEIGHBOR;
  static bf_rt_field_id_t F_NEIGHBOR_LOCAL_MD_NEXTHOP;
  static bf_rt_action_id_t A_NEIGHBOR_REWRITE_L2;
  static bf_rt_field_id_t P_NEIGHBOR_REWRITE_L2_DMAC;

  static bf_rt_table_id_t T_OUTER_NEXTHOP;
  static bf_rt_field_id_t F_OUTER_NEXTHOP_LOCAL_MD_NEXTHOP;
  static bf_rt_action_id_t A_OUTER_NEXTHOP_REWRITE_L2;
  static bf_rt_field_id_t P_OUTER_NEXTHOP_REWRITE_L2_BD;

  /* acl.p4 */

  static bf_rt_table_id_t T_INGRESS_MAC_ACL;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_HDR_MAC_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_HDR_MAC_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_LKP_PCP;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_LKP_DEI;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_field_id_t F_INGRESS_MAC_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_NO_ACTION;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_PERMIT;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_PERMIT_USER_METADATA;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_PERMIT_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_PERMIT_TRAP_ID;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_REDIRECT_NEXTHOP_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_REDIRECT_PORT;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_REDIRECT_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_REDIRECT_PORT_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_TRAP;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_TRAP_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_TRAP_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_COPY;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_COPY_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_COPY_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_MAC_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_MAC_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_TRANSIT;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_DENY;
  static bf_rt_action_id_t A_INGRESS_MAC_ACL_NO_NAT;
  static bf_rt_field_id_t P_INGRESS_MAC_ACL_NO_NAT_DISABLE_NAT;

  static bf_rt_table_id_t T_PRE_INGRESS_ACL;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_PRE_INGRESS_ACL_MATCH_PRIORITY;
  static bf_rt_action_id_t A_PRE_INGRESS_ACL_SET_VRF;
  static bf_rt_field_id_t D_PRE_INGRESS_ACL_SET_VRF_VRF;
  static bf_rt_action_id_t A_PRE_INGRESS_ACL_NO_ACTION;
  static bf_rt_action_id_t A_PRE_INGRESS_ACL_DROP;
  static bf_rt_field_id_t D_PRE_INGRESS_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_PRE_INGRESS_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t T_PRE_INGRESS_DEIVCE_TO_ACL;
  static bf_rt_field_id_t A_PRE_INGRESS_SET_ACL_STATUS;
  static bf_rt_field_id_t D_PRE_INGRESS_SET_ACL_STATUS_ENABLED;

  static bf_rt_table_id_t T_INGRESS_ACL_ETYPE1;
  static bf_rt_field_id_t F_INGRESS_ACL_ETYPE1_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_action_id_t A_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL;
  static bf_rt_field_id_t D_INGRESS_ACL_ETYPE1_SET_ETYPE_LABEL;
  static bf_rt_table_id_t T_INGRESS_ACL_ETYPE2;
  static bf_rt_field_id_t F_INGRESS_ACL_ETYPE2_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_action_id_t A_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL;
  static bf_rt_field_id_t D_INGRESS_ACL_ETYPE2_SET_ETYPE_LABEL;

  static bf_rt_field_id_t T_INGRESS_ACL_QOS_MACADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_ACL_QOS_MACADDR_SMAC_ADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_QOS_MACADDR_DMAC_ADDR;
  static bf_rt_action_id_t A_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t D_INGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t F_INGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY;

  static bf_rt_field_id_t T_INGRESS_ACL_PBR_MACADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_PBR_MACADDR_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_ACL_PBR_MACADDR_SMAC_ADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_PBR_MACADDR_DMAC_ADDR;
  static bf_rt_action_id_t A_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t D_INGRESS_ACL_PBR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t F_INGRESS_ACL_PBR_MACADDR_MATCH_PRIORITY;

  static bf_rt_field_id_t T_INGRESS_ACL_MIRROR_MACADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR;
  static bf_rt_field_id_t F_INGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR;
  static bf_rt_action_id_t A_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t D_INGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t F_INGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY;

  static bf_rt_table_id_t T_INGRESS_IP_ACL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_ETYPE_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_PBR_MAC_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_TTL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_IP_FRAG;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_FIB_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_MAC_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_PCP;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_LKP_DEI;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_NO_ACTION;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_PERMIT;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_PERMIT_USER_METADATA;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_PERMIT_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_PERMIT_TRAP_ID;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_REDIRECT_NEXTHOP_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_REDIRECT_PORT;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_REDIRECT_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_REDIRECT_PORT_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_TRAP;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_TRAP_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_TRAP_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_COPY;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_COPY_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_COPY_METER_INDEX;
  static bf_rt_field_id_t D_INGRESS_IP_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IP_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_INGRESS_IPV4_ACL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TTL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_FRAG;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_FIB_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_IPV4_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_NO_ACTION;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_PERMIT;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_PERMIT_USER_METADATA;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_PERMIT_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_PERMIT_TRAP_ID;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_REDIRECT_NEXTHOP_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_REDIRECT_PORT;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_REDIRECT_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_REDIRECT_PORT_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_TRAP;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_TRAP_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_TRAP_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_COPY;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_COPY_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_COPY_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_IPV4_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IPV4_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_7_0;
  static bf_rt_field_id_t F_IPV4_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_TRANSIT;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_DENY;
  static bf_rt_action_id_t A_INGRESS_IPV4_ACL_NO_NAT;
  static bf_rt_field_id_t P_INGRESS_IPV4_ACL_NO_NAT_DISABLE_NAT;

  static bf_rt_table_id_t T_INGRESS_INNER_DTEL_IPV4_ACL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_PROTOCOL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_DIFFSERV;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_UDP_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_IPV4_TTL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_HDR_INNER_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_IG_MD_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV4_ACL_LOCAL_MD_TUNNEL_VNI;
  static bf_rt_action_id_t A_INGRESS_INNER_DTEL_IPV4_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_INNER_DTEL_IPV4_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_INNER_DTEL_IPV4_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_INGRESS_IPV6_ACL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TTL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_FRAG;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_FIB_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_IPV6_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_NO_ACTION;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_PERMIT;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_PERMIT_USER_METADATA;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_PERMIT_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_PERMIT_TRAP_ID;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_REDIRECT_NEXTHOP_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_REDIRECT_PORT;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_REDIRECT_PORT_LAG_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_REDIRECT_PORT_USER_METADATA;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_TRAP;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_TRAP_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_TRAP_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_COPY;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_COPY_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_COPY_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_IPV6_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IPV6_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_action_id_t F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_15_8;
  static bf_rt_field_id_t F_IPV6_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_TRANSIT;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_DENY;
  static bf_rt_action_id_t A_INGRESS_IPV6_ACL_NO_NAT;
  static bf_rt_field_id_t P_INGRESS_IPV6_ACL_NO_NAT_DISABLE_NAT;

  static bf_rt_table_id_t T_INGRESS_INNER_DTEL_IPV6_ACL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_NEXT_HDR;
  static bf_rt_field_id_t
      F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_UDP_DST_PORT;
  static bf_rt_field_id_t
      F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_IPV6_HOP_LIMIT;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_HDR_INNER_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_IG_MD_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_INNER_DTEL_IPV6_ACL_LOCAL_MD_TUNNEL_VNI;
  static bf_rt_action_id_t A_INGRESS_INNER_DTEL_IPV6_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_INNER_DTEL_IPV6_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_INNER_DTEL_IPV6_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_INGRESS_IP_MIRROR_ACL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_ETYPE_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TTL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_FRAG;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_PCP;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_DEI;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_field_id_t
      F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_IN_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_NO_ACTION;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_PERMIT;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_MIRROR_OUT;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_MIRROR_OUT_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_TRAP;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_TRAP_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_TRAP_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_COPY;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_COPY_TRAP_ID;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_COPY_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_IP_MIRROR_ACL_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_INGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS;
  static bf_rt_action_id_t
      F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL_23_16;
  static bf_rt_field_id_t
      F_INGRESS_IP_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;

  static bf_rt_table_id_t T_INGRESS_IP_MIRROR_ACL_METER_ACTION;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t F_INGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT;
  static bf_rt_action_id_t A_INGRESS_IP_MIRROR_ACL_METER_ACTION_DROP_AND_COUNT;
  static bf_rt_field_id_t
      D_INGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_IP_QOS_ACL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_ETYPE_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_PCP;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_LKP_DEI;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_TC_TC;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_TC_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_COLOR_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_COLOR_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_SET_METER;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TC;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_NO_ACTION;
  static bf_rt_field_id_t P_INGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_INGRESS_IP_QOS_ACL_METER_ACTION;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t F_INGRESS_IP_QOS_ACL_METER_ACTION_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_METER_ACTION_COUNT;
  static bf_rt_action_id_t A_INGRESS_IP_QOS_ACL_METER_ACTION_DROP_AND_COUNT;
  static bf_rt_field_id_t D_INGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_IP_DTEL_ACL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_TTL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_IP_FRAG;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_MATCH_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_NO_ACTION;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_DROP;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_PERMIT;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_REDIRECT_NEXTHOP_INDEX;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_SET_TC;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_SET_TC_TC;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_SET_COLOR;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_SET_COLOR_COLOR;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_SET_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_IFA_CLONE_SAMPLE;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID;
  static bf_rt_action_id_t A_INGRESS_IP_DTEL_ACL_IFA_CLONE_AND_SET_REPORT_TYPE;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_IFA_CLONE_SESSION_ID_WITH_TYPE;
  static bf_rt_field_id_t P_INGRESS_IP_DTEL_ACL_DTEL_REPORT_TYPE_WITH_CLONE;
  static bf_rt_field_id_t D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_IP_DTEL_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_INGRESS_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_L4_SRC_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_SET_SRC_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_SET_SRC_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_L4_DST_PORT_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_L4_DST_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_SET_DST_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_SET_DST_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_QOS_ACL_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_QOS_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_QOS_ACL_L4_SRC_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_QOS_ACL_SET_SRC_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_QOS_ACL_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_QOS_ACL_L4_DST_PORT_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_QOS_ACL_L4_DST_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_QOS_ACL_SET_DST_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_QOS_ACL_SET_DST_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_IP_ACL_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_L4_SRC_PORT_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_L4_SRC_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_SET_SRC_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_SET_SRC_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_IP_ACL_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_L4_DST_PORT_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_INGRESS_IP_ACL_L4_DST_PORT_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_IP_ACL_SET_DST_PORT_LABEL;
  static bf_rt_field_id_t P_INGRESS_IP_ACL_SET_DST_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_INGRESS_TOS_MIRROR_ACL;
  static bf_rt_field_id_t F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t
      F_INGRESS_TOS_MIRROR_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_INGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_action_id_t A_INGRESS_TOS_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_INGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_INGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_field_id_t D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_action_id_t A_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT;
  static bf_rt_field_id_t P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_SESSION_ID;
  static bf_rt_field_id_t P_INGRESS_TOS_MIRROR_ACL_MIRROR_OUT_METER_INDEX;

  static bf_rt_table_id_t T_INGRESS_DROP_STATS;
  static bf_rt_field_id_t F_INGRESS_DROP_STATS_PORT;
  static bf_rt_field_id_t F_INGRESS_DROP_STATS_DROP_REASON;
  static bf_rt_action_id_t A_INGRESS_DROP_STATS_COUNT;
  static bf_rt_field_id_t D_INGRESS_DROP_STATS_PKTS;

  static bf_rt_table_id_t T_INGRESS_SYSTEM_ACL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_INGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_PKT_TYPE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_MAC_DST_ADDR;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_IP_TYPE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_IP_TTL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_IP_PROTO;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_SYSTEM_ACL_LKP_ARP_OPCODE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_VLAN_MISS;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_RACL_DENY;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_COPY_CANCEL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_RMAC_HIT;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_DMAC_MISS;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_MYIP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_GLEAN;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_ARP_SUPPRESS;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_LPM_MISS;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_FIB_DROP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_SAMPLE_PACKET;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_STORM_CONTROL_COLOR;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_LINK_LOCAL;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_BD;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_CHECKS_SAME_IF;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_CHECKS_MRPF;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_STP_STATE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_UNICAST_ENABLE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_ENABLE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV4_MD_MULTICAST_SNOOPING;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_UNICAST_ENABLE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_ENABLE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_IPV6_MD_MULTICAST_SNOOPING;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_MPLS_ENABLE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_DROP_REASON;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_L2_DROP_REASON;
  static bf_rt_field_id_t F_SYSTEM_ACL_PRIORITY;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_MPLS_TRAP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_HOSTIF_TRAP_ID;
  static bf_rt_field_id_t F_SYSTEM_ACL_HDR_MPLS_0_TTL;
  static bf_rt_field_id_t F_SYSTEM_ACL_HDR_MPLS_0_VALID;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_MPLS_ROUTER_ALERT;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_SRV6_TRAP;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_BFD_TO_CPU;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_VRF_UNKNOWN_L3_MULTICAST_TRAP;
  static bf_rt_field_id_t F_SYSTEM_ACL_HDR_IP_OPTIONS_VALID;
  static bf_rt_action_id_t A_SYSTEM_ACL_PERMIT;
  static bf_rt_action_id_t A_SYSTEM_ACL_DROP;
  static bf_rt_field_id_t P_SYSTEM_ACL_DROP_DROP_REASON;
  static bf_rt_field_id_t P_SYSTEM_ACL_DROP_DISABLE_LEARNING;
  static bf_rt_action_id_t A_SYSTEM_ACL_DENY;
  static bf_rt_field_id_t P_SYSTEM_ACL_DENY_DROP_REASON;
  static bf_rt_field_id_t P_SYSTEM_ACL_DENY_DISABLE_LEARNING;
  static bf_rt_action_id_t A_SYSTEM_ACL_COPY_TO_CPU_CANCEL;
  static bf_rt_action_id_t A_SYSTEM_ACL_COPY_TO_CPU;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_TO_CPU_QID;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_TO_CPU_METER_ID;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_TO_CPU_DISABLE_LEARNING;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_TO_CPU_OVERWRITE_QID;
  static bf_rt_action_id_t A_SYSTEM_ACL_REDIRECT_TO_CPU;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_TO_CPU_QID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_TO_CPU_DISABLE_LEARNING;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_TO_CPU_OVERWRITE_QID;
  static bf_rt_action_id_t A_SYSTEM_ACL_COPY_SFLOW_TO_CPU;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_QID;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_METER_ID;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_DISABLE_LEARNING;
  static bf_rt_field_id_t P_SYSTEM_ACL_COPY_SFLOW_TO_CPU_OVERWRITE_QID;
  static bf_rt_action_id_t A_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_QID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_METER_ID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_DISABLE_LEARNING;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_SFLOW_TO_CPU_OVERWRITE_QID;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_NAT_HIT;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_NAT_SAME_ZONE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_TUNNEL_TERMINATE;
  static bf_rt_field_id_t F_SYSTEM_ACL_LOCAL_MD_MULTICAST_HIT;
  static bf_rt_action_id_t A_SYSTEM_ACL_REDIRECT_BFD_TO_CPU;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_QID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_METER_ID;
  static bf_rt_field_id_t P_SYSTEM_ACL_REDIRECT_BFD_TO_CPU_DISABLE_LEARNING;

  static bf_rt_table_id_t T_COPP;
  static bf_rt_field_id_t F_COPP_PACKET_COLOR;
  static bf_rt_field_id_t F_COPP_COPP_METER_ID;
  static bf_rt_field_id_t F_COPP_PRIORITY;
  static bf_rt_action_id_t A_COPP_DROP;
  static bf_rt_action_id_t A_COPP_PERMIT;
  static bf_rt_field_id_t D_COPP_STATS_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_COPP_METER;
  static bf_rt_field_id_t F_COPP_METER_METER_INDEX;
  static bf_rt_field_id_t D_COPP_METER_METER_SPEC_CIR_PPS;
  static bf_rt_field_id_t D_COPP_METER_METER_SPEC_PIR_PPS;
  static bf_rt_field_id_t D_COPP_METER_METER_SPEC_CBS_PKTS;
  static bf_rt_field_id_t D_COPP_METER_METER_SPEC_PBS_PKTS;

  static bf_rt_table_id_t T_EGRESS_COPP;
  static bf_rt_field_id_t F_EGRESS_COPP_PACKET_COLOR;
  static bf_rt_field_id_t F_EGRESS_COPP_COPP_METER_ID;
  static bf_rt_field_id_t F_EGRESS_COPP_PRIORITY;
  static bf_rt_action_id_t A_EGRESS_COPP_DROP;
  static bf_rt_action_id_t A_EGRESS_COPP_PERMIT;
  static bf_rt_field_id_t D_EGRESS_COPP_STATS_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_EGRESS_COPP_METER;
  static bf_rt_field_id_t F_EGRESS_COPP_METER_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_COPP_METER_METER_SPEC_CIR_PPS;
  static bf_rt_field_id_t D_EGRESS_COPP_METER_METER_SPEC_PIR_PPS;
  static bf_rt_field_id_t D_EGRESS_COPP_METER_METER_SPEC_CBS_PKTS;
  static bf_rt_field_id_t D_EGRESS_COPP_METER_METER_SPEC_PBS_PKTS;

  static bf_rt_table_id_t T_EGRESS_MAC_ACL;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_USER_METADATA;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_PCP;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_DEI;
  static bf_rt_field_id_t F_EGRESS_MAC_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_action_id_t A_EGRESS_MAC_ACL_NO_ACTION;
  static bf_rt_action_id_t A_EGRESS_MAC_ACL_DROP;
  static bf_rt_action_id_t A_EGRESS_MAC_ACL_PERMIT;
  static bf_rt_field_id_t P_EGRESS_MAC_ACL_PERMIT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_MAC_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_MAC_ACL_MIRROR_SESSION_ID;
  static bf_rt_field_id_t P_EGRESS_MAC_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_MAC_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_MAC_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_EGRESS_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_L4_SRC_PORT_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_L4_SRC_PORT_PRIORITY;
  static bf_rt_action_id_t A_EGRESS_SET_SRC_PORT_LABEL;
  static bf_rt_field_id_t P_EGRESS_SET_SRC_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_EGRESS_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_L4_DST_PORT_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_L4_DST_PORT_PRIORITY;
  static bf_rt_action_id_t A_EGRESS_SET_DST_PORT_LABEL;
  static bf_rt_field_id_t P_EGRESS_SET_DST_PORT_LABEL_LABEL;

  static bf_rt_table_id_t T_EGRESS_IPV4_ACL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_PROTOCOL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_HDR_IPV4_DIFFSERV;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_USER_METADATA;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_EGRESS_IPV4_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_action_id_t A_EGRESS_IPV4_ACL_NO_ACTION;
  static bf_rt_action_id_t A_EGRESS_IPV4_ACL_DROP;
  static bf_rt_action_id_t A_EGRESS_IPV4_ACL_PERMIT;
  static bf_rt_field_id_t P_EGRESS_IPV4_ACL_PERMIT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IPV4_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_IPV4_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV4_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_EGRESS_IPV4_ACL_MIRROR_IN;
  static bf_rt_field_id_t P_EGRESS_IPV4_ACL_MIRROR_IN_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV4_ACL_MIRROR_IN_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_IPV4_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IPV4_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_EGRESS_IPV6_ACL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_NEXT_HDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_USER_METADATA;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_SRC_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_HDR_IPV6_DST_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_PROTO;
  static bf_rt_field_id_t F_EGRESS_IPV6_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_action_id_t A_EGRESS_IPV6_ACL_NO_ACTION;
  static bf_rt_action_id_t A_EGRESS_IPV6_ACL_DROP;
  static bf_rt_action_id_t A_EGRESS_IPV6_ACL_PERMIT;
  static bf_rt_field_id_t P_EGRESS_IPV6_ACL_PERMIT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IPV6_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_IPV6_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV6_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_EGRESS_IPV6_ACL_MIRROR_IN;
  static bf_rt_field_id_t P_EGRESS_IPV6_ACL_MIRROR_IN_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV6_ACL_MIRROR_IN_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_IPV6_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IPV6_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_EGRESS_TOS_MIRROR_ACL;
  static bf_rt_field_id_t
      F_EGRESS_TOS_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_TOS_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_VALID;
  static bf_rt_field_id_t F_EGRESS_TOS_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_VALID;
  static bf_rt_field_id_t F_EGRESS_TOS_MIRROR_ACL_HDR_IPV4_DIFFSERV;
  static bf_rt_action_id_t A_EGRESS_TOS_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_TOS_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_TOS_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_TOS_MIRROR_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_action_id_t A_EGRESS_TOS_MIRROR_ACL_MIRROR_IN;
  static bf_rt_field_id_t P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_TOS_MIRROR_ACL_MIRROR_IN_SESSION_ID;

  static bf_rt_table_id_t T_EGRESS_DROP_STATS;
  static bf_rt_field_id_t F_EGRESS_DROP_STATS_PORT;
  static bf_rt_field_id_t F_EGRESS_DROP_STATS_DROP_REASON;
  static bf_rt_action_id_t A_EGRESS_DROP_STATS_COUNT;
  static bf_rt_field_id_t D_EGRESS_DROP_STATS_PKTS;

  static bf_rt_table_id_t T_EGRESS_SYSTEM_ACL;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_EG_INTR_MD_EGRESS_PORT;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_ACL_DENY;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_WRED_DROP;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PFC_WD_DROP;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_MTU;
  static bf_rt_field_id_t
      F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_METER_PACKET_ACTION;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_METER_DROP;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_QOS_ACL_METER_COLOR;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_MIRROR_ACL_METER_COLOR;
  static bf_rt_field_id_t
      F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_PORT_ISOLATION_PACKET_DROP;
  static bf_rt_field_id_t
      F_EGRESS_SYSTEM_ACL_LOCAL_MD_FLAGS_BPORT_ISOLATION_PACKET_DROP;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_SYSTEM_ACL_LOCAL_MD_CHECKS_STP;
  static bf_rt_action_id_t A_EGRESS_SYSTEM_ACL_DROP;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_DROP_REASON_CODE;
  static bf_rt_action_id_t A_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_MIRROR_METER_DROP_REASON_CODE;
  static bf_rt_action_id_t A_EGRESS_SYSTEM_ACL_COPY_TO_CPU;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_COPY_TO_CPU_METER_ID;
  static bf_rt_action_id_t A_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_REASON_CODE;
  static bf_rt_field_id_t P_EGRESS_SYSTEM_ACL_REDIRECT_TO_CPU_METER_ID;
  static bf_rt_field_id_t D_EGRESS_SYSTEM_ACL_DROP_STATS_PKTS;

  static bf_rt_table_id_t T_EGRESS_IPV4_MIRROR_ACL;
  static bf_rt_field_id_t
      F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_PROTOCOL;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_HDR_IPV4_DIFFSERV;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t
      F_EGRESS_IPV4_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV4_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_action_id_t A_EGRESS_IPV4_MIRROR_ACL_NO_ACTION;
  static bf_rt_action_id_t A_EGRESS_IPV4_MIRROR_ACL_DROP;
  static bf_rt_action_id_t A_EGRESS_IPV4_MIRROR_ACL_PERMIT;
  static bf_rt_field_id_t P_EGRESS_IPV4_MIRROR_ACL_PERMIT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IPV4_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_IPV4_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV4_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN;
  static bf_rt_field_id_t P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV4_MIRROR_ACL_MIRROR_IN_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IPV4_MIRROR_ACL_COUNTER_SPEC_PKTS;

  static bf_rt_table_id_t T_EGRESS_IPV6_MIRROR_ACL;
  static bf_rt_field_id_t
      F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_NEXT_HDR;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_HDR_IPV6_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_ETHER_TYPE;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_SRC_PORT_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_L4_DST_PORT_LABEL;
  static bf_rt_field_id_t
      F_EGRESS_IPV6_MIRROR_ACL_LOCAL_MD_OUT_PORTS_GROUP_LABEL;
  static bf_rt_field_id_t F_EGRESS_IPV6_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_action_id_t A_EGRESS_IPV6_MIRROR_ACL_NO_ACTION;
  static bf_rt_action_id_t A_EGRESS_IPV6_MIRROR_ACL_DROP;
  static bf_rt_action_id_t A_EGRESS_IPV6_MIRROR_ACL_PERMIT;
  static bf_rt_field_id_t P_EGRESS_IPV6_MIRROR_ACL_PERMIT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IPV6_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_IPV6_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV6_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_action_id_t A_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN;
  static bf_rt_field_id_t P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IPV6_MIRROR_ACL_MIRROR_IN_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IPV6_MIRROR_ACL_COUNTER_SPEC_PKTS;

  //  start acl2.p4 egress
  static bf_rt_table_id_t T_EGRESS_IP_MIRROR_ACL;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_MIRROR_MAC_LABEL;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_SRC_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_IPV6_DST_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_PROTOCOL;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_PCP;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_DEI;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_action_id_t A_EGRESS_IP_MIRROR_ACL_NO_ACTION;
  static bf_rt_field_id_t P_EGRESS_IP_MIRROR_ACL_NO_ACTION_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_MIRROR_ACL_MIRROR;
  static bf_rt_field_id_t P_EGRESS_IP_MIRROR_ACL_MIRROR_METER_INDEX;
  static bf_rt_field_id_t P_EGRESS_IP_MIRROR_ACL_MIRROR_SESSION_ID;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_EGRESS_IP_MIRROR_ACL_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_EGRESS_IP_MIRROR_ACL_METER_ACTION;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t F_EGRESS_IP_MIRROR_ACL_METER_ACTION_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNT;
  static bf_rt_field_id_t
      D_EGRESS_IP_MIRROR_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_EGRESS_IP_QOS_ACL;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_BD;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_QOS_MAC_LABEL;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_IP_SRC_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_IPV6_SRC_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_IP_DST_ADDR_WORD3;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR_WORD2;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_IPV6_DST_ADDR_WORD10;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_PROTOCOL;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_IP_TOS;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_EGRESS_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_MATCH_PRIORITY;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_LOCAL_MD_FLAGS_ROUTED;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_PCP;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_DEI;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_HDR_VLAN_TAG0_VALID;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_NO_ACTION;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_NO_ACTION_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_PCP;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_PCP;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_IPV4_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_IPV4_TOS_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_IPV6_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_IPV6_TOS_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_PCP;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV4_TOS_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_PCP;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_PCP_IPV6_TOS_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_METER;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_PCP;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_TOS;
  static bf_rt_field_id_t P_EGRESS_IP_QOS_ACL_SET_QOS_PARAMS_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_EGRESS_IP_QOS_ACL_METER_ACTION;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t F_EGRESS_IP_QOS_ACL_METER_ACTION_INDEX;
  static bf_rt_action_id_t A_EGRESS_IP_QOS_ACL_METER_ACTION_COUNT;
  static bf_rt_field_id_t D_EGRESS_IP_QOS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_field_id_t T_EGRESS_ACL_QOS_MACADDR;
  static bf_rt_field_id_t F_EGRESS_ACL_QOS_MACADDR_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_ACL_QOS_MACADDR_SMAC_ADDR;
  static bf_rt_field_id_t F_EGRESS_ACL_QOS_MACADDR_DMAC_ADDR;
  static bf_rt_action_id_t A_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t D_EGRESS_ACL_QOS_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t F_EGRESS_ACL_QOS_MACADDR_MATCH_PRIORITY;

  static bf_rt_field_id_t T_EGRESS_ACL_MIRROR_MACADDR;
  static bf_rt_field_id_t F_EGRESS_ACL_MIRROR_MACADDR_PORT_LAG_INDEX;
  static bf_rt_field_id_t F_EGRESS_ACL_MIRROR_MACADDR_SMAC_ADDR;
  static bf_rt_field_id_t F_EGRESS_ACL_MIRROR_MACADDR_DMAC_ADDR;
  static bf_rt_action_id_t A_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t D_EGRESS_ACL_MIRROR_MACADDR_SET_MACADDR_LABEL;
  static bf_rt_field_id_t F_EGRESS_ACL_MIRROR_MACADDR_MATCH_PRIORITY;
  //  acl2.p4 egress complete

  /* multicast.p4 */
  static bf_rt_table_id_t T_IPV4_MULTICAST_BRIDGE_S_G;
  static bf_rt_field_id_t F_IPV4_MULTICAST_BRIDGE_S_G_BD;
  static bf_rt_field_id_t F_IPV4_MULTICAST_BRIDGE_S_G_SRC_ADDR;
  static bf_rt_field_id_t F_IPV4_MULTICAST_BRIDGE_S_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV4_MULTICAST_BRIDGE_S_G_HIT;
  static bf_rt_field_id_t P_IPV4_MULTICAST_BRIDGE_S_G_HIT_MGID;

  static bf_rt_table_id_t T_IPV4_MULTICAST_BRIDGE_X_G;
  static bf_rt_field_id_t F_IPV4_MULTICAST_BRIDGE_X_G_BD;
  static bf_rt_field_id_t F_IPV4_MULTICAST_BRIDGE_X_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV4_MULTICAST_BRIDGE_X_G_HIT;
  static bf_rt_field_id_t P_IPV4_MULTICAST_BRIDGE_X_G_HIT_MGID;

  static bf_rt_table_id_t T_IPV6_MULTICAST_BRIDGE_S_G;
  static bf_rt_field_id_t F_IPV6_MULTICAST_BRIDGE_S_G_BD;
  static bf_rt_field_id_t F_IPV6_MULTICAST_BRIDGE_S_G_SRC_ADDR;
  static bf_rt_field_id_t F_IPV6_MULTICAST_BRIDGE_S_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV6_MULTICAST_BRIDGE_S_G_HIT;
  static bf_rt_field_id_t P_IPV6_MULTICAST_BRIDGE_S_G_HIT_MGID;

  static bf_rt_table_id_t T_IPV6_MULTICAST_BRIDGE_X_G;
  static bf_rt_field_id_t F_IPV6_MULTICAST_BRIDGE_X_G_BD;
  static bf_rt_field_id_t F_IPV6_MULTICAST_BRIDGE_X_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV6_MULTICAST_BRIDGE_X_G_HIT;
  static bf_rt_field_id_t P_IPV6_MULTICAST_BRIDGE_X_G_HIT_MGID;

  static bf_rt_table_id_t T_IPV4_MULTICAST_ROUTE_S_G;
  static bf_rt_field_id_t F_IPV4_MULTICAST_ROUTE_S_G_VRF;
  static bf_rt_field_id_t F_IPV4_MULTICAST_ROUTE_S_G_SRC_ADDR;
  static bf_rt_field_id_t F_IPV4_MULTICAST_ROUTE_S_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV4_MULTICAST_ROUTE_S_G_HIT;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_S_G_HIT_MGID;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_S_G_HIT_PKTS;

  static bf_rt_table_id_t T_IPV4_MULTICAST_ROUTE_X_G;
  static bf_rt_field_id_t F_IPV4_MULTICAST_ROUTE_X_G_VRF;
  static bf_rt_field_id_t F_IPV4_MULTICAST_ROUTE_X_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP;
  static bf_rt_action_id_t A_IPV4_MULTICAST_ROUTE_X_G_HIT_SM;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_MGID;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP;
  static bf_rt_field_id_t P_IPV4_MULTICAST_ROUTE_X_G_HIT_SM_PKTS;

  static bf_rt_table_id_t T_IPV6_MULTICAST_ROUTE_S_G;
  static bf_rt_field_id_t F_IPV6_MULTICAST_ROUTE_S_G_VRF;
  static bf_rt_field_id_t F_IPV6_MULTICAST_ROUTE_S_G_SRC_ADDR;
  static bf_rt_field_id_t F_IPV6_MULTICAST_ROUTE_S_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV6_MULTICAST_ROUTE_S_G_HIT;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_S_G_HIT_MGID;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_S_G_HIT_RPF_GROUP;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_S_G_HIT_PKTS;

  static bf_rt_table_id_t T_IPV6_MULTICAST_ROUTE_X_G;
  static bf_rt_field_id_t F_IPV6_MULTICAST_ROUTE_X_G_VRF;
  static bf_rt_field_id_t F_IPV6_MULTICAST_ROUTE_X_G_GRP_ADDR;
  static bf_rt_action_id_t A_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_MGID;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_RPF_GROUP;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_BIDIR_PKTS;
  static bf_rt_action_id_t A_IPV6_MULTICAST_ROUTE_X_G_HIT_SM;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_MGID;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_RPF_GROUP;
  static bf_rt_field_id_t P_IPV6_MULTICAST_ROUTE_X_G_HIT_SM_PKTS;

  static bf_rt_table_id_t T_MCAST_FWD_RESULT;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_MULTICAST_HIT;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_LKP_IP_TYPE;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_IPV4_MCAST_SNOOPING;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_IPV6_MCAST_SNOOPING;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_LOCAL_MD_MULTICAST_MODE;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_RPF_CHECK;
  static bf_rt_field_id_t F_MCAST_FWD_RESULT_PRIORITY;
  static bf_rt_action_id_t A_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE;
  static bf_rt_field_id_t P_MCAST_FWD_RESULT_SET_MULTICAST_BRIDGE_MRPF;
  static bf_rt_action_id_t A_MCAST_FWD_RESULT_SET_MULTICAST_ROUTE;
  static bf_rt_action_id_t A_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD;
  static bf_rt_field_id_t P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_MRPF;
  static bf_rt_field_id_t P_MCAST_FWD_RESULT_SET_MULTICAST_FLOOD_FLOOD;

  static bf_rt_table_id_t T_BD_FLOOD;
  static bf_rt_field_id_t F_BD_FLOOD_BD;
  static bf_rt_field_id_t F_BD_FLOOD_PKT_TYPE;
  static bf_rt_field_id_t F_BD_FLOOD_FLOOD_TO_MROUTERS;
  static bf_rt_action_id_t A_FLOOD;
  static bf_rt_field_id_t P_FLOOD_MGID;

  static bf_rt_table_id_t T_RID;
  static bf_rt_field_id_t F_RID_REPLICATION_ID;
  static bf_rt_field_id_t A_RID_HIT;
  static bf_rt_action_id_t P_RID_HIT_BD;
  static bf_rt_action_id_t A_RID_TUNNEL_RID_HIT;
  static bf_rt_field_id_t P_RID_TUNNEL_RID_HIT_NEXTHOP;
  static bf_rt_field_id_t P_RID_TUNNEL_RID_HIT_TUNNEL_NEXTHOP;
  static bf_rt_action_id_t A_RID_TUNNEL_MC_RID_HIT;
  static bf_rt_field_id_t P_RID_TUNNEL_MC_RID_HIT_BD;
  static bf_rt_field_id_t P_RID_TUNNEL_MC_RID_HIT_NEXTHOP;
  static bf_rt_field_id_t P_RID_TUNNEL_MC_RID_HIT_TUNNEL_NEXTHOP;

  static bf_rt_table_id_t T_DECAP_ECN;
  static bf_rt_field_id_t F_MODE_ECN;

  /* rewrite.p4 */
  static bf_rt_table_id_t T_MIRROR_REWRITE;
  static bf_rt_field_id_t F_MIRROR_REWRITE_MIRROR_MD_SESSION_ID;
  static bf_rt_action_id_t A_REWRITE_DUMMY;
  static bf_rt_field_id_t P_REWRITE_DUMMY_QUEUE_ID;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE2;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_TTL;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE2_VLAN;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_ETHER_TYPE;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_PCP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_VID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE2_VLAN_TTL;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE3;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_TTL;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE3_VLAN;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_ETHER_TYPE;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_PCP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_VID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_VLAN_TTL;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE3_PLAT;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_TTL;
  static bf_rt_action_id_t A_REWRITE_ERSPAN_TYPE3_PLAT_VLAN;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_ETHER_TYPE;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_PCP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_VID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DMAC;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_SIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_DIP;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TOS;
  static bf_rt_field_id_t P_REWRITE_ERSPAN_TYPE3_PLAT_VLAN_TTL;
  static bf_rt_action_id_t A_REWRITE_RSPAN;
  static bf_rt_field_id_t P_REWRITE_RSPAN_QUEUE_ID;
  static bf_rt_field_id_t P_REWRITE_RSPAN_VID;
  static bf_rt_action_id_t A_REWRITE_DTEL_REPORT;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_SMAC;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_DMAC;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_SIP;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_DIP;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_UDP_DST_PORT;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_UDP_SRC_PORT;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_TTL;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_TOS;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_SESSION_ID;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_MAX_PKT_LEN;
  static bf_rt_action_id_t A_REWRITE_DTEL_REPORT_ENTROPY;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_SMAC;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_DMAC;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_SIP;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_DIP;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_UDP_DST_PORT;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_TTL;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_TOS;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_SESSION_ID;
  static bf_rt_field_id_t P_REWRITE_DTEL_REPORT_ENTROPY_MAX_PKT_LEN;
  static bf_rt_action_id_t A_REWRITE_IP_UDP_LENGTHS;
  static bf_rt_action_id_t A_REWRITE_DTEL_IFA_CLONE;

  /* tunnel.p4 */
  static bf_rt_table_id_t T_TUNNEL_NEXTHOP;
  static bf_rt_field_id_t F_TUNNEL_NEXTHOP_LOCAL_MD_TUNNEL_NEXTHOP;
  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TYPE;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_DIP_INDEX;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_INDEX;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L2_TUNNEL_ENCAP_TUNNEL_MAPPER_INDEX;
  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DMAC;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TYPE;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_DIP_INDEX;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_TUNNEL_INDEX;
  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DMAC;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TYPE;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_VNI;
  static bf_rt_field_id_t P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_DIP_INDEX;
  static bf_rt_field_id_t
      P_TUNNEL_NEXTHOP_L3_TUNNEL_ENCAP_WITH_VNI_TUNNEL_INDEX;
  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_SRV6_ENCAP;
  static bf_rt_field_id_t P_SRV6_ENCAP_SEG_LEN;
  static bf_rt_field_id_t P_SRV6_ENCAP_TUNNEL_INDEX;
  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_SRV6_INSERT;
  static bf_rt_field_id_t P_SRV6_INSERT_SEG_LEN;

  static bf_rt_table_id_t T_IPV4_SRC_VTEP;
  static bf_rt_field_id_t F_IPV4_SRC_VTEP_SRC_ADDR;
  static bf_rt_field_id_t F_IPV4_SRC_VTEP_LOCAL_MD_VRF;
  static bf_rt_field_id_t F_IPV4_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE;
  static bf_rt_action_id_t A_IPV4_SRC_VTEP_MISS;
  static bf_rt_action_id_t A_IPV4_SRC_VTEP_HIT;
  static bf_rt_field_id_t P_IPV4_SRC_VTEP_HIT_IFINDEX;

  static bf_rt_table_id_t T_IPV4_DST_VTEP;
  static bf_rt_field_id_t F_IPV4_DST_VTEP_DST_ADDR;
  static bf_rt_field_id_t F_IPV4_DST_VTEP_SRC_ADDR;
  static bf_rt_field_id_t F_IPV4_DST_VTEP_LOCAL_MD_VRF;
  static bf_rt_field_id_t F_IPV4_DST_VTEP_LOCAL_MD_TUNNEL_TYPE;
  static bf_rt_action_id_t A_IPV4_DST_VTEP_HIT;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_HIT_TTL_MODE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_ECN_MODE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_HIT_QOS_MODE;
  static bf_rt_action_id_t A_IPV4_DST_VTEP_SET_INNER_BD_PROPERTIES;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_BD;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_VRF;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t
      P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t
      P_IPV4_DST_VTEP_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_BD_LABEL;
  //  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_RID;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_LEARNING_MODE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_TTL_MODE;
  static bf_rt_field_id_t P_IPV4_DST_VTEP_SET_PROPERTIES_QOS_MODE;

  static bf_rt_table_id_t T_IPV6_SRC_VTEP;
  static bf_rt_field_id_t F_IPV6_SRC_VTEP_SRC_ADDR;
  static bf_rt_field_id_t F_IPV6_SRC_VTEP_LOCAL_MD_VRF;
  static bf_rt_field_id_t F_IPV6_SRC_VTEP_LOCAL_MD_TUNNEL_TYPE;
  static bf_rt_action_id_t A_IPV6_SRC_VTEP_MISS;
  static bf_rt_action_id_t A_IPV6_SRC_VTEP_HIT;
  static bf_rt_field_id_t P_IPV6_SRC_VTEP_HIT_IFINDEX;

  static bf_rt_table_id_t T_IPV6_DST_VTEP;
  static bf_rt_field_id_t F_IPV6_DST_VTEP_SRC_ADDR;
  static bf_rt_field_id_t F_IPV6_DST_VTEP_DST_ADDR;
  static bf_rt_field_id_t F_IPV6_DST_VTEP_LOCAL_MD_VRF;
  static bf_rt_field_id_t F_IPV6_DST_VTEP_LOCAL_MD_TUNNEL_TYPE;
  static bf_rt_action_id_t A_IPV6_DST_VTEP_HIT;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_HIT_TTL_MODE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_HIT_QOS_MODE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_ECN_MODE;
  static bf_rt_action_id_t A_IPV6_DST_VTEP_SET_INNER_BD_PROPERTIES;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_BD;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_VRF;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_BD_LABEL;
  //  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_RID;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_LEARNING_MODE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_IPV6_UNICAST_ENABLE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_TTL_MODE;
  static bf_rt_field_id_t P_IPV6_DST_VTEP_SET_PROPERTIES_QOS_MODE;

  static bf_rt_table_id_t T_TUNNEL_RMAC;
  static bf_rt_field_id_t F_TUNNEL_RMAC_LOCAL_MD_TUNNEL_VNI;
  static bf_rt_field_id_t F_TUNNEL_RMAC_HDR_INNER_ETHERNET_DST_ADDR;
  static bf_rt_action_id_t A_TUNNEL_RMAC_MISS;
  static bf_rt_action_id_t A_TUNNEL_RMAC_HIT;

  static bf_rt_table_id_t T_TUNNEL_VXLAN_DEVICE_RMAC;
  static bf_rt_field_id_t
      F_TUNNEL_VXLAN_DEVICE_RMAC_HDR_INNER_ETHERNET_DST_ADDR;

  static bf_rt_table_id_t T_VNI_TO_BD_MAPPING;
  static bf_rt_field_id_t F_VNI_TO_BD_MAPPING_LOCAL_MD_TUNNEL_VNI;
  static bf_rt_action_id_t A_VNI_TO_BD_MAPPING_SET_INNER_BD_PROPERTIES;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_BD;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_VRF;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_BD_LABEL;
  //  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_RID;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_LEARNING_MODE;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_IPV4_UNICAST_ENABLE;
  static bf_rt_field_id_t P_VNI_SET_PROPERTIES_IPV6_UNICAST_ENABLE;

  static bf_rt_table_id_t T_TUNNEL_SRC_ADDR_REWRITE;
  static bf_rt_field_id_t F_TUNNEL_SRC_ADDR_REWRITE_TUNNEL_INDEX;
  static bf_rt_action_id_t A_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_SRC_ADDR;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_TTL_VAL;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV4_SIP_REWRITE_DSCP_VAL;
  static bf_rt_action_id_t A_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_SRC_ADDR;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_TTL_VAL;
  static bf_rt_field_id_t P_TUNNEL_SRC_ADDR_REWRITE_IPV6_SIP_REWRITE_DSCP_VAL;

  static bf_rt_table_id_t T_TUNNEL_DST_ADDR_REWRITE;
  static bf_rt_field_id_t F_TUNNEL_DST_ADDR_REWRITE_LOCAL_MD_TUNNEL_DIP_INDEX;
  static bf_rt_action_id_t A_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE;
  static bf_rt_field_id_t P_TUNNEL_DST_ADDR_REWRITE_IPV4_DIP_REWRITE_DST_ADDR;
  static bf_rt_action_id_t A_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE;
  static bf_rt_field_id_t P_TUNNEL_DST_ADDR_REWRITE_IPV6_DIP_REWRITE_DST_ADDR;

  static bf_rt_table_id_t T_TUNNEL_REWRITE_ENCAP_TTL;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_TTL_LOCAL_MD_TUNNEL_INDEX;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_TTL_IPV4_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_TTL_IPV6_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV4_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_TTL_INNER_IPV6_VALID;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V4_PIPE_TTL_VAL;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_PIPE_TTL_VAL;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_PIPE_TTL_VAL;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V6_PIPE_TTL_VAL;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V4_IN_V6_UNIFORM;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_TTL_V6_IN_V4_UNIFORM;

  static bf_rt_table_id_t T_TUNNEL_REWRITE_ENCAP_DSCP;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_DSCP_LOCAL_MD_TUNNEL_INDEX;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_DSCP_IPV4_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_DSCP_IPV6_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV4_VALID;
  static bf_rt_field_id_t F_TUNNEL_REWRITE_ENCAP_DSCP_INNER_IPV6_VALID;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_ECN;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_ECN;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_ECN;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_ECN;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V4_UNIFORM;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V4_IN_V6_UNIFORM;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V4_UNIFORM;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V6_IN_V6_UNIFORM;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE;
  static bf_rt_action_id_t A_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_DSCP_V4_PIPE_DSCP_VAL;
  static bf_rt_field_id_t P_TUNNEL_REWRITE_ENCAP_DSCP_V6_PIPE_DSCP_VAL;

  static bf_rt_table_id_t T_EGRESS_VRF_MAPPING;
  static bf_rt_field_id_t F_EGRESS_VRF_MAPPING_LOCAL_MD_VRF;
  static bf_rt_action_id_t A_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES;
  static bf_rt_field_id_t P_EGRESS_VRF_MAPPING_SET_VRF_PROPERTIES_SMAC;

  static bf_rt_table_id_t T_BD_TO_VNI_MAPPING;
  static bf_rt_field_id_t F_BD_TO_VNI_MAPPING_LOCAL_MD_BD;
  static bf_rt_field_id_t F_BD_TO_VNI_MAPPING_LOCAL_MD_TUNNEL_MAPPER_INDEX;
  static bf_rt_action_id_t A_BD_TO_VNI_MAPPING_SET_VNI;
  static bf_rt_field_id_t P_BD_SET_PROPERTIES_VNI;

  static bf_rt_table_id_t T_VRF_TO_VNI_MAPPING;
  static bf_rt_field_id_t F_VRF_TO_VNI_MAPPING_LOCAL_MD_VRF;
  static bf_rt_action_id_t A_VRF_TO_VNI_MAPPING_SET_VNI;
  static bf_rt_field_id_t P_VRF_SET_PROPERTIES_VNI;

  static bf_rt_table_id_t T_TUNNEL_TABLE;
  static bf_rt_field_id_t F_TUNNEL_TABLE_LOCAL_MD_TUNNEL_TYPE;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV4_VXLAN;
  static bf_rt_field_id_t F_TUNNEL_TABLE_ENCAP_IPV4_VXLAN_PORT;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV6_VXLAN;
  static bf_rt_field_id_t F_TUNNEL_TABLE_ENCAP_IPV6_VXLAN_PORT;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV4_IP;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV6_IP;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV4_GRE;
  static bf_rt_action_id_t A_TUNNEL_TABLE_ENCAP_IPV6_GRE;

  /* qos.p4 */
  static bf_rt_table_id_t T_DSCP_TC_MAP;
  static bf_rt_field_id_t F_DSCP_TC_MAP_QOS_INGRESS_PORT;
  static bf_rt_field_id_t F_DSCP_TC_MAP_LKP_IP_DSCP;
  static bf_rt_action_id_t A_DSCP_TC_MAP_SET_INGRESS_TC;
  static bf_rt_field_id_t F_DSCP_TC_MAP_SET_INGRESS_TC_TC;
  static bf_rt_action_id_t A_DSCP_TC_MAP_SET_INGRESS_COLOR;
  static bf_rt_field_id_t F_DSCP_TC_MAP_SET_INGRESS_COLOR_COLOR;
  static bf_rt_action_id_t A_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR;
  static bf_rt_field_id_t F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC;
  static bf_rt_field_id_t F_DSCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR;
  //  static bf_rt_action_id_t A_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER;
  //  static bf_rt_field_id_t F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC;
  //  static bf_rt_field_id_t
  //  F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR;
  //  static bf_rt_field_id_t
  //  F_DSCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER;

  static bf_rt_table_id_t T_PCP_TC_MAP;
  static bf_rt_field_id_t F_PCP_TC_MAP_QOS_INGRESS_PORT;
  static bf_rt_field_id_t F_PCP_TC_MAP_LKP_PCP;
  static bf_rt_action_id_t A_PCP_TC_MAP_SET_INGRESS_TC;
  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_TC;
  static bf_rt_action_id_t A_PCP_TC_MAP_SET_INGRESS_COLOR;
  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_COLOR_COLOR;
  static bf_rt_action_id_t A_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR;
  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_TC;
  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_AND_COLOR_COLOR;
  //  static bf_rt_action_id_t A_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER;
  //  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_TC;
  //  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_COLOR;
  //  static bf_rt_field_id_t F_PCP_TC_MAP_SET_INGRESS_TC_COLOR_AND_METER_METER;

  static bf_rt_table_id_t T_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_QOS_MD_COLOR;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_QOS_MD_TC;
  static bf_rt_action_id_t A_TRAFFIC_CLASS_SET_ICOS;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_SET_ICOS_ICOS;
  static bf_rt_action_id_t A_TRAFFIC_CLASS_SET_QID;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_SET_QID_QID;
  static bf_rt_action_id_t A_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_ICOS;
  static bf_rt_field_id_t F_TRAFFIC_CLASS_SET_ICOS_AND_QUEUE_QID;

  static bf_rt_table_id_t T_PPG;
  static bf_rt_field_id_t F_PPG_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_PPG_IG_INTR_MD_FOR_TM_INGRESS_COS;
  static bf_rt_action_id_t A_PPG_COUNT;
  static bf_rt_field_id_t P_PPG_STATS_BYTES;
  static bf_rt_field_id_t P_PPG_STATS_PKTS;

  static bf_rt_table_id_t T_QUEUE;
  static bf_rt_field_id_t F_QUEUE_EG_INTR_MD_PORT;
  static bf_rt_field_id_t F_QUEUE_LOCAL_MD_QOS_QID;
  static bf_rt_action_id_t A_QUEUE_COUNT;
  static bf_rt_field_id_t P_QUEUE_STATS_BYTES;
  static bf_rt_field_id_t P_QUEUE_STATS_PKTS;

  static bf_rt_table_id_t T_L3_QOS_MAP;
  static bf_rt_field_id_t F_L3_QOS_MAP_PORT;
  static bf_rt_field_id_t F_L3_QOS_MAP_MD_TC;
  static bf_rt_field_id_t F_L3_QOS_MAP_MD_COLOR;
  static bf_rt_field_id_t F_L3_QOS_MAP_HDR_IPV4_VALID;
  static bf_rt_field_id_t F_L3_QOS_MAP_HDR_IPV6_VALID;
  static bf_rt_action_id_t A_L3_QOS_MAP_SET_IPV4_DSCP;
  static bf_rt_field_id_t F_L3_QOS_MAP_SET_IPV4_DSCP_DSCP;
  static bf_rt_action_id_t A_L3_QOS_MAP_SET_IPV6_DSCP;
  static bf_rt_field_id_t F_L3_QOS_MAP_SET_IPV6_DSCP_DSCP;

  static bf_rt_table_id_t T_L2_QOS_MAP;
  static bf_rt_field_id_t F_L2_QOS_MAP_PORT;
  static bf_rt_field_id_t F_L2_QOS_MAP_MD_TC;
  static bf_rt_field_id_t F_L2_QOS_MAP_MD_COLOR;
  static bf_rt_action_id_t A_L2_QOS_MAP_SET_VLAN_PCP;
  static bf_rt_field_id_t F_L2_QOS_MAP_SET_VLAN_PCP_PCP;

  static bf_rt_table_id_t T_INGRESS_PFC_WD_ACL;
  static bf_rt_field_id_t F_INGRESS_PFC_WD_ACL_QID;
  static bf_rt_field_id_t F_INGRESS_PFC_WD_ACL_PORT;
  static bf_rt_field_id_t A_INGRESS_PFC_WD_ACL_DENY;
  static bf_rt_field_id_t P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t P_INGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_EGRESS_PFC_WD_ACL;
  static bf_rt_field_id_t F_EGRESS_PFC_WD_ACL_QID;
  static bf_rt_field_id_t F_EGRESS_PFC_WD_ACL_PORT;
  static bf_rt_field_id_t A_EGRESS_PFC_WD_ACL_DENY;
  static bf_rt_field_id_t P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t P_EGRESS_PFC_WD_ACL_STATS_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_ETRAP_IPV4_ACL;
  static bf_rt_field_id_t F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_ETRAP_IPV4_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_action_id_t A_ETRAP_IPV4_ACL_SET_METER_AND_TC;
  static bf_rt_field_id_t P_ETRAP_IPV4_ACL_SET_METER_AND_TC_INDEX;
  static bf_rt_field_id_t P_ETRAP_IPV4_ACL_SET_METER_AND_TC_TC;

  static bf_rt_table_id_t T_ETRAP_IPV6_ACL;
  static bf_rt_field_id_t F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_ETRAP_IPV6_ACL_LOCAL_MD_LKP_IP_DST_ADDR;
  static bf_rt_action_id_t A_ETRAP_IPV6_ACL_SET_METER_AND_TC;
  static bf_rt_field_id_t P_ETRAP_IPV6_ACL_SET_METER_AND_TC_INDEX;
  static bf_rt_field_id_t P_ETRAP_IPV6_ACL_SET_METER_AND_TC_TC;

  static bf_rt_table_id_t T_ETRAP_METER;
  static bf_rt_table_id_t F_ETRAP_METER_METER_INDEX;
  static bf_rt_table_id_t D_ETRAP_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_table_id_t D_ETRAP_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_table_id_t D_ETRAP_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_table_id_t D_ETRAP_METER_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_ETRAP_METER_INDEX;
  static bf_rt_table_id_t F_ETRAP_METER_INDEX_LOCAL_MD_QOS_ETRAP_INDEX;
  static bf_rt_table_id_t A_ETRAP_METER_INDEX_ACTION;
  static bf_rt_table_id_t P_ETRAP_METER_INDEX_ACTION_INDEX;

  static bf_rt_table_id_t T_ETRAP_METER_STATE;
  static bf_rt_table_id_t F_ETRAP_METER_STATE_LOCAL_MD_QOS_ETRAP_INDEX;
  static bf_rt_table_id_t A_ETRAP_METER_STATE_ACTION;

  static bf_rt_table_id_t T_ETRAP_STATE;
  static bf_rt_field_id_t F_ETRAP_STATE_LOCAL_MD_QOS_ETRAP_COLOR;
  static bf_rt_action_id_t A_ETRAP_STATE_ETRAP_RED_STATE;
  static bf_rt_action_id_t A_ETRAP_STATE_ETRAP_GREEN_STATE;

  static bf_rt_table_id_t T_ETRAP_STATE_REG;
  static bf_rt_field_id_t F_ETRAP_STATE_REG_REGISTER_INDEX;
  static bf_rt_field_id_t D_ETRAP_STATE_REG_RESULT_VALUE;

  /* meter.p4 */
  static bf_rt_table_id_t T_STORM_CONTROL_STATS;
  static bf_rt_field_id_t F_STORM_CONTROL_STATS_COLOR;
  static bf_rt_field_id_t F_STORM_CONTROL_STATS_PKT_TYPE;
  static bf_rt_field_id_t F_STORM_CONTROL_STATS_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_STORM_CONTROL_STATS_DMAC_MISS;
  static bf_rt_field_id_t F_STORM_CONTROL_STATS_MULTICAST_HIT;
  static bf_rt_action_id_t A_STORM_CONTROL_STATS_COUNT;
  static bf_rt_action_id_t A_STORM_CONTROL_STATS_DROP_AND_COUNT;
  static bf_rt_field_id_t D_STORM_CONTROL_STATS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_STORM_CONTROL_STATS_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_STORM_CONTROL;
  static bf_rt_field_id_t F_STORM_CONTROL_LOCAL_MD_INGRESS_PORT;
  static bf_rt_field_id_t F_STORM_CONTROL_PKT_TYPE;
  static bf_rt_field_id_t F_STORM_CONTROL_DMAC_MISS;
  static bf_rt_field_id_t F_STORM_CONTROL_MULTICAST_HIT;
  static bf_rt_action_id_t A_STORM_CONTROL_SET_METER;
  static bf_rt_field_id_t P_STORM_CONTROL_SET_METER_INDEX;

  static bf_rt_table_id_t T_STORM_CONTROL_METER;
  static bf_rt_field_id_t F_STORM_CONTROL_METER_METER_INDEX;
  static bf_rt_field_id_t D_STORM_CONTROL_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_STORM_CONTROL_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_STORM_CONTROL_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_STORM_CONTROL_METER_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_INGRESS_MIRROR_METER_INDEX;
  static bf_rt_field_id_t
      F_INGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX;
  static bf_rt_action_id_t A_SET_INGRESS_MIRROR_METER;
  static bf_rt_field_id_t D_SET_INGRESS_MIRROR_METER_INDEX;

  static bf_rt_table_id_t T_INGRESS_MIRROR_METER_ACTION;
  static bf_rt_field_id_t F_INGRESS_MIRROR_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_INGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_MIRROR_METER_MIRROR_AND_COUNT;
  static bf_rt_action_id_t A_INGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_MIRROR_METER;
  static bf_rt_field_id_t F_INGRESS_MIRROR_METER_METER_INDEX;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_METER_SPEC_CIR_PPS;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_METER_SPEC_PIR_PPS;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS;
  static bf_rt_field_id_t D_INGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS;

  static bf_rt_table_id_t T_EGRESS_MIRROR_METER_INDEX;
  static bf_rt_field_id_t
      F_EGRESS_MIRROR_METER_INDEX_LOCAL_MD_MIRROR_METER_INDEX;
  static bf_rt_action_id_t A_SET_EGRESS_MIRROR_METER;
  static bf_rt_field_id_t D_SET_EGRESS_MIRROR_METER_INDEX;

  static bf_rt_table_id_t T_EGRESS_MIRROR_METER_ACTION;
  static bf_rt_field_id_t F_EGRESS_MIRROR_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_EGRESS_MIRROR_METER_ACTION_LOCAL_MD_MIRROR_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_MIRROR_METER_MIRROR_AND_COUNT;
  static bf_rt_action_id_t A_EGRESS_MIRROR_METER_NO_MIRROR_AND_COUNT;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_EGRESS_MIRROR_METER;
  static bf_rt_field_id_t F_EGRESS_MIRROR_METER_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_METER_SPEC_CIR_PPS;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_METER_SPEC_PIR_PPS;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_METER_SPEC_CBS_PKTS;
  static bf_rt_field_id_t D_EGRESS_MIRROR_METER_METER_SPEC_PBS_PKTS;

  static bf_rt_table_id_t T_INGRESS_PORT_METER_INDEX;
  static bf_rt_field_id_t
      F_INGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX;
  static bf_rt_action_id_t A_SET_INGRESS_PORT_METER;
  static bf_rt_field_id_t D_SET_INGRESS_PORT_METER_INDEX;

  static bf_rt_table_id_t T_INGRESS_PORT_METER_ACTION;
  static bf_rt_field_id_t F_INGRESS_PORT_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_INGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_PORT_METER_COUNT;
  static bf_rt_action_id_t A_INGRESS_PORT_METER_DROP_AND_COUNT;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_PORT_METER;
  static bf_rt_field_id_t F_INGRESS_PORT_METER_METER_INDEX;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_INGRESS_PORT_METER_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_EGRESS_PORT_METER_INDEX;
  static bf_rt_field_id_t
      F_EGRESS_PORT_METER_INDEX_LOCAL_MD_QOS_PORT_METER_INDEX;
  static bf_rt_action_id_t A_SET_EGRESS_PORT_METER;
  static bf_rt_field_id_t D_SET_EGRESS_PORT_METER_INDEX;

  static bf_rt_table_id_t T_EGRESS_PORT_METER_ACTION;
  static bf_rt_field_id_t F_EGRESS_PORT_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_EGRESS_PORT_METER_ACTION_LOCAL_MD_QOS_PORT_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_PORT_METER_COUNT;
  static bf_rt_action_id_t A_EGRESS_PORT_METER_DROP_AND_COUNT;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_EGRESS_PORT_METER;
  static bf_rt_field_id_t F_EGRESS_PORT_METER_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_EGRESS_PORT_METER_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_INGRESS_ACL_METER_INDEX;
  static bf_rt_field_id_t
      F_INGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX;
  static bf_rt_action_id_t A_SET_INGRESS_ACL_METER;
  static bf_rt_field_id_t D_SET_INGRESS_ACL_METER_INDEX;

  static bf_rt_table_id_t T_INGRESS_ACL_METER_ACTION;
  static bf_rt_field_id_t F_INGRESS_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_INGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX;
  static bf_rt_action_id_t A_INGRESS_ACL_METER_COUNT;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_COUNT_PACKET_ACTION;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_ACL_METER;
  static bf_rt_field_id_t F_INGRESS_ACL_METER_METER_INDEX;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_INGRESS_ACL_METER_METER_SPEC_PBS_KBITS;

  static bf_rt_table_id_t T_EGRESS_ACL_METER_INDEX;
  static bf_rt_field_id_t F_EGRESS_ACL_METER_INDEX_LOCAL_MD_QOS_ACL_METER_INDEX;
  static bf_rt_action_id_t A_SET_EGRESS_ACL_METER;
  static bf_rt_field_id_t D_SET_EGRESS_ACL_METER_INDEX;

  static bf_rt_table_id_t T_EGRESS_ACL_METER_ACTION;
  static bf_rt_field_id_t F_EGRESS_ACL_METER_ACTION_COLOR;
  static bf_rt_field_id_t
      F_EGRESS_ACL_METER_ACTION_LOCAL_MD_QOS_ACL_METER_INDEX;
  static bf_rt_action_id_t A_EGRESS_ACL_METER_COUNT;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_COUNT_PACKET_ACTION;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_ACTION_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_EGRESS_ACL_METER;
  static bf_rt_field_id_t F_EGRESS_ACL_METER_METER_INDEX;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_METER_SPEC_CIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_METER_SPEC_PIR_KBPS;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_METER_SPEC_CBS_KBITS;
  static bf_rt_field_id_t D_EGRESS_ACL_METER_METER_SPEC_PBS_KBITS;

  /* wred.p4 */
  static bf_rt_table_id_t T_ECN;
  static bf_rt_field_id_t F_ECN_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_ECN_LKP_IP_TOS;
  static bf_rt_field_id_t F_ECN_LKP_TCP_FLAGS;
  static bf_rt_action_id_t A_ECN_SET_INGRESS_COLOR;
  static bf_rt_field_id_t D_ECN_SET_INGRESS_COLOR_COLOR;

  static bf_rt_table_id_t T_WRED_SESSION;
  static bf_rt_field_id_t F_WRED_SESSION_INDEX;
  static bf_rt_field_id_t D_WRED_SESSION_TIME_CONSTANT;
  static bf_rt_field_id_t D_WRED_SESSION_MIN_THRES;
  static bf_rt_field_id_t D_WRED_SESSION_MAX_THRES;
  static bf_rt_field_id_t D_WRED_SESSION_MAX_PROB;

  static bf_rt_table_id_t T_V4_WRED_ACTION;
  static bf_rt_field_id_t F_V4_WRED_ACTION_INDEX;
  static bf_rt_field_id_t F_V4_WRED_ACTION_HDR_IPV4_ECN;
  // Below actions is common for both v4 & v6 wred action
  static bf_rt_action_id_t A_WRED_ACTION_DROP;
  static bf_rt_action_id_t A_WRED_ACTION_SET_IPV4_ECN;

  static bf_rt_table_id_t T_V6_WRED_ACTION;
  static bf_rt_field_id_t F_V6_WRED_ACTION_INDEX;
  static bf_rt_field_id_t F_V6_WRED_ACTION_HDR_IPV6_ECN;
  static bf_rt_action_id_t A_WRED_ACTION_SET_IPV6_ECN;

  static bf_rt_table_id_t T_WRED_INDEX;
  static bf_rt_field_id_t F_WRED_INDEX_EG_INTR_MD_EGRESS_PORT;
  static bf_rt_field_id_t F_WRED_INDEX_QOS_MD_QID;
  static bf_rt_field_id_t F_WRED_INDEX_QOS_MD_COLOR;
  static bf_rt_action_id_t A_WRED_INDEX_SET_WRED_INDEX;
  static bf_rt_field_id_t D_SET_WRED_INDEX_WRED_INDEX;

  static bf_rt_table_id_t T_EGRESS_WRED_STATS;
  static bf_rt_field_id_t F_EGRESS_WRED_STATS_PORT;
  static bf_rt_field_id_t F_EGRESS_WRED_STATS_QID;
  static bf_rt_field_id_t F_EGRESS_WRED_STATS_COLOR;
  static bf_rt_field_id_t F_EGRESS_WRED_STATS_WRED_DROP;
  static bf_rt_action_id_t A_EGRESS_WRED_STATS_COUNT;
  static bf_rt_field_id_t D_EGRESS_WRED_STATS_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_EGRESS_WRED_STATS_COUNTER_SPEC_PKTS;

  /* dtel.p4 */
  static bf_rt_table_id_t T_QUEUE_REPORT_QUOTAS;
  static bf_rt_field_id_t F_QUOTAS_REGISTER_INDEX;
  static bf_rt_field_id_t D_QUOTAS_COUNTER;
  static bf_rt_field_id_t D_QUOTAS_LATENCY;

  static bf_rt_table_id_t T_QUEUE_REPORT_THRESHOLDS;
  static bf_rt_field_id_t F_THRESHOLDS_REGISTER_INDEX;
  static bf_rt_field_id_t D_THRESHOLDS_QDEPTH;
  static bf_rt_field_id_t D_THRESHOLDS_LATENCY;

  static bf_rt_table_id_t T_QUEUE_REPORT_ALERT;
  static bf_rt_field_id_t F_QUEUE_REPORT_ALERT_QID;
  static bf_rt_field_id_t F_QUEUE_REPORT_ALERT_PORT;
  static bf_rt_action_id_t A_SET_QALERT;
  static bf_rt_field_id_t D_SET_QALERT_INDEX;
  static bf_rt_field_id_t D_SET_QALERT_QUOTA;
  static bf_rt_field_id_t D_SET_QALERT_QUANTIZATION_MASK;
  static bf_rt_action_id_t A_SET_QMASK;
  static bf_rt_field_id_t D_SET_QMASK_QUANTIZATION_MASK;

  static bf_rt_table_id_t T_QUEUE_REPORT_CHECK_QUOTA;
  static bf_rt_field_id_t F_CHECK_QUOTA_PKT_SRC;
  static bf_rt_field_id_t F_CHECK_QUOTA_QALERT;
  static bf_rt_field_id_t F_CHECK_QUOTA_QID;
  static bf_rt_field_id_t F_CHECK_QUOTA_PORT;
  static bf_rt_action_id_t A_RESET_QUOTA;
  static bf_rt_field_id_t D_RESET_QUOTA_INDEX;
  static bf_rt_action_id_t A_UPDATE_QUOTA;
  static bf_rt_field_id_t D_UPDATE_QUOTA_INDEX;
  static bf_rt_action_id_t A_CHECK_LATENCY_UPDATE_QUOTA;
  static bf_rt_field_id_t D_CHECK_LATENCY_UPDATE_QUOTA_INDEX;

  static bf_rt_table_id_t T_MOD_CONFIG;
  static bf_rt_field_id_t F_MOD_CONFIG_DROP_REASON;
  static bf_rt_field_id_t F_MOD_CONFIG_DTEL_MD_REPORT_TYPE;
  static bf_rt_field_id_t F_MOD_CONFIG_PRIORITY;
  static bf_rt_action_id_t A_MOD_CONFIG_MIRROR;
  static bf_rt_action_id_t A_MOD_CONFIG_MIRROR_AND_SET_D_BIT;

  static bf_rt_table_id_t T_DOD_CONFIG;
  static bf_rt_field_id_t F_DOD_CONFIG_LOCAL_MD_DTEL_REPORT_TYPE;
  static bf_rt_field_id_t F_DOD_CONFIG_EGRESS_PORT;
  static bf_rt_field_id_t F_DOD_CONFIG_QID;
  static bf_rt_field_id_t F_DOD_CONFIG_LOCAL_MD_MULTICAST_ID;
  static bf_rt_field_id_t F_DOD_CONFIG_LOCAL_MD_CPU_REASON;
  static bf_rt_field_id_t F_DOD_CONFIG_PRIORITY;
  static bf_rt_action_id_t A_DOD_CONFIG_ENABLE_DOD;
  static bf_rt_action_id_t A_DOD_CONFIG_DISABLE_DOD;

  static bf_rt_table_id_t T_DTEL_MIRROR_SESSION;
  static bf_rt_field_id_t F_DTEL_MIRROR_SESSION_HDR_ETHERNET_VALID;
  static bf_rt_field_id_t D_DTEL_MIRROR_SESSION_ACTION_MEMBER_ID;
  static bf_rt_field_id_t D_DTEL_MIRROR_SESSION_SELECTOR_GROUP_ID;

  static bf_rt_table_id_t SG_SESSION_SELECTOR_GROUP;
  static bf_rt_field_id_t F_SESSION_SELECTOR_GROUP_ID;
  static bf_rt_field_id_t P_SESSION_SELECTOR_GROUP_MAX_GROUP_SIZE;
  static bf_rt_field_id_t P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_ID_ARRAY;
  static bf_rt_field_id_t P_SESSION_SELECTOR_GROUP_ACTION_MEMBER_STATUS_ARRAY;

  static bf_rt_table_id_t AP_SESSION_SELECTOR;
  static bf_rt_field_id_t F_SESSION_SELECTOR_ACTION_MEMBER_ID;
  static bf_rt_action_id_t A_SET_MIRROR_SESSION;
  static bf_rt_field_id_t P_SET_MIRROR_SESSION_SESSION_ID;

  static bf_rt_table_id_t T_DTEL_ACL;
  static bf_rt_field_id_t F_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_DTEL_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_DTEL_ACL_LOCAL_MD_L4_PORT_LABEL;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_IP_SRC_ADDR;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_IP_DST_ADDR;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_IP_PROTO;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_IP_TTL;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_IP_TOS;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_L4_SRC_PORT;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_L4_DST_PORT;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_TCP_FLAGS;
  static bf_rt_field_id_t F_DTEL_ACL_LKP_MAC_TYPE;
  static bf_rt_field_id_t F_DTEL_ACL_PRIORITY;
  static bf_rt_action_id_t A_DTEL_ACL_HIT;
  static bf_rt_field_id_t P_ACL_HIT_REPORT_TYPE;

  static bf_rt_table_id_t T_IPV4_DTEL_ACL;
  static bf_rt_field_id_t F_IPV4_DTEL_ACL_LOCAL_MD_INGRESS_PORT_LAG_LABEL;
  static bf_rt_field_id_t F_IPV4_DTEL_ACL_LOCAL_MD_BD_LABEL;
  static bf_rt_field_id_t F_IPV4_DTEL_ACL_LOCAL_MD_L4_PORT_LABEL;
  static bf_rt_action_id_t A_IPV4_DTEL_ACL_HIT;
  static bf_rt_field_id_t P_IPV4_ACL_HIT_REPORT_TYPE;

  static bf_rt_table_id_t T_DTEL_CONFIG;
  static bf_rt_field_id_t F_DTEL_CONFIG_PKT_SRC;
  static bf_rt_field_id_t F_DTEL_CONFIG_REPORT_TYPE;
  static bf_rt_field_id_t F_DTEL_CONFIG_DROP_REPORT_FLAG;
  static bf_rt_field_id_t F_DTEL_CONFIG_FLOW_REPORT_FLAG;
  static bf_rt_field_id_t F_DTEL_CONFIG_QUEUE_REPORT_FLAG;
  static bf_rt_field_id_t F_DTEL_CONFIG_DROP_REASON;
  static bf_rt_field_id_t F_DTEL_CONFIG_MIRROR_TYPE;
  static bf_rt_field_id_t F_DTEL_CONFIG_DROP_HDR_VALID;
  static bf_rt_field_id_t F_DTEL_CONFIG_TCP_FLAGS;
  static bf_rt_field_id_t F_DTEL_CONFIG_IPV4_HDR_VALID;
  static bf_rt_field_id_t F_DTEL_CONFIG_IPV6_HDR_VALID;
  static bf_rt_field_id_t F_DTEL_CONFIG_IPV4_DIFFSERV;
  static bf_rt_field_id_t F_DTEL_CONFIG_IPV6_TRAFFIC_CLASS;
  static bf_rt_field_id_t F_DTEL_CONFIG_IFA_CLONED;
  static bf_rt_field_id_t F_DTEL_CONFIG_PRIORITY;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_F_AND_DROP;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_SWITCH_LOCAL_SET_Q_F_AND_DROP;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_DROP;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_DROP_SET_Q;
  static bf_rt_action_id_t A_DTEL_CONFIG_MIRROR_CLONE;
  static bf_rt_action_id_t A_DTEL_CONFIG_DROP;
  static bf_rt_action_id_t A_DTEL_CONFIG_UPDATE;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SWITCH_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_HW_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_NEXT_PROTO;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MD_LENGTH;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_REP_MD_BITS;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_REPORT_TYPE;
  static bf_rt_action_id_t A_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_SWITCH_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_HW_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_NEXT_PROTO;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_MD_LENGTH;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REP_MD_BITS;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_MIRROR_TRUNCATE_REPORT_TYPE;
  static bf_rt_action_id_t A_DTEL_CONFIG_UPDATE_SET_ETRAP;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_SWITCH_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_HW_ID;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_NEXT_PROTO;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_MD_LENGTH;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_REP_MD_BITS;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_REPORT_TYPE;
  static bf_rt_field_id_t D_DTEL_CONFIG_UPDATE_SET_ETRAP_STATUS;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_ALL;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_ALL;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_ALL;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_ALL;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_2;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_2;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_2;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_2;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_3;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_3;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_3;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_3;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_4;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_4;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_4;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_4;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_5;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_5;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_5;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_5;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_6;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_6;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_6;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_6;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV4_DSCP_7;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV4_DSCP_BIT_7;
  static bf_rt_action_id_t A_DTEL_CONFIG_SET_IPV6_DSCP_7;
  static bf_rt_field_id_t D_DTEL_CONFIG_IPV6_DSCP_BIT_7;

  static bf_rt_table_id_t T_INGRESS_PORT_CONVERSION;
  static bf_rt_field_id_t F_INGRESS_PORT_CONVERSION_PORT;
  static bf_rt_field_id_t F_INGRESS_PORT_CONVERSION_REPORT_VALID;
  static bf_rt_action_id_t A_CONVERT_INGRESS_PORT;
  static bf_rt_field_id_t D_CONVERT_INGRESS_PORT_PORT;

  static bf_rt_table_id_t T_EGRESS_PORT_CONVERSION;
  static bf_rt_field_id_t F_EGRESS_PORT_CONVERSION_PORT;
  static bf_rt_field_id_t F_EGRESS_PORT_CONVERSION_REPORT_VALID;
  static bf_rt_action_id_t A_CONVERT_EGRESS_PORT;
  static bf_rt_field_id_t D_CONVERT_EGRESS_PORT_PORT;

  static bf_rt_table_id_t T_EGRESS_DROP_REPORT_BLOOM_FILTER_1;
  static bf_rt_field_id_t F_EGRESS_DROP_REPORT_BLOOM_FILTER_1_REGISTER_INDEX;
  static bf_rt_field_id_t D_EGRESS_DROP_REPORT_BLOOM_FILTER_1_RESULT_VALUE;

  static bf_rt_table_id_t T_EGRESS_DROP_REPORT_BLOOM_FILTER_2;
  static bf_rt_field_id_t F_EGRESS_DROP_REPORT_BLOOM_FILTER_2_REGISTER_INDEX;
  static bf_rt_field_id_t D_EGRESS_DROP_REPORT_BLOOM_FILTER_2_RESULT_VALUE;

  static bf_rt_table_id_t T_EGRESS_FLOW_REPORT_BLOOM_FILTER_1;
  static bf_rt_field_id_t F_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_REGISTER_INDEX;
  static bf_rt_field_id_t D_EGRESS_FLOW_REPORT_BLOOM_FILTER_1_RESULT_VALUE;

  static bf_rt_table_id_t T_EGRESS_FLOW_REPORT_BLOOM_FILTER_2;
  static bf_rt_field_id_t F_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_REGISTER_INDEX;
  static bf_rt_field_id_t D_EGRESS_FLOW_REPORT_BLOOM_FILTER_2_RESULT_VALUE;

  static bf_rt_table_id_t T_INT_EDGE_PORT_LOOKUP;
  static bf_rt_field_id_t F_INT_EDGE_PORT_LOOKUP_PORT;
  static bf_rt_action_id_t A_INT_EDGE_SET_IFA_EDGE;
  static bf_rt_action_id_t A_INT_EDGE_SET_CLONE_MIRROR_SESSION_ID;
  static bf_rt_field_id_t D_INT_EDGE_CLONE_MIRROR_SESSION_ID;

  static bf_rt_table_id_t T_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION;
  static bf_rt_field_id_t F_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REGISTER_INDEX;
  static bf_rt_field_id_t D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_CURRENT;
  static bf_rt_field_id_t D_INGRESS_IP_DTEL_ACL_SAMPLE_SESSION_REG_RATE;

  static bf_rt_table_id_t T_INGRESS_SFLOW_SESSION;
  static bf_rt_field_id_t F_INGRESS_SFLOW_SESSION_REGISTER_INDEX;
  static bf_rt_field_id_t D_INGRESS_SFLOW_SESSION_REG_CURRENT;
  static bf_rt_field_id_t D_INGRESS_SFLOW_SESSION_REG_RATE;

  static bf_rt_table_id_t T_INGRESS_NAT_DNAPT_INDEX;
  static bf_rt_field_id_t F_INGRESS_NAT_DNAPT_INDEX_PROTOCOL;
  static bf_rt_field_id_t F_INGRESS_NAT_DNAPT_INDEX_DIP;
  static bf_rt_field_id_t F_INGRESS_NAT_DNAPT_INDEX_DPORT;
  static bf_rt_table_id_t A_INGRESS_NAT_DNAPT_SET_INDEX;
  static bf_rt_table_id_t P_INGRESS_NAT_DNAPT_SET_INDEX;
  static bf_rt_table_id_t D_INGRESS_NAT_DNAPT_INDEX_TTL;

  static bf_rt_table_id_t T_INGRESS_NAT_DEST_NAPT;
  static bf_rt_field_id_t F_INGRESS_NAT_DEST_NAPT_PROTOCOL;
  static bf_rt_field_id_t F_INGRESS_NAT_DEST_NAPT_DIP;
  static bf_rt_field_id_t F_INGRESS_NAT_DEST_NAPT_DPORT;
  static bf_rt_table_id_t F_INGRESS_NAT_DNAPT_INDEX;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAPT_REWRITE;
  static bf_rt_field_id_t P_DEST_NAPT_REWRITE_DIP;
  static bf_rt_field_id_t P_DEST_NAPT_REWRITE_DPORT;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAPT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAPT_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAPT_TTL;

  static bf_rt_table_id_t T_INGRESS_NAT_DEST_NAT;
  static bf_rt_field_id_t F_INGRESS_NAT_DEST_NAT_DIP;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAT_REWRITE;
  static bf_rt_field_id_t P_DEST_NAT_REWRITE_DIP;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAT_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAT_TTL;

  static bf_rt_table_id_t T_INGRESS_NAT_DEST_NAT_POOL;
  static bf_rt_field_id_t F_INGRESS_NAT_DEST_NAT_POOL_DIP;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAT_POOL_HIT;
  static bf_rt_action_id_t A_INGRESS_NAT_DEST_NAT_POOL_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_DEST_NAT_POOL_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_NAT_FLOW_NAPT;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAPT_PROTOCOL;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAPT_DIP;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAPT_DPORT;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAPT_SIP;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAPT_SPORT;
  static bf_rt_action_id_t A_INGRESS_NAT_FLOW_NAPT_REWRITE;
  static bf_rt_field_id_t P_FLOW_NAPT_REWRITE_DIP;
  static bf_rt_field_id_t P_FLOW_NAPT_REWRITE_SIP;
  static bf_rt_field_id_t P_FLOW_NAPT_REWRITE_DPORT;
  static bf_rt_field_id_t P_FLOW_NAPT_REWRITE_SPORT;
  static bf_rt_action_id_t A_INGRESS_NAT_FLOW_NAPT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_FLOW_NAPT_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_NAT_FLOW_NAT;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAT_DIP;
  static bf_rt_field_id_t F_INGRESS_NAT_FLOW_NAT_SIP;
  static bf_rt_action_id_t A_INGRESS_NAT_FLOW_NAT_REWRITE;
  static bf_rt_field_id_t P_FLOW_NAT_REWRITE_DIP;
  static bf_rt_field_id_t P_FLOW_NAT_REWRITE_SIP;
  static bf_rt_action_id_t A_INGRESS_NAT_FLOW_NAT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_FLOW_NAT_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_INGRESS_NAT_REWRITE;
  static bf_rt_field_id_t F_INGRESS_NAT_REWRITE_NAT_HIT;
  static bf_rt_field_id_t F_INGRESS_NAT_REWRITE_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_NAT_REWRITE_NAT_SAME_ZONE_CHECK;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_TCP_FLOW;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_UDP_FLOW;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_IPSA_IPDA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_TCP_DPORT_IPDA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_UDP_DPORT_IPDA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_IPDA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_TCP_SPORT_IPSA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_UDP_SPORT_IPSA;
  static bf_rt_action_id_t A_INGRESS_NAT_REWRITE_IPSA;

  static bf_rt_table_id_t T_INGRESS_NAT_SNAT;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAT_IPSA;
  static bf_rt_action_id_t A_INGRESS_NAT_SNAT_REWRITE;
  static bf_rt_field_id_t P_SOURCE_NAT_REWRITE_SIP;
  static bf_rt_action_id_t A_INGRESS_NAT_SNAT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAT_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAT_TTL;

  static bf_rt_table_id_t T_INGRESS_NAT_SNAPT_INDEX;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_INDEX_IPSA;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_INDEX_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_INDEX_IP_L4_SRC_PORT;
  static bf_rt_table_id_t A_INGRESS_NAT_SNAPT_SET_INDEX;
  static bf_rt_table_id_t P_INGRESS_NAT_SNAPT_SET_INDEX;
  static bf_rt_table_id_t D_INGRESS_NAT_SNAPT_INDEX_TTL;

  static bf_rt_table_id_t T_INGRESS_NAT_SNAPT;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_IPSA;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_IP_PROTO;
  static bf_rt_field_id_t F_INGRESS_NAT_SNAPT_IP_L4_SRC_PORT;
  static bf_rt_table_id_t F_INGRESS_NAT_SNAPT_INDEX;
  static bf_rt_action_id_t A_INGRESS_NAT_SNAPT_REWRITE;
  static bf_rt_field_id_t P_SOURCE_NAPT_REWRITE_SIP;
  static bf_rt_field_id_t P_SOURCE_NAPT_REWRITE_SPORT;
  static bf_rt_action_id_t A_INGRESS_NAT_SNAPT_MISS;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAPT_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAPT_COUNTER_SPEC_BYTES;
  static bf_rt_field_id_t D_INGRESS_NAT_SNAPT_TTL;

  // SRv6
  static bf_rt_table_id_t T_MY_SID;
  static bf_rt_field_id_t F_MY_SID_VRF;
  static bf_rt_field_id_t F_MY_SID_IPV6_DST_ADDR;
  static bf_rt_field_id_t F_MY_SID_SRH_SEG_LEFT;
  static bf_rt_field_id_t F_MY_SID_SRH_HDR_VALID;
  static bf_rt_field_id_t F_MY_SID_MATCH_PRIORITY;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_WITH_PSP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_WITH_USD;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_UN;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_X;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_X_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_X_WITH_PSP;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_X_WITH_PSP_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_X_WITH_USD;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_X_WITH_USD_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_UA;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_UA_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_T;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_T_VRF;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_DT4;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_DT4_VRF;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_DT6;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_DT6_VRF;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_DT46;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_DT46_VRF;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_DX4;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_DX4_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_DX6;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_DX6_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_B6_ENCAPS_RED;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_B6_ENCAPS_RED_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_END_B6_INSERT_RED;
  static bf_rt_field_id_t D_ENDPOINT_ACTION_END_B6_INSERT_RED_NEXTHOP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_TRAP;
  static bf_rt_action_id_t A_ENDPOINT_ACTION_DROP;
  static bf_rt_field_id_t D_MY_SID_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_MY_SID_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_SID_REWRITE;
  static bf_rt_field_id_t F_SID_REWRITE_LOCAL_MD_TUNNEL_NEXTHOP;
  static bf_rt_action_id_t A_SRV6_ENCAPS_SID_REWRITE_0;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_0_S0;
  static bf_rt_action_id_t A_SRV6_ENCAPS_SID_REWRITE_1;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_1_S0;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_1_S1;
  static bf_rt_action_id_t A_SRV6_ENCAPS_SID_REWRITE_2;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_2_S0;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_2_S1;
  static bf_rt_field_id_t D_SRV6_ENCAPS_SID_REWRITE_2_S2;
  static bf_rt_action_id_t A_SRV6_INSERT_SID_REWRITE_0;
  static bf_rt_field_id_t D_SRV6_INSERT_SID_REWRITE_0_S0;
  static bf_rt_action_id_t A_SRV6_INSERT_SID_REWRITE_1;
  static bf_rt_field_id_t D_SRV6_INSERT_SID_REWRITE_1_S0;
  static bf_rt_field_id_t D_SRV6_INSERT_SID_REWRITE_1_S1;
  static bf_rt_field_id_t D_SRV6_SID_REWRITE_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_SRV6_SID_REWRITE_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_MPLS_LABEL;
  static bf_rt_field_id_t F_MPLS_LABEL_LOCAL_MD_TUNNEL_NEXTHOP;
  static bf_rt_action_id_t A_MPLS_PUSH_1_LABEL;
  static bf_rt_field_id_t P_MPLS_PUSH_1_LABEL0;
  static bf_rt_action_id_t A_MPLS_PUSH_2_LABEL;
  static bf_rt_field_id_t P_MPLS_PUSH_2_LABEL0;
  static bf_rt_field_id_t P_MPLS_PUSH_2_LABEL1;
  static bf_rt_action_id_t A_MPLS_PUSH_3_LABEL;
  static bf_rt_field_id_t P_MPLS_PUSH_3_LABEL0;
  static bf_rt_field_id_t P_MPLS_PUSH_3_LABEL1;
  static bf_rt_field_id_t P_MPLS_PUSH_3_LABEL2;
  static bf_rt_action_id_t A_MPLS_SWAP_LABEL;
  static bf_rt_field_id_t P_MPLS_SWAP_LABEL0;

  static bf_rt_table_id_t T_MPLS_ENCAP;
  static bf_rt_field_id_t F_MPLS_PUSH_COUNT;
  static bf_rt_action_id_t A_MPLS_ENCAP_NOACTION;
  static bf_rt_action_id_t A_MPLS_ENCAP_1;
  static bf_rt_action_id_t A_MPLS_ENCAP_2;
  static bf_rt_action_id_t A_MPLS_ENCAP_3;

  static bf_rt_table_id_t T_MPLS_TTL_REWRITE;
  static bf_rt_field_id_t F_MPLS_TTL_REWRITE_PUSH_COUNT;
  static bf_rt_field_id_t F_MPLS_TTL_REWRITE_SWAP;
  static bf_rt_action_id_t A_MPLS_REWRITE_TTL_1_PIPE;
  static bf_rt_action_id_t A_MPLS_REWRITE_TTL_2_PIPE;
  static bf_rt_action_id_t A_MPLS_REWRITE_TTL_3_PIPE;
  static bf_rt_action_id_t A_MPLS_REWRITE_TTL_DECREMENT;

  static bf_rt_table_id_t T_MPLS_EXP_REWRITE;
  static bf_rt_field_id_t F_MPLS_EXP_REWRITE_PUSH_COUNT;
  static bf_rt_action_id_t A_MPLS_EXP_REWRITE_LABEL1;
  static bf_rt_action_id_t A_MPLS_EXP_REWRITE_LABEL2;
  static bf_rt_action_id_t A_MPLS_EXP_REWRITE_LABEL3;

  static bf_rt_action_id_t A_TUNNEL_NEXTHOP_MPLS_PUSH;
  static bf_rt_field_id_t P_MPLS_PUSH_LABEL_COUNT;
  static bf_rt_field_id_t P_MPLS_PUSH_ENCAP_TTL_MODE;
  static bf_rt_field_id_t P_MPLS_PUSH_ENCAP_TTL;
  static bf_rt_field_id_t P_MPLS_PUSH_ENCAP_QOS_MODE;
  static bf_rt_field_id_t P_MPLS_PUSH_ENCAP_EXP;
  static bf_rt_field_id_t P_MPLS_PUSH_SWAP;

  static bf_rt_table_id_t T_MPLS_FIB;
  static bf_rt_field_id_t F_MPLS_FIB_LOOKUP_LABEL;
  static bf_rt_action_id_t A_MPLS_FIB_MPLS_TERM;
  static bf_rt_action_id_t A_MPLS_FIB_MPLS_SWAP;
  static bf_rt_action_id_t A_MPLS_FIB_MPLS_PHP;
  static bf_rt_action_id_t A_MPLS_FIB_MPLS_DROP;
  static bf_rt_action_id_t A_MPLS_FIB_MPLS_TRAP;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_SWAP_NEXTHOP;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_SWAP_POP_COUNT;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_PHP_NEXTHOP;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_PHP_POP_COUNT;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_PHP_TTL_MODE;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_PHP_QOS_MODE;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_BD;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_VRF;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_VRF_TTL_VIOLATION_VALID;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_VRF_IP_OPTIONS_VIOLATION;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_BD_LABEL;
  //  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_RID;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_LEARN_MODE;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_V4_UNICAST_EN;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_V6_UNICAST_EN;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_POP_COUNT;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_TTL_MODE;
  static bf_rt_field_id_t P_MPLS_FIB_MPLS_TERM_QOS_MODE;
  static bf_rt_field_id_t D_MPLS_FIB_MPLS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_MPLS_FIB_MPLS_COUNTER_SPEC_BYTES;

  static bf_rt_table_id_t T_MPLS_POP;
  static bf_rt_action_id_t A_MPLS_POP_POP1;
  static bf_rt_action_id_t A_MPLS_POP_POP2;
  static bf_rt_action_id_t A_MPLS_POP_POP3;
  static bf_rt_field_id_t F_MPLS_POP_POP_COUNT;

  /* SFC */
  static bf_rt_table_id_t T_SFC_FILTER_EPOCH_REG;
  static bf_rt_field_id_t P_SFC_FILTER_EPOCH_REG_DURATION;

  /* Ip Port Stats */
  static bf_rt_table_id_t T_INGRESS_PORT_IP_STATS;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_PORT;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_HDR_IPV4_VALID;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_HDR_IPV6_VALID;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_DROP;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_COPY_TO_CPU;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_INGRESS_PORT_IP_STATS_PRIORITY;
  static bf_rt_field_id_t D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_INGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES;
  static bf_rt_action_id_t A_INGRESS_PORT_IP_STATS_COUNT;

  static bf_rt_table_id_t T_EGRESS_PORT_IP_STATS;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_PORT;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_HDR_IPV4_VALID;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_HDR_IPV6_VALID;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_DROP;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_COPY_TO_CPU;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_HDR_ETHERNET_DST_ADDR;
  static bf_rt_field_id_t F_EGRESS_PORT_IP_STATS_PRIORITY;
  static bf_rt_field_id_t D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_PKTS;
  static bf_rt_field_id_t D_EGRESS_PORT_IP_STATS_COUNTER_SPEC_BYTES;
  static bf_rt_action_id_t A_EGRESS_PORT_IP_STATS_COUNT;

  /* Rotate Hash */
  static bf_rt_table_id_t T_ROTATE_HASH;
  static bf_rt_action_id_t A_ROTATE_BY_0;
  static bf_rt_action_id_t A_ROTATE_BY_1;
  static bf_rt_action_id_t A_ROTATE_BY_2;
  static bf_rt_action_id_t A_ROTATE_BY_3;
  static bf_rt_action_id_t A_ROTATE_BY_4;
  static bf_rt_action_id_t A_ROTATE_BY_5;
  static bf_rt_action_id_t A_ROTATE_BY_6;
  static bf_rt_action_id_t A_ROTATE_BY_7;
  static bf_rt_action_id_t A_ROTATE_BY_8;
  static bf_rt_action_id_t A_ROTATE_BY_9;
  static bf_rt_action_id_t A_ROTATE_BY_10;
  static bf_rt_action_id_t A_ROTATE_BY_11;
  static bf_rt_action_id_t A_ROTATE_BY_12;
  static bf_rt_action_id_t A_ROTATE_BY_13;
  static bf_rt_action_id_t A_ROTATE_BY_14;
  static bf_rt_action_id_t A_ROTATE_BY_15;

  /* Fold */
  static bf_rt_table_id_t T_INGRESS_FP_FOLD;
  static bf_rt_field_id_t F_INGRESS_FP_IG_INTR_MD_INGRESS_PORT;
  static bf_rt_action_id_t A_INGRESS_FP_FOLD_SET_EGRESS_PORT;
  static bf_rt_field_id_t P_INGRESS_FP_FOLD_SET_EGRESS_PORT_DEV_PORT;

  /* BFD */
  static bf_rt_table_id_t T_BFD_TX_SESSION;
  static bf_rt_field_id_t F_BFD_TX_SESSION_ID;
  static bf_rt_action_id_t A_BFD_TX_SESSION_V4;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_TX_MULT;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_SESSION_ID;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_VRF;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_SIP;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_DIP;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_SPORT;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V4_DPORT;
  static bf_rt_action_id_t A_BFD_TX_SESSION_V6;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_TX_MULT;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_SESSION_ID;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_VRF;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_SIP;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_DIP;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_SPORT;
  static bf_rt_field_id_t P_BFD_TX_SESSION_V6_DPORT;
  static bf_rt_action_id_t A_BFD_TX_SESSION_BFD_TX_DROP_PKT;
  static bf_rt_table_id_t T_BFD_RX_SESSION;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_MY_DISCRIMINATOR;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_YOUR_DISCRIMINATOR;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_VERSION;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_FLAGS;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_DESIRED_MIN_TX_INTERVAL;
  static bf_rt_field_id_t F_BFD_RX_SESSION_HDR_BFD_REQ_MIN_RX_INTERVAL;
  static bf_rt_action_id_t A_BFD_RX_SESSION_INFO;
  static bf_rt_field_id_t P_BFD_RX_SESSION_INFO_RX_MULT;
  static bf_rt_field_id_t P_BFD_RX_SESSION_INFO_SESSION_ID;
  static bf_rt_field_id_t P_BFD_RX_SESSION_INFO_PKTGEN_PIPE;
  static bf_rt_field_id_t P_BFD_RX_SESSION_INFO_RECIRC_PORT;
  static bf_rt_action_id_t A_BFD_RX_SESSION_MISS;
  static bf_rt_table_id_t T_BFD_RX_TIMER;
  static bf_rt_field_id_t F_BFD_RX_TIMER_LOCAL_MD_BFD_PKT_TX;
  static bf_rt_field_id_t F_BFD_RX_TIMER_LOCAL_MD_BFD_SESSION_ID;
  static bf_rt_table_id_t T_BFD_RX_TIMER_REG;
  static bf_rt_field_id_t F_BFD_RX_TIMER_REG_INDEX;
  static bf_rt_field_id_t D_BFD_RX_TIMER_REG_DATA;
  static bf_rt_action_id_t A_BFD_RX_TIMER_RESET;
  static bf_rt_field_id_t P_BFD_RX_TIMER_RESET_SESSION_ID;
  static bf_rt_action_id_t A_BFD_RX_TIMER_CHECK;
  static bf_rt_field_id_t P_BFD_RX_TIMER_CHECK_SESSION_ID;
  static bf_rt_table_id_t T_BFD_PKT_ACTION;
  static bf_rt_field_id_t F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_TX;
  static bf_rt_field_id_t F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKT_ACTION;
  static bf_rt_field_id_t F_BFD_PKT_ACTION_LOCAL_MD_BFD_PKTGEN_PIPE;
  static bf_rt_field_id_t F_BFD_PKT_ACTION_PRIORITY;
  static bf_rt_action_id_t A_BFD_PKT_ACTION_BFD_PKT_TO_CPU;
  static bf_rt_action_id_t A_BFD_PKT_ACTION_BFD_RECIRC_TO_PKTGEN_PIPE;
  static bf_rt_action_id_t A_BFD_PKT_ACTION_BFD_TX_PKT;
  static bf_rt_action_id_t A_BFD_PKT_ACTION_NOACTION;
  static bf_rt_action_id_t A_BFD_PKT_ACTION_BFD_DROP_PKT;
  static bf_rt_table_id_t T_BFD_TX_TIMER;
  static bf_rt_field_id_t F_BFD_TX_TIMER_LOCAL_MD_BFD_SESSION_ID;
  static bf_rt_action_id_t A_BFD_TX_TIMER_CHECK;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_SESSION_ID;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_DETECT_MULTI;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_MY_DISCRIMINATOR;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_YOUR_DISCRIMINATOR;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_DESIRED_MIN_TX_INTERVAL;
  static bf_rt_field_id_t P_BFD_TX_TIMER_CHECK_REQ_MIN_RX_INTERVAL;
  static bf_rt_action_id_t A_BFD_TX_TIMER_BFD_DROP_PKT;

  /* pktgen */
  static bf_rt_table_id_t T_INGRESS_PKTGEN_PORT;
  static bf_rt_field_id_t F_INGRESS_PKTGEN_PORT_ETHER_TYPE;
  static bf_rt_field_id_t F_INGRESS_PKTGEN_PORT_PORT;
};

}  // namespace bf_rt
}  // namespace smi

#endif
