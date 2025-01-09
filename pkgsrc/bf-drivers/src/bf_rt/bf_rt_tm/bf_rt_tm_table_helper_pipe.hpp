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


#ifndef _BF_RT_TM_TABLE_HELPER_PIPE_HPP
#define _BF_RT_TM_TABLE_HELPER_PIPE_HPP

#include "bf_rt_tm_table_helper.hpp"
#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

// TM Pipe common field blocks.

class BfRtTMPipeCfgHelper {
 public:
  BfRtTMPipeCfgHelper() {
    this->clearValues();
    this->clearIds();
  }

  virtual ~BfRtTMPipeCfgHelper() = default;

  bf_status_t completeMirrorDropFields(const bf_rt_target_t &dev_tgt,
                                       const BfRtTMTable &w_table);

  bf_status_t initFieldIds(const BfRtTMTable &w_table,
                           std::set<bf_rt_id_t> &wrk_fields);

  bf_status_t getFieldValues(const BfRtTMTable &w_table,
                             const BfRtTMTableData &s_data);

  bf_status_t setFieldValues(const BfRtTMTable &w_table,
                             BfRtTMTableData *s_data);

  void clearIds() {
    f_mirror_drop_enable = 0;
    f_mirror_drop_dev_port = 0;
    f_mirror_drop_queue_nr = 0;
    f_mirror_drop_pg_id = 0;
    f_mirror_drop_pg_queue = 0;
    f_eg_limit_cells = 0;
    f_eg_hysteresis_cells = 0;
    f_queue_depth_report_mode = 0;
  }

  void clearValues() {
    mirror_drop_enable = false;
    mirror_drop_dev_port = 0;
    mirror_drop_queue_nr = 0;
    mirror_drop_pg_id = 0;
    mirror_drop_pg_queue = 0;
    eg_limit_cells = 0;
    eg_hysteresis_cells = 0;
    queue_depth_report_mode = false;
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_mirror_drop_enable;
  bf_rt_id_t f_mirror_drop_dev_port;
  bf_rt_id_t f_mirror_drop_queue_nr;
  bf_rt_id_t f_mirror_drop_pg_id;
  bf_rt_id_t f_mirror_drop_pg_queue;
  bf_rt_id_t f_eg_limit_cells;
  bf_rt_id_t f_eg_hysteresis_cells;
  bf_rt_id_t f_queue_depth_report_mode;

  // Data Field values
  bool mirror_drop_enable;
  bf_dev_port_t mirror_drop_dev_port;
  bf_tm_queue_t mirror_drop_queue_nr;
  bf_tm_pg_t mirror_drop_pg_id;
  bf_tm_queue_t mirror_drop_pg_queue;
  uint32_t eg_limit_cells;
  uint32_t eg_hysteresis_cells;
  bool queue_depth_report_mode;

  // Maps to convert between string and enum types
  static const std::map<std::string, bool> str_to_qstat_mode;
  static const std::map<bool, std::string> qstat_mode_to_str;
};

//----
class BfRtTMPipeSchedCfgHelper {
 public:
  BfRtTMPipeSchedCfgHelper() {
    this->clearValues();
    this->clearIds();
  }

  virtual ~BfRtTMPipeSchedCfgHelper() = default;

  bf_status_t initFieldIds(const BfRtTMTable &w_table,
                           std::set<bf_rt_id_t> &wrk_fields);

  bf_status_t getFieldValues(const BfRtTMTable &w_table,
                             const BfRtTMTableData &s_data);

  bf_status_t setFieldValues(const BfRtTMTable &w_table,
                             BfRtTMTableData *s_data);

  void clearIds() {
    f_advanced_flow_control_enable = 0;
    f_packet_ifg_compensation = 0;
  }

  void clearValues() {
    advanced_flow_control_enable = false;
    packet_ifg_compensation = 0;
  }

 public:
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_advanced_flow_control_enable;
  bf_rt_id_t f_packet_ifg_compensation;

  // Data Field values
  uint8_t packet_ifg_compensation;
  bool advanced_flow_control_enable;
};

}  // namespace bfrt
#endif
