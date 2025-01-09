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


import binascii
import codecs
import logging
import random
import six
import time

from ptf import config
from ptf.testutils import *

def get_logger(logger_name='Test'):
    logger = logging.getLogger(logger_name)
    if not len(logger.handlers):
        logger.addHandler(logging.StreamHandler())
    return logger


logger = get_logger()


def get_sw_ports():
    swports = list()
    setup_random()
    num_pipes = int(test_param_get('num_pipes'))

    if config["platform"] == 'nn':
        for device, ports, ifname in config["device_sockets"]:
            for port in ports:
                logger.info("Device %d Port %d IF %s", device, port, ifname)
                swports.append(port)
    else:
        for device, port, ifname in config["interfaces"]:
            if port_to_pipe(port) < num_pipes:
                swports.append(port)
    swports.sort()
    random.shuffle(swports)
    return swports


def setup_random(seed_val=None):
    """ Seed the random number generator.  If the seed was specified on the
    commandline (part of the test params) use it.  If the caller is requesting
    a specific seed then use that.  If neither then set a seed based on the
    current time. """
    if seed_val is None:
        if test_param_get('test_seed') != 'None':
            seed_val = int(test_param_get('test_seed'))
    if seed_val is None:
        seed_val = int(time.time())
    random.seed(seed_val)

    logger = logging.getLogger('Test')
    if not len(logger.handlers):
        logger.addHandler(logging.StreamHandler())
    logger.info("Seed is: %d", seed_val)
    return seed_val


def make_port(pipe, local_port):
    """ Given a pipe and a port within that pipe construct the full dev_port number. """
    return (pipe << 7) | local_port


def port_to_pipe(port):
    """ Given a dev_port return the pipe it belongs to. """
    return port >> 7


def port_to_pipe_local_port(port):
    """ Given a dev_port return its ID within a pipe. """
    return port & 0x7F


def bytes2hex(raw_packet):
    """ Converts given packet from 'bytes' (binary data) to 'hex' format. """
    if not isinstance(raw_packet, six.binary_type):
        raw_packet = six.ensure_binary(raw_packet, encoding="utf-8")

    return six.ensure_str(binascii.hexlify(raw_packet), encoding="utf-8")


def always_extend_packet_len(pkt, pktlen):
    return pkt / codecs.decode(
        "".join(["%02x" % (x % 256) for x in range(pktlen - len(pkt))]),
        "hex")


def extend_packet_len(pkt, pktlen):
    if (pktlen - len(pkt)) > 0:
        return always_extend_packet_len(pkt, pktlen)


def mask_set_do_not_care_packet(mask_obj, hdr_type, field_name):
    pkt_obj = mask_obj.exp_pkt
    if hdr_type not in pkt_obj:
        mask_obj.valid = False
        return

    hdr_offset = mask_obj.size - len(pkt_obj[hdr_type])
    offset = 0
    bitwidth = 0
    for f_name, f_size in pkt_obj[hdr_type].args_details():
        if f_name == field_name:
            bitwidth = f_size
            break
        else:
            offset += f_size
    mask_obj.set_do_not_care(hdr_offset * 8 + offset, bitwidth)

