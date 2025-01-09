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



#ifndef _DIAG_HEADERS_
#define _DIAG_HEADERS_


struct ingress_metadata_t {
    bit<12> vlan_id;
    PortId_t  ingress_port;
    PortId_t  egress_port;
}

struct egress_metadata_t {
    bit<12> vlan_id;
}

struct l2_metadata_t {
    bit<48>  lkp_mac_sa;
    bit<48>  lkp_mac_da;
    bit<1>   l2_src_miss;
    bit<1>   l2_src_hit;
    bit<1>   port_vlan_mapping_miss;
    bit<8>   dst_override;
    bit<32>  inter_stage;
    bit<8>   inter_stage_dummy;
    bit<750> dummy_exm_key;
    bit<40>  dummy_tcam_key;
    bit<32>  ingress_tunnel_type;
    bit<24>  tunnel_vni;
    bit<32>  dummy_tcam_meta;
    bit<16>  mau_exm_cntr;
    bit<16>  mau_tcam_cntr;
    MirrorId_t egress_mirror_session_id;
    bit<8>   cpu_redir;
#ifdef DIAG_PARDE_STRAIN
    bit<8>   parde_strain_random;
    bit<64>  parde_hdr_add_cnt;
    bit<64>  parde_hdr_rem_cnt;
#endif
}

struct l3_metadata_t {
    bit<2>   lkp_ip_type;
    bit<4>   lkp_ip_version;
    bit<8>   lkp_ip_proto;
    bit<8>   lkp_dscp;
    bit<8>   lkp_ip_ttl;
    bit<16>  lkp_l4_sport;
    bit<16>  lkp_l4_dport;
    bit<16>  lkp_outer_l4_sport;
    bit<16>  lkp_outer_l4_dport;
    bit<128> lkp_ipv6_sa;
    bit<128> lkp_ipv6_da;
}

struct i_metadata {
    @name(".ingress_metadata") ingress_metadata_t ingress_metadata;
    @name(".l2_metadata")  l2_metadata_t      l2_metadata;
    @name(".l3_metadata") l3_metadata_t       l3_metadata;
}

struct e_metadata {
    @name(".egress_metadata") egress_metadata_t egress_metadata;
    @name(".l2_metadata")  l2_metadata_t      l2_metadata;
    @name(".l3_metadata") l3_metadata_t       l3_metadata;
}

@name("mac_learn_digest") struct mac_learn_digest {
    bit<12> vlan_id;
    bit<48> srcAddr;
    PortId_t  ingress_port;
}

// Header types used by ingress/egress deparsers.
header diag_bridged_metadata_t {
    // user-defined metadata carried over from ingress to egress.
    bit<12> vlan_id;
    bit<4> pad0;
    bit<8> dst_override;
    bit<8> cpu_redir;

    // Add more fields here.
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header fcoe_header_t {
    bit<4>  version;
    bit<4>  type_;
    bit<8>  sof;
    bit<32> rsvd1;
    bit<32> ts_upper;
    bit<32> ts_lower;
    bit<32> size_;
    bit<8>  eof;
    bit<24> rsvd2;
}

header gre_t {
    bit<1>  C;
    bit<1>  R;
    bit<1>  K;
    bit<1>  S;
    bit<1>  s;
    bit<3>  recurse;
    bit<5>  flags;
    bit<3>  ver;
    bit<16> proto;
}

header icmp_t {
    bit<16> typeCode;
    bit<16> hdrChecksum;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header ipv6_t {
    bit<4>   version;
    bit<8>   trafficClass;
    bit<20>  flowLabel;
    bit<16>  payloadLen;
    bit<8>   nextHdr;
    bit<8>   hopLimit;
    bit<128> srcAddr;
    bit<128> dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> hdr_length;
    bit<16> checksum;
}

header nsh_t {
    bit<1>  oam;
    bit<1>  context;
    bit<6>  flags;
    bit<8>  reserved;
    bit<16> protoType;
    bit<24> spath;
    bit<8>  sindex;
}

header nsh_context_t {
    bit<32> network_platform;
    bit<32> network_shared;
    bit<32> service_platform;
    bit<32> service_shared;
}

header nvgre_t {
    bit<24> tni;
    bit<8>  flow_id;
}

header roce_header_t {
    bit<320> ib_grh;
    bit<96>  ib_bth;
}

header trill_t {
    bit<2>  version;
    bit<2>  reserved;
    bit<1>  multiDestination;
    bit<5>  optLength;
    bit<6>  hopCount;
    bit<16> egressRbridge;
    bit<16> ingressRbridge;
}

header vlan_tag_t {
    bit<3>  pri;
    bit<1>  cfi;
    bit<12> vlan_id;
    bit<16> etherType;
}

header vntag_t {
    bit<1>  direction;
    bit<1>  pointer;
    bit<14> destVif;
    bit<1>  looped;
    bit<1>  reserved;
    bit<2>  version;
    bit<12> srcVif;
}

header mpls_t {
    bit<20> label;
    bit<3>  exp;
    bit<1>  bos;
    bit<8>  ttl;
}

header mau_bus_stress_hdr_t {
    mau_stress_hdr_exm_key_len_t   exm_key;
    mau_stress_hdr_tcam_key_len_t  tcam_key;
}

#ifdef DIAG_PARDE_STRAIN

header parde_strain_t {
  bit<1> hdr1_valid;
  bit<1> hdr2_valid;
  bit<1> hdr3_valid;
  bit<1> hdr4_valid;
  bit<1> hdr5_valid;
  bit<1> hdr6_valid;
  bit<1> hdr7_valid;
  bit<1> hdr8_valid;
}

header parde_strain_val_1_t {
    bit<8> value;
}
header parde_strain_val_2_t {
    bit<16> value;
}
header parde_strain_val_3_t {
    bit<32> value;
}
header parde_strain_val_4_t {
    bit<8> value;
    bit<16> value1;
}
header parde_strain_val_5_t {
    bit<16> value;
    bit<32> value1;
}
header parde_strain_val_6_t {
    bit<32> value;
    bit<32> value1;
}
header parde_strain_val_7_t {
    bit<16> value;
    bit<32> value1;
    bit<8>  value2;
}
header parde_strain_val_8_t {
    bit<32> value;
    bit<8>  value1;
    bit<32> value2;
}

#endif // DIAG_PARDE_STRAIN

#ifdef DIAG_PHV_STRESS_ENABLE

@pa_container_size("ingress", "hdr.phv_stress_hdr.f0", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f1", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f2", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f3", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f4", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f5", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f6", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f7", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f8", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f9", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f10", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f11", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f12", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f13", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f14", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f15", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f16", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f17", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f18", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.f19", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f20", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f21", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f22", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f23", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f24", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f25", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.f26", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f27", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f28", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f29", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f30", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f31", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f32", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f33", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f34", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f36", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f37", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f38", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f39", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f40", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f41", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f42", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f43", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f44", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f45", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f46", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f47", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.f48", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f0", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f1", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f2", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f3", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f4", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f5", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f6", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f7", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f8", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f9", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f10", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f11", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f12", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f13", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f14", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f15", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f16", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f17", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f18", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f19", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f20", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f21", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f22", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f23", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f24", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f25", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f26", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f27", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f28", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f29", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f30", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f31", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f32", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f33", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f34", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f36", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f37", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f38", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f39", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f40", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.f41", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f42", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f43", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f44", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f45", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f46", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f47", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.f48", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e0", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e1", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e2", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e3", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e4", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e5", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e6", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e7", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e8", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e9", 8) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.e10", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e11", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e12", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e13", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e14", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e15", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e16", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e17", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e18", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e19", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e20", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e21", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e22", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e23", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e24", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e25", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e26", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.e27", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e0", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e1", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e2", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e3", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e4", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e5", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e6", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e7", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e8", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e9", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e10", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e11", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e12", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e13", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e14", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e15", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e16", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e17", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e18", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e19", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e20", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e21", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e22", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.e23", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e24", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e25", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e26", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.e27", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g0", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g1", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g2", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g3", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g4", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g5", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g6", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g7", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g8", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g9", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g10", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g11", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g12", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g13", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g14", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g15", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g16", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g17", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g18", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g19", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g20", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g21", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g22", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g23", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g24", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g25", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g26", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g27", 32) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.g28", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g29", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.g30", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g0", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g1", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g2", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g3", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g4", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g5", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g6", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g7", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g8", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g9", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g10", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g11", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g12", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g13", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g14", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g15", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g16", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g17", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g18", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g19", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g20", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g21", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g22", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g23", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g24", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g25", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g26", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g27", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g28", 32) 
@pa_container_size("egress", "hdr.phv_stress_hdr.g29", 32)
@pa_container_size("egress", "hdr.phv_stress_hdr.g30", 32)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h0", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h1", 8) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.h2", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h3", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h4", 8) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.h5", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h6", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.h7", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h0", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h1", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h2", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.h3", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h4", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h5", 8)
@pa_container_size("egress", "hdr.phv_stress_hdr.h6", 8) 
@pa_container_size("egress", "hdr.phv_stress_hdr.h7", 8)
@pa_container_size("ingress", "hdr.phv_stress_hdr.i0", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i1", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.i2", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.i3", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i4", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i5", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i6", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i7", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i8", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i9", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i10", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.i11", 16)
@pa_container_size("ingress", "hdr.phv_stress_hdr.i12", 16) 
@pa_container_size("ingress", "hdr.phv_stress_hdr.i13", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.i0", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.i1", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i2", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i3", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i4", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i5", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.i6", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.i7", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i8", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i9", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i10", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i11", 16) 
@pa_container_size("egress", "hdr.phv_stress_hdr.i12", 16)
@pa_container_size("egress", "hdr.phv_stress_hdr.i13", 16) 

header phv_stress_hdr_t {
    bit<16> f0;
    bit<16> f1;
    bit<16> f2;
    bit<16> f3;
    bit<8>  e0;
    bit<8>  e1;
    bit<16> f4;
    bit<16> f5;
    bit<16> f6;
    bit<16> f7;
    bit<8>  e2;
    bit<8>  e3;
    bit<8>  e4;
    bit<8>  e5;
    bit<8>  e6;
    bit<8>  e7;
    bit<16> f8;
    bit<16> f9;
    bit<16> f10;
    bit<16> f11;
    bit<16> f12;
    bit<8>  e8;
    bit<8>  e9;
    bit<8>  e10;
    bit<8>  e11;
    bit<16> f13;
    bit<16> f14;
    bit<16> f15;
    bit<16> f16;
    bit<8>  e12;
    bit<8>  e13;
    bit<8>  e14;
    bit<8>  e15;
    bit<16> f17;
    bit<16> f18;
    bit<16> f19;
    bit<8>  e16;
    bit<8>  e17;
    bit<8>  e18;
    bit<8>  e19;
    bit<32> g4;
    bit<32> g5;
    bit<32> g6;
    bit<32> g7;
    bit<16> f20;
    bit<16> f21;
    bit<16> f22;
    bit<16> f23;
    bit<8>  e20;
    bit<8>  e21;
    bit<8>  e22;
    bit<8>  e23;
    bit<32> g8;
    bit<32> g9;
    bit<32> g10;
    bit<32> g11;
    bit<16> f24;
    bit<16> f25;
    bit<16> f26;
    bit<16> f27;
    bit<8>  e24;
    bit<8>  e25;
    bit<8>  e26;
    bit<8>  e27;
    bit<32> g12;
    bit<32> g13;
    bit<32> g14;
    bit<32> g15;
    bit<16> f28;
    bit<16> f29;
    bit<16> f30;
    bit<16> f31;
    bit<32> g16;
    bit<32> g17;
    bit<32> g18;
    bit<32> g19;
    bit<16> f32;
    bit<16> f33;
    bit<16> f34;
    bit<32> g20;
    bit<32> g21;
    bit<32> g22;
    bit<32> g23;
    bit<16> f36;
    bit<16> f37;
    bit<16> f38;
    bit<16> f39;
    bit<32> g24;
    bit<32> g25;
    bit<32> g26;
    bit<32> g27;
    bit<16> f40;
    bit<16> f41;
    bit<16> f42;
    bit<16> f43;
    bit<16> f44;
    bit<32> g28;
    bit<32> g29;
    // Ethernet + stress hdr at this point is 238B

    // PATTERN_SHIFT stop headers at 240B
#ifdef DIAG_PATTERN_SHIFT_ENABLE
    pad: 16;
#else
    bit<32> g30;
    bit<16> f45;
    bit<16> f46;
    bit<16> f47;
    bit<16> f48;
    bit<32> g0;
    bit<32> g1;
    bit<32> g2;
    bit<32> g3;
    bit<8>  h0;
    bit<8>  h1;
    bit<8>  h2;
    bit<8>  h3;
    bit<16> i0;
    bit<16> i1;
    bit<16> i2;
    bit<16> i3;
    bit<8>  h4;
    bit<8>  h5;
    bit<8>  h6;
    bit<8>  h7;
    bit<16> i4;
    bit<16> i5;
    bit<16> i6;
    bit<16> i7;
    bit<16> i8;
    bit<16> i9;
    bit<16> i10;
    bit<16> i11;
    bit<16> i12;
    bit<16> i13;
#endif // DIAG_PATTERN_SHIFT_ENABLE
}

header left_shift_hdr_t {
    bit<16> f0;
    bit<16> f1;
    bit<32> f11;
    bit<32> f2;
    bit<32> f22;
    bit<32> f3;
    bit<32> f33;
    bit<32> f4;
    bit<16> f5;
    bit<8>  f6;
}

header right_shift_hdr_t {
    bit<8> f1;
}

#endif // DIAG_PHV_STRESS_ENABLE

struct headers_t {
    pktgen_timer_header_t                          timer;
    @name(".bridged_md") diag_bridged_metadata_t   bridged_md;
    @name(".ethernet") ethernet_t                  ethernet;
    @name(".vlan_tag") vlan_tag_t                  vlan_tag;
    @name(".vlan_tag_1") vlan_tag_t                vlan_tag_1;
    @name(".ipv4") ipv4_t                          ipv4;
    @name(".ipv6") ipv6_t                          ipv6;
    @name(".tcp") tcp_t                            tcp;
    @name(".udp")  udp_t                           udp;
    icmp_t                                         icmp;
    fcoe_header_t                                  fcoe;
    gre_t                                          gre;
    ethernet_t                                     inner_ethernet;
    icmp_t                                         inner_icmp;
    ipv4_t                                         inner_ipv4;
    ipv6_t                                         inner_ipv6;
    tcp_t                                          inner_tcp;
    udp_t                                          inner_udp;
    nsh_t                                          nsh;
    nsh_context_t                                  nsh_context;
    nvgre_t                                        nvgre;
    roce_header_t                                  roce;
    trill_t                                        trill;
    vntag_t                                        vntag;
    mpls_t                                         mpls;
    @name(".mau_bus_stress_hdr") mau_bus_stress_hdr_t mau_bus_stress_hdr;
#ifdef DIAG_PARDE_STRAIN
    @name(".parde_strain") parde_strain_t parde_strain;
    @name(".parde_strain_val_1") parde_strain_val_1_t parde_strain_val_1;
    @name(".parde_strain_val_2") parde_strain_val_2_t parde_strain_val_2;
    @name(".parde_strain_val_3") parde_strain_val_3_t parde_strain_val_3;
    @name(".parde_strain_val_4") parde_strain_val_4_t parde_strain_val_4;
    @name(".parde_strain_val_5") parde_strain_val_5_t parde_strain_val_5;
    @name(".parde_strain_val_6") parde_strain_val_6_t parde_strain_val_6;
    @name(".parde_strain_val_7") parde_strain_val_7_t parde_strain_val_7;
    @name(".parde_strain_val_8") parde_strain_val_8_t parde_strain_val_8;
#endif
#ifdef DIAG_PHV_STRESS_ENABLE
    @name(".left_shift_hdr") left_shift_hdr_t         left_shift_hdr;
    @name(".right_shift_hdr") right_shift_hdr_t       right_shift_hdr;
    @name(".phv_stress_hdr") phv_stress_hdr_t         phv_stress_hdr;
#endif
} // headers_t

#endif /* _DIAG_HEADERS_ */
