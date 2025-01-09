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
 * @file pkt_mgr_txrx.c
 * @date
 *
 * Implementation of PCIe packet driver DMA interface
 */

/* Module header files */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <inttypes.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/bf_dma_if.h>
#include <pkt_mgr/bf_pkt.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "pkt_mgr_priv.h"
#include "pkt_mgr_log.h"
#include <lld/lld_err.h>
#include <lld/lld_dr_descriptors.h>
#include <lld/lld_dr_if.h>

extern pkt_drv_info_t
    *pkt_drv_info[PKT_MGR_NUM_DEVICES][PKT_MGR_NUM_SUBDEVICES];

bf_status_t bf_pkt_tx_done_notif_register(bf_dev_id_t dev_id,
                                          bf_pkt_tx_done_notif_cb cb,
                                          bf_pkt_tx_ring_t tx_ring) {
  bf_subdev_id_t subdev_id = 0;
  bf_dev_family_t dev_family = 0;

  if ((dev_id >= PKT_MGR_NUM_DEVICES) || (dev_id < 0)) {
    return BF_INVALID_ARG;
  }

  dev_family = pkt_drv_info[dev_id][0]->dev_family;

  if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    if (tx_ring >= BF_PKT_TX_RING_EXT_MAX) {
      return BF_INVALID_ARG;
    }
    if (tx_ring >= BF_PKT_TX_RING_MAX) {
      if (bf_pcipkt_subdev1_is_en()) {
        subdev_id = 1;
        tx_ring -= BF_PKT_TX_RING_MAX;  // rebase to 0-3
      } else {
        return BF_INVALID_ARG;
      }
    }
  } else if (tx_ring >= BF_PKT_TX_RING_MAX) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[dev_id][subdev_id]) {
    return BF_NOT_READY;
  }
  if (pkt_drv_info[dev_id][subdev_id]->tx_nt_fn[tx_ring]) {
    return BF_ALREADY_EXISTS;
  } else {
    pkt_drv_info[dev_id][subdev_id]->tx_nt_fn[tx_ring] = cb;
    return BF_SUCCESS;
  }
}

bf_status_t bf_pkt_tx_done_notif_deregister(bf_dev_id_t dev_id,
                                            bf_pkt_tx_ring_t tx_ring) {
  bf_subdev_id_t subdev_id = 0;
  bf_dev_family_t dev_family = 0;

  if ((dev_id >= PKT_MGR_NUM_DEVICES) || (dev_id < 0)) {
    return BF_INVALID_ARG;
  }

  dev_family = pkt_drv_info[dev_id][0]->dev_family;

  if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    if (tx_ring >= BF_PKT_TX_RING_EXT_MAX) {
      return BF_INVALID_ARG;
    }
    if (tx_ring >= BF_PKT_TX_RING_MAX) {
      subdev_id = 1;
      tx_ring -= BF_PKT_TX_RING_MAX;  // rebase to 0-3
    }
  } else if (tx_ring >= BF_PKT_TX_RING_MAX) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[dev_id][subdev_id]) {
    return BF_NOT_READY;
  }
  if (!pkt_drv_info[dev_id][subdev_id]->tx_nt_fn[tx_ring]) {
    return BF_INVALID_ARG;
  } else {
    pkt_drv_info[dev_id][subdev_id]->tx_nt_fn[tx_ring] = NULL;
    pkt_drv_info[dev_id][subdev_id]->tx_cookie[tx_ring] = NULL;
    return BF_SUCCESS;
  }
}

bf_status_t bf_pkt_rx_register(bf_dev_id_t dev_id,
                               bf_pkt_rx_callback cb,
                               bf_pkt_rx_ring_t rx_ring,
                               void *rx_cookie) {
  bf_subdev_id_t subdev_id = 0;
  bf_dev_family_t dev_family = 0;

  if ((dev_id >= PKT_MGR_NUM_DEVICES) || (dev_id < 0)) {
    return BF_INVALID_ARG;
  }

  dev_family = pkt_drv_info[dev_id][0]->dev_family;

  if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    if (rx_ring >= BF_PKT_RX_RING_EXT_MAX) {
      return BF_INVALID_ARG;
    }
    if (rx_ring >= BF_PKT_RX_RING_MAX) {
      if (bf_pcipkt_subdev1_is_en()) {
        subdev_id = 1;
        rx_ring -= BF_PKT_RX_RING_MAX;  // rebase to 0-7
      } else {
        return BF_INVALID_ARG;
      }
    }
  } else if (rx_ring >= BF_PKT_RX_RING_MAX) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[dev_id][subdev_id]) {
    return BF_NOT_READY;
  }
  if (pkt_drv_info[dev_id][subdev_id]->rx_cb_fn[rx_ring]) {
    return BF_ALREADY_EXISTS;
  } else {
    pkt_drv_info[dev_id][subdev_id]->rx_cb_fn[rx_ring] = cb;
    pkt_drv_info[dev_id][subdev_id]->rx_cookie[rx_ring] = rx_cookie;
    return BF_SUCCESS;
  }
}

bf_status_t bf_pkt_rx_deregister(bf_dev_id_t dev_id, bf_pkt_rx_ring_t rx_ring) {
  bf_subdev_id_t subdev_id = 0;
  bf_dev_family_t dev_family = 0;

  if ((dev_id >= PKT_MGR_NUM_DEVICES) || (dev_id < 0)) {
    return BF_INVALID_ARG;
  }

  dev_family = pkt_drv_info[dev_id][0]->dev_family;

  if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    if (rx_ring >= BF_PKT_RX_RING_EXT_MAX) {
      return BF_INVALID_ARG;
    }
    if (rx_ring >= BF_PKT_RX_RING_MAX) {
      subdev_id = 1;
      rx_ring -= BF_PKT_RX_RING_MAX;  // rebase to 0-7
    }
  } else if (rx_ring >= BF_PKT_RX_RING_MAX) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[dev_id][subdev_id]) {
    return BF_NOT_READY;
  }
  if (!pkt_drv_info[dev_id][subdev_id]->rx_cb_fn[rx_ring]) {
    return BF_INVALID_ARG;
  } else {
    pkt_drv_info[dev_id][subdev_id]->rx_cb_fn[rx_ring] = NULL;
    pkt_drv_info[dev_id][subdev_id]->rx_cookie[rx_ring] = NULL;
    return BF_SUCCESS;
  }
}

/**
 *
 * Callback functions to be registered with the LLD Client Library for the
 * notification FIFOs.
 *
 */
void pkt_mgr_rx_pkt_cb(bf_dev_id_t chip,
                       bf_subdev_id_t subdev_id,
                       int data_sz,
                       bf_dma_addr_t dma_addr,
                       int s,
                       int e,
                       int cos) {
  bf_pkt *pkt;
  bf_dma_info_t *dma_info;
  bf_sys_dma_pool_handle_t hndl;
  unsigned int size;
  uint8_t *address;
  pkt_drv_info_t *rx_drv_info;
  bf_pkt_rx_ring_t rx_ring;

  if ((chip >= PKT_MGR_NUM_DEVICES) || (chip < 0)) {
    return;
  }
  if (subdev_id != 0) {
    if (!bf_pcipkt_subdev1_is_en()) {
      LOG_ERROR(
          "%s warning values chip %d  subdev %d\n", __func__, chip, subdev_id);
      assert(0);
    }
  }
  rx_drv_info = pkt_drv_info[chip][subdev_id];

  if ((cos >= BF_PKT_RX_RING_MAX) || (cos < 0)) {
    /* Attribute this error to the firat RX ring just for keeping the stats */
    rx_drv_info->rx_stat[0].pkt_param_err++;
    LOG_ERROR("%s bad values cos %d chip %d", __func__, cos, chip);
    return;
  }

  if (subdev_id > 0) {
    rx_ring = cos + BF_PKT_RX_RING_MAX;
  } else {
    rx_ring = cos;
  }
  dma_info = pkt_mgr_get_dma_info(chip, subdev_id);
  if (dma_info == NULL) {
    LOG_ERROR("Invalid dma info at %s:%d", __func__, __LINE__);
    return;
  }
  hndl = dma_info->dma_buff_info[cos + BF_DMA_CPU_PKT_RECEIVE_0]
             .dma_buf_pool_handle;
  size = dma_info->dma_buff_info[cos + BF_DMA_CPU_PKT_RECEIVE_0].dma_buf_size;
  /* Unmap the DMA address and get the virtual address of the buffer before
     it is processed by the packet manager */
  address = bf_mem_dma2virt(hndl, dma_addr);

  if (address != NULL) {
    if (bf_sys_dma_unmap(hndl, address, size, BF_DMA_TO_CPU) != 0) {
      LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                address,
                __func__,
                __LINE__);
    }
  } else {
    rx_drv_info->rx_stat[cos].pkt_addr_err++;
    LOG_ERROR("Invalid virtual address at %s:%d", __func__, __LINE__);
    return;
  }
  if (bf_pkt_alloc_ext_buf(chip,
                           subdev_id,
                           &pkt,
                           data_sz,
                           cos + BF_DMA_CPU_PKT_RECEIVE_0,
                           address,
                           hndl) != 0) {
    rx_drv_info->rx_stat[cos].pkt_alloc_err++;
    LOG_ERROR("%s error allocating a packet", __func__);
    return;
  }
  switch (rx_drv_info->rx_pkt_ctx[cos].pkt_state) {
    case BF_PKT_INIT:
      rx_drv_info->rx_pkt_ctx[cos].pkt_first = pkt;
      if (s != 1) {
        rx_drv_info->rx_stat[cos].pkt_assembly_err++;
        LOG_ERROR("%s:%d incorrect start in pkt chip %d cos %d s:e %d:%d",
                  __func__,
                  __LINE__,
                  chip,
                  cos,
                  s,
                  e);
        goto err_and_exit;
      }
      rx_drv_info->rx_stat[cos].pkt_ok++;
      if (e == 0) {
        // this is not the end packet, chain it
        rx_drv_info->rx_pkt_ctx[cos].pkt_state = BF_PKT_START;
        rx_drv_info->rx_pkt_ctx[cos].pkt_last = pkt;
      } else {
        // this is the end packet. Callback the hndlr
        if (rx_drv_info->rx_cb_fn[cos]) {
          rx_drv_info->rx_cb_fn[cos](chip,
                                     rx_drv_info->rx_pkt_ctx[cos].pkt_first,
                                     rx_drv_info->rx_cookie[cos],
                                     rx_ring);
        } else {
          rx_drv_info->rx_stat[cos].pkt_no_hndl_err++;
          goto err_and_exit;
        }
        // reset the packet building state
        rx_drv_info->rx_pkt_ctx[cos].pkt_state = BF_PKT_INIT;
      }
      return;
    case BF_PKT_START:
      bf_pkt_set_nextseg(rx_drv_info->rx_pkt_ctx[cos].pkt_last, pkt);
      if (s != 0) {
        rx_drv_info->rx_stat[cos].pkt_assembly_err++;
        LOG_ERROR("%s:%d incorrect middle pkt chip %d cos %d s:e %d:%d",
                  __func__,
                  __LINE__,
                  chip,
                  cos,
                  s,
                  e);
        goto err_and_exit;
      }
      rx_drv_info->rx_stat[cos].pkt_ok++;
      if (e == 0) {
        // this is not the end packet, chain it
        rx_drv_info->rx_pkt_ctx[cos].pkt_last = pkt;
      } else {
        // this is the end packet. Callback the hndlr
        if (rx_drv_info->rx_cb_fn[cos]) {
          rx_drv_info->rx_cb_fn[cos](chip,
                                     rx_drv_info->rx_pkt_ctx[cos].pkt_first,
                                     rx_drv_info->rx_cookie[cos],
                                     rx_ring);
        } else {
          rx_drv_info->rx_stat[cos].pkt_no_hndl_err++;
          goto err_and_exit;
        }
        // reset the packet building state
        rx_drv_info->rx_pkt_ctx[cos].pkt_state = BF_PKT_INIT;
      }
      return;
    default:
      goto err_and_exit;
  }

err_and_exit:
  // free up all pkts in the chain and reset the pkt state to init
  pkt = rx_drv_info->rx_pkt_ctx[cos].pkt_first;
  if (pkt) {
    if (bf_pkt_free(chip, pkt) != 0) {
      LOG_ERROR("%s:%d Error while trying to free the pkt", __func__, __LINE__);
    }
  }
  // reset the packet building state after error conditions
  rx_drv_info->rx_pkt_ctx[cos].pkt_state = BF_PKT_INIT;
  rx_drv_info->rx_pkt_ctx[cos].pkt_last = NULL;
  rx_drv_info->rx_pkt_ctx[cos].pkt_first = NULL;
  return;
}

bf_status_t bf_pkt_tx(bf_dev_id_t dev_id,
                      bf_pkt *pkt,
                      bf_pkt_tx_ring_t tx_ring,
                      void *tx_cookie) {
  int s, e;
  bf_dma_addr_t dma_addr;
  bf_sys_dma_pool_handle_t hndl;
  uint32_t size;
  int ret = 0;
  pkt_drv_info_t *tx_drv_info;
  bf_subdev_id_t subdev_id = 0;
  bf_dev_family_t dev_family = 0;

  if ((dev_id >= PKT_MGR_NUM_DEVICES) || (dev_id < 0)) {
    return BF_INVALID_ARG;
  }

  dev_family = pkt_drv_info[dev_id][0]->dev_family;

  if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    if (tx_ring >= BF_PKT_TX_RING_EXT_MAX) {
      return BF_INVALID_ARG;
    }
    if (tx_ring >= BF_PKT_TX_RING_MAX) {
      subdev_id = 1;
      tx_ring -= BF_PKT_TX_RING_MAX;  // rebase to 0-3
    }
  } else if (tx_ring >= BF_PKT_TX_RING_MAX) {
    return BF_INVALID_ARG;
  }

  tx_drv_info = pkt_drv_info[dev_id][subdev_id];

  if (!pkt) {
    tx_drv_info->tx_stat[tx_ring].pkt_param_err++;
    LOG_ERROR("%s bad values chip %d tx-ring %d pkt %p",
              __func__,
              dev_id,
              tx_ring,
              (void *)pkt);
    return BF_INVALID_ARG;
  }

  if (!pkt_mgr_is_cpu_pkt_en(dev_id, subdev_id)) {
    LOG_ERROR(
        "cpu pkt path is disabled for ASIC-%d subdev %d, not transmitting tx "
        "pkt..\n",
        dev_id,
        subdev_id);
    return BF_DEVICE_LOCKED;
  }

  if (!tx_drv_info->tx_nt_fn[tx_ring]) {
    tx_drv_info->tx_stat[tx_ring].pkt_no_hndl_err++;
    /* do not transmit if completion function not registered to avoid packet
     * memory leaks.
     */
    LOG_ERROR(
        "no tx completion handler for ASIC-%d subdev %d, not transmitting tx "
        "pkt..\n",
        dev_id,
        subdev_id);
    return BF_INVALID_ARG;
  }

  /* tbus is configured to insert 4 bytes of CRC
   * pad the packet to the minimum pci pkt size of the device if needed.
   */
  if (bf_pkt_get_pkt_size(pkt) < tx_drv_info->pci_min_pkt_size &&
      !bf_pkt_get_bypass_padding(pkt)) {
    size_t pci_min_pkt_size = tx_drv_info->pci_min_pkt_size;
    size_t pad_size = pci_min_pkt_size - bf_pkt_get_pkt_size(pkt);
    memset(bf_pkt_get_pkt_data(pkt) + bf_pkt_get_pkt_size(pkt), 0, pad_size);
    bf_pkt_set_pkt_size(pkt, pci_min_pkt_size);
  }

  tx_drv_info->tx_cookie[tx_ring] = tx_cookie;
  if (bf_sys_mutex_lock(&(tx_drv_info->tx_pkt_ctx[tx_ring].ring_mtx))) {
    LOG_ERROR("Not able to take mutex lock ASIC-%d subdev %d, tx_ring %d",
              dev_id,
              subdev_id,
              tx_ring);
    return BF_UNEXPECTED;
  }

  s = 1;
  e = 0;

  while (pkt) {
    if (!bf_pkt_get_nextseg(pkt)) {
      e = 1;
    }
    hndl = bf_pkt_get_pkt_dma_pool_handle(pkt);
    size = bf_pkt_get_pkt_size(pkt);
    /* Map the virtual address of the buffer to the DMA address
       before pushing it to the DR */
    if (bf_sys_dma_map(hndl,
                       bf_pkt_get_pkt_data(pkt),
                       bf_pkt_get_phys_pkt_data(pkt),
                       size,
                       &dma_addr,
                       BF_DMA_FROM_CPU) != 0) {
      tx_drv_info->tx_stat[tx_ring].pkt_drop++;
      LOG_ERROR("Unable to map DMA buffer %p at %s:%d",
                bf_pkt_get_pkt_data(pkt),
                __func__,
                __LINE__);
      if (bf_sys_mutex_unlock(&(tx_drv_info->tx_pkt_ctx[tx_ring].ring_mtx))) {
        LOG_ERROR("Not able to unlock mutex ASIC %d subdev %d, tx_ring %d",
                  dev_id,
                  subdev_id,
                  tx_ring);
      }
      return BF_HW_COMM_FAIL;
    }
    ret = lld_subdev_push_tx_packet(dev_id,
                                    subdev_id,
                                    tx_ring,
                                    s,
                                    e,
                                    size,
                                    dma_addr,
                                    (uint64_t)(uintptr_t)tx_cookie);
    if (ret != LLD_OK) {
      tx_drv_info->tx_stat[tx_ring].pkt_drop++;
      // TBD need to take care of freeing pkts here
      if (bf_sys_dma_unmap(
              hndl, bf_pkt_get_pkt_data(pkt), size, BF_DMA_FROM_CPU) != 0) {
        LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                  bf_pkt_get_pkt_data(pkt),
                  __func__,
                  __LINE__);
      }
      LOG_ERROR(
          "Push tx pkt error %d dev %d subdev %d ring %d start %d end %d size "
          "%d addr "
          "0x%" PRIx64 " id 0x%" PRIx64,
          ret,
          dev_id,
          subdev_id,
          tx_ring,
          s,
          e,
          size,
          dma_addr,
          (uint64_t)(uintptr_t)tx_cookie);
      if (bf_sys_mutex_unlock(&(tx_drv_info->tx_pkt_ctx[tx_ring].ring_mtx))) {
        LOG_ERROR("Not able to unlock mutex ASIC-%d subdev %d, tx_ring %d",
                  dev_id,
                  subdev_id,
                  tx_ring);
      }
      return BF_HW_COMM_FAIL;
    }
    tx_drv_info->tx_stat[tx_ring].pkt_ok++;
    s = 0;
    pkt = bf_pkt_get_nextseg(pkt);
  }
  if (lld_dr_start(dev_id, subdev_id, lld_dr_tx_pkt_0 + tx_ring) != LLD_OK) {
    // TBD need to take care of freeing pkts here
    LOG_ERROR("%s:%d error pkt tx dma start chip %d subdev %d tx_ring %d",
              __func__,
              __LINE__,
              dev_id,
              subdev_id,
              tx_ring);
    if (bf_sys_mutex_unlock(&(tx_drv_info->tx_pkt_ctx[tx_ring].ring_mtx))) {
      LOG_ERROR("Not able to unlock mutex ASIC-%d subdev %d, tx_ring %d",
                dev_id,
                subdev_id,
                tx_ring);
    }
    return BF_HW_COMM_FAIL;
  }
  if (bf_sys_mutex_unlock(&(tx_drv_info->tx_pkt_ctx[tx_ring].ring_mtx))) {
    LOG_ERROR("Not able to unlock mutex ASIC-%d subdev %d, tx_ring %d",
              dev_id,
              subdev_id,
              tx_ring);
  }
  return BF_SUCCESS;
}

void pkt_mgr_tx_completion_cb(bf_dev_id_t chip,
                              bf_subdev_id_t subdev_id,
                              bf_dma_dr_id_t dr,
                              uint64_t data_sz_or_ts,
                              uint32_t attr,
                              uint32_t status,
                              uint32_t type,
                              uint64_t msg_id,
                              int s,
                              int e) {
  int tx_ring = dr - lld_dr_cmp_tx_pkt_0;
  pkt_drv_info_t *tx_drv_info;
  bf_pkt_tx_ring_t tx_ring_cb;

  (void)attr;
  (void)data_sz_or_ts;

  if (subdev_id != 0) {
    if (!bf_pcipkt_subdev1_is_en()) {
      LOG_ERROR(
          "%s warning values chip %d  subdev %d\n", __func__, chip, subdev_id);
      assert(0);
    }
  }
  if (chip >= PKT_MGR_NUM_DEVICES) {
    return;
  }
  tx_drv_info = pkt_drv_info[chip][subdev_id];

  if (tx_ring < 0 || tx_ring >= BF_PKT_TX_RING_MAX) {
    LOG_ERROR("%s:%d tx_ring(%d) is greater then BF_PKT_TX_RING_MAX(%d).\n",
              __func__,
              __LINE__,
              tx_ring,
              (int)BF_PKT_TX_RING_MAX);
    return;
  }
  if (subdev_id > 0) {
    tx_ring_cb = tx_ring + BF_PKT_TX_RING_MAX;
  } else {
    tx_ring_cb = tx_ring;
  }
  /* drop the message if cpu pkt path is disabled. This can happen due a message
   * stuck in Tofino before the warm reboot begins. There is no valid buffer
   * corresponding to the msg_id in this case */
  if (!pkt_mgr_is_cpu_pkt_en(chip, subdev_id)) {
    LOG_ERROR("stale tx completion msg from ASIC-%d, dropping..\n", chip);
    return;
  }
  if (type != tx_m_type_pkt) {
    tx_drv_info->tx_stat[tx_ring].pkt_param_err++;
    LOG_ERROR(
        "bad tx completion type chip %d subdev %d dr %d s:e %d:%d type %u "
        "status %u",
        chip,
        subdev_id,
        dr,
        s,
        e,
        type,
        status);
    bf_sys_assert(0);
    return;
  }

  switch (tx_drv_info->tx_pkt_ctx[tx_ring].pkt_state) {
    case BF_PKT_INIT:
      if (s != 1) {
        tx_drv_info->tx_stat[tx_ring].pkt_compl_assembly_err++;
        LOG_ERROR(
            "%s:%d bad start pkt coml chip %d subdev %d tx_ring %d s:e %d:%d",
            __func__,
            __LINE__,
            chip,
            subdev_id,
            tx_ring,
            s,
            e);
        bf_sys_assert(0);
        goto err_and_exit;
      }
      tx_drv_info->tx_stat[tx_ring].pkt_compl_ok++;
      if (e == 0) {
        // this is not the complete packet, chain it
        tx_drv_info->tx_pkt_ctx[tx_ring].pkt_state = BF_PKT_START;
      } else {
        // this packet is a complete packet. Call back the hndlr
        if (tx_drv_info->tx_nt_fn[tx_ring]) {
          tx_drv_info->tx_nt_fn[tx_ring](chip, tx_ring_cb, msg_id, status);
        } else {
          tx_drv_info->tx_stat[tx_ring].pkt_no_hndl_err++;
          LOG_ERROR(
              "error no tx completion function registered, could cause "
              "pkt memory leak\n");
        }
      }
      return;
    case BF_PKT_START:
      if (s != 0) {
        tx_drv_info->tx_stat[tx_ring].pkt_compl_assembly_err++;
        LOG_ERROR(
            "%s:%d bad middle pkt compl chip %d subdev %d tx_ring %d s:e %d:%d",
            __func__,
            __LINE__,
            chip,
            subdev_id,
            tx_ring,
            s,
            e);
        bf_sys_assert(0);
        goto err_and_exit;
      }
      tx_drv_info->tx_stat[tx_ring].pkt_compl_ok++;
      if (e == 1) {
        // this is a complete packet. Call back the hndlr
        if (tx_drv_info->tx_nt_fn[tx_ring]) {
          tx_drv_info->tx_nt_fn[tx_ring](chip, tx_ring_cb, msg_id, status);
        } else {
          tx_drv_info->tx_stat[tx_ring].pkt_no_hndl_err++;
          LOG_ERROR(
              "error no tx completion function registered, could cause "
              "pkt memory leak\n");
        }

        tx_drv_info->tx_pkt_ctx[tx_ring].pkt_state = BF_PKT_INIT;
      }
      return;
    default:
      goto err_and_exit;
  }

err_and_exit:
  return;
  // free pkt ??
}
