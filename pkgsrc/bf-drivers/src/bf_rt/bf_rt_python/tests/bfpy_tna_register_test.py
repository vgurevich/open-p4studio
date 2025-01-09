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


tables = bfrt.tna_register.pipe.SwitchIngress

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


tables.reg_match.add_with_register_action()
tables.test_reg.mod(first=0x45, second=42)
unsynced = tables.test_reg.get(print_ents=False).raw()
if unsynced['data']['SwitchIngress.test_reg.first'][0] != 69 or unsynced['data']['SwitchIngress.test_reg.second'][0] != 42:
  print("Error: unsynced register get returns unexpected vals:\n{}".format(unsynced))

tables.test_reg.operation_register_sync(callback=cb_tester.callback_generate())
cb_tester.wait_test(verifier)

synced = tables.test_reg.get(print_ents=False).raw()
if synced['data']['SwitchIngress.test_reg.first'][0] != 0x45 or synced['data']['SwitchIngress.test_reg.second'][0] != 42:
  print("Error: synced register get returns unexpected vals:\n{}".format(synced))

#reset state
tables.reg_match.delete()
tables.test_reg.mod()
tables.test_reg.operation_register_sync(callback=cb_tester.callback_generate())
cb_tester.wait_test(verifier)

tables.reg_match.clear()

print("Test Success!")
