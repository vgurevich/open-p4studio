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
 * @file perf_registers.h
 * @date
 *
 * Performance registers handling common definitions.
 */

#ifndef _PERF_REGISTERS_H
#define _PERF_REGISTERS_H

#define PERF_REG_ITERS 100000

typedef struct reg_entry {
  uint64_t addr;
  char name[10];
} reg_entry_t;

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
                         struct register_result *result);
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
                            bool indirect);

#endif
