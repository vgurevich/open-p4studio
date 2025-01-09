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


/*******************************************************************************
 *
 *   Diag memory tests UCLI
 *
 *****************************************************************************/
#if DEVDIAG_CONFIG_INCLUDE_UCLI == 1

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <inttypes.h>

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>
#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include "dvm/bf_drv_intf.h"
#include "lld/lld_inst_list_fmt.h"
#include "lld/lld_dev.h"
#include "lld/lld_sku.h"
#include "diag/bf_dev_diag.h"
#include "diag_internal.h"

#define DEVDIAG_CLI_CMD_HNDLR(name) DEVDIAG_ucli_ucli__##name##__
#define DEVDIAG_CLI_CMD_DECLARE(name) \
  static ucli_status_t DEVDIAG_CLI_CMD_HNDLR(name)(ucli_context_t * uc)

#define DEVDIAG_CLI_PROLOGUE(CMD, HELP, USAGE)                             \
  extern char *optarg;                                                     \
  extern int optind;                                                       \
  UCLI_COMMAND_INFO(uc, CMD, -1, HELP " Usage: " CMD " " USAGE);           \
  char usage[] = "Usage: " USAGE "\n";                                     \
  int arg_start = 0;                                                       \
  {                                                                        \
    size_t i;                                                              \
    for (i = 0;                                                            \
         i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]); \
         ++i) {                                                            \
      if (!strncmp(uc->pargs[0].args__[i], CMD, strlen(CMD))) {            \
        arg_start = i;                                                     \
        break;                                                             \
      }                                                                    \
    }                                                                      \
  }                                                                        \
  int argc = uc->pargs->count + 1;                                         \
  if (1 == argc) {                                                         \
    aim_printf(&uc->pvs, "Usage: " USAGE "\n");                            \
    return UCLI_STATUS_OK;                                                 \
  }                                                                        \
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);      \
  optind = 0;

DEVDIAG_CLI_CMD_DECLARE(memtest) {
  DEVDIAG_CLI_PROLOGUE(
      "memtest",
      "Run memory test on MAU/PARDE/TM",
      "Usage: -d <device> -i <memory: 0:mau,1:parde,2:tm> -t "
      "<test-type: "
      "0:pio,1:dma> -l <len: 0:quick,1:ext> [-m <pattern "
      "0:random,1:zeroes,2:ones,3:checkerboard,4:inv-checkerboard,5:prbs,6:"
      "user-defined>] [-a <pattern_data0> -b <pattern_data1>] [-p "
      "<log_pipe_bmp>]");
  bf_dev_id_t dev = 0;
  bool quick = false;
  int len = -1, memory = -1;
  bf_diag_test_type_t test_type = 100;
  bf_diag_test_pattern_t pattern = 0;
  bool data0_specified = false, data1_specified = false;
  uint64_t pattern_data0 = 0;
  uint64_t pattern_data1 = 0;
  bf_status_t status = BF_SUCCESS;
  uint32_t pipe_bmp = 0;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:s:t:l:i:p:m:a:b:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        memory = strtoull(optarg, NULL, 0);
        break;
      case 't':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        test_type = strtoull(optarg, NULL, 0);
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        len = strtoull(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_bmp = strtoull(optarg, NULL, 0);
        break;
      case 'm':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pattern = strtoull(optarg, NULL, 0);
        break;
      case 'a':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pattern_data0 = strtoull(optarg, NULL, 0);
        data0_specified = true;
        break;
      case 'b':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pattern_data1 = strtoull(optarg, NULL, 0);
        data1_specified = true;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES) ||
      (test_type > BF_DIAG_TEST_TYPE_DMA) || (len == -1) || (memory == -1)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (len == 0) {
    quick = true;
  } else {
    quick = false;
  }

  if ((pattern != BF_DIAG_TEST_PATTERN_USER_DEFINED) &&
      (data0_specified || data1_specified)) {
    aim_printf(
        &uc->pvs,
        "Data Pattern can be specified only with user-defined pattern type \n");
    return UCLI_STATUS_OK;
  }

  if ((pattern == BF_DIAG_TEST_PATTERN_USER_DEFINED) &&
      ((!data0_specified) || (!data1_specified))) {
    aim_printf(&uc->pvs,
               "Data Patterns (data0 and data1) need to be specified \n");
    return UCLI_STATUS_OK;
  }

  aim_printf(&uc->pvs, "Data Pattern type: ");
  if (pattern == BF_DIAG_TEST_PATTERN_RANDOM) {
    aim_printf(&uc->pvs, "Random \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_ZEROES) {
    aim_printf(&uc->pvs, "All Zeroes \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_ONES) {
    aim_printf(&uc->pvs, "All Ones \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_CHECKERBOARD) {
    aim_printf(&uc->pvs, "Checkerboard \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_INV_CHECKERBOARD) {
    aim_printf(&uc->pvs, "Inverse Checkerboard \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_PRBS) {
    aim_printf(&uc->pvs, "PRBS 7 \n");
  } else if (pattern == BF_DIAG_TEST_PATTERN_USER_DEFINED) {
    aim_printf(&uc->pvs,
               "User Defined (data0 : 0x%" PRIx64 " data1: 0x%" PRIx64 ") \n",
               pattern_data0,
               pattern_data1);
  } else {
    aim_printf(&uc->pvs, "Unknown type specified, Defaulting to Random \n");
    pattern = BF_DIAG_TEST_PATTERN_RANDOM;
  }

  if (memory == 0) {  // MAU
    aim_printf(&uc->pvs, "Running memory test on MAU\n");
    status = bf_diag_mem_test_mau(
        dev, test_type, quick, pipe_bmp, pattern, pattern_data0, pattern_data1);
  } else if (memory == 1) {  // PARDE
    aim_printf(&uc->pvs, "Running memory test on PARDE\n");
    status = bf_diag_mem_test_parde(
        dev, test_type, quick, pipe_bmp, pattern, pattern_data0, pattern_data1);
  } else if (memory == 2) {  // TM
    aim_printf(&uc->pvs, "Running memory test on TM\n");
    status = bf_diag_mem_test_tm(
        dev, test_type, quick, pipe_bmp, pattern, pattern_data0, pattern_data1);
  } else {
    aim_printf(&uc->pvs, "Invalid memory type \n");
    return UCLI_STATUS_OK;
  }

  if (status == BF_SUCCESS) {
    if (test_type == BF_DIAG_TEST_TYPE_PIO) {
      aim_printf(&uc->pvs, "Memory test result: PASSED \n");
    } else {
      aim_printf(&uc->pvs, "Memory test started successfully \n");
    }
  } else {
    if (test_type == BF_DIAG_TEST_TYPE_PIO) {
      aim_printf(&uc->pvs, "Memory test result: FAILED \n");
    } else {
      aim_printf(&uc->pvs, "Memory test start failed \n");
    }
  }

  return UCLI_STATUS_OK;
}

static inline bf_dev_pipe_t get_pipe_id_from_pipe_addr(
    bf_dev_family_t dev_family, uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_pipe_id_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_pipe_id_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_pipe_id_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return BF_INVALID_PIPE;
}
static inline int get_stage_id_from_pipe_addr(bf_dev_family_t dev_family,
                                              uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_stage_id_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_stage_id_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_stage_id_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return -1;
}
static inline pipe_mem_type_t get_mem_type_from_pipe_addr(
    bf_dev_family_t dev_family, uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_mem_type_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_mem_type_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_mem_type_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return pipe_mem_type_unit_ram;
}
static inline int get_row_from_pipe_addr(bf_dev_family_t dev_family,
                                         uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_row_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_row_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_row_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return -1;
}
static inline int get_col_from_pipe_addr(bf_dev_family_t dev_family,
                                         uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_col_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_col_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_col_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return -1;
}
static inline uint32_t get_mem_addr_from_pipe_addr(bf_dev_family_t dev_family,
                                                   uint64_t addr) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return tf1_get_mem_addr_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO2:
      return tf2_get_mem_addr_from_pipe_addr(addr);
    case BF_DEV_FAMILY_TOFINO3:
      return tf3_get_mem_addr_from_pipe_addr(addr);

    case BF_DEV_FAMILY_UNKNOWN:
      break;
  }
  bf_sys_assert(0);
  return 0xFFFFFFFF;
}

DEVDIAG_CLI_CMD_DECLARE(memtest_result) {
  DEVDIAG_CLI_PROLOGUE("memtest-result",
                       "Print result of last run memtest",
                       "Usage: -d <device>");

  bool pass = false;
  bf_dev_id_t dev = 0;
  bf_diag_mem_results_t results;
  memset(&results, 0, sizeof(results));

  int x;
  while (-1 != (x = getopt(argc, argv, "d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_diag_mem_test_result_get(dev, &results, &pass);
  if (pass) {
    aim_printf(&uc->pvs, "Memory test result: PASSED \n");
    aim_printf(&uc->pvs,
               "Num of DMA messages sent to chip: %d \n",
               results.num_dma_msgs_sent);
    aim_printf(&uc->pvs,
               "Num of DMA completions received from chip: %d \n",
               results.num_dma_cmplts_rcvd);
  } else {
    uint32_t index = 0;
    bf_diag_mem_data_error_t *ptr = NULL;
    aim_printf(&uc->pvs, "Memory test result: FAILED \n");
    aim_printf(&uc->pvs, "Details of errors: \n");
    aim_printf(&uc->pvs,
               "Indirect Read error: %s \n",
               results.ind_read_error ? "Yes" : "No");
    aim_printf(&uc->pvs,
               "Indirect Write error: %s \n",
               results.ind_write_error ? "Yes" : "No");
    aim_printf(&uc->pvs,
               "Write list error: %s \n",
               results.write_list_error ? "Yes" : "No");
    aim_printf(&uc->pvs,
               "Write block error: %s \n",
               results.write_block_error ? "Yes" : "No");
    aim_printf(&uc->pvs,
               "Num of DMA messages sent to chip: %d \n",
               results.num_dma_msgs_sent);
    aim_printf(&uc->pvs,
               "Num of DMA completions received from chip: %d \n",
               results.num_dma_cmplts_rcvd);
    aim_printf(&uc->pvs, "Num of data errors: %d \n", results.num_data_errors);

    bf_dev_family_t dev_family = BF_DEV_FAMILY_UNKNOWN;
    if (lld_dev_is_tofino(dev)) {
      dev_family = BF_DEV_FAMILY_TOFINO;
    } else if (lld_dev_is_tof2(dev)) {
      dev_family = BF_DEV_FAMILY_TOFINO2;
    } else if (lld_dev_is_tof3(dev)) {
      dev_family = BF_DEV_FAMILY_TOFINO3;
    }

    for (index = 0; (index < results.num_data_errors) &&
                    (index < BF_DIAG_MEM_MAX_DATA_ERR);
         index++) {
      ptr = &results.data_error[index];
      aim_printf(
          &uc->pvs,
          "Data Error[%u] : Addr: 0x%016" PRIx64
          "\n               : Pipe %d, Stage %d, mem-type %d, row %d, col "
          "%d, offset %d "
          "\n               : Exp:  %016" PRIx64 "_%016" PRIx64
          "\n               : Got:  %016" PRIx64 "_%016" PRIx64
          "\n               : Mask: %016" PRIx64 "_%016" PRIx64 "\n",
          index + 1,
          ptr->addr,
          get_pipe_id_from_pipe_addr(dev_family, ptr->addr),
          get_stage_id_from_pipe_addr(dev_family, ptr->addr),
          get_mem_type_from_pipe_addr(dev_family, ptr->addr),
          get_row_from_pipe_addr(dev_family, ptr->addr),
          get_col_from_pipe_addr(dev_family, ptr->addr),
          get_mem_addr_from_pipe_addr(dev_family, ptr->addr),
          ptr->exp_0,
          ptr->exp_1,
          ptr->data_0,
          ptr->data_1,
          ptr->mask_0,
          ptr->mask_1);
    }
  }

  return UCLI_STATUS_OK;
}

DEVDIAG_CLI_CMD_DECLARE(regtest) {
  DEVDIAG_CLI_PROLOGUE("regtest",
                       "Run register test on MAU/PARDE/TM",
                       "Usage: -d <device> -i <reg: 0:mau,1:parde,2:tm> -t "
                       "<test-type: 0:pio,1:dma> -l <len: 0:quick,1:ext> [-p "
                       "<log_pipe_bmp>]");

  bf_dev_id_t dev = 0;
  bool quick = false;
  int len = -1, memory = -1;
  bf_diag_test_type_t test_type = 100;
  bf_status_t status = BF_SUCCESS;
  uint32_t pipe_bmp = 0;
  char module_name[100];

  int x;
  while (-1 != (x = getopt(argc, argv, "d:s:t:l:i:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'i':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        memory = strtoull(optarg, NULL, 0);
        break;
      case 't':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        test_type = strtoull(optarg, NULL, 0);
        break;
      case 'l':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        len = strtoull(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_bmp = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES) ||
      (test_type > BF_DIAG_TEST_TYPE_DMA) || (len == -1) || (memory == -1)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (len == 0) {
    quick = true;
  } else {
    quick = false;
  }

  memset(module_name, 0, sizeof(module_name));
  if (memory == 0) {  // MAU
    snprintf(module_name, 100, "MAU");
    aim_printf(&uc->pvs, "Running register test on %s \n", module_name);
    status = bf_diag_reg_test_mau(dev, test_type, quick, pipe_bmp);
  } else if (memory == 1) {  // PARDE
    snprintf(module_name, 100, "PARDE");
    aim_printf(&uc->pvs, "Running register test on %s \n", module_name);
    status = bf_diag_reg_test_parde(dev, test_type, quick, pipe_bmp);
  } else if (memory == 2) {  // TM
    snprintf(module_name, 100, "TM");
    aim_printf(&uc->pvs, "Running register test on %s \n", module_name);
    status = bf_diag_reg_test_tm(dev, test_type, quick, pipe_bmp);
  } else {
    aim_printf(&uc->pvs, "Invalid register type \n");
    return UCLI_STATUS_OK;
  }

  if (status == BF_SUCCESS) {
    if (test_type == BF_DIAG_TEST_TYPE_PIO) {
      aim_printf(&uc->pvs, "%s register test passed \n", module_name);
    } else {
      aim_printf(
          &uc->pvs, "%s register test started successfully \n", module_name);
    }
  } else {
    if (test_type == BF_DIAG_TEST_TYPE_PIO) {
      aim_printf(&uc->pvs, "%s register test failed \n", module_name);
    } else {
      aim_printf(&uc->pvs, "%s register test start failed \n", module_name);
    }
  }

  return UCLI_STATUS_OK;
}

DEVDIAG_CLI_CMD_DECLARE(regtest_result) {
  DEVDIAG_CLI_PROLOGUE("regtest-result",
                       "Print result of last run regtest",
                       "Usage: -d <device>");

  bool pass = false;
  bf_dev_id_t dev = 0;
  bf_diag_reg_results_t results;
  memset(&results, 0, sizeof(results));

  int x;
  while (-1 != (x = getopt(argc, argv, "d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_diag_reg_test_result_get(dev, &results, &pass);
  if (pass) {
    aim_printf(&uc->pvs, "Register test result: PASSED \n");
    aim_printf(&uc->pvs,
               "Num of DMA messages sent to chip: %d \n",
               results.num_dma_msgs_sent);
    aim_printf(&uc->pvs,
               "Num of DMA completions received from chip: %d \n",
               results.num_dma_cmplts_rcvd);
  } else {
    uint32_t index = 0;
    bf_diag_reg_data_error_t *ptr = NULL;
    aim_printf(&uc->pvs, "Register test: FAILED \n");
    aim_printf(&uc->pvs, "Details of errors: \n");
    aim_printf(&uc->pvs,
               "Driver Register Read error: %s \n",
               results.reg_read_error ? "Yes" : "No");
    if (results.reg_read_error) {
      aim_printf(&uc->pvs,
                 "  Driver Register Read error address: %x \n",
                 results.reg_read_error_addr);
    }
    aim_printf(&uc->pvs,
               "Driver Register Write error: %s \n",
               results.reg_write_error ? "Yes" : "No");
    if (results.reg_write_error) {
      aim_printf(&uc->pvs,
                 "  Driver Register Write error address: %x \n",
                 results.reg_write_error_addr);
    }
    aim_printf(&uc->pvs,
               "Instruction List error: %s \n",
               results.ilist_error ? "Yes" : "No");

    aim_printf(&uc->pvs,
               "Num of DMA messages sent to chip: %d \n",
               results.num_dma_msgs_sent);
    aim_printf(&uc->pvs,
               "Num of DMA completions received from chip: %d \n",
               results.num_dma_cmplts_rcvd);

    aim_printf(&uc->pvs, "Num of data errors: %d \n", results.num_data_errors);
    for (index = 0; (index < results.num_data_errors) &&
                    (index < BF_DIAG_REG_MAX_DATA_ERR);
         index++) {
      ptr = &results.data_error[index];
      aim_printf(&uc->pvs,
                 "Data Error[%u] : Addr: 0x%x, Exp-Data %x, Read-Data %x \n",
                 index + 1,
                 ptr->addr,
                 ptr->exp_data,
                 ptr->read_data);
    }
  }

  return UCLI_STATUS_OK;
}

DEVDIAG_CLI_CMD_DECLARE(inttest) {
  DEVDIAG_CLI_PROLOGUE(
      "inttest", "Interrupt test", "Usage: -d <device> [-p <log_pipe_bmp>]");

  bf_dev_id_t dev = 0;
  bool pass = false;
  uint32_t pipe_bmp = 0;
  bf_diag_interrupt_results_t results;

  int x;
  while (-1 != (x = getopt(argc, argv, "d:p:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      case 'p':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        pipe_bmp = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  /* Validate the pipe_mask.  A mask of zero is okay, it means run all pipes.
   * Otherwise the mask must contain at least one pipe inside the legal range
   * for the device. */
  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  uint32_t all_pipes = (1 << num_pipes) - 1;
  if (pipe_bmp && !(all_pipes & pipe_bmp)) {
    aim_printf(&uc->pvs,
               "Illegal pipe-mask provided, 0x%x.  The bitmap of legal pipes "
               "on the device is 0x%x\n",
               pipe_bmp,
               all_pipes);
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  aim_printf(&uc->pvs, "Running Interrupt test \n");

  bf_diag_interrupt_test(dev, pipe_bmp);
  bf_diag_interrupt_test_result_get(dev, &results, &pass);
  if (pass) {
    aim_printf(&uc->pvs, "Interrupt test PASSED \n");
  } else {
    aim_printf(&uc->pvs, "Interrupt test FAILED \n");
  }

  return UCLI_STATUS_OK;
}

DEVDIAG_CLI_CMD_DECLARE(inttest_result) {
  DEVDIAG_CLI_PROLOGUE("inttest-result",
                       "Print result of last run interrupt test",
                       "Usage: -d <device>");

  bool pass = false;
  bf_dev_id_t dev = 0;
  bf_diag_interrupt_results_t results;
  memset(&results, 0, sizeof(results));

  int x;
  while (-1 != (x = getopt(argc, argv, "d:"))) {
    switch (x) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        dev = strtoull(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if ((dev < 0) || (dev >= DEVDIAG_NUM_DEVICES)) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  bf_diag_interrupt_test_result_get(dev, &results, &pass);
  if (pass) {
    aim_printf(&uc->pvs, "Interrupt test result: PASSED \n");
    aim_printf(&uc->pvs,
               "Interrupt test events expected: %u \n",
               results.num_int_events_exp);
    aim_printf(&uc->pvs,
               "Interrupt test events received: %u \n",
               results.num_int_events_rcvd);
  } else {
    uint32_t index = 0;
    bf_diag_interrupt_error_t *ptr = NULL;
    aim_printf(&uc->pvs, "Interrupt test result: FAILED \n");
    aim_printf(&uc->pvs,
               "Interrupt test events expected: %u \n",
               results.num_int_events_exp);
    aim_printf(&uc->pvs,
               "Interrupt test events received: %u \n",
               results.num_int_events_rcvd);
    aim_printf(
        &uc->pvs, "Num of interrupt errors: %d \n", results.num_int_errors);
    aim_printf(&uc->pvs, "Details of errors: \n");
    for (index = 0; (index < results.num_int_errors) &&
                    (index < BF_DIAG_INTERRUPT_ERR_MAX);
         index++) {
      ptr = &results.int_error[index];
      aim_printf(&uc->pvs,
                 "Interrupt Error[%u] : Reg name: %s \n"
                 "                    : Reg Address: 0x%x \n"
                 "                    : Exp  Reg Status Value: 0x%x \n"
                 "                    : Rcvd Reg Status Value: 0x%x \n",
                 index + 1,
                 ptr->reg_name,
                 ptr->addr,
                 ptr->exp_status,
                 ptr->rcvd_status);
    }
  }
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f devdiag_ucli_ucli_handlers__[] = {
    DEVDIAG_CLI_CMD_HNDLR(memtest),
    DEVDIAG_CLI_CMD_HNDLR(memtest_result),
    DEVDIAG_CLI_CMD_HNDLR(regtest),
    DEVDIAG_CLI_CMD_HNDLR(regtest_result),
    DEVDIAG_CLI_CMD_HNDLR(inttest),
    DEVDIAG_CLI_CMD_HNDLR(inttest_result),
    NULL};

static ucli_module_t devdiag_ucli_module__ = {
    "devdiag_ucli",
    NULL,
    devdiag_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *devdiag_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&devdiag_ucli_module__);
  n = ucli_node_create("devdiag", NULL, &devdiag_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("devdiag"));
  return n;
}

#else
void *devdiag_ucli_node_create(void) { return NULL; }
#endif
