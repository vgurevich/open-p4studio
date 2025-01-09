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
import json, os, six, sys

map_cpp_ofile_name = "sai_map.cpp"
map_h_ofile_name = "sai_map.h"

map_h_text = "// This file is auto-generated. Do not edit manually\n\n"
map_h_text += "#ifndef __SAI_MAP_H__\n"
map_h_text += "#define __SAI_MAP_H__\n\n"

map_cpp_text = "// This file is auto-generated. Do not edit manually\n\n"
map_cpp_text += "#include \"saiinternal.h\"\n\n"


sai_to_sw_object_mapping_var_name = "sai_sw_obj_mapping_md_list"
sai_to_sw_object_mapping_decalarion_text = "const sai_to_sw_object_mapping_metadata_t " +\
                                            sai_to_sw_object_mapping_var_name

all_obj_attrs_text = \
   sai_to_sw_object_mapping_decalarion_text + "[]" + " = { \n"

num_sai_objs = 0

# list all sai global enums like "packet_action" because name format is different
# all enums format: SAI_OBJ_NAME_ATTR_NAME_ENUM_NAME, but 
# global enums format: SAI_ATTR_NAME_ENUM_NAME 

def read_file(filename):
    with open(filename) as data_file:
        data = json.load(data_file)
        return data

def add_prefix():
    global map_h_text
    map_h_text += "typedef int32_t sai_enum_data_t;\n\n"
    map_h_text += "typedef uint64_t sw_enum_data_t;\n\n"

    # gen sai_to_sw_enum_mapper_t struct
    map_h_text += "typedef struct sai_to_sw_enum_mapper_s {\n"
    map_h_text += "  sai_enum_data_t sai_enum;\n"
    map_h_text += "  sw_enum_data_t sw_enum;\n"
    map_h_text += "} sai_to_sw_enum_mapper_t;\n\n"

    # gen sai_to_sw_attr_mapping_md_t struct
    map_h_text += "typedef struct sai_to_sw_attr_mapping_md_s {\n"
    map_h_text += "  sai_attr_id_t sai_attr_id;\n"
    map_h_text += "  switch_attr_id_t swi_attr_id;\n"
    map_h_text += "  uint32_t num_enums;\n"
    map_h_text += "  const sai_to_sw_enum_mapper_t * const enum_mapper_list;\n"
    map_h_text += "} sai_to_sw_attr_mapping_md_t;\n\n"

    # gen sai_to_sw_object_mapping_metadata_t struct
    map_h_text += "typedef struct sai_to_sw_object_mapping_metadata_s {\n"
    map_h_text += "  sai_object_type_t sai_ot; \n"
    map_h_text += "  switch_object_type_t sw_ot; \n"
    map_h_text += "  uint32_t num_attrs;\n"
    map_h_text += "  const sai_to_sw_attr_mapping_md_t * const sai_sw_attr_mapper_list;\n"
    map_h_text += "} sai_to_sw_object_mapping_metadata_t ;\n\n"


def map_attributes(file_dict):
    global map_cpp_text, all_obj_attrs_text, num_sai_objs

    has_version = False
    if "version" in file_dict.keys():
        has_version = True
        for key in file_dict.keys():
            if key == "version":
                sai_version = file_dict["version"]
            else:
                sai_ot = key
    else:
        sai_ot = list(file_dict.keys())[0]

    print(file_dict.keys())
    sai_fqn = "SAI_OBJECT_TYPE_" + sai_ot.upper()
    sai_object = file_dict[sai_ot]
    swi_ot = sai_object["switch"]
    swi_fqn = "SWITCH_OBJECT_TYPE_" + swi_ot.upper()
    sai_to_sw_attrs = sai_object["attributes"]
    has_enum_mapping = False
    if "enums" in sai_object:
      sai_to_sw_enum_attrs = sai_object["enums"]
      has_enum_mapping = True

    all_attrs_text = ""
    if has_version:
        all_attrs_text += "#if SAI_API_VERSION >= " + sai_version + "\n"

    var_prefix = "sai_sw_"
    attr_mapping_list_md_var_name =  var_prefix + "ot_SAI_" + sai_ot.upper() + "_attr_mapping_md_list"
    all_attrs_text += "static const sai_to_sw_attr_mapping_md_t " + attr_mapping_list_md_var_name + \
                 "[] = {\n";
    num_attrs = 0
    for sai_attr, sw_attr in six.iteritems(sai_to_sw_attrs):
      sai = "SAI_" + sai_ot.upper() + "_ATTR_" + sai_attr.upper()
      swi = "SWITCH_" + swi_ot.upper() + "_ATTR_" + sw_attr.upper()

      num_enums = 0
      if "enums" in sai_object and sai_attr in sai_object["enums"]:
        # sai_attr has enum mapping. time to gen enum mapper
        sai_to_sw_enum_mapper = sai_object["enums"][sai_attr]

        # Note:
        #  SAI enum name format:
        #    global enums name format: SAI_<ATTR_NAME>_<ENUM_NAME>
        #      if global_enum differs from attr enum name, use enum_to_global conversion
        #    attr enums name format: SAI_<OBJ_NAME>_<ATTR_NAME>_<ENUM_NAME>       
        sai_enum_attr = sai_attr
        if "global_enums" in sai_object and sai_enum_attr in sai_object["global_enums"]:
          if "enum_to_global" in sai_object and sai_enum_attr in sai_object["enum_to_global"]:
            sai_enum_name_prefix = "SAI_"+ sai_object["enum_to_global"][sai_enum_attr].upper()+"_"
          else:
            sai_enum_name_prefix = "SAI_"+ sai_enum_attr.upper()+"_"
        else:
          sai_enum_name_prefix = "SAI_"+ sai_ot.upper() + "_" + sai_enum_attr.upper()+"_"

        sw_enum_name_prefix = swi + "_"
        attr_enum_mapper_var_name  = var_prefix + "enum_mapper_attr_SAI_" + sai_ot.upper() + \
                          "_ATTR_" + sai_enum_attr.upper()
        if has_version:
            map_cpp_text += "#if SAI_API_VERSION >= " + sai_version + "\n"
        map_cpp_text += "static const sai_to_sw_enum_mapper_t " + attr_enum_mapper_var_name + "[] = {\n"; 
        num_enums = 0
        # gen enum mapper for attribute_type SWITCH_TYPE_ENUM
        for sai_enum, sw_enum in six.iteritems(sai_to_sw_enum_mapper):
          sai_enum_name = sai_enum_name_prefix + sai_enum.upper()
          sw_enum_name = sw_enum_name_prefix + sw_enum.upper()
          map_cpp_text += "  {.sai_enum = " + sai_enum_name + ", .sw_enum = " + sw_enum_name  + "},\n"
          num_enums += 1
        map_cpp_text += "};\n"
        if has_version:
            map_cpp_text += "#endif" + "\n"

      # gen sw attr mapper for every sai attribute.
      enum_mapper_list = "NULL"
      if num_enums:
        enum_mapper_list = "&" + attr_enum_mapper_var_name + "[0]"
      all_attrs_text += "  {.sai_attr_id = " + sai + ","\
                "\n   .swi_attr_id = " +  swi + ","\
                "\n   .num_enums = " +  str(num_enums) + ","\
                "\n   .enum_mapper_list = " + enum_mapper_list  + "},\n"
      num_attrs += 1
    all_attrs_text += "};\n"

    if has_version:
        all_attrs_text += "#endif" + "\n"
        all_attrs_text += "\n"

    map_cpp_text += all_attrs_text
    num_sai_objs += 1
    if has_version:
        all_obj_attrs_text += "#if SAI_API_VERSION >= " + sai_version + "\n"
    all_obj_attrs_text += "  {.sai_ot = " + sai_fqn + "," \
                "\n   .sw_ot = " +  swi_fqn + ","\
                "\n   .num_attrs = " +  str(num_attrs) + ","\
                "\n   .sai_sw_attr_mapper_list = " + "&" + attr_mapping_list_md_var_name + "[0]"  + "},\n"
    if has_version:
        all_obj_attrs_text += "#endif" + "\n"

def process_directory(directory):
    global map_cpp_text, map_h_text, all_obj_attrs_text, num_sai_objs
    add_prefix()
    for file in os.listdir(src_dir):
        if file.endswith(".json"):
            data = read_file(os.path.join(src_dir, file))
            map_attributes(data)
    all_obj_attrs_text += "};\n\n"
    # gen extern for sai_sw_obj_mapping_md_list
    map_h_text += "extern " + sai_to_sw_object_mapping_decalarion_text +  \
                  "[" + str(num_sai_objs) + "];\n\n"

    map_cpp_text += all_obj_attrs_text 
    map_h_text += "#endif // __SAI_MAP_H__\n"

if __name__ == "__main__":
    arguments = sys.argv
    src_dir = arguments[1]
    output_dir = arguments[2]
    process_directory(src_dir)
    map_cpp_target_filename = os.path.join(output_dir, map_cpp_ofile_name)
    map_h_target_filename = os.path.join(output_dir, map_h_ofile_name)

    with open(map_cpp_target_filename, 'w') as ofile:
        ofile.write(map_cpp_text)
    with open(map_h_target_filename, 'w') as ofile:
        ofile.write(map_h_text)
