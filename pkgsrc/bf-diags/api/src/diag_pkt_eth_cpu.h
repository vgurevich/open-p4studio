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
 * @file diag_pkt_eth_cpu.h
 * @date
 *
 * Contains definitions of diag eth cpu pkt interface
 *
 */
#ifndef _DIAG_PKT_ETH_CPU_H
#define _DIAG_PKT_ETH_CPU_H

/* Module header includes */
#include "diag_common.h"

bf_status_t diag_eth_cpu_port_init(bf_dev_id_t dev_id,
                                   const char *eth_cpu_port_name);
bf_status_t diag_eth_cpu_port_deinit(bf_dev_id_t dev_id);

#endif
