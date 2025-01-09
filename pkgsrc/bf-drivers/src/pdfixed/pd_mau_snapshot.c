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
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/pdfixed/pd_common.h>

p4_pd_status_t p4_pd_snapshot_timer_enable(pipe_snapshot_hdl_t hdl,
                                           bool disable) {
  bf_snapshot_ig_mode_t mode;
  bool old_val;
  uint32_t usec;
  bf_snapshot_cfg_get(hdl, &old_val, &usec, &mode);
  return bf_snapshot_cfg_set(hdl, disable, mode);
}

p4_pd_status_t p4_pd_snapshot_state_set(pipe_snapshot_hdl_t hdl,
                                        bf_snapshot_state_t state,
                                        uint32_t timer_usec) {
  return bf_snapshot_state_set(hdl, state, timer_usec);
}

p4_pd_status_t p4_pd_snapshot_state_get(pipe_snapshot_hdl_t hdl,
                                        bf_dev_pipe_t dev_pipe_id,
                                        bf_snapshot_state_t *state) {
  return bf_snapshot_pd_state_get(hdl, dev_pipe_id, (int *)state);
}

p4_pd_status_t p4_pd_snapshot_capture_trigger_fields_clr(
    pipe_snapshot_hdl_t hdl) {
  return bf_snapshot_capture_trigger_fields_clr(hdl);
}

p4_pd_status_t p4_pd_snapshot_field_in_scope(p4_pd_dev_target_t dev_tgt,
                                             uint8_t stage,
                                             bf_snapshot_dir_t dir,
                                             char *field_name,
                                             bool *field_exists) {
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_snapshot_field_in_scope(
      dev, dev_pipe_id, stage, dir, field_name, field_exists);
}

p4_pd_status_t p4_pd_snapshot_trigger_field_in_scope(p4_pd_dev_target_t dev_tgt,
                                                     uint8_t stage,
                                                     bf_snapshot_dir_t dir,
                                                     char *field_name,
                                                     bool *field_exists) {
  bf_dev_id_t dev = dev_tgt.device_id;
  bf_dev_pipe_t dev_pipe_id = dev_tgt.dev_pipe_id;

  return bf_snapshot_trigger_field_in_scope(
      dev, dev_pipe_id, stage, dir, field_name, field_exists);
}
