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


#include <stdio.h>
#include <dvm/bf_drv_intf.h>
#include <port_mgr/bf_port_if.h>
#include <tofino/pdfixed/pd_port_mgr.h>

p4_pd_status_t p4_port_mgr_mtu_set(const bf_dev_id_t dev_id,
                                   const int32_t port_id,
                                   const int32_t tx_mtu,
                                   const int32_t rx_mtu) {
  return bf_port_mtu_set(dev_id, port_id, tx_mtu, rx_mtu);
}
