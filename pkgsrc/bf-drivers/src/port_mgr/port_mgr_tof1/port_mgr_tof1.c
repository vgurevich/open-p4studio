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


#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/port_mgr_intf.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_interrupt_if.h>
#include "port_mgr_mac.h"

/** \brief port_mgr_tof1_init:
 *
 * Initialize port_mgr module
 */
void port_mgr_tof1_init(void) {
  lld_register_mac_int_poll_cb(port_mgr_mac_interrupt_poll);
  lld_register_mac_int_dump_cb(port_mgr_mac_interrupt_dump);
}
