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


#ifndef _TM_UCLI_APIS_H_
#define _TM_UCLI_APIS_H_

#include <bf_types/bf_types.h>
#include "traffic_mgr/common/tm_ctx.h"
#include "traffic_mgr/common/tm_error.h"

/**
 * @brief Get ppg icos mask from hardware.
 * For internal using
 * @param[in] dev         ASIC device identifier.
 * @param[in] ppg         ppg whose icos mask has to be fetched.
 * @param[out] icos_mask  icos mask
 * @return                Status of API call.
 *  BF_SUCCESS on success
 *  Non-Zero on error
 */
bf_status_t bf_tm_ppg_icos_mask_get(bf_dev_id_t dev,
                                    bf_tm_ppg_hdl ppg,
                                    uint8_t *icos_mask);
#endif  // _TM_UCLI_APIS_H_
