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


#include "bf_rt_tm_table_impl_pipe.hpp"
#include <climits>
#include "bf_rt_tm_table_helper_pipe.hpp"

namespace bfrt {

bf_status_t BfRtTMPipeMulticastFifoTable::getPipeBitmap(
    const bf_rt_target_t &dev_tgt, uint8_t *bmap) const {
  // TODO: Consider to move this function to TM driver API.
  if (nullptr == bmap) {
    return BF_INVALID_ARG;
  }
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (BF_SUCCESS != status) {
    return status;
  }

  uint8_t pipe_bmap = 0;
  uint8_t n_pipes = 0;

  status = this->workPipesGet(dev_tgt, &n_pipes);
  if (BF_SUCCESS != status) {
    return status;
  }

  if (1 == n_pipes) {
    pipe_bmap = (1 << dev_tgt.pipe_id);
  } else if (n_pipes) {
    if (n_pipes > CHAR_BIT) {
      return BF_NOT_SUPPORTED;  // While Tofino has less than 8 pipes.
    }
    bf_rt_target_t dev_wrk = dev_tgt;
    auto *trafficMgr = TrafficMgrIntf::getInstance();
    status = trafficMgr->bfTMPipeGetFirst(dev_wrk.dev_id, &(dev_wrk.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id);
      return status;
    }
    do {
      pipe_bmap |= (1 << dev_wrk.pipe_id);

      status = trafficMgr->bfTMPipeGetNext(
          dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));
      if (BF_OBJECT_NOT_FOUND == status) {
        status = BF_SUCCESS;
        break;  // No more pipes on this device
      }
    } while (BF_SUCCESS == status);
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't compose pipe bitmask on dev_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id);
  }
  *bmap = pipe_bmap;
  return status;
}

bf_status_t BfRtTMPipeMulticastFifoTable::tableDefaultEntryGet(
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
    return status;
  }

  // Fetch the fields and set it in data
  BfRtTMTableData *tmPipeMcFifoData = static_cast<BfRtTMTableData *>(data);

  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // icos mask
  std::vector<bf_rt_id_t> icos_mask;
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  for (auto fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
    uint8_t curr_icos_mask = 0;
    status = trafficMgr->bfTmMcFifoIcosMappingGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, fifo, &curr_icos_mask);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get icos_mask for fifo = %u",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                fifo);
      return status;
    }
    icos_mask.push_back(static_cast<bf_rt_id_t>(curr_icos_mask));
  }
  status = tmPipeMcFifoData->setValueByName("icos_mask", icos_mask);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set icos_mask field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Arbitration mode
  std::vector<std::string> arb_mode;
  for (auto fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
    bool use_strict_priority;
    status = trafficMgr->bfTmMcFifoArbModeGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, fifo, &use_strict_priority);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get arbitration mode for fifo = %u",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                fifo);
      return status;
    }
    if (use_strict_priority) {
      arb_mode.push_back("STRICT_PRIORITY");
    } else {
      arb_mode.push_back("WRR");
    }
  }
  status = tmPipeMcFifoData->setValueByName("arbitration_mode", arb_mode);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set arbitration_mode field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // WRR weight
  std::vector<bf_rt_id_t> wrr_weight;
  for (auto fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
    uint8_t curr_wrr_weight = 0;
    status = trafficMgr->bfTmMcFifoWrrWeightGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, fifo, &curr_wrr_weight);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get wrr weight for fifo = %u",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                fifo);
      return status;
    }
    wrr_weight.push_back(static_cast<bf_rt_id_t>(curr_wrr_weight));
  }
  status = tmPipeMcFifoData->setValueByName("wrr_weight", wrr_weight);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set wrr_weight field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  // Depth
  std::vector<bf_rt_id_t> depth;
  for (auto fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
    int curr_depth = 0;
    status = trafficMgr->bfTmMcFifoDepthGet(
        dev_tgt.dev_id, dev_tgt.pipe_id, fifo, &curr_depth);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to get depth for fifo = %u",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                fifo);
      return status;
    }
    // Need to divide the value by 8 as that is how it has been
    // advertised in the JSON
    curr_depth /= 8;
    depth.push_back(static_cast<bf_rt_id_t>(curr_depth));
  }
  status = tmPipeMcFifoData->setValueByName("depth", depth);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s Failed to set depth field",
              __func__,
              __LINE__,
              table_name_get().c_str());
    return status;
  }

  return status;
}

bf_status_t BfRtTMPipeMulticastFifoTable::tableDefaultEntrySet(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/,
    const BfRtTableData &data) const {
  // Validation
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    return status;
  }

  // Fetch the fields and set it in data
  const BfRtTMTableData &tmPipeMcFifoData =
      static_cast<const BfRtTMTableData &>(data);
  return this->tableEntrySet_internal(dev_tgt, tmPipeMcFifoData);
}

bf_status_t BfRtTMPipeMulticastFifoTable::tableEntrySet_internal(
    const bf_rt_target_t &dev_tgt, const BfRtTMTableData &data) const {
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint8_t pipe_bmap = 0;

  auto status = this->getPipeBitmap(dev_tgt, &pipe_bmap);
  if (BF_SUCCESS != status) {
    return status;
  }
  // Acquire the entry level lock
  std::lock_guard<std::mutex> lock(this->entry_lock);

  // Fetch the data fields and modify
  // icos mask
  std::vector<bf_rt_id_t> icos_mask;
  status = data.getValueByName("icos_mask", &icos_mask);
  if (status == BF_SUCCESS) {
    for (uint32_t fifo = 0; fifo < icos_mask.size(); fifo++) {
      status = trafficMgr->bfTmMcFifoIcosMappingSet(
          dev_tgt.dev_id,
          pipe_bmap,
          fifo,
          static_cast<uint8_t>(icos_mask[fifo]));
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to set icos_mask for fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  fifo);
        return status;
      }
    }
  }

  // Arbitration mode
  std::vector<std::string> arbitration_mode;
  status = data.getValueByName("arbitration_mode", &arbitration_mode);
  if (status == BF_SUCCESS) {
    for (uint32_t fifo = 0; fifo < icos_mask.size(); fifo++) {
      // Validate the string
      std::vector<std::string> arbitration_modes;
      status =
          this->getDataStringChoices("arbitration_mode", arbitration_modes);
      if (BF_SUCCESS != status) {
        LOG_ERROR("%s:%d %s: Failed to get choices for arbitration mode",
                  __func__,
                  __LINE__,
                  table_name_get().c_str());
        return status;
      }
      if (find(arbitration_modes.begin(),
               arbitration_modes.end(),
               arbitration_mode[fifo]) == arbitration_modes.end()) {
        LOG_ERROR(
            "%s:%d %s fifo %d: Key value for field arbitration_mode is not "
            "valid",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            fifo);
        return BF_INVALID_ARG;
      }

      bool use_strict_pri = (arbitration_mode[fifo] == "STRICT_PRIORITY");
      status = trafficMgr->bfTmMcFifoArbModeSet(
          dev_tgt.dev_id, pipe_bmap, fifo, use_strict_pri);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to set arbitration_mode for fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  fifo);
        return status;
      }
    }
  }

  // WRR weight
  std::vector<bf_rt_id_t> wrr_weight;
  status = data.getValueByName("wrr_weight", &wrr_weight);
  if (status == BF_SUCCESS) {
    for (uint32_t fifo = 0; fifo < wrr_weight.size(); fifo++) {
      status = trafficMgr->bfTmMcFifoWrrWeightSet(
          dev_tgt.dev_id,
          pipe_bmap,
          fifo,
          static_cast<uint8_t>(wrr_weight[fifo]));
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to set wrr_weight for fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  fifo);
        return status;
      }
    }
  }

  // Depth
  std::vector<bf_rt_id_t> depth;
  status = data.getValueByName("depth", &depth);
  if (status == BF_SUCCESS) {
    for (uint32_t fifo = 0; fifo < depth.size(); fifo++) {
      // Need to multiply the depth value by 8 as that is how
      // it has been advertised in the JSON
      status = trafficMgr->bfTmMcFifoDepthSet(
          dev_tgt.dev_id,
          pipe_bmap,
          fifo,
          static_cast<uint16_t>(depth[fifo] * 8));
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to set depth for fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  fifo);
        return status;
      }
    }
  }

  return status;
}

bf_status_t BfRtTMPipeMulticastFifoTable::tableDefaultEntryReset(
    const BfRtSession & /*session*/,
    const bf_rt_target_t &dev_tgt,
    const uint64_t & /*flags*/) const {
  auto status = this->singleOrPipeAll_validate(dev_tgt);
  if (status != BF_SUCCESS) {
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_tgt_curr = dev_tgt;

  if (dev_tgt_curr.pipe_id == BF_DEV_PIPE_ALL) {
    status = trafficMgr->bfTMPipeGetFirst(dev_tgt_curr.dev_id,
                                          &(dev_tgt_curr.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt_curr.dev_id);
      return status;
    }
  }

  do {
    // Fetch the reset values for all the fifos
    std::vector<bf_rt_id_t> icos_mask;
    std::vector<std::string> arb_mode;
    std::vector<bf_rt_id_t> wrr_weight;
    std::vector<bf_rt_id_t> depth;
    for (auto fifo = 0; fifo < BF_PRE_FIFO_COUNT; fifo++) {
      // icos_mask
      uint8_t curr_icos_mask = 0;
      status = trafficMgr->bfTmMcFifoIcosMappingGetDefault(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, fifo, &curr_icos_mask);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Failed to get default icos_mask for pipe = %u, fifo = %u",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            fifo);
        return status;
      }
      icos_mask.push_back(static_cast<bf_rt_id_t>(curr_icos_mask));

      // Arbitration mode
      bool use_strict_priority;
      status = trafficMgr->bfTmMcFifoArbModeGetDefault(dev_tgt_curr.dev_id,
                                                       dev_tgt_curr.pipe_id,
                                                       fifo,
                                                       &use_strict_priority);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d %s Failed to get arbitration mode for pipe = %u, fifo = %u",
            __func__,
            __LINE__,
            table_name_get().c_str(),
            dev_tgt_curr.pipe_id,
            fifo);
        return status;
      }
      if (use_strict_priority) {
        arb_mode.push_back("STRICT_PRIORITY");
      } else {
        arb_mode.push_back("WRR");
      }

      // WRR weight
      uint8_t curr_wrr_weight = 0;
      status = trafficMgr->bfTmMcFifoWrrWeightGetDefault(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, fifo, &curr_wrr_weight);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to get wrr weight for pipe = %u, fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id,
                  fifo);
        return status;
      }
      wrr_weight.push_back(static_cast<bf_rt_id_t>(curr_wrr_weight));

      // Depth
      int curr_depth = 0;
      status = trafficMgr->bfTmMcFifoDepthGetDefault(
          dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, fifo, &curr_depth);
      if (status != BF_SUCCESS) {
        LOG_ERROR("%s:%d %s Failed to get depth for pipe = %u, fifo = %u",
                  __func__,
                  __LINE__,
                  table_name_get().c_str(),
                  dev_tgt_curr.pipe_id,
                  fifo);
        return status;
      }
      depth.push_back(static_cast<bf_rt_id_t>(curr_depth));
    }

    // Populate the data object with the reset values
    BfRtTMTableData tmPipeMcFifoData(this);
    status = tmPipeMcFifoData.setValueByName("icos_mask", icos_mask);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set icos_mask field",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    status = tmPipeMcFifoData.setValueByName("arbitration_mode", arb_mode);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set arbitration_mode field",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    status = tmPipeMcFifoData.setValueByName("wrr_weight", wrr_weight);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set wrr_weight field",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    status = tmPipeMcFifoData.setValueByName("depth", depth);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to set depth field",
                __func__,
                __LINE__,
                table_name_get().c_str());
      return status;
    }

    // Reset the entry
    status = this->tableEntrySet_internal(dev_tgt_curr, tmPipeMcFifoData);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d %s Failed to reset the entry for pipe = %u",
                __func__,
                __LINE__,
                table_name_get().c_str(),
                dev_tgt_curr.pipe_id);
      return status;
    }

    if (BF_DEV_PIPE_ALL != dev_tgt.pipe_id) {
      break;  // single pipe call
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_tgt_curr.dev_id, dev_tgt_curr.pipe_id, &(dev_tgt_curr.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more pipes on this device
    }
  } while (BF_SUCCESS == status);

  return status;
}

bf_status_t BfRtTMPipeMulticastFifoTable::tableClear(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags) const {
  return this->tableDefaultEntryReset(session, dev_tgt, flags);
}

//----------------------------- TM_PIPE_TABLE_INTF

bf_status_t BfRtTMPipeTableIntf::tableSizeGet(const BfRtSession & /*session*/,
                                              const bf_rt_target_t &dev_tgt,
                                              const uint64_t & /*flags*/,
                                              size_t *size) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  uint8_t sz = 0;
  auto status = this->workPipesGet(dev_tgt, &sz);
  if (BF_SUCCESS == status) {
    *size = static_cast<size_t>(sz);
  }
  return status;
}

//---
bf_status_t BfRtTMPipeTableIntf::resetDefaultEntries(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());
  //---
  uint8_t n_pipes = 0;
  auto status = this->workPipesGet(dev_tgt, &n_pipes);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d Invalid dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }
  if (n_pipes == 0) {
    return status;  // Strange, but nothing to do.
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_target_t dev_wrk = dev_tgt;

  if (n_pipes > 1) {
    // Prepare to iterate over all n_pipes
    // instead of only one given in dev_tgt.
    status = trafficMgr->bfTMPipeGetFirst(dev_wrk.dev_id, &(dev_wrk.pipe_id));
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get first pipe on dev_id=%d",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_wrk.dev_id);
      return status;
    }
  }

  do {
    status = BfRtTMTable::tableDefaultEntryReset(session, dev_wrk, flags);
    if (n_pipes == 1 || BF_SUCCESS != status) {
      break;
    }
    status = trafficMgr->bfTMPipeGetNext(
        dev_wrk.dev_id, dev_wrk.pipe_id, &(dev_wrk.pipe_id));
    if (BF_OBJECT_NOT_FOUND == status) {
      status = BF_SUCCESS;
      break;  // No more pipes on this device
    }
  } while (BF_SUCCESS == status);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't clear dev_id=%d pipe_id=%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_wrk.dev_id,
              dev_wrk.pipe_id);
  }
  return status;
}

//---
bf_status_t BfRtTMPipeTableIntf::tableDefaultEntryReset(
    const BfRtSession &session,
    const bf_rt_target_t &dev_tgt,
    const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->resetDefaultEntries(session, dev_tgt, flags);
}
//---
bf_status_t BfRtTMPipeTableIntf::tableClear(const BfRtSession &session,
                                            const bf_rt_target_t &dev_tgt,
                                            const uint64_t &flags) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  return this->resetDefaultEntries(session, dev_tgt, flags);
}

//----------------------------- TM_PIPE_CFG

bf_status_t BfRtTMPipeCfgTable::tableGetResetValues(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPipeCfgHelper pc_hlp;

  auto status = pc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Read defaults from TM
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (pc_hlp.f_mirror_drop_enable) {
      status = trafficMgr->bfTMPipeMirrorDropEnableDefaultGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.mirror_drop_enable));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_enable;
        break;
      }
    }

    if (pc_hlp.f_mirror_drop_dev_port || pc_hlp.f_mirror_drop_queue_nr ||
        pc_hlp.f_mirror_drop_pg_id || pc_hlp.f_mirror_drop_pg_queue) {
      // The (pg_id, pg_queue) is a non-volatile Queue key.
      status = trafficMgr->bfTMPipeMirrorOnDropDestDefaultGet(
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          &(pc_hlp.mirror_drop_dev_port),
          &(pc_hlp.mirror_drop_queue_nr));
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s Get Negative Mirror defaults failed, dev_id=%d, "
            "pipe_id=%d, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            status,
            bf_err_str(status));
        return status;
      }

      status = pc_hlp.completeMirrorDropFields(dev_tgt, *this);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_pg_id;
        break;
      }
    }

    if (pc_hlp.f_eg_limit_cells) {
      status = trafficMgr->bfTMPipeEgLimitDefaultGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.eg_limit_cells));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_limit_cells;
        break;
      }
    }

    if (pc_hlp.f_eg_hysteresis_cells) {
      status = trafficMgr->bfTMPipeEgHysteresisDefaultGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.eg_hysteresis_cells));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_hysteresis_cells;
        break;
      }
    }

    if (pc_hlp.f_queue_depth_report_mode) {
      status = trafficMgr->bfTMPipeQstatReportDefaultGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.queue_depth_report_mode));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_queue_depth_report_mode;
        break;
      }
    }
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d read default value failed, dev_id=%d, pipe_id=%d, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = pc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPipeCfgTable::tableGetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPipeCfgHelper pc_hlp;

  auto status = pc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Read from TM

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (pc_hlp.f_mirror_drop_enable) {
      status = trafficMgr->bfTMPipeMirrorDropEnableGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.mirror_drop_enable));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_enable;
        break;
      }
    }

    if (pc_hlp.f_mirror_drop_dev_port || pc_hlp.f_mirror_drop_queue_nr ||
        pc_hlp.f_mirror_drop_pg_id || pc_hlp.f_mirror_drop_pg_queue) {
      // The (pg_id, pg_queue) is a non-volatile Queue key.
      status = trafficMgr->bfTMPipeMirrorOnDropDestGet(
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          &(pc_hlp.mirror_drop_dev_port),
          &(pc_hlp.mirror_drop_queue_nr));
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s Get Negative Mirror failed, dev_id=%d, pipe_id=%d, "
            "rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            dev_tgt.pipe_id,
            status,
            bf_err_str(status));
        return status;
      }

      status = pc_hlp.completeMirrorDropFields(dev_tgt, *this);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_pg_id;
        break;
      }
    }

    if (pc_hlp.f_eg_limit_cells) {
      status = trafficMgr->bfTMPipeEgLimitGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.eg_limit_cells));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_limit_cells;
        break;
      }
    }

    if (pc_hlp.f_eg_hysteresis_cells) {
      status = trafficMgr->bfTMPipeEgHysteresisGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.eg_hysteresis_cells));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_hysteresis_cells;
        break;
      }
    }

    if (pc_hlp.f_queue_depth_report_mode) {
      status = trafficMgr->bfTMPipeQstatReportGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(pc_hlp.queue_depth_report_mode));
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_queue_depth_report_mode;
        break;
      }
    }
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d read value failed, dev_id=%d, pipe_id=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = pc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPipeCfgTable::tableSetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  BfRtTMPipeCfgHelper pc_hlp;

  auto status = pc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  status = pc_hlp.getFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Chesk field values
  if (!(pc_hlp.f_mirror_drop_pg_id && pc_hlp.f_mirror_drop_pg_queue) &&
      (pc_hlp.f_mirror_drop_pg_id || pc_hlp.f_mirror_drop_pg_queue)) {
    LOG_ERROR(
        "%s:%d %s both mirror_drop_pg_id and mirror_drop_pg_queue "
        "must be given, dev_id=%d, pipe_id=%d",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id);
    return BF_INVALID_ARG;
  }

  //--- Write to TM
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (pc_hlp.f_mirror_drop_enable) {
      status = trafficMgr->bfTMPipeMirrorDropEnableSet(
          dev_tgt.dev_id, dev_tgt.pipe_id, pc_hlp.mirror_drop_enable);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_enable;
        break;
      }
    }

    if (pc_hlp.f_mirror_drop_pg_id && pc_hlp.f_mirror_drop_pg_queue) {
      // Get (dev_port, queue_nr) and check it is mapped.
      bool is_mapped = false;

      status =
          trafficMgr->bfTMPortGroupPortQueueGet(dev_tgt,
                                                pc_hlp.mirror_drop_pg_id,
                                                pc_hlp.mirror_drop_pg_queue,
                                                &(pc_hlp.mirror_drop_dev_port),
                                                &(pc_hlp.mirror_drop_queue_nr),
                                                &is_mapped);
      if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s Can't get negative mirror port queue for dev_id=%d "
            "pg_id=%d pg_queue=%d, rc=%d(%s)",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            pc_hlp.mirror_drop_pg_id,
            pc_hlp.mirror_drop_pg_queue,
            status,
            bf_err_str(status));
        return status;
      }
      // Some validations
      if (!is_mapped) {
        LOG_WARN(
            "%s:%d %s Negative Mirroring queue is not mapped dev_id=%d "
            "pg_id=%d pg_queue=%d",
            __func__,
            __LINE__,
            this->table_name_get().c_str(),
            dev_tgt.dev_id,
            pc_hlp.mirror_drop_pg_id,
            pc_hlp.mirror_drop_pg_queue);
      }

      status =
          trafficMgr->bfTMPipeMirrorOnDropDestSet(dev_tgt.dev_id,
                                                  dev_tgt.pipe_id,
                                                  pc_hlp.mirror_drop_dev_port,
                                                  pc_hlp.mirror_drop_queue_nr);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_mirror_drop_dev_port;
        break;
      }
    }

    if (pc_hlp.f_eg_limit_cells) {
      status = trafficMgr->bfTMPipeEgLimitSet(
          dev_tgt.dev_id, dev_tgt.pipe_id, pc_hlp.eg_limit_cells);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_limit_cells;
        break;
      }
    }

    if (pc_hlp.f_eg_hysteresis_cells) {
      status = trafficMgr->bfTMPipeEgHysteresisSet(
          dev_tgt.dev_id, dev_tgt.pipe_id, pc_hlp.eg_hysteresis_cells);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_eg_hysteresis_cells;
        break;
      }
    }

    if (pc_hlp.f_queue_depth_report_mode) {
      status = trafficMgr->bfTMPipeQstatReportSet(
          dev_tgt.dev_id, dev_tgt.pipe_id, pc_hlp.queue_depth_report_mode);
      if (BF_SUCCESS != status) {
        f_error = pc_hlp.f_queue_depth_report_mode;
        break;
      }
    }
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d write value failed, dev_id=%d, pipe_id=%d, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
  }

  return status;
}

//----------------------------- TM_PIPE_SCHED_CFG

bf_status_t BfRtTMPipeSchedCfgTable::tableGetResetValues(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPipeSchedCfgHelper psc_hlp;

  auto status = psc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Read reset values from TM
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (psc_hlp.f_advanced_flow_control_enable) {
      status = trafficMgr->bfTMPipeSchedAdvFcModeDefaultGet(
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          &(psc_hlp.advanced_flow_control_enable));
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_advanced_flow_control_enable;
        break;
      }
    }
    if (psc_hlp.f_packet_ifg_compensation) {
      status = trafficMgr->bfTMPipeSchedPktIfgCompDefaultGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(psc_hlp.packet_ifg_compensation));
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_packet_ifg_compensation;
        break;
      }
    }
  } while (0);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d read reset value failed, dev_id=%d, pipe_id=%d, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = psc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't reset fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPipeSchedCfgTable::tableGetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  BfRtTMPipeSchedCfgHelper psc_hlp;

  auto status = psc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Read values from TM
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (psc_hlp.f_advanced_flow_control_enable) {
      status = trafficMgr->bfTMPipeSchedAdvFcModeGet(
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          &(psc_hlp.advanced_flow_control_enable));
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_advanced_flow_control_enable;
        break;
      }
    }
    if (psc_hlp.f_packet_ifg_compensation) {
      status = trafficMgr->bfTMPipeSchedPktIfgCompGet(
          dev_tgt.dev_id, dev_tgt.pipe_id, &(psc_hlp.packet_ifg_compensation));
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_packet_ifg_compensation;
        break;
      }
    }
  } while (0);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d read value failed, dev_id=%d, pipe_id=%d, rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
    return status;
  }

  status = psc_hlp.setFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPipeSchedCfgTable::tableSetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  BfRtTMPipeSchedCfgHelper psc_hlp;

  auto status = psc_hlp.initFieldIds(*this, wrk_fields);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't init fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  status = psc_hlp.getFieldValues(*this, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't get fields dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Write values into TM
  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_rt_id_t f_error = 0;

  do {
    if (psc_hlp.f_advanced_flow_control_enable) {
      status = trafficMgr->bfTMPipeSchedAdvFcModeSet(
          dev_tgt.dev_id,
          dev_tgt.pipe_id,
          psc_hlp.advanced_flow_control_enable);
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_advanced_flow_control_enable;
        break;
      }
    }
    if (psc_hlp.f_packet_ifg_compensation) {
      status = trafficMgr->bfTMPipeSchedPktIfgCompSet(
          dev_tgt.dev_id, dev_tgt.pipe_id, psc_hlp.packet_ifg_compensation);
      if (BF_SUCCESS != status) {
        f_error = psc_hlp.f_packet_ifg_compensation;
        break;
      }
    }
  } while (0);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s field %d write value failed, dev_id=%d, pipe_id=%d, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        f_error,
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        status,
        bf_err_str(status));
  }

  return status;
}

}  // namespace bfrt
