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

# interrupt_test.py
#
# Tofino1 MAU memory error interrupt tests.
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

parser = argparse.ArgumentParser(version="interrupt_test 0.2")

group = parser.add_argument_group("InterruptTest")
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

def tcam_scrub_set(swObj, scrub_val):
    swObj.send("tcam-scrub-set -d0 -t " + str(scrub_val))
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')

def tcam_scrub_get(swObj):
    swObj.send("tcam-scrub-get -d0")
    output=swObj.expect('Tcam scrub timer value in msec:\s+\S+')
    output+=swObj.after()
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+')
    status=re.search('Tcam scrub timer value in msec:\s+(\d+)',output)
    scrub_val = int(status.group(1))
    return scrub_val

def cleanupTest():
    print("Test Result: FAILED")
    exit(1)

##############
# Test Cases #
##############

##
# MAU SRAM SBE (single-bit error)
#
def test_tof1_mau_sram_sbe(swObj, write_time):
    print("Test SRAM sbe mem corruption (tbl ipv4_routing_exm_ways_3_pack_5)")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing sram sbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[0] rams array row[7] ram[2] unit_ram_ecc 0x2")
    LeaveAccessMode(swObj)

    swObj.send("write-unit-ram -d0 -p0 -s0 -r7 -c2 -l0 -v 0x2000")

    print("Doing SRAM read")
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after sram sbe test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Sram sbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Sram sbe Test Passed")

    print("Doing SRAM sbe read again")
    sleep(write_time)
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after sram verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Sram sbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Sram sbe verify Test Passed")
# end test_tof1_mau_sram_sbe

##
# MAU SRAM MBE (multi-bit error)
#
def test_tof1_mau_sram_mbe(swObj, write_time):
    print("Test SRAM mbe mem corruption (tbl ipv4_routing_exm_ways_3_pack_5)")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing sram mbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[0] rams array row[7] ram[2] unit_ram_ecc 0x4")
    LeaveAccessMode(swObj)

    swObj.send("write-unit-ram -d0 -p0 -s0 -r7 -c2 -l0 -v 0x2000")

    print("Doing SRAM read")
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after sram mbe test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Sram mbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Sram mbe Test Passed")

    print("Doing SRAM mbe read again")
    sleep(write_time)
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after sram mbe verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Sram mbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Sram mbe verify Test Passed")
# end test_tof1_mau_sram_mbe

##
# MAU TCAM SBE (single-bit error)
#
def test_tof1_mau_tcam_sbe(swObj, write_time):
    print("Test TCAM sbe mem corruption (tbl udp_add_tbl_stage_3)")
    print("Will use a tcam memory unit that has been assigned to a table")

    # Get default scrub time
    tcam_scrub_time_def = tcam_scrub_get(swObj)
    print("Default Tcam scrub timer value in msec: ",tcam_scrub_time_def)

    # Set to lower scrub time for test
    tcam_scrub_time_new = 15
    tcam_scrub_buffer = 5
    tcam_scrub_set(swObj, tcam_scrub_time_new*1000)

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing tcam sbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[1] mau[3] tcams tcam_parity_control[1] 0x2")
    LeaveAccessMode(swObj)

    swObj.send("write-tcam -d0 -p1 -s3 -r1 -c0 -l0 -y1 -t0 -k '11 00 00 00 00 00' -m '00 00 11 00 11 11'")

    print("Wait some time for tcam read to kick in")
    sleep(tcam_scrub_time_new + tcam_scrub_buffer)

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after tcam test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Tcam sbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Tcam sbe Test Passed")

    print("Doing Tcam read again")
    print("Wait for tcam read to kick in")
    sleep(tcam_scrub_time_new + tcam_scrub_buffer)

    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after tcam verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Tcam sbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Tcam sbe Verify Test Passed")

    print("Setting tcam scrub timer back to default value")
    tcam_scrub_set(swObj, tcam_scrub_time_def)
# end test_tof1_mau_tcam_sbe

##
# MAU Map RAM SBE (single-bit error)
#
def test_tof1_mau_map_ram_sbe(swObj, write_time):
    test_name = "Map Ram sbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[9] rams map_alu row[7] adrmux mapram_sbe_inj 0x1")
    LeaveAccessMode(swObj)

    swObj.send("write-map-ram -d0 -p0 -s9 -r7 -c0 -l0 -v 0x23")

    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_mau_map_ram_sbe

##
# MAU Map RAM MBE (multi-bit error)
#
def test_tof1_mau_map_ram_mbe(swObj, write_time):
    test_name = "Map Ram mbe"
    print("Test " + test_name + " mem corruption")

    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[9] rams map_alu row[7] adrmux mapram_mbe_inj 0x1")
    LeaveAccessMode(swObj)

    swObj.send("write-map-ram -d0 -p0 -s9 -r7 -c0 -l0 -v 0x23")

    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print(test_name + " not triggered")
        cleanupTest()
    else:
        print(test_name + " Test Passed")

    print("Doing "  + test_name + " read again")
    sleep(write_time)
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof1_mau_map_ram_mbe

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

    print("Tcam/Sram Interrupt Test script")

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

    print("Print Sram and Tcam Memory units in use")
    swObj.send("memid-hdl -d0")

    test_tof1_mau_sram_sbe(swObj, write_time)
    test_tof1_mau_sram_mbe(swObj, write_time)

    test_tof1_mau_tcam_sbe(swObj, write_time)

    test_tof1_mau_map_ram_sbe(swObj, write_time)
    test_tof1_mau_map_ram_mbe(swObj, write_time)

    print("Test Result: PASSED")
# end __main__
