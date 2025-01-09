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


#ifndef port_mgr_tof1_ha_h_included
#define port_mgr_tof1_ha_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

void port_mgr_tof1_ha_warm_init(bf_dev_id_t dev_id);
bf_status_t port_mgr_tof1_ha_hardware_read(bf_dev_id_t dev_id);
bf_status_t port_mgr_tof1_ha_compute_delta_changes(bf_dev_id_t dev_id);
bf_status_t port_mgr_tof1_ha_port_serdes_upgrade(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port,
                                                 uint32_t fw_ver,
                                                 char *fw_path);
bf_status_t port_mgr_tof1_ha_enable_input_packets(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
