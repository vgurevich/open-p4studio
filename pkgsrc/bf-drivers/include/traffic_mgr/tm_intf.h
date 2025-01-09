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


#ifndef __TM_INTF_H__
#define __TM_INTF_H__

#include <traffic_mgr/traffic_mgr_types.h>

bf_status_t bf_tm_init();

// Used for UT purposes only..
void bf_tm_set_ut_mode_as_model(bf_dev_id_t dev);
void bf_tm_set_ut_mode_as_asic(bf_dev_id_t dev);

#endif
