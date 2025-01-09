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


/*!
 * @file pipe_mgr_meter_tbl.h
 * @date
 *
 *
 * Contains definitions/APIs exposed internally to pipe manager
 * for meter table management.
 */

#ifndef _PIPE_MGR_METER_TBL_H
#define _PIPE_MGR_METER_TBL_H

/* Standard header includes */

/* Module header includes */
#include "pipe_mgr_int.h"
#include <target-utils/third-party/cJSON/cJSON.h>

/* Local header includes */

typedef uint32_t pipe_meter_stage_idx_t;
typedef uint32_t pipe_lpf_stage_idx_t;
typedef uint32_t pipe_wred_stage_idx_t;

#define PIPE_METER_ENT_INVALID_ENTRY_INDEX 0xdeadbeef

typedef enum pipe_mgr_meter_spec_type {
  SPEC_TYPE_METER,
  SPEC_TYPE_LPF,
  SPEC_TYPE_WRED,
} pipe_mgr_meter_spec_type_e;

typedef struct pipe_mgr_meter_move_list_ {
  struct pipe_mgr_meter_move_list_ *next;
  struct pipe_mgr_meter_move_list_ *prev;
  pipe_meter_stage_idx_t src_ent_idx;
  pipe_meter_stage_idx_t dst_ent_idx;
} pipe_mgr_meter_move_list_t;

struct pipe_mgr_meter_op_list_t;

/* API to reset a meter table */
pipe_status_t pipe_mgr_meter_mgr_meter_reset(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    struct pipe_mgr_meter_op_list_t **head_p);

pipe_status_t pipe_mgr_meter_mgr_lpf_reset(
    dev_target_t dev_tgt,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    struct pipe_mgr_meter_op_list_t **head_p);

pipe_status_t pipe_mgr_meter_mgr_wred_reset(
    dev_target_t dev_tgt,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    struct pipe_mgr_meter_op_list_t **head_p);

/* API to update a meter entry specification */
pipe_status_t pipe_mgr_meter_mgr_meter_ent_set(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t meter_idx,
    pipe_meter_spec_t *meter_spec,
    struct pipe_mgr_meter_op_list_t **head_p);

pipe_status_t pipe_mgr_meter_mgr_lpf_ent_set(
    dev_target_t dev_tgt,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    pipe_lpf_idx_t lpf_idx,
    pipe_lpf_spec_t *lpf_spec,
    struct pipe_mgr_meter_op_list_t **head_p);

pipe_status_t pipe_mgr_meter_mgr_wred_ent_set(
    dev_target_t dev_tgt,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    pipe_wred_idx_t wred_idx,
    pipe_wred_spec_t *wred_spec,
    struct pipe_mgr_meter_op_list_t **head_p);

pipe_status_t pipe_mgr_meter_process_op_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    struct pipe_mgr_meter_op_list_t *ml,
    uint32_t *success_count);

pipe_status_t pipe_mgr_meter_download_specs_from_shadow(
    rmt_dev_info_t *dev_info,
    rmt_dev_profile_info_t *dev_profile_info,
    pipe_meter_tbl_info_t *meter_tbl_info,
    void *arg);

void pipe_mgr_meter_free_ops(struct pipe_mgr_meter_op_list_t **l);

pipe_status_t pipe_mgr_meter_verify_idx(bf_dev_id_t device_id,
                                        bf_dev_pipe_t pipe_id,
                                        pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                        pipe_meter_idx_t meter_idx);

pipe_status_t rmt_meter_mgr_meter_attach(bf_dev_id_t device_id,
                                         bf_dev_pipe_t pipe_id,
                                         uint8_t stage_id,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         pipe_meter_idx_t meter_idx,
                                         rmt_virt_addr_t *ent_virt_addr);

pipe_status_t rmt_meter_mgr_direct_lpf_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    pipe_lpf_stage_idx_t lpf_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_lpf_spec_t *lpf_spec,
    rmt_virt_addr_t *ent_virt_addr);

pipe_status_t rmt_meter_mgr_direct_wred_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    pipe_wred_stage_idx_t wred_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_wred_spec_t *wred_spec,
    rmt_virt_addr_t *ent_virt_addr);

pipe_status_t rmt_meter_mgr_meter_detach(pipe_sess_hdl_t sess_hdl,
                                         bf_dev_id_t device_id,
                                         bf_dev_pipe_t pipe_id,
                                         uint8_t stage_id,
                                         pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                         pipe_meter_idx_t meter_idx,
                                         uint32_t pipe_api_flags);

pipe_status_t rmt_meter_mgr_direct_meter_attach(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t meter_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_meter_spec_t *meter_spec,
    rmt_virt_addr_t *ent_virt_addr);

pipe_status_t pipe_mgr_meter_mgr_move_meter_spec(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t pipe_id,
    uint8_t src_stage_id,
    uint8_t dst_stage_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t src_idx,
    pipe_meter_stage_idx_t dst_idx);

pipe_status_t rmt_meter_mgr_direct_meter_attach_max_spec(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t meter_stage_idx,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    uint32_t pipe_api_flags);

pipe_status_t pipe_mgr_meter_mgr_read_dir_entry(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_ent_hdl_t entry_hdl,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    bool from_hw,
    pipe_res_get_data_t *res_data);

pipe_status_t pipe_mgr_meter_mgr_read_entry(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_stage_idx_t meter_idx,
    pipe_res_data_spec_t *res_spec,
    pipe_mgr_meter_spec_type_e spec_type,
    bool from_hw);

pipe_status_t pipe_mgr_meter_mgr_read_entry_in_stage(
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    uint8_t stage_id,
    pipe_meter_stage_idx_t meter_idx,
    pipe_res_data_spec_t *res_spec,
    pipe_mgr_meter_spec_type_e spec_type,
    bool from_hw);

pipe_status_t pipe_mgr_meter_mgr_construct_disabled_virt_addr(
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    rmt_virt_addr_t *virt_addr);

pipe_status_t pipe_mgr_meter_mgr_decode_virt_addr(
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    rmt_virt_addr_t virt_addr,
    bool *pfe,
    bool *pfe_defaulted,
    pipe_meter_idx_t *meter_idx);
pipe_status_t pipe_mgr_meter_log_state(bf_dev_id_t dev_id,
                                       pipe_meter_tbl_hdl_t tbl_hdl,
                                       cJSON *meter_tbls);

pipe_status_t pipe_mgr_meter_restore_state(bf_dev_id_t dev_id,
                                           cJSON *meter_tbl);

bool pipe_mgr_meter_tbl_is_indirect(bf_dev_id_t device_id,
                                    pipe_meter_tbl_hdl_t meter_tbl_hdl);

pipe_status_t pipe_mgr_meter_tbl_sbe_correct(bf_dev_id_t dev_id,
                                             bf_dev_pipe_t log_pipe_id,
                                             dev_stage_t stage_id,
                                             pipe_meter_tbl_hdl_t tbl_hdl,
                                             int line);

pipe_status_t pipe_mgr_meter_tbl_bytecount_adjust_set(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int bytecount);

pipe_status_t pipe_mgr_meter_tbl_bytecount_adjust_get(
    dev_target_t dev_tgt, pipe_meter_tbl_hdl_t meter_tbl_hdl, int *bytecount);

#endif  // _PIPE_MGR_METER_TBL_H
