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


#ifndef _BF_RT_TM_STATE_HPP
#define _BF_RT_TM_STATE_HPP

#include <map>
#include <mutex>
#include <unordered_map>

#include "bf_rt_tm_intf.hpp"

namespace bfrt {

typedef std::map<bf_tm_ppg_id_t, bf_tm_ppg_hdl> BfRtTMPpgIdHdlMap;
typedef std::map<bf_tm_ppg_hdl, bf_tm_ppg_id_t> BfRtTMPpgHdlIdMap;

// ppg counter id to user ppg id map (and reverse)
typedef std::map<bf_tm_ppg_id_t, bf_tm_ppg_id_t> BfRtTMPpgCntMap;

class BfRtTMPpgStateObj {
 public:
  bf_status_t stateTMPpgAdd(const bf_dev_pipe_t &pipe_id,
                            const bf_tm_ppg_id_t &ppg_id,
                            const bf_tm_ppg_hdl &ppg_hdl);

  bf_status_t stateTMPpgCntAdd(const bf_dev_pipe_t &pipe_id,
                               const bf_tm_ppg_id_t &ppg_cnt_id,
                               const bf_tm_ppg_id_t &ppg_id);

  bf_status_t atomicTMPpgAdd(const bf_dev_pipe_t &pipe_id,
                             const bf_tm_ppg_id_t &ppg_id,
                             const bf_tm_ppg_hdl &ppg_hdl);

  bf_status_t stateTMPpgDel(const bf_dev_pipe_t &pipe_id,
                            const bf_tm_ppg_id_t &ppg_id);

  bf_status_t atomicTMPpgDel(const bf_dev_pipe_t &pipe_id,
                             const bf_tm_ppg_id_t &ppg_id);

  bf_status_t stateTMPpgCntDel(const bf_dev_pipe_t &pipe_id,
                               const bf_tm_ppg_id_t &ppg_cnt_id);

  bf_status_t stateTMPpgGet(const bf_dev_pipe_t &pipe_id,
                            const bf_tm_ppg_id_t &ppg_id,
                            bf_tm_ppg_hdl *ppg_hdl) const;

  bf_status_t atomicTMPpgGet(const bf_dev_pipe_t &pipe_id,
                             const bf_tm_ppg_id_t &ppg_id,
                             bf_tm_ppg_hdl *ppg_hdl) const;

  bf_status_t stateTMPpgHdlGet(const bf_dev_pipe_t &pipe_id,
                               const bf_tm_ppg_hdl &ppg_hdl,
                               bf_tm_ppg_id_t *ppg_id) const;

  bf_status_t stateTMCntPpgGet(const bf_dev_pipe_t &pipe_id,
                               const bf_tm_ppg_id_t &ppg_cnt_id,
                               bf_tm_ppg_id_t *ppg_id) const;

  bf_status_t stateTMPpgGetFirst(const bf_dev_pipe_t &pipe_id,
                                 bf_tm_ppg_id_t &ppg_id,
                                 bf_tm_ppg_hdl &ppg_hdl) const;

  bf_status_t atomicTMPpgGetFirst(const bf_dev_pipe_t &pipe_id,
                                  bf_tm_ppg_id_t &ppg_id,
                                  bf_tm_ppg_hdl &ppg_hdl) const;

  bf_status_t stateTMPpgGetNext(const bf_dev_pipe_t &pipe_id,
                                const bf_tm_ppg_id_t &ppg_id,
                                bf_tm_ppg_id_t &ppg_id_next,
                                bf_tm_ppg_hdl &ppg_hdl_next) const;

  bf_status_t atomicTMPpgGetNext(const bf_dev_pipe_t &pipe_id,
                                 const bf_tm_ppg_id_t &ppg_id,
                                 bf_tm_ppg_id_t &ppg_id_next,
                                 bf_tm_ppg_hdl &ppg_hdl_next) const;

  bf_status_t atomicSizeGet(const bf_dev_pipe_t &pipe_id,
                            std::size_t &size) const;

  bf_status_t stateSizeGet(const bf_dev_pipe_t &pipe_id,
                           std::size_t &size) const;

 public:
  // The state object lock. Take it before calling the state* methods.
  // The atomic* methods do set the lock for itself.
  mutable std::mutex state_lock;

 private:
  // Mapping from external ppg_id to internal TM PPG handlers.
  // Each pipe has its independent map.
  // The state_lock is common for all pipes.
  // Consider to split locks per-pipe if needed.
  std::unordered_map<bf_dev_pipe_t, BfRtTMPpgIdHdlMap> pipe_ppg_map;
  std::unordered_map<bf_dev_pipe_t, BfRtTMPpgHdlIdMap> pipe_hdl_map;

  // Mapping from ppg counter id to user defined ppg id (and reverse, for each
  // pipe)
  std::unordered_map<bf_dev_pipe_t, BfRtTMPpgCntMap> pipe_cnt_to_ppg;
};

}  // namespace bfrt
#endif
