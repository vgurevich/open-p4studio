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
 * @file pkt_mgr_drv.c
 * @date
 *
 * Implementation of PCIe packet driver interface
 */

/* Module header files */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>

#include <bf_types/bf_types.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/dvm_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/lld_err.h>
#include <lld/bf_dma_if.h>
#include <lld/bf_int_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>
#include <lld/bf_dev_if.h>
#include <pkt_mgr/bf_pkt.h>
#include <pkt_mgr/pkt_mgr_intf.h>
#include "pkt_mgr_priv.h"
#include "pkt_mgr_log.h"

pkt_drv_info_t *pkt_drv_info[PKT_MGR_NUM_DEVICES][PKT_MGR_NUM_SUBDEVICES];
static bool pkt_drv_lock_state[PKT_MGR_NUM_DEVICES][PKT_MGR_NUM_SUBDEVICES];

#define BF_PKT_CNT (16 * 1024)

/* definitions of pkt_tx_completion and rx_pkt DR interrupts */
#define TOFINO_TX_CPL_DR_ALL_NON_EMPTY_INT \
  0x1FE00UL  // the same for both Tof and Tof2
#define TOFINO_RX_DR_ALL_NON_EMPTY_INT \
  0xFFFF0000UL  // the same for both Tof and Tof2

bool bf_pkt_is_inited(bf_dev_id_t dev_id) {
  if (dev_id < PKT_MGR_NUM_DEVICES && pkt_drv_info[dev_id][PKT_MGR_OP_SUBDEV] &&
      pkt_drv_info[dev_id][PKT_MGR_OP_SUBDEV]->inited) {
    if ((pkt_drv_info[dev_id][PKT_MGR_OP_SUBDEV]->num_subdev > 1) &&
        bf_pcipkt_subdev1_is_en()) {
      if (pkt_drv_info[dev_id][1] && pkt_drv_info[dev_id][1]->inited) {
        return true;
      } else {
        return false;
      }
    } else {
      return true;
    }
  } else {
    return false;
  }
}

static void pkt_mgr_tbus_setup(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id);

static int pkt_mgr_setup_rx_buff(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_dma_info_t *dma_info) {
  unsigned int i, size;
  void *dma_pool;
  bf_dma_addr_t dma_addr;
  int ret = 0;

  for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
    dma_pool = pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].dma_pool;
    if (dma_pool == NULL) {
      LOG_ERROR("DMA pool for Pkt Rx %d on dev %d subdev %d not found",
                i,
                dev_id,
                subdev_id);
      return -1;
    }
    size = bf_dma_get_buf_size(dma_info, BF_DMA_CPU_PKT_RECEIVE_0 + i);

    bf_pkt_buf_info_t *b = pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs;
    for (; b; b = b->next) {
      /* Map the virtual address of the buffer to the DMA addres before it is
         pushed to the free memory DR */
      if (bf_sys_dma_map((bf_sys_dma_pool_handle_t)dma_pool,
                         b->vaddr,
                         b->phy_addr,
                         size,
                         &dma_addr,
                         BF_DMA_TO_CPU) != 0) {
        LOG_ERROR("Unable to map DMA buffer %p at %s:%d",
                  b->vaddr,
                  __func__,
                  __LINE__);
        goto free_and_exit;
      }
      ret = lld_subdev_push_fm(
          dev_id, subdev_id, lld_dr_fm_pkt_0 + i, dma_addr, size);
      if (ret != LLD_OK) {
        if (bf_sys_dma_unmap((bf_sys_dma_pool_handle_t)dma_pool,
                             b->vaddr,
                             size,
                             BF_DMA_TO_CPU) != 0) {
          LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                    b->vaddr,
                    __func__,
                    __LINE__);
        }
        LOG_ERROR(
            "Load pkt FM error %d dev %d subdev %d rx_ring %d addr 0x%" PRIx64
            " (%p) size %d\n",
            ret,
            dev_id,
            subdev_id,
            i,
            dma_addr,
            b->vaddr,
            size);
        goto free_and_exit;
      }
    }
    if (lld_dr_start(dev_id, subdev_id, lld_dr_fm_pkt_0 + i) != LLD_OK) {
      LOG_ERROR("error rx_ring %d lld_dr_start\n", i);
      goto free_and_exit;
    }
  }
  return 0;

free_and_exit:
  bf_sys_assert(0);
  // TBD  free the buffers that were allocated but could not be pushed on DR
  return -1;
}

static void pkt_mgr_teardown_rx_buff(bf_dev_id_t dev_id,
                                     bf_subdev_id_t subdev_id) {
  int i;
  /* For each RX ring... */
  for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
    void *dma_hndl =
        bf_dma_get_buf_pool_hndl(pkt_drv_info[dev_id][subdev_id]->dma_info,
                                 BF_DMA_CPU_PKT_RECEIVE_0 + i);
    /* Go through each buffer allocated to for it... */
    bf_pkt_buf_info_t *b = pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs;
    while (b) {
      /* Take this buffer out of the linked list. */
      pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs = b->next;
      /* Free the DMA memory and the linked-list node. */
      bf_sys_dma_free(dma_hndl, b->vaddr);
      bf_sys_free(b);
      /* Get the next element from the list head. */
      b = pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs;
    }
  }
}

static int pkt_mgr_setup_tx_buff(bf_dev_id_t dev_id,
                                 bf_subdev_id_t subdev_id,
                                 bf_dma_info_t *dma_info) {
  int i;

  for (i = 0; i < BF_PKT_TX_RING_MAX; i++) {
    pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].dma_pool =
        bf_dma_get_buf_pool_hndl(dma_info, BF_DMA_CPU_PKT_TRANSMIT_0 + i);
    if (pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].dma_pool == NULL) {
      return -1;
    }
    pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].pool_buf_size =
        bf_dma_get_buf_size(dma_info, BF_DMA_CPU_PKT_TRANSMIT_0 + i);
    pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].pool_buf_cnt =
        bf_dma_get_buf_cnt(dma_info, BF_DMA_CPU_PKT_TRANSMIT_0 + i);
  }
  return 0;
}

/* pkt_mgr_enable_all_dr
 *
 * Enables all the DRs used by the pkt manager
 */
static void pkt_mgr_enable_all_dr(bf_dev_id_t dev_id,
                                  bf_subdev_id_t subdev_id) {
  bf_dma_dr_id_t dr_id;

  // Enable lld_dr_fm_pkt_0,1,2,3,4,5,6,7
  for (dr_id = lld_dr_fm_pkt_0; dr_id <= lld_dr_fm_pkt_7; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, true);
  }

  // Enable lld_dr_tx_pkt_0,1,2,3
  for (dr_id = lld_dr_tx_pkt_0; dr_id <= lld_dr_tx_pkt_3; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, true);
  }

  // Enable lld_dr_cmp_tx_pkt_0,1,2,3
  for (dr_id = lld_dr_cmp_tx_pkt_0; dr_id <= lld_dr_cmp_tx_pkt_3; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, true);
  }

  /* Enable lld_dr_rx_pkt_0,1,2,3,4,5,6,7 */
  for (dr_id = lld_dr_rx_pkt_0; dr_id <= lld_dr_rx_pkt_7; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, true);
  }
}

/* pkt_mgr_disable_all_dr
 *
 * Disables all the DRs used by the pkt manager
 */
static void pkt_mgr_disable_all_dr(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id) {
  bf_dma_dr_id_t dr_id;

  // Disable lld_dr_fm_pkt_0,1,2,3,4,5,6,7
  for (dr_id = lld_dr_fm_pkt_0; dr_id <= lld_dr_fm_pkt_7; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, false);
  }

  // Disable lld_dr_tx_pkt_0,1,2,3
  for (dr_id = lld_dr_tx_pkt_0; dr_id <= lld_dr_tx_pkt_3; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, false);
  }

  // Disable lld_dr_cmp_tx_pkt_0,1,2,3
  for (dr_id = lld_dr_cmp_tx_pkt_0; dr_id <= lld_dr_cmp_tx_pkt_3; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, false);
  }

  /* Disable lld_dr_rx_pkt_0,1,2,3,4,5,6,7 */
  for (dr_id = lld_dr_rx_pkt_0; dr_id <= lld_dr_rx_pkt_7; dr_id++) {
    lld_subdev_dr_enable_set(dev_id, subdev_id, dr_id, false);
  }
}

static bf_status_t pkt_mgr_init_cpu_path(bf_dev_id_t dev_id,
                                         bf_subdev_id_t subdev_id,
                                         bf_dma_info_t *dma_info) {
  int i;
  bf_status_t status = BF_SUCCESS;

  if (subdev_id == 0 || bf_pcipkt_subdev1_is_en()) {
    // DMA only from subdevice == 0 unless for test purpose
    /* allocate buffer pools for pkt tx free Mem */
    if (pkt_mgr_setup_tx_buff(dev_id, subdev_id, dma_info) < 0) {
      LOG_ERROR("pkt_mgr_setup_tx_buff failed for dev %d subdev %d\n",
                dev_id,
                subdev_id);
      return BF_NO_SYS_RESOURCES;
    }

    /* Populate DMA buffers for Rx packets. */
    if (pkt_mgr_setup_rx_buff(dev_id, subdev_id, dma_info) < 0) {
      LOG_ERROR("pkt_mgr_setup_rx_buff failed for dev %d subdev %d\n",
                dev_id,
                subdev_id);
      return BF_NO_SYS_RESOURCES;
    }
  }

  pkt_mgr_tbus_setup(dev_id, subdev_id);

  // Enable the DR
  if (subdev_id == 0 || bf_pcipkt_subdev1_is_en()) {
    // DMA only from subdevice == 0 unless for test purpose
    pkt_mgr_enable_all_dr(dev_id, subdev_id);

    // drain the DRs in case of any invalid message stuk in Tofino aprior
    for (i = 0; i < BF_PKT_TX_RING_MAX; i++) {
      lld_dr_service(dev_id, subdev_id, lld_dr_cmp_tx_pkt_0 + i, 10000);
    }

    // Claim the pkt_mgr interrupts
    status = bf_int_claim_tbus(dev_id, subdev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "interrupt registration with LLD failed for device %d subdev %d, sts "
          "%s (%d)",
          dev_id,
          subdev_id,
          bf_err_str(status),
          status);
      return status;
    }
    // Enable the pkt_mgr interrupts
    status = bf_int_ena_tbus(dev_id, subdev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "interrupt enable with LLD failed for device %d sts %s "
          "(%d)",
          dev_id,
          bf_err_str(status),
          status);
      return status;
    }
  }
  // Mark cpu pkt path enabled
  pkt_drv_info[dev_id][subdev_id]->en_cpu_pkt = true;

  return status;
}

static bf_status_t pkt_mgr_allocate_rx_bufs(bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev_id) {
  int i;
  for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
    unsigned int j,
        J = bf_dma_get_buf_cnt(pkt_drv_info[dev_id][subdev_id]->dma_info,
                               BF_DMA_CPU_PKT_RECEIVE_0 + i);
    void *dma_hndl =
        bf_dma_get_buf_pool_hndl(pkt_drv_info[dev_id][subdev_id]->dma_info,
                                 BF_DMA_CPU_PKT_RECEIVE_0 + i);
    unsigned int size =
        bf_dma_get_buf_size(pkt_drv_info[dev_id][subdev_id]->dma_info,
                            BF_DMA_CPU_PKT_RECEIVE_0 + i);
    if (!dma_hndl) {
      LOG_ERROR(
          "Failed to get DMA pool handle for Rx pool %d on dev %d subdev %d",
          i,
          dev_id,
          subdev_id);
      return BF_NO_SYS_RESOURCES;
    }
    int avail_space =
        lld_subdev_dr_unused_get(dev_id, subdev_id, lld_dr_fm_pkt_0 + i);
    if (avail_space < 0) {
      LOG_ERROR("Failed to get unused DMA Rx pool space %d on dev %d subdev %d",
                i,
                dev_id,
                subdev_id);
      return BF_NO_SYS_RESOURCES;
    }

    if ((int)J > avail_space) {
      LOG_ERROR(
          "rx_ring %d not enough space exp %d avail %d\n", i, J, avail_space);
      J = avail_space;
    }

    pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].dma_pool = dma_hndl;
    pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].pool_buf_size = size;
    pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].pool_buf_cnt = J;

    for (j = 0; j < J; ++j) {
      bf_pkt_buf_info_t *b = bf_sys_malloc(sizeof *b);
      if (!b) return BF_NO_SYS_RESOURCES;
      int x = bf_sys_dma_alloc(dma_hndl, size, &b->vaddr, &b->phy_addr);
      if (x) {
        LOG_ERROR(
            "Failed to allocate buffer %d of %d for Rx pool %d, size %d, dev "
            "%d subdev %d, sts %d",
            j,
            J,
            i,
            size,
            dev_id,
            subdev_id,
            x);
        return BF_NO_SYS_RESOURCES;
      }
      b->next = pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs;
      pkt_drv_info[dev_id][subdev_id]->rx_bufs[i].bufs = b;
    }
  }
  return BF_SUCCESS;
}

static int pkt_mgr_drv_init_dev(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  unsigned i;

  dr_completion_callback_fn cmplt_cb;
  cmplt_cb = (dr_completion_callback_fn)pkt_mgr_tx_completion_cb;

  // Register callbacks with LLD.
  for (i = 0; i < BF_PKT_TX_RING_MAX; i++) {
    if (lld_register_completion_callback(
            dev_id, subdev_id, lld_dr_cmp_tx_pkt_0 + i, cmplt_cb)) {
      LOG_ERROR("LLD callback reg fails, dev %u subdev %d, fifo %u at %s:%d",
                dev_id,
                subdev_id,
                lld_dr_cmp_tx_pkt_0 + i,
                __func__,
                __LINE__);
      return -1;
    }
  }
  for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
    if (lld_register_rx_packet_callback(
            dev_id, subdev_id, i, pkt_mgr_rx_pkt_cb)) {
      LOG_ERROR("LLD callback reg fails, dev %u subdev %d, cos %u at %s:%d",
                dev_id,
                subdev_id,
                i,
                __func__,
                __LINE__);
      return -1;
    }
  }
  if ((lld_subdev_dr_lock_required(dev_id, subdev_id, lld_dr_tx_pkt_0) !=
       LLD_OK) ||
      (lld_subdev_dr_lock_required(dev_id, subdev_id, lld_dr_tx_pkt_1) !=
       LLD_OK) ||
      (lld_subdev_dr_lock_required(dev_id, subdev_id, lld_dr_tx_pkt_2) !=
       LLD_OK) ||
      (lld_subdev_dr_lock_required(dev_id, subdev_id, lld_dr_tx_pkt_3) !=
       LLD_OK)) {
    LOG_ERROR("LLD pkt dr lock configuration failed for dev %u subdev %d\n",
              dev_id,
              subdev_id);
    return -1;
  }
  return 0;
}

static bf_status_t pkt_mgr_add_device(bf_dev_id_t dev_id,
                                      bf_dev_family_t dev_family,
                                      bf_dma_info_t *dma_info,
                                      bf_dev_init_mode_t warm_init_mode) {
  (void)warm_init_mode;
  int i;
  bool warm_start = false;  // TBD parameterized for this function
  bf_status_t status = BF_SUCCESS;
  lld_err_t lld_err;
  bf_sku_chip_part_rev_t part_rev_number;
  bf_subdev_id_t subdev_id = 0;
  uint32_t num_subdev;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  if (pkt_drv_info[dev_id][subdev_id]) {
    return BF_ALREADY_EXISTS;
  }
  if (lld_sku_get_num_subdev(dev_id, &num_subdev, NULL) != LLD_OK) {
    return LLD_ERR_BAD_PARM;
  }
  if (num_subdev > PKT_MGR_NUM_SUBDEVICES) {
    return LLD_ERR_BAD_PARM;
  }
  /* Get the chip part revision number. */
  lld_err = lld_sku_get_chip_part_revision_number(dev_id, &part_rev_number);
  if (lld_err != LLD_OK) {
    LOG_ERROR(
        "PKT-MGR: Not able to get chip part revision number, dev %d subdev %d, "
        "sts %d",
        dev_id,
        subdev_id,
        lld_err);
    return BF_UNEXPECTED;
  }
  for (subdev_id = 0; subdev_id < (bf_subdev_id_t)num_subdev; subdev_id++) {
    pkt_drv_info[dev_id][subdev_id] =
        (pkt_drv_info_t *)bf_sys_malloc(sizeof(pkt_drv_info_t));
    if (!pkt_drv_info[dev_id][subdev_id]) {
      return BF_NO_SYS_RESOURCES;
    }
    memset(pkt_drv_info[dev_id][subdev_id], 0, sizeof(pkt_drv_info_t));

    pkt_drv_info[dev_id][subdev_id]->warm_start = warm_start;
    pkt_drv_info[dev_id][subdev_id]->inited = true;

    if (warm_start | pkt_drv_lock_state[dev_id][subdev_id]) {
      pkt_drv_lock_state[dev_id][subdev_id] = true;
    }

    /* initialize packet session state */
    for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
      pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].pkt_state = BF_PKT_INIT;
      pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].pkt_first = NULL;
      bf_sys_mutex_init(
          &(pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].ring_mtx));
    }
    for (i = 0; i < BF_PKT_TX_RING_MAX; i++) {
      pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].pkt_state = BF_PKT_INIT;
      pkt_drv_info[dev_id][subdev_id]->rx_pkt_ctx[i].pkt_first = NULL;
      bf_sys_mutex_init(
          &(pkt_drv_info[dev_id][subdev_id]->tx_pkt_ctx[i].ring_mtx));
    }
    /* Initialize device ID and type */
    pkt_drv_info[dev_id][subdev_id]->dev_id = dev_id;
    pkt_drv_info[dev_id][subdev_id]->subdev_id = subdev_id;
    pkt_drv_info[dev_id][subdev_id]->num_subdev = num_subdev;
    pkt_drv_info[dev_id][subdev_id]->dev_family = dev_family;
    pkt_drv_info[dev_id][subdev_id]->dma_info = dma_info + subdev_id;

    pkt_drv_info[dev_id][subdev_id]->part_rev_number = part_rev_number;

    /*
     * Based on the device family and chip part revision number,
     * set the minimum payload size for TX packets so that chip
     * part revision number need not be checked for each TX packet.
     */
    if (dev_family == BF_DEV_FAMILY_TOFINO &&
        part_rev_number == BF_SKU_CHIP_PART_REV_A0) {
      /*
       * For Tofino A0, min pkt size is 80 bytes (excluding 4bytes CRC) and
       * for all other parts, it is 60bytes (excluding 4bytes CRC).
       */
      pkt_drv_info[dev_id][subdev_id]->pci_min_pkt_size =
          PKT_MGR_PCI_MIN_PKT_SIZE_TOF_A0;
    } else {
      pkt_drv_info[dev_id][subdev_id]->pci_min_pkt_size =
          PKT_MGR_PCI_MIN_PKT_SIZE;
    }

    LOG_TRACE("PKT-MGR: %s, dev %d, part rev %d, pci min pkt size %d",
              __func__,
              dev_id,
              pkt_drv_info[dev_id][subdev_id]->part_rev_number,
              pkt_drv_info[dev_id][subdev_id]->pci_min_pkt_size);

    /* Register DR callbacks and setup DR config. */
    if (subdev_id == 0 || bf_pcipkt_subdev1_is_en()) {
      /* pkt DRs enabled only from subdevice == 0 unless for test purpose */
      pkt_mgr_drv_init_dev(dev_id, subdev_id);

      /* Allocate DMA memory for the Rx path. */
      status = pkt_mgr_allocate_rx_bufs(dev_id, subdev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to allocate DMA buffers for Rx packet path, device %d sts "
            "%s",
            dev_id,
            bf_err_str(status));
        return status;
      }
    }

    if (warm_init_mode != BF_DEV_INIT_COLD) {
      /* Do early init for fast reconfig and hitless scenario to
       * keep cpu packet path working and not impacted by
       * fast reconfig flush operation or hitless delta config
       * flush operation.
       */
      status = pkt_mgr_init_cpu_path(dev_id, subdev_id, dma_info + subdev_id);
      if (status != BF_SUCCESS) {
        LOG_ERROR("Unable to init cpu packet path for device %d, sts %s (%d)",
                  dev_id,
                  bf_err_str(status),
                  status);
        return status;
      }
    }
  }
  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;
}

/* API to instantiate a new device */
static bf_status_t pkt_mgr_add_device_cold_boot(
    bf_dev_id_t dev_id,
    bf_dev_family_t dev_family,
    bf_device_profile_t *profile,
    bf_dma_info_t *dma_info,
    bf_dev_init_mode_t warm_init_mode) {
  bf_status_t status = BF_SUCCESS;
  bf_subdev_id_t subdev_id = 0;

  (void)warm_init_mode;
  (void)dev_family;
  (void)profile;

  LOG_TRACE("Entering %s device %u", __func__, dev_id);

  if (warm_init_mode != BF_DEV_INIT_COLD) {
    /* if hitless case/fast reconfig case, cpu packet path is already inited. */
    LOG_TRACE(
        " Init mode is hit less/Fast reconfig case. %s device %u subdev %u",
        __func__,
        dev_id,
        subdev_id);
    return status;
  }

  if (dev_id >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return BF_INVALID_ARG;
  }

  for (subdev_id = 0;
       subdev_id < (bf_subdev_id_t)(pkt_drv_info[dev_id][0]->num_subdev);
       subdev_id++) {
    status = pkt_mgr_init_cpu_path(dev_id, subdev_id, dma_info + subdev_id);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "Unable to init cpu packet path for device %d, subdev_id %d sts %s "
          "(%d)",
          dev_id,
          subdev_id,
          bf_err_str(status),
          status);
      return status;
    }
  }

  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;
}

/* API to de-instantiate a device */
static bf_status_t pkt_mgr_remove_device(bf_dev_id_t dev_id) {
  int i;
  int j;

  LOG_TRACE("Entering %s", __func__);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  for (i = 0; i < PKT_MGR_NUM_SUBDEVICES; i++) {
    if (!pkt_drv_info[dev_id][i]) {
      continue;
    }
    // Disable the DRs
    if (i == 0 || bf_pcipkt_subdev1_is_en()) {
      // DMA only from subdevice == 0 unless for test purpose
      if (i < (int)(pkt_drv_info[dev_id][0]->num_subdev)) {
        pkt_mgr_disable_all_dr(dev_id, i);
      }
    }

    pkt_drv_info[dev_id][i]->en_cpu_pkt = false;

    /* Free up pkts and TX DMA buffers used with the device */
    if (i == 0) {
      pkt_mgr_free_dev_pkts(dev_id);
    }
    if (i == 0 || bf_pcipkt_subdev1_is_en()) {
      // DMA only from subdevice == 0 unless for test purpose
      if (i < (int)(pkt_drv_info[dev_id][0]->num_subdev)) {
        /* Free up RX DMA buffers used with the device */
        pkt_mgr_teardown_rx_buff(dev_id, i);
      }
    }
    for (j = 0; j < BF_PKT_RX_RING_MAX; j++) {
      bf_sys_mutex_del(&(pkt_drv_info[dev_id][i]->rx_pkt_ctx[j].ring_mtx));
    }
    for (j = 0; j < BF_PKT_TX_RING_MAX; j++) {
      bf_sys_mutex_del(&(pkt_drv_info[dev_id][i]->tx_pkt_ctx[j].ring_mtx));
    }
    bf_sys_free(pkt_drv_info[dev_id][i]);
    pkt_drv_info[dev_id][i] = NULL;
  }

  LOG_TRACE("Exiting %s successfully", __func__);
  return BF_SUCCESS;
}

bool pkt_mgr_is_cpu_pkt_en(bf_dev_id_t chip, bf_subdev_id_t subdev_id) {
  if (chip >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return false;
  }
  return (pkt_drv_info[chip][subdev_id]->en_cpu_pkt);
}

int bf_tbus_set_ts_offset(uint8_t chip,
                          bf_subdev_id_t subdev_id,
                          uint16_t offset) {
  uint32_t tbus_off;
  uint32_t val = 0;

  if (chip >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[chip][subdev_id]) {
    return BF_NOT_READY;
  }
  if (bf_pkt_is_locked(chip, subdev_id)) {
    return -1;
  }
  switch (pkt_drv_info[chip][subdev_id]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.ts);
      setp_tbus_flush_flush(&val, offset);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.ts);
      setp_tof2_tbus_flush_flush(&val, offset);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.ts);
      setp_tof3_tbus_flush_flush(&val, offset);
      break;
    default:
      return -1;
  }
  lld_subdev_write_register(chip, subdev_id, tbus_off, val);
  return 0;
}

int bf_tbus_flush_dma(uint8_t chip, bf_subdev_id_t subdev_id) {
  uint32_t tbus_off;
  uint32_t val = 0;

  if (chip >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[chip][subdev_id]) {
    return BF_NOT_READY;
  }
  if (bf_pkt_is_locked(chip, subdev_id)) {
    return -1;
  }
  switch (pkt_drv_info[chip][subdev_id]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.flush);
      setp_tbus_flush_flush(&val, 1);
      subdev_id = 0;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.flush);
      setp_tof2_tbus_flush_flush(&val, 1);
      subdev_id = 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.flush);
      setp_tof3_tbus_flush_flush(&val, 1);
      break;
    default:
      return BF_INVALID_ARG;
  }
  lld_subdev_write_register(chip, subdev_id, tbus_off, val);
  return 0;
}

int bf_tbus_int(uint8_t chip,
                bf_subdev_id_t subdev_id,
                int int_id,
                int high_prio,
                uint32_t *val,
                int get) {
  uint32_t tbus_off;

  if (chip >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[chip][subdev_id]) {
    return BF_NOT_READY;
  }
  if (bf_pkt_is_locked(chip, subdev_id)) {
    return -1;
  }
  if (pkt_drv_info[chip][subdev_id]->dev_family == BF_DEV_FAMILY_TOFINO) {
    switch (int_id) {
      case 0:
        if (high_prio) {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en0_1);
        } else {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en0_0);
        }
        break;
      case 1:
        if (high_prio) {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en1_1);
        } else {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en1_0);
        }
        break;
      case 2:
        if (high_prio) {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en2_1);
        } else {
          tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en2_0);
        }
        break;
      default:
        return -1;
    }
  } else if (pkt_drv_info[chip][subdev_id]->dev_family ==
             BF_DEV_FAMILY_TOFINO2) {
    switch (int_id) {
      case 0:
        if (high_prio) {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en0_1);
        } else {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en0_0);
        }
        break;
      case 1:
        if (high_prio) {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en1_1);
        } else {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en1_0);
        }
        break;
      case 2:
        if (high_prio) {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en2_1);
        } else {
          tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en2_0);
        }
        break;
      default:
        return -1;
    }
  } else if (pkt_drv_info[chip][subdev_id]->dev_family ==
             BF_DEV_FAMILY_TOFINO3) {
    switch (int_id) {
      case 0:
        if (high_prio) {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_1);
        } else {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_0);
        }
        break;
      case 1:
        if (high_prio) {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en1_1);
        } else {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en1_0);
        }
        break;
      case 2:
        if (high_prio) {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en2_1);
        } else {
          tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en2_0);
        }
        break;
      default:
        return -1;
    }
  } else {
    return BF_INVALID_ARG;
  }
  if (get) {
    lld_subdev_read_register(chip, subdev_id, tbus_off, val);
  } else {
    lld_subdev_write_register(chip, subdev_id, tbus_off, *val);
  }
  return 0;
}

int bf_tbus_config(uint8_t chip,
                   bf_subdev_id_t subdev_id,
                   bf_tbus_cfg_t *tbus_cfg) {
  uint32_t tbus_off;
  uint32_t val;
  uint32_t tmp;

  if (chip >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[chip][subdev_id]) {
    return BF_NOT_READY;
  }
  switch (pkt_drv_info[chip][0]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      tbus_off = offsetof(Tofino, device_select.tbc.tbc_tbus.ctrl);
      /* Read tbus ctrl register */
      lld_subdev_read_register(chip, 0, tbus_off, &val);
      /* port aliveness */
      tmp = (tbus_cfg->port_alive) ? 1 : 0;
      setp_tbus_ctrl_port_alive(&val, tmp);
      /* port enable */
      tmp = (tbus_cfg->port_en) ? 1 : 0;
      setp_tbus_ctrl_rx_en(&val, tmp);
      /* leave all crc settings as defaults */
      /* ecc disable */
      tmp = (tbus_cfg->ecc_dis) ? 1 : 0;
      setp_tbus_ctrl_ecc_dec_dis(&val, tmp);
      /* PFX rx threshold */
      tmp = tbus_cfg->pfc_rx_thr;
      setp_tbus_ctrl_pfc_rx(&val, tmp);
      /* PFX fm threshold */
      tmp = tbus_cfg->pfc_fm_thr;
      setp_tbus_ctrl_pfc_fm(&val, tmp);

      setp_tbus_ctrl_crcchk_dis(&val, 1);
      /* Disable CRC removal */
      tmp = (tbus_cfg->crcrmv_dis) ? 1 : 0;
      setp_tbus_ctrl_crcrmv_dis(&val, tmp);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      tbus_off = offsetof(tof2_reg, device_select.tbc.tbc_tbus.ctrl);
      /* Read tbus ctrl register */
      lld_subdev_read_register(chip, 0, tbus_off, &val);
      /* port aliveness */
      tmp = (tbus_cfg->port_alive) ? 1 : 0;
      setp_tof2_tbus_ctrl_port_alive(&val, tmp);
      /* port enable */
      tmp = (tbus_cfg->port_en) ? 1 : 0;
      setp_tof2_tbus_ctrl_rx_en(&val, tmp);
      /* leave all crc settings as defaults */
      /* ecc disable */
      tmp = (tbus_cfg->ecc_dis) ? 1 : 0;
      setp_tof2_tbus_ctrl_ecc_dec_dis(&val, tmp);
      /* PFX rx threshold */
      tmp = tbus_cfg->pfc_rx_thr;
      setp_tof2_tbus_ctrl_pfc_rx(&val, tmp);
      /* PFX fm threshold */
      tmp = tbus_cfg->pfc_fm_thr;
      setp_tof2_tbus_ctrl_pfc_fm(&val, tmp);

      setp_tof2_tbus_ctrl_crcchk_dis(&val, 1);
      /* Disable CRC removal */
      tmp = (tbus_cfg->crcrmv_dis) ? 1 : 0;
      setp_tof2_tbus_ctrl_crcrmv_dis(&val, tmp);

      /* RX channel byte offset */
      tmp = (tbus_cfg->rx_channel_offset) & 0x0f;
      setp_tof2_tbus_ctrl_rx_channel_offset(&val, tmp);
      /* enable keeping the CRC of input error pkt */
      tmp = (tbus_cfg->crcerr_keep) ? 1 : 0;
      setp_tof2_tbus_ctrl_crcerr_keep(&val, tmp);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tbus_off = offsetof(tof3_reg, device_select.tbc.tbc_tbus.ctrl);
      /* Read tbus ctrl register */
      lld_subdev_read_register(chip, subdev_id, tbus_off, &val);
      /* port aliveness */
      tmp = (tbus_cfg->port_alive) ? 1 : 0;
      setp_tof3_tbus_ctrl_port_alive(&val, tmp);
      /* port enable */
      tmp = (tbus_cfg->port_en) ? 1 : 0;
      setp_tof3_tbus_ctrl_rx_en(&val, tmp);
      /* leave all crc settings as defaults */
      /* ecc disable */
      tmp = (tbus_cfg->ecc_dis) ? 1 : 0;
      setp_tof3_tbus_ctrl_ecc_dec_dis(&val, tmp);
      /* PFX rx threshold */
      tmp = tbus_cfg->pfc_rx_thr;
      setp_tof3_tbus_ctrl_pfc_rx(&val, tmp);
      /* PFX fm threshold */
      tmp = tbus_cfg->pfc_fm_thr;
      setp_tof3_tbus_ctrl_pfc_fm(&val, tmp);

      setp_tof3_tbus_ctrl_crcchk_dis(&val, 1);
      /* Disable CRC removal */
      tmp = (tbus_cfg->crcrmv_dis) ? 1 : 0;
      setp_tof3_tbus_ctrl_crcrmv_dis(&val, tmp);

      /* RX channel byte offset */
      tmp = (tbus_cfg->rx_channel_offset) & 0x0f;
      setp_tof3_tbus_ctrl_rx_channel_offset(&val, tmp);
      /* enable keeping the CRC of input error pkt */
      tmp = (tbus_cfg->crcerr_keep) ? 1 : 0;
      setp_tof3_tbus_ctrl_crcerr_keep(&val, tmp);
      break;
    default:
      return BF_INVALID_ARG;
  }

  lld_subdev_write_register(chip, subdev_id, tbus_off, val);
  bf_tbus_set_ts_offset(chip, subdev_id, tbus_cfg->ts_offset);

  return 0;
}

/* For TOFINO-3, only subdevice = 0 pkt DRs are enabled */
bf_status_t bf_dma_service_pkt(bf_dev_id_t dev_id) {
  bf_subdev_id_t subdev_id = 0;
  int i;

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (!pkt_drv_info[dev_id][PKT_MGR_OP_SUBDEV]) {
    return BF_NOT_READY;
  }
  for (subdev_id = 0;
       subdev_id < (bf_subdev_id_t)(pkt_drv_info[dev_id][0]->num_subdev);
       subdev_id++) {
    if (subdev_id != 0 && !bf_pcipkt_subdev1_is_en()) {
      continue;
    }
    for (i = 0; i < BF_PKT_TX_RING_MAX; i++) {
      lld_dr_service(dev_id, subdev_id, lld_dr_cmp_tx_pkt_0 + i, 10000);
    }
    for (i = 0; i < BF_PKT_RX_RING_MAX; i++) {
      lld_dr_service(dev_id, subdev_id, lld_dr_rx_pkt_0 + i, 100);
    }
  }
  return BF_SUCCESS;
}

static void pkt_mgr_tbus_setup(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  // TBD move tbus_cfg to bf_pkt_init() parameter
  bf_tbus_cfg_t tbus_cfg;
  uint32_t val = 0;

  /* default initialization of tbus */
  tbus_cfg.pfc_fm_thr = 0x3;
  tbus_cfg.pfc_rx_thr = 0x3;
  tbus_cfg.ecc_dis = 0;
  tbus_cfg.port_en = 1;
  tbus_cfg.port_alive = 1;
  tbus_cfg.ts_offset = 0;
  tbus_cfg.crcrmv_dis = 0;
  // only use for Tof2, set to default values
  tbus_cfg.rx_channel_offset = 0;
  tbus_cfg.crcerr_keep = 1;
  bf_tbus_config(dev_id, subdev_id, &tbus_cfg);
  /* disable all tbus interrupts */
  bf_tbus_int(dev_id, subdev_id, 0, 0, &val, 0);
  bf_tbus_int(dev_id, subdev_id, 0, 1, &val, 0);
  bf_tbus_int(dev_id, subdev_id, 1, 0, &val, 0);
  bf_tbus_int(dev_id, subdev_id, 1, 1, &val, 0);
  bf_tbus_int(dev_id, subdev_id, 2, 0, &val, 0);
  bf_tbus_int(dev_id, subdev_id, 2, 1, &val, 0);
}

/* enables of disables leaf level INT_EN of TBUS TX_DR and RX_DR interrupts */
/* For TOFINO-3, only subdevice = 0 pkt DR Ints are enabled */
bf_status_t pkt_mgr_dr_int_en(bf_dev_id_t dev_id, bool en) {
  uint32_t reg, en0, en1;

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (bf_drv_is_device_virtual(dev_id) || bf_pkt_is_locked(dev_id, 0) ||
      !bf_pkt_is_inited(dev_id)) {
    return BF_NOT_READY;
  }
  if (en) {
    en0 = TOFINO_TX_CPL_DR_ALL_NON_EMPTY_INT;
    en1 = TOFINO_RX_DR_ALL_NON_EMPTY_INT;
  } else {
    en0 = 0;
    en1 = 0;
  }
  switch (pkt_drv_info[dev_id][0]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      reg = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en0_0);
      lld_subdev_write_register(dev_id, 0, reg, en0);
      reg = offsetof(Tofino, device_select.tbc.tbc_tbus.int_en1_0);
      lld_subdev_write_register(dev_id, 0, reg, en1);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      reg = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en0_0);
      lld_subdev_write_register(dev_id, 0, reg, en0);
      reg = offsetof(tof2_reg, device_select.tbc.tbc_tbus.intr_en1_0);
      lld_subdev_write_register(dev_id, 0, reg, en1);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      reg = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en0_0);
      lld_subdev_write_register(dev_id, 0, reg, en0);
      if (pkt_drv_info[dev_id][0]->num_subdev > 0) {
        lld_subdev_write_register(dev_id, 1, reg, en0);
      }
      reg = offsetof(tof3_reg, device_select.tbc.tbc_tbus.intr_en1_0);
      lld_subdev_write_register(dev_id, 0, reg, en1);
      if (pkt_drv_info[dev_id][0]->num_subdev > 0) {
        lld_subdev_write_register(dev_id, 1, reg, en1);
      }
      break;
    default:
      return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t pkt_mgr_lock_device(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (bf_drv_is_device_virtual(dev_id)) {
    return BF_SUCCESS;
  }
  pkt_drv_lock_state[dev_id][0] = true;
  pkt_drv_lock_state[dev_id][1] = true;
  return BF_SUCCESS;
}

bf_status_t pkt_mgr_create_dma(bf_dev_id_t dev_id) {
  (void)dev_id;
  LOG_TRACE("Entering %s", __func__);
  LOG_TRACE("Exiting %s", __func__);
  return BF_SUCCESS;
}

static bf_status_t pkt_mgr_core_reset(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s", __func__);
  if (bf_drv_is_device_virtual(dev_id)) {
    LOG_TRACE("Exiting %s", __func__);
    return BF_SUCCESS;
  }
  return BF_SUCCESS;
}

bf_status_t pkt_mgr_unlock_device(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (bf_drv_is_device_virtual(dev_id)) {
    return BF_SUCCESS;
  }
  pkt_drv_lock_state[dev_id][0] = false;
  pkt_drv_lock_state[dev_id][1] = false;

  return BF_SUCCESS;
}

static bf_status_t pkt_mgr_push_delta_changes(bf_dev_id_t dev_id) {
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (bf_drv_is_device_virtual(dev_id)) {
    /* Nothing to do for a virtual device */
    return BF_SUCCESS;
  }

  if (!pkt_drv_info[dev_id][PKT_MGR_OP_SUBDEV]) {
    return BF_NOT_READY;
  }

  /*
   * Unlock the device as it is still in locked state
   * during hitless HA
   */
  pkt_mgr_unlock_device(dev_id);

  return BF_SUCCESS;
}

static bf_status_t pkt_mgr_disable_cpu_pkt_path(bf_dev_id_t dev_id) {
  bf_subdev_id_t subdev_id;
  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  for (subdev_id = 0; subdev_id < (int)(pkt_drv_info[dev_id][0]->num_subdev);
       subdev_id++) {
    // Mark cpu pkt path enabled
    pkt_drv_info[dev_id][subdev_id]->en_cpu_pkt = false;
  }
  return BF_SUCCESS;
}

static bf_status_t pkt_mgr_enable_cpu_pkt_path(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  bf_subdev_id_t subdev_id;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= PKT_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  if (bf_drv_is_device_virtual(dev_id)) {
    return BF_SUCCESS;
  }

  if (!pkt_drv_info[dev_id][0]) {
    return BF_NOT_READY;
  }

  for (subdev_id = 0; subdev_id < (int)(pkt_drv_info[dev_id][0]->num_subdev);
       subdev_id++) {
    if (bf_pkt_is_locked(dev_id, subdev_id)) {
      /* This should never happen */
      LOG_ERROR(
          "Device %d subdev %d in locked state while enabling CPU pkt path",
          dev_id,
          subdev_id);
      return -1;
    }

    /*
     * Core reset done during fast reconfig and CPU path has to be enabled
     * strictly in following order -
     *   1. Ebuf programming
     *   2. PGR programming
     *   3. TBUS rx_en
     *
     * Steps 1 and 2 would be done by pipe manager and DMA push would be
     * done during pipe manager unlock device. But it is not guaranteed
     * that the HW writes would have happened for sure during pkt mgr unlock
     * device though pkt manager sequence is after pipe manager.
     *
     * During enable_input_packets phase after device unlock, pipe manager
     * waits for DMA completion and hence HW writes for steps 1 and 2 would
     * have gone through by now. So, it is safe for pkt manager to init CPU
     * packet path at this point and do step 3.
     */
    status = pkt_mgr_init_cpu_path(
        dev_id, subdev_id, pkt_drv_info[dev_id][subdev_id]->dma_info);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "Unable to init cpu packet path for device %d subdev %d, sts %s (%d)",
          dev_id,
          subdev_id,
          bf_err_str(status),
          status);
      return status;
    }
  }
  return BF_SUCCESS;
}

bf_status_t bf_pkt_init(void) {
  bf_status_t sts;
  bf_drv_client_handle_t bf_drv_hdl;

  LOG_TRACE("Entering %s", __func__);

  if (pkt_mgr_init_pkt_queue(BF_PKT_CNT) != PKT_SUCCESS) {
    return BF_NO_SYS_RESOURCES;
  }

  sts = bf_drv_register("pkt-mgr", &bf_drv_hdl);
  bf_sys_assert(sts == BF_SUCCESS);
  bf_drv_client_callbacks_t callbacks = {0};
  callbacks.pkt_mgr_dev_add = pkt_mgr_add_device;
  callbacks.device_add = pkt_mgr_add_device_cold_boot;
  callbacks.device_del = pkt_mgr_remove_device;

  callbacks.lock = pkt_mgr_lock_device;
  callbacks.create_dma = pkt_mgr_create_dma;
  callbacks.core_reset = pkt_mgr_core_reset;
  callbacks.unlock_reprogram_core = pkt_mgr_unlock_device;
  callbacks.enable_input_pkts = pkt_mgr_enable_cpu_pkt_path;
  callbacks.disable_input_pkts = pkt_mgr_disable_cpu_pkt_path;
  callbacks.push_delta_changes = pkt_mgr_push_delta_changes;

  bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_2);

  LOG_TRACE("Exiting %s", __func__);
  return BF_SUCCESS;
}

/* for TOFINO3, pkt DMA is configured only for subdevice=0 */
bf_dma_info_t *pkt_mgr_get_dma_info(bf_dev_id_t id, bf_subdev_id_t subdev_id) {
  if (id >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return NULL;
  }
  return pkt_drv_info[id][subdev_id]->dma_info;
}

bool bf_pkt_is_locked(bf_dev_id_t dev_id, bf_subdev_id_t subdev_id) {
  if (dev_id >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    LOG_ERROR(
        "bad dev_id %d subdev_id %d in bf_pkt_is_locked\n", dev_id, subdev_id);
    return true; /* pretend as locked */
  }
  return pkt_drv_lock_state[dev_id][subdev_id];
}
