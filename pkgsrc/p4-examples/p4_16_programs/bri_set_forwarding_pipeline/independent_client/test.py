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
import grpc
import random
import threading
import time

from ptf import config
from ptf.thriftutils import *
from ptf.testutils import *
from p4testutils.misc_utils import *
from bfruntime_client_base_tests import BfRuntimeTest, BaseTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc
import google.rpc.code_pb2 as code_pb2
from functools import partial
try:
    import queue as q
except ImportError:
    import Queue as q

logger = logging.getLogger('Test')
if not len(logger.handlers):
    logger.addHandler(logging.StreamHandler())

base_pick_path = test_param_get("base_pick_path")
binary_name = test_param_get("arch")
if binary_name is not "tofino2" and binary_name is not "tofino":
    assert 0, "%s is unknown arch" % (binary_name)

if not base_pick_path:
    base_pick_path = "install/share/" + binary_name + "pd/"

base_put_path = test_param_get("base_put_path")
if not base_put_path:
    base_put_path = "install/share/" + binary_name + "pd/forwarding"

logger.info("\nbase_put_path=%s \nbase_pick_path=%s", base_pick_path, base_put_path)

swports = get_sw_ports()


def port_to_pipe(port):
    local_port = port & 0x7F
    assert (local_port < 72)
    pipe = (port >> 7) & 0x3
    assert (port == ((pipe << 7) | local_port))
    return pipe


swports_0 = []
swports_1 = []
swports_2 = []
swports_3 = []
# the following method categorizes the ports in ports.json file as belonging to either of the pipes (0, 1, 2, 3)
for port in swports:
    pipe = port_to_pipe(port)
    if pipe == 0:
        swports_0.append(port)
    elif pipe == 1:
        swports_1.append(port)
    elif pipe == 2:
        swports_2.append(port)
    elif pipe == 3:
        swports_3.append(port)

def create_path_bf_rt(base_path, p4_name_to_use):
    return base_path + "/" + p4_name_to_use + "/bf-rt.json"


def create_path_context(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/context.json"


def create_path_tofino(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/" + binary_name + ".bin"

def get_speed(port):
    speed = str(port_param_get(port, 'speed'))
    return {
        "10G": "BF_SPEED_10G",
        "25G": "BF_SPEED_25G",
        "40G": "BF_SPEED_40G",
        "40G_NB": "BF_SPEED_40G_NB",
        "50G": "BF_SPEED_50G",
        "100G": "BF_SPEED_100G",
        "200G": "BF_SPEED_200G",
        "400G": "BF_SPEED_400G",
        "40G_R2": "BF_SPEED_40G_R2"
    }.get(speed.upper(), "BF_SPEED_NONE")

def get_fec(self):
     fec = str(port_param_get(port, 'fec'))
     return {
         "none": "BF_FEC_TYP_NONE",
         "rs": "BF_FEC_TYP_REED_SOLOMON",
         "fc": "BF_FEC_TYP_FIRECODE",
     }.get(fec, "BF_FEC_TYP_NONE")

def get_an(self):
     an = port_param_get(port, 'auto_neg')
     return {
         "default": "PM_AN_DEFAULT",
         "enable": "PM_AN_FORCE_ENABLE",
         "disable": "PM_AN_FORCE_DISABLE"
     }.get(an, "PM_AN_DEFAULT")

def add_ports(test):
    if test_param_get('target') != "hw": return
    logger.info("Adding ports")
    for port in swports:
        speed = get_speed(port)
        fec = get_fec(port)
        an = get_an(port)
        #import pdb; pdb.set_trace()
        test.port_table.entry_add(
            test.target,
            [test.port_table.make_key([gc.KeyTuple("$DEV_PORT", port)])], [
                test.port_table.make_data([
                    gc.DataTuple("$SPEED", str_val=speed),
                    gc.DataTuple("$FEC", str_val=fec),
                    gc.DataTuple("$AUTO_NEGOTIATION", str_val=an),
                    gc.DataTuple("$PORT_ENABLE", bool_val=True)
                ])
            ], p4_name=test.p4_name)

    logger.info("Check if ports are up")
    sleepfor = 5  #5secs
    ##check if the ports are up or else sleep for 5sec and check again till 60secs
    for port in swports:
        portup = False
        while not portup:
            resp = test.port_table.entry_get(
                test.target,
                [test.port_table.make_key([gc.KeyTuple("$DEV_PORT", port)])], p4_name=test.p4_name)
            for data, _ in resp:
                data_dict = data.to_dict()
            if data_dict["$PORT_UP"]:
                portup = True
                continue
            else:
                if sleepfor == 60:
                    logger.error("ports %s not up after 60 secs", port)
                    assert 0
                time.sleep(5)
                sleepfor += 5

def set_fwd_config(test, client_id, action, bind=False, p4_name="tna_exact_match"):
    """@brief Helper function to send a set fwd config msg for tna_exact_match
        Note, this function cannot be used for sending WARM_INIT_END, since
        it sends a config with it as well. WARM_INIT_END cannot accept a config.
        @param test The test object which has its own interface (Program A)
        @param clinet_id client ID
        @param action proto enum of VERIFY_AND_WARM_INIT_BEGIN or 
        VERIFY_AND_WARM_INIT_BEGIN_AND_END
        @param bind If this client needs to bind to the P4 as well
    """
    p4_name_to_put = p4_name_to_pick = p4_name 
    profile_name_to_put = profile_name_to_pick = "pipe"

    config_list = [
        gc.ForwardingConfig(p4_name_to_put,
                            create_path_bf_rt(base_pick_path, p4_name_to_pick),
                            [gc.ProfileInfo(profile_name_to_put,
                                            create_path_context(base_pick_path, p4_name_to_pick, profile_name_to_pick),
                                            create_path_tofino(base_pick_path, p4_name_to_pick, profile_name_to_pick),
                                            [0, 1, 2, 3])]
                            )
    ]

    success = test.interface.send_set_forwarding_pipeline_config_request(
        action,
        base_put_path,
        config_list)
    if not success:
        raise RuntimeError("Failed to get response for setfwd")
    test.p4_name = p4_name
    if bind:
        test.interface.bind_pipeline_config(p4_name)

# Class to start threads which run different clients
class ClientThread(threading.Thread):
    def __init__(self, client_id, client, max_sleep_time, name, cv=None):
        super(ClientThread, self).__init__(name=name)
        self.client_id = client_id
        self.client = client
        self.max_sleep_time = max_sleep_time
        self._exception_q = q.Queue()
        self.stop_event = threading.Event()
        self.cv = cv

    def run(self):

        self.client.setUp(self.client_id)
        while not self.stop_event.wait(1):
            sleep_time = random.randint(0, self.max_sleep_time)
            time.sleep(sleep_time)
            try:
                if self.cv:
                    self.client.runTest(self.cv)
                else:
                    self.client.runTest()

            except gc.BfruntimeRpcException as e:
                # Check if there are any exceptions
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd in client ID %d because device is locked", 
                        self.client_id)
                else:
                    raise e
            except Exception as e:
                self._exception_q.put(e)
                raise e

        logger.info("Client-thread %d STOPPED", self.client_id)

    def stop(self):
        self.stop_event.set()
        self.join()
        logger.info("Tearing down client with ID %d", self.client_id)
        self.client.tearDown()
        if (not self._exception_q.empty()):
            exception = self._exception_q.get()
            raise exception

class IndependentWriteTest(BfRuntimeTest):
    """
    This test simply performs a write RPC using an independent client
    Steps:
    1. setUp independent client (no bind/subscribe)
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
    3. Perform write operations using p4 tna_exact_match
    """
    def setUp(self):
        client_id = 0
        p4_name = "tna_exact_match"
        BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
            perform_subscribe=False)

        # Send a Verify and Warm_init_begin and end
        logger.info("Sending Verify and warm init begin and end")
        set_fwd_config(self, client_id,
                       bfruntime_pb2.SetForwardingPipelineConfigRequest.
                       VERIFY_AND_WARM_INIT_BEGIN_AND_END)

    def runTest(self):
        ig_port = swports[1]
        eg_port = swports[2]
        dmac = '22:22:22:22:22:22'

        # Get bfrt_info and set it as part of the test
        bfrt_info = self.interface.bfrt_info_get()

        pkt = simple_tcp_packet(eth_dst=dmac)
        exp_pkt = pkt

        target = gc.Target(device_id=0, pipe_id=0xffff)
        self.target = target
        self.p4_name = "tna_exact_match"
        self.port_table = bfrt_info.table_get("$PORT")
        add_ports(self)

        forward_table = bfrt_info.table_get("SwitchIngress.forward")
        forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

        key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
        data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
            "SwitchIngress.hit")]

        forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")

        logger.info("Sending packet on port %d", ig_port)
        send_packet(self, ig_port, pkt)
        logger.info("Expecting packet on port %d", eg_port)
        verify_packets(self, exp_pkt, [eg_port])

        forward_table.entry_del(target, p4_name="tna_exact_match")

        logger.info("Sending packet on port %d", ig_port)
        send_packet(self, ig_port, pkt)
        logger.info("Packet is expected to get dropped.") 
        verify_no_other_packets(self)

class WarmInitTest(BaseTest):
    """
    This test initiates a warm init from a new client while an existing client is 
    already performing an RPC.

    ClientA steps:
    1. setUp independent client (no bind/subscribe)
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
    3. Perform write operations using p4 tna_exact_match

    ClientB steps:
    1. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END 
    2. Wait for response
    """
    class ClientA(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            # Send a Verify and Warm_init_begin and end
            logger.info("Sending Verify and warm init begin and end")
            set_fwd_config(self, client_id,
                       bfruntime_pb2.SetForwardingPipelineConfigRequest.
                       VERIFY_AND_WARM_INIT_BEGIN_AND_END)

        def runTest(self):
            self.performWrite()

        def performWrite(self):
            ig_port = swports[1]
            eg_port = swports[2]
            dmac = '22:22:22:22:22:22'

            # Get bfrt_info and set it as part of the test
            bfrt_info = self.interface.bfrt_info_get()

            pkt = simple_tcp_packet(eth_dst=dmac)
            exp_pkt = pkt

            target = gc.Target(device_id=0, pipe_id=0xffff)
            self.target = target
            self.p4_name = "tna_exact_match"
            self.port_table = bfrt_info.table_get("$PORT")
            add_ports(self)

            forward_table = bfrt_info.table_get("SwitchIngress.forward")
            forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

            key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
            data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                 "SwitchIngress.hit")]

            forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")

            logger.info("Sending packet on port %d", ig_port)
            send_packet(self, ig_port, pkt)
            logger.info("Expecting packet on port %d", eg_port)
            verify_packets(self, exp_pkt, [eg_port])

            forward_table.entry_del(target, p4_name="tna_exact_match")

            logger.info("Sending packet on port %d", ig_port)
            send_packet(self, ig_port, pkt)
            logger.info("Packet is expected to get dropped.")
            verify_no_other_packets(self)

    class ClientB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            self.client_d = client_id
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            # Send a Verify and Warm_init_begin and end
            logger.info("Sending Verify and warm init begin and end")
            set_fwd_config(self, client_id,
                           bfruntime_pb2.SetForwardingPipelineConfigRequest.
                           VERIFY_AND_WARM_INIT_BEGIN_AND_END)

        def runTest(self):
            set_fwd_config(self, self.client_id,
                       bfruntime_pb2.SetForwardingPipelineConfigRequest.
                       VERIFY_AND_WARM_INIT_BEGIN_AND_END)

    def setUp(self):
        client_id = 0 

        self.a = self.ClientA()
        self.b = self.ClientB()
        self.a.setUp(1)
        self.b.setUp(2)

    def runTest(self):
        self.a.runTest()
        self.b.runTest()

    def tearDown(self):
        self.a.tearDown()
        self.b.tearDown()
        BaseTest.tearDown(self)

class TwoClientTest(BaseTest):
    """
    Test one independent client and one subscribed client performing 
    write RPCs simultaneously

    ClientA - Independent client
    Steps:
    1. setUp independent client (no bind/subscribe)
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
    3. Perform write operations using p4 tna_exact_match

    ClientB - Subscribed Client
    Steps:
    1. setUp subscribed client
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
    3. Perform write operations using p4 tna_exact_match

    Here, the complete process for each client happens in a thread
    (including the setUp and the warm init)
    """
    class ClientA(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from client A")
                set_fwd_config(self, client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd in client A ecause device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def runTest(self):
            self.performWrite()

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

        def performWrite(self):
            ig_port = swports[1]
            eg_port = swports[2]
            dmac = '11:11:11:11:11:11'

            # Get bfrt_info and set it as part of the test
            bfrt_info = self.interface.bfrt_info_get()

            pkt = simple_tcp_packet(eth_dst=dmac)
            exp_pkt = pkt

            target = gc.Target(device_id=0, pipe_id=0xffff)

            forward_table = bfrt_info.table_get("SwitchIngress.forward")
            forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

            key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
            data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                 "SwitchIngress.hit")]

            forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")

            forward_table.entry_del(target, p4_name="tna_exact_match")


    class ClientB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from client B")
                set_fwd_config(self, client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END)
                self.interface.bind_pipeline_config(p4_name)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd in client B because device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def runTest(self):
            self.performWrite()

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

        def performWrite(self):
            ig_port = swports[1]
            eg_port = swports[2]
            dmac = '22:22:22:22:22:22'

            # Get bfrt_info and set it as part of the test
            bfrt_info = self.interface.bfrt_info_get()

            pkt = simple_tcp_packet(eth_dst=dmac)
            exp_pkt = pkt

            target = gc.Target(device_id=0, pipe_id=0xffff)

            forward_table = bfrt_info.table_get("SwitchIngress.forward")
            forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

            key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
            data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                 "SwitchIngress.hit")]

            forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")

            forward_table.entry_del(target, p4_name="tna_exact_match")

    def runTest(self):
        clientA = ClientThread(1, self.ClientA(), 5, name='ClientA')
        clientA.start()

        clientB = ClientThread(2, self.ClientB(), 4, name='ClientB')
        clientB.start()

        time.sleep(10)

        clientA.stop()
        clientB.stop()

    def tearDown(self):
        BaseTest.tearDown(self)       

class TwoIndependentClientTest(BaseTest):
    """
    Test two independent clients performing write RPCs simultaneously

    ClientA and ClientB are both independent clients
    Steps:
    1. The two clients are set up in the main thread
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
       for both clients in main thread
    3. Spawn a client thread for ClientA, and begin write RPCs
    4. Perform write RPCs from ClientB in the main thread

    """
    class ClientA(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from client A")
                set_fwd_config(self, client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd because device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def runTest(self):
            self.performWrite()


        def tearDown(self):
            BfRuntimeTest.tearDown(self)


        def performWrite(self):
            ig_port = swports[1]
            eg_port = swports[2]
            dmac = '11:11:11:11:11:11'

            # Get bfrt_info and set it as part of the test
            bfrt_info = self.interface.bfrt_info_get()

            pkt = simple_tcp_packet(eth_dst=dmac)
            exp_pkt = pkt

            target = gc.Target(device_id=0, pipe_id=0xffff)

            forward_table = bfrt_info.table_get("SwitchIngress.forward")
            forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

            key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
            data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                 "SwitchIngress.hit")]

            logger.info("Writing to table in client A")
            forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")

            forward_table.entry_del(target, p4_name="tna_exact_match")


    class ClientB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)

            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from Client B")
                set_fwd_config(self, client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd because device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def runTest(self):
            self.performWrite()


        def tearDown(self):
            BfRuntimeTest.tearDown(self)


        def performWrite(self):
            ig_port = swports[1]
            eg_port = swports[2]
            dmac = '22:22:22:22:22:22'

            try:
                # Get bfrt_info and set it as part of the test
                bfrt_info = self.interface.bfrt_info_get()

                pkt = simple_tcp_packet(eth_dst=dmac)
                exp_pkt = pkt

                target = gc.Target(device_id=0, pipe_id=0xffff)

                forward_table = bfrt_info.table_get("SwitchIngress.forward")
                forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

                key_list = [forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac)])]
                data_list = [forward_table.make_data([gc.DataTuple('port', eg_port)],
                                                     "SwitchIngress.hit")]

                forward_table.entry_add(target, key_list, data_list, p4_name="tna_exact_match")
                forward_table.entry_del(target, p4_name="tna_exact_match")
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd because device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

    def setUp(self):   
        super(TwoIndependentClientTest, self).setUp() 

    def runTest(self):
        # Setting up client
        clientA = self.ClientA()
        clientB = self.ClientB()
        clientB.setUp(2)

        # Starting client A in thread
        clientA_thread = ClientThread(1, clientA, 4, name='ClientA')
        clientA_thread.start()

        # Running clientB in main thread
        for _ in range(5):
            clientB.runTest()
            time.sleep(1)
        logger.info("Tearing down client B")
        clientB.tearDown()

        clientA_thread.stop()

    def tearDown(self):
        BaseTest.tearDown(self)

@disabled
class MultipleIndependentClientTest(BaseTest):
    """
    Test several independent clients performing write RPCs simultaneously

    Steps:
    1. The two clients are set up in the main thread
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
       for both clients in main thread
    3. Spawn a client thread for ClientA, and begin write RPCs
    4. Perform write RPCs from ClientB in the main thread

    """
    class WarmInitClient(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)
            logger.info("Sending Verify and warm init begin and end from dummy client with ID %d", client_id)
            set_fwd_config(self, client_id,
                           bfruntime_pb2.SetForwardingPipelineConfigRequest.
                           VERIFY_AND_WARM_INIT_BEGIN_AND_END)

        def runTest(self):
            pass

    class IndependentClient(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)
            self.client_id = client_id
            self.p4_name = p4_name

        def runTest(self):
            self.performWrite()

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

        def performWrite(self):
            ig_port = swports[1]
            num_entries = 100
            seed = random.randint(1, 65535)
            random.seed(seed)
            logger.info("Seed used %d in client with ID %d", seed, self.client_id)

            # Get bfrt_info and set it as part of the test
            bfrt_info = self.interface.bfrt_info_get("tna_exact_match")
            logger.info("Client %d got bfrt_info", self.client_id)

            vrfs = [x for x in range(num_entries)]
            ipDstAddrs = ["%d.%d.%d.%d" % (
                random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
                range(num_entries)]

            nat_action_data = {}
            nat_action_data['ipSrcAddrs'] = ["%d.%d.%d.%d" % (
                random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
                range(num_entries)]
            nat_action_data['ipDstAddrs'] = ["%d.%d.%d.%d" % (
                random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
                range(num_entries)]
            nat_action_data['ports'] = [swports[random.randint(1, 5)] for x in range(num_entries)]

            route_action_data = {}
            route_action_data['srcMacAddrs'] = [":".join("%02x" % i for i in [random.randint(0, 255) for j in range(6)]) for
                                                k in range(num_entries)]
            route_action_data['dstMacAddrs'] = [":".join("%02x" % i for i in [random.randint(0, 255) for j in range(6)]) for
                                                k in range(num_entries)]
            route_action_data['ports'] = [swports[random.randint(1, 5)] for x in range(num_entries)]

            action_choices = ['SwitchIngress.route', 'SwitchIngress.nat']
            action = [action_choices[random.randint(0, 1)] for x in range(num_entries)]

            target = gc.Target(device_id=0, pipe_id=0xffff)

            logger.info("Adding %d entries on ipRoute Table in client with ID %d", num_entries, self.client_id)

            iproute_table = bfrt_info.table_get("SwitchIngress.ipRoute")
            iproute_table.info.data_field_annotation_add("srcMac", "SwitchIngress.route", "mac")
            iproute_table.info.data_field_annotation_add("dstMac", "SwitchIngress.route", "mac")
            iproute_table.info.data_field_annotation_add("srcAddr", "SwitchIngress.nat", "ipv4")
            iproute_table.info.data_field_annotation_add("dstAddr", "SwitchIngress.nat", "ipv4")
            iproute_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
            key_list = []
            data_list = []
            meter_data = {}
            meter_data['cir'] = [1000 * random.randint(1, 1000) for i in range(num_entries)]
            meter_data['pir'] = [meter_data['cir'][i] * random.randint(1, 5) for i in range(num_entries)]
            meter_data['cbs'] = [1000 * random.randint(1, 100) for i in range(num_entries)]
            meter_data['pbs'] = [meter_data['cbs'][i] * random.randint(1, 5) for i in range(num_entries)]
            for x in range(num_entries):
                dip = ipDstAddrs[x]
                vrf = vrfs[x]
                if action[x] == 'SwitchIngress.route':
                    data_list.append(iproute_table.make_data(
                        [gc.DataTuple('srcMac', route_action_data['srcMacAddrs'][x]),
                         gc.DataTuple('dstMac', route_action_data['dstMacAddrs'][x]),
                         gc.DataTuple('dst_port', route_action_data['ports'][x]),
                         gc.DataTuple('$METER_SPEC_CIR_KBPS', meter_data['cir'][x]),
                         gc.DataTuple('$METER_SPEC_PIR_KBPS', meter_data['pir'][x]),
                         gc.DataTuple('$METER_SPEC_CBS_KBITS', meter_data['cbs'][x]),
                         gc.DataTuple('$METER_SPEC_PBS_KBITS', meter_data['pbs'][x])],
                        "SwitchIngress.route"))
                elif action[x] == 'SwitchIngress.nat':
                    data_list.append(iproute_table.make_data(
                        [gc.DataTuple('srcAddr', nat_action_data['ipSrcAddrs'][x]),
                         gc.DataTuple('dstAddr', nat_action_data['ipDstAddrs'][x]),
                         gc.DataTuple('dst_port', nat_action_data['ports'][x]),
                         gc.DataTuple('$METER_SPEC_CIR_KBPS', meter_data['cir'][x]),
                         gc.DataTuple('$METER_SPEC_PIR_KBPS', meter_data['pir'][x]),
                         gc.DataTuple('$METER_SPEC_CBS_KBITS', meter_data['cbs'][x]),
                         gc.DataTuple('$METER_SPEC_PBS_KBITS', meter_data['pbs'][x])],
                        "SwitchIngress.nat"))
                key_list.append(iproute_table.make_key([gc.KeyTuple('vrf', vrf),
                                                        gc.KeyTuple('hdr.ipv4.dst_addr', dip)]))

            iproute_table.entry_add(target, key_list, data_list, p4_name=self.p4_name)

            logger.info("DONE adding %d entries on ipRoute Table on client with ID %d", num_entries, self.client_id)
            """
            logger.info("Reading %d entries on ipRoute Table on client iwht ID %d", num_entries, self.client_id)
            resp = iproute_table.entry_get(target, key_list, {"from_hw": True}, p4_name=self.p4_name)
            x = 0
            for data, key in resp:
                data_fields = data.to_dict()
                key_fields = key.to_dict()
                #logger.info("Verifying entry %d for action %s", x, action[x])
                assert data_fields["action_name"] == action[x]
                if action[x] == 'SwitchIngress.route':
                    assert data_fields['srcMac'] == route_action_data['srcMacAddrs'][x]
                    assert data_fields['dstMac'] == route_action_data['dstMacAddrs'][x]
                    assert data_fields['dst_port'] == route_action_data['ports'][x]
                elif action[x] == 'SwitchIngress.nat':
                    assert data_fields['srcAddr'] == nat_action_data['ipSrcAddrs'][x]
                    assert data_fields['dstAddr'] == nat_action_data['ipDstAddrs'][x]
                    assert data_fields['dst_port'] == nat_action_data['ports'][x]
                x += 1
            """
            logger.info("Deleting entries on client with ID %d", self.client_id)
            iproute_table.entry_del(target, key_list, p4_name=self.p4_name)

    def setUp(self):
        # Performing one warm init from dummy client initially 
        warmInitClient = self.WarmInitClient()
        warmInitClient.setUp(4)
        warmInitClient.tearDown()
        """
        warmInitClient = self.IndependentClient()
        warmInitClient.setUp(5)
        warmInitClient.runTest()
        warmInitClient.tearDown()
        """
        super(MultipleIndependentClientTest, self).setUp() 

    def runTest(self):
        # Number of clients which reuse a given client_id for testinf purposes
        num_clients = 3

        client_threads_1 = []
        client_threads_2 = []
        client_threads_3 = []

        # Setting up clients
        for i in range(num_clients):
            client_threads_1.append(ClientThread(1, self.IndependentClient(), 0, 
                                                 name='Client1_'.join(str(i + 1))))
            client_threads_1[i].start()
            #client_threads_2.append(ClientThread(2, self.IndependentClient(), 0,
            #                                     name='Client1_'.join(str(i + 1))))
            #client_threads_2[i].start()
            #client_threads_3.append(ClientThread(3, self.IndependentClient(), 0, 
            #                                     name='Client1_'.join(str(i + 1))))
            #client_threads_3[i].start()

        time.sleep(60)
        for i in range(num_clients):
            client_threads_1[i].stop()
            #client_threads_2[i].stop()
            #client_threads_3[i].stop()

        """
        # Setting up clients
        clientA = self.ClientA()
        clientB = self.ClientB()
        clientB.setUp(2)
        
        # Starting client A in thread
        clientA_thread = ClientThread(1, clientA, 4, name='ClientA')
        clientA_thread.start()

        # Running clientB in main thread
        for _ in range(5):
            clientB.runTest()
            time.sleep(1)
        logger.info("Tearing down client B")
        clientB.tearDown()

        clientA_thread.stop()
        """

    def tearDown(self):
        BaseTest.tearDown(self)

@disabled
class IdleTimeoutNotificationTest(BaseTest):
    """
    Test one independent client and one subscribed client performing 
    write RPCs simultaneously

    ClientA - Subscribed Client receiving IdleTimeout notifications 
    Steps:
    1. setUp independent client (no bind/subscribe)
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response
    3. Subscribe + Bind to tna_idletimeout

    ClientB - Independent Client performing Warm Init
    Steps:
    1. setUp independent client (no bind/subscribe)
    2. Send VERIFY_AND_WARM_INIT_BEGIN_AND_END and wait for response

    Here, the complete process for each client happens in a thread
    (including the setUp and the warm init)
    """

    class ClientA(BfRuntimeTest):
        """@brief Simple test of the basic idle timeout configuration parameters and
        their usage.
        """

        def setUp(self, client_id=0):
            p4_name = "tna_idletimeout"
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)
            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from client A")
                set_fwd_config(self, client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END, p4_name=p4_name)
                self.interface.subscribe(notifications=gc.Notifications(False, True, False))
                self.interface.bind_pipeline_config(p4_name)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd in client A ecause device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def runTest(self, cv=None):
            p4_name = "tna_idletimeout"
            ig_port = swports[1]
            eg_port = swports[2]
            seed = random.randint(1, 65535)
            random.seed(seed)
            logger.info("seed used %d", seed)

            dmac = gc.bytes_to_mac(gc.to_bytes(random.randint(0, 2 ** 48 - 1), 6))
            smac = gc.bytes_to_mac(gc.to_bytes(random.randint(0, 2 ** 48 - 1), 6))
            prefix_len = random.randint(0, 48)
            dip = gc.bytes_to_ipv4(gc.to_bytes(random.randint(0, 2 ** 32 - 1), 4))
            dip_mask = ((0xffffffff) << (32 - random.randint(0, 32))) & (0xffffffff)

            # Get bfrt_info and set it as part of the test
            try:
                bfrt_info = self.interface.bfrt_info_get("tna_idletimeout")
            except RuntimeError as e:
                logger.info("Expected error: %s", str(e))
                return
            dmac_table = bfrt_info.table_get("SwitchIngress.dmac")
            dmac_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
            dmac_table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")
            dmac_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

            target = gc.Target(device_id=0, pipe_id=0xffff)

            dmac_table.attribute_entry_scope_set(target, predefined_pipe_scope=True,
                                                 predefined_pipe_scope_val=bfruntime_pb2.Mode.ALL)
            # Set Idle Table attributes
            ttl_query_length = 5000
            max_ttl = 3600000
            min_ttl = 1000
            dmac_table.attribute_idle_time_set(target,
                                               True,
                                               bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE,
                                               ttl_query_length,
                                               max_ttl,
                                               min_ttl)
            if cv:
                with cv: 
                    cv.notify_all()            

            resp = dmac_table.attribute_get(target, "IdleTimeout", p4_name=p4_name)
            for d in resp:
                assert d["ttl_query_interval"] == ttl_query_length
                assert d["max_ttl"] == max_ttl
                assert d["min_ttl"] == min_ttl
                assert d["idle_table_mode"] == bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE
                assert d["enable"] == True

            pkt = simple_tcp_packet(eth_dst=dmac, ip_dst=dip, eth_src=smac)
            exp_pkt = pkt

            # Creating 900 entries to add into the table
            key_list = []
            data_list = []
            dmacs = []
            num_entries = 900
            for i in range(num_entries):
                dmac = gc.bytes_to_mac(gc.to_bytes(random.randint(0, 2 ** 48 - 1), 6))
                dmacs.append(dmac)
                key_list.append(dmac_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', dmac),  # exact
                                      gc.KeyTuple('hdr.ethernet.src_addr', smac, prefix_len=prefix_len),  # LPM
                                      gc.KeyTuple('hdr.ipv4.dst_addr', dip, dip_mask),  # ternary
                                      gc.KeyTuple('$MATCH_PRIORITY', 1)]))  # priority req for ternary
                data_list.append(dmac_table.make_data([gc.DataTuple('port', eg_port),
                                       gc.DataTuple('$ENTRY_TTL', 5000)],
                                      'SwitchIngress.hit'))

            sent_key_set = set()
            for key in key_list:
                key.apply_mask()
                sent_key_set.add(key)

            dmac_table.entry_add(target, key_list, data_list)

            # Check for timeout notifications.
            for i in range(num_entries):
                idle_time = self.interface.idletime_notification_get(10)
                recv_key = bfrt_info.key_from_idletime_notification(idle_time)

                if i % 100 == 0:
                    logger.info("%d entries received", i)

                # Checking if the received key is in the sent key set
                assert recv_key in sent_key_set, "%s received" %(recv_key)

            logger.info("Deleting entries from table")
            dmac_table.entry_del(target)

            logger.info("Disable idle timeout on the table")
            dmac_table.attribute_idle_time_set(target,
                                               False,
                                               bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE)

    class ClientB(BfRuntimeTest):
        def setUp(self, client_id, p4_name="tna_exact_match"):
            BfRuntimeTest.setUp(self, client_id, p4_name, perform_bind=False,
                perform_subscribe=False)
            self.client_id = client_id

        def runTest(self):
            try:
                # Send a Verify and Warm_init_begin and end
                logger.info("Sending Verify and warm init begin and end from Client B")
                set_fwd_config(self, self.client_id,
                               bfruntime_pb2.SetForwardingPipelineConfigRequest.
                               VERIFY_AND_WARM_INIT_BEGIN_AND_END)
            except gc.BfruntimeRpcException as e:
                # Errors tolerated ->
                # 1. Device is locked
                if e.grpc_error_get().code() == grpc.StatusCode.UNAVAILABLE:
                    logger.info("Failed to GetFwd because device is locked")
                else:
                    logger.error("Failed to GetFwdConfig")
                    raise e

        def tearDown(self):
            BfRuntimeTest.tearDown(self)

    def setUp(self):   
        super(IdleTimeoutNotificationTest, self).setUp() 

    def runTest(self):
        # Setting up client
        clientB = self.ClientB()
        clientB.setUp(2)

        cv = threading.Condition()

        # Starting client A in thread
        clientA_thread = ClientThread(1, self.ClientA(), 4, 'ClientA', cv)
        clientA_thread.start()

        with cv:
            cv.wait()

        # Running clientB in main thread
        time.sleep(6.5)
        clientB.runTest()
        clientB.tearDown()

        time.sleep(10)
        clientA_thread.stop()
