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
Ports are in internal loopback mode for all tests below
Diag Loopback and Snake tests
"""


import diag_rpc
import logging
import os
import pdb
import pd_base_tests
import random
import subprocess
import sys
import time
import unittest

from collections import OrderedDict
from diag_rpc.ttypes import *
from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *
from res_pd_rpc.ttypes import *

this_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(this_dir, '../base'))
import api_base_tests
from utils import *

dev_id = 0
MAX_PORT_COUNT = 456

g_arch = test_param_get("arch").lower()
g_is_tofino = g_arch == "tofino"
g_is_tofino2 = g_arch == "tofino2"
g_target = test_param_get("target").lower()
g_is_hw = g_target == 'hw'

def tofLocalPortToOfPort(port, dev_id):
    assert port < MAX_PORT_COUNT
    return (dev_id * MAX_PORT_COUNT) + port

swports = []
for device, port, ifname in config["interfaces"]:
    swports.append(port)
    swports.sort()

if swports == []:
    swports = list(range(17))

print(swports)

def portToPipe(port):
    return port >> 7

def portToPipeLocalId(port):
    return port & 0x7F

def hwsetup(self):
    self.ports = swports
    if (g_is_tofino or g_is_tofino2) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAdd(self, dev_id, self.ports, pal_port_speed_t.BF_SPEED_100G,
                pal_fec_type_t.BF_FEC_TYP_NONE, statusCheck=0)

def hwteardown(self):
    if (g_is_tofino or g_is_tofino2) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portDel(self, dev_id, self.ports)

def isPhyLpbkSupported():
    if g_is_tofino2:
        return False
    return True

@group('maubusstress')
@group('phvstress')
class PairTestBasic(api_base_tests.ThriftInterfaceDataPlane,
                        pd_base_tests.ThriftInterfaceDataPlane):
    # Basic Pair test for 2 sessions (bidir)
    # basic_pair_test and parde_stress_pair_test in slt.py
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isPhyLpbkSupported() is False:
            return
        print("2 session bidir pair test basic")
        if g_is_hw:
            num_pkt1 = 16
            num_pkt2 = 16
        else:
            num_pkt1 = 1
            num_pkt2 = 1
        pkt_size1 = 1518
        pkt_size2 = 1518
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PHY
        bidir = 1
        sess_hdl1 = 0
        sess_hdl2 = 0
        try:
            print("Create two pair-test sessions")
            sess_hdl1 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)

            print("Start the packets")
            self.client.diag_loopback_pair_test_start(sess_hdl1, num_pkt1, pkt_size1, bidir)
            self.client.diag_loopback_pair_test_start(sess_hdl2, num_pkt2, pkt_size2, bidir)
            time.sleep(5)

            print("Stop the packets")
            self.client.diag_loopback_pair_test_stop(sess_hdl1)
            self.client.diag_loopback_pair_test_stop(sess_hdl2)
            time.sleep(9)

            print("Get the result")
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            print("Cleanup")
            self.client.diag_loopback_pair_test_cleanup(sess_hdl1)
            self.client.diag_loopback_pair_test_cleanup(sess_hdl2)
            time.sleep(3)

    def tearDown(self):
        hwteardown(self)

@group('maubusstress')
@group('phvstress')
class PairTestBytePattern(api_base_tests.ThriftInterfaceDataPlane,
                        pd_base_tests.ThriftInterfaceDataPlane):
    # Pair test for 2 sessions (bidir) with user specified data-pattern
    # ext_pair_test in slt.py
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isPhyLpbkSupported() is False:
            return
        print("2 session bidir pair byte pattern - fixed pattern")
        if g_is_hw:
            num_pkt1 = 16
            num_pkt2 = 16
        else:
            num_pkt1 = 1
            num_pkt2 = 1
        pkt_size1 = 1518
        pkt_size2 = 1518
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PHY
        bidir = 1
        sess_hdl1 = 0
        sess_hdl2 = 0
        pattern_len = 1
        pattern_mode = diag_data_pattern_t.DIAG_DATA_PATTERN_FIXED #fixed pattern
        pattern1 = 0
        pattern2 = 0xff
        start_pattern = 0xe
        start_pattern_len = 10
        try:
            print("Create two pair-test sessions")
            sess_hdl1 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)

            print("Set the packet payload data pattern to", hex(pattern1), "for session 1")
            self.client.diag_data_pattern_set(sess_hdl1, pattern_mode, start_pattern, start_pattern_len, pattern1, pattern1, pattern_len)
            print("Set the packet payload data pattern to", hex(pattern2), "for session 2")
            self.client.diag_data_pattern_set(sess_hdl2, pattern_mode, start_pattern, start_pattern_len, pattern2, pattern2, pattern_len)

            print("Start the packets")
            self.client.diag_loopback_pair_test_start(sess_hdl1, num_pkt1, pkt_size1, bidir)
            self.client.diag_loopback_pair_test_start(sess_hdl2, num_pkt2, pkt_size2, bidir)
            time.sleep(5)

            print("Stop the packets")
            self.client.diag_loopback_pair_test_stop(sess_hdl1)
            self.client.diag_loopback_pair_test_stop(sess_hdl2)
            time.sleep(9)

            print("Get the result")
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            print("Cleanup")
            self.client.diag_loopback_pair_test_cleanup(sess_hdl1)
            self.client.diag_loopback_pair_test_cleanup(sess_hdl2)
            time.sleep(3)

    def tearDown(self):
        hwteardown(self)

@group('maubusstress')
@group('phvstress')
class PairTestPacketPayload(api_base_tests.ThriftInterfaceDataPlane,
                        pd_base_tests.ThriftInterfaceDataPlane):
    # Pair test for 2 sessions (bidir) with user specified payload
    # byte_pattern_test in slt.py
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isPhyLpbkSupported() is False:
            return
        print("2 session bidir pair test with user specified packet payload")
        if g_is_hw:
            num_pkt1 = 13
            num_pkt2 = 13
        else:
            num_pkt1 = 1
            num_pkt2 = 1
        pkt_size1 = 1518
        pkt_size2 = 1518
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PHY
        bidir = 1
        sess_hdl1 = 0
        sess_hdl2 = 0
        payload_mode = diag_packet_payload_t.DIAG_PACKET_PAYLOAD_FIXED #fixed pattern
        try:
            print("Create two pair-test sessions")
            sess_hdl1 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)

            payload="AA55AA55AA5500000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffffAA55AA55AA55AA55AA00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000ffffffffffffffffffffffffffffffff00000000000000000000000000000000"

            print("Set the packet payload data for session 1")
            self.client.diag_packet_payload_set(sess_hdl1, payload_mode, payload, "")
            print("Set the packet payload data for session 2")
            self.client.diag_packet_payload_set(sess_hdl2, payload_mode, payload, "")

            print("Start the packets")
            self.client.diag_loopback_pair_test_start(sess_hdl1, num_pkt1, pkt_size1, bidir)
            self.client.diag_loopback_pair_test_start(sess_hdl2, num_pkt2, pkt_size2, bidir)
            time.sleep(5)

            print("Stop the packets")
            self.client.diag_loopback_pair_test_stop(sess_hdl1)
            self.client.diag_loopback_pair_test_stop(sess_hdl2)
            time.sleep(9)

            print("Get the result")
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            print("Cleanup")
            self.client.diag_loopback_pair_test_cleanup(sess_hdl1)
            self.client.diag_loopback_pair_test_cleanup(sess_hdl2)
            time.sleep(3)

    def tearDown(self):
        hwteardown(self)

