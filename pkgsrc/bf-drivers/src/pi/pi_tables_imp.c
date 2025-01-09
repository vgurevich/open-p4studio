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
#include <PI/target/pi_tables_imp.h>

#include "pi_allocators.h"
#include "pi_helpers.h"
#include "pi_log.h"
#include "pi_resource_specs.h"
#include "pi_state.h"

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>

#include <inttypes.h>
#include <string.h>

// In P4Runtime / PI, there is no notion of "group id" for priority numbers; the
// PI priority uses a single 32-bit number space which defines an ordering on
// ALL entries in the table. For this reason, all the entries need to belong to
// the same group (we use group 0). As a first implementation, we choose to
// return an error if the priority is not in the range [0, 2^24[. We could also
// use a sparse map to allocate "pipe_mgr" priority values (we would still be
// limited to 2^24 distinct priority numbers, but the application would be able
// to use any value in the range [0, 2^32[).
#define _PI_MAX_PRIORITY ((1u << 24) - 1)

// pi_priority_t is typedef'd to uint32_t
static uint32_t invert_priority_pi_to_pipe(pi_priority_t from) {
  bf_sys_assert(from <= _PI_MAX_PRIORITY);
  return _PI_MAX_PRIORITY - from;
}

static pi_priority_t invert_priority_pipe_to_pi(uint32_t from) {
  bf_sys_assert(from <= _PI_MAX_PRIORITY);
  return _PI_MAX_PRIORITY - from;
}

static void build_key_and_mask(pi_p4_id_t table_id,
                               const pi_match_key_t *match_key,
                               const pi_p4info_t *p4info,
                               pipe_tbl_match_spec_t *pipe_match_spec) {
  pipe_match_spec->priority = 0;

  uint8_t *match_bits = pipe_match_spec->match_value_bits;

  uint8_t *mask_bits = pipe_match_spec->match_mask_bits;
  memset(mask_bits, 0xff, pipe_match_spec->num_match_bytes);

  int bits_to_reset;
  uint32_t pLen;

  size_t num_match_fields = pi_p4info_table_num_match_fields(p4info, table_id);
  const char *mk_data = match_key->data;
  for (size_t i = 0; i < num_match_fields; i++) {
    const pi_p4info_match_field_info_t *finfo =
        pi_p4info_table_match_field_info(p4info, table_id, i);
    size_t f_bw = finfo->bitwidth;
    size_t nbytes = (f_bw + 7) / 8;

    // key
    switch (finfo->match_type) {
      case PI_P4INFO_MATCH_TYPE_VALID:
        bf_sys_assert(0);
        break;
      case PI_P4INFO_MATCH_TYPE_EXACT:
      case PI_P4INFO_MATCH_TYPE_LPM:
      case PI_P4INFO_MATCH_TYPE_TERNARY:
      case PI_P4INFO_MATCH_TYPE_RANGE:
        memcpy(match_bits, mk_data, nbytes);
        match_bits += nbytes;
        mk_data += nbytes;
        break;
      default:
        bf_sys_assert(0);
    }

    // mask
    // we mask the first byte of the mask for each field properly, for debugging
    // purposes only
    char first_byte_mask =
        pi_p4info_table_match_field_byte0_mask(p4info, table_id, finfo->mf_id);
    switch (finfo->match_type) {
      case PI_P4INFO_MATCH_TYPE_VALID:
        continue;
      case PI_P4INFO_MATCH_TYPE_EXACT:
        mask_bits[0] &= first_byte_mask;
        mask_bits += nbytes;
        break;
      case PI_P4INFO_MATCH_TYPE_LPM:
        mk_data += retrieve_uint32(mk_data, &pLen);
        bits_to_reset = f_bw - pLen;
        if (bits_to_reset >= 8) {
          memset(mask_bits + nbytes - bits_to_reset / 8, 0, bits_to_reset / 8);
        }
        if (bits_to_reset % 8 != 0) {
          mask_bits[nbytes - 1 - bits_to_reset / 8] = (unsigned char)0xFF
                                                      << (bits_to_reset % 8);
        }
        mask_bits[0] &= first_byte_mask;
        mask_bits += nbytes;
        // also modify the match_spec priority
        pipe_match_spec->priority = f_bw - pLen;
        break;
      case PI_P4INFO_MATCH_TYPE_TERNARY:
      case PI_P4INFO_MATCH_TYPE_RANGE:
        memcpy(mask_bits, mk_data, nbytes);
        mask_bits[0] &= first_byte_mask;
        mask_bits += nbytes;
        mk_data += nbytes;
        pipe_match_spec->priority =
            invert_priority_pi_to_pipe(match_key->priority);
        break;
      default:
        bf_sys_assert(0);
    }
  }
}

static void unbuild_key_and_mask(pi_p4_id_t table_id,
                                 const pi_p4info_t *p4info,
                                 const pipe_tbl_match_spec_t *pipe_match_spec,
                                 pi_match_key_t *match_key) {
  match_key->priority = 0;

  const uint8_t *match_bits = pipe_match_spec->match_value_bits;
  const uint8_t *mask_bits = pipe_match_spec->match_mask_bits;

  uint32_t pLen;

  size_t num_match_fields = pi_p4info_table_num_match_fields(p4info, table_id);
  char *mk_data = match_key->data;
  for (size_t i = 0; i < num_match_fields; i++) {
    const pi_p4info_match_field_info_t *finfo =
        pi_p4info_table_match_field_info(p4info, table_id, i);
    size_t f_bw = finfo->bitwidth;
    size_t nbytes = (f_bw + 7) / 8;

    switch (finfo->match_type) {
      case PI_P4INFO_MATCH_TYPE_VALID:
        bf_sys_assert(0);
        break;
      case PI_P4INFO_MATCH_TYPE_EXACT:
        memcpy(mk_data, match_bits, nbytes);
        mk_data += nbytes;
        match_bits += nbytes;
        mask_bits += nbytes;
        break;
      case PI_P4INFO_MATCH_TYPE_LPM:
        memcpy(mk_data, match_bits, nbytes);
        mk_data += nbytes;
        pLen = f_bw - pipe_match_spec->priority;
        mk_data += emit_uint32(mk_data, pLen);
        match_bits += nbytes;
        mask_bits += nbytes;
        break;
      case PI_P4INFO_MATCH_TYPE_TERNARY:
      case PI_P4INFO_MATCH_TYPE_RANGE:
        memcpy(mk_data, match_bits, nbytes);
        mk_data += nbytes;
        match_bits += nbytes;
        memcpy(mk_data, mask_bits, nbytes);
        mk_data += nbytes;
        mask_bits += nbytes;
        match_key->priority =
            invert_priority_pipe_to_pi(pipe_match_spec->priority);
        break;
      default:
        bf_sys_assert(0);
    }
  }
}

static void build_pipe_action_spec_indirect(
    const pi_p4info_t *p4info,
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    pi_indirect_handle_t h,
    pipe_action_spec_t *pipe_action_spec) {
  pi_p4_id_t act_prof_id = pi_p4info_table_get_implementation(p4info, table_id);
  bf_sys_assert(act_prof_id != PI_INVALID_ID);

  memset(&pipe_action_spec->act_data, 0, sizeof(pipe_action_spec->act_data));
  size_t num_indirect_res = 0;
  const pi_state_indirect_res_t *indirect_res = NULL;
  int is_grp_h = (PIPE_GET_HDL_TYPE(h) == PIPE_HDL_TYPE_SEL_TBL);
  if (is_grp_h) {
    pipe_action_spec->pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
    pipe_action_spec->sel_grp_hdl = h;
    indirect_res =
        pi_state_ms_grp_get_res(dev_id, act_prof_id, h, &num_indirect_res);
  } else {
    pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
    pipe_action_spec->adt_ent_hdl = h;
    indirect_res =
        pi_state_ms_mbr_get_res(dev_id, act_prof_id, h, &num_indirect_res);
  }
  pipe_action_spec->resource_count = num_indirect_res;

  for (size_t i = 0; i < num_indirect_res; i++) {
    pipe_res_spec_t *res_spec = &pipe_action_spec->resources[i];
    res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
    uint32_t handle = pi_state_res_id_to_handle(dev_id, indirect_res[i].res_id);
    res_spec->tbl_hdl = handle;
    res_spec->tbl_idx = indirect_res[i].res_idx;
  }
}

pi_direct_res_config_one_t *find_direct_res_config(
    const pi_direct_res_config_t *direct_res_config, pi_p4_id_t res_id) {
  if (direct_res_config == NULL) return NULL;
  for (size_t i = 0; i < direct_res_config->num_configs; i++) {
    pi_direct_res_config_one_t *config = &direct_res_config->configs[i];
    if (res_id == config->res_id) return config;
  }
  return NULL;
}

static pi_status_t build_pipe_action_spec_direct_resources(
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    const pi_direct_res_config_t *direct_res_config,
    const pi_p4info_t *p4info,
    pipe_action_spec_t *pipe_action_spec,
    enum pipe_res_action_tag default_tag) {
  size_t num_direct_resources = 0;
  const pi_p4_id_t *direct_res_ids = pi_p4info_table_get_direct_resources(
      p4info, table_id, &num_direct_resources);
  size_t num_configs_provided = 0;
  for (size_t i = 0; i < num_direct_resources; i++) {
    pipe_res_spec_t *res_spec =
        &pipe_action_spec->resources[pipe_action_spec->resource_count++];
    pi_p4_id_t res_id = direct_res_ids[i];
    pi_res_type_id_t type = PI_GET_TYPE_ID(res_id);
    pi_direct_res_config_one_t *config =
        find_direct_res_config(direct_res_config, res_id);
    res_spec->tbl_hdl = pi_state_res_id_to_handle(dev_id, res_id);
    if (config == NULL) {
      res_spec->tag = default_tag;
      if (default_tag == PIPE_RES_ACTION_TAG_ATTACHED &&
          type == PI_DIRECT_METER_ID) {
        // a meter spec with max rates to ensure that all packets are marked as
        // "GREEN"
        pi_meter_spec_t max_meter_spec;
        memset(&max_meter_spec, 0xff, sizeof(max_meter_spec));
        max_meter_spec.meter_unit =
            (pi_meter_unit_t)pi_p4info_meter_get_unit(p4info, res_id);
        max_meter_spec.meter_type =
            (pi_meter_type_t)pi_p4info_meter_get_type(p4info, res_id);
        convert_to_pipe_meter_spec(
            p4info, res_id, &max_meter_spec, &res_spec->data.meter);
      } else {
        memset(&res_spec->data, 0, sizeof(res_spec->data));
      }
      continue;
    }
    bf_sys_assert(res_id == config->res_id);
    num_configs_provided++;
    res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
    switch (type) {
      case PI_DIRECT_COUNTER_ID:
        // not supported on Tofino
        return PI_STATUS_INVALID_RES_TYPE_ID;
      case PI_DIRECT_METER_ID:
        convert_to_pipe_meter_spec(p4info,
                                   res_id,
                                   (pi_meter_spec_t *)config->config,
                                   &res_spec->data.meter);
        break;
      default:
        return PI_STATUS_INVALID_RES_TYPE_ID;
    }
  }

  // may be redundant with PI library checks
  if (direct_res_config != NULL &&
      (num_configs_provided != direct_res_config->num_configs)) {
    return PI_STATUS_NOT_A_DIRECT_RES_OF_TABLE;
  }

  return PI_STATUS_SUCCESS;
}

static pi_status_t build_pipe_action_spec_common(
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    const pi_table_entry_t *table_entry,
    const pi_p4info_t *p4info,
    pipe_action_spec_t *pipe_action_spec,
    enum pipe_res_action_tag direct_resources_default_tag) {
  switch (table_entry->entry_type) {
    case PI_ACTION_ENTRY_TYPE_DATA:
      build_pipe_action_spec_direct(p4info,
                                    dev_id,
                                    table_id,
                                    table_entry->entry.action_data,
                                    pipe_action_spec);
      break;
    case PI_ACTION_ENTRY_TYPE_INDIRECT:
      build_pipe_action_spec_indirect(p4info,
                                      dev_id,
                                      table_id,
                                      table_entry->entry.indirect_handle,
                                      pipe_action_spec);
      break;
    default:
      // maybe this check can be removed (performed by common PI code)
      LOG_ERROR("%s: Invalid entry type", __func__);
      return PI_STATUS_INVALID_ENTRY_TYPE;
  }

  pi_status_t pi_status =
      build_pipe_action_spec_direct_resources(dev_id,
                                              table_id,
                                              table_entry->direct_res_config,
                                              p4info,
                                              pipe_action_spec,
                                              direct_resources_default_tag);

  return pi_status;
}

static void obtain_action_handle_indirect(const pi_p4info_t *p4info,
                                          pi_dev_id_t dev_id,
                                          pi_p4_id_t table_id,
                                          pi_indirect_handle_t h,
                                          uint32_t *action_handle) {
  pi_p4_id_t act_prof_id = pi_p4info_table_get_implementation(p4info, table_id);
  int is_grp_h = (PIPE_GET_HDL_TYPE(h) == PIPE_HDL_TYPE_SEL_TBL);
  pi_p4_id_t action_id;
  if (is_grp_h)
    action_id = pi_state_ms_grp_get_act(dev_id, act_prof_id, h);
  else
    action_id = pi_state_ms_mbr_get_act(dev_id, act_prof_id, h);
  *action_handle = pi_state_action_id_to_handle(dev_id, table_id, action_id);
}

static pi_status_t obtain_action_handle_common(
    const pi_p4info_t *p4info,
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    const pi_table_entry_t *table_entry,
    uint32_t *action_handle) {
  switch (table_entry->entry_type) {
    case PI_ACTION_ENTRY_TYPE_DATA: {
      const pi_action_data_t *action_data = table_entry->entry.action_data;
      *action_handle = pi_state_action_id_to_handle(
          dev_id, table_id, action_data->action_id);
    } break;
    case PI_ACTION_ENTRY_TYPE_INDIRECT:
      obtain_action_handle_indirect(p4info,
                                    dev_id,
                                    table_id,
                                    table_entry->entry.indirect_handle,
                                    action_handle);
      break;
    default:
      // maybe this check can be removed (performed by common PI code)
      LOG_ERROR("%s: Invalid entry type", __func__);
      return PI_STATUS_INVALID_ENTRY_TYPE;
  }
  return PI_STATUS_SUCCESS;
}

static void build_pipe_match_spec(pi_p4_id_t table_id,
                                  const pi_match_key_t *match_key,
                                  const pi_p4info_t *p4info,
                                  pipe_tbl_match_spec_t *pipe_match_spec) {
  pipe_match_spec->partition_index = 0;  // not supported

  // it would be quite easy to optimize this by maintaining a pool
  allocate_pipe_match_spec(table_id, p4info, pipe_match_spec);

  build_key_and_mask(table_id, match_key, p4info, pipe_match_spec);
}

pi_status_t _pi_table_entry_add(pi_session_handle_t session_handle,
                                pi_dev_tgt_t dev_tgt,
                                pi_p4_id_t table_id,
                                const pi_match_key_t *match_key,
                                const pi_table_entry_t *table_entry,
                                int overwrite,
                                pi_entry_handle_t *entry_handle) {
  (void)overwrite;  // not supported

  LOG_DBG("%s", __func__);

  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  if (match_key->priority > _PI_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  pipe_tbl_match_spec_t pipe_match_spec;
  build_pipe_match_spec(table_id, match_key, p4info, &pipe_match_spec);

  pipe_action_spec_t pipe_action_spec;
  pi_status = build_pipe_action_spec_common(dev_tgt.dev_id,
                                            table_id,
                                            table_entry,
                                            p4info,
                                            &pipe_action_spec,
                                            PIPE_RES_ACTION_TAG_ATTACHED);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  uint32_t action_handle;
  pi_status = obtain_action_handle_common(
      p4info, dev_tgt.dev_id, table_id, table_entry, &action_handle);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  uint32_t ttl_ms = 0;
  const pi_entry_properties_t *properties = table_entry->entry_properties;
  if (pi_entry_properties_is_set(properties, PI_ENTRY_PROPERTY_TYPE_TTL))
    ttl_ms = (uint32_t)(properties->ttl_ns / 1000000);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_mat_ent_hdl_t h;
  pipe_status_t status = pipe_mgr_mat_ent_add(session_handle,
                                              pipe_mgr_dev_tgt,
                                              table_handle,
                                              &pipe_match_spec,
                                              action_handle,
                                              &pipe_action_spec,
                                              ttl_ms,
                                              0 /* flags */,
                                              &h);

  if (status != PIPE_SUCCESS) {
    pi_status = PI_STATUS_TARGET_ERROR + status;
  } else {
    *entry_handle = h;
    pi_status = PI_STATUS_SUCCESS;
  }

  release_pipe_match_spec(&pipe_match_spec);
  cleanup_pipe_action_spec(&pipe_action_spec);

  return pi_status;
}

pi_status_t _pi_table_default_action_set(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         const pi_table_entry_t *table_entry) {
  LOG_DBG("%s", __func__);

  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  // for "direct" only
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    bf_sys_assert(table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA);
    const pi_action_data_t *action_data = table_entry->entry.action_data;
    // cannot set default action if it is const
    if (pi_p4info_table_has_const_default_action(p4info, table_id)) {
      bool has_mutable_action_params;
      const pi_p4_id_t default_action_id =
          pi_p4info_table_get_const_default_action(
              p4info, table_id, &has_mutable_action_params);
      if (default_action_id != action_data->action_id)
        return PI_STATUS_CONST_DEFAULT_ACTION;
      (void)has_mutable_action_params;
    }
  }

  pipe_action_spec_t pipe_action_spec;
  pi_status = build_pipe_action_spec_common(dev_tgt.dev_id,
                                            table_id,
                                            table_entry,
                                            p4info,
                                            &pipe_action_spec,
                                            PIPE_RES_ACTION_TAG_ATTACHED);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  uint32_t action_handle;
  pi_status = obtain_action_handle_common(
      p4info, dev_tgt.dev_id, table_id, table_entry, &action_handle);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_mat_ent_hdl_t h;
  pipe_status_t status = pipe_mgr_mat_default_entry_set(session_handle,
                                                        pipe_mgr_dev_tgt,
                                                        table_handle,
                                                        action_handle,
                                                        &pipe_action_spec,
                                                        0 /* flags */,
                                                        &h);
  // PI does not use a handle for the default entry (at least for now)
  (void)h;
  if (status != PIPE_SUCCESS) {
    pi_status = PI_STATUS_TARGET_ERROR + status;
  } else {
    pi_status = PI_STATUS_SUCCESS;
  }

  cleanup_pipe_action_spec(&pipe_action_spec);

  return pi_status;
}

pi_status_t _pi_table_default_action_reset(pi_session_handle_t session_handle,
                                           pi_dev_tgt_t dev_tgt,
                                           pi_p4_id_t table_id) {
  LOG_DBG("%s", __func__);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);

  pipe_status_t status = PIPE_SUCCESS;
  status = pipe_mgr_mat_tbl_default_entry_reset(
      session_handle, pipe_mgr_dev_tgt, table_handle, 0 /* flags */);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: error when resetting default entry with pipe mgr", __func__);
    return PI_STATUS_TARGET_ERROR + status;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_get(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         pi_table_entry_t *table_entry) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);

  pipe_action_spec_t pipe_action_spec;
  pipe_act_fn_hdl_t action_handle;
  bool is_direct =
      (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID);
  if (is_direct) {
    allocate_pipe_action_data_spec_any(
        table_id, p4info, &pipe_action_spec.act_data);
  }

  pipe_status_t status =
      pipe_mgr_table_get_default_entry(session_handle,
                                       pipe_mgr_dev_tgt,
                                       table_handle,
                                       &pipe_action_spec,
                                       &action_handle,
                                       false,
                                       PIPE_RES_GET_FLAG_ENTRY,
                                       NULL);

  if (status != PIPE_SUCCESS) {
    if (is_direct) release_pipe_action_data_spec(&pipe_action_spec.act_data);
    return PI_STATUS_TARGET_ERROR + status;
  }

  if (IS_ACTION_SPEC_ACT_DATA(&pipe_action_spec)) {
    table_entry->entry_type = PI_ACTION_ENTRY_TYPE_DATA;
    entry_buffer_t ebuf;
    entry_buffer_create(&ebuf);
    char *buf_ptr = entry_buffer_extend(&ebuf, sizeof(pi_action_data_t));
    // no alignment issue since entry_buffer_create uses malloc
    pi_action_data_t *action_data = (pi_action_data_t *)buf_ptr;
    action_data->p4info = p4info;
    pi_p4_id_t action_id =
        pi_state_action_handle_to_id(dev_tgt.dev_id, table_id, action_handle);
    action_data->action_id = action_id;
    size_t action_data_size = pi_p4info_action_data_size(p4info, action_id);
    action_data->data_size = action_data_size;
    table_entry->entry.action_data = action_data;
    buf_ptr = entry_buffer_extend(&ebuf, action_data_size);
    action_data->data = buf_ptr;
    unbuild_pipe_action_spec_direct(p4info,
                                    dev_tgt.dev_id,
                                    table_id,
                                    action_id,
                                    &pipe_action_spec,
                                    action_data);
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(&pipe_action_spec)) {
    table_entry->entry_type = PI_ACTION_ENTRY_TYPE_INDIRECT;
    table_entry->entry.indirect_handle = pipe_action_spec.adt_ent_hdl;
  } else if (IS_ACTION_SPEC_SEL_GRP(&pipe_action_spec)) {
    table_entry->entry_type = PI_ACTION_ENTRY_TYPE_INDIRECT;
    table_entry->entry.indirect_handle = pipe_action_spec.sel_grp_hdl;
  } else {
    bf_sys_assert(0);
  }
  if (is_direct) release_pipe_action_data_spec(&pipe_action_spec.act_data);

  table_entry->entry_properties = NULL;

  // direct resources
  if (pipe_action_spec.resource_count != 0) {
    pi_direct_res_config_t *config =
        bf_sys_malloc(sizeof(pi_direct_res_config_t));
    table_entry->direct_res_config = config;
    config->num_configs = 0;
    // potentially more space than we need, which is fine
    config->configs = bf_sys_calloc(pipe_action_spec.resource_count,
                                    sizeof(pi_direct_res_config_one_t));
    for (int i = 0; i < pipe_action_spec.resource_count; i++) {
      const pipe_res_spec_t *res_spec = &pipe_action_spec.resources[i];
      pi_p4_id_t res_id =
          pi_state_res_handle_to_id(dev_tgt.dev_id, res_spec->tbl_hdl);
      if (!pi_p4info_table_is_direct_resource_of(p4info, table_id, res_id))
        continue;

      pi_res_type_id_t type = PI_GET_TYPE_ID(res_id);
      pi_direct_res_config_one_t *config_one =
          &config->configs[config->num_configs];
      config_one->res_id = res_id;
      if (type == PI_DIRECT_COUNTER_ID) {
        pi_counter_data_t *counter_data =
            bf_sys_malloc(sizeof(pi_counter_data_t));
        convert_to_counter_data(
            p4info, res_id, &res_spec->data.counter, counter_data);
        config_one->config = counter_data;
      } else if (type == PI_DIRECT_METER_ID) {
        pi_meter_spec_t *meter_spec = bf_sys_malloc(sizeof(pi_meter_spec_t));
        convert_from_pipe_meter_spec(
            p4info, res_id, &res_spec->data.meter, meter_spec);
        config_one->config = meter_spec;
      } else {
        LOG_WARN("%s: Unsupported direct resource type for table %u: %" PRIu64,
                 __func__,
                 table_id,
                 type);
        continue;
      }
      config->num_configs++;
    }
  } else {
    table_entry->direct_res_config = NULL;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_done(pi_session_handle_t session_handle,
                                          pi_table_entry_t *table_entry) {
  (void)session_handle;
  if (table_entry->entry_type == PI_ACTION_ENTRY_TYPE_DATA)
    bf_sys_free(table_entry->entry.action_data);
  // drop the const to be able to call free
  pi_direct_res_config_t *config =
      (pi_direct_res_config_t *)table_entry->direct_res_config;
  if (config != NULL) {
    for (size_t i = 0; i < config->num_configs; i++)
      bf_sys_free(config->configs[i].config);
    bf_sys_free(config->configs);
    bf_sys_free(config);
  }
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_default_action_get_handle(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_p4_id_t table_id,
    pi_entry_handle_t *entry_handle) {
  LOG_DBG("%s", __func__);

  static const pi_entry_handle_t invalid_default_handle = (pi_entry_handle_t)-1;
  *entry_handle = invalid_default_handle;

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);

  pipe_mat_ent_hdl_t h;
  pipe_status_t status = pipe_mgr_table_get_default_entry_handle(
      session_handle, pipe_mgr_dev_tgt, table_handle, &h);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  *entry_handle = h;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_delete(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle) {
  LOG_DBG("%s", __func__);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_id, table_id);

  pi_status_t pi_status;
  pipe_status_t status = pipe_mgr_mat_ent_del(session_handle,
                                              PI_DEV_ID_TO_PIPE(dev_id),
                                              table_handle,
                                              entry_handle,
                                              0 /* flags */);
  if (status != PIPE_SUCCESS) {
    pi_status = PI_STATUS_TARGET_ERROR + status;
  } else {
    pi_status = PI_STATUS_SUCCESS;
  }
  return pi_status;
}

pi_status_t _pi_table_entry_delete_wkey(pi_session_handle_t session_handle,
                                        pi_dev_tgt_t dev_tgt,
                                        pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key) {
  LOG_DBG("%s", __func__);

  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  if (match_key->priority > _PI_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  pipe_tbl_match_spec_t pipe_match_spec;
  build_pipe_match_spec(table_id, match_key, p4info, &pipe_match_spec);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_status_t status = pipe_mgr_mat_ent_del_by_match_spec(session_handle,
                                                            pipe_mgr_dev_tgt,
                                                            table_handle,
                                                            &pipe_match_spec,
                                                            0 /* flags */);

  if (status != PIPE_SUCCESS) {
    pi_status = PI_STATUS_TARGET_ERROR + status;
  } else {
    pi_status = PI_STATUS_SUCCESS;
  }

  release_pipe_match_spec(&pipe_match_spec);

  return pi_status;
}

pi_status_t _pi_table_entry_modify(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   pi_entry_handle_t entry_handle,
                                   const pi_table_entry_t *table_entry) {
  LOG_DBG("%s", __func__);

  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_id);

  uint32_t action_handle;
  pi_status = obtain_action_handle_common(
      p4info, dev_id, table_id, table_entry, &action_handle);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  pipe_action_spec_t pipe_action_spec;
  pi_status = build_pipe_action_spec_common(dev_id,
                                            table_id,
                                            table_entry,
                                            p4info,
                                            &pipe_action_spec,
                                            PIPE_RES_ACTION_TAG_NO_CHANGE);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  uint32_t table_handle = pi_state_table_id_to_handle(dev_id, table_id);
  pipe_status_t status = pipe_mgr_mat_ent_set_action(session_handle,
                                                     PI_DEV_ID_TO_PIPE(dev_id),
                                                     table_handle,
                                                     entry_handle,
                                                     action_handle,
                                                     &pipe_action_spec,
                                                     0 /* flags */);

  cleanup_pipe_action_spec(&pipe_action_spec);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  // modify entry TTL if needed
  uint32_t ttl_ms = 0;
  const pi_entry_properties_t *properties = table_entry->entry_properties;
  if (pi_entry_properties_is_set(properties, PI_ENTRY_PROPERTY_TYPE_TTL)) {
    ttl_ms = (uint32_t)(properties->ttl_ns / 1000000);
    status = pipe_mgr_mat_ent_set_idle_ttl(session_handle,
                                           PI_DEV_ID_TO_PIPE(dev_id),
                                           table_handle,
                                           entry_handle,
                                           ttl_ms,
                                           0 /* flags */,
                                           false);
    if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  }

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_table_entry_modify_wkey(pi_session_handle_t session_handle,
                                        pi_dev_tgt_t dev_tgt,
                                        pi_p4_id_t table_id,
                                        const pi_match_key_t *match_key,
                                        const pi_table_entry_t *table_entry) {
  LOG_DBG("%s", __func__);

  pi_status_t pi_status;
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  if (match_key->priority > _PI_MAX_PRIORITY)
    return PI_STATUS_UNSUPPORTED_ENTRY_PRIORITY;
  pipe_tbl_match_spec_t pipe_match_spec;
  build_pipe_match_spec(table_id, match_key, p4info, &pipe_match_spec);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  uint32_t action_handle;
  pi_status = obtain_action_handle_common(
      p4info, dev_tgt.dev_id, table_id, table_entry, &action_handle);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  pipe_action_spec_t pipe_action_spec;
  pi_status = build_pipe_action_spec_common(dev_tgt.dev_id,
                                            table_id,
                                            table_entry,
                                            p4info,
                                            &pipe_action_spec,
                                            PIPE_RES_ACTION_TAG_NO_CHANGE);
  if (pi_status != PI_STATUS_SUCCESS) return pi_status;

  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_status_t status =
      pipe_mgr_mat_ent_set_action_by_match_spec(session_handle,
                                                pipe_mgr_dev_tgt,
                                                table_handle,
                                                &pipe_match_spec,
                                                action_handle,
                                                &pipe_action_spec,
                                                0 /* flags */);

  cleanup_pipe_action_spec(&pipe_action_spec);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  // modify entry TTL if needed
  uint32_t ttl_ms = 0;
  const pi_entry_properties_t *properties = table_entry->entry_properties;
  if (pi_entry_properties_is_set(properties, PI_ENTRY_PROPERTY_TYPE_TTL)) {
    ttl_ms = (uint32_t)(properties->ttl_ns / 1000000);
    pipe_mat_ent_hdl_t entry_handle;
    status = pipe_mgr_match_spec_to_ent_hdl(session_handle,
                                            pipe_mgr_dev_tgt,
                                            table_handle,
                                            &pipe_match_spec,
                                            &entry_handle,
                                            false /* light_pipe_validation */);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s: cannot set TTL because entry handle could not be retrieved",
          __func__);
      release_pipe_match_spec(&pipe_match_spec);
      return PI_STATUS_TARGET_ERROR + status;
    }

    status = pipe_mgr_mat_ent_set_idle_ttl(session_handle,
                                           PI_DEV_ID_TO_PIPE(dev_tgt.dev_id),
                                           table_handle,
                                           entry_handle,
                                           ttl_ms,
                                           0 /* flags */,
                                           false);
    release_pipe_match_spec(&pipe_match_spec);

    if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  }

  return pi_status;
}

static void emit_one_direct_resource(pi_p4_id_t res_id,
                                     void *config,
                                     entry_buffer_t *ebuf) {
  pi_res_type_id_t type = PI_GET_TYPE_ID(res_id);
  PIDirectResMsgSizeFn msg_size_fn;
  PIDirectResEmitFn emit_fn;
  pi_direct_res_get_fns(type, &msg_size_fn, &emit_fn, NULL, NULL);

  emit_p4_id(entry_buffer_extend(ebuf, sizeof(s_pi_p4_id_t)), res_id);
  size_t s = msg_size_fn(config);
  emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), s);
  emit_fn(entry_buffer_extend(ebuf, s), config);
}

static pipe_status_t get_one_entry(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   uint32_t table_handle,
                                   const pi_p4info_t *p4info,
                                   pipe_mat_ent_hdl_t entry_handle,
                                   pipe_tbl_match_spec_t *pipe_match_spec,
                                   pipe_action_spec_t *pipe_action_spec,
                                   entry_buffer_t *ebuf) {
  pipe_act_fn_hdl_t action_handle;
  bool from_hw = false;
  dev_target_t pipe_mgr_dev_tgt = {PI_DEV_ID_TO_PIPE(dev_id), BF_DEV_PIPE_ALL};
  pipe_status_t status = pipe_mgr_get_entry(session_handle,
                                            table_handle,
                                            pipe_mgr_dev_tgt,
                                            entry_handle,
                                            pipe_match_spec,
                                            pipe_action_spec,
                                            &action_handle,
                                            from_hw,
                                            PIPE_RES_GET_FLAG_ENTRY,
                                            NULL);
  if (status != PIPE_SUCCESS) return status;

  emit_entry_handle(entry_buffer_extend(ebuf, sizeof(s_pi_entry_handle_t)),
                    (pi_entry_handle_t)entry_handle);
  // allocate space for priority which will be set after the call to
  // unbuild_key_and_mask below
  entry_buffer_extend(ebuf, sizeof(uint32_t));
  size_t mkey_nbytes = pi_p4info_table_match_key_size(p4info, table_id);
  pi_match_key_t match_key;
  match_key.data = entry_buffer_extend(ebuf, mkey_nbytes);
  unbuild_key_and_mask(table_id, p4info, pipe_match_spec, &match_key);
  // set priority
  emit_uint32(match_key.data - sizeof(uint32_t), match_key.priority);

  char *buf = entry_buffer_extend(ebuf, sizeof(s_pi_action_entry_type_t));
  if (IS_ACTION_SPEC_ACT_DATA(pipe_action_spec)) {
    buf += emit_action_entry_type(buf, PI_ACTION_ENTRY_TYPE_DATA);
    pi_p4_id_t action_id =
        pi_state_action_handle_to_id(dev_id, table_id, action_handle);
    emit_p4_id(entry_buffer_extend(ebuf, sizeof(s_pi_p4_id_t)), action_id);
    size_t adata_size = pi_p4info_action_data_size(p4info, action_id);
    emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), adata_size);
    pi_action_data_t action_data;
    action_data.data = entry_buffer_extend(ebuf, adata_size);
    unbuild_pipe_action_spec_direct(
        p4info, dev_id, table_id, action_id, pipe_action_spec, &action_data);
  } else if (IS_ACTION_SPEC_ACT_DATA_HDL(pipe_action_spec)) {
    buf += emit_action_entry_type(buf, PI_ACTION_ENTRY_TYPE_INDIRECT);
    emit_indirect_handle(
        entry_buffer_extend(ebuf, sizeof(s_pi_indirect_handle_t)),
        pipe_action_spec->adt_ent_hdl);
  } else if (IS_ACTION_SPEC_SEL_GRP(pipe_action_spec)) {
    buf += emit_action_entry_type(buf, PI_ACTION_ENTRY_TYPE_INDIRECT);
    emit_indirect_handle(
        entry_buffer_extend(ebuf, sizeof(s_pi_indirect_handle_t)),
        pipe_action_spec->sel_grp_hdl);
  } else {
    bf_sys_assert(0);
  }

  // no properties
  emit_uint32(entry_buffer_extend(ebuf, sizeof(uint32_t)), 0);

  // direct resources

  // allocate space for the direct resource count in the buffer, we will write
  // the correct value there once we are done processing the resources array
  char *resource_count_ptr = entry_buffer_extend(ebuf, sizeof(uint32_t));
  uint32_t resource_count = 0;
  for (int i = 0; i < pipe_action_spec->resource_count; i++) {
    const pipe_res_spec_t *res_spec = &pipe_action_spec->resources[i];
    pi_p4_id_t res_id = pi_state_res_handle_to_id(dev_id, res_spec->tbl_hdl);
    if (!pi_p4info_table_is_direct_resource_of(p4info, table_id, res_id))
      continue;

    pi_res_type_id_t type = PI_GET_TYPE_ID(res_id);
    if (type == PI_DIRECT_COUNTER_ID) {
      pi_counter_data_t counter_data;
      convert_to_counter_data(
          p4info, res_id, &res_spec->data.counter, &counter_data);
      emit_one_direct_resource(res_id, &counter_data, ebuf);
    } else if (type == PI_DIRECT_METER_ID) {
      pi_meter_spec_t meter_spec;
      convert_from_pipe_meter_spec(
          p4info, res_id, &res_spec->data.meter, &meter_spec);
      emit_one_direct_resource(res_id, &meter_spec, ebuf);
    } else {
      LOG_WARN("%s: Unsupported direct resource type for table %u: %" PRIu64,
               __func__,
               table_id,
               type);
      continue;
    }

    resource_count++;
  }
  emit_uint32(resource_count_ptr, resource_count);

  return PIPE_SUCCESS;
}

static void init_table_fetch_res(pi_table_fetch_res_t *res) {
  res->num_entries = 0;
  res->entries_size = 0;
  res->entries = NULL;
}

#define NUM_ENT_HDLS_READ 512

pi_status_t _pi_table_entries_fetch(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t table_id,
                                    pi_table_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  init_table_fetch_res(res);
  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  int handles[NUM_ENT_HDLS_READ] = {-1};
  int handle_index = 0;
  int num_handles = 1;
  status = pipe_mgr_get_first_entry_handle(
      session_handle, table_handle, pipe_mgr_dev_tgt, &handles[0]);
  if (status != PIPE_SUCCESS && status != PIPE_OBJ_NOT_FOUND)
    return PI_STATUS_TARGET_ERROR + status;
  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);
  res->mkey_nbytes = pi_p4info_table_match_key_size(p4info, table_id);

  // we can reuse the same memory for all entries in the same table
  pipe_tbl_match_spec_t pipe_match_spec;
  allocate_pipe_match_spec(table_id, p4info, &pipe_match_spec);
  pipe_action_spec_t pipe_action_spec;

  // we only need to allocate memory for the action data for direct tables
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    allocate_pipe_action_data_spec_any(
        table_id, p4info, &pipe_action_spec.act_data);
  }

  while (handles[handle_index] != -1) {
    status = get_one_entry(session_handle,
                           dev_tgt.dev_id,
                           table_id,
                           table_handle,
                           p4info,
                           (pipe_mat_ent_hdl_t)handles[handle_index],
                           &pipe_match_spec,
                           &pipe_action_spec,
                           &ebuf);
    handle_index++;
    if (status == PIPE_OBJ_NOT_FOUND) continue;  // handle no longer valid
    if (status != PIPE_SUCCESS) break;
    res->num_entries++;
    if (handle_index == num_handles) {
      // this will skip over the default entry
      status = pipe_mgr_get_next_entry_handles(
          session_handle,
          table_handle,
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
    res->num_entries = 0;
    entry_buffer_destroy(&ebuf);
  } else {
    res->entries_size = ebuf.size;
    res->entries = ebuf.buf;
  }
  release_pipe_match_spec(&pipe_match_spec);
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    release_pipe_action_data_spec(&pipe_action_spec.act_data);
  }
  return pi_status;
}

#undef NUM_ENT_HDLS_READ

static pi_status_t fetch_one_entry(pi_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   uint32_t table_handle,
                                   pi_entry_handle_t entry_handle,
                                   pi_table_fetch_res_t *res,
                                   const pi_p4info_t *p4info) {
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;

  entry_buffer_t ebuf;
  entry_buffer_create(&ebuf);
  res->mkey_nbytes = pi_p4info_table_match_key_size(p4info, table_id);

  pipe_tbl_match_spec_t pipe_match_spec;
  allocate_pipe_match_spec(table_id, p4info, &pipe_match_spec);
  pipe_action_spec_t pipe_action_spec;

  // we only need to allocate memory for the action data for direct tables
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    allocate_pipe_action_data_spec_any(
        table_id, p4info, &pipe_action_spec.act_data);
  }

  status = get_one_entry(session_handle,
                         dev_id,
                         table_id,
                         table_handle,
                         p4info,
                         entry_handle,
                         &pipe_match_spec,
                         &pipe_action_spec,
                         &ebuf);

  if (status == PIPE_OBJ_NOT_FOUND) {
    entry_buffer_destroy(&ebuf);
  } else if (status == PIPE_SUCCESS) {
    res->num_entries = 1;
    res->entries_size = ebuf.size;
    res->entries = ebuf.buf;
  } else {
    entry_buffer_destroy(&ebuf);
    pi_status = PI_STATUS_TARGET_ERROR + status;
  }

  release_pipe_match_spec(&pipe_match_spec);
  if (pi_p4info_table_get_implementation(p4info, table_id) == PI_INVALID_ID) {
    release_pipe_action_data_spec(&pipe_action_spec.act_data);
  }

  return pi_status;
}

pi_status_t _pi_table_entries_fetch_one(pi_session_handle_t session_handle,
                                        pi_dev_id_t dev_id,
                                        pi_p4_id_t table_id,
                                        pi_entry_handle_t entry_handle,
                                        pi_table_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_id);
  init_table_fetch_res(res);
  uint32_t table_handle = pi_state_table_id_to_handle(dev_id, table_id);
  return fetch_one_entry(session_handle,
                         dev_id,
                         table_id,
                         table_handle,
                         entry_handle,
                         res,
                         p4info);
}

pi_status_t _pi_table_entries_fetch_wkey(pi_session_handle_t session_handle,
                                         pi_dev_tgt_t dev_tgt,
                                         pi_p4_id_t table_id,
                                         const pi_match_key_t *match_key,
                                         pi_table_fetch_res_t *res) {
  LOG_DBG("%s", __func__);
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);
  pipe_status_t status;
  pi_status_t pi_status = PI_STATUS_SUCCESS;
  init_table_fetch_res(res);
  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);

  pipe_tbl_match_spec_t pipe_match_spec;
  build_pipe_match_spec(table_id, match_key, p4info, &pipe_match_spec);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);

  pipe_mat_ent_hdl_t h;
  status = pipe_mgr_match_spec_to_ent_hdl(session_handle,
                                          pipe_mgr_dev_tgt,
                                          table_handle,
                                          &pipe_match_spec,
                                          &h,
                                          false /* light_pipe_validation */);

  if (status == PIPE_SUCCESS) {
    pi_status = fetch_one_entry(
        session_handle, dev_tgt.dev_id, table_id, table_handle, h, res, p4info);
  } else if (status != PIPE_OBJ_NOT_FOUND) {
    pi_status = PI_STATUS_TARGET_ERROR + status;
  }

  release_pipe_match_spec(&pipe_match_spec);

  return pi_status;
}

pi_status_t _pi_table_entries_fetch_done(pi_session_handle_t session_handle,
                                         pi_table_fetch_res_t *res) {
  (void)session_handle;
  LOG_DBG("%s", __func__);
  if (res->entries != NULL) bf_sys_free(res->entries);
  return PI_STATUS_SUCCESS;
}

static void idle_time_notify_cb(bf_dev_id_t dev_id,
                                pipe_mat_ent_hdl_t mat_ent_hdl,
                                pipe_idle_time_hit_state_e hs,
                                void *client_data) {
  pi_dev_id_t pi_dev_id = PI_DEV_ID_FROM_PIPE(dev_id);
  pi_state_shared_lock(pi_dev_id);
  if (!pi_state_is_device_assigned(pi_dev_id)) {
    LOG_TRACE("%s: ignoring idle time notification because device was removed",
              __func__);
    pi_state_unlock(pi_dev_id);
    return;
  }
  // This callback supports only idle notifications
  if (hs == ENTRY_ACTIVE) return;

  pipe_status_t status;

  // each CB accesses the scratchspace for the table
  // scratchspace cannot be NULL as long as the table supports idle_timeout as
  // per the P4 program
  // this relies on the assumption that pipe_mgr cannot generate concurrent
  // callbacks for the same table
  pi_state_idle_time_scratchspace_t *scratchspace =
      (pi_state_idle_time_scratchspace_t *)client_data;
  bf_sys_assert(scratchspace != NULL);

  // PI requires the match key to be included in the notification (not just the
  // handle), so we need to get the entry from pipe_mgr.
  pipe_sess_hdl_t sess;
  status = pipe_mgr_client_init(&sess);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s: failed to open pipe_mgr session to retrieve match entry, "
        "dropping idle time notification",
        __func__);
    pi_state_unlock(pi_dev_id);
    return;
  }

  dev_target_t pipe_mgr_dev_tgt = {dev_id, BF_DEV_PIPE_ALL};
  status = pipe_mgr_get_entry(sess,
                              scratchspace->table_handle,
                              pipe_mgr_dev_tgt,
                              mat_ent_hdl,
                              &scratchspace->pipe_match_spec,
                              &scratchspace->pipe_action_spec,
                              &scratchspace->action_handle,
                              false, /* from_hw */
                              PIPE_RES_GET_FLAG_ENTRY,
                              NULL);
  if (status != PIPE_SUCCESS) {
    // could happen if entry was removed by application since the callback was
    // initiated.
    LOG_WARN(
        "%s: failed to retrieve match entry from handle, "
        "dropping idle time notification",
        __func__);
    status = pipe_mgr_client_cleanup(sess);
    if (status != PIPE_SUCCESS) {
      LOG_CRIT("%s: failed to close pipe_mgr session", __func__);
    }
    pi_state_unlock(pi_dev_id);
    return;
  }

  // reset current TTL so that another notification will be generated when the
  // entry expires "again"
  pipe_mgr_mat_ent_reset_idle_ttl(
      sess, dev_id, scratchspace->table_handle, mat_ent_hdl);

  unbuild_key_and_mask(scratchspace->table_id,
                       scratchspace->p4info,
                       &scratchspace->pipe_match_spec,
                       &scratchspace->match_key);

  status = pipe_mgr_client_cleanup(sess);
  if (status != PIPE_SUCCESS) {
    LOG_CRIT("%s: failed to close pipe_mgr session", __func__);
    pi_state_unlock(pi_dev_id);
    return;
  }

  // the application does not take ownership of the PI match_key, which is why
  // we can use the scratchspace copy.
  pi_status_t pi_status =
      pi_table_idle_timeout_notify(pi_dev_id,
                                   scratchspace->table_id,
                                   &scratchspace->match_key,
                                   (pi_entry_handle_t)mat_ent_hdl);

  if (pi_status == PI_STATUS_IDLE_TIMEOUT_NO_MATCHING_CB) {
    LOG_TRACE("%s: idle time notification dropped because of no application CB",
              __func__);
  } else if (pi_status != PI_STATUS_SUCCESS) {
    LOG_ERROR("%s: idle time notification dropped", __func__);
  }
  pi_state_unlock(pi_dev_id);
}

pi_status_t _pi_table_idle_timeout_config_set(
    pi_session_handle_t session_handle,
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    const pi_idle_timeout_config_t *config) {
  // the PI C library ensures that this function is only called for tables which
  // support idle timeout

  // a reasonnable default for pipe_mgr
  static const uint32_t min_supported_query_interval_ms = 500;
  uint32_t min_ttl_ms = (uint32_t)(config->min_ttl_ns / 1000000);
  if (min_ttl_ms < min_supported_query_interval_ms)
    min_ttl_ms = min_supported_query_interval_ms;

  pipe_idle_time_params_t idle_time_params;
  idle_time_params.mode = NOTIFY_MODE;
  idle_time_params.u.notify.ttl_query_interval = min_ttl_ms;
  idle_time_params.u.notify.min_ttl = min_ttl_ms;
  idle_time_params.u.notify.max_ttl = 0xffffffff;
  idle_time_params.u.notify.callback_fn = &idle_time_notify_cb;
  idle_time_params.u.notify.default_callback_choice = 0;

  // the scratchspace enables us to avoid re-allocating the same memory for idle
  // time notifications for a given table; this works because pipe_mgr never
  // issues concurrent calls to idle_time_notify_cb for the same match table.
  pi_state_idle_time_scratchspace_t *scratchspace =
      pi_state_idle_time_get_scratchspace(PI_DEV_ID_TO_PIPE(dev_id), table_id);
  bf_sys_assert(scratchspace != NULL);
  idle_time_params.u.notify.client_data = (void *)scratchspace;

  uint32_t table_handle = pi_state_table_id_to_handle(dev_id, table_id);
  pipe_status_t status = pipe_mgr_idle_set_params(session_handle,
                                                  PI_DEV_ID_TO_PIPE(dev_id),
                                                  table_handle,
                                                  idle_time_params);
  if (status == PIPE_SUCCESS) {
    status = pipe_mgr_idle_tmo_set_enable(
        session_handle, PI_DEV_ID_TO_PIPE(dev_id), table_handle, true);
  }
  return (status == PIPE_SUCCESS) ? PI_STATUS_SUCCESS
                                  : PI_STATUS_TARGET_ERROR + status;
}

pi_status_t _pi_table_entry_get_remaining_ttl(
    pi_session_handle_t session_handle,
    pi_dev_id_t dev_id,
    pi_p4_id_t table_id,
    pi_entry_handle_t entry_handle,
    uint64_t *ttl_ns) {
  uint32_t table_handle = pi_state_table_id_to_handle(dev_id, table_id);
  uint32_t ttl_ms;
  pipe_status_t status =
      pipe_mgr_mat_ent_get_idle_ttl(session_handle,
                                    PI_DEV_ID_TO_PIPE(dev_id),
                                    table_handle,
                                    entry_handle,
                                    &ttl_ms);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  *ttl_ns = (uint64_t)ttl_ms * 1000000;
  return PI_STATUS_SUCCESS;
}

#undef _PI_MAX_PRIORITY
