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



#ifndef port_mgr_tof3_serdes_h
#define port_mgr_tof3_serdes_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

void port_mgr_aw_access_rd32(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             uint32_t offset,
                             uint32_t *r_data,
                             const char *fn);

void port_mgr_aw_access_wr32(bf_dev_id_t dev_id,
                             bf_dev_port_t dev_port,
                             uint32_t offset,
                             uint32_t w_data,
                             const char *fn);
bf_status_t port_mgr_tof3_serdes_clkobs_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_clkobs_pad_t pad,
                                            bf_sds_clkobs_clksel_t clk_src,
                                            int divider);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
