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

"""
csr: Code dealing with compiler-facing JSON files and raw Semifore CSV files

TODO: explain how to generate Semifore CSV files properly
"""

import csv
import sys

import os.path
import hashlib
import copy
import re

from operator import mul
from types import StringTypes

########################################################################
## Utility functions

def product(seq):
    """
    Given a list of integers, return their product

    @param   seq     A list of integers
    @return          The product of the integers in the list
    """
    return reduce(mul, seq, 1 )

def nd_array_loop(count, data, func, dim_idx=0):
    """
    Given an N-dimensional list of objects, drill down to the inner-most lists
    and call func() on them. Essentially a variable-depth nested for loop. The
    reason func is called on the inner-most dimension and not each object itself
    is to allow some state to be kept across iterations of the inner-most loop
    (for example, like an address offset counter).

    @param   count  A list of integers recording the number of elements
                    in each dimension of the data. Should be N elements
                    long.
    @param   data   The N-dimensional list of data
    @param   func   A function that takes one argument, which is called on
                    every list of the inner-most dimension
    """
    if dim_idx < len(count)-1:
        for idx in range(0, count[dim_idx]):
            nd_array_loop(count, data[idx], func, dim_idx+1)
    else:
        func(data)

########################################################################
## Structures

class binary_cache(object):
    """
    A class used to store flat chip_obj lists, each corresponding to one JSON
    file. The "_name" at the top of each JSON is used to index into the cache.
    Requesting a JSON file from here will crunch it into binary if it hasn't
    been already.
    """
    def __init__(self, schema):
        self.schema = schema
        self.templates = {}
        self.binary_templates = {}

    def get_type(self, key):
        """
        Get the addressmap name that the binary data at the given key
        corresponds to, of the form "section_name.addressmap_name".
        """
        return self.templates[key]["_type"]

    def get_data (self, key):
        """
        Return a list of objects inheriting from chip.chip_obj, representing
        the write operations that must be done in the hardware to program a
        hardware object of the given JSON file's "_type"

        These lists are a deep copy of the one stored internally, so it is safe
        to modify them
        """
        if key not in self.binary_templates:
            obj_section, obj_type = self.templates[key]["_type"].split(".")
            obj_schema = self.schema[obj_section][obj_type]
            # TODO: There used to be a first deepcopy here, before the one in the
            #       return statement. 99% sure it was unnecessary, but if things
            #       seem broken revisit this
            self.binary_templates[key] = obj_schema.generate_binary(self.templates[key], self)

        binary_data_copy = []
        for chip_obj in self.binary_templates[key]:
            binary_data_copy.append(chip_obj.deepcopy())
        return binary_data_copy

class csr_object (object):
    """
    Base class for objects in a Semifore register hierarchy

    A Semifore object array is still represented as one csr_object instance,
    albeit with a "count" attribute expressing how many hardware objects this
    Semifore node actually corresponds to.

    Since all objects in Semifore have names and can be arrays, all csr_objects
    have name and count attributes. Since arrays can be multidimensional,
    count is _always_ a tuple of array sizes, even if that tuple has only one
    element. Single elements will have a count of (1,).
    """

    def __init__(self, name, count):
        self.name = name
        self.count = count

    def replicate (self, templatized_self):
        if self.count != (1,):
            last_dim_obj = templatized_self
            for dim in reversed(self.count):
                last_dim_obj = [copy.deepcopy(last_dim_obj) for _ in range(0,dim)]
            return last_dim_obj
        else:
            return templatized_self

class address_map(csr_object):
    """
    A Semifore addressmap. Contains registers and instances of other
    addressmaps.

    In practice, the count of an address_map is always (1,) and it is the
    instances of the addressmap that are actually arrays.

    @attr top_level_obj
        A boolean indicating whether this type of addressmap should be expanded
        out when generating JSON templates, or just represented by a name
        string

    @attr objs
        An ordered list of objects contained in the addressmap - either regs
        address_map_instances

    @attr parent
        A string indicating which parent of the chip hierarchy the addressmap
        falls under ("memories" or "regs)
    """
    def __init__(self, name, count, parent):
        csr_object.__init__(self, name, count)

        self.top_level_obj = False
        self.objs = []
        self.parent = parent

    def min_width(self):
        """
        Some addressmap arrays have an explicit "stride" specifying how much
        address space each element takes up. When they don't, we calculate the
        stride by summing up the widths of all contained objects and rounding
        to the next highest power of 2.

        Whether an addressmap has a stride or not is up to the programmer of
        the original Semifore CSR and, as far as Walle is conserned, arbitrary.
        """
        width = 0
        for obj in self.objs:
            if type(obj) is reg:
                width += obj.width * product(obj.count)
            elif type(obj) is address_map_instance:
                try:
                    multiplier = product(obj.count) * obj.stride
                except:
                    multiplier = 1
                width += obj.map.min_width() * multiplier
            else:
                raise Exception("Unrecognized object in address map ('"+type(obj)+"')")
        return width

    def generate_binary(self, data, cache):
        if data == 0:
            # No-op
            return {}
        elif isinstance(data, StringTypes):
            # Refernce to template

            type_name = self.parent + "." + self.name
            if cache.get_type(data) != type_name:
                raise Exception("Expected type of instantiated object '"+data+"' to be '"+type_name+"', found '"+cache.get_type(data)+"'")

            return cache.get_data(data)
        else:
            # Actual data
            reg_values = []
            for obj in self.objs:
                if type(obj) is reg:
                    if obj.count == (1,):
                        chip_obj = obj.generate_binary(data[obj.name], cache)
                        if chip_obj != None:
                            reg_values.append(chip_obj)
                    elif data[obj.name] != 0:
                        # TODO: we should be able to DMA anything into the chip,
                        #       so this count > 4 heuristic should work well
                        #
                        #       but right now the model doesn't implement chip-
                        #       side register addresses, so we have to force
                        #       direct register writes for the regs part of
                        #       the schema and DMA for the mem part. lame.
                        #       use this count heuristic once the model is fixed.
                        #
                        # if product(obj.count) > 4:
                        root_parent = obj.parent
                        while type(root_parent) is not str:
                            root_parent = root_parent.parent
                        if product(obj.count) > 4 and root_parent=="memories":
                            mem = chip.dma_block(obj.offset, obj.width, src_key=obj.name)
                            def mem_loop(sub_data):
                                for idx in range(0, obj.count[-1]):
                                    obj.generate_binary(sub_data[idx], cache, mem)
                            nd_array_loop(obj.count, data[obj.name], mem_loop)
                            reg_values.append(mem)
                        else:
                            offset = [0]
                            width = obj.width/8 #TODO: warn if not power of (8 or 32 or w/e)?
                            def reg_loop(sub_data):
                                for idx in range(0, obj.count[-1]):
                                    chip_obj = obj.generate_binary(sub_data[idx], cache)
                                    if chip_obj != None:
                                        chip_obj.add_offset(offset[0])
                                        reg_values.append(chip_obj)
                                    offset[0] += width
                            nd_array_loop(obj.count, data[obj.name], reg_loop)

                elif type(obj) is address_map_instance:
                    if obj.count == (1,):
                        sub_chip_objs = obj.generate_binary(data[obj.name], cache)
                        for sub_chip_obj in sub_chip_objs:
                            sub_chip_obj.add_offset(obj.offset)
                            reg_values.append(sub_chip_obj)
                    elif data[obj.name] != 0:
                        offset = [0]
                        def addr_map_loop(sub_data):
                            for idx in range(0, obj.count[-1]):
                                sub_chip_objs = obj.generate_binary(sub_data[idx], cache)
                                for sub_chip_obj in sub_chip_objs:
                                    sub_chip_obj.add_offset(obj.offset+offset[0])
                                    reg_values.append(sub_chip_obj)
                                offset[0] += obj.stride
                        nd_array_loop(obj.count, data[obj.name], addr_map_loop)
                else:
                    raise Exception("Unrecognized object in address map ('"+type(obj)+"')")

            return reg_values

    def generate_template(self):
        self_dict = {}
        if self.top_level_obj:
            self_dict["_type"] = self.parent + "." + self.name
            self_dict["_name"] = "template("+self_dict["_type"]+")"
        for obj in self.objs:
            self_dict[obj.name] = obj.generate_template()
        return self.replicate(self_dict)


class address_map_instance(csr_object):
    """
    TODO: docstring
    """
    def __init__(self, name, count, offset, addrmap, stride):
        csr_object.__init__(self, name, count)

        self.offset = offset
        self.map = addrmap
        self.stride = stride

    def generate_binary(self, data, cache):
        # TODO: should address offset be set to 0 or self.offset here?
        return self.map.generate_binary(data, cache)

    def generate_template(self):
        if self.map.top_level_obj:
            return self.replicate(self.map.name+"_object")
        else:
            return self.replicate(self.map.generate_template())


class reg(csr_object):
    """
    TODO: docstring
    """
    def __init__(self, name, count, offset, width, parent):
        csr_object.__init__(self, name, count)

        self.parent = parent
        self.offset = offset
        self.width = width
        self.fields = []

    def generate_binary(self, data, cache, mem=None):
        if data == 0:
            # No-op
            return None
        elif isinstance(data, StringTypes):
            # Refernce to template

            type_name = self.parent + "." + self.name
            if cache.get_type(data) != type_name:
                raise Exception("Expected type of instantiated object '"+data+"' to be '"+type_name+"', found '"+cache.get_type(data)+"'")

            if mem:
                mem.add_word(cache.get_data(data))
            else:
                return cache.get_data(data)
        else:
            reg_value = [0]
            for field in self.fields:
                width = field.msb-field.lsb+1
                if field.count == (1,):
                    if data[field.name] <= pow(2, width):
                        reg_value[0] |= data[field.name] << field.lsb
                    else:
                        raise Exception(
                            "Width of field '%s.%s' (%i bits) not large enough to hold value (%s)" % (
                            self.name, field.name, width, str(data[field.name])
                        ))
                else:
                    offset = [0]
                    def field_loop(sub_data):
                        for idx in range(0, field.count[-1]):
                            if sub_data[idx] <= pow(2, width):
                                reg_value[0] |= sub_data[idx] << field.lsb + offset[0]
                                offset[0] += width
                            else:
                                raise Exception(
                                    "Width of field '%s.%s[%i]' (%i bits) not large enough to hold value (%i)" % (
                                    self.name, field.name, idx, width, sub_data[idx]
                                ))
                    nd_array_loop(field.count, data[field.name], field_loop)

            if mem:
                mem.add_word(reg_value[0])
            elif self.width <= 32:
                return chip.direct_reg(self.offset, reg_value[0], src_key=self.name)
            else:
                return chip.indirect_reg(self.offset, reg_value[0], self.width, src_key=self.name)

    def generate_template(self):
        self_dict = {}
        for field in self.fields:
            self_dict[field.name] = field.generate_template()
        return self.replicate(self_dict)


class field(csr_object):
    """
    TODO: docstring
    """
    def __init__(self, name, count, msb, lsb, parent):
        csr_object.__init__(self, name, count)

        self.parent = parent
        self.msb = msb
        self.lsb = lsb

    def generate_template(self):
        return self.replicate(0x00)



########################################################################
## Utility functions

def str_to_array_size(size_str):
    """
    Turn an array size from a Semifore CSV string into a tuple of ints

    Semifore CSV formats the size of an array as an int in square brackets.
    Multidimensional arrays are just a lot of these concatenated together:
    [i]
    [i][j][k]
    ...

    If the array is 1D this function will still output the size as a 1-element
    tuple, just for consistency of iterability.
    """
    size_strs = size_str.replace("]","").split("[")[1:]
    sizes = map(int, size_strs)
    if len(sizes) > 0:
        return tuple(sizes)
    else:
        return (1,)



def parse_csrcompiler_csv_fields (filename, section_name):
    """
    Given a Semifore CSV file, parse it into a bunch of csr_object instances.
    Since the chip hierarchy is contained across multiple CSV files, each one
    has a unique "section name".

    @param  filename        The filename of the CSV file to parse
    @param  section_name    A string meaningfully describing the contents of
                            the CSV (eg, "memories" and "regs")
    @return A list of all addressmaps parsed out of the file.
    """

    csv_field_types = {
        "configuration",
        "constant",
        "counter",
        "status",
        "hierarchical_interrupt",
        "interrupt"
    }

    csv_register_types = {
        "register",
        "wide register",
        "userdefined register",
        "userdefined wide register"
    }

    addr_maps = {}
    active_addr_map = None
    active_reg = None
    open_reg = 0
    open_width = 0
    open_reg_name =""
    list_elts = 0
    fld_handlers = 0
    open_grp = 0
    open_grp_name = ""
    open_grp_offset = 0
    open_addr_map = ""

    with open(filename, "rb") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        row_num = 0
        for row in csv_reader:
            array_size = str_to_array_size(row["Array"])

            if row["Type"] in ["addressmap", "userdefined addressmap"] :
                addr_maps[row["Identifier"]] = address_map(row["Identifier"], array_size, section_name)
                active_addr_map = addr_maps[row["Identifier"]]
                open_addr_map = row["Identifier"]
            elif row["Type"] in csv_register_types:
                reg_width = int(row["Register Size"].replace(" bits",""),0)
                active_addr_map.objs.append(reg(row["Identifier"], array_size, int(row["Offset"],0), reg_width, active_addr_map))
                active_reg = active_addr_map.objs[-1]

                identifier = row["Identifier"]
                typename   = open_addr_map + "__" + open_grp_name + "_" + row["Identifier"]
                offset     = open_grp_offset + int(row["Offset"],base=16)

                if open_reg == 1:
                    print "};"
                    print ("reg_decoder_t " + open_reg_name +
                           " = { " + str(list_elts) + ", " +
                           open_reg_name + "_fld_list, " +
                           str(open_reg_width) + " /* bits */ };\n" )
                open_reg = 1
                open_reg_width = reg_width
                open_reg_name = typename
                print ( "reg_decoder_fld_t " +
                         open_reg_name + "_fld_list[] = {" )
                list_elts = 0

            elif row["Type"] in csv_field_types:
                range_tokens = row["Position"].replace("[","").replace("]","").split(":")
                msb = int(range_tokens[0])
                if len(range_tokens) == 1:
                    lsb = msb
                else:
                    lsb = int(range_tokens[1])
                active_reg.fields.append(field(row["Identifier"], array_size, msb, lsb, active_reg))
                if array_size == (1,):
                    print ( "    { \"" + row["Identifier"] + "\", "
                                   + str(msb) + ", " + str(lsb) + ", 0,"
                                   + str(fld_handlers) + ", " )
                else:
                    print ( "    { \"" + row["Identifier"] + "\", "
                                   + str(msb) + ", " + str(lsb) + ", 1,"
                                   + str(fld_handlers) + ", " )
                print ( "      " + "\"" + row["Description"].replace('\n','').replace('\r','') + "\"" + " }," )
                fld_handlers = fld_handlers + 1
                list_elts = list_elts + 1

            elif row["Type"] == "addressmap instance":
                try:
                    stride = int(row["Stride"].replace(" bytes",""),0)
                except:
                    byte_stride = addr_maps[row["Type Name"]].min_width()
                    # Round stride up to the next largest power of 2
                    stride = 1
                    while stride < byte_stride:
                        stride *= 2

                active_addr_map.objs.append(
                    address_map_instance(
                        row["Identifier"],
                        array_size,
                        int(row["Offset"],0),
                        addr_maps[row["Type Name"]],
                        None if array_size == (1,) else stride
                    )
                )

            elif row["Type"] == "group":
                open_grp = 1
                open_grp_name = row["Identifier"]
                open_grp_offset = int(row["Offset"],base=16)

            elif row["Type"] == "userdefined group":
                open_grp = 1
                open_grp_name = row["Identifier"]
                open_grp_offset = int(row["Offset"],base=16)

            elif row["Type"] == "endgroup":
                open_grp = 0
                open_grp_name = ""
                open_grp_offset = 0

            elif row["Type"] in ["group", "endgroup"] :
                # Ignore groups for now
                # TODO: is it okay to ignore them indefinitely?
                pass
            else:
                raise Exception("Unrecognized type '"+row["Type"]+"' in CSV file '"+filename+"' line "+str(row_num))

            row_num += 1

        if open_reg == 1:
            print "};"
            print "reg_decoder_t " + open_reg_name + " = { " + str(list_elts) + ", " + open_reg_name + "_fld_list };\n"

    csv_file.close()
    return addr_maps


def parse_csrcompiler_csv (filename, section_name):
    """
    Given a Semifore CSV file, parse it into a bunch of csr_object instances.
    Since the chip hierarchy is contained across multiple CSV files, each one
    has a unique "section name".

    @param  filename        The filename of the CSV file to parse
    @param  section_name    A string meaningfully describing the contents of
                            the CSV (eg, "memories" and "regs")
    @return A list of all addressmaps parsed out of the file.
    """

    csv_field_types = {
        "configuration",
        "constant",
        "counter",
        "status",
        "hierarchical_interrupt",
        "interrupt"
    }

    csv_register_types = {
        "register",
        "wide register",
        "userdefined register",
        "userdefined wide register"
    }

    csv_memory_types = {
       "userdefined memory"
    }

    addr_maps = {}
    active_addr_map = None
    active_reg = None
    open_arg = 0
    open_arg_name = ""
    list_elts = 0
    open_grp = 0
    open_grp_name = ""
    open_grp_offset = 0
    open_addr_map = ""

    with open(filename, "rb") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        row_num = 0
        for row in csv_reader:
            # Skip the MAU address map since it doesn't use CSR's memspec
            if re.match("mau_addrmap", row["Type Name"]):
                continue
            array_size = str_to_array_size(row["Array"])
            if row["Type"] in ["addressmap", "userdefined addressmap"] :
                addr_maps[row["Identifier"]] = address_map(row["Identifier"], array_size, section_name)
                active_addr_map = addr_maps[row["Identifier"]]
                if open_arg == 1:
                    print "};"
                    print ("cmd_arg_t " + "tof2_" + open_arg_name +
                           "_mem = { " + str(list_elts) + ", " + "tof2_" +
                           open_arg_name + "_mem_list };\n" )
                open_arg = 1
                open_arg_name = row["Identifier"]
                open_addr_map = row["Identifier"]
                print "cmd_arg_item_t " + "tof2_" + open_arg_name + "_mem_list[] = {"
                list_elts = 0
            elif row["Type"] in csv_register_types or row["Type"] in csv_memory_types:
                if row["Type"] in csv_memory_types:
                    mem_width = 16 # bytes
                    mem_row_cnt = int(row["Word Count"], 0)
                    sz = mem_width * mem_row_cnt
                else:
                    if re.match(".*bits.*", row["Register Size"]):
                        reg_width = int(row["Register Size"].replace(" bits",""),0)
                    else:
                        reg_width = 32
                    sz = reg_width / 8
                active_addr_map.objs.append(reg(row["Identifier"], array_size, int(row["Offset"],0), sz, active_addr_map))
                active_reg = active_addr_map.objs[-1]

                elt_offset = 0
                if open_grp_name == "":
                    identifier = row["Identifier"]
                else:
                    identifier = open_grp_name + "_" + row["Identifier"]

                typename   = open_addr_map + "__" + open_grp_name + "_" + row["Identifier"]
                offset     = open_grp_offset + int(row["Offset"],base=16)
                offset     = offset/16

                if row["Type"] in csv_register_types:
                    if array_size == (1,):
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", 0 }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 1:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", 0 }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 2:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", 0 }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 3:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", 0 }," )
                        list_elts = list_elts + 1
                else:
                    if array_size == (1,):
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", " +
                                hex(sz) + " }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 1:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", " +
                                hex((sz)*array_size[0]) + " }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 2:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", " +
                                hex((sz)*array_size[0]*array_size[1]) + " }," )
                        list_elts = list_elts + 1
                    elif len(array_size) == 3:
                        print ( "{ \"" + identifier + "\"" +
                                ", NULL, " + hex(offset) + ", &" +
                                "tof2_" + open_arg_name + "__" + identifier + ", " +
                                hex((sz)*array_size[0]*array_size[1]*array_size[2]) + " }," )
                        list_elts = list_elts + 1

            elif row["Type"] in csv_field_types:
                range_tokens = row["Position"].replace("[","").replace("]","").split(":")
                msb = int(range_tokens[0])
                if len(range_tokens) == 1:
                    lsb = msb
                else:
                    lsb = int(range_tokens[1])
                active_reg.fields.append(field(row["Identifier"], array_size, msb, lsb, active_reg))
            elif row["Type"] == "addressmap instance":
                try:
                    stride = int(row["Stride"].replace(" bytes",""),0)
                except:
                    byte_stride = addr_maps[row["Type Name"]].min_width()
                    # Round stride up to the next largest power of 2
                    stride = 1
                    while stride < byte_stride:
                        stride *= 2

                active_addr_map.objs.append(
                    address_map_instance(
                        row["Identifier"],
                        array_size,
                        int(row["Offset"],0),
                        addr_maps[row["Type Name"]],
                        None if array_size == (1,) else stride
                    )
                )
                elt_offset = 0
                if array_size == (1,):
                    elt_offset = int(row["Offset"],base=16)
                    elt_offset = elt_offset/16

                    # special-case for Comira MAC registers
                    print ( "{ \"" + row["Identifier"] +
                                "\"" + ", &" + "tof2_" + row["Type Name"]  + "_mem, " +
                                hex(elt_offset) + ", NULL }," )
                    list_elts = list_elts + 1
                elif len(array_size) == 1:
                    for idx_i in range(0, array_size[0]):
                        elt_offset = ( int(row["Offset"],base=16) +
                                       stride*(idx_i) )
                        elt_offset = elt_offset/16
                        if row["Type Name"] == "mac_addrmap":
                            print ( "{ \"" + row["Identifier"] +
                                    "[" + str(idx_i) + "]" + "\"" +
                                    ", &comira_list" + ", " +
                                    hex(elt_offset) + ", NULL }," )
                        else:
                            print ( "{ \"" + row["Identifier"] +
                                    "[" + str(idx_i) + "]" + "\"" +
                                    ", &" + "tof2_" + row["Type Name"] + "_mem, " +
                                    hex(elt_offset) + ", NULL }," )
                        list_elts = list_elts + 1
                elif len(array_size) == 2:
                    for idx_i in range(0, array_size[0]):
                        for idx_j in range(0, array_size[1]):
                            elt_offset = ( int(row["Offset"],base=16) +
                                           stride*(idx_i*array_size[1] +
                                           idx_j) )
                            elt_offset = elt_offset/16
                            print ( "{ \"" + row["Identifier"] +
                                    "[" + str(idx_i) + "]" + "[" +
                                    str(idx_j) + "]" + "\"" + ", &" + "tof2_" +
                                    row["Type Name"] + "_mem, " +
                                    hex(elt_offset) + ", NULL }," )
                            list_elts = list_elts + 1
                elif len(array_size) == 3:
                    for idx_i in range(0, array_size[0]):
                        for idx_j in range(0, array_size[1]):
                            for idx_k in range(0, array_size[2]):
                                elt_offset = ( int(row["Offset"],base=16) +
                                               stride*(idx_i*array_size[1]*array_size[2] +
                                               idx_j*array_size[2] + idx_k) )
                                elt_offset = elt_offset/16
                                print ( "{ \"" + row["Identifier"] +
                                        "[" + str(idx_i) + "]" +
                                        "[" + str(idx_j) + "]" +
                                        "[" + str(idx_k) + "]" +
                                        "\"" + ", &" + "tof2_" + row["Type Name"] +
                                        "_mem, " + hex(elt_offset) + ", NULL }," )
                                list_elts = list_elts + 1
            elif row["Type"] == "group":
                open_grp = 1
                open_grp_name = row["Identifier"]
                open_grp_offset = int(row["Offset"],base=16)

            elif row["Type"] == "userdefined group":
                open_grp = 1
                open_grp_name = row["Identifier"]
                open_grp_offset = int(row["Offset"],base=16)

            elif row["Type"] == "endgroup":
                open_grp = 0
                open_grp_name = ""
                open_grp_offset = 0

            elif row["Type"] in ["group", "endgroup"] :
                # Ignore groups for now
                # TODO: is it okay to ignore them indefinitely?
                pass
            else:
                raise Exception("Unrecognized type '"+row["Type"]+"' in CSV file '"+filename+"' line "+str(row_num))

            row_num += 1

        if open_arg == 1:
            print "};"
            print "cmd_arg_t " + "tof2_"+ open_arg_name + "_mem = { " + str(list_elts) + ", " + "tof2_" + open_arg_name + "_mem_list };\n"

    csv_file.close()
    return addr_maps

# csr_compiler_pass0
#
# Pass0 outputs the register decoder structures, documenting the register fields
#
def csr_compiler_pass0( filename ):

    csr_map_types = {
        "addressmap",
        "userdefined addressmap",
    }
    csr_group_types = {
        "group",
        "userdefined group"
    }
    csr_register_types = {
        "register",
        "wide register",
        "userdefined register",
        "userdefined wide register",
    }
    csr_field_types = {
        "configuration",
        "userdefined configuration",
        "constant",
        "counter",
        "status",
        "hierarchicalInterrupt",
        "interrupt"
    }

    csv_memory_types = {
       "userdefined memory"
    }

    nested_map = []
    active_map_name = ""
    active_reg_name = ""
    sz_str = ""
    is_int_reg = 0
    list_elts  = 0
    active_reg_width = 0;

    with open(filename, "rb") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        row_num = 0
        for row in csv_reader:
            array_size = str_to_array_size(row["Array"])
            if row["Type"] in ["endgroup"]:
                active_map_name = nested_map.pop()
                #print "POP : " + active_map_name

            elif row["Type"] in csr_map_types:
                #active_map_name = row["Identifier"]
                active_map_name = row["Type Name"]

            elif row["Type"] in csr_group_types:
                if active_map_name != "":
                    nested_map.append( active_map_name )
                    active_map_name = active_map_name + "__" + row["Type Name"] + "__" + row["Identifier"]
                else:
                    active_map_name = row["Type Name"] + "__" + row["Identifier"]

            elif row["Type"] in csr_register_types or row["Type"] in csv_memory_types:
                if active_reg_name != "":
                    # terminate previous reg
                    print "};"
                    print ("reg_decoder_t " + "tof2_" + active_reg_name +
                           " = { " + str(list_elts) + ", " + "tof2_" +
                           active_reg_name + "_fld_list, " +
                           str(active_reg_width) + " /* bits */, " + str(is_int_reg) + " };" )

                active_reg_name = active_map_name + "__" + row["Identifier"]

                print ""
                print ( "reg_decoder_fld_t " + "tof2_" +
                         active_reg_name + "_fld_list[] = {" )

                active_reg_width = int(row["Register Size"].replace(" bits",""),0)
                is_int_reg = 0
                list_elts  = 0

            elif row["Type"] in csr_field_types:
                # dump field def
                list_elts = list_elts + 1
                range_tokens = row["Position"].replace("[","").replace("]","").split(":")
                msb = int(range_tokens[0])
                if len(range_tokens) == 1:
                    lsb = msb
                else:
                    lsb = int(range_tokens[1])
                # have to manually expand arrays
                if len(array_size) == 1:
                    if array_size[0] == 1:
                        #print "    [" + str(msb) + ":" + str(lsb) + "] : " + row["Identifier"] + row["Array"]

                        print ( "    { \"" + row["Identifier"] + "\", "
                                       + str(msb) + ", " + str(lsb) + ", 0, "
                                       + str(0) + "," )
                        print ( "      " + "\"" + row["Description"].replace('\n','').replace('\r','') + "\"" + " }," )
                    else:
                        field_width = msb - lsb + 1
                        for idx in range(0,array_size[0]):
                            #print "    [" + str(msb) + ":" + str(lsb) + "] : " + row["Identifier"] + "[" + str(idx) + "]"

                            print ( "    { \"" + row["Identifier"] + "[" + str(idx) + "]" + "\", "
                                           + str(msb) + ", " + str(lsb) + ", 0, "
                                           + str(0) + "," )
                            print ( "      " + "\"" + row["Description"].replace('\n','').replace('\r','') + "\"" + " }," )
                            msb = msb + field_width
                            lsb = lsb + field_width
                            list_elts = list_elts + 1
                        # fix-up after loop (leaves it with one extra
                        list_elts = list_elts - 1

        # terminate previous reg
        print "};"
        print ("reg_decoder_t " + "tof2_" + active_reg_name +
               " = { " + str(list_elts) + ", " + "tof2_" +
               active_reg_name + "_fld_list, " +
               str(active_reg_width) + " /* bits */, " + str(is_int_reg) + " };" )

    print ("")
    csv_file.close()

def build_reg_info(dir):
    full_csr_file = os.path.join(dir,"jbay_mem.csv")
    # Generate decoder information
    csr_compiler_pass0( full_csr_file )
    # parse_csrcompiler_csv_fields( full_csr_file, "regs" )
    parse_csrcompiler_csv( full_csr_file, "regs" )

# Unit tests
if __name__ == "__main__":

    build_reg_info( str(sys.argv[1]) )
