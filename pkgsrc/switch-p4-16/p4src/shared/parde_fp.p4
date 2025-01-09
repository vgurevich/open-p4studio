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


#include "headers.p4"
#include "types.p4"

//================================================================================
// Physical Ingress parser on External pipe (Stage 1 of logical ingress pipeline)
//================================================================================
parser SwitchIngressParser_0(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_local_metadata_t local_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
    Checksum() ipv4_checksum;
    Checksum() inner_ipv4_checksum;
    @name(".ingress_udp_port_vxlan")
    value_set<bit<16>>(1) udp_port_vxlan;
    @name(".ingress_cpu_port")
    value_set<switch_cpu_port_value_set_t>(1) cpu_port;

    state start {
        pkt.extract(ig_intr_md);
        local_md.ingress_port = ig_intr_md.ingress_port;
#if defined(PTP_ENABLE) || defined(INT_V2)
        local_md.ingress_timestamp = ig_intr_md.ingress_mac_tstamp;
#else
        local_md.ingress_timestamp = ig_intr_md.ingress_mac_tstamp[31:0];
#endif
        transition parse_port_metadata;
    }

    state parse_resubmit {
        // Parse resubmitted packet here.
        transition accept;
    }

    state parse_port_metadata {
        // Parse port metadata produced by ibuf
        switch_port_metadata_t port_md = port_metadata_unpack<switch_port_metadata_t>(pkt);
        local_md.ingress_port_lag_index = port_md.port_lag_index;
        local_md.ingress_port_lag_label = port_md.port_lag_label;
        transition parse_packet;
    }

    state parse_packet {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type, local_md.ingress_port) {
            cpu_port  : parse_cpu;
            (ETHERTYPE_IPV4, _) : parse_ipv4;
            (ETHERTYPE_ARP, _)  : parse_arp;
            (ETHERTYPE_IPV6, _) : parse_ipv6;
            (ETHERTYPE_VLAN, _) : parse_vlan;
            (ETHERTYPE_QINQ, _) : parse_vlan;
#ifdef MPLS_ENABLE
            (ETHERTYPE_MPLS, _) : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_cpu {
        pkt.extract(hdr.fabric);
        pkt.extract(hdr.cpu);
        local_md.bypass = hdr.cpu.reason_code;
        transition select(hdr.cpu.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_ARP  : parse_arp;
            ETHERTYPE_IPV6 : parse_ipv6;
            ETHERTYPE_VLAN : parse_vlan;
            ETHERTYPE_QINQ : parse_vlan;
#ifdef MPLS_ENABLE
            ETHERTYPE_MPLS : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        ipv4_checksum.add(hdr.ipv4);
        transition select(hdr.ipv4.ihl) {
            5 : parse_ipv4_no_options;
            6 : parse_ipv4_options;
            default : accept;
        }
    }

    state parse_ipv4_options {
        // Only a single 32-bit option (e.g. router alert) is supported.
        pkt.extract(hdr.ipv4_option);
        ipv4_checksum.add(hdr.ipv4_option);
        transition parse_ipv4_no_options;
    }

    state parse_ipv4_no_options {
        local_md.flags.ipv4_checksum_err = ipv4_checksum.verify();
        transition select(hdr.ipv4.protocol, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_ICMP, 0) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 0) : parse_igmp;
            (IP_PROTOCOLS_TCP, 0) : parse_tcp;
            (IP_PROTOCOLS_UDP, 0) : parse_ipv4_udp;
//            (IP_PROTOCOLS_GRE, 0) : parse_ip_gre;
            (IP_PROTOCOLS_IPV4, 0) : parse_ipinip;
            (IP_PROTOCOLS_IPV6, 0) : parse_ipv6inip;
            // Do NOT parse the next header if IP packet is fragmented.
            default : accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_ARP : parse_arp;
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_VLAN : parse_vlan;
            ETHERTYPE_IPV6 : parse_ipv6;
#ifdef MPLS_ENABLE
            ETHERTYPE_MPLS : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_ipv6 {
#ifdef IPV6_ENABLE
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_ICMPV6 : parse_icmp;
            IP_PROTOCOLS_TCP : parse_tcp;
            IP_PROTOCOLS_UDP : parse_ipv6_udp;
//            IP_PROTOCOLS_GRE : parse_ip_gre;
//            IP_PROTOCOLS_IPV4 : parse_ipinip;
//            IP_PROTOCOLS_IPV6 : parse_ipv6inip;
#ifdef SRV6_ENABLE
            IP_PROTOCOLS_ROUTING : parse_srh_base;
#endif /* SRV6_ENABLE */
            default : accept;
        }
#else
        transition accept;
#endif
    }

#ifdef MPLS_ENABLE
    state parse_mpls {
        pkt.extract(hdr.mpls.next);
        transition select(hdr.mpls.last.bos) {
            0 : parse_mpls;
            1 : parse_mpls_bos;
        }
    }

    state parse_mpls_bos {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_MPLS;
        transition select(pkt.lookahead<bit<4>>()) {
            0x4 : parse_inner_ipv4;
            0x6 : parse_inner_ipv6;
            default : accept;
        }
    }
#endif /* MPLS_ENABLE */

#ifdef SRV6_ENABLE
    state parse_srh_base {
        pkt.extract(hdr.srh_base);
        local_md.tunnel.srh_next_hdr = hdr.srh_base.next_hdr;
        transition select(hdr.srh_base.last_entry, hdr.srh_base.seg_left) {
            (0, 1) : parse_srh_active_and_done_0; // active and the last segment in the segment list
            (0, _) : parse_srh_done_0;            // last segment
            (_, 1) : parse_srh_active_and_continue_0; // active segment
            default : parse_srh_continue_0;           // continue parsing
        }
    }

    state parse_srh_active_and_done_0 {
        pkt.extract(hdr.srh_seg_list[0]);
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[0].sid;
        transition parse_srh_next_hdr;
    }

    state parse_srh_done_0 {
        pkt.extract(hdr.srh_seg_list[0]);
        // local_md.tunnel.srh_next_sid is not populated since ip_da is assumed to be carrying the last segment
        transition parse_srh_next_hdr;
    }

    state parse_srh_active_and_continue_0 {
        pkt.extract(hdr.srh_seg_list[0]);
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[0].sid;
        transition parse_srh_segment_1;
    }

    state parse_srh_continue_0 {
        pkt.extract(hdr.srh_seg_list[0]);
        transition parse_srh_segment_1;
    }

    state parse_srh_segment_1 {
        transition select(hdr.srh_base.last_entry, hdr.srh_base.seg_left) {
            (1, 2) : parse_srh_active_and_done_1; // active and the last segment in the segment list
            (1, _) : parse_srh_done_1;            // last segment
            (0, _) : parse_srh_done_1;            // last segment
            default : accept;
        }
    }

    state parse_srh_active_and_done_1 {
        pkt.extract(hdr.srh_seg_list[1]);
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[1].sid;
        transition parse_srh_next_hdr;
    }

    state parse_srh_done_1 {
        pkt.extract(hdr.srh_seg_list[1]);
        transition parse_srh_next_hdr;
    }

    #define PARSE_SRH_SEGMENT(curr, next) \
    state parse_srh_segment_##curr { \
        transition select(hdr.srh_base.last_entry, hdr.srh_base.seg_left) { \
            (##curr, ##next) : parse_srh_active_and_done_##curr; \
            (##curr, _) : parse_srh_done_##curr; \
            (_, 1) : parse_srh_active_and_continue_##curr; \
            default : parse_srh_continue_##curr; \
        } \
    } \
    state parse_srh_active_and_done_##curr { \
        pkt.extract(hdr.srh_seg_list[##curr]); \
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[##curr].sid; \
        transition parse_srh_next_hdr; \
    } \
    state parse_srh_done_##curr { \
        pkt.extract(hdr.srh_seg_list[##curr]);\
        transition parse_srh_next_hdr; \
    } \
    state parse_srh_active_and_continue_##curr { \
        pkt.extract(hdr.srh_seg_list[##curr]); \
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[##curr].sid; \
        transition parse_srh_segment_##next; \
    } \
    state parse_srh_continue_##curr { \
        pkt.extract(hdr.srh_seg_list[##curr]); \
        local_md.tunnel.srh_next_sid = hdr.srh_seg_list[##curr].sid; \
        transition parse_srh_segment_##next; \
    } \
    /*
    PARSE_SRH_SEGMENT(1,2)
    PARSE_SRH_SEGMENT(2,3)
    */
    state parse_srh_next_hdr {
        transition select(hdr.srh_base.next_hdr) {
            IP_PROTOCOLS_IPV6 : parse_inner_ipv6;
            IP_PROTOCOLS_IPV4 : parse_inner_ipv4;
//            IP_PROTOCOLS_NONXT : parse_inner_ethernet;
            default: accept;
        }
    }
#endif /* SRV6_ENABLE */


#ifdef NVGRE_ENABLE
    // With outer IP as V4/V6
    // nvgre key is 32 bits i.e. combination of both vsid[24 bit], flow_id [8 bit]
    // nvgre-st format  - ipv4/ipv6 -> gre -> nvgre[key==config-key] -> ipv4/ipv6
    // nvgre format     - ipv4/ipv6 -> gre -> nvgre[key!=config-key] -> ipv4/ipv6
    state parse_ip_nvgre {
        pkt.extract(hdr.nvgre);
        local_md.tunnel.vni = hdr.nvgre.vsid_flowid[31:8];
        local_md.tunnel.nvgre_flow_id = hdr.nvgre.vsid_flowid[7:0];

        // nvgre-st will have config key value or 0x6400
        transition select(hdr.nvgre.vsid_flowid) {
            nvgre_st_key : parse_nvgre_st;     // nvgre-st
            default : parse_nvgre;             // nvgre
        }
    }

    // nvgre format - ipv4/ipv6 -> gre -> nvgre[key!=config-key] -> ipv4/ipv6
    state parse_nvgre {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_NVGRE;
        transition parse_inner_ethernet;
    }

    // nvgre-st format - ipv4/ipv6 -> gre -> nvgre[key=config-key] -> ipv4/ipv6
    state parse_nvgre_st {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_NVGRE_ST;
        transition parse_inner_ethernet;
    }
#endif

//    state parse_ip_gre {
//        pkt.extract(hdr.gre);
//        transition select(hdr.gre.flags_version, hdr.gre.proto) {
//#ifdef NVGRE_ENABLE
//            (0x2000, GRE_PROTOCOLS_NVGRE) : parse_ip_nvgre;
//#endif
//            //(_, GRE_PROTOCOLS_IP) : parse_inner_ipv4;
//            //(_, GRE_PROTOCOLS_IPV6) : parse_inner_ipv6;
//            default : accept;
//        }
//    }

    state parse_ipv4_udp {
        pkt.extract(hdr.udp);
        local_md.lkp.l4_src_port = hdr.udp.src_port;
        local_md.lkp.l4_dst_port = hdr.udp.dst_port;
        transition select(hdr.udp.dst_port) {
#ifdef VXLAN_ENABLE
            udp_port_vxlan : parse_vxlan;
#endif
            default : accept;
        }
    }

    state parse_ipv6_udp {
        pkt.extract(hdr.udp);
        local_md.lkp.l4_src_port = hdr.udp.src_port;
        local_md.lkp.l4_dst_port = hdr.udp.dst_port;
	transition accept;
    }

    state parse_tcp {
        tcp_h tcp_md = pkt.lookahead<tcp_h>();
        local_md.lkp.l4_src_port = tcp_md.src_port;
        local_md.lkp.l4_dst_port = tcp_md.dst_port;
        local_md.lkp.tcp_flags = tcp_md.flags;
        transition accept;
    }

    state parse_icmp {
        icmp_h icmp_md = pkt.lookahead<icmp_h>();
        local_md.lkp.l4_src_port[7:0]  = icmp_md.type;
        local_md.lkp.l4_src_port[15:8] = icmp_md.code;
        transition accept;
    }

    state parse_igmp {
        igmp_h igmp_md = pkt.lookahead<igmp_h>();
        local_md.lkp.l4_src_port[7:0]  = igmp_md.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        transition accept;
    }

    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_VXLAN;
        local_md.tunnel.vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }

    state parse_ipinip {
#ifdef IPINIP_ENABLE
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv4;
#else
        transition accept;
#endif
    }

    state parse_ipv6inip {
#if defined(IPINIP_ENABLE)
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv6;
#else
        transition accept;
#endif
    }

    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_inner_ipv4;
            ETHERTYPE_IPV6 : parse_inner_ipv6;
            default : accept;
        }
    }

    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        inner_ipv4_checksum.add(hdr.inner_ipv4);
        local_md.flags.inner_ipv4_checksum_err = inner_ipv4_checksum.verify();
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            IP_PROTOCOLS_ICMP : parse_inner_icmp;
            default : accept;
        }
    }

    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            IP_PROTOCOLS_ICMPV6 : parse_inner_icmp;
            default : accept;
        }
    }

    state parse_inner_udp {
        udp_h inner_udp_md = pkt.lookahead<udp_h>();
        local_md.lkp.inner_l4_src_port = inner_udp_md.src_port;
        local_md.lkp.inner_l4_dst_port = inner_udp_md.dst_port;
        transition accept;
    }

    state parse_inner_tcp {
        tcp_h inner_tcp_md = pkt.lookahead<tcp_h>();
        local_md.lkp.inner_l4_src_port = inner_tcp_md.src_port;
        local_md.lkp.inner_l4_dst_port = inner_tcp_md.dst_port;
        local_md.lkp.inner_tcp_flags = inner_tcp_md.flags;
        transition accept;
    }

    state parse_inner_icmp {
        icmp_h inner_icmp_md = pkt.lookahead<icmp_h>();
        local_md.lkp.inner_l4_src_port[7:0] = inner_icmp_md.type;
        local_md.lkp.inner_l4_dst_port[15:8] = inner_icmp_md.code;
        transition accept;
    }

}

//=================================================================================
// Physical Egress parser on internal pipe (2nd stage of logical ingress pipeline)
//=================================================================================
parser SwitchEgressParser_1(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_local_metadata_t local_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    @name(".egress1_udp_port_vxlan")
    value_set<bit<16>>(1) udp_port_vxlan;
    @name(".internal_pipe_cpu_port")
    value_set<switch_cpu_port_value_set_t>(1) recirc_port_cpu_hdr;

    state start {
        pkt.extract(eg_intr_md);
        local_md.ingress_port = eg_intr_md.egress_port;
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.fp_bridged_md);
        local_md.flags.rmac_hit = hdr.fp_bridged_md.rmac_hit;
        transition select(hdr.ethernet.ether_type, eg_intr_md.egress_port) {
            recirc_port_cpu_hdr  : parse_cpu;
            (ETHERTYPE_IPV4, _) : parse_ipv4;
            (ETHERTYPE_ARP, _)  : parse_arp;
            (ETHERTYPE_IPV6, _) : parse_ipv6;
            (ETHERTYPE_VLAN, _) : parse_vlan;
#ifdef QINQ_ENABLE
	    (ETHERTYPE_QINQ, _) : parse_vlan;
#endif
            default : accept;
        }
    }

    state parse_cpu {
        pkt.extract(hdr.fabric);
        pkt.extract(hdr.cpu);
        local_md.bypass = hdr.cpu.reason_code;
        transition select(hdr.cpu.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4_no_tunnel;
            ETHERTYPE_ARP  : parse_arp;
            ETHERTYPE_IPV6 : parse_ipv6;
            ETHERTYPE_VLAN : parse_vlan_no_tunnel;
#ifdef QINQ_ENABLE
            ETHERTYPE_QINQ : parse_vlan_no_tunnel;
#endif
#ifdef MPLS_ENABLE
            ETHERTYPE_MPLS : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_ARP : parse_arp;
            ETHERTYPE_IPV4 : parse_ipv4;
#ifdef QINQ_ENABLE
            ETHERTYPE_VLAN : parse_vlan;
#endif
            ETHERTYPE_IPV6 : parse_ipv6;
            default : accept;
        }
    }

    state parse_vlan_no_tunnel {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_ARP : parse_arp;
            ETHERTYPE_IPV4 : parse_ipv4_no_tunnel;
#ifdef QINQ_ENABLE
            ETHERTYPE_VLAN : parse_vlan;
#endif
            ETHERTYPE_IPV6 : parse_ipv6;
            default : accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol, hdr.ipv4.ihl, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5, 0) : parse_tcp;
            (IP_PROTOCOLS_UDP, 5, 0) : parse_udp_vxlan;
            (IP_PROTOCOLS_ICMP, 5, 0) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5, 0) : parse_igmp;
#if defined(IPINIP_ENABLE)
            (IP_PROTOCOLS_IPV4, 5, 0) : parse_inner_ipv4;
            (IP_PROTOCOLS_IPV6, 5, 0) : parse_inner_ipv6;
#endif
            (_, 6, _) : parse_ipv4_options;
            default : accept;
        }
    }

    state parse_ipv4_no_tunnel {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol, hdr.ipv4.ihl, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5, 0) : parse_tcp;
            (IP_PROTOCOLS_UDP, 5, 0) : parse_udp_no_vxlan;
            (IP_PROTOCOLS_ICMP, 5, 0) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5, 0) : parse_igmp;
            (_, 6, _) : parse_ipv4_options;
            default : accept;
        }
    }

    state parse_ipv4_options {
        pkt.extract(hdr.ipv4_option);
        transition select(hdr.ipv4.protocol) {
            (IP_PROTOCOLS_TCP)  : parse_tcp;
            (IP_PROTOCOLS_UDP)  : parse_udp_no_vxlan;
            (IP_PROTOCOLS_ICMP) : parse_icmp;
            (IP_PROTOCOLS_IGMP) : parse_igmp;
            default : accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_ICMPV6 : parse_icmp;
            IP_PROTOCOLS_TCP : parse_tcp;
            IP_PROTOCOLS_UDP : parse_udp_no_vxlan;
//            IP_PROTOCOLS_GRE : parse_ip_gre;
//            IP_PROTOCOLS_IPV4 : parse_ipinip;
//            IP_PROTOCOLS_IPV6 : parse_ipv6inip;
            default : accept;
        }
    }

//    state parse_ip_gre {
//        pkt.extract(hdr.gre);
//        transition select(hdr.gre.flags_version, hdr.gre.proto) {
//            (_, GRE_PROTOCOLS_IP) : parse_inner_ipv4;
//            (_, GRE_PROTOCOLS_IPV6) : parse_inner_ipv6;
//            default : accept;
//        }
//    }

    state parse_udp_vxlan {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
            udp_port_vxlan : parse_vxlan;
            default : accept;
        }
    }

    state parse_udp_no_vxlan {
        pkt.extract(hdr.udp);
        transition accept;
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
        transition accept;
    }

    state parse_icmp {
        pkt.extract(hdr.icmp);
        transition accept;
    }

    state parse_igmp {
        pkt.extract(hdr.igmp);
        transition accept;
    }

    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_VXLAN;
        local_md.tunnel.vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }

    state parse_ipinip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv4;
    }

    state parse_ipv6inip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv6;
    }

    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_inner_ipv4;
            ETHERTYPE_IPV6 : parse_inner_ipv6;
            default : accept;
        }
    }

    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_ICMP : parse_inner_icmp;
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            IP_PROTOCOLS_IGMP : parse_inner_igmp;
            default : accept;
        }
    }

    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            IP_PROTOCOLS_ICMPV6 : parse_inner_icmp;
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            default : accept;
        }
    }

    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition accept;
    }

    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }

    state parse_inner_icmp {
        pkt.extract(hdr.inner_icmp);
        transition accept;
    }

    state parse_inner_igmp {
        pkt.extract(hdr.inner_igmp);
        transition accept;
    }

}

//=========================================================================================
// Physical Ingress parser on the internal pipe (3rd stage of the logical ingress pipeline)
//=========================================================================================
parser SwitchIngressParser_1(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_local_metadata_t local_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {
#ifdef NAT_ENABLE
    Checksum() tcp_checksum;
    Checksum() udp_checksum;
#endif
    @name(".ingress1_udp_port_vxlan")
    value_set<bit<16>>(1) udp_port_vxlan;
    @name(".ingress_pipe_cpu_port")
    value_set<switch_cpu_port_value_set_t>(1) recirc_port_cpu_hdr;

    state start {
        pkt.extract(ig_intr_md);
        local_md.ingress_port = ig_intr_md.ingress_port;
        transition parse_port_metadata;
    }

    state parse_port_metadata {
        // Parse port metadata produced by ibuf
        switch_fp_port_metadata_t port_md = port_metadata_unpack<switch_fp_port_metadata_t>(pkt);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
	    ETHERTYPE_PFC : parse_pfc;
            default : parse_fp_bridged_md;
        }
    }

    state parse_pfc {
	local_md.bypass = SWITCH_INGRESS_BYPASS_ALL;
        transition accept;
    }

    state parse_fp_bridged_md {
        pkt.extract(hdr.fp_bridged_md);
        local_md.bd = hdr.fp_bridged_md.ingress_bd_tt_myip[12:0];
        local_md.lkp.pkt_type = hdr.fp_bridged_md.pkt_type;
        local_md.flags.routed = hdr.fp_bridged_md.routed;

        local_md.ingress_timestamp = hdr.fp_bridged_md.timestamp;
        local_md.qos.tc = hdr.fp_bridged_md.tc;
        local_md.qos.qid = hdr.fp_bridged_md.qid_icos[4:0];
        local_md.qos.color = hdr.fp_bridged_md.color;
        local_md.qos.icos = hdr.fp_bridged_md.qid_icos[7:5];
        local_md.tunnel.terminate = (bool)hdr.fp_bridged_md.ingress_bd_tt_myip[13:13];

        local_md.flags.ipv4_checksum_err = hdr.fp_bridged_md.ipv4_checksum_err;
        local_md.flags.rmac_hit = hdr.fp_bridged_md.rmac_hit;
        local_md.flags.fib_drop = hdr.fp_bridged_md.fib_drop;
        local_md.flags.fib_lpm_miss = hdr.fp_bridged_md.fib_lpm_miss;
        local_md.multicast.hit = hdr.fp_bridged_md.multicast_hit;
        local_md.flags.acl_deny = hdr.fp_bridged_md.acl_deny;
        local_md.flags.copy_cancel = hdr.fp_bridged_md.copy_cancel;
        local_md.nat.nat_disable = hdr.fp_bridged_md.nat_disable;
        local_md.flags.myip = hdr.fp_bridged_md.ingress_bd_tt_myip[15:14];
        local_md.drop_reason = hdr.fp_bridged_md.drop_reason;
        local_md.hostif_trap_id = hdr.fp_bridged_md.hostif_trap_id;
        local_md.mirror.session_id = hdr.fp_bridged_md.mirror_session_id;

        transition select(hdr.ethernet.ether_type, ig_intr_md.ingress_port) {
            recirc_port_cpu_hdr : parse_cpu;
            (ETHERTYPE_IPV4, _) : parse_ipv4;
            (ETHERTYPE_ARP, _)  : parse_arp;
            (ETHERTYPE_IPV6, _) : parse_ipv6;
            (ETHERTYPE_VLAN, _) : parse_vlan;
            (ETHERTYPE_QINQ, _) : parse_vlan;
            default : accept;
        }
    }

    state parse_cpu {
        pkt.extract(hdr.fabric);
        pkt.extract(hdr.cpu);
        local_md.bypass = hdr.cpu.reason_code;
#if defined(PTP_ENABLE)
        local_md.flags.capture_ts = (bool) hdr.cpu.capture_ts;
#endif
        transition select(hdr.cpu.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_ARP  : parse_arp;
            ETHERTYPE_IPV6 : parse_ipv6;
            ETHERTYPE_VLAN : parse_vlan;
            ETHERTYPE_QINQ : parse_vlan;
#ifdef MPLS_ENABLE
            ETHERTYPE_MPLS : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
#ifdef NAT_ENABLE
        tcp_checksum.subtract({hdr.ipv4.src_addr,hdr.ipv4.dst_addr});
        udp_checksum.subtract({hdr.ipv4.src_addr,hdr.ipv4.dst_addr});
#endif
        transition select(hdr.ipv4.protocol, hdr.ipv4.ihl, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5, 0) : parse_tcp;
            (IP_PROTOCOLS_UDP, 5, 0) : parse_ipv4_udp;
            (IP_PROTOCOLS_ICMP, 5, 0) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5, 0) : parse_igmp;
#if defined(IPINIP_ENABLE)
            (IP_PROTOCOLS_IPV4, 5, 0) : parse_inner_ipv4;
            (IP_PROTOCOLS_IPV6, 5, 0) : parse_inner_ipv6;
#endif
            (_, 6, _) : parse_ipv4_options;
            default : accept;
        }
    }

    state parse_ipv4_options {
        // Only a single 32-bit option (e.g. router alert) is supported.
        pkt.extract(hdr.ipv4_option);
        transition select(hdr.ipv4.protocol, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5)  : parse_tcp;
            (IP_PROTOCOLS_UDP, 5)  : parse_ipv4_udp;
            (IP_PROTOCOLS_ICMP, 5) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5) : parse_igmp;
            default : accept;
        }
    }

    state parse_arp {
        pkt.extract(hdr.arp);
        transition accept;
    }

    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_ARP : parse_arp;
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_VLAN : parse_vlan;
            ETHERTYPE_IPV6 : parse_ipv6;
            default : accept;
        }
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
#ifdef NAT_ENABLE
        tcp_checksum.subtract({hdr.ipv6.src_addr,hdr.ipv6.dst_addr});
        udp_checksum.subtract({hdr.ipv6.src_addr,hdr.ipv6.dst_addr});
#endif
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_ICMPV6 : parse_icmp;
            IP_PROTOCOLS_TCP : parse_tcp;
            IP_PROTOCOLS_UDP : parse_ipv6_udp;
//            IP_PROTOCOLS_GRE : parse_ip_gre;
//            IP_PROTOCOLS_IPV4 : parse_ipinip;
//            IP_PROTOCOLS_IPV6 : parse_ipv6inip;
            default : accept;
        }
    }

//    state parse_ip_gre {
//        pkt.extract(hdr.gre);
//        transition select(hdr.gre.flags_version, hdr.gre.proto) {
//            (_, GRE_PROTOCOLS_IP) : parse_inner_ipv4;
//            (_, GRE_PROTOCOLS_IPV6) : parse_inner_ipv6;
//            default : accept;
//        }
//    }

    state parse_ipv4_udp {
        pkt.extract(hdr.udp);
#ifdef NAT_ENABLE
        udp_checksum.subtract_all_and_deposit(local_md.tcp_udp_checksum);
        udp_checksum.subtract({hdr.udp.checksum});
        udp_checksum.subtract({hdr.udp.src_port, hdr.udp.dst_port});
#endif
        transition select(hdr.udp.dst_port) {
            udp_port_vxlan : parse_vxlan;
            default : accept;
        }
    }

    state parse_ipv6_udp {
        pkt.extract(hdr.udp);
#ifdef NAT_ENABLE
        udp_checksum.subtract_all_and_deposit(local_md.tcp_udp_checksum);
        udp_checksum.subtract({hdr.udp.checksum});
        udp_checksum.subtract({hdr.udp.src_port, hdr.udp.dst_port});
#endif
	transition accept;
    }

    state parse_tcp {
        pkt.extract(hdr.tcp);
#ifdef NAT_ENABLE
        tcp_checksum.subtract_all_and_deposit(local_md.tcp_udp_checksum);
        tcp_checksum.subtract({hdr.tcp.checksum});
        tcp_checksum.subtract({hdr.tcp.src_port, hdr.tcp.dst_port});
#endif
        transition accept;
    }

    state parse_icmp {
        pkt.extract(hdr.icmp);
        transition accept;
    }

    state parse_igmp {
        pkt.extract(hdr.igmp);
        transition accept;
    }

    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_VXLAN;
        local_md.tunnel.vni = hdr.vxlan.vni;
        transition parse_inner_ethernet;
    }

    state parse_ipinip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv4;
    }

    state parse_ipv6inip {
        local_md.tunnel.type = SWITCH_INGRESS_TUNNEL_TYPE_IPINIP;
        transition parse_inner_ipv6;
    }

    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_inner_ipv4;
            ETHERTYPE_IPV6 : parse_inner_ipv6;
            default : accept;
        }
    }

    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_ICMP : parse_inner_icmp;
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            IP_PROTOCOLS_IGMP : parse_inner_igmp;
            default : accept;
        }
    }

    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            IP_PROTOCOLS_ICMPV6 : parse_inner_icmp;
            IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            default : accept;
        }
    }

    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition accept;
    }

    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }

    state parse_inner_icmp {
        pkt.extract(hdr.inner_icmp);
        transition accept;
    }

    state parse_inner_igmp {
        pkt.extract(hdr.inner_igmp);
        transition accept;
    }

}

//============================================================================
// Physical Egress parser on the external pipe (Also logical external pipe)
//============================================================================
parser SwitchEgressParser_0(
        packet_in pkt,
        out switch_header_t hdr,
        out switch_local_metadata_t local_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    @name(".egress_udp_port_vxlan")
    value_set<bit<16>>(1) udp_port_vxlan;
    @name(".egress_cpu_port")
    value_set<switch_cpu_port_value_set_t>(1) cpu_port;
    @name(".egress_nvgre_st_key")
    value_set<switch_nvgre_value_set_t>(1) nvgre_st_key;

    @critical
    state start {
        pkt.extract(eg_intr_md);
        local_md.pkt_length = eg_intr_md.pkt_length;
        local_md.egress_port = eg_intr_md.egress_port;
        local_md.qos.qdepth = eg_intr_md.deq_qdepth;

        switch_port_mirror_metadata_h mirror_md = pkt.lookahead<switch_port_mirror_metadata_h>();
        transition select(eg_intr_md.deflection_flag, mirror_md.src, mirror_md.type) {
#ifdef DTEL_ENABLE
            (1, _, _) : parse_deflected_pkt;
#endif
            (_, SWITCH_PKT_SRC_BRIDGED, _) : parse_bridged_pkt;
            (_, SWITCH_PKT_SRC_CLONED_INGRESS, SWITCH_MIRROR_TYPE_PORT) : parse_port_mirrored_with_fp_bridged_metadata;
            (_, SWITCH_PKT_SRC_CLONED_EGRESS, SWITCH_MIRROR_TYPE_PORT) : parse_port_mirrored_metadata;
            (_, SWITCH_PKT_SRC_CLONED_EGRESS, SWITCH_MIRROR_TYPE_CPU) : parse_cpu_mirrored_metadata;
#ifdef DTEL_ENABLE
            (_, SWITCH_PKT_SRC_CLONED_INGRESS, SWITCH_MIRROR_TYPE_DTEL_DROP) : parse_dtel_drop_metadata_from_ingress;
            (_, _, SWITCH_MIRROR_TYPE_DTEL_DROP) : parse_dtel_drop_metadata_from_egress;
            (_, _, SWITCH_MIRROR_TYPE_DTEL_SWITCH_LOCAL) : parse_dtel_switch_local_metadata;
            (_, _, SWITCH_MIRROR_TYPE_SIMPLE) : parse_simple_mirrored_metadata;
#endif
        }
    }

    state parse_bridged_pkt {
        pkt.extract(hdr.bridged_md);
        local_md.pkt_src = SWITCH_PKT_SRC_BRIDGED;
        local_md.ingress_port = hdr.bridged_md.base.ingress_port;
        local_md.egress_port_lag_index = hdr.bridged_md.base.ingress_port_lag_index;
        local_md.bd = hdr.bridged_md.base.ingress_bd;
        local_md.nexthop = hdr.bridged_md.base.nexthop;

        local_md.cpu_reason = hdr.bridged_md.base.cpu_reason;
        local_md.flags.routed = hdr.bridged_md.base.routed;
        local_md.flags.bypass_egress = hdr.bridged_md.base.bypass_egress;

#if defined(PTP_ENABLE)
        local_md.flags.capture_ts = hdr.bridged_md.base.capture_ts;
#endif
        local_md.lkp.pkt_type = hdr.bridged_md.base.pkt_type;
        local_md.ingress_timestamp = hdr.bridged_md.base.timestamp;
        local_md.qos.tc = hdr.bridged_md.base.tc;
        local_md.qos.qid = hdr.bridged_md.base.qid;
        local_md.qos.color = hdr.bridged_md.base.color;
        local_md.vrf = hdr.bridged_md.base.vrf;

#ifdef TUNNEL_ENABLE
        local_md.tunnel_nexthop = hdr.bridged_md.tunnel.tunnel_nexthop;
        local_md.tunnel.type = 0;
#ifdef VXLAN_ENABLE
        local_md.tunnel.hash = hdr.bridged_md.tunnel.hash;
#endif
#ifdef MPLS_ENABLE
        local_md.tunnel.mpls_pop_count = hdr.bridged_md.tunnel.mpls_pop_count;
#endif
        local_md.tunnel.terminate = hdr.bridged_md.tunnel.terminate;
#endif /* TUNNEL_ENABLE */
#ifdef DTEL_ENABLE
        local_md.dtel.report_type = hdr.bridged_md.dtel.report_type;
        local_md.dtel.hash = hdr.bridged_md.dtel.hash;
        local_md.dtel.session_id = hdr.bridged_md.dtel.session_id;
#endif
        transition parse_ethernet;
    }

#ifdef DTEL_ENABLE
    state parse_deflected_pkt {
        pkt.extract(hdr.bridged_md);
        local_md.pkt_src = SWITCH_PKT_SRC_DEFLECTED;
#ifdef PACKET_LENGTH_ADJUSTMENT
        local_md.mirror.type = SWITCH_MIRROR_TYPE_DTEL_DEFLECT;
#endif
        local_md.dtel.report_type = hdr.bridged_md.dtel.report_type;
        local_md.dtel.hash = hdr.bridged_md.dtel.hash;
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
        local_md.mirror.session_id = hdr.bridged_md.dtel.session_id;
        local_md.qos.qid = hdr.bridged_md.base.qid;
#ifdef INT_V2
        hdr.dtel_metadata_1 = {
            0,
            hdr.bridged_md.base.ingress_port,
            0,
            hdr.bridged_md.dtel.egress_port};
        hdr.dtel_metadata_4 = {
            0,
            hdr.bridged_md.base.timestamp};
        hdr.dtel_drop_report = {
            0,
            hdr.bridged_md.base.qid,
            SWITCH_DROP_REASON_TRAFFIC_MANAGER,
            0};
#else
        local_md.ingress_timestamp = hdr.bridged_md.base.timestamp;
        hdr.dtel_report = {
            0,
            hdr.bridged_md.base.ingress_port,
            0,
            hdr.bridged_md.dtel.egress_port,
            0,
            hdr.bridged_md.base.qid};
        hdr.dtel_drop_report = {
            SWITCH_DROP_REASON_TRAFFIC_MANAGER,
            0};
#endif /* INT_V2 */
        transition accept;
    }
#endif /* DTEL_ENABLE */

    state parse_port_mirrored_with_fp_bridged_metadata {
        switch_port_mirror_metadata_h port_md;
        pkt.extract(port_md);
        // pkt is mirrored on internal pipe,
        // so extract internal headers before parsing pkt
        pkt.extract(hdr.ethernet);
        pkt.advance(sizeInBytes(hdr.fp_bridged_md) * 8);
        local_md.pkt_src = port_md.src;
        local_md.mirror.session_id = port_md.session_id;
        local_md.ingress_timestamp = port_md.timestamp;
        local_md.flags.bypass_egress = true;
#ifdef PACKET_LENGTH_ADJUSTMENT
        local_md.mirror.type = port_md.type;
#endif
#ifdef DTEL_ENABLE
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
#endif
        transition accept;
    }

state parse_port_mirrored_metadata {
        switch_port_mirror_metadata_h port_md;
        pkt.extract(port_md);
        pkt.extract(hdr.ethernet);
        local_md.pkt_src = port_md.src;
        local_md.mirror.session_id = port_md.session_id;
        local_md.ingress_timestamp = port_md.timestamp;
        local_md.flags.bypass_egress = true;
#ifdef PACKET_LENGTH_ADJUSTMENT
        local_md.mirror.type = port_md.type;
#endif
#ifdef DTEL_ENABLE
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
#endif
        transition accept;
    }


    state parse_cpu_mirrored_metadata {
        switch_cpu_mirror_metadata_h cpu_md;
        pkt.extract(cpu_md);
        pkt.extract(hdr.ethernet);
        local_md.pkt_src = cpu_md.src;
        local_md.flags.bypass_egress = true;
        local_md.bd = cpu_md.bd;
        // local_md.ingress_port = cpu_md.md.port;
        local_md.cpu_reason = cpu_md.reason_code;
#ifdef PACKET_LENGTH_ADJUSTMENT
        local_md.mirror.type = cpu_md.type;
#endif
#ifdef DTEL_ENABLE
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
#endif
        transition accept;
    }

#ifdef DTEL_ENABLE
    state parse_dtel_drop_metadata_from_egress {
        switch_dtel_drop_mirror_metadata_h dtel_md;
        pkt.extract(dtel_md);
        local_md.pkt_src = dtel_md.src;
        local_md.mirror.type = dtel_md.type;
        local_md.dtel.report_type = dtel_md.report_type;
        local_md.dtel.hash = dtel_md.hash;
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
        local_md.mirror.session_id = dtel_md.session_id;
#ifdef INT_V2
        hdr.dtel_metadata_1 = {
            0,
            dtel_md.ingress_port,
            0,
            dtel_md.egress_port};
        hdr.dtel_metadata_4 = {
            0,
            dtel_md.timestamp};
        hdr.dtel_drop_report = {
            0,
            dtel_md.qid,
            dtel_md.drop_reason,
            0};
#else
        local_md.ingress_timestamp = dtel_md.timestamp;
        hdr.dtel_report = {
            0,
            dtel_md.ingress_port,
            0,
            dtel_md.egress_port,
            0,
            dtel_md.qid};
        hdr.dtel_drop_report = {
            dtel_md.drop_reason,
            0};
#endif /* INT_V2 */
        transition accept;
    }

    /* Separate parse state for drop metadata from ingress, in order to set
     * hdr.dtel_report.egress_port to SWITCH_PORT_INVALID */
    state parse_dtel_drop_metadata_from_ingress {
        switch_dtel_drop_mirror_metadata_h dtel_md;
        pkt.extract(dtel_md);
        local_md.pkt_src = dtel_md.src;
        local_md.mirror.type = dtel_md.type;
        local_md.dtel.report_type = dtel_md.report_type;
        local_md.dtel.hash = dtel_md.hash;
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
        local_md.mirror.session_id = dtel_md.session_id;
#ifdef INT_V2
        hdr.dtel_metadata_1 = {
            0,
            dtel_md.ingress_port,
            0,
            SWITCH_PORT_INVALID};
        hdr.dtel_metadata_4 = {
            0,
            dtel_md.timestamp};
        hdr.dtel_drop_report = {
            0,
            dtel_md.qid,
            dtel_md.drop_reason,
            0};
#else
        local_md.ingress_timestamp = dtel_md.timestamp;
        hdr.dtel_report = {
            0,
            dtel_md.ingress_port,
            0,
            SWITCH_PORT_INVALID,
            0,
            dtel_md.qid};
        hdr.dtel_drop_report = {
            dtel_md.drop_reason,
            0};
#endif /* INT_V2 */
        transition accept;
    }

    state parse_dtel_switch_local_metadata {
        switch_dtel_switch_local_mirror_metadata_h dtel_md;
        pkt.extract(dtel_md);
        local_md.pkt_src = dtel_md.src;
        local_md.mirror.type = dtel_md.type;
        local_md.dtel.report_type = dtel_md.report_type;
        local_md.dtel.hash = dtel_md.hash;
        // Initialize local_md.dtel.session_id to prevent it from being marked @pa_no_init.
        local_md.dtel.session_id = 0;
        local_md.mirror.session_id = dtel_md.session_id;
#ifdef INT_V2
        hdr.dtel_metadata_1 = {
            0,
            dtel_md.ingress_port,
            0,
            dtel_md.egress_port};
        //TODO: Preserve latency as well as quantized latency
        //      and add to mirror metadata
        //hdr.dtel_metadata_2 = {
        //    dtel_md.latency};
        hdr.dtel_metadata_3 = {
            0,
            dtel_md.qid,
            0,
            dtel_md.qdepth};
        hdr.dtel_metadata_4 = {
            0,
            dtel_md.timestamp};
        hdr.dtel_metadata_5 = {
            0,
            dtel_md.egress_timestamp};
#else
        local_md.ingress_timestamp = dtel_md.timestamp;
        hdr.dtel_report = {
            0,
            dtel_md.ingress_port,
            0,
            dtel_md.egress_port,
            0,
            dtel_md.qid};
        hdr.dtel_switch_local_report = {
            0,
            dtel_md.qdepth,
            dtel_md.egress_timestamp[31:0]};
#endif /* INT_V2 */
        transition accept;
    }

    state parse_simple_mirrored_metadata {
        switch_simple_mirror_metadata_h simple_mirror_md;
        pkt.extract(simple_mirror_md);
        local_md.pkt_src = simple_mirror_md.src;
        local_md.mirror.type = simple_mirror_md.type;
        local_md.mirror.session_id = simple_mirror_md.session_id;
        local_md.flags.bypass_egress = true;
        transition parse_ethernet;
    }
#endif /* DTEL_ENABLE */

    state parse_packet {
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type, eg_intr_md.egress_port) {
            cpu_port : parse_cpu;
            (ETHERTYPE_IPV4, _) : parse_ipv4;
            (ETHERTYPE_IPV6, _) : parse_ipv6;
            (ETHERTYPE_VLAN, _) : parse_vlan;
            (ETHERTYPE_QINQ, _) : parse_vlan;
#ifdef MPLS_ENABLE
            (ETHERTYPE_MPLS, _) : parse_mpls;
#endif // MPLS_ENABLE
	    (ETHERTYPE_PFC, _) : parse_pfc;
            default : accept;
        }
    }

    state parse_pfc {
        local_md.flags.bypass_egress = true;
        transition accept;
    }
    
    state parse_cpu {
        local_md.flags.bypass_egress = true;
        transition accept;
    }

    state parse_vlan {
        pkt.extract(hdr.vlan_tag.next);
        local_md.qos.pcp = hdr.vlan_tag.last.pcp;
        transition select(hdr.vlan_tag.last.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_VLAN : parse_vlan;
            ETHERTYPE_IPV6 : parse_ipv6;
#ifdef MPLS_ENABLE
            ETHERTYPE_MPLS : parse_mpls;
#endif /* MPLS_ENABLE */
            default : accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol, hdr.ipv4.ihl, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5, 0) : parse_tcp;
            (IP_PROTOCOLS_UDP, 5, 0) : parse_ipv4_udp;
            (IP_PROTOCOLS_ICMP, 5, 0) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5, 0) : parse_igmp;
#if defined(EGRESS_TUNNEL_PARSING_ENABLE) && defined(IPINIP_ENABLE)
            (IP_PROTOCOLS_IPV4, 5, 0) : parse_inner_ipv4;
            (IP_PROTOCOLS_IPV6, 5, 0) : parse_inner_ipv6;
#endif
            (_, 6, _) : parse_ipv4_options;
            default : accept;
        }
    }

    state parse_ipv4_options {
        pkt.extract(hdr.ipv4_option);
        transition select(hdr.ipv4.protocol, hdr.ipv4.frag_offset) {
            (IP_PROTOCOLS_TCP, 5)  : parse_tcp;
            (IP_PROTOCOLS_UDP, 5)  : parse_ipv4_udp;
            (IP_PROTOCOLS_ICMP, 5) : parse_icmp;
            (IP_PROTOCOLS_IGMP, 5) : parse_igmp;
            default : accept;
        }
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition select(hdr.ipv6.next_hdr) {
            IP_PROTOCOLS_UDP : parse_ipv6_udp;
            IP_PROTOCOLS_TCP : parse_tcp;
            IP_PROTOCOLS_ICMPV6: parse_icmp;
#ifdef EGRESS_TUNNEL_PARSING_ENABLE
#ifdef IPV6_TUNNEL_ENABLE
#ifdef IPINIP_ENABLE
            IP_PROTOCOLS_IPV4 : parse_inner_ipv4;
            IP_PROTOCOLS_IPV6 : parse_inner_ipv6;
#endif /* IPINIP_ENABLE */
#endif /* IPV6_TUNNEL_ENABLE */
#endif /* EGRESS_TUNNEL_PARSING_ENABLE */
            default : accept;
        }
    }

#ifdef MPLS_ENABLE
    state parse_mpls {
        pkt.extract(hdr.mpls.next);
        transition select(hdr.mpls.last.bos) {
            0 : parse_mpls;
            1 : parse_mpls_bos;
        }
    }

    state parse_mpls_bos {
        transition select(pkt.lookahead<bit<4>>()) {
            0x4 : parse_inner_ipv4;
            0x6 : parse_inner_ipv6;
            default : accept;
        }
    }
#endif /* MPLS_ENABLE */

#ifdef NVGRE_ENABLE
    // nvgre key is 32 bits i.e. combination of both vsid[24 bit], flow_id [8 bit]
    // nvgre-st format  - ipv4/ipv6 -> gre -> nvgre[key==config-key] -> ipv4/ipv6
    // nvgre format     - ipv4/ipv6 -> gre -> nvgre[key!=config-key] -> ipv4/ipv6
    state parse_ip_nvgre {
        pkt.extract(hdr.nvgre);
        // nvgre-st will have config key value or 0x6400
        transition select(hdr.nvgre.vsid_flowid) {
            nvgre_st_key : parse_inner_ethernet;      // nvgre-st
            default  : parse_inner_ethernet;          // nvgre
        }
    }
#endif

    state parse_ip_gre {
        pkt.extract(hdr.gre);
        transition select(hdr.gre.flags_version, hdr.gre.proto) {
#ifdef NVGRE_ENABLE
            (0x2000, GRE_PROTOCOLS_NVGRE) : parse_ip_nvgre;
#endif
            //(_, GRE_PROTOCOLS_IP) : parse_inner_ipv4;
            //(_, GRE_PROTOCOLS_IPV6) : parse_inner_ipv6;
            default : accept;
        }
    }

    state parse_ipv4_udp {
        pkt.extract(hdr.udp);
        transition select(hdr.udp.dst_port) {
#if defined(EGRESS_TUNNEL_PARSING_ENABLE) && defined(VXLAN_ENABLE)
            udp_port_vxlan : parse_vxlan;
#endif
            default : accept;
        }
    }

    state parse_ipv6_udp {
        pkt.extract(hdr.udp);
        transition accept;
    }

    state parse_tcp {
        tcp_h tcp_md = pkt.lookahead<tcp_h>();
        local_md.lkp.l4_src_port = tcp_md.src_port;
        local_md.lkp.l4_dst_port = tcp_md.dst_port;
        local_md.lkp.tcp_flags = tcp_md.flags;
        transition accept;
    }

    state parse_icmp {
        icmp_h icmp_md = pkt.lookahead<icmp_h>();
        local_md.lkp.l4_src_port[7:0]  = icmp_md.type;
        local_md.lkp.l4_src_port[15:8] = icmp_md.code;
        transition accept;
    }

    state parse_igmp {
        igmp_h igmp_md = pkt.lookahead<igmp_h>();
        local_md.lkp.l4_src_port[7:0]  = igmp_md.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        transition accept;
    }

#if defined(EGRESS_TUNNEL_PARSING_ENABLE)
    state parse_vxlan {
        pkt.extract(hdr.vxlan);
        transition parse_inner_ethernet;
    }

    state parse_inner_ethernet {
        pkt.extract(hdr.inner_ethernet);
        transition select(hdr.inner_ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_inner_ipv4;
            ETHERTYPE_IPV6 : parse_inner_ipv6;
            default : accept;
        }
    }

    state parse_inner_ipv4 {
        pkt.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.protocol) {
            // IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            default : accept;
        }
    }

    state parse_inner_ipv6 {
        pkt.extract(hdr.inner_ipv6);
        transition select(hdr.inner_ipv6.next_hdr) {
            // IP_PROTOCOLS_TCP : parse_inner_tcp;
            IP_PROTOCOLS_UDP : parse_inner_udp;
            default : accept;
        }
    }

    state parse_inner_udp {
        pkt.extract(hdr.inner_udp);
        transition accept;
    }

    state parse_inner_tcp {
        pkt.extract(hdr.inner_tcp);
        transition accept;
    }

    state parse_inner_icmp {
        pkt.extract(hdr.inner_icmp);
        transition accept;
    }
#endif

}


//############################################################################
// Mirror packet deparser
//############################################################################
control IngressMirror(
    inout switch_header_t hdr,
    in switch_local_metadata_t local_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {
// Ingress deparser create a copy of the original ingress packet and prepend the prepend the mirror
// header.
    Mirror() mirror;

    apply {
#ifdef MIRROR_ENABLE
        if (ig_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_PORT) {
            mirror.emit<switch_port_mirror_metadata_h>(
                (MirrorId_t)local_md.mirror.session_id,
                {local_md.mirror.src,
                 local_md.mirror.type,
                 local_md.timestamp,
#if __TARGET_TOFINO__ == 1
#if switch_mirror_session_width != 8
                 0,
#endif
#endif
                 local_md.mirror.session_id});

        } else if (ig_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_DTEL_DROP) {
#ifdef DTEL_ENABLE
            mirror.emit<switch_dtel_drop_mirror_metadata_h>(local_md.dtel.session_id, {
                local_md.mirror.src,
                local_md.mirror.type,
                local_md.timestamp,
#if __TARGET_TOFINO__ == 1
                 0,
#endif
                local_md.dtel.session_id,
                local_md.lag_hash,
                local_md.dtel.report_type,
                0,
                local_md.ingress_port,
                0,
                local_md.egress_port,
                0,
                local_md.qos.qid,
                local_md.drop_reason
            });

#endif /* DTEL_ENABLE */
        }
#endif /* MIRROR_ENABLE */
    }
}

control EgressMirror(
    inout switch_header_t hdr,
    in switch_local_metadata_t local_md,
    in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {
// Egress deparser first construct the output packet and then prepend the mirror header.
    Mirror() mirror;

    apply {
#ifdef MIRROR_ENABLE
        if (eg_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_PORT) {
            mirror.emit<switch_port_mirror_metadata_h>((MirrorId_t)local_md.mirror.session_id, {
                local_md.mirror.src,
                local_md.mirror.type,
                local_md.ingress_timestamp,
#if __TARGET_TOFINO__ == 1
#if switch_mirror_session_width != 8
                0,
#endif
#endif
                local_md.mirror.session_id});
        } else if (eg_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_CPU) {
            mirror.emit<switch_cpu_mirror_metadata_h>((MirrorId_t)local_md.mirror.session_id, {
                local_md.mirror.src,
                local_md.mirror.type,
                0,
                local_md.ingress_port,
		0,
                local_md.bd,
                0,
                local_md.egress_port_lag_index,
                local_md.cpu_reason});
        } else if (eg_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_DTEL_SWITCH_LOCAL) {
#ifdef DTEL_ENABLE
            mirror.emit<switch_dtel_switch_local_mirror_metadata_h>(local_md.dtel.session_id, {
                local_md.mirror.src, local_md.mirror.type,
                local_md.ingress_timestamp,
#if __TARGET_TOFINO__ == 1
                0,
#endif
                local_md.dtel.session_id,
                local_md.dtel.hash,
                local_md.dtel.report_type,
                0,
                local_md.ingress_port,
                0,
                local_md.egress_port,
                0,
                local_md.qos.qid,
                0,
                local_md.qos.qdepth,
                local_md.timestamp
            });
#endif /* DTEL_ENABLE */
        } else if (eg_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_DTEL_DROP) {
#ifdef DTEL_ENABLE
            mirror.emit<switch_dtel_drop_mirror_metadata_h>(local_md.dtel.session_id, {
                local_md.mirror.src,
                local_md.mirror.type,
                local_md.ingress_timestamp,
#if __TARGET_TOFINO__ == 1
                 0,
#endif
                local_md.dtel.session_id,
                local_md.dtel.hash,
                local_md.dtel.report_type,
                0,
                local_md.ingress_port,
                0,
                local_md.egress_port,
                0,
                local_md.qos.qid,
                local_md.drop_reason
            });
#endif /* DTEL_ENABLE */
        } else if (eg_intr_md_for_dprsr.mirror_type == SWITCH_MIRROR_TYPE_SIMPLE) {
#ifdef DTEL_ENABLE
            mirror.emit<switch_simple_mirror_metadata_h>(local_md.dtel.session_id, {
                local_md.mirror.src,
                local_md.mirror.type,
#if __TARGET_TOFINO__ == 1
                0,
#endif
                local_md.dtel.session_id
            });
#endif /* DTEL_ENABLE */
        }
#endif /* MIRROR_ENABLE */
    }
}

//############################################################################
// L4 Checksum update
//############################################################################

control IngressNatChecksum(
    inout switch_header_t hdr,
    in switch_local_metadata_t local_md) {
    Checksum() tcp_checksum;
    Checksum() udp_checksum;
    apply {
#ifdef NAT_ENABLE
        if (hdr.ipv4.isValid()) {
            if(hdr.tcp.isValid()) {
              hdr.tcp.checksum = tcp_checksum.update({
                      local_md.lkp.ip_src_addr,
                      local_md.lkp.ip_dst_addr,
                      hdr.tcp.src_port,
                      hdr.tcp.dst_port,
                      local_md.tcp_udp_checksum});
            } else if(hdr.udp.isValid()) {
              hdr.udp.checksum = udp_checksum.update({
                      local_md.lkp.ip_src_addr,
                      local_md.lkp.ip_dst_addr,
                      hdr.udp.src_port,
                      hdr.udp.dst_port,
                      local_md.tcp_udp_checksum});
            }
        }
#endif
    }
}

//############################################################################-
// Ingress Deparser on External pipe
//############################################################################-
control SwitchIngressDeparser_0(
    packet_out pkt,
    inout switch_header_t hdr,
    in switch_local_metadata_t local_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {

    @name(".learning_digest")
    Digest<switch_learning_digest_t>() digest;
    Checksum() ipv4_checksum;

    apply {

        hdr.ipv4.hdr_checksum = ipv4_checksum.update({
            hdr.ipv4.version,
            hdr.ipv4.ihl,
            hdr.ipv4.diffserv,
            hdr.ipv4.total_len,
            hdr.ipv4.identification,
            hdr.ipv4.flags,
            hdr.ipv4.frag_offset,
            hdr.ipv4.ttl,
            hdr.ipv4.protocol,
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            hdr.ipv4_option.type,
            hdr.ipv4_option.length,
            hdr.ipv4_option.value});

        if (ig_intr_md_for_dprsr.digest_type == SWITCH_DIGEST_TYPE_MAC_LEARNING) {
            digest.pack({local_md.bd, local_md.ingress_port_lag_index, hdr.ethernet.src_addr});
        }

        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.fp_bridged_md);
        pkt.emit(hdr.fabric);
        pkt.emit(hdr.cpu);
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.arp); // Ingress only.
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
#ifdef SRV6_ENABLE
        pkt.emit(hdr.srh_base);
        pkt.emit(hdr.srh_seg_list);
#endif /* SRV6_ENABLE */
#ifdef MPLS_ENABLE
        pkt.emit(hdr.mpls);
#endif /* MPLS_ENABLE */
        pkt.emit(hdr.udp);
        pkt.emit(hdr.tcp); // Ingress only.
        pkt.emit(hdr.icmp); // Ingress only.
        pkt.emit(hdr.igmp); // Ingress only.
        pkt.emit(hdr.rocev2_bth); // Ingress only.
#ifdef VXLAN_ENABLE
        pkt.emit(hdr.vxlan);
#endif
        pkt.emit(hdr.gre);
#ifdef NVGRE_ENABLE
        pkt.emit(hdr.nvgre);
#endif
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
//        pkt.emit(hdr.inner_udp);
//        pkt.emit(hdr.inner_tcp);
//        pkt.emit(hdr.inner_icmp);
    }
}


//############################################################################-
// Ingress Deparser on the internal pipe
//############################################################################-
control SwitchIngressDeparser_1(
    packet_out pkt,
    inout switch_header_t hdr,
    in switch_local_metadata_t local_md,
    in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr) {

    IngressMirror() mirror;
#ifdef NAT_ENABLE
    IngressNatChecksum() nat_checksum;
#endif

    apply {
        mirror.apply(hdr, local_md, ig_intr_md_for_dprsr);

#ifdef NAT_ENABLE
        nat_checksum.apply(hdr,local_md);
#endif

        pkt.emit(hdr.bridged_md);
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.arp);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.icmp);
        pkt.emit(hdr.igmp);
        pkt.emit(hdr.vxlan);
        pkt.emit(hdr.gre);
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_tcp);
        pkt.emit(hdr.inner_udp);
        pkt.emit(hdr.inner_icmp);
        pkt.emit(hdr.inner_igmp);
    }
}


//############################################################################-
// Egress Deparser on pipe with external facing ports
//############################################################################-
control SwitchEgressDeparser_0(
        packet_out pkt,
        inout switch_header_t hdr,
        in switch_local_metadata_t local_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {

    EgressMirror() mirror;
    Checksum() ipv4_checksum;
    Checksum() inner_ipv4_checksum;

    apply {
        mirror.apply(hdr, local_md, eg_intr_md_for_dprsr);

        hdr.ipv4.hdr_checksum = ipv4_checksum.update({
            hdr.ipv4.version,
            hdr.ipv4.ihl,
            hdr.ipv4.diffserv,
            hdr.ipv4.total_len,
            hdr.ipv4.identification,
            hdr.ipv4.flags,
            hdr.ipv4.frag_offset,
            hdr.ipv4.ttl,
            hdr.ipv4.protocol,
            hdr.ipv4.src_addr,
            hdr.ipv4.dst_addr,
            hdr.ipv4_option.type,
            hdr.ipv4_option.length,
            hdr.ipv4_option.value});

#ifdef TUNNEL_ENABLE
        if (local_md.inner_ipv4_checksum_update_en) {
            hdr.inner_ipv4.hdr_checksum = inner_ipv4_checksum.update({
                hdr.inner_ipv4.version,
                hdr.inner_ipv4.ihl,
                hdr.inner_ipv4.diffserv,
                hdr.inner_ipv4.total_len,
                hdr.inner_ipv4.identification,
                hdr.inner_ipv4.flags,
                hdr.inner_ipv4.frag_offset,
                hdr.inner_ipv4.ttl,
                hdr.inner_ipv4.protocol,
                hdr.inner_ipv4.src_addr,
                hdr.inner_ipv4.dst_addr});
        }
#endif /* TUNNEL_ENABLE */

        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.fabric); // Egress only.
        pkt.emit(hdr.cpu); // Egress only.
        pkt.emit(hdr.timestamp); // Egress only.
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
#ifdef SRV6_ENABLE
        pkt.emit(hdr.srh_base);
        pkt.emit(hdr.srh_seg_list);
#endif /* SRV6_ENABLE */
#ifdef MPLS_ENABLE
        pkt.emit(hdr.mpls);
#endif /* MPLS_ENABLE */
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.udp);
#ifdef DTEL_ENABLE
        pkt.emit(hdr.dtel); // Egress only.
#endif
        pkt.emit(hdr.icmp); // Egress only.
        pkt.emit(hdr.igmp); // Egress only.
#ifdef INT_V2
        pkt.emit(hdr.dtel_metadata_1); // Egress only.
        pkt.emit(hdr.dtel_metadata_2); // Egress only.
        pkt.emit(hdr.dtel_metadata_3); // Egress only.
        pkt.emit(hdr.dtel_metadata_4); // Egress only.
        pkt.emit(hdr.dtel_metadata_5); // Egress only.
#else
        pkt.emit(hdr.dtel_report); // Egress only.
        pkt.emit(hdr.dtel_switch_local_report); // Egress only.
#endif
        pkt.emit(hdr.dtel_drop_report); // Egress only.
#ifdef VXLAN_ENABLE
        pkt.emit(hdr.vxlan);
#endif
        pkt.emit(hdr.gre);
#ifdef NVGRE_ENABLE
        pkt.emit(hdr.nvgre);
#endif
        pkt.emit(hdr.erspan); // Egress only.
        pkt.emit(hdr.erspan_type2); // Egress only.
        pkt.emit(hdr.erspan_type3); // Egress only.
        pkt.emit(hdr.erspan_platform); // Egress only.
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_udp);
    }
}

//############################################################################-
// Egress Deparser on Internal Pipe
//############################################################################-
control SwitchEgressDeparser_1(
        packet_out pkt,
        inout switch_header_t hdr,
        in switch_local_metadata_t local_md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr) {

    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.fp_bridged_md);
        pkt.emit(hdr.fabric);
        pkt.emit(hdr.cpu);
        pkt.emit(hdr.vlan_tag);
        pkt.emit(hdr.arp);
        pkt.emit(hdr.ipv4);
        pkt.emit(hdr.ipv4_option);
        pkt.emit(hdr.ipv6);
        pkt.emit(hdr.udp);
        pkt.emit(hdr.tcp);
        pkt.emit(hdr.icmp);
        pkt.emit(hdr.igmp);
        pkt.emit(hdr.vxlan);
        pkt.emit(hdr.gre);
        pkt.emit(hdr.inner_ethernet);
        pkt.emit(hdr.inner_ipv4);
        pkt.emit(hdr.inner_ipv6);
        pkt.emit(hdr.inner_tcp);
        pkt.emit(hdr.inner_udp);
        pkt.emit(hdr.inner_icmp);
        pkt.emit(hdr.inner_igmp);
    }
}
