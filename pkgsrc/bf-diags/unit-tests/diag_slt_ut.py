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

#!/usr/bin/env python
from __future__ import print_function

import argparse
import logging
import math
import os
import re
import sys
try:
    import pexpect
except ImportError:
    sys.stderr.write("You do not have pexpect installed\n")
from time import sleep
import time
import datetime
import json

class ssh():
    """
    class ssh
    """

    child = None
    logfile = ""
    SSH_NEWKEY = r'Are you sure you want to continue connecting \(yes/no\)\?'
    PASSWD_PROMPT = 'password: '
    COMMAND_PROMPT = '[$#] '
    TERMINAL_PROMPT = r'Terminal type\?'
    TERMINAL_TYPE = 'vt100'
    def __init__(self, address, username, password, echo="False"):
        print("Connecting to %s" % address)
        self.child = pexpect.spawn('ssh -o StrictHostKeyChecking=no -l %s %s' % (username, address))
        self.child.logfile_read = sys.stdout

        i = self.child.expect([self.PASSWD_PROMPT, self.TERMINAL_PROMPT, self.COMMAND_PROMPT])
        self.child.sendline(password)
        print(self.child.before, self.child.after)
        if i == 1:
            self.child.sendline(self.TERMINAL_TYPE)
            self.child.expect(self.COMMAND_PROMPT)
        self.child.setecho(echo)

    def setLogfile(self, logfile):
        self.logfile = logfile
        self.child.logfile = self.logfile
        self.child.logfile_read = sys.stdout

    def send(self, cmd, timeout=180):
        print(cmd)
        self.child.send(cmd)
        self.child.send("\r\n")

    def expect(self, prompt, timeout=180):
        print(prompt)
        self.child.expect([prompt, pexpect.EOF], timeout)
        return self.child.before

    def before(self):
        return self.child.before

    def after(self):
        return self.child.after

    def sendonly(self, cmd):
        print(cmd)
        self.child.sendline(cmd)

    def read(self):
        self.child.read()

    def sendExpect(self, cmd, prompt, timeout=180):
        print(cmd)
        print(prompt)
        self.child.send(cmd)
        self.child.send("\r\n")
        self.child.expect(prompt, timeout=timeout)
        if re.search("FAILED", self.child.before):
            return 1
        return 0

    def sendlineExpect(self, cmd, prompt, timeout=180):
        print(cmd)
        print(prompt)
        self.child.sendline(cmd)
        self.child.expect(self.COMMAND_PROMPT, timeout=timeout)
        if re.search("FAILED", self.child.before):
            return 1
        return 0

    def closeConnection(self):
        if self.child != None:
            self.child.close()
        if self.logfile != "":
            self.logfile.close()


def port_add_25G(swObj):
    swObj.send("port-add -/- 25G NONE")
    swObj.send("an-set -/- 2")
    swObj.send("port-enb -/-")
    swObj.expect('bf-sde>\s+')
    sleep(10)

def port_del(swObj):
    swObj.send("port-del -/-")
    swObj.expect('bf-sde>\s+')
    sleep(10)

def start_switchd(swObj):
    swObj.send("./tools/run_switchd.sh -p diag")
    swObj.send("bfn123")
    swObj.expect('Starting DIAG RPC server on port 9096')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def port_add(swObj):
    swObj.send("port-add -/- 100G NONE")
    swObj.send("an-set -/- 2")
    swObj.send("port-enb -/-")
    swObj.expect('bf-sde>\s+')
    sleep(10)

def pair_setup(swObj, ports, intportstr):
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("clr_ints")
    swObj.expect('bf-sde>\s+')
    if intportstr != "":
        swObj.send("pair-setup 0 alli 2")
    else:
        swObj.send("pair-setup 0 %s 2" % ports)
    swObj.expect('Pair test Session Handle:\s+\S+')
    pairout=swObj.after()
    session=re.search('Pair test Session Handle:\s+(\d+)', pairout).group(1)
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    return session

def pair_start(swObj, session, pkts, size, bidir=1):
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("pair-start %d %d %d %d" % (int(session), int(pkts), int(size), bidir))
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def pair_stop(swObj, session):
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send('pair-stop %d' % int(session), timeout=600)
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def pair_status(swObj, session):
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("pair-status %d" % int(session), timeout=600)
    sleep(5)
    pair_status=swObj.expect('No of Tx packet completions:\s+')
    pair_status += swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    res=re.search('  Test Status: (\S+)',pair_status)
    pairteststatus=str(res.group(1))

    print("******** PAIR STATUS %s **************" % pairteststatus)
    return pairteststatus

def pair_cleanup(swObj, session):
    swObj.send("pair-cleanup %d" % int(session))
    swObj.expect('bf-sde>\s+')
    swObj.send("int_new")
    swObj.expect('bf-sde>\s+')

def ssetup_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 0")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xff 0xff 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def sssetup_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 1")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xff 0xff 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def shold_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 2")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0x00 0x00 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def sshold_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 3")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0x00 0x00 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def unknown_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 4")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xaa 0x55 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def payload_setup_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 5")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xff 0xff 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def mixed_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 6")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xaa 0x55 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus

def nofailure_test():
    port_add(swObj)
    ports="412,412"
    session1=pair_setup(swObj, ports, "")
    swObj.send("slt-test-mode 7")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    swObj.send("data-pattern %d 1 0xaa 0x55 1" % int(session1))
    swObj.expect("Fixed\s+data\s+pattern\s+set\s+success")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    pair_start(swObj, session1, 15, 850, 0) 
    sleep(60)
    pair_stop(swObj, session1)
    pairteststatus=pair_status(swObj, session1)
    pair_cleanup(swObj, session1)
    port_del(swObj)
    return pairteststatus


usage = "Usage: diag_slt_ut.py --workspace <workspace>"
parser = argparse.ArgumentParser(version="diag_slt_ut.py 1.0", usage=usage)

group = parser.add_argument_group("diag_slt_ut")
group.add_argument("--workspace", type=str, required=True, help="workspace")

if __name__ == "__main__":

    # script version
    version = "1.0"

    starttime=time.time()

    args = parser.parse_args()

    workspace = args.workspace

    echo="False"

    ipaddr="localhost"
    username="admin12"
    password="bfn123" 

    run_status=0
    try:
        swObj = ssh(ipaddr, username, password, echo)
        swObj.expect('[$#:] ')
        swObj.send("bfn123")
        swObj.expect('[$#] ')
        swObj.sendlineExpect("lspci | grep 1d1c", 'admin12@bfn-switch:~ ')
        swObj.sendlineExpect("cd %s" % workspace, '[$#] ')
        swObj.sendonly("sudo ./install/bin/bf_kdrv_mod_unload")
        swObj.expect('[$#:] ')
        swObj.send("bfn123")
        swObj.expect('[$#] ')

        swObj.sendlineExpect("sudo ./install/bin/bf_kdrv_mod_load install/", '[$#] ')
        sleep(10)
        swObj.send("./tools/run_switchd.sh -p diag")
        swObj.expect('bf_switchd: server started - listening on port 9999')
        swObj.send("\r\n")
        swObj.expect('bf-sde>\s+')

        # s_setup test
        ssetup_status=ssetup_test()
        run_status=0 if ssetup_status=="Fail" and run_status==0 else 1
        # ss_setup test
        sssetup_status=sssetup_test()
        run_status=0 if sssetup_status=="Fail" and run_status==0 else 1
        # s_hold test
        shold_status=shold_test()
        run_status=0 if shold_status=="Fail" and run_status==0 else 1
        # ss_hold test
        sshold_status=sshold_test()
        run_status=0 if sshold_status=="Fail" and run_status==0 else 1
        # unknown test
        unknown_status=unknown_test()
        run_status=0 if unknown_status=="Fail" and run_status==0 else 1
        # payload setup
        payload_status=payload_setup_test()
        run_status=0 if payload_status=="Fail" and run_status==0 else 1
        # mixed
        mixed_status=mixed_test()
        run_status=0 if mixed_status=="Fail" and run_status==0 else 1
        # no failure
        nofailure_status=nofailure_test()
        run_status=0 if nofailure_status=="Pass" and run_status==0 else 1

        swObj.sendlineExpect('\003', '[$#:] ')

        swObj.closeConnection()
        # print summary of the result
        print("S_SETUP TEST STATUS (XFAIL): %s" % ssetup_status)
        print("SS_SETUP TEST STATUS (XFAIL): %s" % sssetup_status)
        print("S_HOLD TEST STATUS (XFAIL): %s" % shold_status)
        print("SS_HOLD TEST STATUS (XFAIL): %s" % sshold_status)
        print("UNKNOWN TEST STATUS (XFAIL): %s" % unknown_status)
        print("MIXED TEST STATUS (XFAIL): %s" % mixed_status)
        print("NOFAILURE TEST STATUS (XPASS): %s" % nofailure_status)
        exit(run_status)
    except pexpect.EOF:
        print("EOF")
        exit(1)
    except pexpect.TIMEOUT:
        print("TIMEOUT")
        print("DIAG SLT UNIT TESTS FAILED, THERE WAS AN EXCEPTION")
        exit(1)
