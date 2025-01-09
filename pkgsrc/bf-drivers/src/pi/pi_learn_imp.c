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


#include <PI/target/pi_learn_imp.h>

#include "pi_helpers.h"
#include "pi_log.h"

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>

#include <string.h>

static pipe_status_t lrn_notify_cb(pipe_sess_hdl_t session_handle,
                                   pipe_flow_lrn_msg_t *pipe_lrn_msg,
                                   void *cookie) {
  (void)session_handle;

  pi_dev_id_t pi_dev_id = PI_DEV_ID_FROM_PIPE(pipe_lrn_msg->dev_tgt.device_id);
  pi_state_shared_lock(pi_dev_id);
  if (!pi_state_is_device_assigned(pi_dev_id)) {
    LOG_TRACE("%s: ignoring lrn notification because device was removed",
              __func__);
    pipe_sess_hdl_t lrn_session_handle = pi_state_digest_get_lrn_session();
    pipe_status_t status = pipe_mgr_flow_lrn_notify_ack(
        lrn_session_handle, pipe_lrn_msg->flow_lrn_fld_lst_hdl, pipe_lrn_msg);
    pi_state_unlock(pi_dev_id);
    if (status != PIPE_SUCCESS)
      LOG_ERROR("%s: error when acking dropped lrn notification", __func__);
    return status;
  }

  pi_state_digest_state_t *digest_state = (pi_state_digest_state_t *)cookie;
  bf_sys_assert(digest_state != NULL);

  pi_learn_msg_t *pi_msg = bf_sys_malloc(sizeof(pi_learn_msg_t));
  convert_pipe_dev_tgt(pipe_lrn_msg->dev_tgt, &pi_msg->dev_tgt);
  pi_msg->learn_id = digest_state->digest_id;
  pi_msg->num_entries = pipe_lrn_msg->num_entries;
  pi_msg->entry_size = digest_state->entry_size;
  size_t size = pi_msg->num_entries * pi_msg->entry_size;
  // we need to make a copy of all learn entries, because the application can
  // call pi_learn_msg_done and pi_learn_msg_ack in any order.
  pi_msg->entries = bf_sys_malloc(size);
  memcpy(pi_msg->entries, pipe_lrn_msg->entries, size);

  // We use the pointer as the message id. This makes the implementation of
  // _pi_learn_msg_ack very easy.
  // should really be a static_assert, but it is only available in C11
  bf_sys_assert(sizeof(pi_learn_msg_id_t) >= sizeof(uintptr_t));
  pi_msg->msg_id = (pi_learn_msg_id_t)(uintptr_t)pipe_lrn_msg;

  // application takes ownership of pi_msg and pi_msg->entries until it calls
  // pi_learn_msg_done
  pi_status_t pi_status = pi_learn_new_msg(pi_msg);
  if (pi_status == PI_STATUS_LEARN_NO_MATCHING_CB) {
    LOG_TRACE("%s: learn notification dropped because of no application CB",
              __func__);
  } else if (pi_status != PI_STATUS_SUCCESS) {
    LOG_ERROR("%s: learn notification dropped", __func__);
  }

  pi_state_unlock(pi_dev_id);
  return PIPE_SUCCESS;
}

pi_status_t _pi_learn_config_set(pi_session_handle_t session_handle,
                                 pi_dev_id_t dev_id,
                                 pi_p4_id_t learn_id,
                                 const pi_learn_config_t *config) {
  // we do not use this session handle, we use the one we allocated specifically
  // for learning; the session needs to stay open, since pipe_mgr will use this
  // session when calling our leaning callback.
  (void)session_handle;
  pipe_status_t status;

  pipe_sess_hdl_t lrn_session_handle = pi_state_digest_get_lrn_session();
  pi_state_digest_state_t *digest_state = pi_state_digest_get(dev_id, learn_id);
  bf_sys_assert(digest_state != NULL);  // guaranteed by PI library

  if (config == NULL) {
    status = pipe_mgr_lrn_digest_notification_deregister(
        lrn_session_handle,
        PI_DEV_ID_TO_PIPE(dev_id),
        digest_state->digest_handle);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR("%s: failed to deregister callback for digest %u",
                __func__,
                learn_id);
      return PI_STATUS_TARGET_ERROR + status;
    }
    return PI_STATUS_SUCCESS;
  }

  uint32_t lrn_timeout_us = pi_state_digest_update_lrn_timeout(
      dev_id, (config->max_timeout_ns / 1000));
  // we call this function for every call to _pi_learn_config_set (so for every
  // digest in the program), but that should not be an issue
  status = pipe_mgr_flow_lrn_set_timeout(
      lrn_session_handle, PI_DEV_ID_TO_PIPE(dev_id), lrn_timeout_us);
  if (status != PIPE_SUCCESS)
    LOG_ERROR("%s: failed to set timeout for digest %u", __func__, learn_id);

  // ensure that all data is received in network-byte order so no conversion is
  // needed
  status = pipe_mgr_flow_lrn_set_network_order_digest(PI_DEV_ID_TO_PIPE(dev_id),
                                                      true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: failed to enable network order mode for digest %u",
              __func__,
              learn_id);
  }

  status =
      pipe_mgr_lrn_digest_notification_register(lrn_session_handle,
                                                PI_DEV_ID_TO_PIPE(dev_id),
                                                digest_state->digest_handle,
                                                lrn_notify_cb,
                                                (void *)digest_state);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to register callback for digest %u", __func__, learn_id);
    return PI_STATUS_TARGET_ERROR + status;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_learn_msg_ack(pi_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_p4_id_t learn_id,
                              pi_learn_msg_id_t msg_id) {
  (void)session_handle;
  (void)dev_id;
  (void)learn_id;
  pipe_sess_hdl_t lrn_session_handle = pi_state_digest_get_lrn_session();

  pipe_flow_lrn_msg_t *pipe_lrn_msg = (pipe_flow_lrn_msg_t *)(uintptr_t)msg_id;
  pipe_status_t status = pipe_mgr_flow_lrn_notify_ack(
      lrn_session_handle, pipe_lrn_msg->flow_lrn_fld_lst_hdl, pipe_lrn_msg);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: error when acking learn message", __func__);
    return PI_STATUS_TARGET_ERROR + status;
  }
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_learn_msg_done(pi_learn_msg_t *msg) {
  bf_sys_free(msg->entries);
  bf_sys_free(msg);
  return PI_STATUS_SUCCESS;
}

#undef COOKIE_SENTINEL
