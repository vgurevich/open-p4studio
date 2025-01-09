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
 * @file pipe_mgr_stat_tbl_init.c
 * @date
 *
 *
 * Contains initialization code for statistics tables
 */

#ifndef PIPE_MGR_STAT_TBL_INIT_H
#define PIPE_MGR_STAT_TBL_INIT_H

/* Global header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <target-utils/map/map.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stat_mgr_int.h"

void pipe_mgr_stat_mgr_tbl_cleanup(bf_dev_id_t device_id,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl);

void pipe_mgr_stat_mgr_tbl_delete(bf_dev_id_t device_id,
                                  pipe_mgr_stat_tbl_t *stat_tbl);

pipe_status_t pipe_mgr_stat_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp);

pipe_status_t pipe_mgr_stat_tbl_set_symmetric_mode(
    bf_dev_id_t device_id,
    pipe_stat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp);
#endif
