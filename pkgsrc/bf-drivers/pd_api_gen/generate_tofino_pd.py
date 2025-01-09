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

import six
import argparse
import inspect
import json
import os, os.path
import re
import sys
import traceback

from collections import OrderedDict

from tenjin_util import render_template

tenjin_prefix = "//::"

supported_compiler_version = "2.0.1"

_THIS_DIR = os.path.dirname(os.path.realpath(__file__))

_TEMPLATES_DIR = os.path.join(_THIS_DIR, "templates")

def normalize_field_name(name):
  # Replace $number with _number_ ($5name -> _5_name)
  name = re.sub(r'\$(\d+)', r'_\1_', name)
  # Replace $ with _
  name = re.sub(r'\$', r'_', name)
  return re.sub('[.\[\]]', '_', name)

# Indexed field names generated like this
# vlan_tag_id_$0_ethertype. Below function will change this to
# vlan_tag_id__0__ethertype for PD compatibility.  Glass does not generate any
# names with `$` in them so this should not affect any Glass tests.
def normalize_indexed_field_name(name):
  return re.sub(r'\$(\d+)', r'_\1_', name)

def get_reg_type(byte_width):
  if byte_width == 1:
    return "uint8_t"
  elif byte_width == 2:
    return "uint16_t"
  elif byte_width <= 4:
    return "uint32_t"
  elif byte_width <= 8:
    return "uint64_t"
  else:
    return "uint8_t *"

def get_type(byte_width):
  if byte_width == 1:
    return "uint8_t"
  elif byte_width == 2:
    return "uint16_t"
  elif byte_width <= 4:
    return "uint32_t"
  else:
    return "uint8_t *"

def get_thrift_reg_type(byte_width):
  if byte_width == 1:
    return "byte"
  elif byte_width == 2:
    return "i16"
  elif byte_width <= 4:
    return "i32"
  elif byte_width == 8:
    return "i64"
  else:
    return "binary"

def get_thrift_type(byte_width):
  if byte_width == 1:
    return "byte"
  elif byte_width == 2:
    return "i16"
  elif byte_width <= 4:
    return "i32"
  elif byte_width == 6:
    return "MacAddr_t"
  elif byte_width == 16:
    return "IPv6_t"
  else:
    return "binary"

def get_cli_reg_type(byte_width):
  if byte_width == 1:
    return "byte"
  elif byte_width == 2:
    return "i16"
  elif byte_width <= 4:
    return "i32"
  elif byte_width == 8:
    return "i64"
  else:
    return "stream<" + str(byte_width) + ">"

def get_cli_type(byte_width):
  if byte_width == 1:
    return "byte"
  elif byte_width == 2:
    return "i16"
  elif byte_width <= 4:
    return "i32"
  else:
    return "stream<" + str(byte_width) + ">"

def vtype_to_cli_type(vtype):
  if vtype == 'uint8_t':
    return "byte"
  elif vtype == 'uint16_t':
    return "i16"
  elif vtype == 'uint32_t':
    return "i32"
  else:
    return vtype

def check_version(version, supported):
  version_arr = version.split('.')
  supported_arr = supported.split('.')
  version_len = min(len(version_arr), len(supported_arr))

  for i in six.moves.range(version_len):
    if int(version_arr[i]) > int(supported_arr[i]):
      return True
    elif int(version_arr[i]) < int(supported_arr[i]):
      return False

  if len(version_arr) < len(supported_arr):
    return False

  return True

def check_context_version(context_dict):
  try:
    compiler_version = context_dict["compiler_version"]
  except KeyError:
    error_msg = 'ERROR: Compiler version info not found in context json. Please update compiler (v %s)' % supported_compiler_version
    return error_msg

  if not check_version(compiler_version, supported_compiler_version):
    return 'ERROR: Compiler version ' + compiler_version + ' not supported. Minimum version required: ' + supported_compiler_version
  return ''

def get_action_table(context_dict, match_table):
  if len(match_table['action_data_table_refs']) > 0:
    action_table_handle = match_table['action_data_table_refs'][0]['handle']
    for table in context_dict['tables']:
      if table['handle'] == action_table_handle:
        return table
  return None

def is_action_param(match_table, action, action_table, parameter_name):
  try:
    if action_table:
      for pack_format in action_table['stage_tables'][0]['pack_format']:
        if pack_format['action_handle'] != action['handle']:
          continue
        for field in pack_format['entries'][0]['fields']:
          if field['field_name'] == parameter_name:
            return True
  except:
    # If any of the above fields don't exist in our table, just move on
    pass

  try:
    stage_table = match_table['match_attributes']['stage_tables'][0]
    if 'ternary_indirection_stage_table' in stage_table:
      action_formats = stage_table['ternary_indirection_stage_table']['action_format']
    else:
      action_formats = stage_table['action_format']
    for action_format in action_formats:
      if action_format['action_handle'] != action['handle']:
        continue
      for imm_field in action_format['immediate_fields']:
        if imm_field['param_name'] == parameter_name:
          return True
  except:
    # If any of the above fields don't exist in our table, just move on
    pass

  return False

def pd_dict_populate_tables(pd_dict, context_dict):
  pd_dict['table_info'] = OrderedDict()
  pd_dict['action_info'] = OrderedDict()
  pd_dict['field_info'] = {}
  pd_dict['table_handles'] = {}
  pd_dict['action_handles'] = {}
  pd_dict['table_direct_resources'] = {}
  pd_dict['table_indirect_resources'] = {}
  pd_dict['act_prof_hdl_to_name'] = {}
  table_dir_res = {}
  table_indir_res = {}

  for table in context_dict['tables']:
    if table['table_type'] != 'match':
      continue

    if not table['is_resource_controllable']:
      continue

    tbl_name = table['name']
    table_dir_res[tbl_name] = []
    table_indir_res[tbl_name] = set()

    # General table info
    t_info = {}

    match_attributes = table['match_attributes']

    # Get the value for what PD considers the "match_type" for this table. In
    # the new context JSON, the match_type corresponds to the compiler/driver
    # internal type. For PD, it corresponds to the P4-level match type. This
    # can be found via the match_key_fields.

    match_type = 'exact'
    for field in table['match_key_fields']:
      if field['match_type'] == 'ternary':
        match_type = 'ternary'
        break
      if field['match_type'] == 'lpm':
        match_type = 'lpm'
        break
      if field['match_type'] == 'range':
        match_type = 'range'
        break

    t_info['match_type'] = match_type

    if 'action_profile' in table:
      t_info['action_profile'] = table['action_profile']
    else:
      t_info['action_profile'] = None

    if 'ap_bind_indirect_res_to_match' in table:
        t_info['ap_bind_indirect_res_to_match'] = table['ap_bind_indirect_res_to_match']
    else:
        t_info['ap_bind_indirect_res_to_match'] = None

    if match_attributes['match_type'] == 'algorithmic_tcam':
      t_info['atcam_partition_index'] = normalize_field_name(match_attributes['partition_field_name'])

    t_info['dynamic_match_key_masks'] = False
    if 'uses_dynamic_key_masks' in match_attributes:
      t_info['dynamic_match_key_masks'] = match_attributes['uses_dynamic_key_masks']

    # Timeout is a boolean that indicates whether there is an idletime stage
    # table in use.
    timeout = False
    if match_attributes['match_type'] == 'algorithmic_lpm':
      atcam_table = match_attributes['atcam_table']
      atcam_match_attributes = atcam_table['match_attributes']
      for unit in atcam_match_attributes['units']:
        unit_match_attributes = unit['match_attributes']
        for stage_table in unit_match_attributes['stage_tables']:
          if 'idletime_stage_table' in stage_table:
            timeout = True
    elif match_attributes['match_type'] == 'chained_lpm' or \
            match_attributes['match_type'] == 'algorithmic_tcam':
      for unit in match_attributes['units']:
        unit_match_attributes = unit['match_attributes']
        for stage_table in unit_match_attributes['stage_tables']:
          if 'idletime_stage_table' in stage_table:
            timeout = True
    else:
      for stage_table in match_attributes['stage_tables']:
        if 'idletime_stage_table' in stage_table:
          timeout = True

    t_info['timeout'] = timeout

    # Match fields
    match_fields = []
    for field in table['match_key_fields']:
      field_name = field['name']
      match_type = field['match_type']
      if field['is_valid']:
        match_type = 'valid_' + match_type
        if len(field_name) > 7 and field_name[-7:] == ".$valid":
          field_name = field_name[:-7]

      field_name = normalize_field_name(field_name)
      field_name = normalize_indexed_field_name(field_name)
      f_info = {'bit_width': field['bit_width_full']}
      pd_dict['field_info'][field_name] = f_info

      # Update field if already found previously
      found = False
      for i, (name, m_type) in enumerate(match_fields):
        if name == field_name:
          # Only exact match fields may be upgraded to another match type
          if m_type == 'exact':
            match_fields[i] = (name, match_type)
          found = True
          break
      if not found:
        match_fields.append((field_name, match_type))

    t_info['match_fields'] = match_fields

    # Action info and indirect resources
    actions = []
    for action in table['actions']:
      if 'is_compiler_added_action' in action and action['is_compiler_added_action']:
        continue
      actions.append(action['name'])

      if action['name'] in pd_dict['action_info']:
        a_info = pd_dict['action_info'][action['name']]
      else:
        a_info = {}
        a_info['allowed_to_be_default_action'] = {}

        param_names = []
        param_bit_widths = []
        param_byte_widths = []

        for param in action['p4_parameters']:
          param_names.append(param['name'])
          param_bit_widths.append(param['bit_width'])
          param_byte_widths.append((param['bit_width'] + 7) // 8)

        a_info['param_names'] = param_names
        a_info['param_bit_widths'] = param_bit_widths
        a_info['param_byte_widths'] = param_byte_widths

        a_info['indirect_resources'] = []
        a_info['ind_res_and_action_param'] = []
        for indirect_resource in action['indirect_resources']:
          access_mode = indirect_resource['access_mode']
          resource_name = normalize_field_name(indirect_resource['resource_name'])

          if access_mode == 'index':
            parameter_index = indirect_resource['parameter_index']
            parameter_name = indirect_resource['parameter_name']

            a_info['indirect_resources'].append((access_mode, parameter_index, parameter_name, resource_name))

            action_table = get_action_table(context_dict, table)
            if is_action_param(table, action, action_table, parameter_name):
              a_info['ind_res_and_action_param'].append(parameter_name)
          else:
            value = indirect_resource['value']
            a_info['indirect_resources'].append((access_mode, value, None, resource_name))

        pd_dict['action_info'][action['name']] = a_info

      a_info['allowed_to_be_default_action'][tbl_name] = action['allowed_as_default_action']

      # We haven't populated the resource_handles dict yet, so save the names
      # and replace them with handles later
      # Assumes that names are unique between resource tables
      for _,_,_, res_name in a_info['indirect_resources']:
        table_indir_res[tbl_name].add(res_name)

      pd_dict['action_handles'][(tbl_name, action['name'])] = action['handle']

    t_info['actions'] = actions
    table_indir_res[tbl_name] = list(table_indir_res[tbl_name])

    # Table handle info (assumes each match table has at most one action data table and one selector table)
    adt_hdl = None
    sel_tbl_hdl = None

    if len(table['action_data_table_refs']) > 0:
      adt = table['action_data_table_refs'][0]
      if adt['how_referenced'] == 'indirect':
        adt_hdl = adt['handle']
        pd_dict['act_prof_hdl_to_name'][adt_hdl] = table['action_profile']
    if len(table['selection_table_refs']) > 0:
      sel_tbl_hdl = table['selection_table_refs'][0]['handle']
    pd_dict['table_handles'][tbl_name] = (table['handle'], adt_hdl, sel_tbl_hdl)

    # Set up direct resource lists, replace indirect resource names with handles
    for stat_table in table['statistics_table_refs']:
      if stat_table['how_referenced'] == 'direct':
        table_dir_res[tbl_name].append((stat_table['name'], 'counter', stat_table['handle']))
      else:
        for i in range(len(table_indir_res[tbl_name])):
          if table_indir_res[tbl_name][i] == stat_table['name']:
            table_indir_res[tbl_name][i] = stat_table['handle']

    for meter_table in table['meter_table_refs']:
      if meter_table['how_referenced'] == 'direct':
        meter_type_name = 'meter'

        # Find the meter table, and check granularity + type to get a more specific label
        # for this meter.
        for tmp_table in context_dict['tables']:
          if tmp_table['handle'] == meter_table['handle']:
            meter_type_name = tmp_table['meter_granularity'] + '_meter'
            if tmp_table['meter_type'] == 'lpf':
              meter_type_name = 'lpf'
            elif tmp_table['meter_type'] == 'red':
              meter_type_name = 'wred'
            break

        table_dir_res[tbl_name].append((meter_table['name'], meter_type_name, meter_table['handle']))
      else:
        for i in range(len(table_indir_res[tbl_name])):
          if table_indir_res[tbl_name][i] == meter_table['name']:
            table_indir_res[tbl_name][i] = meter_table['handle']

    for stful_table in table['stateful_table_refs']:
      if stful_table['how_referenced'] == 'direct':
        table_dir_res[tbl_name].append((stful_table['name'], 'register', stful_table['handle']))
      else:
        for i in range(len(table_indir_res[tbl_name])):
          if table_indir_res[tbl_name][i] == stful_table['name']:
            table_indir_res[tbl_name][i] = stful_table['handle']

    pd_dict['table_info'][tbl_name] = t_info

  pd_dict['table_direct_resources'] = table_dir_res
  pd_dict['table_indirect_resources'] = table_indir_res

def pd_dict_populate_resources(pd_dict, context_dict):
  def get_table_that_references(handle, type_of_ref):
    for table in context_dict['tables']:
      if table['table_type'] != 'match':
        continue
      for referenced_table in table[type_of_ref]:
        if referenced_table['handle'] == handle:
          return table
    return None

  counter_info = {}
  meter_info = {}
  register_info = {}
  lpf_info = {}
  wred_info = {}

  counter_handles = {}
  meter_handles = {}
  register_handles = {}
  res_hdls = {}
  res_hdls_rev = {}

  for table in context_dict['tables']:
    name = table['name']
    handle = table['handle']
    table_type = table['table_type']

    # Counter info
    if table_type == 'statistics':
      c_info = {}
      c_info['type_'] = table['statistics_type']

      if table['how_referenced'] == 'direct':
        table_that_references = get_table_that_references(table['handle'], 'statistics_table_refs')
        c_info['binding'] = ('direct', table_that_references['name'])
      else:
        c_info['binding'] = ('indirect', None)

      counter_info[name] = c_info

      counter_handles[name] = handle
      res_hdls[name] = handle
      res_hdls_rev[handle] = name

    # Meter info
    if table_type == 'meter':
      m_info = {}
      m_info['type_'] = table['meter_granularity']
      m_info['pre_color'] = False

      if 'pre_color_field_name' in table:
        m_info['pre_color'] = True
      elif 'enable_color_aware' in table:
        m_info['pre_color'] = table['enable_color_aware']

      if table['how_referenced'] == 'direct':
        table_that_references = get_table_that_references(table['handle'], 'meter_table_refs')
        m_info['binding'] = ('direct', table_that_references['name'])
      else:
        m_info['binding'] = ('indirect', None)

      if table['meter_type'] == 'lpf':
        lpf_info[name] = m_info
      elif table['meter_type'] == 'red':
        wred_info[name] = m_info
      else:
        meter_info[name] = m_info

      meter_handles[name] = handle
      res_hdls[name] = handle
      res_hdls_rev[handle] = name

    # Register info
    if table_type == 'stateful':
      r_info = {}

      if table['how_referenced'] == 'direct':
        table_that_references = get_table_that_references(table['handle'], 'stateful_table_refs')
        r_info['binding'] = ('direct', table_that_references['name'])
      else:
        r_info['binding'] = ('indirect', None)

      r_info['width'] = table['alu_width']
      r_info['layout'] = []
      r_info['table_type'] = 'normal'
      if 'stateful_table_type' in table:
          r_info['table_type'] = table['stateful_table_type']
      if r_info['table_type'] == 'fifo':
          r_info['direction'] = table['stateful_direction']

      if r_info['width'] == 128:
          r_info['width'] = 64
          table['dual_width_mode'] = True

      if table['dual_width_mode']:
        # The memory entry width is the alu_width, but for dual entry mode,
        # we must double it.
        byte_width = (2 * r_info['width']) / 8
        byte_width_each = byte_width / 2
        r_info['layout'].append(("f0", byte_width_each))
        r_info['layout'].append(("f1", byte_width_each))
        p4_pd_prefix = pd_dict['p4_pd_prefix']
        api_prefix = pd_dict['api_prefix']

        r_info['v_type'] = p4_pd_prefix + table['name'] + '_value_t'
        r_info['v_thrift_type'] = api_prefix + table['name'] + '_value_t'
        r_info['v_thrift_type_imp'] = r_info['v_thrift_type']
      else:
        r_info['v_type'] = get_type((r_info['width'] + 7) // 8)
        r_info['v_thrift_type'] = get_thrift_type((r_info['width'] + 7) // 8)

        if r_info['width'] <= 8:
          r_info['v_thrift_type_imp'] = 'int8_t'
        elif r_info['width'] <= 16:
          r_info['v_thrift_type_imp'] = 'int16_t'
        else:
          r_info['v_thrift_type_imp'] = 'int32_t'

      register_info[table['name']] = r_info
      register_handles[name] = handle
      res_hdls[name] = handle
      res_hdls_rev[handle] = name

  # Hash calculation info
  hash_calc = {}
  for dyn_hash_calc in context_dict['dynamic_hash_calculations']:
    dyn_hash_calc_info = {}
    dyn_hash_calc_info['name'] = normalize_field_name(dyn_hash_calc['name'])
    dyn_hash_calc_info['handle'] = dyn_hash_calc['handle']

    field_lists = {}
    for field_list in dyn_hash_calc['field_lists']:
      fl_info = {}
      fl_info['name'] = normalize_field_name(field_list['name'])
      fl_info['handle'] = field_list['handle']

      fields = []
      for field in field_list['fields']:
        field_info = {}
        field_info['name'] = normalize_field_name(field['name'])
        field_info['start_bit'] = field['start_bit']
        field_info['bit_width'] = field['bit_width']
        fields.append(field_info)
      fl_info['fields'] = fields

      field_lists[fl_info['name']] = fl_info
    dyn_hash_calc_info['field_lists'] = field_lists

    algs = []
    for alg in dyn_hash_calc['algorithms']:
      algs.append((normalize_field_name(alg['name']), alg['handle']))
    dyn_hash_calc_info['algs'] = algs
    dyn_hash_calc_info['width'] = (dyn_hash_calc['hash_bit_width'] + 7) // 8

    hash_calc[dyn_hash_calc_info['name']] = dyn_hash_calc_info

  pd_dict['counter_info'] = counter_info
  pd_dict['meter_info'] = meter_info
  pd_dict['register_info'] = register_info
  pd_dict['lpf_info'] = lpf_info
  pd_dict['wred_info'] = wred_info

  pd_dict['counter_handles'] = counter_handles
  pd_dict['meter_handles'] = meter_handles
  pd_dict['register_handles'] = register_handles
  pd_dict['resource_handles'] = res_hdls
  pd_dict['resource_handles_rev'] = res_hdls_rev
  pd_dict['hash_calc'] = hash_calc

def pd_dict_populate_learn_quanta(pd_dict, context_dict):
  pd_dict['learn_quanta'] = []

  for lq in context_dict['learn_quanta']:
    if 'lq_cfg_type' in lq and lq['lq_cfg_type'] is not None:
      # Aggregate phv slices into full fields
      full_fields = []
      for field in lq['fields']:
        found = False
        for full_field in full_fields:
          if field['field_name'] == full_field['field_name']:
            full_field['field_width'] += field['field_width']
            found = True
            break
        if not found:
          full_fields.append(field)
      lq['fields'] = full_fields

      # Add any extra fields to field_info
      for field in lq['fields']:
        field['field_name'] = normalize_field_name(field['field_name'])
        field_name = field['field_name']
        field_width = field['field_width']

        if field_name not in pd_dict['field_info']:
          pd_dict['field_info'][field_name] = {'bit_width': field_width}

      lq['name'] = normalize_field_name(lq['name'])
      lq['cb_fn_type'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_digest_notify_cb'
      lq['notify_ack_fn'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_notify_ack'
      lq['msg_type'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_digest_msg_t'
      lq['deregister_fn'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_deregister'
      lq['register_fn'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_register'
      lq['entry_type'] = pd_dict['p4_pd_prefix'] + lq['name'] + '_digest_entry_t'
      pd_dict['learn_quanta'].append(dict(lq))

def pd_dict_populate_parser_value_set_names (pd_dict, context_dict):
  pd_dict['parser_value_set'] = {}
  pd_dict['parser_value_set']['ingress'] = []
  pd_dict['parser_value_set']['egress'] = []

  if 'parser' in context_dict:
    parser_dict = context_dict['parser']
    for pvs in parser_dict['ingress']:
      if 'uses_pvs' not in pvs or pvs['uses_pvs'] is True:
        pvs["pvs_name"] = normalize_field_name(pvs["pvs_name"])
        pd_dict['parser_value_set']['ingress'].append(pvs)
    for pvs in parser_dict['egress']:
      if 'uses_pvs' not in pvs or pvs['uses_pvs'] is True:
        pvs["pvs_name"] = normalize_field_name(pvs["pvs_name"])
        pd_dict['parser_value_set']['egress'].append(pvs)
  elif 'parsers' in context_dict:
    parser_dict = context_dict['parsers']
    for pvs in parser_dict['ingress']['states']:
      if 'uses_pvs' not in pvs or pvs['uses_pvs'] is True:
        pvs["pvs_name"] = normalize_field_name(pvs["pvs_name"])
        pd_dict['parser_value_set']['ingress'].append(pvs)
    for pvs in parser_dict['egress']['states']:
      if 'uses_pvs' not in pvs or pvs['uses_pvs'] is True:
        pvs["pvs_name"] = normalize_field_name(pvs["pvs_name"])
        pd_dict['parser_value_set']['egress'].append(pvs)

def generate_pd_dict_from_context_json(args, context_json):
  with open(context_json, 'r') as f:
    context_dict = json.load(f, object_pairs_hook=OrderedDict)
    error_msg = check_context_version(context_dict)
    if len(error_msg) > 0:
      six.print_(error_msg)
      sys.exit(1)

  pd_dict = {}

  if args.p4_name is None:
    args.p4_name = context_dict["program_name"]
  if args.p4_prefix is None:
    args.p4_prefix = args.p4_name

  pd_dict['p4_name'] = args.p4_name
  pd_dict['p4_prefix'] = args.p4_prefix
  pd_dict['api_prefix'] = args.p4_prefix + '_'
  pd_dict['p4_pd_prefix'] = 'p4_pd_' + args.p4_prefix + '_'
  pd_dict['gen_exm_test_pd'] = args.gen_exm_test_pd
  pd_dict['gen_perf_test_pd'] = args.gen_perf_test_pd
  pd_dict['gen_md_pd'] = args.gen_md_pd
  pd_dict['gen_hitless_ha_test_pd'] = args.gen_hitless_ha_test_pd
  pd_dict['get_type'] = get_type
  pd_dict['get_reg_type'] = get_reg_type
  pd_dict['get_thrift_type'] = get_thrift_type
  pd_dict['get_thrift_reg_type'] = get_thrift_reg_type
  pd_dict['get_cli_type'] = get_cli_type
  pd_dict['get_cli_reg_type'] = get_cli_reg_type
  pd_dict['vtype_to_cli_type'] = vtype_to_cli_type

  if 'tables' in context_dict:
      p4_hidden = [t for t in context_dict['tables'] if 'p4_hidden' in t and t['p4_hidden'] == True]
      for k in p4_hidden:
          context_dict['tables'].remove(k)

  try:
    for table in context_dict['tables']:
      table['name'] = normalize_field_name(table['name'])
      if 'action_profile' in table:
        table['action_profile'] = normalize_field_name(table['action_profile'])
      if 'actions' in table:
        for action in table['actions']:
          action['name'] = normalize_field_name(action['name'])
      for tbl_ref_name in ['action_data_table_refs', 'selection_table_refs', 'meter_table_refs', 'statistics_table_refs', 'stateful_table_refs']:
          if tbl_ref_name in table:
              for ref in table[tbl_ref_name]:
                  ref["name"] = normalize_field_name(ref["name"])
  except KeyError:
    pass

  try:
    pd_dict_populate_tables(pd_dict, context_dict)
  except IndexError:
    six.print_("ERROR: missing tables dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from context json match table fields: " + error_key)
    sys.exit(1)

  try:
    pd_dict_populate_resources(pd_dict, context_dict)
  except IndexError:
    six.print_("ERROR: missing tables dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from the context json resource fields: " + error_key)
    sys.exit(1)

  try:
    pd_dict_populate_learn_quanta(pd_dict, context_dict)
  except IndexError:
    six.print_("ERROR: missing learn quanta dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from the context json learn quanta fields: " + error_key)
    sys.exit(1)

  try:
    pd_dict['PHV_Container_Fields'] = OrderedDict()

    # Parse the total field widths for all stages, and take the maximum.
    max_width_for_field_ingress = OrderedDict()
    max_width_for_field_egress = OrderedDict()

    # Map fields from all stages to their position offset.
    field_position_ingress = {}
    field_position_egress = {}

    for stage_info in context_dict['phv_allocation']:
      width_for_field_in_stage_ingress = OrderedDict()
      width_for_field_in_stage_egress = OrderedDict()

      all_ingress_records = []
      for ingress_info in stage_info['ingress']:

        for record in ingress_info['records']:
          all_ingress_records.append(record)

      # Go over all PHVs and compute total width.
      for record in all_ingress_records:
        if record['is_pov']:
          continue

        record_field_name = normalize_field_name(record['field_name'])
        if record_field_name not in width_for_field_in_stage_ingress:
          width_for_field_in_stage_ingress[record_field_name] = 0

        width_for_field_in_stage_ingress[record_field_name] += record['field_msb'] - record['field_lsb'] + 1

        # If the field hasn't been seen, save its position offset.
        if record_field_name not in field_position_ingress:
          field_position_ingress[record_field_name] = record['position_offset']

      all_egress_records = []
      for egress_info in stage_info['egress']:

        for record in egress_info['records']:
          all_egress_records.append(record)

      # Go over all PHVs and compute total width.
      for record in all_egress_records:
        if record['is_pov']:
          continue

        record_field_name = normalize_field_name(record['field_name'])
        if record_field_name not in width_for_field_in_stage_egress:
          width_for_field_in_stage_egress[record_field_name] = 0

        width_for_field_in_stage_egress[record_field_name] += record['field_msb'] - record['field_lsb'] + 1

        # If the field hasn't been seen, save its position offset.
        if record_field_name not in field_position_egress:
          field_position_egress[record_field_name] = record['position_offset']

      # Now that we computed the width in this stage, take the max.
      for field, width in six.iteritems(width_for_field_in_stage_ingress):
        byte_width = (width + 7) // 8
        if field not in max_width_for_field_ingress or byte_width > max_width_for_field_ingress[field]:
          max_width_for_field_ingress[field] = byte_width

      for field, width in six.iteritems(width_for_field_in_stage_egress):
        byte_width = (width + 7) // 8
        if field not in max_width_for_field_egress or byte_width > max_width_for_field_egress[field]:
          max_width_for_field_egress[field] = byte_width

    # Convert the dictionary of field names to field offsets to a list sorted on
    # the field offset.
    fields_sorted_ingress = sorted(list(six.iteritems(field_position_ingress)), key=lambda f:f[1])
    fields_sorted_egress = sorted(list(six.iteritems(field_position_egress)), key=lambda f:f[1])
    # Populate PHV_Container_Fields in order of the field offset.
    pd_dict['PHV_Container_Fields'][0] = OrderedDict()
    pd_dict['PHV_Container_Fields'][1] = OrderedDict()
    for f,_ in fields_sorted_ingress:
        pd_dict['PHV_Container_Fields'][0][f] = max_width_for_field_ingress[f]
    for f,_ in fields_sorted_egress:
        pd_dict['PHV_Container_Fields'][1][f] = max_width_for_field_egress[f]
  except IndexError:
    six.print_("ERROR: missing PHV container fields dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from the context json PHV container fields: " + error_key)
    sys.exit(1)

  try:
    pd_dict['POV_Dict'] = OrderedDict()

    pov_headers_ingress = {}
    pov_headers_egress = {}
    for stage_info in context_dict['phv_allocation']:
      for ingress_info in stage_info['ingress']:
        for record in ingress_info['records']:
          if record['is_pov']:
            for pov_header in record['pov_headers']:
              header_name = pov_header['header_name']
              if len(header_name) > 7 and header_name[-7:] == '.$valid':
                header_name = header_name[:-7]
              header_name = normalize_field_name(header_name)
              if header_name not in pov_headers_ingress:
                pov_headers_ingress[header_name] = pov_header['position_offset']

      for egress_info in stage_info['egress']:
        for record in egress_info['records']:
          if record['is_pov']:
            for pov_header in record['pov_headers']:
              header_name = pov_header['header_name']
              if len(header_name) > 7 and header_name[-7:] == '.$valid':
                header_name = header_name[:-7]
              header_name = normalize_field_name(header_name)
              if header_name not in pov_headers_egress:
                pov_headers_egress[header_name] = pov_header['position_offset']

    pd_dict['POV_Dict'][0] = sorted(pov_headers_ingress, key=pov_headers_ingress.get)
    pd_dict['POV_Dict'][1] = sorted(pov_headers_egress, key=pov_headers_egress.get)
  except IndexError:
    six.print_("ERROR: missing POV dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from the context json POV dict fields: " + error_key)
    sys.exit(1)

  try:
    pd_dict_populate_parser_value_set_names(pd_dict, context_dict)
  except IndexError:
    six.print_("ERROR: missing parser value dict in context json")
    sys.exit(1)
  except KeyError:
    exc_traceback = sys.exc_info()[2]
    error_line = traceback.format_exc().splitlines()[-1]
    error_key = error_line.split(' ')[1]
    six.print_("ERROR: missing key from the context json parser value set names: " + error_key)
    sys.exit(1)

### DONT CHANGE BELOW ###

  return pd_dict

def ignore_template_file(filename):
    """
    Ignore these files in template dir
    """
    pattern = re.compile('^\..*|.*\.cache$|.*~$')
    return pattern.match(filename)


def gen_file_lists(templates_dir, gen_dir):
    """
    Generate target files from template; only call once
    """
    files_out = []

    for root, subdirs, files in os.walk(templates_dir):
        for filename in files:
            if ignore_template_file(filename):
                continue
            relpath = os.path.relpath(os.path.join(root, filename), templates_dir)
            template_file = relpath
            target_file = os.path.join(gen_dir, relpath)
            files_out.append((template_file, target_file))
    return files_out

def generate_pd(args, context_json, templates_dir, with_thrift=True):
  pd_dict = generate_pd_dict_from_context_json(args, context_json)

  output_dir = args.out
  if not os.path.exists(output_dir):
    os.mkdir(output_dir)
  files = gen_file_lists(templates_dir, output_dir)
  for template, target in files:
      # not very robust
      if (not with_thrift) and ("thrift" in target):
          continue
      path = os.path.dirname(target)
      if not os.path.exists(path):
          os.mkdir(path)
      with open(target, "w") as f:
          render_template(f, template, pd_dict, templates_dir,
                          prefix=tenjin_prefix)

def construct_parser():
  parser = argparse.ArgumentParser(description='pd api generation')
  parser.add_argument('--path', metavar='path', type=str,
                      help='Path of input files')
  parser.add_argument('--context_json', metavar='context_json', type=str,
                      help='Path to the context json file')
  parser.add_argument('--manifest', metavar='manifest', type=str,
                      help='Manifest File')
  parser.add_argument('-o', '--out', required=True, metavar='output_dir', type=str,
                      help='Path to the output directory')
  parser.add_argument('--p4-name', metavar='p4-name', type=str,
                      help='name of p4 program')
  parser.add_argument('--p4-prefix', metavar='p4-prefix', type=str,
                      help='p4 prefix to be used')
  parser.add_argument('--gen-exm-test-pd', action='store_true',
                      help='flag to turn on exact match test pd generation')
  parser.add_argument('--gen-perf-test-pd', action='store_true',
                      help='flag to turn on perf test pd generation')
  parser.add_argument('--gen-md-pd', action='store_true',
                      help='flag to turn on MD pd generation')
  parser.add_argument('--gen-hitless-ha-test-pd', action='store_true',
                      help='flag to turn on hitless HA test pd generation')
  return parser

def main(templates_dir=_TEMPLATES_DIR):
  parser = construct_parser()
  args, unused_args = parser.parse_known_args()
  if not os.path.exists(args.out):
    os.mkdir(args.out)

  if args.manifest:
      manifest_json = json.load(open(args.manifest, 'r'))
      schema_version = manifest_json['schema_version']
      programs = manifest_json['programs']
      if check_version(schema_version, "2.0.0"):
        # Reorganized the manifest schema
        prog_files = programs[0]['pipes'][0]['files']
        context_json = os.path.join(args.path, prog_files['context']['path'])
      else:
        context_json = os.path.join(args.path, programs[0]['contexts'][0]['path'])
  else:
      context_json = args.context_json
  six.print_("The context.json is:", context_json)
  generate_pd(args, context_json, templates_dir)

if __name__ == "__main__":  # pragma: no cover
  main()
