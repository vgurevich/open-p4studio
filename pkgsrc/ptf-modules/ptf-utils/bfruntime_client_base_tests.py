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


import ptf
from ptf.base_tests import BaseTest
import ptf.testutils as testutils
import p4testutils.misc_utils as misc_utils
import p4testutils.bfrt_utils as bfrt_utils

import grpc
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc

import random
import math

from collections import namedtuple

logger = misc_utils.get_logger()


# This is common to all tests. setUp() is invoked at the beginning of the test
# and tearDown() is called at the end, regardless of whether the test is passed
# or failed/errored.
class BfRuntimeTest(BaseTest):
    def __init__(self):
        BaseTest.__init__(self)
        self.device_id = 0
        self.client_id = 0
        self._swports = []
        self.bfrt_info = None
        self.p4_name = ""

    def tearDown(self):
        if not self.interface.is_independent:
            self.interface.tear_down_stream()
        BaseTest.tearDown(self)

    def setUp(self, client_id=0, p4_name=None, notifications=None,
              perform_bind=True, perform_subscribe=True):
        """@brief Set up connection to gRPC server and bind
        @param client_id Client ID
        @param p4_name Name of P4 program. If none is given,
        then the test performs a bfrt_info_get() and binds to the first
        P4 that comes as part of the bfrt_info_get()
        @param notifications A Notifications object.
        If you need to disable any notifications, then do the below as example,
        gc.Notifications(enable_learn=False)
        else default value is sent as below
            enable_learn = True
            enable_idletimeout = True
            enable_port_status_change = True
            enable_entry_active = True
        @param perform_bind Set this to false if binding is not required
        @param perform_subscribe Set this to false if client does not need to
        subscribe for any notifications
        """
        if perform_bind and not perform_subscribe:
            raise RuntimeError("perform_bind must be equal to perform_subscribe")

        BaseTest.setUp(self)
        self.bfrt_info = None
        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()

        grpc_addr = ':'.join([testutils.test_param_get("grpc_server", default='localhost'),
                              testutils.test_param_get("bfrt_grpc_port", default='50052')] )
        
        self.interface = gc.ClientInterface(grpc_addr, client_id=client_id,
                device_id=0, notifications=notifications,
                perform_subscribe=perform_subscribe)

        # If p4_name wasn't specified, then perform a bfrt_info_get and set p4_name
        # to it
        if not p4_name:
            self.bfrt_info = self.interface.bfrt_info_get()
            p4_name = self.bfrt_info.p4_name_get()

        # Get device configration information
        if self.bfrt_info is None:
            self.bfrt_info = self.interface.bfrt_info_get()
        if testutils.test_param_get("arch") in ["tofino", "tofino2", "tofino3"]:
            self.dev_configuration = bfrt_utils.DevConfiguration(self.bfrt_info)

        # Set forwarding pipeline config (For the time being we are just
        # associating a client with a p4). Currently the grpc server supports
        # only one client to be in-charge of one p4.
        if perform_bind:
            self.interface.bind_pipeline_config(p4_name)

    def swports(self, idx):
        if idx >= len(self._swports):
            self.fail("Index {} is out-of-bound of port map".format(idx))
            return None
        return self._swports[idx]

    # Helper functions to make writing BfRuntime PTF tests easier.
    IpRandom = namedtuple('ip_random', 'ip prefix_len mask')
    MacRandom = namedtuple('mac_random', 'mac mask')

    def insert_table_entry_performance(self, target, table_name,
                           key_fields=[], action_names=[], data_fields=[]):
        """ Insert a new table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (List of (name, value, [mask]) tuples).
            @param action_name : List of Action names.
            @param data_fields : List of (List of (name, value) tuples).
        """
        # TODO: This is a temporary function which takes in a list of keyfields, actionnames
        #       and datafields. Moving forward when we restructure this client, we should
        #       remove this API and make insert_table_entry take in a list of all the
        #       aforementioned things
        assert(len(key_fields) == len(action_names) == len(data_fields))

        req = bfruntime_pb2.WriteRequest()
        req.client_id = self.client_id
        self.cpy_target(req, target)

        try:
            self.stub.Write(self.entry_write_req_make(req, table_name, key_fields,
            action_names, data_fields, bfruntime_pb2.Update.INSERT))
        except grpc.RpcError as e:
            status_code = e.code()
            if status_code != grpc.StatusCode.UNKNOWN:
                logger.info("The error code returned by the server for Performace test is not UNKNOWN, which indicates some error might have occured while trying to add the entries")
                gc.print_grpc_error(e)
                return 0
            else:
                # Retrieve the performace rate (entries per second) encoded in the details
                error_details = e.details()
                error_details_list = error_details.split()
                rate = float(error_details_list.pop())
                return rate

    def generate_random_ip_list(self, num_entries, seed=None):
        """ Generate random, unique, non overalapping IP address/mask """
        unique_keys = {}
        ip_list = []
        i = 0
        if seed is not None:
            random.seed(seed)
        duplicate = False
        while( i < num_entries) :
            duplicate = False
            ip = "%d.%d.%d.%d" % (random.randint(1,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            p_len = random.randint(math.ceil(math.log(num_entries, 2)),32)
            # Check if the dst_ip, p_len is already present in the list
            ipAddrbytes = ip.split('.')
            ipnumber = (int(ipAddrbytes[0]) << 24) + (int(ipAddrbytes[1]) << 16) + (int(ipAddrbytes[2]) << 8) + int(ipAddrbytes[3])
            mask = 0xffffffff
            mask = (mask << (32 - p_len)) & (0xffffffff)
            if ipnumber & mask in unique_keys:
                continue
            for _, each in unique_keys.items():
                each_ip = each[0]
                each_mask = each[1]
                if ipnumber & each_mask == each_ip & each_mask:
                    duplicate = True
                    break
            if duplicate:
                continue
            duplicate = False
            unique_keys[ipnumber & mask] = (ipnumber, mask)
            ip_list.append(self.IpRandom(ip, p_len, mask))
            i += 1
        return ip_list

    def generate_random_mac_list(self, num_entries, seed=None):
        """ Generate random, unique, non overalapping MAC address/mask """
        unique_keys = {}
        mac_list = []
        i = 0
        if seed is not None:
            random.seed(seed)
        duplicate = False
        while( i < num_entries) :
            duplicate = False
            mac = "%02x:%02x:%02x:%02x:%02x:%02x" % (random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            mask = "%02x:%02x:%02x:%02x:%02x:%02x" % (random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            # Check if the dst_ip, p_len is already present in the list
            macAddrBytes = mac.split(':')
            macMaskBytes = mask.split(":")

            macnumber = 0
            masknumber = 0

            for x in range(len(macAddrBytes)):
                macnumber = macnumber | int(macAddrBytes[x], 16) << (8 * (len(macAddrBytes) - x - 1))
                masknumber = masknumber | int(macAddrBytes[x], 16) << (8 * (len(macAddrBytes) - x - 1))

            if macnumber & masknumber in unique_keys:
                continue

            for _, each in unique_keys.items():
                each_mac = each[0]
                each_mask = each[1]
                if macnumber & each_mask == each_mac & each_mask:
                    duplicate = True
                    break
            if duplicate:
                continue
            duplicate = False

            unique_keys[macnumber & masknumber] = (macnumber, masknumber)
            mac_list.append(self.MacRandom(mac, mask))
            i += 1
        return mac_list
