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
 * This file contains APIs for using in TM uCLI
 */

#include <bf_types/bf_types.h>
#include <traffic_mgr/traffic_mgr_types.h>

#include "traffic_mgr/common/tm_ctx.h"

/**
 * @brief Get ppg icos mask from hardware.
 *
 * @param[in] dev         ASIC device identifier.
 * @param[in] ppg         ppg whose icos mask has to be fetched.
 * @param[out] icos_mask  icos mask
 * @return                Status of API call.
 */
bf_status_t bf_tm_ppg_icos_mask_get(bf_dev_id_t dev,
                                    bf_tm_ppg_hdl ppg,
                                    uint8_t *icos_mask) {
  bf_status_t rc = BF_SUCCESS;
  bool defppg;
  int ppg_n, pipe;
  uint16_t mask = 0;

  bf_tm_api_hlp_get_ppg_details(dev, ppg, &defppg, &ppg_n, &pipe, NULL);

  bf_tm_ppg_t *_ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], pipe, ppg_n);
  BF_TM_PPG_CHECK(_ppg, ppg_n, pipe, dev);

  rc = bf_tm_ppg_get_icos_mask(dev, _ppg, &mask, &mask);
  *icos_mask = mask;
  return (rc);
}
