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

#include "bf_rt_tm_table_helper_ppg.hpp"
#include "bf_rt_tm_table_impl_mirror.hpp"

namespace bfrt {

//----------- TM_MIRROR_DPG

bf_status_t BfRtTMMirrorDpgTable::tableGetResetValues(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status = trafficMgr->bfTMPPGMirrorPortHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get mirror DPG handle for dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Buffer
  {
    BfRtTMPpgBufferHelper ppg_helper;

    status = ppg_helper.resetFieldsBuffer(
        dev_tgt, *this, dpg_hdl, wrk_fields, p_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s can't reset fields dev_id=%d mirror dpg_hdl=0x%x",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dpg_hdl);
    }
  }
  return status;
}

//---
bf_status_t BfRtTMMirrorDpgTable::tableGetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    BfRtTMTableData *p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status = trafficMgr->bfTMPPGMirrorPortHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get mirror DPG handle for dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  //--- Buffer
  {
    BfRtTMPpgBufferHelper ppg_helper;

    status =
        ppg_helper.getFieldsBuffer(dev_tgt, *this, dpg_hdl, wrk_fields, p_data);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s can't get fields dev_id=%d mirror dpg_hdl=0x%x",
                __func__,
                __LINE__,
                this->table_name_get().c_str(),
                dev_tgt.dev_id,
                dpg_hdl);
      return status;
    }
  }

  //--- iCoS
  bf_dev_port_t mirror_port_id = 0;
  status = trafficMgr->bfTMPPGPortGet(dev_tgt.dev_id, dpg_hdl, &mirror_port_id);
  if (status) {
    LOG_ERROR(
        "%s:%d %s Can't get mirror Port for dev_id=%d pipe_id=%d "
        "ppg_hdl=0x%x",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        dpg_hdl);
    return status;
  }

  //-- PPG lock to fix the state
  auto device_state = BfRtDevMgrImpl::bfRtDeviceStateGet(
      dev_tgt.dev_id, this->programNameGet());
  if (nullptr == device_state) {
    LOG_ERROR("%s:%d ERROR device state dev_id=%d, program='%s'",
              __func__,
              __LINE__,
              dev_tgt.dev_id,
              this->programNameGet().c_str());
    BF_RT_DBGCHK(0);
    return BF_UNEXPECTED;
  }
  auto tmPpgState = device_state->tmPpgState.getStateObj();

  LOG_DBG("%s:%d %s Get PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());
  // The entry lock should be already taken.
  // Lock the PPG state to read consistent iCoS mappings.
  std::lock_guard<std::mutex> state_mux(tmPpgState->state_lock);
  LOG_DBG("%s:%d %s Got PPG state lock",
          __func__,
          __LINE__,
          this->table_name_get().c_str());

  //-- Read DPG iCoS
  uint8_t icos_mask = 0;
  status = BfRtTMPpgIcosHelper::getFieldsIcos(
      dev_tgt, *this, dpg_hdl, wrk_fields, p_data, icos_mask);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get iCoS fields dev_id=%d pipe_id=%d "
        "mirror port_id=%d dpg_hdl=0x%x",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        mirror_port_id,
        dpg_hdl);

    return status;
  }

  status = BfRtTMPpgIcosHelper::getFieldsPPGs(dev_tgt,
                                              *this,
                                              *tmPpgState,
                                              mirror_port_id,
                                              icos_mask,
                                              wrk_fields,
                                              p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s can't get PPG fields dev_id=%d pipe_id=%d "
        "mirror port_id=%d dpg_hdl=0x%x",
        __func__,
        __LINE__,
        this->table_name_get().c_str(),
        dev_tgt.dev_id,
        dev_tgt.pipe_id,
        mirror_port_id,
        dpg_hdl);
  }

  return status;
}

//---
bf_status_t BfRtTMMirrorDpgTable::tableSetDefaultFields(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields) const {
  LOG_DBG("%s:%d %s", __func__, __LINE__, this->table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_tm_ppg_hdl dpg_hdl = 0;

  auto status = trafficMgr->bfTMPPGMirrorPortHandleGet(
      dev_tgt.dev_id, dev_tgt.pipe_id, &dpg_hdl);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get mirror DPG handle for dev_id=%d pipe_id=%d",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dev_tgt.pipe_id);
    return status;
  }

  BfRtTMPpgBufferHelper ppg_helper;

  status =
      ppg_helper.setFieldsBuffer(dev_tgt, *this, p_data, wrk_fields, dpg_hdl);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set fields dev_id=%d mirror dpg_hdl=0x%x",
              __func__,
              __LINE__,
              this->table_name_get().c_str(),
              dev_tgt.dev_id,
              dpg_hdl);
  }

  return status;
}
}  // namespace bfrt
