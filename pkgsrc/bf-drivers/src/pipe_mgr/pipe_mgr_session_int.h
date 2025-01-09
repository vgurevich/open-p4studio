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
 * @file pipe_mgr_session_int.h
 * @date
 *
 * Session related definitions of pipeline manager
 */

#ifndef _PIPE_MGR_SESSION_INT_H
#define _PIPE_MGR_SESSION_INT_H

/* Module header files */

/* Local header files */
#include "pipe_mgr_int.h"
#include "pipe_mgr_sm.h"

#define PIPE_MGR_SESS_FLAG_VALID (1 << 0)
#define PIPE_MGR_SESS_FLAG_DELETE (1 << 1) /* session can be destroyed */
#define PIPE_MGR_SESS_FLAG_RECFG (1 << 2)  /* Fast-reconfig in progress */

#define PIPE_MGR_SESS_VALID(flags) (flags & PIPE_MGR_SESS_FLAG_VALID)
#define PIPE_MGR_SESS_SET_VALID(flags) (flags |= PIPE_MGR_SESS_FLAG_VALID)
#define PIPE_MGR_SESS_CLR_VALID(flags) (flags &= ~PIPE_MGR_SESS_FLAG_VALID)
#define PIPE_MGR_SESS_DELETE(flags) (flags & PIPE_MGR_SESS_FLAG_DELETE)
#define PIPE_MGR_SESS_SET_DELETE(flags) (flags |= PIPE_MGR_SESS_FLAG_DELETE)
#define PIPE_MGR_SESS_CLR_DELETE(flags) (flags &= ~PIPE_MGR_SESS_FLAG_DELETE)
#define PIPE_MGR_SESS_RECFG(flags) (flags & PIPE_MGR_SESS_FLAG_RECFG)
#define PIPE_MGR_SESS_SET_RECFG(flags) (flags |= PIPE_MGR_SESS_FLAG_RECFG)
#define PIPE_MGR_SESS_CLR_RECFG(flags) (flags &= ~PIPE_MGR_SESS_FLAG_RECFG)

typedef struct pipe_mgr_sess_ctx {
  pipe_sess_hdl_t hdl;
  uint32_t flags;
  bool batchInProg;
  bool txnInProg;
  bool txnIsAtom;
  pipe_mgr_rmutex_t api_in_prog;
  pipe_mgr_sm_tbl_instance_t *tables[PIPE_MGR_NUM_DEVICES];
  pipe_mgr_sm_mir_info_t *mir_ses[PIPE_MGR_NUM_DEVICES];
  struct pipe_mgr_sm_move_list_hdr *txn_mls[PIPE_MGR_NUM_DEVICES];
} pipe_mgr_sess_ctx_t;

/* Exported routines */

pipe_mgr_sess_ctx_t *pipe_mgr_get_sess_ctx(pipe_sess_hdl_t hdl,
                                           const char *where,
                                           const int line);

bool pipe_mgr_valid_session(pipe_sess_hdl_t *sess_hdl,
                            const char *where,
                            const int line);
bool pipe_mgr_session_exists(pipe_sess_hdl_t sess_hdl);

pipe_status_t pipe_mgr_create_session(pipe_sess_hdl_t *sess_hdl);

pipe_status_t pipe_mgr_destroy_session(pipe_sess_hdl_t hdl);

pipe_status_t pipe_mgr_api_enter(pipe_sess_hdl_t hdl);

void pipe_mgr_api_exit(pipe_sess_hdl_t hdl);

void pipe_mgr_exclusive_api_enter(bf_dev_id_t dev_id);

void pipe_mgr_exclusive_api_exit(bf_dev_id_t dev_id);

bool pipe_mgr_sess_in_txn(pipe_sess_hdl_t hdl);

bool pipe_mgr_sess_in_atomic_txn(pipe_sess_hdl_t hdl);

bool pipe_mgr_sess_in_batch(pipe_sess_hdl_t hdl);

#endif /* _PIPE_MGR_SESSION_INT_H */
