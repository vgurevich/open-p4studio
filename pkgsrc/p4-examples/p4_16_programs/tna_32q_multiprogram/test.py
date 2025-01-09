################################################################################
 #  Copyright (C) 2024 Intel Corporation
 #
 #  Licensed under the Apache License, Version 2.0 (the "License");
 #  you may not use this file except in compliance with the License.
 #  You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 #  Unless required by applicable law or agreed to in writing,
 #  software distributed under the License is distributed on an "AS IS" BASIS,
 #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 #  See the License for the specific language governing permissions
 #  and limitations under the License.
 #
 #
 #  SPDX-License-Identifier: Apache-2.0
################################################################################

import logging
import random

import ptf
from ptf import config
from ptf.thriftutils import *
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest, BaseTest
import bfrt_grpc.client as gc

logger = get_logger()
swports = get_sw_ports()


num_pipes = int(testutils.test_param_get('num_pipes'))
pipes = list(range(num_pipes))

swports_by_pipe = {p:list() for p in pipes}
for port in swports:
    pipe = port_to_pipe(port)
    swports_by_pipe[pipe].append(port)


# Tofino-1 uses pipes 0 and 2 as the external pipes while 1 and 3 are
# the internal pipes.
# Tofino-2 uses pipes 0 and 1 as the external pipes while 2 and 3 are
# the internal pipes.
arch = testutils.test_param_get('arch')
if arch == "tofino":
    external_pipes = [0,2]
    internal_pipes = [1,3]
elif arch == "tofino2" or arch == "tofino3":
    external_pipes = [0,1]
    internal_pipes = [2,3]
else:
    assert (arch in ["tofino", "tofino2", "tofino3"])


def get_internal_port_from_external(ext_port):
    pipe_local_port = port_to_pipe_local_port(ext_port)
    int_pipe = internal_pipes[external_pipes.index(port_to_pipe(ext_port))]

    if arch == "tofino":
        # For Tofino-1 we are currently using a 1-to-1 mapping from external
        # port to internal port so just replace the pipe-id.
        return make_port(int_pipe, pipe_local_port)
    elif arch == "tofino2" or arch == "tofino3":
        # For Tofino-2 we are currently using internal ports in 400g mode so up
        # to eight external ports (if maximum break out is configured) can map
        # to the same internal port.
        return make_port(int_pipe, pipe_local_port & 0x1F8)
    else:
        assert (arch in ["tofino", "tofino2", "tofino3"])


def get_port_from_pipes(pipes):
    ports = list()
    for pipe in pipes:
        ports = ports + swports_by_pipe[pipe]
    return random.choice(ports)

def delete_learn_queue(test, target):
    for dg in test.interface.digest_get_iterator(timeout=3):
        pass

def verify_digests(test, target, sip, dip, smac, dmac):
    for dg in test.interface.digest_get_iterator(timeout=3):
        data_list = test.bfrt_info.learn_from_id_get(dg.digest_id).make_data_list(dg)
        data_dict = data_list[0].to_dict()
        if "dst_addr" in data_dict:
            assert data_dict["dst_addr"] == dmac
        if "src_addr" in data_dict:
            assert data_dict["src_addr"] == smac
        if "sip" in data_dict:
            assert data_dict["sip"] == sip
        if "dip" in data_dict:
            assert data_dict["dip"] == dip

def verify_cntr_inc(test, target, dip, ttl, tag, num_pkts):
    logger.info("Verifying counter got incremented on external pipe egress")
    resp = test.a_forward_e.entry_get(target,
                                      [test.a_forward_e.make_key(
                                          [gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.255.255'),
                                           gc.KeyTuple('hdr.ipv4.ttl', ttl, 255),
                                           gc.KeyTuple('hdr.custom_metadata.custom_tag', tag, 0xFFFF),
                                           gc.KeyTuple('$MATCH_PRIORITY', 0)])],
                                      {"from_hw": True},
                                      test.a_forward_e.make_data(
                                          [gc.DataTuple("$COUNTER_SPEC_BYTES"),
                                           gc.DataTuple("$COUNTER_SPEC_PKTS")],
                                          'SwitchEgress_a.hit',
                                          get=True))

    # parse resp to get the counter
    data_dict = next(resp)[0].to_dict()
    recv_pkts = data_dict["$COUNTER_SPEC_PKTS"]
    recv_bytes = data_dict["$COUNTER_SPEC_BYTES"]

    if (num_pkts != recv_pkts):
        logger.error("Error! packets sent = %s received count = %s", str(num_pkts), str(recv_pkts))
        assert 0

    # Default packet size is 100 bytes and model adds 4 bytes of CRC
    # Add 2 bytes for the custom metadata header
    pkt_size = 100 + 4 + 2
    num_bytes = num_pkts * pkt_size

    if (num_bytes != recv_bytes):
        logger.error("Error! bytes sent = %s received count = %s", str(num_bytes), str(recv_bytes))
        assert 0


def get_all_tables(test):
    # Some of these tables can be retrieved using a lesser qualified name like storm_control
    # since it is not present in any other control block of the P4 program.  Other tables
    # such as forward need more specific names to uniquely identify exactly which table is
    # being requested.
    test.a_port_md = test.a.bfrt_info.table_get("$PORT_METADATA")
    test.a_pinning = test.a.bfrt_info.table_get("SwitchIngress_a.pinning")
    test.a_storm_control = test.a.bfrt_info.table_get("storm_control")
    test.a_stats = test.a.bfrt_info.table_get("stats")
    test.a_forward_i = test.a.bfrt_info.table_get("SwitchIngress_a.forward")
    test.a_encap = test.a.bfrt_info.table_get("encap_custom_metadata_hdr")
    test.a_forward_e = test.a.bfrt_info.table_get("SwitchEgress_a.forward")
    test.a_learning = test.a.bfrt_info.table_get("SwitchIngress_a.learning")

    test.b_port_md = test.b.bfrt_info.table_get("$PORT_METADATA")
    test.b_forward_e = test.b.bfrt_info.table_get("SwitchEgress_b.forward")
    test.b_forward_i = test.b.bfrt_info.table_get("SwitchIngress_b.forward")
    test.b_pinning = test.b.bfrt_info.table_get("SwitchIngress_b.pinning")
    test.b_learning = test.b.bfrt_info.table_get("SwitchIngress_b.learning")

    # Add annotations to a few fields to specify their type.
    test.a_forward_i.info.key_field_annotation_add('hdr.ethernet.dst_addr', "mac")
    test.a_forward_e.info.key_field_annotation_add('hdr.ipv4.dst_addr', "ipv4")
    test.b_forward_e.info.key_field_annotation_add('hdr.ipv4.dst_addr', "ipv4")
    test.b_forward_i.info.key_field_annotation_add('hdr.ipv4.dst_addr', "ipv4")

def get_all_learns(test):
    test.a_learn_digest_a = test.a.bfrt_info.learn_get("SwitchIngressDeparser_a.digest_a")
    test.a_learn_digest_a.info.data_field_annotation_add("src_addr", "mac")
    test.a_learn_digest_a.info.data_field_annotation_add("dst_addr", "mac")

    test.a_learn_digest_b = test.a.bfrt_info.learn_get("SwitchIngressDeparser_a.digest_b")
    test.a_learn_digest_b.info.data_field_annotation_add("sip", "ipv4")
    test.a_learn_digest_b.info.data_field_annotation_add("dip", "ipv4")

    test.b_learn_digest_a = test.b.bfrt_info.learn_get("SwitchIngressDeparser_b.digest_a")
    test.b_learn_digest_a.info.data_field_annotation_add("src_addr", "mac")
    test.b_learn_digest_a.info.data_field_annotation_add("dst_addr", "mac")

    test.b_learn_digest_b = test.b.bfrt_info.learn_get("SwitchIngressDeparser_b.digest_b")
    test.b_learn_digest_b.info.data_field_annotation_add("sip", "ipv4")
    test.b_learn_digest_b.info.data_field_annotation_add("dip", "ipv4")


def program_learning(test, target, dg_type_a, dg_type_b):
    test.a_learning.default_entry_set(
            target,
            test.a_learning.make_data(
                [gc.DataTuple("dg_type", dg_type_a)],
                "SwitchIngress_a.dmac_miss")
            )
    test.b_learning.default_entry_set(
            target,
            test.b_learning.make_data(
                [gc.DataTuple("dg_type", dg_type_b)],
                "SwitchIngress_b.dmac_miss")
            )

def program_entries(test, target, ig_port, int_port, eg_port, port_meta_a, port_meta_b, tag, dmac, dip, ttl):
    a_f1, a_f2 = port_meta_a
    b_f1 = port_meta_b

    # Use a fixed meter index and use the default meter configuration which
    # will give a green packet (i.e. color == 0).
    meter_idx = 1
    color = 0

    logger.info("Programming table entries")

    logger.info(" Programming table entries on ingress ext-pipe")
    logger.info("    Table: Port Metadata")
    test.a_port_md.entry_add(
        target,
        [test.a_port_md.make_key(
            [gc.KeyTuple('ig_intr_md.ingress_port', ig_port)])],
        [test.a_port_md.make_data(
            [gc.DataTuple('f1', a_f1),
             gc.DataTuple('f2', a_f2)])])

    logger.info("    Table: storm_control")
    test.a_storm_control.entry_add(
        target,
        [test.a_storm_control.make_key(
            [gc.KeyTuple('ig_intr_md.ingress_port', ig_port)])],
        [test.a_storm_control.make_data(
            [gc.DataTuple('index', meter_idx)],
            'SwitchIngress_a.set_color')])

    logger.info("    Table: stats")
    test.a_stats.entry_add(
        target,
        [test.a_stats.make_key(
            [gc.KeyTuple('qos_md.color', color),
             gc.KeyTuple('ig_intr_md.ingress_port', ig_port)])],
        [test.a_stats.make_data([], "SwitchIngress_a.count")])

    logger.info("    Table: forward")
    test.a_forward_i.entry_add(
        target,
        [test.a_forward_i.make_key(
            [gc.KeyTuple('hdr.ethernet.dst_addr', dmac),
             gc.KeyTuple('hdr.ipv4.ttl', ttl)])],
        [test.a_forward_i.make_data([], 'SwitchIngress_a.hit')])
    # The action will decrement the TTL so let's also decrement it here so it is
    # ready to use as the key in the next table.
    ttl = ttl - 1

    logger.info("    Table: encap_custom_metadata_hdr")
    test.a_encap.entry_add(
        target,
        [test.a_encap.make_key(
            [gc.KeyTuple('ig_md.port_md.f1', a_f1),
             gc.KeyTuple('ig_md.port_md.f2', a_f2)])],
        [test.a_encap.make_data(
            [gc.DataTuple('tag', tag)],
            'SwitchIngress_a.encap_custom_metadata')])

    logger.info("    Table: pinning")
    test.a_pinning.entry_add(
        target,
        [test.a_pinning.make_key(
            [gc.KeyTuple('ig_intr_md.ingress_port', ig_port)])],
        [test.a_pinning.make_data(
            [gc.DataTuple('port', int_port)],
            'SwitchIngress_a.modify_eg_port')])

    logger.info(" Programming table entries on egress int-pipe")
    logger.info("    Table: forward")
    test.b_forward_e.entry_add(
        target,
        [test.b_forward_e.make_key(
            [gc.KeyTuple('hdr.ipv4.dst_addr', dip, prefix_len=31),
             gc.KeyTuple('hdr.ipv4.ttl', ttl),
             gc.KeyTuple('hdr.custom_metadata.custom_tag', tag)])],
        [test.b_forward_e.make_data([], "SwitchEgress_b.hit")])
    # The action will decrement the TTL and increment the tag so let's do the
    # same here so the variables are ready to use in the next table.
    ttl = ttl - 1
    tag = tag + 1

    logger.info(" Programming table entries on ingress int-pipe")
    logger.info("    Table: Port Metadata")
    test.b_port_md.entry_add(
        target,
        [test.b_port_md.make_key(
            [gc.KeyTuple('ig_intr_md.ingress_port', int_port)])],
        [test.b_port_md.make_data(
            [gc.DataTuple('f1', b_f1)])])

    logger.info("    Table: forward")
    test.b_forward_i.entry_add(
        target,
        [test.b_forward_i.make_key(
            [gc.KeyTuple('hdr.ipv4.dst_addr', dip),
             gc.KeyTuple('hdr.ipv4.ttl', ttl),
             gc.KeyTuple('hdr.custom_metadata.custom_tag', tag),
             gc.KeyTuple('ig_md.port_md.f1', b_f1)])],
        [test.b_forward_i.make_data([], "SwitchIngress_b.hit")])
    # The action will decrement the TTL and increment the tag so let's do the
    # same here so the variables are ready to use in the next table.
    ttl = ttl - 1
    tag = tag + 1

    logger.info("    Table: pinning")
    test.b_pinning.entry_add(
        target,
        [test.b_pinning.make_key([gc.KeyTuple('ig_intr_md.ingress_port', int_port)])],
        [test.b_pinning.make_data([gc.DataTuple('port', eg_port)],
                                  'SwitchIngress_b.modify_eg_port')])

    logger.info(" Programming table entries on egress ext-pipe")
    logger.info("    Table: forward")
    test.a_forward_e.entry_add(
        target,
        [test.a_forward_e.make_key(
            [gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.255.255'),
             gc.KeyTuple('hdr.ipv4.ttl', ttl, 0xFF),
             gc.KeyTuple('hdr.custom_metadata.custom_tag', tag, 0xFFFF),
             gc.KeyTuple('$MATCH_PRIORITY', 0)])],
        [test.a_forward_e.make_data(
            [gc.DataTuple('$COUNTER_SPEC_BYTES', 0),
             gc.DataTuple('$COUNTER_SPEC_PKTS', 0)],
            'SwitchEgress_a.hit')])


def delete_entries(test, target):
    logger.info("Deleting table entries")

    logger.info(" Deleting table entries on external pipe ingress")
    test.a_port_md.entry_del(target)
    test.a_storm_control.entry_del(target)
    test.a_stats.entry_del(target)
    test.a_forward_i.entry_del(target)
    test.a_encap.entry_del(target)
    test.a_pinning.entry_del(target)
    test.a_learning.entry_del(target)

    logger.info(" Deleting table entries on internal pipe egress")
    test.b_forward_e.entry_del(target)

    logger.info(" Deleting table entries on internal pipe ingress")
    test.b_port_md.entry_del(target)
    test.b_forward_i.entry_del(target)
    test.b_pinning.entry_del(target)
    test.b_learning.entry_del(target)

    logger.info(" Deleting table entries on external pipe egress")
    test.a_forward_e.entry_del(target)

class MultiProgramTest(BaseTest):
    class ProgramA(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_32q_multiprogram_a"):
            BfRuntimeTest.setUp(self, client_id, p4_name)
            self.bfrt_info = self.interface.bfrt_info_get(p4_name)
            setup_random()

        def runTest(self):
            logger.info("")

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

    class ProgramB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_32q_multiprogram_b"):
            BfRuntimeTest.setUp(self, client_id, p4_name)
            self.bfrt_info = self.interface.bfrt_info_get(p4_name)
            setup_random()

        def runTest(self):
            logger.info("")

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

    def setUp(self):
        # Open 2 connections to the grpc server as 2 separate clients. Client
        # 1 (self.a) is in-charge of program "tna_32q_multiprogram_a" while
        # client 2 (self.b) is in-charge of program "tna_32q_multiprogram_b"
        # Thus any operation on a table in tna_32q_multiprogram_a needs to be
        # called on self.a and any operation on a table in
        # tna_32q_multiprogram_b needs to be called on self.b
        self.a = self.ProgramA()
        self.b = self.ProgramB()
        self.a.setUp(1)
        self.b.setUp(2)

        get_all_tables(self)
        get_all_learns(self)

        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()
        self.target = target = gc.Target(device_id=0)


    def tearDown(self):
        delete_entries(self, self.target)
        self.a.tearDown()
        self.b.tearDown()
        BaseTest.tearDown(self)

    def runTest(self):
        logger.info("")

        ig_port = get_port_from_pipes(external_pipes)
        eg_port = get_port_from_pipes(external_pipes)

        int_port = get_internal_port_from_external(ig_port)
        logger.info("Expected forwarding path:")
        logger.info(" 1. Ingress processing in external pipe %d, ingress port %d", port_to_pipe(ig_port), ig_port)
        logger.info(" 2. Egress processing in internal pipe %d, internal port %d", port_to_pipe(int_port), int_port)
        logger.info(" 3. Loopback on internal port %d", int_port)
        logger.info(" 4. Ingress processing in internal pipe %d, internal port %d", port_to_pipe(int_port), int_port)
        logger.info(" 5. Egress processing in external pipe %d, egress port %d", port_to_pipe(eg_port), eg_port)

        port_meta_a = (0x12345678, 0xabcd)
        port_meta_b = 0x81
        smac = gc.bytes_to_mac(gc.to_bytes(random.randint(0, 2**48 -1), 6))
        dmac = gc.bytes_to_mac(gc.to_bytes(random.randint(0, 2**48 -1), 6))

        sip  = gc.bytes_to_ipv4(gc.to_bytes(random.randint(0, 2**32 -1), 4))
        dip  = gc.bytes_to_ipv4(gc.to_bytes(random.randint(0, 2**32 -1), 4))
        dg_type_a = random.choice([0,1])
        dg_type_b = random.choice([0,1])
        ttl = 64
        tag = 100

        # Use the default "All Pipes" in the target.  This will result in table
        # operations (add/mod/del/etc.) to be applied to all pipes the table is
        # present in.  So, an add to a table in profile A will update the table
        # in all pipes profile A is applied to.
        self.target = target = gc.Target(device_id=0)
        delete_learn_queue(self.a, target)
        delete_learn_queue(self.b, target)

        # Add entries and send one packet, it should forward and come back.
        program_entries(self, target, ig_port, int_port, eg_port, port_meta_a, port_meta_b, tag, dmac, dip, ttl)
        program_learning(self, target, dg_type_a, dg_type_b)

        logger.info("Sending packet on port %d", ig_port)
        pkt = testutils.simple_tcp_packet(eth_dst=dmac,
                                          eth_src=smac,
                                          ip_dst=dip,
                                          ip_src=sip,
                                          ip_ttl=ttl)
        testutils.send_packet(self, ig_port, pkt)

        pkt["IP"].ttl = pkt["IP"].ttl - 4
        exp_pkt = pkt
        logger.info("Expecting packet on port %d", eg_port)
        testutils.verify_packets(self, exp_pkt, [eg_port])

        verify_cntr_inc(self, target, dip, ttl-3, tag+2, 1)
        verify_digests(self.a, target, sip, dip, smac, dmac)
        verify_digests(self.b, target, sip, dip, smac, dmac)

        # Delete the entries and send another packet, it should be dropped.
        delete_entries(self, target)
        logger.info("")
        logger.info("Sending another packet on port %d", ig_port)
        pkt = testutils.simple_tcp_packet(eth_dst=dmac,
                                          ip_dst=dip,
                                          ip_ttl=ttl)
        testutils.send_packet(self, ig_port, pkt)

        logger.info("Packet is expected to get dropped.")
        testutils.verify_no_other_packets(self)


class MultiProgramFixedComponentTest(BaseTest):
    """@brief This test demonstrates accessing "fixed" tables through 2
        separate programs on the same device.
    """
    class ProgramA(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_32q_multiprogram_a"):
            BfRuntimeTest.setUp(self, client_id, p4_name)
            self.bfrt_info = self.interface.bfrt_info_get(p4_name)
            setup_random()

        def runTest(self):
            logger.info("")

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

        def setUpTables(self):
            """@brief this function sets up a certain set of tables
                in the program tna_32q_multiprogram_a
            """
            # Get all PRE table objects
            self.mgid_table = self.bfrt_info.table_get("$pre.mgid")
            self.node_table = self.bfrt_info.table_get("$pre.node")

    class ProgramB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_32q_multiprogram_b"):
            BfRuntimeTest.setUp(self, client_id, p4_name)
            self.bfrt_info = self.interface.bfrt_info_get(p4_name)
            setup_random()

        def runTest(self):
            logger.info("")

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

        def setUpTables(self):
            """@brief this function sets up a certain set of tables
                in the program tna_32q_multiprogram_b
            """
            # Get all PRE table objects
            self.mgid_table = self.bfrt_info.table_get("$pre.mgid")
            self.node_table = self.bfrt_info.table_get("$pre.node")

    def addMgidEntry(self, program, target, mgid_list):
        key_list = []
        data_list = []
        for mgid in mgid_list:
            key_list.append(program.mgid_table.make_key(
                    [gc.KeyTuple('$MGID', (mgid & 0xFFFF))]))
            data_list.append(program.mgid_table.make_data([
                    gc.DataTuple('$MULTICAST_NODE_ID', int_arr_val=[]),
                    gc.DataTuple('$MULTICAST_NODE_L1_XID_VALID', bool_arr_val=[]),
                    gc.DataTuple('$MULTICAST_NODE_L1_XID', int_arr_val=[]),
                    gc.DataTuple('$MULTICAST_ECMP_ID', int_arr_val=[]),
                    gc.DataTuple('$MULTICAST_ECMP_L1_XID_VALID', bool_arr_val=[]),
                    gc.DataTuple('$MULTICAST_ECMP_L1_XID', int_arr_val=[])]))

        program.mgid_table.entry_add(target, key_list, data_list)
        return key_list, data_list

    def addNodeEntry(self, program, target, mgid_list, l2_node_ports_list):
        l1_id = 1
        key_list = []
        data_list = []
        for mgid in mgid_list:
            rid = (~mgid) & 0xFFFF
            key_list += [program.node_table.make_key([
                    gc.KeyTuple('$MULTICAST_NODE_ID', l1_id)])]
            data_list += [program.node_table.make_data([
                    gc.DataTuple('$MULTICAST_RID', rid),
                    gc.DataTuple('$MULTICAST_LAG_ID', int_arr_val=[]),
                    gc.DataTuple('$DEV_PORT', int_arr_val=l2_node_ports_list[l1_id-1])])]
            l1_id += 1
        program.node_table.entry_add(target, key_list, data_list)
        return key_list, data_list

    def getMgidEntry(self, program, target, verify_data_list, verify_key_list):
        resp = program.mgid_table.entry_get(target)
        i = 0
        for data, key in resp:
            assert data == verify_data_list[i], "Received %s expected %s" %(str(data), str(verify_data_list[i]))
            assert key == verify_key_list[i], "Received %s expected %s" %(str(key), str(verify_key_list[i]))
            i += 1
        assert i == len(verify_key_list), "Received %d, expected %d" % (i, len(verify_key_list))

    def getNodeEntry(self, program, target, verify_data_list, verify_key_list):
        resp = program.node_table.entry_get(target)
        i = 0
        for data, key in resp:
            assert data == verify_data_list[i], "Received %s expected %s" %(str(data), str(verify_data_list[i]))
            assert key == verify_key_list[i], "Received %s expected %s" %(str(key), str(verify_key_list[i]))
            i += 1
        assert i == len(verify_key_list), "Received %d, expected %d" % (i, len(verify_key_list))

    def clearTable(self, table, target):
        key_list = []
        resp = table.entry_get(target)
        for data, key in resp:
            key_list.append(key)
        for k in key_list:
            table.entry_del(target, [k])

    def setUp(self):
        # Open 2 connections to the grpc server as 2 separate clients. Client
        # 1 (self.a) is in-charge of program "tna_32q_multiprogram_a" while
        # client 2 (self.b) is in-charge of program "tna_32q_multiprogram_b"
        # Thus any operation on a table in tna_32q_multiprogram_a needs to be
        # called on self.a and any operation on a table in
        # tna_32q_multiprogram_b needs to be called on self.b
        self.a = self.ProgramA()
        self.b = self.ProgramB()
        self.a.setUp(1)
        self.b.setUp(2)

        get_all_tables(self)
        self.a.setUpTables()
        self.b.setUpTables()

        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()


    def tearDown(self):
        print("Clearing tables")
        self.clearTable(self.b.mgid_table, self.target)
        self.clearTable(self.a.node_table, self.target)
        self.a.tearDown()
        self.b.tearDown()
        BaseTest.tearDown(self)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        self.target = target
        num_mgid = 20
        mgid_list = sorted(random.sample(range(0, 0x10000), num_mgid))
        l2_node_ports_list = [sorted(random.sample(swports, random.randint(0, len(swports)))) for mgid in mgid_list]

        # Add MGID entries though A
        # Add Node entries though B
        mgid_key_list, mgid_data_list = self.addMgidEntry(self.a, target, mgid_list)
        node_key_list, node_data_list = self.addNodeEntry(self.b, target, mgid_list, l2_node_ports_list)

        # Get and verify MGID entries though B
        self.getMgidEntry(self.b, target, mgid_data_list, mgid_key_list)
        # Get and Verify Node entries though A
        self.getNodeEntry(self.a, target, node_data_list, node_key_list)
