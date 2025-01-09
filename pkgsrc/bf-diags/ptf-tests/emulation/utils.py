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



import re
import time
import unittest

from ptf.testutils import *

from pal_rpc.ttypes import *


def get_swtype(client):
    board_type = client.pltfm_pm.pltfm_pm_board_type_get()
    swtype = ""
    if re.search("0x0234|0x1234|0x4234|0x5234", hex(board_type)):
        swtype = "mavericks"
    elif re.search("0x2234|0x3234", hex(board_type)):
        swtype = "montara"
    return swtype


def portAdd(client, dev, ports, speed, fec_type, an_mode="", statusCheck=1):
    for i in ports:
        client.pal.pal_port_add(dev, int(i), speed, fec_type)
        if an_mode != "":
            client.pal.pal_port_an_set(dev, int(i), an_mode)
        client.pal.pal_port_enable(dev, int(i))

    if statusCheck:
        checkStatus(client, dev, ports)


def portAddAll(client, dev, speed, fec_type, an_mode=""):
    client.pal.pal_port_add_all(dev, speed, fec_type)
    if an_mode != "":
        client.pal.pal_port_an_set_all(dev, an_mode)
    client.pal.pal_port_enable_all(dev)


def portDel(client, dev, ports):
    for i in ports:
        client.pal.pal_port_del(dev, int(i))


def portDelAll(client, dev):
    client.pal.pal_port_del_all(dev)


def portOperStatus(client, dev, port):
    status = 0
    try:
        status = client.pal.pal_port_oper_status_get(dev, int(port))
    except InvalidPalOperation:
        pass
    return status


def checkStatus(client, dev, ports):
    portsdown = ""

    loopCntr = 10
    for i in range(loopCntr):
        portsdown = ""
        time.sleep(15)
        for j in ports:
            status = portOperStatus(client, dev, j)
            print("Loop %s: Oper status for port %s is %s" % (i, j, status))
            if status == 0:
                portsdown += str(j) + " "
        if portsdown == "":
            break

    assert portsdown == "", "Ports did not come up"
    return portsdown
