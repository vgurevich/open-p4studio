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

#include <core.p4>
#include <v1model.p4>

header Hdr {
    bit<32> fA;
    bit<32> fB;
}

struct Headers { Hdr hdr; }

struct Meta {
    bit<32> color;
}

parser p(packet_in b, out Headers h,
         inout Meta m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.hdr);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    direct_meter<bit<32> >(MeterType.bytes) mtr_0;
    meter(1024, MeterType.bytes) mtr_1;
    direct_counter(CounterType.packets_and_bytes) ctr_0;

    action t_mtr_0_send(bit<9> port) {
        sm.egress_spec = port;
        mtr_0.read(m.color);
    }

    action t_mtr_0_default() {
        mtr_0.read(m.color);
    }

    table t_mtr_0 {
        key = { sm.ingress_port: exact; }
        meters = mtr_0;
        actions = { @tableonly t_mtr_0_send; @defaultonly t_mtr_0_default; }
        default_action = t_mtr_0_default();
    }

    // one version uses the same action parameter for the egress port and the
    // indirect meter index, one version uses 2 different action parameters and
    // the third version uses a constant value for the meter index. This ensures
    // proper testing of the pipe action spec decoding logic.

    action t_mtr_1_send(bit<9> port) {
        sm.egress_spec = port;
        mtr_1.execute_meter((bit<32>)port, m.color);
    }

    action t_mtr_1_send_with_index(bit<9> port, bit<32> index) {
        sm.egress_spec = port;
        mtr_1.execute_meter(index, m.color);
    }

    action t_mtr_1_send_constant_index(bit<9> port) {
        sm.egress_spec = port;
        mtr_1.execute_meter(10, m.color);
    }

    table t_mtr_1 {
        key = { sm.ingress_port: exact; }
        actions = {
            @tableonly t_mtr_1_send;
            @tableonly t_mtr_1_send_with_index;
            @tableonly t_mtr_1_send_constant_index;
        }
    }

    action send(bit<9> port) {
        sm.egress_spec = port;
    }

    action_selector(HashAlgorithm.crc16, 32w128, 32w16) action_sel;

    table t_indirect {
        key = {
            sm.ingress_port: exact;
            h.hdr.fA: selector;
        }
        actions = { send; }
        implementation = action_sel;
    }

    action t_ctr_0_send(bit<9> port) {
        sm.egress_spec = port;
        ctr_0.count();
    }

    action t_ctr_0_default() {
        ctr_0.count();
    }

    // we make this table ternary on purpose: when resetting the default entry
    // of a table with a direct counter, the tcam table mgr will assert if no
    // stats were attached to the entry when it was set. We define this table
    // specifically to test that case and ensure PI prevents the assert from
    // happening.
    table t_ctr_0 {
        key = { sm.ingress_port: ternary; }
        counters = ctr_0;
        actions = { @tableonly t_ctr_0_send; @defaultonly t_ctr_0_default; }
        default_action = t_ctr_0_default();
    }

    apply {
        t_mtr_0.apply();
        t_mtr_1.apply();
        t_indirect.apply();
        t_ctr_0.apply();
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply { }
}

control deparser(packet_out b, in Headers h) {
    apply { b.emit(h); }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
