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

#!/usr/bin/env python3

from __future__ import print_function
import json
from collections import OrderedDict

import os.path
import time
import six
import sys
from tenjin_util import render_template
import argparse

tenjin_prefix = "//::"

CURR_DIR = os.path.dirname(os.path.realpath(__file__))

TEMPLATE_FILE_NAME = "bf_switchapi.xml"
OUTPUT_XML_FILE_NAME = TEMPLATE_FILE_NAME

#global variable
objects = dict()
handle_key_tag = "hkey"
enable_logging_level_debug = 0


class LOG_LEVEL:
    INFO = 0
    DEBUG = 1


# mac address length
__mac_address_width = 6

map_attr_name_to_pval_name = lambda pname: "p_" + pname

get_ip_addr_family_clish_pname = lambda: ("addr_family")
get_ip_addr_sc_clish_pname = lambda: ("address_sc")
get_ip_addr_ip4_clish_pname = lambda: ("ipv4_addr")
get_ip_addr_list_ip4_clish_pname = lambda: ("ipv4_addr_list")
get_ip_addr_ip6_clish_pname = lambda: ("ipv6_addr")
get_ip_addr_list_ip6_clish_pname = lambda: ("ipv6_addr_list")
get_ip_prefix_sc_clish_pname = lambda: ("prefix_sc")
get_ip_prefix_ip4_clish_pname = lambda: ("ipv4_prefix")
get_ip_prefix_list_ip4_clish_pname = lambda: ("ipv4_prefix_list")
get_ip_prefix_ip6_clish_pname = lambda: ("ipv6_prefix")
get_ip_prefix_list_ip6_clish_pname = lambda: ("ipv6_prefix_list")
get_prefix_ipv4_mask_clish_pname = lambda: ("ipv4_mask")
get_prefix_ipv6_mask_clish_pname = lambda: ("ipv6_mask")

is_enum_type = lambda attr_type: (attr_type == "SWITCH_TYPE_ENUM")
is_handle_type = lambda attr_type: (attr_type == "SWITCH_TYPE_OBJECT_ID")

def get_list_type(attr_md_dict):
    list_type = None
    attr_info = attr_md_dict.get('type_info')
    if attr_info is None:
        return list_type
    list_info = attr_info.get('list')
    if list_info is not None:
       list_type = list_info.get('type')
    return list_type

gencli_integer_type = {
    "SWITCH_TYPE_UINT8", "SWITCH_TYPE_INT8", "SWITCH_TYPE_UINT16",
    "SWITCH_TYPE_INT16", "SWITCH_TYPE_UINT32", "SWITCH_TYPE_INT64",
    "SWITCH_TYPE_UINT64"
}


def is_integer_type(attr_type):
    if attr_type in gencli_integer_type:
        return True
    else:
        return False


base_attr_type_to_xml_ptype_dict = {
    "SWITCH_TYPE_BOOL": "BOOL",
    "SWITCH_TYPE_UINT8": "UINT8",
    "SWITCH_TYPE_INT8": "UINT8",
    "SWITCH_TYPE_INT16": "UINT16",
    "SWITCH_TYPE_UINT16": "UINT16",
    "SWITCH_TYPE_INT32": "INT32",
    "SWITCH_TYPE_UINT32": "UINT32",
    "SWITCH_TYPE_INT64": "DYNINT",
    "SWITCH_TYPE_UINT64": "DYNINT",
    "SWITCH_TYPE_MAC": "bf_switchapi_MAC",
    "SWITCH_TYPE_OBJECT_ID": "bf_switchapi_HEX_INT",
    "SWITCH_TYPE_STRING": "STRING",
    "SWITCH_TYPE_RANGE": "bf_switchapi_INT_RANGE"
}

list_attr_type_to_xml_ptype_dict = {
        "SWITCH_TYPE_OBJECT_ID": "bf_switchapi_INT_LIST"
}

inbuilt_clish_pname_dict = {
    "addr_family": {
        "pval_type": "bf_switchapi_addr_family_enum",
        "pname_h_str": "addr family",
        "pval_h_str": "addr family type"
    },
    "address_sc": {
        "pval_type": "address_switch",
        "pname_h_str": "address sc",
        "pval_h_str": "address type"
    },
    "prefix_sc": {
        "pval_type": "prefix_switch",
        "pname_h_str": "prefix sc",
        "pval_h_str": "prefix type"
    },
    "ipv4_addr": {
        "pval_type": "IPADDR",
        "pname_h_str": "ipv4 address",
        "pval_h_str": "IPv4 address"
    },
    "ipv4_addr_list": {
        "pval_type": "bf_switchapi_IPADDR_LIST",
        "pname_h_str": "ipv4 address list",
        "pval_h_str": "comma separated IPv4 address list"
    },
    "ipv4_prefix": {
        "pval_type": "IPADDR_MASK",
        "pname_h_str": "ipv4 prefix",
        "pval_h_str": "IPv4 prefix"
    },
    "ipv4_prefix_list": {
        "pval_type": "bf_switchapi_IPADDR_MASK_LIST",
        "pname_h_str": "ipv4 prefix list",
        "pval_h_str": "comma separated IPv4 prefix list"
    },
    "ipv6_addr": {
        "pval_type": "bf_switchapi_IPv6_ADDR",
        "pname_h_str": "ipv6 address",
        "pval_h_str": "IPv6 address(hex format)"
    },
    "ipv6_addr_list": {
        "pval_type": "bf_switchapi_IPv6_ADDR_LIST",
        "pname_h_str": "ipv6 address list",
        "pval_h_str": "comma separated IPv6 address(hex format) list"
    },
    "ipv6_prefix": {
        "pval_type": "bf_switchapi_IPv6_PREFIX",
        "pname_h_str": "ipv6 prefix",
        "pval_h_str": "IPv6 prefix(hex format)"
    },
    "ipv6_prefix_list": {
        "pval_type": "bf_switchapi_IPv6_PREFIX_LIST",
        "pname_h_str": "ipv6 prefix list",
        "pval_h_str": "comma separated IPv6 prefix(hex format) list"
    },
}


def map_attr_name_to_enum_type(obj_name, attr_name):
    enum_type = "bf_switchapi_" + obj_name + "__" + attr_name + "_enum"
    return enum_type


def map_enum_type_to_attr_name(enum_type):
    prefix = "bf_switchapi_"
    if enum_type.startswith(prefix):
        attr_name = enum_type[len("bf_switchapi_"):enum_type.index(
            "_enum")].rsplit("__", 1)[1]
    else:
        attr_name = enum_type[:enum_type.index("_enum")]
    return attr_name


gen_attr_help_string = lambda attr_name: (attr_name.replace("_", " "))


def get_pname_type_h_str_metadata(pname):
    pval_type = inbuilt_clish_pname_dict[pname]["pval_type"]
    pname_h_str = inbuilt_clish_pname_dict[pname]["pname_h_str"]
    pval_h_str = inbuilt_clish_pname_dict[pname]["pval_h_str"]
    return pname_h_str, pval_type, pval_h_str


addr_family_pname = get_ip_addr_family_clish_pname()
cli_inbuilt_types_enum_dict = {
    addr_family_pname + "_enum": {
        "IPV4_ADDR_FAMILY": 1,
        "IPV6_ADDR_FAMILY": 2
    },
    "acl_type_enum": {
        "MAC_ACL": 1,
        "IP_ACL": 2,
        "IPV6_ACL": 3
    }
}

model_json_enum_type_dict = dict()
model_range_dict = dict()


def map_attr_type_to_xml_ptype(attr_type):
    return (base_attr_type_to_xml_ptype_dict[attr_type])


def gen_clish_param_footer(space):
    footer = '''\n{}</PARAM>'''.format(space)
    return footer


def gen_clish_param_val_element(space, name, h_str, ptype, param_footer=True):
    if param_footer:
        p_val_str = '''\n{}<PARAM name="{}" help="{}" ptype="{}" />'''.format(
            space, name, h_str, ptype)
    else:
        p_val_str = '''\n{}<PARAM name="{}" help="{}" ptype="{}" >'''.format(
            space, name, h_str, ptype)

    return p_val_str


def gen_clish_param_element(space,
                            name,
                            h_str,
                            ptype="SUBCOMMAND",
                            mode="SUBCOMMAND",
                            optional="false",
                            test_exp=None):
    test_attr = ""
    if test_exp != None:
        test_attr = " test = " + test_exp
    # gen param string.
    p_str = '''{}<PARAM name="{}" help="{}" ptype="{}"
               mode="{}" optional="{}"{}>'''.format(space, name, h_str, ptype,
                                                    mode, optional, test_attr)
    return p_str

def gen_clish_switch_element(space,
                             name,
                             h_str,
                             ptype="SUBCOMMAND",
                             mode="switch",
                             optional="false",
                             test_exp=None):
    test_attr = ""
    if test_exp != None:
        test_attr = " test = " + test_exp
    # gen param string.
    p_str = '''{}<PARAM name="{}" help="{}" ptype="{}"
               mode="{}" optional="{}"{}>'''.format(space, name, h_str, ptype,
                                                    mode, optional, test_attr)
    return p_str

def gen_xml_test_exp(test_var, test_value_string):
    # ex: test_exp = '"${p_ipv4_addr}" = "1"
    test_exp = """'"${""" + test_var + '}"' + '=' + '"' + test_value_string + """"'"""
    return test_exp


"""
  if attr_name tagged with "hkey", use following format for pname and pvalue.
    input attr_name format: "hkey"+<handle_prefix>+"_obj_"+<object_type>+<key_attr_name>
    pname format:
     if <handle_prefix> == <key_attr_name>:
       pname = <key_attr_name>
     else:
       pname = <handle_prefix>+"_"+<key_attr_name>

    pvalue format:
      pvalue: "p_"+<handle_prefix"+"_obj_"+<object_type>+<key_attr_name>

    ptype: based on attr_type.
"""


def gen_xml_param_string_for_base_type(objects_dict,
                                       obj_name,
                                       attr_name,
                                       attr_metadata_dict,
                                       attr_name_space_tab,
                                       gen_p_footer=True):
    clish_params_buff = ""
    # gen param string for name=<attr_name>
    is_attr_handle_key = False
    if attr_name[0:4] == handle_key_tag:
        handle_prefix = attr_name[len(handle_key_tag) + 1:attr_name.find(
            "_obj_")]
        obj_key_str = attr_name[attr_name.find("_obj_") + len("_obj_"):]
        key_obj_name = obj_key_str[:obj_key_str.find('_')]
        key_attr_name = obj_key_str[obj_key_str.find('_') + 1:]
        key_obj_attrs_md = objects_dict[key_obj_name]["attributes"]
        pname = key_attr_name
        gencli_log_debug(LOG_LEVEL.DEBUG,
                         "attr_name: {} obj_name: {} key_attr_name: {}",
                         attr_name, obj_name, key_attr_name)
        gencli_log_debug(LOG_LEVEL.DEBUG, "obj_md: {}", objects_dict[obj_name])
        if "description" in key_obj_attrs_md[key_attr_name]:
            pname_h_str = key_obj_attrs_md[key_attr_name]["description"]
        else:
            pname_h_str = gen_attr_help_string(key_attr_name)
        is_attr_handle_key = True
    else:
        pname = attr_name
        gencli_log_debug(LOG_LEVEL.DEBUG, "obj_name:{} attr_md: {}", obj_name,
                         attr_metadata_dict)
        if "description" in attr_metadata_dict:
            pname_h_str = attr_metadata_dict["description"]
        else:
            pname_h_str = "configure " + gen_attr_help_string(attr_name)
    opt = "false"
    format_optional_help_str = True
    if "h_str_md" in attr_metadata_dict and \
      attr_metadata_dict["h_str_md"]["skip_optional_kw"]:
        format_optional_help_str = False

    if attr_metadata_dict["Optional"]:
        opt = "true"
        if format_optional_help_str:
            if "default_value" in attr_metadata_dict["type_info"]:
                pname_h_str = pname_h_str + " (Optional, default=" + \
                            str(attr_metadata_dict["type_info"]["default_value"]) + ")"
            else:
                pname_h_str = pname_h_str + " (Optional)"

    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + attr_name_param_str

    # gen param for value of attr_name, name=p_<attr_name>
    attr_type = attr_metadata_dict["type_info"]["type"]
    if is_enum_type(attr_type):
        pval_type = map_attr_name_to_enum_type(obj_name, attr_name)
    elif is_range_type_integer(attr_type, attr_metadata_dict["type_info"]):
        pval_type = map_attr_type_to_range_ptype(
            attr_metadata_dict["type_info"]["max"])
    else:
        pval_type = map_attr_type_to_xml_ptype(attr_type)

    if is_attr_handle_key:
        pval_h_str = "Value of " + key_attr_name
        pname_val = map_attr_name_to_pval_name(
            attr_name[len(handle_key_tag) + 1:])
    else:
        pval_h_str = "Value of " + attr_name
        pname_val = map_attr_name_to_pval_name(pname)

    attr_val_space_tab = attr_name_space_tab + "  "
    param_str = gen_clish_param_val_element(attr_val_space_tab, pname_val,
                                            pval_h_str, pval_type)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=<attr_name>
    if gen_p_footer:
        param_str = gen_clish_param_footer(attr_name_space_tab)
        clish_params_buff = clish_params_buff + param_str
    return clish_params_buff


'''
 Example:
 Input:
   obj_name: port
   attr_name: ingress_acl_handle
   attr_metadata_dict:
        'h_str_md': {
            'skip_optional_kw': True,
            'is_set_attribute': True
        },
        'Optional': True,
        'description': 'Ingress ACL group or table handle',
        'type_info': OrderedDict([('allowed_object_types', ['acl_group', 'acl_table']), ('type', 'SWITCH_TYPE_OBJECT_ID')])}
 Output:
   pname format: <attr_name>
   pname switch format: p_<attr_name>
   pvalue format: p_<attr_name>_<allowed_type>_oid
   phelp_string: "<handle_prefix> object's oid"
   pvalue_string: "Value of object_id"
   ptype: bf_switchapi_HEX_INT
   Example:
    <PARAM name="ingress_acl_handle" help="Configure ingress_acl_handle (Optional)" ptype="STRING" mode="SUBCOMMAND" optional="true">
        <PARAM name="p_ingress_acl_handle" help="Configure i acl_table_handle (Optional)" ptype="SUBCOMMAND" mode="switch">
            <PARAM name="acl_group" value="ing_acl_group" help="Configure acl_group_handle (Optional)" ptype="SUBCOMMAND" mode="SUBCOMMAND" optional="true">
                <PARAM name="p_ingress_acl_handle_acl_group_oid" help="Value of handle" ptype="bf_switchapi_HEX_INT" />
            </PARAM>
            <PARAM name="acl_table" help="Configure acl_table_handle (Optional)" ptype="SUBCOMMAND" mode="SUBCOMMAND" optional="true">
                <PARAM name="p_ingress_acl_handle_acl_table_oid" help="Value of handle" ptype="bf_switchapi_HEX_INT" />
            </PARAM>
        </PARAM>
    </PARAM>
    The p_ingress_acl_handle makes it a switch operation so only one of acl_group or acl_table selected
    The pvalue name, p_ingress_acl_handle_acl_group_oid makes sure this param is unique.
    For egress_acl_handle/acl_group, which is also a port attribute, the pvalue name would be p_egress_acl_handle_acl_group_oid.
    Hence there is no conflict with acl_group in ingress/egress acl handle and we can still expose the same type from the model.
'''


def gen_xml_param_string_for_object_id_type(objects_dict, obj_name, attr_name,
                                            attr_metadata_dict,
                                            attr_name_space_tab, is_list=False):
    clish_params_buff = ""

    pname = attr_name
    if "h_str_md" in attr_metadata_dict and \
      attr_metadata_dict["h_str_md"]["is_set_attribute"]:
        pname_h_str = "Enter " + obj_name + " object's " + attr_name
    else:
        pname_h_str = "Configure " + attr_name
    pswitch = "p_" + attr_name
    pswitch_h_str = attr_name + " switch helper"

    format_optional_help_str = True
    if "h_str_md" in attr_metadata_dict and \
      attr_metadata_dict["h_str_md"]["skip_optional_kw"]:
        format_optional_help_str = False

    opt = "false"
    if attr_metadata_dict["Optional"]:
        opt = "true"
        if format_optional_help_str:
            if "default_value" in attr_metadata_dict["type_info"]:
                pname_h_str = pname_h_str + " (Optional, default=" + \
                              str(attr_metadata_dict["type_info"]["default_value"]) + ")"
            else:
                pname_h_str = pname_h_str + " (Optional)"

    if is_list:
        pval_type = list_attr_type_to_xml_ptype_dict.get("SWITCH_TYPE_OBJECT_ID")
        pval_h_str = "List of handles"
        allowed_object_types = attr_metadata_dict['type_info']['list']['allowed_object_types']
    else:
        pval_type = map_attr_type_to_xml_ptype("SWITCH_TYPE_OBJECT_ID")
        pval_h_str = "Value of handle"
        allowed_object_types = attr_metadata_dict["type_info"]["allowed_object_types"]

    # generate pname param
    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + "\n" + attr_name_param_str

    # generate pname switch param
    attr_name_param_str = gen_clish_switch_element(
        attr_name_space_tab + "  ", pswitch, pswitch_h_str)
    clish_params_buff = clish_params_buff + "\n" + attr_name_param_str

    # gen param for value of attr_name, allowed_object_type
    for allowed_object_type in \
        allowed_object_types:
        pname_ot = allowed_object_type
        pname_ot_h_str = "Configure " + allowed_object_type + " handle"
        param_ot_str = gen_clish_param_element(
            attr_name_space_tab + "    ", pname_ot, pname_ot_h_str)
        clish_params_buff = clish_params_buff + "\n" + param_ot_str

        pval_ot = "p_" + pname + "_" + allowed_object_type + "_oid"
        param_val_str = gen_clish_param_val_element(
            attr_name_space_tab + "      ", pval_ot, pval_h_str, pval_type)
        clish_params_buff = clish_params_buff + param_val_str

        param_ot_str = gen_clish_param_footer(attr_name_space_tab + "    ")
        clish_params_buff = clish_params_buff + param_ot_str

    param_ot_str = gen_clish_param_element(
        attr_name_space_tab + "    ", "0", "Set null object id")
    clish_params_buff = clish_params_buff + "\n" + param_ot_str
    param_ot_str = gen_clish_param_footer(attr_name_space_tab + "    ")
    clish_params_buff = clish_params_buff + param_ot_str

    #gen switch footer for name=<attr_name>
    param_str = gen_clish_param_footer(attr_name_space_tab + "  ")
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=<attr_name>
    param_str = gen_clish_param_footer(attr_name_space_tab)
    clish_params_buff = clish_params_buff + param_str
    return clish_params_buff


def gen_xml_param_string_for_mac_type(objects_dict, obj_name, attr_name,
                                      attr_metadata_dict, attr_name_space_tab):
    clish_params_buff = ""
    # gen param string for name=<attr_name>
    pname = attr_name
    if "description" in attr_metadata_dict:
        pname_h_str = attr_metadata_dict["description"]
    else:
        pname_h_str = "Mac address"
    opt = "false"
    if attr_metadata_dict["Optional"]:
        opt = "true"
        pname_h_str = pname_h_str + "(Optional)"

    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + attr_name_param_str

    # gen param for value of attr_name, name=p_<attr_name>
    attr_val_space_tab = attr_name_space_tab + "  "
    pname_val = map_attr_name_to_pval_name(pname)
    pval_type = "bf_switchapi_MAC"
    pval_h_str = "Value of " + attr_name + " (in format XX:XX:XX:XX:XX:XX)"
    param_str = gen_clish_param_val_element(attr_val_space_tab, pname_val,
                                            pval_h_str, pval_type)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=<attr_name>
    param_str = gen_clish_param_footer(attr_name_space_tab)
    clish_params_buff = clish_params_buff + param_str
    return clish_params_buff


"""
   address [ipv4_addr <> | ipv6_address]
   prefix [ipv4_addr <> mask <> | ipv6_address <> mask <>]
   pname address
     pname address_sc
       pname ipv4_addr
          pname ipv4_addr_value
       pname_ip6_addr
          pname ip6_addr_value
   pname prefix
      pname prefix_sc
       pname ipv4_addr
          pname ipv4_addr_value
          pname ipv4_mask
          pname ip4_mask_value
       pname ip6_addr
          pname ipv6_addr_value
          pname ipv6_mask
          pname ipv6_mask_value

"""


def gen_xml_param_string_for_ip_addr_type(objects_dict, obj_name, attr_name,
                                          attr_metadata_dict,
                                          attr_name_space_tab, is_list=False):
    clish_params_buff = ""

    # gen param , name=attr_name
    pname = attr_name
    if "description" in attr_metadata_dict:
        pname_h_str = attr_metadata_dict["description"]
    else:
        pname_h_str = gen_attr_help_string(attr_name)
    if attr_metadata_dict["Optional"]:
        pname_h_str = pname_h_str + "(Optional)"
    opt = "false"
    if attr_metadata_dict["Optional"]:
        opt = "true"
    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + attr_name_param_str

    # gen param, name=<attr_name>_address_sc
    addr_sc_space_tab = attr_name_space_tab + "  "
    address_sc = get_ip_addr_sc_clish_pname()
    pname = attr_name + "_" + address_sc
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        address_sc)
    param_str = gen_clish_param_element(
        addr_sc_space_tab, pname, pname_h_str, mode="switch")
    clish_params_buff = clish_params_buff + "\n" + param_str

    sc_params_value_space_tab = addr_sc_space_tab + "  "

    # gen param for value of ipv6_addr, name=p_ipv6_addr
    if is_list:
        ip6_addr_pname = get_ip_addr_list_ip6_clish_pname()
    else:
        ip6_addr_pname = get_ip_addr_ip6_clish_pname()
    pname = attr_name + "_" + ip6_addr_pname
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        ip6_addr_pname)
    pname_val = map_attr_name_to_pval_name(pname)
    param_str = gen_clish_param_val_element(
        sc_params_value_space_tab,
        pname_val,
        pval_h_str,
        pval_type,
        param_footer=False)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=name=p_ipv6_addr
    param_str = gen_clish_param_footer(sc_params_value_space_tab)
    clish_params_buff = clish_params_buff + param_str

    # gen param for value of ipv4_addr, name=p_ipv4_addr
    if is_list:
        ip4_addr_pname = get_ip_addr_list_ip4_clish_pname()
    else:
        ip4_addr_pname = get_ip_addr_ip4_clish_pname()
    pname = attr_name + "_" + ip4_addr_pname
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        ip4_addr_pname)
    pname_val = map_attr_name_to_pval_name(pname)
    param_str = gen_clish_param_val_element(
        sc_params_value_space_tab,
        pname_val,
        pval_h_str,
        pval_type,
        param_footer=False)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=p_ipv4_addr
    param_str = gen_clish_param_footer(sc_params_value_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=address_sc
    param_str = gen_clish_param_footer(addr_sc_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=attr_name
    param_str = gen_clish_param_footer(attr_name_space_tab)
    clish_params_buff = clish_params_buff + param_str

    return clish_params_buff

def gen_xml_param_string_for_uint32(objects_dict, obj_name, attr_name,
                                          attr_metadata_dict,
                                          attr_name_space_tab):
    clish_params_buff = ""
    pname = attr_name
    if "description" in attr_metadata_dict:
        pname_h_str = attr_metadata_dict["description"]
    else:
        pname_h_str = gen_attr_help_string(attr_name)
    if attr_metadata_dict["Optional"]:
        pname_h_str = pname_h_str + "(Optional)"
    opt = "false"
    if attr_metadata_dict["Optional"]:
        opt = "true"
    pswitch = "p_" + attr_name
    pswitch_h_str = attr_name + " switch helper"

    # gen attr param name
    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + attr_name_param_str
    
    # gen param name
    attr_name_lst_space_tab = attr_name_space_tab + "  "
    name_val = attr_name + "_lst"
    param_str = gen_clish_param_element(
        attr_name_lst_space_tab, name_val, pswitch_h_str, mode="switch")
    clish_params_buff = clish_params_buff + "\n" + param_str

    # gen param for value
    start_params_value_space_tab = attr_name_lst_space_tab + "  "
    pname_val =  "p_" + attr_name + "_lst"
    pval_type = "bf_switchapi_INT_LIST"
    pval_h_str = "Value of " + attr_name + ", comma separated list"
    param_str = gen_clish_param_val_element(start_params_value_space_tab, pname_val,
                                            pval_h_str, pval_type, param_footer=False)
    clish_params_buff = clish_params_buff + param_str

    #gen switch footer for value
    param_str = gen_clish_param_footer(start_params_value_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for param name
    param_str = gen_clish_param_footer(attr_name_lst_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for attr param name
    param_str = gen_clish_param_footer(attr_name_space_tab)
    clish_params_buff = clish_params_buff + param_str

    return clish_params_buff

# cmd param format: <attr_name> ip4_addr/<mask-len> or ipv6_addr/mask_len
def gen_xml_param_string_for_ip_prefix_type(objects_dict, obj_name, attr_name,
                                            attr_metadata_dict,
                                            attr_name_space_tab, is_list=False):
    clish_params_buff = ""

    # gen param , name=attr_name
    pname = attr_name
    if "description" in attr_metadata_dict:
        pname_h_str = attr_metadata_dict["description"]
    else:
        pname_h_str = gen_attr_help_string(attr_name)
    if attr_metadata_dict["Optional"]:
        pname_h_str = pname_h_str + "(Optional)"
    opt = "false"
    if attr_metadata_dict["Optional"]:
        opt = "true"
    attr_name_param_str = gen_clish_param_element(
        attr_name_space_tab, pname, pname_h_str, optional=opt)
    clish_params_buff = clish_params_buff + attr_name_param_str

    # gen param, name=<attr_name>_prefix_sc
    prefix_sc_space_tab = attr_name_space_tab + "  "
    prefix_sc = get_ip_prefix_sc_clish_pname()
    pname = attr_name + "_" + prefix_sc
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        prefix_sc)
    param_str = gen_clish_param_element(
        prefix_sc_space_tab, pname, pname_h_str, mode="switch")
    clish_params_buff = clish_params_buff + "\n" + param_str

    sc_params_value_space_tab = prefix_sc_space_tab + "  "
    # gen param for value of ipv4_prefix, name=p_ipv4_addr
    if is_list:
        ip4_prefix_pname = get_ip_prefix_list_ip4_clish_pname()
    else:
        ip4_prefix_pname = get_ip_prefix_ip4_clish_pname()
    pname = attr_name + "_" + ip4_prefix_pname
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        ip4_prefix_pname)
    pname_val = map_attr_name_to_pval_name(pname)
    param_str = gen_clish_param_val_element(
        sc_params_value_space_tab,
        pname_val,
        pval_h_str,
        pval_type,
        param_footer=False)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=p_ipv4_addr
    param_str = gen_clish_param_footer(sc_params_value_space_tab)
    clish_params_buff = clish_params_buff + param_str

    # gen param for value of ipv6_prefix, name=p_ipv6_prefix
    if is_list:
        ip6_prefix_pname = get_ip_prefix_list_ip6_clish_pname()
    else:
        ip6_prefix_pname = get_ip_prefix_ip6_clish_pname()
    pname = attr_name + "_" + ip6_prefix_pname
    pname_h_str, pval_type, pval_h_str = get_pname_type_h_str_metadata(
        ip6_prefix_pname)
    pname_val = map_attr_name_to_pval_name(pname)
    param_str = gen_clish_param_val_element(
        sc_params_value_space_tab,
        pname_val,
        pval_h_str,
        pval_type,
        param_footer=False)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=name=p_ipv6_prefix
    param_str = gen_clish_param_footer(sc_params_value_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=prefix_sc
    param_str = gen_clish_param_footer(prefix_sc_space_tab)
    clish_params_buff = clish_params_buff + param_str

    #gen param footer for name=attr_name
    param_str = gen_clish_param_footer(attr_name_space_tab)
    clish_params_buff = clish_params_buff + param_str

    return clish_params_buff

def gen_xml_param_string_for_list_type(objects_dict, obj_name, attr_name,
                                       attr_metadata_dict,
                                       attr_name_space_tab):
    list_type = get_list_type(attr_metadata_dict)
    if list_type == "SWITCH_TYPE_OBJECT_ID":
        return gen_xml_param_string_for_object_id_type(objects_dict, obj_name, attr_name, attr_metadata_dict,
                attr_name_space_tab, is_list=True)
    elif list_type == "SWITCH_TYPE_IP_ADDRESS":
        return gen_xml_param_string_for_ip_addr_type(objects_dict, obj_name, attr_name, attr_metadata_dict,
                attr_name_space_tab, is_list=True)
    elif list_type == "SWITCH_TYPE_IP_PREFIX":
        return gen_xml_param_string_for_ip_prefix_type(objects_dict, obj_name, attr_name, attr_metadata_dict,
                attr_name_space_tab, is_list=True)
    elif list_type == "SWITCH_TYPE_UINT32":
        return gen_xml_param_string_for_uint32(objects_dict, obj_name, attr_name, attr_metadata_dict,
                attr_name_space_tab)
    else:
        gencli_log_debug(LOG_LEVEL.DEBUG, "Failed to generate xml string for object {} attr {}. bf_switch cli xml generation not supported for attr of type {}:{}",
                         obj_name, attr_name, attr_metadata_dict["type_info"]["type"], list_type)
        clish_params_buff = ""
        return clish_params_buff

def gen_xml_param_string_for_range_type(objects_dict, obj_name, attr_name,
                                        attr_metadata_dict,
                                        attr_name_space_tab):
    clish_params_buff = ""
    return clish_params_buff


"""
  Note: currently SWITCH_TYPE_SELECT used only for key attrs.
        this can be used in future for any attrs.
  input:
    attr_name = "key"
  attr_metadata_dict["type_info"]["attr_list"] = [attr1, attr2, [attr3, attr4], attr5]
  generate pname string as follows
       <PNAME name="object_handle_sc" help= "<>" ptype="SUBCOMMAND" mode="switch">
          <PANME> name="attr1" .....>
            <PNAME> name="p_attr1" ..../>
          </PANME>
          <PANME> name="attr2" .....>
            <PNAME> name="p_attr2" ..../>
          </PANME>
          <PANME> name="attr3" .....>
            <PNAME> name="p_attr3" ..../>
            <PANME> name="attr4" .....>
             <PNAME> name="p_attr4" ..../>
          </PANME>
          <PANME> name="attr5" .....>
            <PNAME> name="p_attr5" ..../>
          </PANME>
       </PANME>
"""


def gen_xml_param_string_for_select_type(objects_dict, object_name, attr_name,
                                         attr_metadata_dict,
                                         attr_name_space_tab):
    # gen param, name="object_handle_sc"
    clish_params_buff = ""
    handle_sc_space_tab = attr_name_space_tab
    pname = "object_handle_sc" + "%.20f" % time.time()
    pname_h_str = "handle sc"
    param_str = gen_clish_param_element(
        handle_sc_space_tab, pname, pname_h_str, mode="switch")
    clish_params_buff = clish_params_buff + param_str

    handle_sc_level_1_tab = handle_sc_space_tab + " "
    key_attrs_list = attr_metadata_dict["type_info"]["attr_list"]
    object_attrs_md = objects_dict[object_name]["attributes"]
    for key_attr_name in key_attrs_list:
        p_buff = ""
        if isinstance(key_attr_name, list):
            p_buff = ""
            key_sub_attr_list = key_attr_name
            for index, sub_key_attr_name in enumerate(key_sub_attr_list):
                sub_key_attr_md_dict = object_attrs_md[sub_key_attr_name]
                if "is_internal" in sub_key_attr_md_dict and \
                   sub_key_attr_md_dict["is_internal"]:
                    continue
                gen_p_footer = True
                if index == 0:
                    sub_attr_space_tab = handle_sc_level_1_tab
                    gen_p_footer = False
                else:
                    sub_attr_space_tab = handle_sc_level_1_tab + " "
                p_buff = p_buff + gen_xml_param_string_for_base_type(
                    objects_dict,
                    object_name,
                    sub_key_attr_name,
                    sub_key_attr_md_dict,
                    sub_attr_space_tab,
                    gen_p_footer=gen_p_footer)
            # gen footer for first pname.
            clish_params_buff = clish_params_buff + "\n" + p_buff + gen_clish_param_footer(
                handle_sc_level_1_tab)
        else:
            key_attr_md_dict = object_attrs_md[attr_name]
            if "is_internal" in key_attr_md_dict and \
               key_attr_md_dict["is_internal"]:
                continue
            p_buff = gen_attr_clish_xml_params_string(
                objects_dict, object_name, key_attr_name, key_attr_md_dict,
                handle_sc_level_1_tab)
            clish_params_buff = clish_params_buff + "\n" + p_buff
    clish_params_buff = clish_params_buff + gen_clish_param_footer(
        handle_sc_space_tab)
    return clish_params_buff


def get_gen_clish_param_f_hdl(type):
    gen_fun_handler_dict = {
        "SWITCH_TYPE_BASE": gen_xml_param_string_for_base_type,
        "SWITCH_TYPE_MAC": gen_xml_param_string_for_mac_type,
        # "SWITCH_TYPE_STRING" : gen_xml_param_string_for_mac_type,
        "SWITCH_TYPE_IP_ADDRESS": gen_xml_param_string_for_ip_addr_type,
        "SWITCH_TYPE_IP_PREFIX": gen_xml_param_string_for_ip_prefix_type,
        #"SWITCH_TYPE_RANGE" : gen_xml_param_string_for_range_type,
        "SWITCH_TYPE_LIST": gen_xml_param_string_for_list_type,
        "SWITCH_TYPE_OBJECT_ID": gen_xml_param_string_for_object_id_type,
        "SWITCH_TYPE_SELECT": gen_xml_param_string_for_select_type
    }
    return gen_fun_handler_dict[type]


def gen_attr_clish_xml_params_string(objects_dict,
                                     obj_name,
                                     attr_name,
                                     attr_metadata_dict,
                                     attr_name_space_tab=""):
    attr_type = attr_metadata_dict["type_info"]["type"]
    type = attr_type
    clish_xml_params_str = ""
    if is_base_type(attr_type):
        type = "SWITCH_TYPE_BASE"

    clish_param_fun_handler = get_gen_clish_param_f_hdl(type)
    clish_xml_params_str = clish_param_fun_handler(
        objects_dict, obj_name, attr_name, attr_metadata_dict,
        attr_name_space_tab)
    return clish_xml_params_str


"""
populate enum_dict as part of processing attr_type_info().
this will be used to populate xml select ptype.
store this enum ptype in the atrribute type field.
"""


# process enum type_info and populate global enum dictionary
def proc_attr_type_enum(obj_name, attr_name, attr_type_info_metadata_dict):
    enum_list = attr_type_info_metadata_dict["enum"]
    enum_type = map_attr_name_to_enum_type(obj_name, attr_name)
    enum_dict = dict()
    for enum_value, enum_name in enumerate(enum_list):
        enum_dict[enum_name] = enum_value
    gencli_log_debug(LOG_LEVEL.DEBUG, "enum_name: {}, enum_dict: {}",
                     enum_type, enum_dict)
    model_json_enum_type_dict[enum_type] = enum_dict
    return


def is_range_type_integer(attr_type, attr_type_info):
    if is_integer_type(attr_type) and "max" in attr_type_info:
        return True
    return False


range_ptype_prefix = lambda: "bf_switchapi_uint_range_"


def map_attr_type_to_range_ptype(max):
    range_name = range_ptype_prefix() + str(max)
    #if not range_name in model_range_dict:
    return range_name


def proc_attr_type_integer(attr_name, attr_type_info_metadata_dict):
    if "max" in attr_type_info_metadata_dict:
        max_value = attr_type_info_metadata_dict["max"]
        range_name = range_ptype_prefix() + str(max_value)
        if not range_name in model_range_dict:
            range = dict()
            range["max"] = max_value
            range["min"] = 1
            model_range_dict[range_name] = range


def is_object_handle_input_key_based(obj_type):
    obj_md = objects[obj_type]
    if "cli_info" in obj_md:
        cli_info_md = obj_md["cli_info"]
        if "format" in cli_info_md and  \
          "key_attrs" in cli_info_md["format"]:
            return True
    return False


def proc_attr_type_obj_handle_and_gen_d_attrs_dict(attr_name, attr_md,
                                                   gen_d_attrs_dict):
    if (attr_name == "device"):
        return

    if "is_internal" in attr_md and attr_md["is_internal"]:
        #skip internal attributes
        gencli_log_debug(LOG_LEVEL.DEBUG, "Skip processing internal attribute")
        return

    type_info = attr_md["type_info"]
    allowed_object_list = type_info["allowed_object_types"]

    d_allowed_obj_handle_md = dict(attr_md)
    if ("is_mandatory" in attr_md and attr_md["is_mandatory"]):
        d_allowed_obj_handle_md["Optional"] = False
    else:
        d_allowed_obj_handle_md["Optional"] = True
    gen_d_attrs_dict["d_attrs_md"][attr_name] = d_allowed_obj_handle_md

    gen_d_attrs_dict["prune_list"].append(attr_name)
    gen_d_attrs_dict["d_attr_names"]["need_catalog_type_input"] = False
    gen_d_attrs_dict["d_attr_names"]["attr_name_list"] = [attr_name]
    return


'''
 @obj_dist: object metadata dictionary
 @attr_name: attribute name to process
 @gen_attr_rem_list: output list. Valid for attr_type == OBJECT_ID.
 @gen_derived_attrs_dict_out: output dict valid for attr_type == OBJECT_ID.
                              container for deriveed attributes.
'''


def process_attribute_and_gen_d_attrs_dict(object_name, attr_name, attr_md,
                                           gen_d_attrs_dict):
    attr_type_info_dict = attr_md["type_info"]
    attr_type = attr_type_info_dict["type"]
    if is_enum_type(attr_type):
        proc_attr_type_enum(object_name, attr_name, attr_type_info_dict)
    elif is_integer_type(attr_type):
        proc_attr_type_integer(attr_name, attr_type_info_dict)
    elif is_handle_type(attr_type):
        proc_attr_type_obj_handle_and_gen_d_attrs_dict(attr_name, attr_md,
                                                       gen_d_attrs_dict)


gencli_base_type = {
    "SWITCH_TYPE_BOOL", "SWITCH_TYPE_UINT8", "SWITCH_TYPE_INT8",
    "SWITCH_TYPE_UINT16", "SWITCH_TYPE_INT16", "SWITCH_TYPE_UINT32",
    "SWITCH_TYPE_INT64", "SWITCH_TYPE_UINT64", "SWITCH_TYPE_ENUM",
    "SWITCH_TYPE_STRING", "SWITCH_TYPE_RANGE"
}


def is_base_type(attr_type):
    if attr_type in gencli_base_type:
        return True
    else:
        return False


def object_generator(objects_dict, object_class):
    for object_name, object_metadata in six.iteritems(objects_dict):
        if (object_metadata["class"] == object_class):
            # processing user view objects
            yield object_name, object_metadata


def counter_object_generator(objects_dict, object_class):
    for object_name, object_metadata in six.iteritems(objects_dict):
        if (object_metadata["class"] == object_class) and \
            "counter" in object_metadata :
            # processing user view  counter objects
            yield object_name, object_metadata


def obj_model_attr_generator(object_metadata):
    for attr_name in object_metadata["pre_process_obj_attrs"]:
        yield attr_name


def attr_generator_with_mandatory_first(object_metadata):
    object_name = object_metadata["attributes"]
    attrs_dict = object_metadata["attributes"]
    mandatory_list = object_metadata["mandatory_attributes"]
    if object_name == "vlan_member":
        vlan_member

    gencli_log_debug(LOG_LEVEL.DEBUG, "mandatory_list: {}", mandatory_list)
    for mandatory_attr_name in mandatory_list:
        gencli_log_debug(LOG_LEVEL.DEBUG, "mandatory_attr_name: {}",
                         mandatory_attr_name)
        if isinstance(mandatory_attr_name, list):
            attr_name = mandatory_attr_name[0]
            attr_md = dict()
            attr_md["type_info"] = dict()
            attr_md["type_info"]["type"] = "SWITCH_TYPE_SELECT"
            attr_md["type_info"]["attr_list"] = mandatory_attr_name[1:]
            yield attr_name, attr_md
            continue
        if ("is_internal" in attrs_dict[mandatory_attr_name]
                and attrs_dict[mandatory_attr_name]["is_internal"]):
            #skip internal attributes
            gencli_log_debug(LOG_LEVEL.DEBUG,
                             "Skip processing internal attribute")
            continue

        if "is_read_only" in attrs_dict[mandatory_attr_name]:
            #skip read-only attribute
            continue

        if mandatory_attr_name == "device":
            #skip device attribute,
            continue
        yield mandatory_attr_name, attrs_dict[mandatory_attr_name]

    for attr_name, attr_metadata in six.iteritems(attrs_dict):
        if ("is_internal" in attr_metadata and attr_metadata["is_internal"]):
            #skip internal attribute
            continue

        if "is_read_only" in attr_metadata:
            #skip read-only attribute
            continue

        if attr_name == "device":
            #skip device attribute,
            continue

        if attr_metadata["Optional"]:
            gencli_log_debug(LOG_LEVEL.DEBUG, "Optional attr_name: {}",
                             attr_name)
            # yield optional attribute
            yield attr_name, attr_metadata


'''
 Note: key_group_list may contains d_attrs. it will be in following format.
    key_attr = [key_1, key_2, [key3, key4, key5]]
 d_attrs are stored as a list inside key_groups_list[0],
   ex: key_groups_list[0] = ["attr_1", "attr_2", ["attr3", "attr4", ["attr5,"attr6"]], "attr_7"]
       here list ["attr3", "attr4", ["attr5,"attr6"]] indicates d_attrs. we need to special treatement for this case.
 so key_attr may be  a list of attr_names of having following format.
 we need to maintain this list in same order as this indicates how to generate "switch" kw during
 clish param generation phase. So attr_md for such attribute will be derived in clish param phase.
 here tag such key_attr as "attr_type_select" and return key list as attribute_metadata
'''


def key_attr_generator(object_metadata):
    attrs_dict = object_metadata["attributes"]
    if not "key_groups" in object_metadata:
        return
    key_groups_list = object_metadata["key_groups"]
    for key_attr in key_groups_list[0]:
        if key_attr == "device":
            #skip device attribute,
            continue
        if isinstance(key_attr, list):
            attr_name = key_attr[0]
            attr_md = dict()
            attr_md["type_info"] = dict()
            attr_md["type_info"]["type"] = "SWITCH_TYPE_SELECT"
            attr_md["type_info"]["attr_list"] = key_attr[1:]
            yield attr_name, attr_md
        else:
            yield key_attr, attrs_dict[key_attr]


def set_non_key_attr_generator(object_metadata):
    attrs_dict = object_metadata["attributes"]
    key_group_list = []
    if "key_groups" in object_metadata:
        key_group_list = object_metadata["key_groups"][0]

    # generate non-key fields
    for attr_name, attr_metadata in six.iteritems(attrs_dict):
        if ("is_internal" in attr_metadata and attr_metadata["is_internal"]):
            #skip internal attribute
            continue

        if "is_create_only" in attr_metadata:
            #skip create-only attribute
            continue

        if "is_read_only" in attr_metadata:
            #skip read-only attribute
            continue

        if attr_name == "device":
            #skip device attribute,
            continue

        if attr_name in key_group_list:
            # skip key attr.
            continue

        set_attr_md = dict(attr_metadata)
        set_attr_md["Optional"] = True
        set_attr_md["h_str_md"] = dict()
        set_attr_md["h_str_md"]["skip_optional_kw"] = True
        set_attr_md["h_str_md"]["is_set_attribute"] = True

        # yield attribute
        yield attr_name, set_attr_md
    #endfor


def set_attr_generator_with_key_first(object_metadata):
    attrs_dict = object_metadata["attributes"]
    if not "key_groups" in object_metadata:
        return
    key_groups_list = object_metadata["key_groups"]
    # generate key fields first
    for key_attr in key_groups_list[0]:
        if key_attr == "device":
            #skip device attribute,
            continue
        if isinstance(key_attr, list):
            attr_name = "key"
            attr_md = dict()
            attr_md["type_info"] = dict()
            attr_md["type_info"]["type"] = "SWITCH_TYPE_SELECT"
            attr_md["type_info"]["attr_list"] = key_attr
            yield attr_name, attr_md
        else:
            yield key_attr, attrs_dict[key_attr]

    # generate non-key fields
    for attr_name, attr_metadata in six.iteritems(attrs_dict):
        if ("is_internal" in attr_metadata and attr_metadata["is_internal"]):
            #skip internal attribute
            continue

        if "is_read_only" in attr_metadata:
            #skip read-only attribute
            continue

        if attr_name == "device":
            #skip device attribute,
            continue

        if attr_name in key_groups_list[0]:
            # skip key attr.
            continue

        set_attr_md = dict(attr_metadata)
        set_attr_md["Optional"] = True
        set_attr_md["h_str_md"] = dict()
        set_attr_md["h_str_md"]["skip_optional_kw"] = True
        set_attr_md["h_str_md"]["is_set_attribute"] = True

        # yield attribute
        yield attr_name, set_attr_md
    #endfor


def gen_object_dict_from_model_json(json_filename):
    global objects
    with open(json_filename, 'r') as model_json:
        json_data = model_json.read()
        model_json_dict = json.loads(
            json_data, object_pairs_hook=OrderedDict)
    #@sandesh: Validate json file and report error in case of invalid file
    objects = model_json_dict["objects"]
    for object_name, object_metadata in six.iteritems(objects):
        gencli_log_debug(LOG_LEVEL.DEBUG, "process object name: {}",
                         object_name)
        obj_attrs_dict = object_metadata["attributes"]
        # process attr_type enums and update enum dict
        gencli_log_debug(LOG_LEVEL.DEBUG, " attrbute dict: {}", obj_attrs_dict)
        mandatory_attrs_list = []
        '''
        for every attribute, execute following steps.
        1. for enum_attr type, update enum dict.
        2. for mandatory attribute, add "optional" field.
        3. Update mandatory_attrs_list for mandatory attrbute.
        '''
        attr_to_d_attrs_names_dict = dict()
        attr_prune_list = list()
        gen_d_dict = dict()
        for attr_name, attr_metadata in six.iteritems(obj_attrs_dict):
            gen_d_attrs_dict = dict()
            gen_d_attrs_dict["d_attrs_md"] = dict()
            gen_d_attrs_dict["d_attr_names"] = dict()
            gen_d_attrs_dict["d_attr_names"]["attr_name_list"] = []
            gen_d_attrs_dict["prune_list"] = []
            process_attribute_and_gen_d_attrs_dict(
                object_name, attr_name, attr_metadata, gen_d_attrs_dict)
            '''
            gen_d_attrs_dict["d_attrs_md"]  - this will have derived attrs
            gen_d_attrs_dict["d_attr_names"] - this is used to update key group list
            gen_d_attrs_dict["prune_list"] - this is to prune list
            '''

            gen_d_dict.update(gen_d_attrs_dict["d_attrs_md"])
            if len(gen_d_attrs_dict["d_attr_names"]["attr_name_list"]):
                attr_to_d_attrs_names_dict[attr_name] = gen_d_attrs_dict[
                    "d_attr_names"]
            attr_prune_list.extend(gen_d_attrs_dict["prune_list"])
            if ("is_mandatory" in attr_metadata
                    and attr_metadata["is_mandatory"]):
                if attr_name in attr_prune_list:
                    # update d_attrs to manadatory_attrs list instead of attr_name.
                    if gen_d_attrs_dict["d_attr_names"][
                            "need_catalog_type_input"]:
                        mandatory_attrs_list.append(
                            gen_d_attrs_dict["d_attr_names"]["attr_name_list"])
                    else:
                        mandatory_attrs_list.extend(
                            gen_d_attrs_dict["d_attr_names"]["attr_name_list"])
                else:
                    mandatory_attrs_list.append(attr_name)
                    attr_metadata["Optional"] = False
            else:
                attr_metadata["Optional"] = True
        #endfor
        object_metadata["mandatory_attributes"] = mandatory_attrs_list
        '''
        keep copy of object's attrs as exists in model.
        this is used by describe command.
        '''
        object_metadata["pre_process_obj_attrs"] = list(obj_attrs_dict)
        key_groups_list = []
        # fix up key-groups for all matching keys in attr_to_d_attrs_names_dict.
        if "key_groups" in object_metadata:
            key_groups_list = object_metadata["key_groups"]

        # delete attrs in prune list.
        for prune_attr in attr_prune_list:
            if key_groups_list and prune_attr in key_groups_list[0]:
                key_groups_list[0].remove(prune_attr)
                if attr_to_d_attrs_names_dict[prune_attr][
                        "need_catalog_type_input"]:
                    key_groups_list[0].append(attr_to_d_attrs_names_dict[
                        prune_attr]["attr_name_list"])
                else:
                    key_groups_list[0].extend(attr_to_d_attrs_names_dict[
                        prune_attr]["attr_name_list"])
            del obj_attrs_dict[prune_attr]

        # update d_attrs.
        obj_attrs_dict.update(gen_d_dict)
    #endfor
    return objects


def gen_merged_enum_dict():
    enum_dict = cli_inbuilt_types_enum_dict.copy()
    enum_dict.update(model_json_enum_type_dict)
    return enum_dict


def gen_cli_target_file(templates_dir, template_filename, target_filename,
                        generated_objects_dict):
    gencli_log_debug(LOG_LEVEL.DEBUG,
                     "generate target_file: {}, template_filename: {}",
                     target_filename, template_filename)
    with open(target_filename, "w") as f:
        render_template(
            f,
            template_filename,
            generated_objects_dict,
            templates_dir,
            prefix=tenjin_prefix)
    gencli_log_debug(LOG_LEVEL.DEBUG, "generated outfile successfully")


def gen_cli_clish_xml_output_file(template_dir, template_file, xml_out_dir,
                                  generated_objects_dict):
    '''
    Generate bf_switchapi.xml target file from template
    '''
    xml_target_filename = os.path.join(xml_out_dir, OUTPUT_XML_FILE_NAME)
    gen_cli_target_file(template_dir, template_file, xml_target_filename,
                        generated_objects_dict)


def gen_ptype_dict():
    ptype_dict = dict()
    ptype_dict["select"] = gen_merged_enum_dict()
    ptype_dict["uint"] = model_range_dict
    return ptype_dict


def generate_model_cli(args):
    # process model.json file, and populate user_ctx_object_dict
    template_file = args.template_file
    template_dir = args.template_dir
    json_filename = args.model_json
    xml_out_dir = args.xo
    gencli_log_debug(LOG_LEVEL.DEBUG, "Input: json_file_name: {}, xo: {}",
                     json_filename, xml_out_dir)
    objects_dict = gen_object_dict_from_model_json(json_filename)
    model_dict = dict()
    model_dict["__objects"] = objects_dict
    model_dict["__PTYPE"] = gen_ptype_dict()
    gencli_log_debug(LOG_LEVEL.DEBUG, "ptype_dict: {}", model_dict["__PTYPE"])
    gencli_log_debug(LOG_LEVEL.DEBUG, "final objects_dict: {}", objects_dict)
    gen_cli_clish_xml_output_file(template_dir, template_file, xml_out_dir,
                                  model_dict)


def construct_cmd_parser():
    parser = argparse.ArgumentParser(description='model cli generation')
    #Add arguments based on need
    parser.add_argument(
        '--template_dir',
        required=True,
        metavar='template_dir',
        type=str,
        help='Source directory of template file')
    parser.add_argument(
        '--template_file',
        required=True,
        metavar='template_file',
        type=str,
        help='Template file name')
    parser.add_argument(
        '--model_json',
        required=True,
        metavar='model_json',
        type=str,
        help='Path to the model json file')
    parser.add_argument(
        '--xo',
        required=True,
        metavar='xml_output_dir',
        type=str,
        help='Path to the xml output directory')
    parser.add_argument(
        '--enable_debug',
        required=False,
        action='store_true',
        help='enable debug')
    return parser


def gencli_log_debug(log_level, format, *args):
    logging_enabled = False
    if log_level == LOG_LEVEL.INFO:
        logging_enabled = True
    else:
        if enable_logging_level_debug:
            logging_enabled = True

    if logging_enabled:
        print(format.format(*args))


def process_cmd_args(args):
    global enable_logging_level_debug
    enable_logging_level_debug = args.enable_debug


def main():
    parser = construct_cmd_parser()
    args = parser.parse_args()
    process_cmd_args(args)
    generate_model_cli(args)


if __name__ == "__main__":
    main()
