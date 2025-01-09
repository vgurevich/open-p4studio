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

#include <core.p4>
#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

#include "common/headers.p4"
#include "common/util.p4"


struct metadata_t {}

// ---------------------------------------------------------------------------
// Ingress parser
// ---------------------------------------------------------------------------
parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    TofinoIngressParser() tofino_parser;

    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select (hdr.ethernet.ether_type) {
	    ETHERTYPE_IPV4 : parse_ipv4;
	    ETHERTYPE_IPV6 : parse_ipv6;
            default : accept;
	}
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------
control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

    apply {
         pkt.emit(hdr);
    }
}

control SwitchIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action hit(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    action nop() {
    }

    table forward_single_field_single_slice {
        key = {
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[15:7] : exact;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_single_field_single_slice_ternary {
        key = {
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[16:7] : ternary;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    @alpm(1)
#if __TARGET_TOFINO__ == 2
    @alpm_partitions(2048)
    @alpm_subtrees_per_partition(2)
#else
    @alpm_partitions(1024)
    @alpm_subtrees_per_partition(2)
#endif
    table forward_single_field_single_slice_lpm {
        key = {
            hdr.ipv6.dst_addr[94:45] : lpm;
            ig_intr_md.ingress_port : exact;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_single_field_single_slice_range {
        key = {
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[76:66] : range;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_single_field_multi_slice {
        key = {
            hdr.ipv6.dst_addr[41:5] : exact;
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[127:54] : ternary;
            hdr.ipv6.dst_addr[53:42] : ternary;
      	    hdr.ipv6.dst_addr[4:0] : exact;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_single_field_multi_slice_alias {
        key = {
            hdr.ipv6.dst_addr[23:9] : exact @name("ipv6.dstaddr_23_9_");
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[57:28] : ternary @name("ipv6.dstaddr_57_28_");
            hdr.ipv6.dst_addr[100:70] : ternary @name("ipv6.dstaddr_100_70_");
      	    hdr.ipv6.dst_addr[7:3] : exact;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_multi_field_multi_slice {
        key = {
            hdr.ipv6.src_addr[84:50] : exact;
            hdr.ipv6.src_addr[43:0] : ternary;
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[127:16] : ternary;
            hdr.ipv6.dst_addr[5:5] : exact;
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_multi_field_multi_slice_alias {
        key = {
            hdr.ipv6.src_addr[53:19] : exact;
            hdr.ipv6.src_addr[106:80] : ternary @name("ipv6.srcaddr_106_80_");
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[81:60] : ternary;
            hdr.ipv6.dst_addr[23:9] : exact @name("ipv6.dstaddr_23_9_");
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    table forward_single_field_single_slice_lpm_alias {
        key = {
            ig_intr_md.ingress_port : exact;
            hdr.ipv6.dst_addr[127:89] : lpm @name("ipv6.dstaddr_127_89_");
        }

        actions = {
            hit;
	    nop;
        }

        const default_action = nop;
        size = 1024;
    }

    apply {
        forward_single_field_single_slice.apply();
        forward_single_field_single_slice_ternary.apply();
 	      forward_single_field_single_slice_lpm.apply();
 	      forward_single_field_single_slice_range.apply();
	      forward_single_field_multi_slice.apply();
 	      forward_multi_field_multi_slice.apply();

        forward_single_field_single_slice_lpm_alias.apply();
	      forward_single_field_multi_slice_alias.apply();
 	      forward_multi_field_multi_slice_alias.apply();

        // No need for egress processing, skip it and use empty controls for egress.
        ig_tm_md.bypass_egress = 1w1;
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         EmptyEgressParser(),
         EmptyEgress(),
         EmptyEgressDeparser()) pipe;

Switch(pipe) main;
