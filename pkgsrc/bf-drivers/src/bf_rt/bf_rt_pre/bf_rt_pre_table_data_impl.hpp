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


#ifndef _BF_RT_PRE_TABLE_DATA_IMPL_HPP
#define _BF_RT_PRE_TABLE_DATA_IMPL_HPP

#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>

#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt_common/bf_rt_utils.hpp>
#include <bf_rt_common/bf_rt_table_field_utils.hpp>

#include "bf_rt_mc_mgr_intf.hpp"

namespace bfrt {

class BfRtPREMGIDTableData : public BfRtTableDataObj {
 public:
  BfRtPREMGIDTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtPREMGIDTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bool> &arr) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bool> *arr) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;

  bf_status_t reset() override final;

  void setMulticastNodeIds(const std::vector<bf_rt_id_t> &id) {
    multicast_node_ids_ = id;
  }

  void setMulticastNodeL1XIdsValid(const std::vector<bool> &id) {
    node_l1_xids_valid_ = id;
  }

  void setMulticastNodeL1XIds(const std::vector<bf_rt_id_t> &id) {
    node_l1_xids_ = id;
  }

  void setMulticastECMPIds(const std::vector<bf_rt_id_t> &id) {
    multicast_ecmp_ids_ = id;
  }

  void setMulticastECMPL1XIdsValid(const std::vector<bool> &id) {
    ecmp_l1_xids_valid_ = id;
  }

  void setMulticastECMPL1XIds(const std::vector<bf_rt_id_t> &id) {
    ecmp_l1_xids_ = id;
  }

  const std::vector<bf_rt_id_t> &getMulticastNodeIds() const {
    return multicast_node_ids_;
  }

  const std::vector<bool> &getMulticastNodeL1XIdsValid() const {
    return node_l1_xids_valid_;
  }

  const std::vector<bf_rt_id_t> &getMulticastNodeL1XIds() const {
    return node_l1_xids_;
  }

  const std::vector<bf_rt_id_t> &getMulticastECMPIds() const {
    return multicast_ecmp_ids_;
  }

  const std::vector<bool> &getMulticastECMPL1XIdsValid() const {
    return ecmp_l1_xids_valid_;
  }

  const std::vector<bf_rt_id_t> &getMulticastECMPL1XIds() const {
    return ecmp_l1_xids_;
  }

 private:
  // Each MGID entry has
  //   - zero or more MC nodes
  //         - optional exclusion id for each node
  //   - zero or more MC ECMPs
  //         - optional exclusion id for each ECMP
  //   This also means that we could have an MGID entry created with
  //   just key and no data. Data can be added later on.
  std::vector<bf_rt_id_t> multicast_node_ids_;
  std::vector<bool> node_l1_xids_valid_;
  std::vector<bf_rt_id_t> node_l1_xids_;
  std::vector<bf_rt_id_t> multicast_ecmp_ids_;
  std::vector<bool> ecmp_l1_xids_valid_;
  std::vector<bf_rt_id_t> ecmp_l1_xids_;
};

class BfRtPREMulticastNodeTableData : public BfRtTableDataObj {
 public:
  BfRtPREMulticastNodeTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtPREMulticastNodeTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;
  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;
  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;

  bf_status_t reset() override final;

  void setRId(const bf_mc_rid_t &id) { rid_ = id; }

  void setPortBitmap(const uint8_t *bitmap) {
    std::memcpy(port_bitmap_, bitmap, sizeof(bf_mc_port_map_t));
    dev_port_list_.clear();
    for (int i = 0; i < BF_PORT_COUNT; i++) {
      bf_dev_port_t p = BIT_IDX_TO_DEV_PORT(i);
      int has_port = 0;
      BF_MC_PORT_MAP_GET(port_bitmap_, p, has_port);
      if (has_port) {
        dev_port_list_.push_back(p);
      }
    }
  }
  void setMulticastDevPorts(const std::vector<bf_rt_id_t> &arr) {
    dev_port_list_ = arr;
    std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
    for (auto const &i : dev_port_list_) {
      BF_MC_PORT_MAP_SET(port_bitmap_, i);
    }
  }

  void setLAGBitmap(const uint8_t *bitmap) {
    std::memcpy(lag_bitmap_, bitmap, sizeof(bf_mc_lag_map_t));
    lag_id_list_.clear();
    for (int i = 0; i < BF_LAG_COUNT; i++) {
      int has_lag = 0;
      BF_MC_LAG_MAP_GET(lag_bitmap_, i, has_lag);
      if (has_lag) {
        lag_id_list_.push_back(i);
      }
    }
  }
  void setMulticastLAGIDs(const std::vector<bf_rt_id_t> &arr) {
    lag_id_list_ = arr;
    std::memset(lag_bitmap_, 0, sizeof(bf_mc_lag_map_t));
    for (auto const &i : lag_id_list_) {
      BF_MC_LAG_MAP_SET(lag_bitmap_, i);
    }
  }

  bf_mc_rid_t getRId() const { return rid_; }

  const uint8_t *getPortBitmap() const { return port_bitmap_; }

  const uint8_t *getLAGBitmap() const { return lag_bitmap_; }

  const std::vector<bf_rt_id_t> &getMulticastDevPorts() const {
    return dev_port_list_;
  }

  const std::vector<bf_rt_id_t> &getMulticastLAGIDs() const {
    return lag_id_list_;
  }

 private:
  // Each MC node entry has -
  //   - RID
  //   - list of LAG Ids
  //   - list of DevPorts
  bf_mc_rid_t rid_ = 0;
  bf_mc_lag_map_t lag_bitmap_ = {0};
  bf_mc_port_map_t port_bitmap_ = {0};
  std::vector<bf_rt_id_t> dev_port_list_;
  std::vector<bf_rt_id_t> lag_id_list_;
};

class BfRtPREECMPTableData : public BfRtTableDataObj {
 public:
  BfRtPREECMPTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtPREECMPTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;

  bf_status_t reset() override final;

  void setMulticastNodeIds(const std::vector<bf_rt_id_t> &id) {
    multicast_node_ids_ = id;
  }

  const std::vector<bf_rt_id_t> &getMulticastNodeIds() const {
    return multicast_node_ids_;
  }

 private:
  // Each MC ECMP entry has
  //   - zero or more MC nodes
  //   This also means that we could have an ECMP entry created with
  //   just key and no data. Data can be added later on.
  std::vector<bf_rt_id_t> multicast_node_ids_;
};

class BfRtPRELAGTableData : public BfRtTableDataObj {
 public:
  BfRtPRELAGTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtPRELAGTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  void setPortBitmap(const uint8_t *bitmap) {
    std::memcpy(port_bitmap_, bitmap, sizeof(bf_mc_port_map_t));
    dev_port_list_.clear();
    for (int i = 0; i < BF_PORT_COUNT; i++) {
      bf_dev_port_t p = BIT_IDX_TO_DEV_PORT(i);
      int has_port = 0;
      BF_MC_PORT_MAP_GET(port_bitmap_, p, has_port);
      if (has_port) {
        dev_port_list_.push_back(p);
      }
    }
  }
  void setMulticastDevPorts(const std::vector<bf_rt_id_t> &arr) {
    dev_port_list_ = arr;
    std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
    for (auto const &i : dev_port_list_) {
      BF_MC_PORT_MAP_SET(port_bitmap_, i);
    }
  }

  const uint8_t *getPortBitmap() const { return port_bitmap_; };

  const std::vector<bf_rt_id_t> &getMulticastDevPorts() const {
    return dev_port_list_;
  }

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;
  // Each LAG entry has a list of dev_ports that are part of the LAG entry
  std::vector<bf_rt_id_t> dev_port_list_;
  bf_mc_port_map_t port_bitmap_ = {0};
  uint64_t lag_msb_count = 0;
  uint64_t lag_lsb_count = 0;
};

class BfRtPREMulticastPruneTableData : public BfRtTableDataObj {
 public:
  BfRtPREMulticastPruneTableData(const BfRtTableObj *tbl_obj)
      : BfRtTableDataObj(tbl_obj){};

  ~BfRtPREMulticastPruneTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const std::vector<bf_rt_id_t> &arr) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       std::vector<bf_rt_id_t> *arr) const override final;

  bf_status_t reset() override final;

  void setPortBitmap(const uint8_t *bitmap) {
    std::memcpy(port_bitmap_, bitmap, sizeof(bf_mc_port_map_t));
    dev_port_list_.clear();
    for (int i = 0; i < BF_PORT_COUNT; i++) {
      bf_dev_port_t p = BIT_IDX_TO_DEV_PORT(i);
      int has_port = 0;
      BF_MC_PORT_MAP_GET(port_bitmap_, p, has_port);
      if (has_port) {
        dev_port_list_.push_back(p);
      }
    }
  }

  void setMulticastDevPorts(const std::vector<bf_rt_id_t> &arr) {
    dev_port_list_ = arr;
    std::memset(port_bitmap_, 0, sizeof(bf_mc_port_map_t));
    for (auto const &i : dev_port_list_) {
      BF_MC_PORT_MAP_SET(port_bitmap_, i);
    }
  }

  const uint8_t *getPortBitmap() const { return port_bitmap_; }

  const std::vector<bf_rt_id_t> &getMulticastDevPorts() const {
    return dev_port_list_;
  }

 private:
  // Each MC prune entry has a list of dev_ports that are part of the LAG entry
  std::vector<bf_rt_id_t> dev_port_list_;
  bf_mc_port_map_t port_bitmap_ = {0};
};

class BfRtPREMulticastPortTableData : public BfRtTableDataObj {
 public:
  BfRtPREMulticastPortTableData(const BfRtTableObj *tbl_obj,
                                const std::vector<bf_rt_id_t> &fields)
      : BfRtTableDataObj(tbl_obj) {
    set_active_fields(fields);
  }

  BfRtPREMulticastPortTableData() = default;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const bool &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint64_t &value) override final;

  bf_status_t setValue(const bf_rt_id_t &field_id,
                       const uint8_t *value,
                       const size_t &size) override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       bool *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       uint64_t *value) const override final;

  bf_status_t getValue(const bf_rt_id_t &field_id,
                       const size_t &size,
                       uint8_t *value) const override final;

  bf_status_t reset() override final;

  bf_status_t reset(const std::vector<bf_rt_id_t> &fields) override final;

  const std::set<bf_rt_id_t> &getActiveDataFields() const {
    return activeFields;
  }
  const std::unordered_map<bf_rt_id_t, bool> &getBoolFieldDataMap() const {
    return boolFieldData;
  }
  const std::unordered_map<bf_rt_id_t, uint64_t> &getU64FieldDataMap() const {
    return u64FieldData;
  }

 private:
  bf_status_t setValueInternal(const bf_rt_id_t &field_id,
                               const uint64_t &value,
                               const uint8_t *value_ptr,
                               const size_t &s);
  bf_status_t getValueInternal(const bf_rt_id_t &field_id,
                               uint64_t *value,
                               uint8_t *value_ptr,
                               const size_t &s) const;
  bf_status_t set_active_fields(const std::vector<bf_rt_id_t> &fields);
  bool checkFieldActive(const bf_rt_id_t &field_id) const;
  std::unordered_map<bf_rt_id_t, uint64_t> u64FieldData;
  std::unordered_map<bf_rt_id_t, bool> boolFieldData;
  std::set<bf_rt_id_t> activeFields;
};

}  // namespace bfrt

#endif  // _BF_RT_PRE_TABLE_DATA_IMPL_HPP
