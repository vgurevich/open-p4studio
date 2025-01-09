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
 * @file perf_int.h
 * @date
 *
 * Performance interrupts handling common definitions.
 */

#ifndef _PERF_INT_H
#define _PERF_INT_H

/* Interrupt tests callback info */
#define PERF_INT_CACHE_MAX 1000
#define PERF_INT_ITERS 300

struct interrupts_result {
  bool status;
  perf_bus_t_enum bus_type;
  int interrupts;
  int iterations;
  double avg_latency_us;
  double sd_latency_us;
  double min_latency_us;
  double max_latency_us;
};

typedef struct perf_int_cache {
  bool valid;
  lld_int_cb cb_fn;
  void *userdata;
  uint32_t status_addr;
  uint32_t status_val;
  uint32_t exp_int_status_val;
  uint32_t enable_addr;
  uint32_t enable_val;
  uint32_t inject_addr;
  struct timespec start;
  struct timespec stop;
  bool callback_received;
} perf_int_cache_t;

/**
 * @brief Return whether interrupts are support for a given bus type
 *
 * @param dev_id device id
 * @param bus_type bus type
 * @return bool
 */
bool is_bus_ints_supported(bf_dev_id_t dev_id, perf_bus_t_enum bus_type);

/**
 * @brief Run performance test that will measure latency of
 * interrupts processing
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @param it number of iterations
 * @return bf_status_t
 */
bf_status_t run_int_test(bf_dev_id_t dev_id,
                         perf_bus_t_enum bus_type,
                         int it,
                         struct interrupts_result *result);

/**
 * @brief Run performance test that will measure latency of
 * interrupts processing
 *
 * @param uc ucli context pointer
 * @param dev_id device id
 * @return bf_status_t
 */
bf_status_t run_interrupts(ucli_context_t *uc, bf_dev_id_t dev_id);

#endif
