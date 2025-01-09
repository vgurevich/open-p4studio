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


#ifndef port_mgr_tof1_map_h
#define port_mgr_tof1_map_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

port_mgr_dev_t *port_mgr_map_dev_id_to_dev_p_allow_unassigned(
    bf_dev_id_t dev_id);
port_mgr_dev_t *port_mgr_map_dev_id_to_dev_p(bf_dev_id_t dev_id);
port_mgr_mac_block_t *port_mgr_map_port_to_mac_block(bf_dev_id_t dev_id,
                                                     uint32_t port);
// port_mgr_port_t *port_mgr_map_dev_port_to_port_allow_unassigned(
//    bf_dev_id_t dev_id, uint32_t port);
// port_mgr_port_t *port_mgr_map_dev_port_to_port(bf_dev_id_t dev_id,
//                                               uint32_t port);
// port_mgr_serdes_t *port_mgr_map_port_lane_to_serdes_allow_unassigned(
//    bf_dev_id_t dev_id, uint32_t port, uint32_t lane);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes_int(
    bf_dev_id_t dev_id,
    bf_dev_port_t port,
    uint32_t lane,
    bool allow_unassigned);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes(bf_dev_id_t dev_id,
                                                         uint32_t port,
                                                         uint32_t lane);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_serdes_allow_unassigned(
    bf_dev_id_t dev_id, uint32_t port, uint32_t lane);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_hw_serdes_int(
    bf_dev_id_t dev_id,
    bf_dev_port_t port,
    uint32_t lane,
    bool allow_unassigned);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_hw_serdes(bf_dev_id_t dev_id,
                                                            uint32_t port,
                                                            uint32_t lane);
port_mgr_serdes_t *port_mgr_tof1_map_port_lane_to_hw_serdes_allow_unassigned(
    bf_dev_id_t dev_id, uint32_t port, uint32_t lane);
// port_mgr_serdes_t *port_mgr_map_port_lane_to_hw_serdes_allow_unassigned(
//    bf_dev_id_t dev_id, uint32_t port, uint32_t lane);
// port_mgr_serdes_t *port_mgr_map_port_lane_to_hw_serdes(bf_dev_id_t dev_id,
//                                                       uint32_t port,
//                                                       uint32_t lane);
port_mgr_mac_block_t *port_mgr_tof1_map_idx_to_mac_block(bf_dev_id_t dev_id,
                                                         uint32_t idx);
port_mgr_mac_block_t *port_mgr_tof1_map_idx_to_mac_block_allow_unassigned(
    bf_dev_id_t dev_id, uint32_t idx);
port_mgr_serdes_t *port_mgr_tof1_map_ring_sd_to_hw_serdes(bf_dev_id_t dev_id,
                                                          uint32_t ring,
                                                          uint32_t sd);
port_mgr_err_t port_mgr_tof1_map_dev_port_to_all(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 bf_dev_pipe_t *pipe_id,
                                                 bf_dev_port_t *port_id,
                                                 int *mac_block,
                                                 int *ch,
                                                 int *is_cpu_port);
bf_status_t port_mgr_map_port_lane_to_gpio_refclk(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port,
                                                  uint32_t lane,
                                                  uint32_t *reg,
                                                  uint32_t *bit);
uint32_t port_mgr_tof1_map_dev_port_to_port_index(bf_dev_id_t dev_id,
                                                  bf_dev_port_t dev_port);
#ifdef __cplusplus
}
#endif /* C++ */

#endif
