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


#ifndef _BF_RT_TM_TABLE_IMPL_MIRROR_HPP
#define _BF_RT_TM_TABLE_IMPL_MIRROR_HPP

#include "bf_rt_tm_table_impl_pipe.hpp"

namespace bfrt {

//------------- TM_MIRROR_DPG

class BfRtTMMirrorDpgTable : public BfRtTMPipeTableIntf {
 public:
  BfRtTMMirrorDpgTable(const std::string &program_name,
                       bf_rt_id_t id,
                       std::string name,
                       const size_t &size)
      : BfRtTMPipeTableIntf(program_name,
                            id,
                            name,
                            size,
                            TableType::TM_MIRROR_DPG,
                            std::set<TableApi>{TableApi::DEFAULT_ENTRY_SET,
                                               TableApi::DEFAULT_ENTRY_RESET,
                                               TableApi::DEFAULT_ENTRY_GET,
                                               TableApi::CLEAR}) {}
  ~BfRtTMMirrorDpgTable() = default;

 protected:
  bf_status_t tableGetResetValues(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableGetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      BfRtTMTableData *p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;

  bf_status_t tableSetDefaultFields(
      const bf_rt_target_t &dev_tgt,
      const BfRtTMTableData &p_data,
      std::set<bf_rt_id_t> &wrk_fields) const override final;
};

}  // namespace bfrt
#endif
