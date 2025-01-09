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
#if __TARGET_TOFINO__ != 1
#include <t2na.p4>
#else
#include <tna.p4>
#endif

@command_line("--disable-parse-max-depth-limit")
#if defined(DIAG_PHV_FLOP_TEST) && DIAG_PHV_FLOP_TEST == 1
@command_line("--disable-parse-min-depth-limit")
#endif

#include "includes/defines.h"
#include "includes/p4_table_sizes.h"
#ifndef DIAG_PHV_FLOP_TEST
  #include "includes/headers.p4"
  #include "includes/parser.p4"
  #include "includes/mau_bus_stress.p4"
#else // DIAG_PHV_FLOP_TEST
  #if defined(DIAG_PHV_FLOP_CONFIG_3)
    #include "includes/full_phv.p4"
  #elif defined(DIAG_PHV_FLOP_CONFIG_4) || defined(DIAG_PHV_FLOP_CONFIG_5)
    #include "includes/phv_datapath.p4"
  #else
    #include "includes/phv_flop_hdrs.p4"
    #include "includes/phv_flop_test.p4"
  #endif
#endif

/* use this pragma to make sure the uninitialized value get initialized.
 * when compiler is compiling the code, @pa_auto_init_metadata will become
 * a Pragma object and is attached to the nearest P4 language object.
 * It will not be preserved if attach to a P4 language object typedef.
 * suggestion is to put @pa_auto_init_metadata next to a control block,
 * for example before control ProcessVlanDecap */
@pa_auto_init_metadata

#ifndef DIAG_PHV_FLOP_TEST

#ifdef DIAG_PARDE_STRESS_POWER
  #define DUMMY_TCAM_KEY hdr.phv_stress_hdr.g4 // 32b
#else
  #define DUMMY_TCAM_KEY meta.l2_metadata.dummy_tcam_key
#endif

/**
 *  ProcessVlanDecap()
 *
 *  used in SwitchEgress
 */
control ProcessVlanDecap(
        inout headers_t hdr,
        inout e_metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

    @name(".nop") action nop() {
    }

    @name(".remove_vlan_single_tagged") action remove_vlan_single_tagged() {
        hdr.ethernet.etherType = hdr.vlan_tag.etherType;
        hdr.vlan_tag.setInvalid();
    }

    @ternary(1) @name(".vlan_decap") table vlan_decap {
        actions = {
            nop;
            remove_vlan_single_tagged;
        }
        key = {
            hdr.vlan_tag.isValid(): exact;
        }
        size = 2;
    }

    apply {
        vlan_decap.apply();
    }
} // ProcessVlanDecap

/**
 *  ProcessVlanEncap()
 *
 *  used in SwitchEgress
 */
control ProcessVlanEncap(
        inout headers_t hdr,
        inout e_metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

    @name(".set_packet_vlan_untagged") action set_packet_vlan_untagged() {
    }

    @name(".set_packet_vlan_tagged") action set_packet_vlan_tagged(bit<12> vlan_id) {
        hdr.vlan_tag.setValid();
        hdr.vlan_tag.etherType = hdr.ethernet.etherType;
        hdr.vlan_tag.vlan_id = vlan_id;
        hdr.vlan_tag.pri = 0;
        hdr.vlan_tag.cfi = 0;
        hdr.ethernet.etherType = 16w0x8100;
    }

    @name(".vlan_encap") table vlan_encap {
        actions = {
            set_packet_vlan_untagged;
            set_packet_vlan_tagged;
        }
        key = {
            eg_intr_md.egress_port   : exact;
            meta.egress_metadata.vlan_id: exact;
        }
        size = VLAN_ENCAP_TABLE_SIZE;
    }

    apply {
        vlan_encap.apply();
    }
} // ProcessVlanEncap

#ifdef DIAG_PATTERN_SHIFT_ENABLE
/**
 *  ProcessPatternShift()
 *
 *  used in SwitchEgress
 */
control ProcessPatternShift(
        inout headers_t hdr,
        inout e_metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

    const bit<32> packet_counter_size = 1 << 19;
    Register<bit<1>, bit<32>>(packet_counter_size, 0) packet_counter;
    @name(".counter_alu")
    RegisterAction<bit<32>, bit<32>, bit<32>>(packet_counter) counter_alu = {
        void apply(inout bit<32> value, out bit<32> rv) {
            value = value + 32w1;
            rv = value;
        }
    };

    const bit<32> size_counter_size = 1 << 19;
    Register<bit<1>, bit<32>>(size_counter_size, 0) size_counter;
    @name(".size_counter_alu")
    RegisterAction<bit<32>, bit<32>, bit<32>>(size_counter) size_counter_alu = {
        void apply(inout bit<32> value) {
            value = value + 32w1;
        }
    };

    @name(".execute_counter_alu") action execute_counter_alu() {
        meta.egress_md.packet_count = counter_alu.execute(32w0);
    }

    @name(".increment_pkt_size") action increment_pkt_size() {
        hdr.right_shift_h.setValid();
        hdr.right_shift_h.f1 = 8w0xff;
        size_counter_alu.execute((bit<32>)hdr.eg_intr_md.pkt_length);
    }

    @name(".restore_pkt_size") action restore_pkt_size() {
        hdr.left_shift_h.setInvalid();
        size_counter_alu.execute((bit<32>)hdr.eg_intr_md.pkt_length);
    }

    @name(".nop") action nop() {
    }

    @name(".get_size_index") action get_size_index() {
        meta.egress_md.size_index[4:0] = (meta.egress_md.packet_count >> 20)[4:0];
    }

    @name(".tbl_packet_count") table tbl_packet_count {
        actions = {
            execute_counter_alu;
        }
        size = 1;
        default_action = execute_counter_alu();
    }

    @name(".tbl_pattern_shift") table tbl_pattern_shift {
        actions = {
            increment_pkt_size;
            restore_pkt_size;
            nop;
        }
        key = {
            meta.egress_md.size_index: exact;
            hdr.eg_intr_md.pkt_length: exact;
        }
        size = 33;
        default_action = nop();
    }

    @name(".tbl_size_index") table tbl_size_index {
        actions = {
            get_size_index;
        }
        size = 1;
        default_action = get_size_index();
    }

    apply {
        tbl_packet_count.apply();
        tbl_size_index.apply();
        tbl_pattern_shift.apply();
    }
} // ProcessPatternShift
#endif // DIAG_PATTERN_SHIFT_ENABLE

#ifdef DIAG_PARDE_STRAIN

// If pkt is being redirected to CPU, invalidate all strain headers.
// If strain hdr is not valid, add the strain hdr if the random number
// check succeeds.
// If the strain hdr is valid, validate the hdr data value. Remove
// the strain hdr if the random number check succeeds.
#define DIAG_PARDE_STRAIN_LOGIC(n)                             \
    if (meta.l2_metadata.cpu_redir == 8w1) {                   \
        hdr.parde_strain_val_##n##.setInvalid();               \
        hdr.parde_strain_val_##n##.value = 0;                  \
        hdr.parde_strain.hdr##n##_valid =                      \
           DIAG_PARDE_STRAIN_HDR_VALID_ZERO_VAL;               \
    } else if (hdr.parde_strain.hdr##n##_valid ==              \
               DIAG_PARDE_STRAIN_HDR_VALID_ZERO_VAL) {         \
        if ((meta.l2_metadata.parde_strain_random &            \
            DIAG_PARDE_STRAIN_##n##_MASK) ==                   \
                DIAG_PARDE_STRAIN_##n##_MASK) {                \
            hdr.parde_strain_val_##n##.setValid();             \
            hdr.parde_strain_val_##n##.value =                 \
                          DIAG_PARDE_STRAIN_VALUE_##n##;       \
            hdr.parde_strain.hdr##n##_valid =                  \
                   DIAG_PARDE_STRAIN_HDR_VALID_ONE_VAL;        \
            meta.l2_metadata.parde_hdr_add_cnt =               \
                   meta.l2_metadata.parde_hdr_add_cnt + 1;     \
        }                                                      \
    } else {                                                   \
        /* Validate the value */                               \
        if (hdr.parde_strain_val_##n##.value !=                \
                      DIAG_PARDE_STRAIN_VALUE_##n##) {         \
            stage_drop();                                      \
        }                                                      \
        if ((meta.l2_metadata.parde_strain_random &            \
            DIAG_PARDE_STRAIN_##n##_MASK) ==                   \
                DIAG_PARDE_STRAIN_##n##_MASK) {                \
            hdr.parde_strain_val_##n##.setInvalid();           \
            hdr.parde_strain_val_##n##.value = 0;              \
            hdr.parde_strain.hdr##n##_valid =                  \
               DIAG_PARDE_STRAIN_HDR_VALID_ZERO_VAL;           \
            meta.l2_metadata.parde_hdr_rem_cnt =               \
                   meta.l2_metadata.parde_hdr_rem_cnt + 1;     \
        }                                                      \
    }
#endif // DIAG_PARDE_STRAIN

/**
 *  ProcessIngressPortProperties()
 *
 *  used in ProcessNormalIngress
 */
control ProcessIngressPortProperties(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".set_ing_port_prop")
    action set_ing_port_prop(L2ExclusionId_t exclusion_id, PortId_t port) {
        ig_tm_md.level2_exclusion_id = exclusion_id;
        meta.ingress_metadata.ingress_port = port;
    }

    @stage(0) @name(".ing_port_prop") table ing_port_prop {
        actions = {
            set_ing_port_prop;
        }
        key = {
            ig_intr_md.ingress_port: exact;
        }
        size = PORTMAP_TABLE_SIZE;
    }

    apply {
        ing_port_prop.apply();
    }
} // ProcessIngressPortProperties

/**
 *  ProcessMac()
 *
 *  used in ProcessNormalIngress
 */
control ProcessMac(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".nop") action nop() {
    }

    @name(".dmac_hit") action dmac_hit(PortId_t port) {
        meta.ingress_metadata.egress_port = port;
        ig_tm_md.ucast_egress_port = port;
    }

    @name(".dmac_mcast_hit") action dmac_mcast_hit(bit<16> mc_index) {
        ig_tm_md.mcast_grp_b = mc_index;
    }

    @name(".dmac_miss") action dmac_miss() {
        meta.ingress_metadata.egress_port = PORT_INDEX_FLOOD;
        ig_tm_md.ucast_egress_port = PORT_INDEX_FLOOD;
    }

    @name(".dmac_drop") action dmac_drop() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }

    @name(".smac_miss") action smac_miss() {
        meta.l2_metadata.l2_src_miss = 1w1;
    }

    @name(".smac_hit") action smac_hit() {
        meta.l2_metadata.l2_src_hit = 1w1;
    }

    @ways(5) @pack(8) @name(".dmac")  table dmac {
        idle_timeout = true;
        actions = {
            nop;
            dmac_hit;
            dmac_mcast_hit;
            dmac_miss;
            dmac_drop;
        }
        key = {
            meta.ingress_metadata.vlan_id: exact;
            hdr.ethernet.dstAddr         : exact;
        }
        size = MAC_TABLE_SIZE;
    }

    @ways(5) @pack(8) @name(".smac") table smac {
        idle_timeout = true;
        actions = {
            nop;
            smac_miss;
            smac_hit;
        }
        key = {
            meta.ingress_metadata.vlan_id: exact;
            hdr.ethernet.srcAddr         : exact;
        }
        size = MAC_TABLE_SIZE;
    }

    apply {
        smac.apply();
        dmac.apply();
    }
} // ProcessMac

/**
 *  ProcessMulticastFlooding()
 *
 *  used in ProcessNormalIngress
 */
control ProcessMulticastFlooding(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".nop") action nop() {
    }

    @name(".set_bd_flood_mc_index") action set_bd_flood_mc_index(bit<16> mc_index) {
        ig_tm_md.mcast_grp_b = mc_index;
    }

    @ways(5) @pack(8) @name(".bd_flood") table bd_flood {
        actions = {
            nop;
            set_bd_flood_mc_index;
        }
        key = {
            meta.ingress_metadata.vlan_id: exact;
        }
        size = BD_FLOOD_TABLE_SIZE;
    }

    apply {
        bd_flood.apply();
    }
} // ProcessMulticastFlooding

/**
 *  ProcessMacLearning()
 *
 *  used in ProcessNormalIngress
 */
control ProcessMacLearning(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".nop") action nop() {
    }

    @name(".generate_learn_notify") action generate_learn_notify() {
        ig_dprsr_md.digest_type = 0;
    }

    @name(".learn_notify") table learn_notify {
        actions = {
            nop;
            generate_learn_notify;
        }
        key = {
            meta.l2_metadata.l2_src_miss: exact;
        }
        size = 2;
    }

    apply {
        learn_notify.apply();
    }
} // ProcessMacLearning

/**
 *  ProcessNormalIngress()
 *
 *  used in SwitchIngress
 */
control ProcessNormalIngress(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".cntPkt") DirectCounter<bit<32>>(CounterType_t.PACKETS) cntPkt;

    // Bridged metadata fields for Egress pipeline.
    @name (".add_bridged_md") action add_bridged_md() {
        hdr.bridged_md.setValid();
        hdr.bridged_md.vlan_id = meta.ingress_metadata.vlan_id;
        hdr.bridged_md.dst_override = meta.l2_metadata.dst_override;
        hdr.bridged_md.cpu_redir = meta.l2_metadata.cpu_redir;
    }

    @name(".set_def_vlan") action set_def_vlan(bit<12> vid, bit<16> ingress_rid) {
        meta.ingress_metadata.vlan_id = vid;
        ig_tm_md.rid = ingress_rid;
    }

    @name(".def_vlan_miss") action def_vlan_miss() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }

    @name(".set_ing_vlan") action set_ing_vlan(bit<12> vid, bit<16> ingress_rid) {
        meta.ingress_metadata.vlan_id = vid;
        ig_tm_md.rid = ingress_rid;
    }

    @name(".port_vlan_miss") action port_vlan_miss() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }

    @name(".nop") action nop() {
    }

    @name(".do_nothing") action do_nothing() {
    }

    @name(".set_exm_table_meta") action set_exm_table_meta() {
    }

    @name(".stage_drop") action stage_drop() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }

    @name(".set_meta0") action set_meta0() {
        meta.l2_metadata.inter_stage = 32w0x1;
    }

    @name(".set_meta") action set_meta() {
        meta.l2_metadata.inter_stage = meta.l2_metadata.inter_stage + 32w0x1;
    }

    action set_tbl_tcam_meta() {
#ifdef DIAG_PARDE_STRESS_POWER
       DUMMY_TCAM_KEY = 32w0x1234;
#else
       meta.l2_metadata.dummy_tcam_meta = 32w0x1234;
#endif
    }

    @name(".override_eg_port") action override_eg_port(PortId_t port) {
        meta.l2_metadata.dst_override = 1;
        meta.ingress_metadata.egress_port = port;
        ig_tm_md.ucast_egress_port = port;
        cntPkt.count();
#if defined(DIAG_QUEUE_STRESS_ENABLE)
        /* Use all scheduler queues in TM */
#if !defined(DIAG_PHV_STRESS_ENABLE)
        /* tcp dst port is different in each packet */
        ig_tm_md.qid = hdr.tcp.dstPort[6:0];
#else
        /* tcp hdr does not exist in phv stress enable mode */
        ig_tm_md.qid = port[6:0];
#endif
#endif // DIAG_QUEUE_STRESS_ENABLE
    }

    @name(".override_mc_eg_port")
    action override_mc_eg_port(bit<16> mgid_a, bit<16> mgid_b, bit<16> rid) {
        meta.l2_metadata.dst_override = 1;
        ig_tm_md.mcast_grp_a = mgid_a;
        ig_tm_md.mcast_grp_b = mgid_b;
        ig_tm_md.rid = rid;
        cntPkt.count();
#if defined(DIAG_QUEUE_STRESS_ENABLE)
#if !defined(DIAG_PHV_STRESS_ENABLE)
        ig_tm_md.qid = hdr.tcp.dstPort[6:0];
#else
        /* tcp hdr does not exist in phv stress enable mode */
        ig_tm_md.qid = mgid[6:0];
#endif
#endif // DIAG_QUEUE_STRESS_ENABLE
    }

    @name(".override_eg_port_to_cpu")
    action override_eg_port_to_cpu(PortId_t port) {
        meta.l2_metadata.dst_override = 1;
#if !defined(DIAG_PHV_STRESS_ENABLE)
        hdr.tcp.srcPort = (bit<16>)ig_intr_md.ingress_port;
#endif
        meta.ingress_metadata.egress_port = port;
        ig_tm_md.ucast_egress_port = port;
        meta.l2_metadata.cpu_redir = 1;
        cntPkt.count();
    }

    @stage(0) @name(".dst_override") table dst_override {
        actions = {
            override_eg_port;
#if !defined(DIAG_PHV_STRESS_ENABLE)
            override_mc_eg_port;
#endif
            override_eg_port_to_cpu;
        }
        key = {
            ig_intr_md.ingress_port: exact;
#ifdef DIAG_PHV_STRESS_ENABLE
            hdr.phv_stress_hdr.f0  : range;
#else
            hdr.tcp.dstPort        : range;
#endif
        }
        size = DST_OVERRIDE_TABLE_SIZE;
        counters = cntPkt;
    }

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE)
    @name(".def_vlan_mapping") table def_vlan_mapping {
        actions = {
            set_def_vlan;
            def_vlan_miss;
        }
        key = {
            ig_intr_md.ingress_port: exact;
        }
        size = PORTMAP_TABLE_SIZE;
    }

    @ways(5) @pack(8) @name(".port_vlan_mapping") table port_vlan_mapping {
        actions = {
            set_ing_vlan;
            port_vlan_miss;
        }
        key = {
            ig_intr_md.ingress_port: exact;
            hdr.vlan_tag.isValid()     : exact;
            hdr.vlan_tag.vlan_id       : exact;
        }
        size = PORT_VLAN_TABLE_SIZE;
    }
#endif

#define TBL_STAGE_TCAM_INST(n,s)                         \
    @stage(##n) @name(STR(.tbl_stage##n)) table tbl_stage##n {       \
        actions = {                                      \
            nop;                                         \
            set_meta;                                    \
        }                                                \
        key = {                                          \
            meta.l2_metadata.inter_stage      : ternary; \
            meta.l2_metadata.inter_stage_dummy: ternary; \
        }                                                \
        size = ##s;                                      \
    }

#define TBL_STAGE_TCAM_DROP_INST(n,s)                    \
    @stage(##n) @name(STR(.tbl_stage##n)) table tbl_stage##n {       \
        actions = {                                      \
            nop;                                         \
            set_meta;                                    \
            stage_drop;                                  \
        }                                                \
        key = {                                          \
            meta.l2_metadata.inter_stage      : ternary; \
            meta.l2_metadata.inter_stage_dummy: ternary; \
        }                                                \
        const default_action = stage_drop;               \
        size = ##s;                                      \
    }

#define TBL_STAGE_EXM_INST(n,s)                        \
    @stage(##n) @name(STR(.tbl_stage##n)) table tbl_stage##n {     \
        actions = {                                    \
            nop;                                       \
            set_meta;                                  \
        }                                              \
        key = {                                        \
            meta.l2_metadata.inter_stage      : exact; \
            meta.l2_metadata.inter_stage_dummy: exact; \
        }                                              \
        size = ##s;                                    \
    }

#define TBL_STAGE_EXM_DROP_INST(n,s)                   \
    @stage(##n) @name(STR(.tbl_stage##n)) table tbl_stage##n {     \
        actions = {                                    \
            nop;                                       \
            set_meta;                                  \
            stage_drop;                                \
        }                                              \
        key = {                                        \
            meta.l2_metadata.inter_stage      : exact; \
            meta.l2_metadata.inter_stage_dummy: exact; \
        }                                              \
        const default_action = stage_drop;             \
        size = ##s;                                    \
    }

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE)
    @stage(0) @name(".tbl_stage0") table tbl_stage0 {
        actions = {
            nop;
            set_meta0;
        }
        key = {
            hdr.ethernet.isValid(): exact;
        }
        size = 2;
    }
    TBL_STAGE_TCAM_INST(1,TCAM_TABLE_SIZE)
    /* Skip stage2 as it has other tables */
    TBL_STAGE_TCAM_DROP_INST(3,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(4,EXM_TBL_4_SIZE)
    TBL_STAGE_TCAM_DROP_INST(5,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(6,EXM_TBL_6_SIZE)
    TBL_STAGE_TCAM_DROP_INST(7,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(8,EXM_TBL_8_SIZE)
    TBL_STAGE_TCAM_DROP_INST(9,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(10,EXM_TBL_SIZE)
    TBL_STAGE_TCAM_DROP_INST(11,TCAM_TABLE_SIZE)
#if DIAG_STAGES > 12
    TBL_STAGE_EXM_DROP_INST(12,EXM_TBL_SIZE)
    TBL_STAGE_TCAM_DROP_INST(13,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(14,EXM_TBL_SIZE)
    TBL_STAGE_TCAM_DROP_INST(15,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(16,EXM_TBL_SIZE)
    TBL_STAGE_TCAM_DROP_INST(17,TCAM_TABLE_SIZE)
    TBL_STAGE_EXM_DROP_INST(18,TCAM_TABLE_SIZE)
    TBL_STAGE_TCAM_DROP_INST(19,TCAM_TABLE_SIZE)
#endif // DIAG_STAGES > 12
#endif // !diag_single_stage && !diag_mau_bus_stress_enable

#define TBL_TCAM_IG_INST(n,s)                      \
    @stage(##n) @name(STR(.tbl_tcam_##n##_ig)) table tbl_tcam_##n##_ig { \
        actions = {                                \
            nop;                                   \
            do_nothing;                            \
            set_tbl_tcam_meta;                     \
        }                                          \
        key = {                                    \
            DUMMY_TCAM_KEY:  ternary;              \
        }                                          \
        size = ##s;                                \
    }

#ifdef DIAG_POWER_ENABLE
    TBL_TCAM_IG_INST(0,TCAM_TABLE_SIZE_MIN)
    TBL_TCAM_IG_INST(3,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(5,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(6,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(7,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(8,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(9,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(10,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(11,TCAM_TABLE_SIZE)
#if DIAG_STAGES > 12
    TBL_TCAM_IG_INST(12,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(13,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(14,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(15,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(16,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(17,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(18,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(19,TCAM_TABLE_SIZE)
#endif // DIAG_STAGES > 12
#endif // DIAG_POWER_ENABLE

#ifdef DIAG_POWER_MAX_ENABLE
    TBL_TCAM_IG_INST(2,TCAM_TABLE_SIZE)
    TBL_TCAM_IG_INST(4,TCAM_TABLE_SIZE)
#endif

#define EXM_TABLE_INST(n,i,s)                               \
    DIAG_POWER_TBL_WAYS                                     \
    @stage(##n) @name(STR(.exm_table##n##_##i##)) table exm_table##n##_##i## { \
      actions = {                                           \
        do_nothing;                                         \
        set_exm_table_meta;                                 \
      }                                                     \
      key = {                                               \
        hdr.ethernet.srcAddr : exact;                       \
        hdr.ethernet.dstAddr : exact;                       \
        hdr.ethernet.etherType : exact;                     \
        meta.l2_metadata.inter_stage: exact;                \
        meta.l2_metadata.dummy_exm_key:  exact;             \
      }                                                     \
      size = ##s;                                           \
    }

#define EXM_TABLE_SMALL_INST(n,i,s)                         \
    DIAG_POWER_TBL_WAYS                                     \
    @stage(##n) @name(STR(.exm_table##n##_##i##)) table exm_table##n##_##i## { \
      actions = {                                           \
        do_nothing;                                         \
        set_exm_table_meta;                                 \
      }                                                     \
      key = {                                               \
        hdr.ethernet.srcAddr : exact;                       \
        hdr.ethernet.dstAddr : exact;                       \
        hdr.ethernet.etherType : exact;                     \
      }                                                     \
      size = ##s;                                           \
    }

#ifdef DIAG_POWER_ENABLE
    EXM_TABLE_SMALL_INST(3,0,EXM_EXTRA_TBL_SIZE_MIN)
    EXM_TABLE_INST(4,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(5,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(6,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(7,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(8,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(9,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(10,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(11,0,EXM_EXTRA_TBL_SIZE)
#if DIAG_STAGES > 12
    EXM_TABLE_INST(12,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(13,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(14,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(15,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(16,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(17,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(18,0,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(19,0,EXM_EXTRA_TBL_SIZE)
#endif // DIAG_STAGES > 12
#endif // DIAG_POWER_ENABLE

#ifdef DIAG_POWER_MAX_ENABLE
    EXM_TABLE_SMALL_INST(3,1,EXM_EXTRA_TBL_SIZE_MIN)
    EXM_TABLE_INST(4,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(5,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(6,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(7,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(8,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(9,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(10,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(11,1,EXM_EXTRA_TBL_SIZE)
#if DIAG_STAGES > 12
    EXM_TABLE_INST(12,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(13,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(14,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(15,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(16,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(17,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(18,1,EXM_EXTRA_TBL_SIZE)
    EXM_TABLE_INST(19,1,EXM_EXTRA_TBL_SIZE)
#endif // DIAG_STAGES > 12
#endif // DIAG_POWER_MAX_ENABLE

    @name(".process_ingress_port_properties") ProcessIngressPortProperties() ingress_port_properties;

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE)
    @name(".process_mac") ProcessMac() mac;
    @name(".process_mac_learning") ProcessMacLearning() mac_learning;
    @name(".process_multicast_flooding")  ProcessMulticastFlooding() multicast_flooding;
#endif

#ifdef DIAG_MAU_BUS_STRESS_ENABLE
    @name(".process_mau_bus_stress_ig") ProcessMauBusStressIg() mau_bus_stress_ig;
#endif

#ifdef DIAG_PARDE_STRAIN
    Random<bit<8>>() rnd_ig;
#endif

    apply {
        /* Stage 0 */
        ingress_port_properties.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);

        dst_override.apply();

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE) && !defined(DIAG_PARDE_STRAIN)

#ifdef DIAG_POWER_ENABLE
        /* Table defined using macro TBL_TCAM_IG_INST */
        //tbl_tcam_0_ig.apply();
#endif
        tbl_stage0.apply();

        /* Stage 1 */
        /* Table defined using macro TBL_STAGE_TCAM_INST */
        tbl_stage1.apply();

        if (meta.l2_metadata.dst_override == 0) {
            if (hdr.vlan_tag.isValid() && (hdr.vlan_tag.vlan_id != 0)) {
                port_vlan_mapping.apply();
            }
            else {
                def_vlan_mapping.apply();
            }

            /* Stage 2 */
            mac.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);

            /* Stage 3 */
            if (meta.ingress_metadata.egress_port == PORT_INDEX_FLOOD) {
                multicast_flooding.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
            }
            if (meta.ingress_metadata.vlan_id != 0) {
                mac_learning.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
            }
        }
#ifdef DIAG_POWER_ENABLE
        /* Table defined using macro EXM_TABLE_INST */
        //exm_table3_0.apply();
#endif
        /* Table defined using macro TBL_STAGE_EXM_INST */
        /* Remove tbl_stage3 for now as it does not fit */
        //tbl_stage3.apply();

#ifdef DIAG_POWER_ENABLE
        //tbl_tcam_3_ig.apply();
        /* Stage 4 */
        /* Table defined using macro EXM_TABLE_INST */
        //exm_table4_0.apply();
#endif
        //tbl_stage4.apply();

#ifdef DIAG_POWER_ENABLE
        /* Stage 5 */
        //exm_table5_0.apply();
#endif
        tbl_stage5.apply();

#ifdef DIAG_POWER_ENABLE
        tbl_tcam_5_ig.apply();
#endif

#if !defined(TOFINO2H)
        /* Stage 6 */
#ifdef DIAG_POWER_ENABLE
        exm_table6_0.apply();
#endif

        tbl_stage6.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_6_ig.apply();
        /* Stage 7 */
        exm_table7_0.apply();
#endif

        tbl_stage7.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_7_ig.apply();
        /* Stage 8 */
        exm_table8_0.apply();
#endif

        tbl_stage8.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_8_ig.apply();
        /* Stage 9 */
        exm_table9_0.apply();
#endif

        tbl_stage9.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_9_ig.apply();
#endif
#endif // !tofino2h

#if DIAG_STAGES > 10

        /* Stage 10 */
#ifdef DIAG_POWER_ENABLE
        exm_table10_0.apply();
#endif

        tbl_stage10.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_10_ig.apply();
        /* Stage 11 */
        exm_table11_0.apply();
#endif

        tbl_stage11.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_11_ig.apply();
#endif

#endif // DIAG_STAGES > 10

#if DIAG_STAGES > 12

        /* Stage 12 */
#ifdef DIAG_POWER_ENABLE
        exm_table12_0.apply();
#endif

        tbl_stage12.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_12_ig.apply();
        /* Stage 13 */
        exm_table13_0.apply();
#endif

        tbl_stage13.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_13_ig.apply();
        /* Stage 14 */
        exm_table14_0.apply();
#endif

        tbl_stage14.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_14_ig.apply();
        /* Stage 15 */
        exm_table15_0.apply();
#endif

        tbl_stage15.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_15_ig.apply();
        /* Stage 16 */
        exm_table16_0.apply();
#endif

        tbl_stage16.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_16_ig.apply();
        /* Stage 17 */
        exm_table17_0.apply();
#endif

        tbl_stage17.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_17_ig.apply();
        /* Stage 18 */
        exm_table18_0.apply();
#endif

        tbl_stage18.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_18_ig.apply();
        /* Stage 19 */
        exm_table19_0.apply();
#endif

        tbl_stage19.apply();
#ifdef DIAG_POWER_ENABLE
        tbl_tcam_19_ig.apply();
#endif

#endif  // DIAG_STAGES > 12
#endif  // !diag_single_stage && !diag_mau_bus_stress_enable && !diag_parde_strain

#ifdef DIAG_MAU_BUS_STRESS_ENABLE
        mau_bus_stress_ig.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
#endif

#ifdef DIAG_PARDE_STRAIN
        meta.l2_metadata.parde_strain_random = rnd_ig.get();

        /* Parde strain Headers */
        DIAG_PARDE_STRAIN_LOGIC(1)
        DIAG_PARDE_STRAIN_LOGIC(2)
        DIAG_PARDE_STRAIN_LOGIC(3)
        DIAG_PARDE_STRAIN_LOGIC(4)
        DIAG_PARDE_STRAIN_LOGIC(5)
#endif
        /* Only add bridged metadata if we are NOT bypassing egress pipeline */
        if (ig_tm_md.bypass_egress == 1w0) {
            add_bridged_md();
        }
    }
} // ProcessNormalIngress

/**
 *  SwitchIngress()
 */
control SwitchIngress(
        inout headers_t hdr,
        inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    @name(".process_normal_ingress") ProcessNormalIngress() normal_ingress;

    apply {
#ifdef DIAG_HOLD_STRESS

        if (ig_intr_md.resubmit_flag == 0 &&
#if defined(TOFINO1)
            ig_intr_md.ingress_port != DIAG_CPU_PORT_1 &&
#endif
            ig_intr_md.ingress_port != DIAG_CPU_PORT) {
            ig_dprsr_md.resubmit_type = DIAG_RESUBMIT_TYPE_BASIC;
        } else {
            ig_dprsr_md.resubmit_type = 0;
            normal_ingress.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
        }
#else
        normal_ingress.apply(hdr, meta, ig_intr_md, ig_prsr_md, ig_dprsr_md, ig_tm_md);
#endif
    }
} // SwitchIngress

/**
 *  SwitchEgress()
 */
control SwitchEgress(
        inout headers_t hdr,
        inout e_metadata meta,
        in egress_intrinsic_metadata_t eg_intr_md,
        in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
        inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
        inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {

    action set_tbl_tcam_meta() {
#ifdef DIAG_PARDE_STRESS_POWER
       DUMMY_TCAM_KEY = 32w0x1234;
#else
       meta.l2_metadata.dummy_tcam_meta = 32w0x1234;
#endif
    }

    @name(".nop") action nop() {
    }

    @name(".do_nothing") action do_nothing() {
    }

    @name(".stage_drop") action stage_drop() {
        eg_intr_md_for_dprs.drop_ctl = 0x1; // Drop packet.
    }

#ifdef DIAG_HOLD_STRESS
    @name(".do_egr_mir") action do_egr_mir(MirrorId_t mirror_sid) {
       meta.l2_metadata.egress_mirror_session_id = mirror_sid;
       eg_intr_md_for_dprs.mirror_type = DIAG_MIRROR_TYPE_EGRESS;
    }

    @name(".tbl_egr_mir") table tbl_egr_mir {
      actions = {
        do_egr_mir;
      }
      default_action = do_egr_mir(DIAG_DROP_MIRROR_SESSION_ID);
      size = 1;
    }
#endif

#define TBL_TCAM_EG_INST(n,s)                      \
    @stage(##n) @name(STR(.tbl_tcam_##n##_eg)) table tbl_tcam_##n##_eg { \
        actions = {                                \
            nop;                                   \
            do_nothing;                            \
            set_tbl_tcam_meta;                     \
        }                                          \
        key = {                                    \
            DUMMY_TCAM_KEY:  ternary;              \
        }                                          \
        size = ##s;                                \
    }

#ifdef DIAG_PARDE_STRESS_POWER
    TBL_TCAM_EG_INST(2,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(3,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(4,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(5,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(6,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(7,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(8,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(9,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(10,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(11,TCAM_TABLE_SIZE)
#if DIAG_STAGES > 12
    TBL_TCAM_EG_INST(12,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(13,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(14,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(15,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(16,TCAM_TABLE_SIZE)
    TBL_TCAM_EG_INST(17,TCAM_TABLE_SIZE)
#endif // DIAG_STAGES > 12
#endif // DIAG_PARDE_STRESS_POWER

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE) && !defined(DIAG_PARDE_STRAIN)
    @name(".process_vlan_decap") ProcessVlanDecap() vlan_decap;
    @name(".process_vlan_encap") ProcessVlanEncap() vlan_encap;
#endif

#ifdef DIAG_MAU_BUS_STRESS_ENABLE
    @name(".process_mau_bus_stress_eg") ProcessMauBusStressEg() mau_bus_stress_eg;
#endif

#ifdef DIAG_PATTERN_SHIFT_ENABLE
    @name(".process_pattern_shift") ProcessPatternShift() pattern_shift;
#endif

#ifdef DIAG_PARDE_STRAIN
    Random<bit<8>>() rnd_eg;
#endif

    apply {

#if !defined(DIAG_SINGLE_STAGE) && !defined(DIAG_MAU_BUS_STRESS_ENABLE) && !defined(DIAG_PARDE_STRAIN)
        if (meta.l2_metadata.dst_override == 0) {
            /* Decap the vlan header if present */
            vlan_decap.apply(hdr, meta, eg_intr_md,
                   eg_intr_md_from_prsr, eg_intr_md_for_dprs,
                   eg_intr_md_for_oport);
            /* Add the vlan header if egress port is tagged */
            vlan_encap.apply(hdr, meta, eg_intr_md,
                   eg_intr_md_from_prsr, eg_intr_md_for_dprs,
                   eg_intr_md_for_oport);
        }
#endif

#ifdef DIAG_PATTERN_SHIFT_ENABLE
        /* Tables for pattern shift experiment.
           Algorithm:
           32b pkt counter. take middle 5bit by modify_field_with_shift
           use the 5b as shift size
           Table matches on the 5b index and current pkt size.
           actions -- add 1B, or reduce 31B, or nop.
        */
        pattern_shift.apply(hdr, meta, eg_intr_md,
                   eg_intr_md_from_prsr, eg_intr_md_for_dprs,
                   eg_intr_md_for_oport);
#endif

#ifdef DIAG_PARDE_STRESS_POWER
        /* Table defined using macro TBL_TCAM_EG_INST */
        tbl_tcam_2_eg.apply();
        tbl_tcam_3_eg.apply();
        tbl_tcam_4_eg.apply();
        tbl_tcam_5_eg.apply();
#if !defined(TOFINO2H)
        tbl_tcam_6_eg.apply();
        tbl_tcam_7_eg.apply();
        tbl_tcam_8_eg.apply();
        tbl_tcam_9_eg.apply();
#endif
#if DIAG_STAGES > 10
        tbl_tcam_10_eg.apply();
        tbl_tcam_11_eg.apply();
#endif
#if DIAG_STAGES > 12
        tbl_tcam_12_eg.apply();
        tbl_tcam_13_eg.apply();
        tbl_tcam_14_eg.apply();
        tbl_tcam_15_eg.apply();
        tbl_tcam_16_eg.apply();
        tbl_tcam_17_eg.apply();
#endif // DIAG_STAGES > 12
#endif // DIAG_PARDE_STRESS_POWER

#ifdef DIAG_HOLD_STRESS
        tbl_egr_mir.apply();
#endif

#ifdef DIAG_MAU_BUS_STRESS_ENABLE
        mau_bus_stress_eg.apply(hdr, meta, eg_intr_md,
                   eg_intr_md_from_prsr, eg_intr_md_for_dprs,
                   eg_intr_md_for_oport);
#endif

#ifdef DIAG_PARDE_STRAIN
        meta.l2_metadata.parde_strain_random = rnd_eg.get();

        /* Parde strain Headers */
        DIAG_PARDE_STRAIN_LOGIC(1)
        DIAG_PARDE_STRAIN_LOGIC(2)
        DIAG_PARDE_STRAIN_LOGIC(3)
        DIAG_PARDE_STRAIN_LOGIC(4)
        DIAG_PARDE_STRAIN_LOGIC(5)
#endif

    }
} // SwitchEgress

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         SwitchEgressParser(),
         SwitchEgress(),
         SwitchEgressDeparser()) pipeline_profile;

#endif // !DIAG_PHV_FLOP_TEST

Switch(pipeline_profile) main;

