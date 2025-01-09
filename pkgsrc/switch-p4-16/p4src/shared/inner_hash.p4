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



#include "types.p4"

//***************************************************************************
// Inner packet hash using hash specific metadata fields extracted in parser
//***************************************************************************

// Flow hash calculation.
control InnerIpv4Hash(in switch_local_metadata_t local_md, out switch_hash_t hash) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) ipv4_hash;
    bit<32> ip_src_addr = local_md.hash_fields.ip_src_addr[95:64];
    bit<32> ip_dst_addr = local_md.hash_fields.ip_dst_addr[95:64];
    bit<8> ip_proto = local_md.hash_fields.ip_proto;
    bit<16> l4_dst_port = local_md.hash_fields.l4_dst_port;
    bit<16> l4_src_port = local_md.hash_fields.l4_src_port;

    action hash_get() {
        hash [31:0] = ipv4_hash.get({ip_src_addr,
                                     ip_dst_addr,
                                     ip_proto,
                                     l4_dst_port,
                                     l4_src_port});
    }

    @placement_priority(-1)
    table dummy {
	actions = { hash_get; }
	default_action = hash_get;
	size = 1;
    }

    apply {
	dummy.apply();
    }
}

control InnerIpv6Hash(in switch_local_metadata_t local_md, out switch_hash_t hash) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) ipv6_hash;
    bit<128> ip_src_addr = local_md.hash_fields.ip_src_addr;
    bit<128> ip_dst_addr = local_md.hash_fields.ip_dst_addr;
    bit<8> ip_proto = local_md.hash_fields.ip_proto;
    bit<16> l4_dst_port = local_md.hash_fields.l4_dst_port;
    bit<16> l4_src_port = local_md.hash_fields.l4_src_port;
    bit<20> ipv6_flow_label = local_md.hash_fields.ipv6_flow_label;

    action hash_get() {
        hash [31:0] = ipv6_hash.get({
#ifdef IPV6_FLOW_LABEL_IN_HASH_ENABLE
                                     ipv6_flow_label,
#endif
                                     ip_src_addr,
                                     ip_dst_addr,
                                     ip_proto,
                                     l4_dst_port,
                                     l4_src_port});
    }

    @placement_priority(-1)
    table dummy {
	actions = { hash_get; }
	default_action = hash_get;
	size = 1;
    }

    apply {
	dummy.apply();
    }
}

control NonIpHash(in switch_header_t hdr, in switch_local_metadata_t local_md, out switch_hash_t hash) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) non_ip_hash;
    mac_addr_t mac_dst_addr = hdr.ethernet.dst_addr;
    mac_addr_t mac_src_addr = hdr.ethernet.src_addr;
    bit<16> mac_type = hdr.ethernet.ether_type;
    switch_port_t port = local_md.ingress_port;

    action hash_get() {
        hash [31:0] = non_ip_hash.get({port,
                                       mac_type,
                                       mac_src_addr,
                                       mac_dst_addr});
    }

    @placement_priority(-1)
    table dummy {
	actions = { hash_get; }
	default_action = hash_get;
	size = 1;
    }

    apply {
	dummy.apply();
    }
}

control InnerLagv4Hash(in switch_local_metadata_t local_md, out switch_hash_t hash) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) lag_hash;
    bit<32> ip_src_addr = local_md.hash_fields.ip_src_addr[95:64];
    bit<32> ip_dst_addr = local_md.hash_fields.ip_dst_addr[95:64];
    bit<8> ip_proto = local_md.hash_fields.ip_proto;
    bit<16> l4_dst_port = local_md.hash_fields.l4_dst_port;
    bit<16> l4_src_port = local_md.hash_fields.l4_src_port;

    action hash_get() {
        hash [31:0] = lag_hash.get({ip_src_addr,
                                     ip_dst_addr,
                                     ip_proto,
                                     l4_dst_port,
                                     l4_src_port});
    }

    @placement_priority(-1)
    table dummy {
	actions = { hash_get; }
	default_action = hash_get;
	size = 1;
    }

    apply {
	dummy.apply();
    }
}

control InnerLagv6Hash(in switch_local_metadata_t local_md, out switch_hash_t hash) {
    Hash<bit<32>>(HashAlgorithm_t.CRC32) lag_hash;
    bit<128> ip_src_addr = local_md.hash_fields.ip_src_addr;
    bit<128> ip_dst_addr = local_md.hash_fields.ip_dst_addr;
    bit<8> ip_proto = local_md.hash_fields.ip_proto;
    bit<16> l4_dst_port = local_md.hash_fields.l4_dst_port;
    bit<16> l4_src_port = local_md.hash_fields.l4_src_port;
    bit<20> ipv6_flow_label = local_md.hash_fields.ipv6_flow_label;

    action hash_get() {
        hash [31:0] = lag_hash.get({
#ifdef IPV6_FLOW_LABEL_IN_HASH_ENABLE
                                     ipv6_flow_label,
#endif
                                     ip_src_addr,
                                     ip_dst_addr,
                                     ip_proto,
                                     l4_dst_port,
                                     l4_src_port});
    }

    @placement_priority(-1)
    table dummy {
	actions = { hash_get; }
	default_action = hash_get;
	size = 1;
    }

    apply {
	dummy.apply();
    }
}
