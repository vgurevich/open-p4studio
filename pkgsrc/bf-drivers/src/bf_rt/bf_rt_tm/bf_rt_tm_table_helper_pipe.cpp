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


#include "bf_rt_tm_table_helper_pipe.hpp"
#include "bf_rt_tm_table_impl_pipe.hpp"

namespace bfrt {

//----------- TM_PIPE_CFG_HELPER

const std::map<std::string, bool> BfRtTMPipeCfgHelper::str_to_qstat_mode{
    {"COLOR", false},
    {"ANY", true},
};

const std::map<bool, std::string> BfRtTMPipeCfgHelper::qstat_mode_to_str{
    {false, "COLOR"},
    {true, "ANY"},
};

bf_status_t BfRtTMPipeCfgHelper::completeMirrorDropFields(
    const bf_rt_target_t &dev_tgt, const BfRtTMTable &w_table) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  auto *trafficMgr = TrafficMgrIntf::getInstance();

  // The (pg_id, pg_queue) is a non-volatile Queue key.

  // This service function assumes that
  // mirror_drop_dev_port and mirror_drop_queue_nr are set and
  // it should get other values: mirror_drop_pg_id and mirror_drop_pg_queue.

  uint8_t pg_base_queue = 0;
  bool is_mapped = false;

  auto status = trafficMgr->bfTMPortBaseQueueGet(dev_tgt.dev_id,
                                                 this->mirror_drop_dev_port,
                                                 &(this->mirror_drop_pg_id),
                                                 &pg_base_queue,
                                                 &is_mapped);
  // The base queue should be mapped if there are any queues mapped.
  if (BF_SUCCESS != status || !is_mapped) {
    LOG_ERROR(
        "%s:%d %s Can't get base queue for dev_id=%d port_id=%d - %s mapped, "
        "rc=%d(%s)",
        __func__,
        __LINE__,
        w_table.table_name_get().c_str(),
        dev_tgt.dev_id,
        this->mirror_drop_dev_port,
        (is_mapped) ? "is" : "not",
        status,
        bf_err_str(status));
  } else {
    this->mirror_drop_pg_queue = pg_base_queue + this->mirror_drop_queue_nr;
  }

  return status;
}

bf_status_t BfRtTMPipeCfgHelper::initFieldIds(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  bf_rt_id_t do_action_id = 0;
  std::string field_name;

  this->clearIds();
  this->clearValues();

  do {
    BFRT_TM_HLP_FIELD_OPTIONAL(mirror_drop_enable)
    BFRT_TM_HLP_FIELD_OPTIONAL(mirror_drop_dev_port)
    BFRT_TM_HLP_FIELD_OPTIONAL(mirror_drop_queue_nr)
    BFRT_TM_HLP_FIELD_OPTIONAL(mirror_drop_pg_id)
    BFRT_TM_HLP_FIELD_OPTIONAL(mirror_drop_pg_queue)

    BFRT_TM_HLP_FIELD(eg_limit_cells)
    BFRT_TM_HLP_FIELD(eg_hysteresis_cells)

    BFRT_TM_HLP_FIELD_OPTIONAL(queue_depth_report_mode)
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s field '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTMPipeCfgHelper::getFieldValues(const BfRtTMTable &w_table,
                                                const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_VALUE(mirror_drop_enable)

    BFRT_TM_HLP_GET_INT(mirror_drop_dev_port, uint32_t)
    BFRT_TM_HLP_GET_INT(mirror_drop_queue_nr, uint8_t)

    BFRT_TM_HLP_GET_INT(mirror_drop_pg_id, uint8_t)
    BFRT_TM_HLP_GET_INT(mirror_drop_pg_queue, uint8_t)

    BFRT_TM_HLP_GET_INT(eg_limit_cells, uint32_t)
    BFRT_TM_HLP_GET_INT(eg_hysteresis_cells, uint32_t)

    BFRT_TM_HLP_GET_ENCODED(queue_depth_report_mode, str_to_qstat_mode);
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s get value field_id=%d failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              err_data_id,
              status);
  }
  return status;
}

bf_status_t BfRtTMPipeCfgHelper::setFieldValues(const BfRtTMTable &w_table,
                                                BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_VALUE(mirror_drop_enable)

    BFRT_TM_HLP_SET_INT(mirror_drop_dev_port)
    BFRT_TM_HLP_SET_INT(mirror_drop_queue_nr)

    BFRT_TM_HLP_SET_INT(mirror_drop_pg_id)
    BFRT_TM_HLP_SET_INT(mirror_drop_pg_queue)

    BFRT_TM_HLP_SET_INT(eg_limit_cells)
    BFRT_TM_HLP_SET_INT(eg_hysteresis_cells)

    BFRT_TM_HLP_SET_ENCODED(queue_depth_report_mode, qstat_mode_to_str);
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s set value field_id=%d failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              err_data_id,
              status);
  }
  return status;
}

//----------- TM_PIPE_SCHED_CFG_HELPER

bf_status_t BfRtTMPipeSchedCfgHelper::initFieldIds(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  bf_rt_id_t do_action_id = 0;
  std::string field_name;

  this->clearIds();
  this->clearValues();

  do {
    BFRT_TM_HLP_FIELD(packet_ifg_compensation)
    BFRT_TM_HLP_FIELD_OPTIONAL(advanced_flow_control_enable)
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s field '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTMPipeSchedCfgHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_INT(packet_ifg_compensation, uint8_t)
    BFRT_TM_HLP_GET_VALUE(advanced_flow_control_enable)
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s get field_id=%d value failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              err_data_id,
              status);
  }
  return status;
}

bf_status_t BfRtTMPipeSchedCfgHelper::setFieldValues(const BfRtTMTable &w_table,
                                                     BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_INT(packet_ifg_compensation)
    BFRT_TM_HLP_SET_VALUE(advanced_flow_control_enable)
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s set field_id=%d value failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              err_data_id,
              status);
  }

  return status;
}

}  // namespace bfrt
