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

import time
import sys
import logging

import unittest

import pd_base_tests

from ptf import config
from ptf.testutils import *
from p4testutils.misc_utils import *
from ptf.thriftutils import *

import os

from hash_driven.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from collections import defaultdict

import random

def_nop_entries_installed = False

color_green = 0
color_yellow = 1
color_red = 3
dev_id = 0

swports = get_sw_ports()

def port_to_pipe(port):
    local_port = port & 0x7F
    assert(local_port < 72)
    pipe = (port >> 7) & 0x3
    assert(port == ((pipe << 7) | local_port))
    return pipe

def advance_model_time_by_clocks(test, shdl, clocks):
    # Convert clocks to pico-seconds and advance time.
    pico_per_clock = 1000
    if test_param_get("arch") == "tofino":
        pico_per_clock = 800

    picos = clocks * pico_per_clock
    test.conn_mgr.advance_model_time(shdl, dev_id, picos)
    test.conn_mgr.complete_operations(shdl)

def InstallAllDefaultEntries(self):
    global def_nop_entries_installed

    if def_nop_entries_installed == True:
        print("Default nop entries already installed")
        return

    sess_hdl = self.conn_mgr.client_init()
    dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

    nop_default_fns = [self.client.simple_meter_set_default_action_nop,
                       self.client.simple_counter_set_default_action_nop,
                       self.client.color_match_set_default_action_nop,
                       self.client.meter_drop_set_default_action_nop]

    print("Installing default entries as NOP for all tables")

    for nop_fn in nop_default_fns:
        nop_fn(sess_hdl, dev_tgt)

    # default_meter_spec = hash_driven_bytes_meter_spec_t(0, 0, 0, 0, False, False)
    self.client.simple_meter_set_default_action_nop(sess_hdl, dev_tgt)
    self.client.simple_counter_set_default_action_nop(sess_hdl, dev_tgt)
    self.client.color_match_set_default_action_nop(sess_hdl, dev_tgt)
    self.client.meter_drop_set_default_action_nop(sess_hdl, dev_tgt)

    def_nop_entries_installed = True

    print("closing session")
    self.conn_mgr.complete_operations(sess_hdl)
    self.conn_mgr.client_cleanup(sess_hdl)

def RemoveAllDefaultEntries(self, restore=False):
    global def_nop_entries_installed

    sess_hdl = self.conn_mgr.client_init()
    dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

    nop_default_fns = [self.client.simple_meter_table_reset_default_entry,
                       self.client.simple_counter_table_reset_default_entry,
                       self.client.color_match_table_reset_default_entry,
                       self.client.meter_drop_table_reset_default_entry]

    print("Removing default entries from all tables")

    for nop_fn in nop_default_fns:
        nop_fn(sess_hdl, dev_tgt)

    if not restore:
        def_nop_entries_installed = False

    print("closing session")
    self.conn_mgr.complete_operations(sess_hdl)
    self.conn_mgr.client_cleanup(sess_hdl)


class TestOneCnt(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["hash_driven"])

    """ Basic meter test with Exact match table, indirectly addressed """
    def runTest(self):
        InstallAllDefaultEntries(self)

        sess_hdl = self.conn_mgr.client_init()
        print("PIPE_MGR gave me that session handle:", sess_hdl)

        port = swports[0]
        meter_idx = 6   # Hash calculation is set to meter based on IPv4 protocol
        stat_idx = 4  # Hash calculation is set to count based on IPv4 version field
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
        v4_pkts_sent = 0

        match_spec = hash_driven_simple_meter_match_spec_t(ipv4Addr_to_i32("10.1.1.10"))

        action_spec = hash_driven_meter_action_action_spec_t(port,
                                                             macAddr_to_string("00:11:22:33:44:55"),
                                                             macAddr_to_string("00:43:53:11:22:33"))

        meter_0_spec = hash_driven_bytes_meter_spec_t(1000, 8, 2000, 16, False)

        self.client.meter_set_meter_0(sess_hdl, dev_tgt, meter_idx, meter_0_spec)
        print("Adding entry")

        entry_hdl = self.client.simple_meter_table_add_with_meter_action(sess_hdl, dev_tgt, match_spec, action_spec)

        match_spec_c = hash_driven_simple_counter_match_spec_t(ipv4Addr_to_i32("10.1.1.10"))
        entry_hdl_c = self.client.simple_counter_table_add_with_cnt_action(sess_hdl, dev_tgt, match_spec_c)
        self.conn_mgr.complete_operations(sess_hdl)


        # Send one packet, and then check counter value
        pkt = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                eth_src='00:22:22:22:22:22',
                                ip_src='200.1.2.3',
                                ip_dst='10.1.1.10',
                                ip_id=101,
                                ip_ttl=64,
                                tcp_sport = 1000,
                                with_tcp_chksum=False)

        exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                    eth_src='00:11:22:33:44:55',
                                    ip_src='200.1.2.3',
                                    ip_dst='10.1.1.10',
                                    ip_id=101,
                                    ip_ttl=63,
                                    tcp_sport = 1000,
                                    with_tcp_chksum=False)

        try:
            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            verify_packets(self, exp_pkt, [port])

            # Verify counter value
            flags = hash_driven_counter_flags_t(1)
            cntr = self.client.counter_read_counter_0(sess_hdl, dev_tgt, stat_idx, flags)
            print("Cntr value here is %s" % str(cntr))
            print("   bytes = %s" % str(cntr.bytes))
            print("   packets = %s" % str(cntr.packets))
            if cntr.packets != v4_pkts_sent:
                error_msg = "Hash-driven packet counter does not match expected value of %d" % v4_pkts_sent
                assert False, error_msg

        finally:
            print("Deleting entry")
            status = self.client.simple_meter_table_delete(sess_hdl,
                                                        0,
                                                        entry_hdl)

            # Clear counter
            cnt_v = hash_driven_counter_value_t(0, 0)
            self.client.counter_write_counter_0(sess_hdl, dev_tgt, stat_idx, cnt_v)

            status = self.client.simple_counter_table_delete(sess_hdl,
                                                             0,
                                                             entry_hdl_c)

class TestAll(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["hash_driven"])

    """ Basic meter test with Exact match table, indirectly addressed """
    def runTest(self):
        # Since this test sets the time to accurately test meters, it can only be run on the model
        if test_param_get('target') != "asic-model":
            return
        InstallAllDefaultEntries(self)

        sess_hdl = self.conn_mgr.client_init()
        advance_model_time_by_clocks(self, sess_hdl, 1000000)

        port = swports[0]
        meter_idx = 6   # Hash calculation is set to meter based on IPv4 protocol
        stat_idx = 4  # Hash calculation is set to count based on IPv4 version field
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
        v4_pkts_sent = 0

        global color_green
        global color_yellow
        global color_red

        match_spec = hash_driven_simple_meter_match_spec_t(ipv4Addr_to_i32("10.1.1.10"))

        action_spec = hash_driven_meter_action_action_spec_t(port,
                                                             macAddr_to_string("00:11:22:33:44:55"),
                                                             macAddr_to_string("00:43:53:11:22:33"))

        # CIR of 1Mbps, with burst size of 500 bytes (5 packets worth), PIR of
        # 2Mbps with burstsize of 1000 bytes (10 packets worth)

        meter_0_spec = hash_driven_bytes_meter_spec_t(1000, 8, 2000, 16, False)

        self.client.meter_set_meter_0(sess_hdl, dev_tgt, meter_idx, meter_0_spec)
        print("Adding entry")

        entry_hdl = self.client.simple_meter_table_add_with_meter_action(sess_hdl, dev_tgt, match_spec, action_spec)

        match_spec_c = hash_driven_simple_counter_match_spec_t(ipv4Addr_to_i32("10.1.1.10"))
        entry_hdl_c = self.client.simple_counter_table_add_with_cnt_action(sess_hdl, dev_tgt, match_spec_c)
        self.conn_mgr.complete_operations(sess_hdl)



        pkt = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                eth_src='00:22:22:22:22:22',
                                ip_src='200.1.2.3',
                                ip_dst='10.1.1.10',
                                ip_id=101,
                                ip_ttl=64,
                                tcp_sport = 1000,
                                with_tcp_chksum=False)

        exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                    eth_src='00:11:22:33:44:55',
                                    ip_src='200.1.2.3',
                                    ip_dst='10.1.1.10',
                                    ip_id=101,
                                    ip_ttl=63,
                                    tcp_sport = 1000,
                                    with_tcp_chksum=False)

       # Do not set the time for meters. Let it stay at zero, since we do not want
       # the model to queue color writes to MapRam which is done to simulate the mapram color write latency.

        try:
            # 10 packets would exhaust the committed burst size, but since color
            # based meters operate on EOP, the color output on the
            # packet will be seen only on the next packet. i,e.. at the
            # end of processing 11 packets, color would be updated, and
            # on the 12th packet, color will take effect on the packet

            for i in range (0, 11):
                send_packet(self, swports[0], pkt)
                v4_pkts_sent += 1
                # raw_input("Enter")
                verify_packet(self, exp_pkt, port)
            print("\nVerified green packets")

            # Now send one more packet to see the color change to yellow

            # Set the time to something > than the mapram color write latency (which is 13 clock cycles currently in the model)
            # This is so that the mapram color writes that are queued get executed.
            advance_model_time_by_clocks(self, sess_hdl, 20)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                        eth_src='00:11:22:33:44:55',
                                        ip_src='200.1.2.3',
                                        ip_dst='10.1.1.10',
                                        ip_id=101,
                                        ip_ttl=63,
                                        ip_tos=color_yellow,
                                        tcp_sport = 1000,
                                        with_tcp_chksum=False)

            verify_packet(self, exp_pkt, port)

            # So far we have sent 12 packets. Send 9 more to exhaust the
            # peak burst size and change the color.
            for i in range (0, 9):
                send_packet(self, swports[0], pkt)
                v4_pkts_sent += 1
                verify_packet(self, exp_pkt, port)
            print("\nVerified yellow packets")

            # Now send one more packet to see the packet color change to red
            exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                        eth_src='00:11:22:33:44:55',
                                        ip_src='200.1.2.3',
                                        ip_dst='10.1.1.10',
                                        ip_id=101,
                                        ip_ttl=63,
                                        ip_tos=color_red,
                                        tcp_sport = 1000,
                                        with_tcp_chksum=False)

            # Bump up the time so that the map-ram color write gets executed
            advance_model_time_by_clocks(self, sess_hdl, 20)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            verify_packet(self, exp_pkt, port)

            # Now set the time to a value such that both the buckets get
            # get filled on the next execution of the meter and subsequent
            # packet gets colored green

            advance_model_time_by_clocks(self, sess_hdl, 25000000)

            # Now send a packet, so that the color gets updated. However
            # the packet color still remains red.

            exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                        eth_src='00:11:22:33:44:55',
                                        ip_src='200.1.2.3',
                                        ip_dst='10.1.1.10',
                                        ip_id=101,
                                        ip_ttl=63,
                                        ip_tos=color_red,
                                        tcp_sport = 1000,
                                        with_tcp_chksum=False)


            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            verify_packet(self, exp_pkt, port)
            print("\nVerified red packets")

            advance_model_time_by_clocks(self, sess_hdl, 20)

            # Now, this packet should have a color of green
            exp_pkt = simple_tcp_packet(eth_dst='00:43:53:11:22:33',
                                        eth_src='00:11:22:33:44:55',
                                        ip_src='200.1.2.3',
                                        ip_dst='10.1.1.10',
                                        ip_id=101,
                                        ip_ttl=63,
                                        ip_tos=color_green,
                                        tcp_sport = 1000,
                                        with_tcp_chksum=False)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            verify_packets(self, exp_pkt, [port])
            print("\nVerified back to green packet")

            flags = hash_driven_counter_flags_t(1)
            cntr = self.client.counter_read_counter_0(sess_hdl, dev_tgt, stat_idx, flags)
            print("IPv4 packet count %s" % str(cntr))
            print("   packets = %s" % str(cntr.packets))
            if cntr.packets != v4_pkts_sent:
                error_msg = "Hash-driven packet counter does not match expected value of %d" % v4_pkts_sent
                assert False, error_msg

        finally:
            print("Deleting entry")
            status = self.client.simple_meter_table_delete(sess_hdl,
                                                        0,
                                                        entry_hdl)
            # Clear counter
            cnt_v = hash_driven_counter_value_t(0, 0)
            self.client.counter_write_counter_0(sess_hdl, dev_tgt, stat_idx, cnt_v)
            status = self.client.simple_counter_table_delete(sess_hdl,
                                                             0,
                                                             entry_hdl_c)

        print("closing session")
        status = self.conn_mgr.client_cleanup(sess_hdl)
        RemoveAllDefaultEntries(self)



class TestExecuteMeterWithOr(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["hash_driven"])

    """ Basic meter test with Exact match table, indirectly addressed """
    def runTest(self):
        # Since this test sets the time to accurately test meters, it can only be run on the model
        if test_param_get('target') != "asic-model":
            return
        InstallAllDefaultEntries(self)

        sess_hdl = self.conn_mgr.client_init()
        advance_model_time_by_clocks(self, sess_hdl, 1000000)

        port = swports[0]

        ETH_DST = '00:24:68:AC:DF:56'
        ETH_SRC = '00:22:22:22:22:22'

        METER_IDX = 77  # arbitrary number
        DST_ADDR = "192.168.50.3"

        SRC_PORT = 555

        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
        v4_pkts_sent = 0

        global color_green
        global color_yellow
        global color_red

        match_spec = hash_driven_meter_drop_match_spec_t(ipv4Addr_to_i32(DST_ADDR))

        action_spec = hash_driven_meter_or_action_action_spec_t(port, METER_IDX)

        # CIR of 1Mbps, with burst size of 500 bytes (5 packets worth), PIR of
        # 2Mbps with burstsize of 1000 bytes (10 packets worth)

        meter_1_spec = hash_driven_bytes_meter_spec_t(1000, 8, 2000, 16, False)

        self.client.meter_set_meter_1(sess_hdl, dev_tgt, METER_IDX, meter_1_spec)
        print("Adding entry")

        entry_hdl = self.client.meter_drop_table_add_with_meter_or_action(sess_hdl, dev_tgt, match_spec, action_spec)
        self.conn_mgr.complete_operations(sess_hdl)

        pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                eth_src=ETH_SRC,
                                ip_src='200.1.2.3',
                                ip_dst=DST_ADDR,
                                ip_id=101,
                                ip_ttl=64,
                                tcp_sport = SRC_PORT)

        exp_pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                    eth_src=ETH_SRC,
                                    ip_src='200.1.2.3',
                                    ip_dst=DST_ADDR,
                                    ip_id=101,
                                    ip_ttl=64,
                                    tcp_sport = SRC_PORT)

       # Do not set the time for meters. Let it stay at zero, since we do not want
       # the model to queue color writes to MapRam which is done to simulate the mapram color write latency.

        try:
            # 10 packets would exhaust the committed burst size, but since color
            # based meters operate on EOP, the color output on the
            # packet will be seen only on the next packet. i,e.. at the
            # end of processing 11 packets, color would be updated, and
            # on the 12th packet, color will take effect on the packet

            for i in range (0, 11):
                send_packet(self, swports[0], pkt)
                v4_pkts_sent += 1
                # raw_input("Enter")
                verify_packet(self, exp_pkt, port)
            print("\nVerified green packets")

            # Now send one more packet to see the color change to yellow

            # Set the time to something > than the mapram color write latency (which is 13 clock cycles currently in the model)
            # This is so that the mapram color writes that are queued get executed.
            advance_model_time_by_clocks(self, sess_hdl, 20)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            exp_pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                        eth_src=ETH_SRC,
                                        ip_src='200.1.2.3',
                                        ip_dst=DST_ADDR,
                                        ip_id=101,
                                        ip_ttl=64,
                                        tcp_sport = SRC_PORT)

            # This test ORs drop_ctl with the meter result, so yellow packets
            # should be dropped.
            verify_no_other_packets(self)
            # verify_packets(self, exp_pkt, [port])

            # So far we have sent 12 packets. Send 9 more to exhaust the
            # peak burst size and change the color.
            for i in range (0, 9):
                send_packet(self, swports[0], pkt)
                v4_pkts_sent += 1
                verify_no_other_packets(self)
                # verify_packets(self, exp_pkt, [port])
            print("\nAll yellow packets dropped")

            # Now send one more packet to see the packet color change to red
            exp_pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                        eth_src=ETH_SRC,
                                        ip_src='200.1.2.3',
                                        ip_dst=DST_ADDR,
                                        ip_id=101,
                                        ip_ttl=64,
                                        tcp_sport = SRC_PORT)

            # Bump up the time so that the map-ram color write gets executed
            advance_model_time_by_clocks(self, sess_hdl, 20)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1

            verify_no_other_packets(self)
            # verify_packets(self, exp_pkt, [port])

            # Now set the time to a value such that both the buckets get
            # get filled on the next execution of the meter and subsequent
            # packet gets colored green

            advance_model_time_by_clocks(self, sess_hdl, 25000000)

            # Now send a packet, so that the color gets updated. However
            # the packet color still remains red.

            exp_pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                        eth_src=ETH_SRC,
                                        ip_src='200.1.2.3',
                                        ip_dst=DST_ADDR,
                                        ip_id=101,
                                        ip_ttl=64,
                                        tcp_sport = SRC_PORT)


            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            # This test ORs drop_ctl with the meter result, so red packets
            # should be dropped.
            # verify_packets(self, exp_pkt, [port])
            verify_no_other_packets(self)
            print("\nVerified red packets dropped")

            advance_model_time_by_clocks(self, sess_hdl, 20)

            # Now, this packet should have a color of green
            exp_pkt = simple_tcp_packet(eth_dst=ETH_DST,
                                        eth_src=ETH_SRC,
                                        ip_src='200.1.2.3',
                                        ip_dst=DST_ADDR,
                                        ip_id=101,
                                        ip_ttl=64,
                                        tcp_sport = SRC_PORT)

            send_packet(self, swports[0], pkt)
            v4_pkts_sent += 1
            verify_packets(self, exp_pkt, [port])
            print("\nVerified back to green packet")


        finally:
            print("Deleting entry")
            status = self.client.meter_drop_table_delete(sess_hdl,
                                                         0,
                                                         entry_hdl)

        print("closing session")
        status = self.conn_mgr.client_cleanup(sess_hdl)
        RemoveAllDefaultEntries(self)
