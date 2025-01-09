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


// ============================================================================
//
// Outer Packet validation
// Validate ethernet, Ipv4 or Ipv6 headers and set the common lookup fields.
// Check for malformed packets and set pkt_type
//
//===============================================================================

control PktValidation(
        in switch_header_t hdr,
        inout switch_local_metadata_t local_md) {
    const switch_uint32_t table_size = MIN_TABLE_SIZE;

    //-----------------------------------------------------------------------------
    // Validate outer L2/L3 header
    // - Drop the packet if src addr is zero or multicast or dst addr is zero.
    // Check for malformed packets and set pkt_type
    //-----------------------------------------------------------------------------
    action valid_ethernet_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.pkt_type = pkt_type;
        local_md.lkp.mac_src_addr = hdr.ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
    }

    @name(".fp_malformed_eth_pkt")
    action malformed_eth_pkt(bit<8> reason) {
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
        local_md.lkp.mac_src_addr = hdr.ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
        local_md.l2_drop_reason = reason;
    }

    @name(".fp_valid_pkt_untagged")
    action valid_pkt_untagged(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
        valid_ethernet_pkt(pkt_type);
    }

    @name(".fp_valid_pkt_tagged")
    action valid_pkt_tagged(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_type = hdr.vlan_tag[0].ether_type;
        local_md.lkp.pcp = hdr.vlan_tag[0].pcp;
        local_md.lkp.dei = hdr.vlan_tag[0].dei;
        valid_ethernet_pkt(pkt_type);
    }

    @name(".fp_valid_pkt_double_tagged")
    action valid_pkt_double_tagged(switch_pkt_type_t pkt_type) {
        local_md.lkp.mac_type = hdr.vlan_tag[1].ether_type;
        local_md.lkp.pcp = hdr.vlan_tag[1].pcp;
        local_md.lkp.dei = hdr.vlan_tag[1].dei;
        valid_ethernet_pkt(pkt_type);
    }

    @name(".fp_validate_ethernet")
    table validate_ethernet {
        key = {
            hdr.ethernet.src_addr : ternary;
            hdr.ethernet.dst_addr : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
        }

        actions = {
            malformed_eth_pkt;
            valid_pkt_untagged;
            valid_pkt_tagged;
        }

        size = table_size;
    }

    //-----------------------------------------------------------------------------
    // Validate outer L3 header
    // - Drop the packet if src addr is zero or multicast or dst addr is zero.
    //-----------------------------------------------------------------------------

    @name(".fp_valid_arp_pkt")
    action valid_arp_pkt() {
#ifdef PARSE_FULL_ARP_HEADER
// TODO : match on arp.target_proto_addr directly in ACL
//        local_md.lkp.ip_dst_addr[63:0]   = 64w0;
//        local_md.lkp.ip_dst_addr[95:64]  = hdr.arp.target_proto_addr;
//        local_md.lkp.ip_dst_addr[127:96] = 32w0;
#endif
        local_md.lkp.arp_opcode = hdr.arp.opcode;
    }

    // IP Packets
    @name(".fp_valid_ipv6_pkt")
    action valid_ipv6_pkt(bool is_link_local) {
        // Set common lookup fields
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.ipv6.traffic_class;
        local_md.lkp.ip_src_addr = hdr.ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.ipv6.dst_addr;
        local_md.flags.link_local = is_link_local;
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
    }

    @name(".fp_valid_ipv4_pkt")
    action valid_ipv4_pkt(switch_ip_frag_t ip_frag, bool is_link_local) {
        // Set common lookup fields
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.ipv4.diffserv;
        local_md.lkp.ip_src_addr[95:64]  = hdr.ipv4.src_addr;
        local_md.lkp.ip_dst_addr[95:64]  = hdr.ipv4.dst_addr;
        local_md.lkp.ip_frag = ip_frag;
        local_md.flags.link_local = is_link_local;
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
    }

    @name(".fp_malformed_ipv4_pkt")
    action malformed_ipv4_pkt(bit<8> reason, switch_ip_frag_t ip_frag) {
        // Set common lookup fields just for dtel acl and hash purposes
        valid_ipv4_pkt(ip_frag, false);
        local_md.drop_reason = reason;
    }

    @name(".fp_malformed_ipv6_pkt")
    action malformed_ipv6_pkt(bit<8> reason) {
        // Set common lookup fields just for dtel acl and hash purposes
        valid_ipv6_pkt(false);
        local_md.drop_reason = reason;
    }

#ifdef MPLS_ENABLE
    // MPLS Packets
    action set_mpls_pkt() {
        local_md.lkp.ip_type = SWITCH_IP_TYPE_MPLS;
        local_md.lkp.mpls_pkt = true;
    }

    @name(".fp_valid_mpls_pkt")
    action valid_mpls_pkt() {
        local_md.lkp.mpls_lookup_label = hdr.mpls[0].label;
        set_mpls_pkt();
    }

    @name(".fp_valid_mpls_null_pkt")
    action valid_mpls_null_pkt() {
        local_md.lkp.mpls_lookup_label = hdr.mpls[1].label;
        set_mpls_pkt();
    }

    // TODO: Fix parser to branch into parsing outer IP headers for null+IP case
    @name(".fp_valid_mpls_null_ipv4_pkt")
    action valid_mpls_null_ipv4_pkt(switch_ip_frag_t ip_frag) {
        local_md.lkp.mpls_lookup_label = hdr.mpls[0].label;
        set_mpls_pkt();
    }

    @name(".fp_valid_mpls_null_ipv6_pkt")
    action valid_mpls_null_ipv6_pkt(bool is_link_local) {
        local_md.lkp.mpls_lookup_label = hdr.mpls[0].label;
        set_mpls_pkt();
    }

    @name(".fp_valid_mpls_router_alert_label")
    action valid_mpls_router_alert_label() {
        local_md.lkp.mpls_router_alert_label = 1;
    }
#endif

    @name(".fp_validate_ip")
    table validate_ip {
        key = {
            hdr.arp.isValid() : ternary;
            hdr.ipv4.isValid() : ternary;
            local_md.flags.ipv4_checksum_err : ternary;
            hdr.ipv4.version : ternary;
            hdr.ipv4.ihl : ternary;
            hdr.ipv4.flags : ternary;
            hdr.ipv4.frag_offset : ternary;
            hdr.ipv4.ttl : ternary;
            hdr.ipv4.src_addr[31:0] : ternary;

            hdr.ipv6.isValid() : ternary;
            hdr.ipv6.version : ternary;
            hdr.ipv6.hop_limit : ternary;
            hdr.ipv6.src_addr[127:0] : ternary;
#ifdef MPLS_ENABLE
            hdr.mpls[0].isValid() : ternary;
            hdr.mpls[0].label : ternary;
            hdr.mpls[1].isValid() : ternary; //To determine MPLS NULL + another label
            hdr.inner_ipv4.isValid() : ternary; // To derermine MPLS NULL + inner IPV4
            hdr.inner_ipv6.isValid() : ternary; // To derermine MPLS NULL + inner IPV6
#endif
        }

        actions = {
            malformed_ipv4_pkt;
            malformed_ipv6_pkt;
            valid_arp_pkt;
            valid_ipv4_pkt;
            valid_ipv6_pkt;
#ifdef MPLS_ENABLE
            valid_mpls_pkt;
            valid_mpls_null_pkt;
            valid_mpls_null_ipv4_pkt;
            valid_mpls_null_ipv6_pkt;
            valid_mpls_router_alert_label;
#endif
        }

        size = table_size;
    }

    apply {
        validate_ethernet.apply();
        validate_ip.apply();
    }
}

// ============================================================================
// Same MAC Check
// Checks if source mac address matches with destination mac address
// ============================================================================
control SameMacCheck(in switch_header_t hdr, inout switch_local_metadata_t local_md) {

    apply {
        if (!local_md.tunnel.terminate) {
            if (hdr.ethernet.src_addr[31:0] == hdr.ethernet.dst_addr[31:0]) {
                if (hdr.ethernet.src_addr[47:32] == hdr.ethernet.dst_addr[47:32]) {
                    local_md.drop_reason = SWITCH_DROP_REASON_OUTER_SAME_MAC_CHECK;
                }
            }
        }
    }
}

// ============================================================================
// Inner packet validation
// Validate ethernet, Ipv4 or Ipv6 common lookup fields.
// NOTE:
// For IPinIP packets, the actions are valid_ipv*. This would set the L2
// lookup fields to 0. The RMAC table is setup to ignore the dmac to process
// these packets
// ============================================================================
control InnerPktValidation(
        in switch_header_t hdr,
        inout switch_local_metadata_t local_md) {

    @name(".valid_inner_ethernet_pkt")
    action valid_ethernet_pkt(switch_pkt_type_t pkt_type) {
        local_md.lkp.pkt_type = pkt_type;
        local_md.lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        /* Note: Inner packet with vlan tag is not supported */
        local_md.lkp.mac_type = hdr.inner_ethernet.ether_type;
    }

    @name(".valid_inner_ipv4_pkt")
    action valid_ipv4_pkt(switch_pkt_type_t pkt_type) {
        valid_ethernet_pkt(pkt_type);
        // Set the common IP lookup fields
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
#ifdef TUNNEL_QOS_MODE_ENABLE
        local_md.lkp.ip_tos = hdr.inner_ipv4.diffserv;
#endif
        local_md.lkp.ip_src_addr[63:0]   = 64w0;
        local_md.lkp.ip_src_addr[95:64]  = hdr.inner_ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0]   = 64w0;
        local_md.lkp.ip_dst_addr[95:64]  = hdr.inner_ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
	local_md.lkp.l4_src_port = local_md.lkp.inner_l4_src_port;
	local_md.lkp.l4_dst_port = local_md.lkp.inner_l4_dst_port;
	local_md.lkp.tcp_flags = local_md.lkp.inner_tcp_flags;
    }

    @name(".valid_inner_ipv6_pkt")
    action valid_ipv6_pkt(switch_pkt_type_t pkt_type) {
        valid_ethernet_pkt(pkt_type);
        // Set the common IP lookup fields
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
#ifdef TUNNEL_QOS_MODE_ENABLE
        local_md.lkp.ip_tos = hdr.inner_ipv6.traffic_class;
#endif
        local_md.lkp.ip_src_addr = hdr.inner_ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.inner_ipv6.dst_addr;
        local_md.flags.link_local = false;
    }


    @name(".valid_inner_ipv4_tcp_pkt")
    action valid_ipv4_tcp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
    }

    @name(".valid_inner_ipv4_udp_pkt")
    action valid_ipv4_udp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv4_pkt(pkt_type);
    }

    @name(".valid_inner_ipv6_tcp_pkt")
    action valid_ipv6_tcp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv6_pkt(pkt_type);
    }

    @name(".valid_inner_ipv6_udp_pkt")
    action valid_ipv6_udp_pkt(switch_pkt_type_t pkt_type) {
        valid_ipv6_pkt(pkt_type);
    }

    @name(".malformed_l2_inner_pkt")
    action malformed_l2_pkt(bit<8> reason) {
        local_md.l2_drop_reason = reason;
    }

    @name(".malformed_l3_inner_pkt")
    action malformed_l3_pkt(bit<8> reason) {
        local_md.drop_reason = reason;
    }

    @name(".validate_inner_ethernet")
    table validate_ethernet {
        key = {
            hdr.inner_ethernet.isValid() : ternary;
            hdr.inner_ethernet.dst_addr : ternary;

            hdr.inner_ipv6.isValid() : ternary;
            hdr.inner_ipv6.version : ternary;
            hdr.inner_ipv6.hop_limit : ternary;

            hdr.inner_ipv4.isValid() : ternary;
            local_md.flags.inner_ipv4_checksum_err : ternary;
            hdr.inner_ipv4.version : ternary;
            hdr.inner_ipv4.ihl : ternary;
            hdr.inner_ipv4.ttl : ternary;
        }

        actions = {
            NoAction;
            valid_ipv4_pkt;
            valid_ipv6_pkt;
            valid_ethernet_pkt;
            malformed_l2_pkt;
            malformed_l3_pkt;
        }
        size = MIN_TABLE_SIZE;
    }

    apply {
        validate_ethernet.apply();
    }
}

// ============================================================================
//
// Select between inner or outer headers for further lookup
//
// ============================================================================
control LkpFieldsI1E1(
        in switch_header_t hdr,
        inout switch_local_metadata_t local_md) {

    //---------------------------------------------------------------
    // Non-tunnel termination case : Use outer headers for lkp fields
    //---------------------------------------------------------------

    action set_ethernet_fields_from_outer() {
        local_md.lkp.mac_type = hdr.ethernet.ether_type;
	local_md.lkp.pcp = 0;
	local_md.lkp.dei = 0;
        local_md.lkp.mac_src_addr = hdr.ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
    }

    action set_qtag_ethernet_fields_from_outer() {
        local_md.lkp.mac_type = hdr.vlan_tag[0].ether_type;
	local_md.lkp.pcp = hdr.vlan_tag[0].pcp;
	local_md.lkp.dei = hdr.vlan_tag[0].dei;
        local_md.lkp.mac_src_addr = hdr.ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.ethernet.dst_addr;
    }

    action set_arp_fields_from_outer() {
	set_ethernet_fields_from_outer();
        local_md.lkp.arp_opcode = hdr.arp.opcode;
    }

    action set_non_ip_fields_from_outer() {
	set_ethernet_fields_from_outer();
    }

    action set_ipv4_fields_from_outer(switch_ip_frag_t ip_frag, bool is_link_local) {
	set_ethernet_fields_from_outer();

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.ipv4.diffserv;
        local_md.lkp.ip_ttl = hdr.ipv4.ttl;
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_src_addr[63:0]   = 64w0;
        local_md.lkp.ip_src_addr[95:64]  = hdr.ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0]   = 64w0;
        local_md.lkp.ip_dst_addr[95:64]  = hdr.ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
        local_md.lkp.ip_frag = ip_frag;
        local_md.flags.link_local = is_link_local;
    }

    action set_ipv6_fields_from_outer(bool is_link_local) {
	set_ethernet_fields_from_outer();

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.ipv6.traffic_class;
        local_md.lkp.ip_ttl = hdr.ipv6.hop_limit;
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_src_addr = hdr.ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.ipv6.dst_addr;
        local_md.lkp.ipv6_flow_label = hdr.ipv6.flow_label;
        local_md.flags.link_local = is_link_local;
    }

    action set_qtag_arp_fields_from_outer() {
	set_qtag_ethernet_fields_from_outer();
        local_md.lkp.arp_opcode = hdr.arp.opcode;
    }

    action set_qtag_non_ip_fields_from_outer() {
	set_ethernet_fields_from_outer();
    }

    action set_qtag_ipv4_fields_from_outer(switch_ip_frag_t ip_frag, bool is_link_local) {
	set_qtag_ethernet_fields_from_outer();

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.ipv4.diffserv;
        local_md.lkp.ip_ttl = hdr.ipv4.ttl;
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_src_addr[63:0]   = 64w0;
        local_md.lkp.ip_src_addr[95:64]  = hdr.ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0]   = 64w0;
        local_md.lkp.ip_dst_addr[95:64]  = hdr.ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
        local_md.lkp.ip_frag = ip_frag;
        local_md.flags.link_local = is_link_local;
    }

    action set_qtag_ipv6_fields_from_outer(bool is_link_local) {
	set_qtag_ethernet_fields_from_outer();

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.ipv6.traffic_class;
        local_md.lkp.ip_ttl = hdr.ipv6.hop_limit;
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_src_addr = hdr.ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.ipv6.dst_addr;
        local_md.lkp.ipv6_flow_label = hdr.ipv6.flow_label;
        local_md.flags.link_local = is_link_local;
    }

    table lkp_ip_fields_from_outer {
        key = {
            hdr.arp.isValid() : exact;
            hdr.vlan_tag[0].isValid() : ternary;
            hdr.ipv4.isValid() : exact;
            hdr.ipv4.flags : ternary;
            hdr.ipv4.frag_offset : ternary;
            hdr.ipv4.src_addr[31:0] : ternary;
            hdr.ipv6.isValid() : exact;
            hdr.ipv6.src_addr[127:112] : ternary;
        }

        actions = {
            NoAction;
            set_ipv4_fields_from_outer;
            set_ipv6_fields_from_outer;
            set_arp_fields_from_outer;
            set_non_ip_fields_from_outer;
            set_qtag_ipv4_fields_from_outer;
            set_qtag_ipv6_fields_from_outer;
            set_qtag_arp_fields_from_outer;
            set_qtag_non_ip_fields_from_outer;
        }

        const default_action = NoAction;
        const entries = {
	    // ------------------ untagged packet -----------------------------------
            // arp pkt
            ( true, false, false,          _,  _,                         _, false, _) : set_arp_fields_from_outer;
            // no-ip
            (false, false, false,          _,  _,                         _, false, _) : set_non_ip_fields_from_outer;
            // link_local ipv4, non-frag
            (false, false,  true, 0x0 &&& 0x1, 0, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_FRAG,  true);
            // link_local ipv4, first frag
            (false, false,  true, 0x1 &&& 0x1, 0, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_HEAD,      true);
            // link_local ipv4, more frags
            (false, false,  true,          _,  _, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_HEAD,  true);
            // ipv4 non-frag
            (false, false,  true, 0x0 &&& 0x1, 0,                         _, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_FRAG, false);
            // ipv4 first frag
            (false, false,  true, 0x1 &&& 0x1, 0,                         _, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_HEAD,     false);
            // ipv4 more frags, invalid version
            (false, false,  true,          _,  _,                         _, false, _) : set_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_HEAD, false);
            // ipv6 link-local
            (false, false, false,          _,  _,                         _,  true, 0xFE80 &&& 0xFFC0) : set_ipv6_fields_from_outer(true);
            // ipv6
            (false, false, false,          _,  _,                         _,  true, _) : set_ipv6_fields_from_outer(false);

	    // ------------------ untagged packet -----------------------------------
            // arp pkt
            ( true,  true, false,          _,  _,                         _, false, _) : set_qtag_arp_fields_from_outer;
            // no-ip
            (false,  true, false,          _,  _,                         _, false, _) : set_qtag_non_ip_fields_from_outer;
            // link_local ipv4, non-frag
            (false,  true,  true, 0x0 &&& 0x1, 0, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_FRAG,  true);
            // link_local ipv4, first frag
            (false,  true,  true, 0x1 &&& 0x1, 0, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_HEAD,      true);
            // link_local ipv4, more frags
            (false,  true,  true,          _,  _, 0xA9FE0000 &&& 0xFFFF0000, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_HEAD,  true);
            // ipv4 non-frag
            (false,  true,  true, 0x0 &&& 0x1, 0,                         _, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_FRAG, false);
            // ipv4 first frag
            (false,  true,  true, 0x1 &&& 0x1, 0,                         _, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_HEAD,     false);
            // ipv4 more frags, invalid version
            (false,  true,  true,          _,  _,                         _, false, _) : set_qtag_ipv4_fields_from_outer(SWITCH_IP_FRAG_NON_HEAD, false);
            // ipv6 link-local
            (false,  true, false,          _,  _,                         _,  true, 0xFE80 &&& 0xFFC0) : set_qtag_ipv6_fields_from_outer(true);
            // ipv6
            (false,  true, false,          _,  _,                         _,  true, _) : set_qtag_ipv6_fields_from_outer(false);
        }

        size = MIN_TABLE_SIZE;
    }


    action set_tcp_ports_from_outer() {
        local_md.lkp.l4_src_port = hdr.tcp.src_port;
        local_md.lkp.l4_dst_port = hdr.tcp.dst_port;
        local_md.lkp.tcp_flags = hdr.tcp.flags;
    }

    action set_udp_ports_from_outer() {
        local_md.lkp.l4_src_port = hdr.udp.src_port;
        local_md.lkp.l4_dst_port = hdr.udp.dst_port;
        local_md.lkp.tcp_flags = 0;
    }

    action set_icmp_ports_from_outer() {
        local_md.lkp.l4_src_port[7:0] = hdr.icmp.type;
        local_md.lkp.l4_src_port[15:8] = hdr.icmp.code;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }

    action set_igmp_ports_from_outer() {
        local_md.lkp.l4_src_port[7:0] = hdr.igmp.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }

    table lkp_other_fields_from_outer {
        key = {
            hdr.tcp.isValid() : exact;
            hdr.udp.isValid() : exact;
            hdr.icmp.isValid() : exact;
            hdr.igmp.isValid() : exact;
        }

        actions = {
            NoAction;
            set_tcp_ports_from_outer;
            set_udp_ports_from_outer;
            set_icmp_ports_from_outer;
#ifdef MULTICAST_ENABLE
            set_igmp_ports_from_outer;
#endif
        }

        const default_action = NoAction;
        const entries = {
            (true, false, false, false) : set_tcp_ports_from_outer();
            (false, true, false, false) : set_udp_ports_from_outer();
            (false, false, true, false) : set_icmp_ports_from_outer();
#ifdef MULTICAST_ENABLE
            (false, false, false, true) : set_igmp_ports_from_outer();
#endif
        }
        size = 16;
    }

    //---------------------------------------------------------------
    // Tunnel termination case : Use inner headers for lkp fields
    //---------------------------------------------------------------

    action set_non_ip_fields_from_inner() {
        local_md.lkp.mac_type = hdr.inner_ethernet.ether_type;
        local_md.lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
    }

    action set_arp_fields_from_inner() {
        local_md.lkp.mac_type = hdr.inner_ethernet.ether_type;
        local_md.lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;
        //      local_md.lkp.arp_opcode = hdr.inner_arp.opcode;
    }

    action set_ipv4_fields_from_inner() {
        local_md.lkp.mac_type = ETHERTYPE_IPV4;
        local_md.lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
        local_md.lkp.ip_tos = hdr.inner_ipv4.diffserv;
        local_md.lkp.ip_ttl = hdr.inner_ipv4.ttl;
        local_md.lkp.ip_proto = hdr.inner_ipv4.protocol;
        local_md.lkp.ip_src_addr[63:0]   = 64w0;
        local_md.lkp.ip_src_addr[95:64]  = hdr.inner_ipv4.src_addr;
        local_md.lkp.ip_src_addr[127:96] = 32w0;
        local_md.lkp.ip_dst_addr[63:0]   = 64w0;
        local_md.lkp.ip_dst_addr[95:64]  = hdr.inner_ipv4.dst_addr;
        local_md.lkp.ip_dst_addr[127:96] = 32w0;
    }

    action set_ipv6_fields_from_inner() {
        local_md.lkp.mac_type = ETHERTYPE_IPV6;
        local_md.lkp.mac_src_addr = hdr.inner_ethernet.src_addr;
        local_md.lkp.mac_dst_addr = hdr.inner_ethernet.dst_addr;

        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
        local_md.lkp.ip_tos = hdr.inner_ipv6.traffic_class;
        local_md.lkp.ip_ttl = hdr.inner_ipv6.hop_limit;
        local_md.lkp.ip_proto = hdr.inner_ipv6.next_hdr;
        local_md.lkp.ip_src_addr = hdr.inner_ipv6.src_addr;
        local_md.lkp.ip_dst_addr = hdr.inner_ipv6.dst_addr;
        local_md.lkp.ipv6_flow_label = hdr.inner_ipv6.flow_label;
        local_md.flags.link_local = false;
    }

    action set_tcp_ports_from_inner() {
        local_md.lkp.l4_src_port = hdr.inner_tcp.src_port;
        local_md.lkp.l4_dst_port = hdr.inner_tcp.dst_port;
        local_md.lkp.tcp_flags = hdr.inner_tcp.flags;
    }

    action set_udp_ports_from_inner() {
        local_md.lkp.l4_src_port = hdr.inner_udp.src_port;
        local_md.lkp.l4_dst_port = hdr.inner_udp.dst_port;
        local_md.lkp.tcp_flags = 0;
    }

    action set_icmp_ports_from_inner() {
        local_md.lkp.l4_src_port[7:0] = hdr.inner_icmp.type;
        local_md.lkp.l4_src_port[15:8] = hdr.inner_icmp.code;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }

    action set_igmp_ports_from_inner() {
        local_md.lkp.l4_src_port[7:0] = hdr.inner_igmp.type;
        local_md.lkp.l4_src_port[15:8] = 0;
        local_md.lkp.l4_dst_port = 0;
        local_md.lkp.tcp_flags = 0;
    }

    action set_ipv4_tcp_fields_from_inner() {
        set_ipv4_fields_from_inner();
        set_tcp_ports_from_inner();
    }

    action set_ipv4_udp_fields_from_inner() {
        set_ipv4_fields_from_inner();
        set_udp_ports_from_inner();
    }

    action set_ipv4_icmp_fields_from_inner() {
        set_ipv4_fields_from_inner();
        set_icmp_ports_from_inner();
    }

    action set_ipv4_igmp_fields_from_inner() {
        set_ipv4_fields_from_inner();
        set_igmp_ports_from_inner();
    }

    action set_ipv6_tcp_fields_from_inner() {
        set_ipv6_fields_from_inner();
        set_tcp_ports_from_inner();
    }

    action set_ipv6_udp_fields_from_inner() {
        set_ipv6_fields_from_inner();
        set_udp_ports_from_inner();
    }

    action set_ipv6_icmp_fields_from_inner() {
        set_ipv6_fields_from_inner();
        set_icmp_ports_from_inner();
    }

    action set_ipv6_igmp_fields_from_inner() {
        set_ipv6_fields_from_inner();
        set_igmp_ports_from_inner();
    }

    table lkp_fields_from_inner {
        key = {
            hdr.inner_ipv4.isValid() : exact;
            hdr.inner_ipv6.isValid() : exact;
            hdr.inner_tcp.isValid() : exact;
            hdr.inner_udp.isValid() : exact;
            hdr.inner_icmp.isValid() : exact;
            hdr.inner_igmp.isValid() : exact;
        }

        actions = {
            NoAction;
            set_ipv4_tcp_fields_from_inner;
            set_ipv4_udp_fields_from_inner;
            set_ipv4_icmp_fields_from_inner;
            set_ipv4_igmp_fields_from_inner;
            set_ipv4_fields_from_inner;
            set_ipv6_tcp_fields_from_inner;
            set_ipv6_udp_fields_from_inner;
            set_ipv6_icmp_fields_from_inner;
            set_ipv6_igmp_fields_from_inner;
            set_ipv6_fields_from_inner;
            set_non_ip_fields_from_inner;
        }

        const default_action = NoAction;
        const entries = {
	    ( true, false,  true, false, false, false) : set_ipv4_tcp_fields_from_inner;
	    ( true, false, false,  true, false, false) : set_ipv4_udp_fields_from_inner;
	    ( true, false, false, false,  true, false) : set_ipv4_igmp_fields_from_inner;
	    ( true, false, false, false, false,  true) : set_ipv4_icmp_fields_from_inner;
	    ( true, false, false, false, false, false) : set_ipv4_fields_from_inner;
	    (false,  true,  true, false, false, false) : set_ipv6_tcp_fields_from_inner;
	    (false,  true, false,  true, false, false) : set_ipv6_udp_fields_from_inner;
	    (false,  true, false, false,  true, false) : set_ipv6_igmp_fields_from_inner;
	    (false,  true, false, false, false,  true) : set_ipv6_icmp_fields_from_inner;
	    (false,  true, false, false, false, false) : set_ipv6_fields_from_inner;
	    (false, false, false, false, false, false) : set_non_ip_fields_from_inner;
        }

        size = MIN_TABLE_SIZE;
    }

    apply {
        if (local_md.tunnel.terminate==true) {
            lkp_fields_from_inner.apply();
        } else {
            lkp_ip_fields_from_outer.apply();
            lkp_other_fields_from_outer.apply();
        }
    }
}

// ============================================================================
//
// Extract L3/L4 header fields (step 0)
//
// ============================================================================

control LkpFieldsE0_0(
    in switch_header_t hdr,
    inout switch_local_metadata_t local_md) {

    action set_ipv4_fields() {
        local_md.lkp.ip_proto = hdr.ipv4.protocol;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV4;
    }

    action set_ipv6_fields() {
        local_md.lkp.ip_proto = hdr.ipv6.next_hdr;
        local_md.lkp.ip_type = SWITCH_IP_TYPE_IPV6;
    }

    action set_udp_ports() {
        local_md.lkp.l4_src_port = hdr.udp.src_port;
        local_md.lkp.l4_dst_port = hdr.udp.dst_port;
        local_md.lkp.tcp_flags = 0;
    }

    action set_ipv4_udp_fields() {
        set_ipv4_fields();
        set_udp_ports();
    }

    action set_ipv6_udp_fields() {
        set_ipv6_fields();
        set_udp_ports();
    }

    table lkp_fields_l3l4 {
        key = {
            hdr.ipv4.isValid() : exact;
            hdr.ipv6.isValid() : exact;
            hdr.udp.isValid() : exact;
        }
        actions = {
            set_ipv4_udp_fields;
            set_ipv4_fields;
            set_ipv6_udp_fields;
            set_ipv6_fields;
        }
        const entries = {
	    ( true, false,  true) : set_ipv4_udp_fields;
	    ( true, false, false) : set_ipv4_fields;
	    (false,  true,  true) : set_ipv6_udp_fields;
	    (false,  true, false) : set_ipv6_fields;
        }
        size = MIN_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress)
            lkp_fields_l3l4.apply();
    }
}


// ============================================================================
//
// Overlay IP addresses in the Egress pipeline for ACL match
//
// ============================================================================

control LkpFieldsE0_1(
    in switch_header_t hdr,
    inout switch_local_metadata_t local_md) {

    action valid_ipv4_pkt() {
        local_md.lkp.ip_src_addr_95_64  = hdr.ipv4.src_addr;
        local_md.lkp.ip_dst_addr_95_64  = hdr.ipv4.dst_addr;
    }

    action valid_ipv6_pkt() {
        local_md.lkp.ip_src_addr_95_64  = hdr.ipv6.src_addr[95:64];
        local_md.lkp.ip_dst_addr_95_64  = hdr.ipv6.dst_addr[95:64];
    }

    @name(".egress_pkt_validation")
    table lkp_fields_ipaddr {
        key = {
            hdr.ipv4.isValid() : ternary;
            hdr.ipv6.isValid() : ternary;
        }
        actions = {
            valid_ipv4_pkt;
            valid_ipv6_pkt;
        }
        const entries = {
            (true, false) : valid_ipv4_pkt;
            (false, true) : valid_ipv6_pkt;
        }
        size = MIN_TABLE_SIZE;
    }
    apply {
        if (!local_md.flags.bypass_egress)
            lkp_fields_ipaddr.apply();
    }
}
