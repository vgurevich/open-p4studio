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
 * @file pipe_mgr_dev_intf.h
 * @date
 *
 * Definitions for device management interface
 */

#ifndef _PIPE_MGR_DEV_INTF_H
#define _PIPE_MGR_DEV_INTF_H

/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <target-utils/third-party/cJSON/cJSON.h>

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/********************************************
 * DEVICE MGMT RELATED API
 ********************************************/
struct bf_dma_info_s;

/* API to add a port to the device */
bf_status_t pipe_mgr_add_port(bf_dev_id_t dev_id,
                              bf_dev_port_t port_id,
                              bf_port_attributes_t *port_attrib,
                              bf_port_cb_direction_t direction);

/* API to remove a port from the device */
bf_status_t pipe_mgr_remove_port(bf_dev_id_t dev_id,
                                 bf_dev_port_t port_id,
                                 bf_port_cb_direction_t direction);

/********************************************
 * RMT TABLE MAPPING RELATED API
 ********************************************/

#endif /* _PIPE_MGR_DEV_INTF_H */
