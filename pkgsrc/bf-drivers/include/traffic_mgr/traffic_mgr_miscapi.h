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

#ifndef __TRAFFIC_MGR_MISCAPI_H__
#define __TRAFFIC_MGR_MISCAPI_H__

#include <traffic_mgr/traffic_mgr_types.h>
#include <bf_types/bf_types.h>

/**
 * @addtogroup tm-buffer-mgmt
 * @{
 */

/**
 * Get cells size in bytes. Using this API application can
 * convert bytes into cell units.
 *
 * Related API : bf_tm_total_cell_count_get()
 *
 * @param[in] dev        ASIC device identifier.
 * @param[out] cell_size Cell size in bytes.
 * @return               Status of API call.
 */
bf_status_t bf_tm_cell_size_in_bytes_get(bf_dev_id_t dev, uint32_t *cell_size);

/**
 * Total Traffic Manager buffer capability measured in cell count.
 * Get total cell count supported by ASIC.
 *
 * Related API : bf_tm_convert_bytes_to_cell()
 *
 * @param[in] dev          ASIC device identifier.
 * @param[out] total_cells TM buffering capacity measured in cell count.
 *                         Application SW can use this API to get TM
 *                         buffer size to plan buffer carving.
 * @return                 Status of API call.
 */
bf_status_t bf_tm_total_cell_count_get(bf_dev_id_t dev, uint32_t *total_cells);

/**
 * Get total unassigned cell counts. Ideally this value should be zero
 * so that all TM buffering capability is used.
 *
 * Related API : bf_tm_total_cell_count_get()
 *
 * @param[in] dev            ASIC device identifier.
 * @param[out] total_cells   Unassigned TM buffering capacity measured
 *                           in cell count.
 * @return                   Status of API call.
 */
bf_status_t bf_tm_total_unassigned_cell_count_get(bf_dev_id_t dev,
                                                  int *total_cells);

/**
 * @brief This API can be issued to complete any pending dma operation to asic.
 * API blocks until all dma operations are complete.
 *
 * @param[in] dev           ASIC device identifier.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_complete_operations(bf_dev_id_t dev);

/**
 * Set number of Global timestamp bits that is to be right shifted.
 *
 * @param[in] dev           ASIC device identifier.
 * @param[in] shift         Number of Global timestamp bits that are right
 *                          shifted.
 *                          Up to 16 bits can be right shifted. Any shift value
 *                          greater than 16 is capped to 16.
 * @return                  Status of API call.
 */
bf_status_t bf_tm_timestamp_shift_set(bf_dev_id_t dev, uint8_t shift);

/* @} */

#endif
