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


#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_spi_if.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/lld_interrupt_if.h>
#include <lld/tofino_defs.h>
#include <lld/lld_diag.h>
#include <diag/bf_dev_diag.h>
#include "diag_internal.h"

bf_status_t bf_diag_mem_test_mau(bf_dev_id_t dev_id,
                                 bf_diag_test_type_t test_type,
                                 bool quick,
                                 uint32_t pipe_bmp,
                                 bf_diag_test_pattern_t pattern,
                                 uint64_t pattern_data0,
                                 uint64_t pattern_data1) {
  bf_status_t status = BF_SUCCESS;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(
      pipe_bmp, 0xfffff, quick, false, pattern, pattern_data0, pattern_data1);
  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("MAU: pio test \n");
    status = lld_traverse_mau_memories(dev_id);
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("MAU: dma test \n");
    status = lld_traverse_mau_memories_dma(dev_id);
  }
  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_mem_test_parde(bf_dev_id_t dev_id,
                                   bf_diag_test_type_t test_type,
                                   bool quick,
                                   uint32_t pipe_bmp,
                                   bf_diag_test_pattern_t pattern,
                                   uint64_t pattern_data0,
                                   uint64_t pattern_data1) {
  bf_status_t status = BF_SUCCESS;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(
      pipe_bmp, 0, quick, false, pattern, pattern_data0, pattern_data1);
  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("Parde: pio test \n");
    status = lld_traverse_all_mems(dev_id, lld_mem_test_cb, false, false);
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("Parde: dma test \n");
    status = lld_traverse_all_mems(dev_id, lld_mem_test_dma_cb, false, false);
    /* Wait for DMA callbacks to finish */
    bf_sys_usleep(6000000);
  }
  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_mem_test_tm(bf_dev_id_t dev_id,
                                bf_diag_test_type_t test_type,
                                bool quick,
                                uint32_t pipe_bmp,
                                bf_diag_test_pattern_t pattern,
                                uint64_t pattern_data0,
                                uint64_t pattern_data1) {
  bf_status_t status = BF_SUCCESS;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(
      pipe_bmp, 0, quick, true, pattern, pattern_data0, pattern_data1);

  // Disabling sch internal memory refresh rate
  lld_mem_test_sch_refresh_enable(dev_id, false);

  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("TM: pio test \n");
    status = lld_traverse_all_mems(dev_id, lld_mem_test_cb, false, true);
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("TM: dma test \n");
    status = lld_traverse_all_mems(dev_id, lld_mem_test_dma_cb, false, true);
    /* Wait for DMA callbacks to finish */
    bf_sys_usleep(6000000);
  }
  // Enabling sch internal memory refresh rate
  lld_mem_test_sch_refresh_enable(dev_id, true);

  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_mem_test_result_get(bf_dev_id_t dev_id,
                                        bf_diag_mem_results_t *results,
                                        bool *pass) {
  bf_diag_mem_results_t *res = NULL;

  res = (bf_diag_mem_results_t *)lld_memtest_results_get(dev_id);
  memcpy(results, res, sizeof(bf_diag_mem_results_t));
  *pass = res->overall_success;
  return BF_SUCCESS;
}

bf_status_t bf_diag_reg_test_mau(bf_dev_id_t dev_id,
                                 bf_diag_test_type_t test_type,
                                 bool quick,
                                 uint32_t pipe_bmp) {
  bf_status_t status = BF_SUCCESS;
  bool dma = false;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(pipe_bmp, 0xffff, quick, false, 0, 0, 0);
  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("MAU: pio test \n");
    dma = false;
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("MAU: dma test \n");
    dma = true;
  }
  status = lld_diag_reg_test_mau(dev_id, quick, dma);
  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_reg_test_parde(bf_dev_id_t dev_id,
                                   bf_diag_test_type_t test_type,
                                   bool quick,
                                   uint32_t pipe_bmp) {
  bf_status_t status = BF_SUCCESS;
  bool dma = false;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(pipe_bmp, 0xffff, quick, false, 0, 0, 0);
  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("Parde: pio test \n");
    dma = false;
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("Parde: dma test \n");
    dma = true;
  }
  status = lld_diag_reg_test_parde(dev_id, quick, dma);
  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_reg_test_tm(bf_dev_id_t dev_id,
                                bf_diag_test_type_t test_type,
                                bool quick,
                                uint32_t pipe_bmp) {
  bf_status_t status = BF_SUCCESS;
  bool dma = false;

  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }
  lld_diag_cfg_set(pipe_bmp, 0xffff, quick, true, 0, 0, 0);
  if (test_type == BF_DIAG_TEST_TYPE_PIO) {
    printf("TM: pio test \n");
    dma = false;
  } else if (test_type == BF_DIAG_TEST_TYPE_DMA) {
    printf("TM: dma test \n");
    dma = true;
  }
  status = lld_diag_reg_test_tm(dev_id, quick, dma);
  lld_diag_cfg_reset();
  return status;
}

bf_status_t bf_diag_reg_test_result_get(bf_dev_id_t dev_id,
                                        bf_diag_reg_results_t *results,
                                        bool *pass) {
  bf_diag_reg_results_t *res = NULL;

  res = (bf_diag_reg_results_t *)lld_regtest_results_get(dev_id);
  memcpy(results, res, sizeof(bf_diag_reg_results_t));
  *pass = res->overall_success;
  return BF_SUCCESS;
}

/* Test interrupts */
bf_status_t bf_diag_interrupt_test(bf_dev_id_t dev_id, uint32_t pipe_bmp) {
  if (pipe_bmp == 0) {
    uint32_t num_pipes = 0, i = 0;
    lld_sku_get_num_active_pipes(dev_id, &num_pipes);
    for (i = 0; i < num_pipes; i++) {
      pipe_bmp |= (0x1u << i);
    }
  }

  return lld_interrupt_test_quick(dev_id, pipe_bmp);
}

bf_status_t bf_diag_interrupt_test_result_get(
    bf_dev_id_t dev_id, bf_diag_interrupt_results_t *results, bool *pass) {
  bf_diag_interrupt_results_t *res = NULL;
  (void)dev_id;

  res = (bf_diag_interrupt_results_t *)lld_inttest_results_get(dev_id);
  memcpy(results, res, sizeof(bf_diag_interrupt_results_t));
  *pass = res->overall_success;
  return BF_SUCCESS;
}
