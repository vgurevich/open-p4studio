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


#ifndef port_mgr_tof2_map_h
#define port_mgr_tof2_map_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_mgr_tof2_physical_dev.h"

bool port_mgr_tof2_dev_port_is_cpu_port(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_map_dev_port_to_all(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t *pipe_id,
                                              uint32_t *port_id,
                                              uint32_t *umac,
                                              uint32_t *ch,
                                              bool *is_cpu_port);
uint32_t port_mgr_tof2_map_dev_port_to_port_index(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port);
uint32_t port_mgr_tof2_map_dev_port_lane_to_sd_base(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    uint32_t ln);
port_mgr_tof2_serdes_t *port_mgr_tof2_map_dev_port_lane_to_serdes(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port, uint32_t ln);
port_mgr_umac3_t *port_mgr_tof2_map_dev_port_lane_to_umac3(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port);
port_mgr_umac4_t *port_mgr_tof2_map_dev_port_lane_to_umac4(
    bf_dev_id_t dev_id, bf_dev_port_t dev_port);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
