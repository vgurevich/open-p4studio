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

from ctypes import *
from tdiTable import *
from tdiTofinoDefs import *

import functools

def target_check_and_set(f):
    @functools.wraps(f)
    def target_wrapper(*args, **kw):
        cintf = args[0]._cintf
        pipe = None
        gress_dir = None
        prsr_id = None
        old_tgt = False

        for k,v in kw.items():
            if k == "pipe":
                pipe = v
            elif k == "gress_dir":
                gress_dir = v
            elif k == "prsr_id":
                prsr_id = v

        if pipe is not None or gress_dir is not None or prsr_id is not None:
            _, old_pipe, old_gress_dir, old_prsr_id = cintf.get_target_vals(cintf._target)
            old_tgt = True
        if pipe is not None:
            cintf._set_pipe(pipe=pipe)
        if gress_dir is not None:
            cintf._set_direction(gress_dir)
        if prsr_id is not None:
            cintf._set_parser(prsr_id)
        # If there is an exception, then revert back the
        # target. Store the ret value of the original function
        # in case it does return something like some entry_get
        # functions and return it at the end
        ret_val = None
        try:
            ret_val = f(*args, **kw)
        except Exception as e:
            if old_tgt:
                cintf._set_pipe(pipe=old_pipe)
                cintf._set_direction(old_gress_dir)
                cintf._set_parser(old_prsr_id)
            raise e
        if old_tgt:
            cintf._set_pipe(pipe=old_pipe)
            cintf._set_direction(old_gress_dir)
            cintf._set_parser(old_prsr_id)
        return ret_val
    return target_wrapper

class TofinoTableEntry(TableEntry):
    @target_check_and_set
    def push(self, verbose=False, pipe=None, gress_dir=None, prsr_id=None):
        TableEntry.push(self, verbose)

    @target_check_and_set
    def update(self, pipe=None, gress_dir=None, prsr_id=None):
        TableEntry.update(self)

    @target_check_and_set
    def remove(self, pipe=None, gress_dir=None, prsr_id=None):
        TableEntry.remove(self)

class TofinoTable(TdiTable):
    key_match_type_cls = KeyMatchTypeTofino
    table_type_cls = TableTypeTofino
    attributes_type_cls = AttributesTypeTofino
    operations_type_cls = OperationsTypeTofino
    flags_type_cls = FlagsTypeTofino
    table_entry_cls = TofinoTableEntry

    """
        Remove $ and change the table names to lower case
    """
    def modify_table_names(self, table_name):
        self.name = table_name.value.decode('ascii')
        #Unify the table name for TDINode (command nodes)
        name_lowercase_without_dollar=self.name.lower().replace("$","")
        if self.table_type in ["PRE_MGID", "PRE_NODE", "PRE_ECMP", "PRE_LAG", "PRE_PRUNE", "PRE_PORT", "MIRROR_CFG"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["TM_PORT_GROUP_CFG", "TM_PORT_GROUP"]:
            self.name = name_lowercase_without_dollar
        if self.table_type in ["SNAPSHOT", "SNAPSHOT_LIVENESS"]:
            self.name = "{}".format(name_lowercase_without_dollar)

    def set_idle_table_poll_mode(self, enable):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["IdleTimeout"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("Error: idle_table_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,          # mode
                c_uint(0)) # poll mode
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                1,
                c_bool(bool(enable)))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def _wrap_port_status_notif_cb(self, callback):
        def callback_wrapper(dev_id, key_hdl, up, cookie):
            entry = self.get_entry(key_content=None, print_entry=False, key_handle=key_hdl)
            dev_port = list(entry.key.values())[0]
            callback(dev_id, dev_port, up)
            return 0
        return self._cintf.port_status_notif_cb_type(callback_wrapper)

    def set_port_status_notif_cb(self, callback):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["port_status_notif_cb"], byref(attr_hdl))
        if sts != 0:
            print("port_status_notif_attribute_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        self.port_status_notif_callback = self._wrap_port_status_notif_cb(callback)
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0, # enable
                c_bool(True))
        if not sts == 0:
            print("port_status set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                2, # c_cb
                self.port_status_notif_callback)
        if not sts == 0:
            print("port_status set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def _wrap_selector_table_update_cb(self, callback):
        def callback_wrapper(session, target, cookie, sel_grp_id, act_mbr_id, logical_table_index, is_add):
            dev_id, pipe_id, direction, parser_id = self._cintf.get_target_vals(target)
            callback(dev_id, pipe_id, direction, parser_id, sel_grp_id, act_mbr_id, logical_table_index, is_add)
            return 0
        return self._cintf.selector_table_update_cb_type(callback_wrapper)

    def set_selector_table_update_cb(self, callback):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_table_selector_table_update_cb_attributes_allocate(self._handle, byref(attr_hdl))
        if sts != 0:
            print("port_status_notif_attribute_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        self.selector_table_update_cb = self._wrap_selector_table_update_cb(callback)
        sts = self._cintf.get_driver().tdi_attributes_selector_table_update_cb_set(attr_hdl, c_bool(bool(True)), self._cintf.get_session(), self.selector_table_update_cb, c_void_p(0))
        if sts != 0:
            print("selector_table_update_cb failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def set_port_stats_poll_intv(self, poll_intv_ms):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["poll_intvl_ms"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("Error: port_stats_poll_intv_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,
                c_uint(poll_intv_ms)) # intvl_val
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            raise TdiTableError("Error: port_stats_poll_intv_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def get_port_stats_poll_intv(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["poll_intvl_ms"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("Error: port_stats_poll_intv_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        self._attr_get(attr_hdl)
        poll_intv_ms = c_uint(0)
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                0,
                byref(poll_intv_ms))
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            raise TdiTableError("Error: port_stats_poll_intv_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        self._attr_deallocate(attr_hdl)
        return poll_intv_ms.value

    def _wrap_idle_timeout_callback(self, callback):
        def callback_wrapper(target, handle, cookie):
            dev_id, pipe_id, direction, parser_id = self._cintf.get_target_vals(target)
            entry = self.get_entry(key_content=None, print_entry = False, key_handle=handle, dev_tgt=target)
            callback(dev_id, pipe_id, direction, parser_id, entry)
            return 0
        return self._cintf.tdi_idle_timeout_cb_type(callback_wrapper)

    def set_idle_table_notify_mode(self, enable, callback, interval, max_ttl, min_ttl):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["IdleTimeout"], byref(attr_hdl))
        if not sts == 0:
            print("idle_table_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        self.idle_tmo_callback = self._wrap_idle_timeout_callback(callback)
        """
        sts = self._cintf.get_driver().tdi_attributes_idle_table_notify_mode_set(attr_hdl,
                                                                                   c_bool(bool(enable)),
                                                                                   self.idle_tmo_callback,
                                                                                   c_uint(interval),
                                                                                   c_uint(max_ttl),
                                                                                   c_uint(min_ttl),
                                                                                   c_void_p(0))
                                                                                   """
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,          # mode
                c_uint(1)) # notify mode
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                1,
                c_bool(bool(enable)))
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                3,
                self.idle_tmo_callback)
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                4,
                c_uint(interval))
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                5,
                c_uint(max_ttl))
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                6,
                c_uint(min_ttl))
        if not sts == 0:
            print("set_idle_table_notify failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1

        if not sts == 0:
            print("idle_table_notify_mode_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def get_idle_table(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["IdleTimeout"], byref(attr_hdl))
        if not sts == 0:
            print("idle_table_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        sts = self._attr_get(attr_hdl)
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            return -1
        enable = c_bool()
        mode = c_uint()
        ttl_interval = c_uint()
        max_ttl = c_uint()
        min_ttl = c_uint()
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                0,
                byref(mode))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                1,
                byref(enable))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                4,
                byref(ttl_interval))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                5,
                byref(max_ttl))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                6,
                byref(min_ttl))
        if not sts == 0:
            print("idle_table_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1

        self._attr_deallocate(attr_hdl)
        mode = self.idle_table_mode(mode.value)
        if mode == "POLL_MODE":
            return {"enable": enable.value, "mode": mode}
        return {"enable": enable.value, "mode": mode, "ttl_interval": ttl_interval.value, "max_ttl": max_ttl.value, "min_ttl": min_ttl.value}

    def set_symmetric_mode(self, enable):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["EntryScope"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("Error: entry_scope_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        entry_scope_val = 0 # symmetric
        if not enable:
            entry_scope_val = 1 # asymmetric
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,
                c_uint(entry_scope_val))
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            raise TdiTableError("Error: entry_scope_symmetric_mode_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)

        ''' 
            PVS table needs special handling
            1. Setting entry_scope args as 0xff. This sets gress as ALL as PVS is defined for a gress
            2. Setting parser_scope as SINGLE.
            PVS table also supports prsr_scope and gress_scope
        '''
        table_type = self.table_type_cls.table_type_str(self.get_type())
        if table_type == "PVS":
            # setting args for entry_scope
            sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                1,                  # TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_ENTRY_SCOPE_ARGS
                c_uint(0xff))       # setting gress to ALL
            if not sts == 0:
                self._attr_deallocate(attr_hdl)
                raise TdiTableError("Error: entry_scope_symmetric_mode_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            
            # setting prsr_scope
            sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                4,                  # TDI_TOFINO_ATTRIBUTES_ENTRY_SCOPE_FIELD_TYPE_PARSER_SCOPE_TYPE
                c_uint(entry_scope_val))
            if not sts == 0:
                self._attr_deallocate(attr_hdl)
                raise TdiTableError("Error: entry_scope_symmetric_mode_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
 
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def get_symmetric_mode(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["EntryScope"], byref(attr_hdl))
        if not sts == 0:
            print("entry_scope_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            return -1
        sts = self._attr_get(attr_hdl)
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            return -1
        entry_scope_val = c_uint()
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                0,
                byref(entry_scope_val))
        if not sts == 0:
            print("symmetric_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_deallocate(attr_hdl)
        return False if entry_scope_val.value == 1 else True

    def meter_byte_count_adjust_set(self, byte_count):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["MeterByteCountAdjust"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("meter_byte_count_adjust_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                0,         # byte_adjust_value
                c_uint(abs(byte_count))) # 
        if not sts == 0:
            print("meter_byte_count_adjust_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            self._attr_deallocate(attr_hdl)
            return -1
        sts = self._cintf.get_driver().tdi_attributes_set_value(attr_hdl,
                1,         # byte_adjust_is_positive
                c_uint(1 if byte_count>0 else 0)) # 
        if not sts == 0:
            print("meter_byte_count_adjust_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            self._attr_deallocate(attr_hdl)
            return -1
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def meter_byte_count_adjust_get(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["MeterByteCountAdjust"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("meter_byte_count_adjust_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
        sts = self._attr_get(attr_hdl)
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            return -1, -1
        byte_count = c_uint()
        is_positive = c_uint()

        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                0,         # byte_adjust_value
                byref(byte_count)) # 
        if not sts == 0:
            print("meter_byte_count_adjust_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            self._attr_deallocate(attr_hdl)
            return -1, -1
        sts = self._cintf.get_driver().tdi_attributes_get_value(attr_hdl,
                1,         # byte_adjust_is_positive
                byref(is_positive)) # 
        if not sts == 0:
            print("meter_byte_count_adjust_get failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            self._attr_deallocate(attr_hdl)
            return -1, -1

        self._attr_deallocate(attr_hdl)
        return 0, byte_count.value if is_positive.value > 0 else -byte_count.value

    def dynamic_key_mask_set(self, key_content):
        attr_hdl = self._cintf.handle_type()
        key_handle = self._make_call_keys(key_content)
        if key_handle == -1:
            raise TdiTableError("dyn_key_mask_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["DynamicKeyMask"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("dyn_key_mask_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
        for name, info in self.key_fields.items():
            value, bytes_ = self.fill_c_byte_arr(key_content[name], info.size)
            sts = self._cintf.get_driver().tdi_attributes_set_value_ptr(attr_hdl, info.id, value, bytes_)
            if not sts == 0:
                self._attr_deallocate(attr_hdl)
                raise TdiTableError("dyn_key_mask_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
        self._attr_set(attr_hdl)
        self._attr_deallocate(attr_hdl)

    def dynamic_key_mask_get(self):
        attr_hdl = self._cintf.handle_type()
        sts = self._cintf.get_driver().tdi_attributes_allocate(self._handle, self.attributes_type_cls.attributes_rev_dict["DynamicKeyMask"], byref(attr_hdl))
        if not sts == 0:
            raise TdiTableError("dyn_key_mask_attributes_allocate failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)))
        sts = self._attr_get(attr_hdl)
        if not sts == 0:
            self._attr_deallocate(attr_hdl)
            return -1
        ret_val = {}
        for name, info in self.key_fields.items():
            value, bytes_ = self.fill_c_byte_arr(0, info.size)
            sts = self._cintf.get_driver().tdi_attributes_get_value_ptr(attr_hdl, info.id, bytes_, value)
            if not sts == 0:
                self._attr_deallocate(attr_hdl)
                raise TdiTableError("dyn_key_mask_set failed on table {}. [{}]".format(self.name, self._cintf.err_str(sts)), self, sts)
            ret_val[name] = self.from_c_byte_arr(value, info.size)
        self._attr_deallocate(attr_hdl)
        return ret_val

    def _process_target_specific_data_field(self, data_field, data_handle, content):
        if self.data_type_cls.data_type_str(data_field.data_type) == "BYTE_STREAM" and \
                ('$bfrt_field_class', 'register_data') in data_field.annotations:
            size = c_uint(0)
            sts = self._cintf.get_driver().tdi_data_field_get_value_u64_array_size(data_handle, data_field.id, byref(size))
            if sts == 0:
                arrtype = c_ulonglong * size.value
                value = arrtype()
                sts = self._cintf.get_driver().tdi_data_field_get_value_u64_array(data_handle, data_field.id, value)
                content[data_field.name] = []
                for i in range(0, size.value):
                    content[data_field.name].append(value[i])
            return True
        return False

    def _stringify_output_target_specific(self, data_field, value):
        if self.data_type_cls.data_type_str(data_field.data_type) == "BYTE_STREAM" and \
                ('$bfrt_field_class', 'register_data') in data_field.annotations:
            return data_field._stringify_int_arr(value)
        return None
