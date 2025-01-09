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

#ifndef _FULL_PHV_
#define _FULL_PHV_

header big_h {
    bit<32> w0;
    bit<32> w1;
    bit<8> b0;
    bit<8> b1;
    bit<16> h0;
    bit<16> h1;
    bit<16> h2;
}

header rest_h {
    bit<32> w0;
    bit<32> w1;
    bit<8> b0;
    bit<16> h0;
    bit<16> h1;
}

//ig big
@pa_container_size("ingress", "hdr.big[0].w0", 32)
@pa_container_size("ingress", "hdr.big[0].w1", 32)
@pa_container_size("ingress", "hdr.big[0].b0", 8)
@pa_container_size("ingress", "hdr.big[0].b1", 8)
@pa_container_size("ingress", "hdr.big[0].h0", 16)
@pa_container_size("ingress", "hdr.big[0].h1", 16)
@pa_container_size("ingress", "hdr.big[0].h2", 16)

@pa_container_size("ingress", "hdr.big[1].w0", 32)
@pa_container_size("ingress", "hdr.big[1].w1", 32)
@pa_container_size("ingress", "hdr.big[1].b0", 8)
@pa_container_size("ingress", "hdr.big[1].b1", 8)
@pa_container_size("ingress", "hdr.big[1].h0", 16)
@pa_container_size("ingress", "hdr.big[1].h1", 16)
@pa_container_size("ingress", "hdr.big[1].h2", 16)

@pa_container_size("ingress", "hdr.big[2].w0", 32)
@pa_container_size("ingress", "hdr.big[2].w1", 32)
@pa_container_size("ingress", "hdr.big[2].b0", 8)
@pa_container_size("ingress", "hdr.big[2].b1", 8)
@pa_container_size("ingress", "hdr.big[2].h0", 16)
@pa_container_size("ingress", "hdr.big[2].h1", 16)
@pa_container_size("ingress", "hdr.big[2].h2", 16)

@pa_container_size("ingress", "hdr.big[3].w0", 32)
@pa_container_size("ingress", "hdr.big[3].w1", 32)
@pa_container_size("ingress", "hdr.big[3].b0", 8)
@pa_container_size("ingress", "hdr.big[3].b1", 8)
@pa_container_size("ingress", "hdr.big[3].h0", 16)
@pa_container_size("ingress", "hdr.big[3].h1", 16)
@pa_container_size("ingress", "hdr.big[3].h2", 16)

@pa_container_size("ingress", "hdr.big[4].w0", 32)
@pa_container_size("ingress", "hdr.big[4].w1", 32)
@pa_container_size("ingress", "hdr.big[4].b0", 8)
@pa_container_size("ingress", "hdr.big[4].b1", 8)
@pa_container_size("ingress", "hdr.big[4].h0", 16)
@pa_container_size("ingress", "hdr.big[4].h1", 16)
@pa_container_size("ingress", "hdr.big[4].h2", 16)

@pa_container_size("ingress", "hdr.big[5].w0", 32)
@pa_container_size("ingress", "hdr.big[5].w1", 32)
@pa_container_size("ingress", "hdr.big[5].b0", 8)
@pa_container_size("ingress", "hdr.big[5].b1", 8)
@pa_container_size("ingress", "hdr.big[5].h0", 16)
@pa_container_size("ingress", "hdr.big[5].h1", 16)
@pa_container_size("ingress", "hdr.big[5].h2", 16)

@pa_container_size("ingress", "hdr.big[6].w0", 32)
@pa_container_size("ingress", "hdr.big[6].w1", 32)
@pa_container_size("ingress", "hdr.big[6].b0", 8)
@pa_container_size("ingress", "hdr.big[6].b1", 8)
@pa_container_size("ingress", "hdr.big[6].h0", 16)
@pa_container_size("ingress", "hdr.big[6].h1", 16)
@pa_container_size("ingress", "hdr.big[6].h2", 16)

//ig big_c
@pa_container_size("ingress", "hdr.big_c[0].w0", 32)
@pa_container_size("ingress", "hdr.big_c[0].w1", 32)
@pa_container_size("ingress", "hdr.big_c[0].b0", 8)
@pa_container_size("ingress", "hdr.big_c[0].b1", 8)
@pa_container_size("ingress", "hdr.big_c[0].h0", 16)
@pa_container_size("ingress", "hdr.big_c[0].h1", 16)
@pa_container_size("ingress", "hdr.big_c[0].h2", 16)

@pa_container_size("ingress", "hdr.big_c[1].w0", 32)
@pa_container_size("ingress", "hdr.big_c[1].w1", 32)
@pa_container_size("ingress", "hdr.big_c[1].b0", 8)
@pa_container_size("ingress", "hdr.big_c[1].b1", 8)
@pa_container_size("ingress", "hdr.big_c[1].h0", 16)
@pa_container_size("ingress", "hdr.big_c[1].h1", 16)
@pa_container_size("ingress", "hdr.big_c[1].h2", 16)

@pa_container_size("ingress", "hdr.big_c[2].w0", 32)
@pa_container_size("ingress", "hdr.big_c[2].w1", 32)
@pa_container_size("ingress", "hdr.big_c[2].b0", 8)
@pa_container_size("ingress", "hdr.big_c[2].b1", 8)
@pa_container_size("ingress", "hdr.big_c[2].h0", 16)
@pa_container_size("ingress", "hdr.big_c[2].h1", 16)
@pa_container_size("ingress", "hdr.big_c[2].h2", 16)

@pa_container_size("ingress", "hdr.big_c[3].w0", 32)
@pa_container_size("ingress", "hdr.big_c[3].w1", 32)
@pa_container_size("ingress", "hdr.big_c[3].b0", 8)
@pa_container_size("ingress", "hdr.big_c[3].b1", 8)
@pa_container_size("ingress", "hdr.big_c[3].h0", 16)
@pa_container_size("ingress", "hdr.big_c[3].h1", 16)
@pa_container_size("ingress", "hdr.big_c[3].h2", 16)

@pa_container_size("ingress", "hdr.big_c[4].w0", 32)
@pa_container_size("ingress", "hdr.big_c[4].w1", 32)
@pa_container_size("ingress", "hdr.big_c[4].b0", 8)
@pa_container_size("ingress", "hdr.big_c[4].b1", 8)
@pa_container_size("ingress", "hdr.big_c[4].h0", 16)
@pa_container_size("ingress", "hdr.big_c[4].h1", 16)
@pa_container_size("ingress", "hdr.big_c[4].h2", 16)

@pa_container_size("ingress", "hdr.big_c[5].w0", 32)
@pa_container_size("ingress", "hdr.big_c[5].w1", 32)
@pa_container_size("ingress", "hdr.big_c[5].b0", 8)
@pa_container_size("ingress", "hdr.big_c[5].b1", 8)
@pa_container_size("ingress", "hdr.big_c[5].h0", 16)
@pa_container_size("ingress", "hdr.big_c[5].h1", 16)
@pa_container_size("ingress", "hdr.big_c[5].h2", 16)

@pa_container_size("ingress", "hdr.big_c[6].w0", 32)
@pa_container_size("ingress", "hdr.big_c[6].w1", 32)
@pa_container_size("ingress", "hdr.big_c[6].b0", 8)
@pa_container_size("ingress", "hdr.big_c[6].b1", 8)
@pa_container_size("ingress", "hdr.big_c[6].h0", 16)
@pa_container_size("ingress", "hdr.big_c[6].h1", 16)
@pa_container_size("ingress", "hdr.big_c[6].h2", 16)

//eg big
@pa_container_size("egress", "hdr.big[0].w0", 32)
@pa_container_size("egress", "hdr.big[0].w1", 32)
@pa_container_size("egress", "hdr.big[0].b0", 8)
@pa_container_size("egress", "hdr.big[0].b1", 8)
@pa_container_size("egress", "hdr.big[0].h0", 16)
@pa_container_size("egress", "hdr.big[0].h1", 16)
@pa_container_size("egress", "hdr.big[0].h2", 16)

@pa_container_size("egress", "hdr.big[1].w0", 32)
@pa_container_size("egress", "hdr.big[1].w1", 32)
@pa_container_size("egress", "hdr.big[1].b0", 8)
@pa_container_size("egress", "hdr.big[1].b1", 8)
@pa_container_size("egress", "hdr.big[1].h0", 16)
@pa_container_size("egress", "hdr.big[1].h1", 16)
@pa_container_size("egress", "hdr.big[1].h2", 16)

@pa_container_size("egress", "hdr.big[2].w0", 32)
@pa_container_size("egress", "hdr.big[2].w1", 32)
@pa_container_size("egress", "hdr.big[2].b0", 8)
@pa_container_size("egress", "hdr.big[2].b1", 8)
@pa_container_size("egress", "hdr.big[2].h0", 16)
@pa_container_size("egress", "hdr.big[2].h1", 16)
@pa_container_size("egress", "hdr.big[2].h2", 16)

@pa_container_size("egress", "hdr.big[3].w0", 32)
@pa_container_size("egress", "hdr.big[3].w1", 32)
@pa_container_size("egress", "hdr.big[3].b0", 8)
@pa_container_size("egress", "hdr.big[3].b1", 8)
@pa_container_size("egress", "hdr.big[3].h0", 16)
@pa_container_size("egress", "hdr.big[3].h1", 16)
@pa_container_size("egress", "hdr.big[3].h2", 16)

@pa_container_size("egress", "hdr.big[4].w0", 32)
@pa_container_size("egress", "hdr.big[4].w1", 32)
@pa_container_size("egress", "hdr.big[4].b0", 8)
@pa_container_size("egress", "hdr.big[4].b1", 8)
@pa_container_size("egress", "hdr.big[4].h0", 16)
@pa_container_size("egress", "hdr.big[4].h1", 16)
@pa_container_size("egress", "hdr.big[4].h2", 16)

@pa_container_size("egress", "hdr.big[5].w0", 32)
@pa_container_size("egress", "hdr.big[5].w1", 32)
@pa_container_size("egress", "hdr.big[5].b0", 8)
@pa_container_size("egress", "hdr.big[5].b1", 8)
@pa_container_size("egress", "hdr.big[5].h0", 16)
@pa_container_size("egress", "hdr.big[5].h1", 16)
@pa_container_size("egress", "hdr.big[5].h2", 16)

@pa_container_size("egress", "hdr.big[6].w0", 32)
@pa_container_size("egress", "hdr.big[6].w1", 32)
@pa_container_size("egress", "hdr.big[6].b0", 8)
@pa_container_size("egress", "hdr.big[6].b1", 8)
@pa_container_size("egress", "hdr.big[6].h0", 16)
@pa_container_size("egress", "hdr.big[6].h1", 16)
@pa_container_size("egress", "hdr.big[6].h2", 16)

//eg big_c
@pa_container_size("egress", "hdr.big_c[0].w0", 32)
@pa_container_size("egress", "hdr.big_c[0].w1", 32)
@pa_container_size("egress", "hdr.big_c[0].b0", 8)
@pa_container_size("egress", "hdr.big_c[0].b1", 8)
@pa_container_size("egress", "hdr.big_c[0].h0", 16)
@pa_container_size("egress", "hdr.big_c[0].h1", 16)
@pa_container_size("egress", "hdr.big_c[0].h2", 16)

@pa_container_size("egress", "hdr.big_c[1].w0", 32)
@pa_container_size("egress", "hdr.big_c[1].w1", 32)
@pa_container_size("egress", "hdr.big_c[1].b0", 8)
@pa_container_size("egress", "hdr.big_c[1].b1", 8)
@pa_container_size("egress", "hdr.big_c[1].h0", 16)
@pa_container_size("egress", "hdr.big_c[1].h1", 16)
@pa_container_size("egress", "hdr.big_c[1].h2", 16)

@pa_container_size("egress", "hdr.big_c[2].w0", 32)
@pa_container_size("egress", "hdr.big_c[2].w1", 32)
@pa_container_size("egress", "hdr.big_c[2].b0", 8)
@pa_container_size("egress", "hdr.big_c[2].b1", 8)
@pa_container_size("egress", "hdr.big_c[2].h0", 16)
@pa_container_size("egress", "hdr.big_c[2].h1", 16)
@pa_container_size("egress", "hdr.big_c[2].h2", 16)

@pa_container_size("egress", "hdr.big_c[3].w0", 32)
@pa_container_size("egress", "hdr.big_c[3].w1", 32)
@pa_container_size("egress", "hdr.big_c[3].b0", 8)
@pa_container_size("egress", "hdr.big_c[3].b1", 8)
@pa_container_size("egress", "hdr.big_c[3].h0", 16)
@pa_container_size("egress", "hdr.big_c[3].h1", 16)
@pa_container_size("egress", "hdr.big_c[3].h2", 16)

@pa_container_size("egress", "hdr.big_c[4].w0", 32)
@pa_container_size("egress", "hdr.big_c[4].w1", 32)
@pa_container_size("egress", "hdr.big_c[4].b0", 8)
@pa_container_size("egress", "hdr.big_c[4].b1", 8)
@pa_container_size("egress", "hdr.big_c[4].h0", 16)
@pa_container_size("egress", "hdr.big_c[4].h1", 16)
@pa_container_size("egress", "hdr.big_c[4].h2", 16)

@pa_container_size("egress", "hdr.big_c[5].w0", 32)
@pa_container_size("egress", "hdr.big_c[5].w1", 32)
@pa_container_size("egress", "hdr.big_c[5].b0", 8)
@pa_container_size("egress", "hdr.big_c[5].b1", 8)
@pa_container_size("egress", "hdr.big_c[5].h0", 16)
@pa_container_size("egress", "hdr.big_c[5].h1", 16)
@pa_container_size("egress", "hdr.big_c[5].h2", 16)

@pa_container_size("egress", "hdr.big_c[6].w0", 32)
@pa_container_size("egress", "hdr.big_c[6].w1", 32)
@pa_container_size("egress", "hdr.big_c[6].b0", 8)
@pa_container_size("egress", "hdr.big_c[6].b1", 8)
@pa_container_size("egress", "hdr.big_c[6].h0", 16)
@pa_container_size("egress", "hdr.big_c[6].h1", 16)
@pa_container_size("egress", "hdr.big_c[6].h2", 16)

//ig rest
@pa_container_size("ingress", "hdr.rest.w0", 32)
@pa_container_size("ingress", "hdr.rest.w1", 32)
@pa_container_size("ingress", "hdr.rest.b0", 8)
@pa_container_size("ingress", "hdr.rest.b1", 16)
@pa_container_size("ingress", "hdr.rest.h0", 16)
@pa_container_size("ingress", "hdr.rest.h1", 16)
@pa_container_size("ingress", "hdr.rest.h2", 16)

//ig rest_c
@pa_container_size("ingress", "hdr.rest_c.w0", 32)
@pa_container_size("ingress", "hdr.rest_c.w1", 32)
@pa_container_size("ingress", "hdr.rest_c.b0", 8)
@pa_container_size("ingress", "hdr.rest_c.b1", 16)
@pa_container_size("ingress", "hdr.rest_c.h0", 16)
@pa_container_size("ingress", "hdr.rest_c.h1", 16)
@pa_container_size("ingress", "hdr.rest_c.h2", 16)

//eg rest
@pa_container_size("egress", "hdr.rest.w0", 32)
@pa_container_size("egress", "hdr.rest.w1", 32)
@pa_container_size("egress", "hdr.rest.b0", 8)
@pa_container_size("egress", "hdr.rest.b1", 16)
@pa_container_size("egress", "hdr.rest.h0", 16)
@pa_container_size("egress", "hdr.rest.h1", 16)
@pa_container_size("egress", "hdr.rest.h2", 16)

//eg rest_c
@pa_container_size("egress", "hdr.rest_c.w0", 32)
@pa_container_size("egress", "hdr.rest_c.w1", 32)
@pa_container_size("egress", "hdr.rest_c.b0", 8)
@pa_container_size("egress", "hdr.rest_c.b1", 16)
@pa_container_size("egress", "hdr.rest_c.h0", 16)
@pa_container_size("egress", "hdr.rest_c.h1", 16)
@pa_container_size("egress", "hdr.rest_c.h2", 16)


@pa_mutually_exclusive("ingress", "ig_intr_md_for_dprsr.mirror_type", "hdr.big[1].b0")
#if __TARGET_TOFINO__ != 1
@pa_mutually_exclusive("ingress", "$tmp2", "hdr.big[2].b0")
#else
@pa_mutually_exclusive("ingress", "$tmp2", "hdr.big[2].h0")
#endif

@pa_mutually_exclusive("egress", "eg_intr_md_for_dprsr.mirror_io_select", "eg_intr_md.egress_port")

struct header_t {
    big_h[7] big_c;
    rest_h rest_c;

    big_h[7] big;
    rest_h rest;
}

struct metadata {}

struct portmeta {
    bit<9> eg_port;
}

parser IgParser(
        packet_in pkt,
        out header_t hdr,
        out metadata md,
        out ingress_intrinsic_metadata_t ig_intr_md,
        out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr) {

    ParserCounter() parser_counter;
    state start {
        pkt.extract(ig_intr_md);
#if __TARGET_TOFINO__ != 1
        parser_counter.set(8w7);
#endif
        portmeta port_md = port_metadata_unpack<portmeta>(pkt);
        ig_intr_md_for_tm.ucast_egress_port = port_md.eg_port;
        transition parse_big;
    }

#if __TARGET_TOFINO__ != 1
    state parse_big {
        pkt.extract(hdr.big.next);
        parser_counter.decrement(1);
        transition select(parser_counter.is_zero()) {
            false : parse_big;
            true : parse_rest;
        }
    }
#else
    state parse_big {
        pkt.extract(hdr.big[0]);
        pkt.extract(hdr.big[1]);
        pkt.extract(hdr.big[2]);
        pkt.extract(hdr.big[3]);
        pkt.extract(hdr.big[4]);
        pkt.extract(hdr.big[5]);
        pkt.extract(hdr.big[6]);
        transition parse_rest;
    }
#endif

    state parse_rest {
        pkt.extract(hdr.rest);
        transition accept;
    }
}


#define copy_n(index) \
    action dup_pkt_##index() { \
        hdr.big_c[##index##].setValid(); \
        hdr.big_c[##index##].w0 = hdr.big[##index##].w0; \
        hdr.big_c[##index##].w1 = hdr.big[##index##].w1; \
        hdr.big_c[##index##].b0 = hdr.big[##index##].b0; \
        hdr.big_c[##index##].b1 = hdr.big[##index##].b1; \
        hdr.big_c[##index##].h0 = hdr.big[##index##].h0; \
        hdr.big_c[##index##].h1 = hdr.big[##index##].h1; \
        hdr.big_c[##index##].h2 = hdr.big[##index##].h2; \
    } \
    /* @stage(##s##) */ \
    table copy_##index { \
        key = { \
            hdr.big[##index##].h2 : ternary; \
            } \
        actions = {NoAction; dup_pkt_##index;} \
        default_action = dup_pkt_##index(); \
        size = 256;\
    } \


control Ig(
        inout header_t hdr,
        inout metadata md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {

    copy_n(0)
    copy_n(1)
    copy_n(2)
    copy_n(3)
    copy_n(4)
    copy_n(5)
    copy_n(6)

    action dup_rest() {
        hdr.rest_c.setValid();
        hdr.rest_c.w0 = hdr.rest.w0;
        hdr.rest_c.w1 = hdr.rest.w1;
        hdr.rest_c.b0 = hdr.rest.b0;
        hdr.rest_c.h0 = hdr.rest.h0;
        hdr.rest_c.h1 = hdr.rest.h1;
    }
    
    table copy_rest {
        actions = {dup_rest;}
        default_action = dup_rest;
        size = 1;
    }

    action dst_set_action(PortId_t port) {
        ig_intr_md_for_tm.ucast_egress_port = port;
	ig_intr_md_for_dprsr.drop_ctl = 0;
    } 
    
    @stage(19) table dst_set {
        actions = {
            dst_set_action;
        }
        size = 1;
        default_action = dst_set_action(0);
    }


    apply {
        copy_0.apply();
        copy_1.apply();
        copy_2.apply();
        copy_3.apply();
        copy_4.apply();
        copy_5.apply();
        copy_6.apply();

        copy_rest.apply();

        //dst_set.apply();
    }
}

control IgDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata md,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        in ingress_intrinsic_metadata_t ig_intr_md) {    
    apply {
        pkt.emit(hdr.big_c);
        pkt.emit(hdr.rest_c);

        pkt.emit(hdr.big);
        pkt.emit(hdr.rest);
    }
}

parser EgParser(
        packet_in pkt,
        out header_t hdr,
        out metadata md,
        out egress_intrinsic_metadata_t eg_intr_md
        ) {

    ParserCounter() parser_counter;
    state start {
        pkt.extract(eg_intr_md);
#if __TARGET_TOFINO__ != 1
        parser_counter.set(8w7);
#endif
        transition parse_big;
    }

#if __TARGET_TOFINO__ != 1
    state parse_big {
        pkt.extract(hdr.big.next);
        parser_counter.decrement(1);
        transition select(parser_counter.is_zero()) {
            false : parse_big;
            true : parse_rest;
        }
    }
#else
    state parse_big {
        pkt.extract(hdr.big[0]);
        pkt.extract(hdr.big[1]);
        pkt.extract(hdr.big[2]);
        pkt.extract(hdr.big[3]);
        pkt.extract(hdr.big[4]);
        pkt.extract(hdr.big[5]);
        pkt.extract(hdr.big[6]);
        transition parse_rest;
    }
#endif

    state parse_rest {
        pkt.extract(hdr.rest);
        transition accept;
    }
}

control Eg(
        inout header_t hdr,
        inout metadata md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
    copy_n(0)
    copy_n(1)
    copy_n(2)
    copy_n(3)
    copy_n(4)
    copy_n(5)
    copy_n(6)

    action dup_rest() {
        hdr.rest_c.setValid();
        hdr.rest_c.w0 = hdr.rest.w0;
        hdr.rest_c.w1 = hdr.rest.w1;
        hdr.rest_c.b0 = hdr.rest.b0;
        hdr.rest_c.h0 = hdr.rest.h0;
        hdr.rest_c.h1 = hdr.rest.h1;
    }
    table copy_rest {
        actions = {dup_rest;}
        default_action = dup_rest;
        size = 1;
    }

    apply {
        copy_0.apply();
        copy_1.apply();
        copy_2.apply();
        copy_3.apply();
        copy_4.apply();
        copy_5.apply();
        copy_6.apply();

        copy_rest.apply();
    }
}
control EgDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr) {
    apply{
        pkt.emit(hdr.big_c);
        pkt.emit(hdr.rest_c);

        pkt.emit(hdr.big);
        pkt.emit(hdr.rest);
    }
}
Pipeline(IgParser(),Ig(),IgDeparser(),EgParser(),Eg(),EgDeparser()) pipeline_profile;
//Switch(pipe) main;

#endif // _FULL_PHV_

