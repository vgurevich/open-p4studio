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
import ptf.packet as scapy
from bfruntime_client_base_tests import BfRuntimeTest, BaseTest
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

class WarminitBaseTest(BfRuntimeTest):
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
    6. Send a WARM_INIT_BEGIN SetForwardingPipelineConfig msg with HITLESS or FAST_RECONFIG
    7. Wait for WARM_INIT_STARTED
    8. Replay entries
    9. Send a WARM_INIT_END msg and wait for WARM_INIT_FINISHED
    10. verify entries
    '''
    def setup_tables(self):
        pass

    def setup_test_data(self):
        pass

    def add_entries(self):
        pass

    def del_entries(self):
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
        self.port_stat_table = self.bfrt_info.table_get("$PORT_STAT")
        self.target = gc.Target(device_id=0, pipe_id=0xffff)
        logger.info(self.target)
        self.num_pipes = bfrt_utils.get_num_pipes(self.bfrt_info.table_get("device_configuration"))
        bfrt_utils.add_ports(self)

        self.seed = random.randint(0, 65535)
        logger.info("Seed used  {}".format(self.seed))
        random.seed(self.seed)

    def runTest(self):
        if testutils.test_param_get("target") != "hw":
            self.setup_tables()
            self.setup_test_data()
            self.add_entries()
            # Start warm-init
            self.send_traffic()
            self.get_entries_and_verify()
            bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=hitless_mode)
            bfrt_utils.add_ports(self)
            self.replay_entries()
            bfrt_utils.end_warm_init(self)

            self.get_entries_and_verify()
            self.send_traffic()
        else:
            self.setup_tables()
            self.setup_test_data()
            self.add_entries()
            bfrt_utils.add_ports(self)
            # Start warm-init
            trd1 = threading.Thread(target=self.send_traffic)
            logger.info("Before thread start")
            trd1.start()
            self.get_entries_and_verify()
            logger.info("After thread start")
            bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=hitless_mode)
            bfrt_utils.add_ports(self)
            self.replay_entries()
            bfrt_utils.end_warm_init(self)

            trd1.join()
            self.get_entries_and_verify()

class WarminitTnaExactMatch(WarminitBaseTest):
    '''
    warm-init hitless or fast reconfig testcase for tna_exact_match
    '''
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
        logger.info("Adding entries")
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        data_list = [self.forward_table.make_data([gc.DataTuple('port', self.eg_port)],
                                             "SwitchIngress.hit")]
        self.forward_table.entry_add(self.target, key_list, data_list)

    def del_entries(self):
        logger.info("Deleting entries")
        key_list = [self.forward_table.make_key([gc.KeyTuple('hdr.ethernet.dst_addr', self.dmac)])]
        self.forward_table.entry_del(self.target, key_list)

    def send_traffic(self):
        if testutils.test_param_get("target") != "hw":
            pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
            exp_pkt = pkt
            logger.info("Sending packet on port {}".format(self.ig_port))
            testutils.send_packet(self, self.ig_port, pkt)
            logger.info("Expecting packet on port {}".format(self.eg_port))
            testutils.verify_packets(self, exp_pkt, [self.eg_port])
        else:
            num_pkts = testutils.test_param_get("num_pkts", 150000)
            pkt = testutils.simple_tcp_packet(eth_dst=self.dmac)
            exp_pkt = pkt
            logger.info("Sending packet on port {}".format(self.ig_port))
            egport_stats_before = get_traffic_stats(self)
            ts=time.time()
            testutils.send_packet(self, self.ig_port, pkt, num_pkts)
            te=time.time()
            ti_per_pkt=(te-ts)*1000/num_pkts
            logger.info("Traffic Start Time {}".format(ts))
            logger.info("Traffic End Time {}".format(te))
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            time.sleep(5)
            egport_stats_after = get_traffic_stats(self)
            loss_ti_ms = ti_per_pkt * (num_pkts - (egport_stats_after - egport_stats_before))
            logger.info("---------------------------------------------")
            logger.info("Traffic loss with warm-init {} is {} ms".format("hitless" if hitless_mode else "fast reconfig", loss_ti_ms))
            logger.info("---------------------------------------------")
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))
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
        logger.info("In setup")
        self.p4_name = "tna_exact_match"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaTernaryMatch(WarminitBaseTest):
    '''
    warm-init hitless or fast reconfig testcase for tna_ternary_match
    '''
    def setup_tables(self):
        self.forward_table = self.bfrt_info.table_get("SwitchIngress.forward")
        self.forward_table.info.key_field_annotation_add("hdr.ipv4.dst_addr", "ipv4")
        self.port_table = self.bfrt_info.table_get("$PORT")
        self.port_stat_table = self.bfrt_info.table_get("$PORT_STAT")

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
        logger.info("Adding entries")
        self.forward_table.entry_add(self.target, self.key_list, self.data_list)

    def del_entries(self):
        logger.info("Deleting entries")
        self.forward_table.entry_del(self.target, self.key_list)

    def send_traffic(self):
        def send_and_verify_packet(self, ingress_port, egress_port, pkt, exp_pkt, num_pkts):
            testutils.send_packet(self, ingress_port, pkt, num_pkts)
            testutils.verify_packet(self, exp_pkt, egress_port)

        if testutils.test_param_get("target") != "hw":
            logger.info("Sending traffic on port {} and verify on port {}".format(self.ig_port, self.eg_port))
            for i in range(self.num_entries):
                dst_ip = self.key_list[i].to_dict()["hdr.ipv4.dst_addr"]["value"]
                pkt = testutils.simple_tcp_packet(ip_dst=dst_ip)
                exp_pkt = pkt
                send_and_verify_packet(self, self.ig_port, self.eg_port, pkt, exp_pkt, 1)
        else:
            logger.info("Sending traffic on port {}".format(self.ig_port))
            num_pkts = testutils.test_param_get("num_pkts", 1000)
            egport_stats_before = get_traffic_stats(self)
            ts=time.time()
            for i in range(self.num_entries):
                dst_ip = self.key_list[i].to_dict()["hdr.ipv4.dst_addr"]["value"]
                pkt = testutils.simple_tcp_packet(ip_dst=dst_ip)
                exp_pkt = pkt
                send_and_verify_packet(self, self.ig_port, self.eg_port, pkt, exp_pkt, num_pkts)
            te=time.time()
            tot_pkts = num_pkts*self.num_entries
            ti_per_pkt=(te-ts)*1000/tot_pkts
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            time.sleep(2)
            egport_stats_after = get_traffic_stats(self)
            loss_ti_ms = ti_per_pkt * (tot_pkts - (egport_stats_after - egport_stats_before))
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))
        logger.info("End of send_traffic")

    def get_entries_and_verify(self):
        logger.info("Get entries")
        resp = self.forward_table.entry_get(self.target)
        i=0
        for data, key in resp:
            self.key_list[i].apply_mask()
            assert key == self.key_list[i], "received {} expected {}".format(key, self.key_list[i])
            assert data == self.data_list[i]
            i+=1

    def setUp(self):
        self.p4_name = "tna_ternary_match"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaPortMetadata(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_port_metadata
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
        elif testutils.test_param_get("arch") == "tofino2" or testutils.test_param_get("arch") == "tofino3":
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

        loop_len = len(list(self.igr_to_egr_port_map.items()))
        field1 = random.sample(range(256, 0xffff), loop_len)
        field2 = random.sample(range(1, 0xffffff), loop_len)
        field3 = random.sample(range(1, 0xffff), loop_len)
        field4 = random.sample(range(1, 0xff), loop_len)

        for ((key, value), f1, f2, f3, f4) in zip(list(self.igr_to_egr_port_map.items()), field1, field2, field3, field4):
            igr_port = key
            self.phase0_data_map[igr_port] = 0
            egr_port = value

            phase0data = self.make_phase0_data(f1, f2, f3, f4)

            if self.phase0_data_map[igr_port] != phase0data:
                self.phase0_data_map[igr_port] = phase0data

            self.key_list += [self.port_metadata_table.make_key([gc.KeyTuple('ig_intr_md.ingress_port', igr_port)])]
            self.data_list += [self.port_metadata_table.make_data([gc.DataTuple('$DEFAULT_FIELD', phase0data)])]
            self.pm_dict[self.key_list[-1]] = self.data_list[-1]

            # entry for the igr port in the exact match table
            self.exm_key_list += [self.port_md_exm_match_table.make_key(
                    [gc.KeyTuple('ig_md.port_md.field1', f1),
                     gc.KeyTuple('ig_md.port_md.field2', f2),
                     gc.KeyTuple('ig_md.port_md.field3', f3),
                     gc.KeyTuple('ig_md.port_md.field4', f4)])]
            self.exm_data_list += [self.port_md_exm_match_table.make_data(
                    [gc.DataTuple('port', egr_port)],
                    'SwitchIngress.hit')]
            self.exm_dict[self.exm_key_list[-1]] = self.exm_data_list[-1]

    def add_entries(self):
        self.port_metadata_table.entry_add(self.target, self.key_list, self.data_list)
        self.port_md_exm_match_table.entry_add(self.target, self.exm_key_list, self.exm_data_list)

    def del_entries(self):
        logger.info("Deleting entries")
        self.port_metadata_table.entry_del(self.target, self.key_list)
        self.port_md_exm_match_table.entry_del(self.target, self.exm_key_list)

    def send_traffic(self):
        pkt = testutils.simple_tcp_packet()
        exp_pkt = pkt
        for key, value in list(self.igr_to_egr_port_map.items()):
            igr_port = key
            egr_port = value
            if testutils.test_param_get("target") != "hw":
                logger.info("Sending packet on port {} and receiving on {}".format(igr_port, egr_port))
                testutils.send_packet(self, igr_port, pkt)
                testutils.verify_packet(self, exp_pkt, egr_port)
            else:
                testutils.send_packet(self, igr_port, pkt, self.num_pkts)

    def get_entries_and_verify(self):
        logger.info("Getting entries")
        resp = self.port_metadata_table.entry_get(self.target)
        for data, key in resp:
            assert self.pm_dict[key] == data

        resp = self.port_md_exm_match_table.entry_get(self.target)
        for data, key in resp:
            assert self.exm_dict[key] == data

    def setUp(self):
        self.p4_name = "tna_port_metadata"
        WarminitBaseTest.setUp(self)

    def runTest(self):
        if testutils.test_param_get("target") != "hw":
            self.setup_tables()
            self.setup_test_data()
            self.add_entries()
            # Start warm-init
            self.send_traffic()
            self.get_entries_and_verify()
            bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=hitless_mode)
            bfrt_utils.add_ports(self)
            self.replay_entries()
            bfrt_utils.end_warm_init(self)

            self.get_entries_and_verify()
            self.send_traffic()
        else:
            self.setup_tables()
            self.setup_test_data()
            self.add_entries()
            bfrt_utils.add_ports(self)

            # Start warm-init
            self.num_pkts = testutils.test_param_get("num_pkts", 10000)
            egport_stats_before = 0
            for key, value in list(self.igr_to_egr_port_map.items()):
                self.eg_port = value
                self.ig_port = key
                egport_stats_before = egport_stats_before + get_traffic_stats(self)
            logger.info("Sending Traffic")
            ts=time.time()
            trd1 = threading.Thread(target=self.send_traffic)
            logger.info("Before thread start")
            trd1.start()
            self.get_entries_and_verify()
            logger.info("After thread start")
            bfrt_utils.start_warm_init(self, self.p4_name, self.num_pipes, hitless=hitless_mode)
            bfrt_utils.add_ports(self)
            self.replay_entries()
            bfrt_utils.end_warm_init(self)

            trd1.join()
            te=time.time()
            tot_pkts = self.num_pkts * len(self.igr_to_egr_port_map.items())
            ti_per_pkt=(te-ts)*1000/tot_pkts
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            self.get_entries_and_verify()
            time.sleep(2)
            egport_stats_after = 0
            for key, value in list(self.igr_to_egr_port_map.items()):
                self.eg_port = value
                self.ig_port = key
                egport_stats_after = egport_stats_after + get_traffic_stats(self)
            loss_ti_ms = ti_per_pkt * (tot_pkts - (egport_stats_after - egport_stats_before))
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))

    def tearDown(self):
        self.del_entries()
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaRangeMatch(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_range_match
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

        for range_size in random.sample(range(1, 511), self.num_entries):
            vrf = 0
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

    def del_entries(self):
        self.forward_table.entry_del(self.target, self.key_list)

    def send_traffic(self):
        logger.info("Sending traffic")
        if testutils.test_param_get("target") == "hw":
            num_pkts = testutils.test_param_get("num_pkts", 10000)
            self.eg_port = self.eg_ports[0]
            egport_stats_before = get_traffic_stats(self)

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
            if testutils.test_param_get("target") != "hw":
                testutils.send_packet(self, self.ig_port, pkt)
                logger.info("Expecting packet on port {}".format(self.eg_ports[0]))
                testutils.verify_packet(self, exp_pkt, self.eg_ports[0])
            else:
                ts=time.time()
                testutils.send_packet(self, self.ig_port, pkt, num_pkts)

        if testutils.test_param_get("target") == "hw":
            te=time.time()
            tot_pkts = num_pkts * self.num_entries
            ti_per_pkt=(te-ts)*1000/tot_pkts
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            time.sleep(2)
            egport_stats_after = get_traffic_stats(self)
            loss_ti_ms = ti_per_pkt * (tot_pkts - (egport_stats_after - egport_stats_before))
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))

        if testutils.test_param_get("target") != "hw":
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
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaBridgedMd(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_bridged_md
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

    def get_traffic_stats_eg_port(self):
        logger.info("get stats on ports {}".format(self.eg_port))
        get_data_list = None
        resp = self.port_stat_table.entry_get(
            self.target,
            [self.port_stat_table.make_key([gc.KeyTuple('$DEV_PORT', self.eg_port)])],
            get_data_list)
        data_dict = next(resp)[0].to_dict()
        tx = data_dict['$FramesTransmittedOK']
        return tx

    def add_entries(self):
        logger.info("Adding entries")
        self.table_bridge_md_ctl.default_entry_set(self.target, self.bridged_data)
        self.table_output_port.default_entry_set(self.target, self.output_data)

    def del_entries(self):
        logger.info("Deleting entries")
        self.table_output_port.default_entry_reset(self.target)
        self.table_bridge_md_ctl.default_entry_reset(self.target)

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

        if testutils.test_param_get("target") != "hw":
            for i,p in enumerate(self.ig_ports):
                testutils.send_packet(self, p, ipkt)
                testutils.verify_packet(self, epkts[i], self.eg_port)

        if testutils.test_param_get("target") == "hw":
            num_pkts = testutils.test_param_get("num_pkts", 10000)
            egport_stats_before = self.get_traffic_stats_eg_port()
            ts=time.time()
            for i,p in enumerate(self.ig_ports):
                testutils.send_packet(self, p, ipkt, num_pkts)
            te=time.time()
            tot_pkts = num_pkts * len(self.ig_ports)
            ti_per_pkt=(te-ts)*1000/tot_pkts
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            time.sleep(1)
            egport_stats_after = self.get_traffic_stats_eg_port()
            loss_ti_ms = ti_per_pkt * (tot_pkts - (egport_stats_after - egport_stats_before))
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))

    def get_entries_and_verify(self):
        resp = self.table_bridge_md_ctl.default_entry_get(self.target)
        for data, key in resp:
            assert data == self.bridged_data
        resp = self.table_output_port.default_entry_get(self.target)
        for data, key in resp:
            assert data == self.output_data

    def setUp(self):
        self.p4_name = "tna_bridged_md"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitTnaBridgedMd, self).tearDown()


class WarminitTnaMirror(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_mirror
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

    def del_entries(self):
        logger.info("Deleting entries")
        self.mirror_fwd_table.entry_del(self.target, self.mirror_fwd_key)
        self.mirror_cfg_table.entry_del(self.target, self.mirror_cfg_key)

    def get_traffic_stats_eg_port(self):
        logger.info("get stats on ports {}".format(self.eg_port))
        get_data_list = None
        resp = self.port_stat_table.entry_get(
            self.target,
            [self.port_stat_table.make_key([gc.KeyTuple('$DEV_PORT', self.eg_port)])],
            get_data_list)
        data_dict = next(resp)[0].to_dict()
        tx = data_dict['$FramesTransmittedOK']
        return tx

    def send_traffic(self):
        logger.info("Sending traffic")
        pkt = simple_eth_packet(pktlen=79)
        pkt = simple_eth_packet(pktlen=200)
        rec_pkt1 = simple_eth_packet(pktlen=self.EXP_LEN1)
        rec_pkt2 = simple_eth_packet(pktlen=self.EXP_LEN2)
        if testutils.test_param_get("target") != "hw":
            for port in swports:
                send_packet(self, port, pkt)
                if port % 2 == 0:
                    verify_packet(self, rec_pkt1, port)
                else:
                    verify_packet(self, rec_pkt2, port)
            verify_no_other_packets(self)
        if testutils.test_param_get("target") == "hw":
            num_pkts = testutils.test_param_get("num_pkts", 100000)
            port = swports[0]
            self.eg_port = port
            egport_stats_before = self.get_traffic_stats_eg_port()
            ts=time.time()
            send_packet(self, port, pkt, num_pkts)
            if port % 2 == 0:
                verify_packet(self, rec_pkt1, port)
            else:
                verify_packet(self, rec_pkt2, port)
            te=time.time()
            ti_per_pkt=(te-ts)*1000/num_pkts
            logger.info("Tx: one packet takes {} ms".format(ti_per_pkt))
            time.sleep(1)
            egport_stats_after = self.get_traffic_stats_eg_port()
            loss_ti_ms = ti_per_pkt * (num_pkts - (egport_stats_after - egport_stats_before))
            if hitless_mode:
                if loss_ti_ms > 0:
                    raise RuntimeError("Traffic loss {} is more than 0 ms".format(loss_ti_ms))
            else:
                if loss_ti_ms > 50:
                    raise RuntimeError("Traffic loss {} is more than 50 ms".format(loss_ti_ms))

    def get_entries_and_verify(self):
        resp = self.mirror_fwd_table.entry_get(self.target)
        for data, key in resp:
            assert key in self.mirror_fwd_key, "Key {} isn't found in mirror_fwd_key list".format(key)
            index = list(self.mirror_fwd_key).index(key)
            assert data == self.mirror_fwd_data[index], "Received {}, expected {}".format(data, self.mirror_fwd_data[index])
        resp = self.mirror_cfg_table.entry_get(self.target)
        for data, key in resp:
            assert key in self.mirror_cfg_key, "Key {} isn't found in mirror_cfg_key list".format(key)

    def setUp(self):
        self.p4_name = "tna_mirror"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitTnaMirror, self).tearDown()

class WarminitTnaPVS(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_pvs
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

    def del_entries(self):
        logger.info("Deleting entries")
        self.vs_table.entry_del(self.target, self.key_list)

    def get_entries_and_verify(self):
        logger.info("Verifying get entry")
        resp = self.vs_table.entry_get(self.target)
        i=0
        for data, key in resp:
            "==============="
            assert key == self.key_list[i], "received {} expected {}".format(key, self.key_list[i])
            i+=1

    def setUp(self):
        self.p4_name = "tna_pvs"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaRegister(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_register
    '''

    def setup_tables(self):
        self.test_reg_table = self.bfrt_info.table_get("SwitchIngress.test_reg")

    def setup_test_data(self):
        logger.info("Setting up test data")
        self.register_idx = random.randint(0, 500)
        register_value_hi = random.randint(0, 1000)
        register_value_lo = random.randint(0, 1000)
        logger.info("Register value hi {}".format(register_value_hi))
        logger.info("Register value lo {}".format(register_value_lo))
        self.register_value_hi_arr = {}
        self.register_value_lo_arr = {}
        num_pipes = int(testutils.test_param_get('num_pipes'))

        for i in range(num_pipes):
            self.register_value_hi_arr[i] = register_value_hi
            self.register_value_lo_arr[i] = register_value_lo

        self.key_list = [self.test_reg_table.make_key([gc.KeyTuple('$REGISTER_INDEX', self.register_idx)])]
        self.data_list = [self.test_reg_table.make_data(
                [gc.DataTuple('SwitchIngress.test_reg.first', register_value_lo),
                 gc.DataTuple('SwitchIngress.test_reg.second', register_value_hi)])]

    def add_entries(self):
        logger.info("Adding entries")
        self.test_reg_table.entry_add(self.target, self.key_list, self.data_list)

    def verifyReadRegisters(self, register_name_lo, register_name_hi, resp, register_value_lo, register_value_hi,
                            data_dict):

        num_pipes = int(testutils.test_param_get('num_pipes'))
        logger.info(num_pipes)
        for i in range(num_pipes):
            value_lo = data_dict[register_name_lo][i]
            value_hi = data_dict[register_name_hi][i]
            logger.info("Register Lo Expected Value ({}) : Read value ({})".format(register_value_lo[i], value_lo))
            logger.info("Register Hi Expected Value ({}) : Read value ({})".format(register_value_hi[i], value_hi))
            if data_dict[register_name_lo][i] != register_value_lo[i]:
                logger.info("Register field lo didn't match with the read value")
                assert (0)
            if data_dict[register_name_hi][i] != register_value_hi[i]:
                logger.info("Register field hi didn't match with the read value")
                assert (0)

    def get_entries_and_verify(self):
        logger.info("Verifying get entry")
        resp = self.test_reg_table.entry_get(
            self. target,
            [self.test_reg_table.make_key([gc.KeyTuple('$REGISTER_INDEX', self.register_idx)])],
            {"from_hw": True})

        data_dict = next(resp)[0].to_dict()
        self.verifyReadRegisters("SwitchIngress.test_reg.first", "SwitchIngress.test_reg.second", resp,
                            self.register_value_lo_arr, self.register_value_hi_arr, data_dict)

    def setUp(self):
        self.p4_name = "tna_register"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaDynHashing(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_dyn_hashing
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

    def del_entries(self):
        logger.info("Deleting entries")
        self.hash_config_table.default_entry_del(self.target)

    def get_entries_and_verify(self):
        logger.info("Getting entry")
        resp = self.hash_config_table.default_entry_get(self.target)
        i=0
        for data, key in resp:
            assert data == self.data_list[i], "received {} expected {}".format(data, self.data_list[i])
            i+=1

    def setUp(self):
        self.p4_name = "tna_dyn_hashing"
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        super(WarminitBaseTest, self).tearDown()

class WarminitTnaMeterLpfWred(WarminitBaseTest):
    '''
    Warm-init hitless or fast reconfig for tna_meter_lpf_wred
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

    def del_entries(self):
        logger.info("Deleting entries")
        self.match_table.entry_del(self.target, self.match_key_list)

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
        WarminitBaseTest.setUp(self)

    def tearDown(self):
        self.del_entries()
        WarminitBaseTest.tearDown(self)
