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


#ifndef _TDI_LEARN_STATE_HPP
#define _TDI_LEARN_STATE_HPP

#ifdef __cplusplus
extern "C" {
#endif
#include "pipe_mgr/pipe_mgr_intf.h"
#include <tdi/tdi_common.h>
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

#include <tdi/tdi_learn.h>
#include <tdi/tdi_learn.hpp>
#include "tdi_learn_impl.hpp"

namespace tdi {

class TdiStateLearn {
 public:
  TdiStateLearn(tdi_id_t id) : learn_id_(id){};

  void stateLearnSet(const TdiLearnObj *learn_obj,
                     const std::weak_ptr<TdiSession> session,
                     tdiCbFunction callback_cpp,
                     tdi_cb_function callback_c,
                     const void *cookie);
  void stateLearnReset();
  std::tuple<const TdiLearnObj *,
             const std::weak_ptr<TdiSession>,
             tdiCbFunction,
             tdi_cb_function,
             const void *>
  stateLearnGet();

 private:
  std::mutex state_lock;
  tdi_id_t learn_id_;
  const TdiLearnObj *learn_obj_;
  std::weak_ptr<TdiSession> session_obj_;
  tdiCbFunction callback_cpp_;
  tdi_cb_function callback_c_;
  void *cookie_;
};

}  // namespace tdi

#endif  // _TDI_LEARN_STATE_HPP
