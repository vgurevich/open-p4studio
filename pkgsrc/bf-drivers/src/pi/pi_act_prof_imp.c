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


#include <PI/int/pi_int.h>
#include <PI/int/serialize.h>
#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_act_prof_imp.h>

#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_select_tbl.h>

#include "pi_allocators.h"
#include "pi_helpers.h"
#include "pi_log.h"
#include "pi_state.h"

#include <inttypes.h>

static pi_p4_id_t act_prof_get_one_table_id(const pi_p4info_t *p4info,
                                            pi_p4_id_t act_prof_id) {
  size_t num_tables = 0;
  const pi_p4_id_t *t_ids =
      pi_p4info_act_prof_get_tables(p4info, act_prof_id, &num_tables);
  bf_sys_assert(num_tables > 0);
  return t_ids[0];
}

static uint32_t act_prof_action_id_to_handle(const pi_p4info_t *p4info,
                                             pi_dev_id_t dev_id,
                                             pi_p4_id_t act_prof_id,
                                             pi_p4_id_t a_id) {
  pi_p4_id_t one_t_id = act_prof_get_one_table_id(p4info, act_prof_id);
  // the Tofino compiler makes sure that the action handles are the same for ALL
  // tables referencing the same action profile, so we just pick the first table
  return pi_state_action_id_to_handle(dev_id, one_t_id, a_id);
}

static uint32_t act_prof_action_handle_to_id(const pi_p4info_t *p4info,
                                             pi_dev_id_t dev_id,
                                             pi_p4_id_t act_prof_id,
                                             uint32_t action_handle) {
  pi_p4_id_t one_t_id = act_prof_get_one_table_id(p4info, act_prof_id);
  return pi_state_action_handle_to_id(dev_id, one_t_id, action_handle);
}

static size_t retrieve_indirect_res_specs(const pi_p4info_t *p4info,
                                          pi_dev_id_t dev_id,
                                          pi_p4_id_t act_prof_id,
                                          const pi_action_data_t *action_data,
                                          pi_state_indirect_res_t *res_specs) {
  // get one parent table, this parent table's resources are a superset of this
  // action profile's resources
  pi_p4_id_t table_id = act_prof_get_one_table_id(p4info, act_prof_id);
  size_t num_indirect_res;
  const pi_state_table_indirect_res_t *indirect_res =
      pi_state_table_indirect_res(dev_id, table_id, &num_indirect_res);
  size_t cnt = 0;
  for (size_t i = 0; i < num_indirect_res; i++) {
    bf_sys_assert(!indirect_res[i].is_match_bound);
    const pi_state_indirect_res_access_info_t *access_info =
        pi_state_indirect_res_access_info(
            dev_id, table_id, action_data->action_id, indirect_res[i].res_id);
    if (!access_info) continue;  // table res not valid for this action
    pi_state_indirect_res_t *res_spec = &res_specs[cnt++];
    res_spec->res_id = indirect_res[i].res_id;
    res_spec->res_idx = retrieve_indirect_res_index(action_data, access_info);
  }
  return cnt;
}

pi_status_t _pi_act_prof_mbr_create(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t act_prof_id,
                                    const pi_action_data_t *action_data,
                                    pi_indirect_handle_t *mbr_handle) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t action_table_handle =
      pi_state_table_id_to_handle(dev_tgt.dev_id, act_prof_id);
  uint32_t action_handle = act_prof_action_id_to_handle(
      p4info, dev_tgt.dev_id, act_prof_id, action_data->action_id);
  pi_p4_id_t one_t_id = act_prof_get_one_table_id(p4info, act_prof_id);

  pipe_adt_ent_hdl_t h;
  pipe_action_spec_t pipe_action_spec = {0};
  build_pipe_action_spec_direct(
      p4info, dev_tgt.dev_id, one_t_id, action_data, &pipe_action_spec);
  pipe_status_t status = pipe_mgr_adt_ent_add(session_handle,
                                              pipe_mgr_dev_tgt,
                                              action_table_handle,
                                              action_handle,
                                              0 /* mbr_id */,
                                              &pipe_action_spec,
                                              &h,
                                              0 /* flags */);

  cleanup_pipe_action_spec(&pipe_action_spec);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  *mbr_handle = h;

  pi_state_ms_mbr_create(dev_tgt.dev_id, act_prof_id, h);
  pi_state_ms_mbr_set_act(
      dev_tgt.dev_id, act_prof_id, h, action_data->action_id);

  // handle indirect resources
  pi_state_indirect_res_t res_specs[PIPE_NUM_TBL_RESOURCES];
  size_t cnt = retrieve_indirect_res_specs(
      p4info, dev_tgt.dev_id, act_prof_id, action_data, res_specs);
  for (size_t i = 0; i < cnt; i++)
    pi_state_ms_mbr_add_res(dev_tgt.dev_id, act_prof_id, h, &res_specs[i]);

  return PI_STATUS_SUCCESS;
}

typedef struct {
  pi_session_handle_t session_handle;
  uint32_t sel_handle;
} rm_mbr_from_grps_aux_t;

static void rm_mbr_from_grps(pi_dev_id_t dev_id,
                             pi_p4_id_t act_prof_id,
                             pi_indirect_handle_t mbr_h,
                             pi_indirect_handle_t grp_h,
                             void *aux) {
  (void)act_prof_id;
  rm_mbr_from_grps_aux_t *aux_ = (rm_mbr_from_grps_aux_t *)aux;
  pipe_mgr_sel_grp_mbr_del(aux_->session_handle,
                           PI_DEV_ID_TO_PIPE(dev_id),
                           aux_->sel_handle,
                           grp_h,
                           mbr_h,
                           0);
}

pi_status_t _pi_act_prof_mbr_delete(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t mbr_handle) {
  LOG_DBG("%s", __func__);

  // if there is a selector, make sure we remove this member from all the groups
  // it belongs to
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  if (sel_handle != 0) {
    rm_mbr_from_grps_aux_t aux = {session_handle, sel_handle};
    pi_state_ms_mbr_apply_to_grps(
        dev_id, act_prof_id, mbr_handle, rm_mbr_from_grps, &aux);
  }

  uint32_t action_table_handle =
      pi_state_table_id_to_handle(dev_id, act_prof_id);
  pipe_status_t status = pipe_mgr_adt_ent_del(session_handle,
                                              PI_DEV_ID_TO_PIPE(dev_id),
                                              action_table_handle,
                                              mbr_handle,
                                              0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  pi_state_ms_mbr_delete(dev_id, act_prof_id, mbr_handle);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_mbr_modify(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t mbr_handle,
                                    const pi_action_data_t *action_data) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_id);

  uint32_t action_table_handle =
      pi_state_table_id_to_handle(dev_id, act_prof_id);
  uint32_t action_handle = act_prof_action_id_to_handle(
      p4info, dev_id, act_prof_id, action_data->action_id);

  // in theory this check is only needed when this member is in a group. However
  // it is simpler to do it unconditionally. Note that the common use case is to
  // change the action data, not the action function.
  pi_p4_id_t curr_action_id =
      pi_state_ms_mbr_get_act(dev_id, act_prof_id, mbr_handle);
  if (action_data->action_id != curr_action_id) {
    LOG_ERROR("%s: Trying to modify action function of a member", __func__);
    return PI_STATUS_TARGET_ERROR;
  }

  // handle indirect resources
  // same as above, in theory this check is only needed when this member is in a
  // group
  {
    pi_state_indirect_res_t res_specs[PIPE_NUM_TBL_RESOURCES];
    size_t cnt = retrieve_indirect_res_specs(
        p4info, dev_id, act_prof_id, action_data, res_specs);
    size_t ref_cnt;
    const pi_state_indirect_res_t *ref_res_specs =
        pi_state_ms_mbr_get_res(dev_id, act_prof_id, mbr_handle, &ref_cnt);
    if (cnt != ref_cnt) {
      LOG_ERROR("%s: Trying to modify indirect resources of member", __func__);
      return PI_STATUS_TARGET_ERROR;
    }
    for (size_t i = 0; i < ref_cnt; i++) {
      bf_sys_assert(res_specs[i].res_id == ref_res_specs[i].res_id);
      if (res_specs[i].res_idx != ref_res_specs[i].res_idx) {
        LOG_ERROR("%s: Trying to modify indirect resources of member",
                  __func__);
        return PI_STATUS_TARGET_ERROR;
      }
    }
  }

  pi_p4_id_t one_t_id = act_prof_get_one_table_id(p4info, act_prof_id);
  pipe_action_spec_t pipe_action_spec = {0};
  build_pipe_action_spec_direct(
      p4info, dev_id, one_t_id, action_data, &pipe_action_spec);

  pipe_status_t status = pipe_mgr_adt_ent_set(session_handle,
                                              dev_id,
                                              action_table_handle,
                                              mbr_handle,
                                              action_handle,
                                              &pipe_action_spec,
                                              0 /* flags */);

  cleanup_pipe_action_spec(&pipe_action_spec);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_create(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t act_prof_id,
                                    size_t max_size,
                                    pi_indirect_handle_t *grp_handle) {
  LOG_DBG("%s", __func__);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_tgt.dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  const size_t pipe_max_size = (1 << (sizeof(uint16_t) * 8)) - 1;
  if (max_size > pipe_max_size) {
    LOG_ERROR("%s: Group size too large", __func__);
    return PI_STATUS_TARGET_ERROR;
  }

  if (max_size == 0) {
    // According to the P4Runtime specification, this case needs to be supported
    // and "the target should use the maximum value it can
    // support". SEL_VEC_WIDTH is the maximum value we can support (in the
    // absence of @selector_max_group_size annotation) so that's what we use.
    const size_t default_max_size = SEL_VEC_WIDTH;
    LOG_WARN(
        "%s: No max size specified when creating action selector group, "
        "defaulting to %" PRIu64
        " but this can lead to a waste of "
        "action data memory",
        __func__,
        default_max_size);
    max_size = default_max_size;
  }

  pipe_sel_grp_hdl_t h;
  pipe_status_t status = pipe_mgr_sel_grp_add(session_handle,
                                              pipe_mgr_dev_tgt,
                                              sel_handle,
                                              0 /* grp_id */,
                                              max_size,
                                              0xdeadbeef,
                                              &h,
                                              0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  *grp_handle = h;

  pi_state_ms_grp_create(dev_tgt.dev_id, act_prof_id, h);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_delete(pi_session_handle_t session_handle,
                                    pi_dev_id_t dev_id,
                                    pi_p4_id_t act_prof_id,
                                    pi_indirect_handle_t grp_handle) {
  LOG_DBG("%s", __func__);

  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  pipe_status_t status = pipe_mgr_sel_grp_del(session_handle,
                                              PI_DEV_ID_TO_PIPE(dev_id),
                                              sel_handle,
                                              grp_handle,
                                              0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  pi_state_ms_grp_delete(dev_id, act_prof_id, grp_handle);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_add_mbr(pi_session_handle_t session_handle,
                                     pi_dev_id_t dev_id,
                                     pi_p4_id_t act_prof_id,
                                     pi_indirect_handle_t grp_handle,
                                     pi_indirect_handle_t mbr_handle) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_id);

  pi_p4_id_t action_id =
      pi_state_ms_mbr_get_act(dev_id, act_prof_id, mbr_handle);
  uint32_t action_handle =
      act_prof_action_id_to_handle(p4info, dev_id, act_prof_id, action_id);
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  pipe_status_t status = pipe_mgr_sel_grp_mbr_add(session_handle,
                                                  PI_DEV_ID_TO_PIPE(dev_id),
                                                  sel_handle,
                                                  grp_handle,
                                                  action_handle,
                                                  mbr_handle,
                                                  0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  if (pi_state_ms_grp_add_mbr(dev_id, act_prof_id, grp_handle, mbr_handle)) {
    LOG_ERROR("%s: error when adding member %" PRIu64 " to group %" PRIu64
              " in action prof %u",
              __func__,
              mbr_handle,
              grp_handle,
              act_prof_id);
    return PI_STATUS_TARGET_ERROR;
  }
  pi_state_ms_grp_set_act(dev_id, act_prof_id, grp_handle, action_id);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_remove_mbr(pi_session_handle_t session_handle,
                                        pi_dev_id_t dev_id,
                                        pi_p4_id_t act_prof_id,
                                        pi_indirect_handle_t grp_handle,
                                        pi_indirect_handle_t mbr_handle) {
  LOG_DBG("%s", __func__);

  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  pipe_status_t status = pipe_mgr_sel_grp_mbr_del(session_handle,
                                                  PI_DEV_ID_TO_PIPE(dev_id),
                                                  sel_handle,
                                                  grp_handle,
                                                  mbr_handle,
                                                  0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  pi_state_ms_grp_remove_mbr(dev_id, act_prof_id, grp_handle, mbr_handle);

  return PI_STATUS_SUCCESS;
}

static pipe_status_t get_one_member(
    pi_session_handle_t session_handle,
    pi_dev_id_t dev_id,
    pi_p4_id_t act_prof_id,
    uint32_t action_table_handle,
    const pi_p4info_t *p4info,
    pipe_adt_ent_hdl_t entry_handle,
    pipe_action_data_spec_t *pipe_action_data_spec,
    entry_buffer_t *ebuf) {
  pipe_act_fn_hdl_t action_handle;
  bool from_hw = false;
  (void)session_handle;
  dev_target_t dev_tgt = {PI_DEV_ID_TO_PIPE(dev_id), BF_DEV_PIPE_ALL};
  pipe_status_t status = pipe_mgr_get_action_data_entry(action_table_handle,
                                                        dev_tgt,
                                                        entry_handle,
                                                        pipe_action_data_spec,
                                                        &action_handle,
                                                        from_hw);
  if (status != PIPE_SUCCESS) return status;

  emit_indirect_handle(
      entry_buffer_extend(ebuf, sizeof(s_pi_indirect_handle_t)),
      (pi_indirect_handle_t)entry_handle);
  pi_p4_id_t action_id =
      act_prof_action_handle_to_id(p4info, dev_id, act_prof_id, action_handle);
  emit_p4_id(entry_buffer_extend(ebuf, sizeof(s_pi_p4_id_t)), action_id);
  size_t adata_size = pi_p4info_action_data_size(p4info, action_id);
  emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), adata_size);
  pi_action_data_t action_data;
  action_data.data = entry_buffer_extend(ebuf, adata_size);
  unbuild_action_spec(action_id, p4info, pipe_action_data_spec, &action_data);

  // write indirect resource indices (stored in pi_state) back to action_data
  size_t num_indirect_res = 0;
  pi_p4_id_t one_t_id = act_prof_get_one_table_id(p4info, act_prof_id);
  const pi_state_indirect_res_t *indirect_res =
      pi_state_ms_mbr_get_res(dev_id,
                              act_prof_id,
                              (pi_indirect_handle_t)entry_handle,
                              &num_indirect_res);
  for (size_t i = 0; i < num_indirect_res; i++) {
    const pi_state_indirect_res_access_info_t *access_info =
        pi_state_indirect_res_access_info(
            dev_id, one_t_id, action_id, indirect_res[i].res_id);
    bf_sys_assert(access_info != NULL);
    emit_indirect_res_index(indirect_res[i].res_idx, &action_data, access_info);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t get_one_group(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   uint32_t sel_handle,
                                   pipe_sel_grp_hdl_t entry_handle,
                                   entry_buffer_t *ebuf,
                                   entry_buffer_t *ebuf_members) {
  pipe_adt_ent_hdl_t member_handle;
  emit_indirect_handle(
      entry_buffer_extend(ebuf, sizeof(s_pi_indirect_handle_t)),
      (pi_indirect_handle_t)entry_handle);
  pipe_status_t status =
      pipe_mgr_get_first_group_member(session_handle,
                                      sel_handle,
                                      PI_DEV_ID_TO_PIPE(dev_id),
                                      entry_handle,
                                      &member_handle);
  size_t member_offset = ebuf_members->size / sizeof(s_pi_indirect_handle_t);
  size_t num_members = 0;
  while (status == PIPE_SUCCESS) {
    num_members++;
    emit_indirect_handle(
        entry_buffer_extend(ebuf_members, sizeof(s_pi_indirect_handle_t)),
        (pi_indirect_handle_t)member_handle);
    status = pipe_mgr_get_next_group_members(session_handle,
                                             sel_handle,
                                             PI_DEV_ID_TO_PIPE(dev_id),
                                             entry_handle,
                                             member_handle,
                                             1,
                                             &member_handle);
  }
  emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), num_members);
  emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), member_offset);
  return PIPE_SUCCESS;
}

static void init_act_prof_fetch_res(pi_act_prof_fetch_res_t *res) {
  res->num_members = 0;
  res->num_groups = 0;
  res->entries_members_size = 0;
  res->entries_members = NULL;
  res->entries_groups_size = 0;
  res->entries_groups = NULL;
  res->num_cumulated_mbr_handles = 0;
  res->mbr_handles = NULL;
}

#define NUM_ENT_HDLS_READ 512

static pi_status_t fetch_members(pi_session_handle_t session_handle,
                                 pi_dev_tgt_t dev_tgt,
                                 pi_p4_id_t act_prof_id,
                                 pi_act_prof_fetch_res_t *res,
                                 const pi_p4info_t *p4info) {
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  uint32_t action_table_handle =
      pi_state_table_id_to_handle(dev_tgt.dev_id, act_prof_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  int handles[NUM_ENT_HDLS_READ] = {-1};
  int handle_index = 0;
  int num_handles = 1;
  status = pipe_mgr_get_first_entry_handle(
      session_handle, action_table_handle, pipe_mgr_dev_tgt, &handles[0]);
  if (status != PIPE_SUCCESS && status != PIPE_OBJ_NOT_FOUND)
    return PI_STATUS_TARGET_ERROR + status;
  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);

  pipe_action_data_spec_t pipe_action_data_spec;
  allocate_pipe_action_data_spec_any(
      act_prof_id, p4info, &pipe_action_data_spec);

  while (handles[handle_index] != -1) {
    status = get_one_member(session_handle,
                            dev_tgt.dev_id,
                            act_prof_id,
                            action_table_handle,
                            p4info,
                            (pipe_adt_ent_hdl_t)handles[handle_index],
                            &pipe_action_data_spec,
                            &ebuf);
    handle_index++;
    if (status == PIPE_OBJ_NOT_FOUND) continue;  // handle no longer valid
    if (status != PIPE_SUCCESS) break;
    res->num_members++;
    if (handle_index == num_handles) {
      status = pipe_mgr_get_next_entry_handles(
          session_handle,
          action_table_handle,
          pipe_mgr_dev_tgt,
          (pipe_mat_ent_hdl_t)handles[num_handles - 1],
          NUM_ENT_HDLS_READ,
          handles);
      // if there are no next entries pipe_mgr returns PIPE_OBJ_NOT_FOUND, but
      // this is OK for us, it just means we reached the end of the table
      if (status != PIPE_SUCCESS && status != PIPE_OBJ_NOT_FOUND) {
        pi_status = PI_STATUS_TARGET_ERROR + status;
        break;
      }
      num_handles = NUM_ENT_HDLS_READ;
      handle_index = 0;
    }
  }

  if (pi_status != PI_STATUS_SUCCESS) {
    res->num_members = 0;
    entry_buffer_destroy(&ebuf);
  } else {
    res->entries_members_size = ebuf.size;
    res->entries_members = ebuf.buf;
  }
  release_pipe_action_data_spec(&pipe_action_data_spec);
  return pi_status;
}

static pi_status_t fetch_groups(pi_session_handle_t session_handle,
                                pi_dev_tgt_t dev_tgt,
                                pi_p4_id_t act_prof_id,
                                pi_act_prof_fetch_res_t *res) {
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_tgt.dev_id, act_prof_id);
  if (sel_handle == 0) return PI_STATUS_SUCCESS;  // no selector

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  int handles[NUM_ENT_HDLS_READ] = {-1};
  int handle_index = 0;
  int num_handles = 1;
  status = pipe_mgr_get_first_entry_handle(
      session_handle, sel_handle, pipe_mgr_dev_tgt, &handles[0]);
  if (status != PIPE_SUCCESS && status != PIPE_OBJ_NOT_FOUND)
    return PI_STATUS_TARGET_ERROR + status;
  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);
  entry_buffer_t ebuf_members;
  entry_buffer_create(&ebuf_members);

  while (handles[handle_index] != -1) {
    status = get_one_group(session_handle,
                           dev_tgt.dev_id,
                           sel_handle,
                           (pipe_sel_grp_hdl_t)handles[handle_index],
                           &ebuf,
                           &ebuf_members);
    handle_index++;
    if (status != PIPE_SUCCESS) break;
    res->num_groups++;
    if (handle_index == num_handles) {
      status = pipe_mgr_get_next_entry_handles(
          session_handle,
          sel_handle,
          pipe_mgr_dev_tgt,
          (pipe_mat_ent_hdl_t)handles[num_handles - 1],
          NUM_ENT_HDLS_READ,
          handles);
      // if there are no next entries pipe_mgr returns PIPE_OBJ_NOT_FOUND, but
      // this is OK for us, it just means we reached the end of the table
      if (status != PIPE_SUCCESS && status != PIPE_OBJ_NOT_FOUND) {
        pi_status = PI_STATUS_TARGET_ERROR + status;
        break;
      }
      num_handles = NUM_ENT_HDLS_READ;
      handle_index = 0;
    }
  }

  if (pi_status != PI_STATUS_SUCCESS) {
    res->num_groups = 0;
    entry_buffer_destroy(&ebuf);
  } else {
    res->entries_groups_size = ebuf.size;
    res->entries_groups = ebuf.buf;
    res->num_cumulated_mbr_handles =
        ebuf_members.size / sizeof(s_pi_indirect_handle_t);
    res->mbr_handles = (pi_indirect_handle_t *)ebuf_members.buf;
  }
  return pi_status;
}

pi_status_t _pi_act_prof_grp_set_mbrs(pi_session_handle_t session_handle,
                                      pi_dev_id_t dev_id,
                                      pi_p4_id_t act_prof_id,
                                      pi_indirect_handle_t grp_handle,
                                      size_t num_mbrs,
                                      const pi_indirect_handle_t *mbr_handles,
                                      const bool *activate) {
  (void)session_handle;
  (void)dev_id;
  (void)act_prof_id;
  (void)grp_handle;
  (void)num_mbrs;
  (void)mbr_handles;
  (void)activate;
  return PI_STATUS_NOT_IMPLEMENTED_BY_TARGET;
}

pi_status_t _pi_act_prof_grp_activate_mbr(pi_session_handle_t session_handle,
                                          pi_dev_id_t dev_id,
                                          pi_p4_id_t act_prof_id,
                                          pi_indirect_handle_t grp_handle,
                                          pi_indirect_handle_t mbr_handle) {
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  pipe_status_t status = pipe_mgr_sel_grp_mbr_enable(session_handle,
                                                     PI_DEV_ID_TO_PIPE(dev_id),
                                                     sel_handle,
                                                     grp_handle,
                                                     mbr_handle,
                                                     0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_act_prof_grp_deactivate_mbr(pi_session_handle_t session_handle,
                                            pi_dev_id_t dev_id,
                                            pi_p4_id_t act_prof_id,
                                            pi_indirect_handle_t grp_handle,
                                            pi_indirect_handle_t mbr_handle) {
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  // the PI common code should have already checked that there is a selector
  bf_sys_assert(sel_handle != 0);

  pipe_status_t status = pipe_mgr_sel_grp_mbr_disable(session_handle,
                                                      PI_DEV_ID_TO_PIPE(dev_id),
                                                      sel_handle,
                                                      grp_handle,
                                                      mbr_handle,
                                                      0 /* flags */);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

#undef NUM_ENT_HDLS_READ

pi_status_t _pi_act_prof_entries_fetch(pi_session_handle_t session_handle,
                                       pi_dev_tgt_t dev_tgt,
                                       pi_p4_id_t act_prof_id,
                                       pi_act_prof_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);
  init_act_prof_fetch_res(res);

  pi_status = fetch_members(session_handle, dev_tgt, act_prof_id, res, p4info);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;
  pi_status = fetch_groups(session_handle, dev_tgt, act_prof_id, res);
  if (pi_status != PI_STATUS_SUCCESS) {
    res->num_members = 0;
    if (res->entries_members != NULL) bf_sys_free(res->entries_members);
    res->entries_members = NULL;
  }
  return pi_status;
}

pi_status_t _pi_act_prof_mbr_fetch(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t mbr_handle,
                                   pi_act_prof_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_id);
  init_act_prof_fetch_res(res);
  uint32_t action_table_handle =
      pi_state_table_id_to_handle(dev_id, act_prof_id);

  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);

  pipe_action_data_spec_t pipe_action_data_spec;
  allocate_pipe_action_data_spec_any(
      act_prof_id, p4info, &pipe_action_data_spec);

  status = get_one_member(session_handle,
                          dev_id,
                          act_prof_id,
                          action_table_handle,
                          p4info,
                          mbr_handle,
                          &pipe_action_data_spec,
                          &ebuf);

  if (status == PIPE_OBJ_NOT_FOUND) {
    entry_buffer_destroy(&ebuf);
  } else if (status == PIPE_SUCCESS) {
    res->num_members = 1;
    res->entries_members_size = ebuf.size;
    res->entries_members = ebuf.buf;
  } else {
    entry_buffer_destroy(&ebuf);
    pi_status = PI_STATUS_TARGET_ERROR + status;
  }

  release_pipe_action_data_spec(&pipe_action_data_spec);
  return pi_status;
}

pi_status_t _pi_act_prof_grp_fetch(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t act_prof_id,
                                   pi_indirect_handle_t grp_handle,
                                   pi_act_prof_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  init_act_prof_fetch_res(res);
  uint32_t sel_handle =
      pi_state_act_prof_get_selector_handle(dev_id, act_prof_id);
  bf_sys_assert(sel_handle != 0);

  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);
  entry_buffer_t ebuf_members;
  entry_buffer_create(&ebuf_members);

  status = get_one_group(
      session_handle, dev_id, sel_handle, grp_handle, &ebuf, &ebuf_members);

  if (status == PIPE_OBJ_NOT_FOUND) {
    entry_buffer_destroy(&ebuf);
    entry_buffer_destroy(&ebuf_members);
  } else if (status == PIPE_SUCCESS) {
    res->num_groups = 1;
    res->entries_groups_size = ebuf.size;
    res->entries_groups = ebuf.buf;
    res->num_cumulated_mbr_handles =
        ebuf_members.size / sizeof(s_pi_indirect_handle_t);
    res->mbr_handles = (pi_indirect_handle_t *)ebuf_members.buf;
  } else {
    entry_buffer_destroy(&ebuf);
    entry_buffer_destroy(&ebuf_members);
    pi_status = PI_STATUS_TARGET_ERROR + status;
  }

  return pi_status;
}

pi_status_t _pi_act_prof_entries_fetch_done(pi_session_handle_t session_handle,
                                            pi_act_prof_fetch_res_t *res) {
  (void)session_handle;
  LOG_DBG("%s", __func__);
  if (res->entries_members != NULL) bf_sys_free(res->entries_members);
  if (res->entries_groups != NULL) bf_sys_free(res->entries_groups);
  if (res->mbr_handles != NULL) bf_sys_free(res->mbr_handles);
  return PI_STATUS_SUCCESS;
}

int _pi_act_prof_api_support(pi_dev_id_t dev_id) {
  (void)dev_id;
  return PI_ACT_PROF_API_SUPPORT_GRP_ADD_AND_REMOVE_MBR;
}
