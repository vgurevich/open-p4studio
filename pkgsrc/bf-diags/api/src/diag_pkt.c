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
 * @file diag_pkt.c
 * @date
 *
 * Contains implementation of diag pkt send thread, recv callback
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //strlen
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <dvm/bf_dma_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <lld/bf_dma_if.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "diag_common.h"
#include "diag_create_pkt.h"
#include "diag_pkt.h"
#include "diags/bf_diag_api.h"
#include "diag_util.h"
#include "diag_pkt_database.h"
#include "diag_pd.h"

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)

static diag_slt_failure_type_e failure_test_mode = DIAG_SLT_FAILURE_TYPE_MAX;
static int failure_test_counter = 0;

void set_slt_failure_test_mode(diag_slt_failure_type_e mode) {
  failure_test_mode = mode;
  failure_test_counter = 0;
}

diag_slt_failure_type_e get_slt_failure_test_mode() {
  return failure_test_mode;
}

void corrupt_recv_pkt_acc_to_test_mode(uint8_t *buf, uint32_t size) {
  // For unit testing the pkt size MUST at least 850 Bytes
  switch (failure_test_mode) {
    case DIAG_SLT_FAILURE_TYPE_S_SETUP:
      // Ensure that the packet has all 0xff
      if (failure_test_counter == 0) {
        // A few 1->0 bit flips in the header
        buf[30] = 0xfe;
        buf[40] = 0xef;
        buf[45] = 0x7f;
      } else if (failure_test_counter >= 1) {
        // Just a single bit flip in the header
        buf[60] = 0xbf;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_SETUP:
      // Ensure that the packet has all 0xff
      if (failure_test_counter == 0) {
        // A single 1->0 bit flip in only bit offset 1
        buf[3] = 0xfd;
      } else if (failure_test_counter == 1) {
        // A few 1->0 bit flips in only bit offset 1
        buf[32] = 0xfd;
        buf[40] = 0xfd;
      } else if (failure_test_counter >= 2) {
        // A few 1->0 flips in bit offset 1 and a few 1->0 flips in
        // other bit offsets
        buf[32] = 0xfd;
        buf[41] = 0xdf;
        buf[7] = 0xfe;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_S_HOLD:
      // Ensure that the packet has all 0x00
      if (failure_test_counter == 0) {
        // A few 0->1 flips anywhere
        buf[56] = 0x01;
        buf[380] = 0x80;
        buf[100] = 0x04;
        buf[385] = 0x20;  // Last byte before the pkt_id starts for
                          // DIAG_PHV_STRESS_ENABLE compile
      } else if (failure_test_counter == 1) {
        // A few 0->1 flips only in the header
        buf[30] = 0x02;
        buf[51] = 0x80;
      } else if (failure_test_counter == 2) {
        // Just a single 0->1 flip in the header
        buf[60] = 0x10;
      } else if (failure_test_counter == 3) {
        // A few 0->1 flips only in the payload
        buf[379] = 0x02;
        buf[383] = 0x08;
      } else if (failure_test_counter == 4) {
        // Just a single 0->1 flip only in the payload
        buf[380] = 0x04;
      } else if (failure_test_counter >= 5) {
        // 2 bytes on 80 byte boundary with 0->1 flip
        buf[410] = 0x04;
        buf[490] = 0x10;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_HOLD:
      // Ensure that the packet has all 0x00
      if (failure_test_counter == 0) {
        // 0->1 bit flips only on consecutive 80 byte boundary
        buf[10] = 0x01;
        buf[90] = 0x80;
        buf[170] = 0x20;
        buf[250] = 0x20;
        buf[330] = 0x20;
      } else if (failure_test_counter == 1) {
        // 0->1 bit flips only on 80 byte boundary (may not be consecutive)
        buf[1] = 0x04;
        buf[10] = 0x01;
        buf[81] = 0x80;
        buf[90] = 0x80;
        buf[161] = 0x20;
        buf[241] = 0x10;
        buf[250] = 0x20;
        buf[330] = 0x20;

        buf[1] = 0x04;
        buf[81] = 0x80;
        buf[161] = 0x20;
        buf[241] = 0x10;
      } else if (failure_test_counter == 2) {
        // 0->1 bit flips on 80 byte boundaries and also some other 0->1 flips
        // in the header
        // Thus the packet will have S_Hold and SS_Hold criteria satisfied but
        // the final
        // category should be SS_Hold
        buf[10] = 0x01;
        buf[90] = 0x80;
        buf[170] = 0x20;
        buf[250] = 0x20;
        buf[330] = 0x20;

        buf[1] = 0x01;
        buf[81] = 0x02;
        buf[100] = 0x04;
      } else if (failure_test_counter == 3) {
        // 0->1 bit flips on 80 byte boundaries and also some other 0->1 flips
        // in the payload
        // Thus the packet will have S_Hold and SS_Hold criteria satisfied but
        // the final
        // category should be SS_Hold
        buf[10] = 0x01;
        buf[90] = 0x80;
        buf[170] = 0x20;
        buf[250] = 0x20;
        buf[330] = 0x20;

        buf[380] = 0x01;
        buf[382] = 0x02;
        buf[384] = 0x04;
      } else if (failure_test_counter >= 4) {
        // 0->1 bit flips on 80 byte boundaries only in the payload
        buf[400] = 0x01;
        buf[480] = 0x04;
        buf[640] = 0x80;
        buf[720] = 0x80;
        buf[800] = 0x80;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_UNKNOWN:
      // Ensure that the packet has 0xaa 0x55
      if (failure_test_counter == 0) {
        // A Single byte in the header with multiple bit flips
        buf[0] = 0xaf;
      } else if (failure_test_counter == 1) {
        // A few bytes in header with multiple bit flips
        buf[0] = 0xaf;
        buf[1] = 0xf5;
      } else if (failure_test_counter == 2) {
        // A Single byte in payload with multiple bit flips
        buf[380] = 0xaf;
      } else if (failure_test_counter == 3) {
        // A few bytes in payload with multiple bit flips
        buf[380] = 0xaf;
        buf[381] = 0xf5;
      } else if (failure_test_counter == 4) {
        // A few bytes with multiple 0->1 and 1->0 bitflips
        buf[100] = 0x0f;
        buf[1] = 0xf0;
      } else if (failure_test_counter >= 5) {
        // A few bytes with multiple 0->1 and 1->0 bitflips
        // But this time the 1->0 flips are strong suspect
        // for setup
        buf[10] = 0xa9;
        buf[1] = 0x57;
        buf[100] = 0x8a;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP:
      // Ensure that the packet has all 0xff
      if (failure_test_counter == 0) {
        // A single 1->0 flip in payload
        buf[380] = 0xfe;
      } else if (failure_test_counter == 1) {
        // A few 1->0 flips in the payload
        buf[380] = 0xef;
        buf[382] = 0xfe;
      } else if (failure_test_counter >= 2) {
        // A few 1->0 flips in the payload but this time the flips are at offset
        // 1
        buf[390] = 0xfd;
        buf[401] = 0xfd;
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_MIXED:
      // Ensure that the packet has 0xaa 0x55
      if (failure_test_counter == 0) {
        // s_setup && s_hold && payload_setup
        buf[0] = 0x8a;    // s_setup
        buf[380] = 0xab;  // hold
        buf[382] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 1) {
        // s_setup && payload_setup
        buf[0] = 0x8a;    // s_setup
        buf[382] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 2) {
        // s_hold && payload_setup
        buf[0] = 0xab;    // s_hold
        buf[382] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 3) {
        // ss_Hold && payload_setup
        buf[0] = 0xab;  // ss_hold
        buf[80] = 0xba;
        buf[160] = 0xea;

        buf[381] = 0x54;
      } else if (failure_test_counter == 4) {
        // s_setup && s_hold
        buf[0] = 0x8a;    // s_setup
        buf[380] = 0xab;  // hold
      } else if (failure_test_counter == 5) {
        // s_setup && ss_hold
        buf[2] = 0x8a;  // s_setup

        buf[0] = 0xab;  // ss_hold
        buf[80] = 0xba;
        buf[160] = 0xea;
      } else if (failure_test_counter == 6) {
        // s_setup && ss_hold && payload_setup
        buf[5] = 0x54;  // s_setup

        buf[402] = 0xab;  // ss_hold
        buf[562] = 0xba;
        buf[642] = 0xea;

        buf[700] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 7) {
        // ss_setup && s_hold && payload_setup
        buf[0] = 0xa8;    // ss_setup
        buf[380] = 0xab;  // s_hold
        buf[382] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 8) {
        // ss_setup && payload_setup
        buf[0] = 0xa8;    // ss_setup
        buf[382] = 0xa8;  // payload_setup
      } else if (failure_test_counter == 9) {
        // ss_setup && s_hold
        buf[0] = 0xa8;    // ss_setup
        buf[380] = 0xab;  // s_hold
      } else if (failure_test_counter == 10) {
        // ss_setup && ss_hold
        buf[2] = 0xa8;  // ss_setup

        buf[0] = 0xab;  // ss_hold
        buf[80] = 0xba;
        buf[160] = 0xea;
      } else if (failure_test_counter >= 11) {
        // ss_setup && ss_hold && payload_setup
        buf[6] = 0xa8;  // ss_setup

        buf[402] = 0xab;  // ss_hold
        buf[562] = 0xba;
        buf[642] = 0xea;

        buf[700] = 0xa8;  // payload_setup
      }
      break;
    case DIAG_SLT_FAILURE_TYPE_NO_FAILURE:
      break;
    default:
      break;
  }
  failure_test_counter++;
}

#endif  // (DIAG_SLT_UNIT_TEST) && (DIAG_PHV_STRESS_ENABLE)

extern const uint8_t DIAG_CPU_SRC_MAC[DIAG_MAC_SIZE];
uint32_t diag_pkt_logs_enabled = false;

/* Diags pkt tx thread create */
bf_status_t diag_pkt_tx_creator(bf_diag_sess_hdl_t sess_hdl) {
  diag_loopback_test_params_t *params;
  diag_session_info_t *sess_info = NULL;

  sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }

  params = DIAG_MALLOC(sizeof(diag_loopback_test_params_t));
  memset(params, 0, sizeof(diag_loopback_test_params_t));
  memcpy(params,
         &DIAG_GET_LOOPBACK_PARAMS(sess_info),
         sizeof(diag_loopback_test_params_t));

  /* Send the packets now, wait for completion */
  return diag_pkt_tx_fn(params);
}

/* Diags pkt inject */
bf_status_t diag_pkt_inject(bf_dev_id_t dev_id,
                            const bf_dev_port_t *port_list,
                            int num_ports,
                            uint32_t num_packet,
                            uint32_t pkt_size) {
  diag_loopback_test_params_t *params;

  params = DIAG_MALLOC(sizeof(diag_loopback_test_params_t));
  memset(params, 0, sizeof(diag_loopback_test_params_t));

  params->dev_id = dev_id;
  params->sess_hdl = 0;
  params->num_ports = num_ports;
  memcpy(&params->port_list[0], port_list, sizeof(bf_dev_port_t) * num_ports);
  params->pkt_size = pkt_size;
  params->num_packet = num_packet;
  params->loop_mode = BF_DIAG_PORT_LPBK_NONE;
  params->test_type = DIAG_TEST_NONE;
  params->bidir = false;
  params->valid = true;

  /* Send the packets now, wait for completion */
  diag_pkt_tx_fn(params);

  return BF_SUCCESS;
}

/* Diags pkt send thread init API */
bf_status_t diag_pkt_tx_fn(void *input_args) {
  uint8_t pkt_buf[DIAG_MAX_PKT_SIZE];
  int ret = 0, num_ports = 0, port_idx = 0;
  int byte_data_off = 0, byte_counter = 0;
  bf_pkt *pkt = NULL;
  bool bidir = false;
  uint32_t tx_ring = BF_PKT_TX_RING_0, tcpsrcport_start = 0, orig_pkt_size = 0;
  int pipe = 0;
  bf_dev_id_t dev_id = 0;
  bf_dev_port_t port = 0, port_list[BF_DIAG_MAX_PORTS];
  diag_test_type_e test_type = DIAG_TEST_NONE;
  uint32_t pkt_size = 0, num_packet = 0, count = 0, total_count = 0;
  uint32_t tcp_dstPort_start = 0, counter = 0, tcpdst_final = 0, tcp_count = 0;
  bf_diag_sess_hdl_t sess_hdl = 0;
  bool use_fixed_pkt_contents = false;
  diag_loopback_test_params_t *params =
      (diag_loopback_test_params_t *)input_args;
  pkt_id_type pkt_id = 0;
  static uint32_t diag_seed_counter = 0;
  unsigned int random_seed = 0;

  if (!params) {
    return BF_INVALID_ARG;
  }
  dev_id = params->dev_id;
  if (DIAG_PKT_RX_PROCESSING_DISABLED(dev_id)) {
    DIAG_PRINT(
        "WARNING: Pkt Rx processing is currently disabled for dev %d. Test "
        "might fail. \n",
        dev_id);
  }
  DIAG_PRINT("Pkt Tx API invoked for dev %d \n", dev_id);
  orig_pkt_size = params->pkt_size;
  pkt_size = orig_pkt_size;
  /* subtract 4 bytes as CRC header is added by PCIE interface */
  if (pkt_size > DIAG_PKT_CRC_HEADER_SIZE) {
    pkt_size = pkt_size - DIAG_PKT_CRC_HEADER_SIZE;
  }
  num_packet = params->num_packet;
  num_ports = params->num_ports;
  test_type = params->test_type;
  bidir = params->bidir;
  sess_hdl = params->sess_hdl;
  tcp_dstPort_start = params->tcp_dstPort_start;
  (void)tcp_dstPort_start;
  memcpy(&port_list[0], &(params->port_list[0]), sizeof(port_list));
  /* Free the passed memory */
  DIAG_FREE(params);

  use_fixed_pkt_contents = is_diag_use_fixed_pkt_contents(sess_hdl);

  for (port_idx = 0; port_idx < num_ports; port_idx++) {
    /* For snake test, send packet on first port only */
    if ((test_type == DIAG_TEST_SNAKE) && (!bidir) && (port_idx > 0)) {
      break;
    }
    /* For snake-bidir test, send packet on first and last port only */
    if ((test_type == DIAG_TEST_SNAKE) && (bidir) && (port_idx != 0) &&
        (port_idx != (num_ports - 1))) {
      continue;
    }
    /* For pair test, do not send packet on last even port */
    if ((test_type == DIAG_TEST_PAIRED_LOOPBACK) &&
        (((port_idx % 2) == 0) && ((port_idx + 1) == num_ports))) {
      continue;
    }
    /* For pair test, do not send packet on odd port if non-bidir */
    if ((test_type == DIAG_TEST_PAIRED_LOOPBACK) && (!bidir) &&
        ((port_idx % 2) != 0)) {
      continue;
    }

    port = port_list[port_idx];

    for (count = 0; count < num_packet; count++) {
      /* if pair-test is being run for all ports, then send very few pkts on
         cpu ports as they might get dropped
      */
      if ((test_type == DIAG_TEST_PAIRED_LOOPBACK) && (num_ports > 2) &&
          (is_diag_cpu_port_any_channel(dev_id, port))) {
        if (bidir) {
          if (count >= DIAG_PAIR_BIDIR_TEST_CPU_MAX_PKT) {
            break;
          }
        } else {
          if (count >= DIAG_PAIR_TEST_CPU_MAX_PKT) {
            break;
          }
        }
      }
      /* if pkt-size is zero, generate pkts of random size */
      if (orig_pkt_size == 0) {
        random_seed = (unsigned int)(time(NULL) + diag_seed_counter);
        diag_seed_counter++;
        // Use pkt-sizes less than 2000 for stress.
        pkt_size = rand_r(&random_seed) % 2000;
        if (pkt_size < DIAG_MIN_PKT_SIZE) {
          pkt_size = DIAG_MIN_PKT_SIZE;
        }
      }
      /* Create the packet */
      memset(pkt_buf, 0, sizeof(pkt_buf));
      ret = diag_create_pkt(
          sess_hdl, pkt_size, pkt_buf, count, DIAG_MIN_PKT_SIZE_ENABLED);
      if (ret < 0) {
        return BF_INVALID_ARG;
      }

// Overwrite any of the packet header fields only when we are not in the
// bit flip stress mode.
#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
      /* Add Random data in ethernet src/dst and ipv6 src/dst */
      byte_data_off = 1;
      byte_counter = 0;
      if (!use_fixed_pkt_contents) {
        for (byte_counter = 0; byte_counter < DIAG_PKT_MAC_SIZE;
             byte_counter++) {
          pkt_buf[DIAG_ETHERNET_DST_MAC_BYTE + byte_counter] =
              (port_idx + count + byte_data_off + byte_counter) % 255;
        }
        byte_data_off += byte_counter;
      }

      /* Encode the sess-hdl in the dst_mac[0][1] field */
      pkt_buf[DIAG_ETHERNET_DST_MAC_BYTE] = DIAG_SESS_HANDLE_TO_HDL(sess_hdl);
      pkt_buf[DIAG_ETHERNET_DST_MAC_BYTE + 1] =
          DIAG_SESS_HANDLE_TO_DEV(sess_hdl);

#ifdef DIAG_POWER_ENABLE
      if (!use_fixed_pkt_contents) {
        /* Randomize src mac only for power case,
           leave byte[0] = 0, to prevent multicast src
        */
        for (byte_counter = 1; byte_counter < DIAG_PKT_MAC_SIZE;
             byte_counter++) {
          pkt_buf[DIAG_ETHERNET_SRC_MAC_BYTE + byte_counter] =
              (port_idx + count + byte_data_off + byte_counter) % 255;
        }
        byte_data_off += byte_counter;
      }
#endif

      if (!use_fixed_pkt_contents) {
        for (byte_counter = 0; byte_counter < DIAG_IP_ADDR_SIZE;
             byte_counter++) {
          pkt_buf[DIAG_IP_SRC_BYTE + byte_counter] =
              (port_idx + count + byte_data_off + byte_counter) % 255;
        }
        byte_data_off += byte_counter;

        for (byte_counter = 0; byte_counter < DIAG_IP_ADDR_SIZE;
             byte_counter++) {
          pkt_buf[DIAG_IP_DST_BYTE + byte_counter] =
              (port_idx + count + byte_data_off + byte_counter) % 255;
        }
        byte_data_off += byte_counter;
      }

      /* Ethernet - 14 bytes, IP - 20 bytes, TCP dst-port is bytes 36 and 37 */
      /* Increment the tcp src port by 1 for each packet */
      if (!use_fixed_pkt_contents) {
        tcp_count = count;
      } else {
        tcp_count = 0;
      }
      pkt_buf[DIAG_TCP_SRC_PORT_BYTE + 1] =
          ((tcpsrcport_start + tcp_count) >> 8) & 0xff;
      pkt_buf[DIAG_TCP_SRC_PORT_BYTE] = (tcpsrcport_start + tcp_count) & 0xff;

      if (test_type == DIAG_TEST_SNAKE) {
        /* tcp-dst port value should be within the range allocated for that
           session
        */
        if (port_idx == 0) {
          tcpdst_final = DIAG_SESS_TCP_DSTPORT_START(sess_hdl) +
                         (tcp_count % DIAG_SESS_DSTPORT_RANGE);
        } else {
          tcpdst_final = DIAG_SESS_TCP_DSTPORT_REV_START(sess_hdl) +
                         (tcp_count % DIAG_SESS_DSTPORT_RANGE);
        }
        pkt_buf[DIAG_TCP_DST_PORT_BYTE] = ((tcpdst_final >> 8) & 0xff);
        pkt_buf[DIAG_TCP_DST_PORT_BYTE + 1] = (tcpdst_final & 0xff);
      } else if ((test_type == DIAG_TEST_PAIRED_LOOPBACK) ||
                 (test_type == DIAG_TEST_LOOPBACK) ||
                 (test_type == DIAG_TEST_MULTICAST_LOOPBACK)) {
        tcpdst_final = DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl) +
                       (tcp_count % DIAG_PORT_DSTPORT_SESS_RANGE);
        pkt_buf[DIAG_TCP_DST_PORT_BYTE] = ((tcpdst_final >> 8) & 0xff);
        pkt_buf[DIAG_TCP_DST_PORT_BYTE + 1] = (tcpdst_final & 0xff);
      } else {
        pkt_buf[DIAG_TCP_DST_PORT_BYTE] = (port >> 8) & 0xff;
        pkt_buf[DIAG_TCP_DST_PORT_BYTE + 1] = (port & 0xff);
      }
#else  // DIAG_PHV_STRESS_ENABLE || DIAG_PHV_FLOP_TEST
      /* Ethernet - 14 bytes, IP - 20 bytes, TCP dst-port is bytes 36 and 37 */
      /* Increment the tcp src port by 1 for each packet */
      if (!use_fixed_pkt_contents) {
        tcp_count = count;
      } else {
        tcp_count = 0;
      }
#if !defined(DIAG_PHV_FLOP_CONFIG_3)
      if (test_type == DIAG_TEST_SNAKE) {
        /* tcp-dst port value should be within the range allocated for that
           session
        */
        if (port_idx == 0) {
          tcpdst_final = DIAG_SESS_TCP_DSTPORT_START(sess_hdl) +
                         (tcp_count % DIAG_SESS_DSTPORT_RANGE);
        } else {
          tcpdst_final = DIAG_SESS_TCP_DSTPORT_REV_START(sess_hdl) +
                         (tcp_count % DIAG_SESS_DSTPORT_RANGE);
        }
        pkt_buf[DIAG_ETHERNET_HDR_SIZE] = ((tcpdst_final >> 8) & 0xff);
        pkt_buf[DIAG_ETHERNET_HDR_SIZE + 1] = (tcpdst_final & 0xff);
      } else if ((test_type == DIAG_TEST_PAIRED_LOOPBACK) ||
                 (test_type == DIAG_TEST_LOOPBACK)) {
        tcpdst_final = DIAG_PORT_TCP_DSTPORT_START(port, sess_hdl) +
                       (tcp_count % DIAG_PORT_DSTPORT_SESS_RANGE);
        pkt_buf[DIAG_ETHERNET_HDR_SIZE] = ((tcpdst_final >> 8) & 0xff);
        pkt_buf[DIAG_ETHERNET_HDR_SIZE + 1] = (tcpdst_final & 0xff);
      } else {
        pkt_buf[DIAG_ETHERNET_HDR_SIZE] = (port >> 8) & 0xff;
        pkt_buf[DIAG_ETHERNET_HDR_SIZE + 1] = (port & 0xff);
      }
#endif
#endif  // DIAG_PHV_STRESS_ENABLE || DIAG_PHV_FLOP_TEST
      /* Generate a unique id for this pkt and encode it in the final bytes of
         the payload */
      pkt_id = diag_encode_pkt_id(sess_hdl, port, pkt_size, pkt_buf);
      if (pkt_id == PKT_DATABASE_ERR) {
        DIAG_PRINT("Unable to encode the pkt_id in the packet\n");
        return BF_INVALID_ARG;
      }
      /* Add this packet to the database */
      if (diag_add_pkt_to_db(pkt_id, pkt_size, pkt_buf) != 0) {
        DIAG_PRINT("Unable to add pkt with pkt_id %lx to the database\n",
                   pkt_id);
        return BF_UNEXPECTED;
      }

      /* Send packets on all rings */
      /* 4 rings per subdevice. Rings 0-3: subdev0, Rings 4-7: subdev1 */
      tx_ring = (tx_ring + 1) % BF_PKT_TX_RING_4;
      pipe = DEV_PORT_TO_PIPE(port);
      if (pipe >= DIAG_SUBDEV_PIPE_COUNT) {
        tx_ring += BF_PKT_TX_RING_4;
      }

      /* Allocate packet */
      if ((!DIAG_DEV_INFO(dev_id)->kernel_pkt) &&
          (!DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port)) {
        /* allocate bf_pkt only if using pkt_mgr */
        if (bf_pkt_alloc(
                dev_id, &pkt, pkt_size, BF_DMA_CPU_PKT_TRANSMIT_0 + tx_ring)) {
          DIAG_PRINT("bf_pkt_alloc failed for ring %u\n", tx_ring);
          return BF_NO_SYS_RESOURCES;
        }
        if (bf_pkt_data_copy(pkt, pkt_buf, pkt_size) != 0) {
          DIAG_PRINT("bf_pkt_copy failed\n");
          bf_pkt_free(dev_id, pkt);
          return BF_NO_SYS_RESOURCES;
        }
      }
      /* Print pkt before sending */
      if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ALL) {
        DIAG_PRINT(
            "========= Packet being sent of id %lx from CPU to port %d of size "
            "%d in session %d tcp-dst-port %d pkt-num %d =========\n",
            pkt_id,
            port,
            pkt_size + DIAG_PKT_CRC_HEADER_SIZE,
            sess_hdl,
            tcpdst_final,
            count + 1);
        for (counter = 0; counter < pkt_size; counter++) {
          DIAG_PRINT("%02x", pkt_buf[counter]);
        }
        DIAG_PRINT("\n");
      }

      /* Send packet */
      if (DIAG_DEV_INFO(dev_id)->eth_cpu_info.use_eth_cpu_port) {
        if (diag_eth_cpu_pkt_tx(dev_id, pkt_buf, pkt_size) != BF_SUCCESS) {
          DIAG_PRINT("diag_eth_cpu_pkt_tx failed for pkt_size %u\n", pkt_size);
          return BF_NO_SYS_RESOURCES;
        }
      } else if (DIAG_DEV_INFO(dev_id)->kernel_pkt) {
        if (diag_kernel_pkt_tx(dev_id, pkt_buf, pkt_size)) {
          DIAG_PRINT("bf_pkt_kernel_tx failed for pkt_size %u\n", pkt_size);
          return BF_NO_SYS_RESOURCES;
        }
      } else {
        if (bf_pkt_tx(dev_id, pkt, tx_ring, (void *)(pkt)) != BF_SUCCESS) {
          DIAG_PRINT("bf_pkt_tx failed for ring %u\n", tx_ring);
          if (bf_pkt_free(dev_id, pkt) != 0) {
            DIAG_PRINT("Error freeing pkt in tx\n");
          }
          return BF_NO_SYS_RESOURCES;
        }
      }

      if (diag_pkt_log_val_get()) {
        DIAG_PRINT(
            "========= Successfully sent pkt with id %lx from CPU to port %d "
            "of size %d in session %d tcp-dst-port %d pkt-num %d =========\n",
            pkt_id,
            port,
            pkt_size + DIAG_PKT_CRC_HEADER_SIZE,
            sess_hdl,
            tcpdst_final,
            count + 1);
      }
      /* Increment stats */
      DIAG_GET_CPU_STATS(dev_id, port, sess_hdl).tx_total += 1;
      total_count++;
      /* Sleep for sometime after sending few packets */
      if (!DIAG_DEV_INFO(dev_id)->kernel_pkt) {
        if ((total_count % DIAG_PKT_SEND_BATCH_SIZE) == 0) {
          bf_sys_usleep(DIAG_PKT_SEND_INTERVAL_US);
        }
      } else {
        if ((total_count % DIAG_KERNEL_PKT_SEND_BATCH_SIZE) == 0) {
          bf_sys_usleep(DIAG_KERNEL_PKT_SEND_INTERVAL_US);
        }
      }
    }
  }

  // keep the compiler happy
  (void)use_fixed_pkt_contents;
  (void)tcp_count;
  (void)tcpsrcport_start;
  (void)byte_counter;
  (void)byte_data_off;
  return BF_SUCCESS;
}

static bf_status_t diag_cb_pkt_tx(bf_dev_id_t dev_id,
                                  bf_pkt_tx_ring_t tx_ring,
                                  uint64_t tx_cookie,
                                  uint32_t status) {
  if (bf_pkt_free(dev_id, (bf_pkt *)((uintptr_t)tx_cookie)) != 0) {
    DIAG_PRINT("Error freeing pkt in tx notify\n");
  }
  DIAG_TX_PKT_COMPLETIONS(dev_id) += 1;
  return BF_SUCCESS;
}

void diag_rcv_pkt_print(const pkt_id_type pkt_id,
                        const uint8_t *buf,
                        const uint32_t received_pkt_size,
                        const uint8_t *exp_bytes,
                        const uint32_t exp_pkt_size) {
  uint32_t cnt = 0;
  DIAG_PRINT("pkt_id %8lx : Expected Pkt\n", pkt_id);
  for (cnt = 0; cnt < exp_pkt_size; cnt++) {
    DIAG_PRINT("%02x", exp_bytes[cnt]);
  }
  DIAG_PRINT("\n");
  DIAG_PRINT("pkt_id %8lx : Received Pkt\n", pkt_id);
  for (cnt = 0; cnt < received_pkt_size; cnt++) {
    DIAG_PRINT("%02x", buf[cnt]);
  }
  DIAG_PRINT("\n");
}

void diag_rcv_static_pkt_print(const uint8_t *buf,
                               const uint32_t size,
                               const uint8_t *exp_bytes,
                               const bool print_exp) {
  uint32_t cnt = 0;

  /* We have already validated received header
     Print header of exp pkt from recvd pkt as we do not have the exp header
  */
  if (print_exp) {
    DIAG_PRINT("Expected pkt: \n");
    for (cnt = 0; (cnt < DIAG_PKT_HEADER_SIZE) && (cnt < size); cnt++) {
#if !defined(DIAG_POWER_ENABLE)
      if ((cnt >= DIAG_SRC_MAC_START_BYTE) &&
          (cnt < (DIAG_SRC_MAC_START_BYTE + DIAG_MAC_SIZE))) {
        DIAG_PRINT("%02x", DIAG_CPU_SRC_MAC[cnt - DIAG_SRC_MAC_START_BYTE]);
      } else if (cnt == DIAG_IP_TTL_BYTE) {
        DIAG_PRINT("%02x", DIAG_PKT_TTL);
      } else
#endif
#if defined(DIAG_MAU_BUS_STRESS_ENABLE)
          if ((cnt >= DIAG_MAU_BUS_STRESS_HDR_START_OFFSET) &&
              (cnt < DIAG_PKT_HEADER_SIZE)) {
        DIAG_PRINT("%02x", 0x00);
      } else
#endif
      {
        DIAG_PRINT("%02x", buf[cnt]);
      }
    }
    if (size > DIAG_PKT_HEADER_SIZE) {
      for (cnt = DIAG_PKT_HEADER_SIZE; cnt < (size - DIAG_PKT_HEADER_SIZE);
           cnt++) {
        DIAG_PRINT("%02x", exp_bytes[cnt]);
      }
    }
    DIAG_PRINT("\n\n");
  }
  DIAG_PRINT("Received pkt: \n");
  for (cnt = 0; cnt < size; cnt++) {
    DIAG_PRINT("%02x", buf[cnt]);
  }
  DIAG_PRINT("\n\n");
}

/* Diags pkt recv pkt callback */
static bf_status_t diag_cb_pkt_rx_common(bf_dev_id_t dev_id,
                                         uint8_t *buf,
                                         uint32_t size) {
  bf_dev_port_t ingress_port = 0, egress_port = 0;
  uint32_t tcp_dst_port = 0;
  uint32_t i = 0, k = 0;
  uint16_t random_seed = 0;
  uint8_t exp_bytes[DIAG_MAX_PKT_SIZE];
  bool err_pkt_hdr = false;
  bf_diag_sess_hdl_t sess_hdl = 0, hdl = 0;
  bf_status_t sts = BF_SUCCESS;
  pkt_id_type pkt_id = PKT_DATABASE_ERR;
  uint8_t total_pkt_id_bytes = 0;
  bool err_bit_flip = false;
  uint64_t num_bytes_mismatch = 0;
  uint64_t num_bits_mismatch = 0;
  uint64_t num_1_to_0_flips = 0;
  uint64_t num_0_to_1_flips = 0;
  diag_session_info_t *diag_session_info = NULL;
  uint32_t cached_pkt_size = 0;

#if defined(DIAG_SLT_UNIT_TEST) && defined(DIAG_PHV_STRESS_ENABLE)
  corrupt_recv_pkt_acc_to_test_mode(buf, size);
#endif

  memset(&exp_bytes, 0, sizeof(exp_bytes));

  if (!DIAG_DEV_VALID(dev_id)) {
    DIAG_PRINT("Packet received from invalid device-id %d\n", dev_id);
    // diag_rcv_pkt_print(PKT_DATABASE_ERR, buf, size, exp_bytes, 0);
    return BF_UNEXPECTED;
  }

  /* Check if rx pkt processing is disabled */
  if (DIAG_PKT_RX_PROCESSING_DISABLED(dev_id)) {
    return BF_SUCCESS;
  }

  if (size < (DIAG_PKT_HEADER_SIZE)) {
/* Do not print packets that diags is not interested in */
#if 0
    DIAG_PRINT(
        "Rcvd invalid pkt on CPU port of size %d (too "
        "small), "
        "Discarding it. !!!!\n",
        size);
    /* Cannot print exp packet as we havent yet recovered it */
    diag_rcv_pkt_print(PKT_DATABASE_ERR, buf, size, exp_bytes, 0);
#endif
    return BF_UNEXPECTED;
  }

  /* Retrieve the pkt_id from the pkt buf */
  if (diag_decode_pkt_id(size, buf, &pkt_id, &sess_hdl, &egress_port) != 0) {
#if 0
    DIAG_PRINT(
        "Unable to decode pkt_id from the received pkt "
        "!!!!\n");
#endif
    DIAG_PKT_ID_CORRUPT_CNT(dev_id) = DIAG_PKT_ID_CORRUPT_CNT(dev_id) + 1;
    return BF_UNEXPECTED;
  }

#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
  /* Ingress port is encoded in tcp src port */
  ingress_port = buf[DIAG_TCP_SRC_PORT_BYTE + 1] |
                 ((uint16_t)buf[DIAG_TCP_SRC_PORT_BYTE] << 8);
  if (ingress_port >= BF_DIAG_MAX_PORTS) {
#if 0
    DIAG_PRINT(
        "  Error: Invalid src port %d encoded in packet's tcp src field\n",
        ingress_port);
#endif
    err_pkt_hdr = true;
  }
#endif

  hdl = DIAG_SESS_HANDLE_TO_HDL(sess_hdl);
  diag_session_info = diag_session_info_get(sess_hdl);
  if (!diag_session_info) {
/* Do not print packets that diags is not interested in */
#if 0
    DIAG_PRINT(
        "Error: Invalid session handle %d encoded in "
        "pkt_id %ld, "
        "discarding pkt.\n",
        sess_hdl,
        pkt_id);
    /* Cannot print exp packet as we havent yet recovered it */
    diag_rcv_pkt_print(PKT_DATABASE_ERR, buf, size, exp_bytes, 0);
#endif
    if ((hdl < DIAG_SESSIONS_MAX_LIMIT) && (ingress_port < BF_DIAG_MAX_PORTS)) {
      DIAG_GET_CPU_STATS(dev_id, ingress_port, hdl).rx_bad += 1;
    }

    sts = BF_UNEXPECTED;
    goto end;
  }

  if (diag_pkt_log_val_get()) {
    /* Print a log indicating the start of processing if any log level is
     * enabled. */
    DIAG_PRINT(
        "========= Start processing for pkt_id %lx from port %d of size %d "
        "in session %d =========\n",
        pkt_id,
        ingress_port,
        size + DIAG_PKT_CRC_HEADER_SIZE,
        sess_hdl);
  }

  diag_slt_failure_type_e failure_type;
  sts = diag_compare_pkt_with_database(size,
                                       buf,
                                       pkt_id,
                                       sess_hdl,
                                       egress_port,
                                       exp_bytes,
                                       &cached_pkt_size,
                                       &num_bytes_mismatch,
                                       &num_bits_mismatch,
                                       &num_1_to_0_flips,
                                       &num_0_to_1_flips,
                                       &failure_type);
  if (sts != BF_SUCCESS) {
    diag_rcv_pkt_print(pkt_id, buf, size, exp_bytes, cached_pkt_size);
    err_bit_flip = true;
  }

  // hold session mutex
  DIAG_MUTEX_LOCK(&diag_session_info->session_mtx);
  diag_session_info->total_bytes_with_bit_flip_detected += num_bytes_mismatch;
  diag_session_info->total_bits_with_bit_flip_detected += num_bits_mismatch;
  diag_session_info->total_1_to_0_flips += num_1_to_0_flips;
  diag_session_info->total_0_to_1_flips += num_0_to_1_flips;
  switch (failure_type) {
    case DIAG_SLT_FAILURE_TYPE_S_SETUP:
      diag_session_info->total_weak_suspect_for_setup++;
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_SETUP:
      diag_session_info->total_strong_suspect_for_setup++;
      break;
    case DIAG_SLT_FAILURE_TYPE_S_HOLD:
      diag_session_info->total_weak_suspect_for_hold++;
      break;
    case DIAG_SLT_FAILURE_TYPE_SS_HOLD:
      diag_session_info->total_strong_suspect_for_hold++;
      break;
    case DIAG_SLT_FAILURE_TYPE_UNKNOWN:
      diag_session_info->total_unknown_failures++;
      break;
    case DIAG_SLT_FAILURE_TYPE_PAYLOAD_SETUP:
      diag_session_info->total_payload_setup_failures++;
      break;
    case DIAG_SLT_FAILURE_TYPE_MIXED:
      diag_session_info->total_mixed_failures++;
      break;
    default:
      break;
  }
  // Release the session mutex
  DIAG_MUTEX_UNLOCK(&diag_session_info->session_mtx);

// Don't compare the received with the expected packet in the older way if
// phv stress mode is enabled, because in that mode the payload as well as
// the headers can be totally random. Thus the older way of comparison
// won't work anyway as the expected packet that we will recreate won't
// match the one that was received.
#if !defined(DIAG_PHV_STRESS_ENABLE) && !defined(DIAG_PHV_FLOP_TEST)
  bool sess_invalid = false;

  /* Seed is in the IP identification field */
  random_seed =
      buf[DIAG_IP_ID_BYTE + 1] | ((uint16_t)buf[DIAG_IP_ID_BYTE] << 8);
  if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ALL) {
    DIAG_PRINT("Random seed in receive is %d \n", random_seed);
  }
  /* Session handle is in dst_mac[0][1] */
  sess_hdl = DIAG_SESS_HANDLE_CREATE(buf[DIAG_ETHERNET_DST_MAC_BYTE + 1],
                                     buf[DIAG_ETHERNET_DST_MAC_BYTE]);
  tcp_dst_port = buf[DIAG_TCP_DST_PORT_BYTE + 1] |
                 ((uint16_t)buf[DIAG_TCP_DST_PORT_BYTE] << 8);
  if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ALL) {
    DIAG_PRINT("Session handle in receive is %u \n", sess_hdl);
  }

  hdl = DIAG_SESS_HANDLE_TO_HDL(sess_hdl);
  if (!diag_session_info_get(sess_hdl)) {
    DIAG_PRINT(
        "  Error: Invalid session handle %d in dst mac, discarding pkt. "
        "Expected pkt cannot be recreated.\n",
        sess_hdl);
    if ((hdl < DIAG_SESSIONS_MAX_LIMIT) && (ingress_port < BF_DIAG_MAX_PORTS)) {
      DIAG_GET_CPU_STATS(dev_id, ingress_port, hdl).rx_bad += 1;
    }
    err_pkt_hdr = true;
    sess_invalid = true;
  }

#if !defined(DIAG_POWER_ENABLE)
  /* Src mac is random in power case */
  if (memcmp(&buf[DIAG_SRC_MAC_START_BYTE], DIAG_CPU_SRC_MAC, DIAG_MAC_SIZE)) {
    int j = 0;
    DIAG_PRINT("  Error: src mac mismatch. expecting: 0x\n");
    for (j = 0; j < DIAG_MAC_SIZE; j++) {
      DIAG_PRINT("%x ", DIAG_CPU_SRC_MAC[j]);
    }
    DIAG_PRINT(" ,got 0x");
    for (j = 6; j < (6 + DIAG_MAC_SIZE); j++) {
      DIAG_PRINT("%x ", buf[j]);
    }
    DIAG_PRINT(" ,\n");
    err_pkt_hdr = true;
  }

  /* ttl is random in power case */
  if (buf[DIAG_IP_TTL_BYTE] != DIAG_PKT_TTL) {
    DIAG_PRINT("  Error: ip ttl mismatch. expecting: 0x%x, got 0x%x \n",
               DIAG_PKT_TTL,
               buf[DIAG_IP_TTL_BYTE]);
    err_pkt_hdr = true;
  }
#endif

  /* CRC header gets removed when pkt comes back from PCIe interface */
  if ((!sess_invalid) &&
      (!diag_is_pkt_size_expected(sess_hdl, size + DIAG_PKT_CRC_HEADER_SIZE))) {
    DIAG_PRINT("  Error: Pkt size mismatch \n");
    err_pkt_hdr = true;
  }

  /* Check for ingress port */
  if ((!sess_invalid) && (ingress_port < BF_DIAG_MAX_PORTS) &&
      (!diag_is_pkt_expected_on_port(sess_hdl, ingress_port, tcp_dst_port))) {
    DIAG_PRINT("  Error: ig port mismatch \n");
    err_pkt_hdr = true;
  }

#else  // DIAG_PHV_STRESS_ENABLE || DIAG_PHV_FLOP_TEST
  /* For phv stress the ingress port cannot be obtained from header.
     Use the egress port obtained from local DB.
  */
  ingress_port = egress_port;
#endif

  if (err_pkt_hdr || err_bit_flip) {
    DIAG_PRINT("  Discarding pkt with pkt_id %lx on CPU from port %d size %d\n",
               pkt_id,
               ingress_port,
               size + DIAG_PKT_CRC_HEADER_SIZE);
    if ((hdl < DIAG_SESSIONS_MAX_LIMIT) && (ingress_port < BF_DIAG_MAX_PORTS)) {
      DIAG_GET_CPU_STATS(dev_id, ingress_port, hdl).rx_bad += 1;
    }
    sts = BF_UNEXPECTED;
    goto end;
  }

  if (diag_pkt_log_val_get()) {
    DIAG_PRINT(
        "  Valid diag pkt with id %lx on from port %d of size %d in session "
        "%d\n",
        pkt_id,
        ingress_port,
        size + DIAG_PKT_CRC_HEADER_SIZE,
        sess_hdl);
  }
  DIAG_GET_CPU_STATS(dev_id, ingress_port, sess_hdl).rx_good += 1;

  DIAG_GET_CPU_STATS(dev_id, ingress_port, sess_hdl).rx_total += 1;

  /* Pkt were injected from CPU to egress_port, pkt may have come back on
     a different port.
     Keep a count of pkts that came back in the originating port stats.
  */
  if (egress_port < BF_DIAG_MAX_PORTS) {
    DIAG_GET_CPU_STATS(dev_id, egress_port, sess_hdl).rx_origin += 1;
  }

  if (diag_pkt_log_val_get()) {
    DIAG_PRINT(
        "========= End Processing for pkt_id   %lx from port %d of size %d in "
        "session %d ========= \n",
        pkt_id,
        ingress_port,
        size + DIAG_PKT_CRC_HEADER_SIZE,
        sess_hdl);
  }
  // keep the compiler happy
  (void)total_pkt_id_bytes;
  (void)num_bytes_mismatch;
  (void)random_seed;
  (void)k;
  (void)i;
  (void)tcp_dst_port;
  return BF_SUCCESS;

end:
  if ((hdl < DIAG_SESSIONS_MAX_LIMIT) && (ingress_port < BF_DIAG_MAX_PORTS)) {
    DIAG_GET_CPU_STATS(dev_id, ingress_port, hdl).rx_total += 1;
  }

  if (diag_pkt_log_val_get()) {
    DIAG_PRINT(
        "========= End Processing for pkt_id   %lx from port %d of size %d in "
        "session %d ========= \n",
        pkt_id,
        ingress_port,
        size + DIAG_PKT_CRC_HEADER_SIZE,
        sess_hdl);
  }
  return sts;
}

/* Diags pkt recv pkt callback */
static bf_status_t diag_cb_pkt_rx(bf_dev_id_t dev_id,
                                  bf_pkt *pkt,
                                  void *cookie,
                                  bf_pkt_rx_ring_t rx_ring) {
  bf_pkt *orig_pkt;
  uint8_t *pkt_buf, *buf_p;
  uint32_t pkt_len = 0;
  uint32_t size = 0;
  uint8_t buf[DIAG_MAX_PKT_SIZE];

  memset(buf, 0, sizeof(buf));
  buf_p = &buf[0];

  /* Save a copy of the packet ptr */
  orig_pkt = pkt;

  /* Assemble the received packet in the buffer */
  do {
    pkt_buf = bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);
    memcpy(buf_p, pkt_buf, pkt_len);
    buf_p += pkt_len;
    size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  diag_cb_pkt_rx_common(dev_id, buf, size);
  if (bf_pkt_free(dev_id, orig_pkt) != 0) {
    DIAG_PRINT("Diag: Error freeing received pkt\n");
  }
  return BF_SUCCESS;
}

/* Diags pkt kernel recv pkt hanlder */
bf_status_t diag_kernel_cb_pkt_rx(bf_dev_id_t dev_id,
                                  uint8_t *buf,
                                  uint32_t size) {
  diag_cb_pkt_rx_common(dev_id, buf, size);
  return BF_SUCCESS;
}

/* Diags pkt eth cpu recv pkt hanlder */
bf_status_t diag_eth_cpu_cb_pkt_rx(bf_dev_id_t dev_id,
                                   uint8_t *buf,
                                   uint32_t size) {
  diag_cb_pkt_rx_common(dev_id, buf, size);
  return BF_SUCCESS;
}

/* Diags pkt rx register */
bf_status_t diag_register_pkt_rx(bf_dev_id_t dev_id) {
  bf_pkt_tx_ring_t tx_ring;
  bf_pkt_tx_ring_t max_tx_ring = BF_PKT_TX_RING_3;
  bf_pkt_rx_ring_t rx_ring;
  bf_pkt_rx_ring_t max_rx_ring = BF_PKT_RX_RING_7;

  /* Register for die-1 rings if needed */
  if (DIAG_DEV_INFO(dev_id)->num_active_pipes > DIAG_SUBDEV_PIPE_COUNT) {
    max_tx_ring = BF_PKT_TX_RING_7;
    max_rx_ring = BF_PKT_RX_RING_15;
  }

  for (tx_ring = BF_PKT_TX_RING_0; tx_ring <= max_tx_ring; tx_ring++) {
    if (bf_pkt_tx_done_notif_register(dev_id, diag_cb_pkt_tx, tx_ring) !=
        BF_SUCCESS) {
      DIAG_PRINT("tx reg failed for ring %d\n", tx_ring);
      return BF_HW_COMM_FAIL;
    }
  }

  for (rx_ring = BF_PKT_RX_RING_0; rx_ring <= max_rx_ring; rx_ring++) {
    if (bf_pkt_rx_register(dev_id, diag_cb_pkt_rx, rx_ring, NULL) !=
        BF_SUCCESS) {
      DIAG_PRINT("diag rx reg failed for ring %d\n", rx_ring);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

/* Diags pkt rx deregister */
bf_status_t diag_deregister_pkt_rx(bf_dev_id_t dev_id) {
  bf_pkt_rx_ring_t ring = BF_PKT_RX_RING_0;

  for (ring = BF_PKT_RX_RING_0; ring <= BF_PKT_RX_RING_7; ring++) {
    if (bf_pkt_rx_deregister(dev_id, ring) != BF_SUCCESS) {
      DIAG_PRINT("diag rx dereg failed for ring %d\n", ring);
      return BF_HW_COMM_FAIL;
    }
  }

  return BF_SUCCESS;
}

void diag_pkt_logs_val_set(uint32_t val) { diag_pkt_logs_enabled = val; }

uint32_t diag_pkt_log_val_get() { return diag_pkt_logs_enabled; }

bool diag_pkt_log_err_enabled() {
  if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ERR) {
    return true;
  }
  if (diag_pkt_log_val_get() & DIAG_PKT_LOG_ALL) {
    return true;
  }
  return false;
}

int diag_payload_data_patterns_set(bf_diag_sess_hdl_t sess_hdl,
                                   uint8_t s,
                                   uint32_t s_len,
                                   uint8_t a,
                                   uint8_t b,
                                   uint32_t pattern_len) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->start_pattern = s;
  sess_info->start_pattern_len = s_len;
  sess_info->data_pattern_a = a;
  sess_info->data_pattern_b = b;
  sess_info->pattern_len = pattern_len;
  return BF_SUCCESS;
}

int diag_payload_data_patterns_set_def(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->start_pattern = 0;
  sess_info->start_pattern_len = 0;
  sess_info->data_pattern_a = 0;
  sess_info->data_pattern_b = 0;
  sess_info->pattern_len = 0;
  return BF_SUCCESS;
}

int diag_payload_data_patterns_get(bf_diag_sess_hdl_t sess_hdl,
                                   uint8_t *s,
                                   uint32_t *s_len,
                                   uint8_t *a,
                                   uint8_t *b,
                                   uint32_t *pattern_len) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  *s = sess_info->start_pattern;
  *s_len = sess_info->start_pattern_len;
  *a = sess_info->data_pattern_a;
  *b = sess_info->data_pattern_b;
  *pattern_len = sess_info->pattern_len;
  return BF_SUCCESS;
}

int diag_use_fixed_pattern_set(bf_diag_sess_hdl_t sess_hdl, bool val) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->use_fixed_pattern = val;
  return BF_SUCCESS;
}

bool is_diag_use_fixed_pattern(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }
  return sess_info->use_fixed_pattern;
}

int diag_payload_type_set(bf_diag_sess_hdl_t sess_hdl,
                          bf_diag_packet_payload_t val) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->payload_type = val;
  return BF_SUCCESS;
}

bool is_diag_use_fixed_payload(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }
  return (sess_info->payload_type == BF_DIAG_PACKET_PAYLOAD_FIXED);
}

int diag_packet_full_type_set(bf_diag_sess_hdl_t sess_hdl,
                              bf_diag_packet_full_t val) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->pkt_full_type = val;
  return BF_SUCCESS;
}

bool is_diag_use_packet_full(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }
  return (sess_info->pkt_full_type == BF_DIAG_PACKET_FULL_FIXED);
}

bool is_diag_use_random_flip_payload(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }
  return (sess_info->payload_type == BF_DIAG_PACKET_PAYLOAD_RANDOM_FLIP);
}

void diag_fixed_payload_free_all(diag_session_info_t *sess_info) {
  uint32_t i = 0;

  for (i = 0; i < DIAG_MAX_FIXED_PAYLOAD_CNT; i++) {
    if (sess_info->fixed_payload[i]) {
      DIAG_FREE(sess_info->fixed_payload[i]);
      sess_info->fixed_payload[i] = NULL;
    }
  }
  sess_info->fixed_payload_cnt = 0;
}

int diag_fixed_payload_set(bf_diag_sess_hdl_t sess_hdl,
                           bool payload_file_valid,
                           const char *payload,
                           const char *payload_file_path) {
  uint32_t i = 0, j = 0;
  char temp[3];
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  diag_fixed_payload_free_all(sess_info);

  /* Payload pattern specified as a string */
  if (!payload_file_valid) {
    int patt_num = 0; /* Only one pattern */

    if (!sess_info->fixed_payload[patt_num]) {
      sess_info->fixed_payload[patt_num] = DIAG_CALLOC(1, DIAG_MAX_PKT_SIZE);
      if (!sess_info->fixed_payload[patt_num]) {
        return BF_NO_SYS_RESOURCES;
      }
    }

    for (i = 0, j = 0; (i < (strlen(payload) - 1)) && (j < DIAG_MAX_PKT_SIZE);
         i += 2, j++) {
      /* One Hex byte is a combination of two characters specified in string */
      temp[0] = payload[i];
      temp[1] = payload[i + 1];
      temp[2] = '\0';
      sess_info->fixed_payload[patt_num][j] = strtoul(temp, NULL, 16);
    }
    sess_info->fixed_payload_cnt = 1;
  } else {
    /* Multiple Payload patterns specified in a file */
    int patt_num = 0;
    char c, *string = NULL;
    FILE *fd = NULL;
    unsigned long fsize = 0, num_bytes_read = 0;

    fd = fopen(payload_file_path, "r");
    if (!fd) {
      return BF_INVALID_ARG;
    }
    fseek(fd, 0, SEEK_END);
    fsize = ftell(fd);
    if (fsize == 0) {
      DIAG_PRINT("File is empty \n");
      fclose(fd);
      return BF_INVALID_ARG;
    }

    fseek(fd, 0, SEEK_SET);
    string = DIAG_MALLOC(fsize + 1);
    if (!string) {
      fclose(fd);
      return BF_NO_SYS_RESOURCES;
    }
    num_bytes_read = fread(string, fsize, 1, fd);
    if (num_bytes_read == 0) {
      DIAG_PRINT("Error: File read error, no content to read\n");
      fclose(fd);
      return BF_INVALID_ARG;
    }
    fclose(fd);
    string[fsize] = 0;

    /* Delimiter for each pkt will be newline character */
    patt_num = 0;
    for (i = 0; i < fsize; i++) {
      c = string[i];

      if (c == '\n') {
        /* Delimiter of payload pattern */
        if (j > 0) {
          patt_num++;
        }
        j = 0;
        if ((patt_num + 1) >= DIAG_MAX_FIXED_PAYLOAD_CNT) {
          DIAG_PRINT("ERROR: Max supported payloads in file is %d \n",
                     DIAG_MAX_FIXED_PAYLOAD_CNT);
          DIAG_FREE(string);
          diag_fixed_payload_free_all(sess_info);
          return BF_INVALID_ARG;
        }
      } else if (c == ' ') {
        /* Skip spaces */
        continue;
      } else {
        if (!sess_info->fixed_payload[patt_num]) {
          sess_info->fixed_payload[patt_num] =
              DIAG_CALLOC(1, DIAG_MAX_PKT_SIZE);
          if (!sess_info->fixed_payload[patt_num]) {
            DIAG_FREE(string);
            diag_fixed_payload_free_all(sess_info);
            return BF_NO_SYS_RESOURCES;
          }
        }
        if (j >= DIAG_MAX_PKT_SIZE) {
          DIAG_PRINT("Packet payload greater than max allowed size of %d \n",
                     DIAG_MAX_PKT_SIZE);
          DIAG_FREE(string);
          diag_fixed_payload_free_all(sess_info);
          return BF_INVALID_ARG;
        }
        /* One Hex byte is a combination of two characters specified in string
         */
        if ((i + 1) < fsize) {
          char c_next;
          c_next = string[i + 1];
          if (c_next == '\n') {
            continue;
          }
          temp[0] = c;
          temp[1] = c_next;
          temp[2] = '\0';
          sess_info->fixed_payload[patt_num][j++] = strtoul(temp, NULL, 16);
          /* Increment one extra time as we used two characters */
          i++;
        }
      }
    }
    /* No delimiter after last payload pattern */
    if (j > 0) {
      patt_num++;
    }
    /* Free the allocated string */
    DIAG_FREE(string);
    if (patt_num <= 0) {
      DIAG_PRINT("ERROR: No payloads found in file \n");
      return BF_INVALID_ARG;
    }
    sess_info->fixed_payload_cnt = patt_num;
    DIAG_PRINT("Number of payloads specified in file is %d \n",
               sess_info->fixed_payload_cnt);
  }

  return BF_SUCCESS;
}

int diag_fixed_payload_set_def(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  diag_fixed_payload_free_all(sess_info);
  return BF_SUCCESS;
}

int diag_fixed_payload_cnt_get(bf_diag_sess_hdl_t sess_hdl,
                               uint32_t *pattern_cnt) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  *pattern_cnt = sess_info->fixed_payload_cnt;
  return BF_SUCCESS;
}

int diag_fixed_payload_get(bf_diag_sess_hdl_t sess_hdl,
                           uint32_t pattern_num,
                           char **payload) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  if (pattern_num >= sess_info->fixed_payload_cnt) {
    return BF_INVALID_ARG;
  }
  *payload = sess_info->fixed_payload[pattern_num];
  return BF_SUCCESS;
}

void diag_packet_full_free(diag_session_info_t *sess_info) {
  uint32_t i = 0;

  if (sess_info->pkt_full) {
    DIAG_FREE(sess_info->pkt_full);
    sess_info->pkt_full = NULL;
  }
}

int diag_packet_full_set(bf_diag_sess_hdl_t sess_hdl,
                         bool pkt_file_valid,
                         const char *pkt,
                         const char *pkt_file_path) {
  uint32_t i = 0, j = 0;
  char temp[3];
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  diag_packet_full_free(sess_info);

  /* Payload pattern specified as a string */
  if (!pkt_file_valid) {
    if (!sess_info->pkt_full) {
      sess_info->pkt_full = DIAG_CALLOC(1, DIAG_MAX_PKT_SIZE);
      if (!sess_info->pkt_full) {
        return BF_NO_SYS_RESOURCES;
      }
    }

    for (i = 0, j = 0; (i < (strlen(pkt) - 1)) && (j < DIAG_MAX_PKT_SIZE);
         i += 2, j++) {
      /* One Hex byte is a combination of two characters specified in string */
      temp[0] = pkt[i];
      temp[1] = pkt[i + 1];
      temp[2] = '\0';
      sess_info->pkt_full[j] = strtoul(temp, NULL, 16);
    }
  } else {
    char c, *string = NULL;
    FILE *fd = NULL;
    unsigned long fsize = 0, num_bytes_read = 0;

    fd = fopen(pkt_file_path, "r");
    if (!fd) {
      return BF_INVALID_ARG;
    }
    fseek(fd, 0, SEEK_END);
    fsize = ftell(fd);
    if (fsize == 0) {
      DIAG_PRINT("File is empty \n");
      fclose(fd);
      return BF_INVALID_ARG;
    }

    fseek(fd, 0, SEEK_SET);
    string = DIAG_MALLOC(fsize + 1);
    if (!string) {
      fclose(fd);
      return BF_NO_SYS_RESOURCES;
    }
    num_bytes_read = fread(string, fsize, 1, fd);
    if (num_bytes_read == 0) {
      DIAG_PRINT("Error: File read error, no content to read\n");
      fclose(fd);
      return BF_INVALID_ARG;
    }
    fclose(fd);
    string[fsize] = 0;

    for (i = 0; i < fsize; i++) {
      c = string[i];

      if (c == ' ') {
        /* Skip spaces */
        continue;
      } else {
        if (!sess_info->pkt_full) {
          sess_info->pkt_full = DIAG_CALLOC(1, DIAG_MAX_PKT_SIZE);
          if (!sess_info->pkt_full) {
            DIAG_FREE(string);
            diag_packet_full_free(sess_info);
            return BF_NO_SYS_RESOURCES;
          }
        }
        if (j >= DIAG_MAX_PKT_SIZE) {
          DIAG_PRINT("Packet greater than max allowed size of %d \n",
                     DIAG_MAX_PKT_SIZE);
          DIAG_FREE(string);
          diag_packet_full_free(sess_info);
          return BF_INVALID_ARG;
        }
        /* One Hex byte is a combination of two characters specified in string
         */
        if ((i + 1) < fsize) {
          char c_next;
          c_next = string[i + 1];
          if (c_next == '\n') {
            continue;
          }
          temp[0] = c;
          temp[1] = c_next;
          temp[2] = '\0';
          sess_info->pkt_full[j++] = strtoul(temp, NULL, 16);
          /* Increment one extra time as we used two characters */
          i++;
        }
      }
    }
    /* Free the allocated string */
    DIAG_FREE(string);
  }

  return BF_SUCCESS;
}

int diag_packet_full_set_def(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  diag_packet_full_free(sess_info);
  return BF_SUCCESS;
}

int diag_packet_full_get(bf_diag_sess_hdl_t sess_hdl, char **full_pkt) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  *full_pkt = sess_info->pkt_full;
  return BF_SUCCESS;
}

int diag_use_fixed_pkt_contents_set(bf_diag_sess_hdl_t sess_hdl, bool val) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return BF_INVALID_ARG;
  }
  sess_info->use_fixed_pkt_contents = val;
  return BF_SUCCESS;
}

bool is_diag_use_fixed_pkt_contents(bf_diag_sess_hdl_t sess_hdl) {
  diag_session_info_t *sess_info = diag_session_info_get(sess_hdl);
  if (!sess_info) {
    return false;
  }
  return sess_info->use_fixed_pkt_contents;
}
