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


/*!
 * @file diag_vlan.c
 * @date
 *
 * Contains implementation of diag vlans
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <pthread.h>
#include <signal.h>
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/bf_dma_if.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "diag_common.h"
#include "diag_pkt.h"
#include "diag_pd.h"
#include "diag_vlan.h"

/* Diag get vlan info */
diag_vlan_t *diag_int_get_vlan_info(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  if (vlan_id >= BF_DIAG_MAX_VLANS) {
    return NULL;
  }
  vlan_p = &(DIAG_DEV_INFO(dev_id)->vlans[vlan_id]);
  if (!vlan_p->valid) {
    return NULL;
  }
  return vlan_p;
}

/* Diag vlan create */
bf_status_t diag_int_vlan_create(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p = NULL;

  vlan_p = &(DIAG_DEV_INFO(dev_id)->vlans[vlan_id]);

  vlan_p->valid = true;
  vlan_p->mc_index = DIAG_GEN_MC_INDEX(vlan_id);
  vlan_p->rid = DIAG_GEN_VLAN_RID(vlan_id);
  return BF_SUCCESS;
}

/* Diag vlan destroy */
bf_status_t diag_int_vlan_destroy(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p = NULL;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  memset(vlan_p, 0, sizeof(diag_vlan_t));
  return BF_SUCCESS;
}

/* Get a vlan port bitmap */
bf_status_t diag_int_get_vlan_port_bitmap(bf_dev_id_t dev_id,
                                          int vlan_id,
                                          uint8_t *port_map,
                                          uint8_t *lag_map) {
  diag_vlan_t *vlan_p = NULL;
  bf_dev_port_t port = 0;
  int index = 0;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  /* Get ports from vlan info */
  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if ((vlan_p->tagged[port].valid) || (vlan_p->untagged[port].valid)) {
      index = DIAG_MAKE_72_PIPE_PORT(DEV_PORT_TO_PIPE(port),
                                     DEV_PORT_TO_LOCAL_PORT(port));
      port_map[index / 8] = (port_map[index / 8] | (1 << (index % 8))) & 0xFF;
    }
  }
  return BF_SUCCESS;
}
