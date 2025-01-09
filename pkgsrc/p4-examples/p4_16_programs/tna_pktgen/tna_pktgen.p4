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

#if __TARGET_TOFINO__ == 2
#include <t2na.p4>
#else
#include <tna.p4>
#endif

header ethernet_h {
    bit<48> dmac;
    bit<48> smac;
    bit<16> etype;
}

#if __TARGET_TOFINO__ == 2
header context_h {
    bit<128> f;
}
#endif

/* This P4 will add a 4B (16B on Tofino-2) internal header to packets when they
 * are recirculated.  This size is chosen since that is the size the Packet
 * Generator will match on for Recirculation triggers. */
#if __TARGET_TOFINO__ == 2
    #define RECIRC_TAG_WIDTH 128
#else
    #define RECIRC_TAG_WIDTH 32
#endif
header recirc_h {
    bit<RECIRC_TAG_WIDTH> tag;
}

struct headers {
    /* The Tofino Packet Generator always adds a 6-byte header at the begining
     * of the generated packet.  On Tofino-2 it may also add an additional
     * "context" header immediately after the 6-byte header which carries user
     * defined values from the packet which triggered the Packet Generator.
     * The formats for the 6-byte header are defined in:
     *     <install>/share/p4c/p4include/tofino1_base.p4
     *     <install>/share/p4c/p4include/tofino2_base.p4
     * These files are automatically included when either tna.p4 or t2na.p4 are
     * included.

    /* The timer header is used by one-shot timer and periodic timer Packet
     * Generator applications, it carries the following fields:
     *   - Pipe-id of the Packet Generator which created the packet
     *   - App-id of the Packet Generator Application which created the packet
     *   - Batch-id of the generated packet
     *   - Packet-id (within the batch) of the generated packet
     */
    pktgen_timer_header_t timer;

    /* The port-down header is used by port-down Packet Generator Applications,
     * it carries the following fields:
     *   - Pipe-id of the Packet Generator which created the packet
     *   - App-id of the Packet Generator Application which created the packet
     *   - Port-id of the port which went down
     *   - Packet-id (within the single batch) of the generated packet
     * Since this header omits the batch-id and instead carries the port-id of
     * the port which went down port-down applications should use only a single
     * batch.
     */
    pktgen_port_down_header_t port_down;

    /* The recirc header is used by Recirc Packet Generator Applications,
     * it carries the following fields on Tofino-1:
     *   - Pipe-id of the Packet Generator which created the packet
     *   - App-id of the Packet Generator Application which created the packet
     *   - Key, a 24-bit user defined field taken from the packet which
     *     triggered the application.
     *   - Packet-id (within the single batch) of the generated packet
     * Since this header omits the batch-id and instead carries context data
     * from the packet which triggered the generator, applications should use
     * only a single batch.
     *
     * On Tofino-2 the "key" field is removed from the header and replaced with
     * the Batch-Id allowing recirc applications to generate multiple batches.
     * The first 16 bytes of the recirculated packet is then added to the
     * generated packet immediately after the 6-byte Packet Generator header.
     * In this example we will treat it as a single 16 byte field but other P4
     * programs are free to define it as multiple fields, however all fields
     * must add up to 128-bits, any padding fields must be explicitly defined in
     * the header to ensure it is 128-bits long.
     */
    pktgen_recirc_header_t recirc;
#if __TARGET_TOFINO__ == 2
    context_h recirc_context;
#endif

#if __TARGET_TOFINO__ == 2
    /* The recirc header is also used by Deparser Packet Generator Applications,
     * it carries the following fields on Tofino-2:
     *   - Pipe-id of the Packet Generator which created the packet
     *   - App-id of the Packet Generator Application which created the packet
     *   - Batch-id of the generated packet
     *   - Packet-id (within the single batch) of the generated packet
     * Immediately after the 6-byte Packet Generator header are 16 bytes of
     * context set by the packet which triggered the application.  This data is
     * specified in an emit call on a Pktgen extern in the ingress deparser.
     */
    pktgen_recirc_header_t dprsr;
    context_h dprsr_context;
#endif

    recirc_h recirc_tag;
    ethernet_h ethernet;
}


/* For the per-port metadata (PORT_METADATA table) we will simply use a single
 * field of the maximum width. */
struct port_md_t {
#if __TARGET_TOFINO__ == 2
    bit<128> f;
#else
    bit<64> f;
#endif
}

struct ing_metadata_t {
    port_md_t port_md;
    bit<128> dprsr_trig_ctx_data;
}

struct pvs_pgen_key_t {
#if __TARGET_TOFINO__ == 2
    bit<2> pad;
    bit<2> pipe_id;
    bit<4> app_id;
#else
    bit<3> pad;
    bit<2> pipe_id;
    bit<3> app_id;
#endif
}
struct pvs_recirc_key_t {
    PortId_t port;
    bit<8> recirc_tag_msb;
}

parser IPrsr(packet_in packet, 
             out headers hdr, 
             out ing_metadata_t md,
             out ingress_intrinsic_metadata_t intr_md) {

    value_set<bit<8>>(1) pgen_timer;
    value_set<bit<8>>(2) pgen_port_down;
    value_set<bit<8>>(1) pgen_recirc;
    value_set<bit<8>>(1) pgen_dprsr;
    value_set<pvs_recirc_key_t>(1) recirc;

    state start {
        packet.extract(intr_md);
        md.port_md = port_metadata_unpack<port_md_t>(packet);

        /* This lookahead and the select statements following need to align with
         * the format of a pvs_pgen_key_t. */
        transition select(packet.lookahead<bit<8>>()) {
            pgen_timer     : parse_pktgen_timer;
            pgen_port_down : parse_pktgen_port_down;
            pgen_recirc    : parse_pktgen_recirc;
#if __TARGET_TOFINO__ == 2
            pgen_dprsr     : parse_pktgen_dprsr;
#endif
            default        : parse_recirc_or_ethernet;
        }
    }

    state parse_recirc_or_ethernet {
        transition select(intr_md.ingress_port, packet.lookahead<bit<8>>()) {
            recirc         : parse_recirc_pkt;
            default        : parse_ethernet;
        }
    }
    state parse_pktgen_timer {
        packet.extract(hdr.timer);
        transition parse_ethernet;
    }
    state parse_pktgen_port_down {
        packet.extract(hdr.port_down);
        transition parse_ethernet;
    }
    state parse_pktgen_recirc {
        packet.extract(hdr.recirc);
#if __TARGET_TOFINO__ == 2
        packet.extract(hdr.recirc_context);
#endif
        transition parse_ethernet;
    }
#if __TARGET_TOFINO__ == 2
    state parse_pktgen_dprsr {
        packet.extract(hdr.dprsr);
        packet.extract(hdr.dprsr_context);
        transition parse_ethernet;
    }
#endif
    state parse_recirc_pkt {
        packet.extract(hdr.recirc_tag);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}


control IDprsr(packet_out pkt,
               inout headers hdr,
               in ing_metadata_t ig_md,
               in ingress_intrinsic_metadata_for_deparser_t dprsr_md) {
#if __TARGET_TOFINO__ == 2
    Pktgen() pgen;
#endif
    apply {
#if __TARGET_TOFINO__ == 2
        if (dprsr_md.pktgen == 1w1) {
            pgen.emit(ig_md.dprsr_trig_ctx_data);
        }
#endif
        pkt.emit(hdr.recirc_tag);
        pkt.emit(hdr.ethernet);
    }
}


control Ing(
        inout headers hdr, 
        inout ing_metadata_t ig_md,
        in ingress_intrinsic_metadata_t intr_md,
        in ingress_intrinsic_metadata_from_parser_t intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t intr_tm_md) {
    /* The ingress processing will run a series of tables depending on the
     * packet type (packet type is determined in parser and checked in the
     * ingress through header validity) as follows:
     *
     * Packet Generator Timer App Packet
     * Run the t_timer_app table to verify the fields in the Packet Generator
     * header, then remove the header and forward to an egress port.  This
     * verifies the pipe-id, app-id, batch-id and packet-id fields.
     *
     * Packet Generator Port Down App Packet
     * Run the t_port_down_app table to verify the fields in the Packet
     * Generator header, then remove the header and forward to an egress port.
     * This verifies the pipe-id, app-id, packet-id, and port-id fields.
     *
     * Packet Generator Recirculation App Packet
     * Run the t_recirc_app table to verify the fields in the Packet Generator
     * header, then remove the header and forward to an egress port.
     * This verifies the pipe-id, app-id, packet-id, and "key" (24-bits of
     * context from recirculated packet) fields on Tofino-1.
     * This verifies the pipe-id, app-id, batch-id, packet-id fields, and 16B of
     * context from the recirculated packet on Tofino-2.
     *
     * Packet Generator Deparser App Packet (Tofino-2 only)
     * Run the t_dprsr_app table to verify the fields in the Packet Generator
     * header, then remove the header and forward to an egress port.
     * This verifies the pipe-id, app-id, batch-id, packet-id fields, and 16B of
     * context from the triggering packet on Tofino-2.
     *
     * Recirculated packet
     * Run the t_recirc table to verify the expected internal header was added
     * to the packet, then remove it and forward to an egress port.
     *
     * Normal packet
     * Run the t_handle_external_packet table, depending on the test case being
     * exercised the packet may be recirculated (to trigger recirc apps) or may
     * setup intrinsic metadata to trigger a deparser app.
     */

    action drop() { intr_dprsr_md.drop_ctl = 1; }
    action fwd(PortId_t dst_port) { intr_tm_md.ucast_egress_port = dst_port; }


    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) timer_app_ctr;
    action count_timer_app_and_fwd(PortId_t dst_port) {
        timer_app_ctr.count();
        hdr.timer.setInvalid();
        fwd(dst_port);
    }
    action count_timer_app_and_drop() {
        timer_app_ctr.count();
        drop();
    }
    table t_timer_app {
        key = {
            hdr.timer.pipe_id   : exact;
            hdr.timer.app_id    : exact;
            hdr.timer.batch_id  : exact;
            hdr.timer.packet_id : exact;
        }
        actions = {
            count_timer_app_and_fwd();
            count_timer_app_and_drop();
        }
        const default_action = count_timer_app_and_drop();
        size = 4096;
        counters = timer_app_ctr;
    }

    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) port_down_app_ctr;
    action count_port_down_app_and_fwd(PortId_t dst_port) {
        port_down_app_ctr.count();
        hdr.port_down.setInvalid();
        fwd(dst_port);
    }
    action count_port_down_app_and_drop() {
        port_down_app_ctr.count();
        drop();
    }
    table t_port_down_app {
        key = {
            hdr.port_down.pipe_id   : exact;
            hdr.port_down.app_id    : exact;
            hdr.port_down.port_num  : exact;
            hdr.port_down.packet_id : exact;
        }
        actions = {
            count_port_down_app_and_fwd();
            count_port_down_app_and_drop();
        }
        const default_action = count_port_down_app_and_drop();
        size = 4096;
        counters = port_down_app_ctr;
    }

    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) recirc_app_ctr;
    action count_recirc_app_and_fwd(PortId_t dst_port) {
        recirc_app_ctr.count();
        hdr.recirc.setInvalid();
        fwd(dst_port);
    }
    action count_recirc_app_and_drop() {
        recirc_app_ctr.count();
        drop();
    }
    table t_recirc_app {
        key = {
#if __TARGET_TOFINO__ == 2
            hdr.recirc.pipe_id   : exact;
            hdr.recirc.app_id    : exact;
            hdr.recirc.batch_id  : exact;
            hdr.recirc.packet_id : exact;
            hdr.recirc_context.f : exact;
#else
            hdr.recirc.pipe_id   : exact;
            hdr.recirc.app_id    : exact;
            hdr.recirc.key       : exact;
            hdr.recirc.packet_id : exact;
#endif
        }
        actions = {
            count_recirc_app_and_fwd();
            count_recirc_app_and_drop();
        }
        const default_action = count_recirc_app_and_drop();
        size = 4096;
        counters = recirc_app_ctr;
    }

#if __TARGET_TOFINO__ == 2
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) dprsr_app_ctr;
    action count_dprsr_app_and_fwd(PortId_t dst_port) {
        dprsr_app_ctr.count();
        hdr.dprsr.setInvalid();
        fwd(dst_port);
    }
    action count_dprsr_app_and_drop() {
        dprsr_app_ctr.count();
        drop();
    }
    table t_dprsr_app {
        key = {
            hdr.dprsr.pipe_id   : exact;
            hdr.dprsr.app_id    : exact;
            hdr.dprsr.batch_id  : exact;
            hdr.dprsr.packet_id : exact;
            hdr.dprsr_context.f : exact;
        }
        actions = {
            count_dprsr_app_and_fwd();
            count_dprsr_app_and_drop();
        }
        const default_action = count_dprsr_app_and_drop();
        size = 4096;
        counters = dprsr_app_ctr;
    }
#endif

    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) recirc_ctr;
    action count_recirc_and_fwd(PortId_t dst_port) {
        recirc_ctr.count();
        hdr.recirc_tag.setInvalid();
        fwd(dst_port);
    }
    action count_recirc_and_drop() {
        recirc_ctr.count();
        drop();
    }
    table t_recirc {
        key = {
            hdr.recirc_tag.tag: exact;
        }
        actions = {
            count_recirc_and_fwd();
            count_recirc_and_drop();
        }
        const default_action = count_recirc_and_drop();
        size = 4096;
        counters = recirc_ctr;
    }

    action fwd_recirc(PortId_t dst_port, bit<RECIRC_TAG_WIDTH> tag) {
        intr_tm_md.ucast_egress_port = dst_port;
        hdr.recirc_tag.setValid();
        hdr.recirc_tag.tag = tag;
    }
#if __TARGET_TOFINO__ == 2
    action dprsr_trig_and_fwd(PortId_t dst_port, bit<128> ctx, bit<10> pkt_buf_start, bit<14> pkt_len) {
        intr_tm_md.ucast_egress_port = dst_port;
        intr_dprsr_md.pktgen = 1;
        /* Deparser triggers for the Packet Generator must specify the generated
         * packet offset and length through metadata. */
        intr_dprsr_md.pktgen_address = 4w0 ++ pkt_buf_start;
        intr_dprsr_md.pktgen_length = pkt_len[9:0];
        ig_md.dprsr_trig_ctx_data = ctx;
    }
#endif
    table t_handle_external_packet {
        key = {
            hdr.ethernet.dmac : exact;
        }
        actions = {
            fwd();
            fwd_recirc();
#if __TARGET_TOFINO__ == 2
            dprsr_trig_and_fwd();
#endif
            drop();
        }
        const default_action = drop();
        size = 1024;
    }

    apply {
        if (hdr.timer.isValid()) {
            t_timer_app.apply();
        } else if (hdr.port_down.isValid()) {
            t_port_down_app.apply();
        } else if (hdr.recirc.isValid()) {
            t_recirc_app.apply();
#if __TARGET_TOFINO__ == 2
        } else if (hdr.dprsr.isValid()) {
            t_dprsr_app.apply();
#endif
        } else if (hdr.recirc_tag.isValid()) {
            t_recirc.apply();
        } else {
            t_handle_external_packet.apply();
        }

        intr_tm_md.bypass_egress = 1w1;
    }
}

/* Empty controls for the egress side, all packets bypass the egress pipeline. */
struct empty_header_t {}
struct empty_metadata_t {}
parser EPrsr(packet_in pkt,
             out empty_header_t hdr,
             out empty_metadata_t eg_md,
             out egress_intrinsic_metadata_t eg_intr_md) {
    state start { transition accept; }
}
control EDprsr(packet_out pkt,
               inout empty_header_t hdr,
               in empty_metadata_t eg_md,
               in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {}
}

control Egr(inout empty_header_t hdr,
            inout empty_metadata_t eg_md,
            in egress_intrinsic_metadata_t eg_intr_md,
            in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
            inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
            inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {}
}

Pipeline(IPrsr(), Ing(), IDprsr(), EPrsr(), Egr(), EDprsr()) pipe;
Switch(pipe) main;
