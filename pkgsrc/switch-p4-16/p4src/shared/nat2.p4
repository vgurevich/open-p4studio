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



#ifndef _P4_NAT_
#define _P4_NAT_

#define INGRESS_SRC_NAT_KEY                     \
    local_md.lkp.ip_src_addr[95:64] : exact;    \

#define INGRESS_SRC_NAPT_KEY                    \
    local_md.lkp.ip_src_addr[95:64] : exact;    \
    local_md.lkp.ip_proto : exact;              \
    local_md.lkp.l4_src_port : exact;           \

#define INGRESS_DEST_NAT_KEY                    \
    local_md.lkp.ip_dst_addr[95:64] : exact;    \

#define INGRESS_DEST_NAPT_KEY                   \
    local_md.lkp.ip_dst_addr[95:64] : exact;    \
    local_md.lkp.ip_proto : exact;              \
    local_md.lkp.l4_dst_port : exact;           \


control DestNat(inout switch_local_metadata_t local_md)() {

    DirectCounter<bit<32>>(CounterType_t.PACKETS) dnat_pool_stats;
    @name(".set_dnat_pool_hit")
    action set_dnat_pool_hit() {
        local_md.nat.dnat_pool_hit = true;
        dnat_pool_stats.count();
    }

    @name(".set_dnat_pool_miss")
    action set_dnat_pool_miss() {
        local_md.nat.dnat_pool_hit = false;
        dnat_pool_stats.count();
    }

    @name(".dnat_pool")
    table dnat_pool {
        key = {
            INGRESS_DEST_NAT_KEY
        }
        actions = {
            set_dnat_pool_hit;
            set_dnat_pool_miss;
        }
        counters = dnat_pool_stats;
        const default_action = set_dnat_pool_miss;
        size = NAT_POOL_TABLE_SIZE;
    }

    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) dest_napt_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) dest_nat_stats;

    @name(".dnapt_miss")
    action dnapt_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NONE;
        dest_napt_stats.count();
    }

    @name(".dnat_miss")
    action dnat_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NONE;
        dest_nat_stats.count();
    }

    @name(".set_dnapt_rewrite")
    action set_dnapt_rewrite(ipv4_addr_t dip, bit<16> dport) {
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.lkp.l4_dst_port = dport;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NAPT;
        dest_napt_stats.count();
    }

    @name(".set_dnat_rewrite")
    action set_dnat_rewrite(ipv4_addr_t dip) {
        local_md.lkp.ip_dst_addr[95:64] = dip;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_DEST_NAT;
        dest_nat_stats.count();
    }

    @name(".dest_napt")
    table dest_napt {
        key = {
            INGRESS_DEST_NAPT_KEY
        }
        actions = {
            set_dnapt_rewrite;
            dnapt_miss;
        }
        const default_action = dnapt_miss;
        size = NAPT_TABLE_SIZE;
        counters = dest_napt_stats;
        idle_timeout = true;
    }

    @name(".dest_nat")
    table dest_nat {
        key = {
            INGRESS_DEST_NAT_KEY
        }
        actions = {
            set_dnat_rewrite;
            dnat_miss;
        }
        const default_action = dnat_miss;
        size = NAT_TABLE_SIZE;
        counters = dest_nat_stats;
        idle_timeout = true;
    }

    apply {
        if(local_md.nat.nat_disable == false) {
            if(local_md.nat.ingress_zone == SWITCH_NAT_OUTSIDE_ZONE_ID) {
                switch(dnat_pool.apply().action_run) {
                    set_dnat_pool_hit : {
                        switch(dest_napt.apply().action_run) {
                            set_dnapt_rewrite : {}
                            dnapt_miss: {dest_nat.apply();}
                        }
                    }
                }
            }
        }
    }
}

control SourceNat(inout switch_local_metadata_t local_md)() {
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) src_napt_stats;
    DirectCounter<bit<32>>(CounterType_t.PACKETS_AND_BYTES) src_nat_stats;

    @name(".set_snapt_rewrite")
    action set_snapt_rewrite(ipv4_addr_t sip, bit<16> sport) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.lkp.l4_src_port = sport;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NAPT;
        src_napt_stats.count();
    }
    @name(".set_snat_rewrite")
    action set_snat_rewrite(ipv4_addr_t sip) {
        local_md.lkp.ip_src_addr[95:64] = sip;
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NAT;
        src_nat_stats.count();
    }

    @name(".source_napt_miss")
    action source_napt_miss() {
        src_napt_stats.count();
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NONE;
    }
    @name(".source_nat_miss")
    action source_nat_miss() {
        local_md.nat.hit = SWITCH_NAT_HIT_TYPE_SRC_NONE;
        src_nat_stats.count();
    }

    @name(".source_nat")
    table source_nat {
        key = {
            INGRESS_SRC_NAT_KEY
        }
        actions = {
            set_snat_rewrite;
            source_nat_miss;
        }
        const default_action = source_nat_miss;
        size = NAT_TABLE_SIZE;
        counters = src_nat_stats;
        idle_timeout = true;
    }

    @name(".source_napt")
    table source_napt {
        key = {
            INGRESS_SRC_NAPT_KEY
        }
        actions = {
            set_snapt_rewrite;
            source_napt_miss;
        }
        const default_action = source_napt_miss;
        size = NAPT_TABLE_SIZE;
        counters = src_napt_stats;
        idle_timeout = true;
    }

    apply {
        if(local_md.nat.ingress_zone == SWITCH_NAT_INSIDE_ZONE_ID && local_md.nat.nat_disable == false) {
            switch(source_napt.apply().action_run) {
                set_snapt_rewrite: {}
                source_napt_miss: {source_nat.apply();}
            }
        }
    }

}

control IngressNatRewrite(inout switch_header_t hdr, inout switch_local_metadata_t local_md)() {
    @name(".rewrite_tcp_ipda")
    action rewrite_tcp_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.tcp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_udp_ipda")
    action rewrite_udp_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
        hdr.udp.dst_port = local_md.lkp.l4_dst_port;
    }
    @name(".rewrite_ipda")
    action rewrite_ipda() {
        hdr.ipv4.dst_addr = local_md.lkp.ip_dst_addr[95:64];
    }
    @name(".rewrite_ipsa")
    action rewrite_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
    }
    @name(".rewrite_tcp_ipsa")
    action rewrite_tcp_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.tcp.src_port = local_md.lkp.l4_src_port;
    }
    @name(".rewrite_udp_ipsa")
    action rewrite_udp_ipsa() {
        hdr.ipv4.src_addr = local_md.lkp.ip_src_addr[95:64];
        hdr.udp.src_port = local_md.lkp.l4_src_port;
    }
    @name(".ingress_nat_rewrite")
    table ingress_nat_rewrite {
        key = {
            local_md.nat.hit : exact;
            local_md.lkp.ip_proto : exact;
            local_md.checks.same_zone_check : ternary;
        }
        actions = {
            rewrite_tcp_ipda;
            rewrite_udp_ipda;
            rewrite_ipda;
            rewrite_tcp_ipsa;
            rewrite_udp_ipsa;
            rewrite_ipsa;
        }
        size = 512;
    }
    apply {
        ingress_nat_rewrite.apply();
    }
}


#endif /* _P4_NAT_ */
