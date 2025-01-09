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
 *  @file lld_tof2_ts.h
 *  @date
 *
 */

#ifndef __LLD_TOF2_TS_H_
#define __LLD_TOF2_TS_H_

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

bf_status_t lld_tof2_ts_global_ts_state_set(bf_dev_id_t dev_id, bool enable);
bf_status_t lld_tof2_ts_global_ts_state_get(bf_dev_id_t dev_id, bool *enable);
bf_status_t lld_tof2_ts_global_ts_value_set(bf_dev_id_t dev_id,
                                            uint64_t global_ts_ns);
bf_status_t lld_tof2_ts_global_ts_value_get(bf_dev_id_t dev_id,
                                            uint64_t *global_ts_ns);
bf_status_t lld_tof2_ts_global_ts_inc_value_set(bf_dev_id_t dev_id,
                                                uint32_t global_inc_ns);
bf_status_t lld_tof2_ts_global_ts_inc_value_get(bf_dev_id_t dev_id,
                                                uint32_t *global_inc_ns);

bf_status_t lld_tof2_ts_global_ts_increment_one_time_set(
    bf_dev_id_t dev_id, uint64_t global_ts_inc_time_ns);
bf_status_t lld_tof2_ts_global_ts_offset_set(bf_dev_id_t dev_id,
                                             uint64_t global_ts_offset_ns);
bf_status_t lld_tof2_ts_global_ts_offset_get(bf_dev_id_t dev_id,
                                             uint64_t *global_ts_offset_ns);
bf_status_t lld_tof2_ts_global_baresync_ts_get(bf_dev_id_t dev_id,
                                               uint64_t *global_ts_ns,
                                               uint64_t *baresync_ts_ns);
// bf_status_t lld_tof2_ts_global_ts_periodic_distribution_timer_set(bf_dev_id_t
// dev_id,
//                                                            uint32_t
//                                                            timer_ns);
bf_status_t lld_tof2_ts_baresync_state_set(bf_dev_id_t dev_id,
                                           uint32_t reset_count_threshold,
                                           uint32_t debounce_count,
                                           bool enable);
bf_status_t lld_tof2_ts_baresync_state_get(bf_dev_id_t dev_id,
                                           uint32_t *reset_count_threshold,
                                           uint32_t *debounce_count,
                                           bool *enable);
bf_status_t lld_tof2_ts_baresync_reset_value_set(bf_dev_id_t dev_id,
                                                 uint64_t baresync_time_ns);
bf_status_t lld_tof2_ts_baresync_reset_value_get(bf_dev_id_t dev_id,
                                                 uint64_t *baresync_time_ns);
bf_status_t lld_tof2_ts_baresync_increment_set(
    bf_dev_id_t dev_id,
    uint32_t baresync_inc_time_ns,
    uint32_t baresync_inc_time_fract_ns,
    uint32_t baresync_inc_time_fract_den);
bf_status_t lld_tof2_ts_baresync_increment_get(
    bf_dev_id_t dev_id,
    uint32_t *baresync_inc_time_ns,
    uint32_t *baresync_inc_time_fract_ns,
    uint32_t *baresync_inc_time_fract_den);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
