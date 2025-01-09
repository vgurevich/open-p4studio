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


#include "bf_rt_tm_table_helper_sched.hpp"
#include "bf_rt_tm_table_impl_port.hpp"
#include "bf_rt_tm_table_impl_queue.hpp"

namespace bfrt {

//----------- TM_PORT_SCHED_CFG_HELPER

const std::map<std::string, bf_port_speed_t>
    BfRtTMPortSchedCfgHelper::str_to_pspeed{
        {"BF_SPEED_NONE", BF_SPEED_NONE},
        {"BF_SPEED_1G", BF_SPEED_1G},
        {"BF_SPEED_10G", BF_SPEED_10G},
        {"BF_SPEED_25G", BF_SPEED_25G},
        {"BF_SPEED_40G", BF_SPEED_40G},
        {"BF_SPEED_40G_R2", BF_SPEED_40G_R2},
        {"BF_SPEED_50G", BF_SPEED_50G},
        {"BF_SPEED_50G_CONS", BF_SPEED_50G_CONS},
        {"BF_SPEED_100G", BF_SPEED_100G},
        {"BF_SPEED_200G", BF_SPEED_200G},
        {"BF_SPEED_400G", BF_SPEED_400G},
    };

const std::map<bf_port_speed_t, std::string>
    BfRtTMPortSchedCfgHelper::pspeed_to_str{
        {BF_SPEED_NONE, "BF_SPEED_NONE"},
        {BF_SPEED_1G, "BF_SPEED_1G"},
        {BF_SPEED_10G, "BF_SPEED_10G"},
        {BF_SPEED_25G, "BF_SPEED_25G"},
        {BF_SPEED_40G, "BF_SPEED_40G"},
        {BF_SPEED_40G_R2, "BF_SPEED_40G_R2"},
        {BF_SPEED_50G, "BF_SPEED_50G"},
        {BF_SPEED_50G_CONS, "BF_SPEED_50G_CONS"},
        {BF_SPEED_100G, "BF_SPEED_100G"},
        {BF_SPEED_200G, "BF_SPEED_200G"},
        {BF_SPEED_400G, "BF_SPEED_400G"},
    };

bf_status_t BfRtTMPortSchedCfgHelper::initFieldIds(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;

  bf_rt_id_t do_action_id = 0;
  std::string field_name;

  f_scheduling_speed = 0;
  f_max_rate_enable = 0;

  f_l1_nodes = 0;
  f_l1_nodes_count = 0;

  do {
    BFRT_TM_HLP_FIELD(scheduling_speed);
    BFRT_TM_HLP_FIELD(max_rate_enable);

    BFRT_TM_HLP_FIELD_OPTIONAL(l1_nodes_count);  // TF2 only
    BFRT_TM_HLP_FIELD_OPTIONAL(l1_nodes);        // TF2 only

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

bf_status_t BfRtTMPortSchedCfgHelper::setFieldValues(const BfRtTMTable &w_table,
                                                     BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_ENCODED(scheduling_speed, pspeed_to_str);
    BFRT_TM_HLP_SET_VALUE(max_rate_enable);

    BFRT_TM_HLP_SET_INT(l1_nodes_count);
    BFRT_TM_HLP_SET_VALUE(l1_nodes);

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

bf_status_t BfRtTMPortSchedCfgHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_ENCODED(scheduling_speed, str_to_pspeed);
    BFRT_TM_HLP_GET_VALUE(max_rate_enable);

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

//----------- TM_SCHED_SHAPING_HELPER
const std::map<std::string, bf_tm_sched_shaper_prov_type_t>
    BfRtTMSchedShapingHelper::str_to_provtype{
        {"UPPER", BF_TM_SCH_RATE_UPPER},
        {"LOWER", BF_TM_SCH_RATE_LOWER},
        {"MIN_ERROR", BF_TM_SCH_RATE_MIN_ERROR},
    };

const std::map<bf_tm_sched_shaper_prov_type_t, std::string>
    BfRtTMSchedShapingHelper::provtype_to_str{
        {BF_TM_SCH_RATE_UPPER, "UPPER"},
        {BF_TM_SCH_RATE_LOWER, "LOWER"},
        {BF_TM_SCH_RATE_MIN_ERROR, "MIN_ERROR"},
    };

bf_status_t BfRtTMSchedShapingHelper::initFields(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  BfRtTable::TableType table_type = BfRtTable::TableType::INVALID;
  bf_status_t status = BF_SUCCESS;

  bf_rt_id_t do_action_id = 0;
  std::string field_name;

  this->clearIds();
  this->clearValues();

  do {
    status = w_table.tableTypeGet(&table_type);
    if (BF_SUCCESS != status) {
      break;
    }

    if (BfRtTable::TableType::TM_L1_NODE_SCHED_SHAPING != table_type) {
      BFRT_TM_HLP_FIELD(provisioning);
    }

    BFRT_TM_HLP_FIELD(unit);
    BFRT_TM_HLP_FIELD(max_rate);
    BFRT_TM_HLP_FIELD(max_burst_size);

    if (BfRtTable::TableType::TM_PORT_SCHED_SHAPING != table_type) {
      BFRT_TM_HLP_FIELD_OPTIONAL(min_rate);
      BFRT_TM_HLP_FIELD_OPTIONAL(min_burst_size);
    }

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

bf_status_t BfRtTMSchedShapingHelper::setFieldValues(const BfRtTMTable &w_table,
                                                     BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_INT(max_rate);
    BFRT_TM_HLP_SET_INT(max_burst_size);
    BFRT_TM_HLP_SET_INT(min_rate);
    BFRT_TM_HLP_SET_INT(min_burst_size);

    BFRT_TM_HLP_SET_ENCODED(provisioning, provtype_to_str);

    if (this->f_unit) {
      std::string field_str((this->pps) ? "PPS" : "BPS");
      status = s_data->setValue(this->f_unit, field_str);
      if (BF_SUCCESS != status) {
        err_data_id = this->f_unit;
        break;
      }
    }

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

bf_status_t BfRtTMSchedShapingHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_INT(max_rate, uint32_t);
    BFRT_TM_HLP_GET_INT(max_burst_size, uint32_t);
    BFRT_TM_HLP_GET_INT(min_rate, uint32_t);
    BFRT_TM_HLP_GET_INT(min_burst_size, uint32_t);

    BFRT_TM_HLP_GET_ENCODED(provisioning, str_to_provtype);

    if (this->f_unit) {
      std::string field_str;
      status = s_data.getValue(this->f_unit, &field_str);
      if (BF_SUCCESS == status) {
        if (field_str == "PPS") {
          this->pps = true;
        } else if (field_str == "BPS") {
          this->pps = false;
        } else {
          LOG_ERROR("%s:%d %s wrong value: unit='%s'",
                    __func__,
                    __LINE__,
                    w_table.table_name_get().c_str(),
                    field_str.c_str());
          status = BF_INVALID_ARG;
          err_data_id = this->f_unit;
          break;
        }
      }
    }

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

//----------- TM_SCHED_CFG_HELPER
const std::map<std::string, bf_tm_sched_prio_t>
    BfRtTMSchedCfgHelper::str_to_prio{
        {"LOW", BF_TM_SCH_PRIO_LOW},
        {"0", BF_TM_SCH_PRIO_0},
        {"1", BF_TM_SCH_PRIO_1},
        {"2", BF_TM_SCH_PRIO_2},
        {"3", BF_TM_SCH_PRIO_3},
        {"4", BF_TM_SCH_PRIO_4},
        {"5", BF_TM_SCH_PRIO_5},
        {"6", BF_TM_SCH_PRIO_6},
        {"7", BF_TM_SCH_PRIO_7},
        {"HIGH", BF_TM_SCH_PRIO_HIGH},
    };

const std::map<bf_tm_sched_prio_t, std::string>
    BfRtTMSchedCfgHelper::prio_to_str{
        {BF_TM_SCH_PRIO_LOW, "LOW"},
        {BF_TM_SCH_PRIO_0, "0"},
        {BF_TM_SCH_PRIO_1, "1"},
        {BF_TM_SCH_PRIO_2, "2"},
        {BF_TM_SCH_PRIO_3, "3"},
        {BF_TM_SCH_PRIO_4, "4"},
        {BF_TM_SCH_PRIO_5, "5"},
        {BF_TM_SCH_PRIO_6, "6"},
        {BF_TM_SCH_PRIO_7, "7"},
        {BF_TM_SCH_PRIO_HIGH, "HIGH"},
    };

bf_status_t BfRtTMSchedCfgHelper::initFields(bf_rt_id_t do_action_id,
                                             const BfRtTMTable &w_table,
                                             std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  std::string field_name;

  BfRtTMSchedCfgHelper::clearIds();
  BfRtTMSchedCfgHelper::clearValues();

  do {
    BFRT_TM_HLP_FIELD_OPTIONAL(scheduling_enable);

    BFRT_TM_HLP_FIELD(min_rate_enable);
    BFRT_TM_HLP_FIELD(min_priority);

    BFRT_TM_HLP_FIELD(max_rate_enable);
    BFRT_TM_HLP_FIELD(max_priority);

    BFRT_TM_HLP_FIELD(dwrr_weight);
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTMSchedCfgHelper::setFieldValues(const BfRtTMTable &w_table,
                                                 BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_VALUE(scheduling_enable);

    BFRT_TM_HLP_SET_VALUE(max_rate_enable);
    BFRT_TM_HLP_SET_ENCODED(max_priority, prio_to_str);

    BFRT_TM_HLP_SET_VALUE(min_rate_enable);
    BFRT_TM_HLP_SET_ENCODED(min_priority, prio_to_str);

    BFRT_TM_HLP_SET_INT(dwrr_weight);

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

bf_status_t BfRtTMSchedCfgHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_VALUE(scheduling_enable);

    BFRT_TM_HLP_GET_VALUE(max_rate_enable);
    BFRT_TM_HLP_GET_ENCODED(max_priority, str_to_prio);

    BFRT_TM_HLP_GET_VALUE(min_rate_enable);
    BFRT_TM_HLP_GET_ENCODED(min_priority, str_to_prio);

    BFRT_TM_HLP_GET_INT(dwrr_weight, uint16_t);

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

//----------- TM_QUEUE_SCHED_CFG_HELPER
const std::map<std::string, bf_port_speed_t>
    BfRtTMQueueSchedCfgHelper::str_to_qspeed{
        {"BF_SPEED_NONE", BF_SPEED_NONE},
        {"BF_SPEED_50G", BF_SPEED_50G},
        {"BF_SPEED_100G", BF_SPEED_100G},
        {"BF_SPEED_200G", BF_SPEED_200G},
        {"BF_SPEED_400G", BF_SPEED_400G},
    };

const std::map<bf_port_speed_t, std::string>
    BfRtTMQueueSchedCfgHelper::qspeed_to_str{
        {BF_SPEED_NONE, "BF_SPEED_NONE"},
        {BF_SPEED_50G, "BF_SPEED_50G"},
        {BF_SPEED_100G, "BF_SPEED_100G"},
        {BF_SPEED_200G, "BF_SPEED_200G"},
        {BF_SPEED_400G, "BF_SPEED_400G"},
    };

const std::map<std::string, bf_tm_sched_adv_fc_mode_t>
    BfRtTMQueueSchedCfgHelper::str_to_afc{{"CREDIT", BF_TM_SCH_ADV_FC_MODE_CRE},
                                          {"XOFF", BF_TM_SCH_ADV_FC_MODE_XOFF}};

const std::map<bf_tm_sched_adv_fc_mode_t, std::string>
    BfRtTMQueueSchedCfgHelper::afc_to_str{{BF_TM_SCH_ADV_FC_MODE_CRE, "CREDIT"},
                                          {BF_TM_SCH_ADV_FC_MODE_XOFF, "XOFF"}};

bf_status_t BfRtTMQueueSchedCfgHelper::initFields(
    const BfRtTMTable &w_table, std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_rt_id_t do_action_id = 0;

  bf_status_t status =
      BfRtTMSchedCfgHelper::initFields(do_action_id, w_table, wrk_fields);
  if (BF_SUCCESS != status) {
    return status;
  }

  std::string field_name;

  BfRtTMQueueSchedCfgHelper::clearIds();
  BfRtTMQueueSchedCfgHelper::clearValues();

  do {
    BFRT_TM_HLP_FIELD_OPTIONAL(scheduling_speed);       // TF2
    BFRT_TM_HLP_FIELD_OPTIONAL(advanced_flow_control);  // TF2
    BFRT_TM_HLP_FIELD_OPTIONAL(pg_l1_node);             // TF2

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTMQueueSchedCfgHelper::setFieldValues(
    const BfRtTMTable &w_table, BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BfRtTMSchedCfgHelper::setFieldValues(w_table, s_data);
  if (BF_SUCCESS != status) {
    return status;
  }

  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_ENCODED(scheduling_speed, qspeed_to_str);    // TF2
    BFRT_TM_HLP_SET_ENCODED(advanced_flow_control, afc_to_str);  // TF2
    BFRT_TM_HLP_SET_INT(pg_l1_node);                             // TF2

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

bf_status_t BfRtTMQueueSchedCfgHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BfRtTMSchedCfgHelper::getFieldValues(w_table, s_data);
  if (BF_SUCCESS != status) {
    return status;
  }

  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_ENCODED(scheduling_speed, str_to_qspeed);    // TF2
    BFRT_TM_HLP_GET_ENCODED(advanced_flow_control, str_to_afc);  // TF2
    BFRT_TM_HLP_GET_INT(pg_l1_node, uint8_t);                    // TF2

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

//----------- TM_L1_NODE_SCHED_CFG_HELPER
bf_status_t BfRtTML1NodeSchedCfgHelper::initActions(
    const BfRtTMTable &w_table) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BF_SUCCESS;
  std::string action_name;

  this->clearActions();

  do {
    BFRT_TM_HLP_ACTION(free);
    BFRT_TM_HLP_ACTION(in_use);

  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              action_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTML1NodeSchedCfgHelper::initFields(
    bf_rt_id_t do_action_id,
    const BfRtTMTable &w_table,
    std::set<bf_rt_id_t> &wrk_fields) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status =
      BfRtTMSchedCfgHelper::initFields(do_action_id, w_table, wrk_fields);
  if (BF_SUCCESS != status) {
    return status;
  }

  std::string field_name;

  BfRtTML1NodeSchedCfgHelper::clearIds();
  BfRtTML1NodeSchedCfgHelper::clearValues();

  this->action_id = do_action_id;

  do {
    BFRT_TM_HLP_FIELD(dev_port);

    BFRT_TM_HLP_FIELD(queues_count);
    BFRT_TM_HLP_FIELD(pg_queues);

    BFRT_TM_HLP_FIELD(priority_propagation);
  } while (0);

  if (BF_SUCCESS != status) {
    LOG_ERROR("%s:%d %s '%s' failed, rc=%d",
              __func__,
              __LINE__,
              w_table.table_name_get().c_str(),
              field_name.c_str(),
              status);
  }
  return status;
}

bf_status_t BfRtTML1NodeSchedCfgHelper::setFieldValues(
    const BfRtTMTable &w_table, BfRtTMTableData *s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  if (nullptr == s_data) {
    return BF_INVALID_ARG;
  }

  bf_status_t status = BfRtTMSchedCfgHelper::setFieldValues(w_table, s_data);
  if (BF_SUCCESS != status) {
    return status;
  }

  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_SET_INT(dev_port);

    BFRT_TM_HLP_SET_INT(queues_count);
    BFRT_TM_HLP_SET_VALUE(pg_queues);

    BFRT_TM_HLP_SET_VALUE(priority_propagation);

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

bf_status_t BfRtTML1NodeSchedCfgHelper::getFieldValues(
    const BfRtTMTable &w_table, const BfRtTMTableData &s_data) {
  LOG_DBG("%s:%d %s", __func__, __LINE__, w_table.table_name_get().c_str());

  bf_status_t status = BfRtTMSchedCfgHelper::getFieldValues(w_table, s_data);
  if (BF_SUCCESS != status) {
    return status;
  }

  bf_rt_id_t err_data_id = 0;

  do {
    BFRT_TM_HLP_GET_INT(dev_port, uint32_t);

    BFRT_TM_HLP_GET_VALUE(priority_propagation);

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

}  // namespace bfrt
