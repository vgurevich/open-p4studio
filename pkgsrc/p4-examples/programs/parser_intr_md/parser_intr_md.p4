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

// Test program for enhancement to use ig_intr_md in the parser for branching
// Compiler-216

#if __TARGET_TOFINO__ == 3
#include <tofino3/intrinsic_metadata.p4>
#elif __TARGET_TOFINO__ == 2
#include <tofino2/intrinsic_metadata.p4>
#else
#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>
#endif

#define ETHERTYPE_IPV4         0x0800

@pragma parser_value_set_size 2
parser_value_set pvs_fabric_port;

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}
header ethernet_t ethernet;

header_type fabric_header_t {
    fields {
        packetType : 3;
        headerVersion : 2;
        packetVersion : 2;
        pad1 : 1;

        fabricColor : 3;
        fabricQos : 5;

        dstDevice : 8;
        dstPortOrGroup : 16;
    }
}
header fabric_header_t fabric_header;

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}
@pragma pa_container ingress ipv4.srcAddr 1
header ipv4_t ipv4;

parser start {
    return select(ig_intr_md.ingress_port) {
        pvs_fabric_port : parse_fabric_header;
        default : parse_ethernet;
    }
}

parser parse_fabric_header {
    extract(ethernet);
    extract(fabric_header);
    return ingress;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800: parse_ipv4;
        default: ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

// Tables
action fwd_to_fabric(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action fwd_to_server(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action fwd_drop() {
    drop();
}

table fwd_packet {
    // all ip packets received from server ports are sent to port 3
    // all fabric packets received from fabric ports are sent to server port 4
    reads {
        ipv4 : valid;
        fabric_header : valid;
    }
    actions {
        fwd_to_fabric;
        fwd_to_server;
        fwd_drop;
    }
    size : 4;
}

control ingress {
    apply(fwd_packet);
}
