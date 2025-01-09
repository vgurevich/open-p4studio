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


#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sched.h>

#include <dvm/bf_drv_intf.h>
#include <tofino_regs/pipe_top_level.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/tofino_defs.h>
#include <lld/lld_err.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_reg_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_subdev_dr_if.h>

#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_log.h"
#include "mc_mgr_drv.h"
#include <lld/lld_tof_addr_conversion.h>
#include <lld/lld_tof2_addr_conversion.h>
#include <lld/lld_tof3_addr_conversion.h>
#include <mc_mgr/mc_mgr_shared_dr.h>

static int buf_pool_init(bf_dev_id_t dev, bf_dma_info_t *dma_info);
static void buf_pool_destroy(bf_dev_id_t dev);
static void buf_pool_zero(bf_dev_id_t dev);
static void mc_mgr_drv_free_buf(mc_mgr_drv_buf_t *b);
static mc_mgr_drv_buf_t *mc_mgr_drv_get_buf(int sid,
                                            bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev);

static void mc_mgr_drv_completion_cb(bf_dev_id_t chip,
                                     bf_subdev_id_t subdev_id,
                                     bf_dma_dr_id_t dr,
                                     uint64_t data_sz_or_ts,
                                     uint32_t attr,
                                     uint32_t status,
                                     uint32_t type,
                                     uint64_t msg_id,
                                     int s,
                                     int e);

bf_status_t mc_mgr_drv_init() { return BF_SUCCESS; }

bf_status_t mc_mgr_drv_init_dev(bf_dev_id_t dev, bf_dma_info_t *dma_info) {
  int x = buf_pool_init(dev, dma_info);
  if (x) return BF_NO_SYS_RESOURCES;

  dr_completion_callback_fn cmplt_cb;
  cmplt_cb = (dr_completion_callback_fn)mc_mgr_drv_completion_cb;

  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      x = lld_dr_lock_required(dev, lld_dr_tx_que_write_list);
      if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
      x = lld_dr_lock_required(dev, lld_dr_cmp_que_write_list);
      if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
      if (mcmgr_tm_register_completion_cb(
              dev, lld_dr_cmp_que_write_list, cmplt_cb, BF_MC_DMA_MSG_ID)) {
        LOG_ERROR("Completion callback registration fails, dev %u at %s:%d",
                  dev,
                  __func__,
                  __LINE__);
        return BF_NOT_READY;
      }
      return BF_SUCCESS;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev);
           subdev++) {
        x = lld_subdev_dr_lock_required(
            dev, subdev, lld_dr_tx_que_write_list_1);
        if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
        x = lld_subdev_dr_lock_required(
            dev, subdev, lld_dr_cmp_que_write_list_1);
        if (x != LLD_OK) return BF_NO_SYS_RESOURCES;
        x = lld_register_completion_callback(
            dev, subdev, lld_dr_cmp_que_write_list_1, cmplt_cb);
        if (x < 0) {
          LOG_ERROR(
              "Completion callback registration fails %d, dev %u,"
              " subdev %u at %s:%d",
              x,
              dev,
              subdev,
              __func__,
              __LINE__);
          return BF_UNEXPECTED;
        }
      }
      return BF_SUCCESS;
    default:
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
  }
}
void mc_mgr_drv_remove_dev(bf_dev_id_t dev) { buf_pool_destroy(dev); }

void mc_mgr_drv_warm_init_quick(bf_dev_id_t dev) { buf_pool_zero(dev); }

static int buf_pool_init(bf_dev_id_t dev, bf_dma_info_t *dma_info) {
  if (!mc_mgr_is_device_locked(dev)) {
    // Enable all the DRs
    mc_mgr_enable_all_dr(dev);
  }

  bf_dma_buf_info_t *bi;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      bi = &dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST];
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      bi = &dma_info->dma_buff_info[BF_DMA_TM_WRITE_LIST_1];
      break;
    default:
      MC_MGR_DBGCHK(0);
      return -1;
  }
  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev);
  bp->pool = bi->dma_buf_pool_handle;
  bp->buf_sz = bi->dma_buf_size;
  bp->buf_cnt = bi->dma_buf_cnt;
  bp->used = MC_MGR_CALLOC(bi->dma_buf_cnt, sizeof(mc_mgr_drv_buf_t));
  if (!bp->used) {
    LOG_ERROR("Failed to allocate memory for mcast buf pool, dev %d", dev);
    MC_MGR_DBGCHK(bp->used);
    MC_MGR_MEMSET(bp, 0, sizeof(struct mc_mgr_drv_buf_pool_t));
    return -1;
  }
  for (unsigned int i = 0; i < bi->dma_buf_cnt; ++i) {
    bf_map_init(&bp->used[i].mgids_updated);
    bf_map_init(&bp->used[i].freed_rdm_addrs);
  }
  MC_MGR_LOCK_INIT(bp->mtx);
  return 0;
}

static void buf_pool_destroy(bf_dev_id_t dev) {
  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev);

  // Disable DR
  mc_mgr_disable_all_dr(dev);

  // TODO - Ensure no thread is running mc_mgr_drv_completion_cb
  unsigned int i;
  for (i = 0; i < bp->buf_cnt; ++i) {
    mc_mgr_drv_buf_t *b = &bp->used[i];
    if (!b->addr) continue;

    if (b->buf_pushed) {
      if (bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_FROM_CPU)) {
        LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                  b->addr,
                  __func__,
                  __LINE__);
      }
    }
    bf_sys_dma_free(b->pool, b->addr);
  }
  for (i = 0; i < bp->buf_cnt; ++i) {
    bf_map_destroy(&bp->used[i].mgids_updated);
    bf_map_destroy(&bp->used[i].freed_rdm_addrs);
  }
  MC_MGR_FREE(bp->used);
  MC_MGR_LOCK_DEL(bp->mtx);
  MC_MGR_MEMSET(bp, 0, sizeof(struct mc_mgr_drv_buf_pool_t));
}

static void buf_pool_zero(bf_dev_id_t dev) {
  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev);

  unsigned int i;
  for (i = 0; i < bp->buf_cnt; ++i) {
    mc_mgr_drv_buf_t *b = &bp->used[i];
    if (!b->addr) continue;

    if (b->buf_pushed) {
      if (bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_FROM_CPU)) {
        LOG_ERROR("Unable to unmap DMA buffer %p at %s:%d",
                  b->addr,
                  __func__,
                  __LINE__);
      }
      b->buf_pushed = false;
      b->addr = 0;
    }
  }
  for (i = 0; i < bp->buf_cnt; ++i) {
    bf_map_init(&bp->used[i].mgids_updated);
    bf_map_init(&bp->used[i].freed_rdm_addrs);
  }

  MC_MGR_LOCK(&bp->mtx);
  bp->in_use = 0;
  MC_MGR_UNLOCK(&bp->mtx);
}

static void mc_mgr_drv_completion_cb(bf_dev_id_t dev,
                                     bf_subdev_id_t subdev,
                                     bf_dma_dr_id_t dr,
                                     uint64_t data_sz_or_ts,
                                     uint32_t attr,
                                     uint32_t status,
                                     uint32_t type,
                                     uint64_t msg_id,
                                     int start_bit,
                                     int end_bit) {
  (void)dr;
  (void)data_sz_or_ts;
  (void)attr;
  (void)type;
  (void)subdev;

  int ret;
  if (!start_bit || !end_bit) {
    LOG_ERROR("Invalid DMA completion, ignoring!");
    return;
  }
  if ((msg_id & 0xf) != BF_MC_DMA_MSG_ID) {
    // Not my DMA completion
    return;
  }
  msg_id >>= 4;  // bottom 4 bits identify user of DR

  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev);

  if (status) {
    LOG_ERROR(
        "Write list failed (%d) dev %d, msgId 0x%" PRIx64, status, dev, msg_id);
    MC_MGR_DBGCHK(!status);
  }
  if (msg_id >= bp->buf_cnt) {
    LOG_ERROR("Unknown buffer returned (0x%" PRIx64 ") at %s:%d",
              msg_id,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(msg_id < bp->buf_cnt);
    return;
  }
  mc_mgr_drv_buf_t *b = &bp->used[msg_id];

  /* Need to unmap the buffer before it is freed so that the DMA
     address is reclaimed by the device */
  ret = bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_FROM_CPU);
  if (ret != 0) {
    LOG_ERROR(
        "Unable to unmap DMA buffer %p at %s:%d", b->addr, __func__, __LINE__);
  }

  int num_subdev = mc_mgr_ctx_num_subdevices(dev);
  bool rdm_change_needed = false;
  /* Go through any MGID updates in this buffer and mark them as ready for an
   * RDM change. */
  mc_mgr_ctx_tree_len_lock(dev, subdev);
  unsigned long key;
  void *unused;
  while (BF_MAP_OK == bf_map_get_first_rmv(&b->mgids_updated, &key, &unused)) {
    int mgid = key >> 8;
    int pipe = 0;
    if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
      pipe = key & 0x7;
    } else {
      pipe = key & 0x3;
    }
    struct mc_mgr_tree_size_t *t;
    bf_map_sts_t s =
        bf_map_get(mc_mgr_ctx_tree_len(dev, subdev), mgid, (void *)&t);
    if (BF_MAP_OK != s) continue;
    t += pipe; /* t is an array sized by number of pipes. */
    if (t->buf != b) continue;
    if ((num_subdev == 1) || ((num_subdev > 1) && (subdev == 1))) {
      rdm_change_needed = true;
      mc_mgr_enqueue_mgid_tail_update(dev, pipe, mgid, t->sw_len);
    }
  }
  mc_mgr_ctx_tree_len_unlock(dev, subdev);

  /* Go through the freed RDM addresses in this buffer and add them to one of
   * the freeing lists. */
  int num_to_free = bf_map_count(&b->freed_rdm_addrs);
  for (int i = 0; i < num_to_free; ++i) {
    bf_map_sts_t s = bf_map_get_first_rmv(&b->freed_rdm_addrs, &key, &unused);
    MC_MGR_DBGCHK(BF_MAP_OK == s);
    uint32_t rdm_addr = key;
    if ((num_subdev == 1) || ((num_subdev > 1) && (subdev == 1))) {
      rdm_change_needed = true;
      mc_mgr_rdm_map_free(dev, rdm_addr);
    }
  }
  if (rdm_change_needed) mc_mgr_start_rdm_change_all_pipes(dev, "dma cmplt");

  mc_mgr_drv_free_buf(b);
  MC_MGR_LOCK(&bp->mtx);
  if (bp->in_use) {
    /* Sleep for sometime to let other threads run */
    if ((num_subdev > 1) && (subdev == 1)) {
      bf_sys_usleep(100);
    }
    --bp->in_use;
  }
  MC_MGR_UNLOCK(&bp->mtx);
}

bf_status_t mc_mgr_drv_service_dr(bf_dev_id_t dev_id) {
#ifdef MC_MGR_INLINE_DR_SERVICE
  bf_subdev_id_t subdev = 0;
  /* Update the receive DR to receive buffers back from the hardware. */
  if (MC_MGR_NUM_DEVICES == dev_id) {
    /* Service all devices. */
    for (dev_id = 0; dev_id < MC_MGR_NUM_DEVICES; ++dev_id) {
      if (!mc_mgr_dev_present(dev_id)) continue;
      bf_dma_dr_id_t dr_id;
      switch (mc_mgr_ctx_dev_family(dev_id)) {
        case BF_DEV_FAMILY_TOFINO:
          dr_id = lld_dr_cmp_que_write_list;
          break;
        case BF_DEV_FAMILY_TOFINO2:
        case BF_DEV_FAMILY_TOFINO3:
          dr_id = lld_dr_cmp_que_write_list_1;
          break;
        default:
          MC_MGR_DBGCHK(0);
          return BF_UNEXPECTED;
      }
      for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev_id);
           subdev++) {
        lld_dr_service(dev_id, subdev, dr_id, 5);
      }
    }
  } else {
    if (dev_id < 0 || dev_id >= MC_MGR_NUM_DEVICES) {
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
    }
    if (!mc_mgr_dev_present(dev_id)) return BF_SUCCESS;
    bf_dma_dr_id_t dr_id;
    switch (mc_mgr_ctx_dev_family(dev_id)) {
      case BF_DEV_FAMILY_TOFINO:
        dr_id = lld_dr_cmp_que_write_list;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        dr_id = lld_dr_cmp_que_write_list_1;
        break;
      default:
        MC_MGR_DBGCHK(0);
        return BF_UNEXPECTED;
    }
    for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev_id);
         subdev++) {
      lld_dr_service(dev_id, subdev, dr_id, 5);
    }
  }
#else
  (void)dev_id;
  /* The runner is handling the DRs in the background, just wait for that to
   * happen. */
  sched_yield();
#endif
  return BF_SUCCESS;
}

void mc_mgr_drv_cmplt_operations(int sid, bf_dev_id_t dev_id) {
  if (mc_mgr_ctx_syncing(dev_id)) return;
  if (mc_mgr_in_batch(sid)) {
    /* If the session is in a batch, the complete is a nop. */
    return;
  }
  if (mc_mgr_is_device_locked(dev_id)) return;

  (void)sid;

  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev_id);
  if (!bp) return;
  bool outstanding_buffers = true;
  while (outstanding_buffers) {
    MC_MGR_LOCK(&bp->mtx);
    outstanding_buffers = bp->in_use != 0;
    MC_MGR_UNLOCK(&bp->mtx);
    if (outstanding_buffers) mc_mgr_drv_service_dr(dev_id);
  }
}

static mc_mgr_drv_buf_t *mc_mgr_drv_get_buf(int sid,
                                            bf_dev_id_t dev_id,
                                            bf_subdev_id_t subdev) {
  mc_mgr_drv_buf_t *b = NULL;
  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev_id);

  bool first = true;
  do {
    void *vaddr = NULL;
    bf_phys_addr_t paddr = 0;
    int x = bf_sys_dma_alloc(bp->pool, bp->buf_sz, &vaddr, &paddr);
    if (!x) {
      x = bf_sys_dma_buffer_index(bp->pool, vaddr);
      if (x >= 0) {
        b = &bp->used[x];
        mc_mgr_drv_buf_t *first_buf = &bp->used[0];
        mc_mgr_drv_buf_t *last_buf = &bp->used[bp->buf_cnt - 1];
        MC_MGR_DBGCHK(first_buf <= b && b <= last_buf);

        b->pool = bp->pool;
        b->addr = vaddr;
        b->phys_addr = paddr;
        b->used = 0;
        b->size = bp->buf_sz;
        b->dev = dev_id;
        b->count = 0;
        b->buf_pushed = 0;
        b->msgId = (x << 4) | BF_MC_DMA_MSG_ID;  // OR in DR user identifier.
        b->wr_list_size = 0;
        MC_MGR_DBGCHK(!b->next);
        MC_MGR_DBGCHK(!b->prev);
        return b;
      }
    } else if (first) {
      mc_mgr_drv_wr_list_t *wl = mc_mgr_ctx_wl(dev_id, subdev, sid);
      if (mc_mgr_in_batch(sid) && wl) {
        /* Since the session is batching we need to clear the batching state
         * briefly so we can push the batch. */
        mc_mgr_ctx_in_batch_set(sid, false);
        mc_mgr_drv_wrl_send(sid, false);
        mc_mgr_ctx_in_batch_set(sid, true);
      } else if (wl) {
        mc_mgr_drv_wrl_send(sid, false);
      }
    }

    first = false;
    mc_mgr_drv_service_dr(dev_id);

  } while (!b);
  return NULL;
}
static void mc_mgr_drv_free_buf(mc_mgr_drv_buf_t *b) {
  if (b == NULL) {
    MC_MGR_DBGCHK(0);
    return;
  }
  mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(b->dev);

  mc_mgr_drv_buf_t *first = &bp->used[0];
  mc_mgr_drv_buf_t *last = &bp->used[bp->buf_cnt - 1];
  MC_MGR_DBGCHK(first <= b && b <= last);
  void *vaddr = b->addr;

  b->addr = 0;
  b->phys_addr = 0;
  b->used = 0;
  b->size = 0;
  b->count = 0;
  b->buf_pushed = 0;
  b->msgId = 0;
  b->wr_list_size = 0;
  MC_MGR_DBGCHK(NULL == b->next);
  MC_MGR_DBGCHK(NULL == b->prev);

  if (vaddr) {
    bf_sys_dma_free(bp->pool, vaddr);
  }
}

int mc_mgr_drv_wrl_append(bf_dev_id_t dev,
                          bf_subdev_id_t subdev_id,
                          int sid,
                          int width,
                          uint64_t addr,
                          uint64_t hi,
                          uint64_t lo,
                          const char *where,
                          const int line) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return -1;
  }

  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return -1;
  }

  if (4 != width && 8 != width && 16 != width) {
    MC_MGR_DBGCHK(0);
    return -1;
  }

  /* Must address a 128-bit chunk. */
  MC_MGR_DBGCHK(0 == (addr & 0xF));
  /* Convert from byte address to 128-bit word address. */
  addr >>= 4;

  if (mc_mgr_is_device_locked(dev)) {
    if (!mc_mgr_ctx_rebuilding(dev)) {
      return 0;
    }
  }
  if (mc_mgr_ctx_syncing(dev)) return 0;

  bf_subdev_id_t subdev_cntr = 0, subdev_start = 0, subdev_end = 0;
  if (subdev_id != MC_MGR_DRV_SUBDEV_ID_ALL) {
    MC_MGR_DBGCHK(subdev_id < (int)mc_mgr_ctx_num_subdevices(dev));
    subdev_start = subdev_id;
    subdev_end = subdev_start + 1;
  } else {
    subdev_start = 0;
    subdev_end = (bf_subdev_id_t)mc_mgr_ctx_num_subdevices(dev);
  }

  for (subdev_cntr = subdev_start; subdev_cntr < subdev_end; subdev_cntr++) {
    mc_mgr_drv_wr_list_t *wl = mc_mgr_ctx_wl(
        dev,
        subdev_cntr,
        sid);  // per session we are trying to enqueue the commands.
    mc_mgr_drv_buf_t *b = NULL;
    if (!wl) {
      LOG_ERROR("%s:%d Failed to get drv wr list, device id %d, session id %d",
                where,
                line,
                dev,
                sid);
      MC_MGR_DBGCHK(wl);
      return -1;
    }

    if (!wl->count) {
      /* First entry being added to the list, get a buffer. */
      b = mc_mgr_drv_get_buf(sid, dev, subdev_cntr);
      if (!b) {
        LOG_ERROR("No DMA buffers for write list from %s:%d", where, line);
        MC_MGR_DBGCHK(b);
        return -1;
      } else {
        /* Add the buffer to the write-list. */
        BF_LIST_DLL_AP(wl->bufList, b, next, prev);
        MC_MGR_DBGCHK(wl->bufList->prev == b);
        ++wl->count;
        /* Set the new buffer to use the correct size for this entry. */
        b->wr_list_size = width;
        b->subdev_id = subdev_cntr;
      }
    } else {
      /* Get the last buffer in the list. */
      BF_LIST_DLL_LAST(wl->bufList, b, next, prev);
      if (b == NULL) {
        MC_MGR_DBGCHK(0);
        return -1;
      }
      b->subdev_id = subdev_cntr;
    }

    /* If must be of the same size as the requested write.  If not a new
     * buffer must be used.  It also must have enough room for this entry. */
    if (b->wr_list_size != (unsigned int)width ||
        (unsigned int)(width + 8 + b->used) > b->size) {
      b = mc_mgr_drv_get_buf(sid, dev, subdev_cntr);
      if (!b) {
        LOG_ERROR("No DMA buffers for write list from %s:%d", where, line);
        MC_MGR_DBGCHK(b);
        return -1;
      } else {
        /* Set the new buffer to use the correct size for this entry. */
        b->wr_list_size = width;
        b->subdev_id = subdev_cntr;
        ++wl->count;
      }
      BF_LIST_DLL_AP(wl->bufList, b, next, prev);
      MC_MGR_DBGCHK(wl->bufList->prev == b);
    }
    ++b->count;
    b->addr[b->used++] = (addr >> 0) & 0xFF;
    b->addr[b->used++] = (addr >> 8) & 0xFF;
    b->addr[b->used++] = (addr >> 16) & 0xFF;
    b->addr[b->used++] = (addr >> 24) & 0xFF;
    b->addr[b->used++] = (addr >> 32) & 0xFF;
    b->addr[b->used++] = (addr >> 40) & 0xFF;
    b->addr[b->used++] = (addr >> 48) & 0xFF;
    b->addr[b->used++] = (addr >> 56) & 0xFF;
    b->addr[b->used++] = (lo >> 0) & 0xFF;
    b->addr[b->used++] = (lo >> 8) & 0xFF;
    b->addr[b->used++] = (lo >> 16) & 0xFF;
    b->addr[b->used++] = (lo >> 24) & 0xFF;
    if (4 == width) return 0;
    b->addr[b->used++] = (lo >> 32) & 0xFF;
    b->addr[b->used++] = (lo >> 40) & 0xFF;
    b->addr[b->used++] = (lo >> 48) & 0xFF;
    b->addr[b->used++] = (lo >> 56) & 0xFF;
    if (8 == width) return 0;
    b->addr[b->used++] = (hi >> 0) & 0xFF;
    b->addr[b->used++] = (hi >> 8) & 0xFF;
    b->addr[b->used++] = (hi >> 16) & 0xFF;
    b->addr[b->used++] = (hi >> 24) & 0xFF;
    b->addr[b->used++] = (hi >> 32) & 0xFF;
    b->addr[b->used++] = (hi >> 40) & 0xFF;
    b->addr[b->used++] = (hi >> 48) & 0xFF;
    b->addr[b->used++] = (hi >> 56) & 0xFF;
  }
  return 0;
}

int mc_mgr_drv_wrl_append_reg(bf_dev_id_t dev,
                              int sid,
                              uint32_t addr,
                              uint32_t data,
                              const char *where,
                              const int line) {
  if (mc_mgr_is_device_locked(dev)) {
    if (!mc_mgr_ctx_rebuilding(dev)) {
      return 0;
    }
  }
  if (mc_mgr_ctx_syncing(dev)) return 0;
  /* Always use a 16B write.  It wastes memory when writting registers but
   * prevents a write-list from being split into multiple DMA buffers just
   * because the width is changing because there is a mix of register and
   * memory writes. */
  int width = 16;
  uint64_t data_hi = 0;
  uint64_t data_lo = data;
  uint64_t full_addr = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      full_addr = tof_dir_to_indir_dev_sel(addr);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      full_addr = tof2_dir_to_indir_dev_sel(addr);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      full_addr = tof3_dir_to_indir_dev_sel(addr);
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  /* Due to a bug in the CSR compile tool the memory addresses are actually
   * byte (?) addresses so they are shifted down to a proper 16B word address
   * in the wrl_append function.  The address conversion above for registers
   * does not have this problem so shift the address up so it can be shifted
   * back down in wrl_append. */
  full_addr = full_addr << 4;
  return mc_mgr_drv_wrl_append(dev,
                               MC_MGR_DRV_SUBDEV_ID_ALL,
                               sid,
                               width,
                               full_addr,
                               data_hi,
                               data_lo,
                               where,
                               line);
}

static bf_status_t mc_mgr_dma_start(bf_dev_id_t dev_id, bf_subdev_id_t subdev) {
  bf_status_t status = BF_SUCCESS;

  if (mc_mgr_ctx_syncing(dev_id)) return BF_SUCCESS;

  /* if device locked, dont push */
  if (mc_mgr_is_device_locked(dev_id)) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  bf_dma_dr_id_t dr_id;
  switch (mc_mgr_ctx_dev_family(dev_id)) {
    case BF_DEV_FAMILY_TOFINO:
      dr_id = lld_dr_tx_que_write_list;
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      dr_id = lld_dr_tx_que_write_list_1;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
  }

  int x = lld_dr_start(dev_id, subdev, dr_id);
  if (LLD_OK != x) {
    LOG_ERROR(
        "Error starting mcast DMA on dev %d subdev %d: %d", dev_id, subdev, x);
    status = BF_HW_COMM_FAIL;
    MC_MGR_DBGCHK(LLD_OK == x);
  }

  return status;
}

bf_status_t mc_mgr_drv_wrl_send(int sid, bool is_last) {
  int ret = 0;
  int push_cnt[BF_MAX_SUBDEV_COUNT];
  int i;
  bf_dev_id_t dev = 0;
  uint64_t dev_map = 0;
  mc_mgr_drv_wr_list_t *wl = NULL;
  bf_dma_addr_t dma_addr;
  bf_subdev_id_t subdev = 0;

  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (mc_mgr_in_batch(sid)) return BF_SUCCESS;

  for (dev = 0; dev < MC_MGR_NUM_DEVICES; ++dev) {
    if (!mc_mgr_dev_present(dev)) continue;
    if (mc_mgr_is_device_locked(dev)) continue;
    if (mc_mgr_ctx_syncing(dev)) continue;
    mc_mgr_drv_buf_pool_t *bp = mc_mgr_ctx_dma(dev);

    bf_dma_dr_id_t dr_id;
    int wl_id;
    switch (mc_mgr_ctx_dev_family(dev)) {
      case BF_DEV_FAMILY_TOFINO:
        wl_id = 0;
        dr_id = lld_dr_tx_que_write_list;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        wl_id = 1;
        dr_id = lld_dr_tx_que_write_list_1;
        break;
      default:
        MC_MGR_DBGCHK(0);
        return BF_UNEXPECTED;
    }

    int num_subdev = mc_mgr_ctx_num_subdevices(dev);
    bf_subdev_id_t subdev_id = 0;
    memset(&push_cnt[0], 0, sizeof(push_cnt));
    for (subdev_id = 0; subdev_id < num_subdev; subdev_id++) {
      push_cnt[subdev_id] = 0;

      wl = mc_mgr_ctx_wl(dev, subdev_id, sid);
      if (wl == NULL) {
        /* Ignore if write list of this subdevice is empty */
        continue;
      }

      if (wl->count == 0) {
        /* It is possible to have RDM addresses to free but no instructions on
         * the write list.  For example, removing an empty ECMP group that is
         * not
         * associated to any MGIDs.  In this case we delete the vector nodes in
         * the RDM but wouldn't post any writes.  To ensure RDM space is quickly
         * reclaimed we'll process those frees now. */
        if ((num_subdev == 1) || ((num_subdev > 1) && (subdev_id == 1))) {
          bf_map_t *to_free = mc_mgr_ctx_rdm_free_addrs(sid, dev);
          int num_to_free = bf_map_count(to_free);
          bool rdm_change_needed = false;
          for (i = 0; i < num_to_free; ++i) {
            unsigned long key;
            void *unused;
            bf_map_sts_t s = bf_map_get_first_rmv(to_free, &key, &unused);
            MC_MGR_DBGCHK(BF_MAP_OK == s);
            uint32_t rdm_addr = key;
            rdm_change_needed = true;
            mc_mgr_rdm_map_free(dev, rdm_addr);
          }
          if (rdm_change_needed)
            mc_mgr_start_rdm_change_all_pipes(dev, "wrl push");
        }
        continue;
      }

      if (is_last) {
        /* Tag all the MGID updates in this write list against the last buffer
         * so
         * that when they come back an RDM change can be started. */
        mc_mgr_drv_buf_t *last_buf = wl->bufList->prev;
        mc_mgr_ctx_tree_len_lock(dev, subdev_id);
        for (int pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
          int mgid = 0;
          while (-1 != (mgid = mc_mgr_ctx_get_rmv_len_update(
                            sid, dev, subdev_id, pipe))) {
            unsigned long key = (mgid << 8) | pipe;
            bf_map_add(&last_buf->mgids_updated, key, NULL);
          }
        }
        mc_mgr_ctx_tree_len_unlock(dev, subdev_id);

        if ((num_subdev == 1) || ((num_subdev > 1) && (subdev_id == 1))) {
          bf_map_t *rdm_map = mc_mgr_ctx_rdm_free_addrs(sid, dev);
          unsigned long key = 0;
          void *data = NULL;
          while (BF_MAP_OK == bf_map_get_first_rmv(rdm_map, &key, &data)) {
            bf_map_add(&last_buf->freed_rdm_addrs, key, data);
          }
        }
      }

      /* For each DMA buffer in the write list... */
      while (wl->bufList) {
        mc_mgr_drv_buf_t *b = wl->bufList;
        BF_LIST_DLL_REM(wl->bufList, b, next, prev);
        if (b == NULL) {
          MC_MGR_DBGCHK(0);
          return BF_UNEXPECTED;
        }
        dev_map |= UINT64_C(1) << b->dev;
        /* Attempt to push it into the DR. */
        b->buf_pushed = 1;
        /* Need to map the virtual to DMA address everytime a buffer is used
         * since
         * the mapping might be different for different instances for
         * architectures with IOMMU.  */
        if (bf_sys_dma_map(b->pool,
                           b->addr,
                           b->phys_addr,
                           b->size,
                           &dma_addr,
                           BF_DMA_FROM_CPU) != 0) {
          LOG_ERROR("Unable to map DMA buffer %p", b->addr);
          return BF_HW_COMM_FAIL;
        }

        MC_MGR_LOCK(&bp->mtx);
        ++bp->in_use;
        MC_MGR_UNLOCK(&bp->mtx);
        ret = lld_subdev_push_wl(b->dev,
                                 subdev_id,
                                 wl_id,
                                 b->wr_list_size,
                                 b->count,
                                 dma_addr,
                                 b->msgId);
        if (LLD_ERR_DR_FULL == ret) {
          b->buf_pushed = 0;
          /* Publish the DR pointers to the DMA engine. */
          mc_mgr_dma_start(b->dev, subdev_id);

#ifdef MC_MGR_INLINE_DR_SERVICE
          /* It is possible that the completion DR is full and if so that
           * will stop the Tx DR from processing data.  So also service the
           * completion DR now just incase. */
          lld_dr_service(b->dev, subdev_id, dr_id, 5);
#else
          /* Wait for the DMA engine to process data. */
          sched_yield();
#endif
          /* Unmap the buffer */
          if ((bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_FROM_CPU)) !=
              0) {
            LOG_ERROR("Unable to unmap DMA buffer %p", b->addr);
          }
          /* Retry with the same buffer. */
          BF_LIST_DLL_PP(wl->bufList, b, next, prev);
          MC_MGR_LOCK(&bp->mtx);
          --bp->in_use;
          MC_MGR_UNLOCK(&bp->mtx);
          continue;
        } else if (LLD_OK != ret) {
          /* Unmap the buffer */
          if ((bf_sys_dma_unmap(b->pool, b->addr, b->size, BF_DMA_FROM_CPU)) !=
              0) {
            LOG_ERROR("Unable to unmap DMA buffer %p", b->addr);
          }
          b->buf_pushed = 0;
          LOG_ERROR(
              "Write List push for dev %d subdev %d fails (%d), sz 0x%x cnt %d "
              "addr "
              "0x%" PRIx64 " id 0x%" PRIx64,
              dev,
              subdev,
              ret,
              b->wr_list_size,
              b->count,
              dma_addr,
              b->msgId);
          MC_MGR_DBGCHK(LLD_OK == ret);
          MC_MGR_LOCK(&bp->mtx);
          --bp->in_use;
          MC_MGR_UNLOCK(&bp->mtx);
          goto done;
        }
        push_cnt[subdev_id] += 1;
      }  // end while (wl->bufList);

      MC_MGR_DBGCHK(push_cnt[subdev_id] == wl->count);

    done:
      MC_MGR_DBGCHK(NULL == wl->bufList);
      wl->count = 0;
      if (ret) {
        return BF_HW_COMM_FAIL;
      }
    }
  }

  /* Publish the DR pointers to the DMA engine. */
  for (i = 0; i < MC_MGR_NUM_DEVICES; ++i) {
    if (dev_map & (UINT64_C(1) << i)) {
      for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(i); ++subdev) {
        if (push_cnt[subdev] > 0) {
          mc_mgr_dma_start(i, subdev);
        }
      }
    }
  }

  return BF_SUCCESS;
}

void mc_mgr_drv_wrl_abort(int sid) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return;
  }

  for (bf_dev_id_t dev = 0; dev < MC_MGR_NUM_DEVICES; ++dev) {
    if (!mc_mgr_dev_present(dev)) continue;

    for (int subdev_id = 0; subdev_id < (int)mc_mgr_ctx_num_subdevices(dev);
         subdev_id++) {
      mc_mgr_drv_wr_list_t *wl = mc_mgr_ctx_wl(dev, subdev_id, sid);
      MC_MGR_DBGCHK(wl);

      if (wl) {
        if (wl->count == 0) continue;

        while (wl->bufList) {
          mc_mgr_drv_buf_t *b = wl->bufList;
          BF_LIST_DLL_REM(wl->bufList, b, next, prev);
          if (b == NULL) {
            MC_MGR_DBGCHK(0);
            continue;
          }
          mc_mgr_drv_free_buf(b);
        }

        wl->count = 0;
      }
    }
  }
}

int mc_mgr_drv_start_rdm_change(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Start RDM Change fails, invalid device %d.%d", dev, pipe);
    return -1;
  }
  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    LOG_ERROR("Start RDM Change fails, invalid pipe %d.%d", dev, pipe);
    return -1;
  }

  uint32_t base = 0, step = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      base = DEF_tofino_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = DEF_tofino_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      base = tof2_reg_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = tof2_reg_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      base = tof3_reg_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = tof3_reg_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  uint32_t addr = base + pipe * step;
  int ret = mc_mgr_write_register(dev, addr, 1);
  if (ret) {
    LOG_ERROR("Failed to write RDM Ctrl register (%d)", ret);
    MC_MGR_DBGCHK(0 == ret);
    return ret;
  }

  return ret;
}

int mc_mgr_drv_read_rdm_change(bf_dev_id_t dev, bf_dev_pipe_t pipe) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Read RDM Change fails, invalid device %d.%d", dev, pipe);
    return -1;
  }
  if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) {
    LOG_ERROR("Read RDM Change fails, invalid pipe %d.%d", dev, pipe);
    return -1;
  }

  uint32_t base = 0, step = 0;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      base = DEF_tofino_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = DEF_tofino_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      base = tof2_reg_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = tof2_reg_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      base = tof3_reg_device_select_tm_top_tm_pre_top_pre_rdm_ctrl_address;
      step = tof3_reg_device_select_tm_top_tm_pre_top_pre_array_element_size;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  uint32_t addr = base + pipe * step;
  uint32_t val = 0, temp = 0;
  int ret = 0;
  for (bf_subdev_id_t subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev);
       ++subdev) {
    ret = lld_subdev_read_register(dev, subdev, addr, &temp);
    if (ret) {
      LOG_ERROR("RDM Change read fails (%d)", ret);
      MC_MGR_DBGCHK(!ret);
    }
    val |= temp;
  }
  // if (val) {
  //  lld_read_register(dev, addr, &val);
  //}
  return val;
}

bf_status_t mc_mgr_write_register(bf_dev_id_t dev_id,
                                  uint32_t reg_addr,
                                  uint32_t reg_data) {
  bf_status_t sts = BF_SUCCESS;
  int sid = 0;

  if (mc_mgr_ctx_syncing(dev_id)) return BF_SUCCESS;

  if (mc_mgr_is_device_locked(dev_id)) {
    if (mc_mgr_drv_wrl_append_reg(
            dev_id, sid, reg_addr, reg_data, __func__, __LINE__)) {
      sts = BF_HW_COMM_FAIL;
    }
  } else {
    for (bf_subdev_id_t subdev = 0;
         subdev < (int)mc_mgr_ctx_num_subdevices(dev_id);
         ++subdev) {
      sts = lld_subdev_write_register(dev_id, subdev, reg_addr, reg_data);
    }
  }

  return sts;
}
