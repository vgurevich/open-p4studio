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

from parser_error.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from collections import OrderedDict

dev_id = 0

swports = get_sw_ports()

def make_ipv4_addr(value):
    b0 = value & 0xff
    b1 = (value >> 8) & 0xff
    b2 = (value >> 16) & 0xff
    b3 = (value >> 24) & 0xff
    return "%d.%d.%d.%d" % (b3, b2, b1, b0)



class TestNotEnoughData(pd_base_tests.ThriftInterfaceDataPlane):
    """ Basic test """
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["parser_error"])

    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

        INPUT_PORT = swports[0]
        OUTPUT_PORT = swports[1]

        # -------------------------------------------------
        # Default actions
        # -------------------------------------------------

        port_table_action_spec = parser_error_set_p_action_spec_t(OUTPUT_PORT)
        self.client.port_table_set_default_action_set_p(sess_hdl, dev_tgt, port_table_action_spec)

        self.client.table_1_set_default_action_do_nothing(sess_hdl, dev_tgt)
        # No need to set default action for drop_table
        self.conn_mgr.complete_operations(sess_hdl)

        # -------------------------------------------------
        # Table entries
        # -------------------------------------------------
        FILLER_LENGTH = 22
        FULL_PAYLOAD_LENGTH = 8
        INPUT_A = 0x0000
        INPUT_B = 0x0000
        INPUT_C = 0x0000
        INPUT_D = 0x0000

        # 4 bytes of FCS will be added by the model.
        # To exercise packets that are too short, account for fact that parser will parse into the FCS.
        payload_lengths = [8, 3, 2, 1, 0, 8, 8, 8]
        RESULT_D = 0x0001

        all_handles = []

        table_1_action_spec = parser_error_set_byte_d_action_spec_t(RESULT_D)
        table_1_match_spec = parser_error_table_1_match_spec_t(INPUT_A, INPUT_B, INPUT_C, INPUT_D)
        table_1_entry_hdl = self.client.table_1_table_add_with_set_byte_d(sess_hdl, dev_tgt, table_1_match_spec, table_1_action_spec)
        all_handles.append(table_1_entry_hdl)

        self.conn_mgr.complete_operations(sess_hdl)

        # -------------------------------------------------
        # -------------------------------------------------

        def _clean_up():
            print("Removing entries")
            for hdl in all_handles:
                self.client.table_1_table_delete(sess_hdl, 0, hdl)
            self.conn_mgr.client_cleanup(sess_hdl)


        # -------------------------------------------------
        # Send packets
        # -------------------------------------------------
        working_payload = ""
        for b in range(0, FILLER_LENGTH + FULL_PAYLOAD_LENGTH):
            if b == (FILLER_LENGTH + FULL_PAYLOAD_LENGTH - 1):
                working_payload += "\x01"
            else:
                working_payload += "\x00"

        failure_string = ""

        try:
            for payload_length in payload_lengths:

                pkt_len = 14 + 20 + 8 + FILLER_LENGTH + payload_length

                print("\nSending packet with payload length %d." % payload_length)

                tx_payload = "\x00"*(FILLER_LENGTH + payload_length)
                pkt = simple_udp_packet(pktlen=pkt_len,
                                        with_udp_chksum=False,
                                        udp_payload=tx_payload.encode())

                exp_pkt = simple_udp_packet(pktlen=pkt_len,
                                            with_udp_chksum=False,
                                            udp_payload=working_payload.encode())

                send_packet(self, INPUT_PORT, pkt)

                if payload_length == FULL_PAYLOAD_LENGTH:
                    failure_string = "Expected packet not received on port %d." % OUTPUT_PORT
                    failure_string += "\n%s" % format_packet(exp_pkt)
                    verify_packet(self, exp_pkt, OUTPUT_PORT)
                else:
                    failure_string = "Packet was not dropped."
                    verify_no_other_packets(self)
        except:
            print("\n%s" % failure_string)
            _clean_up()
            raise

        _clean_up()
