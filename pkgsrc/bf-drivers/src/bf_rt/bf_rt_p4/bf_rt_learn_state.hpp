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


#ifndef _BF_RT_LEARN_STATE_HPP
#define _BF_RT_LEARN_STATE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include "pipe_mgr/pipe_mgr_intf.h"
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <tuple>

#include <bf_rt/bf_rt_learn.h>
#include <bf_rt/bf_rt_learn.hpp>
#include "bf_rt_learn_impl.hpp"

namespace bfrt {

class BfRtStateLearn {
 public:
  BfRtStateLearn(bf_rt_id_t id) : learn_id_(id){};

  void stateLearnSet(const BfRtLearnObj *learn_obj,
                     const std::weak_ptr<BfRtSession> session,
                     bfRtCbFunction callback_cpp,
                     bf_rt_cb_function callback_c,
                     const void *cookie);
  void stateLearnReset();
  std::tuple<const BfRtLearnObj *,
             const std::weak_ptr<BfRtSession>,
             bfRtCbFunction,
             bf_rt_cb_function,
             const void *>
  stateLearnGet();

 private:
  std::mutex state_lock;
  bf_rt_id_t learn_id_ = 0;
  const BfRtLearnObj *learn_obj_ = nullptr;
  std::weak_ptr<BfRtSession> session_obj_;
  bfRtCbFunction callback_cpp_ = nullptr;
  bf_rt_cb_function callback_c_ = nullptr;
  void *cookie_ = nullptr;
};

}  // namespace bfrt

#endif  // _BF_RT_LEARN_STATE_HPP
