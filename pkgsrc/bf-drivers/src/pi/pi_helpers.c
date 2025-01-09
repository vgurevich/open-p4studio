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

#include "pi_allocators.h"
#include "pi_helpers.h"

#include <target-sys/bf_sal/bf_sys_intf.h>

#include <arpa/inet.h>

#include <string.h>

static uint64_t ntohll(uint64_t n) {
#if __BYTE_ORDER__ == __BIG_ENDIAN__
  return n;
#else
  return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
#endif
}

static uint64_t htonll(uint64_t n) {
#if __BYTE_ORDER__ == __BIG_ENDIAN__
  return n;
#else
  return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

void convert_dev_tgt(pi_dev_tgt_t pi_dev_tgt, dev_target_t *pipe_mgr_dev_tgt) {
  pipe_mgr_dev_tgt->device_id = PI_DEV_ID_TO_PIPE(pi_dev_tgt.dev_id);
  pipe_mgr_dev_tgt->dev_pipe_id = pi_dev_tgt.dev_pipe_mask;
}

void convert_pipe_dev_tgt(dev_target_t pipe_mgr_dev_tgt,
                          pi_dev_tgt_t *pi_dev_tgt) {
  pi_dev_tgt->dev_id = pipe_mgr_dev_tgt.device_id;
  pi_dev_tgt->dev_pipe_mask = pipe_mgr_dev_tgt.dev_pipe_id;
}

void build_action_spec(const pi_action_data_t *action_data,
                       const pi_p4info_t *p4info,
                       pipe_action_data_spec_t *pipe_action_data_spec) {
  pi_p4_id_t action_id = action_data->action_id;
  allocate_pipe_action_data_spec(action_id, p4info, pipe_action_data_spec);

  uint8_t *action_data_bits = pipe_action_data_spec->action_data_bits;
  const char *ad_data = action_data->data;

  size_t num_params;
  const pi_p4_id_t *param_ids =
      pi_p4info_action_get_params(p4info, action_id, &num_params);
  for (size_t i = 0; i < num_params; i++) {
    pi_p4_id_t p_id = param_ids[i];
    size_t p_bw = pi_p4info_action_param_bitwidth(p4info, action_id, p_id);
    size_t nbytes = (p_bw + 7) / 8;
    memcpy(action_data_bits, ad_data, nbytes);
    ad_data += nbytes;
    action_data_bits += nbytes;
  }
}

void build_pipe_action_spec_direct(const pi_p4info_t *p4info,
                                   pi_dev_id_t dev_id,
                                   pi_p4_id_t table_id,
                                   const pi_action_data_t *action_data,
                                   pipe_action_spec_t *pipe_action_spec) {
  build_action_spec(action_data, p4info, &pipe_action_spec->act_data);

  pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  pipe_action_spec->resource_count = 0;

  size_t num_indirect_res;
  const pi_state_table_indirect_res_t *indirect_res =
      pi_state_table_indirect_res(dev_id, table_id, &num_indirect_res);
  for (size_t i = 0; i < num_indirect_res; i++) {
    bf_sys_assert(!indirect_res[i].is_match_bound);
    const pi_state_indirect_res_access_info_t *access_info =
        pi_state_indirect_res_access_info(
            dev_id, table_id, action_data->action_id, indirect_res[i].res_id);
    pipe_res_spec_t *res_spec =
        &pipe_action_spec->resources[pipe_action_spec->resource_count++];
    uint32_t handle = pi_state_res_id_to_handle(dev_id, indirect_res[i].res_id);
    res_spec->tbl_hdl = handle;

    if (!access_info) {  // table res not valid for this action
      res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
      continue;
    }

    res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
    res_spec->tbl_idx = retrieve_indirect_res_index(action_data, access_info);
  }
}

void cleanup_pipe_action_spec(pipe_action_spec_t *pipe_action_spec) {
  if (pipe_action_spec->pipe_action_datatype_bmap == PIPE_ACTION_DATA_TYPE) {
    release_pipe_action_data_spec(&pipe_action_spec->act_data);
  }
}

void unbuild_action_spec(pi_p4_id_t action_id,
                         const pi_p4info_t *p4info,
                         const pipe_action_data_spec_t *pipe_action_data_spec,
                         pi_action_data_t *action_data) {
  const uint8_t *action_data_bits = pipe_action_data_spec->action_data_bits;

  char *ad_data = action_data->data;

  size_t num_params;
  const pi_p4_id_t *param_ids =
      pi_p4info_action_get_params(p4info, action_id, &num_params);
  for (size_t i = 0; i < num_params; i++) {
    pi_p4_id_t p_id = param_ids[i];
    size_t p_bw = pi_p4info_action_param_bitwidth(p4info, action_id, p_id);
    size_t nbytes = (p_bw + 7) / 8;
    memcpy(ad_data, action_data_bits, nbytes);
    ad_data += nbytes;
    action_data_bits += nbytes;
  }
}

static void emit_index_to_action_data(size_t idx,
                                      pi_action_data_t *action_data,
                                      size_t offset,
                                      size_t width) {
  char *start = action_data->data + offset;
  bf_sys_assert(width <= sizeof(idx));
  idx = htonll(idx);
  memcpy(start, (char *)(&idx) + sizeof(idx) - width, width);
}

void emit_indirect_res_index(
    size_t idx,
    pi_action_data_t *action_data,
    const pi_state_indirect_res_access_info_t *access_info) {
  bf_sys_assert(access_info);

  switch (access_info->access_mode) {
    case PI_STATE_INDIRECT_RES_ACCESS_MODE_IMMEDIATE:
      return;
    case PI_STATE_INDIRECT_RES_ACCESS_MODE_ARG:
      emit_index_to_action_data(idx,
                                action_data,
                                access_info->index.action_data_pos.offset,
                                access_info->index.action_data_pos.width);
      return;
    default:
      bf_sys_assert(0);
      return;
  }
}

void unbuild_pipe_action_spec_direct(const pi_p4info_t *p4info,
                                     pi_dev_id_t dev_id,
                                     pi_p4_id_t table_id,
                                     pi_p4_id_t action_id,
                                     const pipe_action_spec_t *pipe_action_spec,
                                     pi_action_data_t *action_data) {
  unbuild_action_spec(
      action_id, p4info, &pipe_action_spec->act_data, action_data);

  // write indirect resource indices (from pipe_action_spec->resources) to
  // action_data
  for (int i = 0; i < pipe_action_spec->resource_count; i++) {
    const pipe_res_spec_t *res_spec = &pipe_action_spec->resources[i];
    if (res_spec->tag != PIPE_RES_ACTION_TAG_ATTACHED) continue;
    pi_p4_id_t res_id = pi_state_res_handle_to_id(dev_id, res_spec->tbl_hdl);
    bf_sys_assert(res_id != PI_INVALID_ID);

    const pi_state_indirect_res_access_info_t *access_info =
        pi_state_indirect_res_access_info(dev_id, table_id, action_id, res_id);
    // NULL => direct resources
    if (access_info == NULL) continue;

    emit_indirect_res_index(res_spec->tbl_idx, action_data, access_info);
  }
}

static uint64_t retrieve_index_from_action_data(
    const pi_action_data_t *action_data, size_t offset, size_t width) {
  const char *start = action_data->data + offset;
  uint64_t res = 0;
  bf_sys_assert(width <= sizeof(res));
  memcpy((char *)(&res) + sizeof(res) - width, start, width);
  res = ntohll(res);
  return res;
}

size_t retrieve_indirect_res_index(
    const pi_action_data_t *action_data,
    const pi_state_indirect_res_access_info_t *access_info) {
  bf_sys_assert(access_info);

  switch (access_info->access_mode) {
    case PI_STATE_INDIRECT_RES_ACCESS_MODE_IMMEDIATE:
      return access_info->index.immediate;
    case PI_STATE_INDIRECT_RES_ACCESS_MODE_ARG:
      return retrieve_index_from_action_data(
          action_data,
          access_info->index.action_data_pos.offset,
          access_info->index.action_data_pos.width);
    default:
      bf_sys_assert(0);
      return 0;
  }
}

static pi_indirect_handle_t get_prefix(indirect_h_type_t type) {
  return (((pi_indirect_handle_t)type)
          << (sizeof(pi_indirect_handle_t) * 8 - 1));
}

pi_indirect_handle_t indirect_h_make(pi_indirect_handle_t h,
                                     indirect_h_type_t type) {
  return get_prefix(type) | h;
}

pi_indirect_handle_t indirect_h_clear(pi_indirect_handle_t h) {
  return (~get_prefix(0xff)) & h;
}

int indirect_h_is(pi_indirect_handle_t h, indirect_h_type_t type) {
  return (get_prefix(type) & h) != 0;
}

struct _pipe_mgr_simple_cb_s {
  bf_sys_mutex_t cb_mutex;
  bf_sys_cond_t cb_condvar;
  int cb_status;
};

_pipe_mgr_simple_cb_t *_pipe_mgr_simple_cb_init() {
  _pipe_mgr_simple_cb_t *cb_data = bf_sys_malloc(sizeof(_pipe_mgr_simple_cb_t));
  bf_sys_mutex_init(&cb_data->cb_mutex);
  bf_sys_cond_init(&cb_data->cb_condvar);
  cb_data->cb_status = 0;
  return cb_data;
}

void _pipe_mgr_simple_cb_destroy(_pipe_mgr_simple_cb_t *cb_data) {
  bf_sys_mutex_del(&cb_data->cb_mutex);
  bf_sys_cond_del(&cb_data->cb_condvar);
  bf_sys_free(cb_data);
}

// it is okay if the callback (and the call to notify) happens before the call
// to wait, wait will not block and return immediately (because cb_status == 1)
void _pipe_mgr_simple_cb_wait(_pipe_mgr_simple_cb_t *cb_data) {
  bf_sys_mutex_lock(&cb_data->cb_mutex);
  while (cb_data->cb_status == 0) {
    bf_sys_cond_wait(&cb_data->cb_condvar, &cb_data->cb_mutex);
  }
  bf_sys_mutex_unlock(&cb_data->cb_mutex);
}

void _pipe_mgr_simple_cb_notify(_pipe_mgr_simple_cb_t *cb_data) {
  bf_sys_mutex_lock(&cb_data->cb_mutex);
  bf_sys_assert(cb_data->cb_status == 0);
  cb_data->cb_status = 1;
  bf_sys_cond_wake(&cb_data->cb_condvar);
  bf_sys_mutex_unlock(&cb_data->cb_mutex);
}

void _pipe_mgr_simple_cb_fn(bf_dev_id_t device_id, void *cookie) {
  (void)device_id;
  _pipe_mgr_simple_cb_t *cb_data = (_pipe_mgr_simple_cb_t *)cookie;
  _pipe_mgr_simple_cb_notify(cb_data);
}

#define EBUF_INIT_CAPACITY 1024

char *entry_buffer_create(entry_buffer_t *ebuf) {
  ebuf->capacity = EBUF_INIT_CAPACITY;
  ebuf->size = 0;
  ebuf->buf = bf_sys_malloc(ebuf->capacity);
  return ebuf->buf;
}

char *entry_buffer_extend(entry_buffer_t *ebuf, size_t s) {
  if (s == 0) return NULL;
  while (ebuf->size + s > ebuf->capacity) {
    ebuf->capacity *= 2;
    ebuf->buf = bf_sys_realloc(ebuf->buf, ebuf->capacity);
  }
  size_t prev_size = ebuf->size;
  ebuf->size += s;
  return ebuf->buf + prev_size;
}

void entry_buffer_destroy(entry_buffer_t *ebuf) { bf_sys_free(ebuf->buf); }

#undef EBUF_INIT_SIZE
