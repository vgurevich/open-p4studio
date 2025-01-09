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

from deparse_zero.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from collections import defaultdict

swports = get_sw_ports()

class TestUdp(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["deparse_zero"])

    """
    Basic test that adds a new 16-byte header after UDP,
    15 bytes of zeros and a final byte with a value of 1.
    This test is to check that the compiler's not_parsed optimization whereby
    it repeatedly deparses the same 8-bit container holding a zero value
    is working.
    """
    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

        # Set default entries for tables
        print("Installed default entries")
        self.client.table_1_set_default_action_do_nothing(sess_hdl, dev_tgt)

        # This test will send one UDP packet, with the expected packet having
        # a new 16-byte header following the UDP header.

        # Setup table entries
        DST_ADDR = '128.0.0.7'
        PKT_LEN = 76
        INPUT_PORT = swports[0]
        OUTPUT_PORT = swports[1]

        t1_match_spec = deparse_zero_table_1_match_spec_t(ipv4Addr_to_i32(DST_ADDR))
        t1_action_spec = deparse_zero_set_p_action_spec_t(OUTPUT_PORT)
        t1_entry_hdl = self.client.table_1_table_add_with_set_p(sess_hdl, dev_tgt, t1_match_spec, t1_action_spec)

        self.conn_mgr.complete_operations(sess_hdl)
        print("Finished adding entries")

        payload_length = 32
        pkt_len = 14 + 20 + 8 + payload_length
        ip_len = 20 + 8 + payload_length

        new_hdr_len = 16

        orig_payload = ""
        for b in range(0, payload_length):
            orig_payload += "\xff"

        new_payload = ""
        for b in range(0, new_hdr_len):
            if b < new_hdr_len - 1:
                new_payload += "\x00"
            else:
                new_payload += "\x01"

        new_payload += orig_payload

        pkt_1 = simple_udp_packet(pktlen=pkt_len,
                                  eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR,
                                  ip_id=101,
                                  ip_ttl=64,
                                  udp_sport = 1000,
                                  with_udp_chksum=False,
                                  udp_payload=orig_payload)

        exp_pkt_1 = simple_udp_packet(pktlen=pkt_len+new_hdr_len,
                                      eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR,
                                      ip_id=101,
                                      ip_ttl=64,
                                      udp_sport = 1000,
                                      with_udp_chksum=False,
                                      udp_payload=new_payload)

        try:
            send_packet(self, INPUT_PORT, pkt_1)
            try:
                verify_packets(self, exp_pkt_1, [OUTPUT_PORT])
                print("First packet verified!")
            except:
                print("First packet failed!")
                raise

        finally:
            print("Removing entries")
            self.client.table_1_table_delete(sess_hdl, 0, t1_entry_hdl)
            self.conn_mgr.complete_operations(sess_hdl)
            self.conn_mgr.client_cleanup(sess_hdl)
