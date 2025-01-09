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

"""
Thrift PD interface basic tests
"""

from collections import OrderedDict

import time
import sys
import logging

import unittest
import random

import pd_base_tests

from ptf import config
from ptf.testutils import *
from p4testutils.misc_utils import *
from ptf.thriftutils import *

from resubmit.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *

dev_id=0
swports = get_sw_ports()

class ResubmitTest(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["resubmit"])

    """ Basic test """
    def runTest(self):

        if test_param_get('arch').lower() != "tofino" and test_param_get('arch').lower() != "tofino2":
            return

        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(dev_id, hex_to_i16(0xFFFF))
        # Test
        ig_port = swports[0]

        # Resubmit with no fields
        destination_mac = "AB:CD:EF:11:22:33"
        dstAddr = macAddr_to_string(destination_mac)
        #eg_port = 1
        eg_port = swports[1]

        resub_hdls = []
        nhop_hdls = []

        try:
            # First pass match resubmit table
            match_on = resubmit_l2_resubmit_match_spec_t(dstAddr)
            h = self.client.l2_resubmit_table_add_with_do_resubmit(sess_hdl, dev_tgt, match_on)
            resub_hdls.append(h)

            # Next pass send to egress
            match_on = resubmit_l2_nhop_match_spec_t(dstAddr)
            egr_action = resubmit_nhop_set_action_spec_t(eg_port)
            h = self.client.l2_nhop_table_add_with_nhop_set(sess_hdl, dev_tgt, match_on, egr_action)
            nhop_hdls.append(h)
            self.conn_mgr.complete_operations(sess_hdl)

            # Specify the pktlen, otherwise on the egress side, it will generate minimum sized packet (for CRC)
            pkt = simple_eth_packet(pktlen=1500, eth_dst=destination_mac)
            print("Sending to port", ig_port, "for resubmit w/o fields")
            send_packet(self, ig_port, pkt)
            time.sleep(1)
            print("Verifying packet came back on port", eg_port)
            verify_packet(self, pkt, eg_port)

            # Resubmit with fields
            destination_mac = "11:22:33:44:55:66"
            dstAddr = macAddr_to_string(destination_mac)
            #eg_port = 2
            eg_port = swports[2]
            eg_eth_type = 0x1234
            match_on = resubmit_l2_resubmit_match_spec_t(dstAddr)
            h = self.client.l2_resubmit_table_add_with_do_resubmit_with_fields(sess_hdl, dev_tgt, match_on)
            resub_hdls.append(h)

            match_on = resubmit_l2_nhop_match_spec_t(dstAddr)
            egr_action = resubmit_nhop_set_action_spec_t(eg_port)
            h = self.client.l2_nhop_table_add_with_nhop_set_with_type(sess_hdl, dev_tgt, match_on, egr_action)
            nhop_hdls.append(h)
            self.conn_mgr.complete_operations(sess_hdl)

            pkt = simple_eth_packet(pktlen=1500, eth_dst=destination_mac)
            print("Sending to port", ig_port, "for resubmit with fields")
            send_packet(self, ig_port, pkt)
            exp_pkt = simple_eth_packet(eth_dst=destination_mac,
                                        eth_type=eg_eth_type, pktlen=1500)
            print("Verifying packet came back on port", eg_port)
            verify_packet(self, exp_pkt, eg_port)

        finally:
            for h in resub_hdls:
                self.client.l2_resubmit_table_delete(sess_hdl, dev_id, h)
            for h in nhop_hdls:
                self.client.l2_nhop_table_delete(sess_hdl, dev_id, h)
            self.conn_mgr.client_cleanup(sess_hdl)
