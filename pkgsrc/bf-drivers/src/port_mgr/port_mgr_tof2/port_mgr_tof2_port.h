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


#ifndef PORT_MGR_TOF2_PORT_H_INCLUDED
#define PORT_MGR_TOF2_PORT_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// limit MTU on Tofino to avoid a TM issue
#define PORT_MGR_TOF2_MAX_FRAME_SZ (10 * 1024)
#define PORT_MGR_TOF2_CPU_PORT_UMAC3 (0)
#define PORT_MGR_TOF2_ROT_CPU_PORT_UMAC3 (39)

/* Signature for an iteration handler function */
typedef bf_status_t (*port_mgr_tof2_iter_cb)(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             void *userdata);

bf_status_t port_mgr_tof2_port_add(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof2_port_remove(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof2_port_enable(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool enable);
bf_status_t port_mgr_tof2_port_disable(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_oper_state_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool *up);
bf_status_t port_mgr_tof2_port_oper_state_extended_get(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool *up,
                                                       bool *pcs_ready,
                                                       bool *l_fault,
                                                       bool *r_fault);
bf_status_t port_mgr_tof2_port_read_counter(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_rmon_counter_t ctr_id,
                                            uint64_t *ctr_value);
bf_status_t port_mgr_tof2_port_mac_stats_clear(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_iter(bf_dev_id_t dev_id,
                                    port_mgr_tof2_iter_cb fn,
                                    void *userdata);
bf_status_t bf_serdes_tof2_clkobs_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_clkobs_pad_t pad,
                                      bf_sds_clkobs_clksel_t clk_src,
                                      int divider,
                                      int daisy_sel);
bf_status_t port_mgr_tof2_port_lane_map_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t phys_tx_ln[8],
                                            uint32_t phys_rx_ln[8]);
bf_status_t port_mgr_tof2_port_lane_map_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t phys_tx_ln[8],
                                            uint32_t phys_rx_ln[8]);
int port_mgr_tof2_get_num_lanes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_sigovrd_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t num_lanes,
                                           bf_sigovrd_fld_t ovrd_val);
bf_status_t port_mgr_tof2_port_loopback_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_loopback_mode_e mode);
bf_status_t port_mgr_tof2_set_default_all_ports_state(bf_dev_id_t dev_id);
bf_status_t port_mgr_tof2_port_tx_reset_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_rx_reset_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_flowcontrol_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_port_mac_int_en_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool on);
bf_status_t port_mgr_tof2_port_local_fault_int_en_set(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      bool en);
bf_status_t port_mgr_tof2_port_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool en);
bf_status_t port_mgr_tof2_port_link_gain_int_en_set(bf_dev_id_t dev_id,
                                                    bf_dev_port_t dev_port,
                                                    bool en);
bf_status_t port_mgr_tof2_port_register_default_handler_for_mac_ints(
    bf_dev_id_t dev_id);
bf_status_t port_mgr_tof2_port_tx_ignore_rx_set(bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                bool en);
bf_status_t port_mgr_tof2_port_config_get(bf_dev_id_t dev_id,
                                          bf_dev_port_t dev_port);
bf_status_t port_mgr_tof2_link_fault_status_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_port_link_fault_st_t *link_fault_st);
bf_status_t port_mgr_tof2_port_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info);

bf_status_t port_mgr_tof2_loopbackport_delta_compute(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_ha_port_reconcile_info_t *recon_info);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_TOF2_PORT_H_INCLUDED
