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
Simple PTF test for default_entry.p4
"""

import pd_base_tests
import pdb
import six.moves

from ptf import config
from ptf.testutils import *
from p4testutils.misc_utils import *
from ptf.thriftutils import *

from default_entry.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *

dev_id = 0
num_pipes = int(test_param_get('num_pipes'))

swports = get_sw_ports()

def port_to_pipe(port):
    local_port = port & 0x7F
    assert(local_port < 72)
    pipe = (port >> 7) & 0x3
    assert(port == ((pipe << 7) | local_port))
    return pipe

class TestKeyless(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["default_entry"])

    """
    Basic test that checks if keyless table performs correctly when no match memory
    is allocated, but action data memory is needed.
    """
    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(dev_id, hex_to_i16(0xFFFF))

        print("PIPE_MGR gave me that session handle:", sess_hdl)

        # Setup table entries
        DST_ADDR_IN = '128.0.0.7'
        DST_ADDR_OUT_DEF = '0.0.0.127'
        DST_ADDR_OUT = '192.168.55.2'

        INPUT_PORT = swports[4]
        OUTPUT_PORT = 0

        for read_from_hw in [True, False]:
            def_ent = self.client.keyless_direct_table_get_default_entry(sess_hdl, dev_tgt, read_from_hw)
            assert def_ent.action_desc.name == 'set_ipv4_dst'
            assert def_ent.action_desc.data.default_entry_set_ipv4_dst.action_x == 127

        if 0 not in swports:
            action_spec = default_entry_prepare_keyless_action_spec_t(swports[0])
            self.client.set_egr_exm_set_default_action_prepare_keyless(sess_hdl, dev_tgt, action_spec)
            self.conn_mgr.complete_operations(sess_hdl)
            OUTPUT_PORT = swports[0]

        print("Finished adding entries")
        pkt_1 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9002,
                                  with_tcp_chksum=False)

        exp_pkt_1 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT_DEF,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:
            send_packet(self, INPUT_PORT, pkt_1)
            verify_packet(self, exp_pkt_1, OUTPUT_PORT)
            print("First packet verified!")
        except:
            print("First packet failed!")
            print("Removing entries")
            if 0 not in swports:
                self.client.set_egr_exm_table_reset_default_entry(sess_hdl, dev_tgt)
                self.conn_mgr.complete_operations(sess_hdl)
            self.conn_mgr.client_cleanup(sess_hdl)
            raise

        keyless_direct_action_spec = default_entry_set_ipv4_dst_action_spec_t(ipv4Addr_to_i32(DST_ADDR_OUT))
        def_hdl = self.client.keyless_direct_set_default_action_set_ipv4_dst(sess_hdl, dev_tgt, keyless_direct_action_spec)

        self.conn_mgr.complete_operations(sess_hdl)
        print("Finished adding entries")

        for read_from_hw in [True, False]:
            e1 = self.client.keyless_direct_get_entry(sess_hdl, dev_id, def_hdl, read_from_hw)
            e2 = self.client.keyless_direct_table_get_default_entry(sess_hdl, dev_tgt, read_from_hw)
            assert e1 == e2


        pkt_2 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9002,
                                  with_tcp_chksum=False)

        exp_pkt_2 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:
            send_packet(self, INPUT_PORT, pkt_2)
            verify_packet(self, exp_pkt_2, OUTPUT_PORT)
            print("Second packet verified!")
        except:
            print("Second packet failed!")
            print("Removing entries")
            if 0 not in swports:
                self.client.set_egr_exm_table_reset_default_entry(sess_hdl, dev_tgt)
            self.client.keyless_direct_table_reset_default_entry(sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            self.conn_mgr.client_cleanup(sess_hdl)
            raise

        self.client.keyless_direct_table_reset_default_entry(sess_hdl, dev_tgt)

        self.conn_mgr.complete_operations(sess_hdl)
        print("Finished adding entries")

        pkt_3 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9002,
                                  with_tcp_chksum=False)

        exp_pkt_3 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT_DEF,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:

            # First, send packet that writes copy2cpu
            send_packet(self, INPUT_PORT, pkt_3)
            try:
                verify_packets(self, exp_pkt_3, [OUTPUT_PORT])
                print("Third packet verified!")
            except:
                print("Third packet failed!")
                raise


        finally:
            print("Removing entries")
            if 0 not in swports:
                self.client.set_egr_exm_table_reset_default_entry(sess_hdl, dev_tgt)
                self.conn_mgr.complete_operations(sess_hdl)
            status = self.conn_mgr.client_cleanup(sess_hdl)

class TestKeylessIndirectResources(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["default_entry"])

    def runTest(self):
        # Comment out this test until keyless indirect resource tables are resolved
        return
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))
        print("PIPE_MGR gave me that session handle:", sess_hdl)

        # Setup table entries
        DST_ADDR_IN = '128.0.0.7'
        DST_ADDR_OUT_DEF = '0.0.0.127'

        INPUT_PORT = swports[4]
        OUTPUT_PORT = 1

        if 1 not in swports:
            action_spec = default_entry_prepare_keyless_action_spec_t(swports[1])
            self.client.set_egr_tcam_set_default_action_prepare_keyless(sess_hdl, dev_tgt, action_spec)
            self.conn_mgr.complete_operations(sess_hdl)
            OUTPUT_PORT = swports[1]
        OUTPUT_PIPE = port_to_pipe(OUTPUT_PORT)

        self.conn_mgr.complete_operations(sess_hdl)
        cntr_flags = default_entry_counter_flags_t(0)
        reg_flags = default_entry_register_flags_t(0)
        try:
            print("Default stat and stful indices of 1 and 2")
            print('Reading keyless cntr and reg before sending pkt')
            self.client.counter_hw_sync_keyless_cntr(sess_hdl, dev_tgt, True)
            self.client.register_hw_sync_keyless_reg(sess_hdl, dev_tgt)
            cntr_1_before = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 1, cntr_flags)
            reg_2_before = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 2, reg_flags)
            print("Keyless Counter index 1 value: ", cntr_1_before)
            print("Keyless Register index 2 value: ", reg_2_before)

            pkt = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                    eth_src='00:22:22:22:22:22',
                                    ip_src='1.2.3.4',
                                    ip_dst=DST_ADDR_IN,
                                    ip_id=101,
                                    ip_ttl=100,
                                    tcp_sport = 9003,
                                    with_tcp_chksum=False)

            exp_pkt_def = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                        eth_src='00:22:22:22:22:22',
                                        ip_src='1.2.3.4',
                                        ip_dst=DST_ADDR_OUT_DEF,
                                        ip_id=101,
                                        ip_ttl=100,
                                        tcp_sport = 9006,
                                        with_tcp_chksum=False)
            send_packet(self, INPUT_PORT, pkt)
            verify_packets(self, exp_pkt_def, [OUTPUT_PORT])

            print('Reading keyless cntr after sending pkt')
            self.client.counter_hw_sync_keyless_cntr(sess_hdl, dev_tgt, False)
            self.client.register_hw_sync_keyless_reg(sess_hdl, dev_tgt)
            cntr_1_after = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 1, cntr_flags)
            reg_2_after = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 2, reg_flags)
            print("Keyless Counter index 1 value: ", cntr_1_after)
            print("Keyless Register index 2 value: ", reg_2_after)
            self.assertEqual(cntr_1_after.packets - cntr_1_before.packets, 1)
            self.assertEqual(reg_2_after[OUTPUT_PIPE] - reg_2_before[OUTPUT_PIPE], 5)

            cntr_1_before = cntr_1_after
            reg_2_before = reg_2_after

            print("Changing stat and stful indices to 3 and 4")
            action_spec = default_entry_keyless_counts_action_spec_t(3, 4)
            self.client.keyless_indirect_resources_set_default_action_keyless_counts(sess_hdl, dev_tgt, action_spec)
            self.conn_mgr.complete_operations(sess_hdl)

            print('Reading keyless cntr and reg before sending pkt')
            self.client.counter_hw_sync_keyless_cntr(sess_hdl, dev_tgt, True)
            self.client.register_hw_sync_keyless_reg(sess_hdl, dev_tgt)
            cntr_3_before = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 3, cntr_flags)
            reg_4_before = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 4, reg_flags)
            print("Keyless Counter index 3 value: ", cntr_3_before)
            print("Keyless Register index 4 value: ", reg_4_before)

            pkt = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                    eth_src='00:22:22:22:22:22',
                                    ip_src='1.2.3.4',
                                    ip_dst=DST_ADDR_IN,
                                    ip_id=101,
                                    ip_ttl=100,
                                    tcp_sport = 9003,
                                    with_tcp_chksum=False)

            exp_pkt_def = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                        eth_src='00:22:22:22:22:22',
                                        ip_src='1.2.3.4',
                                        ip_dst=DST_ADDR_OUT_DEF,
                                        ip_id=101,
                                        ip_ttl=100,
                                        tcp_sport = 9006,
                                        with_tcp_chksum=False)
            send_packet(self, INPUT_PORT, pkt)
            verify_packets(self, exp_pkt_def, [OUTPUT_PORT])

            print('Reading keyless cntr after sending pkt')
            self.client.counter_hw_sync_keyless_cntr(sess_hdl, dev_tgt, False)
            self.client.register_hw_sync_keyless_reg(sess_hdl, dev_tgt)
            cntr_3_after = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 3, cntr_flags)
            reg_4_after = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 4, reg_flags)
            print("Keyless Counter index 3 value: ", cntr_3_after)
            print("Keyless Register index 4 value: ", reg_4_after)
            self.assertEqual(cntr_3_after.packets - cntr_3_before.packets, 1)
            self.assertEqual(reg_4_after[OUTPUT_PIPE] - reg_4_before[OUTPUT_PIPE], 5)
            cntr_1_after = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 1, cntr_flags)
            reg_2_after = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 2, reg_flags)
            self.assertEqual(cntr_1_after.packets, cntr_1_before.packets)
            self.assertEqual(reg_2_after[OUTPUT_PIPE], reg_2_before[OUTPUT_PIPE])

            cntr_3_before = cntr_3_after
            reg_4_before = reg_4_after

            print("Resetting stat and stful indices to 1 and 2")
            self.client.keyless_indirect_resources_table_reset_default_entry(sess_hdl, dev_tgt)

            pkt = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                    eth_src='00:22:22:22:22:22',
                                    ip_src='1.2.3.4',
                                    ip_dst=DST_ADDR_IN,
                                    ip_id=101,
                                    ip_ttl=100,
                                    tcp_sport = 9003,
                                    with_tcp_chksum=False)

            exp_pkt_def = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                        eth_src='00:22:22:22:22:22',
                                        ip_src='1.2.3.4',
                                        ip_dst=DST_ADDR_OUT_DEF,
                                        ip_id=101,
                                        ip_ttl=100,
                                        tcp_sport = 9006,
                                        with_tcp_chksum=False)
            send_packet(self, INPUT_PORT, pkt)
            verify_packets(self, exp_pkt_def, [OUTPUT_PORT])

            print('Reading keyless cntr after sending pkt')
            self.client.counter_hw_sync_keyless_cntr(sess_hdl, dev_tgt, False)
            self.client.register_hw_sync_keyless_reg(sess_hdl, dev_tgt)
            cntr_1_after = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 1, cntr_flags)
            reg_2_after = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 2, reg_flags)
            print("Keyless Counter index 1 value: ", cntr_1_after)
            print("Keyless Register index 2 value: ", reg_2_after)
            self.assertEqual(cntr_1_after.packets - cntr_1_before.packets, 1)
            self.assertEqual(reg_2_after[OUTPUT_PIPE] - reg_2_before[OUTPUT_PIPE], 5)
            cntr_3_after = self.client.counter_read_keyless_cntr(sess_hdl, dev_tgt, 3, cntr_flags)
            reg_4_after = self.client.register_read_keyless_reg(sess_hdl, dev_tgt, 4, reg_flags)
            self.assertEqual(cntr_3_after.packets, cntr_3_before.packets)
            self.assertEqual(reg_4_after[OUTPUT_PIPE], reg_4_before[OUTPUT_PIPE])

        finally:
            print("Removing entries")
            if 1 not in swports:
                status = self.client.set_egr_tcam_table_reset_default_entry(sess_hdl, dev_tgt)
                self.conn_mgr.complete_operations(sess_hdl)
            status = self.conn_mgr.client_cleanup(sess_hdl)

class TestAlpmDefault(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["default_entry"])

    def runTest(self):
        sess_hdl = self.conn_mgr.client_init()
        dev_tgt = DevTarget_t(0, hex_to_i16(0xFFFF))

        print("PIPE_MGR gave me that session handle:", sess_hdl)

        # Setup table entries
        DST_ADDR_IN = '128.0.0.7'
        DST_ADDR_OUT_DEF = '0.0.0.127'

        INPUT_PORT = swports[4]
        OUTPUT_PORT = 2

        if 2 not in swports:
            action_spec = default_entry_prepare_keyless_action_spec_t(swports[2])
            self.client.set_egr_alpm_set_default_action_prepare_keyless(sess_hdl, dev_tgt, action_spec)
            self.conn_mgr.complete_operations(sess_hdl)
            OUTPUT_PORT = swports[2]

        print("Finished adding entries")
        pkt_1 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9004,
                                  with_tcp_chksum=False)

        exp_pkt_1 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT_DEF,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:
            send_packet(self, INPUT_PORT, pkt_1)
            verify_packet(self, exp_pkt_1, OUTPUT_PORT)
            print("First packet verified!")
        except:
            print("First packet failed!")
            print("Removing entries")
            if 2 not in swports:
                self.client.set_egr_alpm_table_reset_default_entry(sess_hdl, dev_tgt)
                self.conn_mgr.complete_operations(sess_hdl)
            self.conn_mgr.client_cleanup(sess_hdl)
            raise

        action_spec = default_entry_prepare_keyless_action_spec_t(swports[3])
        def_hdl = self.client.set_egr_alpm_set_default_action_prepare_keyless(sess_hdl, dev_tgt, action_spec)
        self.conn_mgr.complete_operations(sess_hdl)

        for read_from_hw in [True, False]:
            e1 = self.client.set_egr_alpm_get_entry(sess_hdl, dev_id, def_hdl, read_from_hw)
            e2 = self.client.set_egr_alpm_table_get_default_entry(sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_desc == e2.action_desc

        self.conn_mgr.complete_operations(sess_hdl)
        print("Finished adding entries")

        pkt_2 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9004,
                                  with_tcp_chksum=False)

        exp_pkt_2 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT_DEF,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:
            send_packet(self, INPUT_PORT, pkt_2)
            verify_packet(self, exp_pkt_2, swports[3])
            print("Second packet verified!")
        except:
            print("Second packet failed!")
            print("Removing entries")
            self.client.set_egr_alpm_table_reset_default_entry(sess_hdl, dev_tgt)
            self.conn_mgr.complete_operations(sess_hdl)
            self.conn_mgr.client_cleanup(sess_hdl)
            raise

        self.client.set_egr_alpm_table_reset_default_entry(sess_hdl, dev_tgt)
        if 2 not in swports:
            action_spec = default_entry_prepare_keyless_action_spec_t(swports[2])
            self.client.set_egr_alpm_set_default_action_prepare_keyless(sess_hdl, dev_tgt, action_spec)

        self.conn_mgr.complete_operations(sess_hdl)
        print("Finished adding entries")

        pkt_3 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                  eth_src='00:22:22:22:22:22',
                                  ip_src='15.7.127.255',
                                  ip_dst=DST_ADDR_IN,
                                  ip_id=101,
                                  ip_ttl=64,
                                  tcp_sport = 9004,
                                  with_tcp_chksum=False)

        exp_pkt_3 = simple_tcp_packet(eth_dst='00:24:68:AC:DF:56',
                                      eth_src='00:22:22:22:22:22',
                                      ip_src='15.7.127.255',
                                      ip_dst=DST_ADDR_OUT_DEF,
                                      ip_id=101,
                                      ip_ttl=64,
                                      tcp_sport = 9006,
                                      with_tcp_chksum=False)

        try:

            # First, send packet that writes copy2cpu
            send_packet(self, INPUT_PORT, pkt_3)
            try:
                verify_packets(self, exp_pkt_3, [OUTPUT_PORT])
                print("Third packet verified!")
            except:
                print("Third packet failed!")
                raise


        finally:
            print("Removing entries")
            if 2 not in swports:
                self.client.set_egr_alpm_table_reset_default_entry(sess_hdl, dev_tgt)
                self.conn_mgr.complete_operations(sess_hdl)
            status = self.conn_mgr.client_cleanup(sess_hdl)

class ExmMatchDirectDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        print("Verfiying default entry HW read for symmetric table")
        self.client.exm_dir_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_ALL_PIPELINES, 0)
        action_spec = default_entry_act_1_action_spec_t(1947, 5, 88);
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        def_eh = self.client.exm_dir_set_default_action_act_1(self.sess_hdl, dev_tgt, action_spec);
        self.conn_mgr.complete_operations(self.sess_hdl)
        api_def_eh = self.client.exm_dir_table_get_default_entry_handle(self.sess_hdl, dev_tgt);
        #print(def_eh, api_def_eh)

        def_entry_sw = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 1);
        #print(def_entry_hw)

        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, read_from_hw)
            e2 = self.client.exm_dir_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_desc == e2.action_desc

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client.exm_dir_table_reset_default_entry(self.sess_hdl, dev_tgt)

        print("Verfiying default entry HW read for asymmetric table")
        dev_tgt_arr = [DevTarget_t(0, hex_to_i16(x)) for x in range(num_pipes)]

        self.client.exm_dir_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_SINGLE_PIPELINE, 0)
        for k in range(0, num_pipes):
            if k%2 == 0:
                action_spec = default_entry_act_1_action_spec_t(1947, 5, 88);
                def_eh = self.client.exm_dir_set_default_action_act_1(self.sess_hdl, dev_tgt_arr[k], action_spec);
            else:
                action_spec = default_entry_set_egr_action_spec_t(453);
                def_eh = self.client.exm_dir_set_default_action_set_egr(self.sess_hdl, dev_tgt_arr[k], action_spec);

            self.conn_mgr.complete_operations(self.sess_hdl)
            api_def_eh = self.client.exm_dir_table_get_default_entry_handle(self.sess_hdl, dev_tgt_arr[k]);
            #print(def_eh, api_def_eh)

            def_entry_sw = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 0);
            #print(def_entry_sw)
            def_entry_hw = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 1);
            #print(def_entry_hw)

            assert(def_entry_sw == def_entry_hw);

            for read_from_hw in [True, False]:
                e1 = self.client.exm_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, read_from_hw)
                e2 = self.client.exm_dir_table_get_default_entry(self.sess_hdl, dev_tgt_arr[k], read_from_hw)
                assert e1.action_desc == e2.action_desc

            print(("Default entry HW read for asymmetric table for pipe ", dev_tgt_arr[k].dev_pipe_id," successful"))
            print("Clean up the default entry")
            self.client.exm_dir_table_reset_default_entry(self.sess_hdl, dev_tgt_arr[k])

    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class ExmMatchIndirectDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        print("Verfiying default entry HW read for symmetric table")
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        macaddrs = "00:33:44:66:88:22"
        action_spec = default_entry_custom_action_3_action_spec_t( 405, macAddr_to_string(macaddrs), ipv4Addr_to_i32("10.9.8.7"))
        mbr = self.client.custom_action_3_profile_add_member_with_custom_action_3(
                        self.sess_hdl,
                        dev_tgt,
                        action_spec)
        def_eh = self.client.exm_indr_set_default_entry(self.sess_hdl, dev_tgt, mbr);
        #print(def_eh)

        self.conn_mgr.complete_operations(self.sess_hdl)
        def_entry_sw = self.client.exm_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client.exm_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, 1);
        #print(def_entry_hw)

        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client.exm_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, read_from_hw)
            e2 = self.client.exm_indr_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_mbr_hdl == e2.action_mbr_hdl
            assert e1.action_mbr_hdl == mbr

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client.exm_indr_table_reset_default_entry(self.sess_hdl, dev_tgt)

    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class TernaryMatchIndirectDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        print("Verfiying default entry HW read for symmetric table")
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        action_spec = default_entry_custom_action_2_action_spec_t(34)
        mbr = self.client.custom_action_2_profile_add_member_with_custom_action_2(
                        self.sess_hdl,
                        dev_tgt,
                        action_spec)
        def_eh = self.client.tcam_indr_set_default_entry(self.sess_hdl, dev_tgt, mbr);
        self.conn_mgr.complete_operations(self.sess_hdl)
        #print(def_eh)

        def_entry_sw = self.client.tcam_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client.tcam_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, 1);
        #print(def_entry_hw)

        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client.tcam_indr_get_entry(self.sess_hdl, dev_tgt.dev_id, def_eh, read_from_hw)
            e2 = self.client.tcam_indr_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_mbr_hdl == e2.action_mbr_hdl
            assert e1.action_mbr_hdl == mbr

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client.tcam_indr_table_reset_default_entry(self.sess_hdl, dev_tgt)

    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class TernaryMatchDirectDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        print("Verfiying default entry HW read for symmetric table")
        self.client.tcam_dir_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_ALL_PIPELINES, 0)
        action_spec = default_entry_act_1_action_spec_t(1947, 5, 88);
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        def_eh = self.client.tcam_dir_set_default_action_act_1(self.sess_hdl, dev_tgt, action_spec);
        self.conn_mgr.complete_operations(self.sess_hdl)
        api_def_eh = self.client.tcam_dir_table_get_default_entry_handle(self.sess_hdl, dev_tgt);
        #print(def_eh, api_def_eh)

        def_entry_sw = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 1);
        #print(def_entry_hw)

        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, read_from_hw)
            e2 = self.client.tcam_dir_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_desc == e2.action_desc

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client.tcam_dir_table_reset_default_entry(self.sess_hdl, dev_tgt)

        print("Verfiying default entry HW read for asymmetric table")
        dev_tgt_arr = [DevTarget_t(0, hex_to_i16(x)) for x in range(num_pipes)]

        self.client.tcam_dir_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_SINGLE_PIPELINE, 0)
        for k in range(0, num_pipes):
            if k%2 != 0:
                action_spec = default_entry_act_1_action_spec_t(1947, 5, 88);
                def_eh = self.client.tcam_dir_set_default_action_act_1(self.sess_hdl, dev_tgt_arr[k], action_spec);
            else:
                action_spec = default_entry_set_egr_action_spec_t(453);
                def_eh = self.client.tcam_dir_set_default_action_set_egr(self.sess_hdl, dev_tgt_arr[k], action_spec);

            self.conn_mgr.complete_operations(self.sess_hdl)
            api_def_eh = self.client.tcam_dir_table_get_default_entry_handle(self.sess_hdl, dev_tgt_arr[k]);
            #print(def_eh, api_def_eh)

            def_entry_sw = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 0);
            #print(def_entry_sw)
            def_entry_hw = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 1);
            #print(def_entry_hw)

            assert(def_entry_sw == def_entry_hw);

            for read_from_hw in [True, False]:
                e1 = self.client.tcam_dir_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, read_from_hw)
                e2 = self.client.tcam_dir_table_get_default_entry(self.sess_hdl, dev_tgt_arr[k], read_from_hw)
                assert e1.action_desc == e2.action_desc

            print(("Default entry HW read for asymmetric table for pipe ", dev_tgt_arr[k].dev_pipe_id," successful"))
            print("Clean up the default entry")
            self.client.tcam_dir_table_reset_default_entry(self.sess_hdl, dev_tgt_arr[k])


    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class KeylessMatchDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        print("Verfiying default entry HW read for symmetric table")
        self.client.keyless_table_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_ALL_PIPELINES, 0)
        action_spec = default_entry_keyless_action_action_spec_t(1947, 5, 88, 135);
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        def_eh = self.client.keyless_table_set_default_action_keyless_action(self.sess_hdl, dev_tgt, action_spec);
        self.conn_mgr.complete_operations(self.sess_hdl)
        api_def_eh = self.client.keyless_table_table_get_default_entry_handle(self.sess_hdl, dev_tgt);
        #print(def_eh, api_def_eh)

        def_entry_sw = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 1);
        #print(def_entry_hw)

        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, read_from_hw)
            e2 = self.client.keyless_table_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1 == e2

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client.keyless_table_table_reset_default_entry(self.sess_hdl, dev_tgt)

        print("Verfiying default entry HW read for asymmetric table")
        dev_tgt_arr = [DevTarget_t(0, hex_to_i16(x)) for x in range(num_pipes)]

        self.client.keyless_table_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_SINGLE_PIPELINE, 0)
        for k in range(0, num_pipes):
            action_spec = default_entry_keyless_action_action_spec_t(1947, 5, 88, 135);
            def_eh = self.client.keyless_table_set_default_action_keyless_action(self.sess_hdl, dev_tgt_arr[k], action_spec);

            self.conn_mgr.complete_operations(self.sess_hdl)
            api_def_eh = self.client.keyless_table_table_get_default_entry_handle(self.sess_hdl, dev_tgt_arr[k]);
            #print(def_eh, api_def_eh)

            def_entry_sw = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 0);
            #print(def_entry_sw)
            def_entry_hw = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, 1);
            #print(def_entry_hw)

            assert(def_entry_sw == def_entry_hw);

            for read_from_hw in [True, False]:
                e1 = self.client.keyless_table_get_entry(self.sess_hdl, dev_tgt_arr[k].dev_id, api_def_eh, read_from_hw)
                e2 = self.client.keyless_table_table_get_default_entry(self.sess_hdl, dev_tgt_arr[k], read_from_hw)
                assert e1 == e2

            print(("Default entry HW read for asymmetric table for pipe ", dev_tgt_arr[k].dev_pipe_id," successful"))
            print("Clean up the default entry")
            self.client.keyless_table_table_reset_default_entry(self.sess_hdl, dev_tgt_arr[k])

    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class ResourcesDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        '''print("====================== SEL ========================")
        grp_hdls = []
        mbr_hdls = []
        for _ in six.moves.range(4):
            grp_hdls.append(self.client.ecmp_action_profile_create_group(self.sess_hdl, self.dev_tgt, 4))
        ports = {1:0, 2: 0, 3:0, 4: 0}
        action_specs = [default_entry_nhop_set_action_spec_t((x)) for x in ports]
        mbr_hdls = [self.client.ecmp_action_profile_add_member_with_nhop_set(self.sess_hdl, self.dev_tgt, x) for x in action_specs]

        for mbr_hdl in mbr_hdls:
            self.client.ecmp_action_profile_add_member_to_group(self.sess_hdl, self.dev_tgt.dev_id, grp_hdls[0], mbr_hdl);
        self.client.ipv4_routing_select_set_default_entry_with_selector(self.sess_hdl, self.dev_tgt, grp_hdls[0])

        pdb.set_trace()
        api_def_eh = self.client.ipv4_routing_select_table_get_default_entry_handle(self.sess_hdl, self.dev_tgt);
        print(api_def_eh)

        def_entry_sw = self.client.ipv4_routing_select_get_entry(self.sess_hdl, self.dev_tgt.dev_id, api_def_eh, 0);
        print(def_entry_sw)
        def_entry_hw = self.client.ipv4_routing_select_get_entry(self.sess_hdl, self.dev_tgt.dev_id, api_def_eh, 1);
        print(def_entry_hw)
        '''

        print("======================== STATS ========================")
        action_spec = default_entry__CounterAAction1_action_spec_t(3);
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        def_eh = self.client._CounterATable_set_default_action__CounterAAction1(self.sess_hdl, dev_tgt, action_spec);
        self.conn_mgr.complete_operations(self.sess_hdl)
        api_def_eh = self.client._CounterATable_table_get_default_entry_handle(self.sess_hdl, dev_tgt);
        #print(def_eh, api_def_eh)

        def_entry_sw = self.client._CounterATable_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 0);
        #print(def_entry_sw)
        def_entry_hw = self.client._CounterATable_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 1);
        #print(def_entry_hw)
        assert(def_entry_sw == def_entry_hw);

        for read_from_hw in [True, False]:
            e1 = self.client._CounterATable_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, read_from_hw)
            e2 = self.client._CounterATable_table_get_default_entry(self.sess_hdl, dev_tgt, read_from_hw)
            assert e1.action_desc == e2.action_desc

        print("Default entry HW read for symmetric table successful")
        print("Clean up the default entry")
        self.client._CounterATable_table_reset_default_entry(self.sess_hdl, dev_tgt)

        '''print("======================== METERS ========================")
        self.client.meter_tbl_direct_set_property(self.sess_hdl, self.dev_tgt.dev_id, tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE, tbl_property_value_t.ENTRY_SCOPE_ALL_PIPELINES, 0)
        action_spec = default_entry_next_hop_ipv4_action_spec_t(449, 124, 248);
        meter_spec = default_entry_bytes_meter_spec_t(0, 0, 0, 0, False, False);
        dev_tgt = DevTarget_t(self.dev, hex_to_i16(0xFFFF))
        def_eh = self.client.meter_tbl_direct_set_default_action_nop(self.sess_hdl, dev_tgt, meter_spec);
        api_def_eh = self.client.meter_tbl_direct_table_get_default_entry_handle(self.sess_hdl, dev_tgt);
        print(def_eh, api_def_eh)

        def_entry_sw = self.client.meter_tbl_direct_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 0);
        print(def_entry_sw)
        def_entry_hw = self.client.meter_tbl_direct_get_entry(self.sess_hdl, dev_tgt.dev_id, api_def_eh, 1);
        print(def_entry_hw)
        assert(def_entry_sw == def_entry_hw)'''

    def tearDown(self):
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)

class PureKeylessMatchDefEntry(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["default_entry"])

    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print(("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl)))

    def runTest(self):

        def_h1 = self.client.pure_keyless_set_default_action_keyless_set_egr(self.sess_hdl, self.dev_tgt)
        print(("Set Default Action gave me this entry handle %d" %(def_h1)))
        def_h2 = self.client.pure_keyless_table_get_default_entry_handle(self.sess_hdl, self.dev_tgt)
        print(("Get Default Action gave me this entry handle %d" %(def_h2)))
        assert (def_h1 == def_h2)
        print("Resetting the Default Action")
        self.client.pure_keyless_table_reset_default_entry(self.sess_hdl, self.dev_tgt)
        # The entry still has a handle after being reset, we should be able to get it.
        def_h1 = self.client.pure_keyless_table_get_default_entry_handle(self.sess_hdl, self.dev_tgt)

        self.conn_mgr.complete_operations(self.sess_hdl)

        # We can also get the default entry itself using the handle.
        def_entry_sw_1 = self.client.pure_keyless_get_entry(self.sess_hdl, self.dev_tgt.dev_id, def_h1, 0)

        print("Setting the Default Entry Again")
        def_h1 = self.client.pure_keyless_set_default_action_keyless_set_egr(self.sess_hdl, self.dev_tgt)
        print(("Set Default Action gave me this entry handle %d" %(def_h1)))
        def_h2 = self.client.pure_keyless_table_get_default_entry_handle(self.sess_hdl, self.dev_tgt)
        print(("Get Default Action gave me this entry handle %d" %(def_h2)))
        assert (def_h1 == def_h2)
        self.conn_mgr.complete_operations(self.sess_hdl)
        def_entry_sw_1 = self.client.pure_keyless_get_entry(self.sess_hdl, self.dev_tgt.dev_id, def_h1, 0)
        def_entry_hw_1 = self.client.pure_keyless_get_entry(self.sess_hdl, self.dev_tgt.dev_id, def_h1, 1)
        print(def_entry_sw_1)
        print("\n")
        print(def_entry_hw_1)
        print("\n")
        assert(def_entry_sw_1 == def_entry_hw_1)

        for read_from_hw in [True, False]:
            e1 = self.client.pure_keyless_get_entry(self.sess_hdl, self.dev_tgt.dev_id, def_h1, read_from_hw)
            e2 = self.client.pure_keyless_table_get_default_entry(self.sess_hdl, self.dev_tgt, read_from_hw)
            assert e1 == e2

        print("Trying to get the default entry with an invalid entry handle")
        try:
            def_invalid = self.client.pure_keyless_get_entry(self.sess_hdl, self.dev_tgt.dev_id, 7, 0)
        except:
            print("Success: Not able to get entry with an invalid entry handle")
        else:
            print("Error: Able to get entry with an invalid entry handle")
            assert(0)

    def tearDown(self):
        print("Cleaning up the default entry")
        self.client.pure_keyless_table_reset_default_entry(self.sess_hdl, self.dev_tgt)
        self.conn_mgr.complete_operations(self.sess_hdl)
        self.conn_mgr.client_cleanup(self.sess_hdl)
        print(("Closed Session %d" % self.sess_hdl))
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
