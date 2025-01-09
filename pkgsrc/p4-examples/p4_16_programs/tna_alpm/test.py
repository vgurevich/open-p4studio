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

import binascii
import logging
import random
import six
import os
import socket
import ipaddress

from ptf import config
from collections import namedtuple
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as gc
import grpc

this_dir = os.path.dirname(os.path.abspath(__file__))
client_id = 0
p4_name = "tna_alpm"

logger = get_logger()
swports = get_sw_ports()

class IPv4SubsetKeyTest(BfRuntimeTest):
    """@brief Basic IPv4 test for ALPM ATCAM subset key width optimization feature.
    """

    def setUp(self):
        BfRuntimeTest.setUp(self, client_id, p4_name)
        setup_random()

    def tearDown(self):
        bfrt_info = self.interface.bfrt_info_get(p4_name)
        tbl = bfrt_info.table_get("SwitchIngress.ipv4_subset_key")
        target = gc.Target(device_id=0)
        tbl.entry_del(target, [])
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        ig_port = swports[1]
        seed = random.randint(1, 65535)
        num_entries = random.randint(1, 30)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_alpm")

        alpm_forward_table = bfrt_info.table_get("SwitchIngress.ipv4_subset_key")
        alpm_forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        key_random_tuple = namedtuple('key_random', 'vrf dst_ip prefix_len')
        data_random_tuple = namedtuple('data_random', 'eg_port')
        key_tuple_list = []
        data_tuple_list = []
        alpm_dict= {}
        target = gc.Target(device_id=0, pipe_id=0xffff)

        logger.info("IPv4SubsetKey: Installing %d ALPM entries" % (num_entries))
        ip_list = self.generate_random_ip_list(num_entries, seed)
        for i in range(0, num_entries):
            vrf = 0
            dst_ip = getattr(ip_list[i], "ip")
            p_len = getattr(ip_list[i], "prefix_len")

            eg_port = random.choice(swports)

            key_tuple_list.append(key_random_tuple(vrf, dst_ip, p_len))
            data_tuple_list.append(data_random_tuple(eg_port))

            key = alpm_forward_table.make_key([gc.KeyTuple('vrf', vrf),
                                              gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip, prefix_len=p_len)])
            data = alpm_forward_table.make_data([gc.DataTuple('dst_port', eg_port)],
                                              'SwitchIngress.route')
            alpm_forward_table.entry_add(target, [key], [data])
            key.apply_mask()
            alpm_dict[key] = data

        # check get
        resp  = alpm_forward_table.entry_get(target)
        for data, key in resp:
            assert alpm_dict[key] == data
            alpm_dict.pop(key)
        assert len(alpm_dict) == 0

        test_tuple_list = list(zip(key_tuple_list, data_tuple_list))

        logger.info("IPv4SubsetKey: Sending packets for the installed entries to verify")
        # send pkt and verify sent
        for key_item, data_item in test_tuple_list:
            pkt = testutils.simple_tcp_packet(ip_dst=key_item.dst_ip)
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, pkt, data_item.eg_port)

        logger.info("IPv4SubsetKey: All expected packets received")
        logger.info("IPv4SubsetKey: Deleting %d ALPM entries" % (num_entries))

        # Delete table entries
        for item in key_tuple_list:
            alpm_forward_table.entry_del(
                target,
                [alpm_forward_table.make_key([gc.KeyTuple('vrf', item.vrf),
                                              gc.KeyTuple('hdr.ipv4.dst_addr', item.dst_ip,
                                                          prefix_len=item.prefix_len)])])

class IPv6SubsetKeyTest(BfRuntimeTest):
    """@brief Basic IPv6 test for ALPM ATCAM subset key width optimization feature.
    """

    def setUp(self):
        BfRuntimeTest.setUp(self, client_id, p4_name)
        setup_random()

    def tearDown(self):
        bfrt_info = self.interface.bfrt_info_get(p4_name)
        tbl = bfrt_info.table_get("SwitchIngress.ipv6_subset_key")
        target = gc.Target(device_id=0)
        tbl.entry_del(target, [])
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        ig_port = swports[1]
        num_entries = random.randint(1, 30)

        f = open(this_dir+'/ipv6_route_table.txt')
        lines = f.readlines()
        num_lines = random.sample(range(0, 109000), num_entries)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_alpm")

        alpm_forward_table = bfrt_info.table_get("SwitchIngress.ipv6_subset_key")
        alpm_forward_table.info.key_field_annotation_add("hdr.ipv6.dst_addr[127:64]", "ipv6")

        key_random_tuple = namedtuple('key_random', 'dst_ip dst_ip_str prefix_len')
        data_random_tuple = namedtuple('data_random', 'eg_port')
        key_tuple_list = []
        data_tuple_list = []
        alpm_dict= {}
        routes_by_prefix = [list() for _ in range(129)]
        target = gc.Target(device_id=0, pipe_id=0xffff)

        logger.info("IPv6SubsetKey: Installing %d ALPM entries" % (num_entries))
        count = 0
        for i in num_lines:
            eg_port = random.choice(swports)
            vrf = 0
            dst_ip_str = lines[i].split("/")[0]
            p_len = int(lines[i].split("/")[1])
            route = ipaddress.ip_network(lines[i].strip())
            routes_by_prefix[route.prefixlen].append((route,eg_port))

            dst_ip = int(binascii.hexlify(six.ensure_binary(socket.inet_pton(socket.AF_INET6, dst_ip_str))), 16)
            dst_ip = dst_ip >> 64 # use only msb bits


            key_tuple_list.append(key_random_tuple(dst_ip, dst_ip_str, p_len))
            data_tuple_list.append(data_random_tuple(eg_port))

            key = alpm_forward_table.make_key([gc.KeyTuple('vrf', vrf),
                                              gc.KeyTuple('hdr.ipv6.dst_addr[127:64]', dst_ip, prefix_len=p_len)])
            data = alpm_forward_table.make_data([gc.DataTuple('dst_port', eg_port)],
                                              'SwitchIngress.route')
            alpm_forward_table.entry_add(target, [key], [data])
            key.apply_mask()
            alpm_dict[key] = data
            count = count + 1
        routes_by_prefix.reverse()

        # check get
        resp  = alpm_forward_table.entry_get(target)
        for data, key in resp:
            assert alpm_dict[key] == data
            alpm_dict.pop(key)
        assert len(alpm_dict) == 0

        test_tuple_list = list(zip(key_tuple_list, data_tuple_list))

        logger.info("IPv6SubsetKey: Sending packets for the installed entries to verify")
        # send pkt and verify sent
        for key_item, data_item in test_tuple_list:
            pkt = testutils.simple_tcpv6_packet(ipv6_dst=key_item.dst_ip_str)
            testutils.send_packet(self, ig_port, pkt)
            # Find the expected egress port for the packet by matching the IP
            # against the routes we programmed.
            eg_port = None
            dest = ipaddress.ip_address(key_item.dst_ip_str)
            for one_prefix in routes_by_prefix:
                for r,p in one_prefix:
                    if r.overlaps(ipaddress.ip_network(key_item.dst_ip_str + '/128')):
                        eg_port = p
                        break
                if eg_port: break
            testutils.verify_packet(self, pkt, eg_port)

        logger.info("IPv6SubsetKey: All expected packets received")
        logger.info("IPv6SubsetKey: Deleting %d ALPM entries" % (num_entries))

        # Delete table entries
        for item in key_tuple_list:
            alpm_forward_table.entry_del(
                target,
                [alpm_forward_table.make_key([gc.KeyTuple('vrf', 0),
                                              gc.KeyTuple('hdr.ipv6.dst_addr[127:64]', item.dst_ip,
                                                          prefix_len=item.prefix_len)])])

class IPv4ExcludeMSBTest(BfRuntimeTest):
    """@brief Basic IPv4 test for ALPM ATCAM exclude MSB bits optimization feature.
    """

    def setUp(self):
        BfRuntimeTest.setUp(self, client_id, p4_name)
        setup_random()

    def tearDown(self):
        bfrt_info = self.interface.bfrt_info_get(p4_name)
        tbl = bfrt_info.table_get("SwitchIngress.ipv4_exclude_msb")
        target = gc.Target(device_id=0)
        tbl.entry_del(target, [])
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        ig_port = swports[1]
        seed = random.randint(1, 65535)
        num_entries = random.randint(1, 30)

        target = gc.Target(device_id=0, pipe_id=0xffff)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_alpm")

        alpm_forward_table = bfrt_info.table_get("SwitchIngress.ipv4_exclude_msb")
        alpm_forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        key_random_tuple = namedtuple('key_random', 'vrf dst_ip prefix_len')
        data_random_tuple = namedtuple('data_random', 'eg_port')
        key_tuple_list = []
        data_tuple_list = []
        alpm_dict= {}

        logger.info("IPv4ExcludeMSB: Installing %d ALPM entries" % (num_entries))
        ip_list = self.generate_random_ip_list(num_entries, seed)
        for i in range(0, num_entries):
            vrf = 0
            dst_ip = getattr(ip_list[i], "ip")
            p_len = getattr(ip_list[i], "prefix_len")

            eg_port = random.choice(swports)

            key_tuple_list.append(key_random_tuple(vrf, dst_ip, p_len))
            data_tuple_list.append(data_random_tuple(eg_port))

            key = alpm_forward_table.make_key([gc.KeyTuple('vrf', vrf),
                                              gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip, prefix_len=p_len)])
            data = alpm_forward_table.make_data([gc.DataTuple('dst_port', eg_port)],
                                              'SwitchIngress.route')
            alpm_forward_table.entry_add(target, [key], [data])
            key.apply_mask()
            alpm_dict[key] = data

        # check get
        resp  = alpm_forward_table.entry_get(target)
        for data, key in resp:
            assert alpm_dict[key] == data
            alpm_dict.pop(key)
        assert len(alpm_dict) == 0

        test_tuple_list = list(zip(key_tuple_list, data_tuple_list))

        logger.info("IPv4ExcludeMSB: Sending packets for the installed entries to verify")
        # send pkt and verify sent
        for key_item, data_item in test_tuple_list:
            pkt = testutils.simple_tcp_packet(ip_dst=key_item.dst_ip)
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, pkt, data_item.eg_port)

        logger.info("IPv4ExcludeMSB: All expected packets received")
        logger.info("IPv4ExcludeMSB: Deleting %d ALPM entries" % (num_entries))

        # Delete table entries
        for item in key_tuple_list:
            alpm_forward_table.entry_del(
                target,
                [alpm_forward_table.make_key([gc.KeyTuple('vrf', item.vrf),
                                              gc.KeyTuple('hdr.ipv4.dst_addr', item.dst_ip,
                                                          prefix_len=item.prefix_len)])])

class IPv6ExcludeMSBTest(BfRuntimeTest):
    """@brief Basic IPv6 test for ALPM ATCAM exclude MSB bits optimization feature.
    """

    def setUp(self):
        BfRuntimeTest.setUp(self, client_id, p4_name)
        setup_random()

    def tearDown(self):
        bfrt_info = self.interface.bfrt_info_get(p4_name)
        tbl = bfrt_info.table_get("SwitchIngress.ipv6_exclude_msb")
        target = gc.Target(device_id=0)
        tbl.entry_del(target, [])
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        ig_port = swports[1]
        num_entries = random.randint(1, 30)

        f = open(this_dir+'/ipv6_route_table.txt')
        lines = f.readlines()
        num_lines = random.sample(range(0, 109000), num_entries)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_alpm")

        alpm_forward_table = bfrt_info.table_get("SwitchIngress.ipv6_exclude_msb")
        alpm_forward_table.info.key_field_annotation_add("hdr.ipv6.dst_addr", "ipv6")

        key_random_tuple = namedtuple('key_random', 'dst_ip dst_ip_str prefix_len')
        data_random_tuple = namedtuple('data_random', 'eg_port')
        key_tuple_list = []
        data_tuple_list = []
        alpm_dict= {}
        routes_by_prefix = [list() for _ in range(129)]
        target = gc.Target(device_id=0, pipe_id=0xffff)

        logger.info("IPv6ExcludeMSB: Installing %d ALPM entries" % (num_entries))
        count = 0
        for i in num_lines:
            eg_port = random.choice(swports)
            vrf = 0
            dst_ip_str = lines[i].split("/")[0]
            p_len = int(lines[i].split("/")[1])
            route = ipaddress.ip_network(lines[i].strip())
            routes_by_prefix[route.prefixlen].append((route,eg_port))

            dst_ip = int(binascii.hexlify(six.ensure_binary(socket.inet_pton(socket.AF_INET6, dst_ip_str))), 16)

            key_tuple_list.append(key_random_tuple(dst_ip, dst_ip_str, p_len))
            data_tuple_list.append(data_random_tuple(eg_port))

            key = alpm_forward_table.make_key([gc.KeyTuple('vrf', vrf),
                                              gc.KeyTuple('hdr.ipv6.dst_addr', dst_ip, prefix_len=p_len)])
            data = alpm_forward_table.make_data([gc.DataTuple('dst_port', eg_port)],
                                              'SwitchIngress.route')
            alpm_forward_table.entry_add(target, [key], [data])
            key.apply_mask()
            alpm_dict[key] = data
            count = count + 1
        routes_by_prefix.reverse()

        # check get
        resp  = alpm_forward_table.entry_get(target)
        for data, key in resp:
            assert alpm_dict[key] == data
            alpm_dict.pop(key)
        assert len(alpm_dict) == 0

        test_tuple_list = list(zip(key_tuple_list, data_tuple_list))

        logger.info("IPv6ExcludeMSB: Sending packets for the installed entries to verify")
        # send pkt and verify sent
        for key_item, data_item in test_tuple_list:
            pkt = testutils.simple_tcpv6_packet(ipv6_dst=key_item.dst_ip_str)
            testutils.send_packet(self, ig_port, pkt)
            # Find the expected egress port for the packet by matching the IP
            # against the routes we programmed.
            eg_port = None
            dest = ipaddress.ip_address(key_item.dst_ip_str)
            for one_prefix in routes_by_prefix:
                for r,p in one_prefix:
                    if r.overlaps(ipaddress.ip_network(key_item.dst_ip_str + '/128')):
                        eg_port = p
                        break
                if eg_port: break
            testutils.verify_packet(self, pkt, eg_port)

        logger.info("IPv6ExcludeMSB: All expected packets received")
        logger.info("IPv6ExcludeMSB: Deleting %d ALPM entries" % (num_entries))

        # Delete table entries
        for item in key_tuple_list:
            alpm_forward_table.entry_del(
                target,
                [alpm_forward_table.make_key([gc.KeyTuple('vrf', 0),
                                              gc.KeyTuple('hdr.ipv6.dst_addr', item.dst_ip,
                                                          prefix_len=item.prefix_len)])])
