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


#ifndef _BF_RT_TM_TABLE_HELPER_PPG_HPP
#define _BF_RT_TM_TABLE_HELPER_PPG_HPP

#include <climits>
#include "bf_rt_tm_table_impl.hpp"

namespace bfrt {

// PPG common field blocks to reuse in tables with different keys.

class BfRtTMPpgHelper {
 public:
  virtual ~BfRtTMPpgHelper() = default;

  //--------------------

  // Pops out the valid field id from the working set looking by name.
  static bf_status_t popWorkField(const BfRtTMTable &w_table,
                                  const std::string &field_name,
                                  const bf_rt_id_t action_id,
                                  std::set<bf_rt_id_t> &wrk_fields,
                                  bf_rt_id_t &field_id) {
    return w_table.popWorkField(field_name, action_id, wrk_fields, field_id);
  }
};

class BfRtTMPpgIcosHelper : public BfRtTMPpgHelper {
 public:
  virtual ~BfRtTMPpgIcosHelper() = default;

  //
  static bf_status_t setValuesIcos(const BfRtTMTable &w_table,
                                   const uint8_t icos_mask,
                                   std::set<bf_rt_id_t> &wrk_fields,
                                   BfRtTMTableData *p_data);

  static bf_status_t getValuesIcos(const BfRtTMTable &w_table,
                                   const BfRtTMTableData &p_data,
                                   std::set<bf_rt_id_t> &wrk_fields,
                                   uint8_t &new_icos,
                                   uint8_t &new_mask);

  static bf_status_t writeValuesIcos(const bf_rt_target_t &dev_tgt,
                                     const BfRtTMTable &w_table,
                                     bf_tm_ppg_hdl ppg_hdl,
                                     uint8_t new_icos,
                                     uint8_t new_mask);

  // Sets values to 'PPG iCoS map' group of R/O fields.
  static bf_status_t setValuesPPG(const BfRtTMTable &w_table,
                                  const std::vector<bf_tm_ppg_id_t> &ppg_ids,
                                  std::set<bf_rt_id_t> &wrk_fields,
                                  BfRtTMTableData *p_data);

  // Extract dev_port action/value from the data object given.
  static bf_status_t getActionValuePort(const BfRtTMTable &w_table,
                                        const BfRtTMTableData &p_data,
                                        const bf_rt_target_t &dev_tgt,
                                        std::set<bf_rt_id_t> &wrk_fields,
                                        bf_dev_port_t &port_id);

  //---------------------
  static bf_status_t getFieldsIcos(const bf_rt_target_t &dev_tgt,
                                   const BfRtTMTable &w_table,
                                   bf_tm_ppg_hdl ppg_hdl,
                                   std::set<bf_rt_id_t> &wrk_fields,
                                   BfRtTMTableData *p_data,
                                   uint8_t &icos_mask);

  //--------------------
  static bf_status_t getFieldsPPGs(const bf_rt_target_t &dev_tgt,
                                   const BfRtTMTable &w_table,
                                   const BfRtTMPpgStateObj &ppg_state,
                                   bf_dev_port_t port_id,
                                   uint8_t icos_mask,
                                   std::set<bf_rt_id_t> &wrk_fields,
                                   BfRtTMTableData *p_data);

 private:
  static const char *icos_fname[CHAR_BIT];
};

class BfRtTMPpgBufferHelper : public BfRtTMPpgHelper {
 public:
  virtual ~BfRtTMPpgBufferHelper() = default;

  // With TM r/w and data object r/w
  bf_status_t setFieldsBuffer(const bf_rt_target_t &dev_tgt,
                              const BfRtTMTable &w_table,
                              const BfRtTMTableData &p_data,
                              std::set<bf_rt_id_t> &wrk_fields,
                              bf_tm_ppg_hdl ppg_hdl);

  bf_status_t getFieldsBuffer(const bf_rt_target_t &dev_tgt,
                              const BfRtTMTable &w_table,
                              bf_tm_ppg_hdl ppg_hdl,
                              std::set<bf_rt_id_t> &wrk_fields,
                              BfRtTMTableData *p_data);

  bf_status_t resetFieldsBuffer(const bf_rt_target_t &dev_tgt,
                                const BfRtTMTable &w_table,
                                bf_tm_ppg_hdl ppg_hdl,
                                std::set<bf_rt_id_t> &wrk_fields,
                                BfRtTMTableData *p_data);

 private:
  bf_status_t initFieldIds(const BfRtTMTable &w_table,
                           std::set<bf_rt_id_t> &wrk_fields);

  bf_status_t readValues(const bf_rt_target_t &dev_tgt,
                         const BfRtTMTable &w_table,
                         bf_tm_ppg_hdl ppg_hdl,
                         bool for_update);

  bf_status_t getFieldValues(const BfRtTMTable &w_table,
                             const BfRtTMTableData &p_data);

  bf_status_t writeValues(const bf_rt_target_t &dev_tgt,
                          const BfRtTMTable &w_table,
                          bf_tm_ppg_hdl ppg_hdl);

  bf_status_t setFieldValues(const BfRtTMTable &w_table,
                             BfRtTMTableData *p_data);
  //--------------
  // Data Field id's, or zero if not needed.
  bf_rt_id_t f_pool_id;
  bf_rt_id_t f_pool_max_cells;
  bf_rt_id_t f_dynamic_baf;
  bf_rt_id_t f_guaranteed_cells;
  bf_rt_id_t f_hysteresis_cells;

  // Data Field values
  bf_tm_app_pool_t pool_id;
  std::string pool_id_str;

  uint32_t pool_max_cells;

  bf_tm_ppg_baf_t dynamic_baf;
  std::string dynamic_baf_str;

  uint32_t guaranteed_cells;
  uint32_t hysteresis_cells;

  // Maps to convert between string and enum types
  static const std::map<std::string, bf_tm_app_pool_t> str_to_ig_pool;
  static const std::map<bf_tm_app_pool_t, std::string> ig_pool_to_str;

  static const std::map<std::string, bf_tm_ppg_baf_t> str_to_ppgbaf;
  static const std::map<bf_tm_ppg_baf_t, std::string> ppgbaf_to_str;
};

}  // namespace bfrt
#endif
