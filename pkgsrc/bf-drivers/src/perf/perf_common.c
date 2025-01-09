/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/


#include <errno.h>
#include <math.h>

#include <target-utils/uCli/ucli.h>
#include <bfutils/bf_utils.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <lld/lld_sku.h>
#include <pipe_mgr/pipe_mgr_drv.h>

#include <perf/perf_common_intf.h>
#include <perf/perf_env_intf.h>
#include <perf/perf_mem_intf.h>
#include <perf/perf_int_intf.h>
#include <perf/perf_reg_intf.h>
#include "perf_util.h"

char *bus_type_name[PERF_INT_BUS_T_MAX] = {"Pbus", "Mbus", "Cbus", "HostIf"};

struct test_description *tests_list[] = {&sram_dma_test,
                                         &tcam_dma_test,
                                         &interrupts_test,
                                         &reg_indir_test,
                                         &reg_dir_test,
                                         NULL};

struct enum_description enum_list[] = {
    {.enum_name = "bus", .enum_desc = "0:PBUS,1:MBUS,2:CBUS,3:HOSTIF"},
    // last element
    {.enum_name = ""}};

struct test_description **list_tests() {
  return tests_list;
}

struct enum_description *describe_enum() {
  return enum_list;
}

int get_params_n_max() { return PARAMS_N_MAX; }

int get_results_n_max() { return RESULTS_N_MAX; }

int get_metrics_n_max() { return METRICS_N_MAX; }

int get_env_param_n_max() { return ENV_PARAM_N_MAX; }

int get_env_param_value_l_max() { return ENV_PARAM_VALUE_L_MAX; }
