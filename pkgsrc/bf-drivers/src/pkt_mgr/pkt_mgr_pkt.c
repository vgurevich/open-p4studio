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
 * @file pkt_mgr_pkt.c
 * @date
 *
 * Implementation of PCIe packet driver packet interface
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
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>

/*
 * packets are pre allocated.
 * pointers to free packets are maintained in a LIFO queue. The queue is
 * initialized with pointers to pre allocated packets (free at init time).
 * Pointer to a free packet is pulled from the queue head position. queue head
 * is incremented.
 * Pointer to a free packet is pushed to the queue head position. queue head is
 * decremented. Use __sync_val_compare_and_swap() to implement lock-free atomici
 * * ty.
 */

//  PKT queue global variables
static bf_pkt *bf_pkt_pool;         // array of bf_pkt(s)
static bf_pkt **bf_pkt_queue;       // queue of pointers to bf_pkt
static volatile int bf_queue_head;  // current offset into queue of free pkt
static int bf_queue_max;            // depth of the queue
static volatile int bf_pkt_q_gate;  // gate to ensure single access to queue
extern pkt_drv_info_t
    *pkt_drv_info[PKT_MGR_NUM_DEVICES][PKT_MGR_NUM_SUBDEVICES];

pkt_status_t pkt_mgr_init_pkt_queue(int pkt_count) {
  int i;

  // allocate bf_pkt(s)
  bf_pkt_pool = bf_sys_calloc(pkt_count, sizeof(bf_pkt));
  if (!bf_pkt_pool) {
    return PKT_NO_SYS_RESOURCES;
  }
  // allocate a queue of bf_pkt pointers
  bf_pkt_queue = bf_sys_malloc(pkt_count * sizeof(bf_pkt *));
  if (!bf_pkt_queue) {
    bf_sys_free(bf_pkt_pool);
    return PKT_NO_SYS_RESOURCES;
  }

  // initialize the queue elements
  bf_queue_head = 0;
  bf_queue_max = pkt_count;
  bf_pkt_q_gate = 0;
  for (i = 0; i < pkt_count; i++) {
    bf_pkt_queue[i] = &bf_pkt_pool[i];
  }
  return PKT_SUCCESS;
}

void pkt_mgr_uninit_pkt_queue(void) {
  if (bf_pkt_queue) {
    bf_sys_free(bf_pkt_queue);
    bf_pkt_queue = NULL;
  }
  if (bf_pkt_pool) {
    bf_sys_free(bf_pkt_pool);
    bf_pkt_pool = NULL;
  }
  bf_queue_head = 0;
  bf_queue_max = 0;
  bf_pkt_q_gate = 0;
}

int bf_pkt_alloc_ext_buf(bf_dev_id_t id,
                         bf_subdev_id_t subdev_id,
                         bf_pkt **pkt,
                         size_t size,
                         bf_dma_type_t dr,
                         uint8_t *buf,
                         bf_sys_dma_pool_handle_t hndl) {
  int err = 0;

  if (id >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return -1;
  }

  do {
  } while (__sync_bool_compare_and_swap(&bf_pkt_q_gate, 0, 1) == 0);

  if (bf_queue_head >= bf_queue_max) {
    pkt_drv_info[id][subdev_id]->alloc_stat.pkt_alloc_err++;
    err = -1;
  } else {
    pkt_drv_info[id][subdev_id]->alloc_stat.pkt_alloc_ok++;
    *pkt = bf_pkt_queue[bf_queue_head++];
  }

  __sync_bool_compare_and_swap(&bf_pkt_q_gate, 1, 0);

  if (err == 0) {
    bf_phys_addr_t phys_addr;
    if (bf_sys_dma_get_phy_addr_from_pool(hndl, buf, &phys_addr)) {
      bf_pkt_free_ext_buf(id, subdev_id, *pkt);
      return -1;
    }
    bf_pkt_set_pkt_data(*pkt, buf);
    bf_pkt_set_pkt_dma_pool_handle(*pkt, hndl);
    bf_pkt_set_phys_pkt_data(*pkt, phys_addr);
    bf_pkt_set_pkt_size(*pkt, size);
    bf_pkt_set_pkt_dev(*pkt, id);
    bf_pkt_set_pkt_subdev(*pkt, subdev_id);
    bf_pkt_set_pkt_dr(*pkt, dr);
    bf_pkt_set_nextseg(*pkt, NULL);
  }
  return err;
}

int bf_pkt_free_ext_buf(bf_dev_id_t id, bf_subdev_id_t subdev_id, bf_pkt *pkt) {
  int err = 0;

  if (id >= PKT_MGR_NUM_DEVICES || subdev_id >= PKT_MGR_NUM_SUBDEVICES) {
    return -1;
  }

  bf_pkt_set_pkt_data(pkt, NULL);
  bf_pkt_set_pkt_size(pkt, 0);

  do {
  } while (__sync_bool_compare_and_swap(&bf_pkt_q_gate, 0, 1) == 0);

  if (bf_queue_head == 0 || bf_queue_head > bf_queue_max) {
    pkt_drv_info[id][subdev_id]->alloc_stat.pkt_free_err++;
    err = -1;
  } else {
    pkt_drv_info[id][subdev_id]->alloc_stat.pkt_free_ok++;
    bf_queue_head--;
    bf_pkt_queue[bf_queue_head] = pkt;
  }

  __sync_bool_compare_and_swap(&bf_pkt_q_gate, 1, 0);
  return err;
}

int bf_pkt_alloc(bf_dev_id_t id, bf_pkt **pkt, size_t size, bf_dma_type_t dr) {
  void *dma_pool;
  void *vaddr;
  bf_phys_addr_t paddr;
  size_t buf_size, temp_size;
  bf_pkt *first_pkt = NULL;
  bf_pkt *tmp_pkt = NULL;
  bf_pkt *cur_pkt = NULL;
  bf_subdev_id_t subdev_id = 0;

  *pkt = NULL;

  if (id >= PKT_MGR_NUM_DEVICES) {
    return -1;
  }
  // Check that DR is the tx_ring for now
  if (dr < BF_DMA_CPU_PKT_TRANSMIT_0 || dr > BF_DMA_CPU_PKT_TRANSMIT_3) {
    if (!bf_pcipkt_subdev1_is_en()) {
      return -1;
    } else {
      if (pkt_drv_info[id][0]->dev_family != BF_DEV_FAMILY_TOFINO3 ||
          dr > (BF_DMA_CPU_PKT_TRANSMIT_3 + BF_PKT_TX_RING_MAX)) {
        return -1;
      } else {
        dr = dr - BF_PKT_TX_RING_MAX;
        subdev_id = 1;
      }
    }
  }
  if (size <= 0 || size > BF_PKT_MAX_SIZE) {
    LOG_ERROR("error allocating size %zd dma buffer\n", size);
    return -1;
  }
  dma_pool = bf_dma_get_buf_pool_hndl(pkt_mgr_get_dma_info(id, subdev_id), dr);
  if (!dma_pool) {
    return -1;
  }
  buf_size = bf_dma_get_buf_size(pkt_mgr_get_dma_info(id, subdev_id), dr);
  do {
    if (bf_sys_dma_alloc(
            (bf_sys_dma_pool_handle_t)dma_pool, buf_size, &vaddr, &paddr) !=
        0) {
      pkt_drv_info[id][subdev_id]->alloc_stat.buf_alloc_err++;
      LOG_DBG("error tx_ring %d dma buff alloc\n", dr);
      return -1;
    }
    pkt_drv_info[id][subdev_id]->alloc_stat.buf_alloc_ok++;
    if (size <= buf_size) {
      temp_size = size;
      size = 0;
    } else {
      temp_size = buf_size;
      size -= buf_size;
    }
    // and point the bf_pkt to it
    if (bf_pkt_alloc_ext_buf(id,
                             subdev_id,
                             &cur_pkt,
                             temp_size,
                             dr,
                             vaddr,
                             (bf_sys_dma_pool_handle_t)dma_pool) != 0) {
      // free the pool buffer when packet could not be allocated
      bf_sys_dma_free((bf_sys_dma_pool_handle_t)dma_pool, vaddr);
      // free up buffers and packets if allocated in the chain
      cur_pkt = first_pkt;
      while (cur_pkt) {
        bf_sys_dma_free((bf_sys_dma_pool_handle_t)dma_pool,
                        bf_pkt_get_pkt_data(cur_pkt));
        tmp_pkt = bf_pkt_get_nextseg(cur_pkt);
        bf_pkt_free_ext_buf(id, subdev_id, cur_pkt);
        cur_pkt = tmp_pkt;
      }
      return -1;
    }
    if (!first_pkt) {
      first_pkt = cur_pkt;
      tmp_pkt = first_pkt;
    } else {
      bf_pkt_set_nextseg(tmp_pkt, cur_pkt);
      tmp_pkt = cur_pkt;
    }
  } while (size);
  *pkt = first_pkt;
  return 0;
}

int bf_pkt_free(bf_dev_id_t id, bf_pkt *pkt) {
  void *dma_pool;
  bf_pkt *tmp_pkt;
  bf_dma_type_t dr;
  uint8_t *buf;
  size_t buf_size;
  int is_tx_pkt;
  int err = 0;
  bf_dma_addr_t dma_addr;
  bf_phys_addr_t phys_addr;
  bf_subdev_id_t subdev_id;

  if (!pkt) {
    return -1;
  }
  if (id >= PKT_MGR_NUM_DEVICES) {
    return -1;
  }
  dr = bf_pkt_get_pkt_dr(pkt);
  subdev_id = bf_pkt_get_pkt_subdev(pkt);
  dma_pool = bf_dma_get_buf_pool_hndl(pkt_mgr_get_dma_info(id, subdev_id), dr);
  if (!dma_pool) {
    LOG_ERROR("bad dma pool for chip %d subdev %d dr %d", id, subdev_id, dr);
    return -1;
  }
  if (dr >= BF_DMA_CPU_PKT_RECEIVE_0 && dr <= BF_DMA_CPU_PKT_RECEIVE_7) {
    is_tx_pkt = 0;
  } else if (dr >= BF_DMA_CPU_PKT_TRANSMIT_0 &&
             dr <= BF_DMA_CPU_PKT_TRANSMIT_3) {
    is_tx_pkt = 1;
  } else {
    // this must never happen
    bf_sys_assert(0);
    return -1;
  }

  while (pkt) {
    buf = bf_pkt_get_pkt_data(pkt);
    phys_addr = bf_pkt_get_phys_pkt_data(pkt);
    if (is_tx_pkt == 1) {
      /* Unmap the DMA address of the buffer so that it is reclaimed
         by the device */
      if ((bf_sys_dma_unmap(
              dma_pool, buf, bf_pkt_get_pkt_size(pkt), BF_DMA_FROM_CPU)) != 0) {
        LOG_ERROR(
            "Unable to unmap DMA buffer %p at %s:%d", buf, __func__, __LINE__);
      }
      /* move it back to free pool iif it belongs to tx DR */
      bf_sys_dma_free(dma_pool, buf);
      pkt_drv_info[id][subdev_id]->alloc_stat.buf_free_ok++;
    } else {
      /* else if buff belongs to rx DR, recycle it */
      /* recover the original size of the buffer */
      buf_size = bf_dma_get_buf_size(pkt_mgr_get_dma_info(id, subdev_id), dr);
      int push_cnt = 0;
      bool dr_push_err = false;
      do {
        /* Map the virtual address of the buffer to the DMA address before it is
           again
           pushed back into the free memory DR */
        if (bf_sys_dma_map(
                dma_pool, buf, phys_addr, buf_size, &dma_addr, BF_DMA_TO_CPU) !=
            0) {
          LOG_ERROR(
              "Unable to map DMA buffer %p at %s:%d", buf, __func__, __LINE__);
          return -1;
        }
        if ((err = lld_subdev_push_fm(
                 id,
                 subdev_id,
                 lld_dr_fm_pkt_0 + dr - BF_DMA_CPU_PKT_RECEIVE_0,
                 dma_addr,
                 buf_size)) != LLD_OK) {
          ++push_cnt;
          if (bf_sys_dma_unmap(dma_pool, buf, buf_size, BF_DMA_TO_CPU) != 0) {
            LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                      buf,
                      __func__,
                      __LINE__);
          }
          if (!dr_push_err) {
            LOG_ERROR(
                "Return pkt FM error %d dev %d subdev %d rx_ring %d addr "
                "0x%" PRIx64 " size %zu\n",
                err,
                id,
                subdev_id,
                dr - BF_DMA_CPU_PKT_RECEIVE_0,
                dma_addr,
                buf_size);
            dr_push_err = true;
          }
          // continue freeing the pkt depite errors
        } else if (dr_push_err) {
          LOG_ERROR(
              "%s:%d Retry pushing rx_ring %d free memory to device %d subdev "
              "%d"
              "successful, try %d",
              __func__,
              __LINE__,
              dr,
              id,
              subdev_id,
              push_cnt);
        }
      } while (err == LLD_ERR_DR_FULL && push_cnt < 1000);
      if (lld_dr_start(
              id, subdev_id, lld_dr_fm_pkt_0 + dr - BF_DMA_CPU_PKT_RECEIVE_0) !=
          LLD_OK) {
        LOG_ERROR("error rx_ring %d lld_dr_start\n", dr);
        err |= -1;  // continue freeing the pkt depite errors
      }
    }
    // now free up the pkt
    tmp_pkt = bf_pkt_get_nextseg(pkt);
    bf_pkt_free_ext_buf(id, subdev_id, pkt);
    pkt = tmp_pkt;
  }

  return err;
}

int pkt_mgr_free_dev_pkts(bf_dev_id_t id) {
  int i = 0;
  bf_dma_type_t dr;
  uint8_t *buf;
  bf_dev_id_t dev_id;
  void *dma_pool;
  bf_pkt *pkt;

  // free all pkts associated with the device
  for (i = 0; i < bf_queue_max; i++) {
    pkt = bf_pkt_queue[i];
    dev_id = bf_pkt_get_pkt_dev(pkt);
    buf = bf_pkt_get_pkt_data(pkt);
    if (buf && (dev_id == id)) {
      dr = bf_pkt_get_pkt_dr(pkt);
      dma_pool = bf_dma_get_buf_pool_hndl(pkt_mgr_get_dma_info(id, 0), dr);
      bf_pkt_free(id, pkt);
      /* move it back to free pool as it belongs to tx DR */
      if (dr >= BF_DMA_CPU_PKT_RECEIVE_0 && dr <= BF_DMA_CPU_PKT_RECEIVE_7) {
        bf_sys_dma_free(dma_pool, buf);
      }
    }
  }

  return PKT_SUCCESS;
}

int bf_pkt_data_copy(bf_pkt *pkt, const uint8_t *pkt_buf, uint16_t size) {
  bf_pkt *cur_pkt;
  uint16_t cur_pkt_size;
  uint8_t *cur_pkt_data;

  if (!pkt_buf) {
    LOG_ERROR("error packet data copy : pkt_buf is NULL \n");
    return -1;
  }
  cur_pkt = pkt;
  while (cur_pkt) {
    cur_pkt_size = bf_pkt_get_pkt_size(cur_pkt);
    cur_pkt_data = bf_pkt_get_pkt_data(cur_pkt);
    if (size <= cur_pkt_size) {
      memcpy(cur_pkt_data, pkt_buf, size);
      return 0;
    } else {
      memcpy(cur_pkt_data, pkt_buf, cur_pkt_size);
      size -= cur_pkt_size;
      pkt_buf += cur_pkt_size;
      cur_pkt = bf_pkt_get_nextseg(cur_pkt);
    }
  }
  return -1;
}
