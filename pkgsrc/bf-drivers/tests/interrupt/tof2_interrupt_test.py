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

# tof2_interrupt_test.py
#
# Tofino2 MAU memory error interrupt tests.
#
from __future__ import print_function

import argparse
import logging
import os
import re
import sys
import random
import pdb

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
    UCLI_PROMPT = 'bf-sde> '
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

parser = argparse.ArgumentParser(version="tof2_interrupt_test 0.2")

group = parser.add_argument_group("InterruptTest")
group.add_argument("--ip", type=str, required=True, help="ip address of switch")
group.add_argument("--log", type=str, required=True, help="logfilename, ex: test.log")
group.add_argument("--path", type=str, required=True, help="install dir path")

####################
# Utility Routines #
####################

def ucli(swObj, cmd):
    # Send a dummy command and wait for the output to clear out any old output
    # since the last "expect" command.
    """
    swObj.send("dev")
    swObj.expect('Device')
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+\r')
    """
    # Send the command we are interested in
    swObj.send(cmd)
    # Send a dummy command which we'll use to make sure ALL output from the
    # previous command is captured.  Not really sure why this is needed though,
    # just waiting for the uCLI prompt to come back should be enough...
    swObj.send("sku")
    output = swObj.expect('SKU : Tofino2 .*')
    output += swObj.after()
    # Send one more empty line (is this needed???)
    swObj.send("\r\n")
    swObj.expect('bf-sde>\s+\r')
    """
    swObj.send(cmd)
    output = swObj.expect('bf-sde>\s+\r')
    output += swObj.after()
    """
    # Parse the output.
    # First split it into each line of output.
    lines = output.split("\n")
    # Check each line and find where we issued our command, discard all lines up
    # to and including that line.
    cmd_idx = None
    for i,line in enumerate(lines):
        sts = re.search(swObj.UCLI_PROMPT+'.*'+cmd, line)
        if sts is not None:
            cmd_idx = i
    if cmd_idx is None:
        print("Unexpected output running uCLI command \"%s\", output:\n%s" % (cmd, lines))
        raise KeyError
    lines = lines[cmd_idx+1:]
    # Check the remaining lines to find the end of our command's output.  This
    # would be the first line found with the uCLI prompt.
    end_idx = None
    for i,s in enumerate(lines):
        if swObj.UCLI_PROMPT in s:
            end_idx = i
            break
    if end_idx is None:
        print("Unexpected output running uCLI command \"%s\", output:\n%s" % (cmd, lines))
        raise KeyError
    lines = lines[:end_idx]
    return lines

def readInterruptCnt(swObj, output_len=1):
    swObj.send("int_new 0")
    swObj.send("intr_dump -d0 -n %d" % output_len)
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

def reg_rdo(swObj, addr):
    addr_str = "%x" % (addr)
    cmd = "reg_rdo 0 %x" % (addr)
    resp = ucli(swObj, cmd)
    # Expect a single line of output in the following format:
    # 0 : addr : data 
    # The first number is the device-id and the register address and data are
    # eight character hex numbers without a leading 0x.
    fields = resp[0].split(' ')
    val = int(fields[4], 16)
    return val

def get_pipes_enabled(swObj):
    # Get enabled pipes by reading fuse register (bits 131-134 of fuse which are
    # bits 3-6 of func_fuse[4])
    func_fuse_4 = reg_rdo(swObj, 0x80190)
    pipe_dis = (func_fuse_4 >> 3) & 0xF
    pipes = list()
    for i in [0, 1, 2, 3]:
        if 0 == ((1 << i) & pipe_dis):
            pipes.append(i)
    return pipes

def ind_rd(swObj, addr):
    cmd = "ind_rd 0 %x" % (addr)
    resp = ucli(swObj, cmd)
    # Expect a single line of output in the following format:
    # Rd: 0: addr : data data
    # The first number is the device-id and the register address and data are
    # sixteen character hex numbers without a leading 0x.
    try:
        tmp1 = resp[0].split(':')
        tmp2 = tmp1[-1].split()
        hi = int(tmp2[0], 16)
        lo = int(tmp2[1], 16)
    except IndexError:
        print("Unexpected output for ind_rd cmd.  Got \"%s\"", fields)
        raise
    return hi,lo

def ind_wr(swObj, addr, hi, lo):
    cmd = "ind_wr 0 %x %x %x" % (addr, hi, lo)
    ucli(swObj, cmd)

def prsr_mem_ctrl(swObj, pipe, gress, xpb, prsr, en):
    # Address increment to step over pipes, IPBs/EPBs, and parsers.
    pipe_reg_step = 0x1000000
    xpb_reg_step = 0x2000
    prsr_reg_step = 0x400
    # Address of __pipes[0]__pardereg__pgstnreg__ipbprsr4reg[0]__prsr[0]__mem_ctrl
    i_mem_ctrl_base = 0x04c011e8
    # Address of __pipes[0]__pardereg__pgstnreg__epbprsr4reg[0]__prsr[0]__mem_ctrl
    e_mem_ctrl_base = 0x04c211e8

    # Calculate the address of the mem_ctrl register
    offset = pipe * pipe_reg_step + xpb * xpb_reg_step + prsr * prsr_reg_step
    if gress == 0:
        addr = i_mem_ctrl_base + offset
    else:
        addr = e_mem_ctrl_base + offset

    # The value will be the parser id (0-3) when broadcast is turned off and
    # zero when broadcast is turned on.
    if en:
        val = 0
    else:
        val = prsr

    # Write the register to configure the parser's mem_ctrl.  Note that parser 0
    # is special cased because it always has a value of 0 in the mem_ctrl
    # register.  There is no turning broadcast on/off for parser 0 since the
    # write targets parser 0 in both cases.
    if prsr != 0:
        cmd = "reg_wro 0 %x %x" % (addr, val)
        ucli(swObj, cmd)

def cleanupTest():
    print("Test Result: FAILED")
    exit(1)

##############
# Test Cases #
##############

##
# MAU SRAM SBE (single-bit error)
#
def test_tof2_mau_sram_sbe(swObj, write_time):
    print("Test SRAM sbe mem corruption (tbl ipv4_routing_exm_ways_3_pack_5)")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing sram sbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[0] rams array row[7] ram[2] unit_ram_ecc 0x2")
    LeaveAccessMode(swObj)

    # Write to SRAM
    swObj.send("write-unit-ram -d0 -p0 -s0 -r7 -c2 -l0 -v 0x2000")

    # First read
    print("Doing SRAM read")
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after sram sbe test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Sram sbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Sram sbe Test Passed")

    # Second read
    print("Doing SRAM sbe read again")
    sleep(write_time)
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after sram verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Sram sbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Sram sbe verify Test Passed")
# end test_tof2_mau_sram_sbe

##
# MAU SRAM MBE (multi-bit error)
#
def test_tof2_mau_sram_mbe(swObj, write_time):
    print("Test SRAM mbe mem corruption (tbl ipv4_routing_exm_ways_3_pack_5)")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing sram mbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[0] rams array row[7] ram[2] unit_ram_ecc 0x4")
    LeaveAccessMode(swObj)

    # Write to SRAM
    swObj.send("write-unit-ram -d0 -p0 -s0 -r7 -c2 -l0 -v 0x2000")

    # First read
    print("Doing SRAM read")
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after sram mbe test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Sram mbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Sram mbe Test Passed")

    # Second read
    print("Doing SRAM mbe read again")
    sleep(write_time)
    swObj.send("read-unit-ram -d0 -p0 -s0 -r7 -c2 -l0")

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after sram mbe verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Sram mbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Sram mbe verify Test Passed")
# end test_tof2_mau_sram_mbe

##
# MAU TCAM SBE (single-bit error)
#
def test_tof2_mau_tcam_sbe(swObj, write_time):
    print("Test TCAM sbe mem corruption (tbl udp_add_tbl_stage_3)")
    print("Will use a tcam memory unit that has been assigned to a table")

    # Get default scrub time
    tcam_scrub_time_def = tcam_scrub_get(swObj)
    print("Default Tcam scrub timer value in msec: ",tcam_scrub_time_def)

    # Set to lower scrub time for test
    tcam_scrub_time_new = 15
    tcam_scrub_buffer = 5
    tcam_scrub_set(swObj, tcam_scrub_time_new*1000)

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing tcam sbe ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[1] mau[3] tcams tcam_parity_control[1] 0x2")
    LeaveAccessMode(swObj)

    # Write TCAM
    swObj.send("write-tcam -d0 -p1 -s3 -r1 -c0 -l0 -y1 -t0 -k '11 00 00 00 00 00' -m '00 00 11 00 11 11'")

    # Wait for scrub
    print("Wait some time for tcam read to kick in")
    sleep(tcam_scrub_time_new + tcam_scrub_buffer)

    # Check for interrupt
    intr_cnt_after = readInterruptCnt(swObj)
    print("Interrupt count after tcam test: ",intr_cnt_after)
    if (intr_cnt_after <= intr_cnt_before):
        print("Tcam sbe Interrupt not triggered")
        cleanupTest()
    else:
        print("Tcam sbe Test Passed")

    # Wait for scrub
    print("Doing Tcam read again")
    print("Wait for tcam read to kick in")
    sleep(tcam_scrub_time_new + tcam_scrub_buffer)

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after tcam verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print("Tcam sbe Interrupt triggered again after repair")
        cleanupTest()
    else:
        print("Tcam sbe Verify Test Passed")

    # Reset scrub timer
    print("Setting tcam scrub timer back to default value")
    tcam_scrub_set(swObj, tcam_scrub_time_def)
# end test_tof2_mau_tcam_sbe

##
# MAU Map RAM SBE (single-bit error)
#
def test_tof2_mau_map_ram_sbe(swObj, write_time):
    test_name = "Map Ram sbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[9] rams map_alu row[7] adrmux mapram_sbe_inj 0x1")
    LeaveAccessMode(swObj)

    # Write map RAM
    swObj.send("write-map-ram -d0 -p0 -s9 -r7 -c0 -l0 -v 0x23")

    # First read
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

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
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_mau_map_ram_sbe

##
# MAU Map RAM MBE (multi-bit error)
#
def test_tof2_mau_map_ram_mbe(swObj, write_time):
    test_name = "Map Ram mbe"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ",intr_cnt_before)

    # Error injection
    print("Writing " + test_name + " ecc inject register")
    EnterAccessMode(swObj)
    swObj.send("wr dev_0 pipes[0] mau[9] rams map_alu row[7] adrmux mapram_mbe_inj 0x1")
    LeaveAccessMode(swObj)

    # Write to map ram
    swObj.send("write-map-ram -d0 -p0 -s9 -r7 -c0 -l0 -v 0x23")

    # First read
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

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
    swObj.send("read-map-ram -d0 -p0 -s9 -r7 -c0 -l0")

    # Check for no interrupt
    intr_cnt_after_verify = readInterruptCnt(swObj)
    print("Interrupt count after " + test_name + " verify test: ",intr_cnt_after_verify)
    if (intr_cnt_after_verify > intr_cnt_after):
        print(test_name + " Interrupt triggered again after repair")
        cleanupTest()
    else:
        print(test_name + " verify Test Passed")
# end test_tof2_mau_map_ram_mbe

##
# Parser TCAM Parity
#
def test_tof2_prsr_tcam_parity(swObj, write_time):
    test_name = "Parser TCAM Parity"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ", intr_cnt_before)

    pipes = get_pipes_enabled(swObj)
    print("Checking pipes", pipes)

    pipe_step = 0x8000000000
    prsr_step = 0x800

    # Get default scrub time
    tcam_scrub_time_def = tcam_scrub_get(swObj)

    # Test each pipe.
    for pipe in pipes:
        for gress in [0, 1]:
            if gress == 0:
                prsr_tcam_base = 0x26080000400
                xpb_name = "ipb"
            else:
                prsr_tcam_base = 0x26080020400
                xpb_name = "epb"
            # There are 9 IPBs/EPBs per pipe and each has four parsers for a
            # total of 36 ingress parser blocks and 36 egress parser blocks.
            # Pick a random subset of the 36 to test.
            all_parsers = list(range(36))
            to_test = random.sample(all_parsers, 18)
            for prsr_id in to_test:
                prsr = prsr_id & 3
                xpb_id = prsr_id >> 2
                print("Checking pipe %d %s %d prsr %d" % (pipe, xpb_name, xpb_id, prsr))

                # Turn off TCAM scrubbing
                tcam_scrub_set(swObj, 0)

                # Pick one TCAM entry and read it's value.
                tcam_entry = random.randint(0, 255)
                tcam_addr = prsr_tcam_base + pipe * pipe_step + prsr_id * prsr_step + tcam_entry
                orig_hi, orig_lo = ind_rd(swObj, tcam_addr)
                print("Checking entry %d, address 0x%x" % (tcam_entry, tcam_addr))

                # Build up an access command string for this parser
                cmd_base = "wr dev_0 pipes[%d] pardereg pgstnreg %sprsr4reg[%d] prsr[%d] " % (pipe, xpb_name, xpb_id, prsr)

                # Turn off write-broadcasts for this parser so we can directly write
                # its memory to inject the error.
                prsr_mem_ctrl(swObj, pipe, gress, xpb_id, prsr, False)

                # Set the parity error inject
                EnterAccessMode(swObj)
                cmd = cmd_base + "parity 2"
                print("Sending: " + cmd)
                swObj.send(cmd)
                LeaveAccessMode(swObj)

                # Write the TCAM entry to corrupt it.
                ind_wr(swObj, tcam_addr, 1, 2)

                # Turn broadcast writes back on since that is normal operation.
                prsr_mem_ctrl(swObj, pipe, gress, xpb_id, prsr, True)

                # Turn the scrubber back on and wait for it to run
                tcam_scrub_set(swObj, 250)
                sleep(1)
                tcam_scrub_set(swObj, 0)

                # Expect one more interrupt.
                intr_cnt_now = readInterruptCnt(swObj)
                intr_cnt_expected = intr_cnt_before + 1
                if intr_cnt_now != intr_cnt_expected:
                    if intr_cnt_now == intr_cnt_before:
                        print("TCAM parity error not triggered")
                    else:
                        readInterruptCnt(swObj, output_len=-1)
                        print("Current interrupt count (%d) does not match expected count (%d)" % (intr_cnt_now, intr_cnt_expected))
                    print("FAIL pipe %d %s %d prsr %d" % (pipe, xpb_name, xpb_id, prsr))
                    tcam_scrub_set(swObj, tcam_scrub_time_def)
                    cleanupTest()
                intr_cnt_before = intr_cnt_expected

                # Check that the data was restored properly
                hi, lo = ind_rd(swObj, tcam_addr)
                if hi != orig_hi or lo != orig_lo:
                    print("TCAM parity error not corrected properly")
                    print("FAIL pipe %d %s %d prsr %d" % (pipe, xpb_name, xpb_id, prsr))
                    tcam_scrub_set(swObj, tcam_scrub_time_def)
                    cleanupTest()
    tcam_scrub_set(swObj, tcam_scrub_time_def)
# end test_tof2_prsr_tcam_parity

##
# Parser Action RAM ECC
#
def test_tof2_prsr_aram_ecc(swObj, write_time):
    test_name = "Parser Action RAM ECC"
    print("Test " + test_name + " mem corruption")

    # Get initial interrupt count
    intr_cnt_before = readInterruptCnt(swObj)
    print("Interrupt count before test: ", intr_cnt_before)

    pipes = get_pipes_enabled(swObj)
    print("Checking pipes", pipes)

    pipe_step = 0x8000000000
    prsr_step = 0x800

    # Test each pipe.
    for pipe in pipes:
        for gress in [0, 1]: # 0: ingress, 1: egress
            if gress == 0:
                prsr_aram_base = 0x26080000000
                xpb_name = "ipb"
            else:
                prsr_aram_base = 0x26080020000
                xpb_name = "epb"
            # There are 9 IPBs/EPBs per pipe and each has four parsers for a
            # total of 36 ingress parser blocks and 36 egress parser blocks.
            # Pick a random subset of the 36 to test.
            all_parsers = list(range(36))
            to_test = random.sample(all_parsers, 18)
            for prsr_id in to_test:
                prsr = prsr_id & 3
                xpb_id = prsr_id >> 2

                # Pick one ARAM entry and read it's value, note that it is made
                # up of four 16 byte entries.
                entry = random.randint(0, 255)
                addr = prsr_aram_base + pipe * pipe_step + prsr_id * prsr_step + 4 * entry
                print("Checking pipe %d %s %d prsr %d entry %d address 0x%x" % (pipe, xpb_name, xpb_id, prsr, entry, addr))
                orig = [ind_rd(swObj, addr + i) for i in [0,1,2,3]]

                # Turn off write-broadcasts for this parser so we can directly write
                # its memory to inject the error.
                prsr_mem_ctrl(swObj, pipe, gress, xpb_id, prsr, False)

                # Set the ECC error inject
                EnterAccessMode(swObj)
                cmd_base = "wr dev_0 pipes[%d] pardereg pgstnreg %sprsr4reg[%d] prsr[%d] " % (pipe, xpb_name, xpb_id, prsr)
                cmd = cmd_base + "ecc 4" # Bit 2 is MBE
                print("Sending: " + cmd)
                swObj.send(cmd)
                LeaveAccessMode(swObj)

                # Write the TCAM entry to corrupt it.
                for i in [0, 1, 2, 3]:
                    ind_wr(swObj, addr+i, 1, 2)

                # Turn broadcast writes back on since that is normal operation.
                prsr_mem_ctrl(swObj, pipe, gress, xpb_id, prsr, True)

                # Read the memory to trigger the ECC error.
                for i in [0, 1, 2, 3]:
                    ind_rd(swObj, addr + i)

                # Expect one more interrupt.
                intr_cnt_now = readInterruptCnt(swObj)
                intr_cnt_expected = intr_cnt_before + 1
                if intr_cnt_now != intr_cnt_expected:
                    if intr_cnt_now == intr_cnt_before:
                        print("Parser ARAM ECC error not triggered")
                    else:
                        readInterruptCnt(swObj, output_len=-1)
                        print("Current interrupt count (%d) does not match expected count (%d)" % (intr_cnt_now, intr_cnt_expected))
                    print("FAIL pipe %d %s %d prsr %d" % (pipe, xpb_name, xpb_id, prsr))
                    cleanupTest()
                intr_cnt_before = intr_cnt_expected

                # Check that the data was restored properly
                corrected = [ind_rd(swObj, addr + i) for i in [0,1,2,3]]
                if corrected != orig:
                    print("Parser ARAM ECC error not corrected properly")
                    print("FAIL pipe %d %s %d prsr %d" % (pipe, xpb_name, xpb_id, prsr))
                    print("Expected: %s", orig)
                    print("Got: %s", corrected)
                    cleanupTest()
# end test_tof2_prsr_aram_ecc

################
# Main Program #
################

if __name__ == "__main__":

    args = parser.parse_args()

    ipaddr = args.ip
    path = args.path
    username = "admin12"
    password = "bfn123"
    echo = False
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
    sleep(2)

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

    print("Print Sram and Tcam Memory units in use")
    swObj.send("memid-hdl -d0")

    test_tof2_mau_sram_sbe(swObj, write_time)
    test_tof2_mau_sram_mbe(swObj, write_time)

    test_tof2_mau_tcam_sbe(swObj, write_time)

    test_tof2_mau_map_ram_sbe(swObj, write_time)
    test_tof2_mau_map_ram_mbe(swObj, write_time)

    test_tof2_prsr_tcam_parity(swObj, write_time)
    test_tof2_prsr_aram_ecc(swObj, write_time)

    print("Test Result: PASSED")
# end __main__
