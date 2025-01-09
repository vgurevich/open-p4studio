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



#ifndef _P4_QOS_
#define _P4_QOS_

#include "acl.p4"

//-------------------------------------------------------------------------------------------------
// ECN Access control list
//
// @param local_md : Ingress metadata fields.
// @param lkp : Lookup fields.
// @param pkt_color : Packet color
// @param table_size : Size of the ACL table.
//-------------------------------------------------------------------------------------------------
control ECNAcl(in switch_local_metadata_t local_md,
               in switch_lookup_fields_t lkp,
               inout switch_pkt_color_t pkt_color)(
               switch_uint32_t table_size=512) {
    @name(".ecn_acl.set_ingress_color")
    action set_ingress_color(switch_pkt_color_t color) {
        pkt_color = color;
    }

    @name(".ecn_acl.acl")
    table acl {
        key =  {
            local_md.ingress_port_lag_label : ternary;
            lkp.ip_tos : ternary;
            lkp.tcp_flags : ternary;
        }

        actions = {
            NoAction;
            set_ingress_color;
        }

        const default_action = NoAction;
        size = table_size;
    }

    apply {
        acl.apply();
    }
}

//-------------------------------------------------------------------------------------------------
// Ingress PFC Watchdog
// Once PFC storm is detected on a queue, the PFC watchdog can drop or forward at per queue level.
// On drop action, all existing packets in the output queue and all subsequent packets destined to
// the output queue are discarded.
//
// @param port
// @param qid : Queue Id.
// @param table_size : Size of the ACL table.
//-------------------------------------------------------------------------------------------------
control IngressPFCWd(in switch_port_t port,
               in switch_qid_t qid,
               out bool flag)(
               switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    @name(".ingress_pfcwd.acl_deny")
    action acl_deny() {
        flag = true;
        stats.count();
    }

    @ways(2)
    @name(".ingress_pfcwd.acl")
    table acl {
        key = {
            qid : exact;
            port : exact;
        }

        actions = {
            @defaultonly NoAction;
            acl_deny;
        }

        const default_action = NoAction;
        counters = stats;
        size = table_size;
    }

    apply {
#ifdef PFC_ENABLE
        acl.apply();
#endif /* PFC_ENABLE */
    }
}

//-------------------------------------------------------------------------------------------------
// Egress PFC Watchdog
// Once PFC storm is detected on a queue, the PFC watchdog can drop or forward at per queue level.
// On drop action, all existing packets in the output queue and all subsequent packets destined to
// the output queue are discarded.
//
// @param port
// @param qid : Queue Id.
// @param table_size : Size of the ACL table.
//-------------------------------------------------------------------------------------------------
control EgressPFCWd(in switch_port_t port,
               in switch_qid_t qid,
               out bool flag)(
               switch_uint32_t table_size=512) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) stats;

    @name(".egress_pfcwd.acl_deny")
    action acl_deny() {
        flag = true;
        stats.count();
    }

    @ways(2)
    @name(".egress_pfcwd.acl")
    table acl {
        key = {
            qid : exact;
            port : exact;
        }

        actions = {
            @defaultonly NoAction;
            acl_deny;
        }

        const default_action = NoAction;
        counters = stats;
        size = table_size;
    }

    apply {
#ifdef PFC_ENABLE
        acl.apply();
#endif /* PFC_ENABLE */
    }
}

//-------------------------------------------------------------------------------------------------
// Ingress QosMap
// QoS Classification - map dscp/cos/exp -> tc, color
//-------------------------------------------------------------------------------------------------
control IngressQoSMap(inout switch_header_t hdr,
                      inout switch_local_metadata_t local_md)(
        switch_uint32_t dscp_map_size=4096,
        switch_uint32_t pcp_map_size=512) {

    @name(".ingress_qos_map.set_ingress_tc")
    action set_ingress_tc(switch_tc_t tc) {
        local_md.qos.tc = tc;
    }

    @name(".ingress_qos_map.set_ingress_color")
    action set_ingress_color(switch_pkt_color_t color) {
        local_md.qos.color = color;
    }

    @name(".ingress_qos_map.set_ingress_tc_and_color")
    action set_ingress_tc_and_color(
            switch_tc_t tc, switch_pkt_color_t color) {
        set_ingress_tc(tc);
        set_ingress_color(color);
    }

    @name(".ingress_qos_map.dscp_tc_map")
    table dscp_tc_map {
        key = {
            local_md.ingress_port : exact;
            local_md.lkp.ip_tos[7:2] : exact;
        }

        actions = {
            NoAction;
            set_ingress_tc;
            set_ingress_color;
            set_ingress_tc_and_color;
        }

        const default_action = NoAction;
        size = dscp_map_size;
    }

    @name(".ingress_qos_map.pcp_tc_map")
    table pcp_tc_map {
        key = {
            local_md.ingress_port : exact;
            local_md.lkp.pcp : exact;
        }

        actions = {
            NoAction;
            set_ingress_tc;
            set_ingress_color;
            set_ingress_tc_and_color;
        }

        const default_action = NoAction;
        size = pcp_map_size;
    }

#ifdef MPLS_ENABLE
    @name(".ingress_qos_map.exp_tc_map")
    table exp_tc_map {
        key = {
            local_md.ingress_port : exact;
            hdr.mpls[0].exp : exact;
        }

        actions = {
            NoAction;
            set_ingress_tc;
            set_ingress_color;
            set_ingress_tc_and_color;
        }

        size = pcp_map_size;
    }
#endif

    apply {
#ifdef MPLS_ENABLE
        if (!INGRESS_BYPASS(QOS) && hdr.mpls[0].isValid()) {
            exp_tc_map.apply();
         } else
#endif /* MPLS_ENABLE */
        /*
        If IP packet:
            - Lookup DSCP-TC map if attached to the port
            - If result is no action
                - Lookup PCP-TC map
        else if non-IP packet:
            - Lookup PCP-TC map
        */
        if (!INGRESS_BYPASS(QOS)) {
            if (local_md.lkp.ip_type != SWITCH_IP_TYPE_NONE) {
                switch(dscp_tc_map.apply().action_run) {
                    NoAction : { pcp_tc_map.apply(); }
                }
            } else {
                pcp_tc_map.apply();
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
// Ingress QosTC
// QoS Classification - map Traffic Class -> icos, qid
//-------------------------------------------------------------------------------------------------
control IngressTC(inout switch_local_metadata_t local_md)() {

    const bit<32> tc_table_size = 1024;

    @name(".ingress_tc.set_icos")
    action set_icos(switch_cos_t icos) {
        local_md.qos.icos = icos;
    }

    @name(".ingress_tc.set_queue")
    action set_queue(switch_qid_t qid) {
        local_md.qos.qid = qid;
    }

    @name(".ingress_tc.set_icos_and_queue")
    action set_icos_and_queue(switch_cos_t icos, switch_qid_t qid) {
        set_icos(icos);
        set_queue(qid);
    }

    @name(".ingress_tc.traffic_class")
    table traffic_class {
        key = {
            local_md.ingress_port : ternary @name("port");
            local_md.qos.color : ternary @name("color");
            local_md.qos.tc : exact @name("tc");
        }

        actions = {
            set_icos;
            set_queue;
            set_icos_and_queue;
        }

        size = tc_table_size;
    }

    apply {
#ifdef QOS_ENABLE
        if (!INGRESS_BYPASS(QOS)) {
            traffic_class.apply();
        }
#endif /* QOS_ENABLE */
    }
}

//-------------------------------------------------------------------------------------------------
// Ingress per PPG Packet and Byte Stats
//-------------------------------------------------------------------------------------------------
control PPGStats(inout switch_local_metadata_t local_md)() {

    const bit<32> ppg_table_size = 1024;
    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS_AND_BYTES) ppg_stats;
    @name(".ppg_stats.count")
    action count() {
        ppg_stats.count();
    }

    // Asymmetric table to maintain statistics per local port and cos pair.
    @ways(2)
    @name(".ppg_stats.ppg")
    table ppg {
        key = {
            local_md.ingress_port : exact @name("port");
            local_md.qos.icos : exact @name("icos");
        }

        actions = {
            @defaultonly NoAction;
            count;
        }

        const default_action = NoAction;
        size = ppg_table_size;
        counters = ppg_stats;
    }

    apply {
        ppg.apply();
    }
}

//-------------------------------------------------------------------------------------------------
// Egress QoS Marking
// {TC, Color} -> DSCP/PCP
//-------------------------------------------------------------------------------------------------
control EgressQoS(inout switch_header_t hdr,
                  in switch_port_t port,
                  inout switch_local_metadata_t local_md)(
                  switch_uint32_t table_size=1024) {
    // Overwrites 6-bit dscp only.
    @name(".egress_qos.set_ipv4_dscp")
    action set_ipv4_dscp(bit<6> dscp, bit<3> exp) {
        hdr.ipv4.diffserv[7:2] = dscp;
        local_md.tunnel.mpls_encap_exp = exp;
    }

    // Overwrites 6-bit dscp only.
    @name(".egress_qos.set_ipv6_dscp")
    action set_ipv6_dscp(bit<6> dscp, bit<3> exp) {
#ifdef IPV6_ENABLE
        hdr.ipv6.traffic_class[7:2] = dscp;
        local_md.tunnel.mpls_encap_exp = exp;
#endif
    }

    @name(".egress_qos.set_vlan_pcp")
    action set_vlan_pcp(bit<3> pcp, bit<3> exp) {
        local_md.qos.pcp = pcp;
        local_md.tunnel.mpls_encap_exp = exp;
    }

    // This is asymmetric table.
    // To support 32 TC, 3 Colors and 64 ports, table requires 12K entries.
    // If we need to support more TCs, need to increase the table size.
    @name(".egress_qos.l3_qos_map")
    table l3_qos_map {
        key = {
            port : exact @name("port");
            local_md.qos.tc : exact @name("tc");
            local_md.qos.color : exact @name("color");
            hdr.ipv4.isValid() : exact;
            hdr.ipv6.isValid() : exact;
        }

        actions = {
            NoAction;
            set_ipv4_dscp;
            set_ipv6_dscp;
        }

        const default_action = NoAction;
        size = 12288;
    }

    @name(".egress_qos.l2_qos_map")
    table l2_qos_map {
        key = {
            port : exact @name("port");
            local_md.qos.tc : exact @name("tc");
            local_md.qos.color : exact @name("color");
        }

        actions = {
            NoAction;
            set_vlan_pcp;
        }

        const default_action = NoAction;
        size = 6144;
    }

    apply {
#ifdef QOS_ENABLE
        if (!local_md.flags.bypass_egress) {
            l2_qos_map.apply();
            l3_qos_map.apply();
        }
#endif /* QOS_ENABLE */
    }
}

//-------------------------------------------------------------------------------------------------
// Per Queue Stats
//-------------------------------------------------------------------------------------------------
control EgressQueue(in switch_port_t port,
                    inout switch_local_metadata_t local_md)(
                    switch_uint32_t queue_table_size=1024) {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) queue_stats;

    @name(".egress_queue.count")
    action count() {
        queue_stats.count();
    }

    // Asymmetric table to maintain statistics per local port and queue pair. This table does NOT
    // take care of packets that get dropped or sent to cpu by system acl.
#ifdef INIT_BANNED_ON_EGRESS_QOS_QUEUE
    @no_field_initialization
#endif
    @ways(2)
    @pack(2)
    @name(".egress_queue.queue")
    table queue {
        key = {
            port : exact;
            local_md.qos.qid : exact @name("qid");
        }

        actions = {
            @defaultonly NoAction;
            count;
        }

        size = queue_table_size;
        const default_action = NoAction;
        counters = queue_stats;
    }

    apply {
        queue.apply();
    }
}

#endif /* _P4_QOS_ */
