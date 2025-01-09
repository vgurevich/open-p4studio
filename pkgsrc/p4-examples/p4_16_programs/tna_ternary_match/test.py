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
import ipaddress
from math import gcd
import random

from ptf import config
from collections import namedtuple
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc
import grpc
import google.rpc.code_pb2 as code_pb2

logger = get_logger()
swports = get_sw_ports()

# tuple for future refs
key_random_tuple = namedtuple('key_random', 'dst_ip mask priority')
key_random_tuple.__new__.__defaults__ = (None, None, None)

num_pipes = int(testutils.test_param_get('num_pipes'))

swports_0 = []
swports_1 = []
swports_2 = []
swports_3 = []

# the following method categorizes the ports in ports.json file as belonging to either of the pipes (0, 1, 2, 3)
for port in swports:
    pipe = port_to_pipe(port)
    if pipe == 0:
        swports_0.append(port)
    elif pipe == 1:
        swports_1.append(port)
    elif pipe == 2:
        swports_2.append(port)
    elif pipe == 3:
        swports_3.append(port)


def delete_all(forward_table, num_entries, tuple_list, target):
    i = 0
    for item in tuple_list:
        usage = next(forward_table.usage_get(target, flags={'from_hw':False}))
        logger.info("While deleting current entries = %d expected = %d", usage, num_entries - i)
        key = forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', getattr(tuple_list[i], "priority")),
                                     gc.KeyTuple('vrf', 0),
                                     gc.KeyTuple('hdr.ipv4.dst_addr', getattr(item, "dst_ip"),
                                                 getattr(item, "mask"))])
        key.apply_mask()
        forward_table.entry_del(target, [key])
        i += 1


class TernaryMatchTest(BfRuntimeTest):
    """@brief Basic ternary match test using a TCAM-based match table
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        ig_port = swports[1]
        eg_ports = [swports[2], swports[3]]
        dip = '10.10.0.1'

        pkt = testutils.simple_tcp_packet(ip_dst=dip)
        exp_pkt = pkt

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")

        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        target = gc.Target(device_id=0, pipe_id=0xffff)
        try:
            forward_table.entry_add(
                target,
                [forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                         gc.KeyTuple('vrf', 0),
                                         gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.0.0')])],
                [forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                         'SwitchIngress.hit')])

            # check get
            resp = forward_table.entry_get(
                target,
                [forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                         gc.KeyTuple('vrf', 0),
                                         gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.0.0')])],
                {"from_hw": True})

            data_dict = next(resp)[0].to_dict()
            recv_port = data_dict["port"]
            if (recv_port != eg_ports[0]):
                logger.error("Error! port sent = %s received port = %s", str(eg_ports[0]), str(recv_port))
                assert 0

            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, exp_pkt, eg_ports[0])
            testutils.verify_no_other_packets(self, timeout=2)

        finally:
            forward_table.entry_del(
                target,
                [forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                         gc.KeyTuple('vrf', 0),
                                         gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.0.0')])])

            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_no_other_packets(self)

class TernaryGetFromHwAnyPipeTest(BfRuntimeTest):
    """@brief Ternary match table test: Get entry from hw from any pipe.
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)
        # Get bfrt_info and set it as part of the test
        self.bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
        self.all_pipes_target = gc.Target(device_id=0, pipe_id=0xffff)

    def tearDown(self):
        mode = bfruntime_pb2.Mode.ALL
        self.forward_table.attribute_entry_scope_set(self.all_pipes_target,
                                                     predefined_pipe_scope=True,
                                                     predefined_pipe_scope_val=mode)
        BfRuntimeTest.tearDown(self)

    def checkResponse(self, resp, eg_port, valid = True, code = code_pb2.NOT_FOUND):
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
        recv_port = data_dict["port"]
        if (recv_port != eg_port):
            logger.error("Error! egress port = %s received port = %s", str(eg_port), str(recv_port))
            assert 0

    def checkEntry(self, target_scope, target_local, dip, eg_port, valid = True,
                  valid_from_handle = True, valid_from_sw = True):
        key = self.forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                           gc.KeyTuple('vrf', 0),
                                           gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.0.0')])
        resp = self.forward_table.entry_get(
            target_local,
            [key],
            {"from_hw": True})
        self.checkResponse(resp, eg_port, valid)

        # Check get from SW
        resp = self.forward_table.entry_get(
            target_local,
            [key],
            {"from_hw": False})
        self.checkResponse(resp, eg_port, valid and valid_from_sw)

        # Test get with handle
        handle = self.forward_table.handle_get(target_scope, [key])

        resp = self.forward_table.entry_get(
            target_local,
            None,
            handle=handle)
        self.checkResponse(resp, eg_port, valid_from_handle,
                           code = code_pb2.INVALID_ARGUMENT)

        resp = self.forward_table.entry_get(
            target_local,
            None,
            handle = handle,
            flags = {"from_hw": False})
        self.checkResponse(resp, eg_port, valid_from_handle and valid_from_sw)


    def checkDefaultEntry(self, target_local, valid = True):
        resp = self.forward_table.default_entry_get(
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
        if (recv_action != 'SwitchIngress.miss'):
            logger.error("Error! Expected Action witchIngress.miss received = %s", recv_action)
            assert 0

    def runSymmetricScopeTest(self, dip, eg_port):
        logger.info("Symmetric Scope Entry Get from hw any pipe")
        all_targets = [self.all_pipes_target]
        for p in range(num_pipes):
            target_local = gc.Target(device_id=0, pipe_id=p)
            all_targets.append(target_local)

        self.forward_table.entry_add(
            self.all_pipes_target,
            [self.forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                          gc.KeyTuple('vrf', 0),
                                          gc.KeyTuple('hdr.ipv4.dst_addr', dip, '255.255.0.0')])],
            [self.forward_table.make_data([gc.DataTuple('port', eg_port)],
                                           'SwitchIngress.hit')])

        try:
            # check get
            valid_from_sw = True
            for target_local in all_targets:
                self.checkEntry(self.all_pipes_target, target_local, dip,
                                eg_port, valid_from_sw = valid_from_sw)
                # Only target all pipes should be valid when reading from SW.
                valid_from_sw = False
                # Default entry
                resp = self.forward_table.entry_get(
                    target_local,
                    handle=1,
                    flags={"from_hw": True})
                data_dict = next(resp)[0].to_dict()
                recv_action = data_dict["action_name"]
                if (recv_action != 'SwitchIngress.miss'):
                    logger.error("Error! Expected Action witchIngress.miss received = %s", recv_action)
                    assert 0
                self.checkDefaultEntry(target_local)
        finally:
            self.forward_table.entry_del(
               self.all_pipes_target,
               [])

    def runSingleScopeTest(self, dips, eg_port0, eg_port1):
        logger.info("Single Scope Entry Get from hw any pipe")
        all_targets = []
        self.forward_table.attribute_entry_scope_set(self.all_pipes_target,
                        predefined_pipe_scope=True,
                        predefined_pipe_scope_val=bfruntime_pb2.Mode.SINGLE)
        try:
            for p in range(num_pipes):
                target_local = gc.Target(device_id=0, pipe_id=p)
                all_targets.append(target_local)
                if p % 2 == 0:
                    eg_port = eg_port0
                else:
                    eg_port = eg_port1
                self.forward_table.entry_add(
                    target_local,
                    [self.forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                                  gc.KeyTuple('vrf', 0),
                                                  gc.KeyTuple('hdr.ipv4.dst_addr', dips[p], '255.255.0.0')])],
                    [self.forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                  'SwitchIngress.hit')])

            # Check get for each pipe.
            for p in range(num_pipes):
                if p % 2 == 0:
                    eg_port = eg_port0
                else:
                    eg_port = eg_port1
                # Try all entries.
                for q in range(num_pipes):
                    if q == p:
                        valid = True
                    else:
                        valid = False
                    self.checkEntry(all_targets[q], all_targets[p], dips[q],
                                    eg_port, valid, valid_from_handle=valid,
                                    valid_from_sw = valid)

                # Check target All Pipes.
                self.checkEntry(all_targets[p], self.all_pipes_target, dips[p],
                                eg_port, valid = False,
                                valid_from_handle = True)
                # Default entry
                self.checkDefaultEntry(all_targets[p])

            self.checkDefaultEntry(self.all_pipes_target, valid = False)

        finally:
            for local_target in all_targets:
                self.forward_table.entry_del(
                    local_target,
                    [])

    def runUserDefinedScopeTest(self, dips, eg_port0, eg_port1):
        logger.info("User Defined Scope Entry Get from hw any pipe")
        all_targets = []
        for p in range(num_pipes):
            target_local = gc.Target(device_id=0, pipe_id=p)
            all_targets.append(target_local)

        # Set pipes 0 and 1 in scope 1 and pipes 2 and 3 in scope 2
        # Note this cannot be done during replay again, since
        # "changing" entry scope while entries are present isn't
        # allowed.
        if num_pipes >= 4:
            scope_args=0xc03
            num_scopes = 2
        else:
            scope_args=0x3
            num_scopes = 1
        self.forward_table.attribute_entry_scope_set(self.all_pipes_target,
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

                self.forward_table.entry_add(
                    target_local,
                    [self.forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                                  gc.KeyTuple('vrf', 0),
                                                  gc.KeyTuple('hdr.ipv4.dst_addr', dips[scope], '255.255.0.0')])],
                    [self.forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                  'SwitchIngress.hit')])

            # Try all entries.
            for scope in range(num_scopes):
                # Check get for each pipe.
                for p in range(num_pipes):
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
                                    dips[scope], eg_port, valid,
                                    valid_from_handle = valid,
                                    valid_from_sw = valid_from_sw)

                # Check target All Pipes.
                self.checkEntry(all_targets[2*scope], self.all_pipes_target,
                               dips[scope], eg_port,
                               valid = False, valid_from_handle = True)

            for p in range(num_pipes):
                if p < 2 * num_scopes:
                    valid = True
                else:
                    valid = False
                # Default entry
                self.checkDefaultEntry(all_targets[p], valid)

            self.checkDefaultEntry(self.all_pipes_target, valid = False)

        finally:
            for scope in range(num_scopes):
                self.forward_table.entry_del(
                    all_targets[2 * scope],
                    [])

    def runTest(self):
        ig_port = swports[1]
        eg_ports = [swports[2], swports[3]]
        dips = []
        for p in range(1, num_pipes + 1):
            dip = '10.%d.0.1' % (p)
            dips.append(dip)
        self.runSymmetricScopeTest(dips[0], eg_ports[0])
        self.runSingleScopeTest(dips, eg_ports[0], eg_ports[1])
        self.runUserDefinedScopeTest(dips, eg_ports[0], eg_ports[1])


class TernaryMultipleRollbackTest(BfRuntimeTest):
    """@brief This test writes multiple entries to a ternary match table with 
    ROLLBACK_ON_ERROR mode on. So we check that no entries go in if there was 
    an error
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        eg_ports = [swports[5], swports[3]]

        seed = setup_random()

        num_entries = random.randint(10, 100)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        target = gc.Target(device_id=0, pipe_id=0xffff)

        key_list = []
        data_list = []

        ip_tuple_list = self.generate_random_ip_list(num_entries, seed)

        try:
            i = 0
            tuple_list = []

            logger.info("Inserting %d entries", num_entries)
            while i < num_entries:
                prio = random.randint(1, 5000)
                logger.info("Inserting entry_#=%d and dst_ip=%s", i + 1, ip_tuple_list[i].ip)
                dst_ip = ip_tuple_list[i].ip
                mask = ip_tuple_list[i].mask

                tuple_list.append(key_random_tuple(dst_ip, mask, prio))
                key_list.append(forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', prio),
                                                        gc.KeyTuple('vrf', 0),
                                                        gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip, mask)]))

                data_list.append(forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                                         'SwitchIngress.hit'))

                # Add a duplicate entry after every 3 entries
                if (i % 3 == 0):
                    key_list.append(
                        forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', getattr(tuple_list[i], "priority")),
                                                gc.KeyTuple('vrf', 0),
                                                gc.KeyTuple('hdr.ipv4.dst_addr', getattr(tuple_list[i], "dst_ip"),
                                                            getattr(tuple_list[i], "mask"))]))

                    data_list.append(forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                                             'SwitchIngress.hit'))
                i += 1

            try:
                forward_table.entry_add(target, key_list, data_list, bfruntime_pb2.WriteRequest.ROLLBACK_ON_ERROR)
            except gc.BfruntimeRpcException as e:
                # The error list should only have one error since the write
                # request should have failed at the very first error
                error_list = e.sub_errors_get()
                logger.info("Expected error length = %d Received %d",
                            1, len(error_list))
                assert len(error_list) == 1

            # Get Table Usage
            usage = next(forward_table.usage_get(target, flags={'from_hw':False}))
            logger.info("Current entries = %d expected = %d", usage, 0)
            assert usage == 0
        finally:
            logger.info("The cleanup is expected to fail because the rollback removed all entries added before.")
            try:
                delete_all(forward_table, num_entries, tuple_list, target)
            except gc.BfruntimeRpcException as e:
                pass


class TernaryMultipleContinueOnErrorTest(BfRuntimeTest):
    """@brief This test writes multiple entries to a ternary match table with 
    the default CONTINUE_ON_ERROR mode on. Only the ones that are supposed to 
    fail should and the rest of them should go in
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        eg_ports = [swports[5], swports[3]]

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        seed = setup_random()
        num_entries = random.randint(10, 100)

        target = gc.Target(device_id=0, pipe_id=0xffff)
        ip_tuple_list = self.generate_random_ip_list(num_entries, seed)

        try:
            i = 0
            tuple_list = []
            key_list = []
            data_list = []
            logger.info("Inserting %d entries", num_entries)
            while i < num_entries:
                prio = random.randint(1, 5000)
                logger.info("Inserting entry_#=%d and dst_ip=%s", i + 1, ip_tuple_list[i].ip)
                dst_ip = ip_tuple_list[i].ip
                mask = ip_tuple_list[i].mask

                tuple_list.append(key_random_tuple(dst_ip, mask, prio))
                key_list.append(forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', prio),
                                                        gc.KeyTuple('vrf', 0),
                                                        gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip, mask)]))

                data_list.append(forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                                         'SwitchIngress.hit'))
                # Add a duplicate entry after every 3 entries
                if (i % 3 == 0):
                    key_list.append(
                        forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', getattr(tuple_list[i], "priority")),
                                                gc.KeyTuple('vrf', 0),
                                                gc.KeyTuple('hdr.ipv4.dst_addr', getattr(tuple_list[i], "dst_ip"),
                                                            getattr(tuple_list[i], "mask"))]))

                    data_list.append(forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                                             'SwitchIngress.hit'))
                i += 1

            try:
                forward_table.entry_add(target, key_list, data_list)
            except gc.BfruntimeRpcException as e:
                # The error list should have as many errors as there were duplicate entries
                # That means there are num_entries/3 errors
                error_list = e.sub_errors_get()
                import math
                logger.info("Expected error length = %d Received = %d",
                            math.ceil(float(num_entries) / 3), len(error_list))
                print(e)
                assert len(error_list) == math.ceil(float(num_entries) / 3)

            # Get Table Usage
            usage = next(forward_table.usage_get(target, flags={'from_hw':False}))
            logger.info("Current entries = %d expected = %d", usage, num_entries)
            assert usage == num_entries

        finally:
            delete_all(forward_table, num_entries, tuple_list, target)


class TernaryMatchMultipleEntryTest(BfRuntimeTest):
    """@brief Add a set of entries and verify the correct one is selected.
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        eg_ports = [swports[5], swports[3]]

        seed = setup_random()
        num_entries = random.randint(1, 100)

        target = gc.Target(device_id=0, pipe_id=0xffff)
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "bytes")

        ip_tuple_list = self.generate_random_ip_list(num_entries, seed)
        try:
            i = 0
            tuple_list = []
            key_list = []
            data_list = []
            while i < num_entries:
                prio = random.randint(1, 5000)
                logger.info("Inserting entry_#=%d and dst_ip=%s", i + 1, ip_tuple_list[i].ip)
                dst_ip = ip_tuple_list[i].ip
                mask = ip_tuple_list[i].mask

                i += 1
                tuple_list.append(key_random_tuple(gc.ipv4_to_bytes(dst_ip), gc.to_bytes(mask, 4), prio))
                key_list.append(forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', prio),
                                                        gc.KeyTuple('vrf', 0),
                                                        gc.KeyTuple('hdr.ipv4.dst_addr', gc.ipv4_to_bytes(dst_ip),
                                                                    gc.to_bytes(mask, 4))]))

                data_list.append(forward_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                                         'SwitchIngress.hit'))
            forward_table.entry_add(target, key_list, data_list)

            # Get Table Usage
            usage = next(forward_table.usage_get(target, flags={'from_hw':False}))
            logger.info("Current entries = %d expected = %d",
                        usage, num_entries)
            assert usage == num_entries

            # check get all
            resp = forward_table.entry_get(
                target,
                None,
                {"from_hw": True})

            i = 0
            for data, key in resp:
                data_dict = data.to_dict()
                key_dict = key.to_dict()
                recv_port = data_dict["port"]
                if (recv_port != eg_ports[0]):
                    logger.error("Error! port sent = %s received port = %s", str(eg_ports[0]), str(recv_port))
                    assert 0
                ip_addr = getattr(tuple_list[i], "dst_ip")
                mask = getattr(tuple_list[i], "mask")
                for k in range(4):
                    ip_addr[k] = ip_addr[k] & mask[k]
                assert key_dict["vrf"]['value'] == 0
                assert key_dict["hdr.ipv4.dst_addr"]["value"] == ip_addr
                assert key_dict["hdr.ipv4.dst_addr"]["mask"] == mask
                assert key_dict["$MATCH_PRIORITY"]["value"] == getattr(tuple_list[i], "priority")
                i += 1

        finally:
            delete_all(forward_table, num_entries, tuple_list, target)


class AlgorithmicTernaryMatchTest(BfRuntimeTest):
    """@brief Basic example for algorithmic-TCAM-based match tables.
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        ig_port = swports[1]
        eg_ports = [swports[5], swports[3]]

        seed = setup_random()
        num_entries = random.randint(5, 15)

        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        forward_atcam_table = bfrt_info.table_get("SwitchIngress.forward_atcam")
        set_partition_table = bfrt_info.table_get("SwitchIngress.set_partition")
        forward_atcam_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        tuple_list = []
        target = gc.Target(device_id=0, pipe_id=0xffff)
        logger.info("Testing for %d entries", num_entries)
        set_partition_table.entry_add(
            target,
            [set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 6)])],
            [set_partition_table.make_data([gc.DataTuple('p_index', 3)],
                                           'SwitchIngress.init_index')]
        )

        set_partition_table.entry_add(
            target,
            [set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 17)])],
            [set_partition_table.make_data([gc.DataTuple('p_index', 1)],
                                           'SwitchIngress.init_index')]
        )

        dst_ip_list = self.generate_random_ip_list(num_entries, seed)
        atcam_dict = {}
        for i in range(num_entries):
            # insert entry in both set_partition and forward_atcam
            # insert entry for TCP and UDP both in set_partition
            key = forward_atcam_table.make_key([gc.KeyTuple('ig_md.partition.partition_index', 3),
                                               gc.KeyTuple('$MATCH_PRIORITY', 1),
                                               gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip_list[i].ip,
                                                           dst_ip_list[i].mask)])
            data = forward_atcam_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                               'SwitchIngress.hit')
            forward_atcam_table.entry_add(target, [key], [data])
            key.apply_mask()
            atcam_dict[key] = data

            key = forward_atcam_table.make_key([gc.KeyTuple('ig_md.partition.partition_index', 1),
                                               gc.KeyTuple('$MATCH_PRIORITY', 1),
                                               gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip_list[i].ip,
                                                           dst_ip_list[i].mask)])
            data = forward_atcam_table.make_data([gc.DataTuple('port', eg_ports[0])],
                                               'SwitchIngress.hit')
            forward_atcam_table.entry_add(target, [key], [data])
            key.apply_mask()
            atcam_dict[key] = data

        # Check get
        resp = forward_atcam_table.entry_get(target)
        for data, key in resp:
            assert atcam_dict[key] == data, "Received key = %s, received data = %s" %(str(key), str(data))
            atcam_dict.pop(key)
        assert len(atcam_dict) == 0

        # send pkt and verify sent
        for item in dst_ip_list:
            # TCP
            pkt = testutils.simple_tcp_packet(ip_dst=item.ip)
            exp_pkt = pkt
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, exp_pkt, eg_ports[0])

            # UDP
            pkt = testutils.simple_udp_packet(ip_dst=item.ip)
            exp_pkt = pkt
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, exp_pkt, eg_ports[0])

        testutils.verify_no_other_packets(self, timeout=2)
        # Delete both the partition table entries
        set_partition_table.entry_del(
            target,
            [set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 6)])])

        set_partition_table.entry_del(
            target,
            [set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 17)])])

        # Delete all entries
        logger.info("Deleting %d entries", num_entries)

        for item in dst_ip_list:
            forward_atcam_table.entry_del(
                target,
                [forward_atcam_table.make_key([gc.KeyTuple('ig_md.partition.partition_index', 1),
                                               gc.KeyTuple('$MATCH_PRIORITY', 1),
                                               gc.KeyTuple('hdr.ipv4.dst_addr', item.ip, item.mask)])])
            forward_atcam_table.entry_del(
                target,
                [forward_atcam_table.make_key([gc.KeyTuple('ig_md.partition.partition_index', 3),
                                               gc.KeyTuple('$MATCH_PRIORITY', 1),
                                               gc.KeyTuple('hdr.ipv4.dst_addr', item.ip, item.mask)])])

        # send pkt and verify dropped
        for item in dst_ip_list:
            pkt = testutils.simple_tcp_packet(ip_dst=item.ip)
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_no_other_packets(self)


class TCAMMatchModifyTest(BfRuntimeTest):
    """@brief This test does the following
    1. Adds 100 match entries
    2. Sends packets to 100 match entries and verifies
    3. Modifies 100 match entries
    4. Sends packets to 100 modified entries and verifies
    5. Modifies just the direct register resource
    6. Reads back the direct register resource and verifies.
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def runTest(self):
        seed = setup_random()

        num_entries = 100
        ig_ports = [random.choice(swports) for x in range(num_entries)]
        all_ports = swports_0 + swports_1 + swports_2 + swports_3
        eg_ports = [random.choice(all_ports) for x in range(num_entries)]

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        tcam_table = bfrt_info.table_get("SwitchIngress.tcam_table")
        self.tcam_table = tcam_table
        tcam_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
        tcam_table.info.key_field_annotation_add("hdr.ipv4.src_addr", "ipv4")
        tcam_table.info.data_field_annotation_add("srcMac", "SwitchIngress.change_smac", "mac")
        tcam_table.info.data_field_annotation_add("dstMac", "SwitchIngress.change_dmac", "mac")

        target = gc.Target(device_id=0, pipe_id=0xffff)
        ipdst_dict = {}
        ipsrc_dict = {}
        ipDstAddrs = []
        ipSrcAddrs = []
        ipDstAddrsMask = []
        ipSrcAddrsMask = []
        priorities = [x for x in range(num_entries)]
        random.shuffle(priorities)
        action_choices = ['SwitchIngress.change_smac', 'SwitchIngress.change_dmac']
        action = [action_choices[random.randint(0, 1)] for x in range(num_entries)]

        srcMacAddrs = ["%02x:%02x:%02x:%02x:%02x:%02x" % (
            random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
            random.randint(0, 255), random.randint(0, 255)) for x in range(num_entries)]
        dstMacAddrs = ["%02x:%02x:%02x:%02x:%02x:%02x" % (
            random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
            random.randint(0, 255), random.randint(0, 255)) for x in range(num_entries)]

        register_value_hi = [random.randint(1, 10000) for x in range(num_entries)]
        register_value_lo = [random.randint(1, 10000) for x in range(num_entries)]

        ip_tuple_list = self.generate_random_ip_list(num_entries*2, seed)
        for i in range(num_entries):
            ipDstAddrs.append(ip_tuple_list[i].ip)
            ipSrcAddrs.append(ip_tuple_list[i+num_entries].ip)
            ipDstAddrsMask.append(ip_tuple_list[i].mask)
            ipSrcAddrsMask.append(ip_tuple_list[i+num_entries].mask)

        logger.info("Adding %d entries to SwitchIngress.tcam_table table", num_entries)

        for x in range(num_entries):
            if action[x] == 'SwitchIngress.change_smac':
                tcam_table.entry_add(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                           gc.DataTuple('srcMac', srcMacAddrs[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_smac')])

            elif action[x] == 'SwitchIngress.change_dmac':
                tcam_table.entry_add(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                           gc.DataTuple('dstMac', dstMacAddrs[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_dmac')])

        logger.info("DONE Adding %d entries to SwitchIngress.tcam_table table", num_entries)

        logger.info("Sending packets to all %d entries of SwitchIngress.tcam_table table", num_entries)
        for x in range(num_entries):
            pkt = testutils.simple_tcp_packet(ip_dst=ipDstAddrs[x],
                                              ip_src=ipSrcAddrs[x],
                                              with_tcp_chksum=False)
            if action[x] == 'SwitchIngress.change_smac':
                exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                      ip_dst=ipDstAddrs[x],
                                                      ip_src=ipSrcAddrs[x],
                                                      with_tcp_chksum=False)
            elif action[x] == 'SwitchIngress.change_dmac':
                exp_pkt = testutils.simple_tcp_packet(eth_dst=dstMacAddrs[x],
                                                      ip_dst=ipDstAddrs[x],
                                                      ip_src=ipSrcAddrs[x],
                                                      with_tcp_chksum=False)

            testutils.send_packet(self, ig_ports[x], pkt)
            testutils.verify_packet(self, exp_pkt, eg_ports[x])

        testutils.verify_no_other_packets(self, timeout=2)
        logger.info("DONE Sending packets to all %d entries of SwitchIngress.tcam_table table", num_entries)

        # Shuffle around the action data and the register values for a modify
        random.shuffle(srcMacAddrs)
        random.shuffle(dstMacAddrs)
        random.shuffle(action)

        random.shuffle(register_value_hi)
        random.shuffle(register_value_lo)

        logger.info("Modifying %d entries of SwitchIngress.tcam_table table", num_entries)

        for x in range(num_entries):
            if action[x] == 'SwitchIngress.change_smac':
                tcam_table.entry_mod(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                           gc.DataTuple('srcMac', srcMacAddrs[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_smac')])

            elif action[x] == 'SwitchIngress.change_dmac':
                tcam_table.entry_mod(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                           gc.DataTuple('dstMac', dstMacAddrs[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_dmac')])

        logger.info("DONE Modifying %d entries of SwitchIngress.tcam_table table", num_entries)

        logger.info("Sending packets to all %d modified entries of SwitchIngress.tcam_table table", num_entries)

        for x in range(num_entries):
            pkt = testutils.simple_tcp_packet(ip_dst=ipDstAddrs[x],
                                              ip_src=ipSrcAddrs[x],
                                              with_tcp_chksum=False)
            if action[x] == 'SwitchIngress.change_smac':
                exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                      ip_dst=ipDstAddrs[x],
                                                      ip_src=ipSrcAddrs[x],
                                                      with_tcp_chksum=False)
            elif action[x] == 'SwitchIngress.change_dmac':
                exp_pkt = testutils.simple_tcp_packet(eth_dst=dstMacAddrs[x],
                                                      ip_dst=ipDstAddrs[x],
                                                      ip_src=ipSrcAddrs[x],
                                                      with_tcp_chksum=False)

            testutils.send_packet(self, ig_ports[x], pkt)
            testutils.verify_packet(self, exp_pkt, eg_ports[x])

        testutils.verify_no_other_packets(self, timeout=2)

        logger.info("DONE Sending packets to all %d modified entries of SwitchIngress.tcam_table table",
                    num_entries)

        logger.info("Modifying direct register for %d entries of SwitchIngress.tcam_table table", num_entries)

        for x in range(num_entries):
            if action[x] == 'SwitchIngress.change_smac':
                tcam_table.entry_mod(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_smac')])

            elif action[x] == 'SwitchIngress.change_dmac':
                tcam_table.entry_mod(
                    target,
                    [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                          gc.KeyTuple('hdr.ipv4.dst_addr',
                                                      ipDstAddrs[x],
                                                      ipDstAddrsMask[x]),
                                          gc.KeyTuple('hdr.ipv4.src_addr',
                                                      ipSrcAddrs[x],
                                                      ipSrcAddrsMask[x])])],
                    [tcam_table.make_data([gc.DataTuple('SwitchIngress.direct_reg.first', register_value_hi[x]),
                                           gc.DataTuple('SwitchIngress.direct_reg.second', register_value_lo[x])],
                                          'SwitchIngress.change_dmac')])

        logger.info("DONE Modifying direct register for %d entries of SwitchIngress.tcam_table table", num_entries)

        logger.info("Reading direct register from hardware for %d entries of SwitchIngress.tcam_table table", num_entries)

        for x in range(num_entries):
            resp = tcam_table.entry_get(
                target,
                [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                      gc.KeyTuple('hdr.ipv4.dst_addr',
                                                  ipDstAddrs[x],
                                                  ipDstAddrsMask[x]),
                                      gc.KeyTuple('hdr.ipv4.src_addr',
                                                  ipSrcAddrs[x],
                                                  ipSrcAddrsMask[x])])],
                {"from_hw": True},
                tcam_table.make_data([gc.DataTuple("SwitchIngress.direct_reg.first"),
                                      gc.DataTuple("SwitchIngress.direct_reg.second")],
                                     None, get=True))

            fields = next(resp)[0].to_dict()
            assert [register_value_hi[x]] * num_pipes == fields["SwitchIngress.direct_reg.first"]
            assert [register_value_lo[x]] * num_pipes == fields["SwitchIngress.direct_reg.second"]

        logger.info("-ve tests for entry get")
        #negative wildcard read tests
        #1. get for an entry using wrong action should fail
        #2. wildcard read using an action should only get back some entries
        for x in range(num_entries):
            act_to_query = ""
            if action[x] == "SwitchIngress.change_smac":
                act_to_query = "SwitchIngress.change_dmac"
            else:
                act_to_query = "SwitchIngress.change_smac"

            resp = tcam_table.entry_get(
                target,
                [tcam_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                      gc.KeyTuple('hdr.ipv4.dst_addr',
                                                  ipDstAddrs[x],
                                                  ipDstAddrsMask[x]),
                                      gc.KeyTuple('hdr.ipv4.src_addr',
                                                  ipSrcAddrs[x],
                                                  ipSrcAddrsMask[x])])],
                {"from_hw": True},
                tcam_table.make_data([gc.DataTuple("SwitchIngress.direct_reg.first"),
                                      gc.DataTuple("SwitchIngress.direct_reg.second")],
                                     act_to_query, get=True))
            error_recvd = False
            try:
                fields = next(resp)[0].to_dict()
            except gc.BfruntimeRpcException as e:
                error_recvd = True
            assert error_recvd, "Expecting error as part of -ve test of filter get with action"

        # Select opposite of the action of the first entry so that entryGetFirst itself fails
        act_to_query = ""
        if action[0] == "SwitchIngress.change_smac":
            act_to_query = "SwitchIngress.change_dmac"
        else:
            act_to_query = "SwitchIngress.change_smac"
        num_data_for_act = action.count(act_to_query)
        resp = tcam_table.entry_get(target, None, {"from_hw": True}, 
                tcam_table.make_data([gc.DataTuple("SwitchIngress.direct_reg.first"),
                                      gc.DataTuple("SwitchIngress.direct_reg.second")],
                                     act_to_query, get=True))
        recv_count = 0
        for data, key in resp:
            recv_count += 1
        assert recv_count == num_data_for_act, "Expecting %d number of data objects, received %d" \
                % (recv_count, num_data_for_act)

        logger.info("Deleting %d entries of SwitchIngress.tcam_table table", num_entries)
        tcam_table.entry_del(target)
        logger.info("DONE Deleting %d entries of SwitchIngress.tcam_table table", num_entries)


class TCAMMatchIndirectModifyTest(BfRuntimeTest):
    """@brief This test does the following
    1. Adds 100 action profile entries
    2. Adds 100 match entries pointing ot action member IDs
    3. Sends packets to 100 match entries and verifies
    4. Modifies 100 action profile entries
    5. Modifies 100 match entries to point to different action member ids
    6. Sends packets to 100 modified entries and verifies
    7. Modifies just the direct LPF spec
    8. Reads back the direct LPF spec and verifies.
    """

    def setUp(self):
        client_id = 0
        p4_name = "tna_ternary_match"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def macAddrtoint(self, macAddr):
        a = macAddr.split(":")
        a = "".join(a)
        return int(a, 16)

    def findHit(self, num_entries, x, MacAddrs, MacAddrsMask, priorities):
        pkt_mac = self.macAddrtoint(MacAddrs[x])
        hit_index = x
        for y in range(num_entries):
            if priorities[y] < priorities[hit_index]:
                mac = self.macAddrtoint(MacAddrs[y])
                mask = self.macAddrtoint(MacAddrsMask[y])
                if (pkt_mac & mask) == (mac & mask):
                    hit_index = y
        return hit_index

    def runTest(self):
        seed = setup_random()

        num_entries = 100
        ig_ports = [random.choice(swports) for x in range(num_entries)]
        all_ports = swports_0 + swports_1 + swports_2 + swports_3
        eg_ports = [random.choice(all_ports) for x in range(num_entries)]

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_ternary_match")

        action_profile_table = bfrt_info.table_get("SwitchIngress.action_profile")
        tcam_direct_lpf_table = bfrt_info.table_get("SwitchIngress.tcam_direct_lpf")

        action_profile_table.info.data_field_annotation_add("srcAddr", "SwitchIngress.change_ipsrc", "ipv4")
        action_profile_table.info.data_field_annotation_add("dstAddr", "SwitchIngress.change_ipdst", "ipv4")
        tcam_direct_lpf_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
        tcam_direct_lpf_table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")

        target = gc.Target(device_id=0, pipe_id=0xffff)
        srcMac_dict = {}
        dstMac_dict = {}
        srcMacAddrs = []
        dstMacAddrs = []
        srcMacAddrsMask = []
        dstMacAddrsMask = []
        priorities = [x for x in range(num_entries)]
        random.shuffle(priorities)

        action_choices = ['SwitchIngress.change_ipsrc', 'SwitchIngress.change_ipdst']
        action = [action_choices[random.randint(0, 1)] for x in range(num_entries)]

        action_mbr_ids = [x + 1 for x in range(num_entries)]

        ipDstAddrs = ["%d.%d.%d.%d" % (
            random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
            range(num_entries)]
        ipSrcAddrs = ["%d.%d.%d.%d" % (
            random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
            range(num_entries)]

        lpf_types = [random.choice(["RATE", "SAMPLE"]) for x in range(num_entries)]

        gain_time = [round(random.uniform(1, 1000), 2) for x in range(num_entries)]
        decay_time = gain_time
        out_scale = [random.randint(1, 31) for x in range(num_entries)]

        srcMacAddrtuple = self.generate_random_mac_list(num_entries, seed)
        dstMacAddrtuple = self.generate_random_mac_list(num_entries, seed)

        srcMacAddrs = [getattr(each, "mac") for each in srcMacAddrtuple]
        srcMacAddrsMask = [getattr(each, "mask") for each in srcMacAddrtuple]

        dstMacAddrs = [getattr(each, "mac") for each in dstMacAddrtuple]
        dstMacAddrsMask = [getattr(each, "mask") for each in dstMacAddrtuple]

        random.shuffle(action_mbr_ids)

        logger.info("Adding %d entries to SwitchIngress.action_profile table", num_entries)

        try:
            for x in range(num_entries):
                if action[x] == 'SwitchIngress.change_ipsrc':
                    action_profile_table.entry_add(
                        target,
                        [action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', action_mbr_ids[x])])],
                        [action_profile_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                                         gc.DataTuple('srcAddr', ipSrcAddrs[x])],
                                                        'SwitchIngress.change_ipsrc')])
                elif action[x] == 'SwitchIngress.change_ipdst':
                    action_profile_table.entry_add(
                        target,
                        [action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', action_mbr_ids[x])])],
                        [action_profile_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                                         gc.DataTuple('dstAddr', ipDstAddrs[x])],
                                                        'SwitchIngress.change_ipdst')])

            logger.info("DONE Adding %d entries to SwitchIngress.action_profile table", num_entries)

            logger.info("Adding %d entries to SwitchIngress.tcam_direct_lpf table", num_entries)

            for x in range(num_entries):
                tcam_direct_lpf_table.entry_add(
                    target,
                    [tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                                     gc.KeyTuple('hdr.ethernet.dst_addr',
                                                                 dstMacAddrs[x],
                                                                 dstMacAddrsMask[x]),
                                                     gc.KeyTuple('hdr.ethernet.src_addr',
                                                                 srcMacAddrs[x],
                                                                 srcMacAddrsMask[x])])],
                    [tcam_direct_lpf_table.make_data([gc.DataTuple('$ACTION_MEMBER_ID', action_mbr_ids[x]),
                                                      gc.DataTuple('$LPF_SPEC_TYPE', str_val=lpf_types[x]),
                                                      gc.DataTuple('$LPF_SPEC_GAIN_TIME_CONSTANT_NS',
                                                                   float_val=gain_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_DECAY_TIME_CONSTANT_NS',
                                                                   float_val=decay_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_OUT_SCALE_DOWN_FACTOR', out_scale[x])])])

            logger.info("DONE Adding %d entries to SwitchIngress.tcam_direct_lpf table", num_entries)

            logger.info("Sending packets to all %d entries of SwitchIngress.tcam_table table", num_entries)
            for x in range(num_entries):
                pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                  eth_dst=dstMacAddrs[x],
                                                  with_tcp_chksum=False)
                y = self.findHit(num_entries, x, dstMacAddrs, dstMacAddrsMask, priorities)
                if action[y] == 'SwitchIngress.change_ipsrc':
                    exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                          eth_dst=dstMacAddrs[x],
                                                          ip_src=ipSrcAddrs[y],
                                                          with_tcp_chksum=False)
                elif action[y] == 'SwitchIngress.change_ipdst':
                    exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                          eth_dst=dstMacAddrs[x],
                                                          ip_dst=ipDstAddrs[y],
                                                          with_tcp_chksum=False)

                testutils.send_packet(self, ig_ports[x], pkt)
                testutils.verify_packet(self, exp_pkt, eg_ports[y])

            testutils.verify_no_other_packets(self, timeout=2)

            logger.info("DONE Sending packets to all %d entries of SwitchIngress.tcam_table table", num_entries)

            # Shuffle around the action data values for a modify
            random.shuffle(ipSrcAddrs)
            random.shuffle(ipDstAddrs)
            random.shuffle(action)
            random.shuffle(action_mbr_ids)

            logger.info("Modifying %d entries of SwitchIngress.action_profile table", num_entries)
            for x in range(num_entries):
                if action[x] == 'SwitchIngress.change_ipsrc':
                    action_profile_table.entry_mod(
                        target,
                        [action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', action_mbr_ids[x])])],
                        [action_profile_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                                         gc.DataTuple('srcAddr', ipSrcAddrs[x])],
                                                        'SwitchIngress.change_ipsrc')])

                elif action[x] == 'SwitchIngress.change_ipdst':
                    action_profile_table.entry_mod(
                        target,
                        [action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', action_mbr_ids[x])])],
                        [action_profile_table.make_data([gc.DataTuple('dst_port', eg_ports[x]),
                                                         gc.DataTuple('dstAddr', ipDstAddrs[x])],
                                                        'SwitchIngress.change_ipdst')])
            logger.info("DONE Modifying %d entries of SwitchIngress.action_profile table", num_entries)

            logger.info("Modifying %d entries of SwitchIngress.tcam_direct_lpf table", num_entries)
            for x in range(num_entries):
                tcam_direct_lpf_table.entry_mod(
                    target,
                    [tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                                     gc.KeyTuple('hdr.ethernet.dst_addr',
                                                                 dstMacAddrs[x],
                                                                 dstMacAddrsMask[x]),
                                                     gc.KeyTuple('hdr.ethernet.src_addr',
                                                                 srcMacAddrs[x],
                                                                 srcMacAddrsMask[x])])],
                    [tcam_direct_lpf_table.make_data([gc.DataTuple('$ACTION_MEMBER_ID', action_mbr_ids[x]),
                                                      gc.DataTuple('$LPF_SPEC_TYPE', str_val=lpf_types[x]),
                                                      gc.DataTuple('$LPF_SPEC_GAIN_TIME_CONSTANT_NS',
                                                                   float_val=gain_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_DECAY_TIME_CONSTANT_NS',
                                                                   float_val=decay_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_OUT_SCALE_DOWN_FACTOR', out_scale[x])])])

            logger.info("Sending packets to all %d MODIFIED entries of SwitchIngress.tcam_table table", num_entries)

            for x in range(num_entries):
                pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                  eth_dst=dstMacAddrs[x],
                                                  with_tcp_chksum=False)
                y = self.findHit(num_entries, x, dstMacAddrs, dstMacAddrsMask, priorities)
                if action[y] == 'SwitchIngress.change_ipsrc':
                    exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                          eth_dst=dstMacAddrs[x],
                                                          ip_src=ipSrcAddrs[y],
                                                          with_tcp_chksum=False)
                elif action[y] == 'SwitchIngress.change_ipdst':
                    exp_pkt = testutils.simple_tcp_packet(eth_src=srcMacAddrs[x],
                                                          eth_dst=dstMacAddrs[x],
                                                          ip_dst=ipDstAddrs[y],
                                                          with_tcp_chksum=False)

                testutils.send_packet(self, ig_ports[x], pkt)
                testutils.verify_packet(self, exp_pkt, eg_ports[y])

            testutils.verify_no_other_packets(self, timeout=2)

            logger.info("DONE Sending packets to all %d MODIFIED entries of SwitchIngress.tcam_table table",
                        num_entries)

            # Now shuffle around LPF data to do modify
            random.shuffle(lpf_types)
            random.shuffle(gain_time)
            decay_time = gain_time
            random.shuffle(out_scale)

            logger.info("Modifying direct LPF SPEC of %d entries of SwitchIngress.tcam_direct_lpf table", num_entries)

            for x in range(num_entries):
                tcam_direct_lpf_table.entry_mod(
                    target,
                    [tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                                     gc.KeyTuple('hdr.ethernet.dst_addr',
                                                                 dstMacAddrs[x],
                                                                 dstMacAddrsMask[x]),
                                                     gc.KeyTuple('hdr.ethernet.src_addr',
                                                                 srcMacAddrs[x],
                                                                 srcMacAddrsMask[x])])],
                    [tcam_direct_lpf_table.make_data([gc.DataTuple('$LPF_SPEC_TYPE', str_val=lpf_types[x]),
                                                      gc.DataTuple('$LPF_SPEC_GAIN_TIME_CONSTANT_NS',
                                                                   float_val=gain_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_DECAY_TIME_CONSTANT_NS',
                                                                   float_val=decay_time[x]),
                                                      gc.DataTuple('$LPF_SPEC_OUT_SCALE_DOWN_FACTOR', out_scale[x])])])

            logger.info("DONE Modifying direct LPF SPEC of %d entries of SwitchIngress.tcam_direct_lpf table",
                        num_entries)

            logger.info("Reading direct LPF spec from hardware for %d entries of SwitchIngress.tcam_direct_lpf table",
                        num_entries)

            for x in range(num_entries):
                resp = tcam_direct_lpf_table.entry_get(
                    target,
                    [tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                                     gc.KeyTuple('hdr.ethernet.dst_addr',
                                                                 dstMacAddrs[x],
                                                                 dstMacAddrsMask[x]),
                                                     gc.KeyTuple('hdr.ethernet.src_addr',
                                                                 srcMacAddrs[x],
                                                                 srcMacAddrsMask[x])])],
                    {"from_hw": True})

                fields = next(resp)[0].to_dict()
                assert fields["$LPF_SPEC_TYPE"] == lpf_types[x]
                assert abs(fields["$LPF_SPEC_GAIN_TIME_CONSTANT_NS"] - gain_time[x]) <= gain_time[x] * 0.02
                assert abs(fields["$LPF_SPEC_DECAY_TIME_CONSTANT_NS"] - decay_time[x]) <= decay_time[x] * 0.02
                assert fields["$LPF_SPEC_OUT_SCALE_DOWN_FACTOR"] == out_scale[x]

            logger.info("DONE Reading direct LPF spec from hardware for %d entries of SwitchIngress.tcam_table table",
                        num_entries)

        finally:
            logger.info("Deleting %d entries of SwitchIngress.tcam_direct_lpf table", num_entries)
            try:
                for x in range(num_entries):
                    tcam_direct_lpf_table.entry_del(
                        target,
                        [tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', priorities[x]),
                                                         gc.KeyTuple('hdr.ethernet.dst_addr',
                                                                     dstMacAddrs[x],
                                                                     dstMacAddrsMask[x]),
                                                         gc.KeyTuple('hdr.ethernet.src_addr',
                                                                     srcMacAddrs[x],
                                                                     srcMacAddrsMask[x])])])
            except gc.BfruntimeRpcException as e:
                raise e
            try:
                for x in range(num_entries):
                    action_profile_table.entry_del(
                        target,
                        [action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', action_mbr_ids[x])])])
            except gc.BfruntimeRpcException as e:
                raise e
