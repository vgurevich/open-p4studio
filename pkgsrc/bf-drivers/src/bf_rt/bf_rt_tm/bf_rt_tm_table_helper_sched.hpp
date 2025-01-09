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


#ifndef _BF_RT_TM_TABLE_HELPER_SCHED_HPP
#define _BF_RT_TM_TABLE_HELPER_SCHED_HPP

#include "bf_rt_tm_table_helper.hpp"
#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

class BfRtTMPortSchedCfgHelper {
 public:
  BfRtTMPortSchedCfgHelper() {
    this->clearValues();
    this->clearIds();
  }

  virtual ~BfRtTMPortSchedCfgHelper() = default;

  bf_status_t initFieldIds(const BfRtTMTable &w_table,
                           std::set<bf_rt_id_t> &wrk_fields);

  bf_status_t getFieldValues(const BfRtTMTable &w_table,
                             const BfRtTMTableData &s_data);

  bf_status_t setFieldValues(const BfRtTMTable &w_table,
                             BfRtTMTableData *s_data);

  void clearIds() {
    f_scheduling_speed = 0;
    f_max_rate_enable = 0;

    f_l1_nodes_count = 0;
    f_l1_nodes = 0;
  }

  void clearValues() {
    scheduling_speed = BF_SPEED_NONE;
    max_rate_enable = true;

    l1_nodes_count = 0;
    l1_nodes.clear();
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_scheduling_speed;
  bf_rt_id_t f_max_rate_enable;

  // L1 Node associations
  bf_rt_id_t f_l1_nodes_count;
  bf_rt_id_t f_l1_nodes;

  uint8_t l1_nodes_count;
  std::vector<bf_rt_id_t> l1_nodes;

  // Data Field values
  bf_port_speed_t scheduling_speed;
  bool max_rate_enable;

  // Maps to convert between string and enum types
  static const std::map<std::string, bf_port_speed_t> str_to_pspeed;
  static const std::map<bf_port_speed_t, std::string> pspeed_to_str;
};

//-------------
class BfRtTMSchedCfgHelper {
 public:
  BfRtTMSchedCfgHelper() {
    BfRtTMSchedCfgHelper::clearIds();
    BfRtTMSchedCfgHelper::clearValues();
  }

  virtual ~BfRtTMSchedCfgHelper() = default;

  virtual bf_status_t initFields(bf_rt_id_t do_action_id,
                                 const BfRtTMTable &w_table,
                                 std::set<bf_rt_id_t> &wrk_fields);

  virtual bf_status_t getFieldValues(const BfRtTMTable &w_table,
                                     const BfRtTMTableData &s_data);

  virtual bf_status_t setFieldValues(const BfRtTMTable &w_table,
                                     BfRtTMTableData *s_data);

 protected:
  void clearIds() {
    f_scheduling_enable = 0;
    f_min_rate_enable = 0;
    f_min_priority = 0;
    f_max_rate_enable = 0;
    f_max_priority = 0;
    f_dwrr_weight = 0;
  }

  void clearValues() {
    scheduling_enable = false;

    min_rate_enable = false;
    min_priority = BF_TM_SCH_PRIO_LOW;
    max_rate_enable = false;
    max_priority = BF_TM_SCH_PRIO_LOW;
    dwrr_weight = 0;
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_scheduling_enable;
  bf_rt_id_t f_min_rate_enable;
  bf_rt_id_t f_min_priority;
  bf_rt_id_t f_max_rate_enable;
  bf_rt_id_t f_max_priority;
  bf_rt_id_t f_dwrr_weight;

  // Data Field values
  bool scheduling_enable;

  bool min_rate_enable;
  bf_tm_sched_prio_t min_priority;

  bool max_rate_enable;
  bf_tm_sched_prio_t max_priority;

  uint16_t dwrr_weight;

  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_sched_prio_t> str_to_prio;
  static const std::map<bf_tm_sched_prio_t, std::string> prio_to_str;
};

class BfRtTMQueueSchedCfgHelper : public BfRtTMSchedCfgHelper {
 public:
  BfRtTMQueueSchedCfgHelper() : BfRtTMSchedCfgHelper() {
    BfRtTMQueueSchedCfgHelper::clearIds();
    BfRtTMQueueSchedCfgHelper::clearValues();
  }

  virtual ~BfRtTMQueueSchedCfgHelper() = default;

  virtual bf_status_t initFields(const BfRtTMTable &w_table,
                                 std::set<bf_rt_id_t> &wrk_fields);

  virtual bf_status_t getFieldValues(const BfRtTMTable &w_table,
                                     const BfRtTMTableData &s_data) override;

  virtual bf_status_t setFieldValues(const BfRtTMTable &w_table,
                                     BfRtTMTableData *s_data) override;

 protected:
  void clearIds() {
    f_scheduling_speed = 0;
    f_pg_l1_node = 0;
    f_advanced_flow_control = 0;
  }

  void clearValues() {
    scheduling_speed = BF_SPEED_NONE;
    pg_l1_node = 0;
    advanced_flow_control = BF_TM_SCH_ADV_FC_MODE_CRE;
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_scheduling_speed;
  bf_rt_id_t f_pg_l1_node;
  bf_rt_id_t f_advanced_flow_control;

  // Data Field values
  bf_port_speed_t scheduling_speed;
  bf_tm_l1_node_t pg_l1_node;
  bf_tm_sched_adv_fc_mode_t advanced_flow_control;

  // Maps to convert between string and enum types
  static const std::map<std::string, bf_port_speed_t> str_to_qspeed;
  static const std::map<bf_port_speed_t, std::string> qspeed_to_str;

  static const std::map<std::string, bf_tm_sched_adv_fc_mode_t> str_to_afc;
  static const std::map<bf_tm_sched_adv_fc_mode_t, std::string> afc_to_str;
};

class BfRtTML1NodeSchedCfgHelper : public BfRtTMSchedCfgHelper {
 public:
  BfRtTML1NodeSchedCfgHelper() : BfRtTMSchedCfgHelper() {
    BfRtTML1NodeSchedCfgHelper::clearActions();
    BfRtTML1NodeSchedCfgHelper::clearIds();
    BfRtTML1NodeSchedCfgHelper::clearValues();
  }

  virtual ~BfRtTML1NodeSchedCfgHelper() = default;

  bf_status_t initActions(const BfRtTMTable &w_table);

  virtual bf_status_t initFields(bf_rt_id_t do_action_id,
                                 const BfRtTMTable &w_table,
                                 std::set<bf_rt_id_t> &wrk_fields) override;

  virtual bf_status_t getFieldValues(const BfRtTMTable &w_table,
                                     const BfRtTMTableData &s_data) override;

  virtual bf_status_t setFieldValues(const BfRtTMTable &w_table,
                                     BfRtTMTableData *s_data) override;

 protected:
  void clearActions() {
    a_in_use = 0;
    a_free = 0;

    action_id = 0;
  }

  void clearIds() {
    f_dev_port = 0;
    f_queues_count = 0;
    f_pg_queues = 0;
    f_priority_propagation = 0;
  }

  void clearValues() {
    in_use = false;
    dev_port = 0;

    queues_count = 0;
    pg_queues.clear();

    priority_propagation = false;
  }

 public:
  // Data Field id's; zero if not needed.
  bf_rt_id_t f_dev_port;
  bf_rt_id_t f_queues_count;
  bf_rt_id_t f_pg_queues;
  bf_rt_id_t f_priority_propagation;

  // Actions
  bf_rt_id_t a_in_use;
  bf_rt_id_t a_free;
  bf_rt_id_t action_id;

  // Data Field values
  bool in_use;
  bf_dev_port_t dev_port;

  uint8_t queues_count;
  std::vector<bf_rt_id_t> pg_queues;

  bool priority_propagation;
};

class BfRtTMSchedShapingHelper {
 public:
  BfRtTMSchedShapingHelper() {
    this->clearIds();
    this->clearValues();
  }

  virtual ~BfRtTMSchedShapingHelper() = default;

  bf_status_t initFields(const BfRtTMTable &w_table,
                         std::set<bf_rt_id_t> &wrk_fields);

  bf_status_t getFieldValues(const BfRtTMTable &w_table,
                             const BfRtTMTableData &s_data);

  bf_status_t setFieldValues(const BfRtTMTable &w_table,
                             BfRtTMTableData *s_data);

  void clearIds() {
    f_provisioning = 0;
    f_unit = 0;
    f_max_rate = 0;
    f_max_burst_size = 0;
    f_min_rate = 0;
    f_min_burst_size = 0;
  }

  void clearValues() {
    provisioning = BF_TM_SCH_RATE_UPPER;
    pps = false;
    max_burst_size = 0;
    max_rate = 0;
    min_burst_size = 0;
    min_rate = 0;
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_provisioning;
  bf_rt_id_t f_unit;
  bf_rt_id_t f_max_rate;
  bf_rt_id_t f_max_burst_size;
  bf_rt_id_t f_min_rate;
  bf_rt_id_t f_min_burst_size;

  // Data Field values
  bf_tm_sched_shaper_prov_type_t provisioning;
  bool pps;
  uint32_t min_burst_size;
  uint32_t min_rate;
  uint32_t max_burst_size;
  uint32_t max_rate;

  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_sched_shaper_prov_type_t>
      str_to_provtype;
  static const std::map<bf_tm_sched_shaper_prov_type_t, std::string>
      provtype_to_str;
};

}  // namespace bfrt
#endif
