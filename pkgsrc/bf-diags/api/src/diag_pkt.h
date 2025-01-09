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


/*!
 * @file diag_pkt.h
 * @date
 *
 * Contains definitions of diag pkt send thread, recv callbacks
 *
 */
#ifndef _DIAG_PKT_H
#define _DIAG_PKT_H

/* Module header includes */
#include "diag_common.h"

#define DIAG_MIN_INTER_PKT_TIME 100000

bf_status_t diag_pkt_tx_creator(bf_diag_sess_hdl_t sess_hdl);
bf_status_t diag_pkt_tx_fn(void *args);
bf_status_t diag_register_pkt_rx(bf_dev_id_t dev_id);
bf_status_t diag_deregister_pkt_rx(bf_dev_id_t dev_id);
bool diag_pkt_log_err_enabled();
bf_status_t diag_pkt_inject(bf_dev_id_t dev_id,
                            const bf_dev_port_t *port_list,
                            int num_ports,
                            uint32_t num_packet,
                            uint32_t pkt_size);
bf_status_t diag_kernel_setup_pkt_intf(bf_dev_id_t dev_id);
int diag_kernel_pkt_tx(bf_dev_id_t dev_id, uint8_t *pkt_buf, uint32_t pkt_size);
bf_status_t diag_kernel_cb_pkt_rx(bf_dev_id_t dev_id,
                                  uint8_t *buf,
                                  uint32_t size);
bf_status_t diag_eth_cpu_cb_pkt_rx(bf_dev_id_t dev_id,
                                   uint8_t *buf,
                                   uint32_t size);
bf_status_t diag_eth_cpu_pkt_tx(bf_dev_id_t dev_id,
                                const uint8_t *buf,
                                uint32_t size);

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
void set_slt_failure_test_mode(diag_slt_failure_type_e mode);
diag_slt_failure_type_e get_slt_failure_test_mode();
#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)

#endif
