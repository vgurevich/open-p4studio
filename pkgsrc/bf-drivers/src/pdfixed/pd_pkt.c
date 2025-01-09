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
#include <string.h>
#include <inttypes.h>
#include <tofino/pdfixed/pd_conn_mgr.h>
#include <tofino/pdfixed/pd_pkt.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pkt_mgr/bf_pkt.h>
#include <pkt_mgr/pkt_mgr_intf.h>

#define PKT_PD_MAX_SESSION 5
typedef struct pkt_pd_sess {
  bool in_use;
  bf_pkt *tx_pkt;
  bf_pkt *rx_pkt;
  uint32_t tx_ring;
  uint32_t rx_ring;
  uint32_t tx_compl_ring;
  uint32_t tx_compl_status;
  bool rx_state;
} pkt_pd_sess_t;

static pkt_pd_sess_t pkt_pd_session[PKT_PD_MAX_SESSION];

/* need to make these functions atomic when more sessions are supported */
static int pkt_pd_get_free_sess() {
  int i;

  for (i = 0; i < PKT_PD_MAX_SESSION; i++) {
    if (pkt_pd_session[i].in_use == 0) {
      return i;
    }
  }
  return -1;
}

static void pkt_pd_mark_sess_busy(int sess_id, bool in_use) {
  if (sess_id >= PKT_PD_MAX_SESSION) {
    return;
  } else {
    pkt_pd_session[sess_id].in_use = in_use;
  }
}

static bf_status_t pkt_mgr_test_tx_cb(bf_dev_id_t dev_id,
                                      bf_pkt_tx_ring_t tx_ring,
                                      uint64_t tx_cookie,
                                      uint32_t status) {
  int sess = (int)tx_cookie;

  printf("tx_cb id %d ring %d msg %" PRIx64 " status %u\n",
         dev_id,
         tx_ring,
         tx_cookie,
         status);
  if (sess >= PKT_PD_MAX_SESSION) {
    return BF_HW_COMM_FAIL;
  }

  pkt_pd_session[sess].tx_compl_ring = tx_ring;
  pkt_pd_session[sess].tx_compl_status = status;

  if (bf_pkt_free(0, pkt_pd_session[sess].tx_pkt) != 0) {
    printf("error freeing pkt in tx notify\n");
  }

  return BF_SUCCESS;
}

static bf_status_t pkt_mgr_test_rx_cb(bf_dev_id_t dev_id,
                                      bf_pkt *pkt,
                                      void *cookie,
                                      bf_pkt_rx_ring_t rx_ring) {
  int sess = 0;  // TBD how to handle rx pkt withexistingsessions

  (void)cookie;
  printf("rx_cb id %d ring %d sess %d size %d\n",
         dev_id,
         rx_ring,
         sess,
         bf_pkt_get_pkt_size(pkt));

  pkt_pd_session[sess].rx_ring = rx_ring;
  pkt_pd_session[sess].rx_pkt = pkt;
  pkt_pd_session[sess].rx_state = true;

  return BF_SUCCESS;
}

p4_pd_status_t pkt_pd_init(void) {
  int i;

  for (i = 0; i < 4; i++) {
    if (bf_pkt_tx_done_notif_register(0, pkt_mgr_test_tx_cb, i) != BF_SUCCESS) {
      printf("tx reg failed for ring %d (**unregister other handler)\n", i);
      return BF_HW_COMM_FAIL;
    }
  }
  for (i = 0; i < 8; i++) {
    if (bf_pkt_rx_register(0, pkt_mgr_test_rx_cb, i, NULL) != BF_SUCCESS) {
      printf("rx reg failed for ring %d (**unregister other handler)\n", i);
      return BF_HW_COMM_FAIL;
    }
  }
  return BF_SUCCESS;
}

void pkt_pd_cleanup(void) {
  int i;

  for (i = 0; i < 4; i++) {
    bf_pkt_tx_done_notif_deregister(0, i);
  }
  for (i = 0; i < 8; i++) {
    bf_pkt_rx_deregister(0, i);
  }
}

p4_pd_status_t pkt_pd_client_init(p4_pd_sess_hdl_t *sess_hdl) {
  int sess;

  if ((sess = pkt_pd_get_free_sess()) < 0) {
    *sess_hdl = 0;
    return BF_NO_SYS_RESOURCES;
  } else {
    pkt_pd_mark_sess_busy(sess, true);
    *sess_hdl = sess;
    pkt_pd_session[sess].rx_state = false;
    pkt_pd_session[sess].rx_pkt = NULL;
    return BF_SUCCESS;
  }
}

p4_pd_status_t pkt_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl) {
  if (sess_hdl >= PKT_PD_MAX_SESSION) {
    printf("bad sessionId\n");
    return BF_INVALID_ARG;
  }
  pkt_pd_mark_sess_busy(sess_hdl, false);
  return BF_SUCCESS;
}

p4_pd_status_t pkt_pd_pkt_tx_int(p4_pd_sess_hdl_t shdl,
                                 const uint8_t *buf,
                                 uint32_t size,
                                 uint32_t tx_ring,
                                 bf_pkt *pkt) {
  if (!buf || !pkt) {
    printf("Invalid argument\n");
    return -1;
  }

  bf_pkt_data_copy(pkt, buf, (uint16_t)size);

  pkt_pd_session[shdl].tx_pkt = pkt;
  pkt_pd_session[shdl].tx_ring = tx_ring;
  /* pre init the results withfault */
  pkt_pd_session[shdl].tx_compl_ring = 0xffffffff;
  pkt_pd_session[shdl].tx_compl_status = 0xffffffff;
  if (bf_pkt_tx(0, pkt, tx_ring, (void *)((uintptr_t)shdl)) != BF_SUCCESS) {
    printf("bf_pkt_tx failed for ring %u\n", tx_ring);
    if (bf_pkt_free(0, pkt) != 0) {
      printf("Error freeing pkt in pkt_pd_pkt_tx\n");
    }
    return -1;
  }
  return 0;
}

p4_pd_status_t pkt_pd_pkt_tx(p4_pd_sess_hdl_t shdl,
                             const uint8_t *buf,
                             uint32_t size,
                             uint32_t tx_ring) {
  bf_pkt *pkt;
  if (shdl >= PKT_PD_MAX_SESSION) {
    printf("bad sessionId\n");
    return BF_INVALID_ARG;
  }
  if (bf_pkt_alloc(0, &pkt, size, BF_DMA_CPU_PKT_TRANSMIT_0 + tx_ring)) {
    printf("bf_pkt_alloc failed for ring %u\n", tx_ring);
    return -1;
  }

  pkt->bypass_padding = false;
  return (pkt_pd_pkt_tx_int(shdl, buf, size, tx_ring, pkt));
}

/*
 * This function is used to indicate to the packet manager to skip the padding
 * of a short packet being injected into the switch.
 *
 */
p4_pd_status_t pkt_pd_pkt_tx_skip_padding(p4_pd_sess_hdl_t shdl,
                                          const uint8_t *buf,
                                          uint32_t size,
                                          uint32_t tx_ring) {
  bf_pkt *pkt;
  if (shdl >= PKT_PD_MAX_SESSION) {
    printf("bad sessionId\n");
    return BF_INVALID_ARG;
  }
  if (bf_pkt_alloc(0, &pkt, size, BF_DMA_CPU_PKT_TRANSMIT_0 + tx_ring)) {
    printf("bf_pkt_alloc failed for ring %u\n", tx_ring);
    return -1;
  }

  pkt->bypass_padding = true;
  return (pkt_pd_pkt_tx_int(shdl, buf, size, tx_ring, pkt));
}

p4_pd_status_t pkt_pd_verify_tx(p4_pd_sess_hdl_t shdl) {
  if (shdl >= PKT_PD_MAX_SESSION) {
    printf("bad sessionId\n");
    return BF_INVALID_ARG;
  } else {
    if (pkt_pd_session[shdl].tx_ring == pkt_pd_session[shdl].tx_compl_ring) {
      return pkt_pd_session[shdl].tx_compl_status;
    }
  }
  return BF_HW_COMM_FAIL;
}

void pkt_pd_get_rx(p4_pd_sess_hdl_t sess,
                   char *buf,
                   uint32_t *size,
                   uint32_t *rx_ring) {
  uint8_t *pkt_buf;
  bf_pkt *pkt;
  uint32_t pkt_len = 0;

  *size = 0;
  if (sess >= PKT_PD_MAX_SESSION) {
    printf("bad sessionId\n");
    return;
  }

  pkt = pkt_pd_session[sess].rx_pkt;

  *rx_ring = pkt_pd_session[sess].rx_ring;
  if (pkt_pd_session[sess].rx_state == false || !pkt) {
    printf("no received pkt\n");
    return;
  }
  // assemble the received packet in the buffer
  do {
    pkt_buf = bf_pkt_get_pkt_data(pkt);
    pkt_len = bf_pkt_get_pkt_size(pkt);
    memcpy(buf, pkt_buf, pkt_len);
    buf += pkt_len;
    *size += pkt_len;
    pkt = bf_pkt_get_nextseg(pkt);
  } while (pkt);

  if (bf_pkt_free(0, pkt_pd_session[sess].rx_pkt) != 0) {
    printf("error freeing pkt in pkt_pd_get_rx\n");
  }
  pkt_pd_session[sess].rx_state = false;
  pkt_pd_session[sess].rx_pkt = NULL;
}
