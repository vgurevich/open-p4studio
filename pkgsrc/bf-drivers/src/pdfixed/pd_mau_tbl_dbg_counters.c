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


#include <stdio.h>
#include <string.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/pdfixed/pd_common.h>

p4_pd_status_t p4_pd_tbl_dbg_counter_type_set(
    p4_pd_dev_target_t dev_tgt,
    char *tbl_name,
    p4_pd_tbl_dbg_counter_type_t type) {
  dev_target_t pipe_mgr_dev_tgt;

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_tbl_dbg_counter_type_set(
      pipe_mgr_dev_tgt, tbl_name, (bf_tbl_dbg_counter_type_t)type);
}

p4_pd_status_t p4_pd_tbl_dbg_counter_get(p4_pd_dev_target_t dev_tgt,
                                         char *tbl_name,
                                         p4_pd_tbl_dbg_counter_type_t *type,
                                         uint32_t *counter_value) {
  p4_pd_status_t status = 0;
  dev_target_t pipe_mgr_dev_tgt;
  bf_tbl_dbg_counter_type_t temp_type = 0;

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  status = bf_tbl_dbg_counter_get(
      pipe_mgr_dev_tgt, tbl_name, &temp_type, counter_value);
  *type = (p4_pd_tbl_dbg_counter_type_t)temp_type;
  return status;
}

p4_pd_status_t p4_pd_tbl_dbg_counter_clear(p4_pd_dev_target_t dev_tgt,
                                           char *tbl_name) {
  dev_target_t pipe_mgr_dev_tgt;

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_tbl_dbg_counter_clear(pipe_mgr_dev_tgt, tbl_name);
}

p4_pd_status_t p4_pd_tbl_dbg_counter_type_stage_set(
    p4_pd_dev_target_t dev_tgt,
    uint8_t stage_id,
    p4_pd_tbl_dbg_counter_type_t type) {
  dev_target_t pipe_mgr_dev_tgt;

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_tbl_dbg_counter_type_stage_set(
      pipe_mgr_dev_tgt, stage_id, (bf_tbl_dbg_counter_type_t)type);
}

p4_pd_status_t p4_pd_tbl_dbg_counter_stage_get(
    p4_pd_dev_target_t dev_tgt,
    uint8_t stage_id,
    p4_pd_stage_tbl_dbg_counters_t *stage_cntrs) {
  p4_pd_status_t status = 0;
  dev_target_t pipe_mgr_dev_tgt;
  uint32_t value_arr[BF_MAX_LOG_TBLS];
  int num_counters = 0, idx = 0;
  char tbl_name[BF_MAX_LOG_TBLS][BF_TBL_NAME_LEN];
  bf_tbl_dbg_counter_type_t type_arr[BF_MAX_LOG_TBLS];

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  status = bf_tbl_dbg_counter_stage_get(pipe_mgr_dev_tgt,
                                        stage_id,
                                        &type_arr[0],
                                        &value_arr[0],
                                        tbl_name,
                                        &num_counters);

  stage_cntrs->num_counters = num_counters;
  for (idx = 0; idx < num_counters; idx++) {
    stage_cntrs->counter[idx].cntr_type =
        (p4_pd_tbl_dbg_counter_type_t)type_arr[idx];
    stage_cntrs->counter[idx].value = value_arr[idx];
    strncpy(
        stage_cntrs->counter[idx].tbl_name, tbl_name[idx], BF_TBL_NAME_LEN - 1);
    stage_cntrs->counter[idx].tbl_name[BF_TBL_NAME_LEN - 1] = 0;
  }

  return status;
}

p4_pd_status_t p4_pd_tbl_dbg_counter_stage_clear(p4_pd_dev_target_t dev_tgt,
                                                 uint8_t stage_id) {
  dev_target_t pipe_mgr_dev_tgt;

  pipe_mgr_dev_tgt.device_id = dev_tgt.device_id;
  pipe_mgr_dev_tgt.dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_tbl_dbg_counter_stage_clear(pipe_mgr_dev_tgt, stage_id);
}
