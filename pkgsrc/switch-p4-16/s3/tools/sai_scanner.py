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

'''
This script reads all SAI attributes from saimetadata.c. It then reads
attributes and enums from the code and maps and gets a 2nd list.
The 2 lists are then compared to generate a list of supported SAI attrs
Usage: ./sai_scanner.py <sai_dir>
'''

import os
import sys
import re
from re import search

def read_file(filename):
    with open(filename) as data_file:
        data = json.load(data_file)
        return data

# get all object types in SAI
def get_object_types(lines):
    object_types = []
    start = False
    se = "sai_metadata_sai_object_type_t_enum_values"
    for line in lines:
        if line.strip().find(se) != -1:
            start = True
        if start == True:
            if line.strip() == "-1":
                break
            else:
                object_types.append(line.strip()[:-1])
    return object_types[2:]

# get attributes per object type
def get_attributes_per_object_type(ot, lines):
    attributes = []
    start = False
    # construct search string
    ot_short = ot[16:].lower()
    se = "sai_metadata_sai_" + ot_short + "_attr_t_enum_values[] = {"
    for line in lines:
        if line.strip().find(se) != -1:
            start = True
        if start == True:
            if line.strip() == "-1":
                break
            else:
                attributes.append(line.strip()[:-1])
    return attributes[1:]

# get all enum types
def get_all_enum_types(lines):
    skip_types = ["sai_log_level_t",
                  "sai_status_t",
                  "sai_common_api_t",
                  "sai_condition_operator_t",
                  "sai_enum_flags_type_t",
                  "sai_default_value_type_t"]
    enum_types = []
    start = False
    se = "_t_enum_values[] = {"
    for line in lines:
        if line.strip().endswith(se):
            if "attr" not in line.strip():
                enum_types.append(line.strip())
    et = ""
    enums = []
    enum_to_val_map = {}
    for line in lines:
        if line.strip() in enum_types:
            start = True
            et = line.strip().split()[1]
        if start == True:
            if line.strip() == "-1":
                enum_to_val_map[et] = enums[1:]
                enums = []
                start = False
                continue
            else:
                enums.append(line.strip()[:-1])

    for skip in skip_types:
        enum_to_val_map.pop(skip, None)

    return enum_to_val_map

def extract_attrs_and_enums(text, dump=False):
    attrs = []
    enums = []
    keys = {}
    for line in text.split("\n"):
      m = re.search('SAI_\w+', line)
      if m:
        val = m.group()
        if "Unsupported" in line:
            keys[val] = "n"
        else:
            keys[val] = "y"

    for v in keys.keys():
      n = re.search('_ATTR\w+', str(v))
      if n:
        attrs.append(str(v) + "," + keys[v])
      else:
        enums.append(str(v) + "," + keys[v])

    attrs.sort()
    enums.sort()
    if dump:
      print("PRINT ATTRS")
      for attr in attrs:
        print(attr)
      print("PRINT ENUMS")
      for enum in enums:
        print(enum)
    return attrs, enums

# read supported attributes from SAI code
def generate_attributes_supported_from_code(sai_dir, dump=False):
    print("Read supported attributes and enums from SAI implementation code")
    cmd = "ls " + sai_dir + "/sai*.cpp | xargs rgrep SAI_ | grep -v SAI_STATUS* | grep -v SAI_LOG* | grep -v SAI_ASS* | grep -v SAI_MALL* | grep -v attr_mapping_md_list*"
    text = os.popen(cmd).read()
    return extract_attrs_and_enums(text, dump)

# read supported attributes from SAI maps
# hacky but does the job
def generate_attributes_supported_from_maps(sai_dir, dump=False):
    print("Read supported attributes and enums from SAI map jsons")
    maps_dir = sai_dir + "/maps"
    cmd = "python3 " + sai_dir + "/../s3/tools/sai_map_gen.py " + maps_dir + " ."
    print("Executing SAI map gen: ", cmd)
    text = os.popen(cmd).read()

    cmd = "cat sai_map.cpp | grep -v SAI_STATUS* | grep -v attr_mapping_md_list*"
    text = os.popen(cmd).read()

    os.popen("rm sai_map.h")
    os.popen("rm sai_map.cpp")

    return extract_attrs_and_enums(text, dump)

# write data to file in markdown
# each attr is of the form SAI_VLAN_ATTR_VLAN_ID,Y
def write_data_to_md(ot_to_attr_map, attrs, enum_to_val_map, enums):
    out = open("sai.md", "w")
    out.write("# SAI supported attributes #\n\n")
    for ot in object_types:
        out.write("## " + ot + " ##\n\n")
        out.write("|Attribute|Supported|\n")
        out.write("|---------|---------|\n")
        y_attrs = ""
        n_attrs = ""
        for attr in ot_to_attr_map[ot]:
            if attr + ",y" in attrs:
                y_attrs += "| " + attr + "|Y|\n"
            else:
                n_attrs += "| " + attr + "|N|\n"
        out.write(y_attrs)
        out.write(n_attrs)
        out.write("\n\n")
    out.write("# SAI supported enum types #\n\n")
    for et in enum_to_val_map.keys():
        out.write("## " + et + " ##\n\n")
        out.write("|Enum|Supported|\n")
        out.write("|----|---------|\n")
        y_enums = ""
        n_enums = ""
        for enum in enum_to_val_map[et]:
            if enum + ",y" in enums:
                y_enums += "| " + enum + "|Y|\n"
            else:
                n_enums += "| " + enum + "|N|\n"
        out.write(y_enums)
        out.write(n_enums)
        out.write("\n\n")
    out.close()

# write data to file in csv
# each attr is of the form SAI_VLAN_ATTR_VLAN_ID,Y
def write_data_to_csv(ot_to_attr_map, attrs, enum_to_val_map, enums):
    out = open("sai.csv", "w")
    for ot in object_types:
        out.write(ot + "\n")
        y_attrs = ""
        n_attrs = ""
        for attr in ot_to_attr_map[ot]:
            if attr + ",y" in attrs:
                y_attrs += attr + ",Y\n"
            else:
                n_attrs += attr + ",N\n"
        out.write(y_attrs)
        out.write(n_attrs)
    for et in enum_to_val_map.keys():
        out.write(et + "\n")
        y_enums = ""
        n_enums = ""
        for enum in enum_to_val_map[et]:
            if enum + ",y" in enums:
                y_enums += enum + ",Y\n"
            else:
                n_enums += enum + ",N\n"
        out.write(y_enums)
        out.write(n_enums)

if __name__ == "__main__":
    arguments = sys.argv
    if (len(arguments) < 2):
        print("Usage: python3 sai_scanner.py <sai_dir>")
        exit(0)
    sai_dir = arguments[1]

    object_types = []
    ot_to_attr_map = {}
    enum_to_val_map = {}

    # read objects, attributes from SAI
    print("Create baseline from SAI")
    saimd_filename = sai_dir + "/saimetadata.c"
    with open(saimd_filename) as saimetadata_file:
        lines = saimetadata_file.readlines()
        object_types = get_object_types(lines)
        for ot in object_types:
            ot_to_attr_map[ot] = get_attributes_per_object_type(ot, lines)

        enum_to_val_map = get_all_enum_types(lines)

    attrs, enums = generate_attributes_supported_from_code(sai_dir)
    new_attrs, new_enums = generate_attributes_supported_from_maps(sai_dir)
    attrs = attrs + new_attrs
    enums = enums + new_enums
    write_data_to_md(ot_to_attr_map, attrs, enum_to_val_map, enums)
    write_data_to_csv(ot_to_attr_map, attrs, enum_to_val_map, enums)
