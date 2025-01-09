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
 * @file pipe_exm_drv_workflows.h
 * @date
 *
 * Definitions for driver workflows used by action data table manager.
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_act_tbl.h"

pipe_status_t pipe_mgr_adt_program_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_data_t *adt_tbl_data,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t entry_idx,
    uint8_t **shadow_ptr_arr,
    mem_id_t *mem_id_arr,
    uint32_t num_ram_units,
    bool update);
