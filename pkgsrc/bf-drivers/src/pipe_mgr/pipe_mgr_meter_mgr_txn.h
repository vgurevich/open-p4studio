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
 * @file pipe_mgr_meter_txn.h
 * @date
 *
 * Definition for Meter table manager transaction.
 */

#ifndef _PIPE_MGR_METER_MGR_TXN_H
#define _PIPE_MGR_METER_MGR_TXN_H

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr/pipe_mgr_intf.h"

/* Local header includes */
#include "pipe_mgr_meter_mgr_int.h"

pipe_status_t pipe_mgr_meter_update_txn_state(
    pipe_mgr_meter_txn_state_t *txn_state,
    uint8_t pipe_id,
    pipe_meter_idx_t meter_idx);

pipe_status_t pipe_mgr_meter_backup_entry_info(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_meter_idx_t meter_idx);

pipe_status_t pipe_mgr_meter_mgr_backup_stage_idx_info(
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    pipe_meter_idx_t meter_idx);

#endif  // _PIPE_MGR_METER_MGR_TXN_H
