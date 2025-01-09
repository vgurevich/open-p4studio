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


#ifndef _BF_RT_MIRROR_TABLE_IMPL_HPP
#define _BF_RT_MIRROR_TABLE_IMPL_HPP

#include <bf_rt_common/bf_rt_table_impl.hpp>
#include "bf_rt_mirror_table_data_impl.hpp"
#include "bf_rt_mirror_table_key_impl.hpp"

namespace bfrt {

class BfRtInfo;

/* Mirror configuration table */
class BfRtMirrorCfgTable : public BfRtTableObj {
 public:
  BfRtMirrorCfgTable(const std::string &program_name,
                     bf_rt_id_t id,
                     std::string name,
                     const size_t &size)
      : BfRtTableObj(program_name,
                     id,
                     name,
                     size,
                     TableType::MIRROR_CFG,
                     std::set<TableApi>{
                         TableApi::ADD,
                         TableApi::MODIFY,
                         TableApi::DELETE,
                         TableApi::GET,
                         TableApi::GET_FIRST,
                         TableApi::GET_NEXT_N,
                         TableApi::USAGE_GET,
                         TableApi::CLEAR,
                     }){};
  bf_status_t tableEntryAdd(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryMod(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            const BfRtTableData &data) const override final;

  bf_status_t tableEntryDel(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key) const override final;

  bf_status_t tableEntryGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            const BfRtTableKey &key,
                            BfRtTableData *data) const override final;

  bf_status_t tableEntryGetFirst(const BfRtSession &session,
                                 const bf_rt_target_t &dev_tgt,
                                 const uint64_t &flags,
                                 BfRtTableKey *key,
                                 BfRtTableData *data) const override final;

  bf_status_t tableEntryGetNext_n(const BfRtSession &session,
                                  const bf_rt_target_t &dev_tgt,
                                  const uint64_t &flags,
                                  const BfRtTableKey &key,
                                  const uint32_t &n,
                                  keyDataPairs *key_data_pairs,
                                  uint32_t *num_returned) const override final;

  bf_status_t tableUsageGet(const BfRtSession &session,
                            const bf_rt_target_t &dev_tgt,
                            const uint64_t &flags,
                            uint32_t *count) const;
  bf_status_t tableClear(const BfRtSession &session,
                         const bf_rt_target_t &dev_tgt,
                         const uint64_t & /*flags*/) const override final;

  bf_status_t keyAllocate(
      std::unique_ptr<BfRtTableKey> *key_ret) const override final;

  bf_status_t dataAllocate(
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t dataAllocate(
      const std::vector<bf_rt_id_t> &fields,
      const bf_rt_id_t &action_id,
      std::unique_ptr<BfRtTableData> *data_ret) const override final;

  bf_status_t keyReset(BfRtTableKey *key) const override final;

  bf_status_t dataReset(BfRtTableData *data) const override final;

  bf_status_t dataReset(const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override final;

  bf_status_t dataReset(const std::vector<bf_rt_id_t> &fields,
                        const bf_rt_id_t &action_id,
                        BfRtTableData *data) const override final;

  // This API is more than enough to enable action APIs
  // on this table
  bool actionIdApplicable() const override { return true; };

  bf_status_t tableEntryGetInternal(const BfRtSession &session,
                                    const dev_target_t pipe_dev_tgt,
                                    const bf_mirror_session_info_t &s_info,
                                    const bf_mirror_id_t &session_id,
                                    const bool &session_enable,
                                    BfRtTableData *data) const;

  bf_status_t tableEntryModInternal(const BfRtSession &session,
                                    const bf_rt_target_t &dev_tgt,
                                    const BfRtTableKey &key,
                                    const BfRtTableData &data) const;

 private:
  // Static maps to convert between string and enum types for
  // mirror session direction
  static const std::map<std::string, bf_mirror_direction_e>
      mirror_str_to_dir_map;
  static const std::map<bf_mirror_direction_e, std::string>
      mirror_dir_to_str_map;

  // Static maps to convert between string and enum types for
  // packet color
  static const std::map<std::string, bf_tm_color_t>
      mirror_str_to_packet_color_map;
  static const std::map<bf_tm_color_t, std::string>
      mirror_packet_color_to_str_map;
};

}  // namespace bfrt
#endif  //_BF_RT_MIRROR_TABLE_IMPL_HPP
