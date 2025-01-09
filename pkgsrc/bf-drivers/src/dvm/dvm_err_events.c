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


#include <stdio.h>
#include <stdarg.h>
#include <sched.h>
#include <string.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_sku.h>
#include "dvm.h"

#define BF_ERROR_EVT_MAX_STR_SIZE 400

extern dvm_asic_t *dev_id_set[BF_MAX_DEV_COUNT];

#define DVM_ERR_EVENT_CB(dev) dev_id_set[dev]->event_cb
#define DVM_ERR_EVENT_CB_COOKIE(dev) dev_id_set[dev]->event_cb_cookie

/* Register error event callback function. */
bf_status_t bf_register_error_events(bf_dev_id_t dev,
                                     bf_error_event_cb event_cb,
                                     void *cookie) {
  if (dev >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev]) {
    return BF_OBJECT_NOT_FOUND;
  }

  DVM_ERR_EVENT_CB(dev) = event_cb;
  DVM_ERR_EVENT_CB_COOKIE(dev) = cookie;

  return BF_SUCCESS;
}

/* Notify errors to app */
bf_status_t bf_notify_error_events(bf_error_sev_level_t sev,
                                   bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint8_t stage,
                                   uint64_t address,
                                   bf_error_type_t type,
                                   bf_error_block_t blk,
                                   bf_error_block_location_t loc,
                                   const char *obj_name,
                                   bool all_ports_in_pipe,
                                   const bf_dev_port_t *port_list,
                                   int num_ports,
                                   const char *format,
                                   ...) {
  char tmp_sr[BF_ERROR_EVT_MAX_STR_SIZE];
  va_list vargs;

  tmp_sr[0] = '\0';

  va_start(vargs, format);
  vsnprintf(tmp_sr, sizeof(tmp_sr), format, vargs);
  va_end(vargs);

  if (DVM_ERR_EVENT_CB(dev)) {
    if (!all_ports_in_pipe) {
      bf_sys_assert(port_list);
      bf_sys_assert(num_ports > 0);
      void *cookie = DVM_ERR_EVENT_CB_COOKIE(dev);
      DVM_ERR_EVENT_CB(dev)
      (sev,
       dev,
       pipe,
       stage,
       address,
       type,
       blk,
       loc,
       obj_name,
       port_list,
       num_ports,
       tmp_sr,
       cookie);
    } else {
      bf_dev_port_t port_list_all[BF_PORT_COUNT];
      int num_ports_all = 0;

      memset(port_list_all, 0, sizeof(port_list_all));
      /* Fill all ports in pipe */
      bf_fill_pipe_ports(dev, pipe, &port_list_all[0], &num_ports_all);
      void *cookie = DVM_ERR_EVENT_CB_COOKIE(dev);
      DVM_ERR_EVENT_CB(dev)
      (sev,
       dev,
       pipe,
       stage,
       address,
       type,
       blk,
       loc,
       obj_name,
       port_list_all,
       num_ports_all,
       tmp_sr,
       cookie);
    }
  }

  return BF_SUCCESS;
}

/* Port stuck detection */
#define DVM_PORT_STUCK_CB(dev) dev_id_set[dev]->port_stuck_cb
#define DVM_PORT_STUCK_CB_COOKIE(dev) dev_id_set[dev]->port_stuck_cb_cookie

bf_status_t bf_register_port_stuck_events(bf_dev_id_t dev,
                                          bf_port_stuck_cb port_stuck_cb,
                                          void *cookie) {
  if (dev >= BF_MAX_DEV_COUNT) {
    return BF_INVALID_ARG;
  }

  if (!dev_id_set[dev]) {
    return BF_OBJECT_NOT_FOUND;
  }

  DVM_PORT_STUCK_CB(dev) = port_stuck_cb;
  DVM_PORT_STUCK_CB_COOKIE(dev) = cookie;

  return BF_SUCCESS;
}

bf_status_t bf_notify_port_stuck_events(bf_dev_id_t dev,
                                        bf_dev_port_t port,
                                        ...) {
  if (DVM_PORT_STUCK_CB(dev)) {
    void *cookie = DVM_PORT_STUCK_CB_COOKIE(dev);
    DVM_PORT_STUCK_CB(dev)(dev, port, cookie);
  }
  return BF_SUCCESS;
}

/* Fill all ports in a pipe */
bf_status_t bf_fill_pipe_ports(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               bf_dev_port_t *port_list,
                               int *num_ports) {
  uint32_t pipe_id, port_id;
  bf_dev_port_t port = 0;
  bf_dev_pipe_t port_pipe = 0;
  uint32_t num_subdev = 0;

  if (!dev_id_set[dev]) {
    return BF_OBJECT_NOT_FOUND;
  }

  *num_ports = 0;
  lld_sku_get_num_subdev(dev, &num_subdev, NULL);
  for (pipe_id = 0; pipe_id < (BF_SUBDEV_PIPE_COUNT * num_subdev); pipe_id++) {
    for (port_id = 0; port_id < BF_PIPE_PORT_COUNT; port_id++) {
      if (!dev_id_set[dev]->port[pipe_id][port_id].added) continue;

      port = MAKE_DEV_PORT(pipe_id, port_id);
      port_pipe = DEV_PORT_TO_PIPE(port);

      if (pipe != port_pipe) continue;

      port_list[*num_ports] = port;
      *num_ports = *num_ports + 1;
      if (*num_ports >= BF_PORT_COUNT) {
        return BF_OBJECT_NOT_FOUND;
      }
    }
  }
  return BF_SUCCESS;
}
