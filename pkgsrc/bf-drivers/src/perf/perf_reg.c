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
#include <time.h>

#include <target-utils/uCli/ucli.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/lld_tof2_addr_conversion.h>
#include <pipe_mgr/pipe_mgr_drv.h>
#include <tofino_regs/tofino.h>

#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <lld/tof2_reg_drv_defs.h>
#include <lld/tofino_defs.h>

#include "perf_util.h"
#include <perf/perf_common_intf.h>
#include <perf/perf_reg_intf.h>
#include "perf_reg.h"

/**
 * @brief Prepare structure with addresses of the registers for Tofino
 * and for direct and indirect registers
 *
 * @param bus_type bus type
 * @param indirect whether addresses for direct or indirect registers should be
 * generated. If true - indirect. If false - direct.
 * @return struct reg_entry structure containing prepared registers addresses
 */
static bf_status_t get_tofino_reg_entry(perf_bus_t_enum bus_type,
                                        bool indirect,
                                        reg_entry_t *reg_entry) {
  if (reg_entry == NULL) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  switch (bus_type) {
    case PBUS:
      reg_entry->addr = offsetof(Tofino, pipes[0].mau[0].dp.mau_scratch);
      if (indirect) {
        reg_entry->addr = tof_dir_to_indir_pipe(reg_entry->addr);
      }
      break;
    case CBUS:
      reg_entry->addr = offsetof(Tofino, device_select.lfltr0.ctrl.scratch);
      if (indirect) {
        reg_entry->addr = tof_dir_to_indir_dev_sel(reg_entry->addr);
      }
      break;
    case MBUS:
      reg_entry->addr = offsetof(Tofino, macs_t[0].macs.eth_regs.scratch_r);
      if (indirect) {
        reg_entry->addr = tof_dir_to_indir_mac(reg_entry->addr);
      }
      break;
    case HOSTIF:
      reg_entry->addr = offsetof(
          Tofino, device_select.pcie_bar01_regs.pcie_regs.scratch_reg[0]);
      if (indirect) {
        reg_entry->addr = tof_dir_to_indir_dev_sel(reg_entry->addr);
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d: Bus type not supported %d\n", __func__, __LINE__, bus_type);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/**
 * @brief Prepare structure with addresses of the registers for Tofino2
 * and for direct and indirect registers
 *
 * @param bus_type bus type
 * @param indirect whether addresses for direct or indirect registers should be
 * generated. If false - direct. If true - indirect
 * @return struct reg_entry structure containing prepared registers addresses
 */
static bf_status_t get_tofino2_reg_entry(perf_bus_t_enum bus_type,
                                         bool indirect,
                                         reg_entry_t *reg_entry) {
  if (reg_entry == NULL) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  switch (bus_type) {
    case PBUS:
      reg_entry->addr = offsetof(tof2_reg, pipes[0].mau[0].dp.mau_scratch);
      if (indirect) {
        reg_entry->addr = tof2_dir_to_indir_pipe_reg(reg_entry->addr);
      }
      break;
    case CBUS:
      reg_entry->addr =
          offsetof(tof2_reg, device_select.lfltr[0].ctrl.scratch[0]);
      if (indirect) {
        reg_entry->addr = tof2_dir_to_indir_dev_sel(reg_entry->addr);
      }
      break;
    case MBUS:
      reg_entry->addr = offsetof(tof2_reg, eth100g_regs.eth100g_reg.scratch);
      if (indirect) {
        reg_entry->addr = tof2_dir_to_indir_mac_reg(reg_entry->addr);
      }
      break;
    case HOSTIF:
      reg_entry->addr =
          offsetof(tof2_reg, device_select.pcie_bar01_regs.scratch_reg[0]);
      if (indirect) {
        reg_entry->addr = tof2_dir_to_indir_dev_sel(reg_entry->addr);
      }
      break;
    default:
      LOG_ERROR(
          "%s:%d: Bus type not supported %d\n", __func__, __LINE__, bus_type);
      bf_sys_dbgchk(0);
      return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

/**
 * @brief Prepare structure with addresses of the registers. Respectively for
 * Tofino and Tofino2 and for direct and indirect registers
 *
 * @param dev_family Tofino or Tofino2
 * @param bus_type bus type
 * @param indirect whether addresses for direct or indirect registers should be
 * generated. If false - direct. If true - indirect
 * @return struct reg_entry structure containing prepared registers addresses
 */
static bf_status_t get_reg_entry(bf_dev_family_t dev_family,
                                 perf_bus_t_enum bus_type,
                                 bool indirect,
                                 reg_entry_t *reg_entry) {
  if (reg_entry == NULL) {
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  if (dev_family == BF_DEV_FAMILY_TOFINO) {
    get_tofino_reg_entry(bus_type, indirect, reg_entry);

  } else if (dev_family == BF_DEV_FAMILY_TOFINO2) {
    get_tofino2_reg_entry(bus_type, indirect, reg_entry);

  } else {
    LOG_ERROR("%s:%d: Device type not supported %d\n",
              __func__,
              __LINE__,
              dev_family);
    bf_sys_dbgchk(0);
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

/**
 * @brief Run performance test that will write/read to/from regitser,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @param indirect whether addresses for direct or indirect registers should be
 * generated. If true - indirect. If false - direct.
 * @return register_result
 */
bf_status_t run_reg_test(bf_dev_id_t dev_id,
                         perf_bus_t_enum bus_type,
                         int it,
                         bool indirect,
                         struct register_result *result) {
  struct timespec read, write, stop;

  if (!result) {
    LOG_ERROR("%s:%d: No allocated memory for results\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }
  result->status = false;

  bf_dev_type_t dev_type = lld_dev_type_get(dev_id);
  bf_dev_family_t family = bf_dev_type_to_family(dev_type);
  uint64_t data1, data2;
  uint32_t data;

  reg_entry_t reg_entry;
  memset(&reg_entry, 0, sizeof reg_entry);

  if (get_reg_entry(family, bus_type, indirect, &reg_entry) != BF_SUCCESS) {
    LOG_ERROR("%s:%d: Invalid test parameters\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  clock_gettime(CLOCK_MONOTONIC, &read);

  if (indirect) {
    /* register write/read, indirect access */
    for (int i = 0; i < it; i++) {
      lld_ind_read(dev_id, reg_entry.addr, &data1, &data2);
    }
    clock_gettime(CLOCK_MONOTONIC, &write);
    for (int i = 0; i < it; i++) {
      lld_ind_write(dev_id, reg_entry.addr, data1, data2);
    }

  } else {
    /* register write/read, direct access */
    for (int i = 0; i < it; i++) {
      lld_read_register(dev_id, reg_entry.addr, &data);
    }
    clock_gettime(CLOCK_MONOTONIC, &write);
    for (int i = 0; i < it; i++) {
      lld_write_register(dev_id, reg_entry.addr, data);
    }
  }
  clock_gettime(CLOCK_MONOTONIC, &stop);

  if (!ts_to_ops(read, write, it, &result->read_op, &result->read_ns)) {
    LOG_ERROR("%s:%d: Invalid test results\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  if (!ts_to_ops(write, stop, it, &result->write_op, &result->write_ns)) {
    LOG_ERROR("%s:%d: Invalid test results\n", __func__, __LINE__);
    bf_sys_dbgchk(0);
    return BF_UNEXPECTED;
  }

  result->status = true;
  return BF_SUCCESS;
}

struct test_description reg_indir_test = {
    .test_name = "reg_indir",
    .description =
        "The Registers Indirect test measures the speed of chip's internal\n"
        "address space read and write operations.\n"
        "The test uses the CLOCK_MONOTONIC POSIX clock for all measurements.\n"
        "Bus type and number of iterations are specified within the test\n"
        "parameters.\n"
        "The reported test result value is the average processing\n"
        "time of the read / write operations.\n",
    .params = {{.name = "bus", .type = "enum", .defaults = "0"},
               {.name = "iterations", .type = "int", .defaults = "100000"},
               // last element
               {.name = ""}},
    .results = {{.header = "bus", .unit = "[-]", .type = "enum"},
                {.header = "iterations", .unit = "[-]", .type = "int"},
                {.header = "write_ns", .unit = "[ns]", .type = "double"},
                {.header = "write_op", .unit = "[op/s]", .type = "double"},
                {.header = "read_ns", .unit = "[ns]", .type = "double"},
                {.header = "read_op", .unit = "[op/s]", .type = "double"},
                // last element
                {.header = ""}}};

/**
 * @brief Run performance test that will indirect write/read to/from regitser,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @param it number of iterations
 * @return register_result
 */
struct test_results reg_indir(bf_dev_id_t dev_id,
                              perf_bus_t_enum bus_type,
                              int it) {
  struct register_result result;
  struct test_results p_results;
  memset(&result, 0, sizeof(result));
  memset(&p_results, 0, sizeof(p_results));

  run_reg_test(dev_id, bus_type, it, true, &result);

  p_results.status = result.status;
  p_results.res_enum[RES_REG_BUS] = bus_type;
  p_results.res_int[RES_REG_IT] = it;
  p_results.res_double[RES_REG_WRITE_NS] = result.write_ns;
  p_results.res_double[RES_REG_WRITE_OP] = result.write_op;
  p_results.res_double[RES_REG_READ_NS] = result.read_ns;
  p_results.res_double[RES_REG_READ_OP] = result.read_op;

  return p_results;
}

struct test_description reg_dir_test = {
    .test_name = "reg_dir",
    .description =
        "The Registers Direct test measures the speed of a 32-bit\n"
        "PCIe address space read and write operation.\n"
        "The test uses the CLOCK_MONOTONIC POSIX clock for all measurements.\n"
        "Bus type and number of iterations are specified within the test\n"
        "parameters.\n"
        "The reported test result value is the average processing time of\n"
        "the read / write operations.\n",
    .params = {{.name = "bus", .type = "enum", .defaults = "0"},
               {.name = "iterations", .type = "int", .defaults = "100000"},
               // last element
               {.name = ""}},
    .results = {{.header = "bus", .unit = "[-]", .type = "enum"},
                {.header = "iterations", .unit = "[-]", .type = "int"},
                {.header = "write_ns", .unit = "[ns]", .type = "double"},
                {.header = "write_op", .unit = "[op/s]", .type = "double"},
                {.header = "read_ns", .unit = "[ns]", .type = "double"},
                {.header = "read_op", .unit = "[op/s]", .type = "double"},
                // last element
                {.header = ""}}};

/**
 * @brief Run performance test that will direct write/read to/from regitser,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @param it number of iterations
 * @return register_result
 */
struct test_results reg_dir(bf_dev_id_t dev_id,
                            perf_bus_t_enum bus_type,
                            int it) {
  struct register_result result;
  struct test_results p_results;
  memset(&result, 0, sizeof(result));
  memset(&p_results, 0, sizeof(p_results));

  run_reg_test(dev_id, bus_type, it, false, &result);

  p_results.status = result.status;
  p_results.res_enum[RES_REG_BUS] = bus_type;
  p_results.res_int[RES_REG_IT] = it;
  p_results.res_double[RES_REG_WRITE_NS] = result.write_ns;
  p_results.res_double[RES_REG_WRITE_OP] = result.write_op;
  p_results.res_double[RES_REG_READ_NS] = result.read_ns;
  p_results.res_double[RES_REG_READ_OP] = result.read_op;

  return p_results;
}
