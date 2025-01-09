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
#include <PI/pi.h>
#include <PI/target/pi_imp.h>

#include "pi_allocators.h"
#include "pi_helpers.h"
#include "pi_log.h"
#include "pi_packet.h"
#include "pi_state.h"

#include <dvm/bf_drv_intf.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include <tofino/bf_pal/dev_intf.h>

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static pi_status_t pi_bf_assign_device(bf_dev_id_t dev_id,
                                       const pi_p4info_t *p4info,
                                       const char *context_json_path) {
  pi_state_exclusive_lock(dev_id);
  LOG_TRACE("%s: assigning device %u", __func__, dev_id);
  if (pi_state_assign_device(dev_id, p4info, context_json_path)) {
    pi_state_unlock(dev_id);
    return PI_STATUS_TARGET_ERROR;  // need a better error code than this
  }
  pi_state_unlock(dev_id);
  return packet_register_with_pkt_mgr(dev_id);
}

static bf_status_t pi_bf_cb_add_device(bf_dev_id_t dev_id,
                                       bf_dev_family_t dev_family,
                                       bf_device_profile_t *profile,
                                       bf_dma_info_t *dma_info,
                                       bf_dev_init_mode_t warm_init_mode) {
  (void)dev_family;
  (void)dma_info;
  (void)warm_init_mode;
  LOG_TRACE("%s: adding device %u", __func__, dev_id);

  if (profile->num_p4_programs == 0) {
    LOG_TRACE("%s: no P4 profile available, skipping add device", __func__);
    return BF_SUCCESS;
  }

  if ((profile->num_p4_programs != 1) &&
      (profile->p4_programs[DEF_PROGRAM_INDEX].num_p4_pipelines != 1)) {
    LOG_ERROR("%s: error when adding device to PI, only 1 profile supported",
              __func__);
    return PI_STATUS_TARGET_ERROR;
  }
  bf_p4_pipeline_t *pipe_profile =
      &profile->p4_programs[DEF_PROGRAM_INDEX].p4_pipelines[DEF_PROFILE_INDEX];

  pi_status_t pi_status;
  pi_p4info_t *p4info;
  pi_status = pi_add_config_from_file(
      pipe_profile->pi_config_file, PI_CONFIG_TYPE_NATIVE_JSON, &p4info);
  if (pi_status != PI_STATUS_SUCCESS) {
    LOG_ERROR("%s: error when parsing PI config file %s",
              __func__,
              pipe_profile->pi_config_file);
    return pi_status;
  }

  pi_status =
      pi_bf_assign_device(dev_id, p4info, pipe_profile->runtime_context_file);

  if (pi_status != PI_STATUS_SUCCESS) {
    LOG_ERROR("%s: error when assigning device through PI", __func__);
    return pi_status;
  }

  return BF_SUCCESS;
}

static bf_status_t pi_bf_cb_remove_device(bf_dev_id_t dev_id) {
  LOG_TRACE("%s: removing device %u", __func__, dev_id);
  pi_state_exclusive_lock(dev_id);
  if (!pi_state_is_device_assigned(dev_id)) {
    pi_state_unlock(dev_id);
    LOG_TRACE("%s: no P4 profile available, skipping remove device", __func__);
    return BF_SUCCESS;
  }
  pi_state_remove_device(dev_id);
  pi_state_unlock(dev_id);
  return BF_SUCCESS;
}

pi_status_t _pi_init(int *abi_version, void *extra) {
  (void)extra;
  LOG_TRACE("%s", __func__);
  pi_state_init(256);

  /* register callbacks to be notified from bf-drivers when a device is added /
     removed. */
  bf_drv_client_handle_t bf_drv_hdl;
  bf_status_t r = bf_drv_register("pi", &bf_drv_hdl);
  bf_sys_assert(r == BF_SUCCESS);
  bf_drv_client_callbacks_t callbacks = {0};
  callbacks.device_add = pi_bf_cb_add_device;
  callbacks.device_del = pi_bf_cb_remove_device;
  bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_2);

  *abi_version = PI_ABI_VERSION;

  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_assign_device(pi_dev_id_t dev_id,
                              const pi_p4info_t *p4info,
                              pi_assign_extra_t *extra) {
  // this method does not do anything, all the logic is in pi_bf_cb_add_device
  (void)dev_id;
  (void)p4info;
  (void)extra;
  return PI_STATUS_SUCCESS;
}

static pipe_status_t activate_dup_entry_checks(pi_dev_id_t dev_id,
                                               const pi_p4info_t *p4info,
                                               pipe_sess_hdl_t sess) {
  pipe_status_t status;
  for (pi_p4_id_t t_id = pi_p4info_table_begin(p4info);
       t_id != pi_p4info_table_end(p4info);
       t_id = pi_p4info_table_next(p4info, t_id)) {
    // set property for all tables, even if only necessary for ternary tables
    uint32_t table_handle = pi_state_table_id_to_handle(dev_id, t_id);
    pipe_mgr_tbl_prop_value_t property_v;
    property_v.duplicate_check = PIPE_MGR_DUPLICATE_ENTRY_CHECK_ENABLE;
    pipe_mgr_tbl_prop_args_t property_args;
    property_args.value = 0;
    status = pipe_mgr_tbl_set_property(sess,
                                       PI_DEV_ID_TO_PIPE(dev_id),
                                       table_handle,
                                       PIPE_MGR_DUPLICATE_ENTRY_CHECK,
                                       property_v,
                                       property_args);
    if (status != PIPE_SUCCESS) return status;
  }
  return PIPE_SUCCESS;
}

static pi_status_t static_p4_config(pi_dev_id_t dev_id,
                                    const pi_p4info_t *p4info) {
  pipe_status_t status;
  pipe_sess_hdl_t sess;
  status = pipe_mgr_client_init(&sess);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  status = activate_dup_entry_checks(dev_id, p4info, sess);
  if (status != PIPE_SUCCESS) {
    pipe_mgr_client_cleanup(sess);
    return PI_STATUS_TARGET_ERROR + status;
  }
  pipe_mgr_client_cleanup(sess);
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_update_device_start(pi_dev_id_t dev_id,
                                    const pi_p4info_t *p4info,
                                    const char *device_data,
                                    size_t device_data_size) {
  (void)device_data_size;
  bool is_bfrt_present = false;
  LOG_TRACE("%s", __func__);

  bf_status_t status;

  status = bf_pal_device_warm_init_begin(
      dev_id, BF_DEV_WARM_INIT_FAST_RECFG, BF_DEV_SERDES_UPD_NONE, true);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  const char *device_data_curr = device_data;
  uint32_t chunk_size;

// read prog name
#define _PI_UPDATE_MAX_NAME_SIZE 100
  device_data_curr += retrieve_uint32(device_data_curr, &chunk_size);
  if (chunk_size > _PI_UPDATE_MAX_NAME_SIZE) return PI_STATUS_BUFFER_ERROR;
  char prog_name[_PI_UPDATE_MAX_NAME_SIZE + 1];
  strncpy(prog_name, device_data_curr, chunk_size);
  prog_name[chunk_size] = '\0';
  device_data_curr += chunk_size;

#define _PI_UPDATE_MAX_TMP_FILENAME_SIZE (_PI_UPDATE_MAX_NAME_SIZE + 32)

  // cfg data
  device_data_curr += retrieve_uint32(device_data_curr, &chunk_size);
  char cfg_file_name[_PI_UPDATE_MAX_TMP_FILENAME_SIZE];
  snprintf(cfg_file_name,
           _PI_UPDATE_MAX_TMP_FILENAME_SIZE,
           "%s-cfg.bin.XXXXXX",
           prog_name);
  int cfg_fd = mkstemp(cfg_file_name);
  if (cfg_fd == -1) {
    LOG_ERROR("%s: error when trying to create temp config file '%s'",
              __func__,
              cfg_file_name);
    return PI_STATUS_TARGET_ERROR;
  }
  ssize_t written = 0;
  while ((written = write(cfg_fd, device_data_curr, chunk_size)) != -1) {
    device_data_curr += written;
    chunk_size -= written;
    if (chunk_size == 0) break;
  }
  if (written == -1) {
    LOG_ERROR(
        "%s: error when writing config file '%s'", __func__, cfg_file_name);
    close(cfg_fd);
    return PI_STATUS_TARGET_ERROR;
  }

  // p4info native json config
  device_data_curr += retrieve_uint32(device_data_curr, &chunk_size);
  char ctx_file_name[_PI_UPDATE_MAX_TMP_FILENAME_SIZE];
  snprintf(ctx_file_name,
           _PI_UPDATE_MAX_TMP_FILENAME_SIZE,
           "%s-ctx.json.XXXXXX",
           prog_name);
  int ctx_fd = mkstemp(ctx_file_name);
  if (ctx_fd == -1) {
    LOG_ERROR("%s: error when trying to create temp context file '%s'",
              __func__,
              ctx_file_name);
    close(cfg_fd);
    return PI_STATUS_TARGET_ERROR;
  }
  while ((written = write(ctx_fd, device_data_curr, chunk_size)) != -1) {
    device_data_curr += written;
    chunk_size -= written;
    if (chunk_size == 0) break;
  }
  if (written == -1) {
    LOG_ERROR(
        "%s: error when writing context file '%s'", __func__, ctx_file_name);
    close(cfg_fd);
    close(ctx_fd);
    return PI_STATUS_TARGET_ERROR;
  }

  char pi_config_file_name[_PI_UPDATE_MAX_TMP_FILENAME_SIZE];
  snprintf(pi_config_file_name,
           _PI_UPDATE_MAX_TMP_FILENAME_SIZE,
           "%s-pi-config.json.XXXXXX",
           prog_name);
  int pi_config_fd = mkstemp(pi_config_file_name);
  if (pi_config_fd == -1) {
    close(cfg_fd);
    close(ctx_fd);
    return PI_STATUS_TARGET_ERROR;
  }
  if (pi_serialize_config_to_fd(p4info, pi_config_fd, 0) == -1) {
    close(cfg_fd);
    close(ctx_fd);
    close(pi_config_fd);
    return PI_STATUS_TARGET_ERROR;
  }
  bf_device_profile_t device_profile;
  bf_p4_pipeline_t *pipeline_profile =
      &(device_profile.p4_programs[0].p4_pipelines[0]);
  pipeline_profile->p4_pipeline_name[0] = '\0';
  char bfrt_file_name[_PI_UPDATE_MAX_TMP_FILENAME_SIZE];
  int bfrt_fd = -1;
  device_profile.num_p4_programs = 1;
  device_profile.p4_programs[0].bfrt_json_file = NULL;
  device_profile.p4_programs[0].num_p4_pipelines = 1;
  snprintf(
      device_profile.p4_programs[0].prog_name, PROG_NAME_LEN, "%s", prog_name);

  device_data_curr += retrieve_uint32(device_data_curr, &chunk_size);
  if (chunk_size > _PI_UPDATE_MAX_NAME_SIZE) return PI_STATUS_BUFFER_ERROR;
  // If chunk_size is 0, then no p4_pipeline name was sent. Don't try and parse
  // bf-rt.json or the profile name
  if (chunk_size != 0) {
    // read p4_pipeline_name
    char p4_pipeline_name[_PI_UPDATE_MAX_NAME_SIZE + 1];
    strncpy(p4_pipeline_name, device_data_curr, chunk_size);
    p4_pipeline_name[chunk_size] = '\0';
    device_data_curr += chunk_size;

    // bfrt info
    device_data_curr += retrieve_uint32(device_data_curr, &chunk_size);
    is_bfrt_present = true;
    snprintf(bfrt_file_name,
             _PI_UPDATE_MAX_TMP_FILENAME_SIZE,
             "%s-bf-rt.json..XXXXXX",
             prog_name);
    bfrt_fd = mkstemp(bfrt_file_name);
    if (bfrt_fd == -1) {
      LOG_ERROR("%s: error when trying to create temp bfrt file '%s'",
                __func__,
                bfrt_file_name);
      close(cfg_fd);
      close(ctx_fd);
      close(pi_config_fd);
      return PI_STATUS_TARGET_ERROR;
    }
    written = 0;
    while ((written = write(bfrt_fd, device_data_curr, chunk_size)) != -1) {
      device_data_curr += written;
      chunk_size -= written;
      if (chunk_size == 0) break;
    }
    if (written == -1) {
      LOG_ERROR(
          "%s: error when writing bfrt file '%s'", __func__, bfrt_file_name);
      close(cfg_fd);
      close(ctx_fd);
      close(pi_config_fd);
      close(bfrt_fd);
      return PI_STATUS_TARGET_ERROR;
    }

    device_profile.p4_programs[0].bfrt_json_file = bfrt_file_name;

    snprintf(pipeline_profile->p4_pipeline_name,
             _PI_UPDATE_MAX_TMP_FILENAME_SIZE,
             "%s",
             p4_pipeline_name);
  }

  pipeline_profile->num_pipes_in_scope = 0;
  pipeline_profile->cfg_file = cfg_file_name;
  pipeline_profile->runtime_context_file = ctx_file_name;
  pipeline_profile->pi_config_file = pi_config_file_name;
  status = bf_pal_device_add(dev_id, &device_profile);

  unlink(cfg_file_name);
  unlink(ctx_file_name);
  unlink(pi_config_file_name);
  close(cfg_fd);
  close(ctx_fd);
  close(pi_config_fd);
  if (is_bfrt_present) {
    unlink(bfrt_file_name);
    close(bfrt_fd);
  }

  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;

  return static_p4_config(dev_id, p4info);
#undef _PI_UPDATE_MAX_NAME_SIZE
#undef _PI_UPDATE_MAX_TMP_FILENAME_SIZE
}

pi_status_t _pi_update_device_end(pi_dev_id_t dev_id) {
  LOG_TRACE("%s", __func__);
  bf_status_t status = bf_pal_device_warm_init_end(dev_id);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_remove_device(pi_dev_id_t dev_id) {
  // this method does not do anything, all the logic is in
  // pi_bf_cb_remove_device
  (void)dev_id;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_destroy() {
  LOG_TRACE("%s", __func__);
  pi_state_destroy();
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_session_init(pi_session_handle_t *session_handle) {
  LOG_TRACE("%s", __func__);
  pipe_sess_hdl_t sess;
  pipe_status_t status = pipe_mgr_client_init(&sess);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  *session_handle = sess;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_session_cleanup(pi_session_handle_t session_handle) {
  LOG_TRACE("%s", __func__);
  pipe_status_t status = pipe_mgr_client_cleanup(session_handle);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_batch_begin(pi_session_handle_t session_handle) {
  LOG_DBG("%s", __func__);
  pipe_status_t status = pipe_mgr_begin_batch(session_handle);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_batch_end(pi_session_handle_t session_handle, bool hw_sync) {
  LOG_DBG("%s", __func__);
  pipe_status_t status = pipe_mgr_end_batch(session_handle, hw_sync);
  if (status != PIPE_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_packetout_send(pi_dev_id_t dev_id,
                               const char *pkt,
                               size_t size) {
  LOG_DBG("%s", __func__);
  return packet_send_to_pkt_mgr(dev_id, pkt, size);
}

pi_status_t _pi_port_status_get(pi_dev_id_t dev_id,
                                pi_port_t port,
                                pi_port_status_t *port_status) {
  bool state = false;
  bf_status_t status =
      bf_pal_port_oper_state_get(dev_id, (bf_dev_port_t)port, &state);
  *port_status = (state == false) ? PI_PORT_STATUS_DOWN : PI_PORT_STATUS_UP;
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}
