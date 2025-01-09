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
import pdb

import os.path
import hashlib
import copy

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


class a_map():
    def __init__(self, name, offset, array_sz, obj_type, width, type_name):
        self.regs_and_maps = []
        self.name          = name
        self.offset        = offset
        self.array_sz      = array_sz
        self.obj_type      = obj_type
        self.width         = width
        self.type_name     = type_name

    def add_map_entry(self, name, offset, array_sz, obj_type, width, type_name):
        new_map_entry = { 'name':name, 'offset':offset, 'arr_sz':array_sz, 'obj_type':obj_type, 'width':width, 'type_name':type_name, 'fld_list':[] }
        self.regs_and_maps.append( new_map_entry )



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


def array_sz_to_str( sz ):
    if len(sz) == 1:
        if sz[0] == 1:
           return ""
        else:
            return "[" + str(sz[0]) + "]"
    elif len(sz) == 2:
        return "[" + str(sz[0]) + "][" + str(sz[1]) + "]" 
    elif len(sz) == 3:
        return "[" + str(sz[0]) + "][" + str(sz[1]) + "][" + str(sz[2]) + "]"
    elif len(sz) == 4:
        return "[" + str(sz[0]) + "][" + str(sz[1]) + "][" + str(sz[2]) + "][" + str(sz[3]) + "]"


# csr_compiler_pass1
#
# Pass1 simply collects all the different kinds of maps into a dictionary
#
def csr_compiler_pass1( filename ):

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
        "constant",
        "counter",
        "status",
        "hierarchicalInterrupt",
        "interrupt"
    }

    addr_maps = {}
    nested_map = []
    active_map = None
    active_entry = None
    active_fld_list = []
    sz_str = ""

    with open(filename, "rb") as csv_file:
        csv_reader = csv.DictReader(csv_file)
        row_num = 0
        for row in csv_reader:
            array_size = str_to_array_size(row["Array"])
            if row["Type"] in ["endgroup"]:
                active_map = nested_map.pop()
                #print "POP : " + active_map.type_name

            elif row["Type"] in csr_map_types:
                addr_maps[ row["Type Name"] ] = a_map( row["Identifier"], row["Offset"], array_size, "map", 0, row["Type Name"] )
                active_map = addr_maps[ row["Type Name"] ]

            elif row["Type"] in csr_group_types:
                if active_map != None:
                    type_name = active_map.type_name + "__" + row["Type Name"] + "__" + row["Identifier"]
                else:
                    type_name = row["Type Name"] + "__" + row["Identifier"]
                addr_maps[ type_name ] = a_map( row["Identifier"], row["Offset"], array_size, "group", 0, type_name )
                active_map.add_map_entry( row["Identifier"], row["Offset"], array_size, "group", 0, type_name )
                if active_map != None:
                    nested_map.append( active_map )
                    #print "PUSH: " + active_map.type_name

                active_map = addr_maps[ type_name ]

            elif row["Type"] == "addressmap instance":
                if row["Stride"] == "":
                    stride = 0
                else:
                    stride = int(row["Stride"].replace(" bytes",""),0)
                active_map.add_map_entry( row["Identifier"], row["Offset"], array_size, "map", stride, row["Type Name"] )
                
            elif row["Type"] in csr_register_types:
                reg_width = (int(row["Register Size"].replace(" bits",""),0))/8
                active_map.add_map_entry( row["Identifier"], row["Offset"], array_size, "reg", reg_width, "" )
               
    csv_file.close()
    return addr_maps


def csr_compiler_dump_db( addr_maps ):

    for map in addr_maps:
        this_map = addr_maps[ map ]
        sz_str = array_sz_to_str( this_map.array_sz )
        print this_map.type_name + sz_str + " : " + this_map.obj_type

        for entry in this_map.regs_and_maps:
            sz = entry['arr_sz']
            sz_str = array_sz_to_str( sz )
            if entry['obj_type'] == "reg":
                print "    " + entry["offset"] + " : " + entry['name'] + str(sz_str) + " : " + entry['obj_type'] + " : " + entry['width']
            else:
                print "    " + entry["offset"] + " : " + entry['name'] + str(sz_str) + " : " + entry['obj_type'] + " : <" + entry['type_name'] + ">"
        print this_map.type_name + "    : END"
        print " "           



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

            elif row["Type"] in csr_register_types:
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
                                       + str(msb) + ", " + str(lsb) + ", 0,"
                                       + str(0) + ", " )
                        print ( "      " + "\"" + row["Description"].replace('\n','').replace('\r','') + "\"" + " }," )
                    else:
                        field_width = msb - lsb + 1
                        for idx in range(0,array_size[0]):
                            #print "    [" + str(msb) + ":" + str(lsb) + "] : " + row["Identifier"] + "[" + str(idx) + "]"

                            print ( "    { \"" + row["Identifier"] + "[" + str(idx) + "]" + "\", " 
                                           + str(msb) + ", " + str(lsb) + ", 0,"
                                           + str(0) + ", " )
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

    csv_file.close()

already_created_maps = {}


def get_map_width( addr_maps, map_name, arr_sz ):
    width = 0
    #print "// OBJ GET_WIDTH: " + map_name + " arr_sz: " + str(arr_sz)
    for map in addr_maps:
        this_map = addr_maps[ map ]
        if this_map.type_name == map_name:
            for sub_map in this_map.regs_and_maps:
                if sub_map['obj_type'] in ( "map", "group" ):
                    this_width = get_map_width( addr_maps, sub_map['type_name'], sub_map['arr_sz'] )
                elif sub_map['obj_type'] in ( "reg" ):
                    this_width = sub_map['width']
                    this_offset = int(sub_map['offset'], 16)
                    if width < this_offset:
                        width = this_offset
                    if (width & (this_width - 1)) != 0:
                        # must pad for natural alignment
                        width = (width + (this_width - 1)) & ~(this_width - 1)
                width = width + this_width
                #print "//    SUB-OBJ: sz= " + hex(this_width) + " : " + sub_map['name']

    # arrays are packed, so cant round at this level
    if arr_sz == (1,):
        # Round stride up to the next largest power of 2
        stride = 1
        while stride < width:
            stride *= 2
        #print "//    Rounded " + hex(width) + " to " + hex(stride)
    else:
        stride = width

    #print "// OBJ TOTAL sz= " + hex(stride)  + " : " + map_name
    return stride
          
def parse_addr_map( addr_maps, map_name ):
    for map in addr_maps:
        this_map = addr_maps[ map ]
        if this_map.type_name == map_name:
            for sub_map in this_map.regs_and_maps:
                if sub_map['obj_type'] in ( "map", "group" ): 
                    if sub_map['type_name'] not in already_created_maps:
                        already_created_maps[ sub_map['type_name'] ] = 'done'
                        parse_addr_map( addr_maps, sub_map['type_name'] )

            sz_str = array_sz_to_str( this_map.array_sz )
            #
            print "cmd_arg_item_t " + "tof2_" + this_map.type_name + "_list[] = {"
            
            n_entry = 0
            for entry in this_map.regs_and_maps:
                stride = 0
                if entry['obj_type'] == "reg":
                    stride = int(entry['width'])
                elif entry['obj_type'] in ( "map", "group"):
                    stride = int(entry['width'])
                    if stride == 0:
                        stride = get_map_width( addr_maps, entry['type_name'], entry['arr_sz'] )
                #print "// obj: sz = " + hex(stride) + " : " + entry['name'] + ", type : " + entry['type_name'] + ", obj_type : " + entry['obj_type'] + ", width : ", + entry['width']

                # array is expanded below, so stride should be stride of a single entry
                # stride = (stride * product(entry['arr_sz']))

                if entry['obj_type'] == "reg":
                    type_name_to_pass = this_map.type_name
                else:
                    type_name_to_pass = entry['type_name']

                n_entry = n_entry + output_list_lines( entry["name"], "tof2_" + type_name_to_pass, entry["offset"], entry['arr_sz'], stride, entry['obj_type'] )
                #if entry['obj_type'] in ( "map", "group" ):
                #    print "{ \"" + entry["name"] + "\", &" + entry['type_name'] + ", " + entry["offset"] + ", NULL }," 
                #    #print "    " + entry["offset"] + " : " + entry['name'] + str(sz_str) + " : " + entry['obj_type'] + " : <" + entry['type_name'] + ">"
                #else:
                #    print "{ \"" + entry["name"] + "\", NULL, " + entry["offset"] + ", &" + this_map.type_name + "__" + entry["name"] + " }," 
                #    #print "    " + entry["offset"] + " : " + entry['name'] + str(sz_str) + " : " + entry['obj_type'] + " : " + entry['width']
                #n_entry = n_entry + 1
            #print this_map.type_name + "    : END"
            print "};"
            print ""
            print "cmd_arg_t " + "tof2_" + this_map.type_name + " = { " + str(n_entry) + ", " + "tof2_" + this_map.type_name + "_list };"
            print ""


def output_list_lines( name, type_name, offset_str, array_sz, stride, obj_type ):
    n_entry = 0
    i = 0
    j = 0
    k = 0
    l = 0
    base_offset = int(offset_str,base=16)

    if array_sz == (1,):
        offset = base_offset
        if obj_type in ( "map", "group" ):
            # special-case for the Comira register map
            if name == "comira_regs":
                print "{ \"" + "comira_regs" + "\", &" + "comira_list" + ", " + hex(offset) + ", NULL }," 
            else:
                print "{ \"" + name + "\", &" + type_name + ", " + hex(offset) + ", NULL }," 
        else:
            print "{ \"" + name + "\", NULL, " + hex(offset) + ", &" + type_name + "__" + name + " }," 
        n_entry = n_entry + 1

    elif len(array_sz) == 1:
        for i in range(0, array_sz[0]):
            offset = base_offset + (i*stride)
            if obj_type in ( "map", "group" ):
                print "{ \"" + name + "[" + str(i) + "]" + "\", &" + type_name + ", " + hex(offset) + ", NULL },"
            else:
                print "{ \"" + name + "[" + str(i) + "]" + "\", NULL, " + hex(offset) + ", &" + type_name + "__" + name + " },"
            n_entry = n_entry + 1

    elif len(array_sz) == 2:
        for i in range(0, array_sz[0]):
            for j in range(0,array_sz[1]):
                offset = base_offset + (((i*array_sz[1]) + j)*stride)
                if obj_type in ( "map", "group" ):
                    print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "\", &" + type_name + ", " + hex(offset) + ", NULL },"
                else:
                    print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "\", NULL, " + hex(offset) + ", &" + type_name + "__" + name + " },"
                n_entry = n_entry + 1

    elif len(array_sz) == 3:
        for i in range(0, array_sz[0]):
            for j in range(0,array_sz[1]):
                for k in range(0,array_sz[2]):
                    offset = base_offset + (((i*array_sz[1]*array_sz[2]) + (j*array_sz[2]) + k)*stride)
                    if obj_type in ( "map", "group" ):
                        print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "[" + str(k) + "]" + "\", &" + type_name + ", " + hex(offset) + ", NULL },"
                    else:
                        print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "[" + str(k) + "]" + "\", NULL, " + hex(offset) + ", &" + type_name + "__" + name + " },"
                    n_entry = n_entry + 1


    elif len(array_sz) == 4:
        for i in range(0, array_sz[0]):
            for j in range(0,array_sz[1]):
                for k in range(0,array_sz[2]):
                    for l in range(0,array_sz[3]):
                        offset = base_offset + (((i*array_sz[1]*array_sz[2]*array_sz[3]) + (j*array_sz[2]*array_sz[3]) + (k*array_sz[3]) + l)*stride)
                        if obj_type in ( "map", "group" ):
                            print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "[" + str(k) + "]" + "[" + str(l) + "]" + "\", &" + type_name + ", " + hex(offset) + ", NULL }," 
                        else:
                            print "{ \"" + name + "[" + str(i) + "]" + "[" + str(j) + "]" + "[" + str(k) + "]" + "[" + str(l) + "]" + "\", NULL, " + hex(offset) + ", &" + type_name + "__" + name + " }," 
                        n_entry = n_entry + 1

    return n_entry

# csr_compiler_pass2
#
# Pass2 builds the auto-generated header file representing the register hierarchy
#
def csr_compiler_pass2( addr_maps ):
    parse_addr_map( addr_maps, "jbay_reg" )


def build_reg_info(dir):

    addr_maps = {}

    full_csr_file = os.path.join(dir,"jbay_reg.csv")
    csr_compiler_pass0( full_csr_file )
    addr_maps = csr_compiler_pass1( full_csr_file )
    #csr_compiler_dump_db( addr_maps )
    csr_compiler_pass2( addr_maps )


# Unit tests
if __name__ == "__main__":

    build_reg_info( str(sys.argv[1]) )
