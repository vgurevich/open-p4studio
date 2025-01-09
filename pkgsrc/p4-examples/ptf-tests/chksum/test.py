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

import pd_base_tests

from ptf import config
from ptf.testutils import *
from p4testutils.misc_utils import *
from ptf.thriftutils import *

from chksum.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *


dev_id = 0

swports = get_sw_ports()

class TestIPv4Checksum(pd_base_tests.ThriftInterfaceDataPlane):
    """ Basic test """
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["chksum"])

    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

        INPUT_PORT = swports[0]
        OUTPUT_PORT = swports[1]

        egress_port_action_spec = chksum_set_egress_port_action_spec_t(OUTPUT_PORT)
        self.client.egress_port_set_default_action_do_nothing(sess_hdl, dev_tgt)


        egress_port_match_spec = chksum_egress_port_match_spec_t(INPUT_PORT)
        ep_hdl = self.client.egress_port_table_add_with_set_egress_port(sess_hdl,
                                                                        dev_tgt,
                                                                        egress_port_match_spec,
                                                                        egress_port_action_spec)

        self.conn_mgr.complete_operations(sess_hdl)


        # ----------------------------------
        # Send one IPv6 UDP packet
        # ----------------------------------

        DST_MAC = "aa:bb:cc:dd:ee:ff"
        SRC_MAC = "11:22:33:44:55:66"

        IPV4_SRC_ADDR = '128.252.0.7'
        IPV4_DST_ADDR = '192.168.10.15'

        UDP_SRC_PORT = 0x0101
        UDP_DST_PORT = 0x1010

        pkt = simple_udp_packet(pktlen = 76,
                                eth_dst = DST_MAC,
                                eth_src = SRC_MAC,
                                ip_src = IPV4_SRC_ADDR,
                                ip_dst = IPV4_DST_ADDR,
                                ip_ttl = 64,
                                udp_sport = UDP_SRC_PORT,
                                udp_dport = UDP_DST_PORT,
                                with_udp_chksum = False)



        try:
            send_packet(self, INPUT_PORT, pkt)
            verify_packet(self, pkt, OUTPUT_PORT)
        finally:
            self.client.egress_port_table_delete(sess_hdl, 0, ep_hdl)
