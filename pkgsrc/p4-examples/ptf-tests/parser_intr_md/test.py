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

import time
import sys, pdb
import logging
import unittest
import random
import pd_base_tests
from ptf import config
from ptf.testutils import *
from p4testutils.misc_utils import *
from ptf.thriftutils import *
import os
from parser_intr_md.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from devport_mgr_pd_rpc.ttypes import *
this_dir = os.path.dirname(os.path.abspath(__file__))

swports = get_sw_ports()

class parser_intr_md_pvs(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["parser_intr_md"])
        self.server_hdl = None
        self.fabric_hdl = None

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
        print()
        print('Configuring the devices')
        self.sess_hdl = self.conn_mgr.client_init()
        self.dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
        self.client.fwd_packet_set_default_action_fwd_drop(
            self.sess_hdl, self.dev_tgt)

        # Add fowrding actions. If ipv4 is valid, it is a packet from server
        # port, send it to fabric port.
        action_spec = parser_intr_md_fwd_to_fabric_action_spec_t(swports[3])
        match_spec = parser_intr_md_fwd_packet_match_spec_t(
            ipv4_valid=1, fabric_header_valid=0)
        self.fabric_hdl = self.client.fwd_packet_table_add_with_fwd_to_fabric(
            self.sess_hdl, self.dev_tgt, match_spec, action_spec)
        # If fabric_header is valid, it is a packet from fabric port, send it to
        # server port
        srv_action_spec = parser_intr_md_fwd_to_server_action_spec_t(swports[4])
        match_spec = parser_intr_md_fwd_packet_match_spec_t(
            ipv4_valid=0, fabric_header_valid=1)
        self.server_hdl = self.client.fwd_packet_table_add_with_fwd_to_server(
            self.sess_hdl, self.dev_tgt, match_spec, srv_action_spec)
        self.conn_mgr.complete_operations(self.sess_hdl)


    def runTest(self):
        pvs_handles = []
        status = self.client.pvs_pvs_fabric_port_set_property(self.sess_hdl, self.dev_tgt.dev_id, pvs_property_t.PVS_PARSER_SCOPE, pvs_property_value_t.PVS_SCOPE_SINGLE_PARSER, hex_to_byte(0xFF))
        self.conn_mgr.complete_operations(self.sess_hdl)

        fabric_ports = [swports[1], swports[3]]
        fabric_port_parsers = set()
        for p in fabric_ports:
            parser_id = self.devport_mgr.devport_mgr_get_parser_id(0, p)
            fabric_port_parsers.add( parser_id )
            print("Port", p, "maps to parser", parser_id)

        for parser_id in fabric_port_parsers:
            print("Programming parser", parser_id)
            prsr_tgt = DevParserTarget_t(self.dev_tgt.dev_id, hex_to_byte(0xFF), hex_to_i16(0xFFFF), parser_id)
            for port in fabric_ports:
                print("  Adding", port, "to PVS")
                pvs_handles.append(self.client.pvs_pvs_fabric_port_entry_add(
                    self.sess_hdl, prsr_tgt, port, 0xFFFF))

        self.conn_mgr.complete_operations(self.sess_hdl)

        pkt = simple_tcp_packet(eth_dst='00:11:22:33:44:10',
                                eth_src='00:11:22:33:44:11',
                                ip_dst='10.0.0.1',
                                ip_src='10.0.0.2')

        print("Test pkt from port %d (fabric) -> port %d" % (swports[1], swports[4]))
        ing_port = swports[1]
        eg_port = swports[4]
        send_packet(self, ing_port, pkt)
        verify_packet(self, pkt, eg_port)

        print("Test pkt from port %d (server) -> port %d" % (swports[2], swports[3]))
        ing_port = swports[2]
        eg_port = swports[3]
        send_packet(self, ing_port, pkt)
        verify_packet(self, pkt, eg_port)

        for hdl in pvs_handles:
            self.client.pvs_pvs_fabric_port_entry_delete(
                self.sess_hdl, self.dev_tgt.dev_id, hdl)

        self.client.fwd_packet_table_delete(self.sess_hdl, 0, self.server_hdl)
        self.client.fwd_packet_table_delete(self.sess_hdl, 0, self.fabric_hdl)
        print('Test parser_intr_md with pvs .... Pass')
