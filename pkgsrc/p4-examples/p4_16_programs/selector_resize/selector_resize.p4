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

#include <tna.p4>
#include "common/headers.p4"
#include "common/util.p4"

parser SwitchIngressParser(
        packet_in pkt,
        out header_t hdr,
        out empty_metadata_t ig_md,
        out ingress_intrinsic_metadata_t ig_intr_md) {

    TofinoIngressParser() tofino_parser;

    state start {
        tofino_parser.apply(pkt, ig_intr_md);
        transition parse_ethernet;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);
        transition select (hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4 : parse_ipv4;
            ETHERTYPE_IPV6 : parse_ipv6;
            default : reject;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }

    state parse_ipv6 {
        pkt.extract(hdr.ipv6);
        transition accept;
    }
}

// ----------------------------------------------------------------------------
// Nexthop/ECMP resolution
// ----------------------------------------------------------------------------
control Nexthop(inout header_t hdr,
                inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)(
                bit<32> table_size,
                bit<32> act_prof_size,
                bit<32> sel_grp_size,
                bit<32> sel_grp_num,
                bool is_ternary) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) sel_hash;
    ActionProfile(act_prof_size) act_profile;
    ActionSelector(act_profile, sel_hash, SelectorMode_t.FAIR, sel_grp_size, sel_grp_num) ecmp_selector;

    action set_eg_port(PortId_t dst_port) {
        ig_tm_md.ucast_egress_port = dst_port;
    }

    @name ("ecmp")
    table ecmp_e {
        key = {
            hdr.ethernet.dst_addr : exact;
            hdr.ipv4.src_addr : selector;
            hdr.ipv4.dst_addr : selector;
        }

        actions = {
            NoAction;
            set_eg_port;
        }

        default_action = NoAction;
        size = table_size;
        implementation = ecmp_selector;
    }

    @name ("ecmp")
    table ecmp_t {
        key = {
            hdr.ethernet.dst_addr : ternary;
            hdr.ipv4.src_addr : selector;
            hdr.ipv4.dst_addr : selector;
        }

        actions = {
            NoAction;
            set_eg_port;
        }

        default_action = NoAction;
        size = table_size;
        implementation = ecmp_selector;
    }
    apply {
      if (is_ternary) {
        ecmp_t.apply();
      } else {
        ecmp_e.apply();
      }
    }
}

// Smaller copy of previous Nexthop with shared selector.
control NexthopShared(inout header_t hdr,
                inout ingress_intrinsic_metadata_for_tm_t ig_tm_md)(
                bit<32> table_size,
                bit<32> act_prof_size,
                bit<32> sel_grp_size,
                bit<32> sel_grp_num) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) sel_hash;
    ActionProfile(act_prof_size) act_profile;
    ActionSelector(act_profile, sel_hash, SelectorMode_t.FAIR, sel_grp_size, sel_grp_num) ecmp_selector;

    action set_eg_port(PortId_t dst_port) {
        ig_tm_md.ucast_egress_port = dst_port;
    }

    table ecmp_e {
        key = {
            hdr.ethernet.dst_addr : exact;
            hdr.ipv4.src_addr : selector;
            hdr.ipv4.dst_addr : selector;
        }

        actions = {
            NoAction;
            set_eg_port;
        }

        default_action = NoAction;
        size = table_size;
        implementation = ecmp_selector;
    }

    table ecmp_t {
        key = {
            hdr.ethernet.dst_addr : ternary;
            hdr.ipv4.src_addr : selector;
            hdr.ipv4.dst_addr : selector;
        }

        actions = {
            NoAction;
            set_eg_port;
        }

        default_action = NoAction;
        size = table_size;
        implementation = ecmp_selector;
    }
    apply {
      if (hdr.ipv4.ttl == 1) {
        ecmp_e.apply();
      } else {
        ecmp_t.apply();
      }
    }
}


control SwitchIngress(
        inout header_t hdr,
        inout empty_metadata_t ig_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_tm_md) {

    // Table size, num actions, max sel grp size, num selectors
    Nexthop(1024, 56000, 1024, 1024, true) nexthop_t;
    Nexthop(1024, 56000, 1024, 1024, false) nexthop_e;
    NexthopShared(1024, 120, 120, 1) nexthop_shared;

    apply {
      nexthop_e.apply(hdr, ig_tm_md);
      nexthop_t.apply(hdr, ig_tm_md);
      nexthop_shared.apply(hdr, ig_tm_md);
      ig_tm_md.bypass_egress = 1w1;
    }
}

control SwitchIngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in empty_metadata_t ig_md,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md) {
    apply { pkt.emit(hdr); }
}

Pipeline(SwitchIngressParser(),
         SwitchIngress(),
         SwitchIngressDeparser(),
         EmptyEgressParser(),
         EmptyEgress(),
         EmptyEgressDeparser()) pipe;

Switch(pipe) main;
