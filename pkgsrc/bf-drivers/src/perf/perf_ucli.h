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
 * @file perf_ucli.h
 * @date
 *
 * Performance utils handling common definitions.
 */

#ifndef _PERF_UCLI_H
#define _PERF_UCLI_H

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
                       double results[][num_cols]);

/**
 * @brief Print desired string as a banner surrounded by asterisks
 *
 * @param uc ucli context pointer
 * @param string String to be printed
 */
void banner(ucli_context_t *uc, char *string);

/**
 * @brief Print tofino sku, information about model/HW and versions of the
 * subcomponents
 * @param uc ucli context pointer
 * @param dev_id device id
 */
void print_versions(ucli_context_t *uc, bf_dev_id_t dev_id);

#endif
