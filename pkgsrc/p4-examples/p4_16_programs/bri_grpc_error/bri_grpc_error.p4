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

/* -*- P4_16 -*- */
#include <core.p4>
#include <tna.p4>

/* constants and types */
const bit<16> ETHERTYPE_VLAN = 0x8100;
const bit<16> ETHERTYPE_IPV4 = 0x0800;
/* Table Sizes */
const int IPV4_HOST_SIZE = 65536;

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/

header ethernet_h {
    bit<48>   dst_addr;
    bit<48>   src_addr;
    bit<16>   ether_type;
}

header vlan_tag_h {
    bit<3>   pcp;
    bit<1>   cfi;
    bit<12>  vid;
    bit<16>  ether_type;
}

header ipv4_h {
    bit<4>   version;
    bit<4>   ihl;
    bit<8>   diffserv;
    bit<16>  total_len;
    bit<16>  identification;
    bit<3>   flags;
    bit<13>  frag_offset;
    bit<8>   ttl;
    bit<8>   protocol;
    bit<16>  hdr_checksum;
    bit<32>  src_addr;
    bit<32>  dst_addr;
}

/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

struct my_ingress_headers_t {
    ethernet_h   ethernet;
    vlan_tag_h   vlan_tag;
    ipv4_h       ipv4;
}

struct empty_metadata_t {}
struct empty_headers_t {}

/* Ingress Parser */
parser IngressParser(
    packet_in                           pkt,
    out my_ingress_headers_t            hdr,
    out empty_metadata_t                meta,
    out ingress_intrinsic_metadata_t    ig_intr_md)
{
     state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_VLAN:  parse_vlan_tag;
            ETHERTYPE_IPV4:  parse_ipv4;
            default: accept;
        }
    }

    state parse_vlan_tag {
        pkt.extract(hdr.vlan_tag);
        transition select(hdr.vlan_tag.ether_type) {
            ETHERTYPE_IPV4:  parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }

}

/* Match-Action */
control Ingress(
    inout my_ingress_headers_t                       hdr,
    inout empty_metadata_t                           meta,
    in    ingress_intrinsic_metadata_t               ig_intr_md,
    in    ingress_intrinsic_metadata_from_parser_t   ig_prsr_md,
    inout ingress_intrinsic_metadata_for_deparser_t  ig_dprsr_md,
    inout ingress_intrinsic_metadata_for_tm_t        ig_tm_md)
{
    action send(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    action drop() {
        ig_dprsr_md.drop_ctl = 1;
    }

    table ipv4_host {
        key = { hdr.ipv4.dst_addr : exact; }
        actions = {
            send;
            drop;
        }

        size = IPV4_HOST_SIZE;
    }

    Hash<bit<16>>(HashAlgorithm_t.CRC16) sel_hash;
    // Action Profile Size = max group size x max number of groups
    ActionProfile(100) ap;
    ActionSelector(ap, // action profile
                   sel_hash, // hash extern
                   SelectorMode_t.FAIR, // Selector algorithm
                   8, // max group size
                   100 // max number of groups
                   ) sel;

    table ipv4_selector {
        key = { 
          ig_intr_md.ingress_port : exact;
          hdr.ipv4.dst_addr : selector;
        }

        actions = {
            send;
            drop;
        }

        const default_action = drop;
        size = 512;
        implementation = sel;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            ipv4_host.apply();
            ipv4_selector.apply();
        }
    }
}

/* Ingress Deparser */
control IngressDeparser(
    packet_out                                          pkt,
    inout   my_ingress_headers_t                        hdr,
    in      empty_metadata_t                            meta,
    in      ingress_intrinsic_metadata_for_deparser_t   ig_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}

/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

/* Egress Parser */
parser EgressParser(
    packet_in                           pkt,
    out empty_headers_t                 hdr,
    out empty_metadata_t                meta,
    out egress_intrinsic_metadata_t     eg_intr_md)
{
    state start {
        pkt.extract(eg_intr_md);
        transition accept;
    }
}

/* Match-Action */
control Egress(
    inout empty_headers_t                          hdr,
    inout empty_metadata_t                         meta,
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    apply {
    }
}

/* Egress Deparser */
control EgressDeparser(packet_out pkt,
    inout empty_headers_t                               hdr,
    in    empty_metadata_t                              meta,
    in    egress_intrinsic_metadata_for_deparser_t      eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;
