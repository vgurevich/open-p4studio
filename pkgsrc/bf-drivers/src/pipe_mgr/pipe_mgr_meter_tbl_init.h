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
 * @file pipe_mgr_meter_tbl_ini.h
 * @date
 *
 * Definitions related to meter table initialization.
 */

#ifndef _PIPE_MGR_METER_TBL_INIT_H
#define _PIPE_MGR_METER_TBL_INIT_H

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_mgr_int.h"

pipe_status_t pipe_mgr_meter_tbl_init(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                      profile_id_t profile_id,
                                      pipe_bitmap_t *pipe_bmp);

pipe_status_t pipe_mgr_meter_tbl_stage_init(
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    rmt_tbl_info_t *rmt_info);

void pipe_mgr_meter_mgr_tbl_cleanup(bf_dev_id_t device_id,
                                    pipe_meter_tbl_hdl_t meter_tbl_hdl);

void pipe_mgr_meter_mgr_tbl_delete(bf_dev_id_t device_id,
                                   pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                   pipe_mgr_meter_tbl_t *meter_tbl);

pipe_status_t pipe_mgr_meter_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_meter_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_meter_stage_init_shadow_mem(
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    pipe_bitmap_t *pipe_bmp);

pipe_status_t pipe_mgr_meter_init_shadow_mem(pipe_mgr_meter_tbl_t *meter_tbl);

pipe_status_t pipe_mgr_meter_sweep_control(pipe_sess_hdl_t sess_hdl,
                                           rmt_dev_info_t *dev_info,
                                           bool enable);

#endif  // _PIPE_MGR_METER_TBL_INIT_H
