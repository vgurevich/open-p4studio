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



#ifndef _P4_MULTICAST_
#define _P4_MULTICAST_

#ifdef MULTICAST_ENABLE
//-----------------------------------------------------------------------------
// IP Multicast
// @param src_addr : IP source address.
// @param grp_addr : IP group address.
// @param bd : Bridge domain.
// @param group_id : Multicast group id.
// @param s_g_table_size : (s, g) table size.
// @param star_g_table_size : (*, g) table size.
//-----------------------------------------------------------------------------
control MulticastBridge<T>(
        in ipv4_addr_t src_addr,
        in ipv4_addr_t grp_addr,
        in switch_bd_t bd,
        out switch_mgid_t group_id,
        out bit<1> multicast_hit)(
        switch_uint32_t s_g_table_size,
        switch_uint32_t star_g_table_size) {
    @name(".multicast_bridge_s_g_hit")
    action s_g_hit(switch_mgid_t mgid) {
        group_id = mgid;
        multicast_hit = 1;
    }

    @name(".multicast_bridge_star_g_hit")
    action star_g_hit(switch_mgid_t mgid) {
        group_id = mgid;
        multicast_hit = 1;
    }

    action star_g_miss() {
        multicast_hit = 0;
    }

    @name(".multicast_bridge_ipv4_s_g")
    table s_g {
        key =  {
            bd : exact;
            src_addr : exact;
            grp_addr : exact;
        }

        actions = {
            NoAction;
            s_g_hit;
        }

        const default_action = NoAction;
        size = s_g_table_size;
    }

    @ways(2)
    @name(".multicast_bridge_ipv4_star_g")
    table star_g {
        key = {
            bd : exact;
            grp_addr : exact;
        }

        actions = {
            star_g_miss;
            star_g_hit;
        }

        const default_action = star_g_miss;
        size = star_g_table_size;
    }

    apply {
        switch(s_g.apply().action_run) {
            NoAction : { star_g.apply(); }
        }
    }
}

control MulticastBridgev6<T>(
        in ipv6_addr_t src_addr,
        in ipv6_addr_t grp_addr,
        in switch_bd_t bd,
        out switch_mgid_t group_id,
        out bit<1> multicast_hit)(
        switch_uint32_t s_g_table_size,
        switch_uint32_t star_g_table_size) {
    @name(".multicast_bridge_ipv6_s_g_hit")
    action s_g_hit(switch_mgid_t mgid) {
        group_id = mgid;
        multicast_hit = 1;
    }

    @name(".multicast_bridge_ipv6_star_g_hit")
    action star_g_hit(switch_mgid_t mgid) {
        group_id = mgid;
        multicast_hit = 1;
    }

    action star_g_miss() {
        multicast_hit = 0;
    }

    @name(".multicast_bridge_ipv6_s_g")
    table s_g {
        key =  {
            bd : exact;
            src_addr : exact;
            grp_addr : exact;
        }

        actions = {
            NoAction;
            s_g_hit;
        }

        const default_action = NoAction;
        size = s_g_table_size;
    }

    @ways(2)
    @name(".multicast_bridge_ipv6_star_g")
    table star_g {
        key = {
            bd : exact;
            grp_addr : exact;
        }

        actions = {
            star_g_miss;
            star_g_hit;
        }

        const default_action = star_g_miss;
        size = star_g_table_size;
    }

    apply {
        switch(s_g.apply().action_run) {
            NoAction : { star_g.apply(); }
        }
    }
}

control MulticastRoute<T>(
        in ipv4_addr_t src_addr,
        in ipv4_addr_t grp_addr,
        in switch_vrf_t vrf,
        inout switch_multicast_metadata_t multicast_md,
        out switch_multicast_rpf_group_t rpf_check,
        out switch_mgid_t multicast_group_id,
        out bit<1> multicast_hit)(
        switch_uint32_t s_g_table_size,
        switch_uint32_t star_g_table_size) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) s_g_stats;
    //DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) star_g_stats;

    @name(".multicast_route_s_g_hit")
    action s_g_hit(
            switch_mgid_t mgid, switch_multicast_rpf_group_t  rpf_group) {
        multicast_group_id = mgid;
        multicast_hit = 1;
        rpf_check = rpf_group ^ multicast_md.rpf_group;
        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_SM;
        s_g_stats.count();
    }

//    @name(".multicast_route_star_g_hit_bidir")
//    action star_g_hit_bidir(
//            switch_mgid_t mgid, switch_multicast_rpf_group_t rpf_group) {
//        multicast_group_id = mgid;
//        multicast_hit = 1;
//        // rpf check passes if rpf_check != 0
//        rpf_check = rpf_group & multicast_md.rpf_group;
//        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_BIDIR;
//        //star_g_stats.count();
//    }

    @name(".multicast_route_star_g_hit_sm")
    action star_g_hit_sm(
            switch_mgid_t mgid, switch_multicast_rpf_group_t rpf_group) {
        multicast_group_id = mgid;
        multicast_hit = 1;
        // rpf check passes if rpf_check == 0
        rpf_check = rpf_group ^ multicast_md.rpf_group;
        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_SM;
        //star_g_stats.count();
    }

    // Source and Group address pair (S, G) lookup
    @name(".multicast_route_ipv4_s_g")
    table s_g {
        key =  {
            vrf : exact;
            src_addr : exact;
            grp_addr : exact;
        }

        actions = {
            @defaultonly NoAction;
            s_g_hit;
        }

        const default_action = NoAction;
        size = s_g_table_size;
        counters = s_g_stats;
    }

    // Group address (*, G) lookup
    @ways(2)
    @name(".multicast_route_ipv4_star_g")
    table star_g {
        key = {
            vrf : exact;
            grp_addr : exact;
        }

        actions = {
            @defaultonly NoAction;
            star_g_hit_sm;
//            star_g_hit_bidir;
        }

        const default_action = NoAction;
        size = star_g_table_size;
        //counters = star_g_stats;
    }

    apply {
        if (!s_g.apply().hit) {
            star_g.apply();
        }
    }
}


control MulticastRoutev6<T>(
        in ipv6_addr_t src_addr,
        in ipv6_addr_t grp_addr,
        in switch_vrf_t vrf,
        inout switch_multicast_metadata_t multicast_md,
        out switch_multicast_rpf_group_t rpf_check,
        out switch_mgid_t multicast_group_id,
        out bit<1> multicast_hit)(
        switch_uint32_t s_g_table_size,
        switch_uint32_t star_g_table_size) {

    DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) s_g_stats;
    //DirectCounter<bit<switch_counter_width>>(CounterType_t.PACKETS) star_g_stats;

    @name(".multicast_route_ipv6_s_g_hit")
    action s_g_hit(
            switch_mgid_t mgid, switch_multicast_rpf_group_t  rpf_group) {
        multicast_group_id = mgid;
        multicast_hit = 1;
        rpf_check = rpf_group ^ multicast_md.rpf_group;
        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_SM;
        s_g_stats.count();
    }

//    @name(".multicast_route_ipv6_star_g_hit_bidir")
//    action star_g_hit_bidir(
//            switch_mgid_t mgid, switch_multicast_rpf_group_t rpf_group) {
//        multicast_group_id = mgid;
//        multicast_hit = 1;
//        // rpf check passes if rpf_check != 0
//        rpf_check = rpf_group & multicast_md.rpf_group;
//        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_BIDIR;
//        //star_g_stats.count();
//    }
//
    @name(".multicast_route_ipv6_star_g_hit_sm")
    action star_g_hit_sm(
            switch_mgid_t mgid, switch_multicast_rpf_group_t rpf_group) {
        multicast_group_id = mgid;
        multicast_hit = 1;
        // rpf check passes if rpf_check == 0
        rpf_check = rpf_group ^ multicast_md.rpf_group;
        multicast_md.mode = SWITCH_MULTICAST_MODE_PIM_SM;
        //star_g_stats.count();
    }

    // Source and Group address pair (S, G) lookup
    @name(".multicast_route_ipv6_s_g")
    table s_g {
        key =  {
            vrf : exact;
            src_addr : exact;
            grp_addr : exact;
        }

        actions = {
            @defaultonly NoAction;
            s_g_hit;
        }

        const default_action = NoAction;
        size = s_g_table_size;
        counters = s_g_stats;
    }

    // Group address (*, G) lookup
    @ways(2)
    @name(".multicast_route_ipv6_star_g")
    table star_g {
        key = {
            vrf : exact;
            grp_addr : exact;
        }

        actions = {
            @defaultonly NoAction;
            star_g_hit_sm;
//            star_g_hit_bidir;
        }

        const default_action = NoAction;
        size = star_g_table_size;
        //counters = star_g_stats;
    }

    apply {
        if (!s_g.apply().hit) {
            star_g.apply();
        }
    }
}

control IngressMulticast(
        inout switch_header_t hdr,
        in switch_lookup_fields_t lkp,
        inout switch_local_metadata_t local_md)(
        switch_uint32_t ipv4_s_g_table_size,
        switch_uint32_t ipv4_star_g_table_size,
        switch_uint32_t ipv6_s_g_table_size,
        switch_uint32_t ipv6_star_g_table_size) {

    // For each rendezvous point (RP), there is a list of interfaces for which
    // the switch is the designated forwarder (DF).

    MulticastBridge<ipv4_addr_t>(ipv4_s_g_table_size, ipv4_star_g_table_size) ipv4_multicast_bridge;
    MulticastRoute<ipv4_addr_t>(ipv4_s_g_table_size, ipv4_star_g_table_size) ipv4_multicast_route;
    MulticastBridgev6<ipv6_addr_t>(
        ipv6_s_g_table_size, ipv6_star_g_table_size) ipv6_multicast_bridge;
    MulticastRoutev6<ipv6_addr_t>(ipv6_s_g_table_size, ipv6_star_g_table_size) ipv6_multicast_route;

    switch_multicast_rpf_group_t rpf_check;

    @name(".set_multicast_route")
    action set_multicast_route() {
#ifdef FOLDED_SWITCH_PIPELINE
	hdr.fp_bridged_md.fwd_type = SWITCH_FWD_TYPE_MC;
	hdr.fp_bridged_md.fwd_idx = (switch_fwd_idx_t)local_md.multicast.id;
#else
        local_md.egress_port_lag_index = 0;
#endif
        local_md.checks.mrpf = true;
        local_md.flags.routed = true;
    }

    @name(".set_multicast_bridge")
    action set_multicast_bridge(bool mrpf) {
#ifdef FOLDED_SWITCH_PIPELINE
	hdr.fp_bridged_md.fwd_type = SWITCH_FWD_TYPE_MC;
	hdr.fp_bridged_md.fwd_idx = (switch_fwd_idx_t)local_md.multicast.id;
#else
        local_md.egress_port_lag_index = 0;
#endif
        local_md.flags.routed = false;
        local_md.checks.mrpf = mrpf;
    }

    @name(".set_multicast_flood")
    action set_multicast_flood(bool mrpf, bool flood) {
#ifdef FOLDED_SWITCH_PIPELINE
	hdr.fp_bridged_md.fwd_type = SWITCH_FWD_TYPE_L2;
	hdr.fp_bridged_md.fwd_idx = (switch_fwd_idx_t)SWITCH_FLOOD;
#else
        local_md.egress_port_lag_index = SWITCH_FLOOD;
#endif
        local_md.checks.mrpf = mrpf;
        local_md.flags.routed = false;
        local_md.flags.flood_to_multicast_routers = flood;
    }

    @name(".multicast_fwd_result")
    table fwd_result {
        key = {
            local_md.multicast.hit : ternary;
            lkp.ip_type : ternary;
            local_md.ipv4.multicast_snooping : ternary;
            local_md.ipv6.multicast_snooping : ternary;
            local_md.multicast.mode : ternary;
            rpf_check : ternary;
        }

        actions = {
            set_multicast_bridge;
            set_multicast_route;
            set_multicast_flood;
        }
	size = 512;
    }

    apply {
        if (lkp.ip_type == SWITCH_IP_TYPE_IPV4 && local_md.ipv4.multicast_enable) {
            ipv4_multicast_route.apply(hdr.ipv4.src_addr,
                                       hdr.ipv4.dst_addr,
                                       local_md.vrf,
                                       local_md.multicast,
                                       rpf_check,
                                       local_md.multicast.id,
                                       local_md.multicast.hit);
        } else if (lkp.ip_type == SWITCH_IP_TYPE_IPV6 && local_md.ipv6.multicast_enable) {
#ifdef IPV6_ENABLE
            ipv6_multicast_route.apply(hdr.ipv6.src_addr,
                                       hdr.ipv6.dst_addr,
                                       local_md.vrf,
                                       local_md.multicast,
                                       rpf_check,
                                       local_md.multicast.id,
                                       local_md.multicast.hit);
#endif /* IPV6_ENABLE */
        }

        if (local_md.multicast.hit == 0 ||
//            (local_md.multicast.mode == SWITCH_MULTICAST_MODE_PIM_BIDIR && rpf_check == 0) ||
            (local_md.multicast.mode == SWITCH_MULTICAST_MODE_PIM_SM && rpf_check != 0)) {

            if (lkp.ip_type == SWITCH_IP_TYPE_IPV4) {
                ipv4_multicast_bridge.apply(hdr.ipv4.src_addr,
                                            hdr.ipv4.dst_addr,
                                            local_md.bd,
                                            local_md.multicast.id,
                                            local_md.multicast.hit);
            } else if (lkp.ip_type == SWITCH_IP_TYPE_IPV6) {
#ifdef IPV6_ENABLE
                ipv6_multicast_bridge.apply(hdr.ipv6.src_addr,
                                            hdr.ipv6.dst_addr,
                                            local_md.bd,
                                            local_md.multicast.id,
                                            local_md.multicast.hit);
#endif /* IPV6_ENABLE */
            }
        }

        fwd_result.apply();
    }
}
#endif /* MULTICAST_ENABLE */


//-----------------------------------------------------------------------------
// Multicast flooding
//-----------------------------------------------------------------------------
control MulticastFlooding(inout switch_local_metadata_t local_md)(switch_uint32_t table_size) {

    @name(".mcast_flood")
    action flood(switch_mgid_t mgid) {
        local_md.multicast.id = mgid;
    }

    @name(".bd_flood")
    table bd_flood {
        key = {
            local_md.bd : exact @name("bd");
            local_md.lkp.pkt_type : exact @name("pkt_type");
#ifdef MULTICAST_ENABLE
            local_md.flags.flood_to_multicast_routers : exact @name("flood_to_multicast_routers");
#endif
        }

        actions = { flood; }
        size = table_size;
    }

    apply {
        bd_flood.apply();
    }
}

//-----------------------------------------------------------------------------
// Egress/Tunnel Multicast Replication DB
//-----------------------------------------------------------------------------
#if 1
control Replication(in egress_intrinsic_metadata_t eg_intr_md,
                             inout switch_local_metadata_t local_md)(
                             switch_uint32_t table_size=4096) {

    // L3 Multicast with local ports only
    @name(".mc_rid_hit")
    action rid_hit_mc(switch_bd_t bd) {
        local_md.checks.same_bd = bd ^ local_md.bd;
        local_md.bd = bd;
    }

#ifdef L2_VXLAN_ENABLE
    // Tunnel Replication for L2MC/Flood
    @name(".tunnel_rid_hit")
    action rid_hit_tunnel(switch_nexthop_t nexthop, switch_nexthop_t tunnel_nexthop) {
        local_md.nexthop = nexthop;
        local_md.tunnel_nexthop = tunnel_nexthop;
    }

#ifdef MULTICAST_ENABLE
    // Tunnel Replication for L3MC
    @name(".tunnel_mc_rid_hit")
    action rid_hit_tunnel_mc(switch_bd_t bd, switch_nexthop_t nexthop, switch_nexthop_t tunnel_nexthop) {
        local_md.checks.same_bd = bd ^ local_md.bd;
        local_md.bd = bd;
        local_md.nexthop = nexthop;
        local_md.tunnel_nexthop = tunnel_nexthop;
    }
#endif /* MULTICAST_ENABLE */
#endif /* L2_VXLAN_ENABLE */
    // L2MC/Flood-with-local-ports-only - No need to program this table
    action rid_miss() {
        local_md.flags.routed = false;
    }

    @name(".rid")
    table rid {
        key = { eg_intr_md.egress_rid : exact; }
        actions = {
            rid_miss;
            rid_hit_mc;
#ifdef L2_VXLAN_ENABLE
            rid_hit_tunnel;
#ifdef MULTICAST_ENABLE
            rid_hit_tunnel_mc;
#endif /* MULTICAST_ENABLE */
#endif /* L2_VXLAN_ENABLE */
        }

        size = table_size;
        const default_action = rid_miss;
    }

    apply {
        if (eg_intr_md.egress_rid != 0) {
            rid.apply();
#ifdef MULTICAST_ENABLE
            if (local_md.checks.same_bd == 0)
                local_md.flags.routed = false;
#endif /* MULTICAST_ENABLE */
        }
    }
}
#endif

#if 0
control Replication(in egress_intrinsic_metadata_t eg_intr_md,
                             inout switch_local_metadata_t local_md)(
                             switch_uint32_t table_size=4096) {
    @name(".multicast_rid_hit")
    action rid_hit(
        switch_bd_t bd, bool bd_vld,
        switch_nexthop_t nexthop, switch_nexthop_t tunnel_nexthop, bool tunnel_vld) {
        // UC OR L2MC/Flood-with-local-ports-only - No need to program this table
        // L3 Multicast with local ports only - bd_vld=true and tunnel_vld=false
        // L2/L3 Multicast with tunnel and local ports -
        //     bd_vld - needs to be set for both local and remote ports(even for L2MC case)
        //     tunnel_vld - set for remote ports only
        if (bd_vld) {
            local_md.bd = bd;
        }
        if (tunnel_vld) {
            local_md.nexthop = nexthop;
            local_md.tunnel_nexthop = tunnel_nexthop;
        }
        // copy ingress bd to be used later for same_bd check
        local_md.ingress_bd = local_md.bd;
    }

    @name(".rid") @use_hash_action(1)
    table rid {
        key = { eg_intr_md.egress_rid[11:0] : exact; }
        actions = {
            rid_hit;
        }

        size = table_size;
        const default_action = rid_hit(0, false, 0, 0, false);
    }

    apply {
        if (eg_intr_md.egress_rid != 0)
            rid.apply();

          if (local_md.bd == local_md.ingress_bd)
              local_md.flags.routed = false;
    }
}
#endif

#endif /* _P4_MULTICAST_ */
