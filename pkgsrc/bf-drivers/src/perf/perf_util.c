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


#include <errno.h>
#include <math.h>

#include <target-utils/uCli/ucli.h>
#include <bfutils/bf_utils.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_dev.h>
#include <lld/lld_sku.h>

#include "perf_util.h"

/**
 * @brief Count average of elements in an array
 *
 * @param array array of elements to be counted
 * @param elements number of elements in the array
 * @return double average value of the elements in the array
 */
double average(double array[], int elements) {
  if (elements < 1) {
    bf_sys_dbgchk(0);
    return 0;
  }
  double sum = 0;
  for (int i = 0; i < elements; i++) {
    sum = sum + array[i];
  }
  return sum / (double)elements;
}

/**
 * @brief Count average value and standard deviation of the elements in an array
 *
 * @param array array of values to be counted
 * @param elements number of elements in the array
 * @param avg pointer for result average value
 * @param sd pointer for result standard deviation value
 */
void basic_stats(double array[], int elements, double *avg, double *sd) {
  if (elements < 1) {
    bf_sys_dbgchk(0);
    return;
  }
  double mid = 0;
  *avg = average(array, elements);
  for (int i = 0; i < elements; i++) {
    mid += pow(array[i] - *avg, 2);
  }
  *sd = sqrt(mid / elements);
}

/**
 * @brief Calculates the difference between two timestamps
 *
 * @param start Start time
 * @param stop End time
 * @return time in nanoseconds
 */
uint64_t time_delta_ns(struct timespec start, struct timespec stop) {
  struct timespec delta;
  if (stop.tv_nsec < start.tv_nsec) {
    stop.tv_sec -= 1;
    stop.tv_nsec += 1000000000;
  }
  delta.tv_nsec = stop.tv_nsec - start.tv_nsec;
  delta.tv_sec = stop.tv_sec - start.tv_sec;
  while (delta.tv_sec > 0) {
    delta.tv_nsec += 1000000000;
    delta.tv_sec--;
  }
  return delta.tv_nsec;
}

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
               double *ns_per_op) {
  uint64_t nsec = time_delta_ns(start, stop);
  *op_per_s = *ns_per_op = 0;

  if (nsec <= 0 || operations <= 0) {
    return false;
  }

  *op_per_s = (double)1000000000 / (double)nsec * (double)operations;
  *ns_per_op = (double)nsec / (double)operations;

  return true;
}

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
              double *us_per_mb) {
  uint64_t nsec = time_delta_ns(start, stop);
  *us_per_mb = *mb_per_s = 0;

  if (nsec <= 0 || bytes <= 0) {
    return false;
  }

  *us_per_mb =
      (double)1024 * (double)1024 / (double)1000 * nsec / (double)bytes;
  *mb_per_s = (double)1000000 / *us_per_mb;

  return true;
}
