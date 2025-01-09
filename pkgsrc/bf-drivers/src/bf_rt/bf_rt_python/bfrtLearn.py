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
from ctypes import *
import pdb

class BfRtLearnError(Exception):
    def __init__(self, str_rep, lrn_obj, sts, *args,**kwargs):
        self.sts = sts
        self.learn = lrn_obj
        self.str_rep = str_rep
        Exception.__init__(self, str_rep, *args, **kwargs)

class BfRtLearn:

    """
    This class manages the exchange of information between
    the CLI and BF Runtime's C API. It has two primary parts:
      - Looking up the action and field metadata for a table
      - Parsing and reformatting data as the user makes calls

    Note that keys in this object are the c-string representation
    (byte-streams in python) of data, not python strings.
    """
    def __init__(self, cintf, handle, info):
        self._cintf = cintf
        self._bfrt_info = info
        self._handle = handle
        self.bf_rt_id = -1
        self.field_readables = []
        sts = self._init_name()
        if not sts == 0:
            raise BfRtLearnError("Learn init failed.", None, -1)
        sts = self._init_fields()
        if not sts == 0:
            raise BfRtLearnError("Learn init failed.", None, -1)

    def set_frontend(self, bfrt_leaf):
        self.frontend = bfrt_leaf

    def set_id(self, bf_rt_id):
        self.bf_rt_id = bf_rt_id

    def get_id(self):
        return self.bf_rt_id

    def _get_fields(self, data_handle):
        content = {}
        for name, info in self.fields.items():
            sts = -1
            if not info.is_ptr:
                value = c_ulonglong(0)
                sts = self._cintf.get_driver().bf_rt_data_field_get_value(data_handle, info.id, byref(value))
                content[name.decode('ascii')] = value.value
            else:
                value, bytes_ = self.fill_c_byte_arr(0, info.size)
                sts = self._cintf.get_driver().bf_rt_data_field_get_value_ptr(data_handle, info.id, bytes_, value)
                content[name.decode('ascii')] = self.from_c_byte_arr(value, info.size)
            if not sts == 0:
                content[name.decode('ascii')] = self._cintf.err_str(sts)
        return content

    def _wrap_learn_callback(self, callback):
        def callback_wrapper(target, session, data_hdls, num_fields, msg_hdl, cookie):
            dev_id = target.contents.dev_id
            pipe_id = target.contents.pipe_id
            direction = target.contents.direction
            parser_id = target.contents.prsr_id
            content = []
            for idx in range(0, num_fields):
                data_hdl = data_hdls[idx]
                content.append(self._get_fields(data_hdl))
            sts = callback(dev_id, pipe_id, direction, parser_id, session, content)
            self._cintf.get_driver().bf_rt_learn_notify_ack(self._handle, session, msg_hdl)
            return sts
        return self._cintf.learn_cb_type(callback_wrapper)


    def callback_register(self, callback):
        self.wrapped_callback = self._wrap_learn_callback(callback)
        sts = self._cintf.get_driver().bf_rt_learn_callback_register(self._handle,
                                                                     self._cintf.get_session(),
                                                                     self._cintf.get_dev_tgt(),
                                                                     self.wrapped_callback,
                                                                     c_void_p(0))
        if not sts == 0:
            raise BfRtLearnError("Error: callback_register failed on learn {} with sts {}".format(self.name, self._cintf.err_str(sts)), self, sts)

    def callback_deregister(self):
        sts = self._cintf.get_driver().bf_rt_learn_callback_deregister(self._handle, self._cintf.get_session(), self._cintf.get_dev_tgt())
        if not sts == 0:
            raise BfRtLearnError("Error: callback_deregister failed on learn {} with sts {}".format(self.name, self._cintf.err_str(sts)), self, sts)

    class BfRtLearnField:

        """
        This class acts as a storage container for table field info. It
        also exposes a parse method for transforming user input into data
        the BF Runtime C API can accept and a deparse method for printing
        API outputs in an easy to read format.
        """
        def __init__(self, name, id_, size, is_ptr):
            self.name = name
            self.id = id_
            self.size = size
            self.is_ptr = is_ptr

        def _deparse_int(self, value):
            try:
                return int(value)
            except:
                return value

        def _deparse_float(self, value):
            try:
                return float(value)
            except:
                return value

        def _deparse_string(self, value):
            return str(value)

        def _deparse_int_arr(self, value):
            return [value[i] for i in range(len(value))]

        def deparse_output(self, value):
            return self._deparse_int(value)

        def _stringify_int(self, value):
            format_str = '0x{{:0{}X}}'.format(self.size // 4)
            return format_str.format(value)

        def _stringify_float(self, value):
            return '{}'.format(value)

        def _stringify_int_arr(self, value):
            return '{}'.format(value)

        def stringify_output(self, value):
            if isinstance(value, str):
                return value
            return self._stringify_int(value)

    """
    A convenience method for transforming a python integer to a
    network-order byte array.
    """
    @staticmethod
    def fill_c_byte_arr(content, size):
        bytes_ = (size + 7) // 8 # floor division
        value_type = c_ubyte * bytes_
        value = value_type()
        for i in range(0, bytes_):
            value[i] = (content >> 8 * (bytes_ - (i + 1))) & 0xFF
        return value, bytes_

    @staticmethod
    def from_c_byte_arr(value, size):
        bytes_ = (size + 7) // 8 # floor division
        content = 0
        for i in range(0, bytes_):
            content += value[i] << (bytes_ - (i + 1)) * 8
        return content

    def _init_name(self):
        learn_name = c_char_p()
        sts = self._cintf.get_driver().bf_rt_learn_name_get(self._handle, byref(learn_name))
        if not sts == 0:
            raise BfRtLearnError("CLI Error: get learn name failed with status {}.".format(self._cintf.err_str(sts)), self, sts)
        self.name = learn_name.value.decode('ascii')
        return 0

    """
    - Find the number of fields in the table
    - Get the list of field ids
    - Get info about each field (name, type, size, is_ptr)
    """
    def _init_fields(self):
        self.fields = {}
        num_ids = c_uint(-1)
        sts = self._cintf.get_driver().bf_rt_learn_field_id_list_size_get(self._handle, byref(num_ids))
        if not sts == 0:
            print("CLI Error: get num fields for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        if num_ids.value == 0:
            return 0
        array_type = c_uint * num_ids.value
        field_ids = array_type()
        sts = self._cintf.get_driver().bf_rt_learn_field_id_list_get(self._handle, field_ids)
        if not sts == 0:
            print("CLI Error: get field ids for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
            return sts
        for field_id in field_ids:
            field_name = c_char_p()
            sts = self._cintf.get_driver().bf_rt_learn_field_name_get(self._handle, field_id, byref(field_name))
            if not sts == 0:
                print("CLI Error: get field name for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
                return sts
            field_size = c_uint()
            sts = self._cintf.get_driver().bf_rt_learn_field_size_get(self._handle, field_id, byref(field_size))
            if not sts == 0:
                print("CLI Error: get field size for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
                return sts
            field_is_ptr = c_bool()
            sts = self._cintf.get_driver().bf_rt_learn_field_is_ptr_get(self._handle, field_id, byref(field_is_ptr))
            if not sts == 0:
                print("CLI Error: get field is_ptr for {} failed with status {}.".format(self.name, self._cintf.err_str(sts)))
                return sts
            readable = "      {} : size={}".format(field_name.value.decode('ascii'), field_size.value)
            self.field_readables.append(readable.strip())
            self.fields[field_name.value] = self.BfRtLearnField(field_name.value, field_id, field_size.value, field_is_ptr.value)
        return 0
