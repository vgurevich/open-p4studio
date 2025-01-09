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

    reg_version = str(sys.argv[4])
    csv_path = os.path.join(str(sys.argv[1]),"csv/")
    csv_file = os.path.join(csv_path,"cb_reg.csv")
    md5_path = "tof3_reg.csv" + ".md5"
    # make sure the md5 files exist
    subprocess.call(["touch", md5_path])
    subprocess.call(["touch", "tof3_reg.csv.current.md5"])
    with open('tof3_reg.csv.current.md5', "w") as outfile:
        my_cmd = ["md5sum", csv_file]
        subprocess.call(my_cmd, stdout=outfile)
        outfile.close()
    retcode = subprocess.call(["diff", "tof3_reg.csv.current.md5", md5_path])
    if retcode == 0:
        #
        # Check for changes to the python build script
        #
        subprocess.call(["touch", "build_tof3_reg_info.py.current.md5"])
        with open('build_tof3_reg_info.py.current.md5', "w") as outfile:
            my_cmd = ["md5sum", "build_tof3_reg_info.py"]
            subprocess.call(my_cmd, stdout=outfile)
            outfile.close()
        retcode = subprocess.call(["diff", "build_tof3_reg_info.py.current.md5", "build_tof3_reg_info.py.md5"])
        if retcode == 0:
            subprocess.call(["rm", "tof3_reg.csv.current.md5"])
            subprocess.call(["rm", "build_tof3_reg_info.py.current.md5"])
            quit()

        subprocess.call(["mv", "build_tof3_reg_info.py.current.md5", "build_tof3_reg_info.py.md5"])
    subprocess.call(["mv", "tof3_reg.csv.current.md5", md5_path])

    mem_header = sys.argv[2]
    mem_output = sys.argv[3]
    tmp_file = sys.argv[3] + ".tmp"
    subprocess.call(["python", "mem_parse.py", mem_header, tmp_file])

    with open('copyright.txt', 'r') as copyright_file:
        copyright_header = copyright_file.read()
        copyright_file.close()

    with open('tof3_reg_list_autogen.h', "w") as outfile2:
        outfile2.write(copyright_header)
        outfile2.write("/* clang-format off */\n")
        outfile2.flush()
        subprocess.call(["python", "build_tof3_reg_info.py", csv_path], stdout=outfile2 )
        outfile2.write("/* clang-format on */\n")
        outfile2.flush()
        outfile2.close()

    with open('tof3_mem_list_autogen.h', "w") as outfile3:
        outfile3.write(copyright_header)
        outfile3.write("/* clang-format off */\n")
        outfile3.flush()
        subprocess.call(["python", "build_tof3_mem_info.py", csv_path], stdout=outfile3 )
        outfile3.write("/* clang-format on */\n")
        outfile3.flush()
        outfile3.close()


    h_file = str(sys.argv[1]) + "inc/cb_regs/" + "cb_reg_drv.h"

    with open('../../include/lld/tof3_reg_drv_defs.h', "w") as outfile5:
        outfile5.write(copyright_header)
        outfile5.write("/* clang-format off */\n")
        outfile5.flush()
        p1 = subprocess.Popen(["grep", "static const size_t", h_file], stdout=subprocess.PIPE)
        p2 = subprocess.Popen(["grep", "-v", "_byte_address"], stdin=p1.stdout, stdout=subprocess.PIPE)
        p3 = subprocess.Popen(["sed", "-e", "s/static const size_t/\#define/"], stdin=p2.stdout, stdout=subprocess.PIPE)
        p4 = subprocess.Popen(["sed", "-e", "s/=//"], stdin=p3.stdout, stdout=subprocess.PIPE)
        p5 = subprocess.Popen(["sed", "-e", "s/;//"], stdin=p4.stdout, stdout=subprocess.PIPE)
        p6 = subprocess.Popen(["sed", "-e", "s/cb_/DEF_tof3_/"], stdin=p5.stdout, stdout=outfile5)
        p6.wait()
        outfile5.write("/* clang-format on */\n")
        outfile5.flush()
        outfile5.close()

    pipe_top_level_file = str(sys.argv[1]) + "inc/cb_regs/" + "cb_mem_drv.h"
    with open('../../include/tof3_regs/tof3_mem_drv.h', "w") as outfile6:
        outfile6.write(copyright_header)
        outfile6.write("/* clang-format off */\n")
        outfile6.flush()
        p7 = subprocess.Popen(["sed", "-e", "/TYPE DEFINITIONS/,/#endif/{//!d}", pipe_top_level_file], stdout=subprocess.PIPE)
        p8 = subprocess.Popen(["sed", "-e", "s/const size_t/const uint64_t/"], stdin=p7.stdout, stdout=subprocess.PIPE)
        p9 = subprocess.Popen(["sed", "-e", "/_byte_address/d"], stdin=p8.stdout, stdout=subprocess.PIPE)
        p10 = subprocess.Popen(["sed", "-e", "s/CB/TOF3/g"], stdin=p9.stdout, stdout=subprocess.PIPE)
        p11 = subprocess.Popen(["sed", "-e", "s/cb/tof3/g"], stdin=p10.stdout, stdout=subprocess.PIPE)
        p12 = subprocess.Popen(["sed", "-e", "s/_Cb//g"], stdin=p11.stdout, stdout=outfile6)
        p12.wait()
        outfile6.write("/* clang-format on */\n")
        outfile6.flush()
        outfile6.close()

    with open('../../include/tof3_regs/tof3_reg_drv.h', "w") as outfile7:
        outfile7.write(copyright_header)
        outfile7.write("/* clang-format off */\n")
        outfile7.write("/*\n" + "Register Version: \"" + reg_version + "\"\n*/\n")
        outfile7.flush()
        pa = subprocess.Popen(["grep", "-v", "_byte_address", h_file], stdout=subprocess.PIPE)
        pb = subprocess.Popen(["grep", "-v", "static const size_t cb_reg_pipes_"], stdin=pa.stdout, stdout=subprocess.PIPE)
        pc = subprocess.Popen(["grep", "-v", ": cb_reg.pipes"], stdin=pb.stdout, stdout=subprocess.PIPE)
        pd = subprocess.Popen(["sed", "-e", "s/CB/TOF3/g"], stdin=pc.stdout, stdout=subprocess.PIPE)
        pe = subprocess.Popen(["sed", "-e", "s/cb/tof3/g"], stdin=pd.stdout, stdout=subprocess.PIPE)
        pf = subprocess.Popen(["sed", "-e", "s/_Cb//g"], stdin=pe.stdout, stdout=outfile7)
        pf.wait()
        #with open(h_file) as infile:
        #    shutil.copyfileobj(infile, outfile7)
        outfile7.write("/* clang-format on */\n")
        outfile7.flush()
        outfile7.close()

    with open(mem_output, "w") as outfile:
        outfile.write(copyright_header)
        outfile.write("/* clang-format off */\n")
        outfile.write("/*\n" + "Register Version: \"" + reg_version + "\"\n*/\n")
        outfile.flush()
        pd = subprocess.Popen(["sed", "-e", "s/CB/TOF3/g", tmp_file], stdout=subprocess.PIPE)
        pe = subprocess.Popen(["sed", "-e", "s/cb/tof3/g"], stdin=pd.stdout, stdout=subprocess.PIPE)
        pf = subprocess.Popen(["sed", "-e", "s/_Cb//g"], stdin=pe.stdout, stdout=outfile)
        pf.wait()
        outfile.write("/* clang-format on */\n")
        outfile.flush()
        outfile.close()

    subprocess.Popen(["rm", tmp_file])
