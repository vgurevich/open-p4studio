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
import socket
import struct
import math

from ptf import config, mask
import ptf.testutils as testutils
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest

dev_id = 0
p4_program_name = "tna_random"

logger = get_logger()
swports = get_sw_ports()


def ip2int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


def stdev(vals):
    n = len(vals)
    if n <= 1:
        return 0.0

    mean = sum(vals) / float(len(vals))
    sd = 0.0
    for val in vals:
        sd += (float(val) - mean) ** 2
    sd = math.sqrt(sd / float(n - 1))
    return sd


class TestRandom(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        BfRuntimeTest.setUp(self, client_id, p4_program_name)

    def runTest(self):
        try:
            # expected mean and stdev from 32b unsigned uniform random
            exp_mean = (pow(2, 32) - 1) / 2.0
            exp_std = (pow(2, 32) - 1) / math.sqrt(12)

            # compute mean and std from samples
            num_samples = 1000
            rand_vals = []
            print("\nInject %s packets and get random value in srcip field." % num_samples)
            print("It may take time with model.\n")
            for i in range(num_samples):
                ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                                   eth_src='22:33:44:55:66:77',
                                                   ip_src='1.2.3.4',
                                                   ip_dst='100.99.98.97',
                                                   ip_id=101,
                                                   ip_ttl=64,
                                                   udp_sport=0x1234,
                                                   udp_dport=0xabcd)

                testutils.send_packet(self, swports[0], ipkt)
                (rcv_dev, rcv_port, rcv_pkt, pkt_time) = \
                    testutils.dp_poll(self, dev_id, swports[0], timeout=2)
                nrcv = bytes2hex(rcv_pkt)[52:60]  # IP.src
                # print("\n### Received pkt :\n")
                # nrcv.show2()
                # hexdump(nrcv)
                rand_val = int(nrcv, 16)  # convert hex value to int
                rand_vals.append(rand_val)
                # print("32b Random value written in ipv4 src ip : " + str(rand_val))

            # compare mean and std
            mean = sum(rand_vals) / float(len(rand_vals))
            std = stdev(rand_vals)
            print(("Expected Mean : " + str(exp_mean)))
            print(("Observed Mean : " + str(mean)))
            print(("Expected Stdev : " + str(exp_std)))
            print(("Observed Stdev : " + str(std)))

            assert abs(mean - exp_mean) / float(exp_mean) < 0.1
            assert abs(std - exp_std) / float(exp_std) < 0.1

        finally:
            pass
