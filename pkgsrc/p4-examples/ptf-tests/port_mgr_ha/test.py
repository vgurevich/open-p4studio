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

"""
Thrift PD interface basic tests
"""
from __future__ import print_function

from collections import OrderedDict

import time
import sys
import logging
import copy
import pdb

import unittest
import random

import pd_base_tests
from pal_rpc.ttypes import *

from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *

import ptf.dataplane as dataplane

import os
import diag_rpc.diag_rpc as diag_rpc

from fast_reconfig.p4_pd_rpc.ttypes import *
from mc_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from devport_mgr_pd_rpc.ttypes import *
from port_mgr_pd_rpc.ttypes import *
from sd_pd_rpc.ttypes import *

try:
    from diag_rpc.ttypes import *
except ImportError:
    pass

from res_pd_rpc.ttypes import *

dev_id = 0
MAX_PORT_COUNT = 456

sess_hdl = 0
def printCorrectiveActions(ca):
    if ca == dev_port_corrective_action.HA_CA_PORT_NONE:
        print("NONE")
    if ca == dev_port_corrective_action.HA_CA_PORT_ADD:
        print("ADD")
    if ca == dev_port_corrective_action.HA_CA_PORT_ENABLE:
        print("ENABLE")
    if ca == dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE:
        print("ADD_THEN_ENABLE")
    if ca == dev_port_corrective_action.HA_CA_PORT_FLAP:
        print("FLAP")
    if ca == dev_port_corrective_action.HA_CA_PORT_DISABLE:
        print("DISABLE")
    if ca == dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD:
        print("DELETE_THEN_ADD")
    if ca == dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE:
        print("DELETE_THEN_ADD_THEN_ENABLE")
    if ca == dev_port_corrective_action.HA_CA_PORT_DELETE:
        print("DELETE")
    if ca == dev_port_corrective_action.HA_CA_PORT_MAX:
        print("MAX")

def verifyPacketsWithIXIA(self, port):
    num_pkt = 10
    pkt_size = 200
    lb_port_list = [port]
    num_ports = len(lb_port_list)
    loop_mode = diag_loop_mode_t.DIAG_PORT_LPBK_EXT
    try:
        print("Setting up loopback test")
        diag_hdl = self.diag.diag_loopback_test_setup(dev_id, lb_port_list, num_ports, loop_mode)
        time.sleep(1)
        print("Sending out test packets")
        time.sleep(1)
        self.diag.diag_loopback_test_start(diag_hdl, num_pkt, pkt_size)
        print("Waiting to Receive all the packets back")
        time.sleep(1)
        test_status = self.diag.diag_loopback_test_status_get(diag_hdl)
    finally:
        print("Cleaning up the loopback test")
        time.sleep(1)
        self.diag.diag_loopback_test_cleanup(diag_hdl)
        return test_status

def runLoopBackTest(self, port):
    #print("LOOPBACK SKIPPED")
    loop_sts = verifyPacketsWithIXIA(self, port)
    if loop_sts != diag_test_status_t.DIAG_TEST_STATUS_PASS:
        print("********* Loopback Test failed for port : " + str(port) + " *********")
        #assert(0)

def verifyCorrectiveActions(self, port, port_ca, serdes_ca):
    app_port_ca = self.devport_mgr.devport_mgr_port_ca_get(0, port);
    app_serdes_ca = self.devport_mgr.devport_mgr_serdes_ca_get(0, port);

    print("Expected MAC/Port Corrective Action for Port :" + str(port) +" : ", end="")
    printCorrectiveActions(port_ca)
    print("Applied MAC/Port Corrective Action for Port  :" + str(port) +" : ", end="")
    printCorrectiveActions(app_port_ca)

    print("Expected Serdes Corrective Action for Port   :" + str(port) +" : ", end="")
    printCorrectiveActions(serdes_ca)
    print("Applied Serdes Corrective Action for Port    :" + str(port) +" : ", end="")
    printCorrectiveActions(app_serdes_ca)

    if port_ca != app_port_ca:
        print("*********** Expected MAC/Port IS NOT EQUAL TO to Applied MAC/Port Corrective Action for Port : " + str(port) + "***********")
        #assert (0)
    if serdes_ca != app_serdes_ca:
        print("*********** Expected Serdes IS NOT EQUAL TO to Applied Serdes Corrective Action for Port : " + str(port) + "***********")
        #assert(0)

class TestPortMgrHAPortAddOnly(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()

        # Apply config and run test
        print("-- Starting Port Add Only Test --")

        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnableWait(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable - Wait Test --")

        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(15)

        print("Run the loopback tests")
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Run the loopback tests")
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnable(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable Test --")

        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Run the loopback tests")
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnableDisable(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable - Disable Test --")

        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Disable the Ports")
        self.pal.pal_port_dis(0, 184)
        self.pal.pal_port_dis(0, 188)
        self.pal.pal_port_dis(0, 180)
        self.pal.pal_port_dis(0, 164)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Disable the Ports")
        self.pal.pal_port_dis(0, 184)
        self.pal.pal_port_dis(0, 188)
        self.pal.pal_port_dis(0, 180)
        self.pal.pal_port_dis(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)

        # Please note that here although NONE should have the corrective action for the MAC for both the ports, the corrective action
        # that is applied is DISABLE. This is because, the determination of the enable/disable status of the port from the hardware
        # is based on the swreset bit. Now this bit is set by the FSM of the port when it is enabled. So during cfg replay, when the 
        # FSM is kick started by the switchd pm, it sets this bit to 0 (i.e. enables the port). The port disable call that is made
        # during the cfg replay does not touch the hardware. Hence in the delta compute phase, we see the port as enabled. And hence
        # the corrective action is DISABLE.

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnableWaitDisable(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable - Wait - Disable --")

        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        print("Run the loopback tests")
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Disable the Ports")
        self.pal.pal_port_dis(0, 184)
        self.pal.pal_port_dis(0, 188)
        self.pal.pal_port_dis(0, 180)
        self.pal.pal_port_dis(0, 164)
        time.sleep(10)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        print("Non Auto-Neg Ports")
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Auto-Neg Ports")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Disable the Ports")
        self.pal.pal_port_dis(0, 184)
        self.pal.pal_port_dis(0, 188)
        self.pal.pal_port_dis(0, 180)
        self.pal.pal_port_dis(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)


        # Please note that here although NONE should have the corrective action for the MAC for both the ports, the corrective action
        # that is applied is DISABLE. This is because, the determination of the enable/disable status of the port from the hardware
        # is based on the swreset bit. Now this bit is set by the FSM of the port when it is enabled. So during cfg replay, when the 
        # FSM is kick started by the switchd pm, it sets this bit to 0 (i.e. enables the port). The port disable call that is made
        # during the cfg replay does not touch the hardware. Hence in the delta compute phase, we see the port as enabled. And hence
        # the corrective action is DISABLE.

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnableWaitDelete(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable - Wait - Delete Test --")

        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)

        print("Delete the Ports")
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        print("Delete the Ports")
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHAPortAddEnableDelete(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Add - Enable - Delete Test --")

        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        print("Delete the Ports")
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 188)
        print("Delete the Ports")
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 188)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHADiffConfig(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        print("Client Session handle is " + str(sess_hdl))
        # Apply config and run test
        print("-- Starting Diff Config Test --")

        print("Add Non Auto-Neg Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 185,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 186,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_40G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 186)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)
        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 185,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 186,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 185)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(30)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 185, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 186, dev_port_corrective_action.HA_CA_PORT_DISABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Run the loopback tests")
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 185)
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up the Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 185)
        self.pal.pal_port_del(0, 186)
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADeleteOneAddAnother(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Delete One Add Another Test --")

        print("Add Non Auto-Neg Ports 185:10G, 188:100G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 185,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 183:10G, 165:25G, 166:50G, 156:50G, 148:40G, 140:10G, 132:1G, 0:40G_NB, 2:40G_NB, 9:25G, 10:50G, 16:100G")
        self.pal.pal_port_add(0, 183,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 165,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 166,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 156,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 148,
                              pal_port_speed_t.BF_SPEED_40G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 140,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 132,
                              pal_port_speed_t.BF_SPEED_1G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 0,
                              pal_port_speed_t.BF_SPEED_40G_NB,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 2,
                              pal_port_speed_t.BF_SPEED_40G_NB,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 9,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 10,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 16,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 185)
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 183)
        self.pal.pal_port_enable(0, 165)
        self.pal.pal_port_enable(0, 166)
        self.pal.pal_port_enable(0, 156)
        self.pal.pal_port_enable(0, 148)
        self.pal.pal_port_enable(0, 140)
        self.pal.pal_port_enable(0, 132)
        self.pal.pal_port_enable(0, 0)
        self.pal.pal_port_enable(0, 2)
        self.pal.pal_port_enable(0, 9)
        self.pal.pal_port_enable(0, 10)
        self.pal.pal_port_enable(0, 16)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)
        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports 184:100G, 189:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G, 157:25G, 158:50G, 148:100G, 140:40G, 132:10G, 0:40G, 17;10G, 8:40G, 19:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 157,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 158,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 148,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 140,
                              pal_port_speed_t.BF_SPEED_40G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 132,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 0,
                              pal_port_speed_t.BF_SPEED_40G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 8,
                              pal_port_speed_t.BF_SPEED_40G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 17,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 19,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        self.pal.pal_port_an_set(0, 158, 2)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        self.pal.pal_port_enable(0, 157)
        self.pal.pal_port_enable(0, 158)
        self.pal.pal_port_enable(0, 148)
        self.pal.pal_port_enable(0, 140)
        self.pal.pal_port_enable(0, 132)
        self.pal.pal_port_enable(0, 0)
        self.pal.pal_port_enable(0, 8)
        self.pal.pal_port_enable(0, 17)
        self.pal.pal_port_enable(0, 19)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 185, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 183, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 165, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 166, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 156, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 2, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 9, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 10, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 16, dev_port_corrective_action.HA_CA_PORT_DELETE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 189, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 157, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 158, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 148, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE) #FIXME :  here the serdes CA should have been NONE
        verifyCorrectiveActions(self, 140, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 132, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 0, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE) #FIXME : here the serdes CA should have been NONE, but the rx inv parameter read from hw is not right, it is read as false instead of true
        verifyCorrectiveActions(self, 8, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 17, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 19, dev_port_corrective_action.HA_CA_PORT_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Run the loopback tests")
        runLoopBackTest(self, 189)
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)
        runLoopBackTest(self, 157)
        runLoopBackTest(self, 158)
        runLoopBackTest(self, 148)
        runLoopBackTest(self, 140)
        runLoopBackTest(self, 132)
        runLoopBackTest(self, 0)
        runLoopBackTest(self, 8)
        runLoopBackTest(self, 17)
        runLoopBackTest(self, 19)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 189)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)
        self.pal.pal_port_del(0, 157)
        self.pal.pal_port_del(0, 158)
        self.pal.pal_port_del(0, 148)
        self.pal.pal_port_del(0, 140)
        self.pal.pal_port_del(0, 132)
        self.pal.pal_port_del(0, 0)
        self.pal.pal_port_del(0, 8)
        self.pal.pal_port_del(0, 17)
        self.pal.pal_port_del(0, 19)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADiffMTU(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff MTU Replayed Test --")

        print("Add Non Auto-Neg Ports 188:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Change the MTU")
        self.port_mgr.port_mgr_mtu_set(0, 188, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 190, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 180, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 164, 8000, 8000)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(20)
        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports 188:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(30)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_NONE)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADiffSdsCfgFlap2(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff Sds Cfg Replayed Immediate Flap Test --")

        print("Add Non Auto-Neg Ports 188:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Apply wrong Sds Cfg")
        self.sd.sd_tx_drv_inv_set(0, 188, 0, True)
        self.sd.sd_rx_afe_inv_set(0, 188, 0, True)
        self.sd.sd_tx_drv_inv_set(0, 190, 0, True)
        self.sd.sd_rx_afe_inv_set(0, 190, 0, False)
        self.sd.sd_tx_drv_inv_set(0, 180, 0, True)
        self.sd.sd_tx_drv_inv_set(0, 180, 1, False)
        self.sd.sd_tx_drv_inv_set(0, 180, 2, False)
        self.sd.sd_tx_drv_inv_set(0, 180, 3, False)
        self.sd.sd_rx_afe_inv_set(0, 180, 0, True)
        self.sd.sd_rx_afe_inv_set(0, 180, 1, True)
        self.sd.sd_rx_afe_inv_set(0, 180, 2, True)
        self.sd.sd_rx_afe_inv_set(0, 180, 3, True)
        self.sd.sd_tx_drv_inv_set(0, 164, 0, False)
        self.sd.sd_rx_afe_inv_set(0, 164, 0, False)
        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports 188:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(10)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADiffSdsCfgFlap3(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff Sds Cfg Replayed Immediate Flap Test --")

        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Apply wrong Sds Cfg")
        self.sd.sd_port_lane_map_set(0, 188, 1, 3, 2, 0, 1, 3, 2, 0)
        self.sd.sd_port_lane_map_set(0, 180, 0, 2, 3, 1, 0, 2, 3, 1)
        self.sd.sd_port_lane_map_set(0, 164, 1, 0, 2, 3, 1, 0, 2, 3)
        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)
        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE, True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(20)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 189, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        ''' For Auto-negging ports, since the serdes lane remappings were screwed up, the MAC/port CA action is going to tbe ENABLE
	becuase when we read the hardware during delta compute, we find that the port was never enabled in the hardware '''
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 189)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 189)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADiffMacAndSdsCfg(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff Sds Cfg Replayed Immediate Flap Test --")

        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_REED_SOLOMON)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_FIRECODE)
        print("Apply wrong Sds Cfg")
        self.sd.sd_port_lane_map_set(0, 188, 1, 3, 2, 0, 1, 3, 2, 0)
        self.sd.sd_port_lane_map_set(0, 180, 0, 2, 3, 1, 0, 2, 3, 1)
        self.sd.sd_port_lane_map_set(0, 164, 1, 0, 2, 3, 1, 0, 2, 3)
        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE,True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Change the MTU")
        self.port_mgr.port_mgr_mtu_set(0, 188, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 190, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 180, 8000, 8000)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(20)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 189, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_FLAP, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 189)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 189)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)

class TestPortMgrHACADiffMacAndSdsCfg2(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff Sds Cfg Replayed Immediate Flap Test --")

        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_FIRECODE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_FIRECODE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_FIRECODE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Apply wrong Sds Cfg")
        self.sd.sd_port_lane_map_set(0, 188, 1, 3, 2, 0, 1, 3, 2, 0)
        self.sd.sd_port_lane_map_set(0, 180, 0, 2, 3, 1, 0, 2, 3, 1)
        self.sd.sd_port_lane_map_set(0, 164, 1, 0, 2, 3, 1, 0, 2, 3)
        print("Change the MTU")
        self.port_mgr.port_mgr_mtu_set(0, 188, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 190, 8000, 8000)
        self.port_mgr.port_mgr_mtu_set(0, 180, 8000, 8000)
        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE, True)

        # Tear down and set up the thrift client 
        print("Re-setting the thrift client")
        pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add the Ports 188:10G, 189:10G, 190:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(20)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 189, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_NONE)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_DELETE_THEN_ADD_THEN_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_ENABLE, dev_port_corrective_action.HA_CA_PORT_FLAP)

        print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 189)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

        print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 189)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)

        self.conn_mgr.client_cleanup(sess_hdl)
'''
class TestPortMgrHACAAutoNegOnOff(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self, ["fast_reconfig"])

    """ Port Mgr HA """
    def runTest(self):
        if test_param_get('target') == "bmv2":
            return
        sess_hdl = self.conn_mgr.client_init()
        # Apply config and run test
        print("-- Starting Port Diff Sds Cfg Replayed Immediate Flap Test --")

        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G, 184:50G, 186:25G, 187:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 186,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 187,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
	print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
	print("Apply wrong AN settings")
	self.pal.pal_port_an_set(0, 188, 1);
	self.pal.pal_port_an_set(0, 189, 1);
	self.pal.pal_port_an_set(0, 190, 1);
	self.pal.pal_port_an_set(0, 184, 1);
	self.pal.pal_port_an_set(0, 186, 1);
	self.pal.pal_port_an_set(0, 187, 1);

	self.pal.pal_port_an_set(0, 180, 2);
	self.pal.pal_port_an_set(0, 164, 2);

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 186)
        self.pal.pal_port_enable(0, 187)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)
        print("Wait for some time for the ports to come up")
        sys.stdout.flush()
        time.sleep(10)

        # Stage 1 lock
        print("Stage 1 lock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_begin(0, dev_init_mode.DEV_WARM_INIT_FAST_RECFG, dev_serdes_upgrade_mode.DEV_SERDES_UPD_NONE, True)

	# Tear down and set up the thrift client 
	print("Re-setting the thrift client")
	pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
	pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        # Replay state
        print("Replay config")
        sys.stdout.flush()
        print("Add Non Auto-Neg Ports 188:10G, 189:10G, 190:10G, 184:50G, 186:25G, 187:10G")
        sys.stdout.flush()
        self.pal.pal_port_add(0, 188,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 189,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 190,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 184,
                              pal_port_speed_t.BF_SPEED_50G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 186,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 187,
                              pal_port_speed_t.BF_SPEED_10G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
	print("Add Auto-Neg Ports 180:100G, 164:25G")
        self.pal.pal_port_add(0, 180,
                              pal_port_speed_t.BF_SPEED_100G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
        self.pal.pal_port_add(0, 164,
                              pal_port_speed_t.BF_SPEED_25G,
                              pal_fec_type_t.BF_FEC_TYP_NONE)
	print("Apply correct AN settings")
	self.pal.pal_port_an_set(0, 188, 2);
	self.pal.pal_port_an_set(0, 189, 2);
	self.pal.pal_port_an_set(0, 190, 2);
	self.pal.pal_port_an_set(0, 184, 2);
	self.pal.pal_port_an_set(0, 186, 2);
	self.pal.pal_port_an_set(0, 187, 2);

	self.pal.pal_port_an_set(0, 180, 1);
	self.pal.pal_port_an_set(0, 164, 1);

        print("Enable the Ports")
        self.pal.pal_port_enable(0, 188)
        self.pal.pal_port_enable(0, 189)
        self.pal.pal_port_enable(0, 190)
        self.pal.pal_port_enable(0, 184)
        self.pal.pal_port_enable(0, 186)
        self.pal.pal_port_enable(0, 187)
        self.pal.pal_port_enable(0, 180)
        self.pal.pal_port_enable(0, 164)

        # Stage 2 unlock
        print("Stage 2 unlock")
        sys.stdout.flush()
        self.devport_mgr.devport_mgr_warm_init_end(0);

        print("Sleep for sometime to allow dma push")
        sys.stdout.flush()
        time.sleep(20)

        print("Verifying if the corrective actions are as expected")
        verifyCorrectiveActions(self, 188, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 189, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 190, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 184, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 186, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 187, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 180, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)
        verifyCorrectiveActions(self, 164, dev_port_corrective_action.HA_CA_PORT_NONE, dev_port_corrective_action.HA_CA_PORT_FLAP)

	print("Run the loopback tests")
        runLoopBackTest(self, 188)
        runLoopBackTest(self, 189)
        runLoopBackTest(self, 190)
        runLoopBackTest(self, 184)
        runLoopBackTest(self, 186)
        runLoopBackTest(self, 187)
        runLoopBackTest(self, 180)
        runLoopBackTest(self, 164)

	print("Cleaning up thep Ports Used by the test")
        sys.stdout.flush()
        self.pal.pal_port_del(0, 188)
        self.pal.pal_port_del(0, 190)
        self.pal.pal_port_del(0, 189)
        self.pal.pal_port_del(0, 184)
        self.pal.pal_port_del(0, 186)
        self.pal.pal_port_del(0, 187)
        self.pal.pal_port_del(0, 180)
        self.pal.pal_port_del(0, 164)


        self.conn_mgr.client_cleanup(sess_hdl)
'''
