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
import os.path
import six
import sys
from tenjin_util import render_template

tenjin_prefix = "//::"

def read_file(filename):
    with open(filename) as data_file:
        data = json.load(data_file)
        return data

def filter_data(data):
    objects = data["objects"]
    filtered_dict = dict()
    for object_name, object_attributes in six.iteritems(objects):
        if object_attributes["class"] == "user":
            filtered_dict[object_name] = object_attributes["attributes"], object_attributes["object_type"]
    return filtered_dict


def prune_file(filename):
    file_dict = read_file(filename)
    filtered_dict = filter_data(file_dict)
    return filtered_dict

def get_object_names(filename, prefix):
    file_dict = read_file(filename)
    objects = file_dict["objects"]
    filtered_dict = dict()
    names = [prefix + "output.cpp"]
    for object_name, object_attributes in six.iteritems(objects):
        if object_attributes["class"] == "user":
            names.append(prefix + object_name + ".c")
            names.append(prefix + object_name + ".h")
    print(' '.join(names))

def generate_type_convertor_dict():
    d = dict()
    d["SWITCH_TYPE_BOOL"] = "bool"
    d["SWITCH_TYPE_UINT8"] = "uint8_t"
    d["SWITCH_TYPE_UINT16"] = "uint16_t"
    d["SWITCH_TYPE_UINT32"] = "uint32_t"
    d["SWITCH_TYPE_UINT64"] = "uint64_t"
    d["SWITCH_TYPE_ENUM"] = ""
    d["SWITCH_TYPE_MAC"] = "switch_mac_addr_t"
    d["SWITCH_TYPE_STRING"] = "switch_string_t"
    d["SWITCH_TYPE_RANGE"] = "switch_range_t"
    d["SWITCH_TYPE_OBJECT_ID"] = "switch_object_id_t"
    d["SWITCH_TYPE_LIST"] = "switch_attr_list_t"
    d["SWITCH_TYPE_IP_ADDRESS"] = "switch_ip_address_t"
    d["SWITCH_TYPE_IP_PREFIX"] = "switch_ip_prefix_t"
    return d


def get_union_field_dict():
    d = dict()
    d["SWITCH_TYPE_BOOL"] = "booldata"
    d["SWITCH_TYPE_UINT8"] = "u8"
    d["SWITCH_TYPE_UINT16"] = "u16"
    d["SWITCH_TYPE_UINT32"] = "u32"
    d["SWITCH_TYPE_UINT64"] = "u64"
    d["SWITCH_TYPE_ENUM"] = "enumdata.enumdata"
    d["SWITCH_TYPE_MAC"] = "mac"
    d["SWITCH_TYPE_STRING"] = "text"
    d["SWITCH_TYPE_OBJECT_ID"] = "oid"
    d["SWITCH_TYPE_LIST"] = "list"
    d["SWITCH_TYPE_IP_ADDRESS"] = "ipaddr"
    d["SWITCH_TYPE_IP_PREFIX"] = "ipprefix"
    d["SWITCH_TYPE_RANGE"] = "range"
    return d

def object_generator(prune_dict):
    result_dict = get_object_dict(prune_dict)
    for objectname, (counter, _) in six.iteritems(result_dict):
            yield objectname, counter
def object_generator_with_type(prune_dict):
    result_dict = get_object_dict(prune_dict)
    for objectname, (counter, object_type) in six.iteritems(result_dict):
            yield objectname, counter, object_type

def get_object_dict(prune_dict):
    result = dict()
    for objectname, helper in six.iteritems(prune_dict):
        if objectname != "_engine" and objectname != "include":
            attributes, object_type = helper
            counter = 0
            for key, value in six.iteritems(attributes):
                if key != "_engine" and key != "__filename" and key != "include" and key != "__object_type":
                    if "is_internal" in value and value["is_internal"]:
                        continue
                    if "is_read_only" in value and value["is_read_only"]:
                        continue
                    counter += 1
            result[objectname] = counter, object_type
    return result

def attribute_generator(prune_dict, isEnumMode=False):
    for filename, helper in six.iteritems(prune_dict):
        if filename != "_engine" and filename != "include":
            attributes, _ = helper
            attributes["__filename"] = filename
            type_convertor_dict = generate_type_convertor_dict()
            union_field_dict = get_union_field_dict()
            for key, value in six.iteritems(attributes):
                if key != "_engine" and key != "__filename" and key != "include" and key != "__object_type":
                    if "is_internal" in value and value["is_internal"]:
                        continue
                    if "is_read_only" in value and value["is_read_only"]:
                        continue
                    type_info = value["type_info"]
                    attribute_id = value["attr_id"]
                    type = type_info["type"]
                    if "default_value" in type_info and type_info["default_value"]:
                        default = type_info["default_value"]
                    else:
                       if type == "SWITCH_TYPE_IP_ADDRESS" or type == 'SWITCH_TYPE_IP_PREFIX':
                         default = "'0.0.0.0'"
                       elif type == "SWITCH_TYPE_MAC":
                         default = "'00:00:00:00:00:00'"
                       elif type == "SWITCH_TYPE_STRING":
                         default = ""
                       elif type == "SWITCH_TYPE_LIST":
                         default = []
                       else:
                         default = 0
                    if isEnumMode:
                        if type == "SWITCH_TYPE_ENUM":
                            enums = type_info["enum"]
                            yield enums, key, filename
                    else:
                        if type == "SWITCH_TYPE_ENUM":
                            new_type = "switch_" + attributes["__filename"] + "_" + key + "_t"
                        else:
                            if type not in type_convertor_dict:
                                raise Exception("the type" + type + "is not supported")
                            new_type = type_convertor_dict[type]
                        if type not in union_field_dict:
                            raise Exception("the type" + type + "is not supported")

                        field = union_field_dict[type]
                        yield new_type, key, default, type, field, attribute_id, filename

def convert_struct_type(type, key):
    if type == "switch_object_id_t":
       return " struct_object_copy." + key + " = { .data = static_cast<uint64_t> (structobject." + key + ")};"
    elif type =="switch_mac_addr_t":
       return "switch_string_to_mac(structobject." + key + ", struct_object_copy." + key +".mac);"
    else:
       return " struct_object_copy." + key + " = static_cast<" + type + "> (structobject." + key + ");"

def convert_struct_type_getter(type, classType, key):
    if type == "switch_object_id_t":
       return "returnValue.value = static_cast <uint64_t> (value.data);"
    elif classType == "SWITCH_TYPE_ENUM":
        return "returnValue.value =  (thrift_" + type + "::type) value;"
    elif type == "switch_mac_addr_t":
        return 'returnValue.value.clear();char mac_str[18]; snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", value.mac[0], value.mac[1], value.mac[2], value.mac[3], value.mac[4], value.mac[5]);returnValue.value.append(mac_str, 17);'
    else:
       return "returnValue.value = static_cast<thrift_" + type + "> (value);"

def convert_struct_type_setter(type, key):
    if type == "switch_object_id_t":
       return "convertedKey = {.data = static_cast<uint64_t> (" + key + ")};"
    elif type == "switch_mac_addr_t":
       return "switch_string_to_mac(" + key + ", convertedKey.mac);"
    else:
       return "convertedKey = static_cast<" + type + "> (" + key +  ");"


def generate_templates_onefile(json_filename, templates_dir,
                               template_name, output_name):
    prune_dict = prune_file(json_filename)
    with open(output_name, "w") as f:
        render_template(
            f, template_name, prune_dict, templates_dir, prefix=tenjin_prefix)

def get_types(json_filename):
    pruned_dict = prune_file(json_filename)
    switch_types = set()
    for object_name, (sub_dict, _) in six.iteritems(pruned_dict):
        for _, value in six.iteritems(sub_dict):
            type_info = value["type_info"]
            type = type_info["type"]
            switch_types.add(type)
    return switch_types


if __name__ == "__main__":
    arguments = sys.argv
    json_file = arguments[1]
    template_directory = arguments[2]
    template_file = arguments[3]
    output_file = arguments[4]
    generate_templates_onefile(json_file, template_directory, template_file, output_file)
    '''
    generate_templates_onefile(json_file, template_directory, output_directory, "template.h", "switch_model_api.h")
    generate_templates_onefile(json_file, template_directory, output_directory, "template.c", "switch_model_api.c")
    generate_templates_onefile(json_file, template_directory, output_directory,
                           "template.thrift", "switch_model_api.thrift")
    generate_templates_onefile(json_file, template_directory, output_directory,
                           "template.cpp", "switch_model_api_thrift.cpp")
    '''
