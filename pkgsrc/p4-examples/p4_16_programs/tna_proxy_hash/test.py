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
import socket
import struct

from ptf import config, mask
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as gc
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import google.rpc.code_pb2 as code_pb2

dev_id = 0
p4_program_name = "tna_proxy_hash"

logger = get_logger()
swports = get_sw_ports()


def ip2int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


def int2ip(int_val):
    b0 = (int_val >> 24) & 0xff
    b1 = (int_val >> 16) & 0xff
    b2 = (int_val >> 8) & 0xff
    b3 = (int_val >> 0) & 0xff
    return "{}.{}.{}.{}".format(b0, b1, b2, b3)


def crc8_value(val):
    import struct
    import crcmod
    crc8 = crcmod.predefined.Crc("crc-8")
    crc8.update(struct.pack(">I", val))
    return crc8.crcValue


class TestNoop(BfRuntimeTest):
    """@brief Verify packet forwarding functionality.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        ipv4_match_regular = bfrt_info.table_get("SwitchIngress.ipv4_match_regular")
        action_data = ipv4_match_regular.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[0])]
        )
        ipv4_match_regular.default_entry_set(
            target=target,
            data=action_data)

        try:
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='22:22:22:22:22:22',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, ipkt, swports[0])

        finally:
            ipv4_match_regular.default_entry_reset(target)


class TestMatchRegular(BfRuntimeTest):
    """@brief Test regular match table without proxy hash feature.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        ipv4_match_regular = bfrt_info.table_get("SwitchIngress.ipv4_match_regular")
        ipv4_match_regular.info.key_field_annotation_add('hdr.ethernet.dst_addr',
                                                         'mac')
        ipv4_match_regular.info.key_field_annotation_add('hdr.ethernet.src_addr',
                                                         'mac')
        ipv4_match_regular.info.key_field_annotation_add('hdr.ipv4.dst_addr',
                                                         'ipv4')
        ipv4_match_regular.info.key_field_annotation_add('hdr.ipv4.src_addr',
                                                         'ipv4')

        key_data_regular = ipv4_match_regular.make_key([
            gc.KeyTuple(name='hdr.ethernet.dst_addr',
                        value='11:11:11:11:11:11'),
            gc.KeyTuple(name='hdr.ethernet.src_addr',
                        value='22:22:22:22:22:22'),
            gc.KeyTuple(name='hdr.ipv4.dst_addr',
                        value='100.99.98.97'),
            gc.KeyTuple(name='hdr.ipv4.src_addr',
                        value='1.2.3.4'),
        ])

        action_data_regular = ipv4_match_regular.make_data(
            [gc.DataTuple(name='port_id', val=swports[3])], 'SwitchIngress.set_output_port'
        )

        ipv4_match_regular.entry_add(
            target,
            [key_data_regular],
            [action_data_regular])

        try:
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='22:22:22:22:22:22',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, ipkt, swports[3])

        finally:
            ipv4_match_regular.default_entry_reset(target)
            ipv4_match_regular.entry_del(target, [key_data_regular])


class TestMatchProxyHash(BfRuntimeTest):
    """@brief Test match table with proxy hash feature.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        ipv4_match_regular = bfrt_info.table_get("SwitchIngress.ipv4_match_regular")
        action_data = ipv4_match_regular.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[0])]
        )
        ipv4_match_regular.default_entry_set(
            target=target,
            data=action_data)

        ipv4_match_proxy_hash = bfrt_info.table_get("SwitchIngress.ipv4_match_proxy_hash")
        ipv4_match_proxy_hash.info.key_field_annotation_add('hdr.ethernet.dst_addr',
                                                            'mac')
        ipv4_match_proxy_hash.info.key_field_annotation_add('hdr.ethernet.src_addr',
                                                            'mac')
        ipv4_match_proxy_hash.info.key_field_annotation_add('hdr.ipv4.dst_addr',
                                                            'ipv4')
        ipv4_match_proxy_hash.info.key_field_annotation_add('hdr.ipv4.src_addr',
                                                            'ipv4')

        key_data_proxy_hash = ipv4_match_proxy_hash.make_key([
            gc.KeyTuple(name='hdr.ethernet.dst_addr',
                        value='11:11:11:11:11:11'),
            gc.KeyTuple(name='hdr.ethernet.src_addr',
                        value='22:22:22:22:22:22'),
            gc.KeyTuple(name='hdr.ipv4.dst_addr',
                        value='100.99.98.97'),
            gc.KeyTuple(name='hdr.ipv4.src_addr',
                        value='1.2.3.4')
        ])

        action_data_proxy_hash = ipv4_match_proxy_hash.make_data(
            [gc.DataTuple(name='port_id', val=swports[5])], 'SwitchIngress.set_output_port'
        )

        ipv4_match_proxy_hash.entry_add(
            target,
            [key_data_proxy_hash],
            [action_data_proxy_hash])

        try:
            ipkt1 = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                                eth_src='22:22:22:22:22:22',
                                                ip_src='1.2.3.4',
                                                ip_dst='100.99.98.97',
                                                ip_id=101,
                                                ip_ttl=64,
                                                udp_sport=0x1234,
                                                udp_dport=0xabcd)

            testutils.send_packet(self, swports[0], ipkt1)
            testutils.verify_packet(self, ipkt1, swports[5])

            ipkt2 = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                                eth_src='22:22:22:22:22:22',
                                                ip_src='1.2.3.4',
                                                ip_dst='98.97.100.99',
                                                ip_id=101,
                                                ip_ttl=64,
                                                udp_sport=0x1234,
                                                udp_dport=0xabcd)

            testutils.send_packet(self, swports[0], ipkt2)
            testutils.verify_packet(self, ipkt2, swports[0])

        finally:
            ipv4_match_regular.default_entry_reset(target)
            ipv4_match_proxy_hash.entry_del(target, [key_data_proxy_hash])

class ProxyHashGetFromHwAnyPipeTest(BfRuntimeTest):
    """@brief Proxy hash exact match table test: Get entry from hw from any pipe.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)
        # Get bfrt_info and set it as part of the test
        self.bfrt_info = self.interface.bfrt_info_get(p4_program_name)
        self.table = self.bfrt_info.table_get("SwitchIngress.ipv4_match_proxy_hash")
        self.table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
        self.table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")
        self.table.info.key_field_annotation_add('hdr.ipv4.dst_addr', 'ipv4')
        self.table.info.key_field_annotation_add('hdr.ipv4.src_addr', 'ipv4')

        self.all_pipes_target = gc.Target(device_id=0, pipe_id=0xffff)

    def tearDown(self):
        mode = bfruntime_pb2.Mode.ALL
        self.table.attribute_entry_scope_set(self.all_pipes_target,
                                             predefined_pipe_scope=True,
                                             predefined_pipe_scope_val=mode)
        BfRuntimeTest.tearDown(self)

    def checkResponse(self, resp, eg_port, valid, code = code_pb2.NOT_FOUND):
        try:
            data_dict = next(resp)[0].to_dict()
        except gc.BfruntimeReadWriteRpcException as e:
            error_list = e.sub_errors_get()
            assert len(error_list) == 1
            assert valid == False
            p4_error = error_list[0]
            assert p4_error[1].canonical_code == code
            return
        assert valid, "egress port = %s" %(str(eg_port))
        recv_port = data_dict["port_id"]
        if (recv_port != eg_port):
            logger.error("Error! egress port = %s received port = %s", str(eg_port), str(recv_port))
            assert 0

    def checkEntry(self, target_scope, target_local, dmac, smac, dip, sip, eg_port,
                  valid = True, valid_handle = True, valid_from_handle = True,
                  valid_from_sw = True):
        key = self.table.make_key([
            gc.KeyTuple(name='hdr.ethernet.dst_addr',
                        value=dmac),
            gc.KeyTuple(name='hdr.ethernet.src_addr',
                        value=smac),
            gc.KeyTuple(name='hdr.ipv4.dst_addr',
                        value=dip),
            gc.KeyTuple(name='hdr.ipv4.src_addr',
                        value=sip)
        ])
        resp = self.table.entry_get(
            target_local,
            [key],
            {"from_hw": True})
        self.checkResponse(resp, eg_port, valid)

        #From SW
        resp = self.table.entry_get(
            target_local,
            [key],
            {"from_hw": False})
        self.checkResponse(resp, eg_port, valid and valid_from_sw)

        # Test get with handle
        try:
            handle = self.table.handle_get(target_scope, [key])
        except gc.BfruntimeReadWriteRpcException as e:
            error_list = e.sub_errors_get()
            assert len(error_list) == 1
            assert valid_handle == False
            p4_error = error_list[0]
            assert p4_error[1].canonical_code == code_pb2.NOT_FOUND
            return
        assert valid_handle, "i = %s" %(str(i))

        resp = self.table.entry_get(
            target_local,
            None,
            handle=handle)
        self.checkResponse(resp, eg_port, valid_from_handle,
                           code = code_pb2.INVALID_ARGUMENT)
        # From SW
        resp = self.table.entry_get(
            target_local,
            None,
            handle=handle,
            flags = {"from_hw": False})
        self.checkResponse(resp, eg_port, valid_from_handle and valid_from_sw)

    def checkDefaultEntry(self, target_local, valid = True):
        resp = self.table.default_entry_get(
            target_local,
            flags={"from_hw": True})
        try:
            data_dict = next(resp)[0].to_dict()
        except gc.BfruntimeReadWriteRpcException as e:
            error_list = e.sub_errors_get()
            assert len(error_list) == 1
            assert valid == False
            p4_error = error_list[0]
            assert p4_error[1].canonical_code == code_pb2.INVALID_ARGUMENT
            return

        assert valid
        recv_action = data_dict["action_name"]
        if (recv_action != 'NoAction'):
            logger.error("Error! Expected Action NoAction received = %s", recv_action)
            assert 0

    def runSymmetricScopeTest(self, dmac, smac, dip, sip, eg_port):
        logger.info("Symmetric Scope Entry Get from hw any pipe")
        all_targets = [self.all_pipes_target]
        for p in range(self.num_pipes):
            target_local = gc.Target(device_id=0, pipe_id=p)
            all_targets.append(target_local)

        key = self.table.make_key([
            gc.KeyTuple(name='hdr.ethernet.dst_addr',
                        value=dmac),
            gc.KeyTuple(name='hdr.ethernet.src_addr',
                        value=smac),
            gc.KeyTuple(name='hdr.ipv4.dst_addr',
                        value=dip),
            gc.KeyTuple(name='hdr.ipv4.src_addr',
                        value=sip)
        ])
        self.table.entry_add(
            self.all_pipes_target,
            [key],
            [self.table.make_data([gc.DataTuple('port_id', eg_port)],
                                   'SwitchIngress.set_output_port')])

        try:
            # check get
            valid_from_sw = True
            for target_local in all_targets:
                self.checkEntry(self.all_pipes_target, target_local, dmac, smac,
                                dip, sip, eg_port,
                                valid_from_sw = valid_from_sw)
                # Only tatget pipe all is valid from SW for symmetric table.
                valid_from_sw = False
                # Default entry
                resp = self.table.entry_get(
                    target_local,
                    handle=1,
                    flags={"from_hw": True})
                data_dict = next(resp)[0].to_dict()
                recv_action = data_dict["action_name"]
                if (recv_action != 'NoAction'):
                    logger.error("Error! Expected Action NoAction received = %s", recv_action)
                    assert 0
                self.checkDefaultEntry(target_local)
        finally:
            self.table.entry_del(
               self.all_pipes_target,
               [])

    def runSingleScopeTest(self, dmacs, smacs, dips, sips, eg_port0, eg_port1):
        logger.info("Single Scope Entry Get from hw any pipe")
        all_targets = []
        self.table.attribute_entry_scope_set(self.all_pipes_target,
                        predefined_pipe_scope=True,
                        predefined_pipe_scope_val=bfruntime_pb2.Mode.SINGLE)
        try:
            for p in range(self.num_pipes):
                target_local = gc.Target(device_id=0, pipe_id=p)
                all_targets.append(target_local)
                if p % 2 == 0:
                    eg_port = eg_port0
                else:
                    eg_port = eg_port1
                key = self.table.make_key([
                    gc.KeyTuple(name='hdr.ethernet.dst_addr',
                                value=dmacs[p]),
                    gc.KeyTuple(name='hdr.ethernet.src_addr',
                                value=smacs[p]),
                    gc.KeyTuple(name='hdr.ipv4.dst_addr',
                                value=dips[p]),
                    gc.KeyTuple(name='hdr.ipv4.src_addr',
                                value=sips[p])
                ])
                self.table.entry_add(
                    target_local,
                    [key],
                    [self.table.make_data([gc.DataTuple('port_id', eg_port)],
                                           'SwitchIngress.set_output_port')])

            # Check get for each pipe.
            for p in range(self.num_pipes):
                if p % 2 == 0:
                    eg_port = eg_port0
                else:
                    eg_port = eg_port1
                # Try all entries.
                for q in range(self.num_pipes):
                    if q == p:
                        valid = True
                    else:
                        valid = False

                    self.checkEntry(all_targets[q], all_targets[p], dmacs[q],
                                    smacs[q], dips[q], sips[q], eg_port, valid,
                                    valid_from_handle = valid,
                                    valid_from_sw = valid)

                # Check target All Pipes.
                self.checkEntry(all_targets[p], self.all_pipes_target, dmacs[p],
                                smacs[p], dips[p], sips[p], eg_port, valid = False,
                                valid_from_handle = True)
                # Default entry
                self.checkDefaultEntry(all_targets[p])

            self.checkDefaultEntry(self.all_pipes_target, valid = False)

        finally:
            for local_target in all_targets:
                self.table.entry_del(
                    local_target,
                    [])

    def runUserDefinedScopeTest(self, dmacs, smacs, dips, sips, eg_port0, eg_port1):
        logger.info("User Defined Scope Entry Get from hw any pipe")
        all_targets = []
        for p in range(self.num_pipes):
            target_local = gc.Target(device_id=0, pipe_id=p)
            all_targets.append(target_local)

        # Set pipes 0 and 1 in scope 1 and pipes 2 and 3 in scope 2
        # Note this cannot be done during replay again, since
        # "changing" entry scope while entries are present isn't
        # allowed.
        if self.num_pipes >= 4:
            scope_args=0xc03
            num_scopes = 2
        else:
            scope_args=0x3
            num_scopes = 1
        self.table.attribute_entry_scope_set(self.all_pipes_target,
            predefined_pipe_scope=False, user_defined_pipe_scope_val=scope_args)

        try:
            for scope in range(num_scopes):
                if scope == 0:
                    p = 0
                    eg_port = eg_port0
                else:
                    p = 2
                    eg_port = eg_port1
                target_local = all_targets[p]
                key = self.table.make_key([
                    gc.KeyTuple(name='hdr.ethernet.dst_addr',
                                value=dmacs[scope]),
                    gc.KeyTuple(name='hdr.ethernet.src_addr',
                                value=smacs[scope]),
                    gc.KeyTuple(name='hdr.ipv4.dst_addr',
                                value=dips[scope]),
                    gc.KeyTuple(name='hdr.ipv4.src_addr',
                                value=sips[scope])
                ])
                self.table.entry_add(
                    target_local,
                    [key],
                    [self.table.make_data([gc.DataTuple('port_id', eg_port)],
                                           'SwitchIngress.set_output_port')])

            # Try all entries.
            for scope in range(num_scopes):
                # Check get for each pipe.
                for p in range(self.num_pipes):
                    valid = False
                    valid_from_sw = False
                    if scope == 0:
                        if p < 2: valid = True
                        if p == 0: valid_from_sw = True
                        eg_port = eg_port0
                    else:
                        if p >= 2 and p < 4: valid = True
                        if p == 2: valid_from_sw = True
                        eg_port = eg_port1
                    self.checkEntry(all_targets[2*scope], all_targets[p],
                                    dmacs[scope], smacs[scope], dips[scope],
                                    sips[scope], eg_port, valid,
                                    valid_from_handle = valid,
                                    valid_from_sw = valid_from_sw)

                # Check target All Pipes.
                self.checkEntry(all_targets[2*scope], self.all_pipes_target,
                                dmacs[scope], smacs[scope], dips[scope],
                                dips[scope], eg_port, valid = False,
                                valid_handle=False,
                                valid_from_sw = False)


            for p in range(self.num_pipes):
                if p < 2 * num_scopes:
                    valid = True
                else:
                    valid = False
                # Default entry
                self.checkDefaultEntry(all_targets[p], valid)

            self.checkDefaultEntry(self.all_pipes_target, valid = False)

        finally:
            for scope in range(num_scopes):
                self.table.entry_del(
                    all_targets[2 * scope],
                    [])

    def runTest(self):
        ig_port = swports[1]
        eg_ports = [swports[2], swports[3]]

        self.num_pipes = int(testutils.test_param_get('num_pipes'))
        num_pipes = self.num_pipes

        dmacs = ["%02x:%02x:%02x:%02x:%02x:%02x" % (
            random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
            random.randint(0, 255), random.randint(0, 255)) for x in range(num_pipes)]
        smacs = ["%02x:%02x:%02x:%02x:%02x:%02x" % (
            random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
            random.randint(0, 255), random.randint(0, 255)) for x in range(num_pipes)]

        dips = ['100.99.98.97'] * num_pipes
        sips = ['1.2.3.4'] * num_pipes

        self.runSymmetricScopeTest(dmacs[0], smacs[0] , dips[0], sips[0], eg_ports[0])
        self.runSingleScopeTest(dmacs, smacs, dips, sips, eg_ports[0], eg_ports[1])
        self.runUserDefinedScopeTest(dmacs, smacs, dips, sips, eg_ports[0], eg_ports[1])
