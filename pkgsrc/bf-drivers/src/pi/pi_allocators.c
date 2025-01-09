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


#include "pi_allocators.h"

#include <PI/int/pi_int.h>
#include <PI/p4info.h>
#include <PI/pi.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>

// id can be the id of a table or an action profile
static size_t get_max_action_data_size(const pi_p4info_t *p4info,
                                       pi_p4_id_t id) {
  size_t max_adata_size = 0;
  size_t num_actions;
  const pi_p4_id_t *action_ids = NULL;
  if (PI_GET_TYPE_ID(id) == PI_TABLE_ID) {
    action_ids = pi_p4info_table_get_actions(p4info, id, &num_actions);
  } else if (PI_GET_TYPE_ID(id) == PI_ACT_PROF_ID) {
    action_ids = pi_p4info_act_prof_get_actions(p4info, id, &num_actions);
  } else {
    bf_sys_assert(0);
    return 0;
  }
  for (size_t i = 0; i < num_actions; i++) {
    size_t adata_size = pi_p4info_action_data_size(p4info, action_ids[i]);
    max_adata_size =
        (adata_size > max_adata_size) ? adata_size : max_adata_size;
  }
  return max_adata_size;
}

static void get_key_array_sizes(const pi_p4info_t *p4info,
                                pi_p4_id_t table_id,
                                size_t *num_match_bytes,
                                size_t *num_match_bits) {
  *num_match_bytes = 0;
  *num_match_bits = 0;

  size_t num_match_fields = pi_p4info_table_num_match_fields(p4info, table_id);
  for (size_t i = 0; i < num_match_fields; i++) {
    const pi_p4info_match_field_info_t *finfo =
        pi_p4info_table_match_field_info(p4info, table_id, i);
    size_t f_bw = finfo->bitwidth;
    size_t nbytes = (f_bw + 7) / 8;

    switch (finfo->match_type) {
      case PI_P4INFO_MATCH_TYPE_VALID:
        bf_sys_assert(0);
      /* fall through */
      case PI_P4INFO_MATCH_TYPE_EXACT:
      case PI_P4INFO_MATCH_TYPE_LPM:
      case PI_P4INFO_MATCH_TYPE_TERNARY:
      case PI_P4INFO_MATCH_TYPE_RANGE:
        (*num_match_bytes) += nbytes;
        (*num_match_bits) += f_bw;
        break;
      default:
        bf_sys_assert(0);
    }
  }
}

static void get_action_array_sizes(const pi_p4info_t *p4info,
                                   pi_p4_id_t action_id,
                                   size_t *num_action_bytes,
                                   size_t *num_action_bits) {
  *num_action_bytes = 0;
  *num_action_bits = 0;

  size_t num_params;
  const pi_p4_id_t *params =
      pi_p4info_action_get_params(p4info, action_id, &num_params);
  for (size_t i = 0; i < num_params; i++) {
    size_t bitwidth =
        pi_p4info_action_param_bitwidth(p4info, action_id, params[i]);
    *num_action_bits += bitwidth;
    *num_action_bytes += (bitwidth + 7) / 8;
  }
}

void allocate_pipe_match_spec(pi_p4_id_t table_id,
                              const pi_p4info_t *p4info,
                              pipe_tbl_match_spec_t *pipe_match_spec) {
  size_t num_match_bytes;
  size_t num_match_bits;
  get_key_array_sizes(p4info, table_id, &num_match_bytes, &num_match_bits);

  // it would be quite easy to optimize this by maintaining a pool
  uint8_t *match_bits = bf_sys_malloc(num_match_bytes);
  pipe_match_spec->match_value_bits = match_bits;
  uint8_t *mask_bits = bf_sys_malloc(num_match_bytes);
  pipe_match_spec->match_mask_bits = mask_bits;

  pipe_match_spec->num_valid_match_bits = num_match_bits;
  pipe_match_spec->num_match_bytes = num_match_bytes;
}

void release_pipe_match_spec(pipe_tbl_match_spec_t *pipe_match_spec) {
  bf_sys_free(pipe_match_spec->match_value_bits);
  bf_sys_free(pipe_match_spec->match_mask_bits);
}

void allocate_pipe_action_data_spec(
    pi_p4_id_t action_id,
    const pi_p4info_t *p4info,
    pipe_action_data_spec_t *pipe_action_data_spec) {
  size_t num_action_bytes;
  size_t num_action_bits;
  get_action_array_sizes(
      p4info, action_id, &num_action_bytes, &num_action_bits);

  uint8_t *action_data_bits = bf_sys_malloc(num_action_bytes);
  pipe_action_data_spec->action_data_bits = action_data_bits;

  pipe_action_data_spec->num_valid_action_data_bits = num_action_bits;
  pipe_action_data_spec->num_action_data_bytes = num_action_bytes;
}

void allocate_pipe_action_data_spec_any(
    pi_p4_id_t id,
    const pi_p4info_t *p4info,
    pipe_action_data_spec_t *pipe_action_data_spec) {
  size_t max_adata_size = get_max_action_data_size(p4info, id);

  uint8_t *action_data_bits = bf_sys_malloc(max_adata_size);
  pipe_action_data_spec->action_data_bits = action_data_bits;

  pipe_action_data_spec->num_action_data_bytes = max_adata_size;
  pipe_action_data_spec->num_valid_action_data_bits = 0;
}

void release_pipe_action_data_spec(
    pipe_action_data_spec_t *pipe_action_data_spec) {
  bf_sys_free(pipe_action_data_spec->action_data_bits);
}

void allocate_pi_match_key(pi_p4_id_t table_id,
                           const pi_p4info_t *p4info,
                           pi_match_key_t *match_key) {
  size_t mkey_nbytes = pi_p4info_table_match_key_size(p4info, table_id);
  match_key->p4info = p4info;
  match_key->table_id = table_id;
  match_key->priority = 0;
  match_key->data_size = mkey_nbytes;
  match_key->data = bf_sys_malloc(mkey_nbytes);
}

void release_pi_match_key(pi_match_key_t *match_key) {
  bf_sys_free(match_key->data);
}
