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


#include <PI/pi_base.h>
#include <PI/pi_mc.h>
#include <PI/target/pi_mc_imp.h>

#include "pi_log.h"

#include <mc_mgr/mc_mgr_intf.h>
#include <mc_mgr/mc_mgr_types.h>

#include <stdbool.h>

pi_status_t _pi_mc_session_init(pi_mc_session_handle_t *session_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status = bf_mc_create_session(session_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_session_cleanup(pi_mc_session_handle_t session_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status;
  status = bf_mc_complete_operations(session_handle);
  if (status != BF_SUCCESS)
    LOG_ERROR("%s: bf_mc_destroy_session failed", __func__);
  status = bf_mc_destroy_session(session_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_create(pi_mc_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_mc_grp_id_t grp_id,
                              pi_mc_grp_handle_t *grp_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status =
      bf_mc_mgrp_create(session_handle, dev_id, grp_id, grp_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_delete(pi_mc_session_handle_t session_handle,
                              pi_dev_id_t dev_id,
                              pi_mc_grp_handle_t grp_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status = bf_mc_mgrp_destroy(session_handle, dev_id, grp_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

// returns true iff conversion is successful
static bool convert_port_map(const pi_mc_port_t *eg_ports,
                             size_t eg_ports_count,
                             bf_mc_port_map_t *port_map) {
  BF_MC_PORT_MAP_INIT(*port_map);
  for (size_t i = 0; i < eg_ports_count; i++) {
    if (!DEV_PORT_VALIDATE(eg_ports[i])) {
      LOG_ERROR("%s: port %d is out-of-range", __func__, eg_ports[i]);
      return false;
    }
    BF_MC_PORT_MAP_SET(*port_map, eg_ports[i]);
  }
  return true;
}

pi_status_t _pi_mc_node_create(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_rid_t rid,
                               size_t eg_ports_count,
                               const pi_mc_port_t *eg_ports,
                               pi_mc_node_handle_t *node_handle) {
  LOG_TRACE("%s", __func__);
  bf_mc_port_map_t port_map;
  if (!convert_port_map(eg_ports, eg_ports_count, &port_map))
    return PI_STATUS_TARGET_ERROR;
  bf_mc_lag_map_t lag_map;
  BF_MC_LAG_MAP_INIT(lag_map);
  bf_status_t status = bf_mc_node_create(
      session_handle, dev_id, rid, port_map, lag_map, node_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_node_modify(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_node_handle_t node_handle,
                               size_t eg_ports_count,
                               const pi_mc_port_t *eg_ports) {
  LOG_TRACE("%s", __func__);
  bf_mc_port_map_t port_map;
  if (!convert_port_map(eg_ports, eg_ports_count, &port_map))
    return PI_STATUS_TARGET_ERROR;
  bf_mc_lag_map_t lag_map;
  BF_MC_LAG_MAP_INIT(lag_map);
  bf_status_t status =
      bf_mc_node_update(session_handle, dev_id, node_handle, port_map, lag_map);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_node_delete(pi_mc_session_handle_t session_handle,
                               pi_dev_id_t dev_id,
                               pi_mc_node_handle_t node_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status = bf_mc_node_destroy(session_handle, dev_id, node_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_attach_node(pi_mc_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_mc_grp_handle_t grp_handle,
                                   pi_mc_node_handle_t node_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status = bf_mc_associate_node(
      session_handle, dev_id, grp_handle, node_handle, false, 0);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}

pi_status_t _pi_mc_grp_detach_node(pi_mc_session_handle_t session_handle,
                                   pi_dev_id_t dev_id,
                                   pi_mc_grp_handle_t grp_handle,
                                   pi_mc_node_handle_t node_handle) {
  LOG_TRACE("%s", __func__);
  bf_status_t status =
      bf_mc_dissociate_node(session_handle, dev_id, grp_handle, node_handle);
  if (status != BF_SUCCESS) return PI_STATUS_TARGET_ERROR + status;
  return PI_STATUS_SUCCESS;
}
