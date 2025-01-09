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

#ifndef _PHV_DATAPATH_
#define _PHV_DATAPATH_

//#define STAND_ALONE
#ifdef STAND_ALONE
#include <t2na.p4>
#endif


/* TF2 PHV Datapath
 * This P4 covers the following Tofino-2 PHV resources:
 *  48 of 48 normal 32-bit PHVs
 *  48 of 48 normal  8-bit PHVs
 *  72 of 72 normal 16-bit PHVs
 * The PHVs covered are populated from packet data in the parser and carried
 * down the pipeline unchanged to the deparser where they are put back into the
 * packet.  Any packet modifications therefore are likely the result of an
 * issue in the PHV datapath.  Mocha PHVs are used to carry POV bits and the
 * required intrinsic metadata.
 *
 * The PHVs will be split evenly across ingress and egress: 24/24/36 32-bit/
 * 8-bit/16-bit containers per gress. */


/* Main header requiring 4x32-bit PHVs, 4x8-bit PHVs, and 6x16-bit PHVs.
 * Six instances of this header will exactly cover half of the normal PHVs when
 * all fields are allocated to normal PHVs. */
header main_h {
    bit<32> w0;
    bit<32> w1;
    bit<32> w2;
    bit<32> w3;
    bit<8> b0;
    bit<8> b1;
    bit<8> b2;
    bit<8> b3;
    bit<16> h0;
    bit<16> h1;
    bit<16> h2;
    bit<16> h3;
    bit<16> h4;
    bit<16> h5;
}

/* For compability with the control plane we define a specifically named header
 * with a 16-bit field.  This will be used in a match table to control the
 * packet's forwarding destination.  It will also have a 14B padding field
 * representing the Ethernet header added by the control plane. */
header test_data_t {
  bit<(14*8)> ethernet;
  bit<16> pkt_ctrl;
}

// Ingress header
struct i_header_t {
    test_data_t itestdata;
    main_h imain0;
    main_h imain1;
    main_h imain2;
    main_h imain3;
    main_h imain4;
    main_h imain5;
}

// Egress header
struct e_header_t {
    test_data_t etestdata;
    main_h emain0;
    main_h emain1;
    main_h emain2;
    main_h emain3;
    main_h emain4;
    main_h emain5;
}

/* Use annotations to guide the compiler into the allocation we require. */
#define CONTAINER_SZ_TYPE(gress, full_name, sz, type) \
  @pa_container_size(gress, #full_name, sz)           \
  @pa_container_type(gress, #full_name, type)         \
  @do_not_use_clot(gress, #full_name)
#define PER_GRESS_CONTAINERS(gress, name_prefix, type)  \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w0,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w1,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w2,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.w3,  32, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b0,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b1,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b2,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.b3,   8, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h0,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h1,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h2,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h3,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h4,  16, type) \
  CONTAINER_SZ_TYPE(gress, name_prefix##.h5,  16, type)

@pa_allow_pov_on_mocha

// Ingress main_h
PER_GRESS_CONTAINERS("ingress", hdr.imain0, "normal")
PER_GRESS_CONTAINERS("ingress", hdr.imain1, "normal")
PER_GRESS_CONTAINERS("ingress", hdr.imain2, "normal")
PER_GRESS_CONTAINERS("ingress", hdr.imain3, "normal")
PER_GRESS_CONTAINERS("ingress", hdr.imain4, "normal")
PER_GRESS_CONTAINERS("ingress", hdr.imain5, "normal")

@pa_container_type("ingress", "ig_intr_md.ingress_port", "mocha")
@pa_solitary("ingress" , "ig_intr_md_for_tm.ucast_egress_port.$valid")

// Egress main_h
PER_GRESS_CONTAINERS("egress", hdr.emain0, "normal")
PER_GRESS_CONTAINERS("egress", hdr.emain1, "normal")
PER_GRESS_CONTAINERS("egress", hdr.emain2, "normal")
PER_GRESS_CONTAINERS("egress", hdr.emain3, "normal")
PER_GRESS_CONTAINERS("egress", hdr.emain4, "normal")
PER_GRESS_CONTAINERS("egress", hdr.emain5, "normal")

@pa_container_type("egress", "eg_intr_md.egress_port.$valid", "mocha")
@pa_container_type("egress", "eg_intr_md.egress_port", "mocha")

struct metadata {}

struct portmeta {
    bit<9> eg_port;
}

// Header modify operation
#define hdr_main_modify(prefix, index, stage_val) \
    action hdr_main_action_##index##_##stage_val##() { \
        hdr.##prefix##main##index##.w0 = 0; \
        hdr.##prefix##main##index##.w1 = 0; \
        hdr.##prefix##main##index##.w2 = 0; \
        hdr.##prefix##main##index##.w3 = 0; \
        hdr.##prefix##main##index##.b0 = 0; \
        hdr.##prefix##main##index##.b1 = 0; \
        hdr.##prefix##main##index##.b2 = 0; \
        hdr.##prefix##main##index##.b3 = 0; \
        hdr.##prefix##main##index##.h0 = 0; \
        hdr.##prefix##main##index##.h1 = 0; \
        hdr.##prefix##main##index##.h2 = 0; \
        hdr.##prefix##main##index##.h3 = 0; \
        hdr.##prefix##main##index##.h4 = 0; \
        hdr.##prefix##main##index##.h5 = 0; \
    } \
    @stage(stage_val) \
    table hdr_main_##index##_##stage_val { \
        actions = {NoAction; hdr_main_action_##index##_##stage_val;} \
        default_action = NoAction; \
        size = 1;\
    }

#define dummy_tbl(stage_val) \
  @stage(stage_val) table dummy_##stage_val##_tbl {\
    actions = { NoAction; } \
    default_action = NoAction; \
    size = 1; \
  }

// Ingress Parser
parser IgParser(
        packet_in pkt,
        out i_header_t hdr,
        out metadata md,
        out ingress_intrinsic_metadata_t ig_intr_md,
        out ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        out ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr) {

    state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);
        pkt.extract(hdr.itestdata);
        pkt.extract(hdr.imain0);
        pkt.extract(hdr.imain1);
        pkt.extract(hdr.imain2);
        pkt.extract(hdr.imain3);
        pkt.extract(hdr.imain4);
        pkt.extract(hdr.imain5);
        transition accept;
    }
}


// Ingress pipeline
@disable_reserved_i2e_drop_implementation
control Ig(
        inout i_header_t hdr,
        inout metadata md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_md_from_prsr,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {

    hdr_main_modify(i, 0, 0)
    hdr_main_modify(i, 1, 1)
    hdr_main_modify(i, 2, 2)
    hdr_main_modify(i, 3, 3)
    hdr_main_modify(i, 4, 4)
    hdr_main_modify(i, 5, 5)
    dummy_tbl(6)
    dummy_tbl(7)
    dummy_tbl(8)
    dummy_tbl(9)
    dummy_tbl(10)
    dummy_tbl(11)
#ifndef TOFINO2M
    dummy_tbl(12)
    dummy_tbl(13)
    dummy_tbl(14)
    dummy_tbl(15)
    dummy_tbl(16)
    dummy_tbl(17)
    dummy_tbl(18)
    dummy_tbl(19)
#endif

    @name(".cntPkt") DirectCounter<bit<32>>(CounterType_t.PACKETS) cntPkt;
    @name(".override_eg_port") action override_eg_port(PortId_t port) {
        ig_intr_md_for_tm.ucast_egress_port = port;
        cntPkt.count();
    }

    @name(".override_eg_port_to_cpu")
    action override_eg_port_to_cpu(PortId_t port) {
        ig_intr_md_for_tm.ucast_egress_port = port;
        cntPkt.count();
    }

    @stage(0) @name(".dst_override") table dst_override {
        actions = {
            override_eg_port;
            override_eg_port_to_cpu;
        }
        key = {
            ig_intr_md.ingress_port: exact @name("ig_intr_md.ingress_port");
            hdr.itestdata.pkt_ctrl: range @name("testdata.pkt_ctrl");
        }
        size = DST_OVERRIDE_TABLE_SIZE;
        counters = cntPkt;
    }
   
    apply {
        dst_override.apply();
        hdr_main_0_0.apply();
        hdr_main_1_1.apply();
        hdr_main_2_2.apply();
        hdr_main_3_3.apply();
        hdr_main_4_4.apply();
        hdr_main_5_5.apply();
        dummy_6_tbl.apply();
        dummy_7_tbl.apply();
        dummy_8_tbl.apply();
        dummy_9_tbl.apply();
        dummy_10_tbl.apply();
        dummy_11_tbl.apply();
#ifndef TOFINO2M
        dummy_12_tbl.apply();
        dummy_13_tbl.apply();
        dummy_14_tbl.apply();
        dummy_15_tbl.apply();
        dummy_16_tbl.apply();
        dummy_17_tbl.apply();
        dummy_18_tbl.apply();
        dummy_19_tbl.apply();
#endif
    }
}

// Ingress Deparser
control IgDeparser(
        packet_out pkt,
        inout i_header_t hdr,
        in metadata md,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        in ingress_intrinsic_metadata_t ig_intr_md) {
    apply {
        pkt.emit(hdr);
    }
}


// Egress Parser
parser EgParser(
        packet_in pkt,
        out e_header_t hdr,
        out metadata md,
        out egress_intrinsic_metadata_t eg_intr_md
        ) {

    state start {
        pkt.extract(eg_intr_md);
        pkt.extract(hdr.etestdata);
        pkt.extract(hdr.emain0);
        pkt.extract(hdr.emain1);
        pkt.extract(hdr.emain2);
        pkt.extract(hdr.emain3);
        pkt.extract(hdr.emain4);
        pkt.extract(hdr.emain5);
        transition accept;
    }
}

// Egress pipeline
@disable_egress_mirror_io_select_initialization
control Eg(
        inout e_header_t hdr,
        inout metadata md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

    hdr_main_modify(e, 0, 0)
    hdr_main_modify(e, 1, 1)
    hdr_main_modify(e, 2, 2)
    hdr_main_modify(e, 3, 3)
    hdr_main_modify(e, 4, 4)
    hdr_main_modify(e, 5, 5)
    dummy_tbl(6)
    dummy_tbl(7)
    dummy_tbl(8)
    dummy_tbl(9)
    dummy_tbl(10)
    dummy_tbl(11)
#ifndef TOFINO2M
    dummy_tbl(12)
    dummy_tbl(13)
    dummy_tbl(14)
    dummy_tbl(15)
    dummy_tbl(16)
    dummy_tbl(17)
    dummy_tbl(18)
    dummy_tbl(19)
#endif
   
    apply {
        hdr_main_0_0.apply();
        hdr_main_1_1.apply();
        hdr_main_2_2.apply();
        hdr_main_3_3.apply();
        hdr_main_4_4.apply();
        hdr_main_5_5.apply();
        dummy_6_tbl.apply();
        dummy_7_tbl.apply();
        dummy_8_tbl.apply();
        dummy_9_tbl.apply();
        dummy_10_tbl.apply();
        dummy_11_tbl.apply();
#ifndef TOFINO2M
        dummy_12_tbl.apply();
        dummy_13_tbl.apply();
        dummy_14_tbl.apply();
        dummy_15_tbl.apply();
        dummy_16_tbl.apply();
        dummy_17_tbl.apply();
        dummy_18_tbl.apply();
        dummy_19_tbl.apply();
#endif
    }
}

// Egress Deparser
control EgDeparser(
        packet_out pkt,
        inout e_header_t hdr,
        in metadata md,
        in egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr) {
    apply{
        pkt.emit(hdr);
    }
}

Pipeline(IgParser(),Ig(),IgDeparser(),EgParser(),Eg(),EgDeparser()) pipeline_profile;

#ifdef STAND_ALONE
Switch(pipeline_profile) main;
#endif
#endif // _PHV_DATAPATH_
