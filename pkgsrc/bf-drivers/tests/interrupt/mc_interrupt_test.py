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

# mc_interrupt_test.py
#
# Tofino1 multicast memory interrupt tests.
#
from __future__ import print_function

import argparse
import logging
import os
import re
import sys

try:
    import pexpect
except ImportError:
    sys.stderr.write("You do not have pexpect installed\n")

#try:
#    from scp import SCPClient
#except ImportError:
#    sys.stderr.write("You do not have python-scp installed\n")

from time import sleep

##
# ssh class
#
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
    def __init__(self, address, username, password, echo="False", filename=""):
        print("Connecting to %s" % address)
        self.child = pexpect.spawn('ssh -o StrictHostKeyChecking=no -l %s %s' % (username, address))
        if filename == "":
            filename = "logfile-" + address + ".txt"
            self.logfile = file(filename, "wb")
            self.child.logfile = self.logfile
            self.child.logfile_read = sys.stdout
        else:
            self.logfile = file(filename, "wb")
            self.child.logfile = self.logfile
            self.child.logfile_read = sys.stdout

        i = self.child.expect([self.PASSWD_PROMPT, self.TERMINAL_PROMPT, self.COMMAND_PROMPT])
        self.child.sendline(password)
        print(self.child.before, self.child.after)
        if i == 1:
            self.child.sendline(self.TERMINAL_TYPE)
            self.child.expect(self.COMMAND_PROMPT)
        self.child.setecho(echo)

    def send(self, cmd, timeout=180):
        print(cmd)
        self.child.send(cmd)
        self.child.send("\r\n")

    def expect(self, prompt, timeout=180):
        print(prompt)
        self.child.expect(prompt, timeout)
        return self.child.before

    def before(self):
        return self.child.before

    def after(self):
        return self.child.after

    def sendonly(self, cmd):
        print(cmd)
        self.child.sendline(cmd)

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
            self.logfile.close()
    # end closeConnection
# end ssh

parser = argparse.ArgumentParser(version="mc_interrupt_test 0.2")

group = parser.add_argument_group("McInterruptTest")
group.add_argument("--ip", type=str, required=True, help="ip address of switch")
group.add_argument("--log", type=str, required=True, help="logfilename, ex: test.log")
group.add_argument("--path", type=str, required=True, help="install dir path")

####################
# Utility Routines #
####################

def readInterruptCnt(swObj):
    swObj.send("int_new 0")
    swObj.send("intr_dump -d0")
    output = swObj.expect('Total number of interrupts:\s+\S+')
    output += swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    status = re.search('Total number of interrupts:\s+(\d+)', output)
    intr_cnt = int(status.group(1))
    return intr_cnt

def EnterAccessMode(swObj):
    swObj.send("access")
    swObj.send("\r\n")
    swObj.expect('Access:\s+')

def LeaveAccessMode(swObj):
    swObj.send("quit")
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def createMcSession(swObj):
    swObj.send("create-sess")
    output=swObj.expect('Created session w/ handle\s+\S+')
    output+=swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    status=re.search('Created session w/ handle\s+(\S+)',output)
    sess_hdl = status.group(1)
    return sess_hdl

def destroyMcSession(swObj, sess_hdl):
    swObj.send("destroy-sess " + str(sess_hdl))
    swObj.expect('Session\s+\S+')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def mgrpCreate(swObj, sess_hdl, grp_id):
    swObj.send("mgrp-create -d0 -i " + str(grp_id) + " -s " +  str(sess_hdl))
    if (grp_id == 0):
        output=swObj.expect('Create group ' + str(grp_id) + ' on device 0 returned status Success and handle\s+\S+')
    else:
        output=swObj.expect('Create group 0x' + str(grp_id) + ' on device 0 returned status Success and handle\s+\S+')
    output+=swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    if (grp_id == 0):
        status=re.search('Create group ' + str(grp_id) + ' on device 0 returned status Success and handle\s+(\S+)',output)
    else:
        status=re.search('Create group 0x' + str(grp_id) + ' on device 0 returned status Success and handle\s+(\S+)',output)
    mgrp_hdl = status.group(1)
    return mgrp_hdl

def mgrpDestroy(swObj, sess_hdl, grp_hdl):
    swObj.send("mgrp-destroy -d0 -s " +  str(sess_hdl) + " -g " + str(grp_hdl))
    swObj.expect('Destroy group handle\s+\S+')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def nodeCreate(swObj, sess_hdl):
    swObj.send("node-create -d0 -r0 -l0 -s " +  str(sess_hdl))
    output=swObj.expect('Node created for dev 0 with RID 0 returned status Success and node handle\s+\S+')
    output+=swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    status=re.search('Node created for dev 0 with RID 0 returned status Success and node handle\s+(\S+)',output)
    node_hdl = status.group(1)
    return node_hdl

def nodeDestroy(swObj, sess_hdl, node_hdl):
    swObj.send("node-destroy -d0 -s " +  str(sess_hdl) + " -n " + str(node_hdl))
    swObj.expect('Destroy node\s+\S+')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl):
    swObj.send("l1-associate -d0 -s " + str(sess_hdl) + " -g " + str(mgrp_hdl) + " -n " +  str(node_hdl))
    output=swObj.expect('Associate L1 node\s+\S+')
    output+=swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl):
    swObj.send("l1-dissociate -d0 -s " + str(sess_hdl) + " -g " + str(mgrp_hdl) + " -n " +  str(node_hdl))
    swObj.expect('Dissociate L1 node\s+\S+')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def cleanupTest():
    print("Test Result: FAILED")
    exit(1)

##############
# Test Cases #
##############

##
# TM-PRE PMT SBE (single-bit error)
#
def test_tof1_tm_pre_pmt_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE PMT sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x60000000")
    LeaveAccessMode(swObj)

    swObj.send("pmt-wr -s " + str(sess_hdl) + " -d0 -i1 -p0")
    swObj.send("pmt-rd -h -d0 -s1 -e1")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("pmt-rd -h -d0 -s1 -e1")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_pmt_sbe

##
# TM-PRE PMT MBE (multi-bit error)
#
def test_tof1_tm_pre_pmt_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE PMT mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x300000")
    LeaveAccessMode(swObj)

    swObj.send("pmt-wr -s " + str(sess_hdl) + " -d0 -i2 -p2")
    swObj.send("pmt-rd -h -d0 -s2 -e2")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name +  " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("pmt-rd -h -d0 -s2 -e2")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_pmt_mbe

##
# TM-PRE MIT SBE (single-bit error)
#
def test_tof1_tm_pre_mit_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE MIT sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x1000000")
    LeaveAccessMode(swObj)

    grp_id = 0
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    swObj.send("mit-rd -h -d0 -s " + str(grp_id) + " -e " + str(grp_id))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("mit-rd -h -d0 -s " + str(grp_id) + " -e " + str(grp_id))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof1_tm_pre_mit_sbe

##
# TM-PRE MIT MBE (multi-bit error)
#
def test_tof1_tm_pre_mit_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE MIT mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x8000")
    LeaveAccessMode(swObj)

    grp_id = 0
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    swObj.send("mit-rd -h -d0 -s " + str(grp_id) + " -e " + str(grp_id))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("mit-rd -h -d0 -s " + str(grp_id) + " -e " + str(grp_id))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof1_tm_pre_mit_mbe

##
# TM-PRE LIT BM SBE (single-bit error)
#
def test_tof1_tm_pre_lit_bm_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT BM sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x6000000")
    LeaveAccessMode(swObj)

    lagid = 2
    swObj.send("lit-wr -d0 -i " + str(lagid) + " -p3 -s " + str(sess_hdl))
    swObj.send("lit-rd -h -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("lit-rd -h -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_lit_bm_sbe

##
# TM-PRE LIT BM MBE (multi-bit error)
#
def test_tof1_tm_pre_lit_bm_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT BM mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x30000")
    LeaveAccessMode(swObj)

    lagid = 3
    swObj.send("lit-wr -d0 -i " + str(lagid) + " -p3 -s " + str(sess_hdl))
    swObj.send("lit-rd -h -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("lit-rd -h -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_lit_bm_mbe

##
# TM-PRE LIT NP SBE (single-bit error)
#
def test_tof1_tm_pre_lit_np_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT NP sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x18000000")
    LeaveAccessMode(swObj)

    lagid = 6
    swObj.send("lit-np-wr -d0 -i " + str(lagid) + " -l2 -r3 -s " + str(sess_hdl))
    swObj.send("lit-np-rd -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("lit-np-rd -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_lit_np_sbe

##
# TM-PRE LIT NP MBE (multi-bit error)
#
def test_tof1_tm_pre_lit_np_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT NP mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x60000")
    LeaveAccessMode(swObj)

    lagid = 7
    swObj.send("lit-np-wr -d0 -i " + str(lagid) + " -l2 -r3 -s " + str(sess_hdl))
    swObj.send("lit-np-rd -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("lit-np-rd -d0 -v0 -s " + str(lagid) + " -e " + str(lagid))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_tm_pre_lit_np_mbe

##
# TM-PRE RDM SBE (single-bit error)
#
def test_tof1_tm_pre_rdm_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE RDM sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] int_inj 0x80000000")
    LeaveAccessMode(swObj)

    grp_id = 0
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    # Read node from RDM
    swObj.send("l1-read -h -d0 -n " +  str(node_hdl))
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("l1-read -h -d0 -n " +  str(node_hdl))
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof1_tm_pre_rdm_sbe

##
# Deparser PVT SBE (single-bit error)
#
def test_tof1_dprsr_pvt_sbe(swObj, sess_hdl, write_time):
    test_name = "Ig-Deparser Pipe Vector Tbl sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] deparser hdr hir ingr pvt_ecc_ctrl 0x1")
    LeaveAccessMode(swObj)

    # Write group 1
    swObj.send("pvt-wr -d0 -g1 -m6 -s " + str(sess_hdl))
    swObj.send("pvt-rd -h -d0 -s1 -e1")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("pvt-rd -h -d0 -s1 -e1")
    intr_cnt_after_verify = readInterruptCnt(swObj)
# end test_tof1_dprsr_pvt_sbe

##
# Deparser PVT MBE (multi-bit error)
#
def test_tof1_dprsr_pvt_mbe(swObj, sess_hdl, write_time):
    test_name = "Ig-Deparser Pipe Vector Tbl mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] deparser hdr hir ingr pvt_ecc_ctrl 0x2")
    LeaveAccessMode(swObj)

    # Write group 2
    swObj.send("pvt-wr -d0 -g2 -m6 -s " + str(sess_hdl))
    swObj.send("pvt-rd -h -d0 -s2 -e2")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("pvt-rd -h -d0 -s2 -e2")
    intr_cnt_after_verify = readInterruptCnt(swObj)
# end test_tof1_dprsr_pvt_mbe

################
# Main Program #
################

if __name__ == "__main__":

    args = parser.parse_args()

    ipaddr = args.ip
    path = args.path
    username = "admin12"
    password = "bfn123"
    echo = "False"
    logfile = args.log
    if logfile == "":
        logfile = ipaddr + "-interrupt.log"
    swObj = ssh(ipaddr, username, password, echo, logfile)

    print("Multicast Interrupt Test script")

    prompt = username + '@bfn-switch:~/\S+'

    swObj.sendlineExpect("cd " + path + "/..", prompt)

    #swObj.sendonly("sudo ./install/bin/bf_kdrv_mod_unload")
    #swObj.expect('[$#:] ')
    #swObj.send(password)
    #swObj.expect('[$#] ')

    swObj.sendlineExpect("sudo ./install/bin/bf_kdrv_mod_load install/", prompt)
    swObj.expect('[$#:] ')
    swObj.send(password)
    swObj.expect('[$#] ')
    print("Ignore Error if already loaded")
    sleep(15)

    swObj.send("./tools/run_switchd.sh -p basic_ipv4 -- --ucli")
    swObj.expect('bf_switchd: dev_id 0 initialized')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    sleep(15)

    print("Forcing an interrupt poll")
    swObj.send("int_poll 0 n")
    swObj.send("int_new 0")
    sleep(2)

    write_time = 1

    mc_sess_hdl = createMcSession(swObj)
    print("Mc Session Handle is ", mc_sess_hdl)

    test_tof1_tm_pre_pmt_sbe(swObj, mc_sess_hdl, write_time)
    test_tof1_tm_pre_pmt_mbe(swObj, mc_sess_hdl, write_time)

    test_tof1_tm_pre_mit_sbe(swObj, mc_sess_hdl, write_time)
    test_tof1_tm_pre_mit_mbe(swObj, mc_sess_hdl, write_time)

    test_tof1_tm_pre_lit_bm_sbe(swObj, mc_sess_hdl, write_time)
    test_tof1_tm_pre_lit_bm_mbe(swObj, mc_sess_hdl, write_time)

    test_tof1_tm_pre_lit_np_sbe(swObj, mc_sess_hdl, write_time)
    test_tof1_tm_pre_lit_np_mbe(swObj, mc_sess_hdl, write_time)

    test_tof1_tm_pre_rdm_sbe(swObj, mc_sess_hdl, write_time)
    # RDM mbe errors are fatal

    test_tof1_dprsr_pvt_sbe(swObj, mc_sess_hdl, write_time)
#   test_tof1_dprsr_pvt_mbe(swObj, mc_sess_hdl, write_time)
    print("\n** Ig-Deparser PVT MBE test disabled (see DRV-4568) **\n")

    destroyMcSession(swObj, mc_sess_hdl)
    print("Test Result: PASSED")
# end __main__
