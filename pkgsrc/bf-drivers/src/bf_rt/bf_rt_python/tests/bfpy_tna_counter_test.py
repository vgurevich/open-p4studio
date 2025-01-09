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
from time import sleep


tables = bfrt.tna_counter.pipe.SwitchIngress

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
def verifier(dev_id, pipe_id, direction, parser_id):
    global tables
    try:
        if dev_id != 0 or pipe_id != 65535 or direction != 0xFF or parser_id != 0xFF:
            print("Error: Callback got an unexpected dev_target")
    except Exception as e:
        print("Error: {}".format(e))

# Send frame, verify hw vs sw read
src_addr = 0x001122334455
src_addr_mask = 0xffffffffffff
tables.forward.add_with_hit(src_addr, src_addr_mask)

unsynced = tables.forward.get(src_addr, src_addr_mask, print_ents=False).raw()
if unsynced['data']['$COUNTER_SPEC_PKTS'] != 0 or unsynced['data']['$COUNTER_SPEC_BYTES'] != 0:
  print("Error: unsynced counter get returns unexpected vals:\n{}".format(unsynced))

tables.forward.operation_counter_sync(callback=cb_tester.callback_generate())
cb_tester.wait_test(verifier)

synced = tables.forward.get(src_addr, src_addr_mask, print_ents=False).raw()
if synced['data']['$COUNTER_SPEC_PKTS'] != 0 or synced['data']['$COUNTER_SPEC_BYTES'] != 0:
  print("Error: synced counter get returns unexpected vals:\n{}".format(synced))


tables.forward.clear()

tables.forward.add_with_hit(src_addr, src_addr_mask)
handle = tables.forward.get_handle(src_addr, src_addr_mask)
tables.forward.delete(handle=handle)
err = tables.forward.get(src_addr, src_addr_mask, print_ents=False)
if (err != -1):
  print("Error: entry was not deleted")

print("Test Success!")
