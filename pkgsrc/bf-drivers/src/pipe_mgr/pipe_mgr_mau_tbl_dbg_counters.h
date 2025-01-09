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
 * @file pipe_mgr_mau_tbl_dbg_counters.h
 * @date
 *
 * Contains definitions of MAU table dbg counter
 *
 */
#ifndef _PIPE_MGR_MAU_TBL_DBG_COUNTERS_H
#define _PIPE_MGR_MAU_TBL_DBG_COUNTERS_H

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* ---- FUNCTONS ---- */

/**
 * Table debug counter DB init
 * @param dev_id The ASIC id.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_tbl_dbg_counter_init(bf_dev_id_t dev);

/**
 * Table debug counter DB cleanup
 * @param dev_id The ASIC id.
 * @return Status of the API call.
 */
pipe_status_t pipe_mgr_tbl_dbg_counter_cleanup(bf_dev_id_t dev);

/**
 * Table debug counter type to string
 * @param type Counter type.
 * @param buf buffer to write the string
 * @return Status of the API call.
 */
char *pipe_mgr_tbl_dbg_counter_type_to_string(bf_tbl_dbg_counter_type_t type,
                                              char *buf);

/**
 * Table name to direction
 * @param dev      The ASIC id.
 * @param pipe     The pipe id.
 * @param tbl_name Table name.
 * @return direction
 */
int pipe_mgr_tbl_name_to_dir(bf_dev_id_t dev,
                             bf_dev_pipe_t pipe,
                             char *tbl_name);

#endif
