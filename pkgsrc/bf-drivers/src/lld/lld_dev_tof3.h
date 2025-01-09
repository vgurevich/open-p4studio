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


#ifndef LLD_DEV_TOF3_H_INCLUDED
#define LLD_DEV_TOF3_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

bf_status_t lld_dev_tof3_un_reset(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);
bf_status_t lld_dev_tof3_reset_core(bf_dev_id_t dev_id,
                                    bf_subdev_id_t subdev_id);
bf_status_t lld_dev_tof3_set_core_clock(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        int freq);
bf_status_t lld_dev_tof3_get_core_clock(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        int *freq,
                                        uint32_t *pll_val);
bf_status_t lld_dev_tof3_set_pps_clock(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       int freq);
bf_status_t lld_dev_tof3_get_pps_clock(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id,
                                       int *freq,
                                       uint32_t *pll_val);
bf_status_t lld_dev_tof3_set_tm_clock(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      int freq);
bf_status_t lld_dev_tof3_get_tm_clock(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      int *freq,
                                      uint32_t *pll_val);
bf_status_t lld_dev_tof3_tlp_poison_set(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id,
                                        bool en);
#ifdef __cplusplus
}
#endif /* C++ */

#endif  // LLD_DEV_TOF3_H_INCLUDED
