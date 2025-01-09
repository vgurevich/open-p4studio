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

import threading

tables = bfrt.tna_idletimeout.pipe.SwitchIngress

class BfrtPyCallbackTest:
    def __init__(self):
        global threading
        self.complete_sem = threading.Semaphore(0)
        self.ret_kwargs = -1
        self.ret_args = -1

    def callback_generate(self, ret=False):
        if ret:
            def cb(*args, **kwargs):
                self.ret_kwargs = kwargs
                self.ret_args = args
                self.complete_sem.release()
                return 0
            return cb
        def cb(*args, **kwargs):
            self.ret_kwargs = kwargs
            self.ret_args = args
            self.complete_sem.release()
        return cb

    def wait_test(self, verify):
        self.complete_sem.acquire()
        verify(*self.ret_args, **self.ret_kwargs)

cb_tester = BfrtPyCallbackTest()
def verifier(dev_id, pipe_id, direction, parser_id, entry, notif_type):
    global tables
    try:
        if dev_id != 0 or pipe_id != 65535 or direction != 0xff or parser_id != 0xff:
            print("Error: failure on chip info")
        print("Target received in idle cb:", (dev_id, pipe_id, direction, parser_id))
        if entry.raw()["data"]["port"] != 0x45:
            print("Error: entry data mismatch")

        # entry mod
        # object mod
        tables.dmac.entry_with_hit(port=0x1).push()
        # entry get
        ent = tables.dmac.get(print_ents=False)
        if ent.raw()["data"]["port"] != 0x1:
            print("Error: entry obj push failed")

        # object get
        entry.update()
        if entry.raw()["data"]["port"] != 0x1:
            print("Error: entry obj update failed")

        # object del
        entry.remove()
    except Exception as e:
        print("Error: {}".format(e))


tables.dmac.idle_table_set_notify(enable=True,
                                  interval=500,
                                  min_ttl=1000,
                                  max_ttl=10000,
                                  callback=cb_tester.callback_generate())

# entry add
tables.dmac.add_with_hit(ethernet_dst_addr="00:01:02:03:04:05",
                         ethernet_src_addr="06:07:08:09:10:11",
                         ethernet_src_addr_mask=0xffffffffffff,
                         ipv4_dst_addr="0.0.0.1",
                         port=0x45,
                         ENTRY_TTL=2000,
                         MATCH_PRIORITY=0,
                         )

# entry get
eraw = tables.dmac.get(ethernet_dst_addr="00:01:02:03:04:05",
                       ethernet_src_addr="06:07:08:09:10:11",
                       ethernet_src_addr_mask=0xffffffffffff,
                       ipv4_dst_addr="0.0.0.1",
                       MATCH_PRIORITY=0,
                       print_ents=False).raw()

# LPM, ternary, exact, ipv4, mac parsing
if eraw['key']['hdr.ethernet.dst_addr'] != 0x102030405:
    print("Error: inconsistent exact match parse & deparse")
    print(eraw['key']['hdr.ethernet.dst_addr'])

val, mask = eraw['key']['hdr.ethernet.src_addr']
if val != 0x60708091011:
    print("Error: inconsistent LPM parse & deparse")
    print(val)

if mask != 0xffffffffffff:
    print("Error: bad LPM prefix parse & deparse")
    print(mask)

val, mask = eraw['key']['hdr.ipv4.dst_addr']
if val != 1:
    print("Error: inconsistent ternary parse & deparse")
    print(val)

if mask != 0xFFFFFFFF:
    print("Error: bad ternary mask default")
    print(mask)

# entry del
tables.dmac.delete(ethernet_dst_addr="00:01:02:03:04:05",
                   ethernet_src_addr="06:07:08:09:10:11",
                   ethernet_src_addr_mask=0xffffffffffff,
                   MATCH_PRIORITY=0,
                   ipv4_dst_addr="0.0.0.1")

# object add
tables.dmac.entry_with_hit(port=0x45, ENTRY_TTL=2000).push()
cb_tester.wait_test(verifier)
tables.dmac.idle_table_set_notify(enable=False)

print("Test Success!")
