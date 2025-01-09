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
 * @file perf_int_intf.h
 * @date
 *
 * Performance interrupts handling definitions.
 */

#ifndef _PERF_INT_INTF_H
#define _PERF_INT_INTF_H

extern struct test_description interrupts_test;

enum interrupts_enum_res { RES_INT_BUS };
enum interrupts_int_res { RES_INTERRUPTS, RES_ITERATIONS };
enum interrupts_double_res {
  RES_LAT_AVG,
  RES_LAT_SD,
  RES_LAT_MIN,
  RES_LAT_MAX
};

/**
 * @brief Run performance test that will measure latency of the interrupts
 * processing
 *
 * @param dev_id device id
 * @param bus_type Bus type
 * @param it number of iterations
 * @return interrupts_result
 */
struct test_results interrupts(bf_dev_id_t dev_id,
                               perf_bus_t_enum bus_type,
                               int it);

#endif
