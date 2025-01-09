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

from ptf import config
import ptf.testutils as testutils
from ptf.packet import *
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as gc

##### Required for Thrift #####
import pd_base_tests

##### ******************* #####

logger = get_logger()
swports = get_sw_ports()
pkt_len = int(test_param_get('pkt_size'))

def _add_entry(table, target, smac, smac_mask, priority, c_bytes, c_pkts, act):
    table.entry_add(
        target,
        [table.make_key(
            [gc.KeyTuple('hdr.ethernet.src_addr', smac, smac_mask),
             gc.KeyTuple('$MATCH_PRIORITY', priority)])],
        [table.make_data(
            [gc.DataTuple('$COUNTER_SPEC_BYTES', c_pkts),
             gc.DataTuple('$COUNTER_SPEC_PKTS', c_bytes)],
             act)])

def _add_entry2(table, target, smac, smac_mask, index, act):
    table.entry_add(
        target,
        [table.make_key
            ([gc.KeyTuple('hdr.ethernet.src_addr', smac, smac_mask)])],
        [table.make_data(
            [gc.DataTuple('index', index)], act)])

def _setup_match(target, table, act_name, smac, smac_mask):
    table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")

    _add_entry(table, target, smac, smac_mask, 0, 0, 0, act_name)

def _setup_match2(target, table, act_name, smac, smac_mask, index):
    table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")

    _add_entry2(table, target, smac, smac_mask, index, act_name)

def _read_counter(table, target, name, smac, smac_mask):
    resp = table.entry_get(target,
                [table.make_key([gc.KeyTuple('hdr.ethernet.src_addr', smac, smac_mask),
                                               gc.KeyTuple('$MATCH_PRIORITY', 0)])],
                      {"from_hw": True},
                    table.make_data(
                        [gc.DataTuple("$COUNTER_SPEC_BYTES"),
                         gc.DataTuple("$COUNTER_SPEC_PKTS")],
                        name, get=True)
                )

    # parse resp to get the counter
    data_dict = next(resp)[0].to_dict()
    recv_pkts = data_dict["$COUNTER_SPEC_PKTS"]
    recv_bytes = data_dict["$COUNTER_SPEC_BYTES"]
    return (recv_pkts, recv_bytes)

def _read_counter2(table, target, name, smac, smac_mask, index):
    resp = table.entry_get(target,
                [table.make_key([gc.KeyTuple('$COUNTER_INDEX', index)])],
                    {"from_hw": True}, None)

    # parse resp to get the counter
    data_dict = next(resp)[0].to_dict()
    recv_pkts = data_dict["$COUNTER_SPEC_PKTS"]
    recv_bytes = data_dict["$COUNTER_SPEC_BYTES"]
    return (recv_pkts, recv_bytes)

class TrueEgressAccountingTest(BfRuntimeTest):
    """@brief Simple test for true egress accounting.
       The same SMAC key is installed into table "count_src" and "count_src_teop",
       both of which count the number of bytes in the packet. The difference is
       that "count_src_teop" performs the packet truncation and its counter
       produces the true egress byte count.
    """

    def setUp(self):
        client_id = 0
        p4_name = "t2na_counter_true_egress_accounting"
        BfRuntimeTest.setUp(self, client_id, p4_name)

    def verify_teop_counts(self, table, action, target, smac,
            smac_mask, num_pkts, recv_pkts, recv_bytes, num_bytes,
            num_bytes_teop, counter_name, ctr_type, index):

        if ctr_type :
            recv_pkts_teop, recv_bytes_teop = _read_counter(table, target,
                action, smac, smac_mask)
        else :
            recv_pkts_teop, recv_bytes_teop = _read_counter2(table, target,
                action, smac, smac_mask, index)

        logger.info(" ")
        logger.info("verifying for counter = %s", counter_name)
        logger.info(" ")

        # Verify packet count
        if (num_pkts != recv_pkts or recv_pkts != recv_pkts_teop):
            logger.error("Error! packets sent = %d received count = %d", num_pkts, recv_pkts)
            assert 0;
        else:
            logger.info("packets received = %d", recv_pkts)

        # Verify byte count
        if (num_bytes != recv_bytes):
            logger.error("Error! bytes sent = %d received count = %d", num_bytes, recv_bytes)
            assert 0;
        else:
            logger.info("bytes received = %d", recv_bytes)

        # Verify byte count (true egress accounting)
        if (num_bytes_teop != recv_bytes_teop):
            logger.error("Error! bytes sent = %d received count = %d (true egress)", num_bytes_teop, recv_bytes_teop)
            assert 0;
        else:
            logger.info("bytes received = %d (true egress)", recv_bytes_teop)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        ig_port = swports[1]

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("t2na_counter_true_egress_accounting")

        smac = '11:33:55:77:99:00'
        smac_mask = 'ff:ff:ff:ff:ff:ff'
        dmac = '00:11:22:33:44:55'
        index = 0

        count_src = bfrt_info.table_get("SwitchEgress.count_src")
        count_src_teop = bfrt_info.table_get("SwitchEgress.count_src_teop")
        count_src_teop2 = bfrt_info.table_get("SwitchEgress.count_src_teop2")
        count_src_teop3 = bfrt_info.table_get("SwitchEgress.count_src_teop3")
        count_src_teop4 = bfrt_info.table_get("SwitchEgress.count_src_teop4")

        # Install the same SMAC key into both tables so the same packet counts in both tables
        _setup_match(target, count_src, 'SwitchEgress.hit_src', smac, smac_mask)
        _setup_match(target, count_src_teop, 'SwitchEgress.hit_src_teop', smac, smac_mask)
        _setup_match(target, count_src_teop2, 'SwitchEgress.hit_src_teop2', smac, smac_mask)
        _setup_match2(target, count_src_teop3, 'SwitchEgress.hit_src_teop3',
                smac, smac_mask, index)
        _setup_match2(target, count_src_teop4, 'SwitchEgress.hit_src_teop4',
                smac, smac_mask, index)

        # Create input and expected packets
        eth = Ether(dst=dmac, src=smac, type=0x800)
        ip = IP()
        pkt = eth/ip

        pkt /= ("D" * (pkt_len - len(pkt)))
        exp_pkt = Ether(dst=dmac, src=smac, type=0xffff)
        # TODO(sborkows): ATM bf_pktpy's headers are mutable, thus 'len(ip)' will 
        # contain also payload. Thus, changing to 'len(IP())', which does not break 
        # compatibility with Scapy
        exp_pkt /= ("D" * (pkt_len - len(IP()) - len(exp_pkt)))

        # The packet created above does not include the Ethernet FCS so add an additional
        # four bytes to the expected size to account for it.
        inp_pkt_size = len(pkt) + 4
        exp_pkt_size = len(exp_pkt) + 4

        num_pkts = 2;
        num_bytes = num_pkts * inp_pkt_size
        num_bytes_teop = num_pkts * exp_pkt_size

        logger.info("Sending packets on port %d", ig_port)

        for i in range(0, num_pkts):
            testutils.send_packet(self, ig_port, pkt)
            testutils.verify_packet(self, exp_pkt, ig_port)

        logger.info("Expecting packets on port %d", ig_port)

        recv_pkts, recv_bytes = _read_counter(count_src, target,
                'SwitchEgress.hit_src', smac, smac_mask)

        self.verify_teop_counts(count_src_teop, 'SwitchEgress.hit_src_teop',
                target, smac, smac_mask, num_pkts, recv_pkts, recv_bytes,
                num_bytes, num_bytes_teop, "direct_counter_teop", 1, -1);

        self.verify_teop_counts(count_src_teop2, 'SwitchEgress.hit_src_teop2',
                target, smac, smac_mask, num_pkts, recv_pkts, recv_bytes,
                num_bytes, num_bytes_teop, "direct_counter_teop2", 1, -1);

        indirect_counter_teop = bfrt_info.table_get("indirect_counter_teop")
        self.verify_teop_counts(indirect_counter_teop, 'SwitchEgress.hit_src_teop3',
                target, smac, smac_mask, num_pkts, recv_pkts, recv_bytes,
                num_bytes, num_bytes_teop, "indirect_counter_teop", 0, index);

        indirect_counter_teop2 = bfrt_info.table_get("indirect_counter_teop2")
        self.verify_teop_counts(indirect_counter_teop2, 'SwitchEgress.hit_src_teop4',
                target, smac, smac_mask, num_pkts, recv_pkts, recv_bytes,
                num_bytes, num_bytes_teop, "indirect_counter_teop2", 0, index);

        # Clean up
        count_src.entry_del(target)
        count_src_teop.entry_del(target)
        count_src_teop2.entry_del(target)
        count_src_teop3.entry_del(target)
        count_src_teop4.entry_del(target)
