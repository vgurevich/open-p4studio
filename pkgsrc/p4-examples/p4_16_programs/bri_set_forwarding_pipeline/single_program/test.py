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

from ptf import config
import ptf.testutils as testutils
from ptf.testutils import *
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest, BaseTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc
import random

logger = logging.getLogger('Test')
if not len(logger.handlers):
    logger.addHandler(logging.StreamHandler())

base_pick_path = testutils.test_param_get("base_pick_path")
binary_name = testutils.test_param_get("arch")
if binary_name is not "tofino2" and binary_name is not "tofino":
    assert 0, "%s is unknown arch" % (binary_name)

if not base_pick_path:
    base_pick_path = "install/share/" + binary_name + "pd/"

base_put_path = testutils.test_param_get("base_put_path")
if not base_put_path:
    base_put_path = "install/share/" + binary_name + "pd/forwarding"

logger.info("\nbase_put_path=%s \nbase_pick_path=%s", base_pick_path, base_put_path)

swports = get_sw_ports()


def create_path_bf_rt(base_path, p4_name_to_use):
    return base_path + "/" + p4_name_to_use + "/bf-rt.json"


def create_path_context(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/context.json"


def create_path_tofino(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/" + binary_name + ".bin"


def VerifyReadRegisters(self, register_name_lo, register_name_hi, resp, register_value_lo, register_value_hi,
                        data_dict):
    # since the table is symmetric and exists only in stage 0, we know that the response data is going to have
    # 8 data fields (2 (hi and lo) * 4 (num pipes) * 1 (num_stages)). bfrt_server returns all (4) register values
    # corresponding to one field id followed by all (4) register values corresponding to the other field id

    num_pipes = int(testutils.test_param_get('num_pipes'))
    for i in range(num_pipes):
        value_lo = data_dict[register_name_lo][i]
        value_hi = data_dict[register_name_hi][i]
        logger.info("Register Lo Expected Value (%s) : Read value (%s)", str(register_value_lo[i]), str(value_lo))
        logger.info("Register Hi Expected Value (%s) : Read value (%s)", str(register_value_hi[i]), str(value_hi))
        if data_dict[register_name_lo][i] != register_value_lo[i]:
            logger.info("Register field lo didn't match with the read value")
            assert (0)
        if data_dict[register_name_hi][i] != register_value_hi[i]:
            logger.info("Register field hi didn't match with the read value")
            assert (0)

def beginWarmInit(test, program_name):
    p4_name_to_put = p4_name_to_pick = p4_name = program_name
    profile_name_to_put = "pipe"
    profile_name_to_pick = "pipe"

    logger.info("Sending Verify and warm_init_begin  for %s", p4_name_to_put)
    action = bfruntime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_WARM_INIT_BEGIN
    success = test.interface.send_set_forwarding_pipeline_config_request(
        action,
        base_put_path,
        [gc.ForwardingConfig(p4_name_to_put,
                             create_path_bf_rt(base_pick_path, p4_name_to_pick),
                             [gc.ProfileInfo(profile_name_to_put,
                                             create_path_context(base_pick_path, p4_name_to_pick,
                                                                 profile_name_to_pick),
                                             create_path_tofino(base_pick_path, p4_name_to_pick,
                                                                profile_name_to_pick),
                                             [0, 1, 2, 3])]
                             )])
    if not success:
        raise RuntimeError("Failed to setFwd")

def endWarmInit(test):
    action = bfruntime_pb2.SetForwardingPipelineConfigRequest.WARM_INIT_END
    logger.info("Sending warm_init_end")
    test.interface.send_set_forwarding_pipeline_config_request(action)

class SingleProgramNoReplayTest(BfRuntimeTest):
    '''
    Case 1
    This test does the following
    setUp()
    1. Client Subscribes to server
    2. Send a VERIFY_AND_WARM_INIT_BEGIN_AND_END SetForwardingPipelineConfig msg
    3. Wait for WARM_INIT_FINISHED
    4. BIND to program
    runTest()
    1. run the test -> insert entries
    '''

    '''
    Taken from IndirectRegisterIteratorTest
    '''

    def setUp(self):
        client_id = 0

        p4_name_to_put = p4_name_to_pick = p4_name = "tna_register"
        profile_name_to_put = profile_name_to_pick = "pipe"

        # Setup and don't perform bind
        BfRuntimeTest.setUp(self, client_id, perform_bind=False)
        # Send a Verify_and_warm_init_begin_and_end
        logger.info("Sending Verify and warm_init_begin and warm_init_end for %s", p4_name_to_put)
        action = bfruntime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_WARM_INIT_BEGIN_AND_END
        success = self.interface.send_set_forwarding_pipeline_config_request(
            action,
            base_put_path,
            [gc.ForwardingConfig(p4_name_to_put,
                                 create_path_bf_rt(base_pick_path, p4_name_to_pick),
                                 [gc.ProfileInfo(profile_name_to_put,
                                                 create_path_context(base_pick_path, p4_name_to_pick,
                                                                     profile_name_to_pick),
                                                 create_path_tofino(base_pick_path, p4_name_to_pick,
                                                                    profile_name_to_pick),
                                                 [0, 1, 2, 3])]
                                 )])
        if not success:
            raise RuntimeError("Failed to setFwd")

        self.p4_name = p4_name
        self.interface.bind_pipeline_config("tna_register")

    def runTest(self):
        seed = random.randint(1, 65535)
        logger.info("Seed used for RegisterTest is %d", seed)
        random.seed(seed)

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get("tna_register")
        test_reg_table = bfrt_info.table_get("SwitchIngress.test_reg")

        register_idx = random.randint(0, 500)
        register_value_hi = random.randint(0, 1000)
        register_value_lo = random.randint(0, 1000)
        logger.info("Register value hi %s", str(register_value_hi))
        logger.info("Register value lo %s", str(register_value_lo))
        register_value_hi_arr = {}
        register_value_lo_arr = {}
        num_pipes = int(testutils.test_param_get('num_pipes'))

        for i in range(num_pipes):
            register_value_hi_arr[i] = register_value_hi
            register_value_lo_arr[i] = register_value_lo

        target = gc.Target(device_id=0, pipe_id=0xffff)
        test_reg_table.entry_add(
            target,
            [test_reg_table.make_key([gc.KeyTuple('$REGISTER_INDEX', register_idx)])],
            [test_reg_table.make_data(
                [gc.DataTuple('SwitchIngress.test_reg.first', register_value_lo),
                 gc.DataTuple('SwitchIngress.test_reg.second', register_value_hi)])])

        resp = test_reg_table.entry_get(
            target,
            [test_reg_table.make_key([gc.KeyTuple('$REGISTER_INDEX', register_idx)])],
            {"from_hw": True})

        data_dict = next(resp)[0].to_dict()
        VerifyReadRegisters(self, "SwitchIngress.test_reg.first", "SwitchIngress.test_reg.second", resp,
                            register_value_lo_arr, register_value_hi_arr, data_dict)


class SingleProgramWithReplayTest(BfRuntimeTest):
    '''
    Case 2
    This test does the following
    setUp()
    1. Client Subscribes to server 
    2. Send a VERIFY_AND_WARM_INIT_BEGIN SetForwardingPipelineConfig msg
    3. Wait for WARM_INIT_STARTED
    runTest()
    4. BINDS to program with SetForwardingPipelineConfig msg
    5. As part of replay, adds 1k register table entries with random data
    6. Sends a WARM_INIT_END SetForwardingPipelineConfig msg
    7. Wait for WARM_INIT_FINISHED
    8. Does a register sync table operation
    9. Gets all the entries from the register table
    10. Verifies the read values match what has been programmed
    '''
    '''
    Taken from IndirectRegisterIteratorTest
    '''

    def setUp(self):
        client_id = 0
        # Setup and don't perform bind
        BfRuntimeTest.setUp(self, client_id, perform_bind=False)
        # Send a Verify_and_warm_init_begin_and_end
        beginWarmInit(self, "tna_register")


    def replay_entries(self):
        seed = random.randint(0, 65535)
        logger.info("Seed used for Indirect Register Sync Test is %d", seed)
        random.seed(seed)

        num_entries = 1024
        # Program the same register values in all the 4 pipes
        logger.info("Inserting %d entries into the register table", num_entries)
        target = gc.Target(device_id=0, pipe_id=0xffff)

        self.register_value_hi = [random.randint(1, 10000) for x in range(num_entries)]
        self.register_value_lo = [random.randint(1, 10000) for x in range(num_entries)]

        for x in range(num_entries):
            self.test_reg_table.entry_add(
                target,
                [self.test_reg_table.make_key([gc.KeyTuple('$REGISTER_INDEX', x)])],
                [self.test_reg_table.make_data(
                    [gc.DataTuple('SwitchIngress.test_reg.first', self.register_value_lo[x]),
                     gc.DataTuple('SwitchIngress.test_reg.second', self.register_value_hi[x])])])

    def cross_check_entries(self):
        # Apply table operations to sync the registers on the indirect register table
        logger.info("Syncing indirect stful registers")
        target = gc.Target(device_id=0, pipe_id=0xffff)
        self.test_reg_table.operations_execute(target, 'Sync')

        # Get from sw and check its value.
        logger.info("Reading back the register table entries")
        resp = self.test_reg_table.entry_get(
            target,
            None,
            {"from_hw": False})

        # The values read back should match the initialized values
        logger.info("Verifying read back indirect register values")
        i = 0
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            VerifyReadRegisters(self,
                                "SwitchIngress.test_reg.first",
                                "SwitchIngress.test_reg.second",
                                resp,
                                [self.register_value_lo[i]] * 4,
                                [self.register_value_hi[i]] * 4,
                                data_dict)
            assert key_dict["$REGISTER_INDEX"]['value'] == i
            logger.info("Indirect Register values matched for idx %d", i)
            i += 1
        pass

    def runTest(self):
        self.p4_name = "tna_register"
        self.interface.bind_pipeline_config(self.p4_name)

        bfrt_info = self.interface.bfrt_info_get("tna_register")
        self.test_reg_table = bfrt_info.table_get("SwitchIngress.test_reg")
        # Insert some entries as replays
        self.replay_entries()

        # Send a warm_init_end
        logger.info("Sending warm_init_end")

        action = bfruntime_pb2.SetForwardingPipelineConfigRequest.WARM_INIT_END
        self.interface.send_set_forwarding_pipeline_config_request(action)

        self.cross_check_entries()


"""
Case 3: Case where switchd starts with a Program and client just wants to BIND.
For this case, please refer to any of the tna_* programs.
"""


"""
Case 4: Test to check learn digest notif before and after warm_init
"""
class SingleProgramWithDigestTest(BfRuntimeTest):
    '''
    Case 4
    This test does the following
    setUp()
    1. Client Subscribes to server
    runTest()
    2. Loads tna_digest and wait for a notif
    3. Loads tna_register. No digest notification should come
    4. load tna_digest back again and trigger notif
    '''
    '''
    Taken from IndirectRegisterIteratorTest
    '''

    def setUp(self):
        client_id = 0
        # Setup and don't perform bind
        BfRuntimeTest.setUp(self, client_id, perform_bind=False)
        # Send a Verify_and_warm_init_begin_and_end

    def expectDigest(self):
        ''' Simple test to check if a digest is received after sending a packet. '''
        ig_port = swports[2]
        smac = '00:01:02:03:04:05'
        dmac = '00:06:07:08:09:0a'

        pkt = testutils.simple_tcp_packet(eth_dst=dmac, eth_src=smac)

        # The learn object can be retrieved using a lesser qualified name on the condition
        # that it is unique
        learn_filter = self.bfrt_info.learn_get("digest_a")
        learn_filter.info.data_field_annotation_add("src_addr", "mac")
        learn_filter.info.data_field_annotation_add("dst_addr", "mac")

        logger.info("Sending packet on port %d", ig_port)
        testutils.send_packet(self, ig_port, pkt)

        testutils.verify_no_other_packets(self)

        digest = self.interface.digest_get()

        recv_target = digest.target
        self.assertTrue(
            recv_target.device_id == self.device_id,
            "Error! Recv device id = %d does not match expected = %d" % (recv_target.device_id, self.device_id))
        exp_pipe_id = (ig_port >> 7) & 0x3
        self.assertTrue(
            recv_target.pipe_id == exp_pipe_id,
            "Error! Recv pipe id = %d does not match expected = %d" % (recv_target.pipe_id, exp_pipe_id))

        data_list = learn_filter.make_data_list(digest)
        data_dict = data_list[0].to_dict()
        logger.info("Received digest %s", str(data_dict))

    def expectNoDigest(self):
        error_recv = False
        digest = None
        try:
            digest = self.interface.digest_get()
        except RuntimeError as e:
            error_recv = True
            pass
        self.assertTrue(
            (digest == None ) and (error_recv == True),
            "Error! Expected None but received digest as %s"% (str(digest)))


    def runTest(self):
        # Step 2
        # Send warm_init_begin
        p4_name = "tna_digest"
        beginWarmInit(self, p4_name)
        self.interface.bind_pipeline_config(p4_name)

        # Send a warm_init_end
        endWarmInit(self)
        self.bfrt_info = self.interface.bfrt_info_get()
        self.expectDigest()

        # Step 3
        # Send warm_init_begin for tna_register
        p4_name = "tna_register"

        beginWarmInit(self, p4_name)
        # Need to subscribe again since warm_init_begin
        # or warm_init_begin_and_end triggers a refresh
        # client
        self.interface.set_up_stream()
        self.interface.subscribe()
        self.interface.bind_pipeline_config(p4_name)

        # Send a warm_init_end
        endWarmInit(self)
        self.bfrt_info = self.interface.bfrt_info_get()
        self.expectNoDigest()

        # Step 4
        # Send warm_init_begin
        p4_name = "tna_digest"
        beginWarmInit(self, p4_name)
        # Need to subscribe again since warm_init_begin
        # or warm_init_begin_and_end triggers a refresh
        # client
        self.interface.set_up_stream()
        self.interface.subscribe()
        self.interface.bind_pipeline_config(p4_name)
        self.bfrt_info = self.interface.bfrt_info_get()

        # Send a warm_init_end
        endWarmInit(self)
        self.expectDigest()
