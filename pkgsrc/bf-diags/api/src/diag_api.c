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
 * @file diag_api.c
 * @date
 *
 * Contains implementation of diag APIs
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <signal.h>
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/bf_dma_if.h>
#include <port_mgr/bf_port_if.h>
#include <port_mgr/bf_serdes_if.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <tofino/bf_pal/bf_pal_port_intf.h>
#include "diags/bf_diag_api.h"
#include "diag_util.h"
#include "diag_common.h"
#include "diag_pd.h"
#include "diag_vlan.h"
#include "diag_pkt.h"
#include "diag_create_pkt.h"
#include "diag_pkt_database.h"

/* Create vlan */
bf_status_t bf_diag_vlan_create(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p = NULL;

  if (vlan_id > BF_DIAG_MAX_VLANS) {
    return BF_INVALID_ARG;
  }

  if (diag_int_get_vlan_info(dev_id, vlan_id)) {
    return BF_ALREADY_EXISTS;
  }

  diag_int_vlan_create(dev_id, vlan_id);
  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  /* Create the vlan to mc-index entry, currently vlan==mac_index */
  diag_pd_add_bd_flood(
      dev_id, vlan_id, vlan_p->mc_index, &(vlan_p->bd_flood_entry_hdl));

  return BF_SUCCESS;
}

/* Get first vlan */
bf_status_t bf_diag_vlan_get_first(bf_dev_id_t dev_id, int *first_vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  int vlan_id = 0;
  bf_status_t status = BF_SUCCESS;

  *first_vlan_id = -1;
  for (vlan_id = 0; vlan_id < BF_DIAG_MAX_VLANS; vlan_id++) {
    vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
    if (!vlan_p) {
      continue;
    }
    *first_vlan_id = vlan_id;
    break;
  }

  return status;
}

/* Get next vlan */
bf_status_t bf_diag_vlan_get_next(bf_dev_id_t dev_id,
                                  int input_vlan_id,
                                  int num_entries,
                                  int *next_vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  int vlan_id = 0;
  bf_status_t status = BF_SUCCESS;
  int count = 0;

  next_vlan_id[count] = -1;
  for (vlan_id = input_vlan_id + 1; vlan_id < BF_DIAG_MAX_VLANS; vlan_id++) {
    vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
    if (!vlan_p) {
      continue;
    }
    next_vlan_id[count] = vlan_id;
    count++;
    if (count >= num_entries) {
      break;
    }
  }
  if (count < num_entries) {
    next_vlan_id[count] = -1;
  }

  return status;
}

/* Add port to vlan */
bf_status_t bf_diag_port_vlan_add(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  int vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  diag_port_t *port_p = NULL;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  /* Tagged ports */
  port_p = diag_get_port_info(dev_id, port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", port);
    return BF_INVALID_ARG;
  }

  if (!(vlan_p->tagged[port].valid)) {
    p4_pd_entry_hdl_t entry_hdl = 0;
    // DIAG_PRINT("Adding tagged port %d to vlan %d \n", port, vlan_id);
    vlan_p->tagged[port].valid = true;
    diag_pd_add_port_vlan_mapping(dev_id,
                                  vlan_id,
                                  vlan_p->rid,
                                  port,
                                  &(vlan_p->tagged[port].port_vlan_entry_hdl));
    diag_pd_add_vlan_encap(dev_id, port, vlan_id, &entry_hdl);
  }

  return diag_update_vlan_flood_list(dev_id, vlan_id);
}

/* Remove port from vlan */
bf_status_t bf_diag_port_vlan_del(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  int vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  diag_port_t *port_p = NULL;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  /* Tagged ports */
  port_p = diag_get_port_info(dev_id, port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", port);
    return BF_INVALID_ARG;
  }
  if (vlan_p->tagged[port].valid) {
    diag_pd_del_port_vlan_mapping(
        dev_id, vlan_id, port, vlan_p->tagged[port].port_vlan_entry_hdl);
    vlan_p->tagged[port].valid = false;
    vlan_p->tagged[port].port_vlan_entry_hdl = 0;
    diag_pd_del_vlan_encap_by_match_spec(dev_id, port, vlan_id);
    // DIAG_PRINT("Deleting tagged port %d from vlan %d \n", port, vlan_id);
  }

  return diag_update_vlan_flood_list(dev_id, vlan_id);
}

/* Get first port on vlan */
bf_status_t bf_diag_vlan_port_get_first(bf_dev_id_t dev_id,
                                        int vlan_id,
                                        bf_dev_port_t *first_port) {
  diag_vlan_t *vlan_p = NULL;
  bf_dev_port_t port = 0;
  bf_status_t status = BF_SUCCESS;

  *first_port = -1;
  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if (vlan_p->tagged[port].valid) {
      *first_port = port;
      break;
    }
  }

  return status;
}

/* Get next port on vlan */
bf_status_t bf_diag_vlan_port_get_next(bf_dev_id_t dev_id,
                                       int vlan_id,
                                       bf_dev_port_t input_port,
                                       int num_entries,
                                       bf_dev_port_t *next_ports) {
  diag_vlan_t *vlan_p = NULL;
  bf_dev_port_t port = 0;
  bf_status_t status = BF_SUCCESS;
  int count = 0;

  next_ports[count] = -1;
  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  for (port = input_port + 1; port < BF_DIAG_MAX_PORTS; port++) {
    if (vlan_p->tagged[port].valid) {
      next_ports[count] = port;
      count++;
    }
    if (count >= num_entries) {
      break;
    }
  }
  if (count < num_entries) {
    next_ports[count] = -1;
  }

  return status;
}

/* Add default vlan to port to vlan */
bf_status_t bf_diag_port_default_vlan_set(bf_dev_id_t dev_id,
                                          bf_dev_port_t port,
                                          int vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  diag_port_t *port_p = NULL;
  bf_status_t status = BF_SUCCESS;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  port_p = diag_get_port_info(dev_id, port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", port);
    return BF_INVALID_ARG;
  }
  if (port_p->default_vlan) {
    if (port_p->default_vlan == vlan_id) {
      return BF_SUCCESS;
    } else {
      return BF_INVALID_ARG;
    }
  }

  /* Add Untagged ports */
  if (!(vlan_p->untagged[port].valid)) {
    port_p->default_vlan = vlan_id;
    // DIAG_PRINT("Adding default vlan %d to port %d\n", vlan_id, port);
    vlan_p->untagged[port].valid = true;
    status = diag_pd_add_default_vlan(
        dev_id, vlan_id, vlan_p->rid, port, &(port_p->def_vlan_entry_hdl));
    status |= diag_update_vlan_flood_list(dev_id, vlan_id);
  }

  return status;
}

/* Remove default vlan from port */
bf_status_t bf_diag_port_default_vlan_reset(bf_dev_id_t dev_id,
                                            bf_dev_port_t port) {
  diag_vlan_t *vlan_p = NULL;
  diag_port_t *port_p = NULL;
  bf_status_t status = BF_SUCCESS;
  int vlan_id = 0;

  port_p = diag_get_port_info(dev_id, port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", port);
    return BF_INVALID_ARG;
  }
  if (!port_p->default_vlan) {
    return BF_INVALID_ARG;
  }
  vlan_id = port_p->default_vlan;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  /* Untagged ports */
  if (vlan_p->untagged[port].valid) {
    // DIAG_PRINT("Removing default vlan %d from port %d \n", vlan_id, port);
    vlan_p->untagged[port].valid = false;
    status = diag_pd_del_default_vlan(
        dev_id, vlan_id, port, port_p->def_vlan_entry_hdl);
    port_p->default_vlan = 0;
    port_p->def_vlan_entry_hdl = 0;
    status |= diag_update_vlan_flood_list(dev_id, vlan_id);
  }
  return status;
}

/* Get default vlan */
int bf_diag_port_default_vlan_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  int *vlan) {
  diag_port_t *port_p = NULL;

  *vlan = 0;
  port_p = diag_get_port_info(dev_id, port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", port);
    return BF_INVALID_ARG;
  }
  if (!port_p->default_vlan) {
    return BF_INVALID_ARG;
  }
  *vlan = port_p->default_vlan;

  return BF_SUCCESS;
}

/* Get first untagged port on vlan */
bf_status_t bf_diag_default_vlan_port_get_first(bf_dev_id_t dev_id,
                                                int vlan_id,
                                                bf_dev_port_t *first_port) {
  diag_vlan_t *vlan_p = NULL;
  bf_dev_port_t port = 0;
  bf_status_t status = BF_SUCCESS;

  *first_port = -1;
  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if (vlan_p->untagged[port].valid) {
      *first_port = port;
      break;
    }
  }

  return status;
}

/* Get next untagged port on vlan */
bf_status_t bf_diag_default_vlan_port_get_next(bf_dev_id_t dev_id,
                                               int vlan_id,
                                               bf_dev_port_t input_port,
                                               int num_entries,
                                               bf_dev_port_t *next_ports) {
  diag_vlan_t *vlan_p = NULL;
  bf_dev_port_t port = 0;
  bf_status_t status = BF_SUCCESS;
  int count = 0;

  next_ports[count] = -1;
  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  for (port = input_port + 1; port < BF_DIAG_MAX_PORTS; port++) {
    if (vlan_p->untagged[port].valid) {
      next_ports[count] = port;
      count++;
    }
    if (count >= num_entries) {
      break;
    }
  }
  if (count < num_entries) {
    next_ports[count] = -1;
  }

  return status;
}

/* Destroy vlan */
bf_status_t bf_diag_vlan_destroy(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p = NULL;
  diag_port_t *port_p = NULL;
  bf_dev_port_t port = 0;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }
    if (vlan_p->tagged[port].valid) {
      bf_diag_port_vlan_del(dev_id, port, vlan_id);
    }
    if (vlan_p->untagged[port].valid) {
      bf_diag_port_default_vlan_reset(dev_id, port);
    }
  }
  /* Remove the flood entry */
  diag_pd_del_bd_flood(
      dev_id, vlan_id, vlan_p->mc_index, vlan_p->bd_flood_entry_hdl);
  vlan_p->bd_flood_entry_hdl = 0;
  diag_pd_del_vlan_flood_ports(dev_id,
                               vlan_id,
                               vlan_p->mc_index,
                               vlan_p->mc_grp_hdl,
                               vlan_p->mc_node_hdl);
  vlan_p->mc_grp_hdl = 0;
  vlan_p->mc_node_hdl = 0;
  diag_int_vlan_destroy(dev_id, vlan_id);

  return BF_SUCCESS;
}

/* Set the loopback mode */
bf_status_t bf_diag_port_loopback_mode_set(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_diag_port_lpbk_mode_e diag_loop_mode) {
  bf_loopback_mode_e loop_mode = BF_LPBK_NONE;
  diag_port_t *port_p = NULL;
  bool is_internal = false;
  bf_status_t status = BF_SUCCESS;
  bf_status_t sts = BF_SUCCESS;

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_EXT) {
    return BF_SUCCESS;
  }
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }

  if (diag_validate_loopback_mode(dev_id, diag_loop_mode) != BF_SUCCESS) {
    DIAG_PRINT("Loopback mode %d not supported on this chip \n",
               diag_loop_mode);
    return BF_INVALID_ARG;
  }

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    lld_sku_is_dev_port_internal(dev_id, dev_port, &is_internal);
    if (is_internal) {
      /* Do not set the loopback mode for internal ports to LOOPBACK_NONE */
      return BF_SUCCESS;
    }
  }

  port_p = diag_get_port_info(dev_id, dev_port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", dev_port);
    return BF_INVALID_ARG;
  }

  /* Do not set loopback mode on model.
     Model disables channels to set loopback mode and this
     leads to packet drops.
  */
  if (DIAG_DEV_IS_SW_MODEL(dev_id)) {
    port_p->loopback_mode = diag_loop_mode;
    return BF_SUCCESS;
  }

  if (!diag_is_chip_family_tofino1(dev_id)) {
    sts = bf_pal_port_disable(dev_id, dev_port);
    if (sts != BF_SUCCESS) {
      DIAG_PRINT("Port Disable failed for dev port %d \n", dev_port);
    }
    // TF3 needs some delay between port disable and enable
    if (diag_is_chip_family_tofino3(dev_id)) {
      bf_sys_usleep(10000);
    }
  }

  /* Reset the loop mode if it is already set */
  if (port_p->loopback_mode) {
    port_p->loopback_mode = BF_DIAG_PORT_LPBK_NONE;
    loop_mode = diag_get_bf_loop_mode(diag_loop_mode);
    /* Set the loopback mode */
    bf_pal_port_loopback_mode_set(dev_id, dev_port, loop_mode);
  }
  port_p->loopback_mode = diag_loop_mode;
  loop_mode = diag_get_bf_loop_mode(diag_loop_mode);
  /* Set the loopback mode */
  status = bf_pal_port_loopback_mode_set(dev_id, dev_port, loop_mode);

  if (!diag_is_chip_family_tofino1(dev_id)) {
    sts = bf_pal_port_enable(dev_id, dev_port);
    if (sts != BF_SUCCESS) {
      DIAG_PRINT("Port Enable failed for dev port %d \n", dev_port);
    }
  }

  return status;
}

/* Get the loopback mode */
bf_status_t bf_diag_port_loopback_mode_get(
    bf_dev_id_t dev_id,
    bf_dev_port_t dev_port,
    bf_diag_port_lpbk_mode_e *diag_loop_mode) {
  diag_port_t *port_p = NULL;

  *diag_loop_mode = BF_DIAG_PORT_LPBK_NONE;
  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }
  port_p = diag_get_port_info(dev_id, dev_port);
  if (!port_p) {
    return BF_INVALID_ARG;
  }

  *diag_loop_mode = port_p->loopback_mode;
  return BF_SUCCESS;
}

/* Fwd pkt from any port to any port with filter */
bf_status_t bf_diag_forwarding_rule_add(bf_dev_id_t dev_id,
                                        bf_dev_port_t ig_port,
                                        bf_dev_port_t eg_port,
                                        uint32_t tcp_dstPort_start,
                                        uint32_t tcp_dstPort_end,
                                        int priority,
                                        p4_pd_entry_hdl_t *entry_hdl) {
  diag_port_t *port_p = NULL;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, ig_port)) {
    return BF_INVALID_ARG;
  }
  if (!DIAG_DEVPORT_VALID(dev_id, eg_port)) {
    return BF_INVALID_ARG;
  }
  return diag_pd_add_dst_override(dev_id,
                                  ig_port,
                                  eg_port,
                                  tcp_dstPort_start,
                                  tcp_dstPort_end,
                                  priority,
                                  entry_hdl);
}

/* Remove Fwding entry */
bf_status_t bf_diag_forwarding_rule_del(bf_dev_id_t dev_id,
                                        p4_pd_entry_hdl_t entry_hdl) {
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (!entry_hdl) {
    return BF_INVALID_ARG;
  }

  return diag_pd_del_dst_override(dev_id, entry_hdl);
}

/* Fwd pkt from any port to a MC group */
bf_status_t bf_diag_mc_forwarding_rule_add(bf_dev_id_t dev_id,
                                           bf_dev_port_t ig_port,
                                           bf_mc_grp_id_t mc_grp_id_a,
                                           bf_mc_grp_id_t mc_grp_id_b,
                                           uint32_t tcp_dstPort_start,
                                           uint32_t tcp_dstPort_end,
                                           int priority,
                                           p4_pd_entry_hdl_t *entry_hdl) {
  diag_port_t *port_p = NULL;

  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, ig_port)) {
    return BF_INVALID_ARG;
  }
  port_p = diag_get_port_info(dev_id, ig_port);
  if (!port_p) {
    DIAG_PRINT("Port %d does not exist \n", ig_port);
    return BF_INVALID_ARG;
  }
  return diag_pd_mc_add_dst_override(dev_id,
                                     ig_port,
                                     mc_grp_id_a,
                                     mc_grp_id_b,
                                     tcp_dstPort_start,
                                     tcp_dstPort_end,
                                     priority,
                                     entry_hdl);
}

/* Remove Fwding entry by match spec */
bf_status_t bf_diag_forwarding_rule_del_by_match_spec(
    bf_dev_id_t dev_id,
    bf_dev_port_t ig_port,
    uint32_t tcp_dstPort_start,
    uint32_t tcp_dstPort_end,
    int priority) {
  /* Validate device */
  if (!DIAG_DEVPORT_VALID(dev_id, ig_port)) {
    return BF_INVALID_ARG;
  }

  return diag_pd_del_dst_override_by_match_spec(
      dev_id, ig_port, tcp_dstPort_start, tcp_dstPort_end, priority);
}

/* loopback test init */
bf_status_t bf_diag_loopback_test_setup(bf_dev_id_t dev_id,
                                        bf_dev_port_t *port_list,
                                        int num_ports,
                                        bf_diag_port_lpbk_mode_e diag_loop_mode,
                                        bf_diag_sess_hdl_t *sess_hdl) {
  bf_dev_port_t port = 0;
  diag_port_t *port_p = NULL;
  int priority = 0, port_idx = 0;
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t overlap_hdl = 0;

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    DIAG_PRINT("Invalid loopback mode: None \n");
    return BF_INVALID_ARG;
  }
  if (diag_validate_loopback_mode(dev_id, diag_loop_mode) != BF_SUCCESS) {
    DIAG_PRINT("Loopback mode %d not supported on this chip \n",
               diag_loop_mode);
    return BF_INVALID_ARG;
  }
  if ((!port_list) || (num_ports <= 0) || (num_ports > BF_DIAG_MAX_PORTS)) {
    return BF_INVALID_ARG;
  }
  /* Validate ports */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      DIAG_PRINT("Port %d does not exist \n", port);
      return BF_INVALID_ARG;
    }
  }

  /* Check for overlap with existing configs */
  status = diag_session_config_overlap_check(
      dev_id, port_list, num_ports, diag_loop_mode, &overlap_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT(
        "Port Config overlaps with an existing session %u on dev %d with "
        "different loopback mode\n",
        overlap_hdl,
        dev_id);
    return BF_INVALID_ARG;
  }

  status = diag_session_hdl_alloc(dev_id, sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Failed to allocate session on dev %d\n", dev_id);
    return status;
  }

  /* Save the command */
  status = diag_save_loopback_test_params(dev_id,
                                          *sess_hdl,
                                          port_list,
                                          num_ports,
                                          diag_loop_mode,
                                          DIAG_TEST_LOOPBACK);
  if (status != BF_SUCCESS) {
    diag_session_hdl_free(*sess_hdl);
    return status;
  }

  /* Setup config */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);

    /* Clear the stats */
    diag_cpu_port_stats_clear(dev_id, port, *sess_hdl, false);
    if (!diag_port_in_multiple_sessions(dev_id, port)) {
      /* Set the loopback mode only the first time this port is
         added to any session
     */
      bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
    }
    /* Fwd any pkt on port to CPU */
    bf_diag_forwarding_rule_add(dev_id,
                                port,
                                diag_cpu_port_get(dev_id, port),
                                BF_DIAG_TCP_DSTPORT_MIN,
                                BF_DIAG_TCP_DSTPORT_MAX,
                                priority,
                                &(port_p->to_dst_override_hdl[*sess_hdl]));
    /* Fwd any pkt from CPU to port */
    bf_diag_forwarding_rule_add(dev_id,
                                diag_cpu_port_get(dev_id, port),
                                port,
                                DIAG_PORT_TCP_DSTPORT_START(port, *sess_hdl),
                                DIAG_PORT_TCP_DSTPORT_END(port, *sess_hdl),
                                priority,
                                &(port_p->from_src_override_hdl[*sess_hdl]));
  }
  /* Sleep for sometime as ports flap when loopback mode is set */
  if (diag_loop_mode != BF_DIAG_PORT_LPBK_EXT) {
    bf_sys_usleep(2000000);
  }

  return BF_SUCCESS;
}

bf_status_t bf_diag_loopback_test_start(bf_diag_sess_hdl_t sess_hdl,
                                        uint32_t num_packet,
                                        uint32_t pkt_size) {
  bf_diag_test_status_e test_status;
  bf_status_t status = BF_SUCCESS;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    return BF_INVALID_ARG;
  }
  if ((pkt_size < DIAG_MIN_PKT_SIZE) && (pkt_size != 0)) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback\n", sess_hdl);
    return BF_INVALID_ARG;
  }

  /* Make sure test is not in progress */
  test_status = bf_diag_loopback_test_status_get(sess_hdl);
  if (test_status == BF_DIAG_TEST_STATUS_IN_PROGRESS) {
    return BF_ALREADY_EXISTS;
  }
  /* Save the command */
  status = diag_save_runtime_loopback_test_params(dev_id,
                                                  sess_hdl,
                                                  pkt_size,
                                                  num_packet,
                                                  BF_DIAG_TCP_DSTPORT_MIN,
                                                  BF_DIAG_TCP_DSTPORT_MAX,
                                                  false);
  if (status != BF_SUCCESS) {
    return status;
  }
  /* Call the pkt tx fn */
  status = diag_pkt_tx_creator(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Pkt injection failed \n");
    return status;
  }

  return BF_SUCCESS;
}

/* Abort loopback test */
bf_status_t bf_diag_loopback_test_abort(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

/* Get loopback test status */
bf_diag_test_status_e bf_diag_loopback_test_status_get(
    bf_diag_sess_hdl_t sess_hdl) {
  bf_dev_port_t port = 0;
  int port_idx = 0;
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback\n", sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  for (port_idx = 0; port_idx < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
       port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    /* Get the stats */
    test_status = bf_diag_loopback_test_port_status_get(sess_hdl, port, &stats);
    if (test_status != BF_DIAG_TEST_STATUS_PASS) {
      return test_status;
    }
  }

  return BF_DIAG_TEST_STATUS_PASS;
}

/* Get loopback test status per port */
bf_diag_test_status_e bf_diag_loopback_test_port_status_get(
    bf_diag_sess_hdl_t sess_hdl,
    bf_dev_port_t port,
    bf_diag_port_stats_t *stats) {
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  if (!stats) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEVPORT_VALID(dev_id, port)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback\n", sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  /* Get the stats */
  diag_cpu_port_stats_get(dev_id, port, sess_hdl, stats);

  if ((stats->tx_total) != (stats->rx_good)) {
    return BF_DIAG_TEST_STATUS_FAIL;
  } else if ((stats->tx_total == 0) &&
             diag_any_pkt_size_sent_valid(sess_info)) {
    /* Start test was done, but no pkts were transmitted */
    return BF_DIAG_TEST_STATUS_FAIL;
  }

  return BF_DIAG_TEST_STATUS_PASS;
}

/* Cleanup loopback test */
bf_status_t bf_diag_loopback_test_cleanup(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t port = 0;
  int port_idx = 0;
  bf_diag_port_lpbk_mode_e diag_loop_mode = BF_DIAG_PORT_LPBK_NONE;
  diag_port_t *port_p = NULL;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback\n", sess_hdl);
    return BF_INVALID_ARG;
  }

  /* Terminate the thread */
  bf_diag_loopback_test_abort(sess_hdl);
  DIAG_TX_PKT_COMPLETIONS(dev_id) = 0;

  if (!(DIAG_GET_LOOPBACK_PARAMS(sess_info).valid)) {
    return BF_INVALID_ARG;
  }

  for (port_idx = 0; port_idx < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
       port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode !=
        BF_DIAG_PORT_LPBK_EXT) {
      /* If there is another session using this port, don't change the
         loopback mode yet.
      */
      if (!diag_port_in_multiple_sessions(dev_id, port)) {
        /* Set the loopback mode */
        bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
      }
    }
    /* Remove Fwd to CPU entry */
    if (port_p->to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->to_dst_override_hdl[sess_hdl]);
      port_p->to_dst_override_hdl[sess_hdl] = 0;
    }
    /* Remove Fwd from CPU entry */
    if (port_p->from_src_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->from_src_override_hdl[sess_hdl]);
      port_p->from_src_override_hdl[sess_hdl] = 0;
    }
    /* Clear stats */
    diag_cpu_port_stats_clear(dev_id, port, sess_hdl, false);
  }

  DIAG_LOOPBACK_PARAMS_CLEAR(sess_info);
  // Cleanup the pkt database that we created
  diag_cleanup_pkt_database(sess_hdl);
  diag_session_info_del(sess_hdl);
  diag_session_hdl_free(sess_hdl);

  return status;
}

/* Snake test init */
bf_status_t bf_diag_loopback_snake_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl) {
  bf_dev_port_t port = 0;
  diag_port_t *port_p = NULL;
  int priority = 0, port_idx = 0;
  bf_dev_port_t first_port = 0, last_port = 0;
  bf_diag_sess_hdl_t overlap_hdl = 0;
  bf_status_t status = BF_SUCCESS;

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    DIAG_PRINT("Invalid loopback mode: None \n");
    return BF_INVALID_ARG;
  }
  if (diag_validate_loopback_mode(dev_id, diag_loop_mode) != BF_SUCCESS) {
    DIAG_PRINT("Loopback mode %d not supported on this chip \n",
               diag_loop_mode);
    return BF_INVALID_ARG;
  }
  if ((!port_list) || (num_ports <= 0) || (num_ports > BF_DIAG_MAX_PORTS)) {
    return BF_INVALID_ARG;
  }
  /* Validate ports */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      DIAG_PRINT("Port %d does not exist \n", port);
      return BF_INVALID_ARG;
    }
    if (port_idx == 0) {
      first_port = port;
    }
    if (port_idx == (num_ports - 1)) {
      last_port = port;
    }
  }

  /* Check for overlap with existing configs */
  status = diag_session_config_overlap_check(
      dev_id, port_list, num_ports, diag_loop_mode, &overlap_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT(
        "Port Config overlaps with an existing session %u on dev %d with "
        "different loopback mode\n",
        overlap_hdl,
        dev_id);
    return BF_INVALID_ARG;
  }

  status = diag_session_hdl_alloc(dev_id, sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Failed to allocate session on dev %d\n", dev_id);
    return status;
  }

  /* Save the command */
  status = diag_save_loopback_test_params(
      dev_id, *sess_hdl, port_list, num_ports, diag_loop_mode, DIAG_TEST_SNAKE);
  if (status != BF_SUCCESS) {
    diag_session_hdl_free(*sess_hdl);
    return status;
  }

  /* Setup config */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    /* Clear the stats */
    diag_cpu_port_stats_clear(dev_id, port, *sess_hdl, false);
    if (!diag_port_in_multiple_sessions(dev_id, port)) {
      /* Set the loopback mode */
      bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
    }

    /* First port */
    if (port_idx == 0) {
      /* Fwd any pkt from CPU to first port (fwd traffic) */
      bf_diag_forwarding_rule_add(dev_id,
                                  diag_cpu_port_get(dev_id, port),
                                  first_port,
                                  DIAG_SESS_TCP_DSTPORT_START(*sess_hdl),
                                  DIAG_SESS_TCP_DSTPORT_END(*sess_hdl),
                                  priority,
                                  &(port_p->from_src_override_hdl[*sess_hdl]));
      /* Fwd any pkt from first port to last port (rev traffic) */
      bf_diag_forwarding_rule_add(
          dev_id,
          first_port,
          last_port,
          DIAG_SESS_TCP_DSTPORT_REV_START(*sess_hdl),
          DIAG_SESS_TCP_DSTPORT_REV_END(*sess_hdl),
          priority + 500,
          &(port_p->rev_to_dst_override_hdl[*sess_hdl]));
    } else {
      /* Install fwd entry from every port to previous port (rev traffic) */
      bf_diag_forwarding_rule_add(
          dev_id,
          port,
          port_list[port_idx - 1],
          DIAG_SESS_TCP_DSTPORT_REV_START(*sess_hdl),
          DIAG_SESS_TCP_DSTPORT_REV_END(*sess_hdl),
          priority + 500,
          &(port_p->rev_to_dst_override_hdl[*sess_hdl]));
    }

    /* Last port */
    if (port_idx == (num_ports - 1)) {
      /* Fwd any pkt from CPU to last port (rev traffic) */
      bf_diag_forwarding_rule_add(
          dev_id,
          diag_cpu_port_get(dev_id, last_port),
          last_port,
          DIAG_SESS_TCP_DSTPORT_REV_START(*sess_hdl),
          DIAG_SESS_TCP_DSTPORT_REV_END(*sess_hdl),
          priority,
          &(port_p->rev_from_src_override_hdl[*sess_hdl]));
      /* Fwd any pkt from last port to first port (fwd traffic) */
      bf_diag_forwarding_rule_add(dev_id,
                                  last_port,
                                  first_port,
                                  DIAG_SESS_TCP_DSTPORT_START(*sess_hdl),
                                  DIAG_SESS_TCP_DSTPORT_END(*sess_hdl),
                                  priority + 500,
                                  &(port_p->to_dst_override_hdl[*sess_hdl]));
    } else {
      /* Install fwd entry from every port to next port (fwd traffic) */
      bf_diag_forwarding_rule_add(
          dev_id,
          port,
          ((port_idx + 1) < num_ports) ? port_list[port_idx + 1] : port,
          DIAG_SESS_TCP_DSTPORT_START(*sess_hdl),
          DIAG_SESS_TCP_DSTPORT_END(*sess_hdl),
          priority + 500,
          &(port_p->to_dst_override_hdl[*sess_hdl]));
    }
  }
  /* Sleep for sometime as ports flap when loopback mode is set */
  if (diag_loop_mode != BF_DIAG_PORT_LPBK_EXT) {
    bf_sys_usleep(2000000);
  }

  return BF_SUCCESS;
}

/* Start snake test */
bf_status_t bf_diag_loopback_snake_test_start(bf_diag_sess_hdl_t sess_hdl,
                                              uint32_t num_packet,
                                              uint32_t pkt_size,
                                              bool bidir) {
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  bf_status_t status = BF_SUCCESS;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    DIAG_PRINT("Pkt size too large \n");
    return BF_INVALID_ARG;
  }
  if ((pkt_size < DIAG_MIN_PKT_SIZE) && (pkt_size != 0)) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  /* Max pkts is half size of tcp-range */
  if (num_packet > BF_DIAG_TCP_DSTPORT_MID) {
    DIAG_PRINT("Maximum pkts that can be sent is %d\n",
               BF_DIAG_TCP_DSTPORT_MID);
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_snake(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type snake\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    DIAG_PRINT("Setup not done \n");
    return BF_INVALID_ARG;
  }
  /* Make sure test is not in progress */
  memset(&stats, 0, sizeof(stats));
  test_status = bf_diag_loopback_snake_test_status_get(sess_hdl, &stats);
  if (test_status == BF_DIAG_TEST_STATUS_IN_PROGRESS) {
    DIAG_PRINT("Test status in progress \n");
    return BF_ALREADY_EXISTS;
  }
  /* Save the command */
  status = diag_save_runtime_loopback_test_params(
      dev_id, sess_hdl, pkt_size, num_packet, 0, 0, bidir);
  if (status != BF_SUCCESS) {
    return status;
  }

  /* Call the pkt tx fn */
  status = diag_pkt_tx_creator(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Pkt injection failed \n");
    return status;
  }

  return BF_SUCCESS;
}

static void diag_loopback_snake_test_stop_range(bf_dev_id_t dev_id,
                                                diag_session_info_t *sess_info,
                                                bf_dev_port_t port,
                                                uint32_t sess_start,
                                                uint32_t sess_end) {
  int priority = 0;
  uint32_t tcp_start = 0, tcp_end = 0;
  uint32_t tcp_range = DIAG_SNAKE_STOP_TCP_DSTPORT_RANGE;

  if (!sess_info) {
    return;
  }

  tcp_start = sess_start;
  tcp_end = tcp_start;

  /* Drain slowly for eth-cpu to prevent socket buffer overrun */
  if (DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    tcp_range = DIAG_SNAKE_STOP_TCP_DSTPORT_RANGE_ETH;
  }

  while (tcp_end < sess_end) {
    /* If we have reached a max range value, just go to the sess_end */
    if (tcp_end > (tcp_start + DIAG_SNAKE_STOP_TCP_DSTPORT_MAX)) {
      tcp_end = sess_end;
    } else if ((tcp_end + tcp_range) >= sess_end) {
      /* If adding up the range goes over limit, set the sess_end */
      tcp_end = sess_end;
    } else {
      /* Increment by a small range */
      tcp_end += tcp_range;
    }

    DIAG_PRINT("Snake Stop for tcp range (%d - %d) \n", tcp_start, tcp_end);

    /* Delete any old entry */
    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl) {
      bf_diag_forwarding_rule_del(
          dev_id, DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl);
      DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl = 0;
    }

    /* Fwd all pkts to CPU */
    bf_diag_forwarding_rule_add(
        dev_id,
        port,
        diag_cpu_port_get(dev_id, port),
        tcp_start,
        tcp_end,
        priority,
        &(DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl));
    /* Sleep for sometime */
    if (!DIAG_DEV_INFO(dev_id)->kernel_pkt) {
      bf_sys_usleep(DIAG_PKT_RCV_INTERVAL_US);
    } else {
      bf_sys_usleep(3 * DIAG_PKT_RCV_INTERVAL_US);
    }
  }
  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl) {
    /* Wait for sometime for pkts to drain */
    bf_sys_usleep(2 * DIAG_PKT_RCV_INTERVAL_US);
    bf_diag_forwarding_rule_del(
        dev_id, DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl);
    DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl = 0;
  }

  return;
}

void diag_loopback_snake_test_stop_helper(bf_dev_id_t dev_id,
                                          bf_diag_sess_hdl_t sess_hdl) {
  int num_ports = 0;
  diag_session_info_t *sess_info = NULL;
  uint32_t sess_start = 0, sess_end = 0;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;

  /* Always do both ranges as bidir is a runtime parameter */
  /* Stop Fwd traffic */
  sess_start = DIAG_SESS_TCP_DSTPORT_START(sess_hdl);
  sess_end = DIAG_SESS_TCP_DSTPORT_END(sess_hdl);
  diag_loopback_snake_test_stop_range(
      dev_id,
      sess_info,
      DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[num_ports - 1],
      sess_start,
      sess_end);

  /* Stop Rev traffic only if bidir was run at least once */
  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).bidir_sess_en) {
    sess_start = DIAG_SESS_TCP_DSTPORT_REV_START(sess_hdl);
    sess_end = DIAG_SESS_TCP_DSTPORT_REV_END(sess_hdl);
    diag_loopback_snake_test_stop_range(
        dev_id,
        sess_info,
        DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[0],
        sess_start,
        sess_end);
  }

  DIAG_PRINT("Snake Stop DONE \n");
}

/* Stop snake test */
bf_status_t bf_diag_loopback_snake_test_stop(bf_diag_sess_hdl_t sess_hdl) {
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_snake(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type snake\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  memset(&stats, 0, sizeof(stats));
  /* Make sure test 'start' has finished sending all packets  */
  test_status = bf_diag_loopback_snake_test_status_get(sess_hdl, &stats);
  if ((test_status == BF_DIAG_TEST_STATUS_IN_PROGRESS) ||
      (test_status == BF_DIAG_TEST_STATUS_UNKNOWN)) {
    DIAG_PRINT("Snake status in progress or unknown \n");
    return BF_ALREADY_EXISTS;
  }

  diag_loopback_snake_test_stop_helper(dev_id, sess_hdl);

  return BF_SUCCESS;
}

/* Get snake test status
   get counters of first and last port. For inermediate ports mac counters
   should be checked
*/
bf_diag_test_status_e bf_diag_loopback_snake_test_status_get(
    bf_diag_sess_hdl_t sess_hdl, bf_diag_port_stats_t *stats) {
  int num_ports = 0;
  bf_diag_port_stats_t stats_0, stats_1;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!stats) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  memset(&stats_0, 0, sizeof(stats_0));
  memset(&stats_1, 0, sizeof(stats_1));

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_snake(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type snake\n", sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  /* Get the stats of first port */
  diag_cpu_port_stats_get(dev_id,
                          DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[0],
                          sess_hdl,
                          &stats_0);
  /* Get the stats of last port */
  diag_cpu_port_stats_get(
      dev_id,
      DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[num_ports - 1],
      sess_hdl,
      &stats_1);

  /* Is rx same as tx */
  stats->tx_total = stats_0.tx_total + stats_1.tx_total;
  stats->rx_total = stats_0.rx_total + stats_1.rx_total;
  stats->rx_good = stats_0.rx_good + stats_1.rx_good;
  stats->rx_bad = stats_0.rx_bad + stats_1.rx_bad;

  if ((stats->tx_total) != (stats->rx_good)) {
    return BF_DIAG_TEST_STATUS_FAIL;
  } else if ((stats->tx_total == 0) &&
             diag_any_pkt_size_sent_valid(sess_info)) {
    /* Start test was done, but no pkts were transmitted */
    return BF_DIAG_TEST_STATUS_FAIL;
  }

  return BF_DIAG_TEST_STATUS_PASS;
}

/* Cleanup snake test - abort is same as cleanup for snake */
bf_status_t bf_diag_loopback_snake_test_cleanup(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t port = 0;
  int port_idx = 0, num_ports = 0;
  bf_diag_port_lpbk_mode_e diag_loop_mode = BF_DIAG_PORT_LPBK_NONE;
  diag_port_t *port_p = NULL;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_snake(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type snake\n", sess_hdl);
    return BF_INVALID_ARG;
  }

  DIAG_TX_PKT_COMPLETIONS(dev_id) = 0;

  if (!(DIAG_GET_LOOPBACK_PARAMS(sess_info).valid)) {
    return BF_INVALID_ARG;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode !=
        BF_DIAG_PORT_LPBK_EXT) {
      /* If there is another session using this port, don't change the
         loopback mode yet.
      */
      if (!diag_port_in_multiple_sessions(dev_id, port)) {
        /* Set the loopback mode */
        bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
      }
    }
    /* Remove Fwd to CPU entry */
    if (port_p->to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->to_dst_override_hdl[sess_hdl]);
      port_p->to_dst_override_hdl[sess_hdl] = 0;
    }
    if (port_p->rev_to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->rev_to_dst_override_hdl[sess_hdl]);
      port_p->rev_to_dst_override_hdl[sess_hdl] = 0;
    }
    /* Remove Fwd from CPU entry */
    if (port_p->from_src_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->from_src_override_hdl[sess_hdl]);
      port_p->from_src_override_hdl[sess_hdl] = 0;
    }
    if (port_p->rev_from_src_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->rev_from_src_override_hdl[sess_hdl]);
      port_p->rev_from_src_override_hdl[sess_hdl] = 0;
    }
    diag_cpu_port_stats_clear(dev_id, port, sess_hdl, false);
  }
  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl) {
    /* Cleanup snake stop entry */
    bf_diag_forwarding_rule_del(
        dev_id, DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl);
    DIAG_GET_LOOPBACK_PARAMS(sess_info).snake_stop_entry_hdl = 0;
  }

  DIAG_LOOPBACK_PARAMS_CLEAR(sess_info);
  // Cleanup the pkt database that we created
  diag_cleanup_pkt_database(sess_hdl);
  diag_session_info_del(sess_hdl);
  diag_session_hdl_free(sess_hdl);

  return status;
}

/* Set mac aging timeout */
bf_status_t bf_diag_mac_aging_set(bf_dev_id_t dev_id, uint32_t ttl_in_msec) {
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (ttl_in_msec > DIAG_MAX_TTL) {
    DIAG_PRINT("Error: Max ttl supported is %d\n", DIAG_MAX_TTL);
    return BF_INVALID_ARG;
  }
  if (ttl_in_msec < DIAG_MIN_TTL) {
    DIAG_PRINT("Error: Min ttl supported is %d\n", DIAG_MIN_TTL);
    return BF_INVALID_ARG;
  }

  DIAG_DEV_INFO(dev_id)->mac_aging_ttl = ttl_in_msec;

  return BF_SUCCESS;
}

/* Reset mac aging timeout */
bf_status_t bf_diag_mac_aging_reset(bf_dev_id_t dev_id) {
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  DIAG_DEV_INFO(dev_id)->mac_aging_ttl = DIAG_DEF_TTL;

  return BF_SUCCESS;
}

/* Get mac aging timeout */
bf_status_t bf_diag_mac_aging_get(bf_dev_id_t dev_id, uint32_t *ttl_in_msec) {
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  *ttl_in_msec = DIAG_DEV_INFO(dev_id)->mac_aging_ttl;
  return BF_SUCCESS;
}

/* Set learning timeout */
bf_status_t bf_diag_learning_timeout_set(bf_dev_id_t dev_id,
                                         uint32_t timeout_usec) {
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  return diag_pd_learning_timeout_set(dev_id, timeout_usec);
}

/* pair-test init */
bf_status_t bf_diag_loopback_pair_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *i_port_list,
    int i_num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl) {
  bf_dev_port_t port = 0;
  diag_port_t *port_p = NULL;
  int priority = 0, port_idx = 0;
  bf_diag_sess_hdl_t overlap_hdl = 0;
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t port_list[BF_DIAG_MAX_PORTS];
  int num_ports = 0;

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    DIAG_PRINT("Invalid loopback mode: None \n");
    return BF_INVALID_ARG;
  }
  if (diag_validate_loopback_mode(dev_id, diag_loop_mode) != BF_SUCCESS) {
    DIAG_PRINT("Loopback mode %d not supported on this chip \n",
               diag_loop_mode);
    return BF_INVALID_ARG;
  }
  if ((diag_loop_mode == BF_DIAG_PORT_LPBK_EXT) && ((num_ports % 2) != 0)) {
    DIAG_PRINT("Num of ports should be even with external loop pair test \n");
    return BF_INVALID_ARG;
  }
  if ((!i_port_list) || (i_num_ports > BF_DIAG_MAX_PORTS)) {
    return BF_INVALID_ARG;
  }

  if ((i_num_ports == 1) && (i_port_list[0] == BF_DIAG_PORT_GROUP_ALL)) {
    /* Handle special case for 'all' ports */
    diag_get_all_ports_list(dev_id, &port_list[0], &num_ports, false);
    diag_add_cpu_port_to_list(dev_id, &port_list[0], &num_ports, false);
  } else if ((i_num_ports == 1) &&
             (i_port_list[0] == BF_DIAG_PORT_GROUP_ALLI)) {
    /* Handle special case for 'all internal' ports */
    diag_get_all_ports_list(dev_id, &port_list[0], &num_ports, true);
    diag_add_cpu_port_to_list(dev_id, &port_list[0], &num_ports, false);
  } else if ((i_num_ports == 1) &&
             (i_port_list[0] == BF_DIAG_PORT_GROUP_ALL_MESH)) {
    /* Handle special case for 'all mesh' ports */
    diag_get_all_mesh_ports_list(dev_id, &port_list[0], &num_ports, false);
    diag_add_cpu_port_to_list(dev_id, &port_list[0], &num_ports, true);
  } else {
    if (i_num_ports <= 1) {
      return BF_INVALID_ARG;
    }
    /* Copy over the port list as is */
    memcpy(&port_list[0], i_port_list, sizeof(bf_dev_port_t) * i_num_ports);
    num_ports = i_num_ports;
  }

  /* Validate ports */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      DIAG_PRINT("Port %d does not exist \n", port);
      return BF_INVALID_ARG;
    }
  }

  /* Check for overlap with existing configs */
  status = diag_session_config_overlap_check(
      dev_id, port_list, num_ports, diag_loop_mode, &overlap_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT(
        "Port Config overlaps with an existing session %u on dev %d with "
        "different loopback mode\n",
        overlap_hdl,
        dev_id);
    return BF_INVALID_ARG;
  }

  status = diag_session_hdl_alloc(dev_id, sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Failed to allocate session on dev %d\n", dev_id);
    return status;
  }

  /* Save the command */
  status = diag_save_loopback_test_params(dev_id,
                                          *sess_hdl,
                                          port_list,
                                          num_ports,
                                          diag_loop_mode,
                                          DIAG_TEST_PAIRED_LOOPBACK);
  if (status != BF_SUCCESS) {
    diag_session_hdl_free(*sess_hdl);
    return status;
  }

  /* Setup config */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    /* Clear the stats */
    diag_cpu_port_stats_clear(dev_id, port, *sess_hdl, false);
    /* Set the loopback mode */
    if (!diag_port_in_multiple_sessions(dev_id, port)) {
      bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
    }

    /* In pair test - cpu sends pkt to first port,
       first port sends pkt to second port,
       second port sends pkt back to first port
    */
    if ((port_idx % 2) == 0) {
      /* Fwd any pkt from CPU to alternate ports */
      /* If even port index and not last port */
      if ((port_idx + 1) < (num_ports)) {
        bf_diag_forwarding_rule_add(
            dev_id,
            diag_cpu_port_get(dev_id, port),
            port,
            DIAG_PORT_TCP_DSTPORT_START(port, *sess_hdl),
            DIAG_PORT_TCP_DSTPORT_END(port, *sess_hdl),
            priority,
            &(port_p->from_src_override_hdl[*sess_hdl]));
        /* forward from current port to next */
        bf_diag_forwarding_rule_add(dev_id,
                                    port,
                                    port_list[port_idx + 1],
                                    BF_DIAG_TCP_DSTPORT_MIN,
                                    BF_DIAG_TCP_DSTPORT_MAX,
                                    priority + 500,
                                    &(port_p->to_dst_override_hdl[*sess_hdl]));
      } else if (((port_idx + 1) == num_ports) && (port_idx >= 2)) {
        /* If even port index and last port */
        bf_diag_forwarding_rule_add(dev_id,
                                    port,
                                    port_list[port_idx - 2],
                                    BF_DIAG_TCP_DSTPORT_MIN,
                                    BF_DIAG_TCP_DSTPORT_MAX,
                                    priority + 500,
                                    &(port_p->to_dst_override_hdl[*sess_hdl]));
      }
    } else {
      /* Install fwd entry from every alternate port to previous port */
      bf_dev_port_t dst_port = 0;

      /* If only one more port is left, then make it a pair of three ports */
      if ((port_idx + 2) == (num_ports)) {
        /* If odd port index and third last port (total odd ports) */
        dst_port = port_list[port_idx + 1];
      } else {
        /* If odd port index */
        dst_port = port_list[port_idx - 1];
      }
      bf_diag_forwarding_rule_add(dev_id,
                                  diag_cpu_port_get(dev_id, port),
                                  port,
                                  DIAG_PORT_TCP_DSTPORT_START(port, *sess_hdl),
                                  DIAG_PORT_TCP_DSTPORT_END(port, *sess_hdl),
                                  priority,
                                  &(port_p->from_src_override_hdl[*sess_hdl]));
      bf_diag_forwarding_rule_add(dev_id,
                                  port,
                                  dst_port,
                                  BF_DIAG_TCP_DSTPORT_MIN,
                                  BF_DIAG_TCP_DSTPORT_MAX,
                                  priority + 500,
                                  &(port_p->to_dst_override_hdl[*sess_hdl]));
    }
  }
  /* Sleep for sometime as ports flap when loopback mode is set */
  if (diag_loop_mode != BF_DIAG_PORT_LPBK_EXT) {
    bf_sys_usleep(2000000);
  }

  return BF_SUCCESS;
}

/* Start pair test */
bf_status_t bf_diag_loopback_pair_test_start(bf_diag_sess_hdl_t sess_hdl,
                                             uint32_t num_packet,
                                             uint32_t pkt_size,
                                             bool bidir) {
  bf_diag_test_status_e test_status;
  bf_diag_loopback_pair_test_stats_t stats;
  bf_status_t status = BF_SUCCESS;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    DIAG_PRINT("Pkt size too large \n");
    return BF_INVALID_ARG;
  }
  if ((pkt_size < DIAG_MIN_PKT_SIZE) && (pkt_size != 0)) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback_pair(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback pair\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    DIAG_PRINT("Setup not done \n");
    return BF_INVALID_ARG;
  }
  if (bidir && ((DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports % 2) != 0)) {
    DIAG_PRINT(
        "Bidir can only be run on even port count, current port count %d \n",
        DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports);
    return BF_INVALID_ARG;
  }
  /* Make sure test is not in progress */
  test_status = bf_diag_loopback_pair_test_status_get(sess_hdl, &stats);
  if (test_status == BF_DIAG_TEST_STATUS_IN_PROGRESS) {
    DIAG_PRINT("Test status in progress \n");
    return BF_ALREADY_EXISTS;
  }
  /* Save the command */
  status = diag_save_runtime_loopback_test_params(dev_id,
                                                  sess_hdl,
                                                  pkt_size,
                                                  num_packet,
                                                  BF_DIAG_TCP_DSTPORT_MIN + 1,
                                                  0,
                                                  bidir);
  if (status != BF_SUCCESS) {
    return status;
  }
#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  status = diag_pd_reset_shift_counter(dev_id);
#endif
  /* Call the pkt tx fn */
  status = diag_pkt_tx_creator(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Pkt injection failed \n");
    return status;
  }

  return BF_SUCCESS;
}

void diag_loopback_pair_test_stop_helper(bf_dev_id_t dev_id,
                                         bf_diag_sess_hdl_t sess_hdl) {
  int priority = 0, num_ports = 0;
  int count = 0, count_max = 2;
  int port_idx = 0;
  bf_dev_port_t port = 0;
  diag_port_t *port_p = NULL;
  bool bidir = false;
  diag_session_info_t *sess_info = NULL;
  uint32_t tcp_start = 0, tcp_end = 0;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return;
  }
  bidir = DIAG_GET_LOOPBACK_PARAMS(sess_info).bidir;
  (void)bidir;

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;

  /* Drain slowly for eth-cpu to prevent socket buffer overrun */
  if (DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    count_max = 4;
  }
  if (DIAG_DRAIN_FULL_TCP_PORT_RANGE(dev_id)) {
    count_max = 1;
  }

  /* Install the stop entry on all odd indexes */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);

    if (!port_p) {
      continue;
    }
    DIAG_PRINT("Pair Stop test for port %d \n", port);

    tcp_start = DIAG_PORT_TCP_DSTPORT_START(
        DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx], sess_hdl);
    /* Divide the tcp-range into multiple ranges, to slow down pkts going to cpu
     */
    for (count = 0; count < count_max; count++) {
      /* Delete the old entry */
      if (port_p->pair_stop_override_hdl[sess_hdl]) {
        bf_diag_forwarding_rule_del(dev_id,
                                    port_p->pair_stop_override_hdl[sess_hdl]);
        port_p->pair_stop_override_hdl[sess_hdl] = 0;
      }
      /* Delete the old entry */
      if (port_p->pair_stop_override_hdl2[sess_hdl]) {
        bf_diag_forwarding_rule_del(dev_id,
                                    port_p->pair_stop_override_hdl2[sess_hdl]);
        port_p->pair_stop_override_hdl2[sess_hdl] = 0;
      }

      /* Drain full tcp port range if requested by user */
      if (DIAG_DRAIN_FULL_TCP_PORT_RANGE(dev_id)) {
        tcp_start = BF_DIAG_TCP_DSTPORT_MIN;
        tcp_end = BF_DIAG_TCP_DSTPORT_MAX;
      } else {
        tcp_end = tcp_start +
                  (DIAG_PORT_TCP_DSTPORT_END(
                       DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx],
                       sess_hdl) -
                   tcp_start) /
                      (count_max - count);
      }
      /* Fwd any pkt for this range to CPU from this port */
      bf_diag_forwarding_rule_add(
          dev_id,
          DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx],
          diag_cpu_port_get(
              dev_id, DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx]),
          tcp_start,
          tcp_end,
          priority,
          &(port_p->pair_stop_override_hdl[sess_hdl]));

      /* Fwd any pkt for this range to CPU from the paired port */
      bf_diag_forwarding_rule_add(
          dev_id,
          ((port_idx % 2) == 0)
              ? DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx + 1]
              : DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx - 1],
          diag_cpu_port_get(
              dev_id,
              ((port_idx % 2) == 0)
                  ? DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx + 1]
                  : DIAG_GET_LOOPBACK_PARAMS(sess_info)
                        .port_list[port_idx - 1]),
          tcp_start,
          tcp_end,
          priority,
          &(port_p->pair_stop_override_hdl2[sess_hdl]));

      if (!DIAG_DEV_INFO(dev_id)->kernel_pkt) {
        bf_sys_usleep(DIAG_PAIR_PKT_RCV_INTERVAL_US);
      } else {
        bf_sys_usleep(DIAG_PAIR_KERNEL_PKT_RCV_INTERVAL_US);
      }
    }
  }
  /* Wait for sometime for pkts to drain */
  bf_sys_usleep(3 * DIAG_PKT_RCV_INTERVAL_US);

  /* Delete all the entries */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (port_p->pair_stop_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->pair_stop_override_hdl[sess_hdl]);
      port_p->pair_stop_override_hdl[sess_hdl] = 0;
    }
    if (port_p->pair_stop_override_hdl2[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->pair_stop_override_hdl2[sess_hdl]);
      port_p->pair_stop_override_hdl2[sess_hdl] = 0;
    }
  }

  DIAG_PRINT("Pair Stop DONE \n");
}

/* Stop pair test */
bf_status_t bf_diag_loopback_pair_test_stop(bf_diag_sess_hdl_t sess_hdl) {
  bf_diag_test_status_e test_status;
  bf_diag_loopback_pair_test_stats_t stats;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;
  uint64_t curr_pkt_id_corrupt_cnt = 0, new_pkt_id_corrupt_cnt = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback_pair(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback pair\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    DIAG_PRINT("Loop params invalid \n");
    return BF_INVALID_ARG;
  }
  /* Make sure test 'start' has finished sending all packets  */
  test_status = bf_diag_loopback_pair_test_status_get(sess_hdl, &stats);
  if ((test_status == BF_DIAG_TEST_STATUS_IN_PROGRESS) ||
      (test_status == BF_DIAG_TEST_STATUS_UNKNOWN)) {
    DIAG_PRINT("Pair status in progress or unknown \n");
    return BF_ALREADY_EXISTS;
  }

  curr_pkt_id_corrupt_cnt = DIAG_PKT_ID_CORRUPT_CNT(dev_id);
  diag_loopback_pair_test_stop_helper(dev_id, sess_hdl);
  new_pkt_id_corrupt_cnt = DIAG_PKT_ID_CORRUPT_CNT(dev_id);
  sess_info->pkt_id_corrupt_cnt =
      new_pkt_id_corrupt_cnt - curr_pkt_id_corrupt_cnt;

#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  diag_pd_read_counter(dev_id);
#endif

  return BF_SUCCESS;
}

/* Get pair test status
   get counters of first and last port. For inermediate ports mac counters
   should be checked
*/
bf_diag_test_status_e bf_diag_loopback_pair_test_status_get(
    bf_diag_sess_hdl_t sess_hdl, bf_diag_loopback_pair_test_stats_t *stats) {
  int num_ports = 0;
  int port_idx = 0;
  bf_dev_port_t rx_port = 0;
  bf_diag_test_status_e test_status = BF_DIAG_TEST_STATUS_PASS;
  int pair_cnt = 0;
  bf_diag_port_stats_t stats_0, stats_1;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!stats) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  memset(stats, 0, sizeof(bf_diag_loopback_pair_test_stats_t));

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_loopback_pair(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback pair\n",
               sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  stats->total_bytes_with_bit_flip_detected =
      sess_info->total_bytes_with_bit_flip_detected;
  stats->total_bits_with_bit_flip_detected =
      sess_info->total_bits_with_bit_flip_detected;

  stats->total_1_to_0_flips = sess_info->total_1_to_0_flips;
  stats->total_0_to_1_flips = sess_info->total_0_to_1_flips;

  stats->total_weak_suspect_for_setup = sess_info->total_weak_suspect_for_setup;
  stats->total_strong_suspect_for_setup =
      sess_info->total_strong_suspect_for_setup;
  stats->total_weak_suspect_for_hold = sess_info->total_weak_suspect_for_hold;
  stats->total_strong_suspect_for_hold =
      sess_info->total_strong_suspect_for_hold;
  stats->total_unknown_failures = sess_info->total_unknown_failures;
  stats->total_payload_setup_failures = sess_info->total_payload_setup_failures;
  stats->total_mixed_failures = sess_info->total_mixed_failures;

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  /* Go over all ports, increment by 2 */
  for (port_idx = 0;
       ((port_idx + 1) < num_ports) && (num_ports <= BF_DIAG_MAX_PORTS);
       port_idx++, port_idx++) {
    stats->pairs[pair_cnt].port1 =
        DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    memset(&stats_0, 0, sizeof(stats_0));
    /* Get the stats of first port in port-pair */
    diag_cpu_port_stats_get(
        dev_id,
        DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx],
        sess_hdl,
        &stats_0);

    /* Last pair contains 3 ports */
    if ((port_idx + 3) == num_ports) {
      rx_port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx + 2];
    } else {
      rx_port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx + 1];
    }
    stats->pairs[pair_cnt].port2 = rx_port;
    /* Get the stats of last port */
    memset(&stats_1, 0, sizeof(stats_1));
    diag_cpu_port_stats_get(dev_id, rx_port, sess_hdl, &stats_1);

    /* Copy over per port stats */
    stats->pairs[pair_cnt].port_stats[0].tx = stats_0.tx_total;
    stats->pairs[pair_cnt].port_stats[0].rx = stats_0.rx_origin;

    stats->pairs[pair_cnt].port_stats[1].tx = stats_1.tx_total;
    stats->pairs[pair_cnt].port_stats[1].rx = stats_1.rx_origin;

    /* If both ports are different, add the stats to get total */
    if (stats->pairs[pair_cnt].port1 == stats->pairs[pair_cnt].port2) {
      stats->pairs[pair_cnt].tx_total = stats_0.tx_total;
      stats->pairs[pair_cnt].rx_total = stats_0.rx_total;
      stats->pairs[pair_cnt].rx_good = stats_0.rx_good;
      stats->pairs[pair_cnt].rx_bad = stats_0.rx_bad;
    } else {
      stats->pairs[pair_cnt].tx_total = stats_0.tx_total + stats_1.tx_total;
      stats->pairs[pair_cnt].rx_total = stats_0.rx_total + stats_1.rx_total;
      stats->pairs[pair_cnt].rx_good = stats_0.rx_good + stats_1.rx_good;
      stats->pairs[pair_cnt].rx_bad = stats_0.rx_bad + stats_1.rx_bad;
    }
    /* Is rx same as tx */
    if ((stats->pairs[pair_cnt].tx_total) != (stats->pairs[pair_cnt].rx_good)) {
      stats->pairs[pair_cnt].test_status = BF_DIAG_TEST_STATUS_FAIL;
      test_status = BF_DIAG_TEST_STATUS_FAIL;
    } else if ((stats->pairs[pair_cnt].tx_total == 0) &&
               diag_any_pkt_size_sent_valid(sess_info)) {
      /* Start test was done, but no pkts were transmitted */
      stats->pairs[pair_cnt].test_status = BF_DIAG_TEST_STATUS_FAIL;
      test_status = BF_DIAG_TEST_STATUS_FAIL;
    } else {
      stats->pairs[pair_cnt].test_status = BF_DIAG_TEST_STATUS_PASS;
    }
    pair_cnt++;
    if (pair_cnt >= BF_DIAG_MAX_LOOPBACK_TEST_PAIRS) {
      break;
    }
  }
  stats->num_pairs = pair_cnt;

  return test_status;
}

/* Cleanup pair test - abort is same as cleanup for pair */
bf_status_t bf_diag_loopback_pair_test_cleanup(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t port = 0;
  int port_idx = 0, num_ports = 0;
  bf_diag_port_lpbk_mode_e diag_loop_mode = BF_DIAG_PORT_LPBK_NONE;
  diag_port_t *port_p = NULL;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_loopback_pair(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback pair\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }

  DIAG_TX_PKT_COMPLETIONS(dev_id) = 0;

  if (!(DIAG_GET_LOOPBACK_PARAMS(sess_info).valid)) {
    return BF_INVALID_ARG;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode !=
        BF_DIAG_PORT_LPBK_EXT) {
      /* If there is another session using this port, don't change the
         loopback mode yet.
      */
      if (!diag_port_in_multiple_sessions(dev_id, port)) {
        /* Set the loopback mode */
        bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
      }
    }
    /* Remove Fwd to CPU entry */
    if (port_p->to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->to_dst_override_hdl[sess_hdl]);
      port_p->to_dst_override_hdl[sess_hdl] = 0;
    }
    /* Remove Fwd from CPU entry */
    if (port_p->from_src_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->from_src_override_hdl[sess_hdl]);
      port_p->from_src_override_hdl[sess_hdl] = 0;
    }
    /* Remove pair stop entry */
    if (port_p->pair_stop_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->pair_stop_override_hdl[sess_hdl]);
      port_p->pair_stop_override_hdl[sess_hdl] = 0;
    }
    if (port_p->pair_stop_override_hdl2[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->pair_stop_override_hdl2[sess_hdl]);
      port_p->pair_stop_override_hdl2[sess_hdl] = 0;
    }
    diag_cpu_port_stats_clear(dev_id, port, sess_hdl, false);
  }

  DIAG_LOOPBACK_PARAMS_CLEAR(sess_info);
  // Cleanup the pkt database that we created
  diag_cleanup_pkt_database(sess_hdl);
  diag_session_info_del(sess_hdl);
  diag_session_hdl_free(sess_hdl);

  return status;
}

/* Is session valid */
bool bf_diag_session_valid(bf_diag_sess_hdl_t sess_hdl) {
  if (!diag_session_info_get(sess_hdl)) {
    return false;
  }

  return true;
}

/* Delete all sessions */
bf_status_t bf_diag_session_del_all(bf_dev_id_t dev_id) {
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  return diag_session_info_del_all(dev_id, false);
}

/* Inject packets */
bf_status_t bf_diag_packet_inject_from_cpu(bf_dev_id_t dev_id,
                                           bf_dev_port_t *port_list,
                                           int num_ports,
                                           uint32_t num_packet,
                                           uint32_t pkt_size) {
  diag_port_t *port_p = NULL;
  int port_idx = 0;
  bf_dev_port_t port = 0;

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    return BF_INVALID_ARG;
  }
  if (pkt_size < DIAG_MIN_PKT_SIZE) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }

  /* Validate ports */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      DIAG_PRINT("Port %d does not exist \n", port);
      return BF_INVALID_ARG;
    }
  }

  return diag_pkt_inject(dev_id, port_list, num_ports, num_packet, pkt_size);
}

/* Get CPU port */
bf_status_t bf_diag_cpu_port_get(bf_dev_id_t dev_id, bf_dev_port_t *cpu_port) {
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  *cpu_port = diag_cpu_port_get(dev_id, 0);
  return BF_SUCCESS;
}

/* Get cpu pkt stats */
bf_status_t bf_diag_cpu_stats_get(bf_dev_id_t dev_id,
                                  bf_dev_port_t port,
                                  bf_diag_port_stats_t *stats) {
  bf_status_t status = BF_SUCCESS;
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (!stats) {
    return BF_INVALID_ARG;
  }
  status = diag_cpu_port_stats_all_sessions_get(dev_id, port, stats);
  return status;
}

/* Clear cpu pkt stats */
bf_status_t bf_diag_cpu_stats_clear(bf_dev_id_t dev_id,
                                    bf_dev_port_t port,
                                    bool all_ports) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (all_ports) {
    port = 0;
  }
  if (port >= BF_DIAG_MAX_PORTS) {
    return BF_INVALID_ARG;
  }
  for (sess_hdl = 0; sess_hdl < DIAG_SESSIONS_MAX_LIMIT; sess_hdl++) {
    status |= diag_cpu_port_stats_clear(dev_id, port, sess_hdl, all_ports);
  }
  return status;
}

/* multicast-lopback init */
bf_status_t bf_diag_multicast_loopback_test_setup(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *sess_hdl) {
  diag_port_t *port_p = NULL;
  bf_dev_port_t port = 0;
  int priority = 0, port_idx = 0, large_tree_cnt = 0;
  bf_diag_sess_hdl_t overlap_hdl = 0;
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t mc_large_tree_list[BF_DIAG_MAX_PORTS];
  bf_dev_port_t mc_first_non_local_port[BF_DIAG_MAX_PORTS];

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (diag_loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    DIAG_PRINT("Invalid loopback mode: None \n");
    return BF_INVALID_ARG;
  }
  if (diag_validate_loopback_mode(dev_id, diag_loop_mode) != BF_SUCCESS) {
    DIAG_PRINT("Loopback mode %d not supported on this chip \n",
               diag_loop_mode);
    return BF_INVALID_ARG;
  }

  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    bf_dev_port_t local_port = 0, next_port = 0, next_local_port = 0;
    port = port_list[port_idx];
    /* Validate port */
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      DIAG_PRINT("Port %d does not exist \n", port);
      return BF_INVALID_ARG;
    }
    int i;
    for (i = 0; i < DIAG_SESSIONS_MAX_LIMIT; i++) {
      if (port_p->mc[i].mc_grp_id_a != 0) {
        DIAG_PRINT(
            "Port %d already part of a multicast loopback test on session %d\n",
            port,
            i);
        return BF_INVALID_ARG;
      }
    }
    /* Ensure local-port from other pipes are not on the list */
    local_port = DEV_PORT_TO_LOCAL_PORT(port);
    for (uint32_t idx = port_idx + 1; idx < num_ports; idx++) {
      next_port = port_list[idx];
      next_local_port = DEV_PORT_TO_LOCAL_PORT(next_port);
      if (local_port == next_local_port) {
        DIAG_PRINT(
            "Port %d and %d cannot be both specified for this test, all ports "
            "should have different local port ids\n",
            port,
            next_port);
        return BF_INVALID_ARG;
      }
    }
  }

  /* Check for overlap with existing configs */
  status = diag_session_config_overlap_check(
      dev_id, port_list, num_ports, diag_loop_mode, &overlap_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT(
        "Port Config overlaps with an existing session %u on dev %d with "
        "different loopback mode\n",
        overlap_hdl,
        dev_id);
    return BF_INVALID_ARG;
  }

  status = diag_session_hdl_alloc(dev_id, sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Failed to allocate session on dev %d\n", dev_id);
    return status;
  }

  /* Save the command */
  status = diag_save_loopback_test_params(dev_id,
                                          *sess_hdl,
                                          port_list,
                                          num_ports,
                                          diag_loop_mode,
                                          DIAG_TEST_MULTICAST_LOOPBACK);
  if (status != BF_SUCCESS) {
    diag_session_hdl_free(*sess_hdl);
    return status;
  }

  /* Setup config */
  memset(&mc_large_tree_list[0], 0, sizeof(mc_large_tree_list));
  memset(&mc_first_non_local_port[0], 0, sizeof(mc_first_non_local_port));
  int mc_large_tree_idx = 0;
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    bf_dev_port_t new_port = 0, local_port = 0, local_pipe = 0, pipe = 0;
    bf_mc_port_map_t mc_port_map, mc_non_local_port_map;
    bf_mc_lag_map_t mc_lag_map, mc_ecmp_lag_map;
    uint32_t lag_id = 0;

    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }
    /* Clear the stats */
    diag_cpu_port_stats_clear(dev_id, port, *sess_hdl, false);

    local_port = DEV_PORT_TO_LOCAL_PORT(port);
    local_pipe = DEV_PORT_TO_PIPE(port);
    BF_MC_PORT_MAP_INIT(mc_port_map);
    BF_MC_PORT_MAP_INIT(mc_non_local_port_map);
    BF_MC_LAG_MAP_INIT(mc_lag_map);
    BF_MC_LAG_MAP_INIT(mc_ecmp_lag_map);

    /* Using port-id value as the reserved mc-grp id */
    port_p->mc[*sess_hdl].mc_grp_id_a = port + 1;
    /* Use local port (0-72) as lag-id */
    lag_id = local_port + 1; /* Don't use 0 lag-id */
    port_p->mc[*sess_hdl].lag_id = lag_id;
    BF_MC_LAG_MAP_SET(mc_lag_map, lag_id);
    /* Create marge mcast tree only for first 2 ports */
    if (large_tree_cnt < DIAG_MC_TEST_LARGE_TREE_CNT) {
      port_p->mc[*sess_hdl].mc_grp_id_b = port + 1 + BF_DIAG_MAX_PORTS;
      large_tree_cnt++;
    } else {
      port_p->mc[*sess_hdl].mc_grp_id_b = 0;
    }

    for (pipe = 0; pipe < DIAG_DEV_INFO(dev_id)->num_active_pipes; pipe++) {
      new_port = MAKE_DEV_PORT(pipe, local_port);

      /* Set the loopback mode */
      if (!diag_port_in_multiple_sessions(dev_id, new_port)) {
        bf_diag_port_loopback_mode_set(dev_id, new_port, diag_loop_mode);
      }
      if (pipe == local_pipe) {
        BF_MC_PORT_MAP_SET(mc_port_map, new_port);
      } else {
        /* Add ports from non-local pipes to list */
        mc_large_tree_list[mc_large_tree_idx] = new_port;
        mc_large_tree_idx++;
        /* Add ports from non-local pipes to port-map */
        BF_MC_PORT_MAP_SET(mc_non_local_port_map, new_port);

        /* Store the first non-local-pipe port */
        if (mc_first_non_local_port[port_idx] == 0) {
          mc_first_non_local_port[port_idx] = new_port;
        }
      }
    }

    /* Create multicast group */
    status = diag_pd_mc_mgrp_create(dev_id,
                                    port_p->mc[*sess_hdl].mc_grp_id_a,
                                    &(port_p->mc[*sess_hdl].mgrp_hdl_a));
    if (status != BF_SUCCESS) {
      return status;
    }

    if (port_p->mc[*sess_hdl].mc_grp_id_b != 0) {
      status = diag_pd_mc_mgrp_create(dev_id,
                                      port_p->mc[*sess_hdl].mc_grp_id_b,
                                      &(port_p->mc[*sess_hdl].mgrp_hdl_b));
      if (status != BF_SUCCESS) {
        return status;
      }
    }

    /* Create ecmp */
    status = diag_pd_mc_ecmp_create(dev_id, &(port_p->mc[*sess_hdl].ecmp_hdl));
    if (status != BF_SUCCESS) {
      return status;
    }

    /* Add ecmp to Multicast group_a */
    status = diag_pd_mc_associate_ecmp(dev_id,
                                       port_p->mc[*sess_hdl].mgrp_hdl_a,
                                       port_p->mc[*sess_hdl].ecmp_hdl);
    if (status != BF_SUCCESS) {
      return status;
    }

    /* Set the lag members */
    status =
        diag_pd_mc_set_lag_membership(dev_id, lag_id, mc_non_local_port_map);
    if (status != BF_SUCCESS) {
      return status;
    }

    /* Add port from local pipe in the Multicast group_a */
    status = diag_pd_mc_mgrp_ports_add(dev_id,
                                       mc_port_map,
                                       mc_lag_map,
                                       0,
                                       port_p->mc[*sess_hdl].mgrp_hdl_a,
                                       &(port_p->mc[*sess_hdl].node_hdl_a));
    if (status != BF_SUCCESS) {
      return status;
    }

    /* Add ports from non-local pipes to ecmp */
    status = diag_pd_mc_ecmp_ports_add(dev_id,
                                       mc_non_local_port_map,
                                       mc_ecmp_lag_map,
                                       0,
                                       port_p->mc[*sess_hdl].ecmp_hdl,
                                       &(port_p->mc[*sess_hdl].ecmp_node_hdl));
    if (status != BF_SUCCESS) {
      return status;
    }

    /* CPU to port fwding rule */
    bf_diag_forwarding_rule_add(dev_id,
                                diag_cpu_port_get(dev_id, port),
                                port,
                                DIAG_PORT_TCP_DSTPORT_START(port, *sess_hdl),
                                DIAG_PORT_TCP_DSTPORT_END(port, *sess_hdl),
                                priority,
                                &(port_p->from_src_override_hdl[*sess_hdl]));

    /* From port to multicast-group */
    /* Pkts ingressing non-local pipes should get dropped in the MAU as there
       is no forwarding rule
    */
    bf_diag_mc_forwarding_rule_add(
        dev_id,
        port,
        port_p->mc[*sess_hdl].mc_grp_id_a,
        port_p->mc[*sess_hdl].mc_grp_id_b,
        BF_DIAG_TCP_DSTPORT_MIN,
        BF_DIAG_TCP_DSTPORT_MAX,
        priority + 500,
        &(port_p->rev_to_dst_override_hdl[*sess_hdl]));
  }

  /* Now add ports to the large tree */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    bf_mc_port_map_t mc_port_map;
    bf_mc_lag_map_t mc_lag_map;

    port = port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    if (port_p->mc[*sess_hdl].mc_grp_id_b == 0) {
      continue;
    }

    int arr_idx = 0;
    for (arr_idx = 0; arr_idx < mc_large_tree_idx; arr_idx++) {
      BF_MC_LAG_MAP_INIT(mc_lag_map);
      BF_MC_PORT_MAP_INIT(mc_port_map);
      BF_MC_PORT_MAP_SET(mc_port_map, mc_large_tree_list[arr_idx]);

      status = diag_pd_mc_mgrp_ports_add(
          dev_id,
          mc_port_map,
          mc_lag_map,
          0,
          port_p->mc[*sess_hdl].mgrp_hdl_b,
          &(port_p->mc[*sess_hdl].node_hdl_b[arr_idx]));
      if (status != BF_SUCCESS) {
        return status;
      }
    }

    /* We will generate 100's of copies on the first non-local-pipe port */
    BF_MC_LAG_MAP_INIT(mc_lag_map);
    BF_MC_PORT_MAP_INIT(mc_port_map);
    BF_MC_PORT_MAP_SET(mc_port_map, mc_first_non_local_port[port_idx]);
    int start_rid = 10; /* Start from 10 to easiy see in model logs */
    int num_rid = DIAG_MC_TEST_MAX_COPIES_PER_PORT;
    /* Generate less copies on the model, as model can't handle it */
    if (DIAG_DEV_INFO(dev_id)->is_sw_model) {
      num_rid = 2;
    }
    for (int rid = start_rid; rid < (start_rid + num_rid); rid++) {
      bf_mc_node_hdl_t node_hdl = 0;
      status = diag_pd_mc_mgrp_ports_add(
          dev_id,
          mc_port_map,
          mc_lag_map,
          rid,
          port_p->mc[*sess_hdl].mgrp_hdl_b,
          &(port_p->mc[*sess_hdl].copies_node_hdl_b[arr_idx]));
      if (status != BF_SUCCESS) {
        return status;
      }
    }
  }

  /* Sleep for sometime as ports flap when loopback mode is set */
  if (diag_loop_mode != BF_DIAG_PORT_LPBK_EXT) {
    bf_sys_usleep(2000000);
  }

  return BF_SUCCESS;
}

/* Start multicast loopback test */
bf_status_t bf_diag_multicast_loopback_test_start(bf_diag_sess_hdl_t sess_hdl,
                                                  uint32_t num_packet,
                                                  uint32_t pkt_size) {
  bf_status_t status = BF_SUCCESS;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    DIAG_PRINT("Pkt size too large \n");
    return BF_INVALID_ARG;
  }
  if ((pkt_size != 0) && (pkt_size < DIAG_MIN_PKT_SIZE)) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_multicast_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type multicast loopback\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    DIAG_PRINT("Setup not done \n");
    return BF_INVALID_ARG;
  }
  /* Save the command */
  status = diag_save_runtime_loopback_test_params(dev_id,
                                                  sess_hdl,
                                                  pkt_size,
                                                  num_packet,
                                                  BF_DIAG_TCP_DSTPORT_MIN + 1,
                                                  0,
                                                  false);
  if (status != BF_SUCCESS) {
    return status;
  }
#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  status = diag_pd_reset_shift_counter(dev_id);
#endif
  /* Call the pkt tx fn */
  status = diag_pkt_tx_creator(sess_hdl);
  if (status != BF_SUCCESS) {
    DIAG_PRINT("Pkt injection failed \n");
    return status;
  }

  return BF_SUCCESS;
}

void diag_multicast_loopback_test_stop_helper(bf_dev_id_t dev_id,
                                              bf_diag_sess_hdl_t sess_hdl) {
  int priority = 0, num_ports = 0;
  int count = 0, count_max = 2;
  int port_idx = 0;
  bf_dev_port_t port = 0;
  diag_port_t *port_p = NULL;
  diag_session_info_t *sess_info = NULL;
  uint32_t tcp_start = 0, tcp_end = 0;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;

  /* Install the stop entry */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    DIAG_PRINT("Multicast loopback Stop for port %d \n", port);

    tcp_start = DIAG_PORT_TCP_DSTPORT_START(
        DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx], sess_hdl);
    /* Divide the tcp-range into multiple ranges, to slow down pkts going to cpu
     */
    for (count = 0; count < count_max; count++) {
      /* Delete the old entry */
      if (port_p->mc[sess_hdl].mc_loopback_stop_override_hdl) {
        bf_diag_forwarding_rule_del(
            dev_id, port_p->mc[sess_hdl].mc_loopback_stop_override_hdl);
        port_p->mc[sess_hdl].mc_loopback_stop_override_hdl = 0;
      }

      tcp_end = tcp_start +
                (DIAG_PORT_TCP_DSTPORT_END(
                     DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx],
                     sess_hdl) -
                 tcp_start) /
                    (count_max - count);
      /* Fwd any pkt for this range to CPU from this port */
      bf_diag_forwarding_rule_add(
          dev_id,
          DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx],
          diag_cpu_port_get(
              dev_id, DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx]),
          tcp_start,
          tcp_end,
          priority,
          &(port_p->mc[sess_hdl].mc_loopback_stop_override_hdl));

      if (!DIAG_DEV_INFO(dev_id)->kernel_pkt) {
        bf_sys_usleep(DIAG_MULTICAST_LOOPBACK_PKT_RCV_INTERVAL_US);
      } else {
        bf_sys_usleep(DIAG_PAIR_KERNEL_PKT_RCV_INTERVAL_US);
      }
    }
  }
  /* Wait for sometime for pkts to drain */
  bf_sys_usleep(5 * DIAG_PKT_RCV_INTERVAL_US);

  /* Delete all the entries */
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (port_p->mc[sess_hdl].mc_loopback_stop_override_hdl) {
      bf_diag_forwarding_rule_del(
          dev_id, port_p->mc[sess_hdl].mc_loopback_stop_override_hdl);
      port_p->mc[sess_hdl].mc_loopback_stop_override_hdl = 0;
    }
  }

  DIAG_PRINT("Multicast loopback Stop DONE \n");
}

/* Stop multicast loopback test */
bf_status_t bf_diag_multicast_loopback_test_stop(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Invalid dev \n");
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_multicast_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type loopback pair\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    DIAG_PRINT("Loop params invalid \n");
    return BF_INVALID_ARG;
  }

  diag_multicast_loopback_test_stop_helper(dev_id, sess_hdl);

#if defined(DIAG_PATTERN_SHIFT_ENABLE)
  diag_pd_read_counter(dev_id);
#endif

  return BF_SUCCESS;
}

/* Get multicast loopback test status */
bf_diag_test_status_e bf_diag_multicast_loopback_test_status_get(
    bf_diag_sess_hdl_t sess_hdl) {
  bf_dev_port_t port = 0;
  int port_idx = 0;
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_multicast_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type multicast loopback\n",
               sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  for (port_idx = 0; port_idx < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
       port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    /* Get the stats */
    test_status =
        bf_diag_multicast_loopback_test_port_status_get(sess_hdl, port, &stats);
    if (test_status != BF_DIAG_TEST_STATUS_PASS) {
      return test_status;
    }
  }

  return BF_DIAG_TEST_STATUS_PASS;
}

/* Get multicast loopback test status for each port */
bf_diag_test_status_e bf_diag_multicast_loopback_test_port_status_get(
    bf_diag_sess_hdl_t sess_hdl,
    bf_dev_port_t port,
    bf_diag_port_stats_t *stats) {
  bf_diag_test_status_e test_status = BF_DIAG_TEST_STATUS_PASS;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!stats) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  memset(stats, 0, sizeof(bf_diag_port_stats_t));

  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!diag_test_type_multicast_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type multicast loopback\n",
               sess_hdl);
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  stats->total_bytes_with_bit_flip_detected =
      sess_info->total_bytes_with_bit_flip_detected;
  stats->total_bits_with_bit_flip_detected =
      sess_info->total_bits_with_bit_flip_detected;

  stats->total_1_to_0_flips = sess_info->total_1_to_0_flips;
  stats->total_0_to_1_flips = sess_info->total_0_to_1_flips;

  stats->total_weak_suspect_for_setup = sess_info->total_weak_suspect_for_setup;
  stats->total_strong_suspect_for_setup =
      sess_info->total_strong_suspect_for_setup;
  stats->total_weak_suspect_for_hold = sess_info->total_weak_suspect_for_hold;
  stats->total_strong_suspect_for_hold =
      sess_info->total_strong_suspect_for_hold;
  stats->total_unknown_failures = sess_info->total_unknown_failures;
  stats->total_payload_setup_failures = sess_info->total_payload_setup_failures;
  stats->total_mixed_failures = sess_info->total_mixed_failures;

  /* Get the stats */
  diag_cpu_port_stats_get(dev_id, port, sess_hdl, stats);

  if ((stats->tx_total) != (stats->rx_good)) {
    return BF_DIAG_TEST_STATUS_FAIL;
  } else if ((stats->tx_total == 0) &&
             diag_any_pkt_size_sent_valid(sess_info)) {
    /* Start test was done, but no pkts were transmitted */
    return BF_DIAG_TEST_STATUS_FAIL;
  }

  return test_status;
}

/* Cleanup multicast loopback test */
bf_status_t bf_diag_multicast_loopback_test_cleanup(
    bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_port_t port = 0;
  int port_idx = 0, num_ports = 0;
  bf_diag_port_lpbk_mode_e diag_loop_mode = BF_DIAG_PORT_LPBK_NONE;
  diag_port_t *port_p = NULL;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;
  int arr_idx = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  /* Validate device */
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_multicast_loopback(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type multicast loopback\n",
               sess_hdl);
    return BF_INVALID_ARG;
  }

  DIAG_TX_PKT_COMPLETIONS(dev_id) = 0;

  if (!(DIAG_GET_LOOPBACK_PARAMS(sess_info).valid)) {
    return BF_INVALID_ARG;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    port_p = diag_get_port_info(dev_id, port);
    if (!port_p) {
      continue;
    }

    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode !=
        BF_DIAG_PORT_LPBK_EXT) {
      /* If there is another session using this port, don't change the
         loopback mode yet.
      */
      if (!diag_port_in_multiple_sessions(dev_id, port)) {
        /* Set the loopback mode */
        bf_diag_port_loopback_mode_set(dev_id, port, diag_loop_mode);
      }
    }
    /* Remove Fwd to CPU entry */
    if (port_p->to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->to_dst_override_hdl[sess_hdl]);
      port_p->to_dst_override_hdl[sess_hdl] = 0;
    }
    if (port_p->rev_to_dst_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->rev_to_dst_override_hdl[sess_hdl]);
      port_p->rev_to_dst_override_hdl[sess_hdl] = 0;
    }
    /* Remove Fwd from CPU entry */
    if (port_p->from_src_override_hdl[sess_hdl]) {
      bf_diag_forwarding_rule_del(dev_id,
                                  port_p->from_src_override_hdl[sess_hdl]);
      port_p->from_src_override_hdl[sess_hdl] = 0;
    }
    /* Remove pair stop entry */
    if (port_p->mc[sess_hdl].mc_loopback_stop_override_hdl) {
      bf_diag_forwarding_rule_del(
          dev_id, port_p->mc[sess_hdl].mc_loopback_stop_override_hdl);
      port_p->mc[sess_hdl].mc_loopback_stop_override_hdl = 0;
    }

    /* Remove the MGID */
    diag_pd_mc_mgrp_ports_del(dev_id,
                              port_p->mc[sess_hdl].mgrp_hdl_a,
                              port_p->mc[sess_hdl].node_hdl_a);
    if (port_p->mc[sess_hdl].mgrp_hdl_b != 0) {
      for (arr_idx = 0; arr_idx < BF_DIAG_MAX_PORTS; arr_idx++) {
        if (port_p->mc[sess_hdl].node_hdl_b[arr_idx] == 0) {
          break;
        }
        diag_pd_mc_mgrp_ports_del(dev_id,
                                  port_p->mc[sess_hdl].mgrp_hdl_b,
                                  port_p->mc[sess_hdl].node_hdl_b[arr_idx]);
      }
      for (arr_idx = 0; arr_idx < DIAG_MC_TEST_MAX_COPIES_PER_PORT; arr_idx++) {
        if (port_p->mc[sess_hdl].copies_node_hdl_b[arr_idx] == 0) {
          break;
        }
        diag_pd_mc_mgrp_ports_del(
            dev_id,
            port_p->mc[sess_hdl].mgrp_hdl_b,
            port_p->mc[sess_hdl].copies_node_hdl_b[arr_idx]);
      }
    }

    /* Ecmp cleanup */
    diag_pd_mc_dissociate_ecmp(
        dev_id, port_p->mc[sess_hdl].mgrp_hdl_a, port_p->mc[sess_hdl].ecmp_hdl);
    if (port_p->mc[sess_hdl].ecmp_node_hdl != 0) {
      diag_pd_mc_ecmp_ports_del(dev_id,
                                port_p->mc[sess_hdl].ecmp_hdl,
                                port_p->mc[sess_hdl].ecmp_node_hdl);
    }
    /* Destroy ecmp */
    diag_pd_mc_ecmp_destory(dev_id, port_p->mc[sess_hdl].ecmp_hdl);

    /* Destory multicast group */
    diag_pd_mc_mgrp_destroy(dev_id, port_p->mc[sess_hdl].mgrp_hdl_a);
    if (port_p->mc[sess_hdl].mgrp_hdl_b != 0) {
      diag_pd_mc_mgrp_destroy(dev_id, port_p->mc[sess_hdl].mgrp_hdl_b);
    }

    /* Remove the lag members */
    bf_mc_port_map_t mc_lag_port_map;
    BF_MC_PORT_MAP_INIT(mc_lag_port_map);
    diag_pd_mc_set_lag_membership(
        dev_id, port_p->mc[sess_hdl].lag_id, mc_lag_port_map);

    port_p->mc[sess_hdl].mc_grp_id_a = 0;
    port_p->mc[sess_hdl].mc_grp_id_b = 0;
    port_p->mc[sess_hdl].mgrp_hdl_a = 0;
    port_p->mc[sess_hdl].mgrp_hdl_b = 0;
    port_p->mc[sess_hdl].node_hdl_a = 0;
    for (arr_idx = 0; arr_idx < BF_DIAG_MAX_PORTS; arr_idx++) {
      port_p->mc[sess_hdl].node_hdl_b[arr_idx] = 0;
    }
    for (arr_idx = 0; arr_idx < DIAG_MC_TEST_MAX_COPIES_PER_PORT; arr_idx++) {
      port_p->mc[sess_hdl].copies_node_hdl_b[arr_idx] = 0;
    }
    port_p->mc[sess_hdl].lag_id = 0;
    port_p->mc[sess_hdl].ecmp_hdl = 0;

    diag_cpu_port_stats_clear(dev_id, port, sess_hdl, false);
  }

  DIAG_LOOPBACK_PARAMS_CLEAR(sess_info);
  // Cleanup the pkt database that we created
  diag_cleanup_pkt_database(sess_hdl);
  diag_session_info_del(sess_hdl);
  diag_session_hdl_free(sess_hdl);

  return status;
}

/* Set the data pattern for the packets */
bf_status_t bf_diag_data_pattern_set(bf_diag_sess_hdl_t sess_hdl,
                                     bf_diag_data_pattern_t mode,
                                     uint8_t start_pat,
                                     uint32_t start_pat_len,
                                     uint8_t pat_a,
                                     uint8_t pat_b,
                                     uint32_t pattern_len) {
  return diag_data_pattern_set_helper(sess_hdl,
                                      mode,
                                      start_pat,
                                      start_pat_len,
                                      pat_a,
                                      pat_b,
                                      pattern_len,
                                      NULL);
}

/* Set the packet payload */
bf_status_t bf_diag_packet_payload_set(bf_diag_sess_hdl_t sess_hdl,
                                       bf_diag_packet_payload_t mode,
                                       char *payload_str,
                                       char *payload_file_path) {
  return diag_packet_payload_set_helper(
      NULL, sess_hdl, mode, payload_str, payload_file_path);
}

/* Set the full packet */
bf_status_t bf_diag_packet_full_set(bf_diag_sess_hdl_t sess_hdl,
                                    bf_diag_packet_full_t mode,
                                    char *pkt_str,
                                    char *pkt_file_path) {
  return diag_packet_full_set_helper(
      NULL, sess_hdl, mode, pkt_str, pkt_file_path);
}

/* Change the max allowed sessions */
bf_status_t bf_diag_sessions_max_set(bf_dev_id_t dev_id,
                                     uint32_t max_sessions) {
  return diag_sessions_max_set_helper(dev_id, max_sessions, NULL);
}

/* Allow min pkt size of 64 for diag pkts */
bf_status_t bf_diag_min_packet_size_enable(bool enable) {
  DIAG_MIN_PKT_SIZE_ENABLED = enable;
  if (DIAG_MIN_PKT_SIZE_ENABLED) {
    DIAG_PKT_ID_REPLICATION_FACTOR = 1;
  } else {
    DIAG_PKT_ID_REPLICATION_FACTOR = PKT_ID_REPLICATION_FACTOR_DEF;
  }

  return BF_SUCCESS;
}

/* Stream setup */
bf_status_t bf_diag_stream_setup(bf_dev_id_t dev_id,
                                 uint32_t num_pkts,
                                 uint32_t pkt_size,
                                 bf_dev_port_t dst_port,
                                 bf_diag_sess_hdl_t *sess_hdl) {
  uint32_t app_id = 0, pkt_buf_offset = 0;
  p4_pd_status_t status = 0;
  bf_status_t bf_status = BF_SUCCESS;
  p4_pd_entry_hdl_t fwd_hdl = 0;
  diag_session_info_t *sess_info = NULL;
  uint32_t aligned_pkt_size = 0;
  uint32_t timer_nsec = diag_get_pktgen_def_trigger_time_nsecs();

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    DIAG_PRINT("Inavlid pkt size %d, Max allowed is %d\n",
               pkt_size,
               DIAG_MAX_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  if (pkt_size < DIAG_MIN_PKT_SIZE) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  diag_port_t *dst_port_p = diag_get_port_info(dev_id, dst_port);
  if (!dst_port_p) {
    DIAG_PRINT("Destination Port %d does not exist \n", dst_port);
    return BF_INVALID_ARG;
  }
  /* Get pipe where the destination port is */
  bf_dev_pipe_t dst_pipe = DEV_PORT_TO_PIPE(dst_port);

  /* Get Aligned pkt size */
  aligned_pkt_size = diag_pgen_get_aligned_pkt_size(pkt_size);

  /* Make sure there is space in the pkt buffer */
  if ((DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[dst_pipe] +
       aligned_pkt_size) > DIAG_PKTGEN_PKT_BUF_SIZE) {
    DIAG_PRINT("Pktgen pkt buffer full, cleanup unused pktgen sessions \n");
    return BF_INVALID_ARG;
  }
  pkt_buf_offset = DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[dst_pipe];

  bf_status = diag_pgen_find_free_app(dev_id, dst_pipe, &app_id);
  if (bf_status != BF_SUCCESS) {
    DIAG_PRINT("No free pktgen app, cleanup unused pktgen sessions \n");
    return bf_status;
  }

  /* Allocate session */
  bf_status = diag_session_hdl_alloc(dev_id, sess_hdl);
  if (bf_status != BF_SUCCESS) {
    DIAG_PRINT("Failed to allocate session on dev %d\n", dev_id);
    return bf_status;
  }

  /* Use local dst port as src port */
  bf_dev_port_t src_port = DEV_PORT_TO_LOCAL_PORT(dst_port);
  /* Use pktgen port on the destination pipe */
  bf_dev_port_t pktgen_port = diag_get_pktgen_port(dev_id, dst_pipe, app_id);
  /* Call PD */
  status = diag_pd_stream_setup(dev_id,
                                dst_pipe,
                                app_id,
                                pkt_size,
                                pkt_buf_offset,
                                pktgen_port,
                                src_port,
                                num_pkts,
                                timer_nsec);
  if (status != 0) {
    diag_session_hdl_free(*sess_hdl);
    return status;
  }

  /* Setup the forwarding rule */
  int priority = 0;
  bf_diag_forwarding_rule_add(dev_id,
                              dst_port,
                              dst_port,
                              BF_DIAG_TCP_DSTPORT_MIN,
                              BF_DIAG_TCP_DSTPORT_MAX,
                              priority,
                              &fwd_hdl);

  /* Reserve the app-id */
  diag_pgen_app_reserve(dev_id, dst_pipe, app_id, false);

  /* Update the global pkt-gen offset */
  DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[dst_pipe] +=
      aligned_pkt_size;

  /* Save the session */
  status = diag_save_loopback_test_params(
      dev_id, *sess_hdl, NULL, 0, BF_DIAG_PORT_LPBK_NONE, DIAG_TEST_STREAM);

  sess_info = diag_session_info_get(*sess_hdl);
  if (!sess_info) {
    DIAG_PRINT("Diag session %u not found after creation \n", *sess_hdl);
  } else {
    /* Save pktgen params in session */
    DIAG_GET_PGEN_PARAMS(sess_info).dev_id = dev_id;
    DIAG_GET_PGEN_PARAMS(sess_info).fwd_hdl = fwd_hdl;
    DIAG_GET_PGEN_PARAMS(sess_info).app_id = app_id;
    DIAG_GET_PGEN_PARAMS(sess_info).pipe = dst_pipe;
    DIAG_GET_PGEN_PARAMS(sess_info).pktgen_port = pktgen_port;
    DIAG_GET_PGEN_PARAMS(sess_info).src_port = src_port;
    DIAG_GET_PGEN_PARAMS(sess_info).dst_port = dst_port;
    DIAG_GET_PGEN_PARAMS(sess_info).pkt_size = pkt_size;
    DIAG_GET_PGEN_PARAMS(sess_info).aligned_pkt_size = aligned_pkt_size;
    DIAG_GET_PGEN_PARAMS(sess_info).num_pkts = num_pkts;
    DIAG_GET_PGEN_PARAMS(sess_info).pkt_buf_offset = pkt_buf_offset;
    DIAG_GET_PGEN_PARAMS(sess_info).timer_nsec = timer_nsec;
    DIAG_GET_PGEN_PARAMS(sess_info).enabled = false;
  }

  return BF_SUCCESS;
}

/* Stream start */
bf_status_t bf_diag_stream_start(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;
  uint32_t app_id = 0, pkt_size = 0;
  uint32_t pkt_buf_offset = 0;
  bf_dev_pipe_t pipe = 0;
  diag_session_info_t *sess_info = NULL;
  uint8_t pkt_buf[DIAG_MAX_PKT_SIZE];

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type pktgen\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  dev_id = DIAG_GET_PGEN_PARAMS(sess_info).dev_id;
  app_id = DIAG_GET_PGEN_PARAMS(sess_info).app_id;
  pkt_size = DIAG_GET_PGEN_PARAMS(sess_info).pkt_size;
  pkt_buf_offset = DIAG_GET_PGEN_PARAMS(sess_info).pkt_buf_offset;
  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;

  /* Update DB */
  DIAG_GET_PGEN_PARAMS(sess_info).enabled = true;

  memset(pkt_buf, 0, sizeof(pkt_buf));
  int ret = diag_create_pkt(
      sess_hdl, pkt_size, pkt_buf, 1, DIAG_MIN_PKT_SIZE_ENABLED);
  if (ret < 0) {
    DIAG_PRINT("Pkt generation failed for session %u\n", sess_hdl);
    return BF_INVALID_ARG;
  }

  /* Call PD */
  status = diag_pd_stream_start(
      dev_id, pipe, app_id, pkt_size, pkt_buf, pkt_buf_offset);

  return status;
}

/* Stream adjust */
bf_status_t bf_diag_stream_adjust(bf_diag_sess_hdl_t sess_hdl,
                                  uint32_t num_pkts,
                                  uint32_t pkt_size) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t pipe = 0;
  uint32_t curr_pkt_size = 0, curr_aligned_pkt_size = 0;
  uint32_t pkt_buf_offset = 0, aligned_pkt_size = 0;
  diag_session_info_t *sess_info = NULL;
  uint8_t pkt_buf[DIAG_MAX_PKT_SIZE];

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (pkt_size > DIAG_MAX_PKT_SIZE) {
    DIAG_PRINT("Inavlid pkt size %d, Max allowed is %d\n",
               pkt_size,
               DIAG_MAX_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  if (pkt_size < DIAG_MIN_PKT_SIZE) {
    DIAG_PRINT("Minimum pkt size is %d\n", DIAG_MIN_PKT_SIZE);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type pktgen\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  /* Get Aligned pkt size */
  aligned_pkt_size = diag_pgen_get_aligned_pkt_size(pkt_size);

  dev_id = DIAG_GET_PGEN_PARAMS(sess_info).dev_id;
  curr_pkt_size = DIAG_GET_PGEN_PARAMS(sess_info).pkt_size;
  curr_aligned_pkt_size = DIAG_GET_PGEN_PARAMS(sess_info).aligned_pkt_size;
  pkt_buf_offset = DIAG_GET_PGEN_PARAMS(sess_info).pkt_buf_offset;
  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;

  /* If pkt-size is different the pkt-buf needs to be re-carved */
  if (curr_pkt_size != pkt_size) {
    /* Check if any other app has been allocated after this one */
    if ((pkt_buf_offset + curr_aligned_pkt_size) !=
        DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[pipe]) {
      DIAG_PRINT(
          "ERROR: Cannot change pkt size from %d to %d, delete other stream"
          " sessions to change pkt-size\n",
          curr_pkt_size,
          pkt_size);
      return BF_INVALID_ARG;
    }
    DIAG_PRINT("Changing pkt size from %d to %d for stream session %d\n",
               curr_pkt_size,
               pkt_size,
               sess_hdl);
  }

  /* Update the global pkt-gen offset */
  DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[pipe] =
      pkt_buf_offset + aligned_pkt_size;

  /* Update DB with new info */
  DIAG_GET_PGEN_PARAMS(sess_info).pkt_size = pkt_size;
  DIAG_GET_PGEN_PARAMS(sess_info).aligned_pkt_size = aligned_pkt_size;
  DIAG_GET_PGEN_PARAMS(sess_info).num_pkts = num_pkts;

  memset(pkt_buf, 0, sizeof(pkt_buf));
  int ret = diag_create_pkt(
      sess_hdl, pkt_size, pkt_buf, 1, DIAG_MIN_PKT_SIZE_ENABLED);
  if (ret < 0) {
    DIAG_PRINT("Pkt generation failed for session %u\n", sess_hdl);
    return BF_INVALID_ARG;
  }

  /* Call PD */
  status = diag_pd_stream_adjust(dev_id,
                                 pipe,
                                 DIAG_GET_PGEN_PARAMS(sess_info).app_id,
                                 pkt_size,
                                 pkt_buf_offset,
                                 DIAG_GET_PGEN_PARAMS(sess_info).pktgen_port,
                                 DIAG_GET_PGEN_PARAMS(sess_info).src_port,
                                 num_pkts,
                                 DIAG_GET_PGEN_PARAMS(sess_info).timer_nsec,
                                 pkt_buf,
                                 DIAG_GET_PGEN_PARAMS(sess_info).enabled);

  return status;
}

/* Stream stop */
bf_status_t bf_diag_stream_stop(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t pipe = 0;
  uint32_t app_id = 0;
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type pktgen\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  dev_id = DIAG_GET_PGEN_PARAMS(sess_info).dev_id;
  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;
  app_id = DIAG_GET_PGEN_PARAMS(sess_info).app_id;

  /* Update DB */
  DIAG_GET_PGEN_PARAMS(sess_info).enabled = false;

  /* Call PD */
  status = diag_pd_stream_stop(dev_id, pipe, app_id);

  return status;
}

/* Stream counter get */
bf_status_t bf_diag_stream_counter_get(bf_diag_sess_hdl_t sess_hdl,
                                       uint64_t *counter) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;
  bf_dev_pipe_t pipe = 0;
  uint32_t app_id = 0;
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type pktgen\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  dev_id = DIAG_GET_PGEN_PARAMS(sess_info).dev_id;
  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;
  app_id = DIAG_GET_PGEN_PARAMS(sess_info).app_id;

  /* Call PD */
  status = diag_pd_stream_counter_get(dev_id, pipe, app_id, counter);

  return status;
}

/* Stream cleanup */
bf_status_t bf_diag_stream_cleanup(bf_diag_sess_hdl_t sess_hdl) {
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;
  uint32_t app_id = 0;
  bf_dev_pipe_t pipe = 0;
  bf_dev_port_t pktgen_port = 0;
  p4_pd_entry_hdl_t fwd_hdl = 0;
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    DIAG_PRINT("Session handle %u is not of test type pktgen\n", sess_hdl);
    return BF_INVALID_ARG;
  }
  dev_id = DIAG_GET_PGEN_PARAMS(sess_info).dev_id;
  app_id = DIAG_GET_PGEN_PARAMS(sess_info).app_id;
  fwd_hdl = DIAG_GET_PGEN_PARAMS(sess_info).fwd_hdl;
  pktgen_port = DIAG_GET_PGEN_PARAMS(sess_info).pktgen_port;
  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;

  /* Stop the stream if it is running */
  if (DIAG_GET_PGEN_PARAMS(sess_info).enabled) {
    status = diag_pd_stream_stop(dev_id, pipe, app_id);
  }

  /* Delete forwarding rule */
  if (fwd_hdl) {
    bf_diag_forwarding_rule_del(dev_id, fwd_hdl);
  }

  /* Free the app-id */
  diag_pgen_app_reserve(dev_id, pipe, app_id, true);

  /* Call PD */
  status = diag_pd_stream_cleanup(
      dev_id, pktgen_port, !diag_pgen_is_any_app_used(dev_id, pipe));

  /* Fix the pkt buf offset */
  diag_pgen_adjust_global_pkt_buf_offset(dev_id, sess_info);

  /* Free the session */
  DIAG_LOOPBACK_PARAMS_CLEAR(sess_info);
  DIAG_PGEN_PARAMS_CLEAR(sess_info);
  // Cleanup the pkt database that we created
  diag_cleanup_pkt_database(sess_hdl);
  diag_session_info_del(sess_hdl);
  diag_session_hdl_free(sess_hdl);

  return status;
}

/* Apply GFM test patterns.
 * For use during snake/pair tests in the TF2 DIAG_PHV_MOCHA_DARK profile.
 * The mode argument should be 0 when the packet payload causes all PHVs to
 * carry a value of zero.  The mode argument should be 1 when the packet payload
 * causes the PHVs to carry a value of all bits set.
 */
bf_status_t bf_diag_gfm_pattern(bf_dev_id_t dev_id, int mode) {
  bf_status_t status = BF_SUCCESS;

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  const int gfm_rows = 1024;
  uint64_t row_patterns[2];
  uint16_t col_data[64] = {0};
  switch (mode) {
    case 1:
      /* PHV data is all 1s.
       * Apply various GFM patterns knowing all GFM rows will be enabled. */

      /* Program to all 1s and then all 0s to catch single bit stuck-at. */
      row_patterns[0] = 0x000FFFFFFFFFFFFFull;
      status = diag_pd_gfm_pattern(dev_id, 1, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);
      row_patterns[0] = 0x0000000000000000ull;
      status = diag_pd_gfm_pattern(dev_id, 1, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);

      /* Program to alternating bit patterns to catch multi-bit stuck-at. */
      row_patterns[0] = 0x000AAAAAAAAAAAAAull;
      row_patterns[1] = 0x0005555555555555ull;
      status = diag_pd_gfm_pattern(dev_id, 2, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);
      row_patterns[1] = 0x000AAAAAAAAAAAAAull;
      row_patterns[0] = 0x0005555555555555ull;
      status = diag_pd_gfm_pattern(dev_id, 2, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);
      row_patterns[0] = 0x000F0F0F0F0F0F0Full;
      row_patterns[1] = 0x0000F0F0F0F0F0F0ull;
      status = diag_pd_gfm_pattern(dev_id, 2, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);
      row_patterns[1] = 0x000F0F0F0F0F0F0Full;
      row_patterns[0] = 0x0000F0F0F0F0F0F0ull;
      status = diag_pd_gfm_pattern(dev_id, 2, row_patterns, NULL);
      if (status) goto done;
      bf_sys_usleep(10000);

      /* Check for inputs stuck at zero.  A zero input would zero the
       * corresponding GFM row which removes it from the parity computation.  To
       * check if any rows are stuck at zero we program an even number of rows
       * with odd parity.  If all those rows are included they will XOR together
       * and go to zero (even parity), but if one of them is disabled due to an
       * input of zero, they will XOR together and get an odd parity result. */
      row_patterns[0] = 0x0000000000000000ull;
      status = diag_pd_gfm_pattern(dev_id, 1, row_patterns, NULL);
      if (status) goto done;
      /* GFM is all zeros now, change column 51 to one to get odd parity.
       * We do this on row zero and, in a loop, rows 1-1023. */
      for (int row = 1; row < gfm_rows; ++row) {
        col_data[0] = 1;
        col_data[row / 16] |= 1u << (row % 16);
        status = diag_pd_gfm_col(dev_id, 51, col_data);
        if (status) goto done;
        /* Just one MS here since we loop for 1k-1 iterations; that is plenty of
         * time for packets to exercise the GFM hardware. */
        bf_sys_usleep(1000);
        /* Reset column data before we use it again. */
        col_data[0] = 0;
        col_data[row / 16] = 0;
      }

      /* Put the GFM back into a good state (no rows with bad parity) before we
       * are done. */
      status = diag_pd_gfm_col(dev_id, 51, col_data);
      if (status) goto done;
      break;
    case 0:
      /* PHV data is all 0s.
       * Apply various GFM patterns knowing all GFM rows will be disabled.
       * Program one row at a time with bad parity to check for inputs stuck-at
       * one. */
      row_patterns[0] = 0x0000000000000000ull;
      status = diag_pd_gfm_pattern(dev_id, 1, row_patterns, NULL);
      if (status) goto done;
      /* Now that all rows are set to zero reprogram a single column 1024 times.
       * Each time we set a different bit in the column to one which forces bad
       * parity on that row. */
      for (int i = 0; i < gfm_rows; ++i) {
        col_data[i / 16] = 1u << (i % 16);
        status = diag_pd_gfm_col(dev_id, 51, col_data);
        if (status) goto done;
        /* Just one MS here since we loop for 1k iterations; that is plenty of
         * time for packets to exercise the GFM hardware. */
        bf_sys_usleep(1000);
        col_data[i / 16] = 0;
      }
      /* Put the GFM back into a good state (no rows with bad parity) before we
       * are done. */
      status = diag_pd_gfm_col(dev_id, 51, col_data);
      if (status) goto done;
      break;
    default:
      DIAG_PRINT("Unexpected GFM pattern mode %d, expected 0 or 1\n", mode);
      return BF_INVALID_ARG;
  }

done:
  if (status) {
    DIAG_PRINT("Error %s setting GFM test patterns\n", bf_err_str(status));
  }
  return status;
}
