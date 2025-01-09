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
 * @file perf_mem.h
 * @date
 *
 * Performance memory handling definitions.
 */

#ifndef _PERF_MEM_H
#define _PERF_MEM_H

/**
 * @brief Run performance test that will write/read to/from
 * SRAM/TCAM memory, calculate the rate
 *
 * @param dev_id Device id
 * @param mem_type Memory type
 * @param pipes Number of PIPEs
 * @param maus Number of MAUs
 * @param rows Number of ROWs
 * @param cols Number of COLUMNs
 * @param result pointer to struct with results
 * @return ucli_status_t
 */
bf_status_t run_mem_test(bf_dev_id_t dev_id,
                         pipe_mem_type_t mem_type,
                         int pipes,
                         int maus,
                         int rows,
                         int cols,
                         struct test_results *result);

/**
 * @brief Run performance test that will write/read to/from SRAM memory,
 * and calculate the rate.
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return ucli_status_t
 */
ucli_status_t run_sram_dma(ucli_context_t *uc, bf_dev_id_t dev_id);

/**
 * @brief Run performance test that will write/read to/from TCAM memory,
 * and calculate the rate.
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return ucli_status_t
 */
ucli_status_t run_tcam_dma(ucli_context_t *uc, bf_dev_id_t dev_id);

#endif
