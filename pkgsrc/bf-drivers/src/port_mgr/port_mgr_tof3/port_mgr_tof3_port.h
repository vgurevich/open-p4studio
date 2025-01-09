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



#ifndef PORT_MGR_TOF3_PORT_H_INCLUDED
#define PORT_MGR_TOF3_PORT_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

// default rx/tx and mtu
#define PORT_MGR_TOF3_MAX_RX_FRAME_SZ 9216
#define PORT_MGR_TOF3_MAX_TX_FRAME_SZ 9216
#define PORT_MGR_TOF3_CPU_PORT_TMAC (0)
//#define PORT_MGR_TOF3_ROT_CPU_PORT_TMAC3 (39)  // Revisit

/* Signature for an iteration handler function */
typedef bf_status_t (*port_mgr_tof3_iter_cb)(bf_dev_id_t dev_id,
                                             bf_dev_port_t dev_port,
                                             void *userdata);

bf_status_t port_mgr_tof3_port_add(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof3_port_remove(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_port_cb_direction_t direction);
bf_status_t port_mgr_tof3_port_enable(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bool enable);
bf_status_t port_mgr_tof3_port_disable(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_port_oper_state_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool *up);
bf_status_t port_mgr_tof3_port_read_counter(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_rmon_counter_t ctr_id,
                                            uint64_t *ctr_value);
bf_status_t port_mgr_tof3_port_iter(bf_dev_id_t dev_id,
                                    port_mgr_tof3_iter_cb fn,
                                    void *userdata);
bf_status_t bf_serdes_tof3_clkobs_set(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_clkobs_pad_t pad,
                                      bf_sds_clkobs_clksel_t clk_src,
                                      int divider,
                                      int daisy_sel);
bf_status_t port_mgr_tof3_port_lane_map_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t phys_tx_ln[8],
                                            uint32_t phys_rx_ln[8]);
bf_status_t port_mgr_tof3_port_lane_map_get(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            uint32_t phys_tx_ln[8],
                                            uint32_t phys_rx_ln[8]);
int port_mgr_tof3_get_num_lanes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_port_sigovrd_set(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port,
                                           uint32_t num_lanes,
                                           bf_sigovrd_fld_t ovrd_val);
bf_status_t port_mgr_tof3_port_loopback_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port,
                                            bf_loopback_mode_e mode);
bf_status_t port_mgr_tof3_set_default_all_ports_state(bf_dev_id_t dev_id);
bf_status_t port_mgr_tof3_port_tx_reset_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_port_rx_reset_set(bf_dev_id_t dev_id,
                                            bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_port_flowcontrol_set(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_port_mac_int_en_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              bool on);
bf_status_t port_mgr_tof3_port_local_fault_int_en_set(bf_dev_id_t dev_id,
                                                      bf_dev_port_t dev_port,
                                                      bool en);
bf_status_t port_mgr_tof3_port_remote_fault_int_en_set(bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool en);
bf_status_t port_mgr_tof3_port_register_default_handler_for_mac_ints(
                                                  bf_dev_id_t dev_id);

bf_status_t port_mgr_tof3_port_mac_stats_clear(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);

bf_status_t port_mgr_tof3_pcs_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bf_tof3_pcs_status_t *pcs);

bf_status_t port_mgr_tof3_fec_status_get(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         bf_tof3_fec_status_t *fec);
bf_status_t port_mgr_tof3_port_tmac_reset_set(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t val);
bf_status_t port_mgr_tof3_port_fec_lane_symb_err_counter_get(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port,
                                              uint32_t n_ctrs,
                                              uint64_t symb_err[16]);
int port_mgr_tof3_tmac_num_lanes_per_ch(bf_dev_id_t dev_id,
                                        bf_dev_port_t dev_port);

bf_status_t port_mgr_tof3_clear_pfc_data_path(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port);
bf_status_t port_mgr_tof3_ignore_fault(bf_dev_id_t dev_id,bf_dev_port_t dev_port, bool en);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_TOF3_PORT_H_INCLUDED
