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
import struct

from ptf import config
import ptf.testutils as testutils
import ptf.packet as packet
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as gc

dev_id = 0
p4_program_name = "tna_checksum"

logger = get_logger()
swports = get_sw_ports()


def pkt_with_calculated_fields(pkt):
    # This temporary workaround is for compatibility with Scapy
    return pkt.load_bytes(bytes(pkt)) if hasattr(pkt, "load_bytes") \
        else pkt.__class__(bytes(pkt))


class Ipv4ChksumTest(BfRuntimeTest):
    """@brief Base test for the program: verify that a correct checksum is not
    altered.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = ipkt

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)


class Ipv4ChksumErrNoUpdTest(BfRuntimeTest):
    """@brief Verify that the program identifies a bad IPv4 checksum.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)
            # Set incorrect checksum
            ipkt[packet.IP].chksum = 0x4321

            # The default behavior of the pipeline is to detect incorrect
            # checksums, but not to correct them. Therefore, we expect
            # to receive a packet with an incorrect checkum.
            epkt = testutils.simple_tcp_packet(eth_dst='00:00:de:ad:be:ef',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)
            epkt[packet.IP].chksum = 0x4321

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)


class Ipv4ChksumErrUpdTest(BfRuntimeTest):
    """@brief Check if the program identifies a bad IPv4 checksum and is able
    to correct it.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.checksum_upd_ipv4_tcp_udp",
            data_field_list_in=[gc.DataTuple(name="update", val=0x1)])

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)
            # Set incorrect checksum
            ipkt[packet.IP].chksum = 0x4321

            # We expect the program to correct the IPv4 checksum.
            epkt = testutils.simple_tcp_packet(eth_dst='00:00:de:ad:be:ef',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4TranslateNoUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address translation (snat).
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.snat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="update", val=0x0)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=False)
            # We expect the packet to have the same checksum as the packet
            # we sent in.
            # We need to calculate checksums, as they're not set anywhere in the object.
            ipkt_with_chksums = pkt_with_calculated_fields(ipkt)
            epkt[packet.IP].chksum = ipkt_with_chksums[packet.IP].chksum
            epkt[packet.TCP].chksum = ipkt_with_chksums[packet.TCP].chksum

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4TranslateUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address translation (snat) and check if 
    the program corrects the bad checksum for both, IPv4 and TCP.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.snat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class TcpTranslateNoUpdTest(BfRuntimeTest):
    """@brief Apply source TCP port translation (stpat).
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.stpat",
            data_field_list_in=[
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x0)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x4321,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=False)
            # We expect the packet to have the same TCP checksum as the packet
            # we sent in.
            # We need to calculate checksums, as they're not set anywhere in the object.
            ipkt_with_chksums = pkt_with_calculated_fields(ipkt)
            epkt[packet.TCP].chksum = ipkt_with_chksums[packet.TCP].chksum

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class TcpTranslateUpdTest(BfRuntimeTest):
    """@brief Apply source TCP port translation (stpat) and check if the program 
    corrects the bad TCP checksum.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.stpat",
            data_field_list_in=[
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x4321,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4TcpTranslateNoUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address and TCP port translation (sntpat).
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.sntpat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x0)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x4321,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=False)
            # We expect the packet to have the same TCP checksum as the packet
            # we sent in.
            # We need to calculate checksums, as they're not set anywhere in the object.
            ipkt_with_chksums = pkt_with_calculated_fields(ipkt)
            epkt[packet.IP].chksum = ipkt_with_chksums[packet.IP].chksum
            epkt[packet.TCP].chksum = ipkt_with_chksums[packet.TCP].chksum

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4TcpTranslateUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address and TCP port translation (sntpat) 
    and check if the program corrects the bad IPv4 and TCP checksum.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.sntpat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x1234,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            epkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               tcp_sport=0x4321,
                                               tcp_dport=0xabcd,
                                               with_tcp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class UdpTranslateNoUpdTest(BfRuntimeTest):
    """@brief Apply source UDP port translation (supat).
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.supat",
            data_field_list_in=[
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x0)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x4321,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=False)
            # We expect the packet to have the same TCP checksum as the packet
            # we sent in.
            # We need to calculate checksums, as they're not set anywhere in the object.
            ipkt_with_chksums = pkt_with_calculated_fields(ipkt)
            epkt[packet.UDP].chksum = ipkt_with_chksums[packet.UDP].chksum

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class UdpTranslateUpdTest(BfRuntimeTest):
    """@brief Apply source UDP port translation (supat) and check if the program 
    corrects the bad UDP checksum.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.supat",
            data_field_list_in=[
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x4321,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4UdpTranslateNoUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address and UDP port translation (snupat).
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.snupat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x0)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x4321,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=False)
            # We expect the packet to have the same TCP checksum as the packet
            # we sent in.
            # We need to calculate checksums, as they're not set anywhere in the object.
            ipkt_with_chksums = pkt_with_calculated_fields(ipkt)
            epkt[packet.IP].chksum = ipkt_with_chksums[packet.IP].chksum
            epkt[packet.UDP].chksum = ipkt_with_chksums[packet.UDP].chksum

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4UdpTranslateUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address and UDP port translation (snupat) 
    and check if the program corrects the bad IPv4 and UDP checksum.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        table_output_port = bfrt_info.table_get("SwitchIngress.output_port")
        action_data = table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        table_output_port.default_entry_set(target=target, data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = table_translate.make_data(
            action_name="SwitchIngress.snupat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        table_translate.default_entry_set(target=target, data=action_data)

        try:
            # Send packet with checksum, should be updated
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x4321,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=True)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])

            # Send packet without checksum, should not be updated
            ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='1.2.3.4',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x1234,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=False)

            epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                               eth_src='00:00:00:00:00:00',
                                               ip_src='4.3.2.1',
                                               ip_dst='100.99.98.97',
                                               ip_id=101,
                                               ip_ttl=64,
                                               udp_sport=0x4321,
                                               udp_dport=0xabcd,
                                               with_udp_chksum=False)

            testutils.send_packet(self, swports[0], ipkt)
            testutils.verify_packet(self, epkt, swports[1])
        finally:
            table_output_port.default_entry_reset(target)
            table_translate.default_entry_reset(target)


class Ipv4UdpTranslateSpecialUpdTest(BfRuntimeTest):
    """@brief Apply source IPv4 network address and UDP port translation (snupat) 
    and check if the program corrects the bad IPv4 and UDP checksum for special
    cases when the checksum is 0x0000 before or after the translation.
    """

    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get(p4_program_name)

        # Set default output port
        self.table_output_port = bfrt_info.table_get(
            "SwitchIngress.output_port")
        action_data = self.table_output_port.make_data(
            action_name="SwitchIngress.set_output_port",
            data_field_list_in=[gc.DataTuple(name="port_id", val=swports[1])])
        self.table_output_port.default_entry_set(target=self.target,
                                                 data=action_data)

        # Set a default entry in the rewrite table so all packets get
        # their checksum updated.
        self.table_translate = bfrt_info.table_get("SwitchIngress.translate")

        action_data = self.table_translate.make_data(
            action_name="SwitchIngress.snupat",
            data_field_list_in=[
                gc.DataTuple(name="src_addr", val=gc.ipv4_to_bytes("4.3.2.1")),
                gc.DataTuple(name="src_port", val=gc.to_bytes(0x4321, 2)),
                gc.DataTuple(name="update", val=0x1)
            ],
        )

        self.table_translate.default_entry_set(target=self.target,
                                               data=action_data)

        # build key and action data for udp special case
        # use ipv4 0.0.0.0 and sport 0 to generate zero hash
        translate_key = [
            self.table_translate.make_key([
                gc.KeyTuple("hdr.ipv4.src_addr", gc.ipv4_to_bytes("0.0.0.0"))
            ])
        ]
        translate_data = [
            self.table_translate.make_data(
                action_name="SwitchIngress.snupat",
                data_field_list_in=[
                    gc.DataTuple(name="src_addr",
                                 val=gc.ipv4_to_bytes("0.0.0.0")),
                    gc.DataTuple(name="src_port", val=gc.to_bytes(0x0, 2)),
                    gc.DataTuple(name="update", val=0x1)
                ],
            )
        ]
        self.table_translate.entry_add(self.target, translate_key,
                                       translate_data)

        # Test special cases
        # The UDP checksum value 0x0000 denotes the absence of the optional
        # checksum. Therefore, if the actual checksum value is 0x0000, the
        # checksum value is flipped to be 0xffff.

        # Find payload value for UDP checksum 0xffff before translation
        # Uncomment if to find new payload value when the input values have
        # changed.
        # for i in range(2**16):
        #     tpkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
        #                                 eth_src='00:00:00:00:00:00',
        #                                 ip_src='1.2.3.4',
        #                                 ip_dst='100.99.98.97',
        #                                 ip_id=101,
        #                                 ip_ttl=64,
        #                                 udp_sport=0x1234,
        #                                 udp_dport=0xabcd,
        #                                 udp_payload = struct.pack("!I", i),
        #                                 with_udp_chksum = True)
        #     # Force checksum calculation
        #     checksum = tpkt[scapy.UDP].__class__(str(tpkt[scapy.UDP])).chksum
        #     print("Packet: {}, payload: {}, checksum: {}".format(hash(tpkt),
        #                                                          hex(i),
        #                                                          hex(checksum)))
        #     if hex(checksum) == '0xffff':
        #         print("Found payload with hash value 0xffff: {}".format(i))
        #         break
        # Payload for UDP checksum 0xffff: 46530, actual UDP csum value: 0x0

        # Find payload value for UDP checksum 0xffff after translation
        # Uncomment if to find new payload value when the input values have
        # changed.
        # for i in range(2**16):
        #     tpkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
        #                                 eth_src='00:00:00:00:00:00',
        #                                 ip_src='4.3.2.1',
        #                                 ip_dst='100.99.98.97',
        #                                 ip_id=101,
        #                                 ip_ttl=64,
        #                                 udp_sport=0x4321,
        #                                 udp_dport=0xabcd,
        #                                 udp_payload = struct.pack("!I", i),
        #                                 with_udp_chksum = True)
        #     # Force checksum calculation
        #     checksum = tpkt[scapy.UDP].__class__(str(tpkt[scapy.UDP])).chksum
        #     print("Packet: {}, payload: {}, checksum: {}".format(hash(tpkt),
        #                                                          hex(i),
        #                                                          hex(checksum)))
        #     if hex(checksum) == '0xffff':
        #         print("Found payload with hash value 0xffff: {}".format(i))
        #         break
        # Payload for UDP checksum 0xffff: 33495, actual UDP csum value: 0x0

        # Send packet with checksum 0xffff, should be updated
        ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                           eth_src='00:00:00:00:00:00',
                                           ip_src='1.2.3.4',
                                           ip_dst='100.99.98.97',
                                           ip_id=101,
                                           ip_ttl=64,
                                           udp_sport=0x1234,
                                           udp_dport=0xabcd,
                                           udp_payload=struct.pack(
                                               "!I", 46530),
                                           with_udp_chksum=True)

        epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                           eth_src='00:00:00:00:00:00',
                                           ip_src='4.3.2.1',
                                           ip_dst='100.99.98.97',
                                           ip_id=101,
                                           ip_ttl=64,
                                           udp_sport=0x4321,
                                           udp_dport=0xabcd,
                                           udp_payload=struct.pack(
                                               "!I", 46530),
                                           with_udp_chksum=True)

        ipkt["UDP"].chksum = 65535
        testutils.send_packet(self, swports[0], ipkt)
        testutils.verify_packet(self, epkt, swports[1])

        # Send packet with checksum, that should be updated to be 0x0000,
        # and therefore requires the P4 program to flips the bits to be 0xffff
        ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                           eth_src='00:00:00:00:00:00',
                                           ip_src='0.0.0.0',
                                           ip_dst='100.99.98.97',
                                           ip_id=101,
                                           ip_ttl=64,
                                           udp_sport=0x100,
                                           udp_dport=0xabcd,
                                           udp_payload=struct.pack(
                                               "!I", 33495),
                                           with_udp_chksum=True)

        epkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                           eth_src='00:00:00:00:00:00',
                                           ip_src='0.0.0.0',
                                           ip_dst='100.99.98.97',
                                           ip_id=101,
                                           ip_ttl=64,
                                           udp_sport=0x0,
                                           udp_dport=0xabcd,
                                           udp_payload=struct.pack(
                                               "!I", 33495),
                                           with_udp_chksum=True)
        #Add the difference of udp_sport(0x100) to support p4 pipeline
        #udp checksum to calculate the new checksum which totals to 0
        ipkt["UDP"].chksum = 0xfeff
        epkt["UDP"].chksum = 65535
        testutils.send_packet(self, swports[0], ipkt)
        testutils.verify_packet(self, epkt, swports[1])

    def tearDown(self):
        self.table_output_port.default_entry_reset(self.target)
        self.table_translate.default_entry_reset(self.target)
        self.table_translate.entry_del(self.target, [])
        BfRuntimeTest.tearDown(self)
