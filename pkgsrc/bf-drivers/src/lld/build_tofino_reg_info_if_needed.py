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
import subprocess
import shutil

from operator import mul
from types import StringTypes

# Unit tests
if __name__ == "__main__":

    csv_path = os.path.join(str(sys.argv[1]),"csv/")
    csv_file = os.path.join(csv_path,"tofino.csv")
    md5_path = "tofino.csv" + ".md5"
    # make sure the md5 files exist
    subprocess.call(["touch", md5_path])
    subprocess.call(["touch", "tofino.csv.current.md5"])
    with open('tofino.csv.current.md5', "w") as outfile:
        my_cmd = ["md5sum", csv_file]
        subprocess.call(my_cmd, stdout=outfile)
        outfile.close()
    retcode = subprocess.call(["diff", "tofino.csv.current.md5", md5_path])
    if retcode == 0:
        #
        # Check for changes to the python build script
        #
        subprocess.call(["touch", "build_tofino_reg_info.py.current.md5"])
        with open('build_tofino_reg_info.py.current.md5', "w") as outfile:
            my_cmd = ["md5sum", "build_tofino_reg_info.py"]
            subprocess.call(my_cmd, stdout=outfile)
            outfile.close()
        retcode = subprocess.call(["diff", "build_tofino_reg_info.py.current.md5", "build_tofino_reg_info.py.md5"])
        if retcode == 0:
            subprocess.call(["rm", "tofino.csv.current.md5"])
            subprocess.call(["rm", "build_tofino_reg_info.py.current.md5"])
            quit()

        subprocess.call(["mv", "build_tofino_reg_info.py.current.md5", "build_tofino_reg_info.py.md5"])
    subprocess.call(["mv", "tofino.csv.current.md5", md5_path])

    with open('copyright.txt', 'r') as copyright_file:
	file_contents = copyright_file.read()

    with open('reg_list_autogen.h', "w") as outfile2:
	outfile2.write(file_contents)
        outfile2.write("/* clang-format off */\n")
        outfile2.flush()
        subprocess.call(["python", "build_tofino_reg_info.py", csv_path], stdout=outfile2 )
        outfile2.write("/* clang-format on */\n")
        outfile2.flush()
        outfile2.close()

    with open('mem_list_autogen.h', "w") as outfile3:
	outfile3.write(file_contents)
        outfile3.write("/* clang-format off */\n")
        outfile3.flush()
        subprocess.call(["python", "build_tofino_mem_info.py", csv_path], stdout=outfile3 )
        outfile3.write("/* clang-format on */\n")
        outfile3.flush()
        outfile3.close()


    h_file = str(sys.argv[1]) + "inc/tofino_regs/" + "tofino.h"

    with open('../../include/lld/tofino_defs.h', "w") as outfile5:
	outfile5.write(file_contents)
        outfile5.write("/* clang-format off */\n")
        outfile5.flush()
        p1 = subprocess.Popen(["grep", "static const size_t", h_file], stdout=subprocess.PIPE)
        p2 = subprocess.Popen(["sed", "-e", "s/static const size_t/\#define/"], stdin=p1.stdout, stdout=subprocess.PIPE)
        p3 = subprocess.Popen(["sed", "-e", "s/=//"], stdin=p2.stdout, stdout=subprocess.PIPE)
        p4 = subprocess.Popen(["sed", "-e", "s/;//"], stdin=p3.stdout, stdout=subprocess.PIPE)
        p5 = subprocess.Popen(["sed", "-e", "s/tofino_/DEF_tofino_/"], stdin=p4.stdout, stdout=outfile5)
        p5.wait()
        outfile5.write("/* clang-format on */\n")
        outfile5.flush()
        outfile5.close()

    pipe_top_level_file = str(sys.argv[1]) + "inc/tofino_regs/" + "pipe_top_level.h"
    with open('../../include/tofino_regs/pipe_top_level.h', "w") as outfile6:
	outfile6.write(file_contents)
        outfile6.write("/* clang-format off */\n")
        outfile6.flush()
        p6 = subprocess.Popen(["sed", "-e", "/TYPE DEFINITIONS/,/#endif/{//!d}", pipe_top_level_file], stdout=subprocess.PIPE)
        p7 = subprocess.Popen(["sed", "-e", "s/const size_t/const uint64_t/"], stdin=p6.stdout, stdout=outfile6)
        p7.wait()
        outfile6.write("/* clang-format on */\n")
        outfile6.flush()
        outfile6.close()

    with open('../../include/tofino_regs/tofino.h', "w") as outfile7:
	outfile7.write(file_contents)
        outfile7.write("/* clang-format off */\n")
        outfile7.flush()
        with open(h_file) as infile:
            shutil.copyfileobj(infile, outfile7)
        outfile7.write("/* clang-format on */\n")
        outfile7.flush()
        outfile7.close()
