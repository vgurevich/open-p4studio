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

/*************************************************
 * Placeholders for serdes mapping functions
 *************************************************/

#ifndef port_mgr_tof3_serdes_map_h
#define port_mgr_tof3_serdes_map_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "aw_if.h"

uint32_t map_aw_err_to_bf_err(uint32_t rc);
bf_tf3_sd_t *map_dev_port_to_sd(uint32_t dev_id,
                                uint32_t dev_port,
                                uint32_t ln);
uint32_t map_macro_to_address(uint32_t macro);
uint32_t map_address_to_macro(uint32_t addr,
                              uint32_t *subdev_id,
                              uint32_t *macro);
uint32_t map_dev_port_to_macro(uint32_t dev_id,
                               uint32_t dev_port,
                               uint32_t *subdev_id,
                               uint32_t *macro);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // port_mgr_tof3_serdes_map_h
