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


#ifndef port_mgr_physical_dev_h_included
#define port_mgr_physical_dev_h_included

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include "port_mgr_tof1/port_mgr_tof1_physical_dev.h"
#include "port_mgr_tof2/port_mgr_tof2_physical_dev.h"

// Supported chip types
typedef enum {
  PORT_MGR_PDEV_TYPE_UNSPEC = 0,  // catch uninitialized refs
  PORT_MGR_PDEV_TYPE_TOF1 = 1,
  PORT_MGR_PDEV_TYPE_TOF2 = 2,
  PORT_MGR_PDEV_TYPE_TOF3 = 3,
} port_mgr_pdev_type_t;

// physical device (chip-specific)
typedef struct port_mgr_pdev_t {
  port_mgr_pdev_type_t pdev_type;
  union {
    port_mgr_tof1_pdev_t pdev_tof1;
    port_mgr_tof2_pdev_t pdev_tof2;
  } u;
} port_mgr_pdev_t;

port_mgr_tof1_pdev_t *port_mgr_dev_physical_dev_get(bf_dev_id_t dev_id);
port_mgr_tof2_pdev_t *port_mgr_dev_physical_dev_tof2_get(bf_dev_id_t dev_id);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
