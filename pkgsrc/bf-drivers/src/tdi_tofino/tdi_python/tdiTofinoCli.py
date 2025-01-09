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

from __future__ import print_function
from traitlets.config import Config
from prompt_toolkit.input.defaults import create_input
from prompt_toolkit.output.defaults import create_output
from prompt_toolkit.application.current import get_app_session
from ctypes import *
import sys
import os
import builtins
import keyword
import IPython
import types
import tabulate
import pdb
import re
import atexit
import json
import operator
import copy

sys.path.append('../../../third-party/tdi/tdi_python/')
from tdicli import *
from tdiInfo import *
from tdiTofinoTable import *

# warm-init APIs
def warm_init_begin(self, dev_id, init_mode="INIT_COLD", upgrade_agents=None, serdes_upgrade_mode="SERDES_UPD_NONE", p4_programs=None):
    """
    Command to initiate a warm init process for a device
    Args:
        dev_id                          (integer, mandatory)    : Device to warm-init
        init_mode                       (string, optional)      : The warm-init mode to use for the device
            Choices:
                "INIT_COLD"
                "WARM_INIT_FAST_RECFG"
                "WARM_INIT_HITLESS"
                Defaults to "INIT_COLD".
        upgrade_agents                  (bool, optional)        : Flag to denote whether to upgrade the agents. Defaults to None
        serdes_upgrade_mode             (string, optional)      : The mode to use for updating SerDes
            Choices:
                "SERDES_UPD_NONE"
                "SERDES_UPD_FORCED_PORT_RECFG"
                "SERDES_UPD_DEFERRED_PORT_RECFG"
                Defaults to "SERDES_UPD_NONE"
        p4_programs                     (list, optional)        : List of p4_program objects. Defaults to None
            p4_program:
                p4_name         (string, mandatory)             : Name of the p4 program
                base_path       (string, mandatory)             : Absolute path to p4_program directory
                tdi_json_file   (string, mandatory)             : Name of the tdi json file
                p4_pipelines    (list, mandatory)               : List of p4_pipeline objects
                        context_file    (string, mandatory)     : Name of the context file
                        binary_file     (string, mandatory)     : Name of the binary file
                        pipeline_name   (string, mandatory)     : Name of the pipeline
                        pipe_scope      (list, mandatory)       : List of pipes

            Note: tdi_json_file is expected to be present under <base_path>/<p4_name>
                  context_file and binary file are expected to be present under <base_path>/<p4_name>/<pipeline_name>
            Example:
            p4_programs=[{"p4_name":"tna_exact_match", "base_path": "/home_dir/workspace/install/share/tofinopd",
            "p4_pipelines":[{"context_file":"context.json", "binary_file":"tofino.bin", "pipeline_name":"pipe", "pipe_scope":[0, 1, 2, 3]}],
            "tdi_json_file":"bf-rt.json"}]

    Returns:
        integer: 0 if the warm_init_begin is success. -1 if there is a failure

    """
    # python representation of tdi_pipeline_t
    class Pipeline(Structure):
        _fields_ = [('name', c_char_p),
            ('context_path', c_char_p),
            ('binary_path', c_char_p),
            ('scope_vec', POINTER(c_int32)),
            ('num_pipes', c_int)]

    init_mode_c = c_char_p(init_mode.encode('ascii'))
    serdes_upgrade_mode_c = c_char_p(serdes_upgrade_mode.encode('ascii'))

    if upgrade_agents is not None and type(upgrade_agents) != bool:
        print("ERROR: upgrade_agents provided is not bool")
        return -1

    if p4_programs is None:
        sts = self._driver.device_warm_init_begin(dev_id, init_mode_c, serdes_upgrade_mode_c, upgrade_agents, None)
        if not sts == 0:
            print("ERROR: Invalid arguments passed")
            return -1
        return 0

    num_programs = len(p4_programs)

    self.dev_config_hdl = POINTER(c_uint)
    self.dev_config = self.dev_config_hdl()
    sts = self._driver.dev_config_allocate(num_programs, byref(self.dev_config))
    if not sts == 0:
        print("ERROR: Failed to allocate memory for device config")
        return -1

    prog_index = 0
    for program in p4_programs:
        program_name = c_char_p(program['p4_name'].encode('ascii'))
        self._driver.set_program_name(self.dev_config, prog_index, program_name)

        program_path = program['base_path'] + '/' + program['p4_name']
        tdi_info_path = program_path + '/' + program['tdi_json_file']
        tdi_info_path_c = c_char_p(tdi_info_path.encode('ascii'))
        sts = self._driver.set_tdi_info_path(self.dev_config, prog_index, tdi_info_path_c)
        if not sts == 0:
            print("ERROR: Invalid file name or path: ", program_path)
            self._driver.dev_config_deallocate(self.dev_config)
            return -1

        for pipeline in program['p4_pipelines']:
            pipeline_c = Pipeline()
            pipeline_c.name = c_char_p(pipeline['pipeline_name'].encode('ascii'))

            context_path = program_path + '/' + pipeline['pipeline_name'] + '/' + pipeline['context_file']
            binary_path = program_path + '/' + pipeline['pipeline_name'] + '/' + pipeline['binary_file']
            pipeline_c.context_path = c_char_p(context_path.encode('ascii'))
            pipeline_c.binary_path = c_char_p(binary_path.encode('ascii'))

            pipe_scope = pipeline['pipe_scope']
            num_pipes = len(pipe_scope)
            array_type = c_int32*num_pipes
            pipeline_c.scope_vec = array_type(*pipe_scope)
            pipeline_c.num_pipes = num_pipes
            sts = self._driver.set_pipeline(self.dev_config, prog_index, pipeline_c)
            if not sts == 0:
                print("ERROR: Invalid context or binary path")
                self._driver.dev_config_deallocate(self.dev_config)
                return -1

        prog_index = prog_index + 1

    # delete previous session
    sts = destroy_tdi_python_session(dev_id, self._driver)
    if not sts == 0:
        self._driver.dev_config_deallocate(self.dev_config)
        return -1

    sts = self._driver.device_warm_init_begin(dev_id, init_mode_c, serdes_upgrade_mode_c, upgrade_agents, self.dev_config)
    if not sts == 0:
        print("ERROR: Invalid arguments passed")
        self._driver.dev_config_deallocate(self.dev_config)
        return -1

    device_hdl = POINTER(c_uint)()
    sts = self._driver.tdi_device_get(dev_id, byref(device_hdl))
    # Allocate session again
    sts = create_tdi_python_session(dev_id, self._driver, device_hdl)
    if sts != 0:
        return sts

    self._driver.dev_config_deallocate(self.dev_config)
    return 0

def warm_init_end(self, dev_id):
    """
    Command to initiate warm-init end for a device

    Args:
        dev_id (integer, mandatory):    Device to warm-init end

    Returns:
        integer: 0 if the warm_init_begin is success. -1 if there is a failure
    """
    sts = self._driver.device_warm_init_end(dev_id)
    if not sts == 0:
        print("ERROR: Invalid arguments passed")
        return -1
    return 0

# default callbacks
def _port_status_notif_cb_print(dev_id, dev_port, up):
    print("Device id: {} Dev port : {} Port Status : {}".format(dev_id, dev_port, up))

def _idle_table_notify_print(dev_id, pipe_id, direction, parser_id, entry):
    print("Device id: {}\n"
          "Pipe id: {}\n"
          "Direction: {}\n"
          "Parser id: {}\n".format(dev_id, pipe_id, direction, parser_id))
    print(entry)

def _selector_table_update_cb_print(dev_id, pipe_id, direction, parser_id, sel_grp_id, act_mbr_id, logical_table_index, is_add):
    print("Selector update callback called\n")
    print("Device id: {}\n"
          "Pipe id: {}\n"
          "Direction: {}\n"
          "Parser id: {}\n"
          "sel_grp_id: {}\n"
          "act_mbr_id: {}\n"
          "logical_table_index: {}\n"
          "is_add: {}\n"
          .format(dev_id, pipe_id, direction, parser_id, sel_grp_id, act_mbr_id, logical_table_index, is_add))
class TDILeafTofino(TDILeaf):
    def port_status_notif_cb_set(self, callback=None):
        if callback is None:
           callback = _port_status_notif_cb_print
        self._c_tbl.set_port_status_notif_cb(callback)

    def port_stats_poll_intv_set(self, poll_intv_ms=2000):
        self._c_tbl.set_port_stats_poll_intv(poll_intv_ms)

    def port_stats_poll_intv_get(self):
        return self._c_tbl.get_port_stats_poll_intv()

    def idle_table_set_poll(self, enable):
        self._c_tbl.set_idle_table_poll_mode(enable)

    def idle_table_set_notify(self, enable, callback=None, interval=1000, max_ttl=0, min_ttl=0):
        if callback is None:
            callback = _idle_table_notify_print
        if not interval:
            print("{} Error: non-zero interval is required when enabling notify mode.")
            return
        self._c_tbl.set_idle_table_notify_mode(enable, callback, interval, max_ttl, min_ttl)

    def idle_table_get(self):
        ret = self._c_tbl.get_idle_table()
        if ret == -1:
            return
        return ret

    def symmetric_mode_set(self, enable):
        self._c_tbl.set_symmetric_mode(enable)

    def symmetric_mode_get(self):
        ret = self._c_tbl.get_symmetric_mode()
        if ret == -1:
            return
        return ret

    @target_check_and_set
    def meter_byte_count_adjust_set(self, byte_count, pipe=None, gress_dir=None, prsr_id=None):
        self._c_tbl.meter_byte_count_adjust_set(byte_count)

    @target_check_and_set
    def meter_byte_count_adjust_get(self, pipe=None, gress_dir=None, prsr_id=None):
        sts, ret = self._c_tbl.meter_byte_count_adjust_get()
        if sts == -1:
            return
        return ret

    def selector_table_update_cb_set(self, callback=None):
        if callback is None:
           callback = _selector_table_update_cb_print
        self._c_tbl.set_selector_table_update_cb(callback)

    def dynamic_key_mask_get(self):
        ret = self._c_tbl.dynamic_key_mask_get()
        if ret == -1:
            return
        return ret

    TDILeaf.target_check_and_set = target_check_and_set

    # Generating add_with_<action> APIs dynamically
    def _create_add_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None):
    """Add entry to {} table with action: {}

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}",{}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.add_entry(parsed_keys, parsed_data, b'{}')
        '''
        TDILeaf._create_add_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating add APIs dynamically
    def _create_add(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None):
    """Add entry to {} table.

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.add_entry(parsed_keys, parsed_data)
        '''
        TDILeaf._create_add(self, key_fields, data_fields, code_str)

    # Generating set_default_with_<action> APIs dynamically
    def _create_set_default_with_action(self, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None):
    """Set default action for {} table with action: {}

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.set_default_entry(parsed_data, b'{}')
        '''
        TDILeaf._create_set_default_with_action(self, data_fields, action_name, code_str)

    # Generating set_default API dynamically
    def _create_set_default(self, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None):
    """Set default action for {} table.

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return
    self._c_tbl.set_default_entry(parsed_data)
        '''
        TDILeaf._create_set_default(self, data_fields, code_str)

    # Generating reset_default API dynamically
    def _create_reset_default(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, pipe=None, gress_dir=None, prsr_id=None):
    """Set default action for {} table.
    """
    self._c_tbl.reset_default_entry()
        '''
        TDILeaf._create_reset_default(self, code_str)

    # Generating mod_with_<action> APIs dynamically
    def _create_mod_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None, ttl_reset=True):
    """Modify entry in {} table using action: {}

    Parameters:
    {}
    ttl_reset: default=True, set to False in order to not start aging entry from the beggining.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.mod_entry(parsed_keys, parsed_data, b'{}', ttl_reset=ttl_reset)'''
        TDILeaf._create_mod_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating mod_inc_with_<action> APIs dynamically
    def _create_mod_inc_with_action(self, key_fields, data_fields, action_name):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} mod_flag=0, pipe=None, gress_dir=None, prsr_id=None):
    """ Incremental Add/Delete items in the fields that are array for the matched entry in {} table.

    Parameters:
    {}
    mod_flag: 0/1 value for Add/Delete Item in the field that is array type.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {}, b'{}')
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.mod_inc_entry(parsed_keys, parsed_data, mod_flag, b'{}')
        '''
        TDILeaf._create_mod_inc_with_action(self, key_fields, data_fields, action_name, code_str)

    # Generating mod API dynamically
    def _create_mod(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None, ttl_reset=True):
    """Modify any field in the entry at once in {} table.

    Parameters:
    {}
    ttl_reset: default=True, set to False in order to not start aging entry from the beggining.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.mod_entry(parsed_keys, parsed_data, ttl_reset=ttl_reset)
        '''
        TDILeaf._create_mod(self, key_fields, data_fields, code_str)

    # Generating mod_inc API dynamically
    def _create_mod_inc(self, key_fields, data_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} mod_flag=0, pipe=None, gress_dir=None, prsr_id=None):
    """ Incremental Add/Delete items in the fields that are array for the matched entry in {} table.

    Parameters:
    {}
    mod_flag: 0/1 value for Add/Delete Item in the field that is array type.
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    if parsed_data == -1:
        return

    self._c_tbl.mod_inc_entry(parsed_keys, parsed_data, mod_flag)
        '''
        TDILeaf._create_mod_inc(self, key_fields, data_fields, code_str)

    # Generating delete API dynamically
    def _create_del(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} pipe=None, gress_dir=None, prsr_id=None, handle=None):
    """Delete entry from {} table.

    Parameters:
    {}
    """
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return

    self._c_tbl.del_entry(parsed_keys, entry_handle=handle)
        '''
        TDILeaf._create_del(self, key_fields, code_str)

    # Generating get API dynamically
    def _create_get(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} regex=False, return_ents=True, print_ents=True, table=False, from_hw=False, pipe=None, gress_dir=None, prsr_id=None, handle=None):
    """Get entry from {} table.
    If regex param is set to True, perform regex search on key fields.
    When regex is true, default for each field is to accept all.
    We use the python regex search() function:
    https://docs.python.org/3/library/re.html

    Parameters:
    {}
    regex: default=False
    from_hw: default=False
    """
    if regex or table:
        etd = TableEntryDumper(self._c_tbl)
        sts = self._c_tbl.dump(etd, from_hw=from_hw, print_ents=print_ents)
        if sts == 0 and print_ents:
            if regex:
                etd.print_table(regex_strs={})
            else:
                etd.print_table()
        if return_ents:
            return etd.dump_entry_objs(regex_strs={})
        return
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return -1
    objs = self._c_tbl.get_entry(parsed_keys, print_entry=print_ents, from_hw=from_hw, entry_handle=handle)
    if return_ents:
        return objs
    return
        '''
        TDILeaf._create_get(self, key_fields, code_str)

    # Generating get_handle API dynamically
    def _create_get_handle(self, key_fields):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, {} regex=False, return_ents=True, print_ents=True, table=False, from_hw=False, pipe=None, gress_dir=None, prsr_id=None):
    """Get entry handle for specific key from {} table.
    If regex param is set to True, perform regex search on key fields.
    When regex is true, default for each field is to accept all.
    We use the python regex search() function:
    https://docs.python.org/3/library/re.html

    Parameters:
    {}
    regex: default=False
    from_hw: default=False
    handle: default=None
    """
    if regex or table:
        etd = TableEntryDumper(self._c_tbl)
        sts = self._c_tbl.dump(etd, from_hw=from_hw, print_ents=print_ents)
        if sts == 0 and print_ents:
            if regex:
                etd.print_table(regex_strs={})
            else:
                etd.print_table()
        if return_ents:
            return etd.dump_entry_objs(regex_strs={})
        return
    parsed_keys, _ = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return -1
    objs = self._c_tbl.get_entry_handle(parsed_keys, print_entry=print_ents, from_hw=from_hw)
    if return_ents:
        return objs
    return
        '''
        TDILeaf._create_get_handle(self, key_fields, code_str)

    # Generating get_key API dynamically
    def _create_get_key(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, handle, return_ent=True, print_ent=True, from_hw=False, pipe=None, gress_dir=None, prsr_id=None):
    """Get entry key from {} table by entry handle.

    Parameters:
    handle: type=UINT64 size=32
    from_hw: default=False
    print_ents: default=True
    pipe: default=None (use global setting)
    """
    if handle < 0 or handle >= 2**(sizeof(c_uint32)*8):
        return -1
    objs = self._c_tbl.get_entry_key(entry_handle=handle, print_entry=print_ent, from_hw=from_hw)
    if return_ent:
        return objs
    return
        '''
        TDILeaf._create_get_key(self, code_str)

    # Generating get_default API dynamically
    def _create_get_default(self):
        code_str = '''
@TDILeaf.target_check_and_set
def {}(self, return_ent=True, print_ent=True, from_hw=False, pipe=None, gress_dir=None, prsr_id=None):
    """Get default entry from {} table.

    Parameters:
    from_hw: default=False
    print_ents: default=True
    table: default=False
    pipe: default=None (use global setting)
    """
    objs = self._c_tbl.get_default_entry(print_entry=print_ent, from_hw=from_hw)
    if return_ent:
        return objs
    return
        '''
        TDILeaf._create_get_default(self, code_str)

    def _create_attributes(self, key_fields={}, data_fields={}):
        method_name= "dynamic_key_mask_set"
        if method_name not in self._c_tbl.supported_commands:
            return
        param_str, param_docstring, parse_key_call, parse_data_call, param_list = self._make_core_method_strs(method_name, key_fields, data_fields)

        code = '''
def {}(self, {} ):
    """Add dynamic mask attribute to {} .

    Parameters:
    {}
    """
    parsed_keys, parsed_data = self._c_tbl.parse_str_input("{}", {}, {})
    if parsed_keys == -1:
        return
    self._c_tbl.dynamic_key_mask_set(parsed_keys)
        '''.format(method_name, param_str, self._name, param_docstring, method_name, parse_key_call, parse_data_call)
        add_method = self._set_dynamic_method(code, method_name)
        self._children[method_name] = getattr(self, method_name)

    @target_check_and_set
    def info(self, pipe=None, return_info=False, print_info=True):
        return TDILeaf.info(self, return_info, print_info)

    @target_check_and_set
    def clear(self, pipe=None, gress_dir=None, prsr_id=None, batch=True):
        TDILeaf.clear(self, batch=True)

    @target_check_and_set
    def dump(self, table=False, pipe=None, gress_dir=None, prsr_id=None, json=False, from_hw=False, return_ents=False, print_zero=True):
        return TDILeaf.dump(self, table, json, from_hw, return_ents, print_zero)

class CIntfTofino(CIntfTdi):
    target_type_cls = TargetTypeTofino
    leaf_type_cls = TDILeafTofino

    def __init__(self, dev_id, table_cls, info_cls):
        driver_path = install_directory+'/lib/libdriver.so'
        super().__init__(dev_id, TofinoTable, TdiInfo, driver_path)

        self.tdi_idle_timeout_cb_type = CFUNCTYPE(c_int, POINTER(self.TdiDevTgt), self.handle_type, c_void_p)
        self.port_status_notif_cb_type = CFUNCTYPE(c_int, c_uint, self.handle_key_type, c_bool, c_void_p)

    def _set_pipe(self, pipe=0xFFFF):
        pipe_id = c_uint64(pipe);
        sts = self.get_driver().tdi_target_set_value(self._target, self.target_type_cls.target_type_map(target_type_str="pipe_id"), pipe_id);

    def _set_direction(self, direction=0xFFFF):
        sts = self.get_driver().tdi_target_set_value(self._target, self.target_type_cls.target_type_map(target_type_str="direction"), c_uint64(direction));

    def _set_parser(self, parser=0xFF):
        sts = self.get_driver().tdi_target_set_value(self._target, self.target_type_cls.target_type_map(target_type_str="prsr_id"), c_uint64(parser));

    def set_target_vals(self, dev_id, pipe_id, direction, prsr_id, target):
        sts = self.get_driver().tdi_target_set_value(target, self.target_type_cls.target_type_map(target_type_str="dev_id"), dev_id);
        sts = self.get_driver().tdi_target_set_value(target, self.target_type_cls.target_type_map(target_type_str="pipe_id"), pipe_id);
        sts = self.get_driver().tdi_target_set_value(target, self.target_type_cls.target_type_map(target_type_str="direction"), direction);
        sts = self.get_driver().tdi_target_set_value(target, self.target_type_cls.target_type_map(target_type_str="prsr_id"), prsr_id);
        return

    def print_target(self, target):
        return "dev_id={} pipe={} direction={} prsr_id={}".format(*(self.get_target_vals(target)))

    def get_target_vals(self, target):
        dev_id = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="dev_id"), byref(dev_id));
        pipe_id = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="pipe_id"), byref(pipe_id));
        direction = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="direction"), byref(direction));
        prsr_id = c_uint64(0);
        sts = self.get_driver().tdi_target_get_value(target, self.target_type_cls.target_type_map(target_type_str="prsr_id"), byref(prsr_id));
        return dev_id.value, pipe_id.value, direction.value, prsr_id.value

class TofinoCli(TdiCli):
    # Fixed Tables created at child Nodes of the root 'tdi' Node.
    fixed_nodes = ["mirror", "tf1", "tf2", "tf3"]
    cIntf_cls = CIntfTofino

    def fill_dev_node(self, cintf, dev_node):
        dev_node.devcall = cintf._devcall
        dev_node.set_pipe = cintf._set_pipe
        dev_node.set_direction = cintf._set_direction
        dev_node.set_parser = cintf._set_parser
        dev_node.complete_operations = cintf._complete_operations
        dev_node.batch_begin = cintf._begin_batch
        dev_node.batch_flush = cintf._flush_batch
        dev_node.batch_end = cintf._end_batch
        dev_node.transaction_begin = cintf._begin_transaction
        dev_node.transaction_verify = cintf._verify_transaction
        dev_node.transaction_commit = cintf._commit_transaction
        dev_node.transaction_abort = cintf._abort_transaction
        dev_node.p4_programs_list = []

    def create_warm_init_node(self, tdi):
        warm_init_node = TDINode("warm_init", None, parent_node=tdi)
        driver_path = install_directory+'/lib/libdriver.so'
        warm_init_node._driver = CDLL(driver_path)

        # add warm_init APIs to the node
        warm_init_node.warm_init_begin = types.MethodType(warm_init_begin, warm_init_node)
        warm_init_node.warm_init_end = types.MethodType(warm_init_end, warm_init_node)
        warm_init_node._commands = {}
        warm_init_node._commands["warm_init_begin"] = getattr(warm_init_node, "warm_init_begin")
        warm_init_node._commands["warm_init_end"] = getattr(warm_init_node, "warm_init_end")

def start_tdi_tofino(in_fd, out_fd, install_dir, dev_id_list, udf=None, interactive=False):
    global install_directory
    install_directory = install_dir
    tofino_cli = TofinoCli()
    return tofino_cli.start_tdi(in_fd, out_fd, dev_id_list, udf, interactive)

def main():
    start_tdi_tofino(0, 1, os.getcwd(), [0])

if __name__ == "__main__":
    main()
