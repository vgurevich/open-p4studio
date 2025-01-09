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


#ifndef _TDI_TOFINO_SESSION_HPP
#define _TDI_TOFINO_SESSION_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <unordered_map>
#include <algorithm>

#include <tdi/common/tdi_session.hpp>

namespace tdi {
namespace tna {
namespace tofino {

class Session : public tdi::Session {
 public:
  Session(const std::vector<tdi_mgr_type_e> &mgr_type_list);

  virtual ~Session();

  virtual tdi_status_t create() override;
  virtual tdi_status_t destroy() override;

  virtual tdi_status_t completeOperations() const override;

  virtual tdi_handle_t handleGet(const tdi_mgr_type_e &mgr_type) const override;
  // Batching functions
  virtual tdi_status_t beginBatch() const override;

  virtual tdi_status_t flushBatch() const override;

  virtual tdi_status_t endBatch(bool hwSynchronous) const override;

  // Transaction functions
  virtual tdi_status_t beginTransaction(bool isAtomic) const override;

  virtual tdi_status_t verifyTransaction() const override;

  virtual tdi_status_t commitTransaction(bool hwSynchronous) const override;

  virtual tdi_status_t abortTransaction() const override;

  // Hidden
  const bool &isInBatch() const { return in_batch_; }
  const bool &isInPipeBatch() const { return in_pipe_mgr_batch_; }
  const bool &isInMcBatch() const { return in_mc_mgr_batch_; }
  void setPipeBatch(const bool batch) const { in_pipe_mgr_batch_ = batch; }
  void setMcBatch(const bool batch) const { in_mc_mgr_batch_ = batch; }

 private:
  mutable bool in_pipe_mgr_batch_{false};
  mutable bool in_mc_mgr_batch_{false};
  mutable bool in_batch_{false};
  bool mc_mgr_skip_{false};
  tdi_handle_t session_handle_;      // Pipe mgr session handle
  tdi_handle_t pre_session_handle_;  // MC mgr (PRE) session handle
};

}  // namespace tofino
}  // namespace tna
}  // namespace tdi

#endif  // _TDI_TOFINO_SESSION_HPP
