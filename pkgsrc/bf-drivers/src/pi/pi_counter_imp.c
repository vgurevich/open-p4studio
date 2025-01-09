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


#include <PI/p4info.h>
#include <PI/pi.h>
#include <PI/target/pi_counter_imp.h>

#include "pi_helpers.h"
#include "pi_log.h"
#include "pi_resource_specs.h"
#include "pi_state.h"

#include <pipe_mgr/pipe_mgr_intf.h>

static void convert_from_counter_data(const pi_counter_data_t *counter_data,
                                      pipe_stat_data_t *stat_data) {
  if (counter_data->valid & PI_COUNTER_UNIT_BYTES)
    stat_data->bytes = counter_data->bytes;
  if (counter_data->valid & PI_COUNTER_UNIT_PACKETS)
    stat_data->packets = counter_data->packets;
}

pi_status_t _pi_counter_read(pi_session_handle_t session_handle,
                             pi_dev_tgt_t dev_tgt,
                             pi_p4_id_t counter_id,
                             size_t index,
                             int flags,
                             pi_counter_data_t *counter_data) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  uint32_t stat_handle = pi_state_res_id_to_handle(dev_tgt.dev_id, counter_id);
  pipe_status_t status;

  if (flags & PI_COUNTER_FLAGS_HW_SYNC) {
    status = pipe_mgr_stat_ent_database_sync(
        session_handle, pipe_mgr_dev_tgt, stat_handle, index);
    if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  }

  pipe_stat_data_t stat_data = {0};
  pipe_stat_data_t *ptr = &stat_data;
  pipe_stat_ent_idx_t idx = index;
  status = pipe_mgr_stat_ent_query(
      session_handle, pipe_mgr_dev_tgt, stat_handle, &idx, 1, &ptr);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  convert_to_counter_data(p4info, counter_id, &stat_data, counter_data);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_write(pi_session_handle_t session_handle,
                              pi_dev_tgt_t dev_tgt,
                              pi_p4_id_t counter_id,
                              size_t index,
                              const pi_counter_data_t *counter_data) {
  LOG_DBG("%s", __func__);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  uint32_t stat_handle = pi_state_res_id_to_handle(dev_tgt.dev_id, counter_id);
  pipe_stat_data_t stat_data;
  convert_from_counter_data(counter_data, &stat_data);

  pipe_status_t status = pipe_mgr_stat_ent_set(
      session_handle, pipe_mgr_dev_tgt, stat_handle, index, &stat_data);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_read_direct(pi_session_handle_t session_handle,
                                    pi_dev_tgt_t dev_tgt,
                                    pi_p4_id_t counter_id,
                                    pi_entry_handle_t entry_handle,
                                    int flags,
                                    pi_counter_data_t *counter_data) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  pi_p4_id_t table_id = pi_p4info_counter_get_direct(p4info, counter_id);
  // checked by PI common code
  bf_sys_assert(table_id != PI_INVALID_ID);
  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_status_t status;

  if (flags & PI_COUNTER_FLAGS_HW_SYNC) {
    status = pipe_mgr_direct_stat_ent_database_sync(
        session_handle, pipe_mgr_dev_tgt, table_handle, entry_handle);
    if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  }

  pipe_stat_data_t stat_data;
  status = pipe_mgr_mat_ent_direct_stat_query(session_handle,
                                              PI_DEV_ID_TO_PIPE(dev_tgt.dev_id),
                                              table_handle,
                                              entry_handle,
                                              &stat_data);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  convert_to_counter_data(p4info, counter_id, &stat_data, counter_data);

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_write_direct(pi_session_handle_t session_handle,
                                     pi_dev_tgt_t dev_tgt,
                                     pi_p4_id_t counter_id,
                                     pi_entry_handle_t entry_handle,
                                     const pi_counter_data_t *counter_data) {
  LOG_DBG("%s", __func__);

  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);

  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  pi_p4_id_t table_id = pi_p4info_counter_get_direct(p4info, counter_id);
  // checked by PI common code
  bf_sys_assert(table_id != PI_INVALID_ID);
  uint32_t table_handle = pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
  pipe_stat_data_t stat_data;
  convert_from_counter_data(counter_data, &stat_data);

  pipe_status_t status = pipe_mgr_mat_ent_direct_stat_set(
      session_handle, dev_tgt.dev_id, table_handle, entry_handle, &stat_data);

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  return PI_STATUS_SUCCESS;
}

typedef struct {
  PICounterHwSyncCb cb;
  void *cb_cookie;
  pi_p4_id_t counter_id;
} _counter_hw_sync_cb_t;

static void _pipe_mgr_sync_cb(bf_dev_id_t device_id, void *cookie) {
  _counter_hw_sync_cb_t *cb_data = (_counter_hw_sync_cb_t *)cookie;
  cb_data->cb(device_id, cb_data->counter_id, cb_data->cb_cookie);
  bf_sys_free(cb_data);
}

static pi_status_t pi_counter_hw_sync_blocking(
    pi_session_handle_t session_handle,
    pi_dev_tgt_t dev_tgt,
    pi_p4_id_t counter_id) {
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);
  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  pi_p4_id_t table_id = pi_p4info_counter_get_direct(p4info, counter_id);
  _pipe_mgr_simple_cb_t *cb_data = _pipe_mgr_simple_cb_init();
  pipe_status_t status;
  if (table_id == PI_INVALID_ID) {  // indirect counter
    uint32_t stat_handle =
        pi_state_res_id_to_handle(dev_tgt.dev_id, counter_id);
    status = pipe_mgr_stat_database_sync(session_handle,
                                         pipe_mgr_dev_tgt,
                                         stat_handle,
                                         _pipe_mgr_simple_cb_fn,
                                         cb_data);
  } else {  // direct counter
    uint32_t table_handle =
        pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
    status = pipe_mgr_direct_stat_database_sync(session_handle,
                                                pipe_mgr_dev_tgt,
                                                table_handle,
                                                _pipe_mgr_simple_cb_fn,
                                                cb_data);
  }
  _pipe_mgr_simple_cb_wait(cb_data);
  _pipe_mgr_simple_cb_destroy(cb_data);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_counter_hw_sync(pi_session_handle_t session_handle,
                                pi_dev_tgt_t dev_tgt,
                                pi_p4_id_t counter_id,
                                PICounterHwSyncCb cb,
                                void *cb_cookie) {
  LOG_DBG("%s", __func__);
  if (cb == NULL)
    return pi_counter_hw_sync_blocking(session_handle, dev_tgt, counter_id);
  const pi_p4info_t *p4info = pi_get_device_p4info(dev_tgt.dev_id);
  dev_target_t pipe_mgr_dev_tgt;
  convert_dev_tgt(dev_tgt, &pipe_mgr_dev_tgt);
  pi_p4_id_t table_id = pi_p4info_counter_get_direct(p4info, counter_id);
  _counter_hw_sync_cb_t *cb_data = bf_sys_malloc(sizeof(_counter_hw_sync_cb_t));
  cb_data->cb = cb;
  cb_data->cb_cookie = cb_cookie;
  cb_data->counter_id = counter_id;
  pipe_status_t status;
  if (table_id == PI_INVALID_ID) {  // indirect counter
    uint32_t stat_handle =
        pi_state_res_id_to_handle(dev_tgt.dev_id, counter_id);
    status = pipe_mgr_stat_database_sync(session_handle,
                                         pipe_mgr_dev_tgt,
                                         stat_handle,
                                         _pipe_mgr_sync_cb,
                                         cb_data);
  } else {  // direct counter
    uint32_t table_handle =
        pi_state_table_id_to_handle(dev_tgt.dev_id, table_id);
    status = pipe_mgr_direct_stat_database_sync(session_handle,
                                                pipe_mgr_dev_tgt,
                                                table_handle,
                                                _pipe_mgr_sync_cb,
                                                cb_data);
  }

  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}
