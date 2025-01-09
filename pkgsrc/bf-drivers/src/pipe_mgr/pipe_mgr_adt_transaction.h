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
 * @file pipe_mgr_adt_transaction.h
 * @date
 *
 * Action data table transaction handling definitions.
 */

/* Standard header includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_err.h>

/* Local header includes */

pipe_status_t pipe_mgr_adt_txn_commit(bf_dev_id_t dev_id,
                                      pipe_adt_tbl_hdl_t tbl_hdl,
                                      bf_dev_pipe_t *pipes_list,
                                      unsigned nb_pipes);

pipe_status_t pipe_mgr_adt_txn_abort(bf_dev_id_t dev_id,
                                     pipe_adt_tbl_hdl_t tbl_hdl,
                                     bf_dev_pipe_t *pipes_list,
                                     unsigned nb_pipes);
