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

#!/usr/bin/env python
# A simple script to generate boilerplate code from bf-rt.json for a p4 table
#
# Usage: python gen_auto_class.py <path-to-bf-rt.json>  <table_name>
#
# Output:
'''
pipe.SwitchIngress.lag.lag_selector_sel
 - Copy to bf_rt_ids.h
  static bf_rt_table_id_t T_INGRESS_LAG_SELECTOR_SEL;
  static bf_rt_field_id_t F_INGRESS_LAG_SELECTOR_SEL_SELECTOR_GROUP_ID;
  static bf_rt_field_id_t D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_ID;
  static bf_rt_field_id_t D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_STATUS;
  static bf_rt_field_id_t D_INGRESS_LAG_SELECTOR_SEL_MAX_GROUP_SIZE;

 - Copy to bf_rt_ids.cpp
  T_INGRESS_LAG_SELECTOR_SEL = table_id_from_name("pipe.SwitchIngress.lag.lag_selector_sel");
  F_INGRESS_LAG_SELECTOR_SEL_SELECTOR_GROUP_ID = key_id_from_name(T_INGRESS_LAG_SELECTOR_SEL, "$SELECTOR_GROUP_ID");
  D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_ID = data_id_from_name_noaction(T_INGRESS_LAG_SELECTOR_SEL, "$ACTION_MEMBER_ID");
  D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_STATUS = data_id_from_name_noaction(T_INGRESS_LAG_SELECTOR_SEL, "$ACTION_MEMBER_STATUS");
  D_INGRESS_LAG_SELECTOR_SEL_MAX_GROUP_SIZE = data_id_from_name_noaction(T_INGRESS_LAG_SELECTOR_SEL, "$MAX_GROUP_SIZE");

 - Copy to bf_rt_ids.cpp
  bf_rt_table_id_t smi_id::T_INGRESS_LAG_SELECTOR_SEL;
  bf_rt_field_id_t smi_id::F_INGRESS_LAG_SELECTOR_SEL_SELECTOR_GROUP_ID;
  bf_rt_field_id_t smi_id::D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_ID;
  bf_rt_field_id_t smi_id::D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_STATUS;
  bf_rt_field_id_t smi_id::D_INGRESS_LAG_SELECTOR_SEL_MAX_GROUP_SIZE;

 - Copy to the tna file
class ingress_lag_selector_sel : public p4_object_match_action {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_INGRESS_LAG_SELECTOR_SEL;
  static const switch_attr_id_t parent_attr_id = SWITCH_INGRESS_LAG_SELECTOR_SEL_ATTR_PARENT_HANDLE;
  static const switch_attr_id_t status_attr_id = SWITCH_INGRESS_LAG_SELECTOR_SEL_ATTR_STATUS;

 public:
  ingress_lag_selector_sel(const switch_object_id_t parent, switch_status_t &status)
      : p4_object_match_action(smi_id::T_INGRESS_LAG_SELECTOR_SEL,
                               status_attr_id,
                               auto_ot,
                               parent_attr_id,
                               parent) {
    status |= match_key.set_exact(smi_id::F_INGRESS_LAG_SELECTOR_SEL_SELECTOR_GROUP_ID, );

    action_entry.init_indirect_data();
    status |= action_entry.set_arg(smi_id::D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_ID, );
    status |= action_entry.set_arg(smi_id::D_INGRESS_LAG_SELECTOR_SEL_ACTION_MEMBER_STATUS, );
    status |= action_entry.set_arg(smi_id::D_INGRESS_LAG_SELECTOR_SEL_MAX_GROUP_SIZE, );
  }
};
'''
from __future__ import print_function
import json
import os.path
import sys

def read_file(filename):
    with open(filename) as data_file:
        data = json.load(data_file)
        return data

if __name__ == "__main__":
    arguments = sys.argv
    json_file = arguments[1]
    table_name = arguments[2]
    print(table_name)
    bf_rt_ids_h = ''
    bf_rt_ids_cpp1 = ''
    bf_rt_ids_cpp2 = ''
    tna_cpp = ''
    json_data = read_file(json_file)
    for table in json_data["tables"]:
      if table['name'] == table_name:
        table_split = table['name'].split('.')
        upper_table_name = ''
        if table_split[1] == 'SwitchIngress':
          lower_table_name = ('ingress_'+table_split[-1])
        else:
          lower_table_name = ('egress_'+table_split[-1])
        upper_table_name = lower_table_name.upper()
        final_table_name = 'T_'+upper_table_name
        bf_rt_ids_h += '  static bf_rt_table_id_t '+final_table_name+';\n'
        bf_rt_ids_cpp1 += '  '+final_table_name+' = table_id_from_name("'+table['name']+'");\n'
        bf_rt_ids_cpp2 += '  bf_rt_table_id_t smi_id::'+final_table_name+';\n'
        tna_cpp = 'class '+lower_table_name+' : public p4_object_match_action {\n'
        tna_cpp += ' private:\n  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_'+upper_table_name+';\n'
        tna_cpp += '  static const switch_attr_id_t parent_attr_id = SWITCH_'+upper_table_name+'_ATTR_PARENT_HANDLE;\n'
        tna_cpp += '  static const switch_attr_id_t status_attr_id = SWITCH_'+upper_table_name+'_ATTR_STATUS;\n'
        tna_cpp += '\n public:\n  '+lower_table_name+'(const switch_object_id_t parent, switch_status_t &status)\n'
        tna_cpp += '      : p4_object_match_action(smi_id::'+final_table_name+',\n'
        tna_cpp += '                               status_attr_id,\n'
        tna_cpp += '                               auto_ot,\n'
        tna_cpp += '                               parent_attr_id,\n'
        tna_cpp += '                               parent) {\n'
        for key in table['key']:
          key_name = key['name'].replace('$', '')
          upper_key_name = key_name.replace('.', '_').upper()
          final_key_name = 'F_'+upper_table_name+'_'+upper_key_name
          bf_rt_ids_h += '  static bf_rt_field_id_t '+final_key_name+';\n'
          bf_rt_ids_cpp1 += '  '+final_key_name+' = key_id_from_name('+final_table_name+', "'+key['name']+'");\n'
          bf_rt_ids_cpp2 += '  bf_rt_field_id_t smi_id::'+final_key_name+';\n'
          if key['match_type'] == 'Exact':
            tna_cpp += '    status |= match_key.set_exact(smi_id::'+final_key_name+', );\n'
          if key['match_type'] == 'Ternary':
            tna_cpp += '    status |= match_key.set_ternary(smi_id::'+final_key_name+', );\n'
        if 'action_specs' in table.keys():
          for ap in table['action_specs']:
            upper_ap_name = ap['name'].split('.')[-1].replace('.', '_').upper()
            if upper_ap_name == 'NOACTION':
              continue
            final_ap_name = 'A_'+upper_ap_name
            bf_rt_ids_h += '  static bf_rt_action_id_t '+final_ap_name+';\n'
            bf_rt_ids_cpp1 += '  '+final_ap_name+' = action_id_from_name('+final_table_name+', "'+ap['name']+'");\n'
            bf_rt_ids_cpp2 += '  bf_rt_action_id_t smi_id::'+final_ap_name+';\n'
            tna_cpp += '\n    action_entry.init_action_data(smi_id::'+final_ap_name+');\n'
            for data in ap['data']:
              final_data_name = 'D_'+upper_ap_name+'_'+data['name'].upper()
              bf_rt_ids_h += '  static bf_rt_field_id_t '+final_data_name+';\n'
              bf_rt_ids_cpp1 += '  '+final_data_name+' = data_id_from_name('+final_table_name+', '+final_ap_name+', "'+data['name']+'");\n'
              bf_rt_ids_cpp2 += '  bf_rt_field_id_t smi_id::'+final_data_name+';\n'
              tna_cpp += '    status |= action_entry.set_arg(smi_id::'+final_data_name+', );\n'
        if 'data' in table.keys():
          tna_cpp += '\n    action_entry.init_indirect_data();\n'
          for data in table['data']:
            data_name = data['singleton']['name'].replace('$', '')
            final_data_name = 'D_'+upper_table_name+'_'+data_name.upper()
            bf_rt_ids_h += '  static bf_rt_field_id_t '+final_data_name+';\n'
            bf_rt_ids_cpp1 += '  '+final_data_name+' = data_id_from_name_noaction('+final_table_name+', "'+data['singleton']['name']+'");\n'
            bf_rt_ids_cpp2 += '  bf_rt_field_id_t smi_id::'+final_data_name+';\n'
            tna_cpp += '    status |= action_entry.set_arg(smi_id::'+final_data_name+', );\n'
        tna_cpp += '  }\n};\n'

    print(" - Copy to bf_rt_ids.h")
    print(bf_rt_ids_h)
    print(" - Copy to bf_rt_ids.cpp")
    print(bf_rt_ids_cpp1)
    print(" - Copy to bf_rt_ids.cpp")
    print(bf_rt_ids_cpp2)
    print(' - Copy to the tna file')
    print(tna_cpp)
