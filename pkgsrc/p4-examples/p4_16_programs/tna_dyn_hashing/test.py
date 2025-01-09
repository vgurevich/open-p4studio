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
import random

from ptf import config
from ptf.thriftutils import *
from ptf.testutils import *
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as client
from collections import namedtuple
import pprint as pp
import time
import crcmod

logger = get_logger()
swports = get_sw_ports()

HashFieldSlice = namedtuple('HashFieldSlice', 'name start_bit length order')
HashFieldSlice.__new__.__defaults__ = (None, None, 0)

field_width_dict = {
            "hdr.ipv4.protocol": 8,
            "hdr.ipv4.src_addr": 32,
            "hdr.ipv4.dst_addr": 32,
            "hdr.tcp.src_port":  16,
            "hdr.tcp.dst_port":  16}

def garble_bits(val, index_set):
    """ @brief Given a value and a set of indices, this function inverts the bits in the value
    at all the indices in the set
    """
    mask = 0
    if index_set is not None and len(index_set) > 0:
        for index in index_set:
            mask |= (1 << index)
    return (val ^ mask)

def transform_bytes(bytes_, range_list):
    return garble_bits(client.bytes_to_int(bytes_), range_list)

def create_mask(a, b):
    ret = 0
    for i in range(a, b+1):
        ret |= 1 << i
    return ret

class HashFieldInfo():
    def __init__(self, field_slice):
        self.name = field_slice.name
        self.canon_name = field_slice.name
        self.order = field_slice.order

        self.fs = 0
        self.fl = self.fw = field_width_dict[field_slice.name]

        self.ss = 0 if field_slice.start_bit is None else field_slice.start_bit
        self.sl = self.fl - self.ss if field_slice.length is None or \
                field_slice.length > self.fl - self.ss else field_slice.length

    def __str__(self):
        msg = ""
        msg += "Name: " + self.name + "\n"
        msg += "Order: " + str(self.order) + "\n"
        msg += "FieldStart: "   + str(self.fs) + "\n"
        msg += "FieldLength: "  + str(self.fl) + "\n"
        msg += "FieldWidth: "   + str(self.fw) + "\n"
        msg += "SliceStart: "   + str(self.ss) + "\n"
        msg += "SliceLength: "  + str(self.sl) + "\n\n"
        return msg

# Using a slice_tuple list and dict of (canon names -> field_width)
# Returns a dict of (canon_name -> list of bit positions, eg. [0,1,7,8])
# the list of ranges are the opposite if anti is true
def rangeBuilder(slice_tuple_list, anti=False, default_range_dict=None):
    field_info_tuples = [(slice_.name, HashFieldInfo(slice_))\
            for slice_ in slice_tuple_list if slice_.order is not None and slice_.order > 0 ]

    ret_dict = {}
    flattened_ret_dict = {}
    for name, hash_field_info in field_info_tuples:
        canon_name = name
        ret_dict.setdefault(canon_name, []).append(
                    (hash_field_info.fs + hash_field_info.ss,
                             hash_field_info.fs + hash_field_info.ss + hash_field_info.sl - 1))

    for k, v in ret_dict.items():
        if not anti:
            flattened_ret_dict[k] = [i for pair in v for i in range(pair[0], pair[1]+1)]
        else:
            partial = set([i for pair in v for i in range(pair[0], pair[1]+1)])
            complete = set(default_range_dict[k])
            flattened_ret_dict[k] = list(complete - partial)

    # At this point we have a dict of (canon_field_names -> list of ranges)
    # However, we would have missed out on fields which were not in the slice tuple list
    # at all. For non-anti case, these have been masked out so changing those bits won't
    # matter. For anti case, we want to include that range because we want to mess with
    # those bits and not see the hash change
    if default_range_dict is not None:
        for canon_name, default_range_list in default_range_dict.items():
            if canon_name not in flattened_ret_dict:
                if anti:
                    flattened_ret_dict[canon_name] = default_range_list
                else:
                    flattened_ret_dict[canon_name] = []
    return flattened_ret_dict

class DynHashingBaseTest(BfRuntimeTest):
    def setUp(self):
        client_id = 0
        p4_name = "tna_dyn_hashing"
        setup_random()
        BfRuntimeTest.setUp(self, client_id, p4_name)
        self.setup_tables()

    def setup_tables(self):
        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_dyn_hashing")
        self.hash_config_table = bfrt_info.table_get("c_hash.configure")
        self.hash_algo_table = bfrt_info.table_get("c_hash.algorithm")
        self.hash_compute_table = bfrt_info.table_get("c_hash.compute")

        self.forward_table = bfrt_info.table_get("SwitchIngress.forward")
        self.sel_table = bfrt_info.table_get(
            "SwitchIngress.example_action_selector")
        self.action_table = bfrt_info.table_get(
            "SwitchIngress.example_action_selector_ap")
        self.sel_get_mem_table = bfrt_info.table_get(
            "SwitchIngress.example_action_selector_get_member")

    def get_default_slice_list(self, hash_table):
        self.hash_config_table.default_entry_reset(self.target)
        resp = hash_table.default_entry_get(self.target)
        default_range_dict = {}
        for data_in, _ in resp:
            slice_tuple_list = []
            for field_name, inner_list in data_in.to_dict().items():
                if field_name == "action_name" or field_name == "is_default_entry":
                    continue
                # Only one inner_dict will be present since full hash slice is present in
                # default default_entry
                inner_dict = inner_list[0]
                slice_tuple_list.append(
                    HashFieldSlice(
                        name = str(field_name),
                        start_bit = inner_dict["start_bit"],
                        length = inner_dict["length"],
                        order = inner_dict["order"]))
        return slice_tuple_list

    def create_compute_key(self, compute_table, sip, dip, sport, dport):
        return compute_table.make_key([client.KeyTuple("hdr.ipv4.src_addr", sip),
                client.KeyTuple("hdr.ipv4.dst_addr", dip),
                client.KeyTuple("hdr.tcp.src_port", sport),
                client.KeyTuple("hdr.tcp.dst_port", dport)])

    def create_hash_data(self, table, slice_tuples):
        hash_field_slice_dict = {}
        for field, start_bit, length, order in slice_tuples:
            if field in hash_field_slice_dict:
                hash_field_slice_dict[field].append((order, start_bit, length))
            else:
                hash_field_slice_dict[field] = [(order, start_bit, length)]

        hash_field_slice_list = []
        for name, slice_list in hash_field_slice_dict.items():
            inner_tuple = []
            for slice_ in slice_list:
                inner_tuple.append(
                        {"order": client.DataTuple("order", slice_[0]),
                         "start_bit":client.DataTuple("start_bit", slice_[1]),
                         "length":client.DataTuple("length", slice_[2])
                         })
            hash_field_slice_list.append(client.DataTuple(name, container_arr_val = inner_tuple))
        return table.make_data(hash_field_slice_list)

    def program_ap_as(self, group_id, ig_port, eg_ports):
        members = []
        max_grp_size = 100
        member_status = [True] * len(eg_ports)

        for i, port in enumerate(eg_ports):
            # Create a new member for each port with the port number as the id.
            self.action_table.entry_add(
                self.target,
                [self.action_table.make_key([client.KeyTuple('$ACTION_MEMBER_ID',
                                                    port)])],
                [self.action_table.make_data([client.DataTuple('port', port)],
                                        'SwitchIngress.hit')])

            members.append(port)
            member_status[i] = True

        # Add the new member to the selection table.
        # Adding all members at the same time along with the max group size
        self.sel_table.entry_add(
            self.target,
            [self.sel_table.make_key([client.KeyTuple('$SELECTOR_GROUP_ID', group_id)])],
            [self.sel_table.make_data([client.DataTuple('$MAX_GROUP_SIZE', max_grp_size),
                                  client.DataTuple('$ACTION_MEMBER_ID', int_arr_val=members),
                                  client.DataTuple('$ACTION_MEMBER_STATUS',
                                               bool_arr_val=member_status)])])
        self.forward_table.entry_add(
            self.target,
            [self.forward_table.make_key([client.KeyTuple('ig_intr_md.ingress_port',
                                                 ig_port)])],
            [self.forward_table.make_data([client.DataTuple('$SELECTOR_GROUP_ID',
                                                 group_id)])])

    def send_pkt_and_get_hash(self, pkt, ttl):
        pkt["IP"].ttl = ttl
        exp_pkt = pkt
        send_packet(self, self.ig_port, pkt)

        (rcv_dev, rcv_port, rcv_pkt, pkt_time) = dp_poll(self, 0)
        nrcv = pkt.load_bytes(rcv_pkt)
        return client.bytes_to_int(client.mac_to_bytes(str(nrcv["Ether"].src))) & 0xFFFFFFFF

    def send_pkt_and_get_hash_and_rcv_port(self, pkt, ttl):
        pkt["IP"].ttl = ttl
        exp_pkt = pkt
        send_packet(self, self.ig_port, pkt)

        (rcv_dev, rcv_port, rcv_pkt, pkt_time) = dp_poll(self, 0)
        nrcv = pkt.load_bytes(rcv_pkt)
        return client.bytes_to_int(client.mac_to_bytes(str(nrcv["Ether"].src))) & 0xFFFFFFFF, rcv_port

    def tearDown(self):
        self.hash_config_table.default_entry_reset(self.target)
        self.hash_algo_table.default_entry_reset(self.target)

        self.forward_table.entry_del(self.target)
        self.sel_table.entry_del(self.target)
        self.action_table.entry_del(self.target)

        BfRuntimeTest.tearDown(self)

"""@brief Testing masking feature of Dynamic Hash Config with P4 defined
    field slices
"""
class DynHashingMaskingTest(DynHashingBaseTest):
    def run_cfg_test(self, range_dict, num_tries, equal):
        for i in range(num_tries):
            pkt = simple_tcp_packet()
            pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(random.randint(0, 2**32 -1), 4))
            pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(random.randint(0, 2**32 -1), 4))
            pkt["TCP"].sport = random.randint(0, 2**16 -1)
            pkt["TCP"].dport = random.randint(0, 2**16 -1)
            hash_content = self.send_pkt_and_get_hash(pkt, 1)

            transformed_sip = transform_bytes(client.ipv4_to_bytes(pkt["IP"].src), range_dict["hdr.ipv4.src_addr"])
            transformed_dip = transform_bytes(client.ipv4_to_bytes(pkt["IP"].dst), range_dict["hdr.ipv4.dst_addr"])
            transformed_sport = transform_bytes(client.to_bytes(pkt["TCP"].sport, 2), range_dict["hdr.tcp.src_port"])
            transformed_dport = transform_bytes(client.to_bytes(pkt["TCP"].dport, 2),  range_dict["hdr.tcp.dst_port"])

            pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(transformed_sip, 4))
            pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(transformed_dip, 4))
            pkt["TCP"].sport = transformed_sport
            pkt["TCP"].dport = transformed_dport
            hash_content2 = self.send_pkt_and_get_hash(pkt, 1)
            if equal:
                self.assertEqual(hash_content, hash_content2)
            else:
                self.assertNotEqual(hash_content, hash_content2)

    def run_all_hash_tests(self, slice_tuple_list, num_tries=10):
        data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
        self.hash_config_table.default_entry_set(self.target, data)
        # Test to change bits in packet addresses and ports randomly in ranges which have been included
        # Hash value should change
        logger.info("Testing that hash changes")
        range_dict = rangeBuilder(slice_tuple_list, False, self.default_range_dict)
        self.run_cfg_test(range_dict, num_tries, False)

        # Test to change bits in packet addresses and ports randomly in ranges which have been excluded
        # Hash value should not change
        logger.info("Testing that hash doesn't change")
        anti_range_dict = rangeBuilder(slice_tuple_list, True, self.default_range_dict)
        self.run_cfg_test(anti_range_dict, num_tries, True)

    def runTest(self):
        num_tries = 10

        logger.info("=============== Testing Dyn Hashing Configuration ===============")
        logger.info("Programming Action profile and selector")

        self.target = client.Target(device_id=0, pipe_id=0xffff)
        group_id = 1
        eg_ports = swports[2:]
        self.ig_port = swports[1]

        self.program_ap_as(group_id, self.ig_port, eg_ports)

        # Create the ranges of different fields which are originally part of the
        # default hash config
        self.default_range_dict = rangeBuilder(self.get_default_slice_list(self.hash_config_table))

        logger.info("Turning off first 8b for dstIP, last 8b of srcIP")
        slice_tuple_list = []
        # Note that if start bit is not set, then backend takes default 0.
        # If length is not set, then default is taken till end of field
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",              length=24,  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=8,             order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",                           order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",                           order=4))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        logger.info("Masking entire dst_addr out and splitting src_addr into slices")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=3, length=4,  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=9, length=9,  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",                         order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",                          order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",                          order=5))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        logger.info("Swizzling order of some slices")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=3, length=4,  order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=9, length=9,  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",                         order=5))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",                          order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",                          order=4))
        self.run_all_hash_tests(slice_tuple_list, num_tries)


        # -ve tests
        # Overlapping slices
        logger.info("-ve test:: Overlapping slices")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=3, length=4,  order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=1, length=9,  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",                         order=5))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",                          order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",                          order=4))
        error = False
        try:
            data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
            self.hash_config_table.default_entry_set(self.target, data)
        except:
            error = True
        if not error:
            logger.error("Expected failure but no exception was raised")
            assert(0)

        # start_bit > field width is not allowed. Note that length can exceed possible length of slice.
        # It will be truncated to max possible if it exceeds
        logger.info("-ve test:: start_bit > field_width")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=3, length=4,  order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=37, length=53, order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",                         order=5))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",                          order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",                          order=4))
        error = False
        try:
            data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
            self.hash_config_table.default_entry_set(self.target, data)
        except:
            error = True
        if not error:
            logger.error("Expected failure but no exception was raised")
            assert(0)


"""@brief Testing symmetric hashing
"""
class DynHashingSymmetricTest(DynHashingBaseTest):
    def sym_swap(self, in_pkt, sym_pairs):
        def get_correct_num(hash_field, pkt):
            sip = client.bytes_to_int(client.ipv4_to_bytes(pkt["IP"].src))
            dip = client.bytes_to_int(client.ipv4_to_bytes(pkt["IP"].dst))
            sport = pkt["TCP"].sport
            dport = pkt["TCP"].dport
            if hash_field.canon_name == "hdr.ipv4.src_addr":
                return sip
            elif hash_field.canon_name == "hdr.ipv4.dst_addr":
                return dip
            elif hash_field.canon_name == "hdr.tcp.src_port":
                return sport
            elif hash_field.canon_name == "hdr.tcp.dst_port":
                return dport
            assert 0

        # Swaps bits in bit-ranges between 2 nums - lhs_num and rhs_num.
        # eg. lhs_num = 0x12abcdef ; rhs_num = 0x12345678 ;
        #     lhs-end:start [7:0]  ; rhs-end:start [15:8] ;
        # Final result ==
        #     lhs_num = 0x12abcd56 ; rhs_num = 0x1234ef78
        # Assumptions : both ranges are equal and within limits
        def swap_nums(lhs_num, lhs_start, lhs_end, rhs_num, rhs_start, rhs_end):
            tmp_lhs = lhs_num
            tmp_rhs = rhs_num
            lhs_mask = create_mask(lhs_start, lhs_end)
            rhs_mask = create_mask(rhs_start, rhs_end)
            tmp_lhs = (tmp_lhs & lhs_mask) >> lhs_start
            tmp_rhs = (tmp_rhs & rhs_mask) >> rhs_start

            # Zeroing out the bits corresponding
            # to the masks in the nums
            lhs_num &= (lhs_mask ^ 0xffffffff)
            rhs_num &= (rhs_mask ^ 0xffffffff)

            # Putting the extracted bits in the correct
            # position in 2 nums
            lhs_num |= ((tmp_rhs << lhs_start))
            rhs_num |= ((tmp_lhs << rhs_start))
            return lhs_num, rhs_num

        # Swaps bits in 2 non-overlapping ranges in a number
        # eg. lhs_num = 0x12abcdef
        #     lhs-end:start [7:0]  ; rhs-end:start [15:8] ;
        # Final result ==
        #     lhs_num = 0x12abefcd
        # Assumptions : both ranges are equal, within limits
        #               and non-overlapping
        def swap_within_num(lhs_num, lhs_start, lhs_end, rhs_start, rhs_end):
            tmp_lhs = lhs_num
            tmp_rhs = lhs_num
            lhs_mask = create_mask(lhs_start, lhs_end)
            rhs_mask = create_mask(rhs_start, rhs_end)
            tmp_lhs = (tmp_lhs & lhs_mask) >> lhs_start
            tmp_rhs = (tmp_rhs & rhs_mask) >> rhs_start

            # Zeroing out the bits corresponding
            # to the masks in the num
            lhs_num &= (lhs_mask ^ 0xffffffff)
            lhs_num &= (rhs_mask ^ 0xffffffff)

            # Putting the extracted bits in the correct
            # position in num
            lhs_num |= ((tmp_rhs << lhs_start))
            lhs_num |= ((tmp_lhs << rhs_start))
            return lhs_num

        def set_in_pkt(pkt, num, hash_field):
            if hash_field.canon_name == "hdr.ipv4.src_addr":
                pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(num, 4))
            elif hash_field.canon_name == "hdr.ipv4.dst_addr":
                pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(num, 4))
            elif hash_field.canon_name == "hdr.tcp.src_port":
                pkt["TCP"].sport = num
            elif hash_field.canon_name == "hdr.tcp.dst_port":
                pkt["TCP"].dport = num
            else:
                assert 0
            return pkt

        pkt = in_pkt
        for lhs, rhs in sym_pairs:
            lhs_num = get_correct_num(lhs, pkt)
            rhs_num = get_correct_num(rhs, pkt)
            if lhs.canon_name != rhs.canon_name:
                lhs_num, rhs_num = swap_nums(lhs_num,
                                            lhs.fs + lhs.ss,
                                            lhs.fs + lhs.ss + lhs.sl -1,
                                            rhs_num,
                                            rhs.fs + rhs.ss,
                                            rhs.fs + rhs.ss + rhs.sl -1)
                set_in_pkt(pkt, lhs_num, lhs)
                set_in_pkt(pkt, rhs_num, rhs)
            else:
                lhs_num = swap_within_num(lhs_num,
                                        lhs.fs + lhs.ss,
                                        lhs.fs + lhs.ss + lhs.sl -1,
                                        rhs.fs + rhs.ss,
                                        rhs.fs + rhs.ss + rhs.sl -1)
                set_in_pkt(pkt, lhs_num, lhs)
        return pkt

    def run_symmetric_test(self, sym_pairs, num_tries, equal):
        for i in range(num_tries):
            pkt = simple_tcp_packet()
            sip =  client.bytes_to_ipv4(client.to_bytes(random.randint(0, 2**32 -1), 4))
            dip = client.bytes_to_ipv4(client.to_bytes(random.randint(0, 2**32 -1), 4))
            sport = random.randint(0, 2**16 -1)
            dport = random.randint(0, 2**16 -1)
            pkt["IP"].src = sip
            pkt["IP"].dst = dip
            pkt["TCP"].sport = sport
            pkt["TCP"].dport = dport
            hash_content = self.send_pkt_and_get_hash(pkt, 1)
            exp_pkt = pkt.copy()

            # Go over every sym_pair and swap approrpiately
            new_pkt = self.sym_swap(pkt, sym_pairs)
            hash_content2 = self.send_pkt_and_get_hash(new_pkt, 1)
            if equal:
                self.assertEqual(hash_content, hash_content2)
            else:
                self.assertNotEqual(hash_content, hash_content2)
            # ensure that both pkts are different
            self.assertNotEqual(exp_pkt, new_pkt)

    def get_sym_pairs(self, slice_tuple_list):
        pair_list = []
        for i in range(len(slice_tuple_list)):
            for j in range(i+1, len(slice_tuple_list)):
                if slice_tuple_list[i].order == slice_tuple_list[j].order:
                    pair_list.append((HashFieldInfo(slice_tuple_list[i]),
                                      HashFieldInfo(slice_tuple_list[j])))
                    break
        return pair_list

    def run_all_hash_tests(self, slice_tuple_list, num_tries, equal=True):
        # Test to swap symmetric field values and check that hash doesn't
        # change
        # Get all the symmetric pairs from the slice tuple list
        sym_pairs = self.get_sym_pairs(slice_tuple_list)
        if equal:
            data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
            self.hash_config_table.default_entry_set(self.target, data)

        self.run_symmetric_test(sym_pairs, num_tries, equal)

    def runTest(self):
        num_tries = 10

        logger.info("=============== Testing Dyn Hashing Symmetric ===============")
        logger.info("Programming Action profile and selector")

        self.target = client.Target(device_id=0, pipe_id=0xffff)
        group_id = 1
        eg_ports = swports[2:]
        self.ig_port = swports[1]

        self.program_ap_as(group_id, self.ig_port, eg_ports)

        logger.info("Making (sip,dip) and (sport,dport) symmetric")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=3))
        self.run_all_hash_tests(slice_tuple_list, num_tries)
        slice_sym_list = slice_tuple_list

        logger.info("Making some hash slices as symmetric across different fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=0, length=5, order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=7, length=4, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=15, length=3, order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=1, length=4, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=6, length=5, order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=15, length=3, order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",  start_bit=0, length=None, order=5))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",  start_bit=0, length=None, order=5))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        logger.info("Making some hash slices as symmetric across same fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=0, length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=7, length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=15, length=3, order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=1, length=3, order=7))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=6, length=7, order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=15, length=7, order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",  start_bit=0, length=None, order=5))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",  start_bit=0, length=None, order=5))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        # Configuring with non-symmetric but running testcases with symmetric order to start
        # -ve testing
        logger.info("-ve test:: Swapping should create unequal hashes with non-sym cfg")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
        self.hash_config_table.default_entry_set(self.target, data)
        self.run_all_hash_tests(slice_sym_list, num_tries, equal=False)

        # More than 2 fields of same order not possible
        logger.info("-ve test:: More than 2 fields with same order")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=0,  length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=5,  length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=10, length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        error = False
        try:
            data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
            self.hash_config_table.default_entry_set(self.target, data)
        except:
            error = True
        if not error:
            logger.error("Expected failure but no exception was raised")
            assert(0)

        # slices/fields of different lengths cannot be made symmetric
        logger.info("-ve test:: slices/fields of different lengths")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=0,  length=3, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=5,  length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        error = False
        try:
            data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
            self.hash_config_table.default_entry_set(self.target, data)
        except:
            error = True
        if not error:
            logger.error("Expected failure but no exception was raised")
            assert(0)


"""@brief Testing Hash compute and Action Selector Get member
"""
class DynHashingHashComputeTest(DynHashingBaseTest):
    def run_compute_test(self, num_tries):
        for i in range(num_tries):
            pkt = simple_tcp_packet()
            sip_n = random.randint(0, 2**32 -1)
            dip_n = random.randint(0, 2**32 -1)
            sport = random.randint(0, 2**16 -1)
            dport = random.randint(0, 2**16 -1)

            pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(sip_n, 4))
            pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(dip_n, 4))
            pkt["TCP"].sport = sport
            pkt["TCP"].dport = dport
            key = self.create_compute_key(self.hash_compute_table,
                    sip_n,
                    dip_n,
                    sport, dport)
            hash_content, rcv_port = self.send_pkt_and_get_hash_and_rcv_port(pkt, 1)

            resp = self.hash_compute_table.entry_get(self.target, [key])

            hash_content2 = next(resp)[0].to_dict()["hash_value"]
            # Need to shift by 32 since we are using [63:32] in P4
            # hash_content2 >>= 32
            self.assertEqual(hex(hash_content), hex(int(hash_content2)))

            resp = self.sel_get_mem_table.entry_get(self.target,
                    [self.sel_get_mem_table.make_key(
                        [client.KeyTuple("$SELECTOR_GROUP_ID", 1),
                         client.KeyTuple("hash_value", hash_content)])])
            mbr_id = next(resp)[0].to_dict()["$ACTION_MEMBER_ID"]
            self.assertEqual(rcv_port, mbr_id)


    def run_all_hash_tests(self, slice_tuple_list, num_tries):
        # program in the config requested
        data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
        self.hash_config_table.default_entry_set(self.target, data)

        self.run_compute_test(num_tries)

    def runTest(self):
        num_tries = 5

        logger.info("=============== Testing Dyn Hashing Compute ===============")
        logger.info("Programming Action profile and selector")

        self.target = client.Target(device_id=0, pipe_id=0xffff)
        group_id = 1
        eg_ports = swports[2:]
        self.ig_port = swports[1]

        self.program_ap_as(group_id, self.ig_port, eg_ports)

        logger.info("Enabling all fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        logger.info("Rearranging orders of fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=2))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

        logger.info("Slicing up fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=0, length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=7, length=5, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr", start_bit=15, length=3, order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=1, length=3, order=7))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=6, length=7, order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr", start_bit=15, length=7, order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",  start_bit=0, length=None, order=0))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",  start_bit=0, length=None, order=5))
        self.run_all_hash_tests(slice_tuple_list, num_tries)

class UserDefAlgo:
    def __init__(self, name, poly, reverse, init, final_xor, hash_bit_width):
        self.name = name
        self.poly = poly
        self.reverse = reverse
        self.init = init
        self.final_xor = final_xor
        self.hash_bit_width = hash_bit_width

"""@brief Testing hash Algorithm change
"""
class DynHashingHashAlgorithmTest(DynHashingBaseTest):

    def get_algo_list(self):
        algo_list = []
        algo_list.append(UserDefAlgo("crc_64_jones", 0xad93d23594c935a9, True, 0xffffffffffffffff, 0, 64))
        algo_list.append(UserDefAlgo("crc_64_we", 0x42f0e1eba9ea3693, False, 0xffffffffffffffff, 0xffffffffffffffff, 64))
        algo_list.append(UserDefAlgo("crc_64", 0x42f0e1eba9ea3693, False, 0, 0, 64))
        algo_list.append(UserDefAlgo("crc_32_bzip2", 0x04c11db7, False, 0xffffffff, 0xffffffff, 32))
        algo_list.append(UserDefAlgo("x_25", 0x1021, True, 0xffff, 0xffff, 16))
        algo_list.append(UserDefAlgo("crc_8_itu", 0x07, False, 0x55, 0x55, 8))
        algo_list.append(UserDefAlgo("crc_16_mcrf4xx", 0x1021, True, 0xffff, 0, 16))
        return algo_list

    def get_crcmod_str(self, algo_name):
        algo_n_l = list(algo_name)
        for index, c in enumerate(algo_n_l):
            if c == '_':
                algo_n_l[index] = '-'
        return "".join(algo_n_l)

    def run_userdef_algorithm_test(self, num_tries):
        for algo in self.get_algo_list():
            data_algo = self.hash_algo_table.make_data([
                                            client.DataTuple('msb', bool_val=False),
                                            client.DataTuple('extend', bool_val=False),
                                            client.DataTuple('polynomial', algo.poly),
                                            client.DataTuple('reverse', bool_val=algo.reverse),
                                            client.DataTuple('init', algo.init),
                                            client.DataTuple('final_xor', algo.final_xor),
                                            client.DataTuple('hash_bit_width', algo.hash_bit_width),
                                            ],
                                              "user_defined")
            self.hash_algo_table.default_entry_set(self.target, data_algo)
            logger.info("user algo: %s", algo.name)

            for i in range(num_tries):
                pkt = simple_tcp_packet()
                sip_n = random.randint(0, 2**32 -1)
                dip_n = random.randint(0, 2**32 -1)
                sport = random.randint(0, 2**16 -1)
                dport = random.randint(0, 2**16 -1)

                pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(sip_n, 4))
                pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(dip_n, 4))
                pkt["TCP"].sport = sport
                pkt["TCP"].dport = dport
                hash_content = self.send_pkt_and_get_hash(pkt, 1)

                crc_func = crcmod.mkCrcFun((algo.poly | (1 << algo.hash_bit_width)), initCrc=algo.init,
                        rev=algo.reverse)
                s = bytes(client.to_bytes(sip_n, 4) + client.to_bytes(dip_n, 4) +
                        client.to_bytes(sport, 2) + client.to_bytes(dport, 2))
                hash_content2 = crc_func(s)
                # The xorOut param in mkCrcFun doesn't work as
                # intended. It is applied directly to the init value in the beginning by crcmod.
                # so applying final_xor manually instead
                hash_content2 ^= algo.final_xor
                hash_content2 &= 0xffffffff
                self.assertEqual(hex(hash_content), hex(int(hash_content2)))

    def run_predef_algorithm_test(self, num_tries):
        # crc64 and some other variations do not match in Tofino and crcmod. Use user-def
        for algo in ["crc_8", "crc_16", "crc_32"]:
            data_algo = self.hash_algo_table.make_data([ client.DataTuple('msb', bool_val=False),
                                            client.DataTuple('extend', bool_val=False),
                                            client.DataTuple('algorithm_name', str_val=algo)],
                                              "pre_defined")
            self.hash_algo_table.default_entry_set(self.target, data_algo)
            logger.info("predefined algo: %s", algo)

            for i in range(num_tries):
                pkt = simple_tcp_packet()
                sip_n = random.randint(0, 2**32 -1)
                dip_n = random.randint(0, 2**32 -1)
                sport = random.randint(0, 2**16 -1)
                dport = random.randint(0, 2**16 -1)

                pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(sip_n, 4))
                pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(dip_n, 4))
                pkt["TCP"].sport = sport
                pkt["TCP"].dport = dport
                hash_content = self.send_pkt_and_get_hash(pkt, 1)

                crc_func = crcmod.predefined.mkCrcFun(self.get_crcmod_str(algo))
                s = bytes(client.to_bytes(sip_n, 4) + client.to_bytes(dip_n, 4) +
                        client.to_bytes(sport, 2) + client.to_bytes(dport, 2))
                hash_content2 = crc_func(s) & 0xffffffff
                self.assertEqual(hex(hash_content), hex(int(hash_content2)))


    def run_all_hash_tests(self, slice_tuple_list, num_tries):
        # program in the config requested
        data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
        self.hash_config_table.default_entry_set(self.target, data)

        self.run_predef_algorithm_test(num_tries)
        self.run_userdef_algorithm_test(num_tries)

    def runTest(self):
        num_tries = 5

        logger.info("=============== Testing Dyn Hashing Algorithm ===============")
        logger.info("Programming Action profile and selector")

        self.target = client.Target(device_id=0, pipe_id=0xffff)
        group_id = 1
        eg_ports = swports[2:]
        self.ig_port = swports[1]

        self.program_ap_as(group_id, self.ig_port, eg_ports)

        logger.info("Enabling all fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        self.run_all_hash_tests(slice_tuple_list, num_tries)


"""@brief Testing hash rotation change
"""
class DynHashingHashRotationTest(DynHashingBaseTest):

    def get_crcmod_str(self, algo_name):
        algo_n_l = list(algo_name)
        for index, c in enumerate(algo_n_l):
            if c == '_':
                algo_n_l[index] = '-'
        return "".join(algo_n_l)
    def right_rotate(self, n, d):
        return (n & 0xFFFFF00000000)|(((n & 0x3FFFFFFF) << d)| ((n >> 30) & ((2**d)-1)))

    def run_predef_algorithm_test(self, num_tries):
        rotate = 2
        # crc64 and some other variations do not match in Tofino and crcmod. Use user-def
        for algo in ["crc_32"]:
            data_algo = self.hash_algo_table.make_data([ client.DataTuple('msb', bool_val=False),
                                            client.DataTuple('rotate', rotate),
                                            client.DataTuple('extend', bool_val=True),
                                            client.DataTuple('algorithm_name', str_val=algo)],
                                              "pre_defined")
            self.hash_algo_table.default_entry_set(self.target, data_algo)
            logger.info("predefined algo: %s", algo)

            for i in range(num_tries):
                pkt = simple_tcp_packet()
                sip_n = random.randint(0, 2**32 -1)
                dip_n = random.randint(0, 2**32 -1)
                sport = random.randint(0, 2**16 -1)
                dport = random.randint(0, 2**16 -1)

                pkt["IP"].src = client.bytes_to_ipv4(client.to_bytes(sip_n, 4))
                pkt["IP"].dst = client.bytes_to_ipv4(client.to_bytes(dip_n, 4))
                pkt["TCP"].sport = sport
                pkt["TCP"].dport = dport
                hash_content = self.send_pkt_and_get_hash(pkt, 1)

                crc_func = crcmod.predefined.mkCrcFun(self.get_crcmod_str(algo))
                s = bytes(client.to_bytes(sip_n, 4) + client.to_bytes(dip_n, 4) +
                        client.to_bytes(sport, 2) + client.to_bytes(dport, 2))
                hash_content2 = int(crc_func(s) & 0xfffffffffffff)
                hash_content2 = self.right_rotate(hash_content2, rotate)
                print("Expected : ",hex(hash_content2) , " received :", hex(hash_content))
                self.assertEqual(hex(hash_content), hex(int(hash_content2)))
                print("Matched!!")


    def run_all_hash_tests(self, slice_tuple_list, num_tries):
        # program in the config requested
        data = self.create_hash_data(self.hash_config_table, slice_tuple_list)
        self.hash_config_table.default_entry_set(self.target, data)

        self.run_predef_algorithm_test(num_tries)

    def runTest(self):
        num_tries = 5

        logger.info("=============== Testing Dyn Hashing Rotation ===============")
        logger.info("Programming Action profile and selector")

        self.target = client.Target(device_id=0, pipe_id=0xffff)
        group_id = 1
        eg_ports = swports[2:]
        self.ig_port = swports[1]

        self.program_ap_as(group_id, self.ig_port, eg_ports)

        logger.info("Enabling all fields")
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",  order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",  order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",   order=3))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",   order=4))
        self.run_all_hash_tests(slice_tuple_list, num_tries)
