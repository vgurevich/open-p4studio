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


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <bf_rt/bf_rt_learn.h>

#ifdef __cplusplus
}
#endif

#include <bf_rt/bf_rt_learn.hpp>
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt_common/bf_rt_session_impl.hpp>
#include <bf_rt_p4/bf_rt_learn_impl.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>
#include "bf_rt_state_c.hpp"

// learn obj apis

bf_status_t bf_rt_learn_callback_register(const bf_rt_learn_hdl *learn_hdl,
                                          const bf_rt_session_hdl *session,
                                          const bf_rt_target_t *dev_tgt,
                                          const bf_rt_cb_function callback_fn,
                                          const void *cookie) {
  // Casting to learnObj here since we need the internal function
  auto learn = reinterpret_cast<const bfrt::BfRtLearnObj *>(learn_hdl);
  // Get the shared_ptr from the state
  auto &c_state = bfrt::bfrt_c::BfRtCFrontEndSessionState::getInstance();
  auto session_ptr = c_state.getSharedPtr(
      reinterpret_cast<const bfrt::BfRtSession *>(session));

  return learn->bfRtLearnCallbackRegisterHelper(
      session_ptr, *dev_tgt, nullptr, callback_fn, cookie);
}

bf_status_t bf_rt_learn_callback_deregister(const bf_rt_learn_hdl *learn_hdl,
                                            const bf_rt_session_hdl *session,
                                            const bf_rt_target_t *dev_tgt) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  // Get the shared_ptr from the state
  auto &c_state = bfrt::bfrt_c::BfRtCFrontEndSessionState::getInstance();
  auto session_ptr = c_state.getSharedPtr(
      reinterpret_cast<const bfrt::BfRtSession *>(session));

  return learn->bfRtLearnCallbackDeregister(session_ptr, *dev_tgt);
}

bf_status_t bf_rt_learn_notify_ack(const bf_rt_learn_hdl *learn_hdl,
                                   const bf_rt_session_hdl *session,
                                   bf_rt_learn_msg_hdl *const learn_msg_hdl) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  // Get the shared_ptr from the state
  auto &c_state = bfrt::bfrt_c::BfRtCFrontEndSessionState::getInstance();
  auto session_ptr = c_state.getSharedPtr(
      reinterpret_cast<const bfrt::BfRtSession *>(session));
  return learn->bfRtLearnNotifyAck(session_ptr, learn_msg_hdl);
}

bf_status_t bf_rt_learn_id_get(const bf_rt_learn_hdl *learn_hdl,
                               bf_rt_id_t *learn_id_ret) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  return learn->learnIdGet(learn_id_ret);
}

bf_status_t bf_rt_learn_name_get(const bf_rt_learn_hdl *learn_hdl,
                                 const char **learn_name_ret) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearnObj *>(learn_hdl);
  *learn_name_ret = learn->getName().c_str();
  return BF_SUCCESS;
}

bf_status_t bf_rt_learn_field_id_list_size_get(const bf_rt_learn_hdl *learn_hdl,
                                               uint32_t *num_ret) {
  // Only here are we using BfRtLearnObj than BfRtLearn
  // since we need the hidden impl function
  auto learn = reinterpret_cast<const bfrt::BfRtLearnObj *>(learn_hdl);
  *num_ret = learn->learnFieldListSize();
  return BF_SUCCESS;
}

bf_status_t bf_rt_learn_field_id_list_get(const bf_rt_learn_hdl *learn_hdl,
                                          bf_rt_id_t *id_vec_ret) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  std::vector<bf_rt_id_t> temp_vec;
  auto status = learn->learnFieldIdListGet(&temp_vec);
  int i = 0;
  for (auto const &item : temp_vec) {
    id_vec_ret[i++] = item;
  }
  return status;
}

bf_status_t bf_rt_learn_field_id_get(const bf_rt_learn_hdl *learn_hdl,
                                     const char *name,
                                     bf_rt_id_t *field_id) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  std::string temp(name);
  return learn->learnFieldIdGet(temp, field_id);
}

bf_status_t bf_rt_learn_field_size_get(const bf_rt_learn_hdl *learn_hdl,
                                       const bf_rt_id_t field_id,
                                       size_t *size) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  return learn->learnFieldSizeGet(field_id, size);
}

bf_status_t bf_rt_learn_field_is_ptr_get(const bf_rt_learn_hdl *learn_hdl,
                                         const bf_rt_id_t field_id,
                                         bool *is_ptr) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearn *>(learn_hdl);
  return learn->learnFieldIsPtrGet(field_id, is_ptr);
}

bf_status_t bf_rt_learn_field_name_get(const bf_rt_learn_hdl *learn_hdl,
                                       const bf_rt_id_t field_id,
                                       const char **field_name) {
  auto learn = reinterpret_cast<const bfrt::BfRtLearnObj *>(learn_hdl);
  if (learn == nullptr) {
    return BF_UNEXPECTED;
  }
  auto field = learn->getLearnField(field_id);
  if (field == nullptr) {
    return BF_UNEXPECTED;
  }
  *field_name = field->getName().c_str();
  return BF_SUCCESS;
}
