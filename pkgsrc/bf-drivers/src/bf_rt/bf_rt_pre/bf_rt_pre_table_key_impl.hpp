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


#ifndef _BF_RT_PRE_TABLE_KEY_IMPL_HPP
#define _BF_RT_PRE_TABLE_KEY_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <bf_rt_common/bf_rt_table_key_impl.hpp>

namespace bfrt {

class BfRtPREMGIDTableKey : public BfRtTableKeyObj {
 public:
  BfRtPREMGIDTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPREMGIDTableKey() = default;

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

  const uint16_t &getId() const { return mgid_; }

  void setId(const uint16_t id) { mgid_ = id; }

  bf_status_t reset() override final;

 private:
  bf_mc_grp_id_t mgid_ = 0;
};

class BfRtPREMulticastNodeTableKey : public BfRtTableKeyObj {
 public:
  BfRtPREMulticastNodeTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPREMulticastNodeTableKey() = default;

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

  const uint32_t &getId() const { return multicast_node_id_; }

  void setId(const uint32_t id) { multicast_node_id_ = id; }

  bf_status_t reset() override final;

 private:
  bf_rt_id_t multicast_node_id_ = 0;
};

class BfRtPREECMPTableKey : public BfRtTableKeyObj {
 public:
  BfRtPREECMPTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPREECMPTableKey() = default;

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

  const uint32_t &getId() const { return multicast_ecmp_id_; }

  void setId(const uint32_t id) { multicast_ecmp_id_ = id; }

  bf_status_t reset() override final;

 private:
  bf_rt_id_t multicast_ecmp_id_ = 0;
};

class BfRtPRELAGTableKey : public BfRtTableKeyObj {
 public:
  BfRtPRELAGTableKey(const BfRtTableObj *tbl_obj) : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPRELAGTableKey() = default;

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

  const uint8_t &getId() const { return multicast_lag_id_; }

  void setId(const uint8_t id) { multicast_lag_id_ = id; }

  bf_status_t reset() override final;

 private:
  bf_mc_lag_id_t multicast_lag_id_ = 0;
};

class BfRtPREMulticastPruneTableKey : public BfRtTableKeyObj {
 public:
  BfRtPREMulticastPruneTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPREMulticastPruneTableKey() = default;

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

  const uint16_t &getId() const { return multicast_l2_xid_; }

  void setId(const uint16_t id) { multicast_l2_xid_ = id; }

  bf_status_t reset() override final;

 private:
  bf_mc_l2_xid_t multicast_l2_xid_ = 0;
};

class BfRtPREMulticastPortTableKey : public BfRtTableKeyObj {
 public:
  BfRtPREMulticastPortTableKey(const BfRtTableObj *tbl_obj)
      : BfRtTableKeyObj(tbl_obj){};

  ~BfRtPREMulticastPortTableKey() = default;

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

  const bf_rt_id_t &getId() const { return dev_port_; }

  void setId(const bf_rt_id_t id) { dev_port_ = id; }

  bf_status_t reset() override final;

 private:
  bf_rt_id_t dev_port_ = 0;
};

}  // namespace bfrt

#endif  // _BF_RT_PRE_TABLE_KEY_IMPL_HPP
