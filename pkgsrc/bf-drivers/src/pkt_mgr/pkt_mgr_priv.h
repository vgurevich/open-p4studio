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
 * @file pkt_mgr_intf.h
 * @date
 *
 */
#ifndef _PKT_MGR_PRIV_H
#define _PKT_MGR_PRIV_H

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

#include <lld/lld_err.h>
#include <lld/lld_sku.h>

/*!
 * Enum pkt driver's tx and rx packet session state
 */
typedef enum {
  BF_PKT_INIT = 0,
  BF_PKT_START,
  BF_PKT_MIDDLE,
  PF_PKT_END,
} bf_pkt_st;

/*!
 * structure pkt driver's tx and rx session context
 */
typedef struct {
  bf_pkt_st pkt_state;
  bf_pkt *pkt_last;
  bf_pkt *pkt_first;
  void *dma_pool;
  unsigned int pool_buf_size;
  unsigned int pool_buf_cnt;
  bf_sys_mutex_t ring_mtx;
} bf_pkt_ctx_t;

/*!
 * structure for pkt driver's rx DMA buffers
 */
typedef struct bf_pkt_buf_info_t {
  struct bf_pkt_buf_info_t *next;
  void *vaddr;
  bf_phys_addr_t phy_addr;
} bf_pkt_buf_info_t;

/*!
 * structure for pkt driver's packet stats
 */
typedef struct bf_pkt_mgr_alloc_stat_s {
  uint64_t pkt_alloc_ok;
  uint64_t pkt_free_ok;
  uint64_t buf_alloc_ok;
  uint64_t buf_free_ok;
  uint64_t pkt_alloc_err;
  uint64_t pkt_free_err;
  uint64_t buf_alloc_err;
  uint64_t buf_free_err;
} bf_pkt_mgr_alloc_stat_t;

/* RX packet stats */
typedef struct bf_pkt_mgr_rx_stat_s {
  uint64_t pkt_ok;
  uint64_t pkt_addr_err;
  uint64_t pkt_param_err;
  uint64_t pkt_alloc_err;
  uint64_t pkt_free_err;
  uint64_t pkt_assembly_err;
  uint64_t pkt_no_hndl_err;
} bf_pkt_mgr_rx_stat_t;

/* TX packet stats */
typedef struct bf_pkt_mgr_tx_stat_s {
  uint64_t pkt_ok;
  uint64_t pkt_compl_ok;
  uint64_t pkt_drop;
  uint64_t pkt_param_err;
  uint64_t pkt_compl_type_err;
  uint64_t pkt_compl_assembly_err;
  uint64_t pkt_no_hndl_err;
} bf_pkt_mgr_tx_stat_t;

#define PKT_MGR_OP_SUBDEV 0 /* operational sub device index for TOFINO3 */

/*!
 * structure for pkt driver's rx ring
 */
typedef struct {
  bf_pkt_buf_info_t *bufs;
} bf_pkt_buffers_t;

/*!
 * Minimum TX packet size - packets less than this size would
 * be padded
 */
#define PKT_MGR_PCI_MIN_PKT_SIZE 60
#define PKT_MGR_PCI_MIN_PKT_SIZE_TOF_A0 80

/*!
 * structure for pkt driver
 */
typedef struct {
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  bf_dev_family_t dev_family;
  bf_sku_chip_part_rev_t part_rev_number;  // Chip part revision number
  uint32_t pci_min_pkt_size;  // Packets less than this size would be padded
  uint32_t num_subdev;        // number of subdevices
  bf_dma_info_t *dma_info;
  bool warm_start;
  bool inited;
  bool en_cpu_pkt; /* flag to mark cpu pkt path as up or down */
  bf_pkt_rx_callback rx_cb_fn[BF_PKT_RX_RING_MAX];
  void *rx_cookie[BF_PKT_RX_RING_MAX];
  bf_pkt_tx_done_notif_cb tx_nt_fn[BF_PKT_TX_RING_MAX];
  void *tx_cookie[BF_PKT_TX_RING_MAX];
  bf_pkt_ctx_t rx_pkt_ctx[BF_PKT_RX_RING_MAX];
  bf_pkt_ctx_t tx_pkt_ctx[BF_PKT_TX_RING_MAX];
  bf_pkt_buffers_t rx_bufs[BF_PKT_RX_RING_MAX];
  /* statistics */
  bf_pkt_mgr_alloc_stat_t alloc_stat;
  bf_pkt_mgr_rx_stat_t rx_stat[BF_PKT_RX_RING_MAX];
  bf_pkt_mgr_tx_stat_t tx_stat[BF_PKT_TX_RING_MAX];
} pkt_drv_info_t;

/* define  to test debug PCI packet path over subdevice = 1 */
#define ENABLE_PCIPKT_SUBDEV_1 1

/**
 *  Is pci packet path over subdevice != 0 enabled
 *  (by default disabled, enabled only for test purpose)
 *
 */
static inline bool bf_pcipkt_subdev1_is_en(void) {
#ifdef ENABLE_PCIPKT_SUBDEV_1
  return true;
#else
  return false;
#endif
}

/**
 *  Pkt RX callback
 *
 *  @param  chip
 *    chip id
 *  @param subdev_id
 *    subdevice id within the chip
 *  @param  data_sz
 *    DMAed data size
 *  @param dma_addr
 *    address of DMAed data
 *  @param s
 *    start bit
 *  @param e
 *    end bit
 *  @param cos
 *    rx dr index
 *  @return
 *    none
 */
void pkt_mgr_rx_pkt_cb(bf_dev_id_t chip,
                       bf_subdev_id_t subdev_id,
                       int data_sz,
                       bf_dma_addr_t dma_addr,
                       int s,
                       int e,
                       int cos);

/**
 *  Pkt TX completion callback
 *
 *  @param  chip
 *    chip id
 *  @param subdev_id
 *    subdevice id within the chip
 *  @param dr
 *    tx dr index
 *  @param  data_sz_or_ts
 *    DMAed data size or timestamp
 *  @param attr
 *    attribute as in TX completion message
 *  @param status
 *    status of TX pkt DMA
 *  @param type
 *    tx completion message type
 *  @param msg_id
 *    msg_id as in tx-pkt message
 *  @param s
 *    start bit
 *  @param e
 *    end bit
 *  @return
 *    none
 */
void pkt_mgr_tx_completion_cb(bf_dev_id_t chip,
                              bf_subdev_id_t subdev_id,
                              bf_dma_dr_id_t dr,
                              uint64_t data_sz_or_ts,
                              uint32_t attr,
                              uint32_t status,
                              uint32_t type,
                              uint64_t msg_id,
                              int s,
                              int e);

/**
 * returns dma_info pointer
 *
 * @param  chip
 *  chip id
 *  @param subdev_id
 *   subdevice id within the chip
 * @return
 *  dma_info
 */
bf_dma_info_t *pkt_mgr_get_dma_info(bf_dev_id_t chip, bf_subdev_id_t subdev_id);

/**
 * returns state of cpu_pkt_path
 *
 * @param  chip
 *  chip id
 *  @param subdev_id
 *    subdevice id within the chip
 * @return
 *  true if cpu_pkt path is enabled
 *  false if cpu pkt path is disabled
 */
bool pkt_mgr_is_cpu_pkt_en(bf_dev_id_t chip, bf_subdev_id_t subdev_id);

/**
 * allocate a packet and attach an already available buffer to it
 *
 * @param id
 *   chip id
 *  @param subdev_id
 *   subdevice id within the chip
 * @param pkt
 *   bf packet pointer
 * @param size
 *   buffer size
 * @param dr
 *   DR pool to use to allocate buffer (bf dma type)
 * @param buf
 *   buffer to be attached to allocated packet
 * @param hndl
 *   handle of the dma memory pool
 * @return
 *   0 on success, -1 on failure
 */
int bf_pkt_alloc_ext_buf(bf_dev_id_t id,
                         bf_subdev_id_t subdev_id,
                         bf_pkt **pkt,
                         size_t size,
                         bf_dma_type_t dr,
                         uint8_t *buf,
                         bf_sys_dma_pool_handle_t hndl);

/**
 * free a packet without freeing its buffer
 *
 * @param id
 *   chip id
 *  @param subdev_id
 *   subdevice id within the chip
 * @param pkt
 *   bf packet
 * @return
 *   0 on success, -1 on failure
 */
int bf_pkt_free_ext_buf(bf_dev_id_t id, bf_subdev_id_t subdev_id, bf_pkt *pkt);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
