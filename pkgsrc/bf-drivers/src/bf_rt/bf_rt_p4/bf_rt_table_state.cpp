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


#include "bf_rt_table_state.hpp"
#include <bf_rt_common/bf_rt_utils.hpp>

#include <algorithm>

namespace bfrt {
// Reference to next object storage functions
bf_status_t BfRtStateNextRef::setRef(const bf_rt_id_t &session,
                                     const bf_dev_pipe_t &pipe_id,
                                     const pipe_mat_ent_hdl_t &mat_ent_hdl) {
  // Key operations assume max pipe number equal to BF_DEV_PIPE_ALL
  uint32_t key = session << 16;
  key |= pipe_id;
  std::lock_guard<std::mutex> lock(state_lock);
  this->next_ref_[key] = mat_ent_hdl;
  return BF_SUCCESS;
}

bf_status_t BfRtStateNextRef::getRef(const bf_rt_id_t &session,
                                     const bf_dev_pipe_t &pipe_id,
                                     pipe_mat_ent_hdl_t *mat_ent_hdl) const {
  // Key operations assume max pipe number equal to BF_DEV_PIPE_ALL
  uint32_t key = session << 16;
  key |= pipe_id;
  std::lock_guard<std::mutex> lock(state_lock);
  auto handle = this->next_ref_.find(key);
  if (handle == this->next_ref_.end()) {
    return BF_OBJECT_NOT_FOUND;
  }
  *mat_ent_hdl = handle->second;
  return BF_SUCCESS;
}

}  // namespace bfrt
