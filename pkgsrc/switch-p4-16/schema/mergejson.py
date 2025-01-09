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

#!/usr/bin/env python3

from __future__ import print_function
import io
import os
import sys
import json
import argparse
import six
from collections import OrderedDict

top_json = "top.json"

user_json_dir = "appObj"
user_json_list = ["acl.json",\
                  "bridge.json",\
                  "buffer.json",\
                  "ecmp.json",\
                  "hostif.json",\
                  "hash.json",\
                  "bridge_port.json",\
                  "isolation_group.json", \
                  "mac.json",\
                  "meter.json",\
                  "mirror.json",\
                  "multicast.json",\
                  "my_mac.json",\
                  "neighbor.json",\
                  "nexthop.json",\
                  "port.json",\
                  "qos.json",\
                  "queue.json",\
                  "rif.json",\
                  "rmac.json",\
                  "route.json",\
                  "rpf.json",\
                  "scheduler.json",\
                  "stp.json",\
                  "tunnel.json",\
                  "vlan.json",\
                  "vrf.json",\
                  "lag.json",\
                  "wred.json",\
                  "dtel.json",\
                  "sflow.json", \
                  "pfc_wd.json", \
                  "etrap.json", \
                  "nat.json", \
                  "debugcounter.json", \
                  "mpls.json", \
                  "udf.json", \
                  "bfd.json", \
                  "counter.json", \
                  "device.json"]

pd_json_dir = "asicObj"
pd_json_list = ["acl.json",\
                  "bd.json",\
                  "buffer.json",\
                  "device.json",\
                  "ecmp.json",\
                  "fib.json",\
                  "hostif.json",\
                  "lag.json",\
                  "mac.json",\
                  "meter.json",\
                  "mirror.json",\
                  "multicast.json",\
                  "nexthop.json",\
                  "packet.json",\
                  "port.json",\
                  "ppg.json",\
                  "qos.json",\
                  "rewrite.json",\
                  "stp.json",\
                  "scheduler.json",\
                  "wred.json",\
                  "dtel.json",\
                  "sflow.json", \
                  "queue.json", \
                  "pfc_wd.json", \
                  "hash.json", \
                  "etrap.json", \
                  "nat.json", \
                  "rmac.json", \
                  "tunnel.json", \
                  "bfd.json", \
                  "mpls.json"]

def parseTopObject(schema_dir, top_json, obj_dict):
  fname=schema_dir+"/"+top_json
  merge_dict = {}
  with open(fname, "rb") as infile:
      json_data = infile.read()
      json_objects = json.loads(json_data.decode('utf-8'), object_pairs_hook=OrderedDict)

  for obj in json_objects:
    for json_obj_key, json_obj_value in six.iteritems(obj):
      merge_dict[json_obj_key] = json_obj_value
    merge_dict["objects"] = obj_dict
  return merge_dict

def appendObjInDict(json_dict, file_name):
  with open(file_name, "rb") as infile:
      json_data = infile.read()
      json_objects = json.loads(json_data.decode('utf-8'), object_pairs_hook=OrderedDict)

  for obj in json_objects:
    for obj_key, obj_value in six.iteritems(obj):
      json_dict[obj_key] = obj_value

def jsonObjectList(schema_dir, user_json_dir, user_file_list, pd_json_dir, pd_file_list):
  json_dict = OrderedDict()
  #User json objects
  for f in user_file_list:
    fname=schema_dir+"/"+user_json_dir+"/"+f
    #print("fname: {}".format(fname))
    appendObjInDict(json_dict, fname)

  #PD json objects
  for f in pd_file_list:
    fname=schema_dir+"/"+pd_json_dir+"/"+f
    appendObjInDict(json_dict, fname)

  #print("Total json objects in dict %d"%(len(json_dict)))
  return json_dict

if __name__ == "__main__":
  schema_dir = sys.argv[1]
  output_file = sys.argv[2]
  objects_dict = jsonObjectList(schema_dir, user_json_dir, user_json_list, pd_json_dir, pd_json_list)

  merge_objects_dict = parseTopObject(schema_dir, top_json, objects_dict)
  if sys.version_info[0] >= 3:
    unicode=str

  with io.open(output_file, "w", encoding="utf8") as outfile:
    outfile.write(unicode(json.dumps(merge_objects_dict, indent=4, ensure_ascii=False)))
  outfile.close()
