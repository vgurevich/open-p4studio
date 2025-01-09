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


#ifndef _TDI_PRE_TABLE_KEY_IMPL_HPP
#define _TDI_PRE_TABLE_KEY_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <tdi/tdi_table.hpp>
#include <tdi/tdi_table_key.hpp>
#include <tdi_common/tdi_table_key_impl.hpp>

namespace tdi {

class TdiPREMGIDTableKey : public TdiTableKeyObj {
 public:
  TdiPREMGIDTableKey(const TdiTableObj *tbl_obj) : TdiTableKeyObj(tbl_obj){};

  ~TdiPREMGIDTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const uint16_t &getId() const { return mgid_; }

  void setId(const uint16_t id) { mgid_ = id; }

  tdi_status_t reset() override final;

 private:
  bf_mc_grp_id_t mgid_ = 0;
};

class TdiPREMulticastNodeTableKey : public TdiTableKeyObj {
 public:
  TdiPREMulticastNodeTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};

  ~TdiPREMulticastNodeTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const uint32_t &getId() const { return multicast_node_id_; }

  void setId(const uint32_t id) { multicast_node_id_ = id; }

  tdi_status_t reset() override final;

 private:
  tdi_id_t multicast_node_id_ = 0;
};

class TdiPREECMPTableKey : public TdiTableKeyObj {
 public:
  TdiPREECMPTableKey(const TdiTableObj *tbl_obj) : TdiTableKeyObj(tbl_obj){};

  ~TdiPREECMPTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const uint32_t &getId() const { return multicast_ecmp_id_; }

  void setId(const uint32_t id) { multicast_ecmp_id_ = id; }

  tdi_status_t reset() override final;

 private:
  tdi_id_t multicast_ecmp_id_ = 0;
};

class TdiPRELAGTableKey : public TdiTableKeyObj {
 public:
  TdiPRELAGTableKey(const TdiTableObj *tbl_obj) : TdiTableKeyObj(tbl_obj){};

  ~TdiPRELAGTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const uint8_t &getId() const { return multicast_lag_id_; }

  void setId(const uint8_t id) { multicast_lag_id_ = id; }

  tdi_status_t reset() override final;

 private:
  bf_mc_lag_id_t multicast_lag_id_ = 0;
};

class TdiPREMulticastPruneTableKey : public TdiTableKeyObj {
 public:
  TdiPREMulticastPruneTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};

  ~TdiPREMulticastPruneTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const uint16_t &getId() const { return multicast_l2_xid_; }

  void setId(const uint16_t id) { multicast_l2_xid_ = id; }

  tdi_status_t reset() override final;

 private:
  bf_mc_l2_xid_t multicast_l2_xid_ = 0;
};

class TdiPREMulticastPortTableKey : public TdiTableKeyObj {
 public:
  TdiPREMulticastPortTableKey(const TdiTableObj *tbl_obj)
      : TdiTableKeyObj(tbl_obj){};

  ~TdiPREMulticastPortTableKey() = default;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint64_t &value) override final;

  tdi_status_t setValue(const tdi_id_t &field_id,
                        const uint8_t *value,
                        const size_t &size) override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        uint64_t *value) const override final;

  tdi_status_t getValue(const tdi_id_t &field_id,
                        const size_t &size,
                        uint8_t *value) const override final;

  const tdi_id_t &getId() const { return dev_port_; }

  void setId(const tdi_id_t id) { dev_port_ = id; }

  tdi_status_t reset() override final;

 private:
  tdi_id_t dev_port_ = 0;
};

}  // namespace tdi

#endif  // _TDI_PRE_TABLE_KEY_IMPL_HPP
