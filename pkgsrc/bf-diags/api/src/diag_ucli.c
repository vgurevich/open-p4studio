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
 * @file diag_ucli.c
 * @date
 *
 * Contains implementation of diag ucli
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include "diag_common.h"
#include "diags/bf_diag_api.h"
#include "diag_pkt.h"
#include "diag_pd.h"
#include "diag_vlan.h"
#include "diag_util.h"
#include "diag_create_pkt.h"
#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define DIAG_LOOP_MODE_MAX 5

#ifndef DIAG_PROFILE
#define DIAG_PROFILE "No profile specified"
#endif

static ucli_node_t *diag_ucli_node;

/* Parse port list in this format: 1-4,6,11-13,14  */
void diag_parse_port_list(char *ports_bm,
                          bf_dev_port_t *port_arr,
                          int *num_ports) {
  int j = 0, first_port = 0, last_port = 0, temp_port = 0;
  char first_port_str[200];
  const char delimiters_ports[] = ",";
  char *ports = NULL, *range_sep;
  int port_arr_cntr = 0;

  if (ports_bm != NULL) {
    ports = strtok(ports_bm, delimiters_ports);
  }

  while (ports != NULL) {
    /* Start of range */
    range_sep = strchr(ports, '-');
    if (range_sep != NULL) {
      memset(first_port_str, 0, sizeof(first_port_str));
      // DIAG_PRINT("Range sep at %ld \n", (range_sep - ports));
      strncpy(first_port_str, ports, (range_sep - ports));
      // DIAG_PRINT("first_port_str %s \n", first_port_str);
      first_port = strtoul(first_port_str, NULL, 0);
      last_port = strtoul(range_sep + 1, NULL, 0);
      if ((first_port >= BF_DIAG_MAX_PORTS) ||
          (last_port >= BF_DIAG_MAX_PORTS)) {
        DIAG_PRINT("Port parsing failed, first_port %d, last_port %d",
                   first_port,
                   last_port);
        break;
      }
      /* Put ports from range */
      for (j = first_port; j <= last_port; j++) {
        port_arr[port_arr_cntr] = j;
        port_arr_cntr = port_arr_cntr + 1;
      }
    } else {
      temp_port = strtoul(ports, NULL, 0);
      if (temp_port >= BF_DIAG_MAX_PORTS) {
        DIAG_PRINT("Port parsing failed, port %d ", temp_port);
        break;
      }
      port_arr[port_arr_cntr] = temp_port;
      port_arr_cntr = port_arr_cntr + 1;
    }
    ports = strtok(NULL, delimiters_ports);
  }
  *num_ports = port_arr_cntr;
}

bool is_diag_cpu_port_any_channel(bf_dev_id_t dev_id, bf_dev_port_t port) {
  int chnl = 0;
  int cpu_port = bf_eth_cpu_port_get(dev_id);

  /* Check for all 4 channels of cpu port */
  for (chnl = 0; chnl < 4; chnl++) {
    if ((port == (DIAG_DEV_INFO(dev_id)->cpu_port + chnl)) ||
        (port == (cpu_port + chnl))) {
      return true;
    }
  }

  return false;
}

void diag_add_cpu_port_to_list(bf_dev_id_t dev_id,
                               bf_dev_port_t *port_arr,
                               int *num_ports,
                               bool twice) {
  bf_dev_port_t port = 0;
  int port_cnt = *num_ports;

  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    /* Skip non-CPU ports */
    if (!is_diag_cpu_port_any_channel(dev_id, port)) {
      continue;
    }
    if (bf_port_is_valid(dev_id, port) == BF_SUCCESS) {
      if (port_cnt >= BF_DIAG_MAX_PORTS) {
        DIAG_PRINT("Could not add cpu port");
        return;
      }
      port_arr[port_cnt] = port;
      port_cnt++;
      /* Port needs to be added twice for pair-test */
      if (twice) {
        port_arr[port_cnt] = port;
        port_cnt++;
      }
    }
  }

  *num_ports = port_cnt;
}

void diag_get_all_ports_list(bf_dev_id_t dev_id,
                             bf_dev_port_t *port_arr,
                             int *num_ports,
                             bool allow_internal) {
  int port_cnt = 0;
  bf_dev_port_t port = 0;
  bool is_internal = false;
  int max_local_port_val = 64;

  /* We want to skip the internal ports on the chip */
  if (diag_is_chip_family_tofino1(dev_id)) {
    max_local_port_val = 64;
  } else {
    max_local_port_val = 72;
  }

  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if ((bf_port_is_valid(dev_id, port) == BF_SUCCESS) &&
        DIAG_DEVPORT_VALID(dev_id, port) &&
        /* Skip internal ports */
        (DEV_PORT_TO_LOCAL_PORT(port) < max_local_port_val)) {
      is_internal = false;
      lld_sku_is_dev_port_internal(dev_id, port, &is_internal);
      /* Is it allowed to add internal ports */
      if ((is_internal) && (allow_internal)) {
        is_internal = false;
      }
      /* Skip CPU ports */
      if (!is_diag_cpu_port_any_channel(dev_id, port) && (!is_internal)) {
        port_arr[port_cnt] = port;
        port_cnt++;
      }
    }
  }

  *num_ports = port_cnt;
}

/* Get all ports on a pipe */
void diag_get_all_pipe_ports_list(bf_dev_id_t dev_id,
                                  bf_dev_port_t *port_arr,
                                  int *num_ports,
                                  bool allow_internal,
                                  bf_dev_pipe_t pipe) {
  int port_cnt = 0;
  bf_dev_port_t port = 0;
  bool is_internal = false;

  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if (DEV_PORT_TO_PIPE(port) != pipe) {
      continue;
    }
    if (bf_port_is_valid(dev_id, port) == BF_SUCCESS) {
      is_internal = false;
      lld_sku_is_dev_port_internal(dev_id, port, &is_internal);
      /* Is it allowed to add internal ports */
      if ((is_internal) && (allow_internal)) {
        is_internal = false;
      }
      /* Skip CPU ports */
      if (!is_diag_cpu_port_any_channel(dev_id, port) && (!is_internal)) {
        port_arr[port_cnt] = port;
        port_cnt++;
      }
    }
  }

  *num_ports = port_cnt;
}

/* All mesh port list has all ports, but
   they are arranged in a different order to allow
   different testing pattern.
   Dev Port order: 0,444,4,440,8...
*/
void diag_get_all_mesh_ports_list(bf_dev_id_t dev_id,
                                  bf_dev_port_t *port_arr,
                                  int *num_ports,
                                  bool allow_internal) {
  int cntr = 0, all_num_ports = 0;
  bf_dev_port_t all_port_arr[BF_DIAG_MAX_PORTS];

  memset(&all_port_arr[0], 0, sizeof(all_port_arr));

  diag_get_all_ports_list(
      dev_id, &all_port_arr[0], &all_num_ports, allow_internal);
  *num_ports = 0;
  for (cntr = 0; cntr < all_num_ports / 2;) {
    if (((*num_ports) % 2) == 0) {
      port_arr[*num_ports] = all_port_arr[cntr];
    } else {
      port_arr[*num_ports] = all_port_arr[all_num_ports - cntr - 1];
      cntr++;
    }
    *num_ports = *num_ports + 1;
  }
  /* Copy the middle element if num of ports were odd */
  if ((all_num_ports % 2) != 0) {
    port_arr[*num_ports] = all_port_arr[all_num_ports / 2];
    *num_ports = *num_ports + 1;
  }
}

static void diag_print_test_status(bf_diag_test_status_e test_status,
                                   ucli_context_t *uc) {
  switch (test_status) {
    case BF_DIAG_TEST_STATUS_FAIL:
      aim_printf(&uc->pvs, "  Test Status: Fail \n");
      break;
    case BF_DIAG_TEST_STATUS_PASS:
      aim_printf(&uc->pvs, "  Test Status: Pass \n");
      break;
    case BF_DIAG_TEST_STATUS_IN_PROGRESS:
      aim_printf(&uc->pvs, "  Test Status: In Progress \n");
      break;
    case BF_DIAG_TEST_STATUS_UNKNOWN:
    default:
      aim_printf(&uc->pvs, "  Test Status: Unknown \n");
      break;
  }
}

static ucli_status_t diag_ucli_ucli__loopback_test_setup__(ucli_context_t *uc) {
  int num_ports = 0;
  bf_dev_id_t dev_id = 0;
  int loop_mode = 0;
  const char *port_range = NULL;
  bf_dev_port_t port_arr[BF_DIAG_MAX_PORTS];
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  memset(&port_arr[0], 0, sizeof(port_arr));

  UCLI_COMMAND_INFO(uc,
                    "lpbk-setup",
                    3,
                    "lpbk-setup <dev> <port-range> <loop-mode: "
                    "0=none,1=mac,2=phy,3=ext,4=pcs,5=pipe>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  loop_mode = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  if (loop_mode > DIAG_LOOP_MODE_MAX) {
    aim_printf(&uc->pvs, "diag:: Invalid <loop-mode=%d> \n", loop_mode);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Loopback test setup <dev=%d> <port-range=%s>"
             "<loop-mode=%d> \n",
             dev_id,
             port_range,
             loop_mode);

  if (diag_validate_loopback_mode(dev_id, loop_mode) != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "ERROR: loopback mode %d not supported on this chip \n",
               loop_mode);
    return 0;
  }

  if (strncmp(port_range, "alli", strlen("alli")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, true);
    diag_add_cpu_port_to_list(dev_id, &port_arr[0], &num_ports, false);
  } else if (strncmp(port_range, "all", strlen("all")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, false);
    diag_add_cpu_port_to_list(dev_id, &port_arr[0], &num_ports, false);
  } else {
    diag_parse_port_list((char *)port_range, &port_arr[0], &num_ports);
  }
  aim_printf(&uc->pvs, "Num ports is %d \n", num_ports);
  aim_printf(&uc->pvs, "Ports -> ");

  for (int i = 0; i < num_ports; i++) {
    if (i != 0) {
      aim_printf(&uc->pvs, ",", port_arr[i]);
    }
    aim_printf(&uc->pvs, "%d", port_arr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  status = bf_diag_loopback_test_setup(
      dev_id, &port_arr[0], num_ports, loop_mode, &sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Loopback test setup success \n");
    aim_printf(&uc->pvs, "Loopback Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Loopback test setup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__loopback_test_start__(ucli_context_t *uc) {
  int pkt_size = 0, num_pkt = 0;
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "lpbk-start",
                    3,
                    "lpbk-start <sess-hdl(0=default)> <num-pkt> "
                    "<pkt-size(0=random)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkt = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);

  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Loopback test start <sess-hdl=%d> <num-pkt=%d>"
             "<pkt-size=%d> \n",
             sess_hdl,
             num_pkt,
             pkt_size);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_loopback_test_start(sess_hdl, num_pkt, pkt_size);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Loopback test started \n");
  } else {
    aim_printf(&uc->pvs, "Loopback test start failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__loopback_test_stop__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "lpbk-stop", 1, "lpbk-stop <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Loopback Test stop <sess-hdl=%d>\n", sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_loopback_test_abort(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test stop success \n");
  } else {
    aim_printf(&uc->pvs, "Test stop failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__loopback_test_cleanup__(
    ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(
      uc, "lpbk-cleanup", 1, "lpbk-cleanup <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  } else if (!(DIAG_SESS_VALID(sess_hdl))) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(
      &uc->pvs, "diag:: Loopback Test cleanup <sess-hdl=%d>\n", sess_hdl);

  status = bf_diag_loopback_test_cleanup(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test cleanup success \n");
  } else {
    aim_printf(&uc->pvs, "Test cleanup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__loopback_test_status__(
    ucli_context_t *uc) {
  bf_diag_test_status_e test_status;
  int port_idx = 0;
  bf_dev_port_t port = 0;
  bf_diag_port_stats_t stats;
  bf_diag_sess_hdl_t sess_hdl = 0;
  diag_session_info_t *sess_info = NULL;
  bf_status_t status = BF_SUCCESS;
  bf_dev_id_t dev_id = 0;

  UCLI_COMMAND_INFO(uc, "lpbk-status", 1, "lpbk-status <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Loopback Test status <sess-hdl=%d>\n", sess_hdl);
  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }
  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }

  test_status = bf_diag_loopback_test_status_get(sess_hdl);
  diag_print_test_status(test_status, uc);
  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    aim_printf(&uc->pvs,
               "Session handle %u does not exist on dev %d\n",
               sess_hdl,
               dev_id);
    return 0;
  }
  aim_printf(&uc->pvs, "Port loopback status: \n");
  for (port_idx = 0; port_idx < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
       port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    /* Get the stats */
    memset(&stats, 0, sizeof(stats));
    test_status = bf_diag_loopback_test_port_status_get(sess_hdl, port, &stats);
    aim_printf(&uc->pvs, "Port %d : ", port);
    diag_print_test_status(test_status, uc);
    aim_printf(&uc->pvs,
               "  Stats: Tx-total=%d, rx-total=%d, rx-good=%d, rx_bad=%d \n",
               stats.tx_total,
               stats.rx_total,
               stats.rx_good,
               stats.rx_bad);
  }
  aim_printf(&uc->pvs,
             "No of Tx packet completions: %d\n",
             DIAG_TX_PKT_COMPLETIONS(dev_id));

  return 0;
}

static ucli_status_t diag_ucli_ucli__snake_test_setup__(ucli_context_t *uc) {
  int num_ports = 0;
  bf_dev_id_t dev_id = 0;
  int loop_mode = 0;
  const char *port_range = NULL;
  bf_dev_port_t port_arr[BF_DIAG_MAX_PORTS];
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  memset(&port_arr[0], 0, sizeof(port_arr));

  UCLI_COMMAND_INFO(uc,
                    "snake-setup",
                    3,
                    "snake-setup <dev> <port-range> <loop-mode: "
                    "0=none,1=mac,2=phy,3=ext,4=pcs,5=pipe>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  loop_mode = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  if (loop_mode > DIAG_LOOP_MODE_MAX) {
    aim_printf(&uc->pvs, "diag:: Invalid <loop-mode=%d> \n", loop_mode);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Snake test setup <dev=%d> <port-range=%s>"
             "<loop-mode=%d> \n",
             dev_id,
             port_range,
             loop_mode);

  if (diag_validate_loopback_mode(dev_id, loop_mode) != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "ERROR: loopback mode %d not supported on this chip \n",
               loop_mode);
    return 0;
  }

  if (strncmp(port_range, "all_mesh", strlen("all_mesh")) == 0) {
    diag_get_all_mesh_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else if (strncmp(port_range, "alli", strlen("alli")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, true);
  } else if (strncmp(port_range, "all", strlen("all")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else {
    diag_parse_port_list((char *)port_range, &port_arr[0], &num_ports);
  }
  aim_printf(&uc->pvs, "Num ports is %d \n", num_ports);
  aim_printf(&uc->pvs, "Ports -> ");

  for (int i = 0; i < num_ports; i++) {
    if (i != 0) {
      aim_printf(&uc->pvs, ",", port_arr[i]);
    }
    aim_printf(&uc->pvs, "%d", port_arr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  status = bf_diag_loopback_snake_test_setup(
      dev_id, &port_arr[0], num_ports, loop_mode, &sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Snake test setup success \n");
    aim_printf(&uc->pvs, "Snake Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Snake test setup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__snake_test_start__(ucli_context_t *uc) {
  int pkt_size = 0, num_pkt = 0;
  bf_status_t status = BF_SUCCESS;
  int bidir = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "snake-start",
                    4,
                    "snake-start <sess-hdl(0=default)> <num-pkt> "
                    "<pkt-size>(0=random) "
                    "<bidir-traffic>(0:non-bidir, 1:bidir)");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkt = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);
  bidir = strtoul(uc->pargs->args[3], NULL, 0);

  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }
  if (bidir > 1) {
    aim_printf(&uc->pvs, "diag:: Invalid bidir <bidir=%d>\n", bidir);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Snake test start <sess-hdl=%d> <num-pkt=%d> "
             "<pkt-size=%d> <bidir=%d>\n",
             sess_hdl,
             num_pkt,
             pkt_size,
             bidir);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_SNAKE, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status =
      bf_diag_loopback_snake_test_start(sess_hdl, num_pkt, pkt_size, bidir);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Snake test started \n");
  } else {
    aim_printf(&uc->pvs, "Snake test start failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__snake_test_stop__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "snake-stop", 1, "snake-stop<sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Snake Test stop <sess-hdl=%u>\n", sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_SNAKE, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_loopback_snake_test_stop(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test stop success \n");
  } else {
    aim_printf(&uc->pvs, "Test stop failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__snake_test_cleanup__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(
      uc, "snake-cleanup", 1, "snake-cleanup <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_SNAKE, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  } else if (!(DIAG_SESS_VALID(sess_hdl))) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Snake Test cleanup <sess-hdl=%u>\n", sess_hdl);

  status = bf_diag_loopback_snake_test_cleanup(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test cleanup success \n");
  } else {
    aim_printf(&uc->pvs, "Test cleanup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__snake_test_status__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  bf_diag_sess_hdl_t sess_hdl = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "snake-status", 1, "snake-status <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Snake Test status <sess-hdl=%d>\n", sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_SNAKE, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }
  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }

  memset(&stats, 0, sizeof(stats));
  test_status = bf_diag_loopback_snake_test_status_get(sess_hdl, &stats);
  diag_print_test_status(test_status, uc);
  aim_printf(&uc->pvs,
             "Stats: Tx-total=%d, rx-total=%d, rx-good=%d, rx_bad=%d \n",
             stats.tx_total,
             stats.rx_total,
             stats.rx_good,
             stats.rx_bad);
  aim_printf(&uc->pvs,
             "No of Tx packet completions: %d\n",
             DIAG_TX_PKT_COMPLETIONS(dev_id));

  return 0;
}

static ucli_status_t diag_ucli_ucli__set_loopback_mode__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t dev_port = 0;
  bf_diag_port_lpbk_mode_e loop_mode = BF_DIAG_PORT_LPBK_NONE;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc,
                    "lpbk-mode-set",
                    3,
                    "lpbk-mode-set <dev> <dev-port> <loop-mode: "
                    "0=none,1=mac,2=phy,3=ext,4=pcs,5=pipe>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  dev_port = strtoul(uc->pargs->args[1], NULL, 0);
  loop_mode = strtoul(uc->pargs->args[2], NULL, 0);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    aim_printf(&uc->pvs, "Invalid dev_port %d \n", dev_port);
    return UCLI_STATUS_E_ARG;
  }

  aim_printf(&uc->pvs,
             "diag:: Set Diag loopback mode <dev=%d> <dev-port=%d> "
             "<loop-mode=%d> \n",
             dev_id,
             dev_port,
             loop_mode);

  if (diag_validate_loopback_mode(dev_id, loop_mode) != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "ERROR: loopback mode %d not supported on this chip \n",
               loop_mode);
    return 0;
  }
  status = bf_diag_port_loopback_mode_set(dev_id, dev_port, loop_mode);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Loopback mode set success \n");
  } else {
    aim_printf(&uc->pvs, "Loopback mode set failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__cpu_stats_show__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t dev_port = 0;
  bool all_ports = false;
  bf_diag_port_stats_t stats;

  UCLI_COMMAND_INFO(
      uc,
      "cpu-stats-show",
      3,
      "cpu-stats-show <dev> <type: 0=one port,1=all ports> <dev_port>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  all_ports = strtoul(uc->pargs->args[1], NULL, 0);
  dev_port = strtoul(uc->pargs->args[2], NULL, 0);

  aim_printf(&uc->pvs, "diag:: CPU stats show <dev=%d> \n", dev_id);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  aim_printf(&uc->pvs, "dev-port tx-total rx-total rx-good  rx-bad\n");

  if (all_ports) {
    for (dev_port = 0; dev_port < BF_DIAG_MAX_PORTS; dev_port++) {
      if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
        continue;
      }
      memset(&stats, 0, sizeof(stats));
      bf_diag_cpu_stats_get(dev_id, dev_port, &stats);
      aim_printf(&uc->pvs,
                 "%-8d %-8d %-8d %8d %8d\n",
                 dev_port,
                 stats.tx_total,
                 stats.rx_total,
                 stats.rx_good,
                 stats.rx_bad);
    }

  } else {
    if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
      aim_printf(&uc->pvs, "Invalid dev_port %d \n", dev_port);
      return UCLI_STATUS_E_ARG;
    }
    memset(&stats, 0, sizeof(stats));
    bf_diag_cpu_stats_get(dev_id, dev_port, &stats);
    aim_printf(&uc->pvs,
               "%-8d %-8d %-8d %8d %8d\n",
               dev_port,
               stats.tx_total,
               stats.rx_total,
               stats.rx_good,
               stats.rx_bad);
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__cpu_stats_clear__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t dev_port = 0;
  bool all_ports = false;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc,
      "cpu-stats-clear",
      3,
      "cpu-stats-clear <dev> <type: 0=one port,1=all ports> <dev_port>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  all_ports = strtoul(uc->pargs->args[1], NULL, 0);
  dev_port = strtoul(uc->pargs->args[2], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: CPU stats clear <dev=%d> <all_ports=%d> <dev_port=%d> \n",
             dev_id,
             all_ports,
             dev_port);
  status = bf_diag_cpu_stats_clear(dev_id, dev_port, all_ports);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "CPU stats cleared \n");
  } else {
    aim_printf(&uc->pvs, "CPU stats clear failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__cpu_port_print__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t cpu_port = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "cpu-port-print", 1, "cpu-port-print <dev>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: CPU Port <dev=%d> \n", dev_id);
  status = bf_diag_cpu_port_get(dev_id, &cpu_port);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "CPU port is %d \n", cpu_port);
  } else {
    aim_printf(&uc->pvs, "Failed to get CPU port\n");
  }

  if (DIAG_DEV_INFO(dev_id)->num_active_pipes > DIAG_SUBDEV_PIPE_COUNT) {
    aim_printf(
        &uc->pvs, "Second CPU port is %d \n", DIAG_DEV_INFO(dev_id)->cpu_port2);
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__vlan_create__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int vlan_id = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "vlan-create", 2, "vlan-create <dev> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  vlan_id = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: Vlan create <dev=%d> <vlan_id=%d>\n", dev, vlan_id);

  status = bf_diag_vlan_create(dev, vlan_id);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Vlan create success \n");
  } else if (status == BF_ALREADY_EXISTS) {
    aim_printf(&uc->pvs, "Vlan %d already exists \n", vlan_id);
  } else {
    aim_printf(&uc->pvs, "Vlan create failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__port_vlan_add__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_dev_port_t port = 0;
  int vlan_id = 0, index = 0;
  bf_status_t status = BF_SUCCESS;
  const char *port_range = NULL;
  int port_list[BF_DIAG_MAX_PORTS];
  int num_ports = 0;

  memset(&port_list, 0, sizeof(port_list));

  UCLI_COMMAND_INFO(
      uc, "port-vlan-add", 3, "port-vlan-add <dev> <port-range> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  vlan_id = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Vlan port add <dev=%d> <vlan_id=%d> "
             "<port-range=%s>\n",
             dev,
             vlan_id,
             port_range);

  diag_parse_port_list((char *)port_range, &port_list[0], &num_ports);

  for (index = 0; index < num_ports; index++) {
    port = port_list[index];
    status = bf_diag_port_vlan_add(dev, port, vlan_id);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs, "Vlan port add success for port %d\n", port);
    } else {
      aim_printf(&uc->pvs, "Vlan port add failed for port %d\n", port);
    }
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__port_vlan_del__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_dev_port_t port = 0;
  int vlan_id = 0, index = 0;
  bf_status_t status = BF_SUCCESS;
  const char *port_range = NULL;
  int port_list[BF_DIAG_MAX_PORTS];
  int num_ports = 0;

  memset(&port_list, 0, sizeof(port_list));

  UCLI_COMMAND_INFO(
      uc, "port-vlan-del", 3, "port-vlan-del <dev> <port-range> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  vlan_id = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Vlan port del <dev=%d> <vlan_id=%d> "
             "<port-range=%s>\n",
             dev,
             vlan_id,
             port_range);

  diag_parse_port_list((char *)port_range, &port_list[0], &num_ports);

  for (index = 0; index < num_ports; index++) {
    port = port_list[index];
    status = bf_diag_port_vlan_del(dev, port, vlan_id);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs, "Vlan port del success for port %d\n", port);
    } else {
      aim_printf(&uc->pvs, "Vlan port del failed for port %d\n", port);
    }
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__default_vlan_set__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_dev_port_t port = 0;
  int vlan_id = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(
      uc, "def-vlan-set", 3, "def-vlan-set <dev> <port> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  port = strtoul(uc->pargs->args[1], NULL, 0);
  vlan_id = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Default Vlan set <dev=%d> <port=%d> <vlan-id=%d>\n",
             dev,
             port,
             vlan_id);

  status = bf_diag_port_default_vlan_set(dev, port, vlan_id);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Default Vlan set success for port %d\n", port);
  } else {
    aim_printf(&uc->pvs, "Default vlan set failed for port %d\n", port);
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__default_vlan_reset__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  bf_dev_port_t port = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "def-vlan-reset", 2, "def-vlan-reset <dev> <port>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  port = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  if (!DIAG_DEVPORT_VALID(dev, port)) {
    aim_printf(&uc->pvs, "diag:: Invalid <port=%d> \n", port);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: Default Vlan reset <dev=%d> <port=%d>\n", dev, port);

  status = bf_diag_port_default_vlan_reset(dev, port);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Default Vlan reset success for port %d\n", port);
  } else {
    aim_printf(&uc->pvs, "Default vlan reset failed for port %d\n", port);
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__vlan_destroy__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int vlan_id = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "vlan-destroy", 2, "vlan-destroy <dev> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  vlan_id = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: Vlan destroy <dev=%d> <vlan_id=%d>\n", dev, vlan_id);

  status = bf_diag_vlan_destroy(dev, vlan_id);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Vlan destroy success \n");
  } else {
    aim_printf(&uc->pvs, "Vlan destroy failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__vlan_show__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int vlan_id = 0;
  char resp_str[DIAG_UCLI_BUFFER_SIZE];

  UCLI_COMMAND_INFO(uc, "vlan-show", 2, "vlan-show <dev> <vlan-id>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  vlan_id = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: Vlan show <dev=%d> <vlan_id=%d>\n", dev, vlan_id);

  resp_str[0] = '\0';
  diag_vlan_show(dev, vlan_id, resp_str, DIAG_UCLI_BUFFER_SIZE);
  aim_printf(&uc->pvs, "%s\n", resp_str);

  return 0;
}

static ucli_status_t diag_ucli_ucli__vlan_show_all__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int vlan_id = -1;
  char resp_str[DIAG_UCLI_BUFFER_SIZE];

  UCLI_COMMAND_INFO(uc, "vlan-show-all", 1, "vlan-show-all <dev>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs, "diag:: Vlan show all <dev=%d> \n", dev);

  resp_str[0] = '\0';
  diag_vlan_show(dev, vlan_id, resp_str, DIAG_UCLI_BUFFER_SIZE);
  aim_printf(&uc->pvs, "%s\n", resp_str);

  return 0;
}

static ucli_status_t diag_ucli_ucli__fwd_rule_add__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t src_port = 0, dst_port = 0;
  int priority = 0;
  p4_pd_entry_hdl_t entry_hdl = 0;
  bf_status_t status = BF_SUCCESS;
  uint32_t tcp_s = 0, tcp_e = 0;

  UCLI_COMMAND_INFO(uc,
                    "fwd-rule-add",
                    6,
                    "fwd-rule-add <dev> <ig-port> <eg-port> <tcpDstPort-start>"
                    " <tcpDstPort-end> <priority>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  src_port = strtoul(uc->pargs->args[1], NULL, 0);
  dst_port = strtoul(uc->pargs->args[2], NULL, 0);
  tcp_s = strtoul(uc->pargs->args[3], NULL, 0);
  tcp_e = strtoul(uc->pargs->args[4], NULL, 0);
  priority = strtoul(uc->pargs->args[5], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: Fwd-rule add <dev=%d> <ig-port=%d> <eg-port=%d> "
             "<tcpDstport_start=%d> <tcpDstPort_end=%d> <priority=%d>\n",
             dev_id,
             src_port,
             dst_port,
             tcp_s,
             tcp_e,
             priority);
  status = bf_diag_forwarding_rule_add(
      dev_id, src_port, dst_port, tcp_s, tcp_e, priority, &entry_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Entry added with handle %d \n", entry_hdl);
  } else {
    aim_printf(&uc->pvs, "Failed to add entry, rc=%d \n", status);
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__fwd_rule_del__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  p4_pd_entry_hdl_t entry_hdl = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "fwd-rule-del", 2, "fwd-rule-del <dev> <entry-hdl>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  entry_hdl = strtoul(uc->pargs->args[1], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: Fwd-rule del <dev=%d> <entry-hdl=%d> \n",
             dev_id,
             entry_hdl);
  status = bf_diag_forwarding_rule_del(dev_id, entry_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Entry deleted with handle %d \n", entry_hdl);
  } else {
    aim_printf(&uc->pvs, "Failed to delete entry, rc=%d \n", status);
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__pkt_logs__(ucli_context_t *uc) {
  int level = 0;
  uint32_t val = 0;

  UCLI_COMMAND_INFO(uc, "pkt-logs", 1, "pkt-logs <level: 0=off,1=err,2:all>");

  level = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: pkt-logs <level=%d> \n", level);

  if (level == 0) {
    val = DIAG_PKT_LOG_OFF;
  } else if (level == 1) {
    val = DIAG_PKT_LOG_ERR;
  } else if (level == 2) {
    val = DIAG_PKT_LOG_ALL;
  } else {
    aim_printf(&uc->pvs, "diag:: Invalid <level=%d> \n", level);
    return 0;
  }

  diag_pkt_logs_val_set(val);

  return 0;
}

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
static ucli_status_t diag_ucli_ucli__slt_test_mode__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "slt-test-mode", 1, "slt-test-mode <mode>");

  int mode = 0;
  mode = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: slt-test-mode <mode=%d> \n", mode);

  set_slt_failure_test_mode(mode);
  return 0;
}
#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)

static int diag_check_payload_setting(ucli_context_t *uc,
                                      bf_diag_sess_hdl_t sess_hdl,
                                      bool is_data_pattern,
                                      bool is_fixed_payload,
                                      bool is_fixed_pkt_contents,
                                      bool is_full_pkt) {
  if ((!is_full_pkt) && is_diag_use_packet_full(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "Full packet already fixed on sess %d, set pkt-full to "
                 "random first\n",
                 sess_hdl);
    }
    return -1;
  }

  if ((!is_data_pattern) && is_diag_use_fixed_pattern(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "Data patern already fixed on sess %d, set data-pattern to "
                 "random first\n",
                 sess_hdl);
    }
    return -1;
  }

  if ((!is_fixed_payload) && is_diag_use_fixed_payload(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "Packet payload already fixed on sess %d, set pkt-payload to "
                 "random first\n",
                 sess_hdl);
    }
    return -1;
  }

  if ((!is_fixed_pkt_contents) && is_diag_use_fixed_pkt_contents(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "pkt-contents already fixed on sess %d, set pkt-contents to "
                 "random first\n",
                 sess_hdl);
    }
    return -1;
  }

  return 0;
}

bf_status_t diag_data_pattern_set_helper(bf_diag_sess_hdl_t sess_hdl,
                                         bf_diag_data_pattern_t mode,
                                         uint8_t start_pat,
                                         uint32_t start_pat_len,
                                         uint8_t pat_a,
                                         uint8_t pat_b,
                                         uint32_t pattern_len,
                                         ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;

  if ((mode == BF_DIAG_DATA_PATTERN_FIXED) && (pattern_len == 0)) {
    if (uc) {
      aim_printf(&uc->pvs, "ERROR: Pattern length cannot be zero\n");
    }
    return BF_INVALID_ARG;
  }

  if (!bf_diag_session_valid(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs, "Session %d does not exist\n", sess_hdl);
    }
    return BF_INVALID_ARG;
  }

  if (mode == BF_DIAG_DATA_PATTERN_FIXED) {
    if (diag_check_payload_setting(uc, sess_hdl, true, false, false, false) ==
        -1) {
      return BF_INVALID_ARG;
    }
    status = diag_use_fixed_pattern_set(sess_hdl, true);
    status |= diag_payload_data_patterns_set(
        sess_hdl, start_pat, start_pat_len, pat_a, pat_b, pattern_len);
    if (status == BF_SUCCESS) {
      if (uc) {
        aim_printf(&uc->pvs,
                   "Fixed data pattern set success: 0x%x and 0x%x \n",
                   pat_a,
                   pat_b);
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs,
                   "Fixed data pattern set failed: 0x%x and 0x%x \n",
                   pat_a,
                   pat_b);
      }
    }
  } else {
    status = diag_use_fixed_pattern_set(sess_hdl, false);
    status |= diag_payload_data_patterns_set_def(sess_hdl);
    if (status == BF_SUCCESS) {
      if (uc) {
        aim_printf(&uc->pvs, "Random data pattern set success \n");
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs, "Random data pattern set failed \n");
      }
    }
  }

  return status;
}

static ucli_status_t diag_ucli_ucli__data_pattern__(ucli_context_t *uc) {
  bf_diag_data_pattern_t mode = 0;
  uint8_t pat_a = 0, pat_b = 0, start_pat = 0;
  uint32_t pattern_len = 0, start_pat_len = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "data-pattern",
                    7,
                    "data-pattern <sess-hdl> <mode: 0=random,1=fixed> "
                    "<start-pattern> <start-pattern-len> "
                    "<pattern_a> <pattern_b> <each-pattern-len>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);
  start_pat = strtoul(uc->pargs->args[2], NULL, 0);
  start_pat_len = strtoul(uc->pargs->args[3], NULL, 0);
  pat_a = strtoul(uc->pargs->args[4], NULL, 0);
  pat_b = strtoul(uc->pargs->args[5], NULL, 0);
  pattern_len = strtoul(uc->pargs->args[6], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: data-pattern <sess-hdl=%d> <mode=%d> "
             "<start_pat=0x%x> <start_pat_len=%d> <pat_a=0x%x> "
             "<pat_b=0x%x> <each-pattern_len=%d>\n",
             sess_hdl,
             mode,
             start_pat,
             start_pat_len,
             pat_a,
             pat_b,
             pattern_len);

  diag_data_pattern_set_helper(
      sess_hdl, mode, start_pat, start_pat_len, pat_a, pat_b, pattern_len, uc);

  return 0;
}

bf_status_t diag_packet_payload_set_helper(ucli_context_t *uc,
                                           bf_diag_sess_hdl_t sess_hdl,
                                           bf_diag_packet_payload_t mode,
                                           const char *payload_str,
                                           const char *payload_file_path) {
  bf_status_t status = BF_SUCCESS;
  bool p_valid = false, pfile_valid = false;
  /* One hex byte is a combination of two characters */
  uint32_t payload_str_len = 0;

  if (payload_str) {
    payload_str_len = strlen(payload_str) / 2;
  }

  if (mode == BF_DIAG_PACKET_PAYLOAD_FIXED) {
    if (payload_str_len != 0) {
      if (uc) {
        aim_printf(
            &uc->pvs,
            "diag:: User specified payload valid, payload-len=%d bytes\n ",
            payload_str_len);
      }
      p_valid = true;
    }
    if (access(payload_file_path, F_OK) != -1) {
      if (uc) {
        aim_printf(&uc->pvs, "diag:: payload file valid \n");
      }
      pfile_valid = true;
    }
    if ((!p_valid) && (!pfile_valid)) {
      if (uc) {
        aim_printf(&uc->pvs,
                   "ERROR: payload and payload-file-path are both invalid\n");
      }
      return BF_INVALID_ARG;
    }
    if (p_valid && pfile_valid) {
      if (uc) {
        aim_printf(
            &uc->pvs,
            "ERROR: Either payload or payload-file-path should be valid\n");
      }
      return BF_INVALID_ARG;
    }
  }

  if (!bf_diag_session_valid(sess_hdl)) {
    if (uc) {
      aim_printf(&uc->pvs, "Session %d does not exist\n", sess_hdl);
    }
    return BF_INVALID_ARG;
  }

  if (mode == BF_DIAG_PACKET_PAYLOAD_FIXED) {
    if (diag_check_payload_setting(uc, sess_hdl, false, true, false, false) ==
        -1) {
      return BF_INVALID_ARG;
    }
    status = diag_fixed_payload_set(
        sess_hdl, pfile_valid, payload_str, payload_file_path);
    if (status == BF_SUCCESS) {
      diag_payload_type_set(sess_hdl, mode);
      if (uc) {
        aim_printf(&uc->pvs, "Packet fixed payload set success \n");
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs, "Packet fixed payload set failed \n");
      }
      return BF_INVALID_ARG;
    }
  } else {
    status = diag_fixed_payload_set_def(sess_hdl);
    if (status == BF_SUCCESS) {
      diag_payload_type_set(sess_hdl, mode);
      if (uc) {
        aim_printf(&uc->pvs,
                   "Packet random %spayload set success \n",
                   (mode == BF_DIAG_PACKET_PAYLOAD_RANDOM_FLIP) ? "Flip " : "");
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs,
                   "Packet random %spayload set failed \n",
                   (mode == BF_DIAG_PACKET_PAYLOAD_RANDOM_FLIP) ? "Flip " : "");
      }
      return BF_INVALID_ARG;
    }
  }

  return BF_SUCCESS;
}

static ucli_status_t diag_ucli_ucli__pkt_payload__(ucli_context_t *uc) {
  int mode = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  char *payload_str = NULL;
  char *payload_file_path = NULL;

  UCLI_COMMAND_INFO(
      uc,
      "pkt-payload",
      4,
      "pkt-payload <sess-hdl> <mode: 0=random,1=fixed,2=random-flip> "
      "<\"payload\" (\"\"=None)> <\"payload-file-path\" (\"\"=None)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);
  payload_str = (char *)uc->pargs->args[2];
  payload_file_path = (char *)uc->pargs->args[3];

  aim_printf(&uc->pvs,
             "diag:: pkt-payload <sess-hdl=%d> <mode=%d> <payload=%s> "
             "<payload-file-path=%s> \n",
             sess_hdl,
             mode,
             payload_str,
             payload_file_path);

  diag_packet_payload_set_helper(
      uc, sess_hdl, mode, payload_str, payload_file_path);

  return 0;
}

static ucli_status_t diag_ucli_ucli__pkt_contents__(ucli_context_t *uc) {
  int mode = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc,
                    "pkt-contents",
                    2,
                    "pkt-contents <sess-hdl> <mode: 0=random,1=fixed>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: pkt-contents <sess_hdl=%d> <mode=%d> \n",
             sess_hdl,
             mode);
  if (!bf_diag_session_valid(sess_hdl)) {
    aim_printf(&uc->pvs, "Session %d does not exist\n", sess_hdl);
    return 0;
  }

  if (mode) {
    if (diag_check_payload_setting(uc, sess_hdl, false, false, true, false) ==
        -1) {
      return 0;
    }
    status = diag_use_fixed_pkt_contents_set(sess_hdl, true);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs, "Packet contents (header and payload) is fixed \n");
    } else {
      aim_printf(&uc->pvs, "Packet contents fixed set failed \n");
    }
  } else {
    status = diag_use_fixed_pkt_contents_set(sess_hdl, false);
    if (status == BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Packet contents (header and payload) is random  \n");
    } else {
      aim_printf(&uc->pvs, "Packet contents random set failed \n");
    }
  }

  return 0;
}

bf_status_t diag_packet_full_set_helper(ucli_context_t *uc,
                                        bf_diag_sess_hdl_t sess_hdl,
                                        bf_diag_packet_full_t mode,
                                        const char *pkt_str,
                                        const char *pkt_file_path) {
  bf_status_t status = BF_SUCCESS;
  bool p_valid = false, pfile_valid = false;
  /* One hex byte is a combination of two characters */
  uint32_t pkt_str_len = 0;

  if (pkt_str) {
    pkt_str_len = strlen(pkt_str) / 2;
  }

  if (mode == BF_DIAG_PACKET_FULL_FIXED) {
    if (pkt_str_len != 0) {
      if (uc) {
        aim_printf(&uc->pvs,
                   "diag:: User specified packet valid, packet-len=%d bytes\n ",
                   pkt_str_len);
      }
      p_valid = true;
    }
    if (access(pkt_file_path, F_OK) != -1) {
      if (uc) {
        aim_printf(&uc->pvs, "diag:: packet file valid \n");
      }
      pfile_valid = true;
    }
    if ((!p_valid) && (!pfile_valid)) {
      if (uc) {
        aim_printf(&uc->pvs,
                   "ERROR: packet and packet-file-path are both invalid\n");
      }
      return BF_INVALID_ARG;
    }
    if (p_valid && pfile_valid) {
      if (uc) {
        aim_printf(
            &uc->pvs,
            "ERROR: Either packet or packet-file-path should be valid\n");
      }
      return BF_INVALID_ARG;
    }
  }

  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    if (uc) {
      aim_printf(&uc->pvs, "Session handle %u does not exist\n", sess_hdl);
    }
    return BF_INVALID_ARG;
  }
  if (!diag_test_type_stream(sess_info)) {
    if (uc) {
      aim_printf(
          &uc->pvs,
          "ERROR: Packet full command is only available for stream sessions\n");
    }
    return BF_INVALID_ARG;
  }

  if (mode == BF_DIAG_PACKET_FULL_FIXED) {
    if (diag_check_payload_setting(uc, sess_hdl, false, false, false, true) ==
        -1) {
      return BF_INVALID_ARG;
    }
    status =
        diag_packet_full_set(sess_hdl, pfile_valid, pkt_str, pkt_file_path);
    if (status == BF_SUCCESS) {
      diag_packet_full_type_set(sess_hdl, mode);
      if (uc) {
        aim_printf(&uc->pvs, "Packet fixed set success \n");
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs, "Packet fixed set failed \n");
      }
      return BF_INVALID_ARG;
    }
  } else if (mode == BF_DIAG_PACKET_FULL_RANDOM) {
    status = diag_packet_full_set_def(sess_hdl);
    if (status == BF_SUCCESS) {
      diag_packet_full_type_set(sess_hdl, mode);
      if (uc) {
        aim_printf(&uc->pvs, "Packet random set success \n");
      }
    } else {
      if (uc) {
        aim_printf(&uc->pvs, "Packet random set failed \n");
      }
      return BF_INVALID_ARG;
    }
  } else {
    if (uc) {
      aim_printf(&uc->pvs, "Invalid mode \n");
    }
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

static ucli_status_t diag_ucli_ucli__pkt_full__(ucli_context_t *uc) {
  int mode = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  char *pkt_str = NULL;
  char *pkt_file_path = NULL;

  UCLI_COMMAND_INFO(uc,
                    "pkt-full",
                    4,
                    "pkt-full <sess-hdl> <mode: 0=random,1=fixed> "
                    "<\"pkt\" (\"\"=None)> <\"pkt-file-path\" (\"\"=None)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_str = (char *)uc->pargs->args[2];
  pkt_file_path = (char *)uc->pargs->args[3];

  aim_printf(&uc->pvs,
             "diag:: pkt-full <sess-hdl=%d> <mode=%d> <pkt=%s> "
             "<pkt-file-path=%s> \n",
             sess_hdl,
             mode,
             pkt_str,
             pkt_file_path);

  diag_packet_full_set_helper(uc, sess_hdl, mode, pkt_str, pkt_file_path);

  return 0;
}

static ucli_status_t diag_ucli_ucli__pair_test_setup__(ucli_context_t *uc) {
  int num_ports = 0;
  bf_dev_id_t dev_id = 0;
  int loop_mode = 0;
  const char *port_range = NULL;
  bf_dev_port_t port_arr[BF_DIAG_MAX_PORTS];
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  memset(&port_arr[0], 0, sizeof(port_arr));

  UCLI_COMMAND_INFO(uc,
                    "pair-setup",
                    3,
                    "pair-setup <dev> <port-range> <loop-mode: "
                    "0=none,1=mac,2=phy,3=ext,4=pcs,5=pipe>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  loop_mode = strtoul(uc->pargs->args[2], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  if (loop_mode > DIAG_LOOP_MODE_MAX) {
    aim_printf(&uc->pvs, "diag:: Invalid <loop-mode=%d> \n", loop_mode);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Pair test setup <dev=%d> <port-range=%s>"
             "<loop-mode=%d> \n",
             dev_id,
             port_range,
             loop_mode);

  if (diag_validate_loopback_mode(dev_id, loop_mode) != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "ERROR: loopback mode %d not supported on this chip \n",
               loop_mode);
    return 0;
  }
  if (strncmp(port_range, "all_mesh", strlen("all_mesh")) == 0) {
    diag_get_all_mesh_ports_list(dev_id, &port_arr[0], &num_ports, false);
    diag_add_cpu_port_to_list(dev_id, &port_arr[0], &num_ports, true);
  } else if (strncmp(port_range, "alli", strlen("alli")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, true);
    diag_add_cpu_port_to_list(dev_id, &port_arr[0], &num_ports, true);
  } else if (strncmp(port_range, "all", strlen("all")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else {
    diag_parse_port_list((char *)port_range, &port_arr[0], &num_ports);
  }
  aim_printf(&uc->pvs, "Num ports is %d \n", num_ports);
  aim_printf(&uc->pvs, "Ports -> ");

  for (int i = 0; i < num_ports; i++) {
    if (i != 0) {
      aim_printf(&uc->pvs, ",", port_arr[i]);
    }
    aim_printf(&uc->pvs, "%d", port_arr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  status = bf_diag_loopback_pair_test_setup(
      dev_id, &port_arr[0], num_ports, loop_mode, &sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Pair test setup success \n");
    aim_printf(&uc->pvs, "Pair test Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Pair test setup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__pair_test_start__(ucli_context_t *uc) {
  int pkt_size = 0, num_pkt = 0;
  bf_status_t status = BF_SUCCESS;
  int bidir = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "pair-start",
                    4,
                    "pair-start <sess-hdl(0=default)> <num-pkt> "
                    "<pkt-size(0=random)> "
                    "<bidir-traffic(0:non-bidir, 1:bidir)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkt = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);
  bidir = strtoul(uc->pargs->args[3], NULL, 0);

  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  if (bidir > 1) {
    aim_printf(&uc->pvs, "diag:: Invalid bidir <bidir=%d>\n", bidir);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Pair test start <sess-hdl=%u> <num-pkt=%d> "
             "<pkt-size=%d> <bidir=%d>\n",
             sess_hdl,
             num_pkt,
             pkt_size,
             bidir);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_PAIRED_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_loopback_pair_test_start(sess_hdl, num_pkt, pkt_size, bidir);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Pair test started \n");
  } else {
    aim_printf(&uc->pvs, "Pair test start failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__pair_test_stop__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "pair-stop", 1, "pair-stop <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Pair Test stop <sess-hdl=%u>\n", sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_PAIRED_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_loopback_pair_test_stop(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test stop success \n");
  } else {
    aim_printf(&uc->pvs, "Test stop failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__pair_test_cleanup__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(
      uc, "pair-cleanup", 1, "pair-cleanup <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_PAIRED_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  } else if (!(DIAG_SESS_VALID(sess_hdl))) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Pair Test cleanup <sess-hdl=%d>\n", sess_hdl);

  status = bf_diag_loopback_pair_test_cleanup(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test cleanup success \n");
  } else {
    aim_printf(&uc->pvs, "Test cleanup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__pair_test_status__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_diag_test_status_e test_status;
  bf_diag_loopback_pair_test_stats_t stats;
  uint32_t cnt = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  bf_status_t status = BF_SUCCESS;
  uint64_t total_tx = 0, total_rx = 0;
  uint64_t pkt_id_corrupt_cnt = 0;
  uint32_t detail = 0;

  UCLI_COMMAND_INFO(uc,
                    "pair-status",
                    -1,
                    "pair-status <sess-hdl(0=default)> [<detail(0=ON,1=OFF)>]");

  if ((uc->pargs->count == 0) || (uc->pargs->count > 2)) {
    aim_printf(&uc->pvs,
               "diag:: Invalid cmd, USAGE: pair-status <sess-hdl(0=default)> "
               "[<detail(0=ON,1=OFF)>]\n");
    return 0;
  }

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }
  /* Check if second optonal argument has been given */
  if (uc->pargs->count >= 2) {
    detail = strtoul(uc->pargs->args[1], NULL, 0);
  }

  aim_printf(&uc->pvs,
             "diag:: Pair Test status <sess-hdl=%d><detail=%d>\n",
             sess_hdl,
             detail);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_PAIRED_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }
  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    aim_printf(&uc->pvs,
               "Session handle %u does not exist on dev %d\n",
               sess_hdl,
               dev_id);
    return 0;
  }

  memset(&stats, 0, sizeof(stats));
  test_status = bf_diag_loopback_pair_test_status_get(sess_hdl, &stats);
  diag_print_test_status(test_status, uc);
  if (!detail) {
    aim_printf(&uc->pvs, "Port1 Port2 Tx-total Rx-total Rx-good Rx-bad\n");
  } else {
    aim_printf(&uc->pvs,
               "Port1 Port2 Tx-total Rx-total Rx-good Rx-bad Port1-Tx Port1-Rx "
               "Port2-Tx Port2-Rx\n");
  }
  for (cnt = 0;
       (cnt < stats.num_pairs) && (cnt < BF_DIAG_MAX_LOOPBACK_TEST_PAIRS);
       cnt++) {
    if (!detail) {
      aim_printf(&uc->pvs,
                 "%5d %5d %8u %8u %7u %6u\n",
                 stats.pairs[cnt].port1,
                 stats.pairs[cnt].port2,
                 stats.pairs[cnt].tx_total,
                 stats.pairs[cnt].rx_total,
                 stats.pairs[cnt].rx_good,
                 stats.pairs[cnt].rx_bad);
    } else {
      aim_printf(&uc->pvs,
                 "%5d %5d %8u %8u %7u %6u %8d %8d %8d %8d\n",
                 stats.pairs[cnt].port1,
                 stats.pairs[cnt].port2,
                 stats.pairs[cnt].tx_total,
                 stats.pairs[cnt].rx_total,
                 stats.pairs[cnt].rx_good,
                 stats.pairs[cnt].rx_bad,
                 stats.pairs[cnt].port_stats[0].tx,
                 stats.pairs[cnt].port_stats[0].rx,
                 stats.pairs[cnt].port_stats[1].tx,
                 stats.pairs[cnt].port_stats[1].rx);
    }
    total_tx += stats.pairs[cnt].tx_total;
    total_rx += stats.pairs[cnt].rx_total;
  }
  pkt_id_corrupt_cnt = sess_info->pkt_id_corrupt_cnt;

  aim_printf(&uc->pvs,
             "No of Tx packet completions: %d\n",
             DIAG_TX_PKT_COMPLETIONS(dev_id));
  aim_printf(&uc->pvs,
             "No of bytes which experienced bit flips  : %lu\n",
             stats.total_bytes_with_bit_flip_detected);
  aim_printf(&uc->pvs,
             "No of individual bits that flipped       : %lu\n",
             stats.total_bits_with_bit_flip_detected);
  aim_printf(&uc->pvs,
             "No of 1 -> 0 bit flips are               : %lu\n",
             stats.total_1_to_0_flips);
  aim_printf(&uc->pvs,
             "No of 0 -> 1 bit flips are               : %lu\n",
             stats.total_0_to_1_flips);
  aim_printf(&uc->pvs,
             "No of packets with S_Setup failures      : %lu\n",
             stats.total_weak_suspect_for_setup);
  aim_printf(&uc->pvs,
             "No of packets with SS_Setup failures     : %lu\n",
             stats.total_strong_suspect_for_setup);
  aim_printf(&uc->pvs,
             "No of packets with S_Hold failures       : %lu\n",
             stats.total_weak_suspect_for_hold);
  aim_printf(&uc->pvs,
             "No of packets with SS_Hold failures      : %lu\n",
             stats.total_strong_suspect_for_hold);
  aim_printf(&uc->pvs,
             "No of packets with Unknown failures      : %lu\n",
             stats.total_unknown_failures);
  aim_printf(&uc->pvs,
             "No of packets with Payload_Setup failures: %lu\n",
             stats.total_payload_setup_failures);
  aim_printf(&uc->pvs,
             "No of packets with Mixed failures        : %lu\n",
             stats.total_mixed_failures);

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
  diag_slt_failure_type_e mode = get_slt_failure_test_mode();
  uint8_t num_pkts = 15;
  switch (mode) {
    case DIAG_SLT_FAILURE_TYPE_S_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == num_pkts);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == num_pkts);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_S_HOLD:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == num_pkts);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_HOLD:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == num_pkts);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_UNKNOWN:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == num_pkts);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected <
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == num_pkts);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_MIXED:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == num_pkts);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_NO_FAILURE:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    default:
      break;
  }
#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)
  aim_printf(&uc->pvs, "No of Total TX : %lu\n", total_tx);
  aim_printf(&uc->pvs, "No of Total RX : %lu\n", total_rx);
  aim_printf(&uc->pvs, "No of pktID corruptions : %lu\n", pkt_id_corrupt_cnt);
  aim_printf(
      &uc->pvs,
      "No of missing pkts (TotalTX - TotalRX - pktID_corruptions) : %lu\n",
      total_tx - total_rx - pkt_id_corrupt_cnt);

  return 0;
}

static ucli_status_t diag_ucli_ucli__packet_inject__(ucli_context_t *uc) {
  int num_ports = 0;
  bf_dev_id_t dev_id = 0;
  const char *port_range = NULL;
  bf_dev_port_t port_arr[BF_DIAG_MAX_PORTS];
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pkt = 0, pkt_size = 0;

  memset(&port_arr[0], 0, sizeof(port_arr));

  UCLI_COMMAND_INFO(
      uc, "pkt-inj", 4, "pkt-inj <dev> <port-range> <num-pkt> <pkt-size>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }

  port_range = uc->pargs->args[1];
  num_pkt = strtoul(uc->pargs->args[2], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[3], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: Packet inject <dev=%d> <port-range=%s>"
             "<num-pkt=%d> <pkt-size=%d>\n",
             dev_id,
             port_range,
             num_pkt,
             pkt_size);

  if (strncmp(port_range, "alli", strlen("alli")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, true);
  } else if (strncmp(port_range, "all", strlen("all")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else {
    diag_parse_port_list((char *)port_range, &port_arr[0], &num_ports);
  }
  aim_printf(&uc->pvs, "Num ports is %d \n", num_ports);
  aim_printf(&uc->pvs, "Ports -> ");

  for (int i = 0; i < num_ports; i++) {
    if (i != 0) {
      aim_printf(&uc->pvs, ",", port_arr[i]);
    }
    aim_printf(&uc->pvs, "%d", port_arr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  status = bf_diag_packet_inject_from_cpu(
      dev_id, &port_arr[0], num_ports, num_pkt, pkt_size);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Packet inject success \n");
  } else {
    aim_printf(&uc->pvs, "Packet inject failed \n");
  }

  return 0;
}

/* Is Session valid */
static ucli_status_t diag_ucli_ucli__session_valid__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "sess-valid", 1, "sess-valid <sess-hdl>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: Session validate <sess-hdl=%u> \n", sess_hdl);

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "Invalid device \n");
    return 0;
  }

  if (bf_diag_session_valid(sess_hdl)) {
    aim_printf(&uc->pvs, "Dev %d, Session %u VALID \n", dev_id, sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Dev %d, Session %u INVALID \n", dev_id, sess_hdl);
  }

  return 0;
}

/* Delete all sessions */
static ucli_status_t diag_ucli_ucli__session_del_all__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "sess-del-all", 1, "sess-del-all <dev>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: Session delete all <dev=%d> \n", dev_id);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "Invalid device \n");
    return 0;
  }

  status = bf_diag_session_del_all(dev_id);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Dev %d, Session delete all Success \n", dev_id);
  } else {
    aim_printf(&uc->pvs, "Dev %d, Session delete all Failed \n", dev_id);
  }

  return 0;
}

#define DIAG_SMALL_STR_LEN 50
static ucli_status_t diag_session_info_print(ucli_context_t *uc,
                                             diag_session_info_t *sess_info) {
  int i = 0;
  diag_test_type_e test_type;
  bf_diag_port_lpbk_mode_e loop_mode;
  char test_type_str[DIAG_SMALL_STR_LEN];
  char loop_mode_str[DIAG_SMALL_STR_LEN];

  if ((!uc) || (!sess_info)) {
    return 0;
  }

  memset(test_type_str, 0, sizeof(test_type_str));
  memset(loop_mode_str, 0, sizeof(loop_mode_str));

  test_type = DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type;
  if (test_type == DIAG_TEST_LOOPBACK) {
    strncpy(test_type_str, "Loopback test", DIAG_SMALL_STR_LEN);
  } else if (test_type == DIAG_TEST_SNAKE) {
    strncpy(test_type_str, "Snake test", DIAG_SMALL_STR_LEN);
  } else if (test_type == DIAG_TEST_PAIRED_LOOPBACK) {
    strncpy(test_type_str, "Paired Loopback", DIAG_SMALL_STR_LEN);
  } else if (test_type == DIAG_TEST_STREAM) {
    strncpy(test_type_str, "Stream", DIAG_SMALL_STR_LEN);
  } else if (test_type == DIAG_TEST_NONE) {
    strncpy(test_type_str, "None", DIAG_SMALL_STR_LEN);
  } else {
    strncpy(test_type_str, "Unknown", DIAG_SMALL_STR_LEN);
  }

  loop_mode = DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode;
  if (loop_mode == BF_DIAG_PORT_LPBK_MAC) {
    strncpy(loop_mode_str, "Mac", DIAG_SMALL_STR_LEN);
  } else if (loop_mode == BF_DIAG_PORT_LPBK_PHY) {
    strncpy(loop_mode_str, "Phy", DIAG_SMALL_STR_LEN);
  } else if (loop_mode == BF_DIAG_PORT_LPBK_EXT) {
    strncpy(loop_mode_str, "External", DIAG_SMALL_STR_LEN);
  } else if (loop_mode == BF_DIAG_PORT_LPBK_PCS) {
    strncpy(loop_mode_str, "PCS", DIAG_SMALL_STR_LEN);
  } else if (loop_mode == BF_DIAG_PORT_LPBK_NONE) {
    strncpy(loop_mode_str, "None", DIAG_SMALL_STR_LEN);
  } else {
    strncpy(loop_mode_str, "Unknown", DIAG_SMALL_STR_LEN);
  }

  aim_printf(&uc->pvs, "----- Session %u -----\n", sess_info->sess_hdl);
  aim_printf(
      &uc->pvs, "  Device: %d \n", DIAG_GET_LOOPBACK_PARAMS(sess_info).dev_id);
  aim_printf(&uc->pvs,
             "  Session handle: %d \n",
             DIAG_GET_LOOPBACK_PARAMS(sess_info).sess_hdl);
  aim_printf(&uc->pvs, "  Test Type: %s\n", test_type_str);

  if (test_type == DIAG_TEST_STREAM) {
    aim_printf(
        &uc->pvs, "  Num-pkts %d\n", DIAG_GET_PGEN_PARAMS(sess_info).num_pkts);
    aim_printf(
        &uc->pvs, "  Pkt-size: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).pkt_size);
    aim_printf(
        &uc->pvs, "  Dst-port: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).dst_port);
    aim_printf(
        &uc->pvs, "  Enabled: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).enabled);
    aim_printf(&uc->pvs, "  Pipe: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).pipe);
    aim_printf(
        &uc->pvs, "  App-id: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).app_id);
    aim_printf(
        &uc->pvs, "  Src-port: %d\n", DIAG_GET_PGEN_PARAMS(sess_info).src_port);
    aim_printf(&uc->pvs,
               "  Pktgen-port: %d\n",
               DIAG_GET_PGEN_PARAMS(sess_info).pktgen_port);
    aim_printf(&uc->pvs,
               "  Trigger timer in nsec: %d\n",
               DIAG_GET_PGEN_PARAMS(sess_info).timer_nsec);
    aim_printf(&uc->pvs,
               "  Pkt buffer offset: %d\n",
               DIAG_GET_PGEN_PARAMS(sess_info).pkt_buf_offset);
  } else {
    aim_printf(&uc->pvs, "  Loopback mode: %s\n", loop_mode_str);
    aim_printf(&uc->pvs,
               "  Num ports: %d\n",
               DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports);
    aim_printf(&uc->pvs, "  Port list: ");

    for (i = 0; i < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports; i++) {
      aim_printf(
          &uc->pvs, "%d ", DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[i]);
    }
    aim_printf(&uc->pvs, "\n");
    aim_printf(&uc->pvs,
               "  Bidirectional: %s\n",
               DIAG_GET_LOOPBACK_PARAMS(sess_info).bidir ? "Yes" : "No");
  }
  aim_printf(&uc->pvs,
             "  Is data pattern fixed: %s \n",
             sess_info->use_fixed_pattern ? "Yes" : "No");
  if (sess_info->use_fixed_pattern) {
    aim_printf(&uc->pvs,
               "  Pattern_a 0x%x, Pattern_b 0x%x, Each-Pattern-len %d \n",
               sess_info->data_pattern_a,
               sess_info->data_pattern_b,
               sess_info->pattern_len);
  }
  aim_printf(&uc->pvs,
             "  Is packet contents fixed: %s \n",
             sess_info->use_fixed_pkt_contents ? "Yes" : "No");
  aim_printf(&uc->pvs, "----------------------\n");

  return 0;
}

/* Print all sessions */
static ucli_status_t diag_ucli_ucli__session_print__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  diag_session_info_t *sess_info = NULL;

  UCLI_COMMAND_INFO(uc, "sess-print", 1, "sess-print <sess-hdl (0=all)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Session print <sess-hdl=%u> \n", sess_hdl);

  if (sess_hdl != 0) {
    dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
    if (!DIAG_DEV_VALID(dev_id)) {
      aim_printf(&uc->pvs, "Invalid device \n");
      return 0;
    }

    sess_info = diag_session_info_get(sess_hdl);
    if (!sess_info) {
      aim_printf(
          &uc->pvs, "Session %u not found on dev %d \n", sess_hdl, dev_id);
      return 0;
    }
    /* Print the session */
    diag_session_info_print(uc, sess_info);

  } else {
    bf_map_sts_t msts;
    unsigned long sess_hdl = 0;
    uint32_t num_sessions = 0;
    int i = 0;

    aim_printf(&uc->pvs, "Printing all sessions \n");
    for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
      if (!DIAG_DEV_VALID(dev_id)) {
        continue;
      }
      aim_printf(&uc->pvs, " ---Printing all sessions on dev %d ---\n", dev_id);
      num_sessions = 0;
      /* Go over all items in map */
      msts = bf_map_get_first(
          &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
      while (msts == BF_MAP_OK && sess_info) {
        num_sessions++;
        /* Print the session */
        diag_session_info_print(uc, sess_info);

        msts = bf_map_get_next(
            &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
      }
      aim_printf(&uc->pvs, "\n Total Num of sessions: %u \n", num_sessions);
      aim_printf(&uc->pvs,
                 " Num of session handles allocated: %u \n",
                 DIAG_SESSION_INFO(dev_id).num_hdls_alloced);
      aim_printf(&uc->pvs, " Session Handles List: ");
      for (i = 0; i < DIAG_SESSIONS_MAX_LIMIT; i++) {
        if (DIAG_SESSION_INFO(dev_id).sess_hdls[i] == 1) {
          aim_printf(&uc->pvs, "%d ", i + 1);
        }
      }
      aim_printf(&uc->pvs, "\n");
      aim_printf(
          &uc->pvs, " Max allowed sessions: %d\n", diag_sessions_current_max);
      aim_printf(&uc->pvs, " -------------------------------------\n");
    }
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__lrn_timeout_(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  uint32_t timeout = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "lrn-timeout", 2, "lrn-timeout <dev> <usec>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  timeout = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Learning timeout <dev=%d> <timeout=%d usec>\n",
             dev,
             timeout);

  status = diag_pd_learning_timeout_set(dev, timeout);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Learning timeout set success \n");
  } else {
    aim_printf(&uc->pvs, "Learning timeout set failed \n");
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__multicast_loopback_test_setup__(
    ucli_context_t *uc) {
  int num_ports = 0;
  bf_dev_id_t dev_id = 0;
  int loop_mode = 0;
  const char *port_range = NULL;
  bf_dev_port_t port_arr[BF_DIAG_MAX_PORTS];
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  memset(&port_arr[0], 0, sizeof(port_arr));

  UCLI_COMMAND_INFO(uc,
                    "mlpbk-setup",
                    3,
                    "mlpbk-setup <dev> <ports> <loop-mode: "
                    "0=none,1=mac,2=phy,3=ext,4=pcs,5=pipe>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  port_range = uc->pargs->args[1];
  loop_mode = strtoul(uc->pargs->args[2], NULL, 0);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  if (loop_mode > DIAG_LOOP_MODE_MAX) {
    aim_printf(&uc->pvs, "diag:: Invalid <loop-mode=%d> \n", loop_mode);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Multicast loopback test setup <dev=%d> <port-range=%s>"
             "<loop-mode=%d> \n",
             dev_id,
             port_range,
             loop_mode);

  if (diag_validate_loopback_mode(dev_id, loop_mode) != BF_SUCCESS) {
    aim_printf(&uc->pvs,
               "ERROR: loopback mode %d not supported on this chip \n",
               loop_mode);
    return 0;
  }
  if (strncmp(port_range, "all_mesh", strlen("all_mesh")) == 0) {
    diag_get_all_mesh_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else if (strncmp(port_range, "alli", strlen("alli")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, true);
  } else if (strncmp(port_range, "all", strlen("all")) == 0) {
    diag_get_all_ports_list(dev_id, &port_arr[0], &num_ports, false);
  } else if (strncmp(port_range, "pipe0", strlen("pipe0")) == 0) {
    diag_get_all_pipe_ports_list(dev_id, &port_arr[0], &num_ports, true, 0);
  } else if (strncmp(port_range, "pipe1", strlen("pipe1")) == 0) {
    diag_get_all_pipe_ports_list(dev_id, &port_arr[0], &num_ports, true, 1);
  } else if (strncmp(port_range, "pipe2", strlen("pipe2")) == 0) {
    diag_get_all_pipe_ports_list(dev_id, &port_arr[0], &num_ports, true, 2);
  } else if (strncmp(port_range, "pipe3", strlen("pipe3")) == 0) {
    diag_get_all_pipe_ports_list(dev_id, &port_arr[0], &num_ports, true, 3);
  } else {
    diag_parse_port_list((char *)port_range, &port_arr[0], &num_ports);
  }
  aim_printf(&uc->pvs, "Num ports is %d \n", num_ports);
  aim_printf(&uc->pvs, "Ports -> ");

  for (int i = 0; i < num_ports; i++) {
    if (i != 0) {
      aim_printf(&uc->pvs, ",", port_arr[i]);
    }
    aim_printf(&uc->pvs, "%d", port_arr[i]);
  }
  aim_printf(&uc->pvs, "\n");

  status = bf_diag_multicast_loopback_test_setup(
      dev_id, &port_arr[0], num_ports, loop_mode, &sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Multicast loopback test setup success \n");
    aim_printf(
        &uc->pvs, "Multicast loopback test Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Multicast loopback test setup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__mac_aging_(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  uint32_t ttl = 0;
  bf_status_t status = BF_SUCCESS;

  UCLI_COMMAND_INFO(uc, "mac-aging", 2, "mac-aging <dev> <msec>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  ttl = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: Mac aging ttl <dev=%d> <ttl=%d msec>\n", dev, ttl);

  status = bf_diag_mac_aging_set(dev, ttl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Mac aging ttl set success \n");
  } else {
    aim_printf(&uc->pvs, "Mac aging ttl set failed \n");
  }
  return 0;
}

static ucli_status_t diag_ucli_ucli__multicast_loopback_test_start__(
    ucli_context_t *uc) {
  int pkt_size = 0, num_pkt = 0;
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(
      uc,
      "mlpbk-start",
      3,
      "mlpbk-start <sess-hdl(0=default)> <num-pkt> <pkt-size(0=random)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkt = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: Multicast loopback test start <sess-hdl=%u> <num-pkt=%d> "
             "<pkt-size=%d>\n",
             sess_hdl,
             num_pkt,
             pkt_size);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_MULTICAST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_multicast_loopback_test_start(sess_hdl, num_pkt, pkt_size);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Multicast loopback test started \n");
  } else {
    aim_printf(&uc->pvs, "Multicast loopback test start failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__multicast_loopback_test_stop__(
    ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "mlpbk-stop", 1, "mlpbk-stop <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Multicast loopback Test stop <sess-hdl=%u>\n",
             sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_MULTICAST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }

  status = bf_diag_multicast_loopback_test_stop(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test stop success \n");
  } else {
    aim_printf(&uc->pvs, "Test stop failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__multicast_loopback_test_cleanup__(
    ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(
      uc, "mlpbk-cleanup", 1, "mlpbk-cleanup <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_MULTICAST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  } else if (!(DIAG_SESS_VALID(sess_hdl))) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Multicast loopback Test cleanup <sess-hdl=%d>\n",
             sess_hdl);

  status = bf_diag_multicast_loopback_test_cleanup(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Test cleanup success \n");
  } else {
    aim_printf(&uc->pvs, "Test cleanup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__multicast_loopback_test_status__(
    ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_diag_test_status_e test_status;
  bf_diag_port_stats_t stats;
  bf_diag_sess_hdl_t sess_hdl = 0;
  bf_status_t status = BF_SUCCESS;
  int port_idx = 0;
  bf_dev_port_t port = 0;

  UCLI_COMMAND_INFO(
      uc, "mlpbk-status", 1, "mlpbk-status <sess-hdl(0=default)>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  if (sess_hdl > DIAG_SESS_HANDLE_MAX) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs,
             "diag:: Multicast loopback Test status <sess-hdl=%d>\n",
             sess_hdl);

  if (sess_hdl == 0) {
    status = diag_find_default_session(DIAG_TEST_MULTICAST_LOOPBACK, &sess_hdl);
    if (status != BF_SUCCESS) {
      aim_printf(&uc->pvs,
                 "Failed to find default session. Please ensure only one "
                 "session exists or specify exact session handle\n");
      return 0;
    }
    aim_printf(&uc->pvs, "Using default session-handle %d \n", sess_hdl);
  }
  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT(
        "Session handle %u does not exist on dev %d\n", sess_hdl, dev_id);
    return UCLI_STATUS_E_ARG;
  }

  test_status = bf_diag_multicast_loopback_test_status_get(sess_hdl);
  diag_print_test_status(test_status, uc);

  memset(&stats, 0, sizeof(stats));
  for (port_idx = 0; port_idx < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
       port_idx++) {
    port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[port_idx];
    memset(&stats, 0, sizeof(stats));
    bf_diag_multicast_loopback_test_port_status_get(sess_hdl, port, &stats);
    aim_printf(&uc->pvs, "Port Tx-total Rx-total Rx-good Rx-bad\n");
    aim_printf(&uc->pvs,
               "%4d %8u %8u %7u %6u\n",
               port,
               stats.tx_total,
               stats.rx_total,
               stats.rx_good,
               stats.rx_bad);
  }
  aim_printf(&uc->pvs,
             "No of Tx packet completions: %d\n",
             DIAG_TX_PKT_COMPLETIONS(dev_id));
  aim_printf(&uc->pvs,
             "No of bytes which experienced bit flips  : %lu\n",
             stats.total_bytes_with_bit_flip_detected);
  aim_printf(&uc->pvs,
             "No of individual bits that flipped       : %lu\n",
             stats.total_bits_with_bit_flip_detected);
  aim_printf(&uc->pvs,
             "No of 1 -> 0 bit flips are               : %lu\n",
             stats.total_1_to_0_flips);
  aim_printf(&uc->pvs,
             "No of 0 -> 1 bit flips are               : %lu\n",
             stats.total_0_to_1_flips);
  aim_printf(&uc->pvs,
             "No of packets with S_Setup failures      : %lu\n",
             stats.total_weak_suspect_for_setup);
  aim_printf(&uc->pvs,
             "No of packets with SS_Setup failures     : %lu\n",
             stats.total_strong_suspect_for_setup);
  aim_printf(&uc->pvs,
             "No of packets with S_Hold failures       : %lu\n",
             stats.total_weak_suspect_for_hold);
  aim_printf(&uc->pvs,
             "No of packets with SS_Hold failures      : %lu\n",
             stats.total_strong_suspect_for_hold);
  aim_printf(&uc->pvs,
             "No of packets with Unknown failures      : %lu\n",
             stats.total_unknown_failures);
  aim_printf(&uc->pvs,
             "No of packets with Payload_Setup failures: %lu\n",
             stats.total_payload_setup_failures);
  aim_printf(&uc->pvs,
             "No of packets with Mixed failures        : %lu\n",
             stats.total_mixed_failures);

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
  diag_slt_failure_type_e mode = get_slt_failure_test_mode();
  uint8_t num_pkts = 15;
  switch (mode) {
    case DIAG_SLT_FAILURE_TYPE_S_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == num_pkts);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == num_pkts);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_S_HOLD:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == num_pkts);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_HOLD:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == num_pkts);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_UNKNOWN:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == num_pkts);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected <
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == num_pkts);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_MIXED:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == num_pkts);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    case DIAG_SLT_FAILURE_TYPE_NO_FAILURE:
      DIAG_ASSERT(stats.total_weak_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_setup == 0);
      DIAG_ASSERT(stats.total_weak_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_strong_suspect_for_hold == 0);
      DIAG_ASSERT(stats.total_unknown_failures == 0);
      DIAG_ASSERT(stats.total_payload_setup_failures == 0);
      DIAG_ASSERT(stats.total_mixed_failures == 0);

      DIAG_ASSERT(stats.total_1_to_0_flips == 0);
      DIAG_ASSERT(stats.total_0_to_1_flips == 0);
      DIAG_ASSERT(stats.total_bytes_with_bit_flip_detected ==
                  stats.total_bits_with_bit_flip_detected);
      break;
    default:
      break;
  }
#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)
  return 0;
}

static ucli_status_t diag_ucli_ucli__pktid_cnt_show__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  UCLI_COMMAND_INFO(uc, "pktid-cnt-show", 1, "pktid-cnt-show <dev>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  aim_printf(&uc->pvs, "diag:: pktid-cnt-show <dev=%d>\n", dev);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "Invalid device %d \n", dev);
    return 0;
  }

  aim_printf(&uc->pvs,
             "No of packets with Packet-id corruption : %lu\n",
             DIAG_PKT_ID_CORRUPT_CNT(dev));

  return 0;
}

static ucli_status_t diag_ucli_ucli__pktid_cnt_clear__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  UCLI_COMMAND_INFO(uc, "pktid-cnt-clear", 1, "pktid-cnt-clear <dev>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  aim_printf(&uc->pvs, "diag:: pktid-cnt-clear <dev=%d>\n", dev);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "Invalid device %d \n", dev);
    return 0;
  }

  DIAG_PKT_ID_CORRUPT_CNT(dev) = 0;
  aim_printf(&uc->pvs, "Packet-id corruption count reset to zero \n");

  return 0;
}

static ucli_status_t diag_ucli_ucli__drain_all_pkts__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int mode = 0;

  UCLI_COMMAND_INFO(
      uc, "drain-all-pkts", 2, "drain-all-pkts <dev> <mode: 0=off,1=on>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);
  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs, "diag:: drain-all-pkts <dev=%d> <mode=%d>\n", dev, mode);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "Invalid device %d \n", dev);
    return 0;
  }

  if (mode > 1) {
    aim_printf(&uc->pvs, "Error: Invalid mode %d\n", mode);
    return 0;
  }

  DIAG_DRAIN_FULL_TCP_PORT_RANGE(dev) = ((mode == 0) ? false : true);
  aim_printf(&uc->pvs,
             "Drain all pkts %s \n",
             DIAG_DRAIN_FULL_TCP_PORT_RANGE(dev) ? "Enabled" : "Disabled");

  return 0;
}

static ucli_status_t diag_ucli_ucli__sess_max__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int sessions_max = 0;

  UCLI_COMMAND_INFO(
      uc, "sess-max", 2, "sess-max <dev> <max-value(default=32)>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  sessions_max = strtoul(uc->pargs->args[1], NULL, 0);
  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(
      &uc->pvs, "diag:: sess-max <dev=%d> <max-value=%d>\n", dev, sessions_max);

  diag_sessions_max_set_helper(dev, sessions_max, uc);

  return 0;
}

static ucli_status_t diag_ucli_ucli__pkt_rx_process__(ucli_context_t *uc) {
  bf_dev_id_t dev = 0;
  int mode = 0;

  UCLI_COMMAND_INFO(uc,
                    "pkt-rx-process",
                    2,
                    "pkt-rx-process <dev> <mode: 0=disable,1=enable>");

  dev = strtoul(uc->pargs->args[0], NULL, 0);
  mode = strtoul(uc->pargs->args[1], NULL, 0);
  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }
  aim_printf(&uc->pvs, "diag:: pkt-rx-process <dev=%d> <mode=%d>\n", dev, mode);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "Invalid device %d \n", dev);
    return 0;
  }

  if (mode > 1) {
    aim_printf(&uc->pvs, "Error: Invalid mode %d\n", mode);
    return 0;
  }

  DIAG_PKT_RX_PROCESSING_DISABLED(dev) = ((mode == 0) ? true : false);

  aim_printf(&uc->pvs,
             "Pkt Rx processing %s for dev %d \n",
             DIAG_PKT_RX_PROCESSING_DISABLED(dev) ? "Disabled" : "Enabled",
             dev);
  aim_printf(&uc->pvs, "\n");

  return 0;
}

static ucli_status_t diag_ucli_ucli__profile_show__(ucli_context_t *uc) {
  static const char *profile_name = DIAG_PROFILE;

  UCLI_COMMAND_INFO(uc, "profile-show", 0, "profile-show");

  aim_printf(&uc->pvs, "%s\n", profile_name);
  return 0;
}

static ucli_status_t diag_ucli_ucli__min_pkt_size__(ucli_context_t *uc) {
  int mode = 0;

  UCLI_COMMAND_INFO(
      uc, "min-pkt-size", 1, "min-pkt-size <mode: 0=disable,1=enable>");

  mode = strtoul(uc->pargs->args[0], NULL, 0);
  aim_printf(&uc->pvs, "diag:: min-pkt-size <mode=%d>\n", mode);

  if (mode > 1) {
    aim_printf(&uc->pvs, "Error: Invalid mode %d\n", mode);
    return 0;
  }

  bf_diag_min_packet_size_enable((mode == 0) ? false : true);

  aim_printf(&uc->pvs,
             "Min Pkt size %s \n",
             DIAG_MIN_PKT_SIZE_ENABLED ? "Enabled" : "Disabled");
  aim_printf(&uc->pvs, "\n");

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_setup__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pkts = 0, pkt_size = 0;
  bf_dev_port_t dst_port = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "stream-setup",
                    4,
                    "stream-setup <dev> <num-pkts/usec> "
                    "<pkt-size> <dst-port>");

  dev_id = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkts = strtoul(uc->pargs->args[1], NULL, 0);
  // uint32_t timer_nsec = strtoul(uc->pargs->args[2], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);
  dst_port = strtoul(uc->pargs->args[3], NULL, 0);

  if (!DIAG_DEV_VALID(dev_id)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev_id);
    return 0;
  }
  aim_printf(&uc->pvs,
             "diag:: Stream setup <dev=%d> "
             "<num-pkts/usec=%d> <pkt-size=%d> "
             "<dst-port=%d>\n",
             dev_id,
             num_pkts,
             pkt_size,
             dst_port);

  status =
      bf_diag_stream_setup(dev_id, num_pkts, pkt_size, dst_port, &sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Stream setup success \n");
    aim_printf(&uc->pvs, "Stream Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Stream setup failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_adjust__(ucli_context_t *uc) {
  bf_dev_id_t dev_id = 0;
  bf_status_t status = BF_SUCCESS;
  uint32_t num_pkts = 0, pkt_size = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc,
                    "stream-adjust",
                    3,
                    "stream-adjust <sess-hdl> <num-pkts/usec> "
                    "<pkt-size>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);
  num_pkts = strtoul(uc->pargs->args[1], NULL, 0);
  pkt_size = strtoul(uc->pargs->args[2], NULL, 0);

  aim_printf(&uc->pvs,
             "diag:: Stream adjust "
             "<num-pkts/nsec=%d> <pkt-size=%d> \n",
             num_pkts,
             pkt_size);

  status = bf_diag_stream_adjust(sess_hdl, num_pkts, pkt_size);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Strea adjust success \n");
    aim_printf(&uc->pvs, "Stream Session Handle: %u \n", sess_hdl);
  } else {
    aim_printf(&uc->pvs, "Stream adjust failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_start__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "stream-start", 1, "stream-start <sess-hdl>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: Stream start <sess-hdl=%d> \n", sess_hdl);

  status = bf_diag_stream_start(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Stream start success \n");
  } else {
    aim_printf(&uc->pvs, "Stream start failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_stop__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "stream-stop", 1, "stream-stop <sess-hdl>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: Stream stop <sess-hdl=%d> \n", sess_hdl);

  status = bf_diag_stream_stop(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Stream stop success \n");
  } else {
    aim_printf(&uc->pvs, "Stream stop failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_counter__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;
  uint64_t counter = 0;

  UCLI_COMMAND_INFO(uc, "stream-counter", 1, "stream-counter <sess-hdl>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  aim_printf(&uc->pvs, "diag:: Stream counter get <sess-hdl=%d> \n", sess_hdl);

  status = bf_diag_stream_counter_get(sess_hdl, &counter);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Stream counter value: %lu \n", counter);
  } else {
    aim_printf(&uc->pvs, "Stream counter get failed \n");
  }

  return 0;
}

static ucli_status_t diag_ucli_ucli__stream_cleanup__(ucli_context_t *uc) {
  bf_status_t status = BF_SUCCESS;
  bf_diag_sess_hdl_t sess_hdl = 0;

  UCLI_COMMAND_INFO(uc, "stream-cleanup", 1, "stream-cleanup <sess-hdl>");

  sess_hdl = strtoul(uc->pargs->args[0], NULL, 0);

  if (!(DIAG_SESS_VALID(sess_hdl))) {
    aim_printf(
        &uc->pvs, "diag:: Invalid session-hdl <sess-hdl=%d>\n", sess_hdl);
    return 0;
  }

  aim_printf(&uc->pvs, "diag:: Stream cleanup <sess-hdl=%d> \n", sess_hdl);

  status = bf_diag_stream_cleanup(sess_hdl);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "Stream cleanup success \n");
  } else {
    aim_printf(&uc->pvs, "Stream cleanup failed \n");
  }

  return 0;
}
#ifdef DIAG_PHV_MOCHA_DARK
/* A uCLI command to invoke the bf_diag_gfm_pattern API to use for testing GFM
 * on TF2.  A snake or lpbk test should be running and the data-pattern command
 * must be used to set the payload to ensure all PHVs have either all bits set
 * or all bits clear.  While the traffic is looping, this command may be given
 * to cycle the various test patterns through the GFM.  Any errors will result
 * in packet loss.  Note that the mode option should be either 0 or 1; zero is
 * to be used when the PHVs have all bits clear and one when the PHVs have all
 * bits set. */
static ucli_status_t diag_ucli_ucli__gfm_patterns__(ucli_context_t *uc) {
  UCLI_COMMAND_INFO(uc, "gfm-pattern", 2, "gfm-pattern <dev_id> <mode>");

  bf_dev_id_t dev = strtoul(uc->pargs->args[0], NULL, 0);
  int mode = strtoul(uc->pargs->args[1], NULL, 0);

  if (!DIAG_DEV_VALID(dev)) {
    aim_printf(&uc->pvs, "diag:: Invalid <dev=%d> \n", dev);
    return 0;
  }

  aim_printf(
      &uc->pvs, "diag:: GFM pattern test <dev=%d> <mode=%d>\n", dev, mode);

  bf_status_t status = bf_diag_gfm_pattern(dev, mode);
  if (status == BF_SUCCESS) {
    aim_printf(&uc->pvs, "GFM pattern test applied, status success\n");
  } else {
    aim_printf(
        &uc->pvs, "GFM pattern test applied, status %s\n", bf_err_str(status));
  }

  aim_printf(&uc->pvs, "GFM pattern test DONE\n");
  return UCLI_STATUS_OK;
}
#endif

/* <auto.ucli.handlers.start> */
/* <auto.ucli.handlers.end> */
static ucli_command_handler_f diag_ucli_ucli_handlers__[] = {
    diag_ucli_ucli__loopback_test_setup__,
    diag_ucli_ucli__loopback_test_start__,
    diag_ucli_ucli__loopback_test_stop__,
    diag_ucli_ucli__loopback_test_status__,
    diag_ucli_ucli__loopback_test_cleanup__,
    diag_ucli_ucli__snake_test_setup__,
    diag_ucli_ucli__snake_test_start__,
    diag_ucli_ucli__snake_test_stop__,
    diag_ucli_ucli__snake_test_status__,
    diag_ucli_ucli__snake_test_cleanup__,
    diag_ucli_ucli__pair_test_setup__,
    diag_ucli_ucli__pair_test_start__,
    diag_ucli_ucli__pair_test_stop__,
    diag_ucli_ucli__pair_test_status__,
    diag_ucli_ucli__pair_test_cleanup__,
    diag_ucli_ucli__multicast_loopback_test_setup__,
    diag_ucli_ucli__multicast_loopback_test_start__,
    diag_ucli_ucli__multicast_loopback_test_stop__,
    diag_ucli_ucli__multicast_loopback_test_status__,
    diag_ucli_ucli__multicast_loopback_test_cleanup__,
    diag_ucli_ucli__session_valid__,
    diag_ucli_ucli__session_del_all__,
    diag_ucli_ucli__session_print__,
    diag_ucli_ucli__set_loopback_mode__,
    diag_ucli_ucli__fwd_rule_add__,
    diag_ucli_ucli__fwd_rule_del__,
    diag_ucli_ucli__cpu_port_print__,
    diag_ucli_ucli__cpu_stats_show__,
    diag_ucli_ucli__cpu_stats_clear__,
    diag_ucli_ucli__packet_inject__,
    diag_ucli_ucli__vlan_create__,
    diag_ucli_ucli__port_vlan_add__,
    diag_ucli_ucli__port_vlan_del__,
    diag_ucli_ucli__default_vlan_set__,
    diag_ucli_ucli__default_vlan_reset__,
    diag_ucli_ucli__vlan_destroy__,
    diag_ucli_ucli__vlan_show__,
    diag_ucli_ucli__vlan_show_all__,
    diag_ucli_ucli__pkt_logs__,
    diag_ucli_ucli__data_pattern__,
    diag_ucli_ucli__pkt_payload__,
    diag_ucli_ucli__pkt_contents__,
    diag_ucli_ucli__pkt_full__,
    diag_ucli_ucli__lrn_timeout_,
    diag_ucli_ucli__mac_aging_,
    diag_ucli_ucli__pktid_cnt_show__,
    diag_ucli_ucli__pktid_cnt_clear__,
    diag_ucli_ucli__drain_all_pkts__,
    diag_ucli_ucli__sess_max__,
    diag_ucli_ucli__pkt_rx_process__,
    diag_ucli_ucli__min_pkt_size__,
    diag_ucli_ucli__profile_show__,
    diag_ucli_ucli__stream_setup__,
    diag_ucli_ucli__stream_start__,
    diag_ucli_ucli__stream_adjust__,
    diag_ucli_ucli__stream_stop__,
    diag_ucli_ucli__stream_counter__,
    diag_ucli_ucli__stream_cleanup__,
#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
    diag_ucli_ucli__slt_test_mode__,
#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)

#ifdef DIAG_PHV_MOCHA_DARK
    diag_ucli_ucli__gfm_patterns__,
#endif
    NULL};

static ucli_module_t diag_ucli_module__ = {
    "diags_ucli",
    NULL,
    diag_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *diag_ucli_node_create(void) {
  ucli_node_t *n;
  ucli_module_init(&diag_ucli_module__);
  n = ucli_node_create("diags", NULL, &diag_ucli_module__);
  ucli_node_subnode_add(n, ucli_module_log_node_create("diags"));
  return n;
}

void diag_register_ucli_node() {
  ucli_node_t *ucli_node = diag_ucli_node_create();
  diag_ucli_node = ucli_node;
  bf_drv_shell_register_ucli(ucli_node);
}

void diag_unregister_ucli_node() {
  bf_drv_shell_unregister_ucli(diag_ucli_node);
}
