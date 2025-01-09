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

# tof2_mc_interrupt_test.py
#
# Tofino2 multicast memory interrupt tests.
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

parser = argparse.ArgumentParser(version="tof2_mc_interrupt_test 0.2")

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
def test_tof2_tm_pre_pmt_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE PMT sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # pmt0_inject_sbe(25), pmt1_inject_sbe(29)
    mask = hex((1 << 25) | (1 << 29))   # 0x2200'0000
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    pmt_id = 1
    port = 0

    # Write to PMT
    wr_cmd = "pmt-wr -s{0} -d0 -i{1} -p{2}".format(sess_hdl, pmt_id, port)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "pmt-rd -h -d0 -s{0} -e{0}".format(pmt_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_pmt_sbe

##
# TM-PRE PMT MBE (multi-bit error)
#
def test_tof2_tm_pre_pmt_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE PMT mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # pmt0_inject_mbe(26), pmt1_inject_mbe(30)
    mask = hex((1 << 26) | (1 << 30))   # 0x4400'0000
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    pmt_id = 2
    port = 2

    # Write to PMT
    wr_cmd = "pmt-wr -s{0} -d0 -i{1} -p{2}".format(sess_hdl, pmt_id, port)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "pmt-rd -h -d0 -s{0} -e{0}".format(pmt_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name +  " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_pmt_mbe

##
# TM-PRE MIT SBE (single-bit error)
#
def test_tof2_tm_pre_mit_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE MIT sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # mit_inject_sbe(5)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc 0x20")
    LeaveAccessMode(swObj)

    # Parameter values
    grp_id = 0

    # Create objects
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)

    # Write to MIT
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    # First read
    rd_cmd = "mit-rd -h -d0 -s{0} -e{0}".format(grp_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    # Destroy objects
    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof2_tm_pre_mit_sbe

##
# TM-PRE MIT MBE (multi-bit error)
#
def test_tof2_tm_pre_mit_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE MIT mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # mit_inject_mbe(6)
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc 0x40")
    LeaveAccessMode(swObj)

    # Parameter values
    grp_id = 0

    # Create objects
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)

    # Write to MIT
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    # First read
    rd_cmd = "mit-rd -h -d0 -s{0} -e{0}".format(grp_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    # Destroy objects
    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof2_tm_pre_mit_mbe

##
# TM-PRE LIT BM SBE (single-bit error)
#
def test_tof2_tm_pre_lit_bm_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT BM sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # lit0_bm_inject_sbe(9), lit1_bm_inject_sbe(13)
    mask = hex((1 << 9) | (1 << 13))    # 0x2200
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    lag_id = 2
    port = 3

    # Write LIT
    wr_cmd = "lit-wr -d0 -s{0} -i{1} -p{2}".format(sess_hdl, lag_id, port)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "lit-rd -h -d0 -v0 -s{0} -e{0}".format(lag_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_lit_bm_sbe

##
# TM-PRE LIT BM MBE (multi-bit error)
#
def test_tof2_tm_pre_lit_bm_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT BM mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # lit0_bm_inject_mbe(10), lit1_bm_inject_sbe(14)
    mask = hex((1 << 10) | (1 << 14))    # 0x4400
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    lag_id = 3
    port = 3

    # Write LIT
    wr_cmd = "lit-wr -d0 -s{0} -i{1} -p{2}".format(sess_hdl, lag_id, port)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "lit-rd -h -d0 -v0 -s{0} -e{0}".format(lag_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_lit_bm_mbe

##
# TM-PRE LIT NP SBE (single-bit error)
#
def test_tof2_tm_pre_lit_np_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT NP sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # lit0_np_inject_sbe(17), lit1_np_inject_sbe(21)
    mask = hex((1 << 17) | (1 << 21))    # 0x22'0000
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    lag_id = 6

    # Write to LIT
    wr_cmd = "lit-np-wr -d0 -s{0} -i{1} -l2 -r3".format(sess_hdl, lag_id)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "lit-np-rd -d0 -v0 -s{0} -e{0}".format(lag_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_lit_np_sbe

##
# TM-PRE LIT NP MBE (multi-bit error)
#
def test_tof2_tm_pre_lit_np_mbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE LIT NP mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # lit0_np_inject_mbe(18), lit1_np_inject_mbe(22)
    mask = hex((1 << 18) | (1 << 22))    # 0x44'0000
    swObj.send("wr dev_0 device_select tm_top tm_pre_top pre[0] ecc " + mask)
    LeaveAccessMode(swObj)

    # Parameter values
    lag_id = 7

    # Write to LIT
    wr_cmd = "lit-np-wr -d0 -s{0} -i{1} -l2 -r3".format(sess_hdl, lag_id)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "lit-np-rd -d0 -v0 -s{0} -e{0}".format(lag_id)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_tm_pre_lit_np_mbe

##
# TM-PRE RDM SBE (single-bit error)
#
def test_tof2_tm_pre_rdm_sbe(swObj, sess_hdl, write_time):
    test_name = "TM-PRE RDM sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    # Write 64 bits to device_select.tm_top.tm_pre_top.pre[0].ecc
    swObj.send("reg_wro 0 0xc80050 0")
    swObj.send("reg_wro 0 0xc80054 0x2")    # rdm_inject_sbe
    swObj.send("reg_rd 0 0xc80050")

    # Parameter values
    grp_id = 0

    # Create objects
    mgrp_hdl = mgrpCreate(swObj, sess_hdl, grp_id)
    print("Mc Grp Handle is ",mgrp_hdl)
    node_hdl = nodeCreate(swObj, sess_hdl)
    print("Node Handle is ",node_hdl)

    # Write to RDM
    l1Associate(swObj, sess_hdl, mgrp_hdl, node_hdl)

    # First read
    rd_cmd = "l1-read -h -d0 -n{0}".format(node_hdl)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")

    # Destroy objects
    l1Dissociate(swObj, sess_hdl, mgrp_hdl, node_hdl)
    nodeDestroy(swObj, sess_hdl, node_hdl)
    mgrpDestroy(swObj, sess_hdl, mgrp_hdl)
# end test_tof2_tm_pre_rdm_sbe

##
# Deparser PVT SBE (single-bit error)
#
def test_dprsr_pvt_sbe(swObj, sess_hdl, write_time):
    test_name = "Deparser Pipe Vector Tbl sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pardereg dprsrreg dprsrreg inp icr pvt_ecc_ctrl 0x1")
    LeaveAccessMode(swObj)

    # Parameter values
    mgid = 1

    # Write group 1
    wr_cmd = "pvt-wr -d0 -s{0} -g{1} -m6".format(sess_hdl, mgid)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "pvt-rd -h -d0 -s{0} -e{0}".format(mgid)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_dprsr_pvt_sbe

##
# Deparser PVT MBE (multi-bit error)
#
def test_dprsr_pvt_mbe(swObj, sess_hdl, write_time):
    test_name = "Deparser Pipe Vector Tbl mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pardereg dprsrreg dprsrreg inp icr pvt_ecc_ctrl 0x2")
    LeaveAccessMode(swObj)

    # Parameter values
    mgid = 2

    # Write group 2
    wr_cmd = "pvt-wr -d0 -s{0} -g{1} -m6".format(sess_hdl, mgid)
    swObj.send(wr_cmd)

    # First read
    rd_cmd = "pvt-rd -h -d0 -s{0} -e{0}".format(mgid)
    swObj.send(rd_cmd)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " Interrupt not triggered")
        cleanupTest()
    else:
        print(test_name + "Test Passed")

    # Second read
    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send(rd_cmd)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_dprsr_pvt_mbe

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

    swObj.send("./tools/run_switchd.sh -p basic_ipv4 --arch tofino2 -- --ucli")
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
    print("Mc Session Handle is ",mc_sess_hdl)

    test_tof2_tm_pre_pmt_sbe(swObj, mc_sess_hdl, write_time)
    test_tof2_tm_pre_pmt_mbe(swObj, mc_sess_hdl, write_time)

    test_tof2_tm_pre_mit_sbe(swObj, mc_sess_hdl, write_time)
    test_tof2_tm_pre_mit_mbe(swObj, mc_sess_hdl, write_time)

    test_tof2_tm_pre_lit_bm_sbe(swObj, mc_sess_hdl, write_time)
    test_tof2_tm_pre_lit_bm_mbe(swObj, mc_sess_hdl, write_time)

    test_tof2_tm_pre_lit_np_sbe(swObj, mc_sess_hdl, write_time)
    test_tof2_tm_pre_lit_np_mbe(swObj, mc_sess_hdl, write_time)

    test_tof2_tm_pre_rdm_sbe(swObj, mc_sess_hdl, write_time)
    # RDM mbe errors are fatal

    test_dprsr_pvt_sbe(swObj, mc_sess_hdl, write_time)
    test_dprsr_pvt_mbe(swObj, mc_sess_hdl, write_time)

    destroyMcSession(swObj, mc_sess_hdl)
    print("Test Result: PASSED")
# end __main__
