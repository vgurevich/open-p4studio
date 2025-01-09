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


#ifndef PORT_MGR_UCLI_H_INCLUDED
#define PORT_MGR_UCLI_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

bf_status_t port_diag_prbs_stats_display(bf_dev_id_t dev_id,
                                         bf_dev_port_t dev_port,
                                         void *display_ucli_cookie);
bf_status_t port_diag_perf_display(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   int fp,
                                   int ch,
                                   void *display_ucli_cookie);
bf_status_t port_diag_plot_eye(bf_dev_id_t dev_id,
                               bf_dev_port_t dev_port,
                               void *display_ucli_cookie);
bf_status_t port_diag_dfe_set(bf_dev_id_t dev_id,
                              bf_dev_port_t dev_port,
                              uint32_t lane,
                              bool set_all_lane,
                              uint32_t dfe_ctrl,
                              uint32_t hf_val,
                              uint32_t lf_val,
                              uint32_t dc_val,
                              void *display_ucli_cookie);
bf_status_t port_diag_set_tx_eq(bf_dev_id_t dev_id,
                                bf_dev_port_t dev_port,
                                uint32_t lane,
                                bool set_all_lane,
                                int pre,
                                int atten,
                                int post,
                                int slew,
                                void *display_ucli_cookie);
bf_status_t port_diag_rx_inv_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t lane,
                                 bool set_all_lane,
                                 int polarity,
                                 void *display_ucli_cookie);
bf_status_t port_diag_tx_inv_set(bf_dev_id_t dev_id,
                                 bf_dev_port_t dev_port,
                                 uint32_t lane,
                                 bool set_all_lane,
                                 int polarity,
                                 void *display_ucli_cookie);
bf_status_t port_diag_dfe_ical_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t lane,
                                   bool set_all_lane,
                                   void *display_ucli_cookie);
bf_status_t port_diag_dfe_pcal_set(bf_dev_id_t dev_id,
                                   bf_dev_port_t dev_port,
                                   uint32_t lane,
                                   bool set_all_lane,
                                   void *display_ucli_cookie);
bf_status_t port_diag_chg_to_prbs(bf_dev_id_t dev_id,
                                  bf_dev_port_t dev_port,
                                  void *display_ucli_cookie);

#ifdef __cplusplus
}
#endif /* C++ */

#endif  // PORT_MGR_UCLI_H
