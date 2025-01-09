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

# This exemplary script can be run in switchd bfrt_python environment.
# To do this, use the following command:
# %run -i run_all_tests.py

from perfCli import *

if __name__ == "__main__":
    perf = CPerf(dev_id=0)

    perf.env()

    # run sram_dma test all currently supported configurations
    perf.sram_dma.run(pipes=1, maus=1, rows=1, cols=1)
    perf.sram_dma.run(pipes=2, maus=1, rows=1, cols=1)
    perf.sram_dma.run(pipes=1, maus=1, rows=8, cols=10)
    perf.sram_dma.run(pipes=1, maus=12, rows=8, cols=10)
    perf.sram_dma.run(pipes=2, maus=1, rows=8, cols=10)

    # run tcam_dma test all currently supported configurations
    perf.tcam_dma.run(pipes=1, maus=1, rows=1, cols=1)
    perf.tcam_dma.run(pipes=2, maus=1, rows=1, cols=1)
    perf.tcam_dma.run(pipes=1, maus=1, rows=12, cols=2)
    perf.tcam_dma.run(pipes=1, maus=12, rows=12, cols=2)
    perf.tcam_dma.run(pipes=2, maus=1, rows=12, cols=2)

    # run interrupts test all currently supported configurations
    perf.interrupts.run(bus='PBUS', iterations=1)
    perf.interrupts.run(bus='MBUS', iterations=300)
    perf.interrupts.run(bus='CBUS', iterations=300)
    perf.interrupts.run(bus='HOSTIF', iterations=300)

    # run reg_dir test all currently supported configurations
    perf.reg_dir.run(bus='PBUS')
    perf.reg_dir.run(bus='MBUS')
    perf.reg_dir.run(bus='CBUS')
    perf.reg_dir.run(bus='HOSTIF')

    # run reg_indir test all currently supported configurations
    perf.reg_indir.run(bus='PBUS')
    perf.reg_indir.run(bus='MBUS')
    perf.reg_indir.run(bus='CBUS')
    perf.reg_indir.run(bus='HOSTIF')
