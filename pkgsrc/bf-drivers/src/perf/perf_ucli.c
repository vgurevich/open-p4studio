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

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include <dvm/bf_drv_intf.h>

#include <bfutils/bf_utils.h>

#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include <lld/lld_dev.h>
#include <lld/lld_sku.h>
#include <pipe_mgr/pipe_mgr_drv.h>

#include "perf_util.h"
#include <perf/perf_common_intf.h>
#include <perf/perf_env_intf.h>
#include "perf_env.h"
#include <perf/perf_reg_intf.h>
#include "perf_reg.h"
#include <perf/perf_mem_intf.h>
#include "perf_mem.h"
#include <perf/perf_int_intf.h>
#include "perf_int.h"
#include "perf_ucli.h"

/**
 * @brief Print tofino sku, information about model/HW and versions of the
 * subcomponents
 * @param uc ucli context pointer
 * @param dev_id device id
 */
void print_versions(ucli_context_t *uc, bf_dev_id_t dev_id) {
  banner(uc, "ENVIRONMENT");

  struct env_description env_desc;
  env_desc = environment(dev_id);

  if (env_desc.status) {
    aim_printf(&uc->pvs,
               "%s %s\n",
               env_desc.param[ENV_FAMILY].value,
               env_desc.param[ENV_TYPE].value);

    aim_printf(&uc->pvs,
               "%s: %s\n",
               env_desc.param[ENV_SKU].name,
               env_desc.param[ENV_SKU].value);

    aim_printf(&uc->pvs,
               "%s: %s (Internal:%s)\n",
               env_desc.param[ENV_SYSLIBS].name,
               env_desc.param[ENV_SYSLIBS].value,
               env_desc.param[ENV_SYSLIBS_INT].value);

    aim_printf(&uc->pvs,
               "%s: %s (Internal:%s)\n",
               env_desc.param[ENV_UTILS].name,
               env_desc.param[ENV_UTILS].value,
               env_desc.param[ENV_UTILS_INT].value);

    aim_printf(&uc->pvs,
               "%s: %s (Internal:%s)\n",
               env_desc.param[ENV_DRIVERS].name,
               env_desc.param[ENV_DRIVERS].value,
               env_desc.param[ENV_DRIVERS_INT].value);
  }

  FILE *fpt;
  fpt = fopen("perf_versions.txt", "w+");
  if (fpt != NULL) {
    fprintf(fpt,
            "%s %s\n",
            env_desc.param[ENV_FAMILY].value,
            env_desc.param[ENV_TYPE].value);

    fprintf(fpt,
            "%s: %s\n",
            env_desc.param[ENV_SKU].name,
            env_desc.param[ENV_SKU].value);

    fprintf(fpt,
            "%s: %s (Internal:%s)\n",
            env_desc.param[ENV_SYSLIBS].name,
            env_desc.param[ENV_SYSLIBS].value,
            env_desc.param[ENV_SYSLIBS_INT].value);

    fprintf(fpt,
            "%s: %s (Internal:%s)\n",
            env_desc.param[ENV_UTILS].name,
            env_desc.param[ENV_UTILS].value,
            env_desc.param[ENV_UTILS_INT].value);

    fprintf(fpt,
            "%s: %s (Internal:%s)\n",
            env_desc.param[ENV_DRIVERS].name,
            env_desc.param[ENV_DRIVERS].value,
            env_desc.param[ENV_DRIVERS_INT].value);

    fclose(fpt);
  } else {
    aim_printf(&uc->pvs,
               "%s:%d: Cannot open file: %s\n",
               __func__,
               __LINE__,
               strerror(errno));
  }
}

/**
 * @brief Take two dimension array with results along with the size of it and
 * save it to a csv file
 *
 * @param uc ucli context pointer
 * @param file_name Name of the file that the data should be saved in
 * @param num_cols Number of columns in the results array
 * @param num_rows Number of rows in the results array
 * @param result_hdr Array of column names (names of the tests)
 * @param unit_hdr Array of the units (for each test)
 * @param results two dimension array with results
 */
void save_results_file(ucli_context_t *uc,
                       char *file_name,
                       int num_cols,
                       int num_rows,
                       char *result_hdr[],
                       char *unit_hdr[],
                       double results[][num_cols]) {
  FILE *fpt;
  fpt = fopen(file_name, "w+");
  if (fpt != NULL) {
    for (int i = 0; i < num_cols - 1; i++) {
      fprintf(fpt, "%s,", result_hdr[i]);
    }
    fprintf(fpt, "%s", result_hdr[num_cols - 1]);
    fprintf(fpt, "\n");
    for (int i = 0; i < num_cols - 1; i++) {
      fprintf(fpt, "%s,", unit_hdr[i]);
    }
    /* Handle the last element separately to do not add comma
     * at the end of the line */
    fprintf(fpt, "%s\n", unit_hdr[num_cols - 1]);
    for (int k = 0; k < num_rows; k++) {
      for (int i = 0; i < num_cols - 1; i++) {
        fprintf(fpt, "%.2f,", results[k][i]);
      }
      /* Handle the last element separately to do not add comma
       * at the end of the line */
      fprintf(fpt, "%.2f\n", results[k][num_cols - 1]);
    }
    fclose(fpt);
  } else {
    aim_printf(&uc->pvs,
               "%s:%d: Cannot open %s: %s\n",
               __func__,
               __LINE__,
               file_name,
               strerror(errno));
  }
}

/**
 * @brief Print desired string as a banner surrounded by asterisks
 *
 * @param uc ucli context pointer
 * @param string String to be printed
 */
void banner(ucli_context_t *uc, char *string) {
  aim_printf(&uc->pvs, "****************** %-10s ******************\n", string);
}

/**
 * @brief Handler for command that will run all perf tests and summarize the
 * results
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__all__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "all", 1, "show basic info and run all tests <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  print_versions(uc, dev_id);
  run_registers(uc, dev_id, true);
  run_registers(uc, dev_id, false);
  run_sram_dma(uc, dev_id);
  run_tcam_dma(uc, dev_id);
  run_interrupts(uc, dev_id);
  return UCLI_STATUS_OK;
}

/**
 * @brief Print basic info about environment like tofino sku, information
 * about model/HW and versions of the subcomponents
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__environment__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "env", 1, "show basic info about environment <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  print_versions(uc, dev_id);
  return UCLI_STATUS_OK;
}

/**
 * @brief Handler for interrupts perf testing command
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__interrupts__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "interrupts", 1, "test interrupts latency <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  run_interrupts(uc, dev_id);
  return UCLI_STATUS_OK;
}

/**
 * @brief Handler for direct registers perf testing command
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__registers_direct__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "reg_dir", 1, "test registers direct access <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  run_registers(uc, dev_id, false);
  return UCLI_STATUS_OK;
}

/**
 * @brief Handler for indirect registers perf testing command
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__registers_indirect__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(
      uc, "reg_indir", 1, "test registers indirect access <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  run_registers(uc, dev_id, true);
  return UCLI_STATUS_OK;
}

/**
 * @brief Handler for TCAM DMA perf testing command
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__tcam_dma__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "tcam_dma", 1, "test TCAM DMA transfer <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  run_tcam_dma(uc, dev_id);
  return UCLI_STATUS_OK;
}

/**
 * @brief Handler for SRAM DMA perf testing command
 *
 * @param uc ucli context pointer
 * @return ucli_status_t
 */
static ucli_status_t perf_ucli__sram_dma__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "sram_dma", 1, "test SRAM DMA transfer <dev_id>");
  bf_dev_id_t dev_id;
  char *endptr;
  const char *str = uc->pargs->args[0];

  errno = 0;
  dev_id = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str) {
    aim_printf(&uc->pvs, "Incorrect device-id parameter format\n");
    return UCLI_STATUS_E_PARAM;
  }

  run_sram_dma(uc, dev_id);
  return UCLI_STATUS_OK;
}

/**
 * @brief Array of handlers to ucli functions
 *
 */
static ucli_command_handler_f perf_ucli_ucli_handlers__[] = {
    perf_ucli__all__,
    perf_ucli__environment__,
    perf_ucli__sram_dma__,
    perf_ucli__tcam_dma__,
    perf_ucli__interrupts__,
    perf_ucli__registers_direct__,
    perf_ucli__registers_indirect__,
    NULL};

/**
 * @brief Definition of the perf uCLI module
 *
 */
static ucli_module_t perf_ucli_module__ = {
    "perf",
    NULL,
    perf_ucli_ucli_handlers__,
    NULL,
    NULL,
};

/**
 * @brief Create and add node with perf commands to uCLI
 *
 * @return ucli_node_t* pointer to the node
 */
ucli_node_t *perf_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&perf_ucli_module__);
  // name in the parent context and prompt
  n = ucli_node_create("perf", NULL, &perf_ucli_module__);

  // name for the log module
  ucli_node_subnode_add(n, ucli_module_log_node_create("perf"));
  return n;
}

/**
 * @brief Run performance test that will measure latency of
 * interrupts processing
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return bf_status_t
 */
bf_status_t run_interrupts(ucli_context_t *uc, bf_dev_id_t dev_id) {
  int status = UCLI_STATUS_OK;
  perf_bus_t_enum bus_type;

  banner(uc, "INTERRUPTS LATENCY");
  bool is_sw_model;
  bf_status_t sts;
  sts = bf_drv_device_type_get(dev_id, &is_sw_model);
  if (sts != BF_SUCCESS) {
    aim_printf(&uc->pvs, "Error getting device type for device: %d", dev_id);
    return BF_UNEXPECTED;
  }
  if (is_sw_model) {
    aim_printf(&uc->pvs,
               "The test can only be run on hardware, it does not apply to the "
               "model\n");
    return BF_NOT_SUPPORTED;
  }

  enum hdr_t {
    BUS_TYPE,
    INTERRUPTS,
    ITERATIONS,
    LATENCY_AVG,
    LATENCY_SD,
    LATENCY_MIN,
    LATENCY_MAX
  };

  char *result_hdr[] = {"Bus",
                        "Interrupts",
                        "Iterations",
                        "Latency avg",
                        "Latency sd",
                        "Latency min",
                        "Latency max"};

  char *unit_hdr[] = {"[-]", "[-]", "[-]", "[us]", "[us]", "[us]", "[us]"};

  struct interrupts_result results[PERF_INT_BUS_T_MAX] = {0};

  aim_printf(&uc->pvs,
             "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
             result_hdr[BUS_TYPE],
             result_hdr[INTERRUPTS],
             result_hdr[ITERATIONS],
             result_hdr[LATENCY_AVG],
             result_hdr[LATENCY_SD],
             result_hdr[LATENCY_MIN],
             result_hdr[LATENCY_MAX]);
  aim_printf(&uc->pvs,
             "%15s\t%15s\t%15s\t%15s\t%15s\t%15s\t%15s\n",
             unit_hdr[BUS_TYPE],
             unit_hdr[INTERRUPTS],
             unit_hdr[ITERATIONS],
             unit_hdr[LATENCY_AVG],
             unit_hdr[LATENCY_SD],
             unit_hdr[LATENCY_MIN],
             unit_hdr[LATENCY_MAX]);

  for (bus_type = 0; bus_type < PERF_INT_BUS_T_MAX; bus_type++) {
    if (is_bus_ints_supported(dev_id, bus_type)) {
      status = run_int_test(dev_id,
                            bus_type,
                            bus_type == PBUS ? 1 : PERF_INT_ITERS,
                            &results[bus_type]);
      if (status != BF_SUCCESS) {
        aim_printf(&uc->pvs,
                   "%s:%d: %s interrupt error\n",
                   __func__,
                   __LINE__,
                   bus_type_name[bus_type]);
        bf_sys_dbgchk(0);
        return status;
      }

      aim_printf(&uc->pvs,
                 "%15s\t%15d\t%15d\t%15.2f\t%15.2f\t%15.2f\t%15.2f\n",
                 bus_type_name[bus_type],
                 results[bus_type].interrupts,
                 results[bus_type].iterations,
                 results[bus_type].avg_latency_us,
                 results[bus_type].sd_latency_us,
                 results[bus_type].min_latency_us,
                 results[bus_type].max_latency_us);
    }
  }

  FILE *fpt;
  fpt = fopen("perf_interrupts.csv", "w+");
  if (fpt != NULL) {
    fprintf(fpt,
            "%s,%s,%s,%s,%s,%s,%s\n",
            result_hdr[BUS_TYPE],
            result_hdr[INTERRUPTS],
            result_hdr[ITERATIONS],
            result_hdr[LATENCY_AVG],
            result_hdr[LATENCY_SD],
            result_hdr[LATENCY_MIN],
            result_hdr[LATENCY_MAX]);
    fprintf(fpt,
            "%s,%s,%s,%s,%s,%s,%s\n",
            unit_hdr[BUS_TYPE],
            unit_hdr[INTERRUPTS],
            unit_hdr[ITERATIONS],
            unit_hdr[LATENCY_AVG],
            unit_hdr[LATENCY_SD],
            unit_hdr[LATENCY_MIN],
            unit_hdr[LATENCY_MAX]);

    for (bus_type = 0; bus_type < PERF_INT_BUS_T_MAX; bus_type++) {
      fprintf(fpt,
              "%s,%d,%d,%.2f,%.2f,%.2f,%.2f\n",
              bus_type_name[bus_type],
              results[bus_type].interrupts,
              results[bus_type].iterations,
              results[bus_type].avg_latency_us,
              results[bus_type].sd_latency_us,
              results[bus_type].min_latency_us,
              results[bus_type].max_latency_us);
    }
    fclose(fpt);
  } else {
    aim_printf(&uc->pvs,
               "%s:%d: Cannot open perf_ints_latency: %s\n",
               __func__,
               __LINE__,
               strerror(errno));
  }

  return BF_SUCCESS;
}

/**
 * @brief Run performance test that will iterate over indirect/direct registers,
 * iterate read/write operations and calculate the rate
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @param indirect indirect access
 * @return ucli_status_t
 */
ucli_status_t run_registers(ucli_context_t *uc,
                            bf_dev_id_t dev_id,
                            bool indirect) {
  enum rate {
    bus,
    write_ns_hdr,
    write_op_hdr,
    read_ns_hdr,
    read_op_hdr,
    REG_RESULTS
  };
  char *result_hdr[REG_RESULTS];
  char *unit_hdr[REG_RESULTS];

  result_hdr[bus] = "Bus";
  result_hdr[write_ns_hdr] = "Write";
  result_hdr[write_op_hdr] = "Write";
  result_hdr[read_ns_hdr] = "Read";
  result_hdr[read_op_hdr] = "Read";

  unit_hdr[bus] = "[-]";
  unit_hdr[write_ns_hdr] = "[ns/op]";
  unit_hdr[write_op_hdr] = "[op/s]";
  unit_hdr[read_ns_hdr] = "[ns/op]";
  unit_hdr[read_op_hdr] = "[op/s]";

  struct result {
    char bus[10];
    double write_ns;
    double write_op;
    double read_ns;
    double read_op;
  };

  indirect == true ? banner(uc, "REGISTERS INDIRECT ACCESS")
                   : banner(uc, "REGISTERS DIRECT ACCESS");

  for (int i = 0; i < REG_RESULTS; i++) {
    aim_printf(&uc->pvs, "%15s ", result_hdr[i]);
  }
  aim_printf(&uc->pvs, "\n");
  for (int i = 0; i < REG_RESULTS; i++) {
    aim_printf(&uc->pvs, "%15s ", unit_hdr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  struct register_result results[PERF_INT_BUS_T_MAX];
  memset(&results, 0, sizeof(results));

  perf_bus_t_enum bus_type;
  for (bus_type = 0; bus_type < PERF_INT_BUS_T_MAX; bus_type++) {
    run_reg_test(
        dev_id, bus_type, PERF_REG_ITERS, indirect, &results[bus_type]);

    aim_printf(&uc->pvs,
               "%15s %15.2f %15.2f %15.2f %15.2f\n",
               bus_type_name[bus_type],
               results[bus_type].write_ns,
               results[bus_type].write_op,
               results[bus_type].read_ns,
               results[bus_type].read_op);
  }

  FILE *fpt;
  indirect == true ? (fpt = fopen("perf_reg_indir.csv", "w+"))
                   : (fpt = fopen("perf_reg_dir.csv", "w+"));
  if (fpt != NULL) {
    for (int i = 0; i < REG_RESULTS - 1; i++) {
      fprintf(fpt, "%s,", result_hdr[i]);
    }
    /* Handle the last element separately to do not add comma
     * at the end of the line */
    fprintf(fpt, "%s", result_hdr[REG_RESULTS - 1]);
    fprintf(fpt, "\n");
    for (int i = 0; i < REG_RESULTS - 1; i++) {
      fprintf(fpt, "%s,", unit_hdr[i]);
    }
    /* Handle the last element separately to do not add comma
     * at the end of the line */
    fprintf(fpt, "%s", unit_hdr[REG_RESULTS - 1]);
    fprintf(fpt, "\n");

    for (bus_type = 0; bus_type < PERF_INT_BUS_T_MAX; bus_type++) {
      fprintf(fpt,
              "%s,%.2f,%.2f,%.2f,%.2f\n",
              bus_type_name[bus_type],
              results[bus_type].write_ns,
              results[bus_type].write_op,
              results[bus_type].read_ns,
              results[bus_type].read_op);
    }
    fclose(fpt);
  } else {
    aim_printf(&uc->pvs,
               "%s:%d: Cannot open: %s\n",
               __func__,
               __LINE__,
               strerror(errno));
  }
  return UCLI_STATUS_OK;
}

/**
 * @brief Run performance test that will write/read to/from DMA memory,
 * and calculate the rate
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @param mem_type Memory type (SRAM/TCAM)
 * @param num_pipes Number of PIPEs
 * @param num_maus Number of MAUs
 * @param num_rows Number of ROWs
 * @param num_cols Number of COLUMNs
 * @return ucli_status_t
 */
static ucli_status_t run_tcam_sram_group(ucli_context_t *uc,
                                         bf_dev_id_t dev_id,
                                         pipe_mem_type_t mem_type,
                                         int num_pipes,
                                         int num_maus,
                                         int num_rows,
                                         int num_cols,
                                         char *banner_name,
                                         char *file_name) {
  enum dma_rate {
    pipes_idx,
    maus_idx,
    rows_idx,
    cols_idx,
    write_mb,
    write_us,
    read_mb,
    read_us,
    DMA_RATE_RESULT_COLUMNS
  };
  char *result_hdr[] = {
      "Pipes", "MAUs", "Rows", "Cols", "Write", "Write", "Read", "Read"};
  char *unit_hdr[] = {
      "[-]", "[-]", "[-]", "[-]", "[MB/s]", "[us/MB]", "[MB/s]", "[us/MB]"};

  struct dma_test_pattern_t {
    int pipes;
    int maus;
    int rows;
    int cols;
  };

  struct dma_test_pattern_t dma_test_pattern[] = {
      {1, 1, 1, 1},
      {num_pipes, 1, 1, 1},
      {1, 1, num_rows, num_cols},
      {1, num_maus, num_rows, num_cols},
      {num_pipes, 1, num_rows, num_cols}};

#define DMA_RESULT_ROWS \
  (int)(sizeof(dma_test_pattern) / sizeof(struct dma_test_pattern_t))

  double results[DMA_RESULT_ROWS][DMA_RATE_RESULT_COLUMNS] = {0};
  struct test_results raw_results[DMA_RESULT_ROWS];
  memset(raw_results, 0, sizeof(raw_results));

  for (int i = 0; i < DMA_RESULT_ROWS; i++) {
    run_mem_test(dev_id,
                 mem_type,
                 dma_test_pattern[i].pipes,
                 dma_test_pattern[i].maus,
                 dma_test_pattern[i].rows,
                 dma_test_pattern[i].cols,
                 &raw_results[i]);

    if (!raw_results->status) {
      aim_printf(
          &uc->pvs, "%s:%d: %s test failed\n", __func__, __LINE__, banner_name);
      bf_sys_dbgchk(0);
      return UCLI_STATUS_E_ERROR;
    }

    results[i][pipes_idx] = raw_results[i].res_int[RES_PIPES];
    results[i][maus_idx] = raw_results[i].res_int[RES_MAUS];
    results[i][rows_idx] = raw_results[i].res_int[RES_ROWS];
    results[i][cols_idx] = raw_results[i].res_int[RES_COLS];
    results[i][write_mb] = raw_results[i].res_double[RES_WRITE_MB];
    results[i][write_us] = raw_results[i].res_double[RES_WRITE_US];
    results[i][read_mb] = raw_results[i].res_double[RES_READ_MB];
    results[i][read_us] = raw_results[i].res_double[RES_READ_US];
  }

  banner(uc, banner_name);
  for (int i = 0; i < DMA_RATE_RESULT_COLUMNS; i++) {
    aim_printf(&uc->pvs, "%15s ", result_hdr[i]);
  }
  aim_printf(&uc->pvs, "\n");
  for (int i = 0; i < DMA_RATE_RESULT_COLUMNS; i++) {
    aim_printf(&uc->pvs, "%15s ", unit_hdr[i]);
  }
  aim_printf(&uc->pvs, "\n");
  for (int i = 0; i < DMA_RESULT_ROWS; i++) {
    aim_printf(&uc->pvs,
               "%15.0f %15.0f %15.0f %15.0f %15.2f %15.2f %15.2f %15.2f\n",
               results[i][pipes_idx],
               results[i][maus_idx],
               results[i][rows_idx],
               results[i][cols_idx],
               results[i][write_mb],
               results[i][write_us],
               results[i][read_mb],
               results[i][read_us]);
  }
  save_results_file(uc,
                    file_name,
                    DMA_RATE_RESULT_COLUMNS,
                    DMA_RESULT_ROWS,
                    result_hdr,
                    unit_hdr,
                    results);
  return UCLI_STATUS_OK;
}

/**
 * @brief Run performance test that will write/read to/from SRAM memory,
 * and calculate the rate.
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return ucli_status_t
 */
ucli_status_t run_sram_dma(ucli_context_t *uc, bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_PARAM;
  }

  return run_tcam_sram_group(uc,
                             dev_id,
                             pipe_mem_type_unit_ram,
                             dev_info->num_active_pipes,
                             dev_info->num_active_mau,
                             dev_info->dev_cfg.stage_cfg.num_sram_rows,
                             dev_info->dev_cfg.stage_cfg.num_sram_cols,
                             "SRAM DMA TRANSFER",
                             "perf_sram_dma.csv");
}

/**
 * @brief Run performance test that will write/read to/from TCAM memory,
 * and calculate the rate.
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return ucli_status_t
 */
ucli_status_t run_tcam_dma(ucli_context_t *uc, bf_dev_id_t dev_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d: Device doesn't exist for device-id = %d\n",
              __func__,
              __LINE__,
              dev_id);
    bf_sys_dbgchk(0);
    return UCLI_STATUS_E_PARAM;
  }

  return run_tcam_sram_group(uc,
                             dev_id,
                             pipe_mem_type_tcam,
                             dev_info->num_active_pipes,
                             dev_info->num_active_mau,
                             dev_info->dev_cfg.stage_cfg.num_tcam_rows,
                             dev_info->dev_cfg.stage_cfg.num_tcam_cols,
                             "TCAM DMA TRANSFER",
                             "perf_tcam_dma.csv");
}
