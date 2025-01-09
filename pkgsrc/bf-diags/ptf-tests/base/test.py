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
MAX_PORT_COUNT = 456

swports = []
ifnames = {}
for device, port, ifname in config["interfaces"]:
    swports.append(port)
    swports.sort()
    ifnames[port] = ifname

if swports == []:
    swports = list(range(17))

print(swports)

all_ports_list = [0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 128, 132,
                  136, 140, 144, 148, 152, 156, 160, 164, 168, 172, 176, 180, 184, 188,
                  256, 260, 264, 268, 272, 276, 280, 284, 288, 292, 296, 300, 304, 308,
                  312, 316, 384, 388, 392, 396, 400, 404, 408, 412, 416, 420, 424, 428,
                  432, 436, 440, 444]

g_arch = test_param_get("arch").lower()
g_is_tofino = g_arch == "tofino"
g_is_tofino2 = g_arch == "tofino2"
g_is_tofino3 = g_arch == "tofino3"
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
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAdd(self, dev_id, self.ports, pal_port_speed_t.BF_SPEED_100G,
                pal_fec_type_t.BF_FEC_TYP_NONE, statusCheck=1)


def hwsetup_all_ports(self, speed):
    self.ports = []
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portAddAll(self, dev_id, speed,
                pal_fec_type_t.BF_FEC_TYP_NONE)
        time.sleep(90)
    if g_is_tofino or g_is_tofino2 or g_is_tofino3:
        self.ports = validPortList(self)


def hwteardown(self):
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portDel(self, dev_id, self.ports)


def hwteardown_all_ports(self):
    if (g_is_tofino or g_is_tofino2 or g_is_tofino3) and g_is_hw:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        portDelAll(self, dev_id)


def validPortList(self):
    valid_port_list = []
    if g_is_tofino or g_is_tofino2 or g_is_tofino3:
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        for port in all_ports_list:
            valid = self.pal.pal_port_is_valid(dev_id, port)
            if valid:
                valid_port_list.append(port)
    return valid_port_list


def bridgeCreate(self, brname, veth_ifs_all):
    os.system('brctl addbr ' + brname)
    os.system('ifconfig ' + brname + ' up')
    for veth_if in veth_ifs_all:
        os.system('brctl addif ' + brname + ' ' + veth_if)


def bridgeDestroy(self, brname, veth_ifs_all):
    os.system('ifconfig ' + brname + ' down')
    for veth_if in veth_ifs_all:
        os.system('brctl delif ' + brname + ' ' + veth_if)
    os.system('brctl delbr ' + brname)


def bridgeSetup(self, ports):
    self.ports = swports
    if g_is_hw:
        return
    veth_ifs_all_1 = []
    veth_ifs_all_2 = []
    br1_prefix = ''
    br2_prefix = ''
    br2_exists = 0
    count = 0
    for i in ports:
        if count <= 1:
            veth_ifs_all_1.append(ifnames[i])
            br1_prefix = br1_prefix + '-' + str(i) 
        elif count <= 3:
            veth_ifs_all_2.append(ifnames[i])
            br2_prefix = br2_prefix + '-' + str(i)
            br2_exists = 1
        else:
            # Expecting maximum of 4 ports
            assert 0
        count = count + 1    

    brname1 = 'brPort' + br1_prefix
    brname2 = 'brPort' + br2_prefix

    if g_is_tofino or g_is_tofino2 or g_is_tofino3:
        bridgeCreate(self, brname1, veth_ifs_all_1)
        if br2_exists == 1:
            bridgeCreate(self, brname2, veth_ifs_all_2)
    time.sleep(1)


def bridgeTeardown(self, ports):
    self.ports = swports
    if g_is_hw:
        return
    veth_ifs_all_1 = []
    veth_ifs_all_2 = []
    br1_prefix = ''
    br2_prefix = ''
    br2_exists = 0
    count = 0
    for i in ports:
        if count <= 1:
            veth_ifs_all_1.append(ifnames[i])
            br1_prefix = br1_prefix + '-' + str(i) 
        elif count <= 3:
            veth_ifs_all_2.append(ifnames[i])
            br2_prefix = br2_prefix + '-' + str(i) 
            br2_exists = 1
        else:
            # Expecting maximum of 4 ports
            assert 0
        count = count + 1    

    brname1 = 'brPort' + br1_prefix
    brname2 = 'brPort' + br2_prefix

    if g_is_tofino or g_is_tofino2 or g_is_tofino3:
        bridgeDestroy(self, brname1, veth_ifs_all_1)
        if br2_exists == 1:
            bridgeDestroy(self, brname2, veth_ifs_all_2)
    time.sleep(1)


def UntagVlanFlowPkt(self, vlan_id, tag_ports, untag_ports):
    print()
    first_port = untag_ports[0]
    second_port = untag_ports[1]
    print("Sending packet port 4 -> 5 (1.1.1.1 -> 10.0.0.1 [id = 101])")
    pkt1 = simple_tcp_packet(eth_dst='00:00:00:44:44:44',
                             eth_src='00:00:00:55:55:55',
                             ip_dst='10.2.0.1',
                             ip_ttl=64,
                             pktlen=100)
    pkt1_tag = simple_tcp_packet(eth_dst='00:00:00:44:44:44',
                                 eth_src='00:00:00:55:55:55',
                                 ip_dst='10.2.0.1',
                                 ip_ttl=64,
                                 dl_vlan_enable=True,
                                 vlan_vid=vlan_id,
                                 pktlen=104)
    send_packet(self, tofLocalPortToOfPort(first_port, dev_id), pkt1)
    egress_ports = []
    egress_pkts = []
    for p in tag_ports:
        egress_ports.append(p)
        egress_pkts.append(pkt1_tag)
    for p in untag_ports:
        if p != first_port:
            egress_ports.append(p)
            egress_pkts.append(pkt1)
    print("Egress ports are ", egress_ports)
    verify_each_packet_on_each_port(self, egress_pkts, egress_ports)
    # Allow learn to happen sleep for sometime
    time.sleep(4)
    print("Sending packet port 5 -> port 4 (10.0.0.1 -> 1.1.1.1 [id = 101])")
    pkt2 = simple_tcp_packet(eth_dst='00:00:00:55:55:55',
                             eth_src='00:00:00:44:44:44',
                             ip_src='10.0.0.1',
                             ip_dst='1.1.1.1',
                             ip_id=101,
                             ip_ttl=64)
    send_packet(self, tofLocalPortToOfPort(second_port, dev_id), pkt2)
    verify_packets(self, pkt2, [first_port])
    # Allow learn to happen sleep for sometime
    time.sleep(4)
    # Send unicast pkt now
    print("Sending packet port 4 -> port 5 (1.1.1.1 -> 10.0.0.1 [id = 101])")
    send_packet(self, tofLocalPortToOfPort(first_port, dev_id), pkt1)
    verify_packets(self, pkt1, [second_port])
    verify_no_other_packets(self)
    time.sleep(25)


def TagVlanFlowPkt(self, vlan_id, tag_ports, untag_ports):
    print()
    first_port = tag_ports[0]
    second_port = tag_ports[1]
    print("Sending packet port 1 -> 2 (1.1.1.1 -> 10.0.0.1 [id = 101])")
    pkt1 = simple_tcp_packet(eth_dst='00:00:00:66:66:66',
                             eth_src='00:00:00:77:77:77',
                             dl_vlan_enable=True,
                             vlan_vid=vlan_id,
                             ip_dst='10.0.0.1',
                             ip_id=101,
                             ip_ttl=64,
                             pktlen=110)
    pkt1_untag = simple_tcp_packet(eth_dst='00:00:00:66:66:66',
                                   eth_src='00:00:00:77:77:77',
                                   ip_dst='10.0.0.1',
                                   ip_id=101,
                                   ip_ttl=64,
                                   pktlen=106)

    send_packet(self, tofLocalPortToOfPort(first_port, dev_id), pkt1)
    egress_ports = []
    egress_pkts = []
    for p in tag_ports:
        if p != first_port:
            egress_ports.append(p)
            egress_pkts.append(pkt1)
    for p in untag_ports:
        egress_ports.append(p)
        egress_pkts.append(pkt1_untag)
    print("Egress ports are ", egress_ports)
    verify_each_packet_on_each_port(self, egress_pkts, egress_ports)

    # Allow learn to happen sleep for sometime
    time.sleep(4)
    print("Sending packet port 2 -> port 1 (10.0.0.1 -> 1.1.1.1 [id = 101])")
    pkt2 = simple_tcp_packet(eth_dst='00:00:00:77:77:77',
                             eth_src='00:00:00:66:66:66',
                             dl_vlan_enable=True,
                             vlan_vid=vlan_id,
                             ip_src='10.0.0.1',
                             ip_dst='1.1.1.1',
                             ip_id=101,
                             ip_ttl=64)
    send_packet(self, tofLocalPortToOfPort(second_port, dev_id), pkt2)
    verify_packets(self, pkt2, [first_port])
    # Allow learn to happen sleep for sometime
    time.sleep(4)
    # Send unicast pkt now
    print("Sending packet port 1 -> port 2 (1.1.1.1 -> 10.0.0.1 [id = 101])")
    send_packet(self, tofLocalPortToOfPort(first_port, dev_id), pkt1)
    verify_packets(self, pkt1, [second_port])
    verify_no_other_packets(self)
    time.sleep(25)


class TestUnTagVlanFlow(api_base_tests.ThriftInterfaceDataPlane):
    def setUp(self):
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        vlan_id = 21
        tag_ports = [swports[1], swports[2], swports[3]]
        untag_ports = [swports[4], swports[5], swports[6]]
        # 1,2,3 are tagged and 4,5,6 are untagged
        self.client.diag_vlan_create(dev_id, vlan_id)
        for p in tag_ports:
            self.client.diag_port_vlan_add(dev_id, p, vlan_id)
        for p in untag_ports:
            self.client.diag_port_default_vlan_set(dev_id, p, vlan_id)
        # Get default vlan on one port
        def_vlan = self.client.diag_port_default_vlan_get(dev_id, swports[4])
        assert (def_vlan == vlan_id)

        if test_param_get('target') == "hw":
            self.client.diag_learning_timeout_set(dev_id, 1200000)
        else:
            self.client.diag_learning_timeout_set(dev_id, 3000)

        try:
            UntagVlanFlowPkt(self, vlan_id, tag_ports, untag_ports)

        finally:
            print("deleting entries")
            self.client.diag_mac_aging_reset(dev_id)
            self.client.diag_port_vlan_del(dev_id, swports[1], vlan_id)
            self.client.diag_port_default_vlan_reset(dev_id, swports[4])
            self.client.diag_vlan_destroy(dev_id, vlan_id)


class TestTagVlanFlow(api_base_tests.ThriftInterfaceDataPlane):
    def setUp(self):
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        vlan_id = 31
        tag_ports = [swports[1], swports[2], swports[3]]
        untag_ports = [swports[4], swports[5], swports[6]]
        node_ports = []
        # 1,2,3 are tagged and 4,5,6 are untagged
        self.client.diag_vlan_create(dev_id, vlan_id)
        for p in tag_ports:
            node_ports.append(p)
            self.client.diag_port_vlan_add(dev_id, p, vlan_id)
        for p in untag_ports:
            node_ports.append(p)
            self.client.diag_port_default_vlan_set(dev_id, p, vlan_id)
        # Get default vlan on one port
        def_vlan = self.client.diag_port_default_vlan_get(dev_id, swports[4])
        assert (def_vlan == vlan_id)

        if test_param_get('target') == "hw":
            self.client.diag_learning_timeout_set(dev_id, 1200000)
        else:
            self.client.diag_learning_timeout_set(dev_id, 3000)

        try:
            TagVlanFlowPkt(self, vlan_id, tag_ports, untag_ports)
            self.client.diag_mac_aging_reset(dev_id)

        finally:
            print("deleting entries")
            self.client.diag_port_vlan_del(dev_id, swports[1], vlan_id)
            self.client.diag_port_default_vlan_reset(dev_id, swports[4])
            self.client.diag_vlan_destroy(dev_id, vlan_id)


class TestMacAging(api_base_tests.ThriftInterfaceDataPlane):
    def setUp(self):
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        vlan_id = 91
        tag_ports = [swports[1], swports[2], swports[3]]
        node_ports = []
        # 1,2,3 are tagged
        self.client.diag_vlan_create(dev_id, vlan_id)
        for p in tag_ports:
            node_ports.append(p)
            self.client.diag_port_vlan_add(dev_id, p, vlan_id)

        try:
            # Set learning timeout to a lower value in the model
            if test_param_get('target') == "hw":
                self.client.diag_learning_timeout_set(dev_id, 1200000)
            else:
                self.client.diag_learning_timeout_set(dev_id, 3000)
            # Set mac-aging to a small value so that it gets aged out
            self.client.diag_mac_aging_set(dev_id, 5000)
            print ("Sending packet port 1 -> 2 (1.1.1.1 -> 10.0.0.1 [id = 101])")
            pkt1 = simple_tcp_packet(eth_dst='00:00:01:01:01:01',
                                     eth_src='00:00:02:02:02:02',
                                     dl_vlan_enable=True,
                                     vlan_vid=vlan_id,
                                     ip_src='1.1.1.1',
                                     ip_dst='10.0.0.1',
                                     ip_id=101,
                                     ip_ttl=64)
            send_packet(self, tofLocalPortToOfPort(swports[1], dev_id), pkt1)
            egress_ports = []
            for p in node_ports:
                egress_ports.append(p)
            egress_ports.remove(swports[1])
            print("Egress ports are ", egress_ports)
            verify_packets(self, pkt1, egress_ports)
            # Allow learn and age to happen sleep for sometime
            time.sleep(25)
            # should flood again
            print("Sending packet port 2 -> port 1 (10.0.0.1 -> 1.1.1.1 [id = 101])")
            pkt2 = simple_tcp_packet(eth_dst='00:00:02:02:02:02',
                                     eth_src='00:00:01:01:01:01',
                                     dl_vlan_enable=True,
                                     vlan_vid=vlan_id,
                                     ip_src='10.0.0.1',
                                     ip_dst='1.1.1.1',
                                     ip_id=101,
                                     ip_ttl=64)
            send_packet(self, tofLocalPortToOfPort(swports[2], dev_id), pkt2)
            egress_ports = []
            for p in node_ports:
                egress_ports.append(p)
            egress_ports.remove(swports[2])
            print("Egress ports are ", egress_ports)
            verify_packets(self, pkt2, egress_ports)
            time.sleep(25)

        finally:
            print("deleting entries")
            self.client.diag_mac_aging_reset(dev_id)
            self.client.diag_vlan_destroy(dev_id, vlan_id)


class TestForwarding(api_base_tests.ThriftInterfaceDataPlane):
    def setUp(self):
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        tcp_start = 0
        tcp_end = 65530
        ig_port = swports[1]
        eg_port = swports[2]
        pri10 = 10
        hdl2 = self.client.diag_forwarding_rule_add(dev_id, ig_port, eg_port, tcp_start, tcp_end, pri10)
        print("Forwarding rule added with handle ", hdl2)
        # Send pkt and verify
        print("Sending packet port 1 -> port 2)")
        pkt2 = simple_tcp_packet(eth_dst='00:00:00:05:05:05',
                                 eth_src='00:00:00:04:04:04',
                                 ip_src='10.0.0.1',
                                 ip_dst='1.1.1.1',
                                 ip_id=101,
                                 ip_ttl=64)
        send_packet(self, tofLocalPortToOfPort(swports[1], dev_id), pkt2)
        verify_packets(self, pkt2, [eg_port])

        # Add high priority fwding entry
        eg_port = swports[3]
        pri8 = 8
        hdl3 = self.client.diag_forwarding_rule_add(dev_id, ig_port, eg_port, tcp_start, tcp_end, pri8)
        print("Forwarding rule added with handle ", hdl3)
        # Send pkt and verify
        print("Sending packet port 1 -> port 3)")
        pkt3 = simple_tcp_packet(eth_dst='00:00:00:05:05:01',
                                 eth_src='00:00:00:04:04:01',
                                 ip_src='10.0.0.1',
                                 ip_dst='1.1.1.1',
                                 ip_id=101,
                                 ip_ttl=64)
        send_packet(self, tofLocalPortToOfPort(swports[1], dev_id), pkt3)
        verify_packets(self, pkt3, [eg_port])

        print("deleting entries")
        self.client.diag_forwarding_rule_del(dev_id, hdl2)
        self.client.diag_forwarding_rule_del(dev_id, hdl3)
        print("Send pkt after deleting entry, no packet should be received")
        pkt3 = simple_tcp_packet(eth_dst='00:00:00:05:05:02',
                                 eth_src='00:00:00:04:04:02',
                                 ip_src='10.0.0.1',
                                 ip_dst='1.1.1.1',
                                 ip_id=101,
                                 ip_ttl=64)
        send_packet(self, tofLocalPortToOfPort(swports[1], dev_id), pkt3)
        verify_no_other_packets(self)
        time.sleep(25)


@group('loopback')
@group('maubusstress')
class TestExtLoopback(api_base_tests.ThriftInterfaceDataPlane,
                      pd_base_tests.ThriftInterfaceDataPlane):
    # CPU originated external loopback test for 2 ports
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 3
        pkt_size = 600
        num_ports = len(self.port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_test_start(sess_hdl, num_pkt, pkt_size)
            time.sleep(12)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl)
            print(test_status)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtLoopbackSnake(api_base_tests.ThriftInterfaceDataPlane,
                           pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13], swports[14], swports[15]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 2
        pkt_size = 750
        num_ports = len(self.port_list)
        sess_hdl = 0
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        try:
            sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, 0)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            time.sleep(3)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl)
            time.sleep(3)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtLoopbackSnakeBidir(api_base_tests.ThriftInterfaceDataPlane,
                                pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13], swports[14], swports[15]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 1
        pkt_size = 800
        num_ports = len(self.port_list)
        sess_hdl = 0
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        try:
            sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            # bidir is passed to snake start
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, 1)
            time.sleep(1)
            # do not pass bidir for next pkt
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size+50, 0)
            # pass bidir for next pkt
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size-50, 1)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtSnakePair(api_base_tests.ThriftInterfaceDataPlane,
                           pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13], swports[14], swports[15]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 2
        pkt_size = 500
        num_ports = len(self.port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, 0)
            time.sleep(5)
            self.client.diag_loopback_pair_test_stop(sess_hdl)
            time.sleep(7)
            print("Reading counters")
            stats = []
            for i in range(0, num_ports):
                s = self.client.diag_cpu_stats_get(dev_id, self.port_list[i])
                stats.append(s)

            # Print count values
            for i in range(0, num_ports):
                print("Port ", self.port_list[i], " -> tx_tot: ", stats[i].tx_total, " rx_tot: ", stats[i].rx_total, "rx_good: ", stats[i].rx_good, "rx_bad: ", stats[i].rx_bad)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_pair_test_cleanup(sess_hdl)
            time.sleep(3)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtSnakePairBidir(api_base_tests.ThriftInterfaceDataPlane,
                            pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13], swports[14], swports[15]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 1
        pkt_size = 550
        num_ports = len(self.port_list)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl = 0
        try:
            sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, 1)
            time.sleep(1)
            self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size+50, 1)
            time.sleep(5)
            self.client.diag_loopback_pair_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_pair_test_cleanup(sess_hdl)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtPktInj(api_base_tests.ThriftInterfaceDataPlane,
                    pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 2
        pkt_size = 500
        num_ports = len(self.port_list)

        print("Adding forwarding entries")
        # Install forwarding entries
        cpu_port = self.client.diag_cpu_port_get(dev_id)
        hdls = []
        for i in range(0, num_ports):
            hdl = self.client.diag_forwarding_rule_add(
                dev_id, cpu_port, self.port_list[i], self.port_list[i], self.port_list[i], 10)
            hdls.append(hdl)
            hdl = self.client.diag_forwarding_rule_add(dev_id, self.port_list[i],
                                                       cpu_port, 1, 65535, 10)
            hdls.append(hdl)
        time.sleep(2)

        # Clear all counters
        stats = self.client.diag_cpu_stats_clear(dev_id, 0, 1)

        # Send packet
        print("Injecting ", num_pkt, " packets")
        self.client.diag_packet_inject_from_cpu(dev_id, self.port_list, num_ports, num_pkt, pkt_size)
        time.sleep(5)

        # Get counters after send
        print("Reading counters")
        stats = []
        for i in range(0, num_ports):
            s = self.client.diag_cpu_stats_get(dev_id, self.port_list[i])
            stats.append(s)

        # Check count values
        for i in range(0, num_ports):
            print("Port ", self.port_list[i], " -> tx: ", stats[i].tx_total, " rx: ", stats[i].rx_bad)
            assert (stats[i].tx_total == num_pkt)
            # Since pkt is not expected, rx_bad goes up
            assert (stats[i].rx_bad == num_pkt)

        print("Cleaning up")
        # Delete forwarding entries
        for hdl in hdls:
            self.client.diag_forwarding_rule_del(dev_id, hdl)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)


@group('loopback')
class TestSessions(api_base_tests.ThriftInterfaceDataPlane,
                   pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        self.port_list = [swports[13], swports[14], swports[15]]
        num_ports = len(self.port_list)
        sess_hdl = 0
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
        assert (sess_hdl != 0)
        valid = self.client.diag_session_valid(sess_hdl)
        assert valid == 1

        print("Trying to create overlapping sessions")
        # Check for some other valid configs
        # Create an overlapping session
        self.port_list = [swports[14], swports[15]]
        num_ports = len(self.port_list)
        sess_hdl1 = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
        assert (sess_hdl1 != 0)
        self.client.diag_loopback_snake_test_cleanup(sess_hdl1)
        # Create another overlapping session
        self.port_list = [swports[13]]
        num_ports = len(self.port_list)
        sess_hdl1 = self.client.diag_loopback_test_setup(dev_id, self.port_list, num_ports, loop_mode)
        assert (sess_hdl1 != 0)
        self.client.diag_loopback_test_cleanup(sess_hdl1)

        # Create a valid loopback session
        self.port_list = [swports[12]]
        num_ports = len(self.port_list)
        sess_hdl1 = self.client.diag_loopback_test_setup(dev_id, self.port_list, num_ports, loop_mode)
        assert (sess_hdl1 != 0)

        print("Deleting all sessions")
        # Delete all sessions
        self.client.diag_loopback_snake_test_cleanup(sess_hdl)
        self.client.diag_loopback_test_cleanup(sess_hdl1)
        # Make sure sessions got deleted
        valid = self.client.diag_session_valid(sess_hdl)
        assert valid == 0
        valid = self.client.diag_session_valid(sess_hdl1)
        assert valid == 0

    def tearDown(self):
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtLoopbackSnake2(api_base_tests.ThriftInterfaceDataPlane,
                            pd_base_tests.ThriftInterfaceDataPlane):
    # Snake test for 2 sessions
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        bridgeSetup(self, self.port_list1)
        bridgeSetup(self, self.port_list2)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("2 Snake test")
        num_pkt1 = 2
        num_pkt2 = 2
        pkt_size1 = 580
        pkt_size2 = 710
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl1 = 0
        sess_hdl2 = 0
        try:
            print("Creating first snake")
            sess_hdl1 = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)

            print("Creating second snake")
            sess_hdl2 = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)
            print("Sending pkts on both snakes")
            self.client.diag_loopback_snake_test_start(sess_hdl1, num_pkt1, pkt_size1, 0)
            time.sleep(1)
            self.client.diag_loopback_snake_test_start(sess_hdl2, num_pkt2, pkt_size2, 0)
            time.sleep(5)
            print("Stopping both snakes")
            self.client.diag_loopback_snake_test_stop(sess_hdl1)
            time.sleep(1)
            self.client.diag_loopback_snake_test_stop(sess_hdl2)
            time.sleep(5)
            print("Checking test status")
            test_status1 = self.client.diag_loopback_snake_test_status_get(sess_hdl1)
            assert (test_status1 == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status2 = self.client.diag_loopback_snake_test_status_get(sess_hdl2)
            assert (test_status2 == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            print("Deleting both snakes")
            self.client.diag_loopback_snake_test_cleanup(sess_hdl1)
            self.client.diag_loopback_snake_test_cleanup(sess_hdl2)
            time.sleep(3)

    def tearDown(self):
        bridgeTeardown(self, self.port_list1)
        bridgeTeardown(self, self.port_list2)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtLoopbackSnakeBidir2(api_base_tests.ThriftInterfaceDataPlane,
                                 pd_base_tests.ThriftInterfaceDataPlane):
    # Snake bidir test for 2 sessions
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        bridgeSetup(self, self.port_list1)
        bridgeSetup(self, self.port_list2)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("2 snake pair bidir test")
        num_pkt1 = 1
        num_pkt2 = 1
        pkt_size1 = 700
        pkt_size2 = 807
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        sess_hdl1 = 0
        sess_hdl2 = 0
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        try:
            sess_hdl1 = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)
            #bidir is passed to snake start
            self.client.diag_loopback_snake_test_start(sess_hdl1, num_pkt1, pkt_size1, 1)
            self.client.diag_loopback_snake_test_start(sess_hdl2, num_pkt2, pkt_size2, 1)
            time.sleep(1)
            # do not pass bidir for next pkt
            self.client.diag_loopback_snake_test_start(sess_hdl1, num_pkt1, pkt_size1+50, 0)
            # pass bidir for next pkt
            self.client.diag_loopback_snake_test_start(sess_hdl1, num_pkt1, pkt_size1-50, 1)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl1)
            self.client.diag_loopback_snake_test_stop(sess_hdl2)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl1)
            self.client.diag_loopback_snake_test_cleanup(sess_hdl2)

    def tearDown(self):
        bridgeTeardown(self, self.port_list1)
        bridgeTeardown(self, self.port_list2)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtLoopback2(api_base_tests.ThriftInterfaceDataPlane,
                       pd_base_tests.ThriftInterfaceDataPlane):
    # CPU originated external loopback test for 2 sessions
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        bridgeSetup(self, self.port_list1)
        bridgeSetup(self, self.port_list2)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("2 loopback test")
        num_pkt1 = 3
        num_pkt2 = 4
        pkt_size1 = 601
        pkt_size2 = 805
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl1 = 0
        sess_hdl2 = 0
        try:
            sess_hdl1 = self.client.diag_loopback_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)
            self.client.diag_loopback_test_start(sess_hdl1, num_pkt1, pkt_size1)
            self.client.diag_loopback_test_start(sess_hdl2, num_pkt2, pkt_size2)
            time.sleep(12)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_test_cleanup(sess_hdl1)
            self.client.diag_loopback_test_cleanup(sess_hdl2)

    def tearDown(self):
        bridgeTeardown(self, self.port_list1)
        bridgeTeardown(self, self.port_list2)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestExtSnakePair2(api_base_tests.ThriftInterfaceDataPlane,
                        pd_base_tests.ThriftInterfaceDataPlane):
    # Pair test for 2 sessions
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list1 = [swports[12], swports[13]]
        self.port_list2 = [swports[14], swports[15]]
        bridgeSetup(self, self.port_list1)
        bridgeSetup(self, self.port_list2)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("2 loopback pair test")
        num_pkt1 = 2
        num_pkt2 = 2
        pkt_size1 = 501
        pkt_size2 = 1020
        num_ports1 = len(self.port_list1)
        num_ports2 = len(self.port_list2)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sess_hdl1 = 0
        sess_hdl2 = 0
        try:
            sess_hdl1 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list1, num_ports1, loop_mode)
            assert (sess_hdl1 != 0)
            sess_hdl2 = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list2, num_ports2, loop_mode)
            assert (sess_hdl2 != 0)
            self.client.diag_loopback_pair_test_start(sess_hdl1, num_pkt1, pkt_size1, 0)
            self.client.diag_loopback_pair_test_start(sess_hdl2, num_pkt2, pkt_size2, 0)
            time.sleep(5)
            self.client.diag_loopback_pair_test_stop(sess_hdl1)
            self.client.diag_loopback_pair_test_stop(sess_hdl2)
            time.sleep(7)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl1)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl2)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
        finally:
            self.client.diag_loopback_pair_test_cleanup(sess_hdl1)
            self.client.diag_loopback_pair_test_cleanup(sess_hdl2)
            time.sleep(3)

    def tearDown(self):
        bridgeTeardown(self, self.port_list1)
        bridgeTeardown(self, self.port_list2)
        hwteardown(self)


@group('loopback')
@group('maubusstress')
class TestStressPair(api_base_tests.ThriftInterfaceDataPlane,
                     pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup_all_ports(self, pal_port_speed_t.BF_SPEED_100G)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("Stress 100G Pair test")
        if os.path.isdir(this_kpkt_dir):
            return
        if not g_is_hw:
            return
        if os.path.isdir(this_kpkt_dir):
            print("kernel mode packet test")
            num_pkt = 10
            pkt_size = 100
        else:
            num_pkt = 150
            pkt_size = 177
        bidir = 1
        run_time = 90
        self.port_list = self.ports
        num_ports = len(self.port_list)
        if (num_ports % 2) != 0:
            self.port_list.pop(num_ports-1)
            num_ports = len(self.port_list)
        print(self.port_list)
        print("Num ports is ", num_ports)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl = 0
        num_loops = 25
        print("Stress 100G Pair Max Iterations: ", num_loops)
        for loop_num in range(0, num_loops):
            print("Stress 100G Pair Iteration number: ", loop_num)
            try:
                sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list, num_ports, loop_mode)
                assert (sess_hdl != 0)
                print("Starting traffic")
                self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, bidir)
                time.sleep(run_time)
                self.client.diag_loopback_pair_test_stop(sess_hdl)
                time.sleep(7)
                test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
                assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            finally:
                self.client.diag_loopback_pair_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown_all_ports(self)


@group('loopback')
@group('maubusstress')
class TestStress25GPair(api_base_tests.ThriftInterfaceDataPlane,
                        pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup_all_ports(self, pal_port_speed_t.BF_SPEED_25G)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print("Stress 25G Pair test")
        if os.path.isdir(this_kpkt_dir):
            return
        if not g_is_hw:
            return
        if os.path.isdir(this_kpkt_dir):
            print("kernel mode packet test")
            num_pkt = 2
            pkt_size = 80
        else:
            num_pkt = 125
            pkt_size = 177
        bidir = 1
        run_time = 180
        self.port_list = self.ports
        num_ports = len(self.port_list)
        if (num_ports % 2) != 0:
            self.port_list.pop(num_ports-1)
            num_ports = len(self.port_list)
        print(self.port_list)
        print("Num ports is ", num_ports)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl = 0
        num_loops = 10
        print("Stress 25G Pair Max Iterations: ", num_loops)
        for loop_num in range(0, num_loops):
            print("Stress 25G Pair Iteration number: ", loop_num)
            try:
                sess_hdl = self.client.diag_loopback_pair_test_setup(dev_id, self.port_list, num_ports, loop_mode)
                assert (sess_hdl != 0)
                print("Starting traffic")
                self.client.diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, bidir)
                time.sleep(run_time)
                self.client.diag_loopback_pair_test_stop(sess_hdl)
                time.sleep(7)
                test_status = self.client.diag_loopback_pair_test_status_get(sess_hdl)
                assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            finally:
                self.client.diag_loopback_pair_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown_all_ports(self)


@group('loopback')
@group('maubusstress')
class TestStressSnake(api_base_tests.ThriftInterfaceDataPlane,
                      pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup_all_ports(self, pal_port_speed_t.BF_SPEED_100G)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        print ("Stress 100G Snake test")
        if os.path.isdir(this_kpkt_dir):
            return
        if not g_is_hw:
            return
        if os.path.isdir(this_kpkt_dir):
            print("kernel mode packet test")
            num_pkt = 25
        else:
            num_pkt = 1200
        pkt_size = 177
        bidir = 1
        run_time = 240
        self.port_list = self.ports
        num_ports = len(self.port_list)
        print(self.port_list)
        print("Num ports is ", num_ports)
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_MAC
        sess_hdl = 0
        num_loops = 10
        print("Stress 100G Snake Max Iterations: ", num_loops)
        for loop_num in range(0, num_loops):
            print("Stress 100G Snake Iteration number: ", loop_num)
            try:
                sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
                assert (sess_hdl != 0)
                print("Starting traffic")
                self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, bidir)
                time.sleep(run_time)
                self.client.diag_loopback_snake_test_stop(sess_hdl)
                time.sleep(7)
                test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
                assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            finally:
                self.client.diag_loopback_snake_test_cleanup(sess_hdl)

    def tearDown(self):
        hwteardown_all_ports(self)


@group('loopback')
@group('maubusstress')
class TestSessionsMax(api_base_tests.ThriftInterfaceDataPlane,
                      pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        self.port_list = [swports[12], swports[13], swports[14], swports[15]]
        bridgeSetup(self, self.port_list)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 2
        pkt_size = 934
        num_ports = len(self.port_list)
        sess_hdl = 0
        loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
        sessions_max = 128
        sessions_default = 32
        try:
            # Change max sessions to 256
            self.client.diag_sessions_max_set(dev_id, sessions_max)
            # Run the snake test to ensure things are fine 
            sess_hdl = self.client.diag_loopback_snake_test_setup(dev_id, self.port_list, num_ports, loop_mode)
            assert (sess_hdl != 0)
            self.client.diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, 0)
            time.sleep(5)
            self.client.diag_loopback_snake_test_stop(sess_hdl)
            time.sleep(7)
            test_status = self.client.diag_loopback_snake_test_status_get(sess_hdl)
            assert (test_status == diag_test_status_t.DIAG_TEST_STATUS_PASS)
            time.sleep(3)
        finally:
            self.client.diag_loopback_snake_test_cleanup(sess_hdl)
            time.sleep(3)
            # Change max sessions to default of 32
            self.client.diag_sessions_max_set(dev_id, sessions_default)

    def tearDown(self):
        bridgeTeardown(self, self.port_list)
        hwteardown(self)

class TestStream(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 1
        pkt_size = 200
        dst_port = swports[2]
        sess_hdl= 0
        try:
            print("Setting up stream")
            sess_hdl = self.client.diag_stream_setup(dev_id, num_pkt, pkt_size, dst_port)
            assert (sess_hdl != 0)
            counter1 = self.client.diag_stream_counter_get(sess_hdl)
            assert(counter1 == 0)

            print("Starting stream")
            self.client.diag_stream_start(sess_hdl)
            time.sleep(3)
            counter2 = self.client.diag_stream_counter_get(sess_hdl)
            assert(counter2 != 0)
            print("---stream counter value: ", counter2)

            print("Adjusting stream")
            self.client.diag_stream_adjust(sess_hdl, num_pkt+1, pkt_size)
            time.sleep(3)
            counter3 = self.client.diag_stream_counter_get(sess_hdl)
            assert(counter3 > counter2)
            print("---stream counter value: ", counter3)

            print("Stopping stream")
            self.client.diag_stream_stop(sess_hdl)
        finally:
            self.client.diag_stream_cleanup(sess_hdl)
            time.sleep(2)

    def tearDown(self):
        hwteardown(self)

class TestMultiStream(api_base_tests.ThriftInterfaceDataPlane,
                         pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["diag"])

    def setUp(self):
        hwsetup(self)
        api_base_tests.ThriftInterfaceDataPlane.setUp(self)

    """ Basic test """
    def runTest(self):
        print()
        num_pkt = 1
        pkt_size = 200
        dst_port1 = swports[2]
        dst_port2 = swports[4]
        sess_hdl= 0
        try:
            print("Setting up streams")
            sess_hdl1 = self.client.diag_stream_setup(dev_id, num_pkt, pkt_size, dst_port1)
            assert (sess_hdl1 != 0)
            counter1_1 = self.client.diag_stream_counter_get(sess_hdl)
            assert(counter1_1 == 0)
            sess_hdl2 = self.client.diag_stream_setup(dev_id, num_pkt, pkt_size, dst_port2)
            assert (sess_hdl2 != 0)
            counter2_1 = self.client.diag_stream_counter_get(sess_hdl2)
            assert(counter2_1 == 0)

            print("Starting streams")
            self.client.diag_stream_start(sess_hdl1)
            self.client.diag_stream_start(sess_hdl2)
            time.sleep(3)
            counter1_2 = self.client.diag_stream_counter_get(sess_hdl1)
            assert(counter1_2 != 0)
            print("---stream1 counter value: ", counter1_2)
            counter2_2 = self.client.diag_stream_counter_get(sess_hdl2)
            assert(counter2_2 != 0)
            print("---stream2 counter value: ", counter2_2)

            print("Stopping stream")
            self.client.diag_stream_stop(sess_hdl1)
            self.client.diag_stream_stop(sess_hdl2)
        finally:
            self.client.diag_stream_cleanup(sess_hdl1)
            self.client.diag_stream_cleanup(sess_hdl2)
            time.sleep(2)

    def tearDown(self):
        hwteardown(self)

