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
from ptf.thriftutils import *
from ptf.testutils import *
import p4testutils.misc_utils as misc_utils
from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.client as gc

logger = misc_utils.get_logger()
swports = misc_utils.get_sw_ports()

p4_name = "tna_dkm"
client_id = 0
dev_id = 0

class DKMTest(BfRuntimeTest):
    """ A simple example of DKM table usage.
        Adds entries to the table and checks that forwarding works.
        Adjusts the table's mask and again checks that forwarding works.
    """
    def setUp(self):
        BfRuntimeTest.setUp(self, client_id)
        self.bfrt_info = self.interface.bfrt_info_get(p4_name)
        self.t = self.bfrt_info.table_get('Ing.t1')
        self.t.info.key_field_annotation_add('hdr.ipv4.dst_addr', 'ipv4')
        self.t.info.key_field_annotation_add('hdr.ipv4.src_addr', 'ipv4')

    def tearDown(self):
        trgt = gc.Target(device_id=dev_id)
        self.t.entry_del(trgt, [])
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', 0x1FF),
                             gc.KeyTuple('hdr.ipv4.dst_addr', '255.255.255.255'),
                             gc.KeyTuple('hdr.ipv4.src_addr', '255.255.255.255'),
                             gc.KeyTuple('hdr.ipv4.protocol', 0xFF)])
        self.t.attribute_dyn_key_mask_set(trgt, k)
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        trgt = gc.Target(device_id=dev_id)

        # Get the table's current DKM mask, it will have all fields included.
        resp = self.t.attribute_get(trgt, "DynamicKeyMask")
        for d in resp:
            dd = d["fields"].to_dict()
            self.assertEqual(dd["ig_intr_md.ingress_port"]["value"], 0x1FF)
            self.assertEqual(dd["hdr.ipv4.dst_addr"]["value"], '255.255.255.255')
            self.assertEqual(dd["hdr.ipv4.src_addr"]["value"], '255.255.255.255')
            self.assertEqual(dd["hdr.ipv4.protocol"]["value"], 0xFF)

        # Set a default entry for packets which miss and a normal entry as well
        # to try forwarding with no Dynamic Key Mask (DKM) configuration.
        ing_port = swports[-1]
        dflt_port = swports[0]
        hit_port = swports[1]
        dip = "10.1.1.1"
        sip = "10.1.1.2"
        non_matching_dip = "10.1.1.3"
        proto = 17 # UDP
        sip_masked = "10.1.1.0"
        sip_masked_val1 = "10.1.1.1"
        sip_masked_val2 = "10.1.1.4"
        sip_masked_val3 = "10.1.1.7"
        sip_masked_miss = "10.1.1.8"
        d = self.t.make_data([gc.DataTuple('port', dflt_port)], 'Ing.fwd')
        self.t.default_entry_set(trgt, d) 
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', ing_port),
                             gc.KeyTuple('hdr.ipv4.dst_addr', dip),
                             gc.KeyTuple('hdr.ipv4.src_addr', sip),
                             gc.KeyTuple('hdr.ipv4.protocol', proto)])
        d = self.t.make_data([gc.DataTuple('port', hit_port)], 'Ing.fwd')
        self.t.entry_add(trgt, [k], [d])

        # A TCP packet will miss the forwarding rule since the protocol is part
        # of the key.
        p = simple_tcp_packet(ip_dst=dip, ip_src=sip)
        send_packet(self, ing_port, p)
        verify_packet(self, p, dflt_port)
        # A UDP packet from another ingress port will also miss the forwarding
        # rule.
        p = simple_udp_packet(ip_dst=dip, ip_src=sip)
        send_packet(self, dflt_port, p)
        verify_packet(self, p, dflt_port)
        # A UDP packet from the correct ingress port will match the forwarding
        # rule.
        send_packet(self, ing_port, p)
        verify_packet(self, p, hit_port)
        
        # Change the table's mask to ignore the ingress port and protocol.  Note
        # that the table must be empty in order to change the mask since this is
        # a traffic disruptive event.
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', 0),
                             gc.KeyTuple('hdr.ipv4.dst_addr', '255.255.255.255'),
                             gc.KeyTuple('hdr.ipv4.src_addr', '255.255.255.255'),
                             gc.KeyTuple('hdr.ipv4.protocol', 0)])
        try:
            self.t.attribute_dyn_key_mask_set(trgt, k)
            logger.info("Expected a failure while changing a non-empty table's mask")
            self.assertFalse(True)
        except gc.BfruntimeReadWriteRpcException:
            pass
        self.t.entry_del(trgt, [])
        self.t.attribute_dyn_key_mask_set(trgt, k)
        resp = self.t.attribute_get(trgt, "DynamicKeyMask")
        for d in resp:
            dd = d["fields"].to_dict()
            self.assertEqual(dd["ig_intr_md.ingress_port"]["value"], 0)
            self.assertEqual(dd["hdr.ipv4.dst_addr"]["value"], '255.255.255.255')
            self.assertEqual(dd["hdr.ipv4.src_addr"]["value"], '255.255.255.255')
            self.assertEqual(dd["hdr.ipv4.protocol"]["value"], 0)

        # Add our table entries back and test with traffic again.  Since the
        # ingress port and protocol are masked out we set them to zero in the
        # key.
        d = self.t.make_data([gc.DataTuple('port', dflt_port)], 'Ing.fwd')
        self.t.default_entry_set(trgt, d) 
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', 0),
                             gc.KeyTuple('hdr.ipv4.dst_addr', dip),
                             gc.KeyTuple('hdr.ipv4.src_addr', sip),
                             gc.KeyTuple('hdr.ipv4.protocol', 0)])
        d = self.t.make_data([gc.DataTuple('port', hit_port)], 'Ing.fwd')
        self.t.entry_add(trgt, [k], [d])

        # Now any protocol and any ingress port will match the rule
        p = simple_tcp_packet(ip_dst=dip, ip_src=sip)
        send_packet(self, ing_port, p)
        verify_packet(self, p, hit_port)
        send_packet(self, dflt_port, p)
        verify_packet(self, p, hit_port)

        # The IP addresses are still matched though, a packet with different IPs
        # will miss the forwarding rule.
        p = simple_udp_packet(ip_dst=non_matching_dip, ip_src=sip)
        send_packet(self, ing_port, p)
        verify_packet(self, p, dflt_port)

        # Delete the entry so the mask can be changed again.
        self.t.entry_del(trgt, [k])
        # Change the mask to also ignore a few bits of IP src address.
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', 0),
                             gc.KeyTuple('hdr.ipv4.dst_addr', '255.255.255.255'),
                             gc.KeyTuple('hdr.ipv4.src_addr', '255.255.255.248'),
                             gc.KeyTuple('hdr.ipv4.protocol', 0)])
        self.t.attribute_dyn_key_mask_set(trgt, k)

        # Add an entry to match dest IP and part of the source IP
        k = self.t.make_key([gc.KeyTuple('ig_intr_md.ingress_port', 0),
                             gc.KeyTuple('hdr.ipv4.dst_addr', dip),
                             gc.KeyTuple('hdr.ipv4.src_addr', sip_masked),
                             gc.KeyTuple('hdr.ipv4.protocol', 0)])
        d = self.t.make_data([gc.DataTuple('port', hit_port)], 'Ing.fwd')
        self.t.entry_add(trgt, [k], [d])

        # Send a few entries which vary in the masked portion of the source IP,
        # they should all match the entry.
        for i in [sip_masked_val1, sip_masked_val2, sip_masked_val3]:
          p = simple_tcp_packet(ip_dst=dip, ip_src=i)
          send_packet(self, ing_port, p)
          verify_packet(self, p, hit_port)
        # Send one which should not match
        p = simple_tcp_packet(ip_dst=dip, ip_src=sip_masked_miss)
        send_packet(self, ing_port, p)
        verify_packet(self, p, dflt_port)



class DKMKeySliceTest(BfRuntimeTest):
    """ A simple example of DKM table usage.
        Adds entries to the table and checks that forwarding works.
        Adjusts the table's mask and again checks that forwarding works.
        Note that this table uses a field slice in one of the key fields.
    """
    def setUp(self):
        BfRuntimeTest.setUp(self, client_id)
        self.bfrt_info = self.interface.bfrt_info_get(p4_name)
        self.t_fwd = self.bfrt_info.table_get('Ing.t1')
        self.t = self.bfrt_info.table_get('Egr.t2')
        self.cntr = self.bfrt_info.table_get('Egr.cntr')
        # Clear tables to ensure a good starting state
        self.t_fwd.entry_del(gc.Target(device_id=dev_id), [])
        self.t.entry_del(gc.Target(device_id=dev_id), [])
        self.cntr.entry_del(gc.Target(device_id=dev_id), [])

    def tearDown(self):
        trgt = gc.Target(device_id=dev_id)
        self.t_fwd.entry_del(trgt, [])
        self.t.entry_del(trgt, [])
        self.cntr.entry_del(trgt, [])
        k = self.t.make_key([gc.KeyTuple('intr_md.egress_port', 0x1FF),
                             gc.KeyTuple('hdr.ethernet.dst_addr[31:0]', 0xFFFFFFFF)])
        self.t.attribute_dyn_key_mask_set(trgt, k)
        self.cntr.entry_del(trgt, [])
        BfRuntimeTest.tearDown(self)

    def runTest(self):
        trgt = gc.Target(device_id=dev_id)
        port = swports[0]
        ctr_idx = 4095
        ctr_dflt_idx = 1
        k_hit  = self.cntr.make_key([gc.KeyTuple('$COUNTER_INDEX', ctr_idx)])
        k_miss = self.cntr.make_key([gc.KeyTuple('$COUNTER_INDEX', ctr_dflt_idx)])

        # Setup an ingress entry to forward to the egress table.
        d = self.t_fwd.make_data([gc.DataTuple('port', port)], 'Ing.fwd')
        self.t_fwd.default_entry_set(trgt, d) 

        # Get the table's current DKM mask, it will have all fields included.
        resp = self.t.attribute_get(trgt, "DynamicKeyMask")
        for d in resp:
            dd = d["fields"].to_dict()
            self.assertEqual(dd["hdr.ethernet.dst_addr[31:0]"]["value"], 0xFFFFFFFF)
            self.assertEqual(dd["intr_md.egress_port"]["value"], 0x1FF)

        # Change the mask to ignore the top byte and second to last bit.  These
        # are just arbitrary portions of the key to illustrate that fields can
        # be partially masked.
        k = self.t.make_key([gc.KeyTuple('hdr.ethernet.dst_addr[31:0]', 0x00FFFFFD),
                             gc.KeyTuple('intr_md.egress_port', 0x1FF)])
        self.t.attribute_dyn_key_mask_set(trgt, k)

        # Add an entry
        k = self.t.make_key([gc.KeyTuple('hdr.ethernet.dst_addr[31:0]', 0x00FFFFFD),
                             gc.KeyTuple('intr_md.egress_port', port)])
        d = self.t.make_data([gc.DataTuple('i', ctr_idx)], 'Egr.count')
        self.t.entry_add(trgt, [k], [d])
        resp = self.t.entry_get(trgt, [k], {'from_hw':True})
        resp_cnt = 0
        for resp_d, resp_k in resp:
            self.assertEqual(resp_k.to_dict(), k.to_dict())
            self.assertEqual(resp_d.to_dict(), d.to_dict())
            resp_cnt += 1
        self.assertEqual(1, resp_cnt)

        # Add an entry which differs only in the masked bits, this will fail as
        # a duplicate entry.
        k = self.t.make_key([gc.KeyTuple('hdr.ethernet.dst_addr[31:0]', 0xFFFFFFFF),
                             gc.KeyTuple('intr_md.egress_port', port)])
        d = self.t.make_data([gc.DataTuple('i', ctr_idx)], 'Egr.count')
        try:
            self.t.entry_add(trgt, [k], [d])
            logger.info("Expected a failure while adding a duplicate entry")
            self.assertFalse(True)
        except gc.BfruntimeReadWriteRpcException:
            pass

        # Send a packet exactly matching the entry.
        p = simple_udp_packet(eth_dst="FF:FF:00:FF:FF:FD")
        send_packet(self, swports[-1], p)
        verify_packet(self, p, port)

        # Send a packet which matches the entry after masking
        p = simple_udp_packet(eth_dst="FF:FF:FF:FF:FF:FF")
        send_packet(self, swports[-1], p)
        verify_packet(self, p, port)

        # Verify we now have a count of two (e.g. the entry was matched twice).
        resp = self.cntr.entry_get(trgt, [k_hit], {'from_hw':True})
        dd = next(resp)[0].to_dict()
        hit_cnt = dd['$COUNTER_SPEC_PKTS']
        resp = self.cntr.entry_get(trgt, [k_miss], {'from_hw':True})
        dd = next(resp)[0].to_dict()
        miss_cnt = dd['$COUNTER_SPEC_PKTS']
        self.assertEqual(0, miss_cnt)
        self.assertEqual(2, hit_cnt)

        # Send a non-matching packet and ensure it counted as a miss
        p = simple_udp_packet(eth_dst="FF:FF:FF:FF:FF:FE")
        send_packet(self, swports[-1], p)
        verify_packet(self, p, port)
        resp = self.cntr.entry_get(trgt, [k_hit], {'from_hw':True})
        dd = next(resp)[0].to_dict()
        hit_cnt = dd['$COUNTER_SPEC_PKTS']
        resp = self.cntr.entry_get(trgt, [k_miss], {'from_hw':True})
        dd = next(resp)[0].to_dict()
        miss_cnt = dd['$COUNTER_SPEC_PKTS']
        self.assertEqual(1, miss_cnt)
        self.assertEqual(2, hit_cnt)
