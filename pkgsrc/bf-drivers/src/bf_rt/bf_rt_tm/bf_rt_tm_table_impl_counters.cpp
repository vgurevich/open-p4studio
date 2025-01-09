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


#include <bf_rt_common/bf_rt_init_impl.hpp>

#include "bf_rt_tm_table_impl_counters.hpp"

namespace bfrt {

#define BFRT_TM_SET_COUNTER_DATA_FIELD_INT(a_field_name_, p_data_, a_val_) \
  {                                                                        \
    bf_rt_id_t a_field_id_;                                                \
    uint64_t a_field_val_ = static_cast<uint64_t>(a_val_);                 \
                                                                           \
    status = this->dataFieldIdGet((a_field_name_), &a_field_id_);          \
    if (status != BF_SUCCESS) {                                            \
      LOG_ERROR("%s:%d %s has no field %s",                                \
                __func__,                                                  \
                __LINE__,                                                  \
                this->table_name_get().c_str(),                            \
                (a_field_name_));                                          \
      return status;                                                       \
    }                                                                      \
    if (p_data_->checkFieldActive(a_field_id_)) {                          \
      LOG_DBG("%s:%d %s dev_id=%d %s=0x%" PRIX64,                          \
              __func__,                                                    \
              __LINE__,                                                    \
              this->table_name_get().c_str(),                              \
              dev_tgt.dev_id,                                              \
              (a_field_name_),                                             \
              a_field_val_);                                               \
      status = p_data_->setValue(a_field_id_, a_field_val_);               \
      if (status != BF_SUCCESS) {                                          \
        LOG_ERROR("%s:%d %s Can't set data object for %s",                 \
                  __func__,                                                \
                  __LINE__,                                                \
                  this->table_name_get().c_str(),                          \
                  (a_field_name_));                                        \
        return status;                                                     \
      }                                                                    \
      status = BF_SUCCESS;                                                 \
    }                                                                      \
  }

#define BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(                           \
    a_field_name_, p_data_, a_field_has_val_, a_field_val_)              \
  {                                                                      \
    bf_rt_id_t a_field_id_ = 0;                                          \
    status = this->dataFieldIdGet(a_field_name_, &a_field_id_);          \
    if (status != BF_SUCCESS) {                                          \
      LOG_ERROR("%s:%d %s Error in getting fieldId for %s",              \
                __func__,                                                \
                __LINE__,                                                \
                table_name_get().c_str(),                                \
                a_field_name_);                                          \
      return status;                                                     \
    }                                                                    \
    a_field_has_val_ = p_data_.hasValue(a_field_id_);                    \
    if (p_data_.checkFieldActive(a_field_id_) && a_field_has_val_) {     \
      status = p_data_.getValue(a_field_id_, &a_field_val_);             \
      if (status != BF_SUCCESS) {                                        \
        LOG_ERROR("%s:%d %s Error in getting data value for Id %d",      \
                  __func__,                                              \
                  __LINE__,                                              \
                  table_name_get().c_str(),                              \
                  a_field_id_);                                          \
        return status;                                                   \
      }                                                                  \
      auto clear_only_an = Annotation("clear_only", "true");             \
      AnnotationSet annotations{};                                       \
      status = this->dataFieldAnnotationsGet(a_field_id_, &annotations); \
      bool clear_only_field = false;                                     \
      if (status == BF_SUCCESS) {                                        \
        if (annotations.find(clear_only_an) != annotations.end()) {      \
          clear_only_field = true;                                       \
        }                                                                \
      }                                                                  \
      if (clear_only_field && a_field_val_ != 0) {                       \
        LOG_ERROR("%s:%d %s Error in setting value % " PRId64            \
                  " for field %s. This is a clear "                      \
                  "only field.",                                         \
                  __func__,                                              \
                  __LINE__,                                              \
                  table_name_get().c_str(),                              \
                  a_field_val_,                                          \
                  a_field_name_);                                        \
        return BF_INVALID_ARG;                                           \
      }                                                                  \
    }                                                                    \
  }

/*******  Port Counter Table APIs ********/
bf_status_t BfRtTMCounterPortTable::portDropCountGet_helper(
    bf_dev_id_t dev, bf_dev_port_t port, uint64_t *count) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint64_t ig_count = 0;
  uint64_t eg_count = 0;
  bf_status_t status = trafficMgr->bfTmPortDropGetCached(
      dev, DEV_PORT_TO_PIPE(port), port, &ig_count, &eg_count);
  if (status == BF_SUCCESS) {
    if (this->direction == BF_DEV_DIR_INGRESS) {
      *count = ig_count;
    } else if (this->direction == BF_DEV_DIR_EGRESS) {
      *count = eg_count;
    } else {
      status = BF_UNEXPECTED;
      BF_RT_DBGCHK(0);
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPortTable::portUsageGet_helper(bf_dev_id_t dev,
                                                        bf_dev_port_t port,
                                                        uint32_t *count) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t ig_count = 0;
  uint32_t eg_count = 0;
  uint32_t ignore = 0;
  status = trafficMgr->bfTmPortUsageGet(dev,
                                        DEV_PORT_TO_PIPE(port),
                                        port,
                                        &ig_count,
                                        &eg_count,
                                        &ignore,
                                        &ignore);

  if (status == BF_SUCCESS) {
    if (this->direction == BF_DEV_DIR_INGRESS) {
      *count = ig_count;
    } else if (this->direction == BF_DEV_DIR_EGRESS) {
      *count = eg_count;
    } else {
      status = BF_UNEXPECTED;
      BF_RT_DBGCHK(0);
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPortTable::portWatermarkGet_helper(
    bf_dev_id_t dev, bf_dev_port_t port, uint32_t *count) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;
  uint32_t ig_wm = 0;
  uint32_t eg_wm = 0;
  uint32_t ignore = 0;
  status = trafficMgr->bfTmPortUsageGet(
      dev, DEV_PORT_TO_PIPE(port), port, &ignore, &ignore, &ig_wm, &eg_wm);

  if (status == BF_SUCCESS) {
    if (this->direction == BF_DEV_DIR_INGRESS) {
      *count = ig_wm;
    } else if (this->direction == BF_DEV_DIR_EGRESS) {
      *count = eg_wm;
    } else {
      status = BF_UNEXPECTED;
      BF_RT_DBGCHK(0);
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPortTable::portDropCountSet_helper(
    bf_dev_id_t dev,
    bf_dev_port_t port,
    const uint64_t &drop_count_packets) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (this->direction == BF_DEV_DIR_INGRESS) {
    return trafficMgr->bfTmPortDropIngressCacheSet(
        dev, port, drop_count_packets);
  } else if (this->direction == BF_DEV_DIR_EGRESS) {
    return trafficMgr->bfTmPortDropEgressCacheSet(
        dev, port, drop_count_packets);
  }
  BF_RT_DBGCHK(0);
  return BF_UNEXPECTED;
}

bf_status_t BfRtTMCounterPortTable::portWatermarktClear_helper(
    bf_dev_id_t dev, bf_dev_port_t port) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (this->direction == BF_DEV_DIR_INGRESS) {
    return trafficMgr->bfTmPortIngressWatermarkClear(dev, port);
  } else if (this->direction == BF_DEV_DIR_EGRESS) {
    return trafficMgr->bfTmPortEgressWatermarkClear(dev, port);
  }
  BF_RT_DBGCHK(0);
  return BF_UNEXPECTED;
}

bf_status_t BfRtTMCounterPortTable::tableEntryMod_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_dev_port_t dev_port,
    const BfRtTMTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Check if the port was added and enabled
  bool is_enabled = false;
  bool has_mac = false;
  status = TrafficMgrIntf::getInstance()->bfTMPortStatusGet(
      dev_tgt.dev_id, dev_port, NULL, &is_enabled, NULL, NULL, &has_mac);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Failed to get status for dev=%d, port=%d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_port);
    return status;
  }

  // Clear the counters only if the port was enabled
  if (is_enabled) {
    uint64_t drop_count_packets = std::numeric_limits<uint64_t>::max();
    bool drop_count_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("drop_count_packets",
                                          data,
                                          drop_count_packets_has_value,
                                          drop_count_packets);
    if (drop_count_packets_has_value) {
      status = this->portDropCountSet_helper(
          dev_tgt.dev_id, dev_port, drop_count_packets);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to set drop_count_packets = %" PRIu64
                  " for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  drop_count_packets,
                  dev_port);
        return status;
      }
    }

    uint64_t watermark_cells = std::numeric_limits<uint64_t>::max();
    bool watermark_cells_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "watermark_cells", data, watermark_cells_has_value, watermark_cells);

    if (watermark_cells_has_value && watermark_cells == 0) {
      status = this->portWatermarktClear_helper(dev_tgt.dev_id, dev_port);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to clear watermark_cells for dev_port %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_port);
        return status;
      }
    }
  }

  return status;
}

bf_status_t BfRtTMCounterPortTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  const BfRtTMDevPortKey &portCounterKey =
      static_cast<const BfRtTMDevPortKey &>(key);

  bf_dev_port_t dev_port;
  auto status = portCounterKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch and valiate the data fields
  const BfRtTMTableData &portCounterData =
      static_cast<const BfRtTMTableData &>(data);

  // Modify the entry
  std::lock_guard<std::mutex> lock(this->entry_lock);
  return this->tableEntryMod_helper(dev_tgt, dev_port, portCounterData);
}

bf_status_t BfRtTMCounterPortTable::tableEntryGet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_dev_port_t dev_port,
    BfRtTableData *data) const {
  std::lock_guard<std::mutex> lock(this->entry_lock);

  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMTableData *portCounterData = static_cast<BfRtTMTableData *>(data);

  // TM API allows dev_port out of regular port range.
  // A special case might be the mirror port which is #72.
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (BF_SUCCESS != trafficMgr->bfTMPortIsValid(dev_tgt.dev_id, dev_port)) {
    LOG_ERROR("%s:%d %s Incorrect key dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_port);
    return BF_INVALID_ARG;
  }

  // Check if the port was added and enabled
  bf_status_t status = BF_SUCCESS;
  bool is_enabled = false;
  bool has_mac = false;
  status = trafficMgr->bfTMPortStatusGet(
      dev_tgt.dev_id, dev_port, NULL, &is_enabled, NULL, NULL, &has_mac);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Failed to get status for dev=%d, port=%d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_port);
    return status;
  } else if (!is_enabled) {
    // If the port is not enabled, zero out the entry and return success
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
        "drop_count_packets", portCounterData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("usage_cells", portCounterData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", portCounterData, 0);
    return BF_SUCCESS;
  }

  // Fetch the drop count and set it in data
  uint64_t drop_count = 0;
  status = this->portDropCountGet_helper(dev_tgt.dev_id, dev_port, &drop_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get drop count",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", portCounterData, drop_count);

  // Fetch the usage cells and set it in data
  uint32_t usage_cells = 0;
  status = this->portUsageGet_helper(dev_tgt.dev_id, dev_port, &usage_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get usage cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "usage_cells", portCounterData, usage_cells);

  // Fetch the watermark value and set it in data
  uint32_t watermark_cells = 0;
  status =
      this->portWatermarkGet_helper(dev_tgt.dev_id, dev_port, &watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get watermark cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "watermark_cells", portCounterData, watermark_cells);

  return status;
}

bf_status_t BfRtTMCounterPortTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  const BfRtTMDevPortKey &portCounterKey =
      static_cast<const BfRtTMDevPortKey &>(key);

  bf_dev_port_t dev_port;
  auto status = portCounterKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return this->tableEntryGet_helper(dev_tgt, dev_port, data);
}

bf_status_t BfRtTMCounterPortTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMDevPortKey *portCounterKey = static_cast<BfRtTMDevPortKey *>(key);
  bf_rt_id_t key_id;
  auto status = this->keyFieldIdGet("dev_port", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find out the first dev port
  bf_dev_port_t first_dev_port;
  bf_dev_pipe_t first_pipe;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  status = trafficMgr->bfTMPipeGetFirst(dev_tgt.dev_id, &first_pipe);
  if (BF_SUCCESS == status) {
    status = trafficMgr->bfTMPipeGetPortFirst(
        dev_tgt.dev_id, first_pipe, &first_dev_port);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Failed to get first port on dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              first_pipe);
    return status;
  }

  status = portCounterKey->setValue(key_id, first_dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in setting key object for dev_port field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_helper(dev_tgt, first_dev_port, data);
}

bf_status_t BfRtTMCounterPortTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMDevPortKey &portCounterKey =
      static_cast<const BfRtTMDevPortKey &>(key);
  bf_dev_port_t dev_port;
  auto status = portCounterKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // TM API allows dev_port out of regular port range.
  // A special case might be the mirror port which is #72.
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (BF_SUCCESS != trafficMgr->bfTMPortIsValid(dev_tgt.dev_id, dev_port)) {
    LOG_ERROR("%s:%d %s Incorrect key dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_port);
    return BF_INVALID_ARG;
  }

  bf_rt_target_t dev_tgt_curr = dev_tgt;
  uint32_t i = 0;
  for (; i < n; i++) {
    dev_tgt_curr.pipe_id = DEV_PORT_TO_PIPE(dev_port);
    bf_dev_port_t dev_port_next = 0;

    status = trafficMgr->bfTMPipeGetPortNext(
        dev_tgt_curr.dev_id, dev_port, &dev_port_next);
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more dev ports on this pipe, try the next pipe
      status = trafficMgr->bfTMPipeGetNext(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
      if (BF_OBJECT_NOT_FOUND == status) {
        // No more dev ports left on the device
        status = BF_SUCCESS;
        break;
      }

      // Get the first port
      status = trafficMgr->bfTMPipeGetPortFirst(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &dev_port_next);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s Failed to get first port on dev_id=%d pipe_id=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_tgt_curr.dev_id,
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Set the key field
    auto this_key = static_cast<BfRtTMDevPortKey *>((*key_data_pairs)[i].first);
    this_key->setId(dev_port_next);

    // Set the data fields
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);
    status = tableEntryGet_helper(dev_tgt_curr, dev_port_next, this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port_next);

      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    dev_port = dev_port_next;
  }

  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMCounterPortTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  // Clear the counters for all dev ports
  std::lock_guard<std::mutex> lock(this->entry_lock);

  bf_dev_port_t dev_port;
  bf_dev_pipe_t first_pipe;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPipeGetFirst(dev_tgt.dev_id, &first_pipe);
  if (BF_SUCCESS == status) {
    status =
        trafficMgr->bfTMPipeGetPortFirst(dev_tgt.dev_id, first_pipe, &dev_port);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Failed to get first port on dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  // Create a data object with the reset values
  BfRtTMTableData portCounterData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", (&portCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", (&portCounterData), 0);

  bf_rt_target_t dev_tgt_curr = dev_tgt;
  do {
    // Clear the entry
    status =
        this->tableEntryMod_helper(dev_tgt_curr, dev_port, portCounterData);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear port counters for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      return status;
    }

    // Find the next dev port
    dev_tgt_curr.pipe_id = DEV_PORT_TO_PIPE(dev_port);
    bf_dev_port_t dev_port_next = 0;
    status = trafficMgr->bfTMPipeGetPortNext(
        dev_tgt_curr.dev_id, dev_port, &dev_port_next);
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more dev ports on this pipe, try the next pipe
      status = trafficMgr->bfTMPipeGetNext(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
      if (BF_OBJECT_NOT_FOUND == status) {
        // No more dev ports left on the device
        status = BF_SUCCESS;
        break;
      }
      status = trafficMgr->bfTMPipeGetPortFirst(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &dev_port_next);
    }
    dev_port = dev_port_next;
  } while (status == BF_SUCCESS);

  return status;
}

bf_status_t BfRtTMCounterPortTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  uint16_t curr_count = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPipeGetPortCount(
      dev_tgt.dev_id, BF_DEV_PIPE_ALL, &curr_count);
  if (status) {
    LOG_ERROR("%s:%d %s Failed to get get number of ports on dev=%d pipe=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }
  *count = static_cast<uint32_t>(curr_count);

  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPortTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMDevPortKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPortTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtTMDevPortKey *match_key = static_cast<BfRtTMDevPortKey *>(key);
  return match_key->reset();
}

/*******  Queue Counter Table APIs ********/
bf_status_t BfRtTMCounterQueueTable::queueDropCountGet_helper(
    bf_dev_id_t dev,
    const bf_dev_port_t &dev_port,
    const bf_tm_queue_t &queue_nr,
    uint64_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  return trafficMgr->bfTmQDropGetCached(
      dev, DEV_PORT_TO_PIPE(dev_port), dev_port, queue_nr, count);
}

bf_status_t BfRtTMCounterQueueTable::tableEntryMod_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_tm_pg_t &pg_id,
    const uint8_t &pg_queue,
    const BfRtTMTableData &data) const {
  // Validation
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_tgt_curr = dev_tgt;
  if (dev_tgt_curr.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_tgt_curr.dev_id,
                                          &(dev_tgt_curr.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Failed to get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id);
      return status;
    }
  }

  // Check if drop count was provided and if yes, fetch the value
  uint64_t drop_count_packets = std::numeric_limits<uint64_t>::max();
  bool drop_count_packets_has_value = false;
  BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("drop_count_packets",
                                        data,
                                        drop_count_packets_has_value,
                                        drop_count_packets);

  // Check if watermark was provided and if yes, fetch the value
  uint64_t watermark_cells = std::numeric_limits<uint64_t>::max();
  bool watermark_cells_has_value = false;
  BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
      "watermark_cells", data, watermark_cells_has_value, watermark_cells);

  do {
    // Figure out the port id and port specific queue number
    bf_dev_port_t dev_port = 0;
    bf_tm_queue_t queue_nr = 0;
    bool is_mapped = false;
    status = trafficMgr->bfTMPortGroupPortQueueGet(
        dev_tgt_curr, pg_id, pg_queue, &dev_port, &queue_nr, &is_mapped);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s (Pipe %d) pg_id=%d pg_queue=%d Can't get port queue number",
          __func__,
          __LINE__,
          this->table_name_get().c_str(),
          dev_tgt_curr.pipe_id,
          pg_id,
          pg_queue);
      return status;
    }

    if (drop_count_packets_has_value) {
      status = trafficMgr->bfTmQDropCacheSet(dev_tgt_curr.dev_id,
                                             dev_tgt_curr.pipe_id,
                                             dev_port,
                                             queue_nr,
                                             drop_count_packets);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to set drop_count_packets = %" PRIu64
            " for "
            "pg_id=%d, "
            "pg_queue=%d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            drop_count_packets,
            pg_id,
            pg_queue);
        return status;
      }
    }

    if (watermark_cells_has_value && watermark_cells == 0) {
      status = trafficMgr->bfTmQWatermarkClear(
          dev_tgt_curr.dev_id, dev_port, queue_nr);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear watermark_cells for pg_id=%d, "
            "pg_queue=%d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            pg_id,
            pg_queue);
        return status;
      }
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      // Single pipe
      break;
    }

    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more pipes left on this device
      status = BF_SUCCESS;
      break;
    }
  } while (BF_SUCCESS == status);

  return status;
}

bf_status_t BfRtTMCounterQueueTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  const BfRtTMQueueTableKey &queueCounterKey =
      static_cast<const BfRtTMQueueTableKey &>(key);

  // Get the port group id and queue
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;
  auto status = queueCounterKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  const BfRtTMTableData &queueCounterData =
      static_cast<const BfRtTMTableData &>(data);

  // Modify the entry
  std::lock_guard<std::mutex> lock(this->entry_lock);
  return this->tableEntryMod_helper(dev_tgt, pg_id, pg_queue, queueCounterData);
}

bf_status_t BfRtTMCounterQueueTable::tableEntryGet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_tm_pg_t &pg_id,
    const uint8_t &pg_queue,
    BfRtTableData *data) const {
  std::lock_guard<std::mutex> lock(this->entry_lock);

  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Figure out the port id and port specific queue number
  bf_dev_port_t dev_port = 0;
  bf_tm_queue_t queue_nr = 0;
  bool is_mapped = false;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPortGroupPortQueueGet(
      dev_tgt, pg_id, pg_queue, &dev_port, &queue_nr, &is_mapped);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s pg_id=%d pg_queue=%d Can't get port queue number",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              pg_id,
              pg_queue);
    return status;
  }

  // Fetch the drop count and set it in data
  BfRtTMTableData *queueCounterData = static_cast<BfRtTMTableData *>(data);
  uint64_t drop_count = 0;
  status = this->queueDropCountGet_helper(
      dev_tgt.dev_id, dev_port, queue_nr, &drop_count);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to queue get drop count",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", queueCounterData, drop_count);

  // Fetch the usage cells and set it in data
  uint32_t ignore = 0;
  uint32_t usage_cells = 0;
  status = trafficMgr->bfTmQUsageGet(dev_tgt.dev_id,
                                     DEV_PORT_TO_PIPE(dev_port),
                                     dev_port,
                                     queue_nr,
                                     &usage_cells,
                                     &ignore);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get usage cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "usage_cells", queueCounterData, usage_cells);

  // Fetch the watermark value and set it in data
  uint32_t watermark_cells = 0;
  status = trafficMgr->bfTmQUsageGet(dev_tgt.dev_id,
                                     DEV_PORT_TO_PIPE(dev_port),
                                     dev_port,
                                     queue_nr,
                                     &ignore,
                                     &watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get watermark cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "watermark_cells", queueCounterData, watermark_cells);

  return status;
}

bf_status_t BfRtTMCounterQueueTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  const BfRtTMQueueTableKey &queueCounterKey =
      static_cast<const BfRtTMQueueTableKey &>(key);

  // Get the port group id and queue
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;
  auto status = queueCounterKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Validate the device target
  status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Populate the data fields
  return this->tableEntryGet_helper(dev_tgt, pg_id, pg_queue, data);
}

bf_status_t BfRtTMCounterQueueTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMQueueTableKey *queueCounterKey =
      static_cast<BfRtTMQueueTableKey *>(key);

  // Populate the key fields
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;
  queueCounterKey->setId(pg_id, pg_queue);

  // Validate the device target
  auto status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Populate the data fields
  return this->tableEntryGet_helper(dev_tgt, pg_id, pg_queue, data);
}

bf_status_t BfRtTMCounterQueueTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMQueueTableKey &queueCounterKey =
      static_cast<const BfRtTMQueueTableKey &>(key);

  // Get the port group id and queue
  bf_tm_pg_t pg_id = 0;
  uint8_t pg_queue = 0;
  auto status = queueCounterKey.getId(pg_id, pg_queue);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Validate the device target
  status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch the TM config
  uint8_t mau_pipes = 0;
  uint8_t queues_per_pg = 0;
  uint8_t pg_per_pipe = 0;
  status = this->tmDevCfgGet(
      dev_tgt.dev_id, &mau_pipes, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error fetching TM configuration", __func__, __LINE__);
    return status;
  }

  uint32_t i = 0;
  pg_queue++;
  for (; pg_id < pg_per_pipe && i < n && status == BF_SUCCESS; pg_id++) {
    for (; pg_queue < queues_per_pg && i < n; i++) {
      // Set the key field
      auto this_key =
          static_cast<BfRtTMQueueTableKey *>((*key_data_pairs)[i].first);
      this_key->setId(pg_id, pg_queue);

      // Set the data fields
      auto this_data =
          static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);
      status = tableEntryGet_helper(dev_tgt, pg_id, pg_queue, this_data);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in getting data of pg_id=%d, pg_queue=%d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  pg_id,
                  pg_queue);

        // Make the data object null if error
        (*key_data_pairs)[i].second = nullptr;
      }
      pg_queue++;
    }
    pg_queue = 0;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMCounterQueueTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  // Get the TM configuration
  uint8_t pg_per_pipe = 0;
  uint8_t queues_per_pg = 0;
  auto status = this->tmDevCfgGet(
      dev_tgt.dev_id, nullptr, &pg_per_pipe, &queues_per_pg, nullptr);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error fetching TM configuration", __func__, __LINE__);
    return status;
  }

  // Create a data object with the reset values
  BfRtTMTableData queueCounterData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", (&queueCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", (&queueCounterData), 0);

  std::lock_guard<std::mutex> lock(this->entry_lock);
  for (auto pg_id = 0; pg_id < pg_per_pipe; pg_id++) {
    for (auto queue_id = 0; queue_id < queues_per_pg; queue_id++) {
      status = this->tableEntryMod_helper(
          dev_tgt, pg_id, queue_id, queueCounterData);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Failed to clear queue counters for pg_id=%d, "
            "queue_id=%d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            pg_id,
            queue_id);
        return status;
      }
    }
  }
  return status;
}

bf_status_t BfRtTMCounterQueueTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterQueueTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMQueueTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterQueueTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtTMQueueTableKey *match_key = static_cast<BfRtTMQueueTableKey *>(key);
  return match_key->reset();
}

/*******  Pool Counter Table APIs ********/
bf_status_t BfRtTMCounterPoolTable::tableEntryMod_helper(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    const BfRtTMTableData &data) const {
  // Validate the app pool
  auto pool_id_itr = appPoolMap.find(pool);
  if (pool_id_itr == appPoolMap.end()) {
    LOG_TRACE("%s:%d %s Error : Invalid app pool name %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pool.c_str());
    return BF_INVALID_ARG;
  }
  bf_tm_app_pool_t app_pool = pool_id_itr->second;

  // Fetch the value
  bf_status_t status = BF_SUCCESS;
  uint64_t watermark_cells = std::numeric_limits<uint64_t>::max();
  bool watermark_cells_has_value = false;
  BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
      "watermark_cells", data, watermark_cells_has_value, watermark_cells);

  // Update the counter
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (watermark_cells_has_value && watermark_cells == 0) {
    status = trafficMgr->bfTmPoolWatermarkClear(dev_tgt.dev_id, app_pool);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear watermark_cells for pool=%s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPoolTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  const BfRtTMPoolAppTableKey &poolCounterKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);

  // Get the pool value
  std::string pool = "";
  auto status = poolCounterKey.getId(pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch and valiate the data fields
  const BfRtTMTableData &poolCounterData =
      static_cast<const BfRtTMTableData &>(data);

  // Modify the entry
  return this->tableEntryMod_helper(dev_tgt, pool, poolCounterData);
}

bf_status_t BfRtTMCounterPoolTable::tableEntryGet_helper(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Figure out the app pool id
  auto pool_id_itr = appPoolMap.find(pool);
  if (pool_id_itr == appPoolMap.end()) {
    LOG_ERROR("%s:%d %s Error: Invalid pool value %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pool.c_str());
    return BF_INVALID_ARG;
  }
  bf_tm_app_pool_t app_pool = pool_id_itr->second;

  // Fetch the fields and set it in data
  BfRtTMTableData *poolCounterData = static_cast<BfRtTMTableData *>(data);
  uint32_t usage_cells = 0;
  uint32_t watermark_cells = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTmPoolUsageGet(
      dev_tgt.dev_id, app_pool, &usage_cells, &watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to fetch pool usage for %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pool.c_str());
    return status;
  }

  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "usage_cells", poolCounterData, usage_cells);

  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "watermark_cells", poolCounterData, watermark_cells);

  return status;
}

bf_status_t BfRtTMCounterPoolTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  const BfRtTMPoolAppTableKey &poolCounterKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);

  // Get the pool value
  std::string pool = "";
  auto status = poolCounterKey.getId(pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Populate the data fields
  return this->tableEntryGet_helper(dev_tgt, pool, data);
}

bf_status_t BfRtTMCounterPoolTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  BfRtTMPoolAppTableKey *poolCounterKey =
      static_cast<BfRtTMPoolAppTableKey *>(key);

  // Get the first entry key value
  std::vector<std::string> pools;
  auto status = this->getKeyStringChoices("pool", pools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  std::string first_key_value = pools.front();

  // Populate the key field
  poolCounterKey->setId(first_key_value);

  // Populate the data fields
  return this->tableEntryGet_helper(dev_tgt, first_key_value, data);
}

bf_status_t BfRtTMCounterPoolTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMPoolAppTableKey &poolCounterKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);

  // Find all possible pool choices
  std::vector<std::string> pools;
  auto status = this->getKeyStringChoices("pool", pools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the pool value
  std::string pool = "";
  status = poolCounterKey.getId(pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find index of the pool in pools vector
  uint32_t index;
  auto it = find(pools.begin(), pools.end(), pool);
  if (it != pools.end()) {
    index = (it - pools.begin()) + 1;
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool is not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint32_t i;
  for (i = 0; i < n; i++) {
    if (index >= pools.size()) {
      break;
    }

    auto this_key =
        static_cast<BfRtTMPoolAppTableKey *>((*key_data_pairs)[i].first);
    this_key->setId(pools[index]);

    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);
    status = this->tableEntryGet_helper(dev_tgt, pools[index], this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pools[index].c_str());

      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    index++;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMCounterPoolTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  // Find all possible pool choices
  std::vector<std::string> pools;
  auto status = this->getKeyStringChoices("pool", pools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Create a data object with the reset values
  BfRtTMTableData poolCounterData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", (&poolCounterData), 0);

  // Clear the counters for all pools
  for (auto &pool : pools) {
    status = this->tableEntryMod_helper(dev_tgt, pool, poolCounterData);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear pool counters for %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPoolTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPoolTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPoolAppTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPoolTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtTMPoolAppTableKey *match_key = static_cast<BfRtTMPoolAppTableKey *>(key);
  return match_key->reset();
}

/******* Pipe Counter Table APIs ********/
bf_status_t BfRtTMCounterPipeTable::tableDefaultEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Validation - PIPE_ALL is not allowed
  auto status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch the fields and set it in data
  BfRtTMTableData *pipeCounterData = static_cast<BfRtTMTableData *>(data);

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Number of packets which were dropped because of buffer full condition
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint64_t total_buffer_full_drop_packets = 0;
  status = trafficMgr->bfTmPipeBufferFullDropGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &total_buffer_full_drop_packets);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s (Pipe %d) Failed to get total_buffer_full_drop_packets",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id);
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("total_buffer_full_drop_packets",
                                     pipeCounterData,
                                     total_buffer_full_drop_packets);

  // Discard queue current usage and watermark in terms of number of cells
  uint32_t discard_queue_usage_cells = 0;
  uint32_t discard_queue_watermark_cells = 0;
  status = trafficMgr->bfTmQDiscardUsageGet(dev_tgt.dev_id,
                                            dev_tgt.pipe_id,
                                            &discard_queue_usage_cells,
                                            &discard_queue_watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s (Pipe %d) Failed to get discard queue usage",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id);
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "discard_queue_usage_cells", pipeCounterData, discard_queue_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("discard_queue_watermark_cells",
                                     pipeCounterData,
                                     discard_queue_watermark_cells);

  // Number of total cells and packets received
  uint64_t total_cells = 0;
  uint64_t total_packets = 0;
  status = trafficMgr->bfTmPipeCountersGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &total_cells, &total_packets);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s (Pipe %d) Failed to get cell and packet count",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id);
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "total_cells", pipeCounterData, total_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "total_packets", pipeCounterData, total_packets);

  // Unicast and multicast cut through packet count
  uint64_t uc_ct_packets = 0;
  uint64_t mc_ct_packets = 0;
  status = trafficMgr->bfTmCutThroughCountersGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &uc_ct_packets, &mc_ct_packets);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s (Pipe %d) Failed to get uc/mc packet count.",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id);
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "uc_ct_packets", pipeCounterData, uc_ct_packets);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_ct_packets", pipeCounterData, mc_ct_packets);

  // Block level counters
  bf_tm_blklvl_cntrs_t blk_cntrs;
  std::memset(&blk_cntrs, 0, sizeof(bf_tm_blklvl_cntrs_t));
  status = trafficMgr->bfTmBlklvlDropGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &blk_cntrs);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s (Pipe %d) Failed to get block level drop counters.",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev_tgt.pipe_id);
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "ig_no_dest_drop_packets", pipeCounterData, blk_cntrs.wac_no_dest_drop);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "ig_buf_full_drop_packets", pipeCounterData, blk_cntrs.wac_buf_full_drop);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("eg_total_drop_packets",
                                     pipeCounterData,
                                     blk_cntrs.egress_pipe_total_drop);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_queue_drop_packets", pipeCounterData, blk_cntrs.qac_pre_mc_drop);

  // PRE FIFO counters
  bf_tm_pre_fifo_cntrs_t fifo_cntrs;
  std::memset(&fifo_cntrs, 0, sizeof(bf_tm_pre_fifo_cntrs_t));
  status = trafficMgr->bfTmPreFifoDropGet(dev_tgt.dev_id, &fifo_cntrs);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get FIFO counters.",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  uint64_t *mc_input_fifo_drop_packets = nullptr;
  switch (dev_tgt.pipe_id) {
    case 0:
      mc_input_fifo_drop_packets = fifo_cntrs.wac_drop_cnt_pre0_fifo;
      break;
    case 1:
      mc_input_fifo_drop_packets = fifo_cntrs.wac_drop_cnt_pre1_fifo;
      break;
    case 2:
      mc_input_fifo_drop_packets = fifo_cntrs.wac_drop_cnt_pre2_fifo;
      break;
    case 3:
      mc_input_fifo_drop_packets = fifo_cntrs.wac_drop_cnt_pre3_fifo;
      break;
    default:
      LOG_ERROR("%s:%d %s Invalid pipe_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.pipe_id);
      return BF_INVALID_ARG;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("mc_input_fifo_0_drop_packets",
                                     pipeCounterData,
                                     mc_input_fifo_drop_packets[0]);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("mc_input_fifo_1_drop_packets",
                                     pipeCounterData,
                                     mc_input_fifo_drop_packets[1]);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("mc_input_fifo_2_drop_packets",
                                     pipeCounterData,
                                     mc_input_fifo_drop_packets[2]);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("mc_input_fifo_3_drop_packets",
                                     pipeCounterData,
                                     mc_input_fifo_drop_packets[3]);

  return status;
}

bf_status_t BfRtTMCounterPipeTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Fetch the fields and set it in data
  const BfRtTMTableData &pipeCounterData =
      static_cast<const BfRtTMTableData &>(data);
  return tableEntrySet_helper(dev_tgt, pipeCounterData);
}

bf_status_t BfRtTMCounterPipeTable::tableEntrySet_helper(
    const bf_rt_target_t &dev_tgt, const BfRtTMTableData &data) const {
  // Validation
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_tgt_curr = dev_tgt;
  if (dev_tgt_curr.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_tgt_curr.dev_id,
                                          &(dev_tgt_curr.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Failed to get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id);
      return status;
    }
  }

  do {
    // Fetch the data fields and modify
    // Number of packets which were dropped because of buffer full condition
    uint64_t total_buffer_full_drop_packets =
        std::numeric_limits<uint64_t>::max();
    bool total_buffer_full_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "total_buffer_full_drop_packets",
        data,
        total_buffer_full_drop_packets_has_value,
        total_buffer_full_drop_packets);

    if (total_buffer_full_drop_packets_has_value &&
        total_buffer_full_drop_packets == 0) {
      status = trafficMgr->bfTmPipeBufferFullDropClear(dev_tgt_curr.dev_id,
                                                       dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear total_buffer_full_drop_packets",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Discard queue current usage and watermark in terms of number of cells
    uint64_t discard_queue_watermark_cells =
        std::numeric_limits<uint64_t>::max();
    bool discard_queue_watermark_cells_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "discard_queue_watermark_cells",
        data,
        discard_queue_watermark_cells_has_value,
        discard_queue_watermark_cells);

    if (discard_queue_watermark_cells_has_value &&
        discard_queue_watermark_cells == 0) {
      status = trafficMgr->bfTmQDiscardWatermarkClear(dev_tgt_curr.dev_id,
                                                      dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear discard_queue_watermark_cells",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Number of total cells received
    uint64_t total_cells = std::numeric_limits<uint64_t>::max();
    bool total_cells_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "total_cells", data, total_cells_has_value, total_cells);

    if (total_cells_has_value && total_cells == 0) {
      status = trafficMgr->bfTmPipeClearCellCounter(dev_tgt_curr.dev_id,
                                                    dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s (Pipe %d) Failed to clear total_cells",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Number of total packets received
    uint64_t total_packets = std::numeric_limits<uint64_t>::max();
    bool total_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "total_packets", data, total_packets_has_value, total_packets);

    if (total_packets_has_value && total_packets == 0) {
      status = trafficMgr->bfTmPipeClearPacketCounter(dev_tgt_curr.dev_id,
                                                      dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s (Pipe %d) Failed to clear total_packets",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Unicast cut through packet count
    uint64_t uc_ct_packets = std::numeric_limits<uint64_t>::max();
    bool uc_ct_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "uc_ct_packets", data, uc_ct_packets_has_value, uc_ct_packets);

    if (uc_ct_packets_has_value && uc_ct_packets == 0) {
      status = trafficMgr->bfTmPipeClearUcCtPacketCounter(dev_tgt_curr.dev_id,
                                                          dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s (Pipe %d) Failed to clear uc_ct_packets",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Multicast cut through packet count
    uint64_t mc_ct_packets = std::numeric_limits<uint64_t>::max();
    bool mc_ct_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "mc_ct_packets", data, mc_ct_packets_has_value, mc_ct_packets);

    if (mc_ct_packets_has_value && mc_ct_packets == 0) {
      status = trafficMgr->bfTmPipeClearMcCtPacketCounter(dev_tgt_curr.dev_id,
                                                          dev_tgt_curr.pipe_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s (Pipe %d) Failed to clear mc_ct_packets",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Block level counters
    uint32_t clear_mask = 0;
    uint64_t ig_no_dest_drop_packets = std::numeric_limits<uint64_t>::max();
    bool ig_no_dest_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("ig_no_dest_drop_packets",
                                          data,
                                          ig_no_dest_drop_packets_has_value,
                                          ig_no_dest_drop_packets);
    if (ig_no_dest_drop_packets_has_value && ig_no_dest_drop_packets == 0) {
      clear_mask |= WAC_NO_DEST_DROP;
    }

    uint64_t ig_buf_full_drop_packets = std::numeric_limits<uint64_t>::max();
    bool ig_buf_full_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("ig_buf_full_drop_packets",
                                          data,
                                          ig_buf_full_drop_packets_has_value,
                                          ig_buf_full_drop_packets);
    if (ig_buf_full_drop_packets_has_value && ig_buf_full_drop_packets == 0) {
      clear_mask |= WAC_BUF_FULL_DROP;
    }

    uint64_t eg_total_drop_packets = std::numeric_limits<uint64_t>::max();
    bool eg_total_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("eg_total_drop_packets",
                                          data,
                                          eg_total_drop_packets_has_value,
                                          eg_total_drop_packets);
    if (eg_total_drop_packets_has_value && eg_total_drop_packets == 0) {
      clear_mask |= EGRESS_PIPE_TOTAL_DROP;
    }

    uint64_t mc_queue_drop_packets = std::numeric_limits<uint64_t>::max();
    bool mc_queue_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("mc_queue_drop_packets",
                                          data,
                                          mc_queue_drop_packets_has_value,
                                          mc_queue_drop_packets);
    if (mc_queue_drop_packets == 0) {
      clear_mask |= QAC_PRE_MC_DROP;
    }

    if (clear_mask != 0) {
      status = trafficMgr->bfTmBlklvlDropClear(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, clear_mask);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear block counters (clear mask = "
            "%x)",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            clear_mask);
        return status;
      }
    }

    // PRE FIFO counters
    uint64_t mc_input_fifo_0_drop_packets =
        std::numeric_limits<uint64_t>::max();
    bool mc_input_fifo_0_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "mc_input_fifo_0_drop_packets",
        data,
        mc_input_fifo_0_drop_packets_has_value,
        mc_input_fifo_0_drop_packets);

    if (mc_input_fifo_0_drop_packets_has_value &&
        mc_input_fifo_0_drop_packets == 0) {
      status = trafficMgr->bfTmPreFifoDropClear(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, 0);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear mc_input_fifo_0_drop_packets",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    uint64_t mc_input_fifo_1_drop_packets =
        std::numeric_limits<uint64_t>::max();
    bool mc_input_fifo_1_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "mc_input_fifo_1_drop_packets",
        data,
        mc_input_fifo_1_drop_packets_has_value,
        mc_input_fifo_1_drop_packets);

    if (mc_input_fifo_1_drop_packets_has_value &&
        mc_input_fifo_1_drop_packets == 0) {
      status = trafficMgr->bfTmPreFifoDropClear(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, 1);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear mc_input_fifo_1_drop_packets",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    uint64_t mc_input_fifo_2_drop_packets =
        std::numeric_limits<uint64_t>::max();
    bool mc_input_fifo_2_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "mc_input_fifo_2_drop_packets",
        data,
        mc_input_fifo_2_drop_packets_has_value,
        mc_input_fifo_2_drop_packets);

    if (mc_input_fifo_2_drop_packets_has_value &&
        mc_input_fifo_2_drop_packets == 0) {
      status = trafficMgr->bfTmPreFifoDropClear(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, 2);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear mc_input_fifo_2_drop_packets",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    uint64_t mc_input_fifo_3_drop_packets =
        std::numeric_limits<uint64_t>::max();
    bool mc_input_fifo_3_drop_packets_has_value = false;
    BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE(
        "mc_input_fifo_3_drop_packets",
        data,
        mc_input_fifo_3_drop_packets_has_value,
        mc_input_fifo_3_drop_packets);

    if (mc_input_fifo_3_drop_packets_has_value &&
        mc_input_fifo_3_drop_packets == 0) {
      status = trafficMgr->bfTmPreFifoDropClear(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, 3);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s (Pipe %d) Failed to clear mc_input_fifo_3_drop_packets",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id);
        return status;
      }
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      // Single pipe
      break;
    }

    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more pipes left on this device
      status = BF_SUCCESS;
      break;
    }
  } while (BF_SUCCESS == status);

  return status;
}

bf_status_t BfRtTMCounterPipeTable::tableDefaultEntryReset(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;

  // Create a data object with the reset values
  BfRtTMTableData pipeCounterData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "total_buffer_full_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "discard_queue_watermark_cells", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("total_cells", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("total_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("uc_ct_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("mc_ct_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "ig_no_dest_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "ig_buf_full_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "eg_total_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_queue_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_input_fifo_0_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_input_fifo_1_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_input_fifo_2_drop_packets", (&pipeCounterData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "mc_input_fifo_3_drop_packets", (&pipeCounterData), 0);

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  return tableEntrySet_helper(dev_tgt, pipeCounterData);
}

bf_status_t BfRtTMCounterPipeTable::tableClear(const BfRtSession &session,
                                               const bf_rt_target_t &dev_tgt,
                                               const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

/*******  PPG Counter Interface Class Methods ********/
bf_status_t BfRtTMPpgCounterIntf::getCounters(const bf_rt_target_t &dev_tgt,
                                              const bf_dev_pipe_t &pipe,
                                              const bf_tm_ppg_hdl &ppg_hdl,
                                              BfRtTableData *data) const {
  BfRtTMTableData *ppgCounterData = static_cast<BfRtTMTableData *>(data);

  // drop_count_packets
  bf_status_t status = BF_SUCCESS;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint64_t drop_count = 0;
  status = trafficMgr->bfTmPpgDropGetCached(
      dev_tgt.dev_id, pipe, ppg_hdl, &drop_count);

  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", ppgCounterData, drop_count);

  // Rest of the data fields
  uint32_t guaranteed_min_usage_cells = 0;
  uint32_t shared_pool_usage_cells = 0;
  uint32_t skid_pool_usage_cells = 0;
  uint32_t watermark_cells = 0;
  status = trafficMgr->bfTmPpgUsageGet(dev_tgt.dev_id,
                                       pipe,
                                       ppg_hdl,
                                       &guaranteed_min_usage_cells,
                                       &shared_pool_usage_cells,
                                       &skid_pool_usage_cells,
                                       &watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get usage cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "guaranteed_min_usage_cells", ppgCounterData, guaranteed_min_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "shared_pool_usage_cells", ppgCounterData, shared_pool_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "watermark_cells", ppgCounterData, watermark_cells);

  return status;
}

bf_status_t BfRtTMPpgCounterIntf::setCounters(const bf_rt_target_t &dev_tgt,
                                              const bf_tm_ppg_hdl &ppg_hdl,
                                              const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Fetch the data fields
  const BfRtTMTableData &ppgCounterData =
      static_cast<const BfRtTMTableData &>(data);

  uint64_t drop_count_packets = std::numeric_limits<uint64_t>::max();
  bool drop_count_packets_has_value = false;
  BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("drop_count_packets",
                                        ppgCounterData,
                                        drop_count_packets_has_value,
                                        drop_count_packets);

  uint64_t watermark_cells = std::numeric_limits<uint64_t>::max();
  bool watermark_cells_has_value = false;
  BFRT_TM_COUNTER_DATA_GET_AND_VALIDATE("watermark_cells",
                                        ppgCounterData,
                                        watermark_cells_has_value,
                                        watermark_cells);

  // Call the set/clear APIs
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (drop_count_packets_has_value) {
    status = trafficMgr->bfTmPpgDropCacheSet(
        dev_tgt.dev_id, dev_tgt.pipe_id, ppg_hdl, drop_count_packets);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set drop_count_packets = %" PRIu64
                " for dev=%d, pipe=%d, "
                "ppg_hdl=%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                drop_count_packets,
                dev_tgt.dev_id,
                dev_tgt.pipe_id,
                ppg_hdl);
      return status;
    }
  }

  if (watermark_cells_has_value && watermark_cells == 0) {
    status = trafficMgr->bfTmPpgWatermarkClear(dev_tgt.dev_id, ppg_hdl);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Failed to clear watermark_cells for dev=%d, pipe=%d, "
          "ppg_hdl=%d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          ppg_hdl);
      return status;
    }
  }

  return status;
}

/*******  DPG Counter Table APIs ********/
bf_status_t BfRtTMCounterPortDpgTable::tableEntryGet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_dev_port_t &dev_port,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Check the validity of the dev port
  if (!DEV_PORT_VALIDATE(dev_port)) {
    LOG_ERROR("%s:%d %s Invalid key field dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_port);
    return BF_INVALID_ARG;
  }

  // Fetch the dpg handle for the dev port
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_handle = 0;
  auto status =
      trafficMgr->bfTMPPGDefaultPpgGet(dev_tgt.dev_id, dev_port, &dpg_handle);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get DPG handle for dev_id=%d dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_port);
    return status;
  }

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);
  return this->getCounters(
      dev_tgt, DEV_PORT_TO_PIPE(dev_port), dpg_handle, data);
}

bf_status_t BfRtTMCounterPortDpgTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Get the dev port id
  const BfRtTMDevPortKey &devPortKey =
      static_cast<const BfRtTMDevPortKey &>(key);
  bf_dev_port_t dev_port;
  auto status = devPortKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch the entry
  return this->tableEntryGet_helper(dev_tgt, dev_port, data);
}

bf_status_t BfRtTMCounterPortDpgTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  auto first_dev_port = 0;
  BfRtTMDevPortKey *devPortKey = static_cast<BfRtTMDevPortKey *>(key);
  devPortKey->setId(first_dev_port);

  // Fetch the entry
  return this->tableEntryGet_helper(dev_tgt, first_dev_port, data);
}

bf_status_t BfRtTMCounterPortDpgTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMDevPortKey &devPortKey =
      static_cast<const BfRtTMDevPortKey &>(key);
  bf_dev_port_t dev_port;
  auto status = devPortKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  bf_rt_target_t dev_tgt_curr = dev_tgt;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t i = 0;
  for (; i < n; i++) {
    dev_tgt_curr.pipe_id = DEV_PORT_TO_PIPE(dev_port);
    bf_dev_port_t dev_port_next = 0;

    status = trafficMgr->bfTMPipeGetPortNext(
        dev_tgt_curr.dev_id, dev_port, &dev_port_next);
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more dev ports on this pipe, try the next pipe
      status = trafficMgr->bfTMPipeGetNext(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
      if (BF_OBJECT_NOT_FOUND == status) {
        // No more dev ports left on the device
        status = BF_SUCCESS;
        break;
      }

      // Get the first port
      status = trafficMgr->bfTMPipeGetPortFirst(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &dev_port_next);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s Failed to get first port on dev_id=%d pipe_id=%d",
                  __func__,
                  __LINE__,
                  this->table_name_get().c_str(),
                  dev_tgt_curr.dev_id,
                  dev_tgt_curr.pipe_id);
        return status;
      }
    }

    // Set the key field
    auto this_key = static_cast<BfRtTMDevPortKey *>((*key_data_pairs)[i].first);
    this_key->setId(dev_port_next);

    // Set the data fields
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

    status = this->tableEntryGet_helper(dev_tgt_curr, dev_port_next, this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port_next);

      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    dev_port = dev_port_next;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMCounterPortDpgTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }

  uint16_t curr_count = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPipeGetPortCount(
      dev_tgt.dev_id, BF_DEV_PIPE_ALL, &curr_count);
  if (status) {
    LOG_ERROR("%s:%d %s Failed to get get number of ports on dev=%d pipe=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }
  *count = static_cast<uint32_t>(curr_count);

  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPortDpgTable::tableEntrySet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_dev_port_t &dev_port,
    const BfRtTableData &data) const {
  // Fetch the dpg handle for the dev port
  bf_tm_ppg_hdl dpg_handle = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status =
      trafficMgr->bfTMPPGDefaultPpgGet(dev_tgt.dev_id, dev_port, &dpg_handle);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get DPG handle for dev_id=%d dev_port=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_port);
    return status;
  }

  // Fetch the data fields and modify
  bf_rt_target_t dev_tgt_curr = dev_tgt;
  dev_tgt_curr.pipe_id = DEV_PORT_TO_PIPE(dev_port);
  return this->setCounters(dev_tgt_curr, dpg_handle, data);
}

bf_status_t BfRtTMCounterPortDpgTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  const BfRtTMDevPortKey &devPortKey =
      static_cast<const BfRtTMDevPortKey &>(key);

  // Fetch the dev port id from the key
  bf_dev_port_t dev_port;
  auto status = devPortKey.getId(dev_port);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for dev_port",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Modify the entry
  return this->tableEntrySet_helper(dev_tgt, dev_port, data);
}

bf_status_t BfRtTMCounterPortDpgTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  std::lock_guard<std::mutex> lock(this->entry_lock);

  bf_dev_port_t dev_port;
  bf_dev_pipe_t first_pipe;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPipeGetFirst(dev_tgt.dev_id, &first_pipe);
  if (BF_SUCCESS == status) {
    status =
        trafficMgr->bfTMPipeGetPortFirst(dev_tgt.dev_id, first_pipe, &dev_port);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Failed to get first port on dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  // Populate the data fields with 0
  BfRtTMTableData tmCounterPpgData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", (&tmCounterPpgData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", (&tmCounterPpgData), 0);

  bf_rt_target_t dev_tgt_curr = dev_tgt;
  do {
    // Clear the entry
    status = this->tableEntrySet_helper(dev_tgt, dev_port, tmCounterPpgData);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear DPG counters for dev_port %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_port);
      return status;
    }

    // Find the next dev port
    dev_tgt_curr.pipe_id = DEV_PORT_TO_PIPE(dev_port);
    bf_dev_port_t dev_port_next = 0;
    status = trafficMgr->bfTMPipeGetPortNext(
        dev_tgt_curr.dev_id, dev_port, &dev_port_next);
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more dev ports on this pipe, try the next pipe
      status = trafficMgr->bfTMPipeGetNext(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
      if (BF_OBJECT_NOT_FOUND == status) {
        // No more dev ports left on the device
        status = BF_SUCCESS;
        break;
      }
      status = trafficMgr->bfTMPipeGetPortFirst(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &dev_port_next);
    }
    dev_port = dev_port_next;
  } while (status == BF_SUCCESS);

  return status;
}

bf_status_t BfRtTMCounterPortDpgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMDevPortKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPortDpgTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtTMDevPortKey *match_key = static_cast<BfRtTMDevPortKey *>(key);
  return match_key->reset();
}

/*******  Mirror DPG Counter Table APIs ********/
bf_status_t BfRtTMCounterMirrorPortDpgTable::tableDefaultEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Validation - PIPE_ALL is not allowed
  auto status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch the dpg handle for the mirror port
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_handle = 0;
  status = trafficMgr->bfTMPPGMirrorPortHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &dpg_handle);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get mirror DPG handle for dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Fetch the fields and set it in data
  return this->getCounters(dev_tgt, dev_tgt.pipe_id, dpg_handle, data);
}

bf_status_t BfRtTMCounterMirrorPortDpgTable::defaultEntrySet_helper(
    const bf_rt_target_t &dev_tgt, const BfRtTableData &data) const {
  // Validation
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_tgt_curr = dev_tgt;
  if (dev_tgt_curr.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_tgt_curr.dev_id,
                                          &(dev_tgt_curr.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Failed to get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id);
      return status;
    }
  }

  // Clear the counters
  do {
    // Fetch the dpg handle for the mirror port
    bf_tm_ppg_hdl dpg_handle = 0;
    status = trafficMgr->bfTMPPGMirrorPortHandleGet(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &dpg_handle);
    if (status) {
      LOG_ERROR("%s:%d %s Can't get mirror DPG handle for dev_id=%d pipe_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id,
                dev_tgt_curr.pipe_id);
      return status;
    }

    // Acquire the entry level lock
    std::lock_guard<std::mutex> lock(this->entry_lock);

    // Fetch the data fields and modify
    status = this->setCounters(dev_tgt_curr, dpg_handle, data);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear mirror dpg counters for pipe_id=%d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_tgt_curr.pipe_id);
      return status;
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      // Single pipe
      break;
    }

    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more pipes left on this device
      status = BF_SUCCESS;
      break;
    }
  } while (BF_SUCCESS == status);
  return status;
}

bf_status_t BfRtTMCounterMirrorPortDpgTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  return this->defaultEntrySet_helper(dev_tgt, data);
}

bf_status_t BfRtTMCounterMirrorPortDpgTable::tableDefaultEntryReset(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;

  // Populate the data fields with 0
  BfRtTMTableData tmCounterPpgData(this);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", (&tmCounterPpgData), 0);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", (&tmCounterPpgData), 0);

  return this->defaultEntrySet_helper(dev_tgt, tmCounterPpgData);
}

bf_status_t BfRtTMCounterMirrorPortDpgTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

/*******  PPG Counter Table APIs ********/
bf_status_t BfRtTMCounterPpgTable::getCounters(const bf_rt_target_t &dev_tgt,
                                               const bf_dev_pipe_t &pipe,
                                               const bf_tm_ppg_hdl &ppg_hdl,
                                               BfRtTableData *data) const {
  BfRtTMTableData *ppgCounterData = static_cast<BfRtTMTableData *>(data);

  // drop_count_packets
  bf_status_t status = BF_SUCCESS;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint64_t drop_count = 0;
  status = trafficMgr->bfTmPpgDropGetCached(
      dev_tgt.dev_id, pipe, ppg_hdl, &drop_count);

  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "drop_count_packets", ppgCounterData, drop_count);

  // Rest of the data fields
  uint32_t guaranteed_min_usage_cells = 0;
  uint32_t shared_pool_usage_cells = 0;
  uint32_t skid_pool_usage_cells = 0;
  uint32_t watermark_cells = 0;
  status = trafficMgr->bfTmPpgUsageGet(dev_tgt.dev_id,
                                       pipe,
                                       ppg_hdl,
                                       &guaranteed_min_usage_cells,
                                       &shared_pool_usage_cells,
                                       &skid_pool_usage_cells,
                                       &watermark_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get usage cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "guaranteed_min_usage_cells", ppgCounterData, guaranteed_min_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "shared_pool_usage_cells", ppgCounterData, shared_pool_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "skid_pool_usage_cells", ppgCounterData, skid_pool_usage_cells);
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "watermark_cells", ppgCounterData, watermark_cells);

  return status;
}

bf_status_t BfRtTMCounterPpgTable::getPpgHdl_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_tm_ppg_id_t &ppg_counter_id,
    bf_tm_ppg_id_t &ppg_id,
    bf_tm_ppg_hdl &ppg_hdl) const {
  // Check if there is a ppg associated with this counter
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_tgt.dev_id, this->programNameGet());
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d Failed to get device state for dev_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id);
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  // ppg counter id to ppg id
  auto tmPpgState = device_state->tmPpgState.getStateObj();
  auto status =
      tmPpgState->stateTMCntPpgGet(dev_tgt.pipe_id, ppg_counter_id, &ppg_id);
  if (BF_SUCCESS != status) {
    return status;
  }

  // A ppg was associated with the counter. Now find out the ppg handle for it.
  status = tmPpgState->stateTMPpgGet(dev_tgt.pipe_id, ppg_id, &ppg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s Unknown ppg_hdl for ppg_id=0x%x on dev_id=%d pipe_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ppg_id,
        dev_tgt.dev_id,
        dev_tgt.pipe_id);
    return status;
  }
  return status;
}

bf_status_t BfRtTMCounterPpgTable::tableEntryGet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_tm_ppg_id_t &ppg_counter_id,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Validation - PIPE_ALL is not allowed
  auto status = this->singlePipe_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Check if there is a ppg associated with this counter
  bf_tm_ppg_id_t ppg_id = 0;
  bf_tm_ppg_hdl ppg_hdl = 0;
  status = this->getPpgHdl_helper(dev_tgt, ppg_counter_id, ppg_id, ppg_hdl);
  if (BF_OBJECT_NOT_FOUND == status) {
    // No ppg was associated with this counter
    // Returned a zero'ed out entry
    BfRtTMTableData *ppgCtrData = static_cast<BfRtTMTableData *>(data);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("ppg_id", ppgCtrData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("drop_count_packets", ppgCtrData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
        "guaranteed_min_usage_cells", ppgCtrData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
        "shared_pool_usage_cells", ppgCtrData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("skid_pool_usage_cells", ppgCtrData, 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT("watermark_cells", ppgCtrData, 0);
    return BF_SUCCESS;
  }

  // A ppg was associated with the counter
  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Populate the ppg id field
  BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
      "ppg_id", (static_cast<BfRtTMTableData *>(data)), ppg_id);

  // Populate the rest of the fields
  return this->getCounters(dev_tgt, dev_tgt.pipe_id, ppg_hdl, data);
}

bf_status_t BfRtTMCounterPpgTable::tableEntrySet_helper(
    const bf_rt_target_t &dev_tgt,
    const bf_tm_ppg_id_t &ppg_counter_id,
    const BfRtTableData &data) const {
  // Validation
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Invalid device target",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_tgt_curr = dev_tgt;
  if (dev_tgt_curr.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_tgt_curr.dev_id,
                                          &(dev_tgt_curr.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Failed to get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id);
      return status;
    }
  }

  // Clear the counters
  bf_status_t pipe_status = BF_SUCCESS;
  do {
    // Check if there is a ppg associated with this counter
    bf_tm_ppg_id_t ppg_id = 0;
    bf_tm_ppg_hdl ppg_hdl = 0;
    status =
        this->getPpgHdl_helper(dev_tgt_curr, ppg_counter_id, ppg_id, ppg_hdl);
    if (BF_SUCCESS == status) {
      // Fetch the data fields and modify
      status = this->setCounters(dev_tgt_curr, ppg_hdl, data);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Failed to clear PPG counters for pipe=%d, "
            "ppg_counter_id=%d",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            ppg_counter_id);
        return status;
      }
    } else {
      // Continue on other pipes
      pipe_status = status;
      status = BF_SUCCESS;
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      // Single pipe
      status = pipe_status;
      break;
    }

    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      // No more pipes left on this device
      status = pipe_status;
      break;
    }
  } while (BF_SUCCESS == status);
  return status;
}

bf_status_t BfRtTMCounterPpgTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  if (data == nullptr) {
    return BF_INVALID_ARG;
  }

  // Get the ppg counter id
  const BfRtTMPpgCounterTableKey &ppgCtrKey =
      static_cast<const BfRtTMPpgCounterTableKey &>(key);
  bf_tm_ppg_id_t ppg_counter_id;
  auto status = ppgCtrKey.getId(ppg_counter_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for ppg_counter_id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Fetch the entry
  return this->tableEntryGet_helper(dev_tgt, ppg_counter_id, data);
}

bf_status_t BfRtTMCounterPpgTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  if (key == nullptr || data == nullptr) {
    return BF_INVALID_ARG;
  }

  auto first_ppg_counter_id = 0;
  BfRtTMPpgCounterTableKey *ppgCtrKey =
      static_cast<BfRtTMPpgCounterTableKey *>(key);
  ppgCtrKey->setId(first_ppg_counter_id);

  // Fetch the entry
  return this->tableEntryGet_helper(dev_tgt, first_ppg_counter_id, data);
}

bf_status_t BfRtTMCounterPpgTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMPpgCounterTableKey &ppgCtrKey =
      static_cast<const BfRtTMPpgCounterTableKey &>(key);
  bf_tm_ppg_id_t ppg_cnt_start_id;
  auto status = ppgCtrKey.getId(ppg_cnt_start_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for ppg_counter_id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the total PPG count
  uint32_t num_ppgs = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  status = trafficMgr->bfTMPPGTotalCntGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &num_ppgs);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error fetching total PPG count", __func__, __LINE__);
    return status;
  }

  uint32_t i = 0;
  for (uint32_t ppg_cnt_id = ppg_cnt_start_id + 1; ppg_cnt_id < num_ppgs;
       ppg_cnt_id++) {
    if (i >= n) {
      break;
    }

    // Set the key field
    auto this_key =
        static_cast<BfRtTMPpgCounterTableKey *>((*key_data_pairs)[i].first);
    this_key->setId(ppg_cnt_id);

    // Set the data fields
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

    status = this->tableEntryGet_helper(dev_tgt, ppg_cnt_id, this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of ppg_counter_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                ppg_cnt_id);

      // Make the data object null if error
      (*key_data_pairs)[i].second = nullptr;
    }
    i++;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMCounterPpgTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  // Get the ppg counter id
  const BfRtTMPpgCounterTableKey &ppgCtrKey =
      static_cast<const BfRtTMPpgCounterTableKey &>(key);
  bf_tm_ppg_id_t ppg_counter_id;
  auto status = ppgCtrKey.getId(ppg_counter_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for ppg_counter_id",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Modify the entry
  status = this->tableEntrySet_helper(dev_tgt, ppg_counter_id, data);
  if (BF_OBJECT_NOT_FOUND == status) {
    // No ppg was associated with this counter
    LOG_ERROR(
        "%s:%d %s No PPG was associated with ppg_counter_id=%d on dev=%d "
        "pipe=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        ppg_counter_id,
        dev_tgt.dev_id,
        dev_tgt.pipe_id);
  }
  return status;
}

bf_status_t BfRtTMCounterPpgTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  // Get the total PPG count
  uint32_t num_ppgs = 0;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto status = trafficMgr->bfTMPPGTotalCntGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &num_ppgs);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error fetching total PPG count", __func__, __LINE__);
    return status;
  }

  // Clear the counters
  std::lock_guard<std::mutex> lock(this->entry_lock);
  for (uint32_t ppg_id = 0; ppg_id < num_ppgs; ppg_id++) {
    // Populate the data fields with 0
    BfRtTMTableData tmCounterPpgData(this);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
        "drop_count_packets", (&tmCounterPpgData), 0);
    BFRT_TM_SET_COUNTER_DATA_FIELD_INT(
        "watermark_cells", (&tmCounterPpgData), 0);

    // Modify the entry
    status = this->tableEntrySet_helper(dev_tgt, ppg_id, tmCounterPpgData);
    if (status == BF_OBJECT_NOT_FOUND) {
      // If a PPG was not associated with this counter, continue without errors
      status = BF_SUCCESS;
      continue;
    }

    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to clear PPG counters for ppg_counter_id %d",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                ppg_id);
      return status;
    }
  }
  return status;
}

bf_status_t BfRtTMCounterPpgTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  if (count == nullptr) {
    return BF_INVALID_ARG;
  }
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPpgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPpgCounterTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMCounterPpgTable::keyReset(BfRtTableKey *key) const {
  if (key == nullptr) {
    LOG_TRACE("%s:%d %s No key to reset",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  BfRtTMPpgCounterTableKey *match_key =
      static_cast<BfRtTMPpgCounterTableKey *>(key);
  return match_key->reset();
}
}  // namespace bfrt
