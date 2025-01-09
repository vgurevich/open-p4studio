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


/*!
 * @file perf_mem_intf.h
 * @date
 *
 * Performance memory handling definitions.
 */

#ifndef _PERF_MEM_INTF_H
#define _PERF_MEM_INTF_H

extern struct test_description sram_dma_test;
extern struct test_description tcam_dma_test;

enum mem_int_res { RES_PIPES, RES_MAUS, RES_ROWS, RES_COLS };

enum mem_double_res { RES_WRITE_MB, RES_WRITE_US, RES_READ_MB, RES_READ_US };

/**
 * @brief Run performance test that will write/read to/from SRAM memory,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param num_pipes Number of PIPEs
 * @param num_maus Number of MAUs
 * @param num_rows Number of ROWs
 * @param num_cols Number of COLUMNs
 * @return test_results
 */
struct test_results sram_dma(bf_dev_id_t dev_id,
                             int num_pipes,
                             int num_maus,
                             int num_rows,
                             int num_cols);

/**
 * @brief Run performance test that will write/read to/from TCAM memory,
 * and calculate the rate.
 *
 * @param dev_id device id
 * @param num_pipes Number of PIPEs
 * @param num_maus Number of MAUs
 * @param num_rows Number of ROWs
 * @param num_cols Number of COLUMNs
 * @return test_results
 */
struct test_results tcam_dma(bf_dev_id_t dev_id,
                             int num_pipes,
                             int num_maus,
                             int num_rows,
                             int num_cols);

#endif
