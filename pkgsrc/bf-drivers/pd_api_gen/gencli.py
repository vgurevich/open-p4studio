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

from __future__ import print_function

import json
import sys
import os
import argparse
import six


### Begin XML generation ###
def gen_cli_header(fp):
    fp.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    fp.write('<CLISH_MODULE xmlns="http://clish.sourceforge.net/XMLSchema"\n')
    fp.write('xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n')
    fp.write('xsi:schemaLocation="http://clish.sourceforge.net/XMLSchema\n')
    fp.write('http://clish.sourceforge.net/XMLSchema/clish.xsd">\n')
    return


def gen_cli_footer(fp):
    fp.write('</CLISH_MODULE>\n')
    return


def make_rel_lib(libdir):
    lib_ind = libdir.rfind("/lib/")
    return libdir[lib_ind + 1:]


def gen_cli_top_cmd(fp, service, p4_name, libdir):
    rel_lib = make_rel_lib(libdir)
    fp.write('<PLUGIN name="{}" file="{}libpdcli.so"/>\n'.format(
        p4_name, rel_lib))
    view_name = '-'.join(p4_name.split('_'))
    fp.write('<VIEW name="root-view">\n')
    fp.write('  <COMMAND name="{}" help="{} Related Commands" view="{}" viewid="device_id=${{device_id}}">\n'.\
             format(view_name, p4_name, view_name))
    fp.write(
        '    <PARAM name="device" help="Device ID" optional="true" mode="subcommand" ptype="SUBCOMMAND">\n'
    )
    fp.write(
        '      <PARAM name="device_id" help="Device ID" ptype="UINT8" default="0"/>\n'
    )
    fp.write('    </PARAM>\n')
    fp.write('  </COMMAND>\n')
    fp.write('</VIEW>\n\n')
    fp.write('<VIEW name="{}" prompt="{}:${{device_id}}&gt; ">\n'.format(
        view_name, view_name))
    fp.write(
        '  <COMMAND name="end" help="End {} sub-commands" view="root-view"/>\n'
        .format(view_name))
    fp.write(
        '  <COMMAND name="{}" help="prefix for all {} commands"/>\n'.format(
            service, service))
    fp.write('</VIEW>\n\n')
    fp.write('<VIEW name="{}">\n'.format(view_name))
    fp.write('  <COMMAND name="var" help="Declare new shell variable">\n')
    fp.write(
        '    <PARAM name="varname" help="Variable name" optional="false" ptype="STRING">\n'
    )
    fp.write('    </PARAM>\n')
    fp.write('    <ACTION builtin="var_decl_action" />\n')
    fp.write('  </COMMAND>\n')
    fp.write('  <COMMAND name="get" help="Display value of shell variable">\n')
    fp.write(
        '    <PARAM name="varname" help="Variable name" optional="false" ptype="STRING">\n'
    )
    fp.write('    </PARAM>\n')
    fp.write('    <ACTION builtin="var_get_action" />\n')
    fp.write('  </COMMAND>\n')
    fp.write('  <COMMAND name="set" help="Set value of shell variable">\n')
    fp.write(
        '    <PARAM name="varname" help="Variable name" optional="false" ptype="STRING">\n'
    )
    fp.write('    </PARAM>\n')
    fp.write(
        '    <PARAM name="value" help="Value of variable" optional="false" ptype="STRING">\n'
    )
    fp.write('    </PARAM>\n')
    fp.write('    <ACTION builtin="var_set_action" />\n')
    fp.write('  </COMMAND>\n')
    fp.write('  <COMMAND name="dump_table" help="dump table entries"/>\n')
    fp.write(
        '  <COMMAND name="dump_profile" help="dump action profile entries"/>\n'
    )

    global idl
    tables = idl['tables']
    for table in tables:
        fp.write(
            '  <COMMAND name="dump_table {}" help="Dump entries of {} table.">\n'
            .format(table, table))
        fp.write(
            '    <PARAM name="csv" mode="subcommand" help="dump table data to csv file" optional="true" ptype="SUBCOMMAND">\n'
        )
        fp.write(
            '      <PARAM name="p_csv_filename" help="csv filename" ptype="STRING"/>\n'
        )
        fp.write('    </PARAM>\n')
        fp.write(
            '    <PARAM name="read_from_hw" mode="subcommand" help="dump table data from hw (optional, default=False)" optional="true" ptype="SUBCOMMAND">\n'
        )
        fp.write(
            '      <PARAM name="p_read_from_hw" help="read from hw" ptype="BOOL"/>\n'
        )
        fp.write('    </PARAM>\n')
        fp.write(
            '    <ACTION builtin="table_dump_{}_action" />\n'.format(table))
        fp.write('  </COMMAND>\n')
    profiles = idl['action_profiles']
    for profile in profiles:
        fp.write(
            '  <COMMAND name="dump_profile {}" help="Dump entries of {} action profile.">\n'
            .format(profile, profile))
        fp.write(
            '    <PARAM name="csv" mode="subcommand" help="dump profile data to csv file" optional="true" ptype="SUBCOMMAND">\n'
        )
        fp.write(
            '      <PARAM name="p_csv_filename" help="csv filename" ptype="STRING"/>\n'
        )
        fp.write('    </PARAM>\n')
        fp.write(
            '    <PARAM name="read_from_hw" mode="subcommand" help="dump table data from hw (optional, default=False)" optional="true" ptype="SUBCOMMAND">\n'
        )
        fp.write(
            '      <PARAM name="p_read_from_hw" help="read from hw" ptype="BOOL"/>\n'
        )
        fp.write('    </PARAM>\n')
        fp.write('    <ACTION builtin="profile_dump_{}_action" />\n'.format(
            profile))
        fp.write('  </COMMAND>\n')

    fp.write('</VIEW>\n\n')
    return


def get_cli_base_type(ptype):
    ptype_dict = {
        'bool': 'BOOL',
        'byte': 'DYNINT',
        'i16': 'DYNINT',
        'i32': 'DYNINT',
        'int': 'DYNINT',
        'float': 'FLOAT',
        'hex': 'DYNINT',
        'i64': 'DYNINT',
        'null': 'STRING',
        'string': 'STRING',
        'union': 'STRING'
    }
    return ptype_dict[ptype]


def gen_cli_begin_sub_param(fp, prefix, param, helpstr, opt):
    pname = param[0]
    fp.write('  <PARAM name="{}" help="{}" optional="{}" '
             'mode="subcommand" ptype="SUBCOMMAND">\n'.format(
                 pname, helpstr, opt))
    return


def gen_cli_end_sub_param(fp):
    fp.write('  </PARAM>\n')
    return


def gen_cli_simple_param(fp, prefix, param):
    opt = 'false'
    optstr = '(required)'
    if not bool(param[2]):
        opt = 'true'
        if 'device_id' in param[0]:
            optstr = '(optional, default = your selection when opening pdcli, 0 if you didnt do this)'
        else:
            optstr = '(optional, default={})'.format(param[8])
    helpstr = ''
    if bool(param[6]):
        helpstr = '{} byte hex number {}'.format(param[7], optstr)
    elif param[1] == 'float':
        helpstr = 'Float {}'.format(optstr)
    elif param[1] == 'null':
        helpstr = 'NOT SUPPORTED - NO OP {}'.format(optstr)
    else:
        helpstr = 'Integer type: {} {}'.format(param[1], optstr)
    if bool(param[9]):
        helpstr = 'Buffer parameter. Put var here to assign data to it. (optional)'
    gen_cli_begin_sub_param(fp, prefix, param, prefix + ": " + helpstr, opt)
    pname = (prefix + param[0]).replace('.', '_')
    hname = prefix + param[0]
    ptype = get_cli_base_type(param[1])
    if bool(param[6]):
        ptype = 'HEX'
    fp.write('  <PARAM name="p_{}" help="{}" ptype="{}"/>\n'.format(
        pname, helpstr, ptype))
    gen_cli_end_sub_param(fp)
    return


def gen_cli_cmd_param(fp, prefix, param):
    if not isinstance(param[1], list):
        gen_cli_simple_param(fp, prefix, param)
    else:
        prefix = prefix + param[0] + '.'
        for subparam in param[1]:
            gen_cli_cmd_param(fp, prefix, subparam)
    return


top_level_cmds = []


def gen_top_cmds(fp, service, func, namespace, p4_name):
    global top_level_cmds
    prefix = 'p4_{}-'.format(namespace)
    fnames = [func[0]]
    if 'synonyms' in func[2]:
        fnames += func[2]['synonyms']
    for idx, fname in enumerate(fnames):
        view_name = '-'.join(p4_name.split('_'))

        # remove the prefix from the command
        if fname.startswith(prefix):
            fname = fname[len(prefix):]

        # split at dashes and create top level commands
        cmds = fname.split('-')
        cmd = service
        syn_branch = True
        for jdx, c in enumerate(cmds[0:len(cmds) - 1]):
            cmd = cmd + ' ' + c
            if cmd not in top_level_cmds:
                help_str = ""
                if idx != 0 and syn_branch:
                    syn = fnames[0].split('-')[jdx +
                                               1]  #  +1 because the prefix
                    help_str = "Synonym for {}".format(syn)
                    syn_branch = False
                fp.write('<VIEW name="{}">\n'.format(view_name))
                fp.write('<COMMAND name="{}" help="{}"/>\n'.format(
                    cmd, help_str))
                fp.write('</VIEW>\n')
                top_level_cmds.append(cmd)


def gen_cli_cmd(fp, service, func, namespace, p4_name):
    global top_level_cmds
    prefix = 'p4_{}-'.format(namespace)
    fnames = [func[0]]
    if 'synonyms' in func[2]:
        fnames += func[2]['synonyms']
    for idx, fname in enumerate(fnames):
        view_name = '-'.join(p4_name.split('_'))

        # remove the prefix from the command
        if fname.startswith(prefix):
            fname = fname[len(prefix):]

        # split at dashes and replace with spaces
        cmds = fname.split('-')
        cmd = service
        for c in cmds[0:len(cmds) - 1]:
            cmd = cmd + ' ' + c

        # the actual command
        cmd = cmd + ' ' + cmds[-1]
        if cmd not in top_level_cmds:
            top_level_cmds.append(cmd)

        help_str = ""
        if idx != 0 and fnames[0].split('-')[-1] != cmds[-1]:
            syn = fnames[0].split('-')[-1]
            help_str = "Synonym for {}".format(syn)
        fp.write('<VIEW name="{}">\n'.format(view_name))
        fp.write('<COMMAND name="{}" help="{}">\n'.format(cmd, help_str))
        for param in func[1]:
            gen_cli_cmd_param(fp, '', param)
        fp.write('  <ACTION builtin="{}_action"/>\n'.format(func[0].replace(
            '-', '_')))
        fp.write('</COMMAND>\n')
        fp.write('</VIEW>\n\n')
    return


### End XML Generation ###


### Begin Action Stub Generation ###
def gen_act_header(fp):
    fp.write('#include <stdlib.h>\n')
    fp.write('#include <stdio.h>\n')
    fp.write('#include <string.h>\n')
    fp.write('#include <stdint.h>\n')
    fp.write('#include <inttypes.h>\n')
    fp.write('#include <pd/pd.h>\n')
    fp.write('#include <arpa/inet.h>\n')
    fp.write('#include <ctype.h>\n')
    fp.write('#include <target-sys/bf_sal/bf_sys_mem.h>\n')
    fp.write('#include "target-utils/clish/shell.h"\n\n')

    # Maximum number of entry handles to get. Used to do input validation
    # when we parse a string to get the count. The number is arbitrary;
    # it should be such that we don't blow the stack.
    fp.write('#define MAX_NUM_ENTRIES 1024\n\n')

    fp.write('struct PrintInfo {\n')
    fp.write('  int titles_setting;\n')
    fp.write('  char *value_delimiter;\n')
    fp.write('  char *title_delimeter;\n')
    fp.write('  FILE *outfile;\n')
    fp.write('};\n\n')
    fp.write(
        'static struct PrintInfo default_print = { 1, "\\n", ": ", NULL };\n\n'
    )
    fp.write(
        'static int fill_byte_stream(char *stream, const char *source, int width) {\n'
    )
    fp.write('  if (source[1] == \'x\') {\n')
    fp.write('    source += 2;\n')
    fp.write('  }\n')
    fp.write('  int prepend = width * 2 - strlen(source);\n')
    fp.write('  char *pad_source = NULL;\n')
    fp.write('  if (prepend > 0) {\n')
    fp.write('    pad_source = bf_sys_malloc(width * 2 + 1);\n')
    fp.write('    if (!pad_source) return -2;\n')
    fp.write(
        '    memcpy(pad_source + prepend, source, width * 2 - prepend + 1);\n')
    fp.write('    memset(pad_source, 0x30, prepend);\n')
    fp.write('    source = pad_source;\n')
    fp.write('  } else if (prepend < 0) {\n')
    fp.write('    return -1;\n')
    fp.write('  }\n')
    fp.write(
        '  for (int i = 0; i < width; i++) sscanf(source + i * 2, "%02hhX", stream + i);\n'
    )
    fp.write('  if (pad_source) bf_sys_free(pad_source);\n')
    fp.write('  return 0;\n')
    fp.write('};\n\n')
    fp.write('CLISH_PLUGIN_SYM(var_decl_action)\n')
    fp.write('{\n')
    fp.write(
        '  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);\n')
    fp.write('  const clish_parg_t *parg = NULL;\n\n')
    fp.write('  const char *varname = NULL;\n')
    fp.write('  parg = clish_pargv_find_arg(pargv, "varname");\n')
    fp.write('  if (parg) {\n')
    fp.write('    varname = clish_parg__get_value(parg);\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error: the varname argument is required.\\n");\n'
    )
    fp.write('  }\n\n')
    fp.write('  int ret = -1;\n')
    fp.write('  if (varname) {\n')
    fp.write('    if (varname[0] == \'$\') {\n')
    fp.write(
        '      ret = clish_shell_strmap_insert(clish_context, varname);\n')
    fp.write('    } else {\n')
    fp.write(
        '      bfshell_printf(clish_context, "Variable names must start with a $\\n");\n'
    )
    fp.write('    }\n')
    fp.write('  }\n')
    fp.write('  if (ret == -1) {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error declaring variable.\\n");\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Variable %s declared.\\n", varname);\n'
    )
    fp.write('  }\n')
    fp.write('  return 0;\n')
    fp.write('}\n\n')
    fp.write('CLISH_PLUGIN_SYM(var_get_action)\n')
    fp.write('{\n')
    fp.write(
        '  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);\n')
    fp.write('  const clish_parg_t *parg = NULL;\n\n')
    fp.write('  const char *varname = NULL;\n')
    fp.write('  parg = clish_pargv_find_arg(pargv, "varname");\n')
    fp.write('  if (parg) {\n')
    fp.write('    varname = clish_parg__get_value(parg);\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error: the varname argument is required.\\n");\n'
    )
    fp.write('  }\n\n')
    fp.write('  char *ret = NULL;\n')
    fp.write('  if (varname) {\n')
    fp.write('    ret = clish_shell_strmap_get(clish_context, varname);\n')
    fp.write('  }\n')
    fp.write('  if (ret == (char*)0x1) {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error getting variable.\\n");\n')
    fp.write('  } else if (ret == NULL) {\n')
    fp.write('    bfshell_printf(clish_context, "%s : NULL\\n", varname);\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "%s : %s\\n", varname, ret);\n')
    fp.write('  }\n')
    fp.write('  return 0;\n')
    fp.write('}\n\n')
    fp.write('CLISH_PLUGIN_SYM(var_set_action)\n')
    fp.write('{\n')
    fp.write(
        '  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);\n')
    fp.write('  const clish_parg_t *parg = NULL;\n\n')
    fp.write('  const char *varname = NULL;\n')
    fp.write('  const char *value = NULL;\n')
    fp.write('  parg = clish_pargv_find_arg(pargv, "varname");\n')
    fp.write('  if (parg) {\n')
    fp.write('    varname = clish_parg__get_value(parg);\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error: the varname argument is required.\\n");\n'
    )
    fp.write('  }\n\n')
    fp.write('  parg = clish_pargv_find_arg(pargv, "value");\n')
    fp.write('  if (parg) {\n')
    fp.write('    value = clish_parg__get_value(parg);\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error: the value argument is required.\\n");\n'
    )
    fp.write('  }\n\n')
    fp.write('  int ret = -1;\n')
    fp.write('  if (varname && value) {\n')
    fp.write(
        '    ret = clish_shell_strmap_set(clish_context, varname, value);\n')
    fp.write('  }\n')
    fp.write('  if (ret == -1) {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Error setting variable.\\n");\n')
    fp.write('  } else {\n')
    fp.write(
        '    bfshell_printf(clish_context, "Variable %s set successfully.\\n", varname);\n'
    )
    fp.write('  }\n')
    fp.write('}\n\n')
    fp.write('static void parse_ipv4(char *data, const char *ip_str) {\n')
    fp.write('  const char *str_idx = ip_str;\n')
    fp.write('  size_t data_idx = 0;\n\n')
    fp.write('  while (*str_idx && data_idx < 4) {\n')
    fp.write('    if (isdigit((unsigned char)*str_idx)) {\n')
    fp.write('      data[data_idx] *= 10;\n')
    fp.write("      data[data_idx] += *str_idx - '0';\n")
    fp.write('    } else {\n')
    fp.write('      data_idx++;\n')
    fp.write('    }\n')
    fp.write('    str_idx++;\n')
    fp.write('  }\n')
    fp.write('}\n\n')
    fp.write('static uint32_t parse_i32(const char *int_str) {\n')
    fp.write('  if (strstr(int_str, ".")) {\n')
    fp.write('    uint32_t ip_addr = 0;\n')
    fp.write('    parse_ipv4((char *)&ip_addr, int_str);\n')
    fp.write('    return ntohl(ip_addr);\n')
    fp.write('  } else {\n')
    fp.write('    return strtoll(int_str, NULL, 0);\n')
    fp.write('  }\n')
    fp.write('}\n\n')
    return


def gen_dump_plugins(fp):
    global idl
    prefix = idl['function_prefix']
    tables = idl['tables']
    action_profiles = idl['action_profiles']
    for table in tables:
        cur_prefix = prefix + table
        fp.write('CLISH_PLUGIN_SYM(table_dump_{}_action)\n'.format(table))
        fp.write('{\n')
        fp.write(
            '  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);\n'
        )
        fp.write('  const clish_parg_t *parg = NULL;\n')
        fp.write('  const char *p_read_from_hw = NULL;\n')
        fp.write('  const char *p_csv_filename = NULL;\n')
        fp.write('  FILE *csv = NULL;\n')
        fp.write(
            '  if (parg = clish_pargv_find_arg(pargv, "p_csv_filename")) {\n')
        fp.write('    p_csv_filename = clish_parg__get_value(parg);\n')
        fp.write('    csv = fopen(p_csv_filename, "w");\n')
        fp.write('    if (csv) {\n')
        fp.write(
            '      bfshell_printf(clish_context, "Printing csv output to file: %s\\n", p_csv_filename);\n'
        )
        fp.write('    } else {\n')
        fp.write(
            '      bfshell_printf(clish_context, "Couldn\'t open file: %s\\n", p_csv_filename);\n'
        )
        fp.write('      return 0;\n')
        fp.write('    }\n')
        fp.write('  }\n\n')
        fp.write(
            '  if (parg = clish_pargv_find_arg(pargv, "p_read_from_hw")) {\n')
        fp.write('    p_read_from_hw = clish_parg__get_value(parg);\n')
        fp.write('  } else {\n')
        fp.write('    p_read_from_hw = "false";\n')
        fp.write('  }\n\n')
        fp.write('  uint32_t count = 0;\n')
        fp.write('  p4_pd_dev_target_t tgt;\n')
        fp.write('  tgt.device_id = 0;\n')
        fp.write('  tgt.dev_pipe_id = PD_DEV_PIPE_ALL;\n')
        fp.write('  {}_get_entry_count(1,tgt,&count);\n'.format(cur_prefix))
        fp.write('  if (count == 0) {\n')
        fp.write(
            '    bfshell_printf(clish_context, "Table {} has no entries.\\n");\n'
            .format(table))
        fp.write('    if (csv) fclose(csv);\n')
        fp.write('    return 0;\n')
        fp.write('  }\n\n')
        fp.write('  int indexes[count];\n')
        fp.write('  char *hdl = bf_sys_malloc(11);\n\n')
        fp.write(
            '  {}_get_first_entry_handle(1,tgt,indexes);\n'.format(cur_prefix))
        fp.write('  sprintf(hdl, "%d", indexes[0]);\n')
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    bfshell_printf(clish_context, "-------------\\n");\n')
        fp.write(
            '    bfshell_printf(clish_context, "entry_hdl: %s\\n", hdl);\n')
        fp.write('  } else {\n')
        fp.write('    bfshell_printf(clish_context, "%s,", hdl);\n')
        fp.write('    fprintf(csv, "%s,", hdl);\n')
        fp.write('  }\n')
        fp.write('  struct PrintInfo p_info = { 0, ",", "", csv } ;\n')
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    p_info = default_print;\n')
        fp.write('  }\n')
        typecheck_call = '{}_get_entry_typecheck(clish_context,&p_info,"1","0",hdl,p_read_from_hw'.format(
            cur_prefix)
        funcs = idl['services'][0]['functions']
        func = next(
            (f for f in funcs
             if f[0].replace('-', '_') == '{}_get_entry'.format(cur_prefix)),
            None)
        to_add = len(func[1]) - 6
        for j in six.moves.range(0, to_add):
            typecheck_call += ',"0"'
        res = next((st for st in idl['structs']
                    if st['name'] == cur_prefix + '_match_spec_t'), None)
        if res is not None:
            for field in res['fields']:
                typecheck_call += ',"0"'
        indirect = False
        for each in func[1]:
            if each[0] == 'mbr_hdl':
                typecheck_call += ',"0");\n'
                indirect = True
                break
        if not indirect:
            typecheck_call += ',"0","0");\n'
        fp.write('  {}'.format(typecheck_call))
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    bfshell_printf(clish_context, "\\n-------------\\n");\n')
        fp.write('  } else {\n')
        fp.write('    bfshell_printf(clish_context, "\\n");\n')
        fp.write('    fprintf(csv, "\\n");\n')
        fp.write('  }\n')
        fp.write('  if (count > 1) {\n')
        fp.write(
            '    {}_get_next_entry_handles(1,tgt,indexes[0],count-1,&indexes[1]);\n'
            .format(cur_prefix))
        fp.write('    for (int i = 1; i < count; i++) {\n')
        fp.write('      if (indexes[i] == -1) {\n')
        fp.write('        break;\n')
        fp.write('      }\n')
        fp.write('      sprintf(hdl, "%d", indexes[i]);\n')
        fp.write('      if (!p_csv_filename) {\n')
        fp.write(
            '        bfshell_printf(clish_context, "entry_hdl: %s\\n", hdl);\n'
        )
        fp.write('      } else {\n')
        fp.write('        bfshell_printf(clish_context, "%s,", hdl);\n')
        fp.write('        fprintf(csv, "%s,", hdl);\n')
        fp.write('      }\n')
        fp.write('      {}\n'.format(typecheck_call))
        fp.write('      if (!p_csv_filename) {\n')
        fp.write(
            '        bfshell_printf(clish_context, "\\n-------------\\n");\n')
        fp.write('      } else {\n')
        fp.write('        bfshell_printf(clish_context, "\\n");\n')
        fp.write('        fprintf(csv, "\\n");\n')
        fp.write('      }\n')
        fp.write('    }\n')
        fp.write('  }\n\n')
        fp.write('  bf_sys_free(hdl);\n')
        fp.write('  if (csv) fclose(csv);\n')
        fp.write('  return 0;\n')
        fp.write('}\n\n')
    for profile in action_profiles:
        cur_prefix = prefix + profile
        fp.write('CLISH_PLUGIN_SYM(profile_dump_{}_action)\n'.format(profile))
        fp.write('{\n')
        fp.write(
            '  clish_pargv_t *pargv = clish_context__get_pargv(clish_context);\n'
        )
        fp.write('  const clish_parg_t *parg = NULL;\n')
        fp.write('  const char *p_csv_filename = NULL;\n')
        fp.write('  FILE *csv = NULL;\n')
        fp.write(
            '  if (parg = clish_pargv_find_arg(pargv, "p_csv_filename")) {\n')
        fp.write('    p_csv_filename = clish_parg__get_value(parg);\n')
        fp.write('    csv = fopen(p_csv_filename, "w");\n')
        fp.write('    if (csv) {\n')
        fp.write(
            '      bfshell_printf(clish_context, "Printing csv output to file: %s\\n", p_csv_filename);\n'
        )
        fp.write('    } else {\n')
        fp.write(
            '      bfshell_printf(clish_context, "Couldn\'t open file: %s\\n", p_csv_filename);\n'
        )
        fp.write('      return 0;\n')
        fp.write('    }\n')
        fp.write('  }\n\n')
        fp.write('  p4_pd_dev_target_t tgt;\n')
        fp.write('  tgt.device_id = 0;\n')
        fp.write('  tgt.dev_pipe_id = PD_DEV_PIPE_ALL;\n')
        fp.write('  uint32_t count = 0;\n')
        fp.write('  {}_get_act_prof_entry_count(1,tgt,&count);\n'.format(
            cur_prefix))
        fp.write('  if (count == 0) {\n')
        fp.write(
            '    bfshell_printf(clish_context, "Profile {} has no entries.\\n");\n'
            .format(profile))
        fp.write('    if (csv) fclose(csv);\n')
        fp.write('    return 0;\n')
        fp.write('  }\n\n')
        fp.write('  int indexes[count];\n')
        fp.write('  char *hdl = bf_sys_malloc(11);\n\n')
        fp.write('  {}_get_first_member(1,tgt,indexes);\n'.format(cur_prefix))
        fp.write('  sprintf(hdl, "%d", indexes[0]);\n')
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    bfshell_printf(clish_context, "-------------\\n");\n')
        fp.write(
            '    bfshell_printf(clish_context, "member_hdl: %s\\n", hdl);\n')
        fp.write('  } else {\n')
        fp.write('    bfshell_printf(clish_context, "%s,", hdl);\n')
        fp.write('    fprintf(csv, "%s,", hdl);\n')
        fp.write('  }\n')
        fp.write('  struct PrintInfo p_info = { 0, ",", "", csv } ;\n')
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    p_info = default_print;\n')
        fp.write('  }\n')
        typecheck_call = '{}_get_member_typecheck(clish_context,&p_info,"1","0",hdl,"0","0","0");'.format(
            cur_prefix)
        fp.write('  {}'.format(typecheck_call))
        fp.write('  if (!p_csv_filename) {\n')
        fp.write('    bfshell_printf(clish_context, "\\n-------------\\n");\n')
        fp.write('  } else {\n')
        fp.write('    bfshell_printf(clish_context, "\\n");\n')
        fp.write('    fprintf(csv, "\\n");\n')
        fp.write('  }\n')
        fp.write('  if (count > 1) {\n')
        fp.write(
            '    {}_get_next_members(1,tgt,indexes[0],count-1,&indexes[1]);\n'.
            format(cur_prefix))
        fp.write('    for (int i = 1; i < count; i++) {\n')
        fp.write('      sprintf(hdl, "%d", indexes[i]);\n')
        fp.write('      if (!p_csv_filename) {\n')
        fp.write(
            '        bfshell_printf(clish_context, "member_hdl: %s\\n", hdl);\n'
        )
        fp.write('      } else {\n')
        fp.write('        bfshell_printf(clish_context, "%s,", hdl);\n')
        fp.write('        fprintf(csv, "%s,", hdl);\n')
        fp.write('      }\n')
        fp.write('      {}\n'.format(typecheck_call))
        fp.write('      if (!p_csv_filename) {\n')
        fp.write(
            '        bfshell_printf(clish_context, "\\n-------------\\n");\n')
        fp.write('      } else {\n')
        fp.write('        bfshell_printf(clish_context, "\\n");\n')
        fp.write('        fprintf(csv, "\\n");\n')
        fp.write('      }\n')
        fp.write('    }\n')
        fp.write('  }\n\n')
        fp.write('  bf_sys_free(hdl);\n')
        fp.write('  if (csv) fclose(csv);\n')
        fp.write('  return 0;\n')
        fp.write('}\n\n')


def get_act_param(prefix, param):
    if not isinstance(param[1], list):
        return (prefix + param[0], param[1], param[4], param[2], param[5],
                param[6], param[7], param[8], param[9], param[10])
    else:
        prefix = prefix + param[0] + '_'
        splist = []
        for subparam in param[1]:
            sp = get_act_param(prefix, subparam)
            if isinstance(sp, list):
                splist.extend(sp)
            else:
                splist.append(sp)
        return splist


def get_act_params(func):
    plist = []
    for param in func[1]:
        p = get_act_param('', param)
        if isinstance(p, list):
            plist.extend(p)
        else:
            plist.append(p)
    return plist


def gen_act_function(fp, func, params):
    b = lambda x: str(x).lower() if isinstance(x, bool) else x
    n = lambda x: 'NULL' if x is None else x
    fp.write('CLISH_PLUGIN_SYM({}_action)\n'.format(func.replace('-', '_')))
    fp.write('{\n')
    fp.write('  clish_pargv_t *pargv = clish_context__get_pargv(clish_context)'
             ';\n')
    fp.write('  const clish_parg_t *parg = NULL;\n\n')
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        fp.write('  const char *p_{} = NULL;\n'.format(param))
    fp.write('\n')
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        fp.write(
            '  parg = clish_pargv_find_arg(pargv, "p_{}");\n'.format(param))
        if (not req and 'device_id' in param):
            fp.write('  bool p_{}_to_free = false;\n'.format(param))
        fp.write('  if (parg) {\n')
        fp.write('    p_{} = clish_parg__get_value(parg);\n'.format(param))
        fp.write('    if (p_{} == NULL) {{\n'.format(param))
        fp.write('      bfshell_printf(clish_context, "Error: the {} argument is invalid.\\n");\n'
                .format(param))
        fp.write('      return -1;\n')
        fp.write('    }\n')
        fp.write('  } else {\n')
        if (not req and 'device_id' in param):
            fp.write(
                '    p_{} = (char *)clish_shell_expand_var_ex("device_id", clish_context, SHELL_EXPAND_VIEW);\n'
                .format(param))
            fp.write('    p_{}_to_free = true;\n'.format(param))
        elif (not req and 'device_id' not in param):
            fp.write('    p_{} = "{}";\n'.format(param, b(n(default))))
        else:
            fp.write(
                '    bfshell_printf(clish_context, "Error: the {} argument is required.\\n");\n'
                .format(param))
            fp.write('    return -1;\n')
        fp.write('  }\n\n')
    if (len(params) == 0):
        fp.write('  {}_typecheck('.format(func.replace('-', '_')))
        fp.write('clish_context, &default_print')
    else:
        fp.write('  {}_typecheck(\n'.format(func.replace('-', '_')))
        fp.write('    clish_context,\n')
        fp.write('    &default_print,\n')
    i = 0
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        if (i < (len(params) - 1)):
            fp.write('    p_{},\n'.format(param))
        else:
            fp.write('    p_{}'.format(param))
        i = i + 1
    fp.write(');\n')
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        if (not req and 'device_id' in param):
            fp.write('  if(p_{}_to_free) {{\n'.format(param))
            fp.write('    bf_sys_free((void*)p_{});\n'.format(param))
            fp.write('  }\n')
    fp.write('  return 0;\n')
    fp.write('}\n\n')
    return


def gen_act_footer(fp, p4_name, functions):
    global idl
    fp.write('CLISH_PLUGIN_INIT({})\n'.format(p4_name))
    fp.write('{\n')
    fp.write('  clish_plugin_add_sym(plugin,\n')
    fp.write('                       var_decl_action,\n')
    fp.write('                       "var_decl_action");\n')
    fp.write('  clish_plugin_add_sym(plugin,\n')
    fp.write('                       var_get_action,\n')
    fp.write('                       "var_get_action");\n')
    fp.write('  clish_plugin_add_sym(plugin,\n')
    fp.write('                       var_set_action,\n')
    fp.write('                       "var_set_action");\n')
    for func in functions:
        fp.write('  clish_plugin_add_sym(plugin,\n')
        fp.write('                       {}_action,\n'.format(func[0].replace(
            '-', '_')))
        fp.write('                       "{}_action");\n'.format(
            func[0].replace('-', '_')))
    for table in idl['tables']:
        fp.write('  clish_plugin_add_sym(plugin,\n')
        fp.write(
            '                       table_dump_{}_action,\n'.format(table))
        fp.write(
            '                       "table_dump_{}_action");\n'.format(table))
    for profile in idl['action_profiles']:
        fp.write('  clish_plugin_add_sym(plugin,\n')
        fp.write(
            '                       profile_dump_{}_action,\n'.format(profile))
        fp.write('                       "profile_dump_{}_action");\n'.format(
            profile))
    fp.write('  return 0;\n')
    fp.write('}\n')
    return


### End Action Stub Generation ###


### Begin Client RPC Stub Generation ###
def gen_stub_header(fp, namespace, services):
    fp.write('#include <iostream>\n')
    fp.write('#include <string>\n\n')
    fp.write('#include "clish/shell.h"\n\n')
    fp.write('#ifdef STANDALONE\n')
    fp.write('#include <thrift/protocol/TBinaryProtocol.h>\n')
    fp.write('#include <thrift/transport/TSocket.h>\n')
    fp.write('#include <thrift/transport/TTransportUtils.h>\n')
    fp.write('#endif\n')
    fp.write('#include <thrift/TToString.h>\n')
    for service in services:
        fp.write('#include "{}.h"\n'.format(service['name']))
    fp.write('\n')
    fp.write('using namespace std;\n')
    fp.write('using namespace apache::thrift;\n')
    fp.write('#ifdef STANDALONE\n')
    fp.write('using namespace apache::thrift::protocol;\n')
    fp.write('using namespace apache::thrift::transport;\n')
    fp.write('#endif\n')
    fp.write('using namespace {};\n\n'.format(namespace))
    return


def gen_stub_client_init(fp, service):
    fp.write('#ifdef STANDALONE\n')
    fp.write('static {}Client *{}_client = NULL;\n'.format(service, service))
    fp.write('static void rpcClientInit() {\n')
    fp.write('  boost::shared_ptr<TTransport> socket(')
    fp.write('new TSocket("localhost", 9091));\n')
    fp.write('  boost::shared_ptr<TTransport> transport(')
    fp.write('new TBufferedTransport(socket));\n')
    fp.write('  boost::shared_ptr<TProtocol> protocol(')
    fp.write('new TBinaryProtocol(transport));\n')
    fp.write('  {}_client = new {}Client(protocol);\n'.format(
        service, service))
    fp.write('  transport->open();\n')
    fp.write('  return;\n')
    fp.write('}\n')
    fp.write('#else\n')
    fp.write('extern {}If *{}_client;\n'.format(service, service))
    fp.write('static void rpcClientInit() {\n')
    fp.write('  return;\n')
    fp.write('}\n')
    fp.write('#endif\n\n')
    return


def gen_stub_action_start(fp):
    fp.write('extern "C" {\n\n')
    fp.write(
        'extern void bfshell_printf(clish_context_t *, const char *);\n\n')
    return


def is_simple_cpp_type(ptype):
    if is_base_type(ptype):
        return True
    res = next((ty for ty in idl['typedefs'] if ty['name'] == ptype), None)
    if res is not None:
        return True
    return False


def get_cpp_type(ptype):
    arr = False
    if ptype.startswith('stream<', 0, 7):
        ptype = ptype[(ptype.index('<') + 1):ptype.index('>')]
        arr = True
    ptype_dict = {
        'bool': 'bool',
        'byte': 'uint8_t',
        'i16': 'uint16_t',
        'i32': 'uint32_t',
        'int': 'int',
        'float': 'float',
        'hex': 'uint8_t *',
        'i64': 'uint64_t',
        'double': 'double',
        'null': 'void *',
        'string': 'char *'
    }
    if ptype in list(six.iterkeys(ptype_dict)):
        ptype = ptype_dict[ptype]
    if arr:
        ptype = 'uint8_t [' + ptype + ']'
    return ptype


def get_type_conversion(var, param, ptype, custom, width):
    if not custom and ptype in ['bool', 'byte', 'i16', 'i64', 'hex']:
        return '{} = strtoll(p_{}, NULL, 0);'.format(var, param)
    elif not custom and ((ptype == 'i32') or (ptype == 'int')):
        return '{} = parse_i32(p_{});'.format(var, param)
    elif not custom and (ptype == 'float'):
        return '{} = strtof(p_{}, NULL);'.format(var, param)
    elif not custom and (ptype == 'null'):
        return '{} = NULL;'.format(var)
    elif custom:
        to_return = 'switch(fill_byte_stream({}, p_{}, {})) {{\n'.format(
            var, param, width)
        to_return += '      case -1  :\n'
        to_return += '        {{ bfshell_printf(clish_context, "Hex input too long for parameter {}.\\n"); return -1; }}\n'.format(
            param)
        to_return += '        break;\n'
        to_return += '      case -2  :\n'
        to_return += '        {{ bfshell_printf(clish_context, "Malloc failure in parsing of {} parameter.\\n"); return -1; }}\n'.format(
            param)
        to_return += '        break;\n'
        to_return += '    }'
        return to_return
    elif ptype == 'string':
        return '{} = p_{};'.format(var, param)
    else:
        return None


def get_stub_param(prefix, param):
    if not isinstance(param[1], list):
        return prefix + param[0]
    else:
        prefix = prefix + param[0] + '.'
        splist = []
        for subparam in param[1]:
            sp = get_stub_param(prefix, subparam)
            if isinstance(sp, list):
                splist.extend(sp)
            else:
                splist.append(sp)
        return splist


def get_stub_param_list(params):
    plist = []
    for param in params:
        p = get_stub_param('', param)
        if isinstance(p, list):
            plist.extend(p)
        else:
            plist.append(p)
    return (plist)


def gen_union_returns(fp, fname, param, union_data, display_funcs):
    global idl
    for i, stype in enumerate(union_data[1]):
        if stype is not None:
            res = next((st for st in idl['structs'] if st['name'] == stype),
                       None)
            if res is None:
                print("ERROR: Missing action struct")
            for field in res['fields']:
                name = field['name']
                ptype = field['type']
                cust = field['custom']
                width = field['width']
                if ptype.startswith('stream<', 0, 7):
                    cust = True
                    width = int(ptype[(ptype.index('<') + 1):ptype.index('>')])
                    ptype = 'hex'
                if ptype == 'null':
                    continue
                if cust:
                    if 'union_display_hex_{}'.format(width) in display_funcs:
                        continue
                    display_funcs += ['union_display_hex_{}'.format(width)]
                    fp.write('char *union_display_hex_{}(\n'.format(width))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    uint8_t *{}'.format(name))
                else:
                    if 'union_display_{}'.format(ptype) in display_funcs:
                        continue
                    display_funcs += ['union_display_{}'.format(ptype)]
                    fp.write('char *union_display_{}(\n'.format(ptype))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    {} {}'.format(get_cpp_type(ptype), name))
                fp.write(') {\n')
                if cust:
                    fp.write(
                        '  char *ret = bf_sys_malloc({});\n'.format(width * 2 +
                                                                    3))
                    fp.write('  sprintf(ret, "0x')
                    for j in six.moves.range(0, width):
                        fp.write('%02hhX')
                    fp.write('"')
                    for j in six.moves.range(0, width):
                        fp.write(', {}[{}]'.format(name, j))
                    fp.write(');\n')
                else:
                    if ptype == 'float':
                        fp.write(
                            '  char *ret = bf_sys_malloc(15);\n'
                        )  # 15 = max chars of this format str + null term
                        fp.write('  sprintf(ret, "%6.3e", {});\n'.format(name))
                    elif ptype == 'i64':
                        fp.write('  char *ret = bf_sys_malloc(42);\n'
                                 )  # enough digits for dec, hex, format chars
                        fp.write(
                            '  sprintf(ret, "%" PRIu64 " (%#" PRIx64 ")", {}, {});\n'
                            .format(name, name))
                    else:
                        fp.write('  char *ret = bf_sys_malloc(42);\n')
                        fp.write(
                            '  sprintf(ret, "%u (0x%x)", {}, {});\n'.format(
                                name, name))
                fp.write('    if (p_info->titles_setting == 1) {\n')
                fp.write(
                    '      bfshell_printf(clish_context, "%s%s%s%s", display_str, p_info->title_delimeter, ret, p_info->value_delimiter);\n'
                )
                fp.write(
                    '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s%s%s", display_str, p_info->title_delimeter, ret, p_info->value_delimiter);\n'
                )
                fp.write('    } else if (p_info->titles_setting == 2) {\n')
                fp.write(
                    '      bfshell_printf(clish_context, "%s%s", display_str, p_info->title_delimeter);\n'
                )
                fp.write(
                    '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s", display_str, p_info->title_delimeter);\n'
                )
                fp.write('    } else if (p_info->titles_setting == 0) {\n')
                fp.write(
                    '      bfshell_printf(clish_context, "%s%s", ret, p_info->value_delimiter);\n'
                )
                fp.write(
                    '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s", ret, p_info->value_delimiter);\n'
                )
                fp.write('    }\n')
                fp.write('  return ret;\n')
                fp.write('}\n\n')


def gen_buf_returns(fp, params, fname, namespace, display_funcs):
    for idx, (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
              relation_data) in enumerate(params):
        if buf_ret:
            num_elem_var = None
            if relation_data is not None and 'arr_sz' in relation_data:
                num_elem_var = next(
                    (param for param in params
                     if param[0] == relation_data['arr_sz'][1]), None)
                if num_elem_var is None:
                    print(
                        'ERROR: array size variable: {} is missing.'.format(
                            relation_data['arr_sz'][1]))
            if relation_data is not None and 'union' in relation_data:
                gen_union_returns(fp, fname, param, relation_data['union'],
                                  display_funcs)
                continue
            if ptype == 'null':
                continue
            normal = False
            if num_elem_var is not None and 'struct' not in relation_data:
                if cust:
                    if 'display_hex_{}_arr'.format(width) in display_funcs:
                        continue
                    display_funcs += ['display_hex_{}_arr'.format(width)]
                    fp.write('char *display_hex_{}_arr(\n'.format(width))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    {} {},\n'.format(
                        get_cpp_type(num_elem_var[1]), num_elem_var[0]))
                    fp.write('    uint8_t *{}'.format(param))
                else:
                    if 'display_{}_arr'.format(ptype) in display_funcs:
                        continue
                    display_funcs += ['display_{}_arr'.format(ptype)]
                    fp.write('char *display_{}_arr(\n'.format(ptype))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    {} {},\n'.format(
                        get_cpp_type(num_elem_var[1]), num_elem_var[0]))
                    fp.write('    {} *{}'.format(get_cpp_type(ptype), param))
            else:
                if idx + 1 < len(params) and params[
                        idx + 1][9] and 'union' in params[idx + 1][9]:
                    if 'display_union_field_name' in display_funcs:
                        continue
                    display_funcs += ['display_union_field_name']
                    fp.write('char *display_union_field_name(\n')
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    {} *{}'.format(get_cpp_type(ptype), param))
                elif cust:
                    if 'display_hex_{}'.format(width) in display_funcs:
                        continue
                    display_funcs += ['display_hex_{}'.format(width)]
                    fp.write('char *display_hex_{}(\n'.format(width))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    uint8_t *{}'.format(param))
                else:
                    if 'display_{}'.format(ptype) in display_funcs:
                        continue
                    normal = True
                    display_funcs += ['display_{}'.format(ptype)]
                    fp.write('char *display_{}(\n'.format(ptype))
                    fp.write('    clish_context_t *clish_context,\n')
                    fp.write('    char *display_str,')
                    fp.write('    struct PrintInfo *p_info,\n')
                    fp.write('    {} c_{}'.format(get_cpp_type(ptype), param))
            fp.write(') {\n')
            if num_elem_var is not None and 'struct' not in relation_data:
                fp.write('  int iter = {};\n'.format(num_elem_var[0]))
            else:
                fp.write('  int iter = 1;\n')
                if normal:
                    fp.write('  {} *{} = &c_{};\n'.format(
                        get_cpp_type(ptype), param, param))

            fp.write('  char *ret;\n')
            fp.write('  for (int i = 0; i < iter; i++) {\n')
            if cust:
                fp.write('    ret = bf_sys_malloc({});\n'.format(width * 2 +
                                                                 3))
                fp.write('    sprintf(ret, "0x')
                for i in six.moves.range(0, width):
                    fp.write('%02hhX')
                fp.write('"')
                for i in six.moves.range(0, width):
                    fp.write(', {}[{} + i * {}]'.format(param, i, width))
                fp.write(');\n')
            else:
                if idx + 1 < len(
                        params
                ) and params[idx + 1][9] and 'union' in params[idx + 1][
                        9]:  # we assume the element before the union is the enum
                    global idl
                    fp.write(
                        '    const char *enum_str = p4_{}_action_enum_to_string(*{});\n'
                        .format(namespace, param))
                    fp.write(
                        '    ret = bf_sys_malloc(strlen(enum_str) + 1);\n')
                    fp.write('    strcpy(ret, enum_str);\n')
                elif ptype == 'float':
                    fp.write('    ret = bf_sys_malloc(15);\n'
                             )  # 15 = max chars of this format str + null term
                    fp.write(
                        '    sprintf(ret, "%6.3e", {}[i]);\n'.format(param))
                elif ptype == 'i64':
                    fp.write('    ret = bf_sys_malloc(42);\n'
                             )  # enough digits for dec, hex, format chars
                    fp.write(
                        '    sprintf(ret, "%" PRIu64 " (%#" PRIx64 ")", {}[i], {}[i]);\n'
                        .format(param, param))
                else:
                    fp.write('    ret = bf_sys_malloc(42);\n')
                    fp.write('    sprintf(ret, "%u (0x%x)", {}[i], {}[i]);\n'.
                             format(param, param))
            fp.write('    if (p_info->titles_setting == 1) {\n')
            fp.write(
                '      bfshell_printf(clish_context, "%s%s%s%s", display_str, p_info->title_delimeter, ret, p_info->value_delimiter);\n'
            )
            fp.write(
                '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s%s%s", display_str, p_info->title_delimeter, ret, p_info->value_delimiter);\n'
            )
            fp.write('    } else if (p_info->titles_setting == 2) {\n')
            fp.write(
                '      bfshell_printf(clish_context, "%s%s", display_str, p_info->title_delimeter);\n'
            )
            fp.write(
                '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s", display_str, p_info->title_delimeter);\n'
            )
            fp.write('    } else if (p_info->titles_setting == 0) {\n')
            fp.write(
                '      bfshell_printf(clish_context, "%s%s", ret, p_info->value_delimiter);\n'
            )
            fp.write(
                '      if (p_info->outfile) fprintf(p_info->outfile, "%s%s", ret, p_info->value_delimiter);\n'
            )
            fp.write('    }\n')
            fp.write('    if (i != iter - 1) bf_sys_free(ret);\n')
            fp.write('  }\n')
            fp.write('  return ret;\n')
            fp.write('}\n\n')


# Simple integer types that can be initialized by assignment.
INTEGER_TYPES = [
    'int',
    'uint8_t',
    'uint16_t',
    'uint32_t',
    'uint64_t',
    'p4_pd_sess_hdl_t',
    'p4_pd_entry_hdl_t'
]


def gen_stub_action(fp, service, func, params, namespace, display_funcs):
    fname = func[0].replace('-', '_')
    fdict = func[2]
    gen_buf_returns(fp, params, fname, namespace, display_funcs)
    if (len(params) == 0):
        fp.write('int {}_typecheck('.format(fname))
        fp.write('clish_context_t *clish_context, struct PrintInfo *p_info')
    else:
        fp.write('int {}_typecheck(\n'.format(fname))
        fp.write(
            '    clish_context_t *clish_context, struct PrintInfo *p_info,\n')
    i = 0
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        fp.write('    const char *p_{}'.format(param))
        if (i < (len(params) - 1)):
            fp.write(',\n')
        i += 1
    fp.write(') {\n\n')

    fp.write('  const char *default_init = "0";\n\n')
    i = 0
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        fp.write('  const char *{}_var = NULL;\n'.format(param))
        fp.write(
            '  if ((char*)0x1 != clish_shell_strmap_get(clish_context, p_{})){{\n'
            .format(param))
        fp.write('    {}_var = p_{};\n'.format(param, param))
        fp.write(
            '    p_{} = clish_shell_strmap_get(clish_context, p_{});\n'.format(
                param, param))
        fp.write('    if (!p_{}) p_{} = default_init;\n'.format(param, param))
        fp.write('  }} else if (p_{}[0] == \'$\') {{\n'.format(param))
        fp.write(
            '    bfshell_printf(clish_context, "Implicitly declaring variable: %s\\n", p_{});\n'
            .format(param))
        fp.write(
            '    clish_shell_strmap_insert(clish_context, p_{});\n'.format(
                param))
        fp.write('    {}_var = p_{};\n'.format(param, param))
        fp.write('    p_{} = default_init;\n'.format(param))
        if buf_ret:
            fp.write('  } else {\n')
            fp.write('    clish_shell_strmap_insert(clish_context, "${}");\n'.
                     format(param))
            fp.write('    {}_var = "${}";\n'.format(param, param))
            fp.write('    p_{} = default_init;\n'.format(param))
        fp.write('  };\n')
        if (i == (len(params) - 1)):
            fp.write('\n')
        i += 1

    # declarations
    delay_declare = []
    for arg in fdict['arguments']:
        res = next((param for param in params if param[0] == arg[0]), None)
        if res is not None and res[9] is not None and 'arr_sz' in res[9]:
            delay_declare.append(
                (arg[1], arg[0], res[9]['arr_sz'][1], res[5], res[6]))
        elif len(arg) > 8 and 'arr_sz' in arg[8]:
            delay_declare.append(
                (arg[1], arg[0], arg[8]['arr_sz'][1], False, 1))
        else:
            ctype = get_cpp_type(arg[1])
            if ctype in INTEGER_TYPES:
                # If it's an integral type, initialize to zero.
                fp.write('  {} {} = 0;\n'.format(ctype, arg[0]))
            elif ctype == 'bool':
                # If it's a Boolean, initialize to false.
                fp.write('  {} {} = false;\n'.format(ctype, arg[0]))
            else:
                # Define variable and memset to zero.
                fp.write('  {} {};\n'.format(ctype, arg[0]))
                fp.write('  memset(&{0}, 0, sizeof({0}));\n'.format(arg[0]))
    fp.write('\n')

    # return type
    nargs = len(fdict['arguments'])
    rettype = fdict['returnType']
    if rettype is not None:
        fp.write('  {} ret;'.format(get_cpp_type(rettype)))
        simple_ret_type = is_simple_cpp_type(rettype)
    fp.write('\n\n')

    param_list = get_stub_param_list(func[1])
    # fill args
    i = 0
    for (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
         relation_data) in params:
        if relation_data is None:
            fp.write('  if (p_{}) {{\n'.format(param))
            conv = get_type_conversion(param_list[i], param, ptype, cust, width)
            fp.write('    {}\n'.format(conv))
            fp.write('  }\n')
        i += 1
    fp.write('\n')

    # Generate code to perform input validation on the number of entries when
    # it is the result of parsing a string. We do this for self-protection and
    # to suppress a Klocwork warning.
    for delayed in delay_declare:
        if not delayed[3]:
            fp.write('  if ({0} < 1 || {0} >= MAX_NUM_ENTRIES) {{\n'.format(delayed[2]))
            fp.write('    bfshell_printf(clish_context, "invalid number of entries\\n");\n')
            fp.write('    return -1;\n')
            fp.write('  }\n')
    if len(delay_declare):
        fp.write('\n')

    # 0:array_type, 1:array_name, 2:array_size1, 3:is_table, 4:array_size2
    for delayed in delay_declare:
        if delayed[3]:
            fp.write('  {} {}[{} * {}];\n'.format('uint8_t', delayed[1],
                                                  delayed[2], delayed[4]))
            fp.write('  memset({}, 0, {} * {});\n'.format(
                delayed[1], delayed[2], delayed[4]))
        else:
            fp.write('  {} {}[{}];\n'.format(get_cpp_type(delayed[0]),
                                             delayed[1], delayed[2]))
            fp.write('  memset({0}, 0, sizeof({0}));\n'.format(delayed[1]))

    # actual call
    fp.write('  ')
    if ((rettype is not None) and simple_ret_type):
        fp.write('ret = ')
    if (nargs == 0):
        fp.write('{}('.format(fname))
        if ((rettype is not None) and not simple_ret_type):
            fp.write('ret')
    else:
        fp.write('{}(\n'.format(fname))
        if ((rettype is not None) and not simple_ret_type):
            fp.write('    ret,\n')
    i = 0
    for arg in fdict['arguments']:
        if arg[3]:
            res = next(
                (delay for delay in delay_declare if delay[1] == arg[0]), None)
            if res is not None:
                fp.write('    {}'.format(arg[0]))
            else:
                fp.write('    &{}'.format(arg[0]))
        else:
            fp.write('    {}'.format(arg[0]))
        if (i < (nargs - 1)):
            fp.write(',\n')
        i += 1
    fp.write(');\n\n')

    #print filled buffers
    global idl
    if ((rettype is not None) and simple_ret_type):
        fp.write('  if (ret == 0) {\n')
        val_init = True
        for idx, (param, ptype, atype, req, ptr, cust, width, default, buf_ret,
                  relation_data) in enumerate(params):
            if buf_ret:
                if val_init:
                    fp.write('    char *value_ = NULL;\n')
                    val_init = False
                if relation_data is None or 'union' not in relation_data:
                    if ptype == 'null':
                        continue
                    if relation_data is not None \
                            and 'struct' in relation_data \
                            and not relation_data['struct']:
                        continue
                    num_elem_var = None
                    if relation_data is not None and 'arr_sz' in relation_data:
                        num_elem_var = next(
                            (param for param in params
                             if param[0] == relation_data['arr_sz'][1]), None)
                        if num_elem_var is None:
                            print(
                                'ERROR: array size variable: {} is missing.'.
                                format(relation_data['arr_sz'][1]))
                    if num_elem_var is not None \
                            and relation_data is not None \
                            and 'struct' in relation_data \
                            and relation_data['struct']:
                        fp.write(
                            '    for (int i = 0; i < {}; i++) {{\n'.format(
                                num_elem_var[0]))
                        cur_idx = idx
                        while params[cur_idx][
                                9] is not None and 'struct' in params[cur_idx][
                                    9]:
                            params[cur_idx][9]['struct'] = False
                            varname = param_list[cur_idx]
                            dot = varname.find('.')
                            varname = varname[:dot] + '[i]' + varname[dot:]
                            if params[cur_idx][5]:
                                fp.write(
                                    '        value_ = display_hex_{}(clish_context, "{}", p_info, {});\n'
                                    .format(params[cur_idx][6],
                                            params[cur_idx][0], varname))
                            else:
                                fp.write(
                                    '        value_ = display_{}(clish_context, "{}", p_info, {});\n'
                                    .format(params[cur_idx][1],
                                            params[cur_idx][0], varname))
                            fp.write('    bf_sys_free(value_);\n')
                            cur_idx += 1
                        fp.write('    }\n')
                        continue
                    if idx + 1 < len(params) and params[idx + 1][9]:
                        fp.write(
                            '    value_ = display_union_field_name(clish_context, "{}", p_info, (uint8_t *) &{});\n'
                            .format(param, param_list[idx]))
                    elif num_elem_var is not None and not cust:
                        fp.write(
                            '    value_ = display_{}_arr(clish_context, "{}", p_info, {}, {});\n'
                            .format(ptype, param, num_elem_var[0],
                                    param_list[idx]))
                    elif num_elem_var is not None and cust:
                        fp.write(
                            '    value_ = display_hex_{}_arr(clish_context, "{}", p_info, {}, {});\n'
                            .format(width, param, num_elem_var[0],
                                    param_list[idx]))
                    elif num_elem_var is None and cust:
                        fp.write(
                            '    value_ = display_hex_{}(clish_context, "{}", p_info, {});\n'
                            .format(width, param, param_list[idx]))
                    else:
                        fp.write(
                            '    value_ = display_{}(clish_context, "{}", p_info, {});\n'
                            .format(ptype, param, param_list[idx]))
                    fp.write('    if ({}_var) {{\n'.format(param))
                    fp.write(
                        '      clish_shell_strmap_set(clish_context, {}_var, value_);\n'
                        .format(param))
                    fp.write('    }\n')
                    fp.write('    bf_sys_free(value_);\n')
                else:
                    # Note: we don't fill variables from unions because we
                    # don't know how many fields are in their structs.
                    union_data = relation_data['union']
                    fp.write('    switch({}) {{\n'.format(param_list[
                        idx -
                        1]))  #we assume the enum is the item before the union
                    for j in six.moves.range(0, len(union_data[1])):
                        fp.write('      case {} : {{\n'.format(j))
                        if union_data[1][j] is not None:
                            res = next((st for st in idl['structs']
                                        if st['name'] == union_data[1][j]),
                                       None)
                            if res is None:
                                print("ERROR: Missing action struct")
                            for field in res['fields']:
                                if field['type'].startswith('stream<', 0, 7):
                                    field['custom'] = True
                                    field['width'] = int(field['type'][(
                                        field['type'].index('<') +
                                        1):field['type'].index('>')])
                                    field['type'] = 'hex'
                                if field['custom']:
                                    fp.write(
                                        '    value_ = union_display_hex_{}(clish_context, "{}", p_info, {}.{}.{});\n'
                                        .format(field['width'], field['name'],
                                                param_list[idx],
                                                union_data[2][j],
                                                field['name']))
                                else:

                                    fp.write(
                                        '    value_ = union_display_{}(clish_context, "{}", p_info, {}.{}.{});\n'
                                        .format(field['type'], field['name'],
                                                param_list[idx],
                                                union_data[2][j],
                                                field['name']))
                                fp.write('        bf_sys_free(value_);\n')
                        else:
                            fp.write('        //no data\n')
                        fp.write('        break;\n')
                        fp.write('      }\n')
                    fp.write('      default:\n')
                    fp.write('        break;\n')
                    fp.write('    }\n')
        fp.write('    return 0;\n')
        fp.write('  }\n')
        fp.write('  bfshell_printf(clish_context, "operation failed\\n");\n')
        fp.write('  return -1;\n')
    else:
        fp.write('  return 0;\n')
    fp.write('}\n\n')
    return


def gen_stub_action_end(fp):
    fp.write('}\n')
    return


### End Client RPC Stub Generation ###


def is_base_type(atype):
    if atype in {
            'bool', 'byte', 'i16', 'i32', 'int', 'float', 'hex', 'i64',
            'double', 'null', 'string', 'union'
    }:
        return True
    return False


def get_type(arg):
    global idl
    pname = arg[0]
    ptype = arg[1]
    req = arg[2]
    ptr = arg[3]
    cust = arg[4]
    width = arg[5]
    default = arg[6]
    buf_ret = arg[7]
    relation_data = None
    if len(arg) >= 9:
        relation_data = arg[8]
    list_type = False

    # stream
    if ptype.startswith('stream<', 0, 7):
        cust = True
        width = int(ptype[(ptype.index('<') + 1):ptype.index('>')])
        ptype = 'hex'

    # base types
    if is_base_type(ptype):
        if len(arg) == 9:  #we have relation data
            return [
                pname, ptype, req, list_type, ptype, ptr, cust, width, default,
                buf_ret, relation_data
            ]
        return [
            pname, ptype, req, list_type, ptype, ptr, cust, width, default,
            buf_ret, relation_data
        ]

    # enums
    if ptype.startswith('enum '):
        res = next(
            (enum for enum in idl['enums'] if enum['name'] == ptype[5:]), None)
        if res is not None:
            return [
                pname, 'byte', req, list_type, ptype, ptr, cust, width,
                default, buf_ret, relation_data
            ]

    # typedefs
    res = next((ty for ty in idl['typedefs'] if ty['name'] == ptype), None)
    if res is not None:
        if res['type'].startswith('list<', 0, 5):
            list_type = True
        targ = [
            pname, res['type'], req, ptr, cust, width, default, buf_ret,
            relation_data
        ]
        ttype = get_type(targ)
        return ttype

    # structs
    res = next((st for st in idl['structs'] if st['name'] == ptype), None)
    if res is not None:
        flist = []
        for field in res['fields']:
            if field['type'] == 'union':
                targ = [
                    field['name'], field['type'], req, field['pointer'],
                    field['custom'], field['width'], field['default'], buf_ret,
                    field['relation_data']
                ]
            else:
                if relation_data is not None and 'arr_sz' in relation_data:
                    relation_data['struct'] = True
                targ = [
                    field['name'], field['type'], req, field['pointer'],
                    field['custom'], field['width'], field['default'], buf_ret,
                    relation_data
                ]
            flist.append(get_type(targ))
        return [
            pname, flist, req, list_type, ptype, ptr, cust, width, default,
            buf_ret, relation_data
        ]

    print("WARNING: RETURNING NONE! {}, {}, {}, {}, {}, {}".format(
        pname, ptype, req, ptr, cust, width))
    return None


def get_args(args):
    arglist = []
    for arg in args:
        params = get_type(arg)
        arglist.append(params)
    return arglist


def process_function(func):
    return [func[0], get_args(func[1])]


def build_service_list(services):
    slist = []
    for service in services:
        flist = []
        for func in service['functions']:
            f = process_function(func)
            f.append({
                'name': func[0],
                'arguments': func[1],
                'returnType': func[2]
            })
            if len(func) > 3:
                f[2]['synonyms'] = func[3]
            flist.append(f)
        slist.append([service['name'], flist])
    return slist


def process_services(xout, pout, libdir, namespace, p4_name, services):
    slist = build_service_list(services)

    xmlf = open(xout + p4_name + '.xml', 'w')
    actf = open(pout, 'w')
    # current version of the command line doesn't use rpc
    # removing creation of stub file
    # stubf = open(pout[:-2] + '_stub.cc', 'w')

    gen_cli_header(xmlf)
    gen_act_header(actf)
    display_funcs = []
    # gen_stub_header(stubf, namespace, services)
    for (service, functions) in slist:
        gen_cli_top_cmd(xmlf, service, p4_name, libdir)
        # gen_stub_client_init(stubf, service)
        # gen_stub_action_start(stubf)
        for func in functions:
            gen_cli_cmd(xmlf, service, func, namespace, p4_name)
            params = get_act_params(func)
            gen_stub_action(actf, service, func, params, namespace,
                            display_funcs)
            gen_act_function(actf, func[0], params)
            # gen_stub_action(stubf, service, func, params)
        for func in functions:
            gen_top_cmds(xmlf, service, func, namespace, p4_name)
        # gen_stub_action_end(stubf)
    gen_cli_footer(xmlf)
    gen_dump_plugins(actf)
    for (service, functions) in slist:
        gen_act_footer(actf, p4_name, functions)

    xmlf.close()
    actf.close()
    # stubf.close()
    return


def construct_parser():
    parser = argparse.ArgumentParser(description='cli xml, plugin generation')
    parser.add_argument('idl_json',
                        metavar='idl',
                        type=str,
                        help='Path to the idl json file')
    parser.add_argument('-po',
                        '--pout',
                        required=True,
                        metavar='plugin_output_dir',
                        type=str,
                        help='Path to the plugin source output directory')
    parser.add_argument('-xo',
                        '--xout',
                        required=True,
                        metavar='xml_output_dir',
                        type=str,
                        help='Path to xml output directory')
    parser.add_argument('-xd',
                        '--xdel',
                        required=True,
                        metavar='xml_delete_dir',
                        type=str,
                        help='Path to stale xml deletion directory')
    parser.add_argument('-ll',
                        '--libdir',
                        required=True,
                        metavar='pd_library_dir',
                        type=str,
                        help='Path to shared library install directory')
    return parser


def process_cmd_args():
    parser = construct_parser()
    args, unused = parser.parse_known_args()

    if not os.path.exists(args.pout):
        os.makedirs(args.pout)

    if not os.path.exists(args.xout):
        os.makedirs(args.xout)

    if not args.xout.endswith('/'):
        args.xout += '/'

    if not args.libdir.endswith('/'):
        args.libdir += '/'

    if not args.pout.endswith('/'):
        args.pout += '/'

    return args


def main():
    args = process_cmd_args()

    global idl
    with open(args.idl_json) as fp:
        idl = json.load(fp)

    args.pout += 'pdcli.c'

    process_services(args.xout, args.pout, args.libdir, idl['namespace'],
                     idl['p4_name'], idl['services'])
    return


if __name__ == '__main__':
    main()
    exit(0)
