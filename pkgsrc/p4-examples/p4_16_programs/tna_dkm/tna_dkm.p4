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

#include <tna.p4>
#include "common/headers.p4"


struct metadata_t {}

// ---------------------------------------------------------------------------
// Ingress parser
// ---------------------------------------------------------------------------
parser IngPrsr(packet_in pkt,
               out header_t hdr,
               out metadata_t ig_md,
               out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Ingress Pipeline
// ---------------------------------------------------------------------------
control Ing(inout header_t hdr,
            inout metadata_t ig_md,
            in ingress_intrinsic_metadata_t ig_intr_md,
            in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
            inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
            inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    action fwd(PortId_t port) {
        ig_tm_md.ucast_egress_port = port;
    }

    @dynamic_table_key_masks(1)
    table t1 {
        key = {
            ig_intr_md.ingress_port : exact;
            hdr.ipv4.dst_addr       : exact;
            hdr.ipv4.src_addr       : exact;
            hdr.ipv4.protocol       : exact;
        }

        actions = {
            fwd;
        }

        size = 8192;
    }

    apply {
        t1.apply();
    }
}

// ---------------------------------------------------------------------------
// Ingress Deparser
// ---------------------------------------------------------------------------
control IngDprsr(packet_out pkt,
                 inout header_t hdr,
                 in metadata_t ig_md,
                 in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {

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
            hdr.ipv4.dst_addr});

         pkt.emit(hdr);
    }
}

// ---------------------------------------------------------------------------
// Egress parser
// ---------------------------------------------------------------------------
parser EgrPrsr(packet_in pkt,
               out header_t hdr,
               out metadata_t ig_md,
               out egress_intrinsic_metadata_t intr_md) {

    state start {
        pkt.extract(intr_md);
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

// ---------------------------------------------------------------------------
// Egress Pipeline
// ---------------------------------------------------------------------------
control Egr(inout header_t hdr,
            inout metadata_t md,
            in egress_intrinsic_metadata_t intr_md,
            in egress_intrinsic_metadata_from_parser_t intr_md_from_prsr,
            inout egress_intrinsic_metadata_for_deparser_t intr_dprs_md,
            inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {

    Counter<bit<32>, bit<12>>(4096, CounterType_t.PACKETS) cntr;

    action count(bit<12> i) { cntr.count(i); }
    @dynamic_table_key_masks(1)
    table t2 {
        key = {
            hdr.ethernet.dst_addr[31:0] : exact;
            intr_md.egress_port   : exact;
        }

        actions = {
            count;
        }

        const default_action = count(1);

        size = 4096;
    }

    apply {
        t2.apply();
    }
}

// ---------------------------------------------------------------------------
// Egress Deparser
// ---------------------------------------------------------------------------
control EgrDprsr(packet_out pkt,
                 inout header_t hdr,
                 in metadata_t md,
                 in egress_intrinsic_metadata_for_deparser_t intr_dprs_md) {


    apply {
         pkt.emit(hdr);
    }
}


Pipeline(IngPrsr(), Ing(), IngDprsr(),
         EgrPrsr(), Egr(), EgrDprsr()) pipe;

Switch(pipe) main;
