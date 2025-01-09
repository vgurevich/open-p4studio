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

import os
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
  ("tofino",   "x4"),
  ("tofino",   "x5"),
  ("tofino",   "x6"),
  ("tofino",   "x7"),
  ("tofino2",  "y0"),
  ("tofino2",  "y1"),
  ("tofino2",  "y2"),
  ("tofino2",  "y4"),
  ("tofino2",  "y7"),
  ("tofino2",  "y8"),
]
tlist = []

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
  print (p4cStr)
  t = compile_p4(p4cStr)
  t.name = profile_name
  t.start()
  tlist.append(t)

time.sleep(5)
for t in tlist:
  t.join()
  print (t.name + " done")

