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
#if __TARGET_TOFINO__ == 3
#include <t3na.p4>
#elif __TARGET_TOFINO__ == 2
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
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {

    apply {
        pkt.emit(hdr);
    }
}

control SwitchIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {

    Alpm(number_partitions = 1024, subtrees_per_partition = 2) alpm;
    Alpm(number_partitions = 1024, subtrees_per_partition = 2, atcam_subset_width = 18, shift_granularity = 1) alpm_ipv4_subset_key;
    Alpm(number_partitions = 1024, subtrees_per_partition = 2, atcam_subset_width = 32, shift_granularity = 2) alpm_ipv6_subset_key;

    bit<10> vrf;

    action hit(PortId_t port) {
        ig_intr_tm_md.ucast_egress_port = port;
    }

    action miss() {
        ig_intr_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }

    action route(PortId_t dst_port) {
        ig_intr_tm_md.ucast_egress_port = dst_port;
        ig_intr_dprsr_md.drop_ctl = 0x0;
    }

    table ipv4_subset_key {
        key = {
            vrf : exact;
            hdr.ipv4.dst_addr : lpm;
        }

        actions = {
            route;
        }

        size = 10*1024;
        implementation = alpm_ipv4_subset_key;
    }

    table ipv6_subset_key {
        key = {
            vrf : exact;
            hdr.ipv6.dst_addr[127:64] : lpm;
        }

        actions = {
            route;
        }

        size = 10*1024;
        implementation = alpm_ipv6_subset_key;
    }

    @alpm_atcam_exclude_field_msbs(vrf)
    @alpm_atcam_exclude_field_msbs(hdr.ipv4.dst_addr, 7)
    table ipv4_exclude_msb {
        key = {
            vrf : exact;
            hdr.ipv4.dst_addr : lpm;
        }

        actions = {
            route;
        }

        size = 10*1024;
        implementation = alpm;
    }

    @alpm_atcam_exclude_field_msbs(vrf)
    @alpm_atcam_exclude_field_msbs(hdr.ipv6.dst_addr, 15)
    table ipv6_exclude_msb {
        key = {
            vrf : exact;
            hdr.ipv6.dst_addr : lpm;
        }

        actions = {
            route;
        }

        size = 10*1024;
        implementation = alpm;
    }

    apply {
        vrf = 10w0;
        ipv4_subset_key.apply();
        ipv6_subset_key.apply();
        ipv4_exclude_msb.apply();
        ipv6_exclude_msb.apply();

        // No need for egress processing, skip it and use empty controls for egress.
        ig_intr_tm_md.bypass_egress = 1w1;
    }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         EmptyEgressParser(),
         EmptyEgress(),
         EmptyEgressDeparser()) pipe;

Switch(pipe) main;
