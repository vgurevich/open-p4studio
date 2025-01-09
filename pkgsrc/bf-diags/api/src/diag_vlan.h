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
 * @file diag_vlan.h
 * @date
 *
 * Contains definitions of vlan
 *
 */
#ifndef _DIAG_VLAN_H
#define _DIAG_VLAN_H

/* Module header includes */

#include <stdint.h>
#include "stdbool.h"
#include <bf_types/bf_types.h>
#include "diag_common.h"

#define DIAG_GEN_MC_INDEX(_vlan) (_vlan)
#define DIAG_GEN_VLAN_RID(_vlan) (_vlan)

diag_vlan_t *diag_int_get_vlan_info(bf_dev_id_t dev_id, int vlan_id);
bf_status_t diag_int_vlan_create(bf_dev_id_t dev_id, int vlan_id);
bf_status_t diag_int_vlan_destroy(bf_dev_id_t dev_id, int vlan_id);
bf_status_t diag_int_get_vlan_port_bitmap(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          uint8_t *port_map,
                                          uint8_t *lag_map);

#endif
