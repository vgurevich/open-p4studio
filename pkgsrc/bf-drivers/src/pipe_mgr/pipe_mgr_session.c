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
 * @file pipe_mgr_session.c
 * @date
 *
 * Implementation of pipeline management sessions
 */

/* Module header files */
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>

/* Local header files */
#include "pipe_mgr_log.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

bool pipe_mgr_valid_session(pipe_sess_hdl_t *sess_hdl,
                            const char *where,
                            const int line) {
  if (NULL == sess_hdl) {
    LOG_ERROR("No session handle at %s:%d", where, line);
    return false;
  }
  if (*sess_hdl >= PIPE_MGR_MAX_SESSIONS) {
    LOG_ERROR(
        "Session handle (%u) out of range at %s:%d", *sess_hdl, where, line);
    return false;
  }
  if (!PIPE_MGR_SESS_VALID(pipe_mgr_ctx->pipe_mgr_sessions[*sess_hdl].flags)) {
    LOG_ERROR("Session handle (%u) invalid at %s:%d", *sess_hdl, where, line);
    return false;
  }
  return true;
}

bool pipe_mgr_session_exists(pipe_sess_hdl_t sess_hdl) {
  if (sess_hdl >= PIPE_MGR_MAX_SESSIONS) {
    return false;
  }
  if (!PIPE_MGR_SESS_VALID(pipe_mgr_ctx->pipe_mgr_sessions[sess_hdl].flags)) {
    return false;
  }
  return true;
}

pipe_mgr_sess_ctx_t *pipe_mgr_get_sess_ctx(pipe_sess_hdl_t hdl,
                                           const char *where,
                                           const int line) {
  if (!pipe_mgr_valid_session(&hdl, where, line)) return NULL;
  return &(pipe_mgr_ctx->pipe_mgr_sessions[hdl]);
}

pipe_status_t pipe_mgr_create_session(pipe_sess_hdl_t *sess_hdl) {
  int i;
  pipe_status_t ret = PIPE_MAX_SESSIONS_EXCEEDED;

  PIPE_MGR_LOCK(&pipe_mgr_ctx->ses_list_mtx);
  for (i = 0; i < PIPE_MGR_MAX_SESSIONS; i++) {
    if (PIPE_MGR_SESS_VALID(pipe_mgr_ctx->pipe_mgr_sessions[i].flags)) {
      continue;
    }
    PIPE_MGR_SESS_SET_VALID(pipe_mgr_ctx->pipe_mgr_sessions[i].flags);
    *sess_hdl = i;
    ret = PIPE_SUCCESS;
    break;
  }
  PIPE_MGR_UNLOCK(&pipe_mgr_ctx->ses_list_mtx);
  return ret;
}

pipe_status_t pipe_mgr_destroy_session(pipe_sess_hdl_t hdl) {
  if (hdl >= PIPE_MGR_MAX_SESSIONS) {
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_LOCK(&pipe_mgr_ctx->ses_list_mtx);
  if (PIPE_MGR_SESS_VALID(pipe_mgr_ctx->pipe_mgr_sessions[hdl].flags)) {
    if (PIPE_MGR_SESS_RECFG(pipe_mgr_ctx->pipe_mgr_sessions[hdl].flags))
      PIPE_MGR_SESS_SET_DELETE(pipe_mgr_ctx->pipe_mgr_sessions[hdl].flags);
    else
      PIPE_MGR_SESS_CLR_VALID(pipe_mgr_ctx->pipe_mgr_sessions[hdl].flags);
    PIPE_MGR_UNLOCK(&pipe_mgr_ctx->ses_list_mtx);
    return PIPE_SUCCESS;
  }
  PIPE_MGR_UNLOCK(&pipe_mgr_ctx->ses_list_mtx);

  return PIPE_INVALID_ARG;
}

pipe_status_t pipe_mgr_api_enter(pipe_sess_hdl_t hdl) {
  if (hdl >= PIPE_MGR_MAX_SESSIONS) {
    LOG_ERROR("Session handle (%u) out of range", hdl);
    PIPE_MGR_DBGCHK(hdl < PIPE_MGR_MAX_SESSIONS);
    return PIPE_SESSION_NOT_FOUND;
  }
  if (!pipe_mgr_valid_session(&hdl, __func__, __LINE__)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_SESSION_NOT_FOUND;
  }

  /* Serialize all the threads which may be sharing this session since we have
   * checks later for table reservation which are only on session handle. */
  PIPE_MGR_LOCK_R(&pipe_mgr_ctx->pipe_mgr_sessions[hdl].api_in_prog);

  /* Make sure that we take the lock as a reader to prevent any exclusive APIs
   * (callers of pipe_mgr_exclusive_api_enter) from executing while we are using
   * the device. */
  PIPE_MGR_RW_MUTEX_RDLOCK(&pipe_mgr_ctx->api_lock);

  return PIPE_SUCCESS;
}

void pipe_mgr_api_exit(pipe_sess_hdl_t hdl) {
  if (hdl >= PIPE_MGR_MAX_SESSIONS) {
    LOG_ERROR("Session handle (%u) out of range", hdl);
    PIPE_MGR_DBGCHK(hdl < PIPE_MGR_MAX_SESSIONS);
  } else {
    PIPE_MGR_UNLOCK_R(&pipe_mgr_ctx->pipe_mgr_sessions[hdl].api_in_prog);
    PIPE_MGR_RW_MUTEX_RDUNLOCK(&pipe_mgr_ctx->api_lock);
  }
}

void pipe_mgr_exclusive_api_enter(bf_dev_id_t dev_id) {
  (void)dev_id;
  PIPE_MGR_RW_MUTEX_WRLOCK(&pipe_mgr_ctx->api_lock);
}

void pipe_mgr_exclusive_api_exit(bf_dev_id_t dev_id) {
  (void)dev_id;
  PIPE_MGR_RW_MUTEX_WRUNLOCK(&pipe_mgr_ctx->api_lock);
}

bool pipe_mgr_sess_in_txn(pipe_sess_hdl_t hdl) {
  if (!pipe_mgr_valid_session(&hdl, __func__, __LINE__)) return false;
  return pipe_mgr_ctx->pipe_mgr_sessions[hdl].txnInProg;
}

bool pipe_mgr_sess_in_atomic_txn(pipe_sess_hdl_t hdl) {
  if (!pipe_mgr_valid_session(&hdl, __func__, __LINE__)) return false;
  return pipe_mgr_ctx->pipe_mgr_sessions[hdl].txnInProg &&
         pipe_mgr_ctx->pipe_mgr_sessions[hdl].txnIsAtom;
}

bool pipe_mgr_sess_in_batch(pipe_sess_hdl_t hdl) {
  if (!pipe_mgr_valid_session(&hdl, __func__, __LINE__)) return false;
  return pipe_mgr_ctx->pipe_mgr_sessions[hdl].batchInProg;
}
