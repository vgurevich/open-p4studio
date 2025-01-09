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


// This file implements helper functions used in
// tm_api_*.c file.
// Helper functions implemented here are used in TM APIs
// to make MT safe within process, argument checking,
// hardware ready for programming, perform batch mode or not,
// update only sw entries etc..

#include <traffic_mgr/traffic_mgr.h>
#include "traffic_mgr/common/tm_ctx.h"

int bf_tm_api_hlp_get_pool_details(bf_tm_app_pool_t pool, int *direction) {
  uint8_t id = 0;

  switch (pool) {
    case BF_TM_IG_APP_POOL_0:
      *direction = BF_TM_DIR_INGRESS;
      id = 0;
      break;
    case BF_TM_IG_APP_POOL_1:
      *direction = BF_TM_DIR_INGRESS;
      id = 1;
      break;
    case BF_TM_IG_APP_POOL_2:
      *direction = BF_TM_DIR_INGRESS;
      id = 2;
      break;
    case BF_TM_IG_APP_POOL_3:
      *direction = BF_TM_DIR_INGRESS;
      id = 3;
      break;


















































    case BF_TM_EG_APP_POOL_0:
      *direction = BF_TM_DIR_EGRESS;
      id = 0;
      break;
    case BF_TM_EG_APP_POOL_1:
      *direction = BF_TM_DIR_EGRESS;
      id = 1;
      break;
    case BF_TM_EG_APP_POOL_2:
      *direction = BF_TM_DIR_EGRESS;
      id = 2;
      break;
    case BF_TM_EG_APP_POOL_3:
      *direction = BF_TM_DIR_EGRESS;
      id = 3;
      break;


















































    case BF_TM_APP_POOL_LAST:
    default:
      // TODO: Add error handling
      *direction = BF_TM_DIR_EGRESS;
      id = (BF_TM_APP_POOL_LAST - BF_TM_EG_APP_POOL_0 - 1);  // Max EG pool id
      break;
  }
  return (id);
}

bool bf_tm_api_hlp_is_baf_dynamic(int baf) {
  if ((baf == BF_TM_PPG_BAF_DISABLE) || (baf == BF_TM_Q_BAF_DISABLE)) {
    return (false);
  }
  return (true);
}

/* From PPG handle, get ppg, pipe#, port# ; If ppg is default PPG, ppg#
 * itself can tell port#. However when PFC PPG are allocated, port is
 * embedded in ppg handle.
 * The port value encoded in the PPG handler is 'user' device local port,
 * i.e. on TF-3 it is 0..70 and 72 mirror port.
 *
 * NOTE: Both dev and ppg arguments must be valid.
 */
void bf_tm_api_hlp_get_ppg_details(bf_dev_id_t dev,
                                   bf_tm_ppg_hdl ppg,
                                   bool *defppg,
                                   int *ppg_num,
                                   int *pipe,
                                   int *port) {
  if (TM_PPG_IDX(ppg) >= g_tm_ctx[dev]->tm_cfg.pfc_ppg_per_pipe) {
    // Default PPG
    if (defppg) *defppg = true;
  } else {
    if (defppg) *defppg = false;
  }
  if (ppg_num) *ppg_num = TM_PPG_IDX(ppg);
  if (pipe) *pipe = TM_PPG_PIPE(ppg);
  if (port) *port = TM_DPG_LOCAL_PORT(ppg);
}
