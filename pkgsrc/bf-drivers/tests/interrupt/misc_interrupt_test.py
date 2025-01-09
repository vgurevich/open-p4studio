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

# misc_interrupt_test.py
#
# Tofino1 miscellaneous memory interrupt tests.
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

parser = argparse.ArgumentParser(version="misc_interrupt_test 0.2")

group = parser.add_argument_group("MiscInterruptTest")
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

def cleanupTest():
    print("Test Result: FAILED")
    exit(1)

##############
# Test Cases #
##############

##
# Packet Generator SBE/MBE
#
def test_tof1_pktgen_sbe_mbe(swObj, write_time):
    test_name = "Packet-gen sbe/mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # Set both sbe and mbe for interrupt
    swObj.send("wr dev_0 pipes[0] pmarb pgr_reg pgr_common int_inj 0x300")
    LeaveAccessMode(swObj)

    swObj.send("ind_wr 0 0x21c8003c000 0x33 0x44")
    swObj.send("ind_rd 0 0x21c8003c000")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("ind_rd 0 0x21c8003c000")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_pktgen_sbe_mbe

##
# Parser Action RAM SBE
#
def test_tof1_prsr_act_ram_sbe(swObj, write_time):
    test_name = "Parser Action ram sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pmarb ibp18_reg ibp_reg[0] prsr_reg ecc 0x2")
    LeaveAccessMode(swObj)

    swObj.send("ind_wr 0 0x21c80000568 0x12 0x3456")
    swObj.send("ind_rd 0 0x21c80000568")

    # Inject the fake interrupt as interrupt does not fire with CPU reads (only pkts)
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pmarb ibp18_reg ibp_reg[0] prsr_reg intr inject 0x200")
    LeaveAccessMode(swObj)

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("ind_rd 0 0x21c80000568")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_prsr_act_ram_sbe

##
# Parser Action RAM MBE
#
def test_tof1_prsr_act_ram_mbe(swObj, write_time):
    test_name = "Parser Action ram mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pmarb ibp18_reg ibp_reg[0] prsr_reg ecc 0x4")
    LeaveAccessMode(swObj)

    swObj.send("ind_wr 0 0x21c80000568 0x67 0x89ab")
    swObj.send("ind_rd 0 0x21c80000568")

    # Inject the fake interrupt as interrupt does not fire with CPU reads (only pkts)
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] pmarb ibp18_reg ibp_reg[0] prsr_reg intr inject 0x400")
    LeaveAccessMode(swObj)

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("ind_rd 0 0x21c80000568")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_prsr_act_ram_mbe

##
# Galois Field Matrix parity
#
def test_tof1_gfm_parity(swObj, write_time):
    test_name = "Galois Field Matrix (GFM) parity"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    # Inject the fake interrupt as there is no way to corrupt the data
    swObj.send("wr dev_0 pipes[0] mau[0] dp intr_inject_mau_gfm_hash 0x2")
    #swObj.send("wr dev_0 pipes[0] mau[0] dp xbar_hash hash galois_field_matrix[0][0] 0x12")
    #swObj.send("rr dev_0 pipes[0] mau[0] dp xbar_hash hash galois_field_matrix[0][0]")
    LeaveAccessMode(swObj)

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    #swObj.send("rr dev_0 pipes[0] mau[0] dp xbar_hash hash galois_field_matrix[0][0]")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_gfm_parity

##
# Mirror SBE/MBE
#
# Mirroring block inject does not work due to an asic issue.
#
def test_tof1_mirror_sbe_mbe(swObj, write_time):
    test_name = "Mirror sbe/mbe"
    print("Test " + test_name + " mem corruption (does not work due to asic issue)")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] deparser mirror mir_buf_regs mir_glb_group mir_int_dual_inj 0x1f0")
    swObj.send("wr dev_0 pipes[0] deparser mirror mir_buf_desc norm_desc_grp[1023] session_ctrl 0xfff44")
    swObj.send("rr dev_0 pipes[0] deparser mirror mir_buf_desc norm_desc_grp[1023] session_ctrl")
    LeaveAccessMode(swObj)

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        # Ignore as asic has an issue
        #cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("rr dev_0 pipes[0] deparser mirror mir_buf_desc norm_desc_grp[1023] session_ctrl")
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        # Ignore as asic has an issue
        #cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_mirror_sbe_mbe

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

    test_tof1_pktgen_sbe_mbe(swObj, write_time)

    test_tof1_prsr_act_ram_sbe(swObj, write_time)
    test_tof1_prsr_act_ram_mbe(swObj, write_time)

    test_tof1_gfm_parity(swObj, write_time)

    test_tof1_mirror_sbe_mbe(swObj, write_time)

    print("Test Result: PASSED")
# end __main__
