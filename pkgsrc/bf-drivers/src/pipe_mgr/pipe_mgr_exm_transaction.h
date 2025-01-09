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
 * @file pipe_exm_transaction.c
 * @date
 *
 * Exact-match table transaction handling.
 *
 */

/* Standard header includes */

/* Module header includes */

/* Local header includes */
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "cuckoo_move.h"

pipe_status_t pipe_mgr_exm_ent_hdl_txn_add(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_mgr_exm_entry_info_t *entry_info,
    pipe_mgr_exm_operation_e operation,
    pipe_mat_ent_idx_t src_entry_idx,
    pipe_mat_ent_idx_t dst_entry_idx);
