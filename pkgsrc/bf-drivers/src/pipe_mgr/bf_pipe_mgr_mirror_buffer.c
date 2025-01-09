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


#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_mirror_intf.h>

#include "pipe_mgr_int.h"
#include "pipe_mgr_bitmap.h"
#include "pipe_mgr_mirror_buffer_comm.h"
#include "pipe_mgr_mirror_buffer.h"
#include "pipe_mgr_drv_intf.h"

static pipe_status_t api_setup(pipe_sess_hdl_t shdl,
                               bf_dev_id_t dev_id,
                               bf_mirror_id_t sid) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(shdl))) {
    return ret;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    ret = PIPE_INVALID_ARG;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      if (sid == 0) {
        /* Mirror session id 0 is reserved. Return error for all mirror APIs */
        LOG_WARN(
            "Device %d: mirror session id 0 is reserved and cannot be "
            "accessed.",
            dev_id);
        ret = PIPE_INVALID_ARG;
        goto done;
      }
    default:
      break;
  }

  if (pipe_mgr_sess_in_txn(shdl)) {
    ret = PIPE_TXN_NOT_SUPPORTED;
    goto done;
  }
  /* Checkpoint the instruction list before an API when batching. */
  pipe_mgr_drv_ilist_chkpt(shdl);
  /* Reserve the mirror session for this session for the duration of the API
   * (or duration of the batch when batching APIs). */
  ret = pipe_mgr_verify_mir_ses_access(shdl, dev_id, sid, true);
  if (PIPE_SUCCESS != ret) goto done;

  return PIPE_SUCCESS;
done:
  pipe_mgr_api_exit(shdl);
  return ret;
}

static pipe_status_t api_end(pipe_sess_hdl_t shdl, pipe_status_t api_sts) {
  if (PIPE_SUCCESS != api_sts) {
    if (pipe_mgr_sess_in_batch(shdl)) {
      pipe_mgr_drv_ilist_rollback(shdl);
    } else {
      pipe_mgr_sm_release(shdl);
      pipe_mgr_drv_ilist_abort(&shdl);
    }
  } else if (!pipe_mgr_sess_in_batch(shdl)) {
    pipe_mgr_sm_release(shdl);
    api_sts = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);
  }
  pipe_mgr_api_exit(shdl);
  return api_sts;
}

pipe_status_t bf_mirror_session_set(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_target,
                                    bf_mirror_id_t sid,
                                    bf_mirror_session_info_t *s_info,
                                    bool enable) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, s_info->mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;
  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;
  ret = pipe_mgr_mirror_buf_mirror_session_set(
      sess_hdl, dev_target, sid, s_info, enable);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_reset(pipe_sess_hdl_t sess_hdl,
                                      dev_target_t dev_target,
                                      bf_mirror_id_t sid) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;
  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirror_buf_mirror_session_reset(sess_hdl, dev_target, sid);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_enable(pipe_sess_hdl_t sess_hdl,
                                       dev_target_t dev_target,
                                       bf_mirror_direction_e dir,
                                       bf_mirror_id_t sid) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_INVALID_ARG;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_INVALID_ARG;
  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirr_sess_en_or_dis(
      sess_hdl, dev_target, dir, sid, true /*enable*/);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_disable(pipe_sess_hdl_t sess_hdl,
                                        dev_target_t dev_target,
                                        bf_mirror_direction_e dir,
                                        bf_mirror_id_t sid) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirr_sess_en_or_dis(
      sess_hdl, dev_target, dir, sid, false /*enable*/);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_max_mirror_sessions_get(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                bf_mirror_type_e mirror_type,
                                                bf_mirror_id_t *sid) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (sid == NULL) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_get_max_mirror_sessions(device_id, mirror_type, sid);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}
pipe_status_t bf_mirror_base_mirror_session_id_get(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t device_id,
                                                   bf_mirror_type_e mirror_type,
                                                   bf_mirror_id_t *sid) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (sid == NULL) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_get_base_mirror_session_id(device_id, mirror_type, sid);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t bf_mirror_session_meta_flag_update(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;
  ret = pipe_mgr_mirror_session_meta_flag_update(
      sess_hdl, dev_target, sid, mirror_flag, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}
pipe_status_t bf_mirror_session_meta_flag_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_target,
                                              bf_mirror_id_t sid,
                                              bf_mirror_meta_flag_e mirror_flag,
                                              bool *value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  if (!value) return PIPE_INVALID_ARG;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;
  ret = pipe_mgr_mirror_session_meta_flag_get(
      sess_hdl, dev_target, sid, mirror_flag, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_priority_update(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_target,
                                                bf_mirror_id_t sid,
                                                bool value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirror_session_pri_update(sess_hdl, dev_target, sid, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}
pipe_status_t bf_mirror_session_priority_get(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_target,
                                             bf_mirror_id_t sid,
                                             bool *value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (!value) return PIPE_INVALID_ARG;
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirror_session_pri_get(sess_hdl, dev_target, sid, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_coal_mode_update(pipe_sess_hdl_t sess_hdl,
                                                 dev_target_t dev_target,
                                                 bf_mirror_id_t sid,
                                                 bool value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirror_session_coal_mode_update(
      sess_hdl, dev_target, sid, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}
pipe_status_t bf_mirror_session_coal_mode_get(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_target,
                                              bf_mirror_id_t sid,
                                              bool *value) {
  pipe_status_t ret;
  bf_mirror_id_t sid_max;
  bf_mirror_type_e mirror_type =
      pipe_mgr_mirror_buf_mirror_type_get(dev_target, sid);
  if (mirror_type == BF_MIRROR_TYPE_MAX) return PIPE_OBJ_NOT_FOUND;
  if (!value) return PIPE_INVALID_ARG;
  ret = pipe_mgr_get_max_mirror_sessions(
      dev_target.device_id, mirror_type, &sid_max);
  if ((PIPE_SUCCESS != ret) || (sid > sid_max)) return PIPE_OBJ_NOT_FOUND;

  ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirror_session_coal_mode_get(sess_hdl, dev_target, sid, value);

  ret = api_end(sess_hdl, ret);
  return ret;
}

bf_status_t bf_mirror_session_mcast_pipe_vector_set(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_id_t sid,
                                                    int logical_pipe_vector) {
  pipe_status_t ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirr_sess_pipe_vector_set(
      sess_hdl, dev_target, sid, logical_pipe_vector);

  ret = api_end(sess_hdl, ret);
  return ret;
}

bf_status_t bf_mirror_session_mcast_pipe_vector_get(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_id_t sid,
                                                    int *logical_pipe_vector) {
  pipe_status_t ret = api_setup(sess_hdl, dev_target.device_id, sid);
  if (PIPE_SUCCESS != ret) return ret;

  ret = pipe_mgr_mirr_sess_pipe_vector_get(
      sess_hdl, dev_target, sid, logical_pipe_vector);

  ret = api_end(sess_hdl, ret);
  return ret;
}

pipe_status_t bf_mirror_session_get(pipe_sess_hdl_t sess_hdl,
                                    dev_target_t dev_target,
                                    bf_mirror_id_t sid,
                                    bf_mirror_session_info_t *s_info) {
  pipe_status_t ret = PIPE_SUCCESS;

  if (s_info == NULL) return PIPE_INVALID_ARG;

  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }

  ret = pipe_mgr_mirror_session_get(dev_target, sid, s_info);

  pipe_mgr_api_exit(sess_hdl);

  return ret;
}

pipe_status_t bf_mirror_session_enable_get(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_target,
                                           bf_mirror_id_t sid,
                                           bool *session_enable) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (session_enable == NULL) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_mirror_session_enable_get(dev_target, sid, session_enable);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t bf_mirror_session_get_first(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_target,
                                          bf_mirror_session_info_t *s_info,
                                          bf_mirror_get_id_t *first) {
  pipe_status_t ret = PIPE_SUCCESS;
  if ((s_info == NULL) || (first == NULL)) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_mirror_session_get_first(
      dev_target, s_info, &first->sid, &first->pipe_id);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t bf_mirror_session_get_next(pipe_sess_hdl_t sess_hdl,
                                         dev_target_t dev_target,
                                         bf_mirror_get_id_t current,
                                         bf_mirror_session_info_t *next_info,
                                         bf_mirror_get_id_t *next) {
  pipe_status_t ret = PIPE_SUCCESS;
  if ((next_info == NULL) || (next == NULL)) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_mirror_session_get_next(dev_target,
                                         current.sid,
                                         current.pipe_id,
                                         next_info,
                                         &next->sid,
                                         &next->pipe_id);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t bf_mirror_session_get_count(pipe_sess_hdl_t sess_hdl,
                                          dev_target_t dev_target,
                                          uint32_t *count) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (count == NULL) return PIPE_INVALID_ARG;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_mirror_session_get_count(dev_target, count);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}

pipe_status_t bf_mirror_ecc_correct(pipe_sess_hdl_t sess_hdl,
                                    bf_dev_id_t device_id,
                                    bf_dev_pipe_t phy_pipe_id,
                                    bf_mirror_id_t sid) {
  pipe_status_t ret = PIPE_SUCCESS;
  if (PIPE_SUCCESS != (ret = pipe_mgr_api_enter(sess_hdl))) {
    return ret;
  }
  ret = pipe_mgr_mirror_ecc_correct(sess_hdl, device_id, phy_pipe_id, sid);
  pipe_mgr_api_exit(sess_hdl);
  return ret;
}
