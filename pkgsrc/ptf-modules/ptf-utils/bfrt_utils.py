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

from ptf import config
import ptf.testutils as testutils
from misc_utils import *

import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc

g_arch = testutils.test_param_get("arch").lower()

base_pick_path = testutils.test_param_get("base_pick_path")
if not base_pick_path:
    base_pick_path = "install/share/" + g_arch + "pd/"
base_put_path = testutils.test_param_get("base_put_path")
if not base_put_path:
    base_put_path = "install/share/" + g_arch + "pd/forwarding"


class DevConfiguration:
    def __init__(self, bfrt_info: gc._BfRtInfo) -> None:
        self.p4_name = bfrt_info.p4_name_get()
        target = gc.Target(device_id=0, pipe_id=0xffff)
        dev_cfg_table = bfrt_info.table_get("device_configuration")
        for data, _ in dev_cfg_table.default_entry_get(target, p4_name=self.p4_name):
            data_dict = data.to_dict()
            self.__dict__.update(data_dict)
        self.update_tofino_family(bfrt_info)
        self.validate_min_fields()

    def __str__(self) -> str:
        return f'''Family: {self.family}
sku: {self.sku}
num_pipes: {self.num_pipes}
num_stages: {self.num_stages}
num_max_ports: {self.num_max_ports}
external_port_list: {self.external_port_list}
internal_port_list: {self.internal_port_list}
pcie_cpu_port: {self.pcie_cpu_port}
eth_cpu_port_list: {self.eth_cpu_port_list}'''

    def update_tofino_family(self, bfrt_info: gc._BfRtInfo) -> None:
        self.family = [table for table in bfrt_info.table_list_sorted
                       if "device_configuration" in table][0].split(".")[0]

    def validate_min_fields(self) -> None:
        # Check if basic info is read from table
        min_req_keys = ["p4_name", "family", "sku", "num_pipes",
                        "num_stages",
                        "num_max_ports", "recirc_port_list",
                        "pcie_cpu_port", "eth_cpu_port_list",
                        "external_port_list", "internal_port_list"]
        keys_diff = min_req_keys - self.__dict__.keys()
        if len(keys_diff):
            logger.warn(f"Unable to get {keys_diff} in dev_configuration init")


def create_path_bf_rt(base_path, p4_name_to_use):
    return base_path + "/" + p4_name_to_use + "/bf-rt.json"


def create_path_context(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/context.json"


def create_path_tofino(base_path, p4_name_to_use, profile_name):
    return base_path + "/" + p4_name_to_use + "/" + profile_name + "/" + g_arch +".bin"


def start_warm_init(test, p4_name, number_of_pipes, hitless=True, profile_name='pipe', quiet=False):
    if not quiet:
        logger.info("Start warm init %s", "hitless" if hitless is True else "fast_reconfig")

    swports = {}
    port_table = None
    if testutils.test_param_get("target") == "hw":
        if not quiet: logger.info("Store ports configuration on hw run")
        try:
            bfrt_info = test.bfrt_info
        except:
            bfrt_info = test.interface.bfrt_info_get()
        port_table = bfrt_info.table_get("$PORT")
        target = gc.Target(device_id=0)
        resp = port_table.entry_get(target, [])
        for data, key in resp:
            data_dict = data.to_dict()
            key_dict = key.to_dict()
            if data_dict['$IS_INTERNAL']:   # Skip internal ports
                continue
            params = {'$SPEED': data_dict['$SPEED'],
                      '$FEC': data_dict['$FEC'],
                      '$LOOPBACK_MODE': data_dict['$LOOPBACK_MODE'],
                      '$AUTO_NEGOTIATION': data_dict['$AUTO_NEGOTIATION'],
                      '$PORT_ENABLE': data_dict['$PORT_ENABLE'],
                      '$PORT_UP': data_dict['$PORT_UP'],
                      }
            swports.update({key_dict['$DEV_PORT']['value']: params})
    test.warminit_swports = swports

    profile = gc.ProfileInfo(profile_name,
                             create_path_context(base_pick_path, p4_name, profile_name),
                             create_path_tofino(base_pick_path, p4_name, profile_name),
                             list(range(number_of_pipes)))
    fwd_config = gc.ForwardingConfig(p4_name,
                                     create_path_bf_rt(base_pick_path, p4_name),
                                     [profile])
    if hitless is True:
        mode = bfruntime_pb2.SetForwardingPipelineConfigRequest.HITLESS
    else:
        mode = bfruntime_pb2.SetForwardingPipelineConfigRequest.FAST_RECONFIG
    success = test.interface.send_set_forwarding_pipeline_config_request(
                bfruntime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_WARM_INIT_BEGIN,
                base_put_path,
                [fwd_config],
                dev_init_mode=mode,
                timeout=30)
    if not success:
        raise RuntimeError("Failed to get response for setfwd")

    if testutils.test_param_get("target") == "hw":
        port_key = []
        port_data = []
        for dev_port, params in swports.items():
            port_key.append(port_table.make_key([gc.KeyTuple('$DEV_PORT', dev_port)]))
            port_data.append(port_table.make_data([gc.DataTuple('$SPEED', str_val=str(params['$SPEED'])),
                                                   gc.DataTuple('$FEC', str_val=params['$FEC']),
                                                   gc.DataTuple('$AUTO_NEGOTIATION', str_val=params['$AUTO_NEGOTIATION']),
                                                   gc.DataTuple('$LOOPBACK_MODE', str_val=params['$LOOPBACK_MODE']),
                                                   gc.DataTuple('$PORT_ENABLE', bool_val=params['$PORT_ENABLE'])]))
            if not quiet:
                logger.info("Add port {} with speed {} fec {} an {} lpbk {}".format(dev_port,
                                                                                    params['$SPEED'],
                                                                                    params['$FEC'],
                                                                                    params['$AUTO_NEGOTIATION'],
                                                                                    params['$LOOPBACK_MODE']))
        if len(port_key) > 0:
            port_table.entry_add(target, port_key, port_data)


def end_warm_init(test, quiet=False):
    if not quiet: logger.info("End warm init")

    success = test.interface.send_set_forwarding_pipeline_config_request(
                  bfruntime_pb2.SetForwardingPipelineConfigRequest.WARM_INIT_END,
                  base_put_path,
                  timeout=20)
    if not success:
        raise RuntimeError("Failed to get response for setfwd")

    if testutils.test_param_get("target") == "hw":
        sleep_time = 5
        sleep_count = 0
        if not quiet: logger.info("Waiting for ports to come up...")
        try:
            bfrt_info = test.bfrt_info
        except:
            bfrt_info = test.interface.bfrt_info_get()
        port_table = bfrt_info.table_get("$PORT")
        target = gc.Target(device_id=0)
        for port in test.warminit_swports:
            portup = False
            while not portup:
                resp = port_table.entry_get(target, [port_table.make_key([gc.KeyTuple("$DEV_PORT", port)])])
                for data, _ in resp:
                    data_dict = data.to_dict()
                if data_dict["$PORT_UP"] == test.warminit_swports[port]["$PORT_UP"]:
                    portup = True
                    continue
                else:
                    if sleep_count == 60:
                        logger.error("Port {} status check failed after 60 secs, exp status {}", port, test.warminit_swports[port]["$PORT_UP"])
                        assert 0
                    time.sleep(sleep_time)
                    sleep_count += sleep_time


# deprecated. Use: self.dev_configuration.num_pipes
def get_num_pipes(device_conf_table):
    target = gc.Target(device_id=0, pipe_id=0xFFFF, direction=0)
    for data, _ in device_conf_table.default_entry_get(target):
        data_dict = data.to_dict()
        return data_dict['num_pipes']
    return int(testutils.test_param_get('num_pipes'))


def get_speed(port):
    _speed = str(testutils.port_param_get(port, 'speed'))
    speed = _speed if _speed is None else _speed.upper()
    if speed == "1G":
        return "BF_SPEED_1G"
    elif speed == "10G":
        return "BF_SPEED_10G"
    elif speed == "25G":
        return "BF_SPEED_25G"
    elif speed == "40G":
        return "BF_SPEED_40G"
    elif speed == "50G":
        return "BF_SPEED_50G"
    elif speed == "100G":
        return "BF_SPEED_100G"
    elif speed == "200G":
        return "BF_SPEED_200G"
    elif speed == "400G":
        return "BF_SPEED_400G"
    elif speed == "40G_R2":          # 40G 2x20G NRZ, Non-standard speed
        return "BF_SPEED_40G_R2"
    elif speed == "50G_CONS":        # 50G 2x25G NRZ, Consortium mode
        return "BF_SPEED_50G_CONS"
    else:
        logger.info("Unrecognized speed option {}, set to BF_SPEED_10G".format(speed))
        return "BF_SPEED_10G"


def get_fec(port):
    _fec = testutils.port_param_get(port, 'fec')
    fec = _fec if _fec is None else _fec.lower()
    if fec == "none":
        return "BF_FEC_TYP_NONE"
    elif fec == "rs":
        return "BF_FEC_TYP_REED_SOLOMON"
    elif fec == "fc":
        return "BF_FEC_TYP_FIRECODE"
    else:
        logger.info("Unrecognized fec option {}, set to BF_FEC_TYP_NONE".format(fec))
        return "BF_FEC_TYP_NONE"


def get_an(port):
    _an = testutils.port_param_get(port, 'auto_neg')
    an = _an if _an is None else _an.lower()
    if an == "default" or an == "auto":
        return "PM_AN_DEFAULT"
    elif an == "enable" or an == "on":
        return "PM_AN_FORCE_ENABLE"
    elif an == "disable" or an == "off":
        return "PM_AN_FORCE_DISABLE"
    else:
        logger.info("Unrecognized auto_neg option {}, set to PM_AN_FORCE_DISABLE".format(an))
        return "PM_AN_FORCE_DISABLE"


def get_portup(port):
    port_up = testutils.port_param_get(port, 'port_up')
    if port_up is False or port_up == 'down':
        return False
    else:
        return True


def add_ports(test):
    """ Add ports from json configuration file if hw run
        e.g. port configuration in json file:
        {
            "device_port":0,
            "if":"enp173s0f4",
            "speed":"10G",
            "auto_neg":"disable",
            "fec":"none"
        }
    """
    if testutils.test_param_get('target') != "hw":
        return
    try:
        bfrt_info = test.bfrt_info
    except:
        bfrt_info = test.interface.bfrt_info_get()

    target = gc.Target(device_id=0, pipe_id=0xffff)
    port_table = bfrt_info.table_get("$PORT")
    resp = port_table.entry_get(target, [])
    swports = []
    for _, key in resp:
        swports.append(key.to_dict()['$DEV_PORT']['value'])

    new_port_added = False
    for port in config['port_info']:
        if port in swports:
            logger.warn("Port {} already added".format(port))
            continue
        new_port_added = True
        speed = get_speed(port)
        fec = get_fec(port)
        an = get_an(port)
        port_up = get_portup(port)
        port_table.entry_add(
            target,
            [port_table.make_key([gc.KeyTuple("$DEV_PORT", port)])], [
                port_table.make_data([
                    gc.DataTuple("$SPEED", str_val=speed),
                    gc.DataTuple("$FEC", str_val=fec),
                    gc.DataTuple("$AUTO_NEGOTIATION", str_val=an),
                    gc.DataTuple("$PORT_ENABLE", bool_val=port_up)
                ])
            ])
        logger.info("Add port {} with speed {} fec {} an {}".format(port, speed, fec, an))
        swports.append(port)

    if new_port_added:
        check_port_status(test, swports)


def check_port_status(test, swports):
    try:
        bfrt_info = test.bfrt_info
    except:
        bfrt_info = test.interface.bfrt_info_get()

    target = gc.Target(device_id=0, pipe_id=0xffff)
    port_table = bfrt_info.table_get("$PORT")

    logger.info("Waiting for ports to come up...")
    sleep_time = 5
    for port in swports:
        sleep_time_total  = 0
        portup = False
        while not portup:
            resp = port_table.entry_get(
                target,
                [port_table.make_key([gc.KeyTuple("$DEV_PORT", port)])])
            for data, _ in resp:
                data_dict = data.to_dict()
            if data_dict["$PORT_UP"] == get_portup(port):
                portup = True
                continue
            else:
                if sleep_time_total  == 60:
                    assert 0, "Port {} is not getting up after 60 secs".format(port)
                time.sleep(sleep_time)
                sleep_time_total  += sleep_time
    logger.info("All ports {} are up".format(swports))
