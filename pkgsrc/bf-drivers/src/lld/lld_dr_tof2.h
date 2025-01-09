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


#ifndef lld_dr_tof2_h
#define lld_dr_tof2_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

void lld_dr_tof2_dma_enable_set(bf_dev_id_t dev_id, bool en_dma);
void lld_dr_tof2_pbus_arb_ctrl_set(bf_dev_id_t dev_id,
                                   pbus_arb_ctrl_t *pbus_arb_ctrl);
void lld_dr_tof2_cbus_arb_ctrl_set(bf_dev_id_t dev_id,
                                   uint32_t cbus_arb_ctrl_val);
void lld_dr_tof2_enable_set(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id, bool en);
bf_status_t lld_dr_tof2_write_time_mode_set(bf_dev_id_t dev_id,
                                            bf_dma_dr_id_t dr_id,
                                            bool en);
bf_status_t lld_dr_tof2_pushed_ptr_mode_set(bf_dev_id_t dev_id,
                                            bf_dma_dr_id_t dr_id,
                                            bool en);
void lld_dr_tof2_ring_timeout_set(bf_dev_id_t dev_id,
                                  bf_dma_dr_id_t dr_id,
                                  uint16_t timeout);
void lld_dr_tof2_data_timeout_set(bf_dev_id_t dev_id,
                                  bf_dma_dr_id_t dr_id,
                                  uint32_t timeout);
bf_status_t lld_dr_tof2_data_timeout_get(bf_dev_id_t dev_id,
                                         bf_dma_dr_id_t dr_id,
                                         uint32_t *timeout);
void lld_dr_tof2_flush_all(bf_dev_id_t dev_id);
uint32_t lld_dr_tof2_wait_for_flush_done(bf_dev_id_t dev_id,
                                         uint32_t max_tries);
void lld_dr_tof2_clear_link_down(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
