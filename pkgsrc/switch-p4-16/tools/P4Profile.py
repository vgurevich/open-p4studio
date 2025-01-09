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

?import os
import time
import re
import copy
import json
import csv
from collections import OrderedDict
from threading import Thread

profile_list = [
  ("tofino",   "x1"),
  ("tofino",   "x2"),
  ("tofino",   "x6"),
  ("tofino2",  "y1"),
  ("tofino2",  "y2"),
]
tlist = []
#profile_list = [
#  ("tofino2",   "z2"),
#]

p4factory_ws = os.environ.get('WORKSPACE')
p4CompileCmd = "/install/bin/p4c"
pythonPath = "install/lib/python2.7/site-packages/"
context_json="context.json"
power_json="/logs/mau.power.json"
metrics_json="/logs/metrics.json"

def compile_p4(p4cStr):
  print (p4cStr)
  return Thread(target=os.system, args=(p4cStr,))

cwd = os.getcwd()
profile_names = []
for profile in profile_list:
  profile_name = profile[0] +"_"+ profile[1]
  profile_names.append(profile_name)

for profile in profile_list:
  chip = "tofino" if profile[0]=="tofino" else "tofino2"
  name  = profile[1]
  profile_name = profile[0] +"_"+ profile[1]
  if chip == "tofino":
    arch = "tna"
  else:
    arch = "t2na"
  target = profile[0]
  if name=="t0" or name=="t1":
    P4Path="/submodules/bf-switch/p4src/switch-"+ chip +"/switch_"+ chip +".p4"
  else:
    P4Path="/submodules/bf-switch/p4src/switch-"+ chip +"/switch_"+ chip +"_"+ name +".p4"

  p4File=p4factory_ws+P4Path
  p4Shared=p4factory_ws+"/submodules/bf-switch/p4src/shared"
  outDir = "./"+ profile_name
  p4cArgs = "--arch "+ arch +" --target "+ target +" --std p4-16 -g  --display-power-budget"
  p4cOpt = "-Xp4c='--disable-power-check --disable-parse-depth-limit'"
  if profile_name == "tofino_m1":
      p4cOpt = "-Xp4c='--set-max-power 51.05'"
  if profile_name == "tofino2_z2" or profile_name == "tofino2_y3":
      p4cArgs += " --num-stages-override 16"
  cmd="touch "+p4File
  print (cmd)
  p4cStr = p4factory_ws + p4CompileCmd +" -o "+ outDir +" "+ p4cOpt +" "+ p4cArgs +" -I"+ p4Shared +" "+ p4File +" > "+ profile_name +".log 2>&1"
#  print (p4cStr)
  t = compile_p4(p4cStr)
  t.name = profile_name
  t.start()
  tlist.append(t)

time.sleep(5)
for t in tlist:
  t.join()
  print (t.name + " done")

p4_profile_table = {}
p4_profile_stages = {}
p4_profile_power = {}
p4_profile_metrics = OrderedDict()
p4_table_list = []
global_table_list = []
imp_table_list = ["dmac",
                  "ingress_stp.stp0",
                  "egress_stp.stp",
                  "vlan_membership",
                  "not_implemented.SwitchEgress.egress_port_mapping.vlan_membership",
                  "storm_control.storm_control",
                  "ipv4_host",
                  "ipv4_lpm",
                  "ipv6_host",
                  "ipv6_lpm_tcam",
                  "ipv6_lpm128",
                  "ipv6_lpm64",
                  "nexthop",
                  "ecmp_table",
                  "nexthop_ecmp_action_profile",
                  "multicast_route_ipv4_star_g",
                  "multicast_route_ipv4_s_g",
                  "multicast_route_ipv6_star_g",
                  "multicast_route_ipv6_s_g",
                  "rid",
                  "multicast_bridge_ipv4_star_g",
                  "multicast_bridge_ipv4_s_g",
                  "multicast_bridge_ipv6_star_g",
                  "multicast_bridge_ipv6_s_g",
                  "SwitchIngress.ingress_mac_acl.acl",
                  "SwitchIngress.ingress_ipv4_acl.acl",
                  "SwitchIngress.ingress_ipv6_acl.acl",
                  "SwitchIngress.ingress_ip_acl.acl",
                  "SwitchIngress.ingress_ipv4_racl.acl",
                  "SwitchIngress.ingress_ipv6_racl.acl",
                  "SwitchIngress.ingress_ip_racl.acl",
                  "SwitchIngress.ingress_mac_qos_acl.acl",
                  "SwitchIngress.ingress_ipv4_qos_acl.acl",
                  "SwitchIngress.ingress_ipv6_qos_acl.acl",
                  "SwitchIngress.ingress_ip_qos_acl.acl",
                  "SwitchIngress.ingress_mac_mirror_acl.acl",
                  "SwitchIngress.ingress_ipv4_mirror_acl.acl",
                  "SwitchIngress.ingress_ipv6_mirror_acl.acl",
                  "SwitchIngress.ingress_ip_mirror_acl.acl",
                  "SwitchIngress.ingress_ip_dtel_acl.acl",
                  "ingress_system_acl",
                  "SwitchEgress.egress_mac_acl.acl",
                  "SwitchEgress.egress_ipv4_acl.acl",
                  "SwitchEgress.egress_ipv6_acl.acl",
                  "SwitchEgress.egress_ip_acl.acl",
                  "SwitchEgress.egress_mac_mirror_acl.acl",
                  "SwitchEgress.egress_ipv4_mirror_acl.acl",
                  "SwitchEgress.egress_ipv6_mirror_acl.acl",
                  "SwitchEgress.egress_ip_mirror_acl.acl",
                  "egress_system_acl",
                  "total_5_tuple_acl",
                  "ingress_port_mirror",
                  "egress_port_mirror",
                  "ingress_mirror_meter.meter_index",
                  "egress_mirror_meter.meter_index",
                  "ingress_qos_map.dscp_tc_map",
                  "egress_qos.l3_qos_map",
                  "ingress_port_meter.meter_index",
                  "egress_port_meter.meter_index",
                  "ingress_acl_meter.meter_index",
                  "egress_acl_meter.meter_index",
                  "egress_wred.wred",
                  "ecn_acl.acl",
                  "ingress_pfcwd.acl",
                  "bd_to_vni_mapping",
                  "vrf_to_vni_mapping",
                  "ip_in_ip_todo",
                  "dst_addr_rewrite",
                  "dst_addr_rewrite",
                  "vni_to_bd_mapping",
                  "rid",
                  "multicast_underlay_todo",
                  "flow_report.array1",
                  "queue_report.queue_alert",
                  "drop_report.array1",
                  "qinq_todo",
                  "ptp_todo",
                  "ingress_sflow_samplers",
                  "dest_nat",
                  "dest_napt",
                  "mpls_fib",
                  "tunnel_nexthop",
                  "my_sid",
                  "sid_rewrite",
]

row = []
for profile in profile_list:
  chip = "tofino" if profile[0]=="tofino" else "tofino2"
  profile_name = profile[0] +"_"+ profile[1]
  csvFileName=profile_name+".csv"
  csvfd = open(csvFileName, 'w')
  w = csv.writer(csvfd,["Table name", "Table size"])

  # Number of Stages
  resourcejson=cwd+"/"+profile_name+"/pipe/logs/resources.json"
#  print resourcejson
  json_data = json.load(open(resourcejson))
  resources = json_data["resources"]
#  pipes = resources["pipes"]
#  pipe0 = pipes[0]
#  mau = pipe0["mau"]
  mau = resources["mau"]
  nStages = len(mau["mau_stages"])
  p4_profile_stages[profile_name] = nStages

  # Total Power
  powerjson=cwd+"/"+profile_name+"/pipe/logs/power.json"
#  print powerjson
  json_data = json.load(open(powerjson))
  total_power = json_data["total_power"]
  ingress = total_power[0]
  ingress_power = ingress["power"]
  egress = total_power[1]
  egress_power = egress["power"]
  p4_profile_power[profile_name] = ingress_power + egress_power

  # Metrics
  metricjson=cwd+"/"+profile_name+"/pipe/logs/metrics.json"
#  print metricjson
  json_data = json.load(open(metricjson))
  mau = json_data["mau"]
  phv = json_data["phv"]
  p4_profile_metrics[(profile_name,"srams")] = mau["srams"]
  p4_profile_metrics[(profile_name,"tcams")] = mau["tcams"]
  p4_profile_metrics[(profile_name, "ingress_power")] = mau["power"][0]["estimate"]
  p4_profile_metrics[(profile_name, "egress_power")] = mau["power"][1]["estimate"]
  p4_profile_metrics[(profile_name, "ingress_latency")] = mau["latency"][0]["cycles"]
  p4_profile_metrics[(profile_name, "egress_latency")] = mau["latency"][1]["cycles"]
  p4_profile_metrics[(profile_name, "normal_8_bit")] = phv["normal"][0]["containers_occupied"]
  p4_profile_metrics[(profile_name, "normal_16_bit")] = phv["normal"][1]["containers_occupied"]
  p4_profile_metrics[(profile_name, "normal_32_bit")] = phv["normal"][2]["containers_occupied"]
  if (chip == "tofino"):
    p4_profile_metrics[(profile_name, "tagalong_8_bit")] = phv["tagalong"][0]["containers_occupied"]
    p4_profile_metrics[(profile_name, "tagalong_16_bit")] = phv["tagalong"][1]["containers_occupied"]
    p4_profile_metrics[(profile_name, "tagalong_32_bit")] = phv["tagalong"][2]["containers_occupied"]
    p4_profile_metrics[(profile_name, "mocha_8_bit")] = 0
    p4_profile_metrics[(profile_name, "mocha_16_bit")] = 0
    p4_profile_metrics[(profile_name, "mocha_32_bit")] = 0
    p4_profile_metrics[(profile_name, "dark_8_bit")] = 0
    p4_profile_metrics[(profile_name, "dark_16_bit")] = 0
    p4_profile_metrics[(profile_name, "dark_32_bit")] = 0
  if (chip == "tofino2"):
    p4_profile_metrics[(profile_name, "tagalong_8_bit")] = 0
    p4_profile_metrics[(profile_name, "tagalong_16_bit")] = 0
    p4_profile_metrics[(profile_name, "tagalong_32_bit")] = 0
    p4_profile_metrics[(profile_name, "mocha_8_bit")] = phv["mocha"][0]["containers_occupied"]
    p4_profile_metrics[(profile_name, "mocha_16_bit")] = phv["mocha"][1]["containers_occupied"]
    p4_profile_metrics[(profile_name, "mocha_32_bit")] = phv["mocha"][2]["containers_occupied"]
    p4_profile_metrics[(profile_name, "dark_8_bit")] = phv["dark"][0]["containers_occupied"]
    p4_profile_metrics[(profile_name, "dark_16_bit")] = phv["dark"][1]["containers_occupied"]
    p4_profile_metrics[(profile_name, "dark_32_bit")] = phv["dark"][2]["containers_occupied"]

  # Table Sizes
  contextjson=cwd+"/"+profile_name+"/pipe/"+context_json
#  print contextjson
  json_data = json.load(open(contextjson))
  p4_tables = json_data["tables"]
  for table in p4_tables:
    size = table["size"]
    if size == 0 or ("action__" in table["name"] and "ecmp_group__action__" not in table["name"]):
      continue
    if table["name"] not in global_table_list:
      global_table_list.append(table["name"])
    row = []
    row.append(table["name"])
    row.append(table["size"])
    w.writerow(row)
    p4_profile_table[(profile_name,table["name"])] = table["size"]
  csvfd.close()
#print p4_profile_stages
#print p4_profile_power
#print p4_profile_table
#print global_table_list
#print p4_profile_metrics

globalcsv = "P4Table.csv"
fd = open(globalcsv, 'w')
w = csv.writer(fd,["Table name"])
title = ["Table Name"]
for profile in profile_list:
  chip = "tofino" if profile[0]=="tofino" else "tofino2"
  profile_name = profile[0] +"_"+ profile[1]
  title.append(profile_name)
w.writerow(title)

for table in global_table_list:
  sizelist = []
  sizelist.append(table)
  for profile in profile_list:
    chip = "tofino" if profile[0]=="tofino" else "tofino2"
    profile_name = profile[0] +"_"+ profile[1]
    if (profile_name,table) not in p4_profile_table:
#      print "Table %s not in profile_name %s"%(table, profile_name)
      size = 0
    else:
      size = p4_profile_table[(profile_name,table)]
    sizelist.append(size)
  w.writerow(sizelist)
fd.close()

globalcsv2 = "P4Table2.csv"
fd = open(globalcsv2, 'w')
w = csv.writer(fd,["Table name"])
title = ["Table Name"]
for profile in profile_list:
  profile_name = profile[0] +"_"+ profile[1]
  title.append(profile_name)
w.writerow(title)

# Number of Stages
nStages = ["Number of Stages"]
for profile_name in profile_names:
  nStages.append(p4_profile_stages[profile_name])
w.writerow(nStages)

# Power
power = ["Total Power"]
for profile_name in profile_names:
  power.append(p4_profile_power[profile_name])
w.writerow(power)

attributes = ["ingress_latency", "egress_latency", "srams", "tcams",
              "normal_8_bit", "normal_16_bit", "normal_32_bit",
              "tagalong_8_bit", "tagalong_16_bit", "tagalong_32_bit",
              "mocha_8_bit", "mocha_16_bit", "mocha_32_bit",
              "dark_8_bit", "dark_16_bit", "dark_32_bit"]
for attribute in attributes:
#  print attribute
  values = []
  values.append(attribute)
#  print values
  for profile_name in profile_names:
    values.append(p4_profile_metrics[(profile_name, attribute)])
  w.writerow(values)

# SRAMs
#srams = ["Total SRAMs"]
#for profile_name in profile_names:
#  srams.append(p4_profile_metrics[(profile_name, "srams")])
#w.writerow(srams)
#
## TCAMs
#tcams = ["Total TCAMs"]
#for profile_name in profile_names:
#  tcams.append(p4_profile_metrics[(profile_name, "tcams")])
#w.writerow(tcams)

#for table in imp_table_list:
blank = []
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)
w.writerow(blank)

#Table Sizes
for table in imp_table_list:
  sizelist = []
  sizelist.append(table)
  for profile in profile_list:
    profile_name = profile[0] +"_"+ profile[1]
    if (profile_name,table) not in p4_profile_table:
#      print "Table %s not in profile %s"%(table, profile_name)
      size = 0
    else:
      size = p4_profile_table[(profile_name,table)]
    sizelist.append(size)
  w.writerow(sizelist)
fd.close()
