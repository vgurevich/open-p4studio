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


#ifndef __COMMON_HOSTIF_H__
#define __COMMON_HOSTIF_H__

#define SWITCH_ETHERTYPE_IPV4 0x0800
#define SWITCH_ETHERTYPE_IPV6 0x86dd
#define SWITCH_ETHERTYPE_PTP 0x88F7
#define SWITCH_ETHERTYPE_ARP 0x806
#define SWITCH_ETHERTYPE_LLDP 0x88CC
#define SWITCH_ETHERTYPE_QINQ 0x9100
#define SWITCH_ETHERTYPE_DOT1Q 0x8100
#define SWITCH_ETHERTYPE_EAPOL 0x888E

#define SWITCH_HOSTIF_IP_PROTO_VRRP 112
#define SWITCH_HOSTIF_IP_PROTO_OSPF 89
#define SWITCH_HOSTIF_IP_PROTO_PIM 103
#define SWITCH_HOSTIF_IP_PROTO_IGMP 2
#define SWITCH_HOSTIF_IP_PROTO_ICMP 1
#define SWITCH_HOSTIF_IP_PROTO_TCP 6
#define SWITCH_HOSTIF_IP_PROTO_UDP 17
#define SWITCH_HOSTIF_IP_PROTO_ICMPV6 58
#define SWITCH_HOSTIF_IPV6_ICMP_TYPE_RS 133
#define SWITCH_HOSTIF_IPV6_ICMP_TYPE_RA 134
#define SWITCH_HOSTIF_IPV6_ICMP_TYPE_NS 135
#define SWITCH_HOSTIF_IPV6_ICMP_TYPE_NA 136
#define SWITCH_HOSTIF_IPV6_ICMP_REDIRECT 137
#define SWITCH_HOSTIF_BGP_PORT 179
#define SWITCH_HOSTIF_SSH_PORT 22
#define SWITCH_HOSTIF_GNMI_PORT 9339
#define SWITCH_HOSTIF_P4RT_PORT 9559
#define SWITCH_HOSTIF_NTPCLIENT_PORT 123
#define SWITCH_HOSTIF_NTPSERVER_PORT 123
#define SWITCH_HOSTIF_SNMP_PORT 161
#define SWITCH_HOSTIF_DHCP_PORT1 67
#define SWITCH_HOSTIF_DHCP_PORT2 68
#define SWITCH_HOSTIF_DHCPV6_PORT1 546
#define SWITCH_HOSTIF_DHCPV6_PORT2 547
#define SWITCH_HOSTIF_BFD_DST_PORT1 3784
#define SWITCH_HOSTIF_BFD_DST_PORT2 4784
#define SWITCH_HOSTIF_PTP_DST_PORT1 319
#define SWITCH_HOSTIF_PTP_DST_PORT2 320
#define SWITCH_HOSTIF_LDP_PORT 179
#define SWITCH_HOSTIF_ICCP_PORT 8888
#define SWITCH_HOSTIF_IGMP_TYPE_QUERY 0x11
#define SWITCH_HOSTIF_IGMP_TYPE_LEAVE 0x17
#define SWITCH_HOSTIF_IGMP_TYPE_V1_REPORT 0x12
#define SWITCH_HOSTIF_IGMP_TYPE_V2_REPORT 0x16
#define SWITCH_HOSTIF_IGMP_TYPE_V3_REPORT 0x22
#define SWITCH_HOSTIF_IPV6_MLD_V1_V2_QUERY 130
#define SWITCH_HOSTIF_IPV6_MLD_V1_REPORT 131
#define SWITCH_HOSTIF_IPV6_MLD_V1_DONE 132
#define SWITCH_HOSTIF_MLD_V2_REPORT 143

#endif /* __COMMON_HOSTIF_H__ */
