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


#ifndef BF_FSM_HDLRS_H_INCLUDED
#define BF_FSM_HDLRS_H_INCLUDED

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif
// non-AN port fsm handlers
bf_status_t bf_fsm_init_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_re_init_serdes_rx(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_pll(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_signal_ok(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_dfe_quick(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_dfe_done(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_remote_fault(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_pcs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_ena_mac(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

// AN port fsm handlers
bf_status_t bf_fsm_an_init_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_pll1(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_base_pg(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_next_pg(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_an_good(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_pll2(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_an_cmplt(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_pcs_up(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_pcal_done(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_wait_for_port_dwn_event(bf_dev_id_t dev_id,
                                              bf_dev_port_t dev_port);
bf_status_t bf_fsm_an_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

bf_status_t bf_fsm_config_serdes(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_for_port_dwn_event(bf_dev_id_t dev_id,
                                           bf_dev_port_t dev_port);
bf_status_t bf_fsm_re_config_serdes_rx(bf_dev_id_t dev_id,
                                       bf_dev_port_t dev_port);
// Non serdes fsm handlers
bf_status_t bf_fsm_enable_mac_tx_rx(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_assert_rs_fec(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_deassert_rs_fec(bf_dev_id_t dev_id, bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_lpbk_port_up(bf_dev_id_t dev_id,
                                     bf_dev_port_t dev_port);
bf_status_t bf_fsm_default_abort(bf_dev_id_t dev_id, bf_dev_port_t dev_port);

// Port fsm handlers based on port-directions
bf_status_t bf_fsm_config_for_tx_mode(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port);
bf_status_t bf_fsm_wait_for_port_up_in_tx_mode(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port);
#ifdef __cplusplus
}
#endif /* C++ */

#endif  // BF_FSM_HDLRS_H_INCLUDED
