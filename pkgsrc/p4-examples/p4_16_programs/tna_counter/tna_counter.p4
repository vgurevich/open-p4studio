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
            default : reject;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
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

    // Create indirect counter
    Counter<bit<32>, PortId_t>(
        512, CounterType_t.PACKETS_AND_BYTES) indirect_counter;

    // Create direct counters
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) direct_counter;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) direct_counter_2;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) direct_counter_3;

    action hit(PortId_t port) {
        // Call direct counter. Note that no index parameter is required because
        // the index is implicitely generated from the entry of the the 
        // associated match table.
        direct_counter.count();
        ig_intr_tm_md.ucast_egress_port = port;
    }

    action hit_dst(PortId_t port) {
        // Call indirect counter. Note that for indirect counters an index 
        // parameter must be provided.
        indirect_counter.count(port);
        ig_intr_tm_md.ucast_egress_port = port;
        ig_intr_dprsr_md.drop_ctl = 0x0; // clear drop packet
    }

    action miss() {
        ig_intr_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }
    action nop() {
    }

    table forward {
        key = {
            hdr.ethernet.src_addr : ternary;
        }

        actions = {
            hit;
            @defaultonly nop;
        }

        size = 1024;
        const default_action = nop;
        // Associate this table with a direct counter
        counters = direct_counter;
    }

    action hit_forward_exact(PortId_t port) {
        // Call direct counter. Note that no index parameter is required because
        // the index is implicitely generated from the entry of the the 
        // associated match table.
        direct_counter_2.count();
        ig_intr_tm_md.ucast_egress_port = port;
    }

    table forward_exact {
        key = {
            hdr.ethernet.src_addr : exact;
        }

        actions = {
            hit_forward_exact;
            @defaultonly nop;
        }

        size = 1024;
        const default_action = nop;
        // Associate this table with a direct counter
        counters = direct_counter_2;
    }

    table forward_dst {
        key = {
            hdr.ethernet.dst_addr : exact;
        }

        actions = {
            hit_dst;
            @defaultonly nop;
        }

        const default_action = nop;
        size = 1024;
        // No association a counter required, since an indirect counter is used.
    }

    // Table forward_or_drop will count both dropped and forwarded packets
    action set_port(PortId_t port) {
        direct_counter_3.count();
        ig_intr_tm_md.ucast_egress_port = port;
    }

    action drop() {
        direct_counter_3.count();
        ig_intr_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }
 
    // Table uses 2 actions, with and wihtout action data
    table forward_or_drop {
        key = {
            hdr.ethernet.dst_addr : exact;
        }

        actions = {
            set_port;
            drop;
            @defaultonly nop;
        }

        const default_action = nop;
        size = 1024;
        counters = direct_counter_3;
    }

    apply {
        forward.apply();
        forward_exact.apply();
        forward_dst.apply();
        forward_or_drop.apply();

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
