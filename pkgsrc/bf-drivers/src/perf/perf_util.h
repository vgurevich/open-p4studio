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
 * @file perf_util.h
 * @date
 *
 * Performance utils handling definitions.
 */

#ifndef _PERF_UTIL_H
#define _PERF_UTIL_H

/**
 * @brief Count average of elements in an array
 *
 * @param array array of elements to be counted
 * @param elements number of elements in the array
 * @return double average value of the elements in the array
 */
double average(double array[], int elements);

/**
 * @brief Count average value and standard deviation of the elements in an array
 *
 * @param array array of values to be counted
 * @param elements number of elements in the array
 * @param avg pointer for result average value
 * @param sd pointer for result standard deviation value
 */
void basic_stats(double array[], int elements, double *avg, double *sd);

/**
 * @brief Calculates the difference between two timestamps
 *
 * @param start Start time
 * @param stop End time
 * @return time in nanoseconds
 */
uint64_t time_delta_ns(struct timespec start, struct timespec stop);

/**
 * @brief Counts rates based on the timestamps and number of operations
 *
 * @param start Start timestamp
 * @param stop End timestamp
 * @param operations number of operations performed
 * @param op_per_s rate in operations per second
 * @param ns_per_op nanoseconds per operations
 * @return bool
 */
bool ts_to_ops(struct timespec start,
               struct timespec stop,
               int operations,
               double *op_per_s,
               double *ns_per_op);

/**
 * @brief Counts rates based on the timestamps and number of bytes
 *
 * @param start Start timestamp
 * @param stop End timestamp
 * @param bytes number of bytes
 * @param mb_per_s rate in megabytes per second
 * @param us_per_mb microseconds per megabyte
 * @return bool
 */
bool ts_to_mb(struct timespec start,
              struct timespec stop,
              int bytes,
              double *mb_per_s,
              double *us_per_mb);

#endif
