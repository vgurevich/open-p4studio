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

from ptf import config
from collections import namedtuple
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as client

logger = get_logger()
swports = get_sw_ports()


class RangeMatchTest(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id)
        setup_random()

    def runTest(self):
        ig_port = swports[1]
        eg_ports = [swports[5], swports[3]]

        num_entries = 5

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get()
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

        # Make sure the table starts off empty
        target = client.Target(device_id=0, pipe_id=0xFFFF)
        resp = forward_table.entry_get(target, None, {"from_hw": False})
        for data, key in resp:
            assert 0, "Shouldn't have hit here since table is supposed to be empty"

        key_random_tuple = namedtuple('key_random', 'dst_ip pkt_length_start pkt_length_end range_size')
        tuple_list = []
        for i in range(0, num_entries):
            vrf = 0
            range_size = random.randint(1, 511)
            dst_ip = "%d.%d.%d.%d" % (
                random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            pkt_length_start = random.randint(60, 511)
            tuple_list.append(key_random_tuple(dst_ip, pkt_length_start, pkt_length_start + range_size, range_size))
            forward_table.entry_add(
                target,
                [forward_table.make_key([client.KeyTuple('$MATCH_PRIORITY', 1),
                                         client.KeyTuple('hdr.ipv4.dst_addr', dst_ip),
                                         client.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)])],
                [forward_table.make_data([client.DataTuple('port', eg_ports[0])],
                                         'SwitchIngress.hit')]
            )

        for i, (dip, low, high, _) in enumerate(tuple_list):
            logger.info("Rule %d:  %s total_len %d to %d forwards to port %d", i, dip, low, high, eg_ports[0])

        # send pkt and verify sent
        for i, item in enumerate(tuple_list):
            # select a random length between the range
            eth_hdr_size = 14
            dst_ip, pkt_length_start, pkt_length_end, range_size = item[0], item[1], item[2], item[3]
            pkt_len = random.randint(pkt_length_start, pkt_length_end) + eth_hdr_size
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            exp_pkt = pkt
            logger.info("Sending packet on port %d for rule %d with total_len %d", ig_port, i, pkt_len - eth_hdr_size)
            testutils.send_packet(self, ig_port, pkt)

            logger.info("Expecting packet on port %d", eg_ports[0])
            testutils.verify_packet(self, exp_pkt, eg_ports[0])

        for i, item in enumerate(tuple_list):
            # select a length more than the range, it should be dropped
            pkt_len = pkt_length_end + eth_hdr_size + 2
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            exp_pkt = pkt
            logger.info("Sending packet on port %d to miss rule %d with total_len %d", ig_port, i,
                        pkt_len - eth_hdr_size)
            testutils.send_packet(self, ig_port, pkt)

            logger.info("Packet is expected to get dropped.")
        testutils.verify_no_other_packets(self)

        # check get
        for item in tuple_list:
            dst_ip, pkt_length_start, pkt_length_end, range_size = item[0], item[1], item[2], item[3]
            resp = forward_table.entry_get(
                target,
                [forward_table.make_key([client.KeyTuple('$MATCH_PRIORITY', 1),
                                         client.KeyTuple('hdr.ipv4.dst_addr',
                                                         dst_ip),
                                         client.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)])],
                {"from_hw": True})

            data_dict = next(resp)[0].to_dict()
            recv_port = data_dict["port"]
            if (recv_port != eg_ports[0]):
                logger.error("Error! port sent = %s received port = %s", str(eg_ports[0]), str(recv_port))
                assert 0

        # send pkt and verify sent
        for i, item in enumerate(tuple_list):
            eth_hdr_size = 14
            dst_ip, pkt_length_start, pkt_length_end, range_size = item[0], item[1], item[2], item[3]
            pkt_len = random.randint(pkt_length_start, pkt_length_end) + eth_hdr_size
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            exp_pkt = pkt
            logger.info("Sending packet on port %d for rule %d with total_len %d", ig_port, i, pkt_len - eth_hdr_size)
            testutils.send_packet(self, ig_port, pkt)

            logger.info("Expecting packet on port %d", eg_ports[0])
            testutils.verify_packet(self, exp_pkt, eg_ports[0])

            forward_table.entry_del(
                target,
                [forward_table.make_key([client.KeyTuple('$MATCH_PRIORITY', 1),
                                         client.KeyTuple('hdr.ipv4.dst_addr',
                                                         dst_ip),
                                         client.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)])])

        # send pkt and verify dropped
        for item in tuple_list:
            eth_hdr_size = 14
            dst_ip, pkt_length_start, pkt_length_end, range_size = item[0], item[1], item[2], item[3]
            pkt_len = random.randint(pkt_length_start, pkt_length_end) + eth_hdr_size
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            logger.info("Sending packet on port %d with total_len %d", ig_port, pkt_len - eth_hdr_size)
            testutils.send_packet(self, ig_port, pkt)

            logger.info("Packet is expected to get dropped.")
        testutils.verify_no_other_packets(self, timeout=2)
        forward_table.entry_del(target)


class RangeMatchWithErrorResponseTest(BfRuntimeTest):
    """@brief Add entries in forward_table, modify key and verify response for error.

        1. Add the entries to forward_table.
        2. Modify Key with random packet length.
        4. Get response with error_in_resp flag.
        5. Verify the error in response is equal to error added.
        6. delete entry with ignore_not_found flag set.
        7. Delete entries.
    """
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id)
        setup_random()

    def parse_response(self, resp, errors, num_entries, num_valid_entries):
        try:
            recieve_num = 0
            key_list_recv = []
            num_read = 0
            for _, key in resp:
                num_read += 1

            logger.info("Receive entries = %d", num_read)
            assert num_read == num_valid_entries

        except client.BfruntimeRpcException as e:
            error_list = e.sub_errors_get()
            for error in error_list:
                errors.append(error)
            logger.info("Received errors = %d", len(error_list))

    def runTest(self):

        ig_port = swports[1]
        eg_ports = [swports[5], swports[3]]
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get()
        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
        target = client.Target(device_id=0, pipe_id=0xFFFF)
        key_list = []
        data_list =[]
        #client metadata field to indicate server to send status in response
        client_metadata = [("error_in_resp", "1"), ("ignore_not_found", "1")]
        num_entries =  1
        num_valid_entries = num_entries
        # Populate key and data
        tuple_list = []
        key_random_tuple = namedtuple('key_random', 'dst_ip pkt_length_start pkt_length_end range_size')
        vrf = 0
        range_size = random.randint(1, 511)
        dst_ip = "%d.%d.%d.%d" % (
            random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
        pkt_length_start = random.randint(60, 511)
        tuple_list.append(key_random_tuple(dst_ip, pkt_length_start, pkt_length_start + range_size, range_size))
        key_list.append(forward_table.make_key([client.KeyTuple('$MATCH_PRIORITY', 1),
                                         client.KeyTuple('hdr.ipv4.dst_addr', dst_ip),
                                         client.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)]))

        data_list.append(forward_table.make_data([client.DataTuple('port', eg_ports[0])],
                                         'SwitchIngress.hit'))
        logger.info("Adding %d entries", num_entries)
        forward_table.entry_add(target, key_list, data_list*num_valid_entries)
        #modifying key
        pkt_length_start = random.randint(512, 600)

        key_list.append(forward_table.make_key([client.KeyTuple('$MATCH_PRIORITY', 1),
                                         client.KeyTuple('hdr.ipv4.dst_addr', dst_ip),
                                         client.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)]))


        logger.info("get %d entries with errors_in_response in metadata", num_entries)
        resp = forward_table.entry_get(target,
                                       key_list, {"from_hw": False}, metadata=client_metadata)
        num_valid_entries = num_entries
        error_response = []
        self.parse_response(resp, error_response, num_entries, num_valid_entries)
        assert len(error_response) == 1, "errors received is not same as expected."
        logger.info("PASS: RangeMatchWithErrorResponseTest pass with error in response check.")

        logger.info("Checking delete entry with ignore_not_found flag")
        forward_table.entry_del(target, key_list, metadata=client_metadata)
        # Delete all the entries
        forward_table.entry_del(target)
