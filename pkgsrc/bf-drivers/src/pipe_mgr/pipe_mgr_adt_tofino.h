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
 * @file pipe_mgr_adt_tofino.h
 * @date
 *
 * Contains definitions for Tofino specific services for action data table
 * management exposed to the action data table manager.
 *
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"

#define TOF_ADT_VADDR_NUM_BITS 21

pipe_status_t pipe_adt_tof_generate_vaddr(
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_adt_ent_idx_t adt_ent_idx,
    rmt_virt_addr_t *adt_virt_addr_p);

pipe_status_t pipe_mgr_adt_tof_encode_entry(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_data_spec_t *act_data_spec,
    pipe_adt_ent_idx_t adt_ent_idx,
    uint8_t **shadow_ptr_arr);

void pipe_mgr_adt_tof_unbuild_virt_addr(rmt_virt_addr_t virt_addr,
                                        uint32_t adt_entry_width,
                                        uint32_t *ram_line,
                                        vpn_id_t *vpn,
                                        uint32_t *entry_position);
