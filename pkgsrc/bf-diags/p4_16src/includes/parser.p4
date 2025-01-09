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

#ifndef _DIAG_PARSER_
#define _DIAG_PARSER_

/**
 *  PacketParser()
 *
 *  Used in both SwitchIngressParser and SwitchEgressParser.
 */
parser PacketParser(packet_in packet,
                    inout headers_t hdr,
                    inout l2_metadata_t l2_metadata,
                    inout l3_metadata_t l3_metadata) {
    
    state start {
        transition parse_ethernet;
    }    

    state parse_ethernet {
        packet.extract(hdr.ethernet);
#if defined(DIAG_PHV_STRESS_ENABLE)
        transition parse_phv_stress_hdr;
#elif defined(DIAG_PARDE_STRAIN)
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4: parse_ipv4_parde_strain;
            ETHERTYPE_IPV6: parse_ipv6_parde_strain;
            default: accept;
        }
#else
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_VLAN: parse_vlan_tag;
            ETHERTYPE_QINQ: parse_qinq;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_ARP: parse_arp_rarp;
            ETHERTYPE_NSH: parse_nsh;
            ETHERTYPE_ROCE: parse_roce;
            ETHERTYPE_FCOE: parse_fcoe;
            ETHERTYPE_TRILL: parse_trill;
            ETHERTYPE_VNTAG: parse_vntag;
            ETHERTYPE_LLDP: parse_set_prio_high;
            ETHERTYPE_LACP: parse_set_prio_high;
            default: accept;
        }
#endif
    }
    state parse_arp_rarp {
        transition parse_set_prio_med;
    }

#ifdef DIAG_PARSE_EOMPLS
    // no transition to this state
    state parse_eompls {
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_MPLS;
        transition parse_inner_ethernet;
    }
#endif

    state parse_fcoe {
        packet.extract(hdr.fcoe);
        transition accept;
    }

#ifdef DIAG_PARSE_GRE
    // no transition to this state
    state parse_gre {
        packet.extract(hdr.gre);
        transition select(hdr.gre.C, hdr.gre.R, hdr.gre.K, hdr.gre.S,
                          hdr.gre.s, hdr.gre.recurse, hdr.gre.flags,
                          hdr.gre.ver, hdr.gre.proto) {
            (1w0x0, 1w0x0, 1w0x1, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, ETHERTYPE_ETHERNET): parse_nvgre;
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, ETHERTYPE_IPV4): parse_gre_ipv4;
            (1w0x0, 1w0x0, 1w0x0, 1w0x0, 1w0x0, 3w0x0, 5w0x0, 3w0x0, ETHERTYPE_IPV6): parse_gre_ipv6;
            default: accept;
        }
    }

    state parse_gre_ipv4 {
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_GRE;
        transition parse_inner_ipv4;
    }

    state parse_gre_ipv6 {
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_GRE;
        transition parse_inner_ipv6;
    }
#endif // DIAG_PARSE_GRE

    state parse_icmp {
        packet.extract(hdr.icmp);
        l3_metadata.lkp_outer_l4_sport = hdr.icmp.typeCode;
        transition select(hdr.icmp.typeCode) {
            16w0x8200 &&& 16w0xfe00: parse_set_prio_med;
            16w0x8400 &&& 16w0xfc00: parse_set_prio_med;
            16w0x8800 &&& 16w0xff00: parse_set_prio_med;
            default: accept;
        }
    }

    state parse_inner_ethernet {
        packet.extract(hdr.inner_ethernet);
        l2_metadata.lkp_mac_sa = hdr.inner_ethernet.srcAddr;
        l2_metadata.lkp_mac_da = hdr.inner_ethernet.dstAddr;
        transition select(hdr.inner_ethernet.etherType) {
            ETHERTYPE_IPV4: parse_inner_ipv4;
            ETHERTYPE_IPV6: parse_inner_ipv6;
            default: accept;
        }
    }

    state parse_inner_icmp {
        packet.extract(hdr.inner_icmp);
        l3_metadata.lkp_l4_sport = hdr.inner_icmp.typeCode;
        transition accept;
    }

    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        l3_metadata.lkp_ip_proto = hdr.inner_ipv4.protocol;
        l3_metadata.lkp_ip_ttl = hdr.inner_ipv4.ttl;
        transition select(hdr.inner_ipv4.protocol) {
            IP_PROTOCOLS_ICMP: parse_inner_icmp;
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            IP_PROTOCOLS_UDP: parse_inner_udp;
            default: accept;
        }
    }

    state parse_inner_ipv6 {
        packet.extract(hdr.inner_ipv6);
        l3_metadata.lkp_ipv6_sa = hdr.inner_ipv6.srcAddr;
        l3_metadata.lkp_ipv6_da = hdr.inner_ipv6.dstAddr;
        l3_metadata.lkp_ip_proto = hdr.inner_ipv6.nextHdr;
        l3_metadata.lkp_ip_ttl = hdr.inner_ipv6.hopLimit;
        transition select(hdr.inner_ipv6.nextHdr) {
            IP_PROTOCOLS_ICMP: parse_inner_icmp;
            IP_PROTOCOLS_TCP: parse_inner_tcp;
            IP_PROTOCOLS_UDP: parse_inner_udp;
            default: accept;
        }
    }

    state parse_inner_tcp {
        packet.extract(hdr.inner_tcp);
        l3_metadata.lkp_l4_sport = hdr.inner_tcp.srcPort;
        l3_metadata.lkp_l4_dport = hdr.inner_tcp.dstPort;
        transition accept;
    }

    state parse_inner_udp {
        packet.extract(hdr.inner_udp);
        l3_metadata.lkp_l4_sport = hdr.inner_udp.srcPort;
        l3_metadata.lkp_l4_dport = hdr.inner_udp.dstPort;
        transition accept;
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_ICMP: parse_icmp;
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            IP_PROTOCOLS_MPLS: parse_mpls;
            IP_PROTOCOLS_IPV6: parse_ipv6;
            default: accept;
        }
    }

    state parse_ipv4_in_ip {
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_IP_IN_IP;
        transition parse_inner_ipv4;
    }

    state parse_ipv6 {
        packet.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            IP_PROTOCOLS_ICMP: parse_icmp;
            IP_PROTOCOLS_TCP: parse_tcp;
            IP_PROTOCOLS_UDP: parse_udp;
            IP_PROTOCOLS_MPLS: parse_mpls;
            default: accept;
        }
    }

    state parse_ipv6_in_ip {
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_IP_IN_IP;
        transition parse_inner_ipv6;
    }

    state parse_mpls {
        packet.extract(hdr.mpls);
        transition accept;
    }

    state parse_nsh {
        packet.extract(hdr.nsh);
        packet.extract(hdr.nsh_context);
        transition select(hdr.nsh.protoType) {
            ETHERTYPE_IPV4: parse_inner_ipv4;
            ETHERTYPE_IPV6: parse_inner_ipv6;
            ETHERTYPE_ETHERNET: parse_inner_ethernet;
            default: accept;
        }
    }

#ifdef DIAG_PARSE_GRE
    state parse_nvgre {
        packet.extract(hdr.nvgre);
        l2_metadata.ingress_tunnel_type = INGRESS_TUNNEL_TYPE_NVGRE;
        l2_metadata.tunnel_vni = hdr.nvgre.tni;
        transition parse_inner_ethernet;
    }
#endif

    state parse_qinq {
        packet.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.etherType) {
            ETHERTYPE_VLAN: parse_qinq_vlan;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_ARP: parse_arp_rarp;
            ETHERTYPE_NSH: parse_nsh;
            ETHERTYPE_ROCE: parse_roce;
            ETHERTYPE_FCOE: parse_fcoe;
            ETHERTYPE_TRILL: parse_trill;
            ETHERTYPE_VNTAG: parse_vntag;
            ETHERTYPE_LLDP: parse_set_prio_high;
            ETHERTYPE_LACP: parse_set_prio_high;
            default: accept;
        }
    }

    state parse_qinq_vlan {
        packet.extract(hdr.vlan_tag_1);
        transition select(hdr.vlan_tag_1.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_ARP: parse_arp_rarp;
            ETHERTYPE_NSH: parse_nsh;
            ETHERTYPE_ROCE: parse_roce;
            ETHERTYPE_FCOE: parse_fcoe;
            ETHERTYPE_TRILL: parse_trill;
            ETHERTYPE_VNTAG: parse_vntag;
            ETHERTYPE_LLDP: parse_set_prio_high;
            ETHERTYPE_LACP: parse_set_prio_high;
            default: accept;
        }
    }

    state parse_roce {
        packet.extract(hdr.roce);
        transition accept;
    }

    state parse_set_prio_high {
        //ig_prsr_ctrl.priority = CONTROL_TRAFFIC_PRIO_5;
        transition accept;
    }

    state parse_set_prio_med {
        //ig_prsr_ctrl.priority = CONTROL_TRAFFIC_PRIO_3;
        transition accept;
    }

    state parse_tcp {
        packet.extract(hdr.tcp);
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
        transition parse_mau_bus_stress_hdr;
#else
        transition accept;
#endif
    }

    state parse_trill {
        packet.extract(hdr.trill);
        transition parse_inner_ethernet;
    }

    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }

    state parse_vlan_tag {
        packet.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            ETHERTYPE_MPLS: parse_mpls;
            ETHERTYPE_IPV6: parse_ipv6;
            ETHERTYPE_ARP: parse_arp_rarp;
            ETHERTYPE_NSH: parse_nsh;
            ETHERTYPE_ROCE: parse_roce;
            ETHERTYPE_FCOE: parse_fcoe;
            ETHERTYPE_TRILL: parse_trill;
            ETHERTYPE_VNTAG: parse_vntag;
            ETHERTYPE_LLDP: parse_set_prio_high;
            ETHERTYPE_LACP: parse_set_prio_high;
            default: accept;
        }
    }

    state parse_vntag {
        packet.extract(hdr.vntag);
        transition parse_inner_ethernet;
    }

#ifdef DIAG_PHV_STRESS_ENABLE
    state parse_phv_stress_hdr {
        packet.extract(hdr.phv_stress_hdr);
#if defined(DIAG_PATTERN_SHIFT_ENABLE)
        transition select(latest.pad) {
            0 mask 0: parse_left_shift;
            // never transition to the following state
            default: parse_right_shift;
        }
#else
        transition accept;
#endif
    }
#endif

#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
    state parse_mau_bus_stress_hdr {
        packet.extract(hdr.mau_bus_stress_hdr);
        transition accept;
    }
#endif

#if defined(DIAG_PARDE_STRAIN)
    state parse_ipv4_parde_strain {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            IP_PROTOCOLS_TCP: parse_tcp_parde_strain;
            default: accept;
        }
    }

    state parse_ipv6_parde_strain {
        packet.extract(hdr.ipv6);
        transition select(hdr.ipv6.nextHdr) {
            IP_PROTOCOLS_TCP: parse_tcp_parde_strain;
            default: accept;
        }
    }

    state parse_tcp_parde_strain {
        packet.extract(hdr.tcp);
        transition parse_parde_strain_hdr;
    }

    state parse_parde_strain_hdr {
        packet.extract(hdr.parde_strain);
        transition select(hdr.parde_strain.hdr1_valid) {
            1: parse_parde_strain_val_1;
            default: parse_parde_strain_val_2_check;
        }
    }

#define PARSE_PARDE_STRAIN_VAL_CHECK(n, m)                    \
    state parse_parde_strain_val_##n##_check {                \
        transition select(hdr.parde_strain.hdr##n##_valid) {  \
            1: parse_parde_strain_val_##n##;                  \
            default: parse_parde_strain_val_##m##_check;      \
        }                                                     \
    }

#define PARSE_PARDE_STRAIN_VAL_CHECK_LAST(n)                  \
    state parse_parde_strain_val_##n##_check {                \
        transition select(hdr.parde_strain.hdr##n##_valid) {  \
            1: parse_parde_strain_val_##n##;                  \
            default: accept;                                  \
        }                                                     \
    }
    
    PARSE_PARDE_STRAIN_VAL_CHECK(1,2)
    PARSE_PARDE_STRAIN_VAL_CHECK(2,3)
    PARSE_PARDE_STRAIN_VAL_CHECK(3,4)
    PARSE_PARDE_STRAIN_VAL_CHECK(4,5)
    PARSE_PARDE_STRAIN_VAL_CHECK_LAST(5)

#define PARSE_PARDE_STRAIN_VAL(n,m)                         \
    state parse_parde_strain_val_##n## {                    \
        packet.extract(hdr.parde_strain_val_##n##);         \
        transition parse_parde_strain_val_##m##_check;      \
    }

#define PARSE_PARDE_STRAIN_VAL_LAST(n)                      \
    state parse_parde_strain_val_##n## {                    \
        packet.extract(hdr.parde_strain_val_##n##);         \
        transition accept;                                  \
    }

    PARSE_PARDE_STRAIN_VAL(1,2)
    PARSE_PARDE_STRAIN_VAL(2,3)
    PARSE_PARDE_STRAIN_VAL(3,4)
    PARSE_PARDE_STRAIN_VAL(4,5)
    PARSE_PARDE_STRAIN_VAL_LAST(5)
#endif // DIAG_PARDE_STRAIN

} // PacketParser

#define TOF1_PGEN_PORT 68
#define TOF2_PGEN_PORT 6
#define TOF3_PGEN_PORT 6
/**
 *  SwitchIngressParser()
 */
parser SwitchIngressParser(
        packet_in packet,
        out headers_t hdr,
        out i_metadata ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    PacketParser() pkt_parser;

    state start {
        packet.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1 : parse_resubmit;
            0 : parse_port_metadata;
        }
    }

    state parse_resubmit {
        // Parse resubmitted packet here.
        transition parse_port_metadata;
    }

    state parse_port_metadata {
#if   __TARGET_TOFINO__ == 3
        packet.advance(192);
#elif __TARGET_TOFINO__ == 2
        packet.advance(192);
#else
        packet.advance(64);
#endif
        transition select(ig_intr_md.ingress_port) {
#if   __TARGET_TOFINO__ == 3
            TOF3_PGEN_PORT : parse_pktgen_timer;
#elif __TARGET_TOFINO__ == 2
            TOF2_PGEN_PORT : parse_pktgen_timer;
#else
            TOF1_PGEN_PORT : parse_pktgen_timer;
#endif
            default : parse_ethernet_wrapper;
        }
    }

    state parse_pktgen_timer {
        packet.extract(hdr.timer);
        transition parse_ethernet_wrapper;
    }

    state parse_ethernet_wrapper {
        // Parse the ethernet packet
        pkt_parser.apply(packet, hdr, ig_md.l2_metadata, ig_md.l3_metadata);
        transition accept;
    }

} // SwitchIngressParser

/**
 *  SwitchIngressDeparser()
 */
control SwitchIngressDeparser(
        packet_out packet,
        inout headers_t hdr,
        in i_metadata ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    Digest<mac_learn_digest>() digest;
    Resubmit() resubmit;

    apply {
        if (ig_dprsr_md.resubmit_type == DIAG_RESUBMIT_TYPE_BASIC) {
            resubmit.emit();
        }

        if (ig_dprsr_md.digest_type == 0) {
            digest.pack({ig_md.ingress_metadata.vlan_id,
                         hdr.ethernet.srcAddr,
                         ig_md.ingress_metadata.ingress_port });
        }

        packet.emit(hdr.bridged_md);  // Ingress only
        packet.emit(hdr.ethernet);
#ifdef DIAG_PHV_STRESS_ENABLE
        packet.emit(hdr.phv_stress_hdr);
#endif
        packet.emit(hdr.vlan_tag);
        packet.emit(hdr.vlan_tag_1);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
        packet.emit(hdr.mau_bus_stress_hdr);
#endif
#if defined(DIAG_PARDE_STRAIN)
        packet.emit(hdr.parde_strain);
        packet.emit(hdr.parde_strain_val_1);
        packet.emit(hdr.parde_strain_val_2);
        packet.emit(hdr.parde_strain_val_3);
        packet.emit(hdr.parde_strain_val_4);
        packet.emit(hdr.parde_strain_val_5);
        packet.emit(hdr.parde_strain_val_6);
        packet.emit(hdr.parde_strain_val_7);
        packet.emit(hdr.parde_strain_val_8);
#endif
        packet.emit(hdr.vntag);
        packet.emit(hdr.trill);
        packet.emit(hdr.fcoe);
        packet.emit(hdr.roce);
        packet.emit(hdr.nsh);
        packet.emit(hdr.nsh_context);
        packet.emit(hdr.gre);
        packet.emit(hdr.nvgre);
        packet.emit(hdr.icmp);
        packet.emit(hdr.mpls);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv6);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_udp);
        packet.emit(hdr.inner_tcp);
        packet.emit(hdr.inner_icmp);
    }
} // SwitchIngressDeparser

/**
 *  SwitchEgressParser()
 */
parser SwitchEgressParser(
        packet_in packet,
        out headers_t hdr,
        out e_metadata eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    PacketParser() pkt_parser;
    
    state start {
        packet.extract(eg_intr_md);
        transition parse_bridged_pkt;
    }

    state parse_bridged_pkt {
        packet.extract(hdr.bridged_md);
        eg_md.egress_metadata.vlan_id = hdr.bridged_md.vlan_id;
        eg_md.l2_metadata.dst_override = hdr.bridged_md.dst_override;
        eg_md.l2_metadata.cpu_redir = hdr.bridged_md.cpu_redir;
        // Parse the ethernet packet
        pkt_parser.apply(packet, hdr, eg_md.l2_metadata, eg_md.l3_metadata);
        transition accept;
    }
} // SwitchEgressParser

/**
 *  EgressMirror()
 *
 *  Egress Mirror packet in deparser
 */
control EgressMirror(
        inout headers_t hdr,
        in e_metadata eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {

    // Egress deparser create a copy of the packet.
    Mirror() mirror;

    apply {
        /* Egress Mirroring */
        if (eg_dprsr_md.mirror_type == DIAG_MIRROR_TYPE_EGRESS) {
            mirror.emit(eg_md.l2_metadata.egress_mirror_session_id);
        }    
    }
} // EgressMirror

/**
 *  SwitchEgressDeparser()
 */
control SwitchEgressDeparser(
        packet_out packet,
        inout headers_t hdr,
        in e_metadata eg_md,
        in egress_intrinsic_metadata_for_deparser_t eg_dprsr_md) {

    EgressMirror() mirror;

    apply {
        mirror.apply(hdr, eg_md, eg_dprsr_md);

        packet.emit(hdr.ethernet);
#ifdef DIAG_PHV_STRESS_ENABLE
        packet.emit(hdr.phv_stress_hdr);
#endif
        packet.emit(hdr.vlan_tag);
        packet.emit(hdr.vlan_tag_1);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
        packet.emit(hdr.mau_bus_stress_hdr);
#endif
#if defined(DIAG_PARDE_STRAIN)
        packet.emit(hdr.parde_strain);
        packet.emit(hdr.parde_strain_val_1);
        packet.emit(hdr.parde_strain_val_2);
        packet.emit(hdr.parde_strain_val_3);
        packet.emit(hdr.parde_strain_val_4);
        packet.emit(hdr.parde_strain_val_5);
        packet.emit(hdr.parde_strain_val_6);
        packet.emit(hdr.parde_strain_val_7);
        packet.emit(hdr.parde_strain_val_8);
#endif
        packet.emit(hdr.vntag);
        packet.emit(hdr.trill);
        packet.emit(hdr.fcoe);
        packet.emit(hdr.roce);
        packet.emit(hdr.nsh);
        packet.emit(hdr.nsh_context);
        packet.emit(hdr.gre);
        packet.emit(hdr.nvgre);
        packet.emit(hdr.icmp);
        packet.emit(hdr.mpls);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv6);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_udp);
        packet.emit(hdr.inner_tcp);
        packet.emit(hdr.inner_icmp);
    }
} // SwitchEgressDeparser

#endif /* _DIAG_PARSER_ */
