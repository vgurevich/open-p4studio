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


#include <iostream>

#include <bf_rt/bf_rt_session.hpp>
#include "bf_rt_learn_state.hpp"

namespace bfrt {

void BfRtStateLearn::stateLearnSet(const BfRtLearnObj *learn_obj,
                                   const std::weak_ptr<BfRtSession> session,
                                   bfRtCbFunction callback_cpp,
                                   bf_rt_cb_function callback_c,
                                   const void *cookie) {
  std::lock_guard<std::mutex> lock(state_lock);
  learn_obj_ = learn_obj;
  callback_cpp_ = callback_cpp;
  callback_c_ = callback_c;
  session_obj_ = session;
  cookie_ = const_cast<void *>(cookie);
}

void BfRtStateLearn::stateLearnReset() {
  std::lock_guard<std::mutex> lock(state_lock);
  learn_obj_ = nullptr;
  callback_cpp_ = nullptr;
  callback_c_ = nullptr;
  session_obj_.reset();
  cookie_ = nullptr;
}

std::tuple<const BfRtLearnObj *,
           const std::weak_ptr<BfRtSession>,
           bfRtCbFunction,
           bf_rt_cb_function,
           const void *>
BfRtStateLearn::stateLearnGet() {
  std::lock_guard<std::mutex> lock(state_lock);
  return std::make_tuple(
      learn_obj_, session_obj_, callback_cpp_, callback_c_, cookie_);
}

}  // namespace bfrt
