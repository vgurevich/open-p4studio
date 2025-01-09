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

import datetime
import sys

debug_mode = False
debug_trace_enable = False
debug_res_ids = set([])

#debug_mode = True
#debug_trace_enable = True 
#debug_res_ids = set( [('Match', 23157), ('Stateful', 26111), ('Stateful', 52105)] )

def log(s):
    print(datetime.datetime.now(), ' ', s)
    sys.stdout.flush()

def debug_fn(s):
    if debug_trace_enable == True:
        log(s)
