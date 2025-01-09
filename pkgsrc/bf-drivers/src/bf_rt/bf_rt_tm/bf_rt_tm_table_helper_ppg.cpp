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


#include "bf_rt_tm_table_helper_ppg.hpp"
#include "bf_rt_tm_table_impl_port.hpp"
#include "bf_rt_tm_table_impl_ppg.hpp"

namespace bfrt {

//----------- TM_PPG_ICOS_HELPER

const char *BfRtTMPpgIcosHelper::icos_fname[CHAR_BIT] = {"icos_0",
                                                         "icos_1",
                                                         "icos_2",
                                                         "icos_3",
                                                         "icos_4",
                                                         "icos_5",
                                                         "icos_6",
                                                         "icos_7"};
//---
bf_status_t BfRtTMPpgIcosHelper::getValuesIcos(const BfRtTMTable &w_table,
                                               const BfRtTMTableData &p_data,
                                               std::set<bf_rt_id_t> &wrk_fields,
                                               uint8_t &new_icos,
                                               uint8_t &new_mask) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (!(p_data.hasValues()) || wrk_fields.empty()) {
    return BF_SUCCESS;  // Nothing to do is ok.
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t do_action_id = 0;
  bf_rt_id_t data_id = 0;
  uint8_t res_icos = 0;
  uint8_t res_mask = 0;

  uint8_t w_mask = 1;
  bool icos_i = false;

  for (int i = 0; i < CHAR_BIT && BF_SUCCESS == status; w_mask <<= 1, i++) {
    data_id = 0;

    status = BfRtTMPpgHelper::popWorkField(
        w_table, icos_fname[i], do_action_id, wrk_fields, data_id);

    if (BF_SUCCESS == status && data_id && p_data.hasValue(data_id)) {
      status = p_data.getValue(data_id, &icos_i);
      if (BF_SUCCESS == status) {
        res_icos = (icos_i) ? (res_icos | w_mask) : (res_icos & ~w_mask);
        res_mask |= w_mask;  // Mark this bit as significant for the change.
      }
    }
  }
  LOG_DBG("%s:%d %s res_icos=0x%x res_mask=0x%x",
          __func__,
          __LINE__,
          w_table.table_name_get().c_str(),
          res_icos,
          res_mask);

  if (BF_SUCCESS == status) {
    new_icos = res_icos;
    new_mask = res_mask;
  } else {
    LOG_ERROR("%s:%d %s can't get field_id=%d value",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              data_id);
  }

  return status;
}

//---
bf_status_t BfRtTMPpgIcosHelper::writeValuesIcos(const bf_rt_target_t &dev_tgt,
                                                 const BfRtTMTable &w_table,
                                                 bf_tm_ppg_hdl ppg_hdl,
                                                 uint8_t new_icos,
                                                 uint8_t new_mask) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());
  bf_status_t status = BF_UNEXPECTED;

  if (new_mask == 0) {
    LOG_DBG("%s:%d %s no iCoS to set",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str());
    return BF_SUCCESS;  // No iCoS to change.
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  uint8_t old_icos = 0;

  // Get current bitmap to update it if not all bits to set.
  if (new_mask != UINT8_MAX) {
    status =
        trafficMgr->bfTMPPGIcosMappingGet(dev_tgt.dev_id, ppg_hdl, &old_icos);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s dev_id=%d PPG 0x%x can't get iCoS, rc=%d(%s)",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                dev_tgt.dev_id,
                ppg_hdl,
                status,
                bf_err_str(status));
      return status;
    }
    new_icos |= (old_icos & (~new_mask));

    LOG_DBG("%s:%d %s dev_id=%d ppg_hdl=0x%x change iCoS=0x%x to 0x%x",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            dev_tgt.dev_id,
            ppg_hdl,
            old_icos,
            new_icos);
  } else {
    LOG_DBG("%s:%d %s dev_id=%d ppg_hdl=0x%x set iCoS=0x%x",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            dev_tgt.dev_id,
            ppg_hdl,
            new_icos);
  }

  // Application have to take care of PPGs 'detached' from iCoS
  if (0 == new_icos) {
    LOG_TRACE(
        "%s:%d %s dev_id=%d ppg_hdl=0x%x set to zero requested."
        "The PPG should be either assigned to some iCoS or deleted.",
        __func__,
        __LINE__,
        w_table.table_name_get().c_str(),
        dev_tgt.dev_id,
        ppg_hdl);
  }

  status = trafficMgr->bfTMPPGIcosMappingSet(dev_tgt.dev_id, ppg_hdl, new_icos);
  if (BF_SUCCESS != status) {
    LOG_ERROR(
        "%s:%d %s dev_id=%d ppg_id=0x%x can't change iCoS=0x%x to 0x%x, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        w_table.table_name_get().c_str(),
        dev_tgt.dev_id,
        ppg_hdl,
        old_icos,
        new_icos,
        status,
        bf_err_str(status));
  }

  return status;
}

//-----------
bf_status_t BfRtTMPpgIcosHelper::setValuesIcos(const BfRtTMTable &w_table,
                                               const uint8_t icos_mask,
                                               std::set<bf_rt_id_t> &wrk_fields,
                                               BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  bf_rt_id_t do_action_id = 0;
  bf_rt_id_t data_id = 0;

  uint8_t w_mask = 1;
  for (int i = 0; i < CHAR_BIT && BF_SUCCESS == status; w_mask <<= 1, i++) {
    status = BfRtTMPpgHelper::popWorkField(
        w_table, icos_fname[i], do_action_id, wrk_fields, data_id);
    if (BF_SUCCESS == status && data_id) {
      status = p_data->setValue(data_id, static_cast<bool>(w_mask & icos_mask));
    }
  }

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set field_id=%d value",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              data_id);
  }
  return status;
}

bf_status_t BfRtTMPpgIcosHelper::setValuesPPG(
    const BfRtTMTable &w_table,
    const std::vector<bf_tm_ppg_id_t> &ppg_ids,
    std::set<bf_rt_id_t> &wrk_fields,
    BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }
  if (wrk_fields.empty()) {
    return BF_SUCCESS;
  }

  bf_rt_id_t data_id = 0;

  bf_status_t status = w_table.dataFieldIdGet("icos_ppg_map", &data_id);
  if (BF_SUCCESS == status) {
    auto f_found = wrk_fields.find(data_id);
    if (f_found != wrk_fields.end()) {
      std::vector<bf_rt_id_t> ppg_map(ppg_ids.begin(), ppg_ids.end());

      status = p_data->setValue(data_id, ppg_map);
      if (BF_SUCCESS == status) {
        wrk_fields.erase(f_found);
      }
    }
  }

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set icos_ppg_map, rc=%d(%s)",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              status,
              bf_err_str(status));
  }

  return status;
}

//----
bf_status_t BfRtTMPpgIcosHelper::getFieldsIcos(const bf_rt_target_t &dev_tgt,
                                               const BfRtTMTable &w_table,
                                               bf_tm_ppg_hdl ppg_hdl,
                                               std::set<bf_rt_id_t> &wrk_fields,
                                               BfRtTMTableData *p_data,
                                               uint8_t &icos_mask) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // Process DPG iCoS mapping.
  uint8_t icos_mask_w = 0;
  auto status =
      trafficMgr->bfTMPPGIcosMappingGet(dev_tgt.dev_id, ppg_hdl, &icos_mask_w);
  if (status) {
    LOG_ERROR("%s:%d %s Can't get TM PPG iCoS map for dev_id=%d ppg_hdl=0x%x",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              dev_tgt.dev_id,
              ppg_hdl);
    return status;
  }

  status = BfRtTMPpgIcosHelper::setValuesIcos(
      w_table, icos_mask_w, wrk_fields, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set iCoS values dev_id=%d ppg_hdl=0x%x",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              dev_tgt.dev_id,
              ppg_hdl);
  }

  icos_mask = icos_mask_w;
  return status;
}

bf_status_t BfRtTMPpgIcosHelper::getFieldsPPGs(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTable &w_table,
    const BfRtTMPpgStateObj &ppg_state,
    bf_dev_port_t port_id,
    uint8_t icos_mask,
    std::set<bf_rt_id_t> &wrk_fields,
    BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // Process PPG ids.

  uint8_t icos_cnt = 0;
  auto status =
      trafficMgr->bfTMPortIcosCntGet(dev_tgt.dev_id, port_id, &icos_cnt);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't get Port settings for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  std::vector<bf_tm_ppg_hdl> ppg_hdlrs(icos_cnt, 0);
  uint8_t ppg_mask = ~icos_mask;  // PPG's are inverse of DPG iCoS map

  // Read the actual PPG map to check even if empty is expected.
  uint8_t got_mask = 0;
  status = trafficMgr->bfTMPortPPGMapGet(
      dev_tgt.dev_id, port_id, &got_mask, ppg_hdlrs.data());
  if (BF_SUCCESS == status && got_mask != ppg_mask) {
    LOG_TRACE(
        "%s:%d %s TM race condition on DPG and PPG iCoS read "
        "dev_id=%d port_id=%d",
        __func__,
        __LINE__,
        w_table.table_name_get().c_str(),
        dev_tgt.dev_id,
        port_id);
    return BF_EAGAIN;
  }
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't get Port PPG iCoS map for dev_id=%d port_id=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              dev_tgt.dev_id,
              port_id);
    return status;
  }

  // Translate PPG handlers to PPG id's known.
  //
  // The mask and ppg_hdlrs[] should be consistent at this point, however
  // it is possible that some PPGs are created directly by TM API calls from
  // another application, so there are no PPG handlers and user-defined ids
  // registered in the BF-RT TM internal state for them.
  // These 'hidden' PPGs are count to the tm.ppg.cfg table usage,
  // and their iCoS-es are shown as detached from DPGs.

  std::vector<bf_tm_ppg_id_t> ppg_ids;
  uint8_t w_mask = 1;

  for (int i = 0; i < CHAR_BIT; w_mask <<= 1, i++) {
    bf_tm_ppg_id_t ppg_id = 0;

    if (got_mask & w_mask) {
      status = ppg_state.stateTMPpgHdlGet(
          DEV_PORT_TO_PIPE(port_id), ppg_hdlrs.at(i), &ppg_id);
      if (BF_OBJECT_NOT_FOUND == status) {
        LOG_TRACE(
            "%s:%d %s unknown ppg_id for ppg_hdl=0x%x on dev_id=%d pipe_id=%d "
            "dev_port=%d icos_%d",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            ppg_hdlrs.at(i),
            dev_tgt.dev_id,
            DEV_PORT_TO_PIPE(port_id),
            port_id,
            i);
        ppg_id = 0;  // set as nil
      } else if (BF_SUCCESS != status) {
        LOG_ERROR(
            "%s:%d %s failed ppg_id for ppg_hdl=0x%x on dev_id=%d pipe_id=%d "
            "dev_port=%d icos_%d, rc=%d(%s)",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            ppg_hdlrs.at(i),
            dev_tgt.dev_id,
            DEV_PORT_TO_PIPE(port_id),
            port_id,
            i,
            status,
            bf_err_str(status));
        return status;
      }
    }

    ppg_ids.push_back(ppg_id);
  }

  status =
      BfRtTMPpgIcosHelper::setValuesPPG(w_table, ppg_ids, wrk_fields, p_data);
  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s can't set PPG ids' values",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str());
  }
  return status;
}

//----------- TM_PPG_BUFFER_HELPER

const std::map<std::string, bf_tm_app_pool_t>
    BfRtTMPpgBufferHelper::str_to_ig_pool{
        {"IG_APP_POOL_0", BF_TM_IG_APP_POOL_0},
        {"IG_APP_POOL_1", BF_TM_IG_APP_POOL_1},
        {"IG_APP_POOL_2", BF_TM_IG_APP_POOL_2},
        {"IG_APP_POOL_3", BF_TM_IG_APP_POOL_3},
    };

const std::map<bf_tm_app_pool_t, std::string>
    BfRtTMPpgBufferHelper::ig_pool_to_str{
        {BF_TM_IG_APP_POOL_0, "IG_APP_POOL_0"},
        {BF_TM_IG_APP_POOL_1, "IG_APP_POOL_1"},
        {BF_TM_IG_APP_POOL_2, "IG_APP_POOL_2"},
        {BF_TM_IG_APP_POOL_3, "IG_APP_POOL_3"},
    };

const std::map<std::string, bf_tm_ppg_baf_t>
    BfRtTMPpgBufferHelper::str_to_ppgbaf{
        {"1.5%", BF_TM_PPG_BAF_1_POINT_5_PERCENT},
        {"3%", BF_TM_PPG_BAF_3_PERCENT},
        {"6%", BF_TM_PPG_BAF_6_PERCENT},
        {"11%", BF_TM_PPG_BAF_11_PERCENT},
        {"20%", BF_TM_PPG_BAF_20_PERCENT},
        {"33%", BF_TM_PPG_BAF_33_PERCENT},
        {"50%", BF_TM_PPG_BAF_50_PERCENT},
        {"66%", BF_TM_PPG_BAF_66_PERCENT},
        {"80%", BF_TM_PPG_BAF_80_PERCENT},
        {"DISABLE", BF_TM_PPG_BAF_DISABLE}};

const std::map<bf_tm_ppg_baf_t, std::string>
    BfRtTMPpgBufferHelper::ppgbaf_to_str{
        {BF_TM_PPG_BAF_1_POINT_5_PERCENT, "1.5%"},
        {BF_TM_PPG_BAF_3_PERCENT, "3%"},
        {BF_TM_PPG_BAF_6_PERCENT, "6%"},
        {BF_TM_PPG_BAF_11_PERCENT, "11%"},
        {BF_TM_PPG_BAF_20_PERCENT, "20%"},
        {BF_TM_PPG_BAF_33_PERCENT, "33%"},
        {BF_TM_PPG_BAF_50_PERCENT, "50%"},
        {BF_TM_PPG_BAF_66_PERCENT, "66%"},
        {BF_TM_PPG_BAF_80_PERCENT, "80%"},
        {BF_TM_PPG_BAF_DISABLE, "DISABLE"}};

bf_status_t BfRtTMPpgBufferHelper::setFieldsBuffer(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTable &w_table,
    const BfRtTMTableData &p_data,
    std::set<bf_rt_id_t> &wrk_fields,
    bf_tm_ppg_hdl ppg_hdl) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  auto status = this->initFieldIds(w_table, wrk_fields);
  if (BF_SUCCESS != status) {
    return status;
  }

  // Read the current block of related values to have everything for update.
  status = this->readValues(dev_tgt, w_table, ppg_hdl, true);
  if (BF_SUCCESS != status) {
    return status;
  }

  // Get what values are given in the data object overriding current values.
  status = this->getFieldValues(w_table, p_data);
  if (BF_SUCCESS != status) {
    return status;
  }

  // Update the block of values in HW.
  return this->writeValues(dev_tgt, w_table, ppg_hdl);
}

//---
bf_status_t BfRtTMPpgBufferHelper::initFieldIds(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  //---- Select fields according to the action and TM APIs
  f_guaranteed_cells = 0;
  f_hysteresis_cells = 0;
  f_pool_id = 0;
  f_pool_max_cells = 0;
  f_dynamic_baf = 0;

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t do_action_id = 0;
  std::string field_name;

  do {
    field_name = "guaranteed_cells";
    status = this->popWorkField(
        w_table, field_name, do_action_id, wrk_fields, f_guaranteed_cells);
    if (BF_SUCCESS != status) {
      break;
    }

    field_name = "hysteresis_cells";
    status = this->popWorkField(
        w_table, field_name, do_action_id, wrk_fields, f_hysteresis_cells);
    if (BF_SUCCESS != status) {
      break;
    }

    field_name = "pool_id";
    status = this->popWorkField(
        w_table, field_name, do_action_id, wrk_fields, f_pool_id);
    if (BF_SUCCESS != status) {
      break;
    }

    field_name = "pool_max_cells";
    status = this->popWorkField(
        w_table, field_name, do_action_id, wrk_fields, f_pool_max_cells);
    if (BF_SUCCESS != status) {
      break;
    }

    field_name = "dynamic_baf";
    status = this->popWorkField(
        w_table, field_name, do_action_id, wrk_fields, f_dynamic_baf);

  } while (0);

  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d %s field '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }

  return status;
}

//---
bf_status_t BfRtTMPpgBufferHelper::readValues(const bf_rt_target_t &dev_tgt,
                                              const BfRtTMTable &w_table,
                                              bf_tm_ppg_hdl ppg_hdl,
                                              bool for_update) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  dynamic_baf_str = "";
  pool_id_str = "";
  pool_id = BF_TM_IG_APP_POOL_0;
  pool_max_cells = 0;
  dynamic_baf = BF_TM_PPG_BAF_80_PERCENT;
  guaranteed_cells = 0;
  hysteresis_cells = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (f_guaranteed_cells && !(for_update)) {
    // Do not read current single value before its update where its new
    // value is given.
    status = trafficMgr->bfTMPPGuaranteedMinLimitGet(
        dev_tgt.dev_id, ppg_hdl, &guaranteed_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get guaranteed cells for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  }

  //----- Read block of related fields:
  // Load all these three if we need to update at least one, but no need
  // to read current values on update when we have all new values given.
  // The f_hysteresis_cells will be also read and later possibly overriden
  // from the data object if provided for an update.

  bool f_block_need = (f_pool_id || f_pool_max_cells || f_dynamic_baf);
  bool f_block_done = false;

  if (f_block_need &&
      !(f_pool_id && f_pool_max_cells && f_dynamic_baf && for_update)) {
    LOG_DBG("%s:%d %s Read current block fields for ppg_hdl=0x%x",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            ppg_hdl);

    uint32_t pool_id_val = 0;
    status =
        trafficMgr->bfTMPPGAppPoolIdGet(dev_tgt.dev_id, ppg_hdl, &pool_id_val);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get PG app pool for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
    pool_id = static_cast<bf_tm_app_pool_t>(pool_id_val);

    status = trafficMgr->bfTMPPGAppPoolUsageGet(dev_tgt.dev_id,
                                                ppg_hdl,
                                                pool_id,
                                                &pool_max_cells,
                                                &dynamic_baf,
                                                &hysteresis_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get PG app pool settings for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
    try {
      dynamic_baf_str = ppgbaf_to_str.at(dynamic_baf);
      pool_id_str = ig_pool_to_str.at(pool_id);
    } catch (std::out_of_range &) {
      LOG_ERROR("%s:%d Unknown value from the driver", __func__, __LINE__);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    f_block_done = true;
  }

  if ((f_hysteresis_cells && !(for_update)) ||
      (f_block_need && !(f_block_done))) {
    // 'hysteresis_cells' is requested along for read or its value is needed
    // to complement all the other values in the block for update.
    status = trafficMgr->bfTMPPGGuaranteedMinSkidHysteresisGet(
        dev_tgt.dev_id, ppg_hdl, &hysteresis_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get hysteresis cells for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  }

  return status;
}

bf_status_t BfRtTMPpgBufferHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  uint64_t cells;

  do {
    if (f_pool_id) {
      status = p_data.getValue(f_pool_id, &pool_id_str);
      if (BF_SUCCESS != status) {
        break;
      }
      try {
        pool_id = str_to_ig_pool.at(pool_id_str);
      } catch (std::out_of_range &) {
        LOG_ERROR("%s:%d pool_id value is out of range", __func__, __LINE__);
        status = BF_INVALID_ARG;
        break;
      }
    }
    if (f_dynamic_baf) {
      status = p_data.getValue(f_dynamic_baf, &dynamic_baf_str);
      if (BF_SUCCESS != status) {
        break;
      }
      try {
        dynamic_baf = str_to_ppgbaf.at(dynamic_baf_str);
      } catch (std::out_of_range &) {
        LOG_ERROR("%s:%d dynamic_baf is out of range", __func__, __LINE__);
        status = BF_INVALID_ARG;
        break;
      }
    }
    if (f_pool_max_cells) {
      status = p_data.getValue(f_pool_max_cells, &cells);
      if (BF_SUCCESS != status) {
        break;
      }
      pool_max_cells = static_cast<uint32_t>(cells);
    }
    if (f_hysteresis_cells) {
      status = p_data.getValue(f_hysteresis_cells, &cells);
      if (BF_SUCCESS != status) {
        break;
      }
      hysteresis_cells = static_cast<uint32_t>(cells);
    }
    if (f_guaranteed_cells) {
      status = p_data.getValue(f_guaranteed_cells, &cells);
      if (BF_SUCCESS != status) {
        break;
      }
      guaranteed_cells = static_cast<uint32_t>(cells);
    }

  } while (0);  // Get fields' values

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't get values from fields",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str());
  }

  return status;
}

bf_status_t BfRtTMPpgBufferHelper::writeValues(const bf_rt_target_t &dev_tgt,
                                               const BfRtTMTable &w_table,
                                               bf_tm_ppg_hdl ppg_hdl) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();
  bf_status_t status = BF_SUCCESS;

  if (f_pool_id || f_pool_max_cells || f_dynamic_baf) {
    // (!) All these values and hysteresis_cells must have been ready.
    LOG_DBG("%s:%d %s Write block fields for ppg_hdl=0x%x",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str(),
            ppg_hdl);
    status = trafficMgr->bfTMPPGAppPoolUsageSet(dev_tgt.dev_id,
                                                ppg_hdl,
                                                pool_id,
                                                pool_max_cells,
                                                dynamic_baf,
                                                hysteresis_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't set PG app pool settings of ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  } else if (f_hysteresis_cells) {
    // 'hysteresis_cells' is requested along.
    status = trafficMgr->bfTMPPGGuaranteedMinSkidHysteresisSet(
        dev_tgt.dev_id, ppg_hdl, hysteresis_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't set hysteresis cells of ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  }

  if (f_guaranteed_cells) {
    status = trafficMgr->bfTMPPGGuaranteedMinLimitSet(
        dev_tgt.dev_id, ppg_hdl, guaranteed_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't set guaranteed cells of ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
    }
  }

  return status;
}

//---
bf_status_t BfRtTMPpgBufferHelper::getFieldsBuffer(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTable &w_table,
    bf_tm_ppg_hdl ppg_hdl,
    std::set<bf_rt_id_t> &wrk_fields,
    BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto status = this->initFieldIds(w_table, wrk_fields);
  if (BF_SUCCESS != status) {
    return status;
  }

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (f_guaranteed_cells) {
    // Do not read current single value before its update where its new
    // value is given.
    status = trafficMgr->bfTMPPGuaranteedMinLimitGet(
        dev_tgt.dev_id, ppg_hdl, &guaranteed_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get guaranteed cells for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  }

  // Read the current block of related values.
  status = this->readValues(dev_tgt, w_table, ppg_hdl, false);
  if (BF_SUCCESS != status) {
    return status;
  }

  status = this->setFieldValues(w_table, p_data);

  return status;
}

//---
bf_status_t BfRtTMPpgBufferHelper::setFieldValues(const BfRtTMTable &w_table,
                                                  BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;

  do {
    if (f_pool_id) {
      status = p_data->setValue(f_pool_id, pool_id_str);
      if (BF_SUCCESS != status) {
        break;
      }
    }
    if (f_dynamic_baf) {
      status = p_data->setValue(f_dynamic_baf, dynamic_baf_str);
      if (BF_SUCCESS != status) {
        break;
      }
    }
    if (f_pool_max_cells) {
      status = p_data->setValue(f_pool_max_cells,
                                static_cast<uint64_t>(pool_max_cells));
      if (BF_SUCCESS != status) {
        break;
      }
    }
    if (f_hysteresis_cells) {
      status = p_data->setValue(f_hysteresis_cells,
                                static_cast<uint64_t>(hysteresis_cells));
      if (BF_SUCCESS != status) {
        break;
      }
    }
    if (f_guaranteed_cells) {
      status = p_data->setValue(f_guaranteed_cells,
                                static_cast<uint64_t>(guaranteed_cells));
    }

  } while (0);  // Set fields' values

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s Can't assign values for fields",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str());
  }

  return status;
}

bf_status_t BfRtTMPpgBufferHelper::resetFieldsBuffer(
    const bf_rt_target_t &dev_tgt,
    const BfRtTMTable &w_table,
    bf_tm_ppg_hdl ppg_hdl,
    std::set<bf_rt_id_t> &wrk_fields,
    BfRtTMTableData *p_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == p_data) {
    return BF_INVALID_ARG;
  }

  auto status = this->initFieldIds(w_table, wrk_fields);
  if (BF_SUCCESS != status) {
    return status;
  }

  //--- Get data from HW depending on what set of fields is requested.

  dynamic_baf_str = "";
  pool_id_str = "";
  pool_id = BF_TM_IG_APP_POOL_0;
  pool_max_cells = 0;
  dynamic_baf = BF_TM_PPG_BAF_80_PERCENT;
  guaranteed_cells = 0;
  hysteresis_cells = 0;

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  if (f_pool_id || f_pool_max_cells || f_dynamic_baf) {
    LOG_DBG("%s:%d %s get shared buffer defaults",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str());
    status = trafficMgr->bfTMPPGAppPoolUsageGetDefault(
        dev_tgt.dev_id, ppg_hdl, &pool_id, &pool_max_cells, &dynamic_baf);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get PG app pool defaults for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }

    try {
      dynamic_baf_str = ppgbaf_to_str.at(dynamic_baf);
      pool_id_str = ig_pool_to_str.at(pool_id);
    } catch (std::out_of_range &) {
      LOG_ERROR("%s:%d unknown values from the driver", __func__, __LINE__);
      BF_RT_DBGCHK(0);
      return BF_UNEXPECTED;
    }
  }

  if (f_hysteresis_cells || f_guaranteed_cells) {
    LOG_DBG("%s:%d %s get static buffer defaults",
            __func__,
            __LINE__,
            w_table.table_name_get().c_str());
    status = trafficMgr->bfTMPPGBufferGetDefault(
        dev_tgt.dev_id, ppg_hdl, &guaranteed_cells, &hysteresis_cells);
    if (BF_SUCCESS != status) {
      LOG_ERROR("%s:%d %s Can't get buffer defaults for ppg_hdl=0x%x",
                __func__,
                __LINE__,
                w_table.table_name_get().c_str(),
                ppg_hdl);
      return status;
    }
  }

  return this->setFieldValues(w_table, p_data);
}
}  // namespace bfrt
