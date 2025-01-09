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


#ifndef _BF_RT_TABLE_STATE_HPP
#define _BF_RT_TABLE_STATE_HPP

#include "pipe_mgr/pipe_mgr_intf.h"
#include <bf_rt/bf_rt_common.h>

#include <bf_rt_common/bf_rt_table_data_impl.hpp>

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace bfrt {
// This class stores handles for GetNext_n function calls in case
// of previously returned keys were deleted from device.
class BfRtStateNextRef {
 public:
  BfRtStateNextRef(bf_rt_id_t tbl_id) : table_id(tbl_id){};

  bf_status_t setRef(const bf_rt_id_t &session,
                     const bf_dev_pipe_t &pipe_id,
                     const pipe_mat_ent_hdl_t &mat_ent_hdl);
  bf_status_t getRef(const bf_rt_id_t &session,
                     const bf_dev_pipe_t &pipe_id,
                     pipe_mat_ent_hdl_t *mat_ent_hdl) const;

 private:
  bf_rt_id_t table_id;
  mutable std::mutex state_lock;

  // Store handle for GetNext_n function call per pipe.
  // Key is build using session number in upper 16 bits,
  // and pipe number for lower 16 bits.
  // (session_id << 16 | pipe_id)
  std::unordered_map<uint32_t, pipe_mat_ent_hdl_t> next_ref_;
};

}  // namespace bfrt

#endif  // _BF_RT_TABLE_STATE_HPP
