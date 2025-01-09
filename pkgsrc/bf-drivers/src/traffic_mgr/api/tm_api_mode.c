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


/* This file implements TM buffer management APIs exported to
 * client/application.
 * APIs in this file are the northbound interface to TM.
 */

#include <traffic_mgr/traffic_mgr.h>

#include "traffic_mgr/common/tm_ctx.h"

bf_status_t bf_tm_batch_update_start(bf_dev_id_t dev) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  g_tm_ctx[dev]->api_batch_mode = true;
  TM_UNLOCK_AND_FLUSH(dev);
  return (BF_SUCCESS);
}

bf_status_t bf_tm_batch_update_end(bf_dev_id_t dev) {
  BF_TM_INVALID_ARG(TM_IS_DEV_INVALID(dev));
  TM_LOCK(dev, g_tm_ctx[dev]->lock);
  g_tm_ctx[dev]->api_batch_mode = false;
  TM_UNLOCK_AND_FLUSH(dev);
  return (BF_SUCCESS);
}
