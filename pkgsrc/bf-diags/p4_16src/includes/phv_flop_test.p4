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



#ifndef _PHV_FLOP_TEST_
#define _PHV_FLOP_TEST_

struct metadata_t {
}

/**
 *  FlopIngressParser()
 */
parser FlopIngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    state start {
        pkt.extract(ig_intr_md);
        transition select(ig_intr_md.resubmit_flag) {
            1 : parse_resubmit;
            0 : parse_port_metadata;
        }
    }

    state parse_resubmit {
        transition reject;
    }

    state parse_port_metadata {
        pkt.advance(PORT_METADATA_SIZE);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition parse_testdata;
    }

    state parse_testdata {
        pkt.extract(hdr.testdata);
        transition parse_phv_flop_hdr;
    }

    state parse_phv_flop_hdr {
        pkt.extract(hdr.flopdata);
        transition accept;
    }

} // FlopIngressParser

/**
 *  FlopIngress()
 */
control FlopIngress(
        inout header_t hdr,
        inout metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md) {

    @name(".cntPkt") DirectCounter<bit<32>>(CounterType_t.PACKETS) cntPkt;

    @name(".override_eg_port") action override_eg_port(PortId_t port) {
        ig_intr_tm_md.ucast_egress_port = port;
        cntPkt.count();
    }

    @name(".override_eg_port_to_cpu")
    action override_eg_port_to_cpu(PortId_t port) {
        ig_intr_tm_md.ucast_egress_port = port;
        cntPkt.count();
    }

    @stage(0) @name(".dst_override") table dst_override {
        actions = {
            override_eg_port;
            override_eg_port_to_cpu;
        }
        key = {
            ig_intr_md.ingress_port: exact;
            hdr.testdata.pkt_ctrl: range;
        }
        size = DST_OVERRIDE_TABLE_SIZE;
        counters = cntPkt;
    }

#ifdef DIAG_PHV_FLOP_CONFIG_1

    action ing_and_phv() {
        // This action is a no-op. It activates the MAU stage, causing
        // the test data in the PHVs to pass through the stage unmodified.
        hdr.flopdata.w0 = hdr.flopdata.w0 & hdr.flopdata.w0;
    }

// Define an exact-match table in the specified stage.
// The compiler treats this as a concurrent operation. Concurrent operations
// use the action flops. We use a compiler switch to override this behavior
// for the match-flop test.
#define ING_EXM_TBL(n) \
    @stage(##n) @name(STR(.ing_exm_tbl_##n)) table ing_exm_tbl_##n { \
        key = { \
            hdr.ethernet.isValid(): exact; \
        } \
        actions = { \
            NoAction; \
            ing_and_phv; \
        } \
        const entries = { \
            false: NoAction(); \
            true : ing_and_phv(); \
        } \
        const default_action = ing_and_phv(); \
        size = 2; \
    }

    ING_EXM_TBL(0)
    ING_EXM_TBL(1)
    ING_EXM_TBL(2)
    ING_EXM_TBL(3)
    ING_EXM_TBL(4)
    ING_EXM_TBL(5)
    ING_EXM_TBL(6)
    ING_EXM_TBL(7)
    ING_EXM_TBL(8)
    ING_EXM_TBL(9)
    ING_EXM_TBL(10)
    ING_EXM_TBL(11)

#endif // DIAG_PHV_FLOP_CONFIG_1

    apply {
        dst_override.apply();
#ifdef DIAG_PHV_FLOP_CONFIG_1
        ing_exm_tbl_0.apply();
        ing_exm_tbl_1.apply();
        ing_exm_tbl_2.apply();
        ing_exm_tbl_3.apply();
        ing_exm_tbl_4.apply();
        ing_exm_tbl_5.apply();
        ing_exm_tbl_6.apply();
        ing_exm_tbl_7.apply();
        ing_exm_tbl_8.apply();
        ing_exm_tbl_9.apply();
        ing_exm_tbl_10.apply();
        ing_exm_tbl_11.apply();
        ig_intr_tm_md.bypass_egress = 1w1;
#endif // DIAG_PHV_FLOP_CONFIG_1
    }

} // FlopIngress

/**
 *  FlopIngressDeparser()
 */
control FlopIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.testdata);
        pkt.emit(hdr.flopdata);
    }
} // FlopIngressDeparser

#if defined(DIAG_PHV_FLOP_CONFIG_1)

/**
 *  EmptyEgressParser()
 */
parser EmptyEgressParser(
        packet_in pkt,
        out empty_header_t hdr,
        out empty_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {
    state start {
#ifdef DIAG_DUMMY_OUTPUT
        pkt.extract(hdr.dummy);
#endif
        transition accept;
    }
} // EmptyEgressParser

/**
 *  EmptyEgress()
 */
control EmptyEgress(
        inout empty_header_t hdr,
        inout empty_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {
    apply {}
} // EmptyEgress

/**
 *  EmptyEgressDeparser()
 */
control EmptyEgressDeparser(
        packet_out pkt,
        inout empty_header_t hdr,
        in empty_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {
#ifdef DIAG_DUMMY_OUTPUT
        // The p4_pd_diag_eg_snapshot_trig_spec structure is based on the
        // output of the Egress stage. We must appear to output something,
        // even if we always bypass the egress stage, or the structure will
        // be empty and we'll get a bunch of compiler errors.
        //
        // This is an effective no-op because the dummy header is never valid.
        // It consumes a PHV; therefore we suppress it when we can.
        pkt.emit(hdr.dummy);
#endif
    }
} // EmptyEgressDeparser

/**
 *  pipeline_profile
 */
Pipeline(FlopIngressParser(),
         FlopIngress(),
         FlopIngressDeparser(),
         EmptyEgressParser(),
         EmptyEgress(),
         EmptyEgressDeparser()) pipeline_profile;

#elif defined(DIAG_PHV_FLOP_CONFIG_2)

/**
 *  FlopEgressParser()
 */
parser FlopEgressParser(
        packet_in pkt,
        out header_t hdr,
        out empty_metadata_t eg_md,
        out egress_intrinsic_metadata_t eg_intr_md) {

    state start {
        pkt.extract(eg_intr_md);
        pkt.extract(hdr.othernet);
        pkt.extract(hdr.testdata);
        pkt.extract(hdr.flopdata);
        transition accept;
    }
} // FlopEgressParser

/**
 *  FlopEgress()
 */
control FlopEgress(
        inout header_t hdr,
        inout empty_metadata_t eg_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_oport_md) {

    action egr_and_phv() {
        // This action is a no-op. It activates the MAU stage, causing
        // the test data in the PHVs to pass through the stage unmodified.
        hdr.flopdata.w16 = hdr.flopdata.w16 & hdr.flopdata.w16;
    }

// Define an exact-match table in each specified stage.
// The compiler treats this as a concurrent operation. Concurrent operations
// use the action flops. We use a compiler switch to override this behavior
// for the match-flop test.
#define EGR_EXM_TBL(n) \
    @stage(##n) @ways(2) @name(STR(.egr_exm_tbl_##n)) table egr_exm_tbl_##n { \
        key = { \
            hdr.othernet.isValid(): exact; \
        } \
        actions = { \
            NoAction; \
            egr_and_phv; \
        } \
        const entries = { \
            false: NoAction(); \
            true : egr_and_phv(); \
        } \
        const default_action = egr_and_phv(); \
        size = 2; \
    }

    EGR_EXM_TBL(0)
    EGR_EXM_TBL(1)
    EGR_EXM_TBL(2)
    EGR_EXM_TBL(3)
    EGR_EXM_TBL(4)
    EGR_EXM_TBL(5)
    EGR_EXM_TBL(6)
    EGR_EXM_TBL(7)
    EGR_EXM_TBL(8)
    EGR_EXM_TBL(9)
    EGR_EXM_TBL(10)
    EGR_EXM_TBL(11)

    apply {
        egr_exm_tbl_0.apply();
        egr_exm_tbl_1.apply();
        egr_exm_tbl_2.apply();
        egr_exm_tbl_3.apply();
        egr_exm_tbl_4.apply();
        egr_exm_tbl_5.apply();
        egr_exm_tbl_6.apply();
        egr_exm_tbl_7.apply();
        egr_exm_tbl_8.apply();
        egr_exm_tbl_9.apply();
        egr_exm_tbl_10.apply();
        egr_exm_tbl_11.apply();
    }
} // FlopEgress

/**
 *  FlopEgressDeparser()
 */
control FlopEgressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in empty_metadata_t eg_md,
        in egress_intrinsic_metadata_for_deparser_t ig_intr_dprs_md) {
    apply {
        pkt.emit(hdr.othernet);
        pkt.emit(hdr.testdata);
        pkt.emit(hdr.flopdata);
    }
} // FlopEgressDeparser

/**
 *  pipeline_profile
 */
Pipeline(FlopIngressParser(),
         FlopIngress(),
         FlopIngressDeparser(),
         FlopEgressParser(),
         FlopEgress(),
         FlopEgressDeparser()) pipeline_profile;

#endif // DIAG_PHV_FLOP_CONFIG_2

#endif /* _PHV_FLOP_TEST_ */
