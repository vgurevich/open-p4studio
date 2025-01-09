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


#ifndef __P4_16_TYPES_H__
#define __P4_16_TYPES_H__

/**
 * The following enums and definitions are a copy of the p4 types.p4 program.
 * Not all values are duplicated here. Some come from switch.json directly.
 */

namespace smi {

typedef uint8_t switch_tunnel_mode_t;
const switch_tunnel_mode_t SWITCH_TUNNEL_MODE_PIPE = 0;
const switch_tunnel_mode_t SWITCH_TUNNEL_MODE_UNIFORM = 1;

const switch_tunnel_mode_t SWITCH_ECN_MODE_STANDARD = 0;
const switch_tunnel_mode_t SWITCH_ECN_MODE_COPY_FROM_OUTER = 1;

typedef uint8_t switch_pkt_src_t;
const switch_pkt_src_t SWITCH_PKT_SRC_BRIDGED = 0;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_INGRESS = 1;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_EGRESS = 2;
const switch_pkt_src_t SWITCH_PKT_SRC_DEFLECTED = 3;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_EGRESS_IN_PKT = 4;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_INGRESS_RSPAN = 5;
const switch_pkt_src_t SWITCH_PKT_SRC_CLONED_EGRESS_RSPAN = 6;

typedef uint8_t switch_packet_action_t;
const switch_packet_action_t SWITCH_PACKET_ACTION_PERMIT = 0;
const switch_packet_action_t SWITCH_PACKET_ACTION_DROP = 1;
const switch_packet_action_t SWITCH_PACKET_ACTION_COPY = 2;
const switch_packet_action_t SWITCH_PACKET_ACTION_TRAP = 3;

typedef uint8_t switch_mirror_type_t;
const switch_mirror_type_t SWITCH_MIRROR_TYPE_INVALID = 0;

typedef uint8_t switch_ip_frag_t;
const switch_ip_frag_t SWITCH_IP_FRAG_NON_FRAG = 0b00;  // Not fragmented.
const switch_ip_frag_t SWITCH_IP_FRAG_HEAD =
    0b10;  // First fragment of the fragmented packets.
const switch_ip_frag_t SWITCH_IP_FRAG_NON_HEAD =
    0b11;  // Fragment with non-zero offset.

typedef uint32_t switch_drop_reason_t;
const switch_drop_reason_t SWITCH_DROP_REASON_UNKNOWN = 0;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO = 10;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST = 11;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO = 12;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_ETHERNET_MISS = 13;
const switch_drop_reason_t SWITCH_DROP_REASON_SRC_MAC_ZERO = 14;
const switch_drop_reason_t SWITCH_DROP_REASON_SRC_MAC_MULTICAST = 15;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_ZERO = 16;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK = 17;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST = 18;
const switch_drop_reason_t SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST = 19;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID = 25;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO = 26;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST = 27;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK = 28;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_MISS = 29;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID = 30;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM = 31;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK = 32;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED = 33;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E = 34;
const switch_drop_reason_t SWITCH_DROP_REASON_OUTER_IP_DST_UNSPECIFIED = 35;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_VERSION_INVALID = 40;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_TTL_ZERO = 41;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_MULTICAST = 42;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_LOOPBACK = 43;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_IHL_INVALID = 44;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_INVALID_CHECKSUM = 45;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_CLASS_E = 46;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_DST_LINK_LOCAL = 47;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL = 48;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_DST_UNSPECIFIED = 49;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED = 50;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_LPM4_MISS = 51;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_LPM6_MISS = 52;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE = 53;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_PORT_RMAC_MISS = 54;
const switch_drop_reason_t SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS = 55;
const switch_drop_reason_t SWITCH_DROP_REASON_STP_STATE_LEARNING = 56;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING = 57;
const switch_drop_reason_t SWITCH_DROP_REASON_SAME_IFINDEX = 58;
const switch_drop_reason_t SWITCH_DROP_REASON_MULTICAST_SNOOPING_ENABLED = 59;
const switch_drop_reason_t SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN = 60;
const switch_drop_reason_t SWITCH_DROP_REASON_MTU_CHECK_FAIL = 70;
const switch_drop_reason_t SWITCH_DROP_REASON_TRAFFIC_MANAGER = 71;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL = 72;
const switch_drop_reason_t SWITCH_DROP_REASON_WRED = 73;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_PORT_METER = 75;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_ACL_METER = 76;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_PORT_METER = 77;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_ACL_METER = 78;
const switch_drop_reason_t SWITCH_DROP_REASON_ACL_DROP = 80;
const switch_drop_reason_t SWITCH_DROP_REASON_RACL_DENY = 81;
const switch_drop_reason_t SWITCH_DROP_REASON_URPF_CHECK_FAIL = 82;
const switch_drop_reason_t SWITCH_DROP_REASON_IPSG_MISS = 83;
const switch_drop_reason_t SWITCH_DROP_REASON_IFINDEX = 84;
const switch_drop_reason_t SWITCH_DROP_REASON_CPU_COLOR_YELLOW = 85;
const switch_drop_reason_t SWITCH_DROP_REASON_CPU_COLOR_RED = 86;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL_COLOR_YELLOW = 87;
const switch_drop_reason_t SWITCH_DROP_REASON_STORM_CONTROL_COLOR_RED = 88;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_UNICAST = 89;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_MULTICAST = 90;
const switch_drop_reason_t SWITCH_DROP_REASON_L2_MISS_BROADCAST = 91;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_ACL_DROP = 92;
const switch_drop_reason_t SWITCH_DROP_REASON_NEXTHOP = 93;
const switch_drop_reason_t SWITCH_DROP_REASON_NON_IP_ROUTER_MAC = 94;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_IPV4_DISABLE = 99;
const switch_drop_reason_t SWITCH_DROP_REASON_L3_IPV6_DISABLE = 100;
const switch_drop_reason_t SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP = 101;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_PFC_WD_DROP = 102;
const switch_drop_reason_t SWITCH_DROP_REASON_MPLS_LABEL_DROP = 103;
const switch_drop_reason_t SWITCH_DROP_REASON_SRV6_MY_SID_DROP = 104;
const switch_drop_reason_t SWITCH_DROP_REASON_PORT_ISOLATION_DROP = 105;
const switch_drop_reason_t SWITCH_DROP_REASON_DMAC_RESERVED = 106;
const switch_drop_reason_t SWITCH_DROP_REASON_NON_ROUTABLE = 107;
const switch_drop_reason_t SWITCH_DROP_REASON_MPLS_DISABLE = 108;
const switch_drop_reason_t SWITCH_DROP_REASON_EGRESS_STP_STATE_BLOCKING = 110;
const switch_drop_reason_t SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH = 111;
const switch_drop_reason_t SWITCH_DROP_REASON_SIP_BC = 112;
const switch_drop_reason_t SWITCH_DROP_REASON_IPV6_MC_SCOPE0 = 113;
const switch_drop_reason_t SWITCH_DROP_REASON_IPV6_MC_SCOPE1 = 114;
const switch_drop_reason_t SWITCH_DROP_REASON_ACL_DENY = 115;

const switch_drop_reason_t drop_reasons[] = {
    SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_ETHERNET_MISS,
    SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK,
    SWITCH_DROP_REASON_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_SRC_MAC_MULTICAST,
    SWITCH_DROP_REASON_DST_MAC_ZERO,
    SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST,
    SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST,
    SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO,
    SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_OUTER_IP_MISS,
    SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK,
    SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED,
    SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E,
    SWITCH_DROP_REASON_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_IP_TTL_ZERO,
    SWITCH_DROP_REASON_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_IP_IHL_INVALID,
    SWITCH_DROP_REASON_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_IP_SRC_CLASS_E,
    SWITCH_DROP_REASON_IP_DST_LINK_LOCAL,
    SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL,
    SWITCH_DROP_REASON_IP_DST_UNSPECIFIED,
    SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED,
    SWITCH_DROP_REASON_IP_LPM4_MISS,
    SWITCH_DROP_REASON_IP_LPM6_MISS,
    SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE,
    SWITCH_DROP_REASON_L3_PORT_RMAC_MISS,
    SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS,
    SWITCH_DROP_REASON_STP_STATE_LEARNING,
    SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING,
    SWITCH_DROP_REASON_EGRESS_STP_STATE_BLOCKING,
    SWITCH_DROP_REASON_SAME_IFINDEX,
    SWITCH_DROP_REASON_MULTICAST_SNOOPING_ENABLED,
    SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN,
    SWITCH_DROP_REASON_MTU_CHECK_FAIL,
    SWITCH_DROP_REASON_TRAFFIC_MANAGER,
    SWITCH_DROP_REASON_STORM_CONTROL,
    SWITCH_DROP_REASON_WRED,
    SWITCH_DROP_REASON_INGRESS_PORT_METER,
    SWITCH_DROP_REASON_INGRESS_ACL_METER,
    SWITCH_DROP_REASON_EGRESS_PORT_METER,
    SWITCH_DROP_REASON_EGRESS_ACL_METER,
    SWITCH_DROP_REASON_ACL_DROP,
    SWITCH_DROP_REASON_RACL_DENY,
    SWITCH_DROP_REASON_URPF_CHECK_FAIL,
    SWITCH_DROP_REASON_IPSG_MISS,
    SWITCH_DROP_REASON_IFINDEX,
    SWITCH_DROP_REASON_CPU_COLOR_YELLOW,
    SWITCH_DROP_REASON_CPU_COLOR_RED,
    SWITCH_DROP_REASON_STORM_CONTROL_COLOR_YELLOW,
    SWITCH_DROP_REASON_STORM_CONTROL_COLOR_RED,
    SWITCH_DROP_REASON_L2_MISS_UNICAST,
    SWITCH_DROP_REASON_L2_MISS_MULTICAST,
    SWITCH_DROP_REASON_L2_MISS_BROADCAST,
    SWITCH_DROP_REASON_EGRESS_ACL_DROP,
    SWITCH_DROP_REASON_NEXTHOP,
    SWITCH_DROP_REASON_NON_IP_ROUTER_MAC,
    SWITCH_DROP_REASON_L3_IPV4_DISABLE,
    SWITCH_DROP_REASON_L3_IPV6_DISABLE,
    SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP,
    SWITCH_DROP_REASON_EGRESS_PFC_WD_DROP,
    SWITCH_DROP_REASON_MPLS_LABEL_DROP,
    SWITCH_DROP_REASON_SRV6_MY_SID_DROP,
    SWITCH_DROP_REASON_PORT_ISOLATION_DROP,
    SWITCH_DROP_REASON_DMAC_RESERVED,
    SWITCH_DROP_REASON_NON_ROUTABLE,
    SWITCH_DROP_REASON_MPLS_DISABLE,
    SWITCH_DROP_REASON_SIP_BC,
    SWITCH_DROP_REASON_IPV6_MC_SCOPE0,
    SWITCH_DROP_REASON_IPV6_MC_SCOPE1,
    SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH,
    SWITCH_DROP_REASON_ACL_DENY};

const switch_drop_reason_t ingress_drop_reasons[] = {
    SWITCH_DROP_REASON_OUTER_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_SRC_MAC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_DST_MAC_ZERO,
    SWITCH_DROP_REASON_OUTER_ETHERNET_MISS,
    SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK,
    SWITCH_DROP_REASON_SRC_MAC_ZERO,
    SWITCH_DROP_REASON_SRC_MAC_MULTICAST,
    SWITCH_DROP_REASON_DST_MAC_ZERO,
    SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_UCAST,
    SWITCH_DROP_REASON_DST_MAC_MCAST_DST_IP_BCAST,
    SWITCH_DROP_REASON_OUTER_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_TTL_ZERO,
    SWITCH_DROP_REASON_OUTER_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_OUTER_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_OUTER_IP_MISS,
    SWITCH_DROP_REASON_OUTER_IP_IHL_INVALID,
    SWITCH_DROP_REASON_OUTER_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_OUTER_IP_DST_LOOPBACK,
    SWITCH_DROP_REASON_OUTER_IP_SRC_UNSPECIFIED,
    SWITCH_DROP_REASON_OUTER_IP_SRC_CLASS_E,
    SWITCH_DROP_REASON_IP_VERSION_INVALID,
    SWITCH_DROP_REASON_IP_TTL_ZERO,
    SWITCH_DROP_REASON_IP_SRC_MULTICAST,
    SWITCH_DROP_REASON_IP_SRC_LOOPBACK,
    SWITCH_DROP_REASON_IP_IHL_INVALID,
    SWITCH_DROP_REASON_IP_INVALID_CHECKSUM,
    SWITCH_DROP_REASON_IP_SRC_CLASS_E,
    SWITCH_DROP_REASON_IP_DST_LINK_LOCAL,
    SWITCH_DROP_REASON_IP_SRC_LINK_LOCAL,
    SWITCH_DROP_REASON_IP_DST_UNSPECIFIED,
    SWITCH_DROP_REASON_IP_SRC_UNSPECIFIED,
    SWITCH_DROP_REASON_IP_LPM4_MISS,
    SWITCH_DROP_REASON_IP_LPM6_MISS,
    SWITCH_DROP_REASON_IP_BLACKHOLE_ROUTE,
    SWITCH_DROP_REASON_L3_PORT_RMAC_MISS,
    SWITCH_DROP_REASON_PORT_VLAN_MAPPING_MISS,
    SWITCH_DROP_REASON_STP_STATE_LEARNING,
    SWITCH_DROP_REASON_INGRESS_STP_STATE_BLOCKING,
    SWITCH_DROP_REASON_SAME_IFINDEX,
    SWITCH_DROP_REASON_MULTICAST_SNOOPING_ENABLED,
    SWITCH_DROP_REASON_IN_L3_EGRESS_LINK_DOWN,
    SWITCH_DROP_REASON_INGRESS_ACL_METER,
    SWITCH_DROP_REASON_INGRESS_PORT_METER,
    SWITCH_DROP_REASON_NEXTHOP,
    SWITCH_DROP_REASON_ACL_DROP,
    SWITCH_DROP_REASON_INGRESS_PFC_WD_DROP,
    SWITCH_DROP_REASON_L3_IPV4_DISABLE,
    SWITCH_DROP_REASON_L3_IPV6_DISABLE,
    SWITCH_DROP_REASON_NON_IP_ROUTER_MAC,
    SWITCH_DROP_REASON_L2_MISS_UNICAST,
    SWITCH_DROP_REASON_L2_MISS_MULTICAST,
    SWITCH_DROP_REASON_L2_MISS_BROADCAST,
    SWITCH_DROP_REASON_CPU_COLOR_YELLOW,
    SWITCH_DROP_REASON_CPU_COLOR_RED,
    SWITCH_DROP_REASON_MPLS_LABEL_DROP,
    SWITCH_DROP_REASON_SRV6_MY_SID_DROP,
    SWITCH_DROP_REASON_DMAC_RESERVED,
    SWITCH_DROP_REASON_NON_ROUTABLE,
    SWITCH_DROP_REASON_MPLS_DISABLE,
    SWITCH_DROP_REASON_SIP_BC,
    SWITCH_DROP_REASON_IPV6_MC_SCOPE0,
    SWITCH_DROP_REASON_IPV6_MC_SCOPE1,
    SWITCH_DROP_REASON_IP_MULTICAST_DMAC_MISMATCH,
    SWITCH_DROP_REASON_ACL_DENY};

const switch_drop_reason_t egress_drop_reasons[] = {
    SWITCH_DROP_REASON_MTU_CHECK_FAIL,
    SWITCH_DROP_REASON_TRAFFIC_MANAGER,
    SWITCH_DROP_REASON_STORM_CONTROL,
    SWITCH_DROP_REASON_WRED,
    SWITCH_DROP_REASON_EGRESS_PORT_METER,
    SWITCH_DROP_REASON_EGRESS_ACL_METER,
    SWITCH_DROP_REASON_RACL_DENY,
    SWITCH_DROP_REASON_URPF_CHECK_FAIL,
    SWITCH_DROP_REASON_IPSG_MISS,
    SWITCH_DROP_REASON_IFINDEX,
    SWITCH_DROP_REASON_STORM_CONTROL_COLOR_YELLOW,
    SWITCH_DROP_REASON_STORM_CONTROL_COLOR_RED,
    SWITCH_DROP_REASON_EGRESS_ACL_DROP,
    SWITCH_DROP_REASON_EGRESS_PFC_WD_DROP,
    SWITCH_DROP_REASON_PORT_ISOLATION_DROP};

typedef enum switch_pkt_color_t {
  SWITCH_PKT_COLOR_GREEN = 0,
  SWITCH_PKT_COLOR_YELLOW = 1,
  SWITCH_PKT_COLOR_RED = 3
} switch_pkt_color_t;

typedef enum switch_ip_type_t {
  SWITCH_IP_TYPE_NONE = 0,
  SWITCH_IP_TYPE_IPV4 = 1,
  SWITCH_IP_TYPE_IPV6 = 2
} switch_ip_type_t;

typedef enum switch_api_tunnel_type_t {
  SWITCH_IPV4_VXLAN = 1,
  SWITCH_IPV6_VXLAN = 2,
  SWITCH_IPV4_IPIP = 3,
  SWITCH_IPV6_IPIP = 4,
  SWITCH_IPV4_NVGRE = 5,
  SWITCH_IPV6_NVGRE = 6,
  SWITCH_MPLS = 7,
  SWITCH_SRV6_ENCAP = 8,
  SWITCH_SRV6_INSERT = 9,
  SWITCH_IPV4_GRE = 10,
  SWITCH_IPV6_GRE = 11
} switch_api_tunnel_type_t;

typedef enum switch_pkt_type_t {
  SWITCH_PKT_TYPE_UNICAST = 0,
  SWITCH_PKT_TYPE_MULTICAST = 1,
  SWITCH_PKT_TYPE_BROADCAST = 2
} switch_pkt_type_t;

typedef uint8_t switch_nat_type_t;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_FLOW_NONE = 1;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_FLOW_NAPT = 2;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_FLOW_NAT = 3;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_DEST_NONE = 4;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_DEST_NAPT = 5;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_DEST_NAT = 6;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_SRC_NONE = 7;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_SRC_NAPT = 8;
const switch_nat_type_t SWITCH_NAT_HIT_TYPE_SRC_NAT = 9;

typedef uint8_t bfd_pkt_action_t;
const bfd_pkt_action_t BFD_PKT_ACTION_DROP = 0x00;
const bfd_pkt_action_t BFD_PKT_ACTION_TIMEOUT = 0x01;
const bfd_pkt_action_t BFD_PKT_ACTION_NORMAL = 0x02;

#define SWITCH_RIF_INSIDE_NAT_ZONE_ID 0
#define SWITCH_RIF_OUTSIDE_NAT_ZONE_ID 1

typedef uint8_t switch_myip_type_t;

typedef uint8_t switch_in_ports_label_t;
typedef uint8_t switch_out_ports_label_t;

typedef uint32_t switch_ig_port_lag_label_t;
typedef uint32_t switch_eg_port_lag_label_t;
typedef uint16_t switch_bd_label_t;
/* The following typedef is not directly from p4 code.
 * It is used for acl_table and acl_group acl_label, label allocation, etc */
typedef std::conditional<(sizeof(switch_ig_port_lag_label_t) >
                          sizeof(switch_bd_label_t)),
                         switch_ig_port_lag_label_t,
                         switch_bd_label_t>::type switch_acl_label_t;

const uint16_t ether_type_bfn = 0x9000;

#define SWITCH_DTEL_SUPPRESS_REPORT 0x8
#define SWITCH_DTEL_REPORT_TYPE_IFA_CLONE 0x10
#define SWITCH_DTEL_IFA_EDGE 0x20
#define SWITCH_DTEL_REPORT_TYPE_ETRAP_CHANGE 0x40
#define SWITCH_DTEL_REPORT_TYPE_ETRAP_HIT 0x80

typedef uint16_t switch_dtel_rep_md_bits_t;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_LEVEL_1_IF_IDS = 0x4000;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_HOP_LATENCY = 0x2000;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_QUEUE_OCCUPANCY =
    0x1000;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_INGRESS_TSTAMP = 0x0800;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_EGRESS_TSTAMP = 0x0400;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_LEVEL_2_IF_IDS = 0x0200;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_EG_PORT_TX_UTIL =
    0x0100;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_BUFFER_OCCUPANCY =
    0x0080;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_DROP_REASON = 0x0001;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_DROP_DEFAULT =
    SWITCH_DTEL_REP_MD_BITS_LEVEL_1_IF_IDS |
    SWITCH_DTEL_REP_MD_BITS_INGRESS_TSTAMP |
    SWITCH_DTEL_REP_MD_BITS_DROP_REASON;
const uint8_t SWITCH_DTEL_MD_LENGTH_DROP_DEFAULT = 4;
const switch_dtel_rep_md_bits_t SWITCH_DTEL_REP_MD_BITS_SWITCH_LOCAL_DEFAULT =
    SWITCH_DTEL_REP_MD_BITS_LEVEL_1_IF_IDS |
    SWITCH_DTEL_REP_MD_BITS_QUEUE_OCCUPANCY |
    SWITCH_DTEL_REP_MD_BITS_INGRESS_TSTAMP |
    SWITCH_DTEL_REP_MD_BITS_EGRESS_TSTAMP;
const uint8_t SWITCH_DTEL_MD_LENGTH_SWITCH_LOCAL_DEFAULT = 6;

// Outer header sizes for Telemetry Report v0.5
/* Up to the beginning of the Telemetry Report v0.5 header
 * 14 (Eth) + 20 (IPv4) + 8 (UDP) + 4 (CRC) = 46 bytes */
const uint16_t DTEL_REPORT_V0_5_OUTER_HEADERS_LENGTH = 46;
/* 12 (Telem Report Fixed Header) + 16 (Telem Switch Local Report Header) = 28*/
const uint16_t DTEL_REPORT_V0_5_DTEL_HEADERS_MAX_LENGTH = 28;
/* 12 (Telem Report Fixed Header) + 12 (Telem Drop Report Header) = 24 bytes */
const uint16_t DTEL_REPORT_V0_5_DTEL_HEADERS_DROP_LENGTH = 24;

// Outer header sizes for Telemetry Report v2.0
/* Outer headers + part of Telem Report v2 length not included in report_length
 * 14 (Eth) + 20 (IPv4) + 8 (UDP) + 12 (DTEL) + 4 (CRC) = 58 bytes */
const uint16_t DTEL_REPORT_V2_OUTER_HEADERS_LENGTH = 58;
/* Max length of Telem Report v2 headers part that is included in report_length
 * 8 (rep_md_bits + domain_specific_id + ds_md_bits + ds_md_status)
 * + 4 (level_1_if_ids) + 4 (queue_id + queue_occupancy)
 * + 8 (ingress_timestamp) + 8 (egress_timestamp)
 * = 32 bytes */
const uint16_t DTEL_REPORT_V2_DTEL_HEADERS_MAX_LENGTH = 32;
/* Max length of Telem Report v2 headers part that is included in report_length
 * for dropped packets
 * 8 (rep_md_bits + domain_specific_id + ds_md_bits + ds_md_status)
 * + 4 (level_1_if_ids) + 8 (ingress_timestamp) + 4 (queue_id + drop_reason)
 * = 24 bytes */
const uint16_t DTEL_REPORT_V2_DTEL_HEADERS_DROP_MAX_LENGTH = 24;

const uint16_t len_of_t1_switch_dtel_switch_local_mirror_metadata_h = 25;
const uint16_t len_of_t2_switch_dtel_switch_local_mirror_metadata_h = 24;
const uint16_t len_of_t1_switch_dtel_v2_switch_local_mirror_metadata_h = 29;
const uint16_t len_of_t2_switch_dtel_v2_switch_local_mirror_metadata_h = 28;
const uint16_t len_of_t1_switch_port_mirror_metadata_h = 10;
const uint16_t len_of_t2_switch_port_mirror_metadata_h = 9;

const uint8_t SWITCH_SFLOW_INVALID_ID = 0xFF;

}  // namespace smi

#endif  // __DROP_REASONS_H__
