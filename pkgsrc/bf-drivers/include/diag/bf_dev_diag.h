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


#ifndef BF_DEV_DIAG_H_INCLUDED
#define BF_DEV_DIAG_H_INCLUDED

typedef enum bf_diag_test_type_ {
  BF_DIAG_TEST_TYPE_PIO,
  BF_DIAG_TEST_TYPE_DMA,
} bf_diag_test_type_t;

typedef enum bf_diag_test_pattern_ {
  BF_DIAG_TEST_PATTERN_RANDOM,
  BF_DIAG_TEST_PATTERN_ZEROES,
  BF_DIAG_TEST_PATTERN_ONES,
  BF_DIAG_TEST_PATTERN_CHECKERBOARD,
  BF_DIAG_TEST_PATTERN_INV_CHECKERBOARD,
  BF_DIAG_TEST_PATTERN_PRBS,
  BF_DIAG_TEST_PATTERN_USER_DEFINED,
} bf_diag_test_pattern_t;

/* Diag Memory test results */
typedef struct bf_diag_mem_data_error_ {
  uint64_t addr;
  uint64_t exp_0;
  uint64_t exp_1;
  uint64_t data_0;
  uint64_t data_1;
  uint64_t mask_0;
  uint64_t mask_1;
} bf_diag_mem_data_error_t;

#define BF_DIAG_MEM_MAX_DATA_ERR 400
typedef struct bf_diag_mem_results_ {
  bool overall_success;
  bool ind_write_error;
  bool ind_read_error;
  bool write_list_error;
  bool write_block_error;
  uint64_t ind_write_error_addr;
  uint64_t ind_read_error_addr;
  uint32_t num_data_errors;
  bf_diag_mem_data_error_t data_error[BF_DIAG_MEM_MAX_DATA_ERR];
  uint32_t num_dma_msgs_sent;
  uint32_t num_dma_cmplts_rcvd;
} bf_diag_mem_results_t;

/* Diag register test results */
typedef struct bf_diag_reg_data_error_ {
  uint32_t addr;
  uint32_t exp_data;
  uint32_t read_data;
} bf_diag_reg_data_error_t;

#define BF_DIAG_REG_MAX_DATA_ERR 400
typedef struct bf_diag_reg_results_ {
  bool overall_success;
  bool reg_write_error;
  bool reg_read_error;
  bool ilist_error;
  uint32_t reg_write_error_addr;
  uint32_t reg_read_error_addr;
  uint32_t num_data_errors;
  bf_diag_reg_data_error_t data_error[BF_DIAG_REG_MAX_DATA_ERR];
  uint32_t num_dma_msgs_sent;
  uint32_t num_dma_cmplts_rcvd;
} bf_diag_reg_results_t;

/* Diag interrupt test results */
#define LLD_INT_RES_REG_NAME_LEN 140
typedef struct bf_diag_interrupt_error_ {
  char reg_name[LLD_INT_RES_REG_NAME_LEN];
  uint32_t addr;
  uint32_t exp_status;
  uint32_t rcvd_status;
} bf_diag_interrupt_error_t;

#define BF_DIAG_INTERRUPT_ERR_MAX 400
typedef struct bf_diag_interrupt_results_ {
  bool overall_success;
  uint32_t num_int_errors;
  uint32_t num_int_events_exp;
  uint32_t num_int_events_rcvd;
  bf_diag_interrupt_error_t int_error[BF_DIAG_INTERRUPT_ERR_MAX];
} bf_diag_interrupt_results_t;

bf_status_t bf_diag_mem_test_mau(bf_dev_id_t dev_id,
                                 bf_diag_test_type_t test_type,
                                 bool quick,
                                 uint32_t pipe_bmp,
                                 bf_diag_test_pattern_t pattern,
                                 uint64_t pattern_data0,
                                 uint64_t pattern_data1);
bf_status_t bf_diag_mem_test_parde(bf_dev_id_t dev_id,
                                   bf_diag_test_type_t test_type,
                                   bool quick,
                                   uint32_t pipe_bmp,
                                   bf_diag_test_pattern_t pattern,
                                   uint64_t pattern_data0,
                                   uint64_t pattern_data1);
bf_status_t bf_diag_mem_test_tm(bf_dev_id_t dev_id,
                                bf_diag_test_type_t test_type,
                                bool quick,
                                uint32_t pipe_bmp,
                                bf_diag_test_pattern_t pattern,
                                uint64_t pattern_data0,
                                uint64_t pattern_data1);
bf_status_t bf_diag_mem_test_result_get(bf_dev_id_t dev_id,
                                        bf_diag_mem_results_t *results,
                                        bool *pass);
bf_status_t bf_diag_reg_test_mau(bf_dev_id_t dev_id,
                                 bf_diag_test_type_t test_type,
                                 bool quick,
                                 uint32_t pipe_bmp);
bf_status_t bf_diag_reg_test_parde(bf_dev_id_t dev_id,
                                   bf_diag_test_type_t test_type,
                                   bool quick,
                                   uint32_t pipe_bmp);
bf_status_t bf_diag_reg_test_tm(bf_dev_id_t dev_id,
                                bf_diag_test_type_t test_type,
                                bool quick,
                                uint32_t pipe_bmp);
bf_status_t bf_diag_reg_test_result_get(bf_dev_id_t dev_id,
                                        bf_diag_reg_results_t *results,
                                        bool *pass);
bf_status_t bf_diag_interrupt_test(bf_dev_id_t dev_id, uint32_t pipe_bmp);

bf_status_t bf_diag_interrupt_test_result_get(
    bf_dev_id_t dev_id, bf_diag_interrupt_results_t *results, bool *pass);

#endif  // BF_DEV_DIAG_H_INCLUDED
