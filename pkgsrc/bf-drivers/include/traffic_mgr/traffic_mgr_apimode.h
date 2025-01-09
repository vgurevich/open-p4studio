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
 * APIs for client application to program Traffic Manager block to
 * desired QoS behaviour.
 */

#ifndef __TRAFFIC_MGR_APIMODE_H__
#define __TRAFFIC_MGR_APIMODE_H__

#include <traffic_mgr/traffic_mgr_types.h>
#include <bf_types/bf_types.h>

/**
 * @file traffic_mgr_apimode.h
 * \brief This file contains Traffic Manager helper functions
 *        that application/client can make use.
 */

/**
 * @addtogroup tm-api
 * @{
 *  This file contains Traffic Manager helper functions
 *  that application/client can make use.
 */

/**
 * By default traffic manager updates are non batched. All configuration
 * updates are pushed to hardware/asic immediately.  Updates are made
 * efficient by batching writes over DMA to hardware.
 * Batch mode is very useful during system start. Bulk of Traffic Manager
 * configuration during TM initialization can be pushed to hardware in
 * batch mode.
 *
 * Related API : bf_tm_batch_update_end()
 *
 * @param dev      ASIC device identifier.
 * @return         Status of API call.
 */
bf_status_t bf_tm_batch_update_start(bf_dev_id_t dev);

/**
 * All traffic manager configurations that are not pushed to hardware
 * during batch mode processing will be flushed to hardware. Also
 * Traffic Manager update mode is set back to non batch mode.
 * If write bulk mode is desired, application is expected to invoke
 * bf_tm_batch_update_start().
 *
 * Related API : bf_tm_batch_update_start()
 *
 * @param dev      ASIC device identifier.
 * @return         Status of API call.
 */
bf_status_t bf_tm_batch_update_end(bf_dev_id_t dev);

/* @} */

#endif
