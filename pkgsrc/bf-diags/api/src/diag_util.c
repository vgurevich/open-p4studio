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
 * @file diag_util.c
 * @date
 *
 * Contains implementation of diag utilities
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
#include "diag_util.h"
#include "diag_pd.h"
#include "diag_common.h"
#include "diag_vlan.h"
#include "bf_diag_ver.h"

/* Get number of active pipes */
bf_status_t diag_get_num_active_pipes_and_cpu_port(bf_dev_id_t dev_id,
                                                   int *ret_num_pipes,
                                                   int *cpu_port,
                                                   int *cpu_port2,
                                                   int *eth_cpu_port) {
  uint32_t num_pipes = 0;
  bf_status_t status = 0;

  status = lld_sku_get_num_active_pipes(dev_id, &num_pipes);
  *ret_num_pipes = num_pipes;

  *cpu_port = bf_pcie_cpu_port_get(dev_id);
  if (*cpu_port == -1) {
    return BF_INVALID_ARG;
  }
  /* Get other PCIe port for 2 die */
  if (num_pipes > DIAG_SUBDEV_PIPE_COUNT) {
    *cpu_port2 = bf_pcie_cpu_port2_get(dev_id);
    if (*cpu_port2 == -1) {
      return BF_INVALID_ARG;
    }
  }

  *eth_cpu_port = bf_eth_cpu_port_get(dev_id);
  if (*eth_cpu_port == -1) {
    return BF_INVALID_ARG;
  }
  // Workaround for tofino2/tofino3 eth CPU port.
  // API returns BF_TOF2_ETH_CPU_PORT=2
  // We want to use 4 which is 33/2 when on hw and stick with 2 for model
  if (diag_is_chip_family_tofino2(dev_id) ||
      diag_is_chip_family_tofino3(dev_id)) {
    if (!(DIAG_DEV_INFO(dev_id)->is_sw_model)) {
      *eth_cpu_port += 2;
    }
  }

  return status;
}

/* Get part revision */
bf_status_t diag_get_part_revision(bf_dev_id_t dev_id,
                                   bf_sku_chip_part_rev_t *rev) {
  return lld_sku_get_chip_part_revision_number(dev_id, rev);
}

/* Diag get port info */
diag_port_t *diag_get_port_info(bf_dev_id_t dev_id, int port) {
  diag_port_t *port_p = NULL;
  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, port)) {
    return NULL;
  }
  if (diag_devport_is_valid(dev_id, port) != BF_SUCCESS) {
    /* Drivers does not know about cpu pcie port */
    if (DIAG_DEV_INFO(dev_id)->cpu_port != port) {
      return NULL;
    }
  }
  port_p = &(DIAG_DEV_INFO(dev_id)->ports[port]);
  return port_p;
}

/* Get BF loopback mode from diag mode */
bf_loopback_mode_e diag_get_bf_loop_mode(
    bf_diag_port_lpbk_mode_e diag_loop_mode) {
  bf_loopback_mode_e mode = BF_LPBK_NONE;

  switch (diag_loop_mode) {
    case BF_DIAG_PORT_LPBK_PHY:
      mode = BF_LPBK_SERDES_NEAR;
      break;
    case BF_DIAG_PORT_LPBK_MAC:
      mode = BF_LPBK_MAC_NEAR;
      break;
    case BF_DIAG_PORT_LPBK_PCS:
      mode = BF_LPBK_PCS_NEAR;
      break;
    case BF_DIAG_PORT_LPBK_EXT:
      mode = BF_LPBK_NONE;
      break;
    case BF_DIAG_PORT_LPBK_PIPE:
      mode = BF_LPBK_PIPE;
      break;
    case BF_DIAG_PORT_LPBK_NONE:
      mode = BF_LPBK_NONE;
      break;
    default:
      mode = BF_LPBK_NONE;
      break;
  }
  return mode;
}

/* Get Diag loopback mode from BF mode */
bf_diag_port_lpbk_mode_e diag_get_loop_mode(bf_loopback_mode_e loop_mode) {
  bf_diag_port_lpbk_mode_e mode = BF_DIAG_PORT_LPBK_NONE;

  switch (loop_mode) {
    case BF_LPBK_SERDES_NEAR:
    case BF_LPBK_SERDES_FAR:
      mode = BF_DIAG_PORT_LPBK_PHY;
      break;
    case BF_LPBK_MAC_NEAR:
    case BF_LPBK_MAC_FAR:
      mode = BF_DIAG_PORT_LPBK_MAC;
      break;
    case BF_LPBK_PCS_NEAR:
      mode = BF_DIAG_PORT_LPBK_PCS;
      break;
    case BF_LPBK_NONE:
      mode = BF_DIAG_PORT_LPBK_NONE;
      break;
    default:
      mode = BF_DIAG_PORT_LPBK_NONE;
      break;
  }
  return mode;
}

/* Vlan show helper */
bf_status_t diag_vlan_show_helper(diag_vlan_t *vlan_p,
                                  int vlan_id,
                                  char *resp_str,
                                  int max_str_len,
                                  int *curr_len) {
  int c_len = *curr_len;
  bf_dev_port_t port = 0;

  if (!vlan_p) {
    return BF_INVALID_ARG;
  }
  c_len += snprintf(resp_str + c_len,
                    (c_len < max_str_len) ? (max_str_len - c_len - 1) : 0,
                    "%-7d",
                    vlan_id);

  for (port = 0; port < BF_DIAG_MAX_PORTS; port++) {
    if ((vlan_p->tagged[port].valid) || (vlan_p->untagged[port].valid)) {
      c_len += snprintf(resp_str + c_len,
                        (c_len < max_str_len) ? (max_str_len - c_len - 1) : 0,
                        "%d(%s) ",
                        port,
                        vlan_p->tagged[port].valid ? "T" : "U");
    }
  }
  c_len += snprintf(resp_str + c_len,
                    (c_len < max_str_len) ? (max_str_len - c_len - 1) : 0,
                    "\n");

  *curr_len = c_len;
  return BF_SUCCESS;
}

/* Show vlan */
bf_status_t diag_vlan_show(bf_dev_id_t dev_id,
                           int input_vlan_id,
                           char *resp_str,
                           int max_str_len) {
  diag_vlan_t *vlan_p = NULL;
  int vlan_id = 0;
  int c_len = strlen(resp_str);

  (void)dev_id;

  c_len += snprintf(resp_str + c_len,
                    (c_len < max_str_len) ? (max_str_len - c_len - 1) : 0,
                    "Vlan   Ports        \n");
  c_len += snprintf(
      resp_str + c_len,
      (c_len < max_str_len) ? (max_str_len - c_len - 1) : 0,
      "------------------------------------------------------------\n");
  /* Dump all vlans */
  if (input_vlan_id != -1) {
    vlan_p = diag_int_get_vlan_info(dev_id, input_vlan_id);
    if (vlan_p) {
      diag_vlan_show_helper(
          vlan_p, input_vlan_id, resp_str, max_str_len, &c_len);
    } else {
      DIAG_PRINT("Vlan %d does not exist \n", input_vlan_id);
    }
  } else {
    for (vlan_id = 0; vlan_id < BF_DIAG_MAX_VLANS; vlan_id++) {
      vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
      if (!vlan_p) {
        continue;
      }
      diag_vlan_show_helper(vlan_p, vlan_id, resp_str, max_str_len, &c_len);
    }
  }

  return BF_SUCCESS;
}

bf_status_t diag_insert_pkt_size(bf_diag_sess_hdl_t sess_hdl,
                                 uint32_t pkt_size) {
  uint32_t count = 0;
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }

  /* Store special value if user requested random pkts */
  if (pkt_size == 0) {
    pkt_size = DIAG_RANDOM_PKT_SIZE_VAL;
  }

  for (count = 0; count < DIAG_TEST_MAX_PKT_SIZES; count++) {
    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] == pkt_size) {
      return BF_SUCCESS;
    }
    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] == 0) {
      DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] = pkt_size;
      return BF_SUCCESS;
    }
  }
  printf("Failed to add more pkt sizes, max allowed %d \n",
         DIAG_TEST_MAX_PKT_SIZES);

  return BF_INVALID_ARG;
}

bool diag_is_pkt_size_expected(bf_diag_sess_hdl_t sess_hdl, uint32_t pkt_size) {
  uint32_t count = 0;
  diag_session_info_t *sess_info = NULL;

  if (pkt_size == 0) {
    return false;
  }

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }

  /* Check for valid pkt sizes in this session */
  for (count = 0; count < DIAG_TEST_MAX_PKT_SIZES; count++) {
    if ((DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] ==
         pkt_size) ||
        (DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] ==
         DIAG_RANDOM_PKT_SIZE_VAL)) {
      return true;
    }
  }

  return false;
}

/* Any valid pkt size that has been sent */
bool diag_any_pkt_size_sent_valid(const diag_session_info_t *sess_info) {
  uint32_t count = 0;

  if (!sess_info) {
    return false;
  }

  /* Check for valid pkt sizes in this session */
  for (count = 0; count < DIAG_TEST_MAX_PKT_SIZES; count++) {
    if (DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_sizes_sent[count] != 0) {
      return true;
    }
  }

  return false;
}

bool diag_is_pkt_expected_on_port(bf_diag_sess_hdl_t sess_hdl,
                                  bf_dev_port_t port,
                                  uint32_t tcp_dst_port) {
  int count = 0;
  diag_session_info_t *sess_info = NULL;
  int num_ports = 0;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }

  num_ports = DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports;
  /* Check for valid ports in this session */
  for (count = 0; count < num_ports; count++) {
    /* Is the ingress port in our list of valid ports */
    if (port == DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[count]) {
      /* Do some extra checks for each test */
      if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_LOOPBACK) {
        /* For ext loopback, we do not know where the pkt will come back on */
        if (DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode ==
            BF_DIAG_PORT_LPBK_EXT) {
          return true;
        }
        /* Is the recvd port in tcp range */
        if ((tcp_dst_port >=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl)) &&
            (tcp_dst_port <=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_END(port, sess_hdl))) {
          return true;
        }
        DIAG_PRINT(
            "Src Port Mismatch, ingress_port %d, tcp_dst_port %d, "
            " expected rcv tcp_dst_port (%d, %d) \n",
            port,
            tcp_dst_port,
            DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl),
            DIAG_PORT_TCP_DSTPORT_END(port, sess_hdl));
        return false;
      } else if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type ==
                 DIAG_TEST_SNAKE) {
        /* We do not encode the port for snake test, skip check */
        return true;
      } else if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type ==
                 DIAG_TEST_PAIRED_LOOPBACK) {
        /* Is the recvd port in tcp range */
        if ((tcp_dst_port >=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl)) &&
            (tcp_dst_port <=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_END(port, sess_hdl))) {
          return true;
        }
        /* The pkt may come back on the port pair for external or no loopback */
        bf_dev_port_t pair_port = 0;
        if ((count % 2) == 0) {
          pair_port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[count + 1];
        } else {
          pair_port = DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[count - 1];
        }
        /* Is the recvd port in tcp range */
        if ((tcp_dst_port >=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_START(pair_port, sess_hdl)) &&
            (tcp_dst_port <=
             (uint32_t)DIAG_PORT_TCP_DSTPORT_END(pair_port, sess_hdl))) {
          return true;
        }
        DIAG_PRINT(
            "Src Port Mismatch, ingress_port %d, tcp_dst_port %d, "
            " expected rcv tcp_dst_port (%d, %d) \n",
            port,
            tcp_dst_port,
            DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl),
            DIAG_PORT_TCP_DSTPORT_END(port, sess_hdl));
        return false;
      } else {
        return true;
      }
    }
  }

  return false;
}

/* Save the loopback test params */
bf_status_t diag_save_loopback_test_params(bf_dev_id_t dev_id,
                                           bf_diag_sess_hdl_t sess_hdl,
                                           bf_dev_port_t *port_arr,
                                           int num_ports,
                                           bf_diag_port_lpbk_mode_e loop_mode,
                                           diag_test_type_e test_type) {
  diag_session_info_t *sess_info = NULL;
  bf_status_t status = BF_SUCCESS;

  sess_info = DIAG_CALLOC(1, sizeof(diag_session_info_t));
  if (!sess_info) {
    return BF_NO_SYS_RESOURCES;
  }
  sess_info->sess_hdl = sess_hdl;
  sess_info->dev_id = dev_id;
  sess_info->use_fixed_pattern = false;
  sess_info->use_fixed_pkt_contents = false;
  sess_info->total_bytes_with_bit_flip_detected = 0;
  sess_info->total_bits_with_bit_flip_detected = 0;
  sess_info->total_1_to_0_flips = 0;
  sess_info->total_0_to_1_flips = 0;
  sess_info->total_weak_suspect_for_setup = 0;
  sess_info->total_strong_suspect_for_setup = 0;
  sess_info->total_weak_suspect_for_hold = 0;
  sess_info->total_strong_suspect_for_hold = 0;
  sess_info->total_unknown_failures = 0;
  sess_info->total_payload_setup_failures = 0;
  sess_info->total_mixed_failures = 0;
  DIAG_MUTEX_INIT(&sess_info->session_mtx);

  DIAG_GET_LOOPBACK_PARAMS(sess_info).valid = true;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).dev_id = dev_id;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).sess_hdl = sess_hdl;
  if (port_arr) {
    memcpy(&(DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[0]),
           port_arr,
           sizeof(bf_dev_port_t) * num_ports);
  }
  DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports = num_ports;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).loop_mode = loop_mode;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type = test_type;

  status = diag_session_info_add(sess_hdl, sess_info);
  if (status != BF_SUCCESS) {
    DIAG_FREE(sess_info);
    return status;
  }

  return BF_SUCCESS;
}

/* Save the runtime loopback test params */
bf_status_t diag_save_runtime_loopback_test_params(bf_dev_id_t dev_id,
                                                   bf_diag_sess_hdl_t sess_hdl,
                                                   uint32_t pkt_size,
                                                   uint32_t num_packet,
                                                   uint32_t tcp_dstPort_start,
                                                   uint32_t tcp_dstPort_end,
                                                   bool bidir) {
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  if (dev_id != DIAG_GET_LOOPBACK_PARAMS(sess_info).dev_id) {
    return BF_INVALID_ARG;
  }

  DIAG_GET_LOOPBACK_PARAMS(sess_info).num_packet = num_packet;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).pkt_size = pkt_size;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).tcp_dstPort_start = tcp_dstPort_start;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).tcp_dstPort_end = tcp_dstPort_end;
  DIAG_GET_LOOPBACK_PARAMS(sess_info).bidir = bidir;
  /* Remember the fact that bidir traffic was run at least once */
  if (bidir) {
    DIAG_GET_LOOPBACK_PARAMS(sess_info).bidir_sess_en = true;
  }

  return diag_insert_pkt_size(sess_hdl, pkt_size);
}

bf_status_t diag_update_vlan_flood_list(bf_dev_id_t dev_id, int vlan_id) {
  diag_vlan_t *vlan_p;
  bf_mc_port_map_t port_map;
  bf_mc_lag_map_t lag_map;

  vlan_p = diag_int_get_vlan_info(dev_id, vlan_id);
  if (!vlan_p) {
    return BF_INVALID_ARG;
  }

  /* Update the mc flood list */
  memset(&port_map, 0, sizeof(port_map));
  memset(&lag_map, 0, sizeof(lag_map));
  /* Get the port map */
  diag_int_get_vlan_port_bitmap(dev_id, vlan_id, &port_map[0], &lag_map[0]);
  /* Update case if vlan already exists */
  if (vlan_p->mc_node_hdl) {
    return diag_pd_upd_vlan_flood_ports(dev_id,
                                        vlan_id,
                                        vlan_p->mc_index,
                                        &port_map[0],
                                        &lag_map[0],
                                        vlan_p->mc_node_hdl);
  } else {
    return diag_pd_add_vlan_flood_ports(dev_id,
                                        vlan_id,
                                        vlan_p->mc_index,
                                        vlan_p->rid,
                                        &port_map[0],
                                        &lag_map[0],
                                        &(vlan_p->mc_grp_hdl),
                                        &(vlan_p->mc_node_hdl));
  }

  return BF_SUCCESS;
}

/* Cleanup loopback or snake test */
bf_status_t diag_test_cleanup(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }

  /* Cleanup old command if it exists */
  if (!DIAG_GET_LOOPBACK_PARAMS(sess_info).valid) {
    return BF_SUCCESS;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_LOOPBACK) {
    return bf_diag_loopback_test_cleanup(sess_hdl);
  } else if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_SNAKE) {
    return bf_diag_loopback_snake_test_cleanup(sess_hdl);
  } else if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type ==
             DIAG_TEST_PAIRED_LOOPBACK) {
    return bf_diag_loopback_pair_test_cleanup(sess_hdl);
  }

  return BF_SUCCESS;
}

/* Get cpu pkt stats */
bf_status_t diag_cpu_port_stats_get(bf_dev_id_t dev_id,
                                    bf_dev_port_t port,
                                    bf_diag_sess_hdl_t sess_hdl,
                                    bf_diag_port_stats_t *stats) {
  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, port)) {
    return BF_INVALID_ARG;
  }
  stats->tx_total = DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).tx_total;
  stats->rx_total = DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_total;
  stats->rx_good = DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_good;
  stats->rx_bad = DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_bad;
  stats->rx_origin = DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_origin;

  return BF_SUCCESS;
}

/* Get cpu pkt stats for all sessions */
bf_status_t diag_cpu_port_stats_all_sessions_get(bf_dev_id_t dev_id,
                                                 bf_dev_port_t port,
                                                 bf_diag_port_stats_t *stats) {
  bf_diag_sess_hdl_t sess_hdl = 0;
  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, port)) {
    return BF_INVALID_ARG;
  }

  stats->tx_total = 0;
  stats->rx_total = 0;
  stats->rx_good = 0;
  stats->rx_bad = 0;

  for (sess_hdl = 0; sess_hdl < DIAG_SESSIONS_MAX_LIMIT; sess_hdl++) {
    stats->tx_total += DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).tx_total;
    stats->rx_total += DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_total;
    stats->rx_good += DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_good;
    stats->rx_bad += DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).rx_bad;
  }

  return BF_SUCCESS;
}

/* Clear cpu pkt stats */
bf_status_t diag_cpu_port_stats_clear(bf_dev_id_t dev_id,
                                      bf_dev_port_t dev_port,
                                      bf_diag_sess_hdl_t sess_hdl,
                                      bool all_ports) {
  if (all_ports) {
    dev_port = 0;  // assign valid port number for port validate check to pass
  }
  /* Validate port */
  if (!DIAG_DEVPORT_VALID(dev_id, dev_port)) {
    return BF_INVALID_ARG;
  }
  if (sess_hdl >= DIAG_SESSIONS_MAX_LIMIT) {
    return BF_INVALID_ARG;
  }

  if (!all_ports) {
    memset(&DIAG_GET_CPU_STATS(dev_id, dev_port, sess_hdl),
           0,
           sizeof(bf_diag_port_stats_t));
  } else {
    memset(&DIAG_GET_CPU_STATS(dev_id, 0, 0),
           0,
           sizeof(bf_diag_port_stats_t) * BF_DIAG_MAX_PORTS *
               DIAG_SESSIONS_MAX_LIMIT);
  }

  return BF_SUCCESS;
}

bf_status_t diag_devport_is_valid(bf_dev_id_t dev_id, bf_dev_port_t dev_port) {
  return bf_port_is_valid(dev_id, dev_port);
}

const char *bf_diag_get_version() { return BF_DIAG_VER; }

const char *bf_diag_get_internal_version() { return BF_DIAG_INTERNAL_VER; }

diag_session_info_t *diag_session_info_get(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = NULL;
  bf_map_sts_t msts;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_DIAG_TEST_STATUS_UNKNOWN;
  }

  msts = bf_map_get(
      &DIAG_SESSION_INFO(dev_id).sess_map, sess_hdl, (void **)&sess_info);

  if (msts != BF_MAP_OK) {
    return NULL;
  }

  return sess_info;
}

bf_status_t diag_session_info_add(bf_diag_sess_hdl_t sess_hdl,
                                  diag_session_info_t *sess_info) {
  bf_map_sts_t msts;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (diag_session_info_get(sess_hdl)) {
    DIAG_PRINT("Session %u already exists \n", sess_hdl);
    return BF_ALREADY_EXISTS;
  }
  msts = bf_map_add(
      &DIAG_SESSION_INFO(dev_id).sess_map, sess_hdl, (void *)sess_info);

  if (msts != BF_MAP_OK) {
    DIAG_PRINT("Session %u add failed \n", sess_hdl);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t diag_session_info_del(bf_diag_sess_hdl_t sess_hdl) {
  bf_map_sts_t msts;
  diag_session_info_t *sess_info = NULL;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    DIAG_PRINT("Session %u does not exist \n", sess_hdl);
    return BF_SESSION_NOT_FOUND;
  }
  msts = bf_map_rmv(&DIAG_SESSION_INFO(dev_id).sess_map, sess_hdl);

  if (msts != BF_MAP_OK) {
    DIAG_PRINT("Session %u delete failed \n", sess_hdl);
    return BF_INVALID_ARG;
  }
  DIAG_MUTEX_DEINIT(&sess_info->session_mtx);
  DIAG_FREE(sess_info);

  return BF_SUCCESS;
}

/* Cleanup all sessions */
bf_status_t diag_session_info_del_all(bf_dev_id_t dev_id, bool dev_del) {
  bf_map_sts_t msts = BF_MAP_OK;
  diag_session_info_t *sess_info = NULL;
  unsigned long sess_hdl = 0;
  bf_diag_sess_hdl_t hdl = 0;

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  /* Remove all items from map */
  while (msts == BF_MAP_OK) {
    msts = bf_map_get_first(
        &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
    if ((msts == BF_MAP_OK) && sess_info) {
      hdl = sess_info->sess_hdl;
      DIAG_PRINT("Deleting Session %u \n", hdl);
      /* Cleanup session properly if not a device delete */
      if (!dev_del) {
        diag_test_cleanup(hdl);
      }
      if (bf_diag_session_valid(hdl)) {
        diag_session_info_del(hdl);
        diag_session_hdl_free(hdl);
      }
    }
    sess_info = NULL;
  }
  bf_map_destroy(&DIAG_SESSION_INFO(dev_id).sess_map);

  return BF_SUCCESS;
}

bf_status_t diag_session_hdl_alloc(bf_dev_id_t dev_id,
                                   bf_diag_sess_hdl_t *sess_hdl) {
  uint32_t i = 0;

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  *sess_hdl = 0;
  /* For snake bi-bdir we preallocate tcp port ranges for each session.
     This affects the number of unique tcp-dst port values for that session.
     Hence, do not increase max-sessions to a large number.
  */
  for (i = 0; i < diag_sessions_current_max; i++) {
    if (DIAG_SESSION_INFO(dev_id).sess_hdls[i] == 1) {
      continue;
    }
    /* Found a free handle, zero is invalid value of handle */
    /* Bits 0-7: session-handle, 8-15: dev_id */
    *sess_hdl = DIAG_SESS_HANDLE_CREATE(dev_id, (i + 1));
    /* Mark handle as used */
    DIAG_SESSION_INFO(dev_id).sess_hdls[i] = 1;
    DIAG_SESSION_INFO(dev_id).num_hdls_alloced += 1;
    DIAG_PRINT("Allocated handle %u on dev %d\n", *sess_hdl, dev_id);
    return BF_SUCCESS;
  }

  return BF_NO_SYS_RESOURCES;
}

bf_status_t diag_session_hdl_free(bf_diag_sess_hdl_t sess_hdl) {
  uint32_t hdl_pos = 0;
  bf_dev_id_t dev_id = 0;

  dev_id = DIAG_SESS_HANDLE_TO_DEV(sess_hdl);
  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  if (!DIAG_SESS_VALID(sess_hdl)) {
    return BF_INVALID_ARG;
  }

  /* Lower 8 bits is handle */
  hdl_pos = DIAG_SESS_HANDLE_TO_HDL(sess_hdl) - 1;

  if (DIAG_SESSION_INFO(dev_id).sess_hdls[hdl_pos] != 1) {
    return BF_INVALID_ARG;
  }
  /* Mark handle as free */
  DIAG_SESSION_INFO(dev_id).sess_hdls[hdl_pos] = 0;
  DIAG_SESSION_INFO(dev_id).num_hdls_alloced -= 1;
  DIAG_PRINT("Freed handle %u on dev %d\n", sess_hdl, dev_id);
  return BF_SUCCESS;
}

static bool diag_port_in_list(bf_dev_port_t port,
                              bf_dev_port_t *port_list,
                              int num_ports) {
  int i = 0;

  for (i = 0; i < num_ports; i++) {
    if (port == port_list[i]) {
      return true;
    }
  }

  return false;
}

/* Check if a port belongs to multiple sessions with a different loopback mode
 */
bf_status_t diag_session_config_overlap_check(
    bf_dev_id_t dev_id,
    bf_dev_port_t *port_list,
    int num_ports,
    bf_diag_port_lpbk_mode_e diag_loop_mode,
    bf_diag_sess_hdl_t *overlap_hdl) {
  bf_map_sts_t msts;
  diag_session_info_t *sess_info = NULL;
  unsigned long sess_hdl = 0;
  int i = 0;

  if (!DIAG_DEV_VALID(dev_id)) {
    return BF_INVALID_ARG;
  }

  /* Go over all items in map */
  msts = bf_map_get_first(
      &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
  while (msts == BF_MAP_OK && sess_info) {
    for (i = 0; i < DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports; i++) {
      if (diag_port_in_list(DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[i],
                            port_list,
                            num_ports)) {
        if ((sess_info) &&
            (sess_info->loop_params.loop_mode != diag_loop_mode)) {
          *overlap_hdl = sess_hdl;
          DIAG_PRINT(
              "Loopback mode on Port %d is different on session %lu on dev %d "
              "\n",
              DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[i],
              sess_hdl,
              dev_id);
          return BF_ALREADY_EXISTS;
        }
        /* Cannot have overlapping ports in the special full tcp port drain mode
           We could end up draining pkts of other session in this mode
         */
        if (DIAG_DRAIN_FULL_TCP_PORT_RANGE(dev_id)) {
          DIAG_PRINT(
              "Special all pkt drain mode is enabled on dev %d. "
              "Port already exists in session %lu\n",
              dev_id,
              sess_hdl);
          return BF_INVALID_ARG;
        }
      }
    }

    msts = bf_map_get_next(
        &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
  }

  return BF_SUCCESS;
}

/* Check if port belongs to another session */
bool diag_port_in_multiple_sessions(bf_dev_id_t dev_id,
                                    bf_dev_port_t dev_port) {
  bf_map_sts_t msts;
  diag_session_info_t *sess_info = NULL;
  unsigned long sess_hdl = 0;
  uint32_t count = 0;

  if (!DIAG_DEV_VALID(dev_id)) {
    return false;
  }

  /* Go over all items in map */
  msts = bf_map_get_first(
      &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
  while (msts == BF_MAP_OK && sess_info) {
    if (diag_port_in_list(dev_port,
                          &(DIAG_GET_LOOPBACK_PARAMS(sess_info).port_list[0]),
                          DIAG_GET_LOOPBACK_PARAMS(sess_info).num_ports)) {
      count++;
    }

    msts = bf_map_get_next(
        &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
  }

  if (count > 1) {
    return true;
  }

  return false;
}

/* Find a default session handle for a test type */
bf_status_t diag_find_default_session(diag_test_type_e test_type,
                                      bf_diag_sess_hdl_t *ret_sess_hdl) {
  bf_map_sts_t msts;
  diag_session_info_t *sess_info = NULL;
  unsigned long sess_hdl = 0;
  bf_dev_id_t dev_id = 0;

  *ret_sess_hdl = 0;

  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (!DIAG_DEV_VALID(dev_id)) {
      continue;
    }

    /* Go over all items in map */
    msts = bf_map_get_first(
        &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
    while (msts == BF_MAP_OK && sess_info) {
      if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == test_type) {
        /* Already found one handle earlier, found another one */
        if ((*ret_sess_hdl) != 0) {
          return BF_INVALID_ARG;
        }
        *ret_sess_hdl = sess_hdl;
      }

      msts = bf_map_get_next(
          &DIAG_SESSION_INFO(dev_id).sess_map, &sess_hdl, (void *)&sess_info);
    }
  }

  /* If we did not find a session handle */
  if ((*ret_sess_hdl) == 0) {
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

bf_status_t diag_sessions_max_set_helper(bf_dev_id_t dev,
                                         uint32_t max_sessions,
                                         ucli_context_t *uc) {
  int i = 0;

  if (!DIAG_DEV_VALID(dev)) {
    if (uc) {
      aim_printf(&uc->pvs, "Invalid device %d \n", dev);
    }
    return BF_INVALID_ARG;
  }

  if (max_sessions < DIAG_SESSIONS_MAX_DEFAULT) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "Error: Invalid max-value %d, cannot be less than %d\n",
                 max_sessions,
                 DIAG_SESSIONS_MAX_DEFAULT);
    }
    return BF_INVALID_ARG;
  }

  if (max_sessions > DIAG_SESSIONS_MAX_LIMIT) {
    if (uc) {
      aim_printf(&uc->pvs,
                 "Error: Invalid max-value %d, cannot be greater than %d\n",
                 max_sessions,
                 DIAG_SESSIONS_MAX_LIMIT);
    }
    return BF_INVALID_ARG;
  }

  for (i = 0; i < DIAG_SESSIONS_MAX_LIMIT; i++) {
    if (DIAG_SESSION_INFO(dev).sess_hdls[i] == 1) {
      if (uc) {
        aim_printf(&uc->pvs,
                   "Error: Session %d is active, delete all sessions using "
                   "sess-del-all\n",
                   i);
      }
      return BF_INVALID_ARG;
    }
  }

  diag_sessions_current_max = max_sessions;

  if (uc) {
    aim_printf(
        &uc->pvs, "Max sessions changed to %d \n", diag_sessions_current_max);
  }

  return BF_SUCCESS;
}

bool diag_test_type_loopback(const diag_session_info_t *sess_info) {
  if (!sess_info) {
    return false;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_LOOPBACK) {
    return true;
  }

  return false;
}

bool diag_test_type_snake(const diag_session_info_t *sess_info) {
  if (!sess_info) {
    return false;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_SNAKE) {
    return true;
  }

  return false;
}

bool diag_test_type_loopback_pair(const diag_session_info_t *sess_info) {
  if (!sess_info) {
    return false;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type ==
      DIAG_TEST_PAIRED_LOOPBACK) {
    return true;
  }

  return false;
}

bool diag_test_type_multicast_loopback(const diag_session_info_t *sess_info) {
  if (!sess_info) {
    return false;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type ==
      DIAG_TEST_MULTICAST_LOOPBACK) {
    return true;
  }

  return false;
}

bool diag_test_type_stream(const diag_session_info_t *sess_info) {
  if (!sess_info) {
    return false;
  }

  if (DIAG_GET_LOOPBACK_PARAMS(sess_info).test_type == DIAG_TEST_STREAM) {
    return true;
  }

  return false;
}

bool diag_is_chip_family_tofino1(bf_dev_id_t dev_id) {
  return (DIAG_DEV_INFO(dev_id)->chip_family == BF_DEV_FAMILY_TOFINO);
}

bool diag_is_chip_family_tofino2(bf_dev_id_t dev_id) {
  return (DIAG_DEV_INFO(dev_id)->chip_family == BF_DEV_FAMILY_TOFINO2);
}

bool diag_is_chip_family_tofino3(bf_dev_id_t dev_id) {
  return (DIAG_DEV_INFO(dev_id)->chip_family == BF_DEV_FAMILY_TOFINO3);
}

/* Get the cpu port */
int diag_cpu_port_get(bf_dev_id_t dev_id, bf_dev_port_t port) {
  /* Use the eth cpu port if specified in conf file */
  if (DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
    return DIAG_DEV_INFO(dev_id)->eth_cpu_info.eth_cpu_port;
  }
  if (DEV_PORT_TO_PIPE(port) >= DIAG_SUBDEV_PIPE_COUNT) {
    return DIAG_DEV_INFO(dev_id)->cpu_port2;
  }
  return DIAG_DEV_INFO(dev_id)->cpu_port;
}

/* Check if any device exists */
bool diag_any_device_exists() {
  bf_dev_id_t dev_id = 0;

  for (dev_id = 0; dev_id < BF_MAX_DEV_COUNT; dev_id++) {
    if (DIAG_DEV_INFO(dev_id)) {
      return true;
    }
  }

  return false;
}

/* Get default pktgen src port */
bf_dev_port_t diag_get_pktgen_port(bf_dev_id_t dev_id,
                                   bf_dev_pipe_t pipe,
                                   uint32_t app_id) {
  bf_dev_port_t port = 0;

  if (diag_is_chip_family_tofino1(dev_id)) {
    /* Tofino1 Pktgen port: All pipes only port 68 */
    port = 68;
  } else if (diag_is_chip_family_tofino2(dev_id)) {
    /* Tofino2 Pktgen port:
       Pipe 0: Only port 6
       Pipes 1,2,3: Port 0,2,4,6
    */
    if (pipe == 0) {
      port = 6;
    } else {
      /* Returning port 6 for all cases as diag needs other ports for
         pcie and eth cpu
      */
      switch (app_id % 4) {
        case 0:
        case 1:
        case 2:
        case 3:
        default:
          port = 6;
          break;
      }
    }
  } else if (diag_is_chip_family_tofino3(dev_id)) {
    /* Tofino3 Pktgen port: All pipes only port 6 */
    port = 6;
  } else {
    port = 6;
  }

  return (MAKE_DEV_PORT(pipe, port));
}

bf_status_t diag_pgen_port_add(bf_dev_id_t dev_id) {
  /* Maximum of 4 pktgen ports per pipe in all chips */
  bf_dev_port_t port = 0, port_list[4] = {0, 0, 0, 0};
  bf_status_t status = BF_SUCCESS;
  bf_dev_pipe_t pipe = 0;
  uint32_t num_ports = 0, ch = 0, num_pktgen_channels = 0;

  /* Do not enable Pktgen on Tofino2 A0 as pcie stops working */
  if (diag_is_chip_family_tofino2(dev_id) &&
      (!(DIAG_DEV_INFO(dev_id)->is_sw_model))) {
    if (DIAG_DEV_INFO(dev_id)->part_rev == BF_SKU_CHIP_PART_REV_A0) {
      return BF_SUCCESS;
    }
  }

  if (diag_is_chip_family_tofino2(dev_id)) {
    num_pktgen_channels = 2;
  } else {
    num_pktgen_channels = 1;
  }

  for (pipe = 0; pipe < DIAG_DEV_INFO(dev_id)->num_active_pipes; pipe++) {
    num_ports = 0;

    port_list[0] = diag_get_pktgen_port(dev_id, pipe, 0);
    num_ports++;

    for (uint32_t port_idx = 0; port_idx < num_ports; port_idx++) {
      port = port_list[port_idx];

      /* Remove the relevant pgen port channels if they already exist */
      for (ch = 0; ch < num_pktgen_channels; ch++) {
        bf_port_remove(dev_id, port + ch);
      }

      /* Add the pgen src port back in 100G */
      status = bf_port_add(dev_id, port, BF_SPEED_100G, BF_FEC_TYP_NONE);
      if (status != BF_SUCCESS) {
        DIAG_PRINT("Pktgen Port add failed for port %d", port);
      }
    }
  }

  return BF_SUCCESS;
}

/* Return max pktgen apps supported */
uint32_t diag_pgen_max_apps_on_chip(bf_dev_id_t dev_id) {
  uint32_t max_apps = 8;

  if (diag_is_chip_family_tofino1(dev_id)) {
    max_apps = 8;
  } else {
    max_apps = 16;
  }
  return max_apps;
}

bool diag_pgen_is_any_app_used(bf_dev_id_t dev_id, bf_dev_pipe_t pipe) {
  bool any_app_used = false;

  for (uint32_t app_id_cnt = 0; app_id_cnt < DIAG_PGEN_APPS_MAX_LIMIT;
       app_id_cnt++) {
    if (DIAG_PGEN_APP_USED(dev_id, pipe, app_id_cnt)) {
      any_app_used = true;
      break;
    }
  }

  return any_app_used;
}

/* Adjust the pktgen global pkt offset after all sessions are deleted */
void diag_pgen_adjust_global_pkt_buf_offset(bf_dev_id_t dev_id,
                                            diag_session_info_t *sess_info) {
  bf_dev_pipe_t pipe = 0;
  bool any_app_used = false;

  pipe = DIAG_GET_PGEN_PARAMS(sess_info).pipe;
  any_app_used = diag_pgen_is_any_app_used(dev_id, pipe);

  /* If all sessions are gone, reset pkt buf offset to 0 */
  if (!any_app_used) {
    DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[pipe] = 0;
  } else {
    uint32_t pkt_buf_offset = 0, aligned_pkt_size = 0;

    pkt_buf_offset = DIAG_GET_PGEN_PARAMS(sess_info).pkt_buf_offset;
    aligned_pkt_size = DIAG_GET_PGEN_PARAMS(sess_info).aligned_pkt_size;

    /* If this is session at the end of the pkt buffer,
          fix the pkt buffer offset */
    if ((pkt_buf_offset + aligned_pkt_size) ==
        DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[pipe]) {
      DIAG_DEV_INFO(dev_id)->pgen_global_pkt_buf_offset[pipe] -=
          aligned_pkt_size;
    }
  }
  return;
}

/* Find an unused pkt-gen app */
bf_status_t diag_pgen_find_free_app(bf_dev_id_t dev_id,
                                    bf_dev_pipe_t pipe,
                                    uint32_t *app_id) {
  uint32_t app_id_cnt = 0;
  uint32_t chip_app_limit = diag_pgen_max_apps_on_chip(dev_id);
  bool found_free = false;

  /* Find a free app */
  for (app_id_cnt = 0; app_id_cnt < DIAG_PGEN_APPS_MAX_LIMIT; app_id_cnt++) {
    if (app_id_cnt >= chip_app_limit) {
      break;
    }
    if (DIAG_PGEN_APP_USED(dev_id, pipe, app_id_cnt)) {
      continue;
    }
    found_free = true;
    break;
  }

  if (!found_free) {
    return BF_INVALID_ARG;
  }
  /* Use the first free app */
  *app_id = app_id_cnt;

  return BF_SUCCESS;
}

/* Reserve a pkt-gen app */
void diag_pgen_app_reserve(bf_dev_id_t dev_id,
                           bf_dev_pipe_t pipe,
                           uint32_t app_id,
                           bool free) {
  if (free) {
    DIAG_PGEN_APP_USED(dev_id, pipe, app_id) = 0;
    DIAG_PGEN_APP_USED_CNT(dev_id, pipe) -= 1;
  } else {
    DIAG_PGEN_APP_USED(dev_id, pipe, app_id) = 1;
    DIAG_PGEN_APP_USED_CNT(dev_id, pipe) += 1;
  }

  return;
}

uint32_t diag_pgen_get_aligned_pkt_size(uint32_t pkt_size) {
  uint32_t aligned_pkt_size = 0;
  int aligned_to = 16;

  /* Align pkt size to 16 byte */
  aligned_pkt_size = ((pkt_size + (aligned_to - 1)) & ~(aligned_to - 1));

  return aligned_pkt_size;
}

/* Get Default trigger time for pktgen */
uint32_t diag_get_pktgen_def_trigger_time_nsecs() {
  /* Set default to 1000 nano-sec (1 usec) */
  return 1000;
}

/* Validate loopback mode on the current chip */
bf_status_t diag_validate_loopback_mode(bf_dev_id_t dev_id,
                                        bf_diag_port_lpbk_mode_e loop_mode) {
  bool is_tofino1 = diag_is_chip_family_tofino1(dev_id);
  bool is_tofino2 = diag_is_chip_family_tofino2(dev_id);
  bool is_tofino3 = diag_is_chip_family_tofino3(dev_id);

  /* pipe loopback not supported on tofino1  */
  if ((loop_mode == BF_DIAG_PORT_LPBK_PIPE) && (is_tofino1)) {
    return BF_NOT_SUPPORTED;
  }

  /* phy loopback not supported on tofino2 */
  if ((loop_mode == BF_DIAG_PORT_LPBK_PHY) && (is_tofino2)) {
    return BF_NOT_SUPPORTED;
  }

  /* MAC and Serdes loopback not supported for tofino3 */
  if (((loop_mode == BF_DIAG_PORT_LPBK_MAC) ||
       (loop_mode == BF_DIAG_PORT_LPBK_PHY)) &&
      (is_tofino3)) {
    return BF_NOT_SUPPORTED;
  }

  return BF_SUCCESS;
}
