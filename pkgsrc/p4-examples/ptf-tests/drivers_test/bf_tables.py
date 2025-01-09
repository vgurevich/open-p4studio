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


from randomdict import RandomDict

import binascii
import six
import time
import datetime
import sys
import bf_log
import math
import uuid
import pdb
import json
import copy

import unittest
import random

import pd_base_tests

from ptf import config
from ptf.testutils import *
from ptf.packet import *
from ptf.thriftutils import *

import os

from drivers_test.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
from plcmt_pd_rpc.ttypes import *

import bf_log

p4_name = 'drivers_test'

def pipe_to_pipe_id(pipe):
    if pipe == 'all':
        return hex_to_i16(0xFFFF)
    return hex_to_i16(pipe)

class Table(object):
    def __init__(self, test, resource_map, name):
        self.client = test.client
        self.plcmt = test.plcmt
        self.devport_mgr = test.devport_mgr
        self.ref_type = None
        if 'ref_type' in resource_map[name]:
            self.ref_type = resource_map[name]['ref_type']
        self.size = resource_map[name]['size']
        self.name = name
        self.resource_map = RandomDict()
        self.action_funcs = resource_map[name]['action_funcs']

    def refresh_thrift(self, test):
        self.client = test.client
        self.plcmt = test.plcmt
        self.devport_mgr = test.devport_mgr

    def default_init(self, sess_hdl, p_dev_id, v_dev_id):
        ''' Perform any table level initializations '''
        pass

    def default_teardown(self, sess_hdl, p_dev_id, v_dev_id):
        ''' Perform any teardown'''
        pass

    def get_ref_type(self):
        ''' Return the reference type '''
        return self.ref_type

    def get_size(self):
        ''' Return the size allocated '''
        return self.size

    def get_name(self):
        ''' Return the name(type) of the table '''
        return self.name

    def valid_action_funcs(self):
        ''' Return the valid action funcs this table participates in'''
        return self.action_funcs

    def append_resource_to_fn_map(self, resource_map, key, fn_map, count=1):
        for action_func in resource_map[key]['action_funcs']:
            fn_map['resources'][action_func].append((key, count))

    def get_resources_needed(self, action_func):
        ''' Return the resources needed to perform an ent_add or ent_modify.
            These will be passed as params to these APIs
        '''
        if action_func not in self.fn_map['resources']:
            return []
        return self.fn_map['resources'][action_func]

    def supports_modification(self):
        ''' Return True if the table supports modification of the entry '''
        return True

    def support_spec_change(self):
        ''' Return True if this table can change it's spec using modify API'''
        ''' Match spec cannot change. So return False '''
        return True

    def swap_states(self, r1, r2):
        ''' Swap any state maintained for the 2 resources '''
        pass

    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''
        pass

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis '''
        pass

    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        pass

    def ent_modify(self, rdata, action_func=None, target=None, params=None, with_hitless_ha=False):
        ''' Modify the entry into the table by calling thrift apis '''
        pass

    def update_exp_port(self, rdata, action_func):
        ''' Update the expected port. Return a tuple with expected port and pkt-count'''
        return (None, None)

    def verify_pkt(self, rdata, pkt, mdata, strict=True):
        ''' Verify that the pkt parameters conform to the expected transformations '''
        pass

    def enable_updates_after_hitless_ha(self, sess_hdl, p_dev_id, v_dev_id):
        ''' Register callbacks after hitless HA for virtual device '''
        pass

    def install_resource_in_llp(self, sess_hdl, rdata, p_dev_id):
        ''' Install the resource in LLP '''
        pass

class IdleTable(object):
    def __init__(self, bit_width, twoway, perflowdisable, min_ttl, max_ttl, query_ttl, clock_speed):
        self.bit_width = bit_width
        self.twoway = twoway
        self.perflowdisable = perflowdisable
        self.min_ttl = min_ttl
        self.max_ttl = max_ttl
        self.query_ttl = query_ttl
        self.clock_speed = clock_speed

        assert self.max_ttl >= max(self.query_ttl, self.min_ttl)
        self.calc_sweep_period()

    def gen_rand_ttl(self):
        ttl = random.choice([0]+ [random.randint(self.min_ttl, self.max_ttl)] * 3)
        return ttl

    def calc_sweep_period(self):
        notify_time = self.query_ttl

        no_states = (1 << self.bit_width)-1

        if self.twoway:
            no_states -= 1
        if self.perflowdisable:
            no_states -= 1

        sweep_count = notify_time * (self.clock_speed//1000)
        sweep_count //= no_states
        sweep_count >>= 21

        sweep_interval = min(15, int(math.ceil(math.log(sweep_count, 2))))
        self.hw_sweep_period = (1000 << (sweep_interval + 21))//self.clock_speed

        self.hw_notify_period = self.hw_sweep_period * no_states

        self.sw_sweep_period = notify_time

        if self.twoway:
            self.twowayerr = self.hw_sweep_period + self.sw_sweep_period
        else:
            self.twowayerr = 0

    def calc_err_margin(self, ttl):
        active_err = self.sw_sweep_period + self.twowayerr
        if self.sw_sweep_period:
            err = self.sw_sweep_period
            err += self.sw_sweep_period #- (abs(ttl-self.hw_notify_period) % self.sw_sweep_period)
        else:
            err = abs(ttl-self.hw_notify_period)
        err += active_err
        # Account for some more error due to the communication - 5000ms
        err += 5000
        return err

    def replace_adt_ent_hdl(self, drv_data, mbr_hdl):
        ''' Replace the action entry handle in the drv_data blob '''
        pass

class MatchTable(Table):
    def __init__(self, test, resource_map, valid_ports):
        match_tbl = resource_map['Match']
        size = match_tbl['size']
        match_type = match_tbl['match_type']
        tbl_name = match_tbl['name']

        super(MatchTable, self).__init__(test, resource_map, 'Match')

        self.parameter_map = RandomDict()
        self.parameter_map['Match'] = 'match_spec'

        self.has_idle_time = match_tbl['has_idle']
        self.has_selector = 'Selector' in resource_map

        self.init_fn_map(resource_map)

        self.match_type = match_type
        self.count_diff_priorities = 4
        self.default_entry_hdl = dict()

    def init_fn_map(self, resource_map):
        match_tbl = resource_map['Match']
        tbl_name = match_tbl['name']
        fn_map = RandomDict()
        fn_map['resources'] = RandomDict()
        fn_map['create_match_spec'] = globals()['_'.join([p4_name, tbl_name, 'match_spec_t'])]
        fn_map['add_match_entry'] = RandomDict()
        fn_map['add_match_entry_with_selector'] = RandomDict()
        fn_map['del_match_entry'] = getattr(self.client, tbl_name + '_table_delete')
        fn_map['add_default_entry'] = RandomDict()
        fn_map['add_default_entry_with_selector'] = RandomDict()
        fn_map['del_default_entry'] = getattr(self.client, tbl_name + '_table_reset_default_entry')
        fn_map['modify_match_entry'] = RandomDict()
        fn_map['modify_match_entry_with_selector'] = RandomDict()
        fn_map['set_property'] = getattr(self.client, tbl_name + '_set_property')
        fn_map['idle_tmo_enable'] = None
        fn_map['idle_tmo_disable'] = None
        fn_map['get_ttl'] = None
        if resource_map['Virtual_device']:
            fn_map['enable_updates'] = getattr(self.client, tbl_name + '_register_mat_update_cb')
            fn_map['get_updates'] = self.client.get_tbl_updates
            fn_map['program_updates'] = self.plcmt.process_plcmt_data
            fn_map['program_all_updates'] = self.client.program_all_updates
            fn_map['enable_callbacks_for_hitless_ha'] = self.client.enable_callbacks_for_hitless_ha
            fn_map['restore_virtual_device_state'] = self.client.restore_virtual_dev_state
            fn_map['get_entry_from_plcmt_data'] = getattr(self.client, tbl_name + '_get_entry_from_plcmt_data')
            fn_map['replace_adt_ent_hdl_in_drv_data'] = self.plcmt.replace_adt_ent_hdl
            fn_map['replace_sel_grp_hdl_in_drv_data'] = self.plcmt.replace_sel_grp_hdl
            fn_map['replace_ttl_in_drv_data'] = self.plcmt.replace_ttl

        for action_func in resource_map['Action_funcs']:
            fn_map['resources'][action_func] = []
            fn_map['add_match_entry'][action_func] = None
            fn_map['modify_match_entry'][action_func] = None
            fn_map['add_match_entry_with_selector'][action_func] = None
            fn_map['modify_match_entry_with_selector'][action_func] = None
            fn_map['add_default_entry'][action_func] = None
            fn_map['add_default_entry_with_selector'][action_func] = None

        if 'Action' in resource_map:
            self.append_resource_to_fn_map(resource_map, 'Action', fn_map)

            if resource_map['Action']['ref_type'] == 'DIRECT':
                self.parameter_map['Action'] = 'action_spec'
                for action_func in resource_map['Action']['action_funcs']:
                    fn_map['add_match_entry'][action_func] = getattr(self.client, tbl_name + '_table_add_with_' + action_func)
                    fn_map['add_default_entry'][action_func] = getattr(self.client, tbl_name + '_set_default_action_' + action_func)
                    fn_map['modify_match_entry'][action_func] = getattr(self.client,
                           '_'.join([tbl_name, 'table_modify_with', action_func]))
            else:
                self.parameter_map['Action'] = 'mbr'
                for action_func in resource_map['Action']['action_funcs']:
                    fn_map['add_match_entry'][action_func] = getattr(self.client, tbl_name + '_add_entry')
                    fn_map['add_default_entry'][action_func] = getattr(self.client, tbl_name + '_set_default_entry')
                    fn_map['modify_match_entry'][action_func] = getattr(self.client,
                           '_'.join([tbl_name, 'modify_entry']))


        if 'Selector' in resource_map:
            self.append_resource_to_fn_map(resource_map, 'Selector', fn_map)
            self.parameter_map['Selector'] = 'grp'
            for action_func in resource_map['Selector']['action_funcs']:
                fn_map['add_match_entry_with_selector'][action_func] = getattr(self.client, tbl_name + '_add_entry_with_selector')
                fn_map['add_default_entry_with_selector'][action_func] = getattr(self.client, tbl_name + '_set_default_entry_with_selector')
                fn_map['modify_match_entry_with_selector'][action_func] = getattr(self.client, tbl_name + '_modify_entry_with_selector')

        all_resources = []
        index_in_match_api = False
        if resource_map['Action']['ref_type'] == 'INDIRECT':
            index_in_match_api = True

        others = [('Stats', None), ('Meter', 'meter_spec'), ('Stateful', 'r_spec')]
        for s in others:
            key, dir_spec = s
            if key in resource_map:
                if resource_map[key]['ref_type'] == 'DIRECT':
                    self.append_resource_to_fn_map(resource_map, key, fn_map)
                    self.parameter_map[key] = dir_spec
                elif resource_map[key]['ref_type'] == 'INDIRECT' and index_in_match_api == True:
                    indir_spec = resource_map[key]['name'] + '_index'
                    self.append_resource_to_fn_map(resource_map, key, fn_map)
                    self.parameter_map[key] = indir_spec
                    all_resources.append((key, indir_spec))

        self.all_resources = all_resources
        self.fn_map = fn_map

        if self.has_idle_time:
            self.fn_map['idle_tmo_enable'] = getattr(self.client, \
                    tbl_name + '_idle_tmo_enable')
            self.fn_map['idle_tmo_disable'] = getattr(self.client, \
                    tbl_name + '_idle_tmo_disable')
            self.fn_map['get_ttl'] = getattr(self.client, \
                    tbl_name + '_get_ttl')
            self.idle_tmo_enabled = False

    def refresh_thrift(self, test, resource_map):
        super(MatchTable, self).refresh_thrift(test)
        self.init_fn_map(resource_map)
        bf_log.log("Updated thrift connection for %s" % self.get_name())

    def idle_timeout_enable(self, sess_hdl, p_dev_id, v_dev_id):
        idle_time_params_t = globals()[ \
                          '_'.join([p4_name, 'idle_time_params_t'])]
        notify_flag = globals()[\
                      '_'.join([p4_name, 'idle_time_mode'])].NOTIFY_MODE

        idle_tbl_params = idle_time_params_t(
                notify_flag,
                self.query_intvl,
                self.max_ttl,
                self.min_ttl,
                12
                )

        self.fn_map["idle_tmo_enable"](sess_hdl,
                p_dev_id,
                idle_tbl_params)
        if p_dev_id != v_dev_id:
            self.fn_map["idle_tmo_enable"](sess_hdl,
                    v_dev_id,
                    idle_tbl_params)
        self.idle_tmo_enabled = True

    def get_resources_needed(self, action_func):
        needed = super(MatchTable, self).get_resources_needed(action_func)[:]
        # Use selector in some entries and action in some others
        if self.has_selector and len(needed) > 0:
            remove_item = random.choice(['Action', 'Selector'])
            for item, count in needed:
                if item == remove_item:
                    needed.remove((remove_item, count))
        return needed

    def default_init(self, sess_hdl, p_dev_id, v_dev_id):
        ''' Perform any table level initializations '''
        if self.has_idle_time:
            idle_time_params_t = globals()[ \
                              '_'.join([p4_name, 'idle_time_params_t'])]
            notify_flag = globals()[\
                          '_'.join([p4_name, 'idle_time_mode'])].NOTIFY_MODE

            self.query_intvl = 5000
            self.max_ttl = 250000
            self.min_ttl = 5000
            if test_param_get('arch') == "tofino":
                if test_param_get('target') == "asic-model":
                    self.clock_speed = 1271000000
                else:
                    self.clock_speed = self.devport_mgr.devport_mgr_get_clock_speed(p_dev_id).bps_clock_speed
            else:
                self.clock_speed = 1000000000
            self.idle = IdleTable(3, True, True, self.min_ttl, self.max_ttl, self.query_intvl, self.clock_speed)
            idle_tbl_params = idle_time_params_t(
                    notify_flag,
                    self.query_intvl,
                    self.max_ttl,
                    self.min_ttl,
                    12
                    )
            if self.idle_tmo_enabled == False:
                self.fn_map["idle_tmo_enable"](sess_hdl,
                        p_dev_id,
                        idle_tbl_params)
                if p_dev_id != v_dev_id:
                    self.fn_map["idle_tmo_enable"](sess_hdl,
                            v_dev_id,
                            idle_tbl_params)
                self.idle_tmo_enabled = True

        if p_dev_id != v_dev_id:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def default_teardown(self, sess_hdl, p_dev_id, v_dev_id):
        ''' Perform any teardown'''
        if self.has_idle_time:
            self.min_ttl = None
            self.max_ttl = None
            self.query_intvl = None
            self.idle = None

            if self.idle_tmo_enabled == True:
                self.fn_map["idle_tmo_disable"](sess_hdl,
                        p_dev_id)
                if p_dev_id != v_dev_id:
                    self.fn_map["idle_tmo_disable"](sess_hdl,
                            v_dev_id)
                self.idle_tmo_enabled = False

    def support_spec_change(self):
        ''' Return True if this table can change it's spec using modify API'''
        ''' Match spec cannot change. So return False '''
        return False

    def get_match_param(self, rdata, action_func):
        x = rdata['uniq_val']

        match_params = dict()

        if self.match_type == 'exm':
            ip_dst = x & 0xffffffff
            match_params['ipv4_dstAddr'] = hex_to_i32(ip_dst)
            match_params['ipv4_srcAddr'] = ipv4Addr_to_i32('10.1.1.1')
            match_params['ipv4_protocol'] = 6
            match_params['ethernet_dstAddr'] = macAddr_to_string('AA:BB:CC:DD:EE:FF')
        else:
            if self.match_type == 'atcam' or self.match_type == 'alpm':
                match_params['vlan_tag_vlan_id'] = x & 0xfff
                x = x >> 12

            # Remove log(count_diff_priorities) bits

            bits_to_remove = int(math.log(self.count_diff_priorities, 2))
            bits_to_add = self.count_diff_priorities - 1
            ip_dst = ((x>>bits_to_remove)<<bits_to_add) & 0xffffffff
            match_params['ipv4_dstAddr'] = hex_to_i32(ip_dst)

            y = x & ((1<<bits_to_remove)-1)

            if self.match_type == 'alpm':
                match_params['ipv4_dstAddr_prefix_length'] = 32 - y
            else:
                mask = (1<<bits_to_add)-1
                mask &= mask<<y

                # Based on y, figure out the dest mask
                ip_dst_mask = (0xffffffff<<bits_to_add) | mask
                ip_dst_mask &= 0xffffffff

                match_params['ipv4_dstAddr_mask'] = hex_to_i32(ip_dst_mask)
        return match_params


    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''
        match_params = self.get_match_param(rdata, action_func)
        match_spec = self.fn_map['create_match_spec'](**match_params)
        return match_spec

    def id_to_ent_hdl(self, rdata):
        ''' Return the entry-hdl for a given id'''
        return rdata['entry_hdl']

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis '''
        dev_tgt = DevTarget_t(target['v_dev_id'], pipe_to_pipe_id(target['pipe']))
        fn_params = {
                     'sess_hdl': target['sess_hdl'],
                     'dev_tgt': dev_tgt,
                     'match_spec' : self.get_spec(rdata, action_func)
                    }
        if self.has_idle_time:
            ttl = self.idle.gen_rand_ttl()
            fn_params['ttl'] = ttl
            rdata['ttl'] = ttl

        for ptype, spec in self.all_resources:
            if spec is not None:
                fn_params[spec] = 0

        for ptype, spec in params.items():
            if self.parameter_map[ptype] is not None:
                fn_params[self.parameter_map[ptype]] = spec

        x = rdata['uniq_val']
        if self.match_type == 'atcam':
            x = x >> 12
        if self.match_type == 'tcam' or self.match_type == 'atcam':
            bits_to_remove = int(math.log(self.count_diff_priorities, 2))
            y = x & ((1<<bits_to_remove)-1)
            fn_params['priority'] = y

        make_default = False
        if replay:
            if 'is_default_entry' in pre_ha_rdata:
                make_default = True
        elif target['pipe'] not in self.default_entry_hdl:
            make_default = random.choice([False,True])

        if 'priority' in fn_params:
            if fn_params['priority'] != self.count_diff_priorities-1:
                make_default = False

        if self.match_type == 'alpm':
            match_params = self.get_match_param(rdata, action_func)
            if match_params['ipv4_dstAddr_prefix_length'] != (32 - (self.count_diff_priorities - 1)):
                make_default = False

        if with_hitless_ha and target['v_dev_id'] != target['p_dev_id']:
            # Do not allocate default entry for multi device hitless ha
            make_default = False

        if make_default:
            fn_params.pop('match_spec')
            fn_params.pop('ttl', None)
            fn_params.pop('priority', None)

        # Based on the action func, call the right api
        bf_log.debug_fn(fn_params)

        if make_default:
            bf_log.log('ADDING DEFAULT ENTRY')
            if 'Selector' in params.keys():
                entry_hdl = self.fn_map['add_default_entry_with_selector'][action_func](**fn_params)
            else:
                entry_hdl = self.fn_map['add_default_entry'][action_func](**fn_params)
            rdata['is_default_entry'] = True
            self.default_entry_hdl[target['pipe']] = entry_hdl
        else:
            if 'Selector' in params.keys():
                entry_hdl = self.fn_map['add_match_entry_with_selector'][action_func](**fn_params)
            else:
                entry_hdl = self.fn_map['add_match_entry'][action_func](**fn_params)
        drv_data = ""
        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            if with_hitless_ha:
                # Cache drv data etc only if we are doing hitless HA test
                for each_update in x:
                    update_type = each_update.update_type
                    if update_type == drivers_test_tbl_update_type.MAT_UPDATE_TYPE:
                        mat_update = each_update.update_data.mat
                        if mat_update.update_type == drivers_test_mat_update_type.MAT_UPDATE_ADD:
                            entry_handle = mat_update.update_params.add.entry_hdl
                            if entry_handle == entry_hdl:
                                drv_data = mat_update.update_params.add.drv_data
                        elif mat_update.update_type == drivers_test_mat_update_type.MAT_UPDATE_SET_DFLT:
                            rdata['default_entry_add'] = x
                            drv_data = mat_update.update_params.set_dflt.drv_data
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

        assert entry_hdl != 0, 'Match entry add failed'
        rdata['entry_hdl'] = entry_hdl
        rdata['drv_data'] = drv_data

    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        entry_hdl = rdata['entry_hdl']

        if 'is_default_entry' in rdata:
            bf_log.log('DELETING DEFAULT ENTRY')
            dev_tgt = DevTarget_t(target['v_dev_id'], pipe_to_pipe_id(target['pipe']))
            self.fn_map['del_default_entry'](target['sess_hdl'], dev_tgt)
            del self.default_entry_hdl[target['pipe']]
            del rdata['is_default_entry']
        else:
            self.fn_map['del_match_entry'](target['sess_hdl'], target['v_dev_id'], entry_hdl)
        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def ent_modify(self, rdata, action_func=None, target=None, params=None, with_hitless_ha=False):
        ''' Modify the entry into the table by calling thrift apis '''
        entry_hdl = rdata['entry_hdl']
        fn_params = {
                     'sess_hdl': target['sess_hdl'],
                     'dev_id': target['v_dev_id'],
                     'entry' : entry_hdl
                    }

        for ptype, spec in self.all_resources:
            if spec is not None:
                fn_params[spec] = 0

        for ptype, spec in params.items():
            if self.parameter_map[ptype] is not None:
                fn_params[self.parameter_map[ptype]] = spec

        # Based on the action func, call the right api
        bf_log.debug_fn(fn_params)
        if 'Selector' in params.keys():
            self.fn_map['modify_match_entry_with_selector'][action_func](**fn_params)
        else:
            self.fn_map['modify_match_entry'][action_func](**fn_params)

        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            if with_hitless_ha:
                for each_update in x:
                    update_type = each_update.update_type
                    if update_type == drivers_test_tbl_update_type.MAT_UPDATE_TYPE:
                        mat_update = each_update.update_data.mat
                        assert mat_update.update_type == drivers_test_mat_update_type.MAT_UPDATE_MOD
                        drv_data = mat_update.update_params.mod.drv_data
                        rdata['drv_data'] = drv_data
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def update_pkt_params(self, rdata, mdata, pkt_params):
        action_func = mdata['action_func']
        match_params = self.get_match_param(rdata, action_func)

        if self.match_type == 'exm':
            pkt_params['ip_dst'] = i32_to_ipv4Addr(match_params['ipv4_dstAddr'])
            pkt_params['ip_src'] = i32_to_ipv4Addr(match_params['ipv4_srcAddr'])
            pkt_params['eth_dst'] = match_params['ethernet_dstAddr']
        else:
            x = rdata['uniq_val']
            if self.match_type == 'atcam' or self.match_type == 'alpm':
                pkt_params['dl_vlan_enable'] = True
                pkt_params['vlan_vid'] = x & 0xfff
                x = x >> 12

            bits_to_remove = int(math.log(self.count_diff_priorities, 2))
            bits_to_add = self.count_diff_priorities - 1

            y = x & ((1<<bits_to_remove)-1)
            mask = 0
            if y:
                mask = 1<<(y-1)

            ip_dst = match_params['ipv4_dstAddr']
            ip_dst |= mask

            pkt_params['ip_dst'] = i32_to_ipv4Addr(ip_dst)

    def set_symmetric(self, sess_hdl, dev_id, val):
        '''Set/reset the symmetric mode'''

        prop = tbl_property_t.TBL_PROP_TBL_ENTRY_SCOPE
        prop_val = tbl_property_value_t.ENTRY_SCOPE_ALL_PIPELINES
        if val == 0:
            prop_val = tbl_property_value_t.ENTRY_SCOPE_SINGLE_PIPELINE
        self.fn_map['set_property'](sess_hdl, dev_id, prop, prop_val, 0)

    def update_idle_time_param(self, rdata):
        if self.has_idle_time:
            time_active = datetime.datetime.now()
            rdata['active_time'] = time_active

    def verify_idletime(self, rdata, mdata):
        if self.has_idle_time:
            # Figure out the expected state based on init-ttl,
            # entry added/active time
            sess_hdl = mdata['sess_hdl']
            p_dev_id = mdata['p_dev_id']
            ent_hdl = self.id_to_ent_hdl(rdata)
            now = datetime.datetime.now()
            cur_ttl = self.fn_map['get_ttl'](sess_hdl, p_dev_id, ent_hdl)

            init_ttl = rdata['ttl']
            if init_ttl == 0:
                assert cur_ttl == 0
                return

            assert cur_ttl <= init_ttl

            active_time = rdata['active_time']
            diff = now - active_time

            err_margin = self.idle.calc_err_margin(init_ttl)
            max_entry_alive_time = datetime.timedelta(milliseconds=(init_ttl+err_margin))
            if test_param_get('target') == "asic-model":
                max_entry_alive_time *= 2

            if diff >= max_entry_alive_time:
                if cur_ttl != 0:
                    bf_log.log('Idle verify failed!!! Entry 0x%x Active %s Current %s Diff %s ErrMargin %d MaxAliveTime %s CurTTL %d' % (ent_hdl, active_time, now, diff, err_margin, max_entry_alive_time, cur_ttl))
                    assert cur_ttl == 0, 'Current ttl %d is not zero' % (cur_ttl)

    def update_entry_add_time(self, rdata, time_added):
        if self.has_idle_time:
            rdata['active_time'] = time_added

    def register_cbs_for_hitless_ha(self, sess_hdl, phy_device_id):
        self.fn_map['enable_callbacks_for_hitless_ha'](sess_hdl, phy_device_id)
        self.fn_map['enable_updates'](sess_hdl, phy_device_id)

    def get_updates_after_hitless_ha(self, sess_hdl, phy_device_id):
        x = self.fn_map['get_updates'](phy_device_id)
        return x

    def restore_virtual_device_state(self, sess_hdl, virtual_device_id, state):
        self.fn_map['restore_virtual_device_state'](sess_hdl, virtual_device_id, state)

    def get_resource_id_from_mspec(self, mspec):
        if self.match_type == 'exm':
            return (('Match', i32_to_hex(mspec.ipv4_dstAddr)))
        else:
            uniq_val = int(i32_to_hex(mspec.ipv4_dstAddr)//2)
            if self.match_type == 'atcam':
                uniq_val = uniq_val << 12
                uniq_val |= mspec.vlan_tag_vlan_id
            mask = i32_to_hex(mspec.ipv4_dstAddr_mask)
            bits_to_add = self.count_diff_priorities - 1
            mask &= ((1 << bits_to_add) - 1)
            cnt = bits_to_add
            while mask:
                mask &= (mask-1)
                cnt = cnt - 1
            return (('Match', uniq_val + cnt))


    def get_mspec_from_plcmt_data(self, dev_id, drv_data):
        x = self.fn_map['get_entry_from_plcmt_data'](drv_data)
        return x.match_spec

    def enable_updates_after_hitless_ha(self, sess_hdl, p_dev_id, v_dev_id):
        if p_dev_id != v_dev_id and self.fn_map['enable_updates']:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def replace_adt_ent_hdl(self, drv_data, mbr_hdl):
        new_drv_data = self.fn_map['replace_adt_ent_hdl_in_drv_data'](drv_data, mbr_hdl)
        return new_drv_data

    def replace_sel_grp_hdl(self, drv_data, grp_hdl):
        new_drv_data = self.fn_map['replace_sel_grp_hdl_in_drv_data'](drv_data, grp_hdl)
        return new_drv_data

    def replace_ttl(self, drv_data, ttl):
        new_drv_data = self.fn_map['replace_ttl_in_drv_data'](drv_data, ttl)
        return new_drv_data

    def install_resource_in_llp(self, sess_hdl, rdata, p_dev_id):
        assert 'is_default_entry' in rdata
        self.fn_map['program_updates'](sess_hdl, p_dev_id, rdata['default_entry_add'])

    def replay_move_list_to_llp(self, sess_hdl, p_dev_id, mat_move_list):
        self.fn_map['program_updates'](sess_hdl, p_dev_id, mat_move_list)

class ActionTable(Table):
    def __init__(self, test, resource_map, valid_ports):
        action_tbl = resource_map['Action']
        size = action_tbl['size']
        ref_type = action_tbl['ref_type']

        super(ActionTable, self).__init__(test, resource_map, 'Action')

        self.init_fn_map(resource_map)
        self.ref_type = ref_type
        self.valid_ports = valid_ports

    def init_fn_map(self, resource_map):
        action_tbl = resource_map['Action']
        ref_type = action_tbl['ref_type']
        action_profile = action_tbl['name']

        fn_map = RandomDict()
        fn_map['resources'] = RandomDict()
        fn_map['add_action_entry'] = RandomDict()
        fn_map['create_action_spec'] = RandomDict()
        fn_map['add_action_entry'] = RandomDict()
        fn_map['modify_action_entry'] = RandomDict()
        fn_map['del_action_entry'] = None
        fn_map['enable_updates'] = None
        fn_map['get_updates'] = None
        if ref_type == 'INDIRECT':
            fn_map['del_action_entry'] = getattr(self.client, action_profile + '_del_member')
            fn_map['enable_updates'] = getattr(self.client, action_profile + '_register_adt_update_cb')
            fn_map['get_updates'] = self.client.get_tbl_updates
            fn_map['program_updates'] = self.plcmt.process_plcmt_data
            if resource_map['Virtual_device']:
                fn_map['program_all_updates'] = self.client.program_all_updates
            fn_map['get_info_from_plcmt_data'] = getattr(self.client, action_profile + '_get_full_member_info_from_plcmt_data')


        for action_func in action_tbl['action_funcs']:
            fn_map['resources'][action_func] = []
            fn_map['add_action_entry'][action_func] = None
            fn_map['modify_action_entry'][action_func] = None
            fn_map['create_action_spec'][action_func] = globals()[
                    '_'.join([p4_name, action_func, 'action_spec_t'])]
            if ref_type == 'INDIRECT':
                fn_map['add_action_entry'][action_func] = getattr(self.client,
                       '_'.join([action_profile, 'add_member_with', action_func]))
                fn_map['modify_action_entry'][action_func] = getattr(self.client,
                       '_'.join([action_profile, 'modify_member_with', action_func]))

        index_in_match_api = False
        if resource_map['Action']['ref_type'] == 'INDIRECT':
            index_in_match_api = True

        extra_params = dict()
        for action_func in resource_map['Action_funcs']:
            extra_params[action_func] = dict()

        parameter_map = RandomDict()
        others = [('Stats', 'action_stat_idx'), ('Meter', 'action_meter_idx'), ('Stateful', 'action_stful_idx')]
        for s in others:
            key, spec = s
            if key in resource_map:
                if resource_map[key]['ref_type'] == 'INDIRECT':
                    if index_in_match_api == False:
                        self.append_resource_to_fn_map(resource_map, key, fn_map)
                        parameter_map[key] = spec
                    else:
                        for action_func in resource_map[key]['action_funcs']:
                            extra_params[action_func][spec] = 0

        self.extra_params = extra_params
        self.parameter_map = parameter_map
        self.fn_map = fn_map

    def refresh_thrift(self, test, resource_map):
        super(ActionTable, self).refresh_thrift(test)
        self.init_fn_map(resource_map)
        bf_log.log("Updated thrift connection for %s" % self.get_name())

    def default_init(self, sess_hdl, p_dev_id, v_dev_id):
        if p_dev_id != v_dev_id and self.fn_map['enable_updates'] is not None:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def supports_modification(self):
        ''' Return True if the table supports modification of the entry '''
        if self.ref_type == 'DIRECT':
            return False
        return True

    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''

        if self.ref_type == 'INDIRECT':
            return rdata['mbr_hdl']

        return rdata['action_spec']

    def get_action_param(self, rdata, action_func):
        x = rdata['uniq_val']
        if action_func not in self.fn_map['create_action_spec']:
            # We do not participate in this action
            return None

        eg_port = self.valid_ports[x % len(self.valid_ports)]
        x = x//len(self.valid_ports)

        action_param = dict()
        action_param['action_port'] = eg_port

        if action_func == 'tcp_sport_modify':
            action_param['action_sPort'] = hex_to_i16(x & 0xffff)
        elif action_func == 'tcp_dport_modify':
            action_param['action_dPort'] = hex_to_i16(x & 0xffff)
        elif action_func == 'ipsa_modify':
            action_param['action_ipsa'] = hex_to_i32(x & 0xffffffff)
        elif action_func == 'ipda_modify':
            action_param['action_ipda'] = hex_to_i32(x & 0xffffffff)
        elif action_func == 'ipds_modify':
            action_param['action_ds'] = hex_to_byte(x & 0xff)
        elif action_func == 'ipttl_modify':
            action_param['action_ttl'] = hex_to_byte(x & 0xff)
        else:
            assert 0

        action_param.update(self.extra_params[action_func])
        return action_param

    def get_action_spec(self, rdata, action_func):
        action_param = self.get_action_param(rdata, action_func)
        return self.fn_map['create_action_spec'][action_func](**action_param)

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis '''
         # If it is a direct action, then just create the spec
         # In case of indirect action, add the entry by calling thrift

        action_spec = self.get_action_spec(rdata, action_func)

        for ptype, spec in params.items():
            setattr(action_spec, self.parameter_map[ptype], spec)
        rdata['action_spec'] = action_spec
        rdata['action_name'] = action_func

        if self.fn_map['add_action_entry'][action_func] is not None:
            dev_tgt = DevTarget_t(target['v_dev_id'], pipe_to_pipe_id(target['pipe']))
            fn_params = {
                         'sess_hdl' : target['sess_hdl'],
                         'dev_tgt' : dev_tgt,
                         'action_spec' : action_spec
                        }

            mbr_hdl = self.fn_map['add_action_entry'][action_func](**fn_params)
            assert mbr_hdl != 0, 'Action entry add failed'
            rdata['mbr_hdl'] = mbr_hdl
            if target['v_dev_id'] != target['p_dev_id'] and self.fn_map['get_updates'] is not None:
                x = self.fn_map['get_updates'](target['v_dev_id'])
                if with_hitless_ha:
                    # Cache the move list only if we are doing hitless HA test
                    rdata['adt_move_list'] = x
                self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def get_updates_after_hitless_ha(self, phy_device_id):
        x = []
        if self.ref_type == 'INDIRECT':
            x = self.fn_map['get_updates'](phy_device_id)
        return x

    def get_info_from_plcmt_data(self, device_id, mbr_hdl, drv_data):
        x = self.fn_map['get_info_from_plcmt_data'](device_id, mbr_hdl, drv_data)
        return ((x.name, x.data))

    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        if self.fn_map['del_action_entry'] is not None:
            mbr_hdl = rdata['mbr_hdl']
            self.fn_map['del_action_entry'](target['sess_hdl'], target['v_dev_id'], mbr_hdl)
            if target['v_dev_id'] != target['p_dev_id'] and self.fn_map['get_updates'] is not None:
                x = self.fn_map['get_updates'](target['v_dev_id'])
                self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def ent_modify(self, rdata, action_func=None, target=None, params=None, with_hitless_ha=False):
        ''' Modify the entry into the table by calling thrift apis '''
        action_spec = self.get_action_spec(rdata, action_func)

        for ptype, spec in params.items():
            setattr(action_spec, self.parameter_map[ptype], spec)
        rdata['action_spec'] = action_spec

        if self.fn_map['modify_action_entry'][action_func] is not None:
            mbr_hdl = rdata['mbr_hdl']
            fn_params = {
                         'sess_hdl' : target['sess_hdl'],
                         'dev_id' : target['v_dev_id'],
                         'mbr' : mbr_hdl,
                         'action_spec' : action_spec
                        }

            self.fn_map['modify_action_entry'][action_func](**fn_params)
            if target['v_dev_id'] != target['p_dev_id'] and self.fn_map['get_updates'] is not None:
                x = self.fn_map['get_updates'](target['v_dev_id'])
                if with_hitless_ha:
                    rdata['adt_move_list'] = copy.deepcopy(x)
                    rdata['adt_move_list'][0].update_data.adt.update_type = drivers_test_adt_update_type.ADT_UPDATE_ADD
                self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def verify_pkt(self, rdata, pkt, mdata, strict=True):
        ''' Verify that the pkt parameters conform to the expected transformations '''
        action_func = mdata['action_func']
        action_param = self.get_action_param(rdata, action_func)

        rcv_pkt = six.ensure_str(binascii.hexlify(pkt))
        rcv_pkt = [rcv_pkt[i:i + 2] for i in range(0, len(rcv_pkt), 2)]

        bf_log.debug_fn('Action verify ' + action_func + ' '
                        + str(action_param))

        # INFO(sborkows): if packet has a tag, its ether type will be 0x8100
        # and fields from IP and TCP will be shifted by 4B
        offset = 4 if ''.join(rcv_pkt[12:14]) == "8100" else 0
        rcv_tcp_sport = int(''.join(rcv_pkt[34 + offset:36 + offset]), 16)
        rcv_tcp_dport = int(''.join(rcv_pkt[36 + offset:38 + offset]), 16)
        rcv_ipsa = '.'.join([str(int(x, 16))
                             for x in rcv_pkt[26 + offset:30 + offset]])
        rcv_ipda = '.'.join([str(int(x, 16))
                             for x in rcv_pkt[30 + offset:34 + offset]])
        rcv_ipds = int(rcv_pkt[15 + offset], 16)
        rcv_ipttl = int(rcv_pkt[22 + offset], 16)

        if action_func == 'tcp_sport_modify':
            tcp_sport = action_param['action_sPort']
            tcp_sport = i16_to_hex(tcp_sport)
            if strict and rcv_tcp_sport != tcp_sport:
                bf_log.log('Pkt tcp sport %d is not %d' % (rcv_tcp_sport, tcp_sport))
            assert rcv_tcp_sport == tcp_sport, 'Pkt tcp sport %d is not %d' % (rcv_tcp_sport, tcp_sport)
        elif action_func == 'tcp_dport_modify':
            tcp_dport = action_param['action_dPort']
            tcp_dport = i16_to_hex(tcp_dport)
            if strict and rcv_tcp_dport != tcp_dport:
                bf_log.log('Pkt tcp dport %d is not %d' % (rcv_tcp_dport, tcp_dport))
            assert rcv_tcp_dport == tcp_dport, 'Pkt tcp dport %d is not %d' % (rcv_tcp_dport, tcp_dport)
        elif action_func == 'ipsa_modify':
            ipsa = action_param['action_ipsa']
            ipsa = i32_to_ipv4Addr(ipsa)
            if strict and rcv_ipsa != ipsa:
                bf_log.log('Pkt ipsa %s is not %s' % (rcv_ipsa, ipsa))
            assert rcv_ipsa == ipsa, 'Pkt ipsa %s is not %s' % (rcv_ipsa, ipsa)
        elif action_func == 'ipda_modify':
            ipda = action_param['action_ipda']
            ipda = i32_to_ipv4Addr(ipda)
            if strict and rcv_ipda != ipda:
                bf_log.log('Pkt ipda %s is not %s' % (rcv_ipda, ipda))
            assert rcv_ipda == ipda, 'Pkt ipda %s is not %s' % (rcv_ipda, ipda)
        elif action_func == 'ipds_modify':
            ipds = action_param['action_ds']
            ipds = byte_to_hex(ipds)
            if strict and rcv_ipds != ipds:
                bf_log.log('Pkt iptos %s is not %s' % (rcv_ipds, ipds))
            assert rcv_ipds == ipds, 'Pkt iptos %s is not %s' % (rcv_ipds, ipds)
        elif action_func == 'ipttl_modify':
            ipttl = action_param['action_ttl']
            ipttl = byte_to_hex(ipttl)
            if strict and rcv_ipttl != ipttl:
                bf_log.log('Pkt ipttl %s is not %s' % (rcv_ipttl, ipttl))
            assert rcv_ipttl == ipttl, 'Pkt ipttl %s is not %s' % (rcv_ipttl, ipttl)
        else:
            assert 0


    def update_exp_port(self, rdata, action_func):
        action_param = self.get_action_param(rdata, action_func)
        return (action_param['action_port'], 1)

    def register_cbs_for_hitless_ha(self, sess_hdl, phy_device_id):
        if self.fn_map['enable_updates']:
            self.fn_map['enable_updates'](sess_hdl, phy_device_id)

    def get_resource_id_from_aspec_tcp_sport_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "tcp_sport_modify":
                if rdata['action_spec'].action_sPort == action_spec.action_sPort and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec_tcp_dport_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "tcp_dport_modify":
                if rdata['action_spec'].action_dPort == action_spec.action_dPort and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec_ipsa_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "ipsa_modify":
                if rdata['action_spec'].action_ipsa == action_spec.action_ipsa and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec_ipda_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "ipda_modify":
                if rdata['action_spec'].action_ipda == action_spec.action_ipda and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec_ipds_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "ipds_modify":
                if rdata['action_spec'].action_ds == action_spec.action_ds and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec_ipttl_modify(self, action_spec, installed_rids, get_rdata_fn):
        for resource_id in installed_rids:
            rdata = get_rdata_fn(resource_id)
            if rdata['action_name'] == "ipttl_modify":
                if rdata['action_spec'].action_ttl == action_spec.action_ttl and rdata['action_spec'].action_port == action_spec.action_port:
                    return resource_id
        assert 0

    def get_resource_id_from_aspec(self, action_name, action_spec, installed_rids, get_rdata_fn):
        if action_name == "tcp_sport_modify":
            action = action_spec.drivers_test_tcp_sport_modify
            uniq_val = action.action_sPort
            return self.get_resource_id_from_aspec_tcp_sport_modify(action, installed_rids, get_rdata_fn)
        elif action_name == "tcp_dport_modify":
            action = action_spec.drivers_test_tcp_dport_modify
            return self.get_resource_id_from_aspec_tcp_dport_modify(action, installed_rids, get_rdata_fn)
        elif action_name == "ipsa_modify":
            action = action_spec.drivers_test_ipsa_modify
            return self.get_resource_id_from_aspec_ipsa_modify(action, installed_rids, get_rdata_fn)
        elif action_name == "ipda_modify":
            action = action_spec.drivers_test_ipda_modify
            return self.get_resource_id_from_aspec_ipda_modify(action, installed_rids, get_rdata_fn)
            uniq_val = action.action_ipda
        elif action_name == "ipds_modify":
            action = action_spec.drivers_test_ipds_modify
            return self.get_resource_id_from_aspec_ipds_modify(action, installed_rids, get_rdata_fn)
        elif action_name == "ipttl_modify":
            action = action_spec.drivers_test_ipttl_modify
            uniq_val = action.action_ttl
            return self.get_resource_id_from_aspec_ipttl_modify(action, installed_rids, get_rdata_fn)
        else:
            assert 0

    def enable_updates_after_hitless_ha(self, sess_hdl, p_dev_id, v_dev_id):
        if p_dev_id != v_dev_id and self.fn_map['enable_updates']:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def install_resource_in_llp(self, sess_hdl, rdata, p_dev_id):
        self.fn_map['program_updates'](sess_hdl, p_dev_id, rdata['adt_move_list'])

    def replay_move_list_to_llp(self, sess_hdl, p_dev_id, adt_move_list):
        self.fn_map['program_updates'](sess_hdl, p_dev_id, adt_move_list)

class SelectorTable(Table):
    def __init__(self, test, resource_map, valid_ports):
        selector_tbl = resource_map['Selector']
        ref_type = selector_tbl['ref_type']
        assert ref_type == 'INDIRECT'

        super(SelectorTable, self).__init__(test, resource_map, 'Selector')
        self.init_fn_map(resource_map)

    def init_fn_map(self, resource_map):
        selector_tbl = resource_map['Selector']
        action_profile = selector_tbl['name']
        fn_map = RandomDict()
        fn_map['resources'] = RandomDict()
        fn_map['create_group'] = getattr(self.client, action_profile + '_create_group')
        fn_map['delete_group'] = getattr(self.client, action_profile + '_del_group')
        fn_map['add_member_to_group'] = getattr(self.client, action_profile + '_add_member_to_group')
        fn_map['del_member_from_group'] = getattr(self.client, action_profile + '_del_member_from_group')
        fn_map['group_member_state_set'] = getattr(self.client, action_profile + '_group_member_state_set')
        fn_map['enable_updates'] = None
        fn_map['get_updates'] = None
        fn_map['program_updates'] = None
        if resource_map['Virtual_device']:
            fn_map['enable_updates'] = getattr(self.client, action_profile + '_register_sel_update_cb')
            fn_map['get_updates'] = self.client.get_tbl_updates
            fn_map['program_updates'] = self.plcmt.process_plcmt_data
            fn_map['program_all_updates'] = self.client.program_all_updates

        for action_func in selector_tbl['action_funcs']:
            fn_map['resources'][action_func] = []

        self.parameter_map = RandomDict()
        others = [('Action', 'mbr')]
        for s in others:
            key, spec = s
            if key in resource_map:
                if resource_map[key]['ref_type'] == 'INDIRECT':
                    self.append_resource_to_fn_map(resource_map, key, fn_map)
                    self.parameter_map[key] = spec
        self.fn_map = fn_map

    def refresh_thrift(self, test, resource_map):
        super(SelectorTable, self).refresh_thrift(test)
        self.init_fn_map(resource_map)
        bf_log.log("Updated thrift connection for %s" % self.get_name())

    def default_init(self, sess_hdl, p_dev_id, v_dev_id):
        if p_dev_id != v_dev_id:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def get_resources_needed(self, action_func):
        needed = super(SelectorTable, self).get_resources_needed(action_func)[:]
        if len(needed) > 0:
            needed = [('Action', random.randint(2,20))]
        return needed

    def supports_modification(self):
        ''' Return True if the table supports modification of the entry '''
        return False

    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''
        return rdata['grp_hdl']

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis '''

        action_mbrs = params['Action']
        group_size = len(action_mbrs)

        dev_tgt = DevTarget_t(target['v_dev_id'], pipe_to_pipe_id(target['pipe']))
        rdata['grp_hdl'] = self.fn_map['create_group'](target['sess_hdl'], dev_tgt, group_size)
        rdata['group_size'] = group_size
        rdata['mbrs'] = action_mbrs

        # Add each of the members
        for mbr in action_mbrs:
            self.fn_map['add_member_to_group'](target['sess_hdl'], target['v_dev_id'], rdata['grp_hdl'], mbr)
        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)
            if with_hitless_ha:
                # Cache the selector group update only if we are doing hitless HA
                rdata['sel_grp_update'] = x

    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        for mbr in rdata['mbrs']:
            self.fn_map['del_member_from_group'](target['sess_hdl'], target['v_dev_id'], rdata['grp_hdl'], mbr)
        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)
        self.fn_map['delete_group'](target['sess_hdl'], target['v_dev_id'], rdata['grp_hdl'])
        if target['v_dev_id'] != target['p_dev_id']:
            x = self.fn_map['get_updates'](target['v_dev_id'])
            self.fn_map['program_updates'](target['sess_hdl'], target['p_dev_id'], x)

    def ent_modify(self, rdata, action_func=None, target=None, params=None, with_hitless_ha=False):
        ''' Modify the entry into the table by calling thrift apis '''
        assert 0

    def verify_pkt(self, rdata, pkt, mdata, strict=True):
        ''' Verify that the pkt parameters conform to the expected transformations '''
        pass

    def update_exp_port(self, rdata, action_func):
        return (None, 1)
#        return (None, min(rdata['group_size'], 5))

    def enable_updates_after_hitless_ha(self, sess_hdl, p_dev_id, v_dev_id):
        if p_dev_id != v_dev_id and self.fn_map['enable_updates']:
            self.fn_map['enable_updates'](sess_hdl, v_dev_id)

    def install_resource_in_llp(self, sess_hdl, rdata, p_dev_id):
        self.fn_map['program_updates'](sess_hdl, p_dev_id, rdata['sel_grp_update'])

    def register_cbs_for_hitless_ha(self, sess_hdl, phy_device_id):
        self.fn_map['enable_updates'](sess_hdl, phy_device_id)

    def replay_move_list_to_llp(self, sess_hdl, p_dev_id, sel_move_list):
        self.fn_map['program_updates'](sess_hdl, p_dev_id, sel_move_list)

class StatsTable(Table):
    def __init__(self, test, resource_map, valid_ports):
        stats_tbl = resource_map['Stats']
        ref_type = stats_tbl['ref_type']

        super(StatsTable, self).__init__(test, resource_map, 'Stats')

        self.init_fn_map(resource_map)

        flags_class = globals()['_'.join([p4_name, 'counter_flags_t'])]

        self.hw_sync_flag = flags_class(1)
        self.ref_type = ref_type
        self.action_funcs = stats_tbl['action_funcs']

    def init_fn_map(self, resource_map):
        stats_tbl = resource_map['Stats']
        fn_map = RandomDict()
        fn_map['resources'] = RandomDict()
        fn_map['read_stats'] = getattr(self.client, 'counter_read_' + stats_tbl['name'])
        fn_map['write_stats'] = getattr(self.client, 'counter_write_' + stats_tbl['name'])
        fn_map['counter_value_t'] = globals()[p4_name + '_counter_value_t']
        self.fn_map = fn_map

    def refresh_thrift(self, test, resource_map):
        super(StatsTable, self).refresh_thrift(test)
        self.init_fn_map(resource_map)
        bf_log.log("Updated thrift connection for %s" % self.get_name())

    def supports_modification(self):
        ''' Return True if the table supports modification of the entry '''
        return False

    def support_spec_change(self):
        ''' Return True if this table can change it's spec using modify API'''
        ''' Match spec cannot change. So return False '''
        return False

    def swap_states(self, r1, r2):
        ''' Swap any state maintained for the 2 resources '''
        bf_log.debug_fn('Swapping stats states ' + str(r1) + str(r2))
        r1_count, r2_count = 0, 0

        if 'count' in r1:
            r1_count = r1['count']
        if 'count' in r2:
            r2_count = r2['count']

        r1['count'], r2['count'] = r2_count, r1_count

    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''
        x = rdata['uniq_val']

        if self.ref_type == 'INDIRECT':
            return hex_to_i32(x%self.get_size())
        else:
            return None

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis and return a unique id'''
        # In case of indirect stats, the count never really changes due to ent-add
        rdata.setdefault('count', 0)
        if self.ref_type == 'DIRECT':
            rdata['count'] = 0

    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        # In case of indirect stats, the count never really becomes zero
        if self.ref_type == 'INDIRECT':
            dev_tgt = DevTarget_t(target['p_dev_id'], pipe_to_pipe_id(target['pipe']))
            fn_params = {
                            'sess_hdl' : target['sess_hdl'],
                            'dev_tgt' : dev_tgt,
                            'index' : self.get_spec(rdata, None),
                            'counter_value' : self.fn_map['counter_value_t'](0,0)
                        }
#            entry_count = self.fn_map['write_stats'](**fn_params)
            return
        rdata['count'] = 0

    def verify_pkt(self, rdata, pkt, mdata, strict=True):
        ''' Verify that the pkt parameters conform to the expected transformations '''
        ''' Make sure that the mdata['pkt_count'] matches what we read from hardware '''
        pkt_count = mdata['pkt_count']

        dev_tgt = DevTarget_t(mdata['p_dev_id'], pipe_to_pipe_id(mdata['pipe']))
        fn_params = {
                        'sess_hdl' : mdata['sess_hdl'],
                        'dev_tgt' : dev_tgt,
                        'flags' : self.hw_sync_flag
                    }
        if self.ref_type == 'DIRECT':
            fn_params['entry'] = mdata['match_entry_hdl']
        else:
            fn_params['index'] = self.get_spec(rdata, None)

        bf_log.debug_fn( 'Stats verify ' +  str(fn_params))
        entry_count = self.fn_map['read_stats'](**fn_params)
        entry_count = entry_count.packets
        old_count = rdata['count']

        bf_log.debug_fn('Entry count %d old-count %d pkt-count %d' % \
                (entry_count, old_count, pkt_count))
        if strict and (entry_count - old_count) != pkt_count:
            bf_log.log('Stats count does not match for ' + str(fn_params) \
                    + ' entry-count %d old-count %d pkt-count %d' % \
                    (entry_count, old_count, pkt_count))
        assert (entry_count - old_count) == pkt_count, \
               'Stats count does not match for ' + str(fn_params) \
               + ' entry-count %d old-count %d pkt-count %d' % \
               (entry_count, old_count, pkt_count)
        rdata['count'] = old_count + pkt_count

    def clear_count(self, rdata):
        rdata['count'] = 0


class StatefulTable(Table):
    def __init__(self, test, resource_map, valid_ports):
        # Call Table constructor
        super(StatefulTable, self).__init__(test, resource_map, 'Stateful')

        stful_tbl = resource_map['Stateful']
        self.size = stful_tbl['size']
        self.ref_type = stful_tbl['ref_type']
        num_pipes = int(test_param_get('num_pipes'))
        self.pipe_list = [x for x in range(num_pipes)]
        self.pipe_list_cnt = len(self.pipe_list)

        self.init_fn_map(resource_map)

        self.hw_sync_flag = globals()[p4_name + '_register_flags_t'](read_hw_sync = True)
        self.hw_async_flag = globals()[p4_name + '_register_flags_t'](read_hw_sync = False)

        self.reg_vals = {}
        self.stages = None

    def init_fn_map(self, resource_map):
        stful_tbl = resource_map['Stateful']
        fn_map = RandomDict()
        fn_map['resources'] = RandomDict()
        fn_map['write'] = getattr(self.client, 'register_write_' + stful_tbl['name'])
        fn_map['query'] = getattr(self.client, 'register_read_' + stful_tbl['name'])
        fn_map['sync'] = getattr(self.client, 'register_hw_sync_' + stful_tbl['name'])
        self.fn_map = fn_map

    def refresh_thrift(self, test, resource_map):
        super(StatefulTable, self).refresh_thrift(test)
        self.init_fn_map(resource_map)
        bf_log.log("Updated thrift connection for %s" % self.get_name())

    def supports_modification(self):
        ''' Return True if the table supports modification of the entry '''
        return False

    def support_spec_change(self):
        ''' Return True if this table can change it's spec using modify API'''
        ''' Match spec cannot change. So return False '''
        return False

    def init_val(self, rdata):
        return rdata['uniq_val'] & 0xFF

    def swap_states(self, r1, r2):
        ''' Swap any state maintained for the 2 resources '''
        bf_log.debug_fn('Swapping stateful states ' + str(r1) + str(r2))

        if self.init_val(r1) == self.init_val(r2):
            r1_spec, r2_spec = [], []

            if 'r_spec' in r1:
                r1_spec = r1['r_spec']
            if 'r_spec' in r2:
                r2_spec = r2['r_spec']

            r1['r_spec'], r2['r_spec'] = r2_spec, r1_spec

    def get_spec(self, rdata, action_func):
        ''' Return the spec used for APIs from the id'''
        x = rdata['uniq_val']
        # Indirect - Return index
        # Direct - Return Spec
        if self.ref_type == 'INDIRECT':
            return x % self.size
        else:
            return self.init_val(rdata)

    def ent_add(self, rdata, action_func=None, target=None, params=None, replay=False, pre_ha_rdata=None, with_hitless_ha=False):
        ''' Add the entry into the table by calling thrift apis'''
        index = rdata['uniq_val'] % self.size

        if self.ref_type == 'INDIRECT':
            value = index & 0xFF
            dev_tgt = DevTarget_t(target['p_dev_id'], pipe_to_pipe_id(target['pipe']))
            self.fn_map['write'](target['sess_hdl'], dev_tgt, index, hex_to_i32(value))
            # Do a read to determine the number of stages the table uses.  This
            # only needs to be done once.
            if self.stages is None:
                x = self.fn_map['query'](target['sess_hdl'], dev_tgt, index, self.hw_sync_flag)
                if target['pipe'] == 'all':
                    self.stages = len(x) // self.pipe_list_cnt
                else:
                    self.stages = len(x)
            if target['pipe'] == 'all':
                self.reg_vals[(target['pipe'], index)] = [self.stages*value]*self.pipe_list_cnt
            else:
                self.reg_vals[(target['pipe'], index)] = [self.stages*value]
        else:
            value = self.init_val(rdata)
            if target['pipe'] == 'all':
                rdata['r_spec'] = []
                for x in range(self.pipe_list_cnt):
                    rdata['r_spec'].append(value)
            else:
                rdata['r_spec'] = [value]


    def ent_del(self, rdata, target):
        ''' Remove the entry by calling thrift apis '''
        if self.ref_type == 'INDIRECT':
            #dev_tgt = DevTarget_t(target['p_dev_id'], pipe_to_pipe_id(target['pipe']))
            #index =  rdata['uniq_val'] % self.size
            #self.fn_map['write'](target['sess_hdl'], dev_tgt, index, 0)
            pass
        else:
            del rdata['r_spec']

    def clear_stful(self, rdata, pipe):
        if self.ref_type == 'INDIRECT':
            index = rdata['uniq_val'] % self.size
            value = index & 0xFF
            if pipe == 'all':
                self.reg_vals[('all', index)] = [self.stages*value]*self.pipe_list_cnt
            else:
                self.reg_vals[(pipe, index)] = [self.stages*value]
        else:
            value = self.init_val(rdata)
            if pipe == 'all':
                rdata['r_spec'] = []
                for x in range(self.pipe_list_cnt):
                    rdata['r_spec'].append(value)
            else:
                rdata['r_spec'] = [value]

    def verify_pkt(self, rdata, pkt, mdata, strict=True):
        ''' Verify that the pkt parameters conform to the expected transformations '''
        # Read stateful value from chip
        dev_tgt = DevTarget_t(mdata['p_dev_id'], pipe_to_pipe_id(mdata['pipe']))
        sess_hdl = mdata['sess_hdl']
        ing_pipe = mdata['ingress_pipe']
        flag = self.hw_sync_flag
        if self.ref_type == 'DIRECT':
            entry = mdata['match_entry_hdl']
            exp_val = rdata['r_spec']
            reg = self.fn_map['query'](sess_hdl, dev_tgt, entry, flag)
            val = [i32_to_hex(r) for r in reg]
        else:
            index = self.get_spec(rdata, None)
            if mdata['pipe'] == 'all':
                pipe = 'all'
            else:
                pipe = ing_pipe
            exp_val = self.reg_vals[(pipe, index)]
            reg = self.fn_map['query'](sess_hdl, dev_tgt, index, flag)
            val = [i32_to_hex(r) for r in reg]

        # Transform saved value based on the action function used in indirect
        # cases or using a fixed transformation in the direct case.
        if self.ref_type == 'DIRECT':
            if mdata['pipe'] != 'all':
                ing_pipe = 0
            exp_val[ing_pipe] = (exp_val[ing_pipe] + 1) & 0xFFFFFFFF
            rdata['r_spec'] = exp_val
            summed_vals = val
        else:
            # Sum up values for all stages.
            if mdata['pipe'] == 'all':
                update_idx = ing_pipe
                summed_vals = [0]*self.pipe_list_cnt
                for i in range(len(val)):
                    x = summed_vals[i%self.pipe_list_cnt]
                    y = val[i]
                    summed_vals[i%self.pipe_list_cnt] = (x + y) & 0xFFFFFFFF
            else:
                update_idx = 0
                summed_vals = [0]
                for i in range(len(val)):
                    x = summed_vals[0]
                    y = val[i]
                    summed_vals[0] = (x + y) & 0xFFFFFFFF

            action_func = mdata['action_func']

            x = exp_val[update_idx]
            if 'tcp_dport_modify' == action_func:
                #print("Index", index, val, "pipe", ing_pipe, " +1")
                x += 1
            elif 'ipsa_modify' == action_func:
                #print("Index", index, val, "pipe", ing_pipe, " +1")
                x += 1
            elif 'ipda_modify' == action_func:
                #print("Index", index, val, "pipe", ing_pipe, " +100")
                x += 100
            elif 'ipds_modify' == action_func:
                #print("Index", index, val, "pipe", ing_pipe, " +1234")
                x += 1234
            elif 'ipttl_modify' == action_func:
                #print("Index", index, val, "pipe", ing_pipe, " +333")
                x += 333
            else:
                print("Unexpected action function:", action_func)
                assert(0)
            exp_val[update_idx] = x & 0xFFFFFFFF
            self.reg_vals[(pipe,index)] = exp_val

        # Ensure the transformed saved value equals the read value.
        if exp_val != summed_vals and self.ref_type == 'DIRECT':
            # Due to an entry move across stage, it is possible that
            # the stateful entry starts from initial value
            value = self.get_spec(rdata, None)
            if mdata['pipe'] == 'all':
                exp_val = []
                for x in range(self.pipe_list_cnt):
                    exp_val.append(value)
            else:
                exp_val = [value]
            exp_val[ing_pipe] = (exp_val[ing_pipe] + 1) & 0xFFFFFFFF
            rdata['r_spec'] = exp_val

        if strict and exp_val != summed_vals:
            bf_log.log("Stateful verify fails, register value is " + str(val) +  " summed is " + str(summed_vals) + " expected " + str(exp_val) + " pipe is " + str(mdata['pipe']) + ":" + str(ing_pipe) + " initial value was " + str(rdata['uniq_val'] & 0xFF) + " uniq_val is " + str(rdata['uniq_val']))
            assert(exp_val == summed_vals)


