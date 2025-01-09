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


#include "bf_rt_tm_table_impl_pool.hpp"

namespace bfrt {

/******* Pool config table APIs ********/
bf_status_t BfRtTMPoolCfgTable::specialPoolSizeSet_internal(
    bf_dev_id_t dev, const std::string &pool, uint32_t cells) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (pool == "IG_SKID_POOL") {
    return trafficMgr->bfTmPoolSkidSizeSet(dev, cells);
  } else if (pool == "IG_EG_NEGATIVE_MIRROR_POOL") {
    return trafficMgr->bfTmPoolMirrorOnDropSizeSet(dev, cells);
  } else if (pool == "EG_PRE_FIFO") {
    bf_dev_pipe_t pipe_id_curr = BF_DEV_PIPE_ALL;
    auto status = trafficMgr->bfTMPipeGetFirst(dev, &pipe_id_curr);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev);
      return status;
    }
    do {
      for (uint8_t fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
        status =
            trafficMgr->bfTmPreFifoLimitSet(dev, pipe_id_curr, fifo, cells);
        if (status != BF_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s Failed to set pre FIFO limit for dev = %u, pipe = %u, "
              "fifo = %u, cells = %u",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              dev,
              pipe_id_curr,
              fifo,
              cells);
          return status;
        }
      }
      status = trafficMgr->bfTMPipeGetNext(dev, pipe_id_curr, &pipe_id_curr);
      if (BF_OBJECT_NOT_FOUND == status) {
        status = BF_SUCCESS;
        break;
      }
    } while (BF_SUCCESS == status);
    return status;
  } else if (pool == "IG_EG_GUARANTEED_MIN") {
    return trafficMgr->bfTmGlobalMinLimitSet(dev, cells);
  } else if (pool == "EG_UNICAST_CUT_THROUGH") {
    return trafficMgr->bfTmPoolUcCutThroughSizeSet(dev, cells);
  } else if (pool == "EG_MULTICAST_CUT_THROUGH") {
    return trafficMgr->bfTmPoolMcCutThroughSizeSet(dev, cells);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtTMPoolCfgTable::specialPoolSizeGet_internal(
    bf_dev_id_t dev, const std::string &pool, uint32_t *cells) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (pool == "IG_SKID_POOL") {
    return trafficMgr->bfTmPoolSkidSizeGet(dev, cells);
  } else if (pool == "IG_EG_NEGATIVE_MIRROR_POOL") {
    return trafficMgr->bfTmPoolMirrorOnDropSizeGet(dev, cells);
  } else if (pool == "EG_PRE_FIFO") {
    bf_dev_pipe_t pipe_id_curr = BF_DEV_PIPE_ALL;
    uint8_t fifo_id = 0;
    auto status = trafficMgr->bfTMPipeGetFirst(dev, &pipe_id_curr);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev);
      return status;
    }
    status = trafficMgr->bfTmPreFifoLimitGet(dev, pipe_id_curr, fifo_id, cells);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s Failed to set pre FIFO limit for dev = %u, pipe = %u, "
          "fifo = %d",
          __func__,
          __LINE__,
          table_name_get().c_str(),
          dev,
          pipe_id_curr,
          fifo_id);
    }
    return status;
  } else if (pool == "IG_EG_GUARANTEED_MIN") {
    return trafficMgr->bfTmGlobalMinLimitGet(dev, cells);
  } else if (pool == "EG_UNICAST_CUT_THROUGH") {
    return trafficMgr->bfTmPoolUcCutThroughSizeGet(dev, cells);
  } else if (pool == "EG_MULTICAST_CUT_THROUGH") {
    return trafficMgr->bfTmPoolMcCutThroughSizeGet(dev, cells);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtTMPoolCfgTable::tableEntryMod(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Get the pool from the key object
  const BfRtTMPoolTableKey &poolConfKey =
      static_cast<const BfRtTMPoolTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  // Get the data fields from the data object
  const BfRtTMTableData &poolConfData =
      static_cast<const BfRtTMTableData &>(data);

  bf_rt_id_t data_id = 0;
  status = this->dataFieldIdGet("size_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting fieldId for size_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  uint64_t size_cells;
  status = poolConfData.getValue(data_id, &size_cells);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }

  // Check the validity of the pool string and call the TM API
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto pool_id_itr = appPoolMap.find(pool);
  if (pool_id_itr != appPoolMap.end()) {
    // Application pools
    status = trafficMgr->bfTmAppPoolSizeSet(
        dev_tgt.dev_id, pool_id_itr->second, static_cast<uint32_t>(size_cells));
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set pool size for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
    }
  } else if (std::find(specialPools.begin(), specialPools.end(), pool) !=
             specialPools.end()) {
    status = this->specialPoolSizeSet_internal(
        dev_tgt.dev_id, pool, static_cast<uint32_t>(size_cells));
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set pool size for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
    }
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  return status;
}

bf_status_t BfRtTMPoolCfgTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMTableData *poolConfData = static_cast<BfRtTMTableData *>(data);

  // Check the validity of the pool string and call the TM API
  std::vector<std::string> allPools;
  status = this->getKeyStringChoices("pool", allPools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (find(allPools.begin(), allPools.end(), pool) == allPools.end()) {
    LOG_ERROR(
        "%s:%d %s Key value for field pool not valid, please check the json "
        "for the list of valid values.",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t size_cells = 0;
  auto pool_id_itr = appPoolMap.find(pool);
  if (pool_id_itr != appPoolMap.end()) {
    // Application pools
    status = trafficMgr->bfTmAppPoolSizeGet(
        dev_tgt.dev_id, pool_id_itr->second, &size_cells);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get pool size for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }
  } else if (std::find(specialPools.begin(), specialPools.end(), pool) !=
             specialPools.end()) {
    status =
        this->specialPoolSizeGet_internal(dev_tgt.dev_id, pool, &size_cells);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get pool size for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  bf_rt_id_t data_id;
  status = this->dataFieldIdGet("size_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for size_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfData->setValue(data_id, static_cast<uint64_t>(size_cells));
  if (status) {
    LOG_TRACE("%s:%d %s Error in Setting data object for size_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return status;
}

bf_status_t BfRtTMPoolCfgTable::tableEntryGet(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolTableKey &poolConfKey =
      static_cast<const BfRtTMPoolTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }
  return tableEntryGet_internal(dev_tgt, pool, data);
}

bf_status_t BfRtTMPoolCfgTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMPoolTableKey *poolConfKey = static_cast<BfRtTMPoolTableKey *>(key);
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey->setValue(key_id, "IG_APP_POOL_0");
  if (status) {
    LOG_TRACE("%s:%d %s Error in setting key object for pool field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return tableEntryGet_internal(dev_tgt, "IG_APP_POOL_0", data);
}

bf_status_t BfRtTMPoolCfgTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolTableKey &poolConfKey =
      static_cast<const BfRtTMPoolTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  // Find all possible pool choices
  std::vector<std::string> allPools;
  status = this->getKeyStringChoices("pool", allPools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find index of the key in allPools vector
  uint32_t index;
  auto it = find(allPools.begin(), allPools.end(), pool);
  if (it != allPools.end()) {
    index = (it - allPools.begin()) + 1;
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint32_t i;
  for (i = 0; i < n; i++) {
    auto this_key =
        static_cast<BfRtTMPoolTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);
    if (index >= allPools.size()) {
      break;
    }

    status = this_key->setValue(key_id, allPools[index]);
    if (status) {
      LOG_ERROR("%s:%d %s Error in setting key object for pool",
                __func__,
                __LINE__,
                table_name_get().c_str());
      break;
    }

    status = tableEntryGet_internal(dev_tgt, allPools[index], this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                allPools[index].c_str());
      break;
    }
    index++;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMPoolCfgTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolCfgTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPoolTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolCfgTable::keyReset(BfRtTableKey *key) const {
  BfRtTMPoolTableKey *match_key = static_cast<BfRtTMPoolTableKey *>(key);
  return match_key->reset();
}

/******* Skid pool table APIs ********/
bf_status_t BfRtTMPoolSkidTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Get the data fields from the data object
  const BfRtTMTableData &skidPoolData =
      static_cast<const BfRtTMTableData &>(data);

  bf_rt_id_t data_id = 0;
  status = this->dataFieldIdGet("resume_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in getting fieldId for resume_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  uint64_t resume_limit_cells;
  status = skidPoolData.getValue(data_id, &resume_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }

  // Call the TM API
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  status = trafficMgr->bfTmSkidPoolHysteresisSet(
      dev_tgt.dev_id, static_cast<uint32_t>(resume_limit_cells));
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set skid pool resume limit!",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
  return status;
}

bf_status_t BfRtTMPoolSkidTable::tableDefaultEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;

  BfRtTMTableData *skidPoolData = static_cast<BfRtTMTableData *>(data);

  // Call the TM API
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t resume_limit_cells = 0;
  status = trafficMgr->bfTmSkidPoolHysteresisGet(dev_tgt.dev_id,
                                                 &resume_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get resume limit for skid pool!",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  bf_rt_id_t data_id;
  status = this->dataFieldIdGet("resume_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in finding fieldId for resume_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = skidPoolData->setValue(data_id,
                                  static_cast<uint64_t>(resume_limit_cells));
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Error in Setting data object for resume_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return status;
}

bf_status_t BfRtTMPoolSkidTable::tableDefaultEntryReset(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;
  auto *trafficMgr = TrafficMgrIntf::getInstance();

  uint32_t def_resume_limit_cells = 0;
  status = trafficMgr->bfTmSkidPoolHysteresisGetDefault(
      dev_tgt.dev_id, &def_resume_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get default resume limit for skid pool!",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  status = trafficMgr->bfTmSkidPoolHysteresisSet(dev_tgt.dev_id,
                                                 def_resume_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to reset skid pool resume limit!",
              __func__,
              __LINE__,
              table_name_get().c_str());
  }
  return status;
}

bf_status_t BfRtTMPoolSkidTable::tableClear(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

/******* App pool table APIs ********/
bf_status_t BfRtTMPoolAppTable::tableEntryMod(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Get the pool from the key object
  const BfRtTMPoolAppTableKey &poolConfKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

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

  // Get the data fields from the data object
  const BfRtTMTableData &poolConfData =
      static_cast<const BfRtTMTableData &>(data);

  // Get the action id from data
  bf_rt_id_t action_id;
  status = poolConfData.actionIdGet(&action_id);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Can't get action id from the data object",
              __func__,
              __LINE__,
              this->table_name_get().c_str());
    return status;
  }

  // Check and proceed only if action id is not 0
  if (action_id == 0) {
    LOG_TRACE("%s:%d %s : Error : Action id is invalid (0) ",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Get the action name from action id
  std::string action_name;
  status = this->actionNameGet(action_id, &action_name);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s : Error : Failed to get action name for action id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              action_id);

    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  if (action_name == "color_drop_disable") {
    // Disable color drop
    status = trafficMgr->bfTmPoolColorDropDisable(dev_tgt.dev_id, app_pool);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to disable color drop for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
    }
  } else if (action_name == "color_drop_enable") {
    // Enable color drop
    status = trafficMgr->bfTmPoolColorDropEnable(dev_tgt.dev_id, app_pool);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to enable color drop for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
    }

    // Set the color drop limits
    // Green
    bf_rt_id_t field_id = 0;
    bool field_has_value = false;
    status = this->dataFieldIdGet("green_limit_cells", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting fieldId for green_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    field_has_value = poolConfData.hasValue(field_id);
    if (field_has_value) {
      uint64_t green_limit_cells = 0;
      status = poolConfData.getValue(field_id, &green_limit_cells);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Error in getting value for green_limit_cells",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      } else {
        status = trafficMgr->bfTmAppPoolColorDropLimitSet(
            dev_tgt.dev_id,
            app_pool,
            BF_TM_COLOR_GREEN,
            static_cast<uint32_t>(green_limit_cells));
        if (status != BF_SUCCESS) {
          LOG_ERROR("%s:%d %s Failed to set green color drop limit for pool %s",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    pool.c_str());
          return status;
        }
      }
    }

    // Yellow
    status = this->dataFieldIdGet("yellow_limit_cells", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting fieldId for yellow_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    field_has_value = poolConfData.hasValue(field_id);
    if (field_has_value) {
      uint64_t yellow_limit_cells = 0;
      status = poolConfData.getValue(field_id, &yellow_limit_cells);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Error in getting value for yellow_limit_cells",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      } else {
        status = trafficMgr->bfTmAppPoolColorDropLimitSet(
            dev_tgt.dev_id,
            app_pool,
            BF_TM_COLOR_YELLOW,
            static_cast<uint32_t>(yellow_limit_cells));
        if (status != BF_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s Failed to set yellow color drop limit for pool %s",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pool.c_str());
          return status;
        }
      }
    }

    // Red
    status = this->dataFieldIdGet("red_limit_cells", action_id, &field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Error in getting fieldId for red_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    field_has_value = poolConfData.hasValue(field_id);
    if (field_has_value) {
      uint64_t red_limit_cells = 0;
      status = poolConfData.getValue(field_id, &red_limit_cells);
      if (status != BF_SUCCESS) {
        LOG_TRACE("%s:%d %s Error in getting value for red_limit_cells",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      } else {
        status = trafficMgr->bfTmAppPoolColorDropLimitSet(
            dev_tgt.dev_id,
            app_pool,
            BF_TM_COLOR_RED,
            static_cast<uint32_t>(red_limit_cells));
        if (status != BF_SUCCESS) {
          LOG_ERROR("%s:%d %s Failed to set red color drop limit for pool %s",
                    __func__,
                    __LINE__,
                    table_name_get().c_str(),
                    pool.c_str());
          return status;
        }
      }
    }
  }
  return status;
}

bf_status_t BfRtTMPoolAppTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMTableData *poolConfData = static_cast<BfRtTMTableData *>(data);

  // Figure out the app pool id
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

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bool drop_state;
  trafficMgr->bfTmPoolColorDropStateGet(dev_tgt.dev_id, app_pool, &drop_state);

  bf_rt_id_t action_id;
  if (drop_state) {
    status = actionIdGet("color_drop_enable", &action_id);
    if (status) {
      LOG_ERROR("%s:%d %s No 'color_drop_enable' action to use for pool %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                pool.c_str());
      return status;
    }

    poolConfData->actionIdSet(action_id);

    std::vector<bf_rt_id_t> activeFields;

    // Green
    bf_rt_id_t green_field_id;
    uint32_t green_drop_limit;
    status = trafficMgr->bfTmPoolColorDropLimitGet(
        dev_tgt.dev_id, app_pool, BF_TM_COLOR_GREEN, &green_drop_limit);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get green color drop limit for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }

    status =
        this->dataFieldIdGet("green_limit_cells", action_id, &green_field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for green_limit_cells "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return status;
    }
    activeFields.push_back(green_field_id);

    // Yellow
    bf_rt_id_t yellow_field_id;
    uint32_t yellow_drop_limit;
    status = trafficMgr->bfTmPoolColorDropLimitGet(
        dev_tgt.dev_id, app_pool, BF_TM_COLOR_YELLOW, &yellow_drop_limit);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get yellow color drop limit for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }

    status =
        this->dataFieldIdGet("yellow_limit_cells", action_id, &yellow_field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for "
          "yellow_limit_cells "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return status;
    }
    activeFields.push_back(yellow_field_id);

    // Red
    bf_rt_id_t red_field_id;
    uint32_t red_drop_limit;
    status = trafficMgr->bfTmPoolColorDropLimitGet(
        dev_tgt.dev_id, app_pool, BF_TM_COLOR_RED, &red_drop_limit);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get red color drop limit for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                pool.c_str());
      return status;
    }

    status = this->dataFieldIdGet("red_limit_cells", action_id, &red_field_id);
    if (status != BF_SUCCESS) {
      LOG_TRACE(
          "%s:%d %s : Error : Failed to get the field id for red_limit_cells "
          "field",
          __func__,
          __LINE__,
          table_name_get().c_str());
      return status;
    }
    activeFields.push_back(red_field_id);

    // Set active fields
    poolConfData->setActiveFields(activeFields);

    // Set the data fields
    status = poolConfData->setValue(green_field_id,
                                    static_cast<uint64_t>(green_drop_limit));
    if (status) {
      LOG_TRACE("%s:%d %s Error in Setting data object for green_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    status = poolConfData->setValue(yellow_field_id,
                                    static_cast<uint64_t>(yellow_drop_limit));
    if (status) {
      LOG_TRACE("%s:%d %s Error in Setting data object for yellow_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    status = poolConfData->setValue(red_field_id,
                                    static_cast<uint64_t>(red_drop_limit));
    if (status) {
      LOG_TRACE("%s:%d %s Error in Setting data object for red_limit_cells",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }
  } else {
    status = actionIdGet("color_drop_disable", &action_id);
    if (status) {
      LOG_ERROR("%s:%d %s No 'color_drop_disable' action to use for pool %s",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                pool.c_str());
      return status;
    }

    poolConfData->actionIdSet(action_id);
  }

  return status;
}

bf_status_t BfRtTMPoolAppTable::tableEntryGet(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              const BfRtTableKey &key,
                                              BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolAppTableKey &poolConfKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }
  return tableEntryGet_internal(dev_tgt, pool, data);
}

bf_status_t BfRtTMPoolAppTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMPoolAppTableKey *poolConfKey =
      static_cast<BfRtTMPoolAppTableKey *>(key);
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey->setValue(key_id, "IG_APP_POOL_0");
  if (status) {
    LOG_TRACE("%s:%d %s Error in setting key object for pool field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return tableEntryGet_internal(dev_tgt, "IG_APP_POOL_0", data);
}

bf_status_t BfRtTMPoolAppTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolAppTableKey &poolConfKey =
      static_cast<const BfRtTMPoolAppTableKey &>(key);
  std::string pool;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &pool);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  // Find all possible app pool choices
  std::vector<std::string> appPools;
  status = this->getKeyStringChoices("pool", appPools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get app pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find index of the key in appPools vector
  uint32_t index;
  auto it = find(appPools.begin(), appPools.end(), pool);
  if (it != appPools.end()) {
    index = (it - appPools.begin()) + 1;
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint32_t i;
  for (i = 0; i < n; i++) {
    if (index >= appPools.size()) {
      break;
    }

    auto this_key =
        static_cast<BfRtTMPoolTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);
    status = this_key->setValue(key_id, appPools[index]);
    if (status) {
      LOG_ERROR("%s:%d %s Error in setting key object for pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                appPools[index].c_str());
      break;
    }

    status = tableEntryGet_internal(dev_tgt, appPools[index], this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of pool %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                appPools[index].c_str());
      break;
    }
    index++;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMPoolAppTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolAppTable::keyAllocate(
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

bf_status_t BfRtTMPoolAppTable::keyReset(BfRtTableKey *key) const {
  BfRtTMPoolAppTableKey *match_key = static_cast<BfRtTMPoolAppTableKey *>(key);
  return match_key->reset();
}

/******* Pool color table APIs ********/
bf_status_t BfRtTMPoolColorTable::tableEntryMod_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &color,
    const uint32_t &color_drop_resume_limit) const {
  bf_status_t status = BF_SUCCESS;

  // Check the validity of the color string and call the TM API
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  auto color_id_itr = colorMap.find(color);
  if (color_id_itr != colorMap.end()) {
    status = trafficMgr->bfTmPoolColorDropHysteresisSet(
        dev_tgt.dev_id,
        color_id_itr->second,
        static_cast<uint32_t>(color_drop_resume_limit));
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set resume limit cells for color %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                color.c_str());
    }
  } else {
    LOG_TRACE("%s:%d %s Key value for field color not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }
  return status;
}

bf_status_t BfRtTMPoolColorTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  bf_status_t status = BF_SUCCESS;

  // Get the color from the key object
  const BfRtTMPoolColorTableKey &poolColorKey =
      static_cast<const BfRtTMPoolColorTableKey &>(key);
  std::string color;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("color", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolColorKey.getValue(key_id, &color);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  // Get the data fields from the data object
  const BfRtTMTableData &poolColorData =
      static_cast<const BfRtTMTableData &>(data);

  bf_rt_id_t data_id = 0;
  status = this->dataFieldIdGet("color_drop_resume_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in getting fieldId for color_drop_resume_limit_cells",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }

  uint64_t resume_limit_cells;
  status = poolColorData.getValue(data_id, &resume_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting data value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }
  return this->tableEntryMod_internal(dev_tgt, color, resume_limit_cells);
}

bf_status_t BfRtTMPoolColorTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &color,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMTableData *poolColorData = static_cast<BfRtTMTableData *>(data);

  // Check the validity of the color string and call the TM API
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t resume_limit_cells = 0;
  auto color_id_itr = colorMap.find(color);
  if (color_id_itr != colorMap.end()) {
    status = trafficMgr->bfTmPoolColorDropHysteresisGet(
        dev_tgt.dev_id, color_id_itr->second, &resume_limit_cells);
    if (status != BF_SUCCESS) {
      LOG_TRACE("%s:%d %s Failed to get resume limit cells for color %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                color.c_str());
      return status;
    }
  } else {
    LOG_TRACE("%s:%d %s Key value for field color not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  bf_rt_id_t data_id;
  status = this->dataFieldIdGet("color_drop_resume_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Error in finding fieldId for color_drop_resume_limit_cells",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  status = poolColorData->setValue(data_id,
                                   static_cast<uint64_t>(resume_limit_cells));
  if (status) {
    LOG_TRACE(
        "%s:%d %s Error in Setting data object for "
        "color_drop_resume_limit_cells",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  return status;
}

bf_status_t BfRtTMPoolColorTable::tableEntryGet(const BfRtSession & /*session*/,
                                                const bf_rt_target_t &dev_tgt,
                                                const uint64_t & /*flags*/,
                                                const BfRtTableKey &key,
                                                BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolColorTableKey &poolColorKey =
      static_cast<const BfRtTMPoolColorTableKey &>(key);
  std::string color;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("color", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolColorKey.getValue(key_id, &color);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }
  return this->tableEntryGet_internal(dev_tgt, color, data);
}

bf_status_t BfRtTMPoolColorTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMPoolColorTableKey *poolColorKey =
      static_cast<BfRtTMPoolColorTableKey *>(key);
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("color", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the first entry key name
  std::vector<std::string> color_choices;
  status = this->getKeyStringChoices("color", color_choices);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get color choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  std::string key_value = color_choices.front();
  status = poolColorKey->setValue(key_id, key_value);
  if (status) {
    LOG_TRACE("%s:%d %s Error in setting key object for color field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return tableEntryGet_internal(dev_tgt, key_value, data);
}

bf_status_t BfRtTMPoolColorTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  bf_status_t status = BF_SUCCESS;
  const BfRtTMPoolColorTableKey &poolConfKey =
      static_cast<const BfRtTMPoolColorTableKey &>(key);
  std::string color;
  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("color", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = poolConfKey.getValue(key_id, &color);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              key_id);
    return status;
  }

  // Get the choices for the color field
  std::vector<std::string> color_strs;
  status = this->getKeyStringChoices("color", color_strs);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get color choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find index of the color in color_strs vector
  uint32_t index;
  auto it = find(color_strs.begin(), color_strs.end(), color);
  if (it != color_strs.end()) {
    index = (it - color_strs.begin()) + 1;
  } else {
    LOG_TRACE("%s:%d %s Key value for field color is not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint32_t i;
  for (i = 0; i < n; i++) {
    if (index >= color_strs.size()) {
      break;
    }

    auto this_key =
        static_cast<BfRtTMPoolColorTableKey *>((*key_data_pairs)[i].first);
    auto this_data =
        static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

    status = this_key->setValue(key_id, color_strs[index]);
    if (status) {
      LOG_ERROR("%s:%d %s Error in setting key object for color",
                __func__,
                __LINE__,
                table_name_get().c_str());
      break;
    }

    status = tableEntryGet_internal(dev_tgt, color_strs[index], this_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s: Error in getting data of color %s",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                color_strs[index].c_str());
      break;
    }
    index++;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMPoolColorTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolColorTable::tableClear(const BfRtSession & /*session*/,
                                             const bf_rt_target_t &dev_tgt,
                                             const uint64_t & /*flags*/) const {
  bf_status_t status = BF_SUCCESS;

  // Get the possible color choices
  std::vector<std::string> color_choices;
  status = this->getKeyStringChoices("color", color_choices);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get color choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Iterate over and clear entries
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  for (const auto &color : color_choices) {
    auto color_id_itr = colorMap.find(color);
    uint32_t def_resume_limit_cells = 0;
    if (color_id_itr != colorMap.end()) {
      status = trafficMgr->bfTmPoolColorDropHysteresisDefaultGet(
          dev_tgt.dev_id, color_id_itr->second, &def_resume_limit_cells);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Failed to get default resume limit cells for color %s",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            (color_id_itr->first).c_str());
        return status;
      }
      this->tableEntryMod_internal(dev_tgt, color, def_resume_limit_cells);
    }
  }
  return status;
}

bf_status_t BfRtTMPoolColorTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMPoolColorTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolColorTable::keyReset(BfRtTableKey *key) const {
  BfRtTMPoolColorTableKey *match_key =
      static_cast<BfRtTMPoolColorTableKey *>(key);
  return match_key->reset();
}

/******* App Pool PFC table APIs ********/
bf_status_t BfRtTMPoolAppPfcTable::tableEntryMod_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    const uint8_t &cos,
    const uint32_t &pfc_limit_cells) const {
  // Check the validity of the key fields and then call the TM API
  auto pool_id_itr = igAppPoolMap.find(pool);
  if (pool_id_itr == igAppPoolMap.end()) {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t max_pfc_levels = 0;
  auto status =
      trafficMgr->bfTmMaxPfcLevelsGet(dev_tgt.dev_id, &max_pfc_levels);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get max PFC levels",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (cos >= max_pfc_levels) {
    LOG_TRACE("%s:%d %s Key value for field cos is not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  status = trafficMgr->bfTmPoolPfcLimitSet(
      dev_tgt.dev_id, pool_id_itr->second, cos, pfc_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set pfc_limit_cells for pool %s, cos %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              pool.c_str(),
              cos);
  }
  return status;
}

bf_status_t BfRtTMPoolAppPfcTable::tableEntryMod(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const BfRtTableData &data) const {
  // Get the key fields
  const BfRtTMAppPoolPfcTableKey &appPoolPfcKey =
      static_cast<const BfRtTMAppPoolPfcTableKey &>(key);

  std::string pool;
  uint8_t cos;
  auto status = appPoolPfcKey.getIds(pool, cos);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the data fields from the data object
  const BfRtTMTableData &appPoolPfcData =
      static_cast<const BfRtTMTableData &>(data);

  bf_rt_id_t data_id = 0;
  status = this->dataFieldIdGet("pfc_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting fieldId for pfc_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  uint64_t pfc_limit_cells;
  status = appPoolPfcData.getValue(data_id, &pfc_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting data value for Id %d",
              __func__,
              __LINE__,
              table_name_get().c_str(),
              data_id);
    return status;
  }
  return this->tableEntryMod_internal(dev_tgt, pool, cos, pfc_limit_cells);
}

bf_status_t BfRtTMPoolAppPfcTable::tableEntryGet_internal(
    const bf_rt_target_t &dev_tgt,
    const std::string &pool,
    const uint8_t &cos,
    BfRtTableData *data) const {
  BfRtTMTableData *appPoolPfcData = static_cast<BfRtTMTableData *>(data);

  // Check the validity of the key fields and then call the TM API
  auto pool_id_itr = igAppPoolMap.find(pool);
  if (pool_id_itr == igAppPoolMap.end()) {
    LOG_TRACE("%s:%d %s Key value for field pool not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t max_pfc_levels = 0;
  auto status =
      trafficMgr->bfTmMaxPfcLevelsGet(dev_tgt.dev_id, &max_pfc_levels);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get max PFC levels",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (cos >= max_pfc_levels) {
    LOG_TRACE("%s:%d %s Key value for field cos not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  uint32_t pfc_limit_cells = 0;
  status = trafficMgr->bfTmPoolPfcLimitGet(
      dev_tgt.dev_id, pool_id_itr->second, cos, &pfc_limit_cells);
  if (status != BF_SUCCESS) {
    LOG_TRACE(
        "%s:%d %s Failed to get pool PFC limit cells for pool %s at pfc level "
        "%d",
        __func__,
        __LINE__,
        table_name_get().c_str(),
        pool.c_str(),
        cos);
    return status;
  }

  bf_rt_id_t data_id;
  status = this->dataFieldIdGet("pfc_limit_cells", &data_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for pfc_limit_cells",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status =
      appPoolPfcData->setValue(data_id, static_cast<uint64_t>(pfc_limit_cells));
  if (status) {
    LOG_TRACE(
        "%s:%d %s Error in Setting data object for "
        "pfc_limit_cells",
        __func__,
        __LINE__,
        table_name_get().c_str());
    return status;
  }
  return status;
}

bf_status_t BfRtTMPoolAppPfcTable::tableEntryGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    BfRtTableData *data) const {
  const BfRtTMAppPoolPfcTableKey &appPoolPfcKey =
      static_cast<const BfRtTMAppPoolPfcTableKey &>(key);

  std::string pool;
  uint8_t cos;
  auto status = appPoolPfcKey.getIds(pool, cos);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  return this->tableEntryGet_internal(dev_tgt, pool, cos, data);
}

bf_status_t BfRtTMPoolAppPfcTable::tableEntryGetFirst(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    BfRtTableKey *key,
    BfRtTableData *data) const {
  bf_status_t status = BF_SUCCESS;
  BfRtTMAppPoolPfcTableKey *appPoolPfcKey =
      static_cast<BfRtTMAppPoolPfcTableKey *>(key);

  bf_rt_id_t key_id;
  status = this->keyFieldIdGet("pool", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for color",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the possible values for "pool" field and pick the first one
  std::vector<std::string> pool_choices;
  status = this->getKeyStringChoices("pool", pool_choices);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  std::string pool_value = pool_choices.front();
  status = appPoolPfcKey->setValue(key_id, pool_value);
  if (status) {
    LOG_TRACE("%s:%d %s Error in setting key object for pool field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Set "cos" field to 0
  status = this->keyFieldIdGet("cos", &key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding fieldId for cos",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  auto cos_value = 0;
  status = appPoolPfcKey->setValue(key_id, cos_value);
  if (status) {
    LOG_TRACE("%s:%d %s Error in setting key object for cos field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return tableEntryGet_internal(dev_tgt, pool_value, cos_value, data);
}

bf_status_t BfRtTMPoolAppPfcTable::tableEntryGetNext_n(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableKey &key,
    const uint32_t &n,
    keyDataPairs *key_data_pairs,
    uint32_t *num_returned) const {
  const BfRtTMAppPoolPfcTableKey &appPoolPfcKey =
      static_cast<const BfRtTMAppPoolPfcTableKey &>(key);

  std::string pool;
  uint8_t cos;
  auto status = appPoolPfcKey.getIds(pool, cos);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in getting key values",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the possible choices for the pool field
  bf_rt_id_t pool_key_id;
  bf_rt_id_t cos_key_id;
  status = this->keyFieldIdGet("pool", &pool_key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding key field id for pool",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  status = this->keyFieldIdGet("cos", &cos_key_id);
  if (status != BF_SUCCESS) {
    LOG_TRACE("%s:%d %s Error in finding key field id for cos",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get the possible pool choices
  std::vector<std::string> pools;
  status = this->getKeyStringChoices("pool", pools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Find index of the pool in pools vector
  uint32_t pool_idx;
  auto it = find(pools.begin(), pools.end(), pool);
  if (it != pools.end()) {
    pool_idx = (it - pools.begin());
  } else {
    LOG_TRACE("%s:%d %s Key value for field pool is not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // Check validity of cos value
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t max_pfc_levels = 0;
  status = trafficMgr->bfTmMaxPfcLevelsGet(dev_tgt.dev_id, &max_pfc_levels);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get max PFC levels",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }
  if (cos >= max_pfc_levels) {
    LOG_TRACE("%s:%d %s Key value for field cos not valid",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_INVALID_ARG;
  }

  // As we need to skip the current entry, increment cos value by 1
  cos++;

  uint32_t i = 0;
  for (; pool_idx < pools.size() && i < n; pool_idx++) {
    for (; cos < max_pfc_levels && i < n; cos++) {
      auto this_key =
          static_cast<BfRtTMAppPoolPfcTableKey *>((*key_data_pairs)[i].first);
      auto this_data =
          static_cast<BfRtTMTableData *>((*key_data_pairs)[i].second);

      status = this_key->setValue(pool_key_id, pools[pool_idx]);
      if (status) {
        LOG_ERROR("%s:%d %s Error in setting key object for pool",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      }

      status = this_key->setValue(cos_key_id, cos);
      if (status) {
        LOG_ERROR("%s:%d %s Error in setting key object for cos",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      }

      status = tableEntryGet_internal(dev_tgt, pools[pool_idx], cos, this_data);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Error in getting data for pool %s, cos %d",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  pools[pool_idx].c_str(),
                  cos);
        // Make the data object null if error
        (*key_data_pairs)[i].second = nullptr;
      }
      i++;
    }
    cos = 0;
  }
  if (num_returned) {
    *num_returned = i;
  }
  return status;
}

bf_status_t BfRtTMPoolAppPfcTable::tableUsageGet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t & /*dev_tgt*/,
    const uint64_t & /*flags*/,
    uint32_t *count) const {
  *count = this->_table_size;
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolAppPfcTable::tableClear(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  // Get the possible choices for the pool field
  std::vector<std::string> pools;
  auto status = this->getKeyStringChoices("pool", pools);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s: Unable to get pool choices",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Get all the possible PFC values
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint32_t max_pfc_levels = 0;
  status = trafficMgr->bfTmMaxPfcLevelsGet(dev_tgt.dev_id, &max_pfc_levels);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to get max PFC levels",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  for (const auto &pool : pools) {
    auto pool_id_itr = igAppPoolMap.find(pool);
    if (pool_id_itr == igAppPoolMap.end()) {
      LOG_TRACE("%s:%d %s Key value for field pool not valid",
                __func__,
                __LINE__,
                table_name_get().c_str());
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    for (uint32_t pfc = 0; pfc < max_pfc_levels; pfc++) {
      // Get the default values from TM
      uint32_t def_limit = 0;
      status = trafficMgr->bfTmPoolPfcLimitGetDefault(
          dev_tgt.dev_id, pool_id_itr->second, pfc, &def_limit);
      if (status != BF_SUCCESS) {
        LOG_TRACE(
            "%s:%d %s Failed to get default pfc limit cells for pool %s, cos "
            "%d.",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            pool.c_str(),
            pfc);
        return status;
      }

      // Set that value back
      this->tableEntryMod_internal(dev_tgt, pool, pfc, def_limit);
    }
  }
  return status;
}

bf_status_t BfRtTMPoolAppPfcTable::keyAllocate(
    std::unique_ptr<BfRtTableKey> *key_ret) const {
  *key_ret = std::unique_ptr<BfRtTableKey>(new BfRtTMAppPoolPfcTableKey(this));
  if (*key_ret == nullptr) {
    LOG_TRACE("%s:%d %s Error : Failed to allocate key",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

bf_status_t BfRtTMPoolAppPfcTable::keyReset(BfRtTableKey *key) const {
  BfRtTMAppPoolPfcTableKey *match_key =
      static_cast<BfRtTMAppPoolPfcTableKey *>(key);
  return match_key->reset();
}
}  // namespace bfrt
