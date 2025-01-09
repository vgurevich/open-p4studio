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

#if __TARGET_TOFINO__ == 3
#include <tofino3/intrinsic_metadata.p4>
#elif __TARGET_TOFINO__ == 2
#include <tofino2/intrinsic_metadata.p4>
#else
#include <tofino/intrinsic_metadata.p4>
#endif
#include "tofino/stateful_alu_blackbox.p4"

action n() {}
action n1(x) {modify_field(ig_intr_md_for_tm.mcast_grp_a, x);}
action n2(x) {modify_field(ig_intr_md_for_tm.mcast_grp_b, x);}
action N(x) {modify_field(ig_intr_md_for_tm.rid, x);}

table p0 {
  reads {
    ig_intr_md.ingress_port : exact;
  }
  actions {N;}
  default_action: N(0);
  size: 288;
}
// Control plane operations are tested with this table.
@pragma dont_trim
@pragma stage 2 1024
table exm {
  reads {
    ig_intr_md.ingress_port : exact;
    ig_intr_md_for_tm.rid   : exact;
  }
  actions {n;n1;n2;}
  default_action: n1(0x1234);
  size: 8192;
}
// Control plane operations are tested with this table.
@pragma dont_trim
table tcam {
  reads {
    ig_intr_md.ingress_port : ternary;
  }
  actions {n;n1;n2;}
  default_action: n1(0x1234);
  size : 1024;
}

@pragma atcam_partition_index ethernet.etherType
table atcam {
  reads {
    ethernet.etherType : exact;
    ig_intr_md.ingress_port : ternary;
  }
  actions {n;n1;n2;}
  default_action: n1(0xABCD);
  size : 10240;
}

@pragma use_hash_action 1
table ha {
  reads {
    ig_intr_md.ingress_port : exact;
  }
  actions {n;}
  default_action : n();
#if __TARGET_TOFINO__ == 3
  size: 2048;
#else
  size: 512;
#endif
}
counter ha_cntr {
  type: packets;
  direct: ha;
}

@pragma alpm 1
table alpm {
  reads {
    ig_intr_md.ingress_port : lpm;
  }
  actions {n;}
  size: 1024;
}

table range {
  reads {
    ig_intr_md.ingress_port : range;
  }
  actions {n;n1;n2;}
  default_action: n1(0xFACE);
  size: 1024;
}

/*
 * Shared Counter
 */
counter cntr {
    type: packets_and_bytes;
    instance_count: 1000;
    min_width : 32;
}

/*
 * Shared Selection Table w/ Counter
 */
action a(x) {
  modify_field(ig_intr_md_for_tm.ucast_egress_port, x);
}
action b(x,i) {
  modify_field(ig_intr_md_for_tm.ucast_egress_port, x);
  count(cntr, i);
}
action c(x) {
  a(x);
  count(cntr, 1);
}
action d(x,y,i) {
  modify_field(ipv6.srcAddr, x);
  modify_field(ipv6.flowLabel, y);
  count(cntr, i);
}
action e() {
  count(cntr, 0);
}
action_profile sel_ap {
  actions { a;b;c;d;e; }
  dynamic_action_selection : sel_as;
  size: 10240;
}
action_selector sel_as {
  selection_key : sel_as_hash;
}
field_list_calculation sel_as_hash {
    input { sel_as_hash_fields; }
    algorithm : crc32;
    output_width : 29;
}
field_list sel_as_hash_fields {
    ethernet.dstAddr;
    ethernet.srcAddr;
}

@pragma selector_max_group_size 120
table exm_sel {
  reads {
    ipv6.valid : exact;
    ipv6.srcAddr : exact;
    ipv6.dstAddr : exact;
  }
  action_profile : sel_ap;
  size : 2048;
}
@pragma selector_max_group_size 120
table tcam_sel {
  reads {
    ipv6.valid : exact;
    ipv6.srcAddr : exact;
    ipv6.dstAddr : lpm;
  }
  action_profile : sel_ap;
  size : 2048;
}

/*
 * Shared Indirect Action w/ Counter
 */
action_profile ap {
  actions { a;b;c;d;e; }
}
table exm_ap {
  reads {
    ipv6.valid : exact;
    ipv6.srcAddr : exact;
    ipv6.dstAddr : exact;
  }
  action_profile : ap;
  size : 2048;
}
table tcam_ap {
  reads {
    ipv6.valid : exact;
    ipv6.srcAddr : exact;
    ipv6.dstAddr : lpm;
  }
  action_profile : ap;
  size : 2048;
}

/*
 * Keyless table with direct register.
 */
register r0 {
  width : 32;
  direct : r;
}
blackbox stateful_alu r0_alu {
  reg: r0;
  update_lo_1_value: register_lo + 1;
}
table r {
  actions {r0_inc; r0_inc_duplicate;}
  default_action: r0_inc;
  size: 1;
}
action r0_inc() {
  r0_alu.execute_stateful_alu();
}
action r0_inc_duplicate() {
  r0_alu.execute_stateful_alu();
}


/*
 * Keyless tables with action params.
 */
table e_with_key{
  reads { ethernet.dstAddr : exact; }
  actions { set_dmac; }
  size : 10;
}
table t_with_key{
  reads { ethernet.srcAddr : ternary; }
  actions { set_smac; }
  size : 10;
}
table t_no_key{
  actions { set_smac; }
  size : 1;
  default_action: set_smac;
}
action set_dmac(dmac) {
  modify_field(ethernet.dstAddr, dmac);
}
action set_smac(s) {
  modify_field(ethernet.srcAddr, s);
}

/*
 * Table with no key to run indirect resource from PHV
 */
field_list bf_hash_fields {
    ipv6.srcAddr;
    ipv6.dstAddr;
}
field_list_calculation bf_hash {
    input { bf_hash_fields; }
    algorithm: random;
    output_width: 14;
}
register bloom_filter {
    width : 1;
    instance_count : 16384;
}
blackbox stateful_alu bf_alu {
    reg: bloom_filter;
    update_lo_1_value: set_bitc;
    output_value: alu_lo;
    output_dst: md.bf_tmp;
}
action run_bf() {
  bf_alu.execute_stateful_alu_from_hash(bf_hash);
}

table bf {
  actions { run_bf; }
  size : 1;
  default_action: run_bf;
}

@pragma idletime_precision 1
@pragma ways 4
@pragma pack 9
@pragma stage 11
table wide_exm {
  reads {
    ethernet.etherType : exact;
    ethernet.srcAddr   : exact;
    ethernet.dstAddr   : exact;
  }
  actions {
    exm_a1; exm_a2; exm_a3;
  }
  size : 36864;
  support_timeout: true;
}
action exm_a1(smac, etype) {
  modify_field(ethernet.srcAddr, smac);
  modify_field(ethernet.etherType, etype);
  modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}
action exm_a2(dmac, etype) {
  modify_field(ethernet.dstAddr, dmac);
  modify_field(ethernet.etherType, etype);
  modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}
action exm_a3() {
  modify_field(ethernet.dstAddr, 0);
  modify_field(ethernet.srcAddr, 0);
  modify_field(ig_intr_md_for_tm.ucast_egress_port, ig_intr_md.ingress_port);
}

header_type ethernet_t {
  fields {
    dstAddr : 48;
    srcAddr : 48;
    etherType : 16;
  }
}
header_type ipv6_t {
  fields {
    version : 4;
    trafficClass : 8;
    flowLabel : 20;
    payloadLen : 16;
    nextHdr : 8;
    hopLimit : 8;
    srcAddr : 128;
    dstAddr : 128;
  }
}
header_type user_metadata_t {
  fields {
    bf_tmp : 1;
  }
}
metadata user_metadata_t md;
header ethernet_t ethernet;
header ipv6_t ipv6;
parser start {
  extract(ethernet);
  extract(ipv6);
  return ingress;
}
control ingress {
  if (0 == ig_intr_md.resubmit_flag) {
    apply(p0);
  }
  if (ethernet.etherType == 0xabcd) {
    apply(exm);
    apply(tcam);
    apply(atcam);
    apply(ha);
    apply(alpm);
    apply(range);
    if (ig_intr_md.ingress_port == 0) {
      apply(exm_sel);
    } else if (ig_intr_md.ingress_port == 1) {
      apply(tcam_sel);
    } else if (ig_intr_md.ingress_port == 2) {
      apply(exm_ap);
    } else if (ig_intr_md.ingress_port == 3) {
      apply(tcam_ap);
    }
    apply(r);
    apply(e_with_key);
    apply(t_with_key);
    apply(t_no_key);
    apply(bf);
  }
  apply(wide_exm);
}
