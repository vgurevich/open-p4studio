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


import time

from ptf import config
import ptf.testutils as testutils
import pal_rpc.ttypes as pal_rpc_ttypes
import misc_utils

logger = misc_utils.get_logger()


def add_ports(test):
    if testutils.test_param_get('target') != "hw":
        return

    for port in config['port_info']:
        speed = get_speed(port)
        fec = get_fec(port)
        an = get_an(port)
        test.pal.pal_port_add(0, port, speed, fec)
        test.pal.pal_port_an_set(0, port, an)
        test.pal.pal_port_enable(0, port)
        logger.info("Add port {} with speed {} fec {} an {}"
                    .format(port, speed_to_string(speed), fec_to_string(fec), an_to_string(an)))


def check_port_status(test, swports):
    if testutils.test_param_get('target') != "hw":
        return

    logger.info("Waiting for ports to come up...")
    sleep_time = 5
    for port in swports:
        sleep_time_total  = 0
        portup = False
        while not portup:
            x = test.pal.pal_port_oper_status_get(0, port)
            if x == pal_rpc_ttypes.pal_oper_status_t.BF_PORT_UP:
                portup = True
            else:
                if sleep_time_total  == 60:
                    assert 0, "Port {} status check failed after 60 secs".format(port)
                time.sleep(sleep_time)
                sleep_time_total  += sleep_time
    logger.info("All ports {} are up".format(swports))


def get_speed(port):
    _speed = str(testutils.port_param_get(port, 'speed'))
    speed = _speed if _speed is None else _speed.upper()
    if speed == "1G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_1G
    elif speed == "10G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_10G
    elif speed == "25G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_25G
    elif speed == "40G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_40G
    elif speed == "40G_NB":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_40G_NB
    elif speed == "50G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_50G
    elif speed == "50G_CONS":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_50G_CONS
    elif speed == "100G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_100G
    elif speed == "200G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_200G
    elif speed == "400G":
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_400G
    else:
        logger.info("Unrecognized speed option {}, set to BF_SPEED_10G".format(speed))
        return pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_10G


def speed_to_string(speed):
    if speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_1G:
        return "1G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_10G:
        return "10G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_25G:
        return "25G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_40G:
        return "40G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_40G_NB:
        return "40G_NB"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_50G:
        return "50G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_50G_CONS:
        return "50G_CONS"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_100G:
        return "100G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_200G:
        return "200G"
    elif speed == pal_rpc_ttypes.pal_port_speed_t.BF_SPEED_400G:
        return "400G"
    else:
        assert 0, "Unrecognized port speed type"


def get_fec(port):
    _fec = testutils.port_param_get(port, 'fec')
    fec = _fec if _fec is None else _fec.lower()
    if fec == "none":
        return pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_NONE
    elif fec == "rs":
        return pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_REED_SOLOMON
    elif fec == "fc":
        return pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_FIRECODE
    else:
        logger.info("Unrecognized fec option {}, set to BF_FEC_TYP_NONE".format(fec))
        return pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_NONE


def fec_to_string(fec):
    if fec == pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_NONE:
        return "NONE"
    elif fec == pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_REED_SOLOMON:
        return "RS"
    elif fec == pal_rpc_ttypes.pal_fec_type_t.BF_FEC_TYP_FIRECODE:
        return "FC"
    else:
        assert 0, "Unrecognized port fec type"


def get_an(port):
    _an = testutils.port_param_get(port, 'auto_neg')
    an = _an if _an is None else _an.lower()
    if an == "default" or an == "auto":
        return pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_DEFAULT
    elif an == "enable" or an == "on":
        return pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_FORCE_ENABLE
    elif an == "disable" or an == "off":
        return pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_FORCE_DISABLE
    else:
        logger.info("Unrecognized auto_neg option {}, set to PM_AN_FORCE_DISABLE".format(an))
        return pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_FORCE_DISABLE


def an_to_string(fec):
    if fec == pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_DEFAULT:
        return "NONE"
    elif fec == pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_FORCE_ENABLE:
        return "ENABLE"
    elif fec == pal_rpc_ttypes.pal_autoneg_policy_t.BF_AN_FORCE_DISABLE:
        return "DISABLE"
    else:
        assert 0, "Unrecognized port speed type"
