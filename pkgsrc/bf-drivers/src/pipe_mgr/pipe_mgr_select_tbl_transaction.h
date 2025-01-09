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
 * @file pipe_mgr_select_tbl_transaction.h
 * @date
 *
 *
 * Contains definitions relating to pipe mgr's selector table management
 */
#ifndef _PIPE_MGR_SELECT_TBL_TRANSACTION_H
#define _PIPE_MGR_SELECT_TBL_TRANSACTION_H

pipe_status_t pipe_mgr_sel_discard_all(sel_tbl_t *sel_tbl, sel_tbl_t *bsel_tbl);

pipe_status_t pipe_mgr_sel_restore_all(sel_tbl_t *sel_tbl, sel_tbl_t *bsel_tbl);

pipe_status_t pipe_mgr_sel_grp_backup_one_refcount(
    sel_tbl_info_t *sel_tbl_info,
    sel_tbl_t *sel_tbl,
    pipe_sel_grp_hdl_t sel_grp_hdl);

pipe_status_t pipe_mgr_sel_grp_backup_one(sel_tbl_t *sel_tbl,
                                          pipe_sel_grp_hdl_t sel_grp_hdl);

pipe_status_t pipe_mgr_sel_grp_mbr_backup_one(
    sel_tbl_info_t *sel_tbl_info,
    sel_tbl_t *sel_tbl,
    sel_grp_info_t *sel_grp,
    pipe_sel_grp_mbr_hdl_t grp_mbr_hdl);

pipe_status_t pipe_mgr_sel_backup_fallback_entry(sel_tbl_t *sel_tbl);

#endif
