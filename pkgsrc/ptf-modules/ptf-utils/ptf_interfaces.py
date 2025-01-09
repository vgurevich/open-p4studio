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


import sys
import os
import json
import subprocess
import sys
import six
cur_dir = os.path.dirname(os.path.realpath(__file__))

def check_and_execute_program(command):
    program = command[0]
    for path in os.environ["PATH"].split(os.pathsep):
        path = path.strip('"')
        exe_file = os.path.join(path, program)
        if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
            return subprocess.call(command)
    six.print_("Cannot find %s in PATH(%s)" % (program,
                                                         os.environ["PATH"]), file=sys.stderr)
    sys.exit(1)

def port_cfg_to_str(config):
    port_cfg_str = ""
    for k in config:
        port_cfg_str += ";" + str(k) + "=" + str(config[k])
    return port_cfg_str

def interface_list(args):
    intf_list = []
    port_list = {}
    addr_list = {}
    device=0

    cleanscript = 'port_mapping_clean'
    check_and_execute_program([cleanscript])

    if "port_info" in args and os.path.isfile(args.port_info):
        connNode = 'PortConn'
        mapNode ='PortToVeth'
        ifMapNode ='PortToIf'
        nmNode = 'PortToNM'
        port_connections = {}
        json_file = open(args.port_info)
        json_data = json.load(json_file)

        noOfPortToVeth = 0
        noOfIfMap = 0
        noOfNM = 0
        noOfConn = 0

        if (json_data.get(nmNode)):
            noOfNM = len(json_data[nmNode])
        else:
            if (json_data.get(connNode)):
                noOfConn = len(json_data[connNode])
            if (json_data.get(mapNode)):
                noOfPortToVeth = len(json_data[mapNode])
            if (json_data.get(ifMapNode)):
                noOfIfMap = len(json_data[ifMapNode])

        for count in range(0,noOfPortToVeth):
            port_cfg = json_data[mapNode][count]
            port = port_cfg['device_port']
            veth1 = port_cfg['veth1']
            veth2 = port_cfg['veth2']
            port_list[port] = "veth%d" % veth2
            six.print_(port_list[port])
            port_list[port] += port_cfg_to_str(port_cfg)

        for count in range(0,noOfIfMap):
            port_cfg = json_data[ifMapNode][count]
            port = port_cfg['device_port']
            ifname = port_cfg['if']
            port_list[port] = ifname
            six.print_(port_list[port])
            port_list[port] += port_cfg_to_str(port_cfg)

        for count in range(0, noOfNM):
            port_cfg = json_data[nmNode][count]
            ports = port_cfg['device_ports']
            NMaddr = port_cfg['addr']
            if NMaddr in addr_list:
                addr_list[NMaddr] += str(ports)
            else:
                addr_list[NMaddr] = str(ports)
            six.print_("Nanomsg Socket at ", NMaddr)

        for count in range(0, noOfConn):
            port1 = json_data[connNode][count]['device_port1']
            port2 = json_data[connNode][count]['device_port2']
            port_connections[port1] = port2
            port_connections[port2] = port1
            veth_pair1 = port_list[port1]
            veth_pair2 = port_list[port2]
            setupscript = 'port_mapping_setup'
            check_and_execute_program([setupscript, str(port1), str(port2),
                                       veth_pair1, veth_pair2])
        if noOfNM != 0:
            for addr in addr_list:
                intf_str = "%d-%s@%s" % (device, addr_list[addr], addr)
                if intf_str not in intf_list:
                    intf_list += ["--device-socket", intf_str]
        else:
            for port in port_list:
                intf_str = "%d-%d@%s" % (device, port, port_list[port])
                if intf_str not in intf_list:
                    intf_list += ["--interface", intf_str]
    else:
        n_chips = 1
        # Most arch start with port 8
        base_port = 8
        max_ports = int(args.max_ports)
        if args.arch.lower() in ["tofino"]:
            base_port = 0
        elif args.arch.lower() == "tofino3":
            # Get the max_ports per die
            max_ports = int(max_ports / 2)
            if int(args.num_pipes) > 4:
                n_chips = 2
        for chip_id in range(n_chips):
            chip_base_port = chip_id*512 + base_port
            for n_veth in range(max_ports):
                if args.arch.lower() == "tofino3":
                    port = n_veth*2
                else:
                    port = n_veth
                intf_list += ["--interface", "%d-%d@veth%d" % (device,
                        port+chip_base_port, 2 * (n_veth + (chip_id* max_ports)) + 1)]

    if args.cpu_port != "None" and args.cpu_veth != "None":
        intf_str = "%d-%s@veth%s" % (device, args.cpu_port, args.cpu_veth)
        if intf_str not in intf_list:
            intf_list += ["--interface", intf_str]

    return intf_list
