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


#ifndef _BF_RT_STATE_IMPL_HPP
#define _BF_RT_STATE_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include <bf_rt/bf_rt_session.hpp>

namespace bfrt {

class BfRtSessionImpl : public BfRtSession {
 public:
  BfRtSessionImpl();

  BfRtSessionImpl(bf_rt_id_t sess_id, bf_rt_id_t mc_sess_id);

  ~BfRtSessionImpl();

  bf_status_t sessionDestroy();

  bf_status_t sessionCompleteOperations() const;

  const bf_rt_id_t &sessHandleGet() const { return session_handle_; }

  const bf_rt_id_t &preSessHandleGet() const { return pre_session_handle_; }

  const bool &isValid() const { return valid_; }

  // Batching functions
  bf_status_t beginBatch() const;

  bf_status_t flushBatch() const;

  bf_status_t endBatch(bool hwSynchronous) const;

  // Transaction functions
  bf_status_t beginTransaction(bool isAtomic) const;

  bf_status_t verifyTransaction() const;

  bf_status_t commitTransaction(bool hwSynchronous) const;

  bf_status_t abortTransaction() const;

  // Hidden
  bf_status_t sessionCreateInternal();
  const bool &isInBatch() const { return in_batch_; }
  const bool &isInPipeBatch() const { return in_pipe_mgr_batch_; }
  const bool &isInMcBatch() const { return in_mc_mgr_batch_; }
  void setPipeBatch(const bool batch) const { in_pipe_mgr_batch_ = batch; }
  void setMcBatch(const bool batch) const { in_mc_mgr_batch_ = batch; }

 private:
  mutable bool in_pipe_mgr_batch_;
  mutable bool in_mc_mgr_batch_;
  mutable bool in_batch_;
  bf_rt_id_t session_handle_;      // Pipe mgr session handle
  bf_rt_id_t pre_session_handle_;  // MC mgr (PRE) session handle
  bool valid_;
};

}  // namespace bfrt

#endif  // _BF_RT_STATE_IMPL_HPP
