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

#!/usr/bin/python


import sys
import argparse
import json
import os

def get_field(data_obj, field):
    for key, data in data_obj.iteritems():
        if key == field:
            return data
    return None
#

def compose_supported_apis(supported_apis, dox_file):
    if len(supported_apis) != 0:
      dox_file.write("<H3>Supported BF-RT APIs</H3>\n\n")
      dox_file.write("<table>\n")
      #
      dox_file.write("<tr>")
      dox_file.write("<th><b>BF-RT API</b></th>")
      dox_file.write("<th><b>dev_tgt</b></th>")
      dox_file.write("<th><b>Description</b></th>")
      dox_file.write("</tr>")
      #
      if dict == type(supported_apis):
        for api_name, api_decl in supported_apis.iteritems():
          dox_file.write("<tr>")
          dox_file.write("<td><b>BfRtTable::" + api_name + "()</b></td>")
          #
          api_doc = ""
          if "target_attributes" in api_decl:
              if len(api_decl["target_attributes"]) != 0:
                  required_tgt = ""
                  rtgt_sep = ""
                  accepted_tgt = ""
                  atgt_sep = ""
                  for tgt_attr in api_decl["target_attributes"]:
                      if tgt_attr.endswith('_all'):
                          accepted_tgt += atgt_sep + tgt_attr
                          atgt_sep = ", "
                      else:
                          required_tgt += rtgt_sep + tgt_attr
                          rtgt_sep = ", "
                  #
                  if len(required_tgt) != 0:
                      api_doc += " requires (" + required_tgt + ")"
                  if len(accepted_tgt) != 0:
                      api_doc += " accepts (" + accepted_tgt + ")"
                  #
                  api_doc += "."
          #
          dox_file.write("<td>" + api_doc + "</td>")

          api_description = api_decl["description"] if "description" in api_decl else ""
          dox_file.write("<td>" + api_description + "</td>")
          dox_file.write("</tr>")
        # for apis
      #
      dox_file.write("</table>\n")
#

def compose_data_fields(dox_file, data_fields, action_name, table_group_id):
    # Data fields
    num_data_fields = len(data_fields) if len(data_fields) != 0 else 1
    action_id = str(action_name).replace("$", "").replace(".","_").upper()

    action_text = "@anchor " + table_group_id + "__" + action_id + " <b>'" + action_name + "'</b> action" if len(action_name) else ""
    if len(data_fields) > 0:
      i = 0
      for entry_obj in data_fields:
        dox_file.write("<tr>")
        if i == 0:
          dox_file.write("<td rowspan=\"" + str(num_data_fields) + "\">" + action_text + (" data" if len(action_name) else "Data") + "</td>")
        #
        i += 1
        field_attr = []
        field_mandatory = get_field(entry_obj, "mandatory")
        if field_mandatory:
          field_attr.append("M")
        #
        field_ro = get_field(entry_obj, "read_only")
        if field_ro:
          field_attr.append("R/O")
        #
        field_description = ""
        field_obj = get_field(entry_obj, "singleton")
        if field_obj == None:
          # Some jsons like tf2.pktgen.app_cfg have no singleton
          field_obj = entry_obj
        #
        field_name = get_field(field_obj, "name")
        dox_file.write("<td>")
        if field_name:
          field_id = str(field_name).replace("$", "").replace(".","_").upper()
          if len(action_id):
            field_id = action_id + "__" + field_id
          dox_file.write("@anchor " + table_group_id + "__" + field_id + "\n " + str(field_name))
        dox_file.write("</td>")
        #
        field_id = get_field(field_obj, "id")
        dox_file.write("<td>")
        dox_file.write(str(field_id) if field_id else "")
        dox_file.write("</td>")
        #
        field_rep = get_field(field_obj, "repeated")
        if field_rep:
          field_attr.append("R")
        #
        field_type = get_field(field_obj, "type")
        choices_str = ""
        if field_type:
            field_choices = get_field(field_type, "choices")
            if field_choices:
              choices_str = "choices:<br>" + ", ".join(field_choices)
            #
            field_type = get_field(field_type, "type")
        #
        dox_file.write("<td>")
        dox_file.write(str(field_type) if field_type else "")
        dox_file.write("</td>")
        #
        field_description = get_field(field_obj, "description")
        #
        dox_file.write("<td>")
        dox_file.write("; ".join(field_attr))
        if len(choices_str):
          dox_file.write("<br>" + choices_str)
        #
        dox_file.write("</td>")
        #
        dox_file.write("<td>")
        dox_file.write(str(field_description) if field_description else "")
        dox_file.write("</td>")
        #
        dox_file.write("</tr>")  # Data fields
      #
    else:
      # 'Command' - action without data fields
      dox_file.write("<tr>")
      dox_file.write("<td>" + action_text + "</td>")
      dox_file.write("<td></td>")
      dox_file.write("<td></td>")
      dox_file.write("<td></td>")
      dox_file.write("<td></td>")
      dox_file.write("<td></td>")
      dox_file.write("</tr>")  # Data Action fields
    #
# compose_data_fields()

def compose_key_fields(dox_file, key_fields):
    # Key fields
    if not key_fields:
      return
    #
    num_key_fields = len(key_fields) if len(key_fields) != 0 else 1
    if len(key_fields) > 0:
      i = 0
      for entry_obj in key_fields:
        dox_file.write("<tr style=\"background-color:lightgray;\">")
        if i == 0:
          dox_file.write("<td rowspan=\"" + str(num_key_fields) + "\"><b>Key</b></td>")
        #
        i += 1
        field_attr = []
        #
        field_name = get_field(entry_obj, "name")
        dox_file.write("<td>")
        dox_file.write(str(field_name) if field_name else "")
        dox_file.write("</td>")
        #
        field_id = get_field(entry_obj, "id")
        dox_file.write("<td>")
        dox_file.write(str(field_id) if field_id else "")
        dox_file.write("</td>")
        #
        field_mandatory = get_field(entry_obj, "mandatory")
        if field_mandatory:
          field_attr.append("M")
        #
        field_rep = get_field(entry_obj, "repeated")
        if field_rep:
          field_attr.append("R")
        #
        key_type = get_field(entry_obj, "match_type")
        if key_type:
          field_attr.append(str(key_type))
        #
        field_type = get_field(entry_obj, "type")
        choices_str = ""
        if field_type:
            field_choices = get_field(field_type, "choices")
            if field_choices:
              choices_str = "choices:<br>" + ", ".join(field_choices)
            #
            field_type = get_field(field_type, "type")
        #
        dox_file.write("<td>")
        dox_file.write(str(field_type) if field_type else "")
        dox_file.write("</td>")
        #
        dox_file.write("<td>")
        dox_file.write("; ".join(field_attr))
        if len(choices_str):
          dox_file.write("<br>" + choices_str)
        #
        dox_file.write("</td>")
        #
        field_description = get_field(entry_obj, "description")
        dox_file.write("<td>")
        dox_file.write(str(field_description) if field_description else "")
        dox_file.write("</td>")
        #
        dox_file.write("</tr>")  # Key fields
      #
    else:
        dox_file.write("<tr>")
        dox_file.write("<td><b>Key</b></td>")
        dox_file.write("<td></td>")
        dox_file.write("<td>N/A</td>")
        dox_file.write("<td>N/A</td>")
        dox_file.write("<td>N/A</td>")
        dox_file.write("<td>N/A</td>")
        dox_file.write("</tr>")  # Key fields
    #
# compose_key_fields

def compose_table_layout(key_fields, common_data_fields, action_specs, dox_file, table_group_id):
    if key_fields or common_data_fields or action_specs:
        dox_file.write("<H3>Table Layout</H3>\n\n")
        dox_file.write("<table>\n")

        # Header
        dox_file.write("<tr>")
        dox_file.write("<th></td>")
        dox_file.write("<th><b>Name</b></th>")
        dox_file.write("<th><b>Id</b></th>")
        dox_file.write("<th><b>Type</b></th>")
        dox_file.write("<th><b>Read-Only(R/O),<br>Repeated(R),<br>Mandatory(M)</b></th>")
        dox_file.write("<th><b>Description</b></th>")
        dox_file.write("</tr>")
        # TODO: annotations

        compose_key_fields(dox_file, key_fields)

        has_actions = False
        # Data fields for Actions, if any.
        if action_specs and len(action_specs):
          has_actions = True
          for action_obj in action_specs:
            action_name = get_field(action_obj, "name")
            if not action_name or len(str(action_name)) == 0:
              action_id = get_field(action_obj, "id")
              action_id = str(action_id) if action_id else "NA"
              action_name = "action_id_" + action_id
            #
            action_data = get_field(action_obj, "data")
            compose_data_fields(dox_file, action_data, action_name, table_group_id)
          #
        #

        # Common Data Fields
        if common_data_fields and len(common_data_fields):
          compose_data_fields(dox_file, common_data_fields, "", table_group_id)
        #
        dox_file.write("</table>\n")
#

def process_table(table_data, dox_file, module_group, chip_group):
    # Table name
    ori_table_name = get_field(table_data, "name")
    table_id = ori_table_name.replace("$", "")
    table_id = table_id.replace(".", "_").upper()

    node_id = ""
    table_name_a = ori_table_name.split('.', 3);
    # Make sure device and module prefixes are in the group id composed from json file name
    # even if a table name doesn't contain the same.
    table_group_id = chip_group + "_" + module_group + "_" + table_id  # TF1_PORT_PORT
    #
    if len(table_name_a) >= 3 and table_name_a[0].upper() == chip_group and table_name_a[1].upper() == module_group:
        # Create a tree node group for tables with more than 3 layers deep, e.g.
        # tf1.tm.counter.dpg under tf1.tm.counter node group as tf1.tm.counter.dpg table group.
        # Tables like tf1.tm.cfg table are kept under their device.module nodes.
        # Do not introduce more than thee layers for now.
        if len(table_name_a) == 3:
          # Top level table without a node, e.g. tf1.tm.cfg
          table_group_id = table_id  # TF1_TM_CFG
        else:
          node_id = chip_group + "_" + module_group + "_" + table_name_a[2].upper()  # TF1_TM_PORT
          table_group_id = node_id + "_" + table_name_a[3].upper().replace(".","_")  # TF1_PORT_PORT_CFG
          #
          node_title = table_name_a[0] + '.' + table_name_a[1] + '.' + table_name_a[2]
          dox_file.write(" /** @addtogroup " + node_id + " " + node_title + " object \n")
          dox_file.write(" * @{\n")
          dox_file.write(" */\n\n")  # Wrap table to the node
        #
    #
    table_group_title = ori_table_name
    if table_group_title[0] == '$':
      # Prepend an old-fashioned table name (like $PORT) with chip-family text
      # to align its group title at the doxygen TOC with other dot-prefixed tables
      # (like 'tf1.tm.port.port_cfg'), and avoid confusion with the actual table name.
      table_group_title = chip_group.lower() + ": " + table_group_title  # "tf1: $PORT"
    #
    # Each Table without chip.module name prefix goes into its own group.
    dox_file.write("/**\n")
    dox_file.write(" * @defgroup " + table_group_id + " " + table_group_title + " table\n")

    table_desc = get_field(table_data, "description")
    if table_desc:
      dox_file.write("\n * @brief " + table_desc + "\n\n")
    #

    dox_file.write("<H3>Table properties</H3>\n\n")
    dox_file.write("<table>\n")
    #
    dox_file.write("<tr>")
    dox_file.write("<th><b>Table name</b></th>")
    dox_file.write("<th><b>Table size</b></th>")
    dox_file.write("<th><b>Table type</b></th>")
    dox_file.write("</tr>")
    #
    dox_file.write("<tr>")
    dox_file.write("<td><b>" + ori_table_name + "</b></td>")
    #
    dox_file.write("<td>")
    table_size = get_field(table_data, "size")
    if table_size:
        dox_file.write(str(table_size))
    dox_file.write("</td>")
    #
    dox_file.write("<td>");
    table_type = get_field(table_data, "table_type")
    if table_type:
        dox_file.write(str(table_type))
    dox_file.write("</td>")
    dox_file.write("</tr>")
    dox_file.write("</table>\n")

    key_fields = get_field(table_data, "key")
    data_fields = get_field(table_data, "data")
    action_specs = get_field(table_data, "action_specs")
    compose_table_layout(key_fields, data_fields, action_specs, dox_file, table_group_id)

    supported_apis = get_field(table_data, "supported_apis")
    if supported_apis != None:
        compose_supported_apis(supported_apis, dox_file)
    #

    dox_file.write(" * @{\n")
    dox_file.write(" */\n")
    dox_file.write("/** @} */\n\n")

    if len(node_id):
      dox_file.write("/** @} */\n")  # close node group
    #
#

def json_to_dox(json_file, dox_file, module_group, chip_group):
    data = json.load(json_file)

    table_names = []
    for table in data["tables"]:
        table_names.append(table["name"])
    #
    table_names.sort()
    #
    for table_name in table_names:
        table_obj = None
        for table in data["tables"]:
            if table["name"] == table_name:
                table_obj = table
                break
        #
        assert table_obj != None
        process_table(table, dox_file, module_group, chip_group)
    #
#

# Convert JSON file suffix to Tofino chip name used as a doxygen @defgroup
chip_suffix_to_defgroup = { 'tf1': 'Tofino',
                            'tf2': 'Tofino-2',
                            'tf3': 'Tofino-3',
                            'all': 'Tofino family'
                          }
main_group = "BFRT_FF"

def parse_arguments():
    parser = argparse.ArgumentParser(description = "BFRT fixed function JSON doxygen filter.", add_help = True)
    parser.add_argument('in_file', metavar = 'FNAME', type = str,
                        help="BFRT fixed function's JSON file to process")
    args = vars(parser.parse_args())
    return args

def main():
    args = parse_arguments()

    dox_file = sys.stdout
    json_file = open(args['in_file'])

    # Decompose json file name.
    # e.g. with src/bf_rt/bf_rt_tm/bf_rt_tm_tf1.json
    #
    in_path_fname = os.path.splitext(args['in_file'])           # src/bf_rt/bf_rt_tm/bf_rt_tm_tf1
    in_path_module, in_fname = os.path.split(in_path_fname[0])  # (src/bf_rt/bf_rt_tm, bf_rt_tm_tf1)
    in_path, in_module = os.path.split(in_path_module)          # (src/bf_rt, bf_rt_tm)

    module_id, chip_id = tuple(in_fname.rsplit('_', 1))     #  (bf_rt_tm, tf1)
    module_group = module_id.split('bf_rt_', 1)[1].upper()  #  tm --> TM

    chip_id = chip_id if chip_id in chip_suffix_to_defgroup else 'all'
    chip_group = chip_id.upper()
    chip_name  = chip_suffix_to_defgroup[chip_id]

    # Compose JSON file for doxygen's source parser.
    #
    dox_file.write("/* THIS FILE IS AUTOGENERATED. DO NOT MODIFY MANUALLY! */\n\n")
    dox_file.write("/** @addtogroup " + main_group + " BF-RT Fixed Function Tables\n")
    dox_file.write(" * @{\n")
    dox_file.write(" */\n\n")

    # Create groups for top two node levels: chip and driver's module, e.g. tf1.tm
    # Doxygen will merge these groups from different modules: port, tm, mirror, etc.
    group_id = chip_group
    group_title = chip_name
    dox_file.write("/** @addtogroup " + group_id + " " + group_title + "\n")  #  TF1 'Tofino'
    dox_file.write(" * @{\n")
    dox_file.write(" */\n\n")

    group_id = chip_group + "_" + module_group
    group_title = chip_name + " Driver " + module_group + " tables"
    dox_file.write("/** @defgroup " + group_id + " " + group_title + "\n")  # TF1_TM 'Tofino Driver TM tables'
    dox_file.write(" * @{\n")
    dox_file.write(" */\n\n")

    json_to_dox(json_file, dox_file, module_group, chip_group)  # TM TF1

    dox_file.write("/** @} */\n")  # module group
    #
    dox_file.write("/** @} */\n")  # chip group
    #
    dox_file.write("/** @} */\n")  # main group

    #  json_file.close()  # no need to close stdin
    dox_file.close()
    #

if __name__ == "__main__":
    main()
