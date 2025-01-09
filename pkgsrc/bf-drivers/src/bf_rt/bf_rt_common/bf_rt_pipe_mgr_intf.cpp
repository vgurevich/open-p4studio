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


#ifdef __cplusplus
extern "C" {
#endif
#include <bf_rt/bf_rt_common.h>
#ifdef __cplusplus
}
#endif

/* bf_rt_includes */
#include <bf_rt/bf_rt_info.hpp>
#include "bf_rt_pipe_mgr_intf.hpp"

namespace bfrt {

std::unique_ptr<IPipeMgrIntf> IPipeMgrIntf::instance = nullptr;
std::mutex IPipeMgrIntf::pipe_mgr_intf_mtx;

pipe_status_t PipeMgrIntf::pipeMgrInit() { return pipe_mgr_init(); }

void PipeMgrIntf::pipeMgrCleanup() { return pipe_mgr_cleanup(); }

pipe_status_t PipeMgrIntf::pipeMgrClientInit(pipe_sess_hdl_t *sess_hdl) {
  return pipe_mgr_client_init(sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrClientCleanup(pipe_sess_hdl_t def_sess_hdl) {
  return pipe_mgr_client_cleanup(def_sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrCompleteOperations(
    pipe_sess_hdl_t def_sess_hdl) {
  return pipe_mgr_complete_operations(def_sess_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrBeginTxn(pipe_sess_hdl_t shdl,
                                           bool isAtomic) {
  return pipe_mgr_begin_txn(shdl, isAtomic);
}

pipe_status_t PipeMgrIntf::pipeMgrVerifyTxn(pipe_sess_hdl_t shdl) {
  return pipe_mgr_verify_txn(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrAbortTxn(pipe_sess_hdl_t shdl) {
  return pipe_mgr_abort_txn(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrCommitTxn(pipe_sess_hdl_t shdl,
                                            bool hwSynchronous) {
  return pipe_mgr_commit_txn(shdl, hwSynchronous);
}

pipe_status_t PipeMgrIntf::pipeMgrBeginBatch(pipe_sess_hdl_t shdl) {
  return pipe_mgr_begin_batch(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrFlushBatch(pipe_sess_hdl_t shdl) {
  return pipe_mgr_flush_batch(shdl);
}

pipe_status_t PipeMgrIntf::pipeMgrEndBatch(pipe_sess_hdl_t shdl,
                                           bool hwSynchronous) {
  return pipe_mgr_end_batch(shdl, hwSynchronous);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchSpecFree(
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_spec_free(match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrGetActionDirectResUsage(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    bool *has_dir_stats,
    bool *has_dir_meter,
    bool *has_dir_lpf,
    bool *has_dir_wred,
    bool *has_dir_stful) {
  return pipe_mgr_get_action_dir_res_usage(device_id,
                                           mat_tbl_hdl,
                                           act_fn_hdl,
                                           has_dir_stats,
                                           has_dir_meter,
                                           has_dir_lpf,
                                           has_dir_wred,
                                           has_dir_stful);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchSpecToEntHdl(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mat_ent_hdl_t *mat_ent_hdl,
    bool light_pipe_validation) {
  return pipe_mgr_match_spec_to_ent_hdl(sess_hdl,
                                        dev_tgt,
                                        mat_tbl_hdl,
                                        match_spec,
                                        mat_ent_hdl,
                                        light_pipe_validation);
}

pipe_status_t PipeMgrIntf::pipeMgrEntHdlToMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *entry_pipe,
    const pipe_tbl_match_spec_t **match_spec) {
  return pipe_mgr_ent_hdl_to_match_spec(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, entry_pipe, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_key_mask_spec_set(
      sess_hdl, device_id, mat_tbl_hdl, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecReset(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  return pipe_mgr_match_key_mask_spec_reset(sess_hdl, device_id, mat_tbl_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMatchKeyMaskSpecGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec) {
  return pipe_mgr_match_key_mask_spec_get(
      sess_hdl, device_id, mat_tbl_hdl, match_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntAdd(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_mat_ent_add(sess_hdl,
                              dev_tgt,
                              mat_tbl_hdl,
                              match_spec,
                              act_fn_hdl,
                              (pipe_action_spec_t *)act_data_spec,
                              ttl,
                              pipe_api_flags,
                              ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntAddOrMod(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    bool *is_added) {
  return pipe_mgr_mat_ent_add_or_mod(sess_hdl,
                                     dev_tgt,
                                     mat_tbl_hdl,
                                     match_spec,
                                     act_fn_hdl,
                                     (pipe_action_spec_t *)act_data_spec,
                                     ttl,
                                     pipe_api_flags,
                                     ent_hdl_p,
                                     is_added);
}

pipe_status_t PipeMgrIntf::pipeMgrMatDefaultEntrySet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_mat_default_entry_set(sess_hdl,
                                        dev_tgt,
                                        mat_tbl_hdl,
                                        act_fn_hdl,
                                        (pipe_action_spec_t *)act_spec,
                                        pipe_api_flags,
                                        ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrTableGetDefaultEntry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data) {
  return pipe_mgr_table_get_default_entry(sess_hdl,
                                          dev_tgt,
                                          mat_tbl_hdl,
                                          pipe_action_spec,
                                          act_fn_hdl,
                                          from_hw,
                                          res_get_flags,
                                          res_data);
}

pipe_status_t PipeMgrIntf::pipeMgrTableGetDefaultEntryHandle(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t *ent_hdl_p) {
  return pipe_mgr_table_get_default_entry_handle(
      sess_hdl, dev_tgt, mat_tbl_hdl, ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrMatTblClear(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              uint32_t pipe_api_flags) {
  return pipe_mgr_mat_tbl_clear(sess_hdl, dev_tgt, mat_tbl_hdl, pipe_api_flags);
}
pipe_status_t PipeMgrIntf::pipeMgrMatEntDel(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            pipe_mat_ent_hdl_t mat_ent_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_del(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDelByMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_del_by_match_spec(
      sess_hdl, dev_tgt, mat_tbl_hdl, match_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatTblDefaultEntryReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_tbl_default_entry_reset(
      sess_hdl, dev_tgt, mat_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetAction(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_action(sess_hdl,
                                     device_id,
                                     mat_tbl_hdl,
                                     mat_ent_hdl,
                                     act_fn_hdl,
                                     act_spec,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetActionByMatchSpec(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_action_by_match_spec(
      sess_hdl,
      dev_tgt,
      mat_tbl_hdl,
      match_spec,
      act_fn_hdl,
      (pipe_action_spec_t *)act_spec,
      pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetResource(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags) {
  return pipe_mgr_mat_ent_set_resource(sess_hdl,
                                       device_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       resources,
                                       resource_count,
                                       pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntAdd(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_adt_mbr_id_t mbr_id,
    const pipe_action_spec_t *action_spec,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p,
    uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_add(sess_hdl,
                              dev_tgt,
                              adt_tbl_hdl,
                              act_fn_hdl,
                              mbr_id,
                              (pipe_action_spec_t *)action_spec,
                              adt_ent_hdl_p,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntDel(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                            pipe_adt_ent_hdl_t adt_ent_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_del(
      sess_hdl, device_id, adt_tbl_hdl, adt_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    const pipe_action_spec_t *action_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_adt_ent_set(sess_hdl,
                              device_id,
                              adt_tbl_hdl,
                              adt_ent_hdl,
                              act_fn_hdl,
                              (pipe_action_spec_t *)action_spec,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntHdlGet(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_tgt,
                                               pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                               pipe_adt_mbr_id_t mbr_id,
                                               pipe_adt_ent_hdl_t *adt_ent_hdl,
                                               bool check_only) {
  return pipe_mgr_adt_ent_hdl_get(
      sess_hdl, dev_tgt, adt_tbl_hdl, mbr_id, adt_ent_hdl, check_only);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntMbrIdGet(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t dev_id,
                                                 pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                                 pipe_adt_ent_hdl_t adt_ent_hdl,
                                                 pipe_adt_mbr_id_t *mbr_id,
                                                 bf_dev_pipe_t *mbr_pipe) {
  return pipe_mgr_adt_mbr_id_get(
      sess_hdl, dev_id, adt_tbl_hdl, adt_ent_hdl, mbr_id, mbr_pipe);
}

pipe_status_t PipeMgrIntf::pipeMgrAdtEntDataGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_adt_tbl_hdl_t adt_tbl_hdl,
    pipe_adt_mbr_id_t mbr_id,
    pipe_adt_ent_hdl_t *adt_ent_hdl,
    pipe_mgr_adt_ent_data_t *ent_data) {
  return pipe_mgr_adt_ent_data_get(
      sess_hdl, dev_tgt, adt_tbl_hdl, mbr_id, adt_ent_hdl, ent_data);
}

pipe_status_t PipeMgrIntf::pipeMgrTblIsTern(bf_dev_id_t dev_id,
                                            pipe_tbl_hdl_t tbl_hdl,
                                            bool *is_tern) {
  return pipe_mgr_tbl_is_tern(dev_id, tbl_hdl, is_tern);
}

pipe_status_t PipeMgrIntf::pipeMgrSelTblRegisterCb(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_mgr_sel_tbl_update_callback cb,
    void *cb_cookie) {
  return pipe_mgr_sel_tbl_register_cb(
      sess_hdl, device_id, sel_tbl_hdl, cb, cb_cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrSelTblProfileSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_tbl_profile_t *sel_tbl_profile) {
  return pipe_mgr_sel_tbl_profile_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_tbl_profile);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpAdd(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_id_t sel_grp_id,
                                            uint32_t max_grp_size,
                                            uint32_t adt_offset,
                                            pipe_sel_grp_hdl_t *sel_grp_hdl_p,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_add(sess_hdl,
                              dev_tgt,
                              sel_tbl_hdl,
                              sel_grp_id,
                              max_grp_size,
                              adt_offset,
                              sel_grp_hdl_p,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpDel(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t device_id,
                                            pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                            pipe_sel_grp_hdl_t sel_grp_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_del(
      sess_hdl, device_id, sel_tbl_hdl, sel_grp_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpSizeSet(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                uint32_t max_grp_size) {
  return pipe_mgr_sel_grp_size_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_grp_hdl, max_grp_size);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrAdd(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_act_fn_hdl_t act_fn_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_add(sess_hdl,
                                  device_id,
                                  sel_tbl_hdl,
                                  sel_grp_hdl,
                                  act_fn_hdl,
                                  adt_ent_hdl,
                                  pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrDel(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t device_id,
                                               pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                               pipe_sel_grp_hdl_t sel_grp_hdl,
                                               pipe_adt_ent_hdl_t adt_ent_hdl,
                                               uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_del(sess_hdl,
                                  device_id,
                                  sel_tbl_hdl,
                                  sel_grp_hdl,
                                  adt_ent_hdl,
                                  pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrsSet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                uint32_t num_mbrs,
                                                pipe_adt_ent_hdl_t *mbrs,
                                                bool *enable,
                                                uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbrs_set(sess_hdl,
                                   device_id,
                                   sel_tbl_hdl,
                                   sel_grp_hdl,
                                   num_mbrs,
                                   mbrs,
                                   enable,
                                   pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrsGet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t device_id,
                                                pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                                pipe_sel_grp_hdl_t sel_grp_hdl,
                                                uint32_t mbrs_size,
                                                pipe_adt_ent_hdl_t *mbrs,
                                                bool *enable,
                                                uint32_t *mbrs_populated,
                                                bool from_hw) {
  return pipe_mgr_sel_grp_mbrs_get(sess_hdl,
                                   device_id,
                                   sel_tbl_hdl,
                                   sel_grp_hdl,
                                   mbrs_size,
                                   mbrs,
                                   enable,
                                   mbrs_populated,
                                   from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrDisable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_disable(sess_hdl,
                                      device_id,
                                      sel_tbl_hdl,
                                      sel_grp_hdl,
                                      adt_ent_hdl,
                                      pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrEnable(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_grp_mbr_enable(sess_hdl,
                                     device_id,
                                     sel_tbl_hdl,
                                     sel_grp_hdl,
                                     adt_ent_hdl,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrStateGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    enum pipe_mgr_grp_mbr_state_e *mbr_state_p) {
  return pipe_mgr_sel_grp_mbr_state_get(
      sess_hdl, device_id, sel_tbl_hdl, sel_grp_hdl, adt_ent_hdl, mbr_state_p);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpIdGet(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_sel_tbl_hdl_t sel_tbl_hdl,
                                              pipe_sel_grp_hdl_t sel_grp_hdl,
                                              pipe_sel_grp_id_t *sel_grp_id) {
  return pipe_mgr_sel_grp_id_get(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_grp_hdl, sel_grp_id);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpHdlGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_id_t sel_grp_id,
    pipe_sel_grp_hdl_t *sel_grp_hdl) {
  return pipe_mgr_sel_grp_hdl_get(
      sess_hdl, dev_tgt, sel_tbl_hdl, sel_grp_id, sel_grp_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrSelFallbackMbrSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_adt_ent_hdl_t adt_ent_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_fallback_mbr_set(
      sess_hdl, dev_tgt, sel_tbl_hdl, adt_ent_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrSelFallbackMbrReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    uint32_t pipe_api_flags) {
  return pipe_mgr_sel_fallback_mbr_reset(
      sess_hdl, dev_tgt, sel_tbl_hdl, pipe_api_flags);
}
pipe_status_t PipeMgrIntf::pipeMgrSelGrpMbrGetFromHash(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t sel_tbl_hdl,
    pipe_sel_grp_hdl_t grp_hdl,
    uint8_t *hash,
    uint32_t hash_len,
    pipe_adt_ent_hdl_t *adt_ent_hdl_p) {
  return pipe_mgr_sel_grp_mbr_get_from_hash(
      sess_hdl, dev_id, sel_tbl_hdl, grp_hdl, hash, hash_len, adt_ent_hdl_p);
}

pipe_status_t PipeMgrIntf::pipeMgrLrnDigestNotificationRegister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_notify_cb callback_fn,
    void *callback_fn_cookie) {
  return pipe_mgr_lrn_digest_notification_register(sess_hdl,
                                                   device_id,
                                                   flow_lrn_fld_lst_hdl,
                                                   callback_fn,
                                                   callback_fn_cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrLrnDigestNotificationDeregister(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl) {
  return pipe_mgr_lrn_digest_notification_deregister(
      sess_hdl, device_id, flow_lrn_fld_lst_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnNotifyAck(
    pipe_sess_hdl_t sess_hdl,
    pipe_fld_lst_hdl_t flow_lrn_fld_lst_hdl,
    pipe_flow_lrn_msg_t *pipe_flow_lrn_msg) {
  return pipe_mgr_flow_lrn_notify_ack(
      sess_hdl, flow_lrn_fld_lst_hdl, pipe_flow_lrn_msg);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnTimeoutSet(pipe_sess_hdl_t sess_hdl,
                                                    bf_dev_id_t device_id,
                                                    uint32_t usecs) {
  return pipe_mgr_flow_lrn_set_timeout(sess_hdl, device_id, usecs);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnTimeoutGet(bf_dev_id_t device_id,
                                                    uint32_t *usecs) {
  return pipe_mgr_flow_lrn_get_timeout(device_id, usecs);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnIntrModeSet(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool en) {
  return pipe_mgr_flow_lrn_set_intr_mode(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnIntrModeGet(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_id_t device_id,
                                                     bool *en) {
  return pipe_mgr_flow_lrn_get_intr_mode(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrInactiveNodeDeleteSet(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, bool en) {
  return pipe_mgr_inactive_node_delete_set(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrInactiveNodeDeleteGet(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t device_id, bool *en) {
  return pipe_mgr_inactive_node_delete_get(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrSelectorMbrOrderSet(pipe_sess_hdl_t sess_hdl,
                                                      bf_dev_id_t device_id,
                                                      bool en) {
  return pipe_mgr_selector_tbl_member_order_set(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrSelectorMbrOrderGet(pipe_sess_hdl_t sess_hdl,
                                                      bf_dev_id_t device_id,
                                                      bool *en) {
  return pipe_mgr_selector_tbl_member_order_get(sess_hdl, device_id, en);
}

pipe_status_t PipeMgrIntf::pipeMgrFlowLrnSetNetworkOrderDigest(
    bf_dev_id_t device_id, bool network_order) {
  return pipe_mgr_flow_lrn_set_network_order_digest(device_id, network_order);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatQuery(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_query(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_set(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntDirectStatLoad(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_mat_ent_direct_stat_load(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntQuery(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_ent_idx_t *stat_ent_idx,
    size_t num_entries,
    pipe_stat_data_t **stat_data) {
  return pipe_mgr_stat_ent_query(
      sess_hdl, dev_target, stat_tbl_hdl, stat_ent_idx, num_entries, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatTableReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_table_reset(sess_hdl, dev_tgt, stat_tbl_hdl, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                             pipe_stat_ent_idx_t stat_ent_idx,
                                             pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_ent_set(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_ent_idx, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntLoad(pipe_sess_hdl_t sess_hdl,
                                              dev_target_t dev_tgt,
                                              pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                              pipe_stat_ent_idx_t stat_idx,
                                              pipe_stat_data_t *stat_data) {
  return pipe_mgr_stat_ent_load(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_idx, stat_data);
}

pipe_status_t PipeMgrIntf::pipeMgrStatDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_mgr_stat_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrDirectStatDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_stat_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_mgr_direct_stat_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeMgrStatEntDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    pipe_stat_ent_idx_t stat_ent_idx) {
  return pipe_mgr_stat_ent_database_sync(
      sess_hdl, dev_tgt, stat_tbl_hdl, stat_ent_idx);
}

pipe_status_t PipeMgrIntf::pipeMgrDirectStatEntDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_direct_stat_ent_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterEntSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t meter_idx,
    pipe_meter_spec_t *meter_spec,
    uint32_t pipe_api_flags) {
  return pipe_mgr_meter_ent_set(
      sess_hdl, dev_tgt, meter_tbl_hdl, meter_idx, meter_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterByteCountSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int byte_count) {
  return pipe_mgr_meter_set_bytecount_adjust(
      sess_hdl, dev_tgt, meter_tbl_hdl, byte_count);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterByteCountGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    int *byte_count) {
  return pipe_mgr_meter_get_bytecount_adjust(
      sess_hdl, dev_tgt, meter_tbl_hdl, byte_count);
}

pipe_status_t PipeMgrIntf::pipeMgrModelTimeAdvance(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t device_id,
                                                   uint64_t tick_time) {
  return pipe_mgr_model_time_advance(sess_hdl, device_id, tick_time);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReset(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                             uint32_t pipe_api_flags) {
  return pipe_mgr_meter_reset(sess_hdl, dev_tgt, meter_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrLpfReset(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
                                           uint32_t pipe_api_flags) {
  return pipe_mgr_lpf_reset(sess_hdl, dev_tgt, lpf_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrWredReset(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_wred_tbl_hdl_t wred_tbl_hdl,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_wred_reset(sess_hdl, dev_tgt, wred_tbl_hdl, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReadEntry(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_meter_spec_t *meter_spec) {
  return pipe_mgr_meter_read_entry(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, meter_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrMeterReadEntryIdx(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    pipe_meter_idx_t index,
    pipe_meter_spec_t *meter_spec,
    bool from_hw) {
  return pipe_mgr_meter_read_entry_idx(
      sess_hdl, dev_tgt, meter_tbl_hdl, index, meter_spec, from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrLpfEntSet(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                            pipe_lpf_idx_t lpf_idx,
                                            pipe_lpf_spec_t *lpf_spec,
                                            uint32_t pipe_api_flags) {
  return pipe_mgr_lpf_ent_set(
      sess_hdl, dev_tgt, meter_tbl_hdl, lpf_idx, lpf_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrLpfReadEntryIdx(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_lpf_tbl_hdl_t lpf_tbl_hdl,
    pipe_lpf_idx_t index,
    pipe_lpf_spec_t *lpf_spec,
    bool from_hw) {
  return pipe_mgr_lpf_read_entry_idx(
      sess_hdl, dev_tgt, lpf_tbl_hdl, index, lpf_spec, from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrLpfReadEntry(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_tgt,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               pipe_lpf_spec_t *lpf_spec) {
  return pipe_mgr_lpf_read_entry(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, lpf_spec);
}
pipe_status_t PipeMgrIntf::pipeMgrWredEntSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                             pipe_wred_idx_t red_idx,
                                             pipe_wred_spec_t *wred_spec,
                                             uint32_t pipe_api_flags) {
  return pipe_mgr_wred_ent_set(
      sess_hdl, dev_tgt, meter_tbl_hdl, red_idx, wred_spec, pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeMgrWredReadEntry(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t mat_ent_hdl,
                                                pipe_wred_spec_t *wred_spec) {
  return pipe_mgr_wred_read_entry(
      sess_hdl, dev_tgt, mat_tbl_hdl, mat_ent_hdl, wred_spec);
}

pipe_status_t PipeMgrIntf::pipeMgrWredReadEntryIdx(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_wred_tbl_hdl_t wred_tbl_hdl,
    pipe_wred_idx_t index,
    pipe_wred_spec_t *wred_spec,
    bool from_hw) {
  return pipe_mgr_wred_read_entry_idx(
      sess_hdl, dev_tgt, wred_tbl_hdl, index, wred_spec, from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrExmEntryActivate(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_exm_entry_activate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrExmEntryDeactivate(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_exm_entry_deactivate(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntSetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    bool reset) {
  return pipe_mgr_mat_ent_set_idle_ttl(sess_hdl,
                                       device_id,
                                       mat_tbl_hdl,
                                       mat_ent_hdl,
                                       ttl,
                                       pipe_api_flags,
                                       reset);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntResetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  return pipe_mgr_mat_ent_reset_idle_ttl(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTmoEnableSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bool enable) {
  return pipe_mgr_idle_tmo_set_enable(sess_hdl, device_id, mat_tbl_hdl, enable);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleRegisterTmoCb(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb cb,
    void *client_data) {
  return pipe_mgr_idle_register_tmo_cb(
      sess_hdl, device_id, mat_tbl_hdl, cb, client_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleRegisterTmoCbWithMatchSpecCopy(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_expiry_cb_with_match_spec_copy cb,
    void *client_data) {
  return pipe_mgr_idle_register_tmo_cb_with_match_spec_copy(
      sess_hdl, device_id, mat_tbl_hdl, cb, client_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeGetHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e *idle_time_data) {
  return pipe_mgr_idle_time_get_hit_state(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, idle_time_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeSetHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_idle_time_hit_state_e idle_time_data) {
  return pipe_mgr_idle_time_set_hit_state(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, idle_time_data);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleTimeUpdateHitState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_tmo_update_complete_cb callback_fn,
    void *cb_data) {
  return pipe_mgr_idle_time_update_hit_state(
      sess_hdl, device_id, mat_tbl_hdl, callback_fn, cb_data);
}

pipe_status_t PipeMgrIntf::pipeMgrMatEntGetIdleTtl(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t *ttl) {
  return pipe_mgr_mat_ent_get_idle_ttl(
      sess_hdl, device_id, mat_tbl_hdl, mat_ent_hdl, ttl);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleParamsGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_time_params_t *params) {
  return pipe_mgr_idle_get_params(sess_hdl, device_id, mat_tbl_hdl, params);
}

pipe_status_t PipeMgrIntf::pipeMgrIdleParamsSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_idle_time_params_t params) {
  return pipe_mgr_idle_set_params(sess_hdl, device_id, mat_tbl_hdl, params);
}

pipe_status_t PipeMgrIntf::pipeStfulEntSet(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_target,
                                           pipe_stful_tbl_hdl_t stful_tbl_hdl,
                                           pipe_stful_mem_idx_t stful_ent_idx,
                                           pipe_stful_mem_spec_t *stful_spec,
                                           uint32_t pipe_api_flags) {
  return pipe_stful_ent_set(sess_hdl,
                            dev_target,
                            stful_tbl_hdl,
                            stful_ent_idx,
                            stful_spec,
                            pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_stful_database_sync(
      sess_hdl, dev_tgt, stful_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectDatabaseSync(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stful_tbl_sync_cback_fn cback_fn,
    void *cookie) {
  return pipe_stful_direct_database_sync(
      sess_hdl, dev_tgt, mat_tbl_hdl, cback_fn, cookie);
}

pipe_status_t PipeMgrIntf::pipeStfulQueryGetSizes(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    int *num_pipes) {
  return pipe_stful_query_get_sizes(
      sess_hdl, device_id, stful_tbl_hdl, num_pipes);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectQueryGetSizes(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    int *num_pipes) {
  return pipe_stful_direct_query_get_sizes(
      sess_hdl, device_id, mat_tbl_hdl, num_pipes);
}

pipe_status_t PipeMgrIntf::pipeStfulEntQuery(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_idx_t stful_ent_idx,
    pipe_stful_mem_query_t *stful_query,
    uint32_t pipe_api_flags) {
  return pipe_stful_ent_query(sess_hdl,
                              dev_tgt,
                              stful_tbl_hdl,
                              stful_ent_idx,
                              stful_query,
                              pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulDirectEntQuery(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_stful_mem_query_t *stful_query,
    uint32_t pipe_api_flags) {
  return pipe_stful_direct_ent_query(sess_hdl,
                                     device_id,
                                     mat_tbl_hdl,
                                     mat_ent_hdl,
                                     stful_query,
                                     pipe_api_flags);
}

pipe_status_t PipeMgrIntf::pipeStfulTableReset(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_spec_t *stful_spec) {
  return pipe_stful_table_reset(sess_hdl, dev_tgt, stful_tbl_hdl, stful_spec);
}

pipe_status_t PipeMgrIntf::pipeStfulTableResetRange(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    pipe_stful_mem_idx_t stful_ent_idx,
    uint32_t num_indices,
    pipe_stful_mem_spec_t *stful_spec) {
  return pipe_stful_table_reset_range(
      sess_hdl, dev_tgt, stful_tbl_hdl, stful_ent_idx, num_indices, stful_spec);
}

pipe_status_t PipeMgrIntf::pipeStfulParamSet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_tbl_hdl_t tbl_hdl,
                                             pipe_reg_param_hdl_t rp_hdl,
                                             int64_t value) {
  return pipe_stful_param_set(sess_hdl, dev_tgt, tbl_hdl, rp_hdl, value);
}

pipe_status_t PipeMgrIntf::pipeStfulParamGet(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_tbl_hdl_t tbl_hdl,
                                             pipe_reg_param_hdl_t rp_hdl,
                                             int64_t *value) {
  return pipe_stful_param_get(sess_hdl, dev_tgt, tbl_hdl, rp_hdl, value);
}

pipe_status_t PipeMgrIntf::pipeStfulParamReset(pipe_sess_hdl_t sess_hdl,
                                               dev_target_t dev_tgt,
                                               pipe_tbl_hdl_t tbl_hdl,
                                               pipe_reg_param_hdl_t rp_hdl) {
  return pipe_stful_param_reset(sess_hdl, dev_tgt, tbl_hdl, rp_hdl);
}

pipe_status_t PipeMgrIntf::pipeStfulParamGetHdl(bf_dev_id_t dev,
                                                const char *name,
                                                pipe_reg_param_hdl_t *rp_hdl) {
  return pipe_stful_param_get_hdl(dev, name, rp_hdl);
}

bf_dev_pipe_t PipeMgrIntf::devPortToPipeId(uint16_t dev_port_id) {
  return dev_port_to_pipe_id(dev_port_id);
}

pipe_status_t PipeMgrIntf::pipeMgrGetFirstEntryHandle(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    int *entry_handle) {
  return pipe_mgr_get_first_entry_handle(
      sess_hdl, tbl_hdl, dev_tgt, entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextEntryHandles(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_handle,
    int n,
    int *next_entry_handles) {
  return pipe_mgr_get_next_entry_handles(
      sess_hdl, tbl_hdl, dev_tgt, entry_handle, n, next_entry_handles);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextEntries(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    size_t n,
    bool from_hw,
    uint32_t *res_get_flags,
    pipe_tbl_match_spec_t *pipe_match_specs,
    pipe_action_spec_t **pipe_action_specs,
    pipe_act_fn_hdl_t *act_fn_hdls,
    pipe_res_get_data_t *res_data,
    pipe_mat_ent_hdl_t *last_ent_hdl,
    uint32_t *num_returned) {
  return pipe_mgr_get_n_next_entries(sess_hdl,
                                     tbl_hdl,
                                     dev_tgt,
                                     entry_hdl,
                                     n,
                                     from_hw,
                                     res_get_flags,
                                     pipe_match_specs,
                                     pipe_action_specs,
                                     act_fn_hdls,
                                     res_data,
                                     last_ent_hdl,
                                     num_returned);
}

pipe_status_t PipeMgrIntf::pipeMgrGetFirstGroupMember(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t *mbr_hdl) {
  return pipe_mgr_get_first_group_member(
      sess_hdl, tbl_hdl, dev_id, sel_grp_hdl, mbr_hdl);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNextGroupMembers(
    pipe_sess_hdl_t sess_hdl,
    pipe_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    pipe_adt_ent_hdl_t mbr_hdl,
    int n,
    pipe_adt_ent_hdl_t *next_mbr_hdls) {
  return pipe_mgr_get_next_group_members(
      sess_hdl, tbl_hdl, dev_id, sel_grp_hdl, mbr_hdl, n, next_mbr_hdls);
}

pipe_status_t PipeMgrIntf::pipeMgrGetSelGrpMbrCount(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint32_t *count) {
  return pipe_mgr_get_sel_grp_mbr_count(
      sess_hdl, dev_id, tbl_hdl, sel_grp_hdl, count);
}

pipe_status_t PipeMgrIntf::pipeMgrSelGrpParamsGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_sel_tbl_hdl_t tbl_hdl,
    pipe_sel_grp_hdl_t sel_grp_hdl,
    uint32_t *max_size,
    uint32_t *adt_offset) {
  return pipe_mgr_get_sel_grp_params(
      sess_hdl, dev_id, tbl_hdl, sel_grp_hdl, max_size, adt_offset);
}

pipe_status_t PipeMgrIntf::pipeMgrGetReservedEntryCount(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    size_t *count) {
  return pipe_mgr_get_reserved_entry_count(sess_hdl, dev_tgt, tbl_hdl, count);
}

pipe_status_t PipeMgrIntf::pipeMgrGetTotalHwEntryCount(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t tbl_hdl,
    size_t *count) {
  return pipe_mgr_get_total_hw_entry_count(sess_hdl, dev_tgt, tbl_hdl, count);
}

pipe_status_t PipeMgrIntf::pipeMgrGetEntryCount(pipe_sess_hdl_t sess_hdl,
                                                dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t tbl_hdl,
                                                bool read_from_hw,
                                                uint32_t *count) {
  return pipe_mgr_get_entry_count(
      sess_hdl, dev_tgt, tbl_hdl, read_from_hw, count);
}

pipe_status_t PipeMgrIntf::pipeMgrGetEntry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw,
    uint32_t res_get_flags,
    pipe_res_get_data_t *res_data) {
  return pipe_mgr_get_entry(sess_hdl,
                            tbl_hdl,
                            dev_tgt,
                            entry_hdl,
                            pipe_match_spec,
                            pipe_action_spec,
                            act_fn_hdl,
                            from_hw,
                            res_get_flags,
                            res_data);
}

pipe_status_t PipeMgrIntf::pipeMgrGetActionDataEntry(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    pipe_action_data_spec_t *pipe_action_data_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool from_hw) {
  return pipe_mgr_get_action_data_entry(
      tbl_hdl, dev_tgt, entry_hdl, pipe_action_data_spec, act_fn_hdl, from_hw);
}

pipe_status_t PipeMgrIntf::pipeMgrTblSetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_tbl_prop_type_t property,
    pipe_mgr_tbl_prop_value_t value,
    pipe_mgr_tbl_prop_args_t args) {
  return pipe_mgr_tbl_set_property(
      sess_hdl, dev_id, tbl_hdl, property, value, args);
}

pipe_status_t PipeMgrIntf::pipeMgrTblGetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    pipe_mgr_tbl_prop_type_t property,
    pipe_mgr_tbl_prop_value_t *value,
    pipe_mgr_tbl_prop_args_t *args) {
  return pipe_mgr_tbl_get_property(
      sess_hdl, dev_id, tbl_hdl, property, value, args);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryAdd(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    bf_dev_direction_t gress,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    uint32_t parser_value,
    uint32_t parser_mask,
    pipe_pvs_hdl_t *pvs_entry_handle) {
  return pipe_mgr_pvs_entry_add(sess_hdl,
                                dev_id,
                                pvs_handle,
                                gress,
                                pipeid,
                                parser_id,
                                parser_value,
                                parser_mask,
                                pvs_entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryModify(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_pvs_hdl_t pvs_entry_handle,
    uint32_t parser_value,
    uint32_t parser_mask) {
  return pipe_mgr_pvs_entry_modify(sess_hdl,
                                   dev_id,
                                   pvs_handle,
                                   pvs_entry_handle,
                                   parser_value,
                                   parser_mask);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryDelete(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_pvs_hdl_t pvs_entry_handle) {
  return pipe_mgr_pvs_entry_delete(
      sess_hdl, dev_id, pvs_handle, pvs_entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsClear(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_pvs_hdl_t pvs_handle,
                                           bf_dev_direction_t gress,
                                           bf_dev_pipe_t pipeid,
                                           uint8_t parser_id) {
  return pipe_mgr_pvs_table_clear(
      sess_hdl, dev_id, pvs_handle, gress, pipeid, parser_id);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryGet(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_pvs_hdl_t pvs_handle,
                                              pipe_pvs_hdl_t pvs_entry_handle,
                                              uint32_t *parser_value,
                                              uint32_t *parser_value_mask,
                                              uint8_t *entry_gress,
                                              bf_dev_pipe_t *entry_pipe,
                                              uint8_t *entry_parser_id) {
  return pipe_mgr_pvs_entry_get(sess_hdl,
                                dev_id,
                                pvs_handle,
                                pvs_entry_handle,
                                parser_value,
                                parser_value_mask,
                                entry_gress,
                                entry_pipe,
                                entry_parser_id);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryGetHw(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                bf_dev_direction_t gress,
                                                bf_dev_pipe_t pipeid,
                                                uint8_t parser_id,
                                                pipe_pvs_hdl_t pvs_handle,
                                                pipe_pvs_hdl_t pvs_entry_handle,
                                                uint32_t *parser_value,
                                                uint32_t *parser_value_mask) {
  return pipe_mgr_pvs_entry_hw_get(sess_hdl,
                                   dev_id,
                                   gress,
                                   pipeid,
                                   parser_id,
                                   pvs_handle,
                                   pvs_entry_handle,
                                   parser_value,
                                   parser_value_mask);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryHandleGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    bf_dev_direction_t gress,
    bf_dev_pipe_t pipeid,
    uint8_t parser_id,
    uint32_t parser_value,
    uint32_t parser_mask,
    pipe_pvs_hdl_t *pvs_entry_handle) {
  return pipe_mgr_pvs_entry_handle_get(sess_hdl,
                                       dev_id,
                                       pvs_handle,
                                       gress,
                                       pipeid,
                                       parser_id,
                                       parser_value,
                                       parser_mask,
                                       pvs_entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryGetFirst(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    bf_dev_direction_t gress,
    bf_dev_pipe_t pipe_id,
    uint8_t parser_id,
    pipe_pvs_hdl_t *entry_handle) {
  return pipe_mgr_pvs_entry_get_first(
      sess_hdl, dev_id, pvs_handle, gress, pipe_id, parser_id, entry_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryGetNext(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t devid,
    pipe_pvs_hdl_t pvs_handle,
    bf_dev_direction_t gress,
    bf_dev_pipe_t pipe_id,
    uint8_t parser_id,
    pipe_pvs_hdl_t entry_handle,
    int n,
    pipe_pvs_hdl_t *next_handles) {
  return pipe_mgr_pvs_entry_get_next(sess_hdl,
                                     devid,
                                     pvs_handle,
                                     gress,
                                     pipe_id,
                                     parser_id,
                                     entry_handle,
                                     n,
                                     next_handles);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsEntryGetCount(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_id_t devid,
                                                   pipe_pvs_hdl_t pvs_handle,
                                                   bf_dev_direction_t gress,
                                                   bf_dev_pipe_t pipe_id,
                                                   uint8_t parser_id,
                                                   bool from_hw,
                                                   uint32_t *count) {
  return pipe_mgr_pvs_entry_get_count(
      sess_hdl, devid, pvs_handle, gress, pipe_id, parser_id, from_hw, count);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsSetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t value,
    pipe_mgr_pvs_prop_args_t args) {
  return pipe_mgr_pvs_set_property(
      sess_hdl, dev_id, pvs_handle, property, value, args);
}

pipe_status_t PipeMgrIntf::pipeMgrPvsGetProperty(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_pvs_hdl_t pvs_handle,
    pipe_mgr_pvs_prop_type_t property,
    pipe_mgr_pvs_prop_value_t *value,
    pipe_mgr_pvs_prop_args_t args) {
  return pipe_mgr_pvs_get_property(
      sess_hdl, dev_id, pvs_handle, property, value, args);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle) {
  return pipe_mgr_hash_calc_input_set(sess_hdl, dev_id, handle, fl_handle);
}
pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputDefaultSet(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_hash_calc_hdl_t handle) {
  return pipe_mgr_hash_calc_input_default_set(sess_hdl, dev_id, handle);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t *fl_handle) {
  return pipe_mgr_hash_calc_input_get(sess_hdl, dev_id, handle, fl_handle);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputFieldAttrSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list) {
  return pipe_mgr_hash_calc_input_field_attribute_set(
      sess_hdl, dev_id, handle, fl_handle, attr_count, attr_list);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputFieldAttributeGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t max_attr_count,
    pipe_hash_calc_input_field_attribute_t *attr_list,
    uint32_t *num_attr_filled) {
  return pipe_mgr_hash_calc_input_field_attribute_get(sess_hdl,
                                                      dev_id,
                                                      handle,
                                                      fl_handle,
                                                      max_attr_count,
                                                      attr_list,
                                                      num_attr_filled);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputFieldAttribute2Get(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    pipe_hash_calc_input_field_attribute_t **attr_list,
    uint32_t *num_attr_filled) {
  return pipe_mgr_hash_calc_input_field_attribute_2_get(
      sess_hdl, dev_id, handle, fl_handle, attr_list, num_attr_filled);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcAttributeListDestroy(
    pipe_hash_calc_input_field_attribute_t *attr_list) {
  return pipe_mgr_hash_calc_attribute_list_destroy(attr_list);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcInputFieldAttrCountGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_fld_lst_hdl_t fl_handle,
    uint32_t *attr_count) {
  return pipe_mgr_hash_calc_input_field_attribute_count_get(
      sess_hdl, dev_id, handle, fl_handle, attr_count);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcAlgorithmSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_hash_alg_hdl_t al_handle,
    const bfn_hash_algorithm_t *algorithm,
    uint64_t rotate) {
  return pipe_mgr_hash_calc_algorithm_set(
      sess_hdl, dev_id, handle, al_handle, algorithm, rotate);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcAlgorithmGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    pipe_hash_alg_hdl_t *al_handle,
    bfn_hash_algorithm_t *algorithm,
    uint64_t *rotate) {
  return pipe_mgr_hash_calc_algorithm_get(
      sess_hdl, dev_id, handle, al_handle, algorithm, rotate);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcSeedSet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_hash_calc_hdl_t handle,
                                                  pipe_hash_seed_t seed) {
  return pipe_mgr_hash_calc_seed_set(sess_hdl, dev_id, handle, seed);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcSeedGet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_id_t dev_id,
                                                  pipe_hash_calc_hdl_t handle,
                                                  pipe_hash_seed_t *seed) {
  return pipe_mgr_hash_calc_seed_get(sess_hdl, dev_id, handle, seed);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcAlgorithmReset(
    pipe_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, pipe_hash_calc_hdl_t handle) {
  return pipe_mgr_hash_calc_algorithm_reset(sess_hdl, dev_id, handle);
}

pipe_status_t PipeMgrIntf::pipeMgrHashCalcCalculateHashValueWithCfg(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_hash_calc_hdl_t handle,
    uint32_t attr_count,
    pipe_hash_calc_input_field_attribute_t *attrs,
    uint32_t hash_len,
    uint8_t *hash) {
  return pipe_mgr_hash_calc_calculate_hash_value_with_cfg(
      sess_hdl, dev_id, handle, attr_count, attrs, hash_len, hash);
}

bf_dev_pipe_t PipeMgrIntf::pipeGetHdlPipe(pipe_mat_ent_hdl_t entry_hdl) {
  return PIPE_GET_HDL_PIPE(entry_hdl);
}

/************ Snapshot APIs **************/

bf_status_t PipeMgrIntf::bfSnapshotMonitoringMode(
    bf_dev_id_t dev_id,
    bool interrupt_or_polling,
    bf_snapshot_triggered_cb trig_cb) {
  return bf_snapshot_monitoring_mode(dev_id, interrupt_or_polling, trig_cb);
}

bf_status_t PipeMgrIntf::bfSnapshotDoPolling(bf_dev_id_t dev) {
  return bf_snapshot_do_polling(dev);
}

bf_status_t PipeMgrIntf::bfSnapshotCreate(bf_dev_id_t dev,
                                          bf_dev_pipe_t pipe,
                                          dev_stage_t start_stage,
                                          dev_stage_t end_stage,
                                          bf_snapshot_dir_t dir,
                                          pipe_snapshot_hdl_t *hdl) {
  return bf_snapshot_create(dev, pipe, start_stage, end_stage, dir, hdl);
}

bf_status_t PipeMgrIntf::bfSnapshotHandleGet(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             dev_stage_t start_stage,
                                             dev_stage_t end_stage,
                                             bf_snapshot_dir_t *dir,
                                             pipe_snapshot_hdl_t *hdl) {
  return bf_snapshot_handle_get(dev, pipe, start_stage, end_stage, dir, hdl);
}

bf_status_t PipeMgrIntf::bfSnapshotNextEntryHandlesGet(
    bf_dev_id_t dev_id,
    pipe_snapshot_hdl_t hdl,
    int n,
    int *next_entry_handles) {
  return bf_snapshot_next_handles_get(dev_id, hdl, n, next_entry_handles);
}

bf_status_t PipeMgrIntf::bfSnapshotDelete(pipe_snapshot_hdl_t hdl) {
  return bf_snapshot_delete(hdl);
}

bf_status_t PipeMgrIntf::bfSnapshotClear(bf_dev_id_t dev) {
  return bf_snapshot_clear(dev);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureTriggerSet(pipe_snapshot_hdl_t hdl,
                                                     void *trig_spec,
                                                     void *trig_mask) {
  return bf_snapshot_capture_trigger_set(hdl, trig_spec, trig_mask);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureTriggerFieldAdd(
    pipe_snapshot_hdl_t hdl, char *field_name, uint64_t value, uint64_t mask) {
  return bf_snapshot_capture_trigger_field_add(hdl, field_name, value, mask);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureTriggerFieldGet(
    pipe_snapshot_hdl_t hdl,
    char *field_name,
    uint64_t *value,
    uint64_t *mask) {
  return bf_snapshot_capture_trigger_field_get(hdl, field_name, value, mask);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureTriggerFieldsClr(
    pipe_snapshot_hdl_t hdl) {
  return bf_snapshot_capture_trigger_fields_clr(hdl);
}

bf_status_t PipeMgrIntf::bfSnapshotCapturePhvFieldsDictSize(
    pipe_snapshot_hdl_t hdl, uint32_t *total_size, uint32_t *per_stage_size) {
  return bf_snapshot_capture_phv_fields_dict_size(
      hdl, total_size, per_stage_size);
}

bf_status_t PipeMgrIntf::bfSnapshotFirstHandleGet(
    bf_dev_id_t dev_id, bf_dev_pipe_t pipe, pipe_snapshot_hdl_t *entry_hdl) {
  return bf_snapshot_first_handle_get(dev_id, pipe, entry_hdl);
}

bf_status_t PipeMgrIntf::bfSnapshotUsageGet(bf_dev_id_t dev_id,
                                            bf_dev_pipe_t pipe,
                                            uint32_t *count) {
  return bf_snapshot_usage_get(dev_id, pipe, count);
}

bf_status_t PipeMgrIntf::bfSnapshotTotalPhvCountGet(bf_dev_id_t dev_id,
                                                    uint32_t *count) {
  return bf_snapshot_total_phv_count_get(dev_id, count);
}

bf_status_t PipeMgrIntf::bfSnapshotStagesGet(bf_dev_id_t dev_id,
                                             bf_dev_pipe_t pipe,
                                             uint32_t size,
                                             int *stages) {
  return bf_snapshot_stages_get(dev_id, pipe, size, stages);
}

bf_status_t PipeMgrIntf::bfSnapshotRawCaptureGet(bf_dev_id_t dev_id,
                                                 bf_dev_pipe_t pipe,
                                                 dev_stage_t stage,
                                                 uint32_t size,
                                                 uint32_t *fields,
                                                 bool *fields_v) {
  return bf_snapshot_raw_capture_get(
      dev_id, pipe, stage, size, fields, fields_v);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureGet(
    pipe_snapshot_hdl_t hdl,
    bf_dev_pipe_t pipe,
    uint8_t *capture,
    bf_snapshot_capture_ctrl_info_arr_t *ctrl_info_arr,
    int *num_captures) {
  return bf_snapshot_capture_get(
      hdl, pipe, capture, ctrl_info_arr, num_captures);
}

bf_status_t PipeMgrIntf::bfSnapshotCaptureDecodeFieldValue(
    pipe_snapshot_hdl_t hdl,
    bf_dev_pipe_t pipe,
    dev_stage_t stage,
    uint8_t *capture,
    int num_captures,
    char *field_name,
    uint64_t *field_value,
    bool *field_valid) {
  return bf_snapshot_capture_decode_field_value(hdl,
                                                pipe,
                                                stage,
                                                capture,
                                                num_captures,
                                                field_name,
                                                field_value,
                                                field_valid);
}

bf_status_t PipeMgrIntf::bfSnapshotStateSet(pipe_snapshot_hdl_t hdl,
                                            bf_snapshot_state_t state,
                                            uint32_t usec) {
  return bf_snapshot_state_set(hdl, state, usec);
}

bf_status_t PipeMgrIntf::bfSnapshotStateGet(
    pipe_snapshot_hdl_t hdl,
    uint32_t size,
    pipe_snapshot_fsm_state_t *fsm_state,
    bool *state) {
  return bf_snapshot_state_get(hdl, size, fsm_state, state);
}

bf_status_t PipeMgrIntf::bfSnapshotCfgSet(pipe_snapshot_hdl_t hdl,
                                          bool timer_disable,
                                          bf_snapshot_ig_mode_t mode) {
  return bf_snapshot_cfg_set(hdl, timer_disable, mode);
}

bf_status_t PipeMgrIntf::bfSnapshotCfgGet(pipe_snapshot_hdl_t hdl,
                                          bool *timer_enabled,
                                          uint32_t *usec,
                                          bf_snapshot_ig_mode_t *mode) {
  return bf_snapshot_cfg_get(hdl, timer_enabled, usec, mode);
}

bf_status_t PipeMgrIntf::bfSnapshotThreadGet(pipe_snapshot_hdl_t hdl,
                                             uint32_t size,
                                             int *threads) {
  return bf_snapshot_capture_thread_get(hdl, size, threads);
}

bf_status_t PipeMgrIntf::bfSnapshotFieldInScope(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                dev_stage_t stage,
                                                bf_snapshot_dir_t dir,
                                                char *field_name,
                                                bool *exists) {
  return bf_snapshot_field_in_scope(dev, pipe, stage, dir, field_name, exists);
}

bf_status_t PipeMgrIntf::bfSnapshotTriggerFieldInScope(bf_dev_id_t dev,
                                                       bf_dev_pipe_t pipe,
                                                       dev_stage_t stage,
                                                       bf_snapshot_dir_t dir,
                                                       char *field_name,
                                                       bool *exists) {
  return bf_snapshot_trigger_field_in_scope(
      dev, pipe, stage, dir, field_name, exists);
}

bf_status_t PipeMgrIntf::bfSnapshotEntryParamsGet(pipe_snapshot_hdl_t hdl,
                                                  bf_dev_id_t *dev,
                                                  bf_dev_pipe_t *pipe,
                                                  dev_stage_t *s_stage,
                                                  dev_stage_t *e_stage,
                                                  bf_snapshot_dir_t *dir) {
  return bf_snapshot_entry_params_get(hdl, dev, pipe, s_stage, e_stage, dir);
}

bf_status_t PipeMgrIntf::bfSnapshotNumTrigFieldsGet(pipe_snapshot_hdl_t hdl,
                                                    int *count) {
  return bf_snapshot_num_trig_fields_get(hdl, count);
}

/************ Table Debug Counters APIs **************/
bf_status_t PipeMgrIntf::bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                         char *tbl_name,
                                         bf_tbl_dbg_counter_type_t *type,
                                         uint32_t *value) {
  return bf_tbl_dbg_counter_get(dev_tgt, tbl_name, type, value);
}

bf_status_t PipeMgrIntf::bfDbgCounterGet(bf_dev_target_t dev_tgt,
                                         uint32_t stage,
                                         uint32_t log_tbl,
                                         bf_tbl_dbg_counter_type_t *type,
                                         uint32_t *value) {
  return bf_log_tbl_dbg_counter_get(dev_tgt, stage, log_tbl, type, value);
}

bf_status_t PipeMgrIntf::bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                         char *tbl_name,
                                         bf_tbl_dbg_counter_type_t type) {
  return bf_tbl_dbg_counter_type_set(dev_tgt, tbl_name, type);
}

bf_status_t PipeMgrIntf::bfDbgCounterSet(bf_dev_target_t dev_tgt,
                                         uint32_t stage,
                                         uint32_t log_tbl,
                                         bf_tbl_dbg_counter_type_t type) {
  return bf_log_tbl_dbg_counter_type_set(dev_tgt, stage, log_tbl, type);
}

bf_status_t PipeMgrIntf::bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                           char *tbl_name) {
  return bf_tbl_dbg_counter_clear(dev_tgt, tbl_name);
}

bf_status_t PipeMgrIntf::bfDbgCounterClear(bf_dev_target_t dev_tgt,
                                           uint32_t stage,
                                           uint32_t log_tbl) {
  return bf_log_tbl_dbg_counter_clear(dev_tgt, stage, log_tbl);
}

bf_status_t PipeMgrIntf::bfDbgCounterTableListGet(bf_dev_target_t dev_tgt,
                                                  char **tbl_list,
                                                  int *num_tbls) {
  return pipe_mgr_tbl_dbg_counter_get_list(dev_tgt, tbl_list, num_tbls);
}

pipe_status_t PipeMgrIntf::pipeMgrHitlessHaRestoreVirtualDevState(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_tbl_hdl_t tbl_hdl,
    pipe_plcmt_info *info,
    uint32_t *processed,
    pd_ha_restore_cb_1 cb1) {
  return pipe_mgr_hitless_ha_restore_virtual_dev_state(
      sess_hdl, dev_id, tbl_hdl, info, processed, cb1);
}

bf_status_t PipeMgrIntf::pipeRegisterMatUpdateCb(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_mat_tbl_hdl_t tbl_hdl,
                                                 pipe_mat_update_cb cb,
                                                 void *cb_cookie) const {
  return pipe_register_mat_update_cb(
      sess_hdl, device_id, tbl_hdl, cb, cb_cookie);
}
bf_status_t PipeMgrIntf::pipeRegisterAdtUpdateCb(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_adt_tbl_hdl_t tbl_hdl,
                                                 pipe_adt_update_cb cb,
                                                 void *cb_cookie) const {
  return pipe_register_adt_update_cb(
      sess_hdl, device_id, tbl_hdl, cb, cb_cookie);
}
bf_status_t PipeMgrIntf::pipeRegisterSelUpdateCb(pipe_sess_hdl_t sess_hdl,
                                                 bf_dev_id_t device_id,
                                                 pipe_sel_tbl_hdl_t tbl_hdl,
                                                 pipe_sel_update_cb cb,
                                                 void *cb_cookie) const {
  return pipe_register_sel_update_cb(
      sess_hdl, device_id, tbl_hdl, cb, cb_cookie);
}

pipe_status_t PipeMgrIntf::pipeSetAdtEntHdlInMatData(
    void *data, pipe_adt_ent_hdl_t adt_ent_hdl) {
  return pipe_set_adt_ent_hdl_in_mat_data(data, adt_ent_hdl);
}

pipe_status_t PipeMgrIntf::pipeSetSelGrpHdlInMatData(
    void *data, pipe_adt_ent_hdl_t sel_grp_hdl) {
  return pipe_set_sel_grp_hdl_in_mat_data(data, sel_grp_hdl);
}

pipe_status_t PipeMgrIntf::pipeSetTtlInMatData(void *data, uint32_t ttl) {
  return pipe_set_ttl_in_mat_data(data, ttl);
}

pipe_status_t PipeMgrIntf::pipeMgrTcamScrubTimerSet(bf_dev_id_t dev,
                                                    uint32_t msec_timer) {
  return pipe_mgr_tcam_scrub_timer_set(dev, msec_timer);
}

uint32_t PipeMgrIntf::pipeMgrTcamScrubTimerGet(bf_dev_id_t dev) {
  return pipe_mgr_tcam_scrub_timer_get(dev);
}

bf_status_t PipeMgrIntf::pipeMgrTblHdlPipeMaskGet(
    bf_dev_id_t dev_id,
    const std::string &prog_name,
    const std::string &pipeline_name,
    uint32_t *pipe_mask) const {
  return pipe_mgr_tbl_hdl_pipe_mask_get(
      dev_id, prog_name.c_str(), pipeline_name.c_str(), pipe_mask);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNumPipelines(bf_dev_id_t dev_id,
                                                  uint32_t *num_pipes) const {
  return pipe_mgr_get_num_pipelines(dev_id, num_pipes);
}

pipe_status_t PipeMgrIntf::pipeMgrGetNumActiveStages(
    bf_dev_id_t dev_id, uint8_t *num_active_stages) const {
  return pipe_mgr_get_num_active_stages(dev_id, num_active_stages);
}

/*************** DVM APIs ***************/
pipe_status_t PipeMgrIntf::pipeMgrLrtDrTimeoutSet(bf_dev_id_t dev_id,
                                                  uint32_t timeout_us) {
  return bf_drv_lrt_dr_timeout_set_us(dev_id, timeout_us);
}

pipe_status_t PipeMgrIntf::pipeMgrLrtDrTimeoutGet(bf_dev_id_t dev_id,
                                                  uint32_t *timeout_us) const {
  return bf_drv_lrt_dr_timeout_get_us(dev_id, timeout_us);
}

/*************** Pktgen APIs ***************/
bf_status_t PipeMgrIntf::pipeMgrRecirEnableSet(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool enable) {
  return bf_recirculation_set(sess_hdl, dev_id, dev_port, enable);
}

bf_status_t PipeMgrIntf::pipeMgrRecirEnableGet(bf_dev_id_t dev_id,
                                               bf_dev_port_t dev_port,
                                               bool *enable) const {
  return (bf_recirculation_get(dev_id, dev_port, enable));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenEnableSet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                bool enable) {
  return (bf_pktgen_enable_set(sess_hdl, dev_id, dev_port, enable));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenEnableGet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                bf_dev_port_t dev_port,
                                                bool *enable) const {
  return bf_pktgen_enable_get(sess_hdl, dev_id, dev_port, enable);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenRecirPatternMatchingEnableSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bool enable) {
  return bf_pktgen_recirc_pattern_matching_set(
      sess_hdl, dev_id, dev_port, enable);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenRecirPatternMatchingEnableGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bool *enable) const {
  return bf_pktgen_recirc_pattern_matching_get(
      sess_hdl, dev_id, dev_port, enable);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenClearPortDownSet(pipe_sess_hdl_t sess_hdl,
                                                       bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port) {
  return (bf_pktgen_clear_port_down(sess_hdl, dev_id, dev_port));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenClearPortDownGet(pipe_sess_hdl_t sess_hdl,
                                                       bf_dev_id_t dev_id,
                                                       bf_dev_port_t dev_port,
                                                       bool *cleared) {
  return (bf_pktgen_port_down_get(sess_hdl, dev_id, dev_port, cleared));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPortDownMaskSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *mask) {
  return (bf_pktgen_cfg_port_down_mask(sess_hdl, dev_tgt, port_mask_sel, mask));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPortDownMaskGet(
    bf_dev_target_t dev_tgt,
    uint32_t port_mask_sel,
    struct bf_tof2_port_down_sel *mask) const {
  return (bf_pktgen_port_down_mask_get(dev_tgt, port_mask_sel, mask));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPortDownReplayModeSet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode) {
  return (bf_pktgen_port_down_replay_mode_set(sess_hdl, dev_tgt, mode));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPortDownReplayModeGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode) const {
  return (bf_pktgen_port_down_replay_mode_get(sess_hdl, dev_tgt, mode));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenAppSet(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_target_t dev_tgt,
                                             int app_id,
                                             bf_pktgen_app_cfg_t *cfg) {
  return (bf_pktgen_cfg_app(sess_hdl, dev_tgt, app_id, cfg));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenAppGet(pipe_sess_hdl_t sess_hdl,
                                             bf_dev_target_t dev_tgt,
                                             int app_id,
                                             bf_pktgen_app_cfg_t *cfg) const {
  return bf_pktgen_cfg_app_get(sess_hdl, dev_tgt, app_id, cfg);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenAppEnableSet(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_target_t dev_tgt,
                                                   int app_id,
                                                   bool enable) {
  return bf_pktgen_app_set(sess_hdl, dev_tgt, app_id, enable);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenAppEnableGet(pipe_sess_hdl_t sess_hdl,
                                                   bf_dev_target_t dev_tgt,
                                                   int app_id,
                                                   bool *enable) const {
  return bf_pktgen_app_get(sess_hdl, dev_tgt, app_id, enable);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenWritePktBuffer(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    uint32_t pktgen_byte_buf_offset,
    uint32_t size,
    const uint8_t *buf) {
  return (bf_pktgen_write_pkt_buffer(
      sess_hdl, dev_tgt, pktgen_byte_buf_offset, size, buf));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenClearPktBuffer(pipe_sess_hdl_t sess_hdl,
                                                     bf_dev_target_t dev_tgt) {
  return (bf_pktgen_clear_pkt_buffer(sess_hdl, dev_tgt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPktBufferGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    uint32_t pktgen_byte_buf_offset,
    uint32_t size,
    uint8_t *buf) const {
  return bf_pktgen_pkt_buffer_get(
      sess_hdl, dev_tgt, pktgen_byte_buf_offset, size, buf);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenBatchCntSet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int app_id,
                                                  uint64_t batch_cnt) {
  return (bf_pktgen_set_batch_counter(sess_hdl, dev_tgt, app_id, batch_cnt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPktCntSet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_target_t dev_tgt,
                                                int app_id,
                                                uint64_t pkt_cnt) {
  return (bf_pktgen_set_pkt_counter(sess_hdl, dev_tgt, app_id, pkt_cnt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenTriggerCntSet(pipe_sess_hdl_t sess_hdl,
                                                    bf_dev_target_t dev_tgt,
                                                    int app_id,
                                                    uint64_t trigger_cnt) {
  return (
      bf_pktgen_set_trigger_counter(sess_hdl, dev_tgt, app_id, trigger_cnt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenBatchCntGet(pipe_sess_hdl_t sess_hdl,
                                                  bf_dev_target_t dev_tgt,
                                                  int app_id,
                                                  uint64_t *batch_cnt) const {
  return (bf_pktgen_get_batch_counter(sess_hdl, dev_tgt, app_id, batch_cnt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPktCntGet(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_target_t dev_tgt,
                                                int app_id,
                                                uint64_t *pkt_cnt) const {
  return (bf_pktgen_get_pkt_counter(sess_hdl, dev_tgt, app_id, pkt_cnt));
}

bf_status_t PipeMgrIntf::pipeMgrPktgenTriggerCntGet(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_target_t dev_tgt,
    int app_id,
    uint64_t *trigger_cnt) const {
  return (
      bf_pktgen_get_trigger_counter(sess_hdl, dev_tgt, app_id, trigger_cnt));
}

uint32_t PipeMgrIntf::pipeMgrPktgenAppCountGet(bf_dev_id_t dev_id) const {
  return bf_pktgen_get_app_count(dev_id);
}

bf_dev_port_t PipeMgrIntf::pipeMgrPktgenPortGet(bf_dev_id_t dev_id) const {
  return bf_pktgen_port_get(dev_id);
}

bf_status_t PipeMgrIntf::pipeMgrPktgenPortGetNext(bf_dev_id_t dev_id,
                                                  bf_dev_port_t *port) const {
  return bf_pktgen_get_next_port(dev_id, port);
}

bf_dev_port_t PipeMgrIntf::pipeMgrPktgenMaxPortGet(bf_dev_id_t dev_id) const {
  return bf_pktgen_max_port_get(dev_id);
}

// ***************** Mirror APIs ***************** //
bf_status_t PipeMgrIntf::pipeMgrMirrorSessionSet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_session_info_t *s_info,
    bool enable) {
  return bf_mirror_session_set(sess_hdl, dev_target, sid, s_info, enable);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionReset(pipe_sess_hdl_t sess_hdl,
                                                   dev_target_t dev_target,
                                                   bf_mirror_id_t sid) {
  return bf_mirror_session_reset(sess_hdl, dev_target, sid);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionEnable(pipe_sess_hdl_t sess_hdl,
                                                    dev_target_t dev_target,
                                                    bf_mirror_direction_e dir,
                                                    bf_mirror_id_t sid) {
  return bf_mirror_session_enable(sess_hdl, dev_target, dir, sid);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionDisable(pipe_sess_hdl_t sess_hdl,
                                                     dev_target_t dev_target,
                                                     bf_mirror_direction_e dir,
                                                     bf_mirror_id_t sid) {
  return bf_mirror_session_disable(sess_hdl, dev_target, dir, sid);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionMetaFlagUpdate(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool value) {
  return bf_mirror_session_meta_flag_update(
      sess_hdl, dev_target, sid, mirror_flag, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionMetaFlagGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_meta_flag_e mirror_flag,
    bool *value) {
  return bf_mirror_session_meta_flag_get(
      sess_hdl, dev_target, sid, mirror_flag, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionPriorityUpdate(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool value) {
  return bf_mirror_session_priority_update(sess_hdl, dev_target, sid, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionPriorityGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool *value) {
  return bf_mirror_session_priority_get(sess_hdl, dev_target, sid, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionCoalModeUpdate(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool value) {
  return bf_mirror_session_coal_mode_update(sess_hdl, dev_target, sid, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionCoalModeGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool *value) {
  return bf_mirror_session_coal_mode_get(sess_hdl, dev_target, sid, value);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bf_mirror_session_info_t *s_info) const {
  return bf_mirror_session_get(sess_hdl, dev_target, sid, s_info);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionGetFirst(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_session_info_t *s_info,
    bf_mirror_get_id_t *first) const {
  return bf_mirror_session_get_first(sess_hdl, dev_target, s_info, first);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionGetNext(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_get_id_t current,
    bf_mirror_session_info_t *next_info,
    bf_mirror_get_id_t *next) const {
  return bf_mirror_session_get_next(
      sess_hdl, dev_target, current, next_info, next);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionEnableGet(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_target,
    bf_mirror_id_t sid,
    bool *session_enable) const {
  return bf_mirror_session_enable_get(
      sess_hdl, dev_target, sid, session_enable);
}

bf_status_t PipeMgrIntf::pipeMgrMirrorSessionCountGet(pipe_sess_hdl_t sess_hdl,
                                                      dev_target_t dev_target,
                                                      uint32_t *count) const {
  return bf_mirror_session_get_count(sess_hdl, dev_target, count);
}

bf_status_t PipeMgrIntf::pipeMgrPortRXPrsrPriThreshSet(bf_dev_id_t dev_id,
                                                       bf_dev_port_t port_id,
                                                       uint32_t threshold) {
  return bf_pipe_mgr_port_iprsr_threshold_set(dev_id, port_id, threshold);
}

bf_status_t PipeMgrIntf::pipeMgrPortRXPrsrPriThreshGet(
    bf_dev_id_t dev_id, bf_dev_port_t port_id, uint32_t *threshold) const {
  return bf_pipe_mgr_port_iprsr_threshold_get(dev_id, port_id, threshold);
}
}  // namespace bfrt
