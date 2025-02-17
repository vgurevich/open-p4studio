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

#include "tofino/intrinsic_metadata.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

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

header_type meta_t {
     fields {
         pad_0 : 4;
         vrf : 12;
     }
}


header ethernet_t ethernet;
header ipv4_t ipv4;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return ingress;
}

field_list ipv4_field_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags;
    ipv4.fragOffset;
    ipv4.ttl;
    ipv4.protocol;
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation ipv4_chksum_calc {
    input {
        ipv4_field_list;
    }
    algorithm : csum16;
    output_width: 16;
}

calculated_field ipv4.hdrChecksum {
    update ipv4_chksum_calc;
}

action hop(ttl, egress_port) {
    add_to_field(ttl, -1);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action ipv4_lpm_hit(egress_port) {
    hop(ipv4.ttl, egress_port);
}

action ipv4_lpm_hit_change_dmac(egress_port, dstmac) {
    hop(ipv4.ttl, egress_port);
    modify_field(ethernet.dstAddr, dstmac);
}

action lpm_miss(){
    drop();
}

action nop() {}

@pragma alpm 1
table ipv4_alpm {
    reads {
        meta.vrf: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        ipv4_lpm_hit_change_dmac;
        lpm_miss;
        nop;
    }
    size: 8192;
}

@pragma alpm 1
@pragma alpm_partitions 1024
@pragma alpm_subtrees_per_partition 1
table ipv4_alpm_small {
    reads {
        meta.vrf: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        ipv4_lpm_hit_change_dmac;
        lpm_miss;
        nop;
    }
    size: 4096;
}

@pragma alpm 1
@pragma alpm_partitions 1024
@pragma alpm_subtrees_per_partition 4
table ipv4_alpm_large {
    reads {
        meta.vrf: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        lpm_miss;
        nop;
    }
    size: 200000;
}

@pragma alpm 1
table ipv4_alpm_idle {
    reads {
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        nop;
    }
    size: 15000;
    support_timeout: true;
}

@pragma alpm 1
@pragma alpm_atcam_exclude_field_msbs ipv4.protocol 8
@pragma alpm_atcam_exclude_field_msbs ipv4.dstAddr 7
table ipv4_alpm_excluded {
    reads {
        ipv4.protocol: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        lpm_miss;
        nop;
    }
    size: 100000;
}

/* Main control flow */
control ingress {
    apply(ipv4_alpm);
    apply(ipv4_alpm_small);
    apply(ipv4_alpm_large);
    apply(ipv4_alpm_idle);
    apply(ipv4_alpm_excluded);
}
