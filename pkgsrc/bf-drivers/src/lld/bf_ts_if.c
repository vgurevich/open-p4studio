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
 * @file bf_ts_if.c
 * @date
 *
 */

/**
 * @addtogroup lld-ts-api
 * @{
 * This is a description of Timestamp APIs.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>
#include <string.h>
#include <time.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/bf_ts_if.h>
#include "lld_dev.h"
#include "lld_tof_ts.h"
#include "lld_tof2_ts.h"
#include "lld_tof3_ts.h"

/**
 * @brief bf_ts_global_ts_state_set
 *  Enable global timestamp function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable or disable timestamp function
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_state_set(bf_dev_id_t dev_id, bool enable) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_state_set(dev_id, enable));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_state_set(dev_id, enable));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_state_set(dev_id, enable));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_state_get
 *  Get the global timestamp enable state
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable: bool
 *   enable state of global timestamp function
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_state_get(bf_dev_id_t dev_id, bool *enable) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_state_get(dev_id, enable));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_state_get(dev_id, enable));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_state_get(dev_id, enable));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_value_set
 *  Configure the global timestamp counter
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_value_set(bf_dev_id_t dev_id,
                                      uint64_t global_ts_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_value_set(dev_id, global_ts_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_value_set(dev_id, global_ts_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_value_set(dev_id, global_ts_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_value_get
 *  Get the global timestamp counter
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_value_get(bf_dev_id_t dev_id,
                                      uint64_t *global_ts_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_value_get(dev_id, global_ts_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_value_get(dev_id, global_ts_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_value_get(dev_id, global_ts_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_inc_value_set
 *  Configure the global timestamp counter inc value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_inc_value_set(bf_dev_id_t dev_id,
                                          uint32_t global_inc_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_inc_value_set(dev_id, global_inc_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_inc_value_set(dev_id, global_inc_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_inc_value_set(dev_id, global_inc_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_inc_value_get
 *  get the global timestamp counter inc value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64
 *   Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_inc_value_get(bf_dev_id_t dev_id,
                                          uint32_t *global_inc_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_inc_value_get(dev_id, global_inc_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_inc_value_get(dev_id, global_inc_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_inc_value_get(dev_id, global_inc_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_increment_one_time_set
 *  Increment Global timestamp register one-time
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_inc_time_ns:  int64
 *   Value to increment Global timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_increment_one_time_set(
    bf_dev_id_t dev_id, uint64_t global_ts_inc_time_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_increment_one_time_set(dev_id,
                                                        global_ts_inc_time_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_increment_one_time_set(
        dev_id, global_ts_inc_time_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_increment_one_time_set(
        dev_id, global_ts_inc_time_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_offset_set
 *  Set global timestamp offset value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_offset_ns:  int64
 *   Value to set for Global timestamp offset value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_offset_set(bf_dev_id_t dev_id,
                                       uint64_t global_ts_offset_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_offset_set(dev_id, global_ts_offset_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_offset_set(dev_id, global_ts_offset_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_offset_set(dev_id, global_ts_offset_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_ts_offset_get
 *  Get global timestamp offset value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_offset_ns:  int64
 *   Value to set for Global timestamp offset value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_ts_offset_get(bf_dev_id_t dev_id,
                                       uint64_t *global_ts_offset_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_ts_offset_get(dev_id, global_ts_offset_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_ts_offset_get(dev_id, global_ts_offset_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_ts_offset_get(dev_id, global_ts_offset_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_global_baresync_ts_get
 *  Trigger and retrieve both global timestamp value and baresync timestamp
 *value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param global_ts_ns:  int64 ptr
 *   Pointer to buffer to store global timestamp value in ns
 *
 * @param baresync_ts_ns:  int64 ptr
 *   Pointer to buffer to store baresync timestamp value in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_global_baresync_ts_get(bf_dev_id_t dev_id,
                                         uint64_t *global_ts_ns,
                                         uint64_t *baresync_ts_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_global_baresync_ts_get(
        dev_id, global_ts_ns, baresync_ts_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_global_baresync_ts_get(
        dev_id, global_ts_ns, baresync_ts_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_global_baresync_ts_get(
        dev_id, global_ts_ns, baresync_ts_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

#if 0
// API to read the eTS out of egress MAC?s FIFO

typedef bf_port_egress_ts_s {
    bool     *ts_valid;
    int      *ts_id; // 1..4+
    uint64_t *ts;
} bf_port_egress_ts_t;

bf_status_t bf_port_pop_from_egress_ts_fifo ( bf_dev_id_t dev_id, bf_dev_port_t egress_port, bf_port_egress_ts_t *egress_ts)

// API to set TxTsDelta offset that can be used to  accommodate the fixed delays incurred in the Serdes/PHY before capturing eTS to the FIFO that is triggered by eg_intr_md_for_oport.capture_tstamp_on_tx being set to 1

bf_status_t bf_ts_port_egress_ts_tx_delta_set( bf_dev_id_t chip, bf_dev_port_t port, int delta);

// API to set RxTsDelta offset that can be used to  accommodate the fixed delays incurred in the Serdes/PHY before capturing iTS metadata to the pipeline

bf_status_t bf_ts_port_ingress_ts_rx_delta_set( bf_dev_id_t chip, bf_dev_port_t port, int delta);

// Optional API for debug

// API to set per clock Global_Timestamp increment value (ts_inc_value: 4-bit ns and 28-bit fractional ns in units of 2^-28 ns). Mainly for debug purposes as is initialized by the driver depending on the clock speed of the SKU

bf_status_t bf_ts_global_ts_per_clk_increment_set( bf_dev_id_t dev_id,
uint8_t global_ts_inc_time_ns, // range <0..15>
uint32_t global_ts_inc_time_fract_ns);

// API to set the frequency at which Global_Timestamp is distributed to Tofino blocks (ts_timer). Driver initialized it to 1us value as recommended by the HW team.

bf_status_t bf_ts_global_ts_periodic_distribution_timer_set( bf_dev_id_t dev_id, uint32_t timer_ns) {
}
#endif

/* Baresync related API */

/**
 * @brief bf_ts_baresync_state_set
 *  Enable baresync function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable or disable baresync function
 *
 * @param reset_count_threshold: uint32_t
 *   reset count threshold value
 *
 * @param debounce_count: uint32_t
 *   debounce count value
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_state_set(bf_dev_id_t dev_id,
                                     uint32_t reset_count_threshold,
                                     uint32_t debounce_count,
                                     bool enable) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_state_set(
        dev_id, reset_count_threshold, debounce_count, enable));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_state_set(
        dev_id, reset_count_threshold, debounce_count, enable));
#if 0
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_baresync_state_set(
        dev_id, reset_count_threshold, debounce_count, enable));
#endif
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_baresync_state_get
 *  get the configuration of baresync function
 *
 * @param dev_id: int
 *  chip id
 *
 * @param enable:  bool
 *   Enable state of baresync function
 *
 * @param reset_count_threshold: uint32_t
 *   reset count threshold value
 *
 * @param debounce_count: uint32_t
 *   debounce count value
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_state_get(bf_dev_id_t dev_id,
                                     uint32_t *reset_count_threshold,
                                     uint32_t *debounce_count,
                                     bool *enable) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_state_get(
        dev_id, reset_count_threshold, debounce_count, enable));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_state_get(
        dev_id, reset_count_threshold, debounce_count, enable));
#if 0
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_baresync_state_get(
        dev_id, reset_count_threshold, debounce_count, enable));
#endif
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_baresync_reset_value_set
 *  Set the value to be written into baresync timestamp register on reset
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_time_ns:  int64
 *   Value to be written into baresync register on reset in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_reset_value_set(bf_dev_id_t dev_id,
                                           uint64_t baresync_time_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_reset_value_set(dev_id, baresync_time_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_reset_value_set(dev_id, baresync_time_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_baresync_reset_value_set(dev_id, baresync_time_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_baresync_reset_value_get
 *  Get the value of baresync timestamp register on reset
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_time_ns:  int64
 *   Value of the baresync register on reset in ns
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_reset_value_get(bf_dev_id_t dev_id,
                                           uint64_t *baresync_time_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_reset_value_get(dev_id, baresync_time_ns));
  } else if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_reset_value_get(dev_id, baresync_time_ns));
  } else if (lld_dev_is_tof3(dev_id)) {
    return (lld_tof3_ts_baresync_reset_value_get(dev_id, baresync_time_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_baresync_increment_set
 *  Set the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_increment_set(bf_dev_id_t dev_id,
                                         uint32_t baresync_inc_time_ns,
                                         uint32_t baresync_inc_time_fract_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_increment_set(
        dev_id, baresync_inc_time_ns, baresync_inc_time_fract_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_ts_baresync_increment_get
 *  Get the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_ts_baresync_increment_get(bf_dev_id_t dev_id,
                                         uint32_t *baresync_inc_time_ns,
                                         uint32_t *baresync_inc_time_fract_ns) {
  if (lld_dev_is_tofino(dev_id)) {
    return (lld_tof_ts_baresync_increment_get(
        dev_id, baresync_inc_time_ns, baresync_inc_time_fract_ns));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_tof2_ts_baresync_increment_set
 *  Set the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @param baresync_inc_time_fract_den:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_tof2_ts_baresync_increment_set(
    bf_dev_id_t dev_id,
    uint32_t baresync_inc_time_ns,
    uint32_t baresync_inc_time_fract_ns,
    uint32_t baresync_inc_time_fract_den) {
  if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_increment_set(dev_id,
                                               baresync_inc_time_ns,
                                               baresync_inc_time_fract_ns,
                                               baresync_inc_time_fract_den));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @brief bf_tof2_ts_baresync_increment_get
 *  Get the baresync increment value
 *
 * @param dev_id: int
 *  chip id
 *
 * @param baresync_inc_time_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 20-bit ns)
 *
 * @param baresync_inc_time_fract_ns:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @param baresync_inc_time_fract_den:  int32
 *   baresync increment value (baresync_ts_inc_value: 28-bit fractional ns in
 *units of 2^-28 ns)
 *
 * @return status
 *   BF_SUCCESS on success
 *   BF ERROR code on failure
 *
 */

bf_status_t bf_tof2_ts_baresync_increment_get(
    bf_dev_id_t dev_id,
    uint32_t *baresync_inc_time_ns,
    uint32_t *baresync_inc_time_fract_ns,
    uint32_t *baresync_inc_time_fract_den) {
  if (lld_dev_is_tof2(dev_id)) {
    return (lld_tof2_ts_baresync_increment_get(dev_id,
                                               baresync_inc_time_ns,
                                               baresync_inc_time_fract_ns,
                                               baresync_inc_time_fract_den));
  } else {
    return BF_UNEXPECTED;
  }
}

/**
 * @}
 */
