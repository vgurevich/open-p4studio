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
Diag base tests
"""
import api_base_tests
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
from utils import *
from pal_rpc.ttypes import *


this_dir = os.path.dirname(os.path.abspath(__file__))
this_kpkt_dir = "/sys/devices/pci0000:00/0000:00:03.0/0000:05:00.0/net/ens1"

dev_id = 0
MAX_PORT_COUNT = 64
swports = []
for device, port, ifname in config["interfaces"]:
    swports.append(port)
    swports.sort()

if swports == []:
    swports = list(range(17))

print(swports)

all_ports_list = []
for i in range(0, 65):
    all_ports_list.append(i)

g_arch = test_param_get("arch").lower()
g_is_tofino = g_arch == "tofino"
g_is_tofino2 = g_arch == "tofino2"
g_target = test_param_get("target").lower()
g_is_hw = g_target == 'hw'


def tofLocalPortToOfPort(port, dev_id):
    assert port < MAX_PORT_COUNT
    return (dev_id * MAX_PORT_COUNT) + port


def portToPipe(port):
    return port >> 7


def portToPipeLocalId(port):
    return port & 0x7F


def hwsetup(self):
    self.ports = swports
    if g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAdd(self, dev_id, self.ports, pal_port_speed_t.BF_SPEED_25G,
                pal_fec_type_t.BF_FEC_TYP_NONE, statusCheck=1)


def hwsetup_all_ports(self, speed):
    self.ports = []
    if g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAddAll(self, dev_id, speed,
                pal_fec_type_t.BF_FEC_TYP_NONE)
        time.sleep(90)
    if g_is_tofino or g_is_tofino2:
        self.ports = validPortList(self)


def hwteardown_all_ports(self):
    if g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portDelAll(self, dev_id)


def get_even_ports(self, ports):
    port_list = []
    for i in ports:
        if int(i) % 2 == 0:
            port_list.append(i)
    return port_list


def get_odd_ports(self, ports):
    port_list = []
    for i in ports:
        if int(i) % 2 != 0:
            port_list.append(i)
    return port_list


class TestPair(api_base_tests.ThriftInterfaceDataPlane,
               pd_base_tests.ThriftInterfaceDataPlane):

    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup_all_ports(self, pal_port_speed_t.BF_SPEED_25G)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Pair Test """
    def runTest(self):
        print()
        print("Pair Test 25G")
        self.ports = swports
        evenports = get_even_ports(self, self.ports)
        oddports = get_odd_ports(self, self.ports)
        if g_is_tofino and 64 in evenports:
            evenports.remove(64)
        elif g_is_tofino2 and 0 in evenports:
            evenports.remove(0)

        print(evenports)
        print(oddports)

        bidir = 1
        num_pkt = 1
        if g_is_hw:
            num_pkt = 8

        pkt_size = 1518
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_PHY
        run_time = 90
        print("PAIR TEST SETUP ...")
        sess_hdl1 = self.client.diag_loopback_pair_test_setup(dev_id, evenports, len(evenports), loop_mode)
        sess_hdl2 = self.client.diag_loopback_pair_test_setup(dev_id, oddports, len(oddports), loop_mode)
        # need to set the pattern
        print("PAIR TEST DATA PATTERN ...")
        self.client.diag_data_pattern_set(sess_hdl1, 0, 0, 1, 0xFF, 0xFF, 1)
        self.client.diag_data_pattern_set(sess_hdl2, 0, 0, 1, 0x00, 0x00, 1)
        print("PAIR TEST START ...")
        self.client.diag_loopback_pair_test_start(sess_hdl1, num_pkt, pkt_size, bidir)
        self.client.diag_loopback_pair_test_start(sess_hdl2, num_pkt, pkt_size, bidir)
        print("SLEEPING FOR %s SECONDS ..." % run_time)
        time.sleep(run_time)
        print("PAIR TEST STOP ...")
        self.client.diag_loopback_pair_test_stop(sess_hdl2)
        self.client.diag_loopback_pair_test_stop(sess_hdl1)
        time.sleep(7)
        print("PAIR TEST STATUS ...")
        pair_status1 = self.client.diag_loopback_pair_test_status_get(sess_hdl2)
        pair_status2 = self.client.diag_loopback_pair_test_status_get(sess_hdl1)
        assert(pair_status1 == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        assert(pair_status2 == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        print("PAIR TEST CLEANUP ...")
        self.client.diag_loopback_pair_test_cleanup(sess_hdl2)
        self.client.diag_loopback_pair_test_cleanup(sess_hdl1)

    def tearDown(self):
        hwteardown_all_ports(self)
