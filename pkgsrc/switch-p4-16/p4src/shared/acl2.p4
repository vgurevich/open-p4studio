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



#ifndef _P4_ACL2_
#define _P4_ACL2_

//-----------------------------------------------------------------------------
// Ingress MAC ACL
//-----------------------------------------------------------------------------
control IngressMacAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    // sai_action : forward
    action no_action() {
        stats.count();
    }

    // sai_action : drop
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }

    // sai_action : transit/copy cancel
    action transit() {
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : deny
    action deny() {
        local_md.flags.acl_deny = true;
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : trap 
    action trap(switch_hostif_trap_t trap_id) {
        local_md.hostif_trap_id = trap_id;
        stats.count();
    }

    action disable_nat(bool nat_dis) {
#ifdef NAT_ENABLE
        local_md.nat.nat_disable = nat_dis;
#endif
        stats.count();
    }

    table acl {
        key = {
            local_md.lkp.mac_src_addr : ternary;
            local_md.lkp.mac_dst_addr : ternary;
            local_md.lkp.mac_type : ternary;
            local_md.lkp.pcp : ternary;
            local_md.lkp.dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.ingress_port_lag_index : ternary;
            //local_md.flags.rmac_hit : ternary;
        }

        actions = {
            drop;
            deny;
            transit;
            trap;
            disable_nat;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Ingress IPv4 ACL
//-----------------------------------------------------------------------------
control IngressIpv4Acl(inout switch_local_metadata_t local_md)(
                       switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    // sai_action : forward
    action no_action() {
        stats.count();
    }

     // sai_action : drop
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }

    // sai_action : transit/copy_cancel
    action transit() {
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : deny
    action deny() {
        local_md.flags.acl_deny = true;
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : trap 
    action trap(switch_hostif_trap_t trap_id) {
        local_md.hostif_trap_id = trap_id;
        stats.count();
    }

    action disable_nat(bool nat_dis) {
#ifdef NAT_ENABLE
        local_md.nat.nat_disable = nat_dis;
#endif
        stats.count();
    }

    table acl {
        key = {
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr[95:64] : ternary;
            local_md.lkp.ip_dst_addr[95:64] : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.l4_src_port_label : ternary;
            local_md.l4_dst_port_label : ternary;
            local_md.ingress_port_lag_index : ternary;
            local_md.flags.rmac_hit : ternary;
        }

        actions = {
            drop;
            deny;
            transit;
            trap;
            disable_nat;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Ingress IPv6 ACL
//-----------------------------------------------------------------------------
control IngressIpv6Acl(inout switch_local_metadata_t local_md)(
                       switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    // sai_action : forward
    action no_action() {
        stats.count();
    }

    // sai_action : drop
    action drop() {
        local_md.flags.acl_deny = true;
        stats.count();
    }

    // sai_action : transit/copy_cancel
    action transit() {
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : deny
    action deny() {
        local_md.flags.acl_deny = true;
        local_md.flags.copy_cancel = true;
        stats.count();
    }

    // sai_action : trap 
    action trap(switch_hostif_trap_t trap_id) {
        local_md.hostif_trap_id = trap_id;
        stats.count();
    }

    action disable_nat(bool nat_dis) {
#ifdef NAT_ENABLE
        local_md.nat.nat_disable = nat_dis;
#endif
        stats.count();
    }

    table acl {
        key = {
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr : ternary;
            local_md.lkp.ip_dst_addr : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.l4_src_port_label : ternary;
            local_md.l4_dst_port_label : ternary;
            local_md.ingress_port_lag_index : ternary;
            local_md.flags.rmac_hit : ternary;
        }

        actions = {
            drop;
            deny;
            transit;
            trap;
            disable_nat;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Ingress QoS ACL
//-----------------------------------------------------------------------------
control IngressQosAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    // ************************* ACL TCAM *********************************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;
    DirectMeter(MeterType_t.BYTES) meter;

    action no_action(switch_acl_meter_id_t meter_index) {
        stats.count();
#ifdef INGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_meter(switch_acl_meter_id_t meter_index) {
        stats.count();
#ifdef INGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    // This functionality is not mandatory if there is an egress qos acl in the profile.
    action set_pcp(bit<3> pcp) {
        local_md.lkp.pcp = pcp;
        stats.count();
    }

    action set_tos(switch_uint8_t tos) {
        local_md.lkp.ip_tos = tos;
        stats.count();
    }

    action set_tc(switch_acl_meter_id_t meter_index, switch_tc_t tc) {
        local_md.qos.tc = tc;
        stats.count();
#ifdef INGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_color(switch_acl_meter_id_t meter_index, switch_pkt_color_t color) {
        local_md.qos.color = color;
        stats.count();
#ifdef INGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_qos_params(switch_acl_meter_id_t meter_index,
                          switch_tc_t tc,
                          switch_pkt_color_t color) {
        local_md.qos.tc = tc;
        local_md.qos.color = color;
        stats.count();
#ifdef INGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_index = meter_index;
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
#endif
    }

    table acl {
        key = {
            local_md.qos_mac_label : ternary;
            local_md.etype_label : ternary;
            local_md.lkp.pcp : ternary;
            local_md.lkp.dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr : ternary;
            local_md.lkp.ip_dst_addr : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.l4_src_port_label : ternary;
            local_md.l4_dst_port_label : ternary;
            local_md.ingress_port_lag_index : ternary;
            local_md.flags.rmac_hit : ternary;
        }

        // TODO combination of basic actions
        actions = {
            //set_pcp;
            //set_tos;
            set_tc;
            set_color;
            set_qos_params;
#ifdef INGRESS_ACL_METER_ENABLE
            set_meter;
#endif
            no_action;
        }

        const default_action = no_action(0);
        counters = stats;
#ifdef INGRESS_ACL_METER_ENABLE
        meters = meter;
#endif
        size = table_size;
    }

    // ************************* Meter Stats and action**************************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.BYTES) meter_stats;

//    @name(".ingress_acl_meter.count")
    action count() {
        meter_stats.count();
    }

    action drop_and_count() {
        meter_stats.count();
        
        local_md.drop_reason = SWITCH_DROP_REASON_INGRESS_ACL_METER;
    }

//    @name(".ingress_acl_meter.meter_action")
    table meter_action {
        key = {
            local_md.qos.acl_meter_color: exact;
            local_md.qos.acl_meter_index: exact;
        }

        actions = {
            @defaultonly NoAction;
            drop_and_count();
            count;
        }

        const default_action = NoAction;
        /* Single rate two color policer with drop/no-drop action only */
        size = table_size*2;
        counters = meter_stats;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
#ifdef INGRESS_ACL_METER_ENABLE
            meter_action.apply();
#endif
        }
    }
}

//-----------------------------------------------------------------------------
// Ingress Mirror ACL
//-----------------------------------------------------------------------------
control IngressMirrorAcl(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    // ************************* ACL TCAM *********************************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;
    DirectMeter(MeterType_t.BYTES) meter;

    action no_action(switch_mirror_meter_id_t meter_index) {
        stats.count();
#ifdef INGRESS_MIRROR_METER_ENABLE
        local_md.mirror.meter_color = (bit<2>)meter.execute();
        local_md.mirror.meter_index = meter_index;
#endif
    }

    action mirror_in(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
#ifdef FOLDED_SWITCH_PIPELINE
        hdr.fp_bridged_md.mirror_session_id = session_id;
#else
        local_md.mirror.session_id = session_id;
        local_md.mirror.type = SWITCH_MIRROR_TYPE_PORT;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_INGRESS;
#endif
        stats.count();
#ifdef INGRESS_MIRROR_METER_ENABLE
        local_md.mirror.meter_color = (bit<2>)meter.execute();
        local_md.mirror.meter_index = meter_index;
#endif
    }

    @stage(10)
    table acl {
        key = {
            local_md.mirror_mac_label : ternary;
            local_md.etype_label : ternary;
            local_md.lkp.pcp : ternary;
            local_md.lkp.dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr : ternary;
            local_md.lkp.ip_dst_addr : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.l4_src_port_label : ternary;
            local_md.l4_dst_port_label : ternary;
            local_md.ingress_port_lag_index : ternary;
            local_md.flags.rmac_hit : ternary;
        }

        actions = {
            mirror_in;
            no_action;
        }

        const default_action = no_action(0);
        counters = stats;
#ifdef INGRESS_MIRROR_METER_ENABLE
        meters = meter;
#endif
        size = table_size;
    }

    // ************************* Meter Stats and Action **********************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.BYTES) meter_stats;

//    @name(".ingress_acl_meter.count")
    action count() {
        meter_stats.count();
    }

    action drop_and_count() {
        meter_stats.count();
        local_md.mirror.type = SWITCH_MIRROR_TYPE_INVALID;
    }

//    @name(".ingress_acl_meter.meter_action")
    table meter_action {
        key = {
            local_md.mirror.meter_color: exact;
            local_md.mirror.meter_index: exact;
        }

        actions = {
            @defaultonly NoAction;
            drop_and_count();
            count;
        }

        const default_action = NoAction;
        size = table_size*2;
        counters = meter_stats;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
#ifdef INGRESS_MIRROR_METER_ENABLE
            meter_action.apply();
#endif
        }
    }
}

//-----------------------------------------------------------------------------
// Ingress PBR ACL
//-----------------------------------------------------------------------------
control IngressPbrAcl(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    action no_action() {
        stats.count();
    }

    action redirect_port(switch_port_lag_index_t egress_port_lag_index) {
#ifdef FOLDED_SWITCH_PIPELINE
        hdr.fp_bridged_md.fwd_type = SWITCH_FWD_TYPE_L2;
        hdr.fp_bridged_md.fwd_idx = (switch_fwd_idx_t)egress_port_lag_index;
        hdr.fp_bridged_md.routed = false;
#else
        local_md.egress_port_lag_index = egress_port_lag_index;
        local_md.nexthop = 0;
        local_md.flags.routed = false;
#endif
        local_md.acl_port_redirect = true;
        stats.count();
    }

    action redirect_nexthop(switch_nexthop_t nexthop_index) {
#ifdef FOLDED_SWITCH_PIPELINE
        hdr.fp_bridged_md.fwd_type = SWITCH_FWD_TYPE_L3;
        hdr.fp_bridged_md.fwd_idx =(switch_fwd_idx_t)nexthop_index;
        hdr.fp_bridged_md.routed = true;
        hdr.fp_bridged_md.fib_lpm_miss = false;
#else
        local_md.nexthop = nexthop_index;
        local_md.flags.routed = true;
        local_md.flags.fib_lpm_miss = false;
#endif
        stats.count();
    }
#if __TARGET_TOFINO__ == 2
    @placement_priority(1)
#endif
    table acl {
        key = {
            local_md.pbr_mac_label : ternary;
            local_md.etype_label : ternary;
            local_md.lkp.pcp : ternary;
            local_md.lkp.dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr : ternary;
            local_md.lkp.ip_dst_addr : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.l4_src_port_label : ternary;
            local_md.l4_dst_port_label : ternary;
            local_md.ingress_port_lag_index : ternary;
            local_md.flags.rmac_hit : ternary;
        }

        actions = {
            redirect_port;
            redirect_nexthop;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!INGRESS_BYPASS(ACL)) {
            acl.apply();
        }
    }
}

// ----------------------------------------------------------------------------
// Range check for L4 Source/Destinations port
// ----------------------------------------------------------------------------
control LOU(inout switch_local_metadata_t local_md) {

    const switch_uint32_t table_size = 64 * 1024;

    @name(".set_ingress_src_port_label")
    action set_src_port_label(bit<8> label) {
        local_md.l4_src_port_label = label;
    }

    @name(".set_ingress_dst_port_label")
    action set_dst_port_label(bit<8> label) {
        local_md.l4_dst_port_label = label;
    }

#ifndef FOLDED_SWITCH_PIPELINE
    @name(".ingress_l4_dst_port")
#endif /* !FOLDED_SWITCH_PIPELINE */
    @use_hash_action(1) @pack(16)
    table l4_dst_port {
        key = {
            local_md.lkp.l4_dst_port : exact;
        }

        actions = {
            set_dst_port_label;
        }

        const default_action = set_dst_port_label(0);
        size = table_size;
    }

#ifndef FOLDED_SWITCH_PIPELINE
    @name(".ingress_l4_src_port")
#endif /* !FOLDED_SWITCH_PIPELINE */
    @use_hash_action(1) @pack(16)
    table l4_src_port {
        key = {
            local_md.lkp.l4_src_port : exact;
        }

        actions = {
            set_src_port_label;
        }

        const default_action = set_src_port_label(0);
        size = table_size;
    }

    apply {
        l4_src_port.apply();
        l4_dst_port.apply();
    }
}

// -------------------------------------------------------------------------------
// Take advantage of sparsity of ethertye values to compress it to a shorter field
// -------------------------------------------------------------------------------
control AclEtype(inout switch_local_metadata_t local_md) {

    action set_etype_label(switch_etype_label_t label) {
        local_md.etype_label = label;
    }

    table etype {
        key = {
            local_md.lkp.mac_type : exact;
        }

        actions = {
            set_etype_label;
        }

        const default_action = set_etype_label(0);
        size = 1 << switch_etype_label_width;
    }

    apply {
        etype.apply();
    }
}

// -------------------------------------------------------------------------------
// Compress MAC address to a shorter field
// -------------------------------------------------------------------------------
control AclMacAddr(in switch_port_lag_index_t port_lag_index,
                   in mac_addr_t smac_addr,
                   in mac_addr_t dmac_addr,
                   out switch_mac_addr_label_t mac_label) {

    action set_mac_addr_label(switch_mac_addr_label_t label) {
        mac_label = label;
    }

    table mac {
        key = {
            port_lag_index : ternary;
            smac_addr : ternary;
            dmac_addr : ternary;
        }

        actions = {
            set_mac_addr_label;
        }

        const default_action = set_mac_addr_label(0);
        size = 512;
    }

    apply {
        mac.apply();
    }
}

//-----------------------------------------------------------------------------
//
// Ingress System ACL
//
//-----------------------------------------------------------------------------
control IngressSystemAcl(
        in switch_header_t hdr,
        inout switch_local_metadata_t local_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr)(
        switch_uint32_t table_size=512) {

    const switch_uint32_t drop_stats_table_size = 8192;

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) stats;

    @name(".ingress_copp_meter")
    Meter<bit<8>>(1 << switch_copp_meter_id_width, MeterType_t.PACKETS) copp_meter;
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) copp_stats;

    switch_copp_meter_id_t copp_meter_id;

    @name(".ingress_system_acl_permit")
    action permit() {
        local_md.drop_reason = SWITCH_DROP_REASON_UNKNOWN;
    }
    @name(".ingress_system_acl_drop")
    action drop(switch_drop_reason_t drop_reason, bool disable_learning) {
        ig_intr_md_for_dprsr.drop_ctl = 0x1;
        ig_intr_md_for_dprsr.digest_type =
            disable_learning ? SWITCH_DIGEST_TYPE_INVALID : ig_intr_md_for_dprsr.digest_type;
        local_md.drop_reason = drop_reason;
#ifdef NAT_ENABLE
        local_md.nat.hit = SWITCH_NAT_HIT_NONE;
#endif
    }

    @name(".ingress_system_acl_copy_to_cpu_cancel")
    action copy_to_cpu_cancel() {
        ig_intr_md_for_tm.copy_to_cpu = 1w0;
    }
    @name(".ingress_system_acl_deny")
    action deny(switch_drop_reason_t drop_reason, bool disable_learning) {
        drop(drop_reason, disable_learning);
        copy_to_cpu_cancel();
    }

    @name(".ingress_system_acl_copy_to_cpu")
    action copy_to_cpu(switch_cpu_reason_t reason_code,
                       switch_qid_t qid,
                       switch_copp_meter_id_t meter_id,
                       bool disable_learning, bool overwrite_qid) {
        local_md.qos.qid = overwrite_qid ? qid : local_md.qos.qid;
        // local_md.qos.icos = icos;
        ig_intr_md_for_tm.copy_to_cpu = 1w1;
        ig_intr_md_for_dprsr.digest_type =
            disable_learning ? SWITCH_DIGEST_TYPE_INVALID : ig_intr_md_for_dprsr.digest_type;
        ig_intr_md_for_tm.packet_color = (bit<2>) copp_meter.execute(meter_id);
        copp_meter_id = meter_id;
        local_md.cpu_reason = reason_code;
        local_md.drop_reason = SWITCH_DROP_REASON_UNKNOWN;
    }

    @name(".ingress_system_acl_copy_sflow_to_cpu")
    action copy_sflow_to_cpu(switch_cpu_reason_t reason_code,
                             switch_qid_t qid,
                             switch_copp_meter_id_t meter_id,
                             bool disable_learning, bool overwrite_qid) {
        copy_to_cpu(reason_code + (bit<16>)local_md.sflow.session_id, qid, meter_id, disable_learning, overwrite_qid);
    }

    @name(".ingress_system_acl_redirect_to_cpu")
    action redirect_to_cpu(switch_cpu_reason_t reason_code,
                           switch_qid_t qid,
                           switch_copp_meter_id_t meter_id,
                           bool disable_learning, bool overwrite_qid) {
        ig_intr_md_for_dprsr.drop_ctl = 0b1;
#ifdef NAT_ENABLE
        local_md.nat.hit = SWITCH_NAT_HIT_NONE;
#endif
        local_md.flags.redirect_to_cpu = true;
        copy_to_cpu(reason_code, qid, meter_id, disable_learning, overwrite_qid);
    }

    @name(".ingress_system_acl_redirect_sflow_to_cpu")
    action redirect_sflow_to_cpu(switch_cpu_reason_t reason_code,
                                 switch_qid_t qid,
                                 switch_copp_meter_id_t meter_id,
                                 bool disable_learning, bool overwrite_qid) {
        ig_intr_md_for_dprsr.drop_ctl = 0b1;
        local_md.flags.redirect_to_cpu = true;
        copy_sflow_to_cpu(reason_code, qid, meter_id, disable_learning, overwrite_qid);
    }

    @name(".ingress_system_acl")
    table system_acl {
        key = {
            local_md.bd : ternary;
            local_md.ingress_port_lag_index : ternary;

            // Lookup fields
            local_md.lkp.pkt_type : ternary;
            local_md.lkp.mac_type : ternary;
            local_md.lkp.mac_dst_addr : ternary;
            local_md.lkp.ip_type : ternary;
            local_md.lkp.ip_ttl : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.ip_frag : ternary;
            local_md.lkp.ip_dst_addr : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.arp_opcode : ternary;

            // Flags
            local_md.flags.vlan_arp_suppress : ternary;
            local_md.flags.vrf_ttl_violation : ternary;
            local_md.flags.vrf_ttl_violation_valid : ternary;
            local_md.flags.vrf_ip_options_violation : ternary;
#ifndef FOLDED_SWITCH_PIPELINE
            local_md.flags.port_vlan_miss : ternary;
#endif
            local_md.flags.acl_deny : ternary;
            local_md.flags.copy_cancel : ternary;
            local_md.flags.rmac_hit : ternary;
            local_md.flags.dmac_miss : ternary;
            local_md.flags.myip : ternary;
            local_md.flags.glean : ternary;
            local_md.flags.routed : ternary;
            local_md.flags.fib_lpm_miss : ternary;
            local_md.flags.fib_drop : ternary;
//            local_md.qos.acl_meter_color : ternary;
            local_md.qos.storm_control_color : ternary;
            local_md.flags.link_local : ternary;
            local_md.flags.port_meter_drop : ternary;

#ifndef DO_NOT_USE_SAME_IF
            local_md.checks.same_if : ternary;
#endif
#ifndef FOLDED_SWITCH_PIPELINE
            local_md.stp.state_ : ternary;
#endif
            local_md.flags.pfc_wd_drop : ternary;
            local_md.ipv4.unicast_enable : ternary;
            local_md.ipv6.unicast_enable : ternary;
            local_md.checks.mrpf : ternary;
            local_md.ipv4.multicast_enable : ternary;
            local_md.ipv4.multicast_snooping : ternary;
            local_md.ipv6.multicast_enable : ternary;
            local_md.ipv6.multicast_snooping : ternary;
            local_md.multicast.hit : ternary;
            local_md.flags.vrf_unknown_l3_multicast_trap : ternary;
            local_md.sflow.sample_packet : ternary;
#ifndef FOLDED_SWITCH_PIPELINE
            local_md.l2_drop_reason : ternary;
#endif
            local_md.drop_reason : ternary;
#ifdef NAT_ENABLE
            // punt packets to cpu if nat.hit == SWITCH_NAT_HIT_TYPE_DEST_NONE
            local_md.nat.hit : ternary;
            local_md.checks.same_zone_check : ternary;
#endif
            local_md.tunnel.terminate : ternary;
            local_md.hostif_trap_id : ternary;

            // Header fields
            hdr.ipv4_option.isValid() : ternary;
        }

        actions = {
            permit;
            drop;
            deny;
            copy_to_cpu;
            copy_to_cpu_cancel;
            redirect_to_cpu;
            copy_sflow_to_cpu;
            redirect_sflow_to_cpu;
        }

        const default_action = permit;
        size = table_size;
    }

    @name(".ingress_copp_drop")
    action copp_drop() {
        ig_intr_md_for_tm.copy_to_cpu = 1w0;
        copp_stats.count();
    }

    @name(".ingress_copp_permit")
    action copp_permit() {
        copp_stats.count();
    }

    @name(".ingress_copp")
    table copp {
        key = {
            ig_intr_md_for_tm.packet_color : ternary;
            copp_meter_id : ternary;
        }

        actions = {
            copp_permit;
            copp_drop;
        }

        const default_action = copp_permit;
        size = (1 << switch_copp_meter_id_width + 1);
        counters = copp_stats;
    }

    @name(".ingress_drop_stats_count")
    action count() { stats.count(); }

    @name(".ingress_drop_stats")
    table drop_stats {
        key = {
            local_md.drop_reason : exact @name("drop_reason");
            local_md.ingress_port : exact @name("port");
        }

        actions = {
            @defaultonly NoAction;
            count;
        }

        const default_action = NoAction;
        counters = stats;
        size = drop_stats_table_size;
    }

    apply {
        if (!INGRESS_BYPASS(SYSTEM_ACL)) {
            switch(system_acl.apply().action_run) {
                copy_to_cpu : { copp.apply(); }
                redirect_to_cpu : { copp.apply(); }
                default: {}
            }
        }
        drop_stats.apply();
    }
}


//-----------------------------------------------------------------------------
// Egress MAC ACL
//-----------------------------------------------------------------------------
control EgressMacAcl(in switch_header_t hdr,
                     inout switch_local_metadata_t local_md)(
                     switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    action no_action() {
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    action drop() {
        local_md.flags.acl_deny = true;
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    table acl {
        key = {
            hdr.ethernet.src_addr : ternary;
            hdr.ethernet.dst_addr : ternary;
            local_md.lkp.mac_type : ternary;
            hdr.vlan_tag[0].pcp : ternary;
            hdr.vlan_tag[0].dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.egress_port_lag_index : ternary;
            local_md.flags.routed : ternary;
        }

        actions = {
            drop;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Egress IPv4 ACL
//-----------------------------------------------------------------------------
control EgressIpv4Acl(in switch_header_t hdr,
                      inout switch_local_metadata_t local_md)(
                      switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    action no_action() {
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    action drop() {
        local_md.flags.acl_deny = true;
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    table acl {
        key = {
            local_md.bd[12:0]: ternary @name("local_md.bd");
            local_md.lkp.ip_src_addr_95_64 : ternary;
            local_md.lkp.ip_dst_addr_95_64 : ternary;
            local_md.lkp.ip_proto : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.egress_port_lag_index : ternary;
            local_md.flags.routed : ternary;
        }

        actions = {
            drop;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Egress IPv6 ACL
//-----------------------------------------------------------------------------
control EgressIpv6Acl(in switch_header_t hdr,
                      inout switch_local_metadata_t local_md)(
                      switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    action no_action() {
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    action drop() {
        local_md.flags.acl_deny = true;
        stats.count(sizeInBytes(hdr.bridged_md));
    }

    table acl {
        key = {
            local_md.bd[12:0]: ternary @name("local_md.bd");

            hdr.ipv6.src_addr[127:96] : ternary;
            local_md.lkp.ip_src_addr_95_64 : ternary;
            hdr.ipv6.src_addr[63:0] : ternary;

            hdr.ipv6.dst_addr[127:96] : ternary;
            local_md.lkp.ip_dst_addr_95_64 : ternary;
            hdr.ipv6.dst_addr[63:0] : ternary;

            local_md.lkp.ip_proto : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.egress_port_lag_index : ternary;
            local_md.flags.routed : ternary;
        }

        actions = {
            drop;
            no_action;
        }

        const default_action = no_action;
        counters = stats;
        size = table_size;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
        }
    }
}

//-----------------------------------------------------------------------------
// Egress QoS ACL
//-----------------------------------------------------------------------------
control EgressQosAcl(inout switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    // ************************* ACL TCAM *********************************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;
    DirectMeter(MeterType_t.BYTES) meter;

    action no_action(switch_acl_meter_id_t meter_index) {
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_meter(switch_acl_meter_id_t meter_index) {
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_pcp(bit<3> pcp, switch_acl_meter_id_t meter_index) {
        hdr.vlan_tag[0].pcp = pcp;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_ipv4_tos(switch_uint8_t tos, switch_acl_meter_id_t meter_index) {
        hdr.ipv4.diffserv = tos;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_ipv6_tos(switch_uint8_t tos, switch_acl_meter_id_t meter_index) {
        hdr.ipv6.traffic_class = tos;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_pcp_and_ipv4_tos(bit<3> pcp, switch_uint8_t tos, switch_acl_meter_id_t meter_index) {
        hdr.vlan_tag[0].pcp = pcp;
        hdr.ipv4.diffserv = tos;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

    action set_pcp_and_ipv6_tos(bit<3> pcp, switch_uint8_t tos, switch_acl_meter_id_t meter_index) {
        hdr.vlan_tag[0].pcp = pcp;
        hdr.ipv6.traffic_class = tos;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_ACL_METER_ENABLE
        local_md.qos.acl_meter_color = (bit<2>)meter.execute();
        local_md.qos.acl_meter_index = meter_index;
#endif
    }

#ifdef FOLDED_SWITCH_PIPELINE
    @ignore_table_dependency("SwitchEgress_0.egress_ip_mirror_acl.acl")
#else
    @ignore_table_dependency("SwitchEgress.egress_ip_mirror_acl.acl")
#endif
    table acl {
        key = {
            local_md.qos_mac_label : ternary;
            local_md.lkp.mac_type : ternary;
            hdr.vlan_tag[0].pcp : ternary;
            hdr.vlan_tag[0].dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");

            hdr.ipv6.src_addr[127:96] : ternary;
            local_md.lkp.ip_src_addr_95_64 : ternary;
            hdr.ipv6.src_addr[63:0] : ternary;

            hdr.ipv6.dst_addr[127:96] : ternary;
            local_md.lkp.ip_dst_addr_95_64 : ternary;
            hdr.ipv6.dst_addr[63:0] : ternary;

            local_md.lkp.ip_proto : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.egress_port_lag_index : ternary;
            local_md.lkp.ip_type : ternary;
            local_md.flags.routed : ternary;
        }

        actions = {
#ifdef EGRESS_ACL_METER_ENABLE
            set_meter;
#endif
            set_pcp;
            set_ipv4_tos;
            set_ipv6_tos;
            set_pcp_and_ipv4_tos;
            set_pcp_and_ipv6_tos;
            no_action;
        }

        const default_action = no_action(0);
        counters = stats;
#ifdef EGRESS_ACL_METER_ENABLE
        meters = meter;
#endif
        size = table_size;
    }

    // ************************* Meter Stats and Action ***********************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.BYTES) meter_stats;

//    @name(".egress_acl_meter.count")
    action count() {
        meter_stats.count();
    }

//    @Name(".egress_acl_meter.meter_action")
    table meter_action {
        key = {
            local_md.qos.acl_meter_color : exact;
            local_md.qos.acl_meter_index : exact;
        }

        actions = {
            @defaultonly NoAction;
            count;
        }

        const default_action = NoAction;
        size = table_size*2;
        counters = meter_stats;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
#ifdef EGRESS_ACL_METER_ENABLE
            meter_action.apply();
#endif
        }
    }
}

//-----------------------------------------------------------------------------
// Egress Mirror ACL
//-----------------------------------------------------------------------------
control EgressMirrorAcl(in switch_header_t hdr, inout switch_local_metadata_t local_md)(
        switch_uint32_t table_size=512) {

    // ************************* ACL TCAM *********************************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;
    DirectMeter(MeterType_t.BYTES) meter;

    action no_action(switch_mirror_meter_id_t meter_index) {
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_MIRROR_METER_ENABLE
        local_md.mirror.meter_color = (bit<2>)meter.execute();
        local_md.mirror.meter_index = meter_index;
#endif
    }

    action mirror_out(switch_mirror_meter_id_t meter_index, switch_mirror_session_t session_id) {
        local_md.mirror.type = SWITCH_MIRROR_TYPE_PORT;
        local_md.mirror.src = SWITCH_PKT_SRC_CLONED_EGRESS;
        local_md.mirror.session_id = session_id;
        local_md.mirror.meter_index = meter_index;
        stats.count(sizeInBytes(hdr.bridged_md));
#ifdef EGRESS_MIRROR_METER_ENABLE
        local_md.mirror.meter_color = (bit<2>)meter.execute();
        local_md.mirror.meter_index = meter_index;
#endif
    }

    /* Note: Mirror ACL will not match on the final PCP eoverwritten by QoS ACL" */
#ifdef FOLDED_SWITCH_PIPELINE
    @ignore_table_dependency("SwitchEgress_0.egress_ip_qos_acl.acl")
#else
    @ignore_table_dependency("SwitchEgress.egress_ip_qos_acl.acl")
#endif
    table acl {
        key = {
            local_md.mirror_mac_label : ternary;
            local_md.lkp.mac_type : ternary;
            hdr.vlan_tag[0].pcp : ternary;
            hdr.vlan_tag[0].dei : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            local_md.bd[12:0]: ternary @name("local_md.bd");

            hdr.ipv6.src_addr[127:96] : ternary;
            local_md.lkp.ip_src_addr_95_64 : ternary;
            hdr.ipv6.src_addr[63:0] : ternary;

            hdr.ipv6.dst_addr[127:96] : ternary;
            local_md.lkp.ip_dst_addr_95_64 : ternary;
            hdr.ipv6.dst_addr[63:0] : ternary;

            local_md.lkp.ip_proto : ternary;
            local_md.lkp.ip_tos : ternary;
            local_md.lkp.tcp_flags : ternary;
            local_md.lkp.l4_dst_port : ternary;
            local_md.lkp.l4_src_port : ternary;
            local_md.egress_port_lag_index : ternary;
            local_md.lkp.ip_type : ternary;
            local_md.flags.routed : ternary;
        }

        actions = {
            mirror_out;
            no_action;
        }

        const default_action = no_action(0);
        counters = stats;
#ifdef EGRESS_MIRROR_METER_ENABLE
        meters = meter;
#endif
        size = table_size;
    }

    // ************************* Meter Stats and Action **********************
    DirectCounter<bit<switch_counter_width>>(CounterType_t.BYTES) meter_stats;

//    @name(".egress_acl_meter.count")
    action count() {
        meter_stats.count();
    }

//    @name(".egress_acl_meter.meter_action")
    table meter_action {
        key = {
            local_md.mirror.meter_color: exact;
            local_md.mirror.meter_index: exact;
        }

        actions = {
            @defaultonly NoAction;
            count;
        }

        const default_action = NoAction;
        size = table_size*2;
        counters = meter_stats;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            acl.apply();
#ifdef EGRESS_MIRROR_METER_ENABLE
            meter_action.apply();
#endif
        }
    }
}

//-----------------------------------------------------------------------------
//
// Egress System ACL
//
//-----------------------------------------------------------------------------
control EgressSystemAcl(
        inout switch_header_t hdr,
        inout switch_local_metadata_t local_md,
        in egress_intrinsic_metadata_t eg_intr_md,
        out egress_intrinsic_metadata_for_deparser_t eg_intr_md_for_dprsr)(
        switch_uint32_t table_size=512) {

    // Take care of any drop conditions triggered earlier in the pipeline
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) drop_stats;

    @name(".egress_system_acl_drop")
    action drop(switch_drop_reason_t reason_code) {
        local_md.drop_reason = reason_code;
        eg_intr_md_for_dprsr.drop_ctl = 0x1;
        local_md.mirror.type = SWITCH_MIRROR_TYPE_INVALID;
        drop_stats.count(sizeInBytes(hdr.bridged_md));
    }

#ifdef PTP_ENABLE
    @name(".egress_system_acl_ingress_timestamp")
    action insert_timestamp() {
        hdr.timestamp.setValid();
        hdr.timestamp.timestamp = local_md.ingress_timestamp;
        drop_stats.count(sizeInBytes(hdr.bridged_md));
    }
#endif /* PTP_ENABLE */

    action mirror_meter_drop(switch_drop_reason_t reason_code) {
        local_md.mirror.type = SWITCH_MIRROR_TYPE_INVALID;
        drop_stats.count(sizeInBytes(hdr.bridged_md));
    }

    @name(".egress_system_acl")
    table system_acl {
        key = {
            eg_intr_md.egress_port : ternary;
            local_md.flags.acl_deny : ternary;
            local_md.checks.mtu : ternary;
            local_md.checks.stp : ternary;
            local_md.flags.wred_drop : ternary;
            local_md.flags.pfc_wd_drop : ternary;
            local_md.qos.acl_meter_color : ternary;
            local_md.flags.port_meter_drop : ternary;
            local_md.mirror.meter_color : ternary;
            
            //local_md.checks.same_if : ternary;
            local_md.flags.port_isolation_packet_drop : ternary;
            local_md.flags.bport_isolation_packet_drop : ternary;
        }

        /* Programming Note: acl entry for mirror_meter_drop is the lowest priority entry and doesn't need to match on egress port */
        actions = {
            drop;
            mirror_meter_drop;
#ifdef PTP_ENABLE
            insert_timestamp;
#endif /* PTP_ENABLE */
        }

        counters = drop_stats;
        size = table_size;
    }

    apply {
        if (!local_md.flags.bypass_egress) {
            system_acl.apply();
        }
    }
}

#endif /* _P4_ACL2_ */
