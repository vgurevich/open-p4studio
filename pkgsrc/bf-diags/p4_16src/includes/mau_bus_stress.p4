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

#ifndef _DIAG_MAU_BUS_STRESS_
#define _DIAG_MAU_BUS_STRESS_

#ifdef DIAG_MAU_BUS_STRESS_ENABLE

control ProcessMauBusStressIg(inout headers_t hdr, inout i_metadata meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {
        
    @name(".mau_bus_stress_nop") action mau_bus_stress_nop() {
    }
    @name(".mau_bus_stress_cntr_inc_ig") action mau_bus_stress_cntr_inc_ig() {
        meta.l2_metadata.mau_exm_cntr = meta.l2_metadata.mau_exm_cntr + 16w1;
        meta.l2_metadata.mau_tcam_cntr = meta.l2_metadata.mau_tcam_cntr + 16w1;
    }
    @name(".mau_bus_stress_hdr_add") action mau_bus_stress_hdr_add() {
        hdr.mau_bus_stress_hdr.setValid();
    }
    @name(".set_mau_exm_key_ig") action set_mau_exm_key_ig(mau_stress_hdr_exm_key_len_t val) {
        hdr.mau_bus_stress_hdr.exm_key = val;
        meta.l2_metadata.mau_exm_cntr = meta.l2_metadata.mau_exm_cntr + 16w1;
    }
    @name(".stage_drop") action stage_drop() {
        ig_dprsr_md.drop_ctl = 0x1; // Drop packet.
    }
    @name(".set_mau_tcam_key_ig") action set_mau_tcam_key_ig(mau_stress_hdr_tcam_key_len_t val) {
        hdr.mau_bus_stress_hdr.tcam_key = val;
        meta.l2_metadata.mau_tcam_cntr = meta.l2_metadata.mau_tcam_cntr + 16w1;
    }

#define MAU_EXM_TABLE_IG_INST(n)                                 \
    @stage(##n) @ways(5)                                         \
    @name(STR(.mau_exm_table##n##_ig)) table mau_exm_table##n##_ig { \
        actions = {                                              \
            mau_bus_stress_nop;                                  \
            set_mau_exm_key_ig;                                  \
        }                                                        \
        key = {                                                  \
            hdr.mau_bus_stress_hdr.exm_key  : exact;             \
            meta.l2_metadata.mau_exm_cntr: exact;                \
        }                                                        \
        size = MAU_BUS_STRESS_EXM_TBL_SIZE;                      \
    }

#define MAU_EXM_TABLE_IG_DROP_INST(n)                            \
    @stage(##n)                                                  \
    @name(STR(.mau_exm_table##n##_ig)) table mau_exm_table##n##_ig { \
        actions = {                                              \
            mau_bus_stress_nop;                                  \
            mau_bus_stress_cntr_inc_ig;                          \
            stage_drop;                                          \
        }                                                        \
        key = {                                                  \
            hdr.mau_bus_stress_hdr.exm_key   : exact;            \
            hdr.mau_bus_stress_hdr.tcam_key  : exact;            \
            meta.l2_metadata.mau_exm_cntr : exact;               \
            meta.l2_metadata.mau_tcam_cntr: exact;               \
        }                                                        \
        const default_action = stage_drop;                       \
        size = MAU_BUS_STRESS_EXM_TBL_SIZE;                      \
    }
   
    @stage(1) @ways(5)       
    @name(".mau_exm_table1_ig") table mau_exm_table1_ig {
        actions = {        
            mau_bus_stress_nop;
            set_mau_exm_key_ig;
        }
        key = {
            meta.l2_metadata.mau_exm_cntr: exact;
        }                
        size = MAU_BUS_STRESS_EXM_TBL_SIZE;
    }
    MAU_EXM_TABLE_IG_INST(2)
    MAU_EXM_TABLE_IG_INST(3)
    MAU_EXM_TABLE_IG_INST(4)
#if defined(TOFINO2H)
    MAU_EXM_TABLE_IG_DROP_INST(5)
#else
    MAU_EXM_TABLE_IG_INST(5)
#endif
    MAU_EXM_TABLE_IG_INST(6)
    MAU_EXM_TABLE_IG_INST(7)
    MAU_EXM_TABLE_IG_INST(8)
    MAU_EXM_TABLE_IG_INST(9)
    MAU_EXM_TABLE_IG_INST(10)
#if defined(TOFINO1) || defined(TOFINO2M)
    MAU_EXM_TABLE_IG_DROP_INST(11)
#else
    MAU_EXM_TABLE_IG_INST(11)
#endif
    MAU_EXM_TABLE_IG_INST(12)
    MAU_EXM_TABLE_IG_INST(13)
    MAU_EXM_TABLE_IG_INST(14)
    MAU_EXM_TABLE_IG_INST(15)
    MAU_EXM_TABLE_IG_INST(16)
    MAU_EXM_TABLE_IG_INST(17)
    MAU_EXM_TABLE_IG_INST(18)
    MAU_EXM_TABLE_IG_DROP_INST(19)

#define MAU_TCAM_TABLE_IG_INST(n)                       \
    @stage(##n)                                         \
    @name(STR(.mau_tcam_table##n##_ig)) table mau_tcam_table##n##_ig { \
        actions = {                                     \
            mau_bus_stress_nop;                         \
            set_mau_tcam_key_ig;                        \
        }                                               \
        key = {                                         \
            hdr.mau_bus_stress_hdr.tcam_key  : ternary; \
            meta.l2_metadata.mau_tcam_cntr: ternary;    \
        }                                               \
        size = MAU_BUS_STRESS_TCAM_TBL_SIZE;            \
    }

    MAU_TCAM_TABLE_IG_INST(1)
    MAU_TCAM_TABLE_IG_INST(2)
    MAU_TCAM_TABLE_IG_INST(3)
    MAU_TCAM_TABLE_IG_INST(4)
    MAU_TCAM_TABLE_IG_INST(5)
    MAU_TCAM_TABLE_IG_INST(6)
    MAU_TCAM_TABLE_IG_INST(7)
    MAU_TCAM_TABLE_IG_INST(8)
    MAU_TCAM_TABLE_IG_INST(9)
    MAU_TCAM_TABLE_IG_INST(10)
    MAU_TCAM_TABLE_IG_INST(11)
    MAU_TCAM_TABLE_IG_INST(12)
    MAU_TCAM_TABLE_IG_INST(13)
    MAU_TCAM_TABLE_IG_INST(14)
    MAU_TCAM_TABLE_IG_INST(15)
    MAU_TCAM_TABLE_IG_INST(16)
    MAU_TCAM_TABLE_IG_INST(17)
    MAU_TCAM_TABLE_IG_INST(18)
    
    @name(".mau_bus_stress_hdr_add_ig") table mau_bus_stress_hdr_add_ig {
        actions = {
            mau_bus_stress_hdr_add;
        }
        const default_action = mau_bus_stress_hdr_add;
        size = 1;
    }

    apply {
        /* Flip the mau_bus_stress header between 0x00 and 0xff */
        mau_exm_table1_ig.apply();
        mau_tcam_table1_ig.apply();
        mau_exm_table2_ig.apply();
        mau_tcam_table2_ig.apply();
        mau_exm_table3_ig.apply();
        mau_tcam_table3_ig.apply();
        mau_exm_table4_ig.apply();
        mau_tcam_table4_ig.apply();
        mau_exm_table5_ig.apply();
#if !defined(TOFINO2H)
        mau_tcam_table5_ig.apply();
        mau_exm_table6_ig.apply();
        mau_tcam_table6_ig.apply();
        mau_exm_table7_ig.apply();
        mau_tcam_table7_ig.apply();
        mau_exm_table8_ig.apply();
        mau_tcam_table8_ig.apply();
        mau_exm_table9_ig.apply();
#endif
#if !defined(TOFINO2H)
        mau_tcam_table9_ig.apply();
        mau_exm_table10_ig.apply();
        mau_tcam_table10_ig.apply();
        mau_exm_table11_ig.apply();
#endif
#if !defined(TOFINO1) && !defined(TOFINO2M) && !defined(TOFINO2H)
        mau_tcam_table11_ig.apply();
        mau_exm_table12_ig.apply();
        mau_tcam_table12_ig.apply();
        mau_exm_table13_ig.apply();
        mau_tcam_table13_ig.apply();
        mau_exm_table14_ig.apply();
        mau_tcam_table14_ig.apply();
        mau_exm_table15_ig.apply();
        mau_tcam_table15_ig.apply();
        mau_exm_table16_ig.apply();
        mau_tcam_table16_ig.apply();
        mau_exm_table17_ig.apply();
        mau_tcam_table17_ig.apply();
        mau_exm_table18_ig.apply();
        mau_tcam_table18_ig.apply();
        /* Drop pkt if any mau_bus_stress header flip failed */
        mau_exm_table19_ig.apply();
#endif
        mau_bus_stress_hdr_add_ig.apply();
    }
}

control ProcessMauBusStressEg(inout headers_t hdr, inout e_metadata meta,
    in egress_intrinsic_metadata_t eg_intr_md,
    in egress_intrinsic_metadata_from_parser_t eg_intr_md_from_prsr,
    inout egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprs,
    inout egress_intrinsic_metadata_for_output_port_t eg_intr_md_for_oport) {
        
    @name(".mau_bus_stress_nop") action mau_bus_stress_nop() {
    }
    @name(".mau_bus_stress_cntr_inc_eg") action mau_bus_stress_cntr_inc_eg() {
        meta.l2_metadata.mau_exm_cntr = meta.l2_metadata.mau_exm_cntr + 16w1;
        meta.l2_metadata.mau_tcam_cntr = meta.l2_metadata.mau_tcam_cntr + 16w1;
    }
    @name(".stage_drop") action stage_drop() {
        eg_intr_md_for_dprs.drop_ctl = 0x1; // Drop packet.
    }
    @name(".set_mau_exm_key_eg") action set_mau_exm_key_eg(mau_stress_hdr_exm_key_len_t val) {
        hdr.mau_bus_stress_hdr.exm_key = val;
        meta.l2_metadata.mau_exm_cntr = meta.l2_metadata.mau_exm_cntr + 16w1;
    }
    @name(".set_mau_tcam_key_eg") action set_mau_tcam_key_eg(mau_stress_hdr_tcam_key_len_t val) {
        hdr.mau_bus_stress_hdr.tcam_key = val;
        meta.l2_metadata.mau_tcam_cntr = meta.l2_metadata.mau_tcam_cntr + 16w1;
    }

#define MAU_EXM_TABLE_EG_INST(n)                                 \
    @stage(##n)                                                  \
    @name(STR(.mau_exm_table##n##_eg)) table mau_exm_table##n##_eg { \
        actions = {                                              \
            mau_bus_stress_nop;                                  \
            set_mau_exm_key_eg;                                  \
        }                                                        \
        key = {                                                  \
            hdr.mau_bus_stress_hdr.exm_key  : exact;             \
            meta.l2_metadata.mau_exm_cntr: exact;                \
        }                                                        \
        size = MAU_BUS_STRESS_EXM_TBL_SIZE;                      \
    }

#define MAU_EXM_TABLE_EG_DROP_INST(n)                            \
    @stage(##n)                                                  \
    @name(STR(.mau_exm_table##n##_eg)) table mau_exm_table##n##_eg { \
        actions = {                                              \
            mau_bus_stress_nop;                                  \
            mau_bus_stress_cntr_inc_eg;                          \
            stage_drop;                                          \
        }                                                        \
        key = {                                                  \
            hdr.mau_bus_stress_hdr.exm_key   : exact;            \
            hdr.mau_bus_stress_hdr.tcam_key  : exact;            \
            meta.l2_metadata.mau_exm_cntr : exact;               \
            meta.l2_metadata.mau_tcam_cntr: exact;               \
        }                                                        \
        const default_action = stage_drop;                       \
        size = MAU_BUS_STRESS_EXM_TBL_SIZE;                      \
    }
  
    MAU_EXM_TABLE_EG_INST(1)
    MAU_EXM_TABLE_EG_INST(2)
    MAU_EXM_TABLE_EG_INST(3)
    MAU_EXM_TABLE_EG_INST(4)
#if defined(TOFINO2H) || defined(TOFINO1)
    /* To keep Tofino1 power usage within limit, reduce egress stage usage */
    MAU_EXM_TABLE_EG_DROP_INST(5)
#else
    MAU_EXM_TABLE_EG_INST(5)
#endif
    MAU_EXM_TABLE_EG_INST(6)
    MAU_EXM_TABLE_EG_INST(7)
    MAU_EXM_TABLE_EG_INST(8)
    MAU_EXM_TABLE_EG_DROP_INST(9)

#define MAU_TCAM_TABLE_EG_INST(n)                       \
    @stage(##n)                                         \
    @name(STR(.mau_tcam_table##n##_eg)) table mau_tcam_table##n##_eg { \
        actions = {                                     \
            mau_bus_stress_nop;                         \
            set_mau_tcam_key_eg;                        \
        }                                               \
        key = {                                         \
            hdr.mau_bus_stress_hdr.tcam_key  : ternary; \
            meta.l2_metadata.mau_tcam_cntr: ternary;    \
        }                                               \
        size = MAU_BUS_STRESS_TCAM_TBL_SIZE;            \
    }

    MAU_TCAM_TABLE_EG_INST(1)
    MAU_TCAM_TABLE_EG_INST(2)
    MAU_TCAM_TABLE_EG_INST(3)
    MAU_TCAM_TABLE_EG_INST(4)
    MAU_TCAM_TABLE_EG_INST(5)
    MAU_TCAM_TABLE_EG_INST(6)
    MAU_TCAM_TABLE_EG_INST(7)
    MAU_TCAM_TABLE_EG_INST(8)

    apply {
        /* Flip the mau_bus_stress header between 0x00 and 0xff */
        mau_exm_table1_eg.apply();
        mau_tcam_table1_eg.apply();
        mau_exm_table2_eg.apply();
        mau_tcam_table2_eg.apply();
        mau_exm_table3_eg.apply();
        mau_tcam_table3_eg.apply();
        mau_exm_table4_eg.apply();
        mau_tcam_table4_eg.apply();
        mau_exm_table5_eg.apply();
#if !defined(TOFINO2H) && !defined(TOFINO1)
        mau_tcam_table5_eg.apply();
        mau_exm_table6_eg.apply();
        mau_tcam_table6_eg.apply();
        mau_exm_table7_eg.apply();
        mau_tcam_table7_eg.apply();
        mau_exm_table8_eg.apply();
        mau_tcam_table8_eg.apply();
        /* Drop pkt if any mau_bus_stress header flip failed */
        mau_exm_table9_eg.apply();
#endif
    }
}

#endif

#endif /* _DIAG_MAU_BUS_STRESS_ */
