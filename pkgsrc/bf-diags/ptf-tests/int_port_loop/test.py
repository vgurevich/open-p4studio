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
from pal_rpc.ttypes import *

this_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(this_dir, '../base'))
import api_base_tests
from utils import *

dev_id = 0
MAX_PORT_COUNT = 456

g_arch = test_param_get("arch").lower()
g_is_tofino = g_arch == "tofino"
g_is_tofino2 = g_arch == "tofino2"
g_is_tofino3 = g_arch == "tofino3"
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
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAdd(self, dev_id, self.ports, pal_port_speed_t.BF_SPEED_100G,
                pal_fec_type_t.BF_FEC_TYP_NONE, statusCheck=0)

def hwteardown(self):
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portDel(self, dev_id, self.ports)

def isMacLpbkSupported():
    if g_is_tofino3:
        return False
    return True

@group('loopback')
@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntLoopbackSnake(api_base_tests.ThriftInterfaceDataPlane,
                           pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)
        print(self.ports)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 3
        pkt_size = 600
        port_list = [swports[12], swports[13], swports[14], swports[15]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        try:
            sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, 0)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('loopback')
@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntLoopbackSnakeBidir(api_base_tests.ThriftInterfaceDataPlane,
                                pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 3
        pkt_size = 600
        port_list = [swports[12], swports[13], swports[14], swports[15]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl= 0
        try:
            sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            #bidir flag is passed to snake start
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, 1)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntMacLoopback(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    # CPU originated internal MAC loopback test on one port
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 4
        pkt_size = 1100
        port_list = [swports[2]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl= 0
        try:
            sess_hdl = self.client.diag_loopback_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(7)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            # Check port status and stats
            port_status = self.client.diag_loopback_test_port_status_get(sess_hdl, swports[2])
            assert(port_status.status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            assert(port_status.tx_total == num_pkt)
            assert(port_status.rx_total == num_pkt)
            assert(port_status.rx_good == num_pkt)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntPhyLoopback(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    # CPU originated internal PHY loopback test on one port
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        # not supported in tofino2/tofino3 asic
        if g_is_tofino2 or g_is_tofino3:
            return
        num_pkt = 5
        pkt_size = 592
        port_list = [swports[3]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PHY
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(7)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntPCSLoopback(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    # CPU originated internal PCS loopback test on one port
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 3
        pkt_size = 1350
        port_list = [swports[4]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PCS
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(7)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('loopback')
@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntSnakePair(api_base_tests.ThriftInterfaceDataPlane,
                       pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)
        print(self.ports)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 1
        pkt_size = 900
        port_list = [swports[12], swports[13], swports[14], swports[15]]
        print("Ports are ", port_list)
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl= 0
        try:
            sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, 0)
            time.sleep(5)
            self.client.diag_loopback_pair_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_pair_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

@group('loopback')
@group('maubusstress')
@group('phvstress')
@group('pardestrain')
class TestIntSnakePairBidir(api_base_tests.ThriftInterfaceDataPlane,
                            pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 1
        pkt_size = 650
        port_list = [swports[10], swports[11], swports[14], swports[15]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, 1)
            time.sleep(5)
            self.client.diag_loopback_pair_test_stop(sess_hdl)
            time.sleep(11)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_pair_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown(self)

class TestIntMulticastLoopback(api_base_tests.ThriftInterfaceDataPlane,
                           pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12]]
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        # Skip Test for HW runs for now, test needs ports in all pipes
        if g_is_hw or isMacLpbkSupported() is False:
            return
        num_pkt = 1
        pkt_size = 780
        num_ports = len(self.port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_multicast_loopback_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            print("Starting packets")
            self.client.diag_multicast_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(5)
            print("Stopping packets")
            self.client.diag_multicast_loopback_test_stop(sess_hdl)
            time.sleep(11)
            print("Reading counters")
            stats = []
            for i in range(0, num_ports):
                s = self.client.diag_cpu_stats_get(dev_id, self.port_list[i])
                stats.append(s)

            # Print count values
            for i in range(0, num_ports):
                print("Port ", self.port_list[i], " -> tx_tot: ", stats[i].tx_total,
                      " rx_tot: ", stats[i].rx_total, "rx_good: ", stats[i].rx_good,
                      "rx_bad: ", stats[i].rx_bad)
            test_status = self.client.diag_multicast_loopback_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_multicast_loopback_test_cleanup(sess_hdl)
            time.sleep(3)

    def tearDown(self):
        hwteardown(self)

class TestMinPktSize(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    # Test minimum pkt size of 64
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        if isMacLpbkSupported() is False:
            return
        num_pkt = 2
        pkt_size = 64
        port_list = [swports[2]]
        num_ports = len(port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl= 0
        try:
            # Enable min pkt size
            self.client.diag_min_packet_size_enable(1)
            sess_hdl = self.client.diag_loopback_test_setup(dev_id, port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(7)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            # Check port status and stats
            port_status = self.client.diag_loopback_test_port_status_get(sess_hdl, swports[2])
            assert(port_status.status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            assert(port_status.tx_total == num_pkt)
            assert(port_status.rx_total == num_pkt)
            assert(port_status.rx_good == num_pkt)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl)
            # Disable min pkt size
            self.client.diag_min_packet_size_enable(0)

    def tearDown(self):
        hwteardown(self)
