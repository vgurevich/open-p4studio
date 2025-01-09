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


#ifndef port_mgr_serdes_h_included
#define port_mgr_serdes_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

bf_status_t port_mgr_serdes_init(bf_dev_id_t dev_id);
void port_mgr_serdes_init_warm_boot(bf_dev_id_t dev_id);
int port_mgr_serdes_get_num_rings(bf_dev_id_t dev_id);
int port_mgr_serdes_get_num_serdes(bf_dev_id_t dev_id);
void port_mgr_serdes_set_is_serdes(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_serdes_is_serdes(bf_dev_id_t dev_id, int ring, int sd);
int port_mgr_serdes_is_eth_serdes(bf_dev_id_t dev_id, int ring, int sd);

bf_status_t port_mgr_serdes_log_dfe(bf_dev_id_t dev_id,
                                    bf_dev_port_t port,
                                    int lane);
bf_status_t port_mgr_serdes_hw_cfg_get(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port);
bf_status_t port_mgr_serdes_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info);
bf_status_t port_mgr_serdes_delta_settings_apply(bf_dev_id_t dev_id,
                                                 bf_dev_port_t dev_port);
bf_status_t port_mgr_serdes_firmware_upgrade(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             uint32_t fw_ver,
                                             char *fw_path);
bf_status_t port_mgr_serdes_tof_dfe_cfg_get(bf_dev_id_t dev_id,
                                            uint32_t ring,
                                            uint32_t sd,
                                            uint32_t *dfe_ctrl,
                                            uint32_t *hf_val,
                                            uint32_t *lf_val,
                                            uint32_t *dc_val);
bf_status_t port_mgr_serdes_tof_dfe_cfg_set(bf_dev_id_t dev_id,
                                            uint32_t ring,
                                            uint32_t sd,
                                            uint32_t dfe_ctrl,
                                            uint32_t hf_val,
                                            uint32_t lf_val,
                                            uint32_t dc_val);
bf_status_t port_mgr_serdes_tof_dfe_cfg_default_set(bf_dev_id_t dev_id,
                                                    uint32_t ring,
                                                    uint32_t sd);
bf_status_t port_mgr_tof1_serdes_lane_map_set(
    bf_dev_id_t dev_id,
    bf_mac_block_id_t mac_block,
    bf_mac_block_lane_map_t *lane_map);
bf_status_t port_mgr_tof1_serdes_lane_map_get(
    bf_dev_id_t dev_id,
    bf_mac_block_id_t mac_block,
    bf_mac_block_lane_map_t *lane_map);
bf_status_t bf_serdes_clkobs_clksel_set(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port,
                                        bf_clkobs_pad_t pad,
                                        bf_sds_clkobs_clksel_t clk_src);
bf_status_t bf_serdes_clkobs_div_set(bf_dev_id_t dev_id,
                                     bf_clkobs_pad_t pad,
                                     int divider,
                                     bool daisy_sel);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // port_mgr_serdes_h_included
