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



// ----------------------------------------------------------------------------
// Nexthop/ECMP resolution
//
// @param local_md : Ingress metadata fields
// @param nexthop_table_size : Number of nexthops.
// @param ecmp_group_table_size : Number of ECMP groups.
// @param ecmp_selction_table_size : Maximum number of ECMP members.
//
// ----------------------------------------------------------------------------
control Nexthop(inout switch_local_metadata_t local_md)(
                switch_uint32_t nexthop_table_size,
                switch_uint32_t ecmp_group_table_size,
                switch_uint32_t ecmp_selection_table_size,
                switch_uint32_t ecmp_max_members_per_group=64) {

    Hash<switch_uint16_t>(HashAlgorithm_t.IDENTITY) selector_hash;
    @name(".nexthop_ecmp_action_profile")
    ActionProfile(ecmp_selection_table_size) ecmp_action_profile;
#ifdef RESILIENT_ECMP_HASH_ENABLE
    @name(".nexthop_ecmp_selector")
    ActionSelector(ecmp_action_profile,
                   selector_hash,
                   SelectorMode_t.RESILIENT,
                   ecmp_max_members_per_group,
                   ecmp_group_table_size) ecmp_selector;
#else
    @name(".nexthop_ecmp_selector")
    ActionSelector(ecmp_action_profile,
                   selector_hash,
                   SelectorMode_t.FAIR,
                   ecmp_max_members_per_group,
                   ecmp_group_table_size) ecmp_selector;
#endif

    // ---------------- IP Nexthop ----------------
    @name(".nexthop_set_nexthop_properties")
    action set_nexthop_properties(switch_port_lag_index_t port_lag_index,
#ifdef L3_UNICAST_SELF_FORWARDING_CHECK
                                  switch_bd_t bd,
#endif
                                  switch_nat_zone_t zone) {
#ifdef NAT_ENABLE
        local_md.checks.same_zone_check = local_md.nat.ingress_zone ^ zone;
#endif
        local_md.egress_port_lag_index = port_lag_index;
#ifdef L3_UNICAST_SELF_FORWARDING_CHECK
        local_md.checks.same_bd = local_md.bd ^ bd;
        local_md.checks.same_if = local_md.ingress_port_lag_index ^ port_lag_index;
#endif
#ifdef TUNNEL_ENCAP_ENABLE
        // Flattned tunnel + immediate IP nexthop for MPLS, SAL etc.
        local_md.tunnel_nexthop = local_md.nexthop;
#endif /* TUNNEL_ENCAP_ENABLE */
    }

    @name(".set_ecmp_properties")
    action set_ecmp_properties(switch_port_lag_index_t port_lag_index,
#ifdef L3_UNICAST_SELF_FORWARDING_CHECK
                               switch_bd_t bd,
#endif
                               switch_nexthop_t nexthop_index, switch_nat_zone_t zone) {
        local_md.nexthop = nexthop_index;
#ifdef TUNNEL_ENCAP_ENABLE
        local_md.tunnel_nexthop = local_md.nexthop;
#endif /* TUNNEL_ENCAP_ENABLE */
#ifdef L3_UNICAST_SELF_FORWARDING_CHECK
        set_nexthop_properties(port_lag_index, bd, zone);
#else
        set_nexthop_properties(port_lag_index, zone);
#endif
    }

    // ----------------  Post Route Flood ----------------
    @name(".set_nexthop_properties_post_routed_flood")
    action set_nexthop_properties_post_routed_flood(switch_bd_t bd, switch_mgid_t mgid, switch_nat_zone_t zone) {
        local_md.egress_port_lag_index = 0;
        local_md.multicast.id = mgid;
#ifdef NAT_ENABLE
        local_md.checks.same_zone_check = local_md.nat.ingress_zone ^ zone;
#endif
    }

    // ---------------- Glean ----------------
    @name(".set_nexthop_properties_glean")
    action set_nexthop_properties_glean(switch_hostif_trap_t trap_id) {
        local_md.flags.glean = true;
#ifndef NEXTHOP_GLEAN_TRAP_DISABLE
        local_md.hostif_trap_id = trap_id;
#endif
    }

    @name(".set_ecmp_properties_glean")
    action set_ecmp_properties_glean(switch_hostif_trap_t trap_id) {
        set_nexthop_properties_glean(trap_id);
    }

    // ---------------- Drop ----------------
    @name(".set_nexthop_properties_drop")
    action set_nexthop_properties_drop(switch_drop_reason_t drop_reason) {
        local_md.drop_reason = drop_reason;
    }

    @name(".set_ecmp_properties_drop")
    action set_ecmp_properties_drop() {
        set_nexthop_properties_drop(SWITCH_DROP_REASON_NEXTHOP);
    }

#ifdef INDEPENDENT_TUNNEL_NEXTHOP_ENABLE
    //  ---------------- Tunnel Encap ----------------
    @name(".set_nexthop_properties_tunnel")
    action set_nexthop_properties_tunnel(switch_tunnel_ip_index_t dip_index) {
        // TODO : Disable cut-through for non-ip packets.
        local_md.tunnel.dip_index = dip_index;
        local_md.egress_port_lag_index = 0;
        local_md.tunnel_nexthop = local_md.nexthop;
    }

    @name(".set_ecmp_properties_tunnel")
    action set_ecmp_properties_tunnel(switch_tunnel_ip_index_t dip_index, switch_nexthop_t nexthop_index) {
        local_md.tunnel.dip_index = dip_index;
        local_md.egress_port_lag_index = 0;
        local_md.tunnel_nexthop = nexthop_index;
    }
#endif /* INDEPENDENT_TUNNEL_NEXTHOP_ENABLE */

    @ways(2)
    @name(".ecmp_table")
    table ecmp {
        key = {
            local_md.nexthop : exact;
            local_md.ecmp_hash : selector;
        }

        actions = {
            @defaultonly NoAction;
            set_ecmp_properties;
            set_ecmp_properties_drop;
            set_ecmp_properties_glean;
#ifdef INDEPENDENT_TUNNEL_NEXTHOP_ENABLE
            set_ecmp_properties_tunnel;
#endif /* INDEPENDENT_TUNNEL_NEXTHOP_ENABLE */
        }

        const default_action = NoAction;
        size = ecmp_group_table_size;
        implementation = ecmp_selector;
    }

#ifdef INIT_BANNED_ON_NEXTHOP
    @no_field_initialization
#endif
    @name(".nexthop")
    table nexthop {
        key = {
            local_md.nexthop : exact;
        }

        actions = {
            @defaultonly NoAction;
            set_nexthop_properties;
            set_nexthop_properties_drop;
            set_nexthop_properties_glean;
            set_nexthop_properties_post_routed_flood;
#ifdef INDEPENDENT_TUNNEL_NEXTHOP_ENABLE
            set_nexthop_properties_tunnel;
#endif /* INDEPENDENT_TUNNEL_NEXTHOP_ENABLE */
        }

        const default_action = NoAction;
        size = nexthop_table_size;
    }

    apply {
#ifdef ACL_REDIRECT_PORT_ENABLE
      if (local_md.acl_port_redirect == true) {
          local_md.flags.routed = false;
          local_md.nexthop = 0;
      }
      else {
#endif
#if __TARGET_TOFINO__ != 1
#if defined(ACL_REDIRECT_NEXTHOP_ENABLE) && !defined(ACL2_ENABLE)
        if (local_md.acl_nexthop != 0) {
            local_md.flags.fib_lpm_miss = false;
            local_md.nexthop = local_md.acl_nexthop;
        }
#endif /* ACL_REDIRECT_NEXTHOP_ENABLE && !ACL2_ENABLE */
#endif /* __TARGET_TOFINO__ != 1 */
        switch(nexthop.apply().action_run) {
            NoAction : { ecmp.apply(); }
            default : {}
            }
#ifdef ACL_REDIRECT_PORT_ENABLE
      }
#endif
    }
}

#ifdef INDEPENDENT_TUNNEL_NEXTHOP_ENABLE
//--------------------------------------------------------------------------
// Route lookup and ECMP resolution for Tunnel Destination IP
//-------------------------------------------------------------------------
control OuterFib(inout switch_local_metadata_t local_md)(
                 switch_uint32_t ecmp_max_members_per_group=64) {
    Hash<switch_uint16_t>(HashAlgorithm_t.IDENTITY) selector_hash;
    @name(".outer_fib_ecmp_action_profile")
    ActionProfile(ECMP_SELECT_TABLE_SIZE) ecmp_action_profile;
    @name(".outer_fib_ecmp_selector")
    ActionSelector(ecmp_action_profile,
                   selector_hash,
                   SelectorMode_t.FAIR,
                   ecmp_max_members_per_group,
                   ECMP_GROUP_TABLE_SIZE) ecmp_selector;

    @name(".outer_fib_set_nexthop_properties")
    action set_nexthop_properties(switch_port_lag_index_t port_lag_index,
                                  switch_nexthop_t nexthop_index) {
        local_md.nexthop = nexthop_index;
        local_md.egress_port_lag_index = port_lag_index;
    }

    @name(".outer_fib")
    table fib {
        key = {
            local_md.tunnel.dip_index : exact;
            local_md.outer_ecmp_hash : selector;
        }

        actions = {
            NoAction;
            set_nexthop_properties;
        }

        const default_action = NoAction;
        implementation = ecmp_selector;
        size = 1 << switch_tunnel_ip_index_width;
    }

    apply {
        fib.apply();
    }
}
#endif /* INDEPENDENT_TUNNEL_NEXTHOP_ENABLE */

//--------------------------------------------------------------------------
// Egress Pipeline: Neighbor lookup for both routed and tunnel encap cases
//-------------------------------------------------------------------------

control Neighbor(inout switch_header_t hdr,
                inout switch_local_metadata_t local_md)() {

    @name(".neighbor_rewrite_l2")
    action rewrite_l2(mac_addr_t dmac) {
        hdr.ethernet.dst_addr = dmac;
    }

#ifndef FOLDED_SWITCH_PIPELINE
    @use_hash_action(1)
#endif
    @name (".neighbor")
    table neighbor {
        key = { local_md.nexthop : exact; } // Programming_note : Program if nexthop_type == IP
        actions = {
            rewrite_l2;
        }

        const default_action = rewrite_l2(0);
        size = NEXTHOP_TABLE_SIZE;
    }

    apply {
        // Should not rewrite packets redirected to CPU.
        if (!local_md.flags.bypass_egress && local_md.flags.routed && local_md.nexthop != 0) {
            neighbor.apply();
        }
    }
}

//--------------------------------------------------------------------------
// Egress Pipeline: Outer Nexthop lookup for both routed and tunnel encap cases
//-------------------------------------------------------------------------

control OuterNexthop(inout switch_header_t hdr,
                inout switch_local_metadata_t local_md)() {

    @name(".outer_nexthop_rewrite_l2")
    action rewrite_l2(switch_bd_t bd) {
        local_md.bd = bd;
    }

#ifndef FOLDED_SWITCH_PIPELINE
    @use_hash_action(1)
#endif
    @name(".outer_nexthop")
    table outer_nexthop {
        key = { local_md.nexthop : exact; } // Programming_note : Program if nexthop_type == IP or MPLS
        actions = {
            rewrite_l2;
        }

        const default_action = rewrite_l2(0);
        size = NEXTHOP_TABLE_SIZE;
    }

    apply {
        if (!local_md.flags.bypass_egress && local_md.flags.routed && local_md.nexthop != 0) {
            outer_nexthop.apply();
        }
    }
}

