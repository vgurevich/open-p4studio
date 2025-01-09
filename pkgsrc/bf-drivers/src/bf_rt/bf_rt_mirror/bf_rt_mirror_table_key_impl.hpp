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


#ifndef _BF_RT_MIRROR_TABLE_KEY_IMPL_HPP
#define _BF_RT_MIRROR_TABLE_KEY_IMPL_HPP

#include <bf_rt_common/bf_rt_table_key_impl.hpp>

namespace bfrt {

class BfRtMirrorCfgTableKey : public BfRtTableKeyObj {
 public:
  BfRtMirrorCfgTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};
  ~BfRtMirrorCfgTableKey() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  const bf_mirror_id_t &getId() const { return session_id_; }

  void setId(const bf_mirror_id_t id) { session_id_ = id; }

 private:
  bf_mirror_id_t session_id_ = 0;
};

}  // namespace bfrt
#endif  //_BF_RT_MIRROR_TABLE_KEY_IMPL_HPP
