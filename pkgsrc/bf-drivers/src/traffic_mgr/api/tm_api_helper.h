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


/*
 */

#ifndef __TM_API_HELPER_H__
#define __TM_API_HELPER_H__

#include <stdint.h>
#include <stdbool.h>
#include <traffic_mgr/traffic_mgr_types.h>

int bf_tm_api_hlp_get_pool_details(bf_tm_app_pool_t pool, int *direction);
bool bf_tm_api_hlp_is_baf_dynamic(int baf);
void bf_tm_api_hlp_get_ppg_details(
    bf_dev_id_t, bf_tm_ppg_hdl, bool *, int *, int *, int *);

#endif
