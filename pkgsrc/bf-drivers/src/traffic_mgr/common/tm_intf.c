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


#include <string.h>

#include <dvm/bf_drv_intf.h>

#include "tm_ctx.h"
#include <traffic_mgr/tm_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/bf_dev_if.h>
#include "tm_init.h"

bf_status_t bf_tm_init() {
  /* Register with DVM for add/remove device notifications. */
  bf_drv_client_handle_t bf_drv_hdl;
  bf_status_t rc = bf_drv_register("TM", &bf_drv_hdl);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Registration for device and port discovery failed");
    return rc;
  }

  bf_drv_client_callbacks_t callbacks = {0};

  callbacks.device_add = tm_add_device;
  callbacks.device_del = tm_remove_device;
  callbacks.port_add = tm_add_port;
  callbacks.port_del = tm_remove_port;
  callbacks.port_status = tm_update_port_status;
  callbacks.port_admin_state = tm_update_port_admin_state;
  callbacks.lock = tm_lock_device;
  callbacks.unlock_reprogram_core = tm_unlock_device;
  callbacks.core_reset = tm_chip_init_sequence_during_fast_reconfig;
  callbacks.config_complete = tm_config_complete;
  callbacks.push_delta_changes = tm_push_delta_cfg_hitless_restart;
  callbacks.disable_input_pkts = tm_disable_all_port_tx;
  callbacks.warm_init_quick = tm_warm_init_quick;

  // aim_log_flag_set_all("trace", 1);
  rc = bf_drv_client_register_callbacks(
      bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_1);
  if (rc == BF_SUCCESS) {
    LOG_TRACE("TM: Registered for device and port discovery");
  } else {
    LOG_ERROR("TM: Registration for device and port discovery failed");
  }
  return (rc);
}

// UT related functions. Should not be used for any other
// purpose other than UT script/program set model depening
// on test is with model or asic.
void bf_tm_set_ut_mode_as_model(bf_dev_id_t dev) {
  g_tm_ctx[dev]->target = BF_TM_TARGET_MODEL;
}

void bf_tm_set_ut_mode_as_asic(bf_dev_id_t dev) {
  g_tm_ctx[dev]->target = BF_TM_TARGET_ASIC;
}
