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

import random
import time
import threading

import ptf.testutils as testutils
from ptf.testutils import *
from p4testutils.misc_utils import *
import ptf.packet as scapy
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc
from collections import namedtuple
from p4testutils import misc_utils
from p4testutils import bfrt_utils

logger = misc_utils.get_logger()
swports = misc_utils.get_sw_ports()

HashFieldSlice = namedtuple('HashFieldSlice', 'name start_bit length order')

warminit_mode = testutils.test_param_get("mode", "hitless")
logger.info("warm init mode is {}".format(warminit_mode))

if warminit_mode == "fast-reconfig":
    hitless_mode = False
else:
    hitless_mode = True

swports_0 = []
swports_1 = []
swports_2 = []
swports_3 = []
# the following method categorizes the ports in ports.json file as belonging to either of the pipes (0, 1, 2, 3)
for port in swports:
    pipe = misc_utils.port_to_pipe(port)
    if pipe == 0:
        swports_0.append(port)
    elif pipe == 1:
        swports_1.append(port)
    elif pipe == 2:
        swports_2.append(port)
    elif pipe == 3:
        swports_3.append(port)


def get_traffic_stats(self):
    logger.info("get stats on ports {} {}".format(self.eg_port, self.ig_port))
    for port in [self.ig_port, self.eg_port]:
        get_data_list = None
        resp = self.port_stat_table.entry_get(
            self.target,
            [self.port_stat_table.make_key([gc.KeyTuple('$DEV_PORT', port)])],
            get_data_list)
        data_dict = next(resp)[0].to_dict()
        if port == self.eg_port :
            tx = data_dict['$FramesTransmittedOK']
    return tx


def put_program_on_device(test):
    if not test.p4_name:
        test.p4_name = "tna_exact_match"
    p4_name_to_put = p4_name_to_pick = test.p4_name
    profile_name_to_put = "pipe"
    profile_name_to_pick = "pipe"

    logger.info("Sending Verify and warm_init_begin and warm_init_end for {}".format(p4_name_to_put))
    action = bfruntime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_WARM_INIT_BEGIN_AND_END
    success = test.interface.send_set_forwarding_pipeline_config_request( \
        action,
        bfrt_utils.base_put_path,
        [gc.ForwardingConfig(p4_name_to_put,
                             bfrt_utils.create_path_bf_rt(bfrt_utils.base_pick_path, p4_name_to_pick),
                             [gc.ProfileInfo(profile_name_to_put,
                                             bfrt_utils.create_path_context(bfrt_utils.base_pick_path, p4_name_to_pick,
                                                                 profile_name_to_pick),
                                             bfrt_utils.create_path_tofino(bfrt_utils.base_pick_path, p4_name_to_pick,
                                                                profile_name_to_pick),
                                             range(int(testutils.test_param_get('num_pipes'))))]
                             )])
    if not success:
        raise RuntimeError("Failed to setFwd")
    test.interface.bind_pipeline_config(test.p4_name)


class HitlessBaseTest(BfRuntimeTest):
    '''
    This acts as the Base test for all HA tests and provides the following template
    to write HA tests.
    setUp()
    1. Client Subscribes to server
    2. Send a SetForwardingPipelineConfig msg to put the program on the device first
    3. Wait for WARM_INIT_FINISHED
    runTest()
    4. BINDS to program with SetForwardingPipelineConfig msg
    5. Add entries, send traffic
    6. Send a WARM_INIT_BEGIN SetForwardingPipelineConfig msg with HITLESS
    7. Wait for WARM_INIT_STARTED
    8. Replay entries
    9. Send a WARM_INIT_END msg and wait for WARM_INIT_FINISHED
    10. verify entries, Send traffic
    '''
    def setup_tables(self):
        pass

    def setup_test_data(self):
        pass

    def add_entries(self):
        pass

    def send_traffic(self):
        pass

    def replay_entries(self):
        logger.info("replaying entries")
        self.add_entries()

    def get_entries_and_verify(self):
        pass

    def setUp(self):
        self.client_id = 0
        # Setup
        BfRuntimeTest.setUp(self, self.client_id, perform_bind=False)
        # Send a Verify_and_warm_init_begin_and_end
        put_program_on_device(self)

        # set up tables
        self.bfrt_info = self.interface.bfrt_info_get()
        self.port_table = self.bfrt_info.table_get("$PORT")
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        bfrt_utils.add_ports(self)
        self.num_pipes = bfrt_utils.get_num_pipes(self.bfrt_info.table_get("device_configuration"))

        self.seed = random.randint(0, 65535)
        logger.info("Seed used {}".format(self.seed))
        random.seed(self.seed)

    def runTest(self):
        self.setup_tables()
        self.setup_test_data()

        self.add_entries()
        self.send_traffic()
        self.get_entries_and_verify()

        bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=True)
        self.replay_entries()
        bfrt_utils.end_warm_init(self)

        self.get_entries_and_verify()
        self.send_traffic()


class HitlessTnaExactMatchTrafficLoss(HitlessBaseTest):
    '''
    This testcase is to measure any traffic loss with warm-init hitless
    '''
    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
        self.port_table = self.bfrt_info.table_get("$PORT")
        self.port_stat_table = self.bfrt_info.table_get("$PORT_STAT")

    def setup_test_data(self):
        self.dmac = '22:22:22:22:22:22'
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.target = gc.Target(device_id=0, pipe_id=0xffff)

    def add_entries(self):
        logger.info("Adding entries")
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        data_list = [self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                             "SwitchIngress.hit")]
        self.forward_table.entry_add(self.target, key_list, data_list)

    def send_traffic(self):
        num_pkts = testutils.test_param_get("num_pkts", 200000)
        pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
        exp_pkt = pkt
        logger.info("Sending packet on port {}".format(self.ig_port))
        ts=time.time()
        testutils.send_packet(self, self.ig_port, pkt, num_pkts)
        te=time.time()
        ti_per_pkt=(te-ts)*1000/num_pkts
        logger.info(ts)
        logger.info(te)
        logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
        time.sleep(5)
        egport_stats = get_traffic_stats(self)
        loss_ti_ms = ti_per_pkt * (num_pkts - egport_stats)
        logger.info("Traffic loss is {} ms".format(loss_ti_ms))
        if loss_ti_ms > 0:
            raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
        logger.info("End of send_traffic")

    def get_entries_and_verify(self):
        logger.info("Get entries")
        resp = self.forward_table.entry_get(self.target)
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            assert data_dict["port"] == self.eg_port
            assert key_dict["hdr.ethernet.dst_addr"]["value"] == self.dmac

    def setUp(self):
        if testutils.test_param_get("target") != "hw":
            logger.info("This testcase is meant to be run only on the hardware. So skipping")
            return
        self.p4_name = "tna_exact_match"
        HitlessBaseTest.setUp(self)

    def runTest(self):
        if testutils.test_param_get("target") != "hw":
            logger.info("This testcase is meant to be run only on the hardware. So skipping")
            return
        self.setup_tables()
        self.setup_test_data()

        self.add_entries()
        trd1 = threading.Thread(target=self.send_traffic)
        logger.info("Before thread start")
        trd1.start()
        self.get_entries_and_verify()
        logger.info("After thread start")
        bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=True)

        self.replay_entries()
        bfrt_utils.end_warm_init(self)

        trd1.join()
        self.get_entries_and_verify()

    def tearDown(self):
        if testutils.test_param_get("target") != "hw":
            logger.info("This testcase is meant to be run only on the hardware. So skipping")
            return
        super(HitlessBaseTest, self).tearDown()

class HitlessTnaExactMatch(HitlessBaseTest):

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
        self.port_table = self.bfrt_info.table_get("$PORT")

    def setup_test_data(self):
        self.dmac = '22:22:22:22:22:22'
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.target = gc.Target(device_id=0, pipe_id=0xffff)

    def add_entries(self):
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        data_list = [self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                             "SwitchIngress.hit")]
        self.forward_table.entry_add(self.target, key_list, data_list)

    def send_traffic(self):
        pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
        exp_pkt = pkt
        logger.info("Sending packet on port {}".format(self.ig_port))
        testutils.send_packet(self, self.ig_port, pkt)
        logger.info("Expecting packet on port {}".format(self.eg_port))
        testutils.verify_packets(self, exp_pkt, [self.eg_port])

    def get_entries_and_verify(self):
        resp = self.forward_table.entry_get(self.target)
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            assert data_dict["port"] == self.eg_port
            assert key_dict["hdr.ethernet.dst_addr"]["value"] == self.dmac

    def setUp(self):
        self.p4_name = "tna_exact_match"
        HitlessBaseTest.setUp(self)


class HitlessTnaExactMatchEntryUserDefinedScope(HitlessBaseTest):
    '''
    Test User defined Scope asymmetric Match table
    '''

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

    def setup_test_data(self):
        self.dmac = '22:22:22:22:22:22'
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        self.target0 = gc.Target(device_id=0, pipe_id=0x00)
        # Set pipes 0 and 1 in scope 1 and pipes 2 and 3 in scope 2
        # Note this cannot be done during replay again, since
        # "changing" entry scope while entries are present isn't
        # allowed.
        self.forward_table.attribute_entry_scope_set(self.target,
                predefined_pipe_scope=False, user_defined_pipe_scope_val=0xc03)

    def add_entries(self):
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        data_list = [self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                             "SwitchIngress.hit")]
        self.forward_table.entry_add(self.target0, key_list, data_list)

    def send_traffic(self):
        def send_and_verify_packet(self, ingress_port, egress_port, pkt, exp_pkt):
            logger.info("Sending packet on port {}".format(ingress_port))
            testutils.send_packet(self, ingress_port, pkt)
            logger.info("Expecting packet on port {}".format(egress_port))
            testutils.verify_packet(self, exp_pkt, egress_port)

        def send_and_verify_no_other_packet(self, ingress_port, pkt):
            logger.info("Sending packet on port {} (negative test); expecting no packet".format(ingress_port))
            testutils.send_packet(self, ingress_port, pkt)
            testutils.verify_no_other_packets(self)
        pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
        exp_pkt = pkt
        # Since we have installed the entry in scope 1 (pipe 0 and 1) only,
        # we expect the packet to hit the entries in pipes 0 and 1
        # and miss in pipes 2 and 3
        send_and_verify_packet(self, swports_0[0], self.eg_port, pkt, exp_pkt)
        send_and_verify_packet(self, swports_0[1], self.eg_port, pkt, exp_pkt)
        send_and_verify_packet(self, swports_0[2], self.eg_port, pkt, exp_pkt)
        # In folded pipes conf the swports_1 ports might be empty
        if len(swports_1):
            send_and_verify_packet(self, swports_1[1], self.eg_port, pkt, exp_pkt)
            send_and_verify_packet(self, swports_1[2], self.eg_port, pkt, exp_pkt)
        # In folded pipes conf the swports_2 ports might be empty
        if self.num_pipes > 2 and len(swports_2):
            send_and_verify_no_other_packet(self, swports_2[0], pkt)
        # In folded pipes conf the swports_3 ports might be empty
        if self.num_pipes > 3 and len(swports_3):
            send_and_verify_no_other_packet(self, swports_3[0], pkt)

    def get_entries_and_verify(self):
        resp = self.forward_table.attribute_get(self.target, "EntryScope")
        for d in resp:
            logger.info("received {}".format(d))
            assert d["gress_scope"]["predef"] == bfruntime_pb2.Mode.ALL
            assert "predef" not in d["pipe_scope"]
            assert d["pipe_scope"]["user_defined"] == 0xc03
            assert d["prsr_scope"]["predef"] == bfruntime_pb2.Mode.ALL
        resp = self.forward_table.entry_get(self.target)
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            assert data_dict["port"] == self.eg_port
            assert key_dict["hdr.ethernet.dst_addr"]["value"] == self.dmac

    def runTest(self):
        ######## Disabling this test. Remove this function to enable back #########
        pass

    def setUp(self):
        self.p4_name = "tna_exact_match"
        HitlessBaseTest.setUp(self)

class HitlessTnaExactMatchEntrySingleScope(HitlessBaseTest):
    '''
    Test only Single Scope asymmetric Match table
    '''

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

    def setup_test_data(self):
        self.dmac = '22:22:22:22:22:22'
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        self.target0 = gc.Target(device_id=0, pipe_id=0x00)
        # Set all pipes to be in different scopes. Also known as Single scope
        self.forward_table.attribute_entry_scope_set(self.target,
                predefined_pipe_scope=True,
                predefined_pipe_scope_val=bfruntime_pb2.Mode.SINGLE)

    def add_entries(self):
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        data_list = [self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                             "SwitchIngress.hit")]
        self.forward_table.entry_add(self.target0, key_list, data_list)

    def send_traffic(self):
        def send_and_verify_packet(self, ingress_port, egress_port, pkt, exp_pkt):
            logger.info("Sending packet on port {}".format(ingress_port))
            testutils.send_packet(self, ingress_port, pkt)
            logger.info("Expecting packet on port {}".format(egress_port))
            testutils.verify_packet(self, exp_pkt, egress_port)

        def send_and_verify_no_other_packet(self, ingress_port, pkt):
            logger.info("Sending packet on port {} (negative test); expecting no packet".format(ingress_port))
            testutils.send_packet(self, ingress_port, pkt)
            testutils.verify_no_other_packets(self)
        pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
        exp_pkt = pkt
        # Since we have installed the entry in pipe0 only as single scope, we expect
        # the packet to get dropped in pther pipes
        send_and_verify_packet(self, swports_0[0], self.eg_port, pkt, exp_pkt)
        send_and_verify_packet(self, swports_0[1], self.eg_port, pkt, exp_pkt)
        send_and_verify_packet(self, swports_0[2], self.eg_port, pkt, exp_pkt)
        # In folded pipes conf the swports_1 ports might be empty
        if len(swports_1):
            send_and_verify_no_other_packet(self, swports_1[1], pkt)
            send_and_verify_no_other_packet(self, swports_1[2], pkt)
        # In folded pipes conf the swports_2 ports might be empty
        if self.num_pipes > 2 and len(swports_2):
            send_and_verify_no_other_packet(self, swports_2[0], pkt)
        # In folded pipes conf the swports_3 ports might be empty
        if self.num_pipes > 3 and len(swports_3):
                send_and_verify_no_other_packet(self, swports_3[0], pkt)

    def get_entries_and_verify(self):
        resp = self.forward_table.attribute_get(self.target, "EntryScope")
        for d in resp:
            logger.info("received {}".format(d))
            assert d["gress_scope"]["predef"] == bfruntime_pb2.Mode.ALL
            assert d["pipe_scope"]["predef"] == bfruntime_pb2.Mode.SINGLE
            assert d["prsr_scope"]["predef"] == bfruntime_pb2.Mode.ALL
        resp = self.forward_table.entry_get(self.target)
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            assert data_dict["port"] == self.eg_port
            assert key_dict["hdr.ethernet.dst_addr"]["value"] == self.dmac

    def setUp(self):
        self.p4_name = "tna_exact_match"
        HitlessBaseTest.setUp(self)

class HitlessTnaTernaryMatch(HitlessBaseTest):
    '''
    '''

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

    def setup_test_data(self):
        tuple_list = []
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.key_list = []
        self.data_list = []
        self.num_entries = 100
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        self.ip_random_list = self.generate_random_ip_list(self.num_entries, self.seed)
        prio = random.randint(1, 5000)
        for i in range(self.num_entries):
            self.key_list.append(
                    self.forward_table.make_key(
                        [gc.KeyTuple('$MATCH_PRIORITY', prio),
                         gc.KeyTuple('vrf', 0),
                         gc.KeyTuple('hdr.ipv4.dst_addr', getattr(self.ip_random_list[i], "ip"), getattr(self.ip_random_list[i], "mask"))]))

            self.data_list.append(self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                                         'SwitchIngress.hit'))

    def add_entries(self):
        self.forward_table.entry_add(self.target, self.key_list, self.data_list)

    def send_traffic(self):
        def send_and_verify_packet(self, ingress_port, egress_port, pkt, exp_pkt):
            testutils.send_packet(self, ingress_port, pkt)
            testutils.verify_packet(self, exp_pkt, egress_port)

        logger.info("Sending traffic")
        for i in range(self.num_entries):
            dst_ip = self.key_list[i].to_dict()["hdr.ipv4.dst_addr"]["value"]
            pkt = testutils.simple_tcp_packet(ip_dst=dst_ip)
            exp_pkt = pkt
            send_and_verify_packet(self, self.ig_port, self.eg_port, pkt, exp_pkt)

    def get_entries_and_verify(self):
        resp = self.forward_table.entry_get(self.target)
        i=0
        for data, key in resp:
            self.key_list[i].apply_mask()
            assert key == self.key_list[i], "received {} expected {}".format(key, self.key_list[i])
            assert data == self.data_list[i]
            i+=1

    def setUp(self):
        self.p4_name = "tna_ternary_match"
        HitlessBaseTest.setUp(self)

class HitlessTnaTernaryMatchAtcam(HitlessBaseTest):
    '''
    '''

    def setup_tables(self):
        self.forward_atcam_table = self.bfrt_info.table_get("SwitchIngress.forward_atcam")
        self.set_partition_table = self.bfrt_info.table_get("SwitchIngress.set_partition")
        self.forward_atcam_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

    def setup_test_data(self):
        self.ig_port = swports[1]
        self.eg_port = swports[2]
        self.key_list_1 = []
        self.key_list_2 = []
        self.data_list = []
        self.num_entries = 30
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        self.ip_random_list = self.generate_random_ip_list(self.num_entries, self.seed)
        self.atcam_dict = {}
        for i in range(self.num_entries):
            self.key_list_1.append(
                    self.forward_atcam_table.make_key(
                        [gc.KeyTuple('$MATCH_PRIORITY', 0),
                         gc.KeyTuple('ig_md.partition.partition_index', 3),
                         gc.KeyTuple('hdr.ipv4.dst_addr', getattr(self.ip_random_list[i], "ip"), getattr(self.ip_random_list[i], "mask"))]))
            self.key_list_2.append(
                    self.forward_atcam_table.make_key(
                        [gc.KeyTuple('$MATCH_PRIORITY', 0),
                         gc.KeyTuple('ig_md.partition.partition_index', 1),
                         gc.KeyTuple('hdr.ipv4.dst_addr', getattr(self.ip_random_list[i], "ip"), getattr(self.ip_random_list[i], "mask"))]))


            self.data_list.append(self.forward_atcam_table.make_data([gc.DataTuple('port', self.eg_port)], 'SwitchIngress.hit'))
            self.key_list_1[-1].apply_mask()
            self.key_list_2[-1].apply_mask()
            self.atcam_dict[self.key_list_1[-1]] = self.data_list[-1]
            self.atcam_dict[self.key_list_2[-1]] = self.data_list[-1]
        self.partition_key_1 = self.set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 6)])
        self.partition_data_1 = self.set_partition_table.make_data([gc.DataTuple('p_index', 3)], 'SwitchIngress.init_index')
        self.partition_key_2 = self.set_partition_table.make_key([gc.KeyTuple('hdr.ipv4.protocol', 17)])
        self.partition_data_2 = self.set_partition_table.make_data([gc.DataTuple('p_index', 1)], 'SwitchIngress.init_index')

    def add_entries(self):
        self.set_partition_table.entry_add(self.target, [self.partition_key_1], [self.partition_data_1])
        self.set_partition_table.entry_add(self.target, [self.partition_key_2], [self.partition_data_2])

        self.forward_atcam_table.entry_add(self.target, self.key_list_1, self.data_list)
        self.forward_atcam_table.entry_add(self.target, self.key_list_2, self.data_list)

    def send_traffic(self):
        def send_and_verify_packet(self, ingress_port, egress_port, pkt, exp_pkt):
            testutils.send_packet(self, ingress_port, pkt)
            testutils.verify_packet(self, exp_pkt, egress_port)

        logger.info("Sending traffic")
        for i in range(self.num_entries):
            dst_ip = self.key_list_1[i].to_dict()["hdr.ipv4.dst_addr"]["value"]
            pkt = testutils.simple_tcp_packet(ip_dst=dst_ip)
            exp_pkt = pkt
            send_and_verify_packet(self, self.ig_port, self.eg_port, pkt, exp_pkt)

            dst_ip = self.key_list_2[i].to_dict()["hdr.ipv4.dst_addr"]["value"]
            pkt = testutils.simple_tcp_packet(ip_dst=dst_ip)
            exp_pkt = pkt
            send_and_verify_packet(self, self.ig_port, self.eg_port, pkt, exp_pkt)


    def get_entries_and_verify(self):
        resp = self.forward_atcam_table.entry_get(self.target)
        atcam_dict = self.atcam_dict.copy()
        for data, key in resp:
            assert atcam_dict[key] == data
            atcam_dict.pop(key)
        assert len(atcam_dict) == 0

    def setUp(self):
        self.p4_name = "tna_ternary_match"
        HitlessBaseTest.setUp(self)


class HitlessTnaTernaryMatchIndirect(HitlessBaseTest):
    '''
    HA test for ternary indirect match table
    P4 program = tna_ternary_match
    '''

    def setup_tables(self):
        self.action_profile_table = self.bfrt_info.table_get("SwitchIngress.action_profile")
        self.tcam_direct_lpf_table = self.bfrt_info.table_get("SwitchIngress.tcam_direct_lpf")
        self.action_profile_table.info.data_field_annotation_add("srcAddr", "SwitchIngress.change_ipsrc", "ipv4")
        self.action_profile_table.info.data_field_annotation_add("dstAddr", "SwitchIngress.change_ipdst", "ipv4")
        self.tcam_direct_lpf_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")
        self.tcam_direct_lpf_table.info.key_field_annotation_add("hdr.ethernet.src_addr", "mac")

    def setup_test_data(self):
        logger.info("Setting up test data")
        tuple_list = []
        self.num_entries = 100

        self.ig_ports = [random.choice(swports) for x in range(self.num_entries)]
        self.all_ports = swports_0 + swports_1 + swports_2 + swports_3
        self.eg_ports = [random.choice(self.all_ports) for x in range(self.num_entries)]
        self.action_key_list = []
        self.action_data_list = []
        self.tcam_key_list = []
        self.tcam_data_list = []

        self.srcMac_dict = {}
        self.dstMac_dict = {}
        self.srcMacAddrs = []
        self.dstMacAddrs = []
        self.srcMacAddrsMask = []
        self.dstMacAddrsMask = []
        self.priorities = [x for x in range(self.num_entries)]
        random.shuffle(self.priorities)

        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        action_choices = ['SwitchIngress.change_ipsrc', 'SwitchIngress.change_ipdst']
        self.action = [action_choices[random.randint(0, 1)] for x in range(self.num_entries)]

        self.action_mbr_ids = [x + 1 for x in range(self.num_entries)]

        self.ipDstAddrs = ["{}.{}.{}.{}".format(
            random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
            range(self.num_entries)]
        self.ipSrcAddrs = ["{}.{}.{}.{}".format(
            random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255)) for x in
            range(self.num_entries)]

        self.lpf_types = [random.choice(["RATE", "SAMPLE"]) for x in range(self.num_entries)]

        self.gain_time = [round(random.uniform(1, 1000), 2) for x in range(self.num_entries)]
        self.decay_time = self.gain_time
        self.out_scale = [random.randint(1, 31) for x in range(self.num_entries)]

        self.srcMacAddrtuple = self.generate_random_mac_list(self.num_entries, self.seed)
        self.dstMacAddrtuple = self.generate_random_mac_list(self.num_entries, self.seed)

        self.srcMacAddrs = [getattr(each, "mac") for each in self.srcMacAddrtuple]
        self.srcMacAddrsMask = [getattr(each, "mask") for each in self.srcMacAddrtuple]

        self.dstMacAddrs = [getattr(each, "mac") for each in self.dstMacAddrtuple]
        self.dstMacAddrsMask = [getattr(each, "mask") for each in self.dstMacAddrtuple]

        for x in range(self.num_entries):
            if self.action[x] == 'SwitchIngress.change_ipsrc':
                self.action_key_list += [self.action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', self.action_mbr_ids[x])])]
                self.action_data_list += [self.action_profile_table.make_data([gc.DataTuple('dst_port', self.eg_ports[x]),
                                                     gc.DataTuple('srcAddr', self.ipSrcAddrs[x])],
                                                    'SwitchIngress.change_ipsrc')]
            elif self.action[x] == 'SwitchIngress.change_ipdst':
                self.action_key_list += [self.action_profile_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID', self.action_mbr_ids[x])])]
                self.action_data_list += [self.action_profile_table.make_data([gc.DataTuple('dst_port', self.eg_ports[x]),
                                                     gc.DataTuple('dstAddr', self.ipDstAddrs[x])],
                                                        'SwitchIngress.change_ipdst')]
            self.tcam_key_list += [self.tcam_direct_lpf_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', self.priorities[x]),
                                             gc.KeyTuple('hdr.ethernet.dst_addr',
                                                         self.dstMacAddrs[x],
                                                         self.dstMacAddrsMask[x]),
                                             gc.KeyTuple('hdr.ethernet.src_addr',
                                                         self.srcMacAddrs[x],
                                                         self.srcMacAddrsMask[x])])]
            self.tcam_data_list += [self.tcam_direct_lpf_table.make_data([gc.DataTuple('$ACTION_MEMBER_ID', self.action_mbr_ids[x]),
                                              gc.DataTuple('$LPF_SPEC_TYPE', str_val=self.lpf_types[x]),
                                              gc.DataTuple('$LPF_SPEC_GAIN_TIME_CONSTANT_NS',
                                                           float_val=self.gain_time[x]),
                                              gc.DataTuple('$LPF_SPEC_DECAY_TIME_CONSTANT_NS',
                                                           float_val=self.decay_time[x]),
                                              gc.DataTuple('$LPF_SPEC_OUT_SCALE_DOWN_FACTOR', self.out_scale[x])])]

    def add_entries(self):
        self.action_profile_table.entry_add(self.target, self.action_key_list, self.action_data_list)
        self.tcam_direct_lpf_table.entry_add(self.target, self.tcam_key_list, self.tcam_data_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        for x in range(self.num_entries):
            pkt = testutils.simple_tcp_packet(eth_src=self.srcMacAddrs[x],
                                              eth_dst=self.dstMacAddrs[x],
                                              with_tcp_chksum=False)
            if self.action[x] == 'SwitchIngress.change_ipsrc':
                exp_pkt = testutils.simple_tcp_packet(eth_src=self.srcMacAddrs[x],
                                                      eth_dst=self.dstMacAddrs[x],
                                                      ip_src=self.ipSrcAddrs[x],
                                                      with_tcp_chksum=False)
            elif self.action[x] == 'SwitchIngress.change_ipdst':
                exp_pkt = testutils.simple_tcp_packet(eth_src=self.srcMacAddrs[x],
                                                      eth_dst=self.dstMacAddrs[x],
                                                      ip_dst=self.ipDstAddrs[x],
                                                      with_tcp_chksum=False)
            testutils.send_packet(self, self.ig_ports[x], pkt)
            testutils.verify_packet(self, exp_pkt, self.eg_ports[x])
        testutils.verify_no_other_packets(self, timeout=2)

    def get_entries_and_verify(self):
        resp = self.tcam_direct_lpf_table.entry_get(self.target)
        x = 0
        for data, key in resp:
            data_dict = data.to_dict()
            '''
            assert data_dict["$LPF_SPEC_TYPE"] == self.lpf_types[x], "expected {} received {}".format(data_dict["$LPF_SPEC_TYPE"], self.lpf_types[x]))
            assert abs(data_dict["$LPF_SPEC_GAIN_TIME_CONSTANT_NS"] - self.gain_time[x]) <= self.gain_time[x] * 0.02
            assert abs(data_dict["$LPF_SPEC_DECAY_TIME_CONSTANT_NS"] - self.decay_time[x]) <= self.decay_time[x] * 0.02
            assert data_dict["$LPF_SPEC_OUT_SCALE_DOWN_FACTOR"] == self.out_scale[x]
            '''
            x += 1
        resp = self.action_profile_table.entry_get(self.target)
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            mbr_id = key_dict['$ACTION_MEMBER_ID']['value']
            x = mbr_id - 1
            assert data_dict["action_name"] == self.action[x]
            if data_dict["action_name"] == "SwitchIngress.change_ipsrc":
                assert data_dict["srcAddr"] == self.ipSrcAddrs[x]
            elif data_dict["action_name"] == "SwitchIngress.change_ipdst":
                assert data_dict["dstAddr"] == self.ipDstAddrs[x]

    def setUp(self):
        self.p4_name = "tna_ternary_match"
        HitlessBaseTest.setUp(self)

class HitlessTnaPortMetadata(HitlessBaseTest):
    '''
    HA test for port_metadata tables
    P4 program = tna_port_metadata
    '''

    def setup_tables(self):
        self.port_metadata_table = self.bfrt_info.table_get("SwitchIngressParser.$PORT_METADATA")
        self.port_md_exm_match_table = self.bfrt_info.table_get("SwitchIngress.port_md_exm_match")

    def make_phase0_data(self, field1, field2, field3, field4):
        """@brief Pack all fields into one phase0_data. For tofino 2, it is
        left shifted 64 more because the field is a 128 bit value
        """
        phase0data = (field1 << 48) | (field2 << 24) | (field3 << 8) | field4
        if testutils.test_param_get("arch") == "tofino":
            pass
        elif testutils.test_param_get("arch") == "tofino2":
            phase0data = phase0data << 64
        return phase0data

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.igr_to_egr_port_map = {}
        self.num_entries = 10
        igr_port_list = random.sample(swports, self.num_entries)
        egr_port_list = random.sample(swports, self.num_entries)
        for x in range(self.num_entries):
            self.igr_to_egr_port_map[igr_port_list[x]] = egr_port_list[x]
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        self.phase0_data_map = {}
        self.key_list = []
        self.data_list = []
        self.pm_dict = {}
        self.exm_key_list = []
        self.exm_data_list = []
        self.exm_dict = {}
        # Initialize the phase0 data map
        for key, value in list(self.igr_to_egr_port_map.items()):
            igr_port = key
            self.phase0_data_map[igr_port] = 0

        for key, value in list(self.igr_to_egr_port_map.items()):
            igr_port = key
            egr_port = value

            # For each igr port add a entry in the port_metadata (phase0) table
            # Form data to be programmed in the phase0 table for this ingress port
            phase0data = 0
            field1 = 0
            field2 = 0
            field3 = 0
            field4 = 0
            while True:
                field1 = random.randint(256, 0xffff)  # 16 bit
                field2 = random.randint(1, 0xffffff)  # 24 bits
                field3 = random.randint(1, 0xffff)  # 16 bits
                field4 = random.randint(1, 0xff)  # 8 bits

                phase0data = self.make_phase0_data(field1, field2, field3, field4)

                if self.phase0_data_map[igr_port] != phase0data:
                    self.phase0_data_map[igr_port] = phase0data
                    break

            self.key_list += [self.port_metadata_table.make_key([gc.KeyTuple('ig_intr_md.ingress_port', igr_port)])]
            self.data_list += [self.port_metadata_table.make_data([gc.DataTuple('$DEFAULT_FIELD', phase0data)])]
            self.pm_dict[self.key_list[-1]] = self.data_list[-1]

            # entry for the igr port in the exact match table
            self.exm_key_list += [self.port_md_exm_match_table.make_key(
                    [gc.KeyTuple('ig_md.port_md.field1', field1),
                     gc.KeyTuple('ig_md.port_md.field2', field2),
                     gc.KeyTuple('ig_md.port_md.field3', field3),
                     gc.KeyTuple('ig_md.port_md.field4', field4)])]
            self.exm_data_list += [self.port_md_exm_match_table.make_data(
                    [gc.DataTuple('port', egr_port)],
                    'SwitchIngress.hit')]
            self.exm_dict[self.exm_key_list[-1]] = self.exm_data_list[-1]

    def add_entries(self):
        self.port_metadata_table.entry_add(self.target, self.key_list, self.data_list)
        self.port_md_exm_match_table.entry_add(self.target, self.exm_key_list, self.exm_data_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        pkt = testutils.simple_tcp_packet()
        exp_pkt = pkt
        for key, value in list(self.igr_to_egr_port_map.items()):
            igr_port = key
            egr_port = value
            logger.info("Sending packet on port {}".format(igr_port))
            testutils.send_packet(self, igr_port, pkt)
            logger.info("Expecting packet on port {}".format(egr_port))
            testutils.verify_packet(self, exp_pkt, egr_port)
            logger.info("Packet received on port {} as expected".format(egr_port))

    def get_entries_and_verify(self):
        resp = self.port_metadata_table.entry_get(self.target)
        for data, key in resp:
            assert self.pm_dict[key] == data

        resp = self.port_md_exm_match_table.entry_get(self.target)
        for data, key in resp:
            assert self.exm_dict[key] == data


    def setUp(self):
        self.p4_name = "tna_port_metadata"
        HitlessBaseTest.setUp(self)

class HitlessTnaLpmAlpm(HitlessBaseTest):
    '''
    HA test for ALPM tables
    P4 program = tna_lpm_match
    '''

    def setup_tables(self):
        self.alpm_forward_table = self.bfrt_info.table_get("SwitchIngress.alpm_forward")
        self.alpm_forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
        self.alpm_forward_table.info.data_field_annotation_add("srcMac", "SwitchIngress.route", "mac")
        self.alpm_forward_table.info.data_field_annotation_add("dstMac", "SwitchIngress.route", "mac")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.key_list = []
        self.data_list = []
        self.alpm_dict = {}
        self.num_entries = random.randint(1, 30)
        self.target = gc.Target(device_id=0, pipe_id=0xffff)

        self.ig_port = swports[1]
        ip_list = self.generate_random_ip_list(self.num_entries, self.seed)
        for i in range(0, self.num_entries):
            vrf = 0
            dst_ip = getattr(ip_list[i], "ip")
            p_len = getattr(ip_list[i], "prefix_len")

            srcMac = "%02x:%02x:%02x:%02x:%02x:%02x" % tuple([random.randint(0, 255) for x in range(6)])
            dstMac = "%02x:%02x:%02x:%02x:%02x:%02x" % tuple([random.randint(0, 255) for x in range(6)])
            eg_port = swports[random.randint(1, 4)]

            target = gc.Target(device_id=0, pipe_id=0xffff)
            self.key_list += [self.alpm_forward_table.make_key([gc.KeyTuple('vrf', vrf),
                                              gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip, prefix_len=p_len)])]
            self.data_list += [self.alpm_forward_table.make_data([gc.DataTuple('dst_port', eg_port),
                                               gc.DataTuple('srcMac', srcMac),
                                               gc.DataTuple('dstMac', dstMac)],
                                              'SwitchIngress.route')]
            self.key_list[-1].apply_mask()
            self.alpm_dict[self.key_list[-1]] = self.data_list[-1]

    def add_entries(self):
        self.alpm_forward_table.entry_add(self.target, self.key_list, self.data_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        for k, d in zip(self.key_list, self.data_list):
            key = k.to_dict()
            data = d.to_dict()
            pkt = testutils.simple_tcp_packet(ip_dst=key["hdr.ipv4.dst_addr"]["value"])
            exp_pkt = testutils.simple_tcp_packet(eth_dst=data["dstMac"],
                                                  eth_src=data["srcMac"],
                                                  ip_dst=key["hdr.ipv4.dst_addr"]["value"])
            logger.info("Sending packet on port {}".format(self.ig_port))
            testutils.send_packet(self, self.ig_port, pkt)

            logger.info("Verifying entry for IP address {}, prefix_length {}"
                        .format(key["hdr.ipv4.dst_addr"]["value"], key["hdr.ipv4.dst_addr"]["prefix_len"]))
            logger.info("Expecting packet on port {}".format(data["dst_port"]))
            testutils.verify_packet(self, exp_pkt, data["dst_port"])

    def get_entries_and_verify(self):
        resp = self.alpm_forward_table.entry_get(self.target)
        for data, key in resp:
            assert self.alpm_dict[key] == data

    def setUp(self):
        self.p4_name = "tna_lpm_match"
        HitlessBaseTest.setUp(self)

class HitlessTnaRange(HitlessBaseTest):
    '''
    HA test for Range tables
    P4 program = tna_range_match
    '''

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.ig_port = swports[1]
        self.eg_ports = [swports[5], swports[3]]
        self.num_entries = 10
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)

        self.key_list = []
        self.data_list = []

        for i in range(0, self.num_entries):
            vrf = 0
            range_size = random.randint(1, 511)
            dst_ip = "{}.{}.{}.{}".format(
                random.randint(1, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
            pkt_length_start = random.randint(60, 511)
            self.key_list += [self.forward_table.make_key([gc.KeyTuple('$MATCH_PRIORITY', 1),
                                         gc.KeyTuple('hdr.ipv4.dst_addr', dst_ip),
                                         gc.KeyTuple('hdr.ipv4.total_len',
                                                         low=pkt_length_start,
                                                         high=pkt_length_start + range_size)])]
            self.data_list += [self.forward_table.make_data([gc.DataTuple('port', self.eg_ports[0])], 'SwitchIngress.hit')]

    def add_entries(self):
        self.forward_table.entry_add(self.target, self.key_list, self.data_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        for k, d in zip(self.key_list, self.data_list):
            # select a random length between the range
            key = k.to_dict()
            data = d.to_dict()
            eth_hdr_size = 14

            dst_ip = key["hdr.ipv4.dst_addr"]["value"]
            pkt_length_start = key["hdr.ipv4.total_len"]["low"]
            pkt_length_end = key["hdr.ipv4.total_len"]["high"]
            range_size = key["hdr.ipv4.total_len"]["high"] - key["hdr.ipv4.total_len"]["low"]

            pkt_len = random.randint(pkt_length_start, pkt_length_end) + eth_hdr_size
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            exp_pkt = pkt
            logger.info("Sending packet on port {} for with total_len {}".format(self.ig_port, pkt_len - eth_hdr_size))
            testutils.send_packet(self, self.ig_port, pkt)

            logger.info("Expecting packet on port {}".format(self.eg_ports[0]))
            testutils.verify_packet(self, exp_pkt, self.eg_ports[0])

        for k, d in zip(self.key_list, self.data_list):
            key = k.to_dict()
            data = d.to_dict()
            eth_hdr_size = 14

            dst_ip = key["hdr.ipv4.dst_addr"]["value"]
            pkt_length_start = key["hdr.ipv4.total_len"]["low"]
            pkt_length_end = key["hdr.ipv4.total_len"]["high"]
            range_size = key["hdr.ipv4.total_len"]["high"] - key["hdr.ipv4.total_len"]["low"]
            # select a length more than the range, it should be dropped
            pkt_len = pkt_length_end + eth_hdr_size + 2
            pkt = testutils.simple_tcp_packet(pktlen=pkt_len, ip_dst=dst_ip)
            exp_pkt = pkt
            logger.info("Sending packet on port {} with total_len {}".format(self.ig_port, pkt_len - eth_hdr_size))
            testutils.send_packet(self, self.ig_port, pkt)

            logger.info("Packet is expected to get dropped.")
            testutils.verify_no_other_packets(self)

    def get_entries_and_verify(self):
        resp = self.forward_table.entry_get(self.target)
        i=0
        for data, key in resp:
            assert key == self.key_list[i], "received {} expected {}".format(key, self.key_list[i])
            assert data == self.data_list[i]
            i+=1

    def setUp(self):
        self.p4_name = "tna_range_match"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        super(HitlessTnaRange, self).tearDown()

class HitlessTnaActionSelector(HitlessBaseTest):
    '''
    HA test for Action Selector tables
    P4 program = tna_action_selector
    '''

    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.action_table = self.bfrt_info.table_get("SwitchIngress.example_action_selector_ap")
        self.sel_table = self.bfrt_info.table_get("SwitchIngress.example_action_selector")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.num_entries = 4
        #self.seed = 1076
        #self.seed = 30971
        self.ig_ports = [swports[i] for i in range(self.num_entries)]
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.max_grp_size = 7

        self.match_key_list = []
        self.match_data_list = []
        self.action_key_list = []
        self.action_data_list = []
        self.sel_key_list = []
        self.sel_data_list = []

        self.num_act_prof_entries = len(swports) - self.num_entries
        self.num_sel_grps = 100

        self.egress_ports = [swports[i] for i in range(self.num_entries, len(swports))]
        self.action_mbr_ids = [x for x in range(self.num_act_prof_entries)]
        self.sel_grp_ids = [x for x in range(self.num_sel_grps)]
        self.status = [True, False]
        self.num_mbrs_in_grps = [random.randint(1, 7) for x in range(self.num_sel_grps)]
        self.mbrs_in_grps = [(random.sample(self.action_mbr_ids, self.num_mbrs_in_grps[x]),
                         [self.status[random.randint(0, 1)]
                          for y in range(self.num_mbrs_in_grps[x])])
                        for x in range(self.num_sel_grps)]

        # Make status of atleast one of the port as active to avoid failure when forward table reference this
        for mem, status in self.mbrs_in_grps:
          status[0] = True

        # Construct input for selector table
        # This list contains dictionaries for each entry
        # dict(grp_id -> dict(act_member -> mem_status) )
        self.mem_dict_dict = {}
        for j in range(self.num_sel_grps):
            members, member_status = self.mbrs_in_grps[j]
            mem_dict = {members[i]: member_status[i]
                        for i in range(0, len(members))}
            self.mem_dict_dict[self.sel_grp_ids[j]] = mem_dict

        logger.info("Making {} entries to action profile table".format(self.num_act_prof_entries))
        for j in range(self.num_act_prof_entries):
            # Create a new member for each port with the port number as the id.
            self.action_key_list += [self.action_table.make_key([gc.KeyTuple('$ACTION_MEMBER_ID',
                                                    self.action_mbr_ids[j])])]
            self.action_data_list += [self.action_table.make_data([gc.DataTuple('port', self.egress_ports[j])],
                                        'SwitchIngress.hit')]

        # Add the new member to the selection table.
        logger.info("Making {} groups for selector table".format(self.num_sel_grps))
        for grp_id, mem_dict in self.mem_dict_dict.items():
            self.sel_key_list += [self.sel_table.make_key([gc.KeyTuple('$SELECTOR_GROUP_ID',
                                                 grp_id)])]
            self.sel_data_list += [self.sel_table.make_data([gc.DataTuple('$MAX_GROUP_SIZE',
                                                   self.max_grp_size),
                                      gc.DataTuple('$ACTION_MEMBER_ID',
                                                   int_arr_val=list(mem_dict.keys())),
                                      gc.DataTuple('$ACTION_MEMBER_STATUS',
                                                   bool_arr_val=list(mem_dict.values()))])]
        # Add entry to the forward table
        # Select one out of Action mem ID or Select Grp ID
        for i in range(self.num_entries):
            fwd_data = random.choice(["$SELECTOR_GROUP_ID", "$ACTION_MEMBER_ID"])
            self.match_key_list += [self.forward_table.make_key([gc.KeyTuple('ig_intr_md.ingress_port',
                                                 self.ig_ports[i])])]
            if fwd_data == "$SELECTOR_GROUP_ID":
                self.match_data_list += [self.forward_table.make_data([gc.DataTuple('$SELECTOR_GROUP_ID',
                                                   random.choice(list(self.mem_dict_dict.keys()))
                                                   )])]
            else:
                self.match_data_list += [self.forward_table.make_data([gc.DataTuple('$ACTION_MEMBER_ID',
                                                   random.choice(self.action_mbr_ids)
                                                   )])]

    def add_entries(self):
        self.action_table.entry_add(self.target, self.action_key_list, self.action_data_list)
        self.sel_table.entry_add(self.target, self.sel_key_list, self.sel_data_list)
        self.forward_table.entry_add(self.target, self.match_key_list, self.match_data_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        for i in range(self.num_entries):
            logger.info("Match entry #{}".format(i))
            data = self.match_data_list[i].to_dict()
            eg_ports = []
            if "$SELECTOR_GROUP_ID" in data:
                # Get the action members which are active and make a list
                # of possible eg_ports
                logger.info("Sending to one of selector entries")
                act_dict = self.mem_dict_dict[data["$SELECTOR_GROUP_ID"]]
                mem_id_list = [mem for mem, status in act_dict.items() if status]
                for mem in mem_id_list:
                    for j in range(self.num_act_prof_entries):
                        if self.action_mbr_ids[j] == mem:
                            eg_ports.append(self.egress_ports[j])
            else:
                for j in range(self.num_act_prof_entries):
                    if self.action_mbr_ids[j] == data["$ACTION_MEMBER_ID"]:
                        eg_ports.append(self.egress_ports[j])
            if len(eg_ports) == 0:
                logger.info("empty eg_ports!")
                continue

            pkt = testutils.simple_tcp_packet()
            exp_pkt = pkt
            logger.info("Sending packet on port {}".format(self.ig_ports[i]))
            testutils.send_packet(self, self.ig_ports[i], pkt)
            logger.info("Expecting packet on one of enabled ports {}".format(eg_ports))
            testutils.verify_any_packet_any_port(self, [exp_pkt], eg_ports)


    def get_entries_and_verify(self):
        # Validate forward table
        # Cant predict the order/sequence after hitless replay, so need to compare by entries
        i=0
        for key in self.match_key_list:
            resp = self.forward_table.entry_get( self.target, [key], {"from_hw": True})

            for d, k in resp:
                assert k == key, "received {} expected {}".format(k, key)
                assert d == self.match_data_list[i], "received {} expected {}".format(d, self.match_data_list[i])
            i+=1

        # Validate Action table
        i=0
        for key in self.action_key_list:
            resp = self.action_table.entry_get( self.target, [key], {"from_hw": False})

            for d, k in resp:
                assert k == key, "received {} expected {}".format(k, key)
                assert d == self.action_data_list[i], "received {} expected {}".format(d, self.action_data_list[i])
            i+=1

        # Validate Selector table
        i=0
        for key in self.sel_key_list:
            resp = self.sel_table.entry_get( self.target, [key], {"from_hw": True})

            data_dict = next(resp)[0].to_dict()
            inital_data_dict = self.sel_data_list[i].to_dict()
            assert sorted(data_dict["$ACTION_MEMBER_ID"])== sorted(inital_data_dict["$ACTION_MEMBER_ID"]),\
                "received {} expected {}".format(data_dict["$ACTION_MEMBER_ID"], inital_data_dict["$ACTION_MEMBER_ID"])
            i+=1

    def setUp(self):
        self.p4_name = "tna_action_selector"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        super(HitlessTnaActionSelector, self).tearDown()

class HitlessKeyLessTable(HitlessBaseTest):
    '''
    HA test for Keyless table
    P4 program = tna_bridged_md
    '''

    def setup_tables(self):
        self.table_output_port = self.bfrt_info.table_get("SwitchIngress.output_port")
        self.table_bridge_md_ctl= self.bfrt_info.table_get("SwitchIngress.bridge_md_ctrl")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.eg_port = swports[1]
        self.ig_ports = [swports[x] for x in range(6)]

        self.bridged_data = self.table_bridge_md_ctl.make_data([],
                "SwitchIngress.bridge_add_ig_intr_md")
        self.bridged_key = self.table_bridge_md_ctl.make_key([])

        self.output_data = self.table_output_port.make_data(
            [gc.DataTuple("port_id", self.eg_port)],
            "SwitchIngress.set_output_port")

    def add_entries(self):
        logger.info("Adding entries")
        self.table_bridge_md_ctl.default_entry_set(self.target, self.bridged_data)
        self.table_output_port.default_entry_set(self.target, self.output_data)

    def send_traffic(self):
        logger.info("Sending traffic")

        ipkt = testutils.simple_tcp_packet(eth_dst='11:11:11:11:11:11',
                                           eth_src='22:33:44:55:66:77',
                                           ip_src='1.2.3.4',
                                           ip_dst='100.99.98.97',
                                           ip_id=101,
                                           ip_ttl=64,
                                           tcp_sport=0x1234,
                                           tcp_dport=0xabcd,
                                           with_tcp_chksum=True)

        epkt_tmpl = testutils.simple_tcp_packet(eth_dst='00:00:00:00:00:02',
                                                eth_src='22:33:44:55:66:77',
                                                ip_src='1.2.3.4',
                                                ip_dst='100.99.98.97',
                                                ip_id=101,
                                                ip_ttl=64,
                                                tcp_sport=0x1234,
                                                tcp_dport=0xabcd,
                                                with_tcp_chksum=True)

        epkts = []
        for p in self.ig_ports:
            epkt = epkt_tmpl.copy()[scapy.Ether]
            dmac = "00:00:00:00:{:02x}:{:02x}".format(p >> 8, p & 0xFF)
            epkt.dst = dmac
            epkts.append(epkt)

        for i,p in enumerate(self.ig_ports):
            testutils.send_packet(self, p, ipkt)
            testutils.verify_packet(self, epkts[i], self.eg_port)


    def get_entries_and_verify(self):
        resp = self.table_bridge_md_ctl.default_entry_get(self.target)
        for data, key in resp:
            assert data == self.bridged_data
        resp = self.table_output_port.default_entry_get(self.target)
        for data, key in resp:
            assert data == self.output_data

    def setUp(self):
        self.p4_name = "tna_bridged_md"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        super(HitlessKeyLessTable, self).tearDown()

class HitlessHashActionTable(HitlessBaseTest):
    '''
    HA test for Hash action table. Tables which are of size = 2^key_size
    P4 program = tna_mirror
    '''

    def setup_tables(self):
        self.mirror_cfg_table = self.bfrt_info.table_get("$mirror.cfg")
        self.mirror_fwd_table = self.bfrt_info.table_get("mirror_fwd")

    def setup_test_data(self):
        logger.info("Setting up test data")

        if testutils.test_param_get("arch") == "tofino":
            MIR_SESS_COUNT = 1024
            MAX_SID_NORM = 1015
            MAX_SID_COAL = 1023
            BASE_SID_NORM = 1
            BASE_SID_COAL = 1016
            self.EXP_LEN1 = 127
            self.EXP_LEN2 = 63
        elif testutils.test_param_get("arch") == "tofino2":
            MIR_SESS_COUNT = 256
            MAX_SID_NORM = 255
            MAX_SID_COAL = 255
            BASE_SID_NORM = 0
            BASE_SID_COAL = 0
            self.EXP_LEN1 = 127
            self.EXP_LEN2 = 59

        self.sids = random.sample(range(BASE_SID_NORM, MAX_SID_NORM), len(swports))
        self.sids.sort()

        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.mirror_fwd_key = []
        self.mirror_fwd_data = []
        self.mirror_cfg_key = []
        self.mirror_cfg_data = []

        for port, sid in zip(swports, self.sids):
            self.mirror_fwd_key += [self.mirror_fwd_table.make_key([
                gc.KeyTuple('ig_intr_md.ingress_port', port)])]
            self.mirror_fwd_data += [self.mirror_fwd_table.make_data([gc.DataTuple('dest_port', 511),
                                          gc.DataTuple('ing_mir', 1),
                                          gc.DataTuple('ing_ses', sid),
                                          gc.DataTuple('egr_mir', 0),
                                          gc.DataTuple('egr_ses', 0)],
                                         'SwitchIngress.set_md')]
            if port % 2 == 0:
                max_len = 128
            else:
                max_len = 64
            self.mirror_cfg_key += [self.mirror_cfg_table.make_key([gc.KeyTuple('$sid', sid)])]
            self.mirror_cfg_data += [self.mirror_cfg_table.make_data([gc.DataTuple('$direction', str_val="INGRESS"),
                                                 gc.DataTuple('$ucast_egress_port', port),
                                                 gc.DataTuple('$ucast_egress_port_valid', bool_val=True),
                                                 gc.DataTuple('$session_enable', bool_val=True),
                                                 gc.DataTuple('$max_pkt_len', max_len)],
                                                '$normal')]


    def add_entries(self):
        logger.info("Adding entries")
        self.mirror_cfg_table.entry_add(self.target, self.mirror_cfg_key, self.mirror_cfg_data)
        self.mirror_fwd_table.entry_add(self.target, self.mirror_fwd_key, self.mirror_fwd_data)


    def send_traffic(self):
        logger.info("Sending traffic")
        pkt = simple_eth_packet(pktlen=79)
        pkt = simple_eth_packet(pktlen=200)
        rec_pkt1 = simple_eth_packet(pktlen=self.EXP_LEN1)
        rec_pkt2 = simple_eth_packet(pktlen=self.EXP_LEN2)
        for port in swports:
            send_packet(self, port, pkt)
            if port % 2 == 0:
                verify_packet(self, rec_pkt1, port)
            else:
                verify_packet(self, rec_pkt2, port)
        verify_no_other_packets(self)

    def get_entries_and_verify(self):
        resp = self.mirror_fwd_table.entry_get(self.target)

        for data, key in resp:
            found_key = False
            i = 0
            for cfg_key in self.mirror_fwd_key:
                if key == cfg_key:
                    found_key = True
                    assert data == self.mirror_fwd_data[i], "received {} expected {}".format(data, self.mirror_fwd_data[i])
                    break
                i += 1
            assert found_key, "Key {} and {} didn't match in self.mirror_cfg_key, self.mirror_fwd_data"

        resp = self.mirror_cfg_table.entry_get(self.target)
        for data, key in resp:
            found_key = False
            for cfg_key in self.mirror_cfg_key:
                if key == cfg_key:
                    found_key = True
                    break
            # assert key == self.mirror_cfg_key, "received {} expected {}".format(key, self.mirror_cfg_ke)
            assert found_key, "Key {} didn't mach in self.mirror_cfg_key"
            #TODO fix below. server sending more fields than being sent by client. Some are garbage.  Make madatory work
            #assert data == self.mirror_cfg_data[i], "received {} expected {}".format(data, self.mirror_cfg_data[i])

    def setUp(self):
        self.p4_name = "tna_mirror"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        super(HitlessHashActionTable, self).tearDown()

class HitlessPVS(HitlessBaseTest):
    '''
    P4 program = tna_pvs
    '''

    def setup_tables(self):
        self.vs_table = self.bfrt_info.table_get("ParserI.vs")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.target = gc.Target(device_id=0, pipe_id=0xffff, direction=0xff, prsr_id=0xff)
        self.vs_table.attribute_entry_scope_set(self.target,
                                           config_gress_scope=True, predefined_gress_scope_val=bfruntime_pb2.Mode.ALL,
                                           config_pipe_scope=True, predefined_pipe_scope=True,
                                           predefined_pipe_scope_val=bfruntime_pb2.Mode.ALL, pipe_scope_args=0xff,
                                           config_prsr_scope=True, predefined_prsr_scope_val=bfruntime_pb2.Mode.ALL,
                                           prsr_scope_args=0xff)
        self.key_list = []
        for i in [1, 2, 3, 4]:
            f16 = i
            f8 = i + 10
            self.key_list += [self.vs_table.make_key([gc.KeyTuple('f16', f16, 0xffff),
                                   gc.KeyTuple('f8', f8, 0xff)])]

    def add_entries(self):
        logger.info("Adding entries")
        self.vs_table.entry_add(self.target, self.key_list)

    def get_entries_and_verify(self):
        logger.info("Verifying get entry")
        resp = self.vs_table.entry_get(self.target)
        i=0
        for data, key in resp:
            "==============="
            logger.info(key.to_dict())
            logger.info(data.to_dict())
            assert key == self.key_list[i], "received {} expected {}".format(key, self.key_list[i])
            i+=1


    def setUp(self):
        self.p4_name = "tna_pvs"
        HitlessBaseTest.setUp(self)

class HitlessDynHashing(HitlessBaseTest):
    '''
    P4 program = tna_dyn_hashing
    '''

    def setup_tables(self):
        self.hash_config_table = self.bfrt_info.table_get("c_hash.configure")

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
                        {"order": gc.DataTuple("order", slice_[0]),
                         "start_bit":gc.DataTuple("start_bit", slice_[1]),
                         "length":gc.DataTuple("length", slice_[2])
                         })
            hash_field_slice_list.append(gc.DataTuple(name, container_arr_val = inner_tuple))
        return table.make_data(hash_field_slice_list)

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        slice_tuple_list = []
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.src_addr",   start_bit=0, length=32, order=4))
        slice_tuple_list.append(HashFieldSlice(name="hdr.ipv4.dst_addr",   start_bit=0, length=32, order=1))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.src_port",    start_bit=0, length=16, order=2))
        slice_tuple_list.append(HashFieldSlice(name="hdr.tcp.dst_port",    start_bit=0, length=16, order=3))
        self.data_list = [self.create_hash_data(self.hash_config_table, slice_tuple_list)]

    def add_entries(self):
        logger.info("Adding entries")
        self.hash_config_table.default_entry_set(self.target, self.data_list[0])


    def get_entries_and_verify(self):
        logger.info("Verifying get entry")
        resp = self.hash_config_table.default_entry_get(self.target)
        i=0
        for data, key in resp:
            assert data == self.data_list[i], "received {} expected {}".format(data, self.data_list[i])
            i+=1


    def setUp(self):
        self.p4_name = "tna_dyn_hashing"
        HitlessBaseTest.setUp(self)

class HitlessIndirectMeterTest(HitlessBaseTest):
    '''
    P4 program = tna_meter_lpf_wred
    '''

    def setup_tables(self):
        self.meter_table = self.bfrt_info.table_get("SwitchIngress.meter")
        self.match_table = self.bfrt_info.table_get("SwitchIngress.meter_color")
        self.match_table.info.key_field_annotation_add("hdr.ethernet.dst_addr", "mac")

    def getMeterData(self, num_entries):
        meter_data = {}
        meter_data['cir'] = [1000 * random.randint(1, 1000) for i in range(num_entries)]
        meter_data['pir'] = [meter_data['cir'][i] * random.randint(1, 5) for i in range(num_entries)]
        meter_data['cbs'] = [1000 * random.randint(1, 100) for i in range(num_entries)]
        meter_data['pbs'] = [meter_data['cbs'][i] * random.randint(1, 5) for i in range(num_entries)]
        return meter_data

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.num_entries =  random.randint(1,100)
        self.meter_key_list = []
        self.meter_data_list = []
        self.match_key_list = []
        self.match_data_list = []
        key_set = set()
        meter_indices = [x + 1 for x in range(self.num_entries)]
        logger.info("Number of entries {}".format(self.num_entries))
        self.match_dict = {}

        for i in range(self.num_entries):
            mac_addr = "%02x:%02x:%02x:%02x:%02x:%02x" % (
                random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
                random.randint(0, 255), random.randint(0, 255))
            while mac_addr in key_set:
                mac_addr = "%02x:%02x:%02x:%02x:%02x:%02x" % (
                    random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), random.randint(0, 255),
                    random.randint(0, 255), random.randint(0, 255))
            self.match_key_list += [self.match_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', mac_addr)])]
            self.match_data_list += [self.match_table.make_data(
                    [gc.DataTuple('meter_idx', meter_indices[i])],
                    'SwitchIngress.set_color')]
            key_set.add(mac_addr)
            self.match_dict[self.match_key_list[-1]] = self.match_data_list[-1]


        self.meter_data = self.getMeterData(self.num_entries)
        for x in range(self.num_entries):
            self.meter_key_list += [self.meter_table.make_key(
                    [gc.KeyTuple('$METER_INDEX', x)])]
            self.meter_data_list += [self.meter_table.make_data(
                    [gc.DataTuple('$METER_SPEC_CIR_KBPS',  self.meter_data['cir'][x]),
                     gc.DataTuple('$METER_SPEC_PIR_KBPS',  self.meter_data['pir'][x]),
                     gc.DataTuple('$METER_SPEC_CBS_KBITS', self.meter_data['cbs'][x]),
                     gc.DataTuple('$METER_SPEC_PBS_KBITS', self.meter_data['pbs'][x])])]

    def add_entries(self):
        logger.info("Adding entries")
        self.meter_table.entry_add(self.target, self.meter_key_list, self.meter_data_list)
        self.match_table.entry_add(self.target, self.match_key_list, self.match_data_list)


    def send_traffic(self):
        logger.info("Sending traffic")
        for x in range(self.num_entries):
            pkt = testutils.simple_tcp_packet(eth_dst=self.match_key_list[x].to_dict()["hdr.ethernet.dst_addr"]["value"],
                                                      with_tcp_chksum=False)
            testutils.send_packet(self, swports[0], pkt)

    def get_entries_and_verify(self):
        logger.info("Verifying get entry")
        resp = self.meter_table.entry_get(self.target, None, {"from_hw": False})
        i = 0
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            recv_cir = data_dict["$METER_SPEC_CIR_KBPS"]
            recv_pir = data_dict["$METER_SPEC_PIR_KBPS"]
            recv_cbs = data_dict["$METER_SPEC_CBS_KBITS"]
            recv_pbs = data_dict["$METER_SPEC_PBS_KBITS"]

            # Read back meter values are not always the same. It should be within a 2% error rate
            assert abs(recv_cir - self.meter_data['cir'][i]) < self.meter_data['cir'][i] * 0.02
            assert abs(recv_pir - self.meter_data['pir'][i]) < self.meter_data['pir'][i] * 0.02
            assert abs(recv_cbs - self.meter_data['cbs'][i]) < self.meter_data['cbs'][i] * 0.02
            assert abs(recv_pbs - self.meter_data['pbs'][i]) < self.meter_data['pbs'][i] * 0.02

            assert key_dict["$METER_INDEX"]['value'] == i
            i += 1
            if i == self.num_entries:
                break

        resp = self.match_table.entry_get(self.target)
        i=0
        for data, key in resp:
            assert self.match_dict[key] == data
            i+=1


    def setUp(self):
        self.p4_name = "tna_meter_lpf_wred"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        HitlessBaseTest.tearDown(self)

class HitlessSelectorResize(HitlessBaseTest):
    '''
    P4 program = selector_resize
    '''

    def setup_tables(self):
        self.act_profile = self.bfrt_info.table_get("nexthop_shared.act_profile")
        self.selector = self.bfrt_info.table_get("nexthop_shared.ecmp_selector")
        # Ternary table doesn't need ttl to be set
        self.match_table = self.bfrt_info.table_get("nexthop_shared.ecmp_t")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.target = gc.Target(device_id=0, pipe_id=0xFFFF)
        self.num_entries =  random.randint(1,100)
        self.sel_grp_id = 0
        self.sel_sizes = [self.num_entries, self.num_entries +1, self.num_entries + 2]
        self.ap_key_list = []
        self.ap_data_list = []
        self.ap_entries = []
        self.match_key_list = []
        self.match_data_list = []
        self.dmacs = []
        self.sel_key_list = []
        self.sel_data_list = []
        self.ap_dict = {}
        self.mat_dict = {}
        logger.info("Number of entries {}".format(self.num_entries))

        for x in range(self.num_entries):
            key = self.act_profile.make_key(
                    [gc.KeyTuple('$ACTION_MEMBER_ID', x)])
            self.ap_key_list.append(key)
            data = self.act_profile.make_data(
                    [gc.DataTuple('dst_port', swports[1])],
                    'SwitchIngress.nexthop_shared.set_eg_port')
            self.ap_data_list.append(data)
            self.ap_entries.append(x)
            self.ap_dict[self.ap_key_list[-1]] = self.ap_data_list[-1]

        mask = 0xffffffffffff
        for x in range(self.num_entries):
            dmac = 0x1122334400 | x
            # Priority required to be in line with entry_get
            key = self.match_table.make_key(
                    [gc.KeyTuple('hdr.ethernet.dst_addr', dmac, mask),
                     gc.KeyTuple('$MATCH_PRIORITY', 0)])
            data = self.match_table.make_data(
                    [gc.DataTuple('$SELECTOR_GROUP_ID', self.sel_grp_id)])
            self.match_key_list.append(key)
            self.match_data_list.append(data)
            self.mat_dict[self.match_key_list[-1]] = self.match_data_list[-1]
            self.dmacs.append(dmac)

        self.sel_key = self.selector.make_key(
                [gc.KeyTuple('$SELECTOR_GROUP_ID', self.sel_grp_id)])
        for size in self.sel_sizes:
            data = self.selector.make_data(
                [
                    gc.DataTuple('$MAX_GROUP_SIZE', size),
                    gc.DataTuple('$ACTION_MEMBER_ID', int_arr_val=self.ap_entries),
                    gc.DataTuple('$ACTION_MEMBER_STATUS', bool_arr_val=[True] *
                        len(self.ap_entries))
                ])
            self.sel_data_list.append(data)

    def add_entries(self):
        logger.info("Adding entries")
        self.act_profile.entry_add(self.target, self.ap_key_list, self.ap_data_list)
        self.selector.entry_add(self.target, [self.sel_key], [self.sel_data_list[0]])
        self.match_table.entry_add(self.target, self.match_key_list, self.match_data_list)
        for sel_data in self.sel_data_list:
            self.selector.entry_mod(self.target, [self.sel_key], [sel_data])

    def send_traffic(self):
        logger.info("Sending traffic")
        for x in range(self.num_entries):
            pkt = testutils.simple_tcp_packet(eth_dst=
                    gc.bytes_to_mac(gc.to_bytes(self.dmacs[x], 6)))
            testutils.send_packet(self, swports[0], pkt)
            testutils.verify_packet(self, pkt, swports[1])

    def get_entries_and_verify(self):
        logger.info("Verifying get entry")

        resp = self.act_profile.entry_get(self.target)
        i=0
        for data, key in resp:
            assert self.ap_dict[key] == data
            i+=1
        assert i == self.num_entries

        resp = self.match_table.entry_get(self.target)
        i=0
        for data, key in resp:
            assert self.mat_dict[key] == data
            i+=1
        assert i == self.num_entries

        resp = self.selector.entry_get(self.target)
        i=0
        for data, key in resp:
            assert self.sel_key == key
            # After mod, last size should be applied
            #assert self.sel_data_list[-1] == data
            i+=1
        assert i == 1

    def setUp(self):
        self.p4_name = "selector_resize"
        HitlessBaseTest.setUp(self)

    def tearDown(self):
        self.interface.clear_all_tables()
        HitlessBaseTest.tearDown(self)


class FastSelectorResizeTest(HitlessSelectorResize):

    def tearDown(self):
        global warminit_hitless_mode
        warminit_hitless_mode = True
        HitlessSelectorResize.tearDown(self)

    def runTest(self):
        global warminit_hitless_mode
        warminit_hitless_mode = False
        HitlessSelectorResize.runTest(self)
