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

#ifndef _DIAG_DEFINES_
#define _DIAG_DEFINES_

// Different Profiles in Diag
//  1. Basic Diags
//  2. Diag Single stage                 DIAG_SINGLE_STAGE
//  3. Diag Power                        DIAG_POWER_ENABLE
//  4. MAU bus stress                    DIAG_MAU_BUS_STRESS_ENABLE
//  5. PHV, Parde Stress                 DIAG_PHV_PARDE_STRESS_ENABLE
//  6. PHV, Parde, Hold Stress           DIAG_PHV_PARDE_HOLD_STRESS_ENABLE
//  7. Queue Stress                      DIAG_QUEUE_STRESS_ENABLE
//  8. Parde Strain                      DIAG_PARDE_STRAIN
//  9. PHV Action Flop Tests for TF1     DIAG_PHV_FLOP_TEST={1|2}
// 10. PHV Match Flop Tests  for TF1     DIAG_PHV_FLOP_MATCH={1|2}
// 11. PHV Flop Tests for Ruije TF2      DIAG_PHV_FLOP_TEST=3
// 12. PHV Datapath for TF2 action dep   DIAG_PHV_FLOP_TEST=4
// 13. PHV Datapath for TF2 match dep    DIAG_PHV_FLOP_TEST=5
// 14. PHV Dark/Mocha TF2 action dep     DIAG_PHV_MOCHA_DARK
// 15. PHV Dark/Mocha TF2 match dep      DIAG_PHV_MOCHA_DARK_MATCH_DEPENDENT

// Different products supported in Diag
// 1. Tofino1  - 12 stages - TOFINO1
// 2. Tofino2  - 20 stages - TOFINO2
// 3. Tofino2u - 20 stages - TOFINO2U
// 4. Tofino2m - 12 stages - TOFINO2M
// 5. Tofino2h -  8 stages - TOFINO2H
// 6. Tofino3  - 20 stages - TOFINO3

// Temporarily override the format checker so we can indent
// the nested preprocessor directives.
// clang-format off

#if __TARGET_TOFINO__ == 1
  #define TOFINO1
#elif __TARGET_TOFINO__ == 2
  #if __TOFINO2_VARIANT__ == 2
    #define TOFINO2M
  #elif __TOFINO2_VARIANT__ == 3
    #define TOFINO2H
  #else
    #define TOFINO2
    #define TOFINO2U
  #endif
#elif __TARGET_TOFINO__ == 3
  #define TOFINO3
#endif

#if defined(TOFINO2H)
  #define DIAG_STAGES 8
#elif defined(TOFINO1) || defined(TOFINO2M)
  #define DIAG_STAGES 12
#else
  #define DIAG_STAGES 20
#endif

#if defined(DIAG_POWER_MAX_ENABLE)
  #define DIAG_POWER_ENABLE
#endif

#if defined(DIAG_PHV_PARDE_HOLD_STRESS_ENABLE)
  #define DIAG_SINGLE_STAGE
  #define DIAG_PHV_STRESS_ENABLE
  #define DIAG_PARDE_STRESS_POWER
  #define DIAG_HOLD_STRESS
#endif

#if defined(DIAG_PHV_PARDE_STRESS_ENABLE)
  #define DIAG_SINGLE_STAGE
  #define DIAG_PHV_STRESS_ENABLE
  #define DIAG_PARDE_STRESS_POWER
#endif

#if defined(DIAG_PHV_FLOP_TEST)
  #if DIAG_PHV_FLOP_TEST == 1
    #define DIAG_PHV_FLOP_CONFIG_1
  #elif DIAG_PHV_FLOP_TEST == 2
    #define DIAG_PHV_FLOP_CONFIG_2
  #elif DIAG_PHV_FLOP_TEST == 3
    #define DIAG_PHV_FLOP_CONFIG_3
  #elif DIAG_PHV_FLOP_TEST == 4
    #define DIAG_PHV_FLOP_CONFIG_4
  #elif DIAG_PHV_FLOP_TEST == 5
    #define DIAG_PHV_FLOP_CONFIG_5
  #else
    #error "DIAG_PHV_FLOP_TEST must equal 1,2,3,4 or 5"
  #endif
#elif defined(DIAG_PHV_FLOP_MATCH)
  #define DIAG_PHV_FLOP_TEST
  #if DIAG_PHV_FLOP_MATCH == 1
    #define DIAG_PHV_FLOP_CONFIG_1
  #elif DIAG_PHV_FLOP_MATCH == 2
    #define DIAG_PHV_FLOP_CONFIG_2
  #else
    #error "DIAG_PHV_FLOP_MATCH must equal 1 or 2"
  #endif
#endif

/* Special Ports */
#if defined(TOFINO3)
  #define PORT_INDEX_FLOOD 2047
#else
  #define PORT_INDEX_FLOOD 511
#endif

#if defined(TOFINO1)
  #define DIAG_CPU_PORT 320
  #define DIAG_CPU_PORT_1 192
#else
  #define DIAG_CPU_PORT 0
#endif

/* Resubmit types */
#define DIAG_RESUBMIT_TYPE_BASIC 3w1

/* Mirror types */
#if defined(TOFINO1)
  #define DIAG_MIRROR_TYPE_INGRESS 3w1
  #define DIAG_MIRROR_TYPE_EGRESS 3w2
  #define DIAG_DROP_MIRROR_SESSION_ID 10w0
#else
  #define DIAG_MIRROR_TYPE_INGRESS 4w1
  #define DIAG_MIRROR_TYPE_EGRESS 4w2
  #define DIAG_DROP_MIRROR_SESSION_ID 8w0
#endif

#if defined(DIAG_PARDE_STRAIN)
  #define DIAG_PARDE_STRAIN_HDR_VALID_ZERO_VAL 1w0
  #define DIAG_PARDE_STRAIN_HDR_VALID_ONE_VAL 1w1
  #define DIAG_PARDE_STRAIN_VALUE_1 0xaa
  #define DIAG_PARDE_STRAIN_VALUE_2 0xbb
  #define DIAG_PARDE_STRAIN_VALUE_3 0xcc
  #define DIAG_PARDE_STRAIN_VALUE_4 0xdd
  #define DIAG_PARDE_STRAIN_VALUE_5 0xee
  #define DIAG_PARDE_STRAIN_VALUE_6 0xff
  #define DIAG_PARDE_STRAIN_VALUE_7 0x12
  #define DIAG_PARDE_STRAIN_VALUE_8 0x34
  #define DIAG_PARDE_STRAIN_1_MASK 0x1
  #define DIAG_PARDE_STRAIN_2_MASK 0x2
  #define DIAG_PARDE_STRAIN_3_MASK 0x4
  #define DIAG_PARDE_STRAIN_4_MASK 0x8
  #define DIAG_PARDE_STRAIN_5_MASK 0x10
  #define DIAG_PARDE_STRAIN_6_MASK 0x20
  #define DIAG_PARDE_STRAIN_7_MASK 0x40
  #define DIAG_PARDE_STRAIN_8_MASK 0x80
#endif

// Reenable format checking.
// clang-format on

#define STR(x) #x

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
typedef bit<192> mau_stress_hdr_exm_key_len_t;
typedef bit<192> mau_stress_hdr_tcam_key_len_t;

typedef bit<16> ether_type_t;
const ether_type_t ETHERTYPE_IPV4 = 16w0x0800;
const ether_type_t ETHERTYPE_IPV6 = 16w0x86dd;
const ether_type_t ETHERTYPE_VLAN = 16w0x8100;
const ether_type_t ETHERTYPE_QINQ = 16w0x9100;
const ether_type_t ETHERTYPE_MPLS = 16w0x8847;
const ether_type_t ETHERTYPE_LLDP = 16w0x88cc;
const ether_type_t ETHERTYPE_LACP = 16w0x8809;
const ether_type_t ETHERTYPE_NSH = 16w0x894f;
const ether_type_t ETHERTYPE_ROCE = 16w0x8915;
const ether_type_t ETHERTYPE_FCOE = 16w0x8906;
const ether_type_t ETHERTYPE_ETHERNET = 16w0x6658;
const ether_type_t ETHERTYPE_ARP = 16w0x0806;
const ether_type_t ETHERTYPE_VNTAG = 16w0x8926;
const ether_type_t ETHERTYPE_TRILL = 16w0x22f3;

typedef bit<8> ip_protocol_t;
const ip_protocol_t IP_PROTOCOLS_ICMP = 1;
const ip_protocol_t IP_PROTOCOLS_IPV4 = 4;
const ip_protocol_t IP_PROTOCOLS_TCP = 6;
const ip_protocol_t IP_PROTOCOLS_UDP = 17;
const ip_protocol_t IP_PROTOCOLS_IPV6 = 41;
const ip_protocol_t IP_PROTOCOLS_GRE = 47;
const ip_protocol_t IP_PROTOCOLS_ICMPV6 = 58;
const ip_protocol_t IP_PROTOCOLS_EIGRP = 88;
const ip_protocol_t IP_PROTOCOLS_OSPF = 89;
const ip_protocol_t IP_PROTOCOLS_PIM = 103;
const ip_protocol_t IP_PROTOCOLS_VRRP = 112;
const ip_protocol_t IP_PROTOCOLS_MPLS = 137;

typedef bit<32> ingress_tunnel_type_t;
const ingress_tunnel_type_t INGRESS_TUNNEL_TYPE_GRE = 2;
const ingress_tunnel_type_t INGRESS_TUNNEL_TYPE_IP_IN_IP = 2;
const ingress_tunnel_type_t INGRESS_TUNNEL_TYPE_NVGRE = 5;
const ingress_tunnel_type_t INGRESS_TUNNEL_TYPE_MPLS = 6;

typedef bit<3> ctrl_prio_t;
const ctrl_prio_t CONTROL_TRAFFIC_PRIO_3 = 3;
const ctrl_prio_t CONTROL_TRAFFIC_PRIO_5 = 5;
const ctrl_prio_t CONTROL_TRAFFIC_PRIO_7 = 7;

#endif /* _DIAG_DEFINES_ */
