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


#ifndef PORT_MGR_TOF1_PORT_H_INCLUDED
#define PORT_MGR_TOF1_PORT_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// limit MTU on Tofino to avoid a TM issue
#define PORT_MGR_TOF_MAX_FRAME_SZ (10 * 1024)
// Default max-rx-jabber-size, it should be multiple of 16 and lower than 16K
#define PORT_MGR_TOF_DEFLT_MAX_JAB_SZ (0x4000 - 0x10)

bf_status_t port_mgr_tof1_port_add(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof1_port_remove(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof1_port_enable(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool enable);
bf_status_t port_mgr_tof1_port_serdes_upgrade(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t fw_ver,
                                              char *fw_path);

bf_status_t port_mgr_port_disable(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t port_mgr_port_disable_all(bf_dev_id_t dev_id);
bf_status_t port_mgr_port_set_speed(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port,
                                    bf_port_speed_t speed);

int port_mgr_ch_reqd_by_speed(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              bf_port_speed_t speed);
uint32_t port_mgr_ch_in_use(bf_dev_id_t dev_id, bf_dev_port_t port);
void port_mgr_port_pgm_speed_fec(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 bf_port_speed_t speed,
                                 bf_fec_type_t fec);
bf_status_t port_mgr_port_eth_cpu_port_reset(bf_dev_id_t dev_id,
                                             bf_dev_port_t eth_cpu_port);

void port_mgr_link_up_actions(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
void port_mgr_link_dn_actions(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
int port_mgr_tof1_get_num_lanes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t port_mgr_tof1_port_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info);
bf_status_t port_mgr_port_serdes_upgrade(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         uint32_t fw_ver,
                                         char *fw_path);
void port_mgr_init_port_mtx(bf_dev_id_t dev_id);
void port_mgr_deinit_port_mtx(bf_dev_id_t dev_id);
bf_status_t port_mgr_port_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           bool en);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_TOF1_PORT_H_INCLUDED
