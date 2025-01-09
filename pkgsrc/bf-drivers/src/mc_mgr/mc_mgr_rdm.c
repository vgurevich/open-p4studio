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


#include "mc_mgr.h"
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_tof_addr_conversion.h>
#include <lld/tofino_defs.h>
#include <math.h>
#include <tofino_regs/pipe_top_level.h>
#include "mc_mgr_handle.h"
#include "mc_mgr_int.h"
#include "mc_mgr_bh.h"

extern bf_status_t bf_mc_do_node_garbage_collection(bf_mc_session_hdl_t shdl);
static void encode_line(bf_dev_id_t dev, mc_mgr_rdm_line_t *line);
void tof_mc_mgr_rdm_encode_line(mc_mgr_rdm_line_t *line);
void tof2_mc_mgr_rdm_encode_line(mc_mgr_rdm_line_t *line);
void tof3_mc_mgr_rdm_encode_line(mc_mgr_rdm_line_t *line);
bf_status_t tof_mc_mgr_rdm_decode_line(mc_mgr_rdm_line_t *line);
bf_status_t tof2_mc_mgr_rdm_decode_line(mc_mgr_rdm_line_t *line);
bf_status_t tof3_mc_mgr_rdm_decode_line(mc_mgr_rdm_line_t *line);
static void mc_mgr_rdm_map_free_now(int sid,
                                    bf_dev_id_t dev,
                                    mc_mgr_rdm_t *rdm_map,
                                    uint32_t addr);
static void set_node_invalid(bf_dev_id_t dev,
                             mc_mgr_rdm_t *rdm_map,
                             uint32_t addr);

void mc_mgr_rdm_map_init(mc_mgr_rdm_t **rdm_map_p,
                         bf_dev_id_t dev,
                         bf_dev_family_t dev_family,
                         uint32_t num_max_pipes) {
  mc_mgr_rdm_t *rdm_map = MC_MGR_CALLOC(1, sizeof(mc_mgr_rdm_t));
  if (!rdm_map) return;

  int rdm_blk_count, rdm_blk_size, rdm_line_count;
  int rdm_blk_id_bit_width;
  *rdm_map_p = rdm_map;

  if (dev_family == BF_DEV_FAMILY_TOFINO) {
    rdm_blk_count = TOF_MC_MGR_RDM_BLK_COUNT;
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
    rdm_line_count = TOF_MC_MGR_RDM_LINE_COUNT;
    rdm_blk_id_bit_width = TOF_MC_MGR_RDM_BLK_ID_BITWIDTH;
  } else if (dev_family == BF_DEV_FAMILY_TOFINO2) {
    rdm_blk_count = TOF2_MC_MGR_RDM_BLK_COUNT;
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
    rdm_line_count = TOF2_MC_MGR_RDM_LINE_COUNT;
    rdm_blk_id_bit_width = TOF2_MC_MGR_RDM_BLK_ID_BITWIDTH;
  } else if (dev_family == BF_DEV_FAMILY_TOFINO3) {
    rdm_blk_count = TOF3_MC_MGR_RDM_BLK_COUNT;
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
    rdm_line_count = TOF3_MC_MGR_RDM_LINE_COUNT;
    rdm_blk_id_bit_width = TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH;
  } else {
    return;
  }

  rdm_map->dev = dev;
  rdm_map->blk_id_width = rdm_blk_id_bit_width;

  int i, j;
  /*malloc all the arrays*/
  rdm_map->free_blocks_ =
      MC_MGR_CALLOC(BF_BITSET_ARRAY_SIZE(rdm_blk_count), sizeof(uint64_t));
  if (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) {
    rdm_map->blk_ids =
        MC_MGR_CALLOC(((rdm_blk_count + 7) / 8), sizeof(uint32_t));
    rdm_map->blk_ids_other_die =
        MC_MGR_CALLOC(((rdm_blk_count + 7) / 8), sizeof(uint32_t));
  } else {
    rdm_map->blk_ids =
        MC_MGR_CALLOC(((rdm_blk_count + 15) / 16), sizeof(uint32_t));
    rdm_map->blk_ids_other_die =
        MC_MGR_CALLOC(((rdm_blk_count + 15) / 16), sizeof(uint32_t));
  }
  rdm_map->rdm = MC_MGR_CALLOC(rdm_line_count, sizeof(mc_mgr_rdm_line_t));
  rdm_map->blocks = MC_MGR_CALLOC(rdm_blk_count, sizeof(mc_mgr_rdm_block_t));
  for (i = 0; i < rdm_blk_count; i++) {
    for (j = 0; j < 2; j++) {
      rdm_map->blocks[i].free_entries_[j] =
          MC_MGR_CALLOC(BF_BITSET_ARRAY_SIZE(rdm_blk_size), sizeof(uint64_t));
    }
  }
  for (i = 0; i < (int)num_max_pipes; i++) {
    for (j = 0; j < 2; j++) {
      rdm_map->used_blocks_[i][j] =
          MC_MGR_CALLOC(BF_BITSET_ARRAY_SIZE(rdm_blk_count), sizeof(uint64_t));
    }
  }
  /* Actual number of lines */
  rdm_map->rdm_line_count = rdm_line_count;
  /* Initial rdm_blk_count */
  rdm_map->rdm_blk_count = rdm_blk_count;
  /* Mark all blocks as free. */
  bf_bs_init(&rdm_map->free_blocks, rdm_blk_count, rdm_map->free_blocks_);
  bf_bs_set_all(&rdm_map->free_blocks, 1);
  /* Ensure all "used_blocks" per pipe are zero. */
  for (i = 0; i < (int)num_max_pipes; ++i) {
    bf_bs_init(&rdm_map->used_blocks[i][0],
               rdm_blk_count,
               rdm_map->used_blocks_[i][0]);
    bf_bs_set_all(&rdm_map->used_blocks[i][0], 0);
    bf_bs_init(&rdm_map->used_blocks[i][1],
               rdm_blk_count,
               rdm_map->used_blocks_[i][1]);
    bf_bs_set_all(&rdm_map->used_blocks[i][1], 0);
  }
  for (i = 0; i < rdm_blk_count; ++i) {
    rdm_map->blocks[i].p2a = power2_allocator_create(512, rdm_blk_size / 512);
    MC_MGR_DBGCHK(rdm_map->blocks[i].p2a);
    rdm_map->blocks[i].id = i;
    rdm_map->blocks[i].rdm = rdm_map;
    rdm_map->blocks[i].pipe = -1;
    bf_bs_init(&rdm_map->blocks[i].free_entries[0],
               rdm_blk_size,
               rdm_map->blocks[i].free_entries_[0]);
    bf_bs_init(&rdm_map->blocks[i].free_entries[1],
               rdm_blk_size,
               rdm_map->blocks[i].free_entries_[1]);
    rdm_map->blocks[i].waiting_free_entries =
        &rdm_map->blocks[i].free_entries[0];
    rdm_map->blocks[i].queued_free_entries =
        &rdm_map->blocks[i].free_entries[1];
  }

  /* Mark address zero of block zero as used since it is not valid. */
  int x = power2_allocator_reserve(rdm_map->blocks[0].p2a, 0, 1);
  MC_MGR_DBGCHK(!x);

  MC_MGR_LOCK_INIT(rdm_map->rdm_change_list_mtx);
}

void mc_mgr_rdm_map_cleanup(mc_mgr_rdm_t **rdm_map) {
  int i, j, rdm_blk_count;
  rdm_blk_count = (*rdm_map)->rdm_blk_count;
  MC_MGR_LOCK_DEL((*rdm_map)->rdm_change_list_mtx);
  for (i = 0; i < rdm_blk_count; ++i) {
    power2_allocator_destroy((*rdm_map)->blocks[i].p2a);
  }
  MC_MGR_FREE((*rdm_map)->free_blocks_);
  MC_MGR_FREE((*rdm_map)->blk_ids);
  MC_MGR_FREE((*rdm_map)->blk_ids_other_die);
  MC_MGR_FREE((*rdm_map)->rdm);
  for (i = 0; i < rdm_blk_count; i++) {
    for (j = 0; j < 2; j++) {
      MC_MGR_FREE((*rdm_map)->blocks[i].free_entries_[j]);
    }
  }
  MC_MGR_FREE((*rdm_map)->blocks);
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes((*rdm_map)->dev); i++) {
    for (j = 0; j < 2; j++) {
      MC_MGR_FREE((*rdm_map)->used_blocks_[i][j]);
    }
  }
  MC_MGR_FREE(*rdm_map);
  *rdm_map = NULL;
}

bf_status_t mc_mgr_rdm_reserve_tails(int sid, bf_dev_id_t dev, int pipe) {
  struct mc_mgr_tail_info *ti = mc_mgr_ctx_tail_info(dev);
  bf_bitset_t ports;
  uint64_t ports_[BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)] = {0};
  bf_bs_init(&ports, BF_PIPE_PORT_COUNT, ports_);
  uint32_t a = mc_mgr_rdm_map_get(
      sid, dev, pipe, mc_mgr_rdm_node_type_lag, ti->tail_l2_size);
  int a_width = node_width(mc_mgr_rdm_node_type_lag);
  if (!a) return BF_NO_SYS_RESOURCES;
  for (unsigned int i = 0; i < ti->tail_l2_size; ++i) {
    bool last = i == ti->tail_l2_size - 1;
    uint32_t rdm_addr = a + i * a_width;
    mc_mgr_rdm_write_lag(
        sid, dev, rdm_addr, 0xFF, !last ? rdm_addr + a_width : 0);
  }
  ti->tail_l2_addr[pipe] = a;

  uint32_t b = mc_mgr_rdm_map_get(
      sid, dev, pipe, mc_mgr_rdm_node_type_end, ti->num_tails);
  int b_width = node_width(mc_mgr_rdm_node_type_end);
  if (!b) return BF_NO_SYS_RESOURCES;
  for (unsigned int i = 0; i < ti->num_tails; ++i) {
    uint32_t rdm_addr = b + i * b_width;
    int len = ti->tail_size[i];
    uint32_t l2_addr = len ? a + a_width * (ti->tail_l2_size - len) : 0;
    mc_mgr_rdm_write_l1_end(sid, dev, rdm_addr, l2_addr, 0);
  }
  ti->tail_base[pipe] = b;
  return BF_SUCCESS;
}

static void swap_lists(mc_mgr_rdm_block_t *blk) {
  /* There can't be anything on the waiting list right now else it will move
   * back to the queued list! */
  MC_MGR_DBGCHK(bf_bs_all_0s(blk->waiting_free_entries));

  bf_bitset_t *tmp = blk->waiting_free_entries;
  blk->waiting_free_entries = blk->queued_free_entries;
  blk->queued_free_entries = tmp;
}
static void mc_mgr_start_rdm_change(bf_dev_id_t dev,
                                    int pipe,
                                    const char *who) {
  /* Don't restart a previous change. */
  if (mc_mgr_ctx_rdm_pending_get(dev, pipe)) {
    return;
  }

  /* For all blocks in the pipe, swap their lists. */
  int blk = -1;
  while (-1 != (blk = bf_bs_first_set(
                    &mc_mgr_ctx_rdm_map(dev)->used_blocks[pipe][0], blk))) {
    swap_lists(&mc_mgr_ctx_rdm_map(dev)->blocks[blk]);
  }
  while (-1 != (blk = bf_bs_first_set(
                    &mc_mgr_ctx_rdm_map(dev)->used_blocks[pipe][1], blk))) {
    swap_lists(&mc_mgr_ctx_rdm_map(dev)->blocks[blk]);
  }
  /* Write the HW register to initiate a change. */
  mc_mgr_drv_start_rdm_change(dev, pipe);
  /* Update our state to indicate a change in in progress. */
  mc_mgr_ctx_rdm_pending_set(dev, pipe);
  LOG_TRACE("Started RDM Change on %d.%d (%s)", dev, pipe, who);
}
void mc_mgr_start_rdm_change_all_pipes(bf_dev_id_t dev, const char *who) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);
  for (int pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    mc_mgr_start_rdm_change(dev, pipe, who);
  }
  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);
}
static void blk_process_pending_list(bf_dev_id_t dev,
                                     mc_mgr_rdm_block_t *blk,
                                     mc_mgr_rdm_addr_list_t **addrs) {
  int idx = -1;
  int rdm_blk_size;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
  } else {
    return;
  }
  while (-1 != (idx = bf_bs_first_set(blk->waiting_free_entries, idx))) {
    /* Clear it from the bitmap. */
    bf_bs_set(blk->waiting_free_entries, idx, 0);
    /* Do the actual free. */
    uint32_t addr = idx + blk->id * rdm_blk_size;
    mc_mgr_rdm_addr_append(addrs, addr);
  }
}
static bool mgid_tail_process_pending_list(int sid,
                                           bf_dev_id_t dev,
                                           int pipe,
                                           mc_mgr_rdm_t *rdm_map) {
  bool did_work = !!rdm_map->for_next_rdm_change[pipe];
  while (rdm_map->for_next_rdm_change[pipe]) {
    struct mc_mgr_mgid_list_t *x = rdm_map->for_next_rdm_change[pipe];
    rdm_map->for_next_rdm_change[pipe] = x->next;
    mc_mgr_set_l1_tail(sid, dev, pipe, x->mgid, x->update_length);
    MC_MGR_FREE(x);
  }
  return did_work;
}
void mc_mgr_enqueue_mgid_tail_update(bf_dev_id_t dev,
                                     int pipe,
                                     int mgid,
                                     int length) {
  /* length argument is needed because we cannot simply evaluate the tree when
   * the RDM-Change-Done is received because there could have been additional
   * changes to the tree which depend on a future RDM-Change-Done signal. */
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  struct mc_mgr_mgid_list_t *x = MC_MGR_MALLOC(sizeof *x);
  if (!x) return;
  x->mgid = mgid;
  x->update_length = length;

  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);
  x->next = rdm_map->after_next_rdm_change[pipe];
  rdm_map->after_next_rdm_change[pipe] = x;
  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);
}
void mc_mgr_rdm_change_done(int sid,
                            bf_dev_id_t dev,
                            int pipe,
                            bool restartable) {
  int i;
  int blk = -1;
  bool needs_rdm_change = false;
  bool did_work = false;
  mc_mgr_rdm_addr_list_t *to_free = NULL;

  mc_mgr_one_at_a_time_begin();

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);

  LOG_TRACE("Detected RDM Change Complete on %d.%d", dev, pipe);

  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);
  /* Visit each block owned by the pipe. */
  for (i = 0; i < 2; ++i) {
    while (-1 != (blk = bf_bs_first_set(&rdm_map->used_blocks[pipe][i], blk))) {
      /* Clear out the pending list.  */
      blk_process_pending_list(dev, &rdm_map->blocks[blk], &to_free);

      /* If any of the queued lists are non-empty, start another RDM
       * Change. */
      if (restartable && !needs_rdm_change &&
          !bf_bs_all_0s(rdm_map->blocks[blk].queued_free_entries)) {
        needs_rdm_change = true;
      }
    }
  }
  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);

  did_work = !!to_free;
  while (to_free) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_free, &x);
    mc_mgr_rdm_map_free_now(sid, dev, rdm_map, x);
  }

  /* Process any pending MGID tail updates for the pipe.  Safe to work with the
   * pending list's pointer because no other threads will modify it since the
   * RDM change is still marked as in-progress. */
  did_work |= mgid_tail_process_pending_list(sid, dev, pipe, rdm_map);

  /* Write the new RDM contents (zeroed out nodes) to HW. */
  if (did_work) {
    bf_status_t sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
    }
  }

  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);
  /* The pending MGID list is now empty, promote the next MGID list and if it
   * is not empty kick off another RDM change request. */
  if (rdm_map->after_next_rdm_change[pipe]) {
    rdm_map->for_next_rdm_change[pipe] = rdm_map->after_next_rdm_change[pipe];
    rdm_map->after_next_rdm_change[pipe] = NULL;
    needs_rdm_change = true;
  }

  /* RDM change is now complete, update state. */
  mc_mgr_ctx_rdm_pending_clr(dev, pipe);

  /* Initiate the next RDM Change if needed. */
  if (needs_rdm_change) {
    LOG_TRACE("  Restarting RDM Change %d.%d", dev, pipe);
    mc_mgr_start_rdm_change(dev, pipe, "Restart");
  }
  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);
  mc_mgr_one_at_a_time_end();
}

void mc_mgr_wait_rdm_change(int sid, bf_dev_id_t dev, int pipe) {
  int i;
  uint32_t is_pending = 0;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);

  /* If this special value is passed as the pipe, wait for all pipes. */
  if ((int)mc_mgr_ctx_num_max_pipes(dev) == pipe) {
    for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i)
      is_pending |= mc_mgr_ctx_rdm_pending_get(dev, i) << i;
  } else {
    is_pending = mc_mgr_ctx_rdm_pending_get(dev, pipe) << pipe;
  }

  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);

  if (!is_pending) return;

  while (is_pending) {
    for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
      if ((is_pending >> i) & 1) {
        if (!mc_mgr_drv_read_rdm_change(dev, i)) {
          is_pending &= ~(1 << i);
          mc_mgr_rdm_change_done(sid, dev, i, false);
        }
      }
    }
  }
}

/*
 * mc_mgr_rdm_map_return - Given dev and addr, mark addr free
 *                       - Used to return pre-allocated addresses on failure
 * mc_mgr_rdm_map_get - Get address, mark used, write block-id register
 * mc_mgr_mark_addr_used - Given addr, mark as used, updates shadow of block-id
 *                         reg
 *                       - Used in HW sync during HA
 * mc_mgr_mark_addr_free - Given addr, mark as free, updates shadow of block-id
 *                         reg
 *                       - Free function in HA case
 * mc_mgr_rdm_map_free_now - Writes node as invalid (shadow+dma), update shadow
 *                           of block-id reg
 *                         - Only used when RDM change is complete
 * mc_mgr_rdm_map_enqueue_free - Queues address against DMA buffer
 *                             - Standard free of allocated nodes
 *
 */

void mc_mgr_rdm_map_return(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  int blk;
  int blk_addr, rdm_blk_size;

  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
    blk = tof_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
    blk = tof2_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
    blk = tof3_rdm_addr_to_blk(dev, addr);
  } else {
    return;
  }
  blk_addr = addr - blk * rdm_blk_size;

  int size =
      power2_allocator_get_index_size(rdm_map->blocks[blk].p2a, blk_addr);
  int ret = power2_allocator_release(rdm_map->blocks[blk].p2a, blk_addr);
  if (ret || size == -1 || size == 0) {
    LOG_ERROR("RDM return fails (%d) for addr 0x%x - 0x%x (%d)",
              ret,
              addr,
              addr + size - 1,
              size);
    MC_MGR_DBGCHK(0 == ret);
    MC_MGR_DBGCHK(size != -1);
    MC_MGR_DBGCHK(size > 0);
    return;
  }

  /* If the block is now empty, return it to the free list. */
  int count = power2_allocator_alloc_count(rdm_map->blocks[blk].p2a);

  /* Block zero always has one node reserved. */
  bool empty = !count || (1 == count && !blk);
  if (empty) {
    /* Don't know which pipe the block was assigned to, just clear it in
     * all pipes. */
    int pipe;
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      /* Remove from the pipe's used block list. */
      bf_bs_set(&rdm_map->used_blocks[pipe][0], blk, 0);
      bf_bs_set(&rdm_map->used_blocks[pipe][1], blk, 0);
    }
    /* Add to free block list. */
    bf_bs_set(&rdm_map->free_blocks, blk, 1);
    rdm_map->blocks[blk].pipe = -1;
    if (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) {
      rdm_map->blk_ids[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
      rdm_map->blk_ids_other_die[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
    } else {
      rdm_map->blk_ids[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
      rdm_map->blk_ids_other_die[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
    }
    LOG_TRACE("Returning RDM address %#x - %#x (%d) and freeing block %d",
              addr,
              addr + size - 1,
              size,
              blk);
  } else {
    LOG_TRACE(
        "Returning RDM address %#x - %#x (%d)", addr, addr + size - 1, size);
  }
}

static uint32_t rdm_map_get(int sid,
                            bf_dev_id_t dev,
                            int pipe,
                            mc_mgr_rdm_node_type_e type,
                            int count,
                            int n_tries) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  int addr;
  int width = node_width(type);
  bool l1_l2 = !node_is_l1(type);
  bool is_tof3 =
      (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) ? true : false;
  /* First check blocks owned by the pipe for a free entry. */
  int blk = -1;
  while (-1 !=
         (blk = bf_bs_first_set(&rdm_map->used_blocks[pipe][l1_l2], blk))) {
    if (-1 != (addr = power2_allocator_alloc(rdm_map->blocks[blk].p2a,
                                             width * count))) {
      break;
    }
  }
  if (-1 == blk) {
    /* No free entries in the owned blocks.  Allocate a new block. */
    blk = bf_bs_first_set(&rdm_map->free_blocks, -1);
    if (-1 == blk) {
      /* No free blocks either.  Out of RDM space (or compaction is required).
       * Run garbage collection now (which also requires DR servicing) to try
       * and recover nodes. */
      if (!mc_mgr_is_device_locked(dev) && n_tries > 0) {
        mc_mgr_drv_service_dr(dev);
        bf_mc_do_node_garbage_collection(mc_mgr_ctx_int_sess());
        return rdm_map_get(sid, dev, pipe, type, count, n_tries - 1);
      }
      LOG_ERROR(
          "Out of RDM resources, session %#x cannot allocate %d %s nodes on "
          "dev %d pipe %d",
          mc_mgr_encode_sess_hdl(sid),
          count,
          MC_MGR_RDM_NODE_TYPE_STR(type),
          dev,
          pipe);
      return 0;
    }
    mark_block_used(rdm_map, pipe, blk, l1_l2);
    /* Allocate an entry from the block. */
    addr = power2_allocator_alloc(rdm_map->blocks[blk].p2a, width * count);
    /* Shouldn't fail on a new block. */
    if (-1 == addr) {
      MC_MGR_DBGCHK(0);
      return 0;
    }
    /* Assign the block to the pipe. */
    mc_mgr_set_rdm_blk_id_grp_wrl(sid, dev, is_tof3 ? (blk / 8) : (blk / 16));
  }
  int rdm_blk_size;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
  } else {
    return 0;
  }
  if (-1 == addr) {
    MC_MGR_DBGCHK(-1 != addr);
  } else {
    if (addr < 0 || addr >= rdm_blk_size) {
      MC_MGR_DBGCHK(0);
      return 0;
    }
    if (addr + width * count - 1 >= rdm_blk_size) {
      MC_MGR_DBGCHK(0);
      return 0;
    }
  }

  uint32_t rdm_address = addr + blk * rdm_blk_size;
  if (2 == width) {
    MC_MGR_DBGCHK(!(rdm_address & 1));
  }
  LOG_TRACE("Allocated RDM addresses %#x - %#x",
            rdm_address,
            rdm_address + width * count - 1);
  return rdm_address;
}

uint32_t mc_mgr_rdm_map_get(int sid,
                            bf_dev_id_t dev,
                            int pipe,
                            mc_mgr_rdm_node_type_e type,
                            int count) {
  int n_tries = 8;
  return rdm_map_get(sid, dev, pipe, type, count, n_tries);
}

bool mc_mgr_mark_addr_used(mc_mgr_rdm_t *rdm_map,
                           bf_dev_id_t dev,
                           int addr,
                           int pipe,
                           int sz,
                           bool is_l2_node) {
  int blk;
  int blk_addr = rdm_addr_to_blk_addr(dev, addr);
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    blk = tof_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    blk = tof2_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    blk = tof3_rdm_addr_to_blk(dev, addr);
  } else {
    return false;
  }
  /* If the block is free, assign it to this pipe. */
  if (rdm_map->blocks[blk].pipe == -1) {
    int x = power2_allocator_reserve(rdm_map->blocks[blk].p2a, blk_addr, sz);
    if (x) return false;
    mark_block_used(rdm_map, pipe, blk, is_l2_node);
    return true;
  }
  /* If the block is assigned to the pipe also check that the node type can
   * use the block. */
  if (rdm_map->blocks[blk].pipe == pipe &&
      bf_bs_get(&rdm_map->used_blocks[pipe][is_l2_node], blk)) {
    int x = power2_allocator_reserve(rdm_map->blocks[blk].p2a, blk_addr, sz);
    if (x) return false;
    return true;
  }
  return false;
}
bool mc_mgr_rdm_mark_addr_free(mc_mgr_rdm_t *rdm_map,
                               bf_dev_id_t dev,
                               int addr,
                               int pipe) {
  int blk;
  int blk_addr = rdm_addr_to_blk_addr(dev, addr);
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    blk = tof_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    blk = tof2_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    blk = tof3_rdm_addr_to_blk(dev, addr);
  } else {
    return false;
  }
  /* Block must be assigned to this pipe. */
  if (rdm_map->blocks[blk].pipe != pipe) return false;

  int x = power2_allocator_release(rdm_map->blocks[blk].p2a, blk_addr);
  if (x) return false;
  set_node_invalid(dev, rdm_map, addr);
  /* If this emptied the block mark it as free. */
  uint32_t c = power2_allocator_alloc_count(rdm_map->blocks[blk].p2a);
  if (!c || (1 == c && !blk)) {
    mark_block_free(rdm_map, pipe, blk);
  }
  return true;
}

static int rdm_node_width(mc_mgr_rdm_t *rdm_map, int addr) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  int index = addr & 1;
  return node_width(line->type[index]);
}

static void mc_mgr_rdm_map_free_now(int sid,
                                    bf_dev_id_t dev,
                                    mc_mgr_rdm_t *rdm_map,
                                    uint32_t addr) {
  int blk;
  int blk_addr, rdm_blk_size;
  int size;
  int ret;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    blk = tof_rdm_addr_to_blk(dev, addr);
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    blk = tof2_rdm_addr_to_blk(dev, addr);
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    blk = tof3_rdm_addr_to_blk(dev, addr);
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
  } else {
    return;
  }
  blk_addr = addr - blk * rdm_blk_size;
  size = power2_allocator_get_index_size(rdm_map->blocks[blk].p2a, blk_addr);
  ret = power2_allocator_release(rdm_map->blocks[blk].p2a, blk_addr);
  if (ret || size == -1 || size == 0) {
    LOG_ERROR("RDM free fails (%d) for addr 0x%x - 0x%x (%d)",
              ret,
              addr,
              addr + size - 1,
              size);
    MC_MGR_DBGCHK(0 == ret);
    MC_MGR_DBGCHK(size != -1);
    MC_MGR_DBGCHK(size > 0);
    return;
  }
  /* If the block is now empty, return it to the free list. */
  int count = power2_allocator_alloc_count(rdm_map->blocks[blk].p2a);

  /* Update the RDM contents. */
  int x = 0;
  for (x = 0; x < size;) {
    int width = rdm_node_width(rdm_map, addr + x);
    MC_MGR_DBGCHK(1 == width || 2 == width);
    mc_mgr_rdm_invalidate_node(sid, dev, addr + x);
    x += width;
  }

  /* Block zero always has one node reserved. */
  bool empty = !count || (1 == count && !blk);
  if (empty) {
    /* Don't know which pipe the block was assigned to, just clear it in
     * all pipes. */
    int pipe;
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      /* Remove from the pipe's used block list. */
      bf_bs_set(&rdm_map->used_blocks[pipe][0], blk, 0);
      bf_bs_set(&rdm_map->used_blocks[pipe][1], blk, 0);
    }
    /* Add to free block list. */
    bf_bs_set(&rdm_map->free_blocks, blk, 1);
    rdm_map->blocks[blk].pipe = -1;
    if (rdm_map->blk_id_width == TOF3_MC_MGR_RDM_BLK_ID_BITWIDTH) {
      rdm_map->blk_ids[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
      rdm_map->blk_ids_other_die[blk / 8] &= ~(0xf << (4 * (blk & 0x7)));
    } else {
      rdm_map->blk_ids[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
      rdm_map->blk_ids_other_die[blk / 16] &= ~(0x3 << (2 * (blk & 0xF)));
    }
    LOG_TRACE(
        "Returning dev %d RDM address %#x - %#x (%d) and freeing block %d",
        dev,
        addr,
        addr + size - 1,
        size,
        blk);
  } else {
    LOG_TRACE("Releasing dev %d RDM address %#x - %#x (%d)",
              dev,
              addr,
              addr + size - 1,
              size);
  }
}

void mc_mgr_rdm_map_enqueue_free(int sid, bf_dev_id_t dev, uint32_t addr) {
  if (!mc_mgr_ctx_syncing(dev)) {
    /* Add the address to the free list. */
    unsigned long key = addr;
    void *data = NULL;
    bf_map_t *map = mc_mgr_ctx_rdm_free_addrs(sid, dev);
    bf_map_sts_t s = bf_map_add(map, key, data);
    MC_MGR_DBGCHK(BF_MAP_OK == s);
  } else {
    mc_mgr_rdm_map_return(dev, addr);
  }
}

void mc_mgr_rdm_map_free(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  int blk, rdm_blk_size;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    rdm_blk_size = TOF_MC_MGR_RDM_BLK_SIZE;
    blk = tof_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    rdm_blk_size = TOF2_MC_MGR_RDM_BLK_SIZE;
    blk = tof2_rdm_addr_to_blk(dev, addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    rdm_blk_size = TOF3_MC_MGR_RDM_BLK_SIZE;
    blk = tof3_rdm_addr_to_blk(dev, addr);
  } else {
    return;
  }
  int blk_addr = addr - blk * rdm_blk_size;

  MC_MGR_LOCK(&rdm_map->rdm_change_list_mtx);
  bf_bs_set(rdm_map->blocks[blk].queued_free_entries, blk_addr, 1);
  MC_MGR_UNLOCK(&rdm_map->rdm_change_list_mtx);
}

inline static bool line_is_shareable(mc_mgr_rdm_line_t *line) {
  if (!line) {
    MC_MGR_DBGCHK(0);
    return false;
  }
  return (mc_mgr_rdm_node_type_invalid == line->type[0] ||
          mc_mgr_rdm_node_type_end == line->type[0] ||
          mc_mgr_rdm_node_type_port18 == line->type[0] ||
          mc_mgr_rdm_node_type_lag == line->type[0]);
}

uint32_t mc_mgr_rdm_next_l1(mc_mgr_rdm_t *rdm_map, uint32_t addr) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  switch (line->type[addr & 1]) {
    case mc_mgr_rdm_node_type_invalid:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_rid:
      return line->u.rid.next_l1;
    case mc_mgr_rdm_node_type_xid:
      return line->u.xid.next_l1;
    case mc_mgr_rdm_node_type_end:
      return 0;
    case mc_mgr_rdm_node_type_ecmp:
      return line->u.ecmp.next_l1;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return line->u.ecmp_xid.next_l1;
    case mc_mgr_rdm_node_type_vector:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_port18:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_port72:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_lag:
      MC_MGR_DBGCHK(0);
      return 0;
    default:
      MC_MGR_DBGCHK(0);
  }
  return 0;
}
uint32_t mc_mgr_rdm_l1_node_get_l2_ptr(mc_mgr_rdm_t *rdm_map, uint32_t addr) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  switch (line->type[addr & 1]) {
    case mc_mgr_rdm_node_type_rid:
      return line->u.rid.next_l2;
    case mc_mgr_rdm_node_type_xid:
      return line->u.xid.next_l2;
    case mc_mgr_rdm_node_type_end:
      return line->u.end[addr & 1].next_l2;
    case mc_mgr_rdm_node_type_ecmp:
      return 0;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return 0;
    case mc_mgr_rdm_node_type_vector:
      return 0;
    case mc_mgr_rdm_node_type_port18:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_port72:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_lag:
      MC_MGR_DBGCHK(0);
      return 0;
    case mc_mgr_rdm_node_type_invalid:
      MC_MGR_DBGCHK(0);
      return 0;
    default:
      MC_MGR_DBGCHK(0);
      return 0;
  }
}
uint32_t mc_mgr_rdm_next_l2(mc_mgr_rdm_t *rdm_map,
                            uint32_t addr,
                            bool block_level) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  switch (line->type[addr & 1]) {
    case mc_mgr_rdm_node_type_invalid:
      return 0;
    case mc_mgr_rdm_node_type_rid:
      return line->u.rid.next_l2;
    case mc_mgr_rdm_node_type_xid:
      return line->u.xid.next_l2;
    case mc_mgr_rdm_node_type_end:
      return line->u.end[addr & 1].next_l2;
    case mc_mgr_rdm_node_type_ecmp:
      return 0;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return 0;
    case mc_mgr_rdm_node_type_vector:
      return 0;
    case mc_mgr_rdm_node_type_port18:
      if (block_level) return 0;
      return !line->u.port18[addr & 1].last ? addr + 1 : 0;
    case mc_mgr_rdm_node_type_port72:
      if (block_level) return 0;
      return !line->u.port72.last ? addr + 2 : 0;
    case mc_mgr_rdm_node_type_lag:
      return line->u.lag[addr & 1].next_l2;
    default:
      MC_MGR_DBGCHK(0);
  }
  return 0;
}
void mc_mgr_rdm_unlink_and_get_lag_node_by_lag_id(int sid,
                                                  bf_dev_id_t dev,
                                                  uint32_t l1_addr,
                                                  uint32_t *lag_node,
                                                  int lag_id) {
  uint32_t l2_addr =
      mc_mgr_rdm_l1_node_get_l2_ptr(mc_mgr_ctx_rdm_map(dev), l1_addr);
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  /* No L2 tree. */
  if (!l2_addr) {
    if (lag_node) *lag_node = 0;
    return;
  }

  /* No LAG nodes in L2 tree. */
  if (mc_mgr_rdm_node_type_port72 == mc_mgr_rdm_type(rdm_map, l2_addr) ||
      mc_mgr_rdm_node_type_port18 == mc_mgr_rdm_type(rdm_map, l2_addr)) {
    if (lag_node) *lag_node = 0;
    return;
  }

  /* Walk through all the LAG nodes in the tree. */
  uint32_t prev_addr = l1_addr;
  while (mc_mgr_rdm_node_type_lag == mc_mgr_rdm_type(rdm_map, l2_addr)) {
    uint32_t next_l2_addr = mc_mgr_rdm_next_l2(rdm_map, l2_addr, true);
    int32_t l2_lag_id = mc_mgr_rdm_get_l2_lagid(dev, l2_addr);
    if (l2_lag_id == lag_id) {
      mc_mgr_rdm_update_next_l2(sid, dev, prev_addr, next_l2_addr);
      if (lag_node) *lag_node = l2_addr;
      return;
    }
    prev_addr = l2_addr;
    l2_addr = next_l2_addr;
  }

  if (lag_node) *lag_node = 0;
  return;
}

void mc_mgr_rdm_get_port_node_from_l1(bf_dev_id_t dev,
                                      uint32_t addr,
                                      uint32_t *port_node,
                                      uint32_t *prev_node) {
  uint32_t l2_addr =
      mc_mgr_rdm_l1_node_get_l2_ptr(mc_mgr_ctx_rdm_map(dev), addr);
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  /* No L2 tree. */
  if (!l2_addr) {
    if (port_node) *port_node = 0;
    if (prev_node) *prev_node = addr;
    return;
  }

  /* No LAG nodes in L2 tree. */
  if (mc_mgr_rdm_node_type_port72 == mc_mgr_rdm_type(rdm_map, l2_addr) ||
      mc_mgr_rdm_node_type_port18 == mc_mgr_rdm_type(rdm_map, l2_addr)) {
    if (port_node) *port_node = l2_addr;
    if (prev_node) *prev_node = addr;
    return;
  }

  /* Walk through all the LAG nodes in the tree. */
  while (mc_mgr_rdm_node_type_lag == mc_mgr_rdm_type(rdm_map, l2_addr)) {
    uint32_t next_l2_addr = mc_mgr_rdm_next_l2(rdm_map, l2_addr, true);
    if (mc_mgr_rdm_node_type_port72 != mc_mgr_rdm_type(rdm_map, l2_addr) &&
        mc_mgr_rdm_node_type_port18 != mc_mgr_rdm_type(rdm_map, l2_addr)) {
      if (port_node) *port_node = next_l2_addr;
      if (prev_node) *prev_node = l2_addr;
      return;
    }
    l2_addr = next_l2_addr;
  }

  if (port_node) *port_node = 0;
  if (prev_node) *prev_node = 0;
  return;
}
uint32_t mc_mgr_rdm_get_next_l1(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(rdm_map, addr);
  switch (t) {
    case mc_mgr_rdm_node_type_rid:
      return line->u.rid.next_l1;
    case mc_mgr_rdm_node_type_xid:
      return line->u.xid.next_l1;
    case mc_mgr_rdm_node_type_ecmp:
      return line->u.ecmp.next_l1;
    case mc_mgr_rdm_node_type_ecmp_xid:
      return line->u.ecmp_xid.next_l1;
    default:
      MC_MGR_DBGCHK(0);
      return 0;
  }
}
uint32_t mc_mgr_rdm_get_l2_lagid(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(rdm_map, addr);
  switch (t) {
    case mc_mgr_rdm_node_type_lag:
      return line->u.lag[addr & 1].lag_id;
    default:
      MC_MGR_DBGCHK(0);
      return 0;
  }
}
void mc_mgr_rdm_set_next_l1(mc_mgr_rdm_t *rdm_map,
                            uint32_t addr,
                            uint32_t next_l1) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(rdm_map, addr);
  switch (t) {
    case mc_mgr_rdm_node_type_rid:
      line->u.rid.next_l1 = next_l1;
      break;
    case mc_mgr_rdm_node_type_xid:
      line->u.xid.next_l1 = next_l1;
      break;
    case mc_mgr_rdm_node_type_ecmp:
      line->u.ecmp.next_l1 = next_l1;
      break;
    case mc_mgr_rdm_node_type_ecmp_xid:
      line->u.ecmp_xid.next_l1 = next_l1;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return;
  }
  /* Fill in the bit vector representation. */
  encode_line(rdm_map->dev, line);
}
void mc_mgr_rdm_update_next_l1(int sid,
                               bf_dev_id_t dev,
                               uint32_t addr,
                               uint32_t next_l1) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (addr == next_l1) {
    MC_MGR_DBGCHK(0);
    return;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_set_next_l1(rdm_map, addr, next_l1);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE("Update L1-next at addr %#x, next %#x", addr, next_l1);
  return;
}

static void set_next_l2(mc_mgr_rdm_t *rdm_map,
                        uint32_t addr,
                        uint32_t next_l2) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(rdm_map, addr);
  switch (t) {
    case mc_mgr_rdm_node_type_rid:
      line->u.rid.next_l2 = next_l2;
      break;
    case mc_mgr_rdm_node_type_xid:
      line->u.xid.next_l2 = next_l2;
      break;
    case mc_mgr_rdm_node_type_end:
      line->u.end[addr & 1].next_l2 = next_l2;
      break;
    case mc_mgr_rdm_node_type_lag:
      line->u.lag[addr & 1].next_l2 = next_l2;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return;
  }
  /* Fill in the bit vector representation. */
  encode_line(rdm_map->dev, line);
}
void mc_mgr_rdm_update_next_l2(int sid,
                               bf_dev_id_t dev,
                               uint32_t addr,
                               uint32_t next_l2) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  set_next_l2(rdm_map, addr, next_l2);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE("Update L2-next at dev %d addr %#x, next %#x", dev, addr, next_l2);
  return;
}

bf_status_t mc_mgr_ecc_correct_rdm(bf_dev_id_t dev, uint32_t address) {
  mc_mgr_rdm_line_t *line;
  bf_status_t sts;
  int line_no = 0, sid = 0;
  int bit_cnt, rdm_line_count;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    rdm_line_count = TOF_MC_MGR_RDM_LINE_COUNT;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    rdm_line_count = TOF2_MC_MGR_RDM_LINE_COUNT;
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    rdm_line_count = TOF3_MC_MGR_RDM_LINE_COUNT;
  } else {
    return BF_INVALID_ARG;
  }
  bit_cnt = log2(rdm_line_count);

  mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);
  line_no = address & mc_mgr_get_bit_mask(bit_cnt);
  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  if (line_no >= rdm_line_count) {
    /* An invalid line-no lookup could cause an ecc interrupt, ignore the ecc */
    LOG_ERROR(" Invalid line-no %d in RDM ECC correct (dev %d)", line_no, dev);
    return BF_INVALID_ARG;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  line = &rdm_map->rdm[line_no];

  /* Append to the write list. */
  sts = mc_mgr_set_rdm_wrl(sid, dev, line_no, line->data[1], line->data[0]);
  if (BF_SUCCESS == sts) sts = mc_mgr_drv_wrl_send(sid, false);
  return sts;
}

static bool extract_port18_ports(bf_dev_id_t dev,
                                 mc_mgr_rdm_t *rdm_map,
                                 uint32_t addr,
                                 bf_bitset_t *ports) {
  int pipe_mask = 0, i, line, which;
  do {
    if (mc_mgr_rdm_node_type_port18 != mc_mgr_rdm_type(rdm_map, addr))
      return false;
    line = rdm_addr_to_line(dev, addr);
    which = addr & 1;
    unsigned int pipe = rdm_map->rdm[line].u.port18[which].pipe;
    if (pipe >= (unsigned int)mc_mgr_ctx_num_max_pipes(dev)) return false;
    if (pipe_mask & (1 << pipe)) return false;
    pipe_mask |= 1 << pipe;
    for (i = 0; i < 16; ++i) {
      if (rdm_map->rdm[line].u.port18[which].ports & (UINT64_C(1) << i)) {
        bf_bs_set(&ports[pipe], i * 4, 1);
      }
    }
    for (i = 0; i < 2; ++i) {
      if (rdm_map->rdm[line].u.port18[which].spv & (UINT64_C(1) << i)) {
        bf_bs_set(&ports[pipe], i * 4 + 64, 1);
      }
    }
    ++addr;
  } while (!rdm_map->rdm[line].u.port18[which].last);
  return true;
}
static bool extract_port72_ports(bf_dev_id_t dev,
                                 mc_mgr_rdm_t *rdm_map,
                                 uint32_t addr,
                                 bf_bitset_t *ports) {
  int pipe_mask = 0, i, line;
  do {
    if (mc_mgr_rdm_node_type_port72 != mc_mgr_rdm_type(rdm_map, addr))
      return false;
    line = rdm_addr_to_line(dev, addr);
    unsigned int pipe = rdm_map->rdm[line].u.port72.pipe;
    if (pipe >= mc_mgr_ctx_num_max_pipes(dev)) return false;
    if (pipe_mask & (1 << pipe)) return false;
    pipe_mask |= 1 << pipe;
    for (i = 0; i < 64; ++i) {
      if (rdm_map->rdm[line].u.port72.ports & (UINT64_C(1) << i)) {
        bf_bs_set(&ports[pipe], i, 1);
      }
    }
    for (i = 0; i < 8; ++i) {
      if (rdm_map->rdm[line].u.port72.spv & (UINT64_C(1) << i)) {
        bf_bs_set(&ports[pipe], i + 64, 1);
      }
    }
    addr += 2;
  } while (!rdm_map->rdm[line].u.port72.last);
  return true;
}

static void set_node_invalid(bf_dev_id_t dev,
                             mc_mgr_rdm_t *rdm_map,
                             uint32_t addr) {
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Shouldn't invalidate twice. */
  if (mc_mgr_rdm_node_type_invalid == line->type[addr & 1]) {
    LOG_ERROR("Freeing free node at dev %d addr %#x", dev, addr);
    MC_MGR_DBGCHK(0);
    return;
  }

  bool full_width = false;
  if (addr & 1) {
  } else {
    full_width = line->type[0] == mc_mgr_rdm_node_type_rid ||
                 line->type[0] == mc_mgr_rdm_node_type_xid ||
                 line->type[0] == mc_mgr_rdm_node_type_ecmp ||
                 line->type[0] == mc_mgr_rdm_node_type_ecmp_xid ||
                 line->type[0] == mc_mgr_rdm_node_type_vector ||
                 line->type[0] == mc_mgr_rdm_node_type_port72;
  }
  if (full_width) {
    MC_MGR_MEMSET(line, 0, sizeof(mc_mgr_rdm_line_t));
    line->type[0] = line->type[1] = mc_mgr_rdm_node_type_invalid;
  } else {
    line->type[addr & 1] = mc_mgr_rdm_node_type_invalid;
    encode_line(dev, line);
  }
  LOG_TRACE("Set node invalid at dev %d addr %#x", dev, addr);
}

void mc_mgr_rdm_invalidate_node(int sid, bf_dev_id_t dev, uint32_t addr) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  set_node_invalid(dev, rdm_map, addr);

  /* Append to the write list. */
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  bool full_width = false;
  if (addr & 1) {
  } else {
    full_width = line->type[0] == mc_mgr_rdm_node_type_rid ||
                 line->type[0] == mc_mgr_rdm_node_type_xid ||
                 line->type[0] == mc_mgr_rdm_node_type_ecmp ||
                 line->type[0] == mc_mgr_rdm_node_type_ecmp_xid ||
                 line->type[0] == mc_mgr_rdm_node_type_vector ||
                 line->type[0] == mc_mgr_rdm_node_type_port72;
  }
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  if (full_width) {
    LOG_TRACE("Clear node pair at dev %d addr %#x", dev, addr & 0xFFFFFFFE);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[0]);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
  } else {
    LOG_TRACE("Clear node at dev %d addr %#x (%s/%s)",
              dev,
              addr,
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]));
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[addr & 1]);
  }
  return;
}

void mc_mgr_rdm_write_l1(int sid,
                         bf_dev_id_t dev,
                         uint32_t addr,
                         uint32_t next_l1,
                         uint32_t l2,
                         uint16_t rid,
                         uint16_t xid,
                         bool xid_val) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr || addr & 1) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_node_type_e node_type =
      xid_val ? mc_mgr_rdm_node_type_xid : mc_mgr_rdm_node_type_rid;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[1] ||
      (node_type != line->type[0] &&
       mc_mgr_rdm_node_type_invalid != line->type[0])) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[0] ||
                  node_type == line->type[0]);
    return;
  }

  /* Fill in the structure representation. */
  line->type[0] = node_type;
  if (xid_val) {
    line->u.xid.next_l1 = next_l1;
    line->u.xid.next_l2 = l2;
    line->u.xid.rid = rid;
    line->u.xid.xid = xid;
  } else {
    line->u.rid.next_l1 = next_l1;
    line->u.rid.next_l2 = l2;
    line->u.rid.rid = rid;
  }

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  if (xid_val) {
    LOG_TRACE(
        "Write L1Xid node at addr %#x, rid %#x, xid %#x, l2 %#x, "
        "next %#x",
        addr,
        rid,
        xid,
        l2,
        next_l1);
  } else {
    LOG_TRACE("Write L1Rid node at addr %#x, rid %#x, l2 %#x, next %#x",
              addr,
              rid,
              l2,
              next_l1);
  }
  return;
}

void mc_mgr_rdm_write_ptr(int sid,
                          bf_dev_id_t dev,
                          uint32_t addr,
                          uint32_t p0,
                          uint32_t p1,
                          uint32_t next,
                          uint16_t xid,
                          bool use_xid) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr || addr & 1) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_node_type_e node_type =
      use_xid ? mc_mgr_rdm_node_type_ecmp_xid : mc_mgr_rdm_node_type_ecmp;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[1] ||
      (node_type != line->type[0] &&
       mc_mgr_rdm_node_type_invalid != line->type[0])) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[0] ||
                  node_type == line->type[0]);
    return;
  }

  /* Fill in the structure representation. */
  line->type[0] = node_type;
  if (use_xid) {
    line->u.ecmp_xid.next_l1 = next;
    line->u.ecmp_xid.vector0 = p0;
    line->u.ecmp_xid.vector1 = p1;
    line->u.ecmp_xid.xid = xid;
  } else {
    line->u.ecmp.next_l1 = next;
    line->u.ecmp.vector0 = p0;
    line->u.ecmp.vector1 = p1;
  }

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  if (use_xid) {
    LOG_TRACE(
        "Write ECMP Ptr node at addr %#x, vec %#x/%#x XID 0x%04x next %#x",
        addr,
        p0,
        p1,
        xid,
        next);
  } else {
    LOG_TRACE("Write ECMP Ptr node at addr %#x, vec %#x/%#x next %#x",
              addr,
              p0,
              p1,
              next);
  }
  return;
}

uint16_t mc_mgr_rdm_get_id_from_vec(mc_mgr_rdm_t *rdm_map, uint32_t addr) {
  MC_MGR_DBGCHK(0 == (addr & 1));
  MC_MGR_DBGCHK(mc_mgr_rdm_node_type_vector == mc_mgr_rdm_type(rdm_map, addr));
  return rdm_map->rdm[addr / 2].u.vector.id;
}

void mc_mgr_rdm_write_vec(int sid,
                          bf_dev_id_t dev,
                          uint32_t addr,
                          uint32_t vec,
                          uint32_t base,
                          uint16_t id) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr || addr & 1) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_node_type_e node_type = mc_mgr_rdm_node_type_vector;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[1] ||
      (node_type != line->type[0] &&
       mc_mgr_rdm_node_type_invalid != line->type[0])) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[0] ||
                  node_type == line->type[0]);
    return;
  }

  /* Fill in the structure representation. */
  line->type[0] = node_type;
  line->u.vector.base_l1 = base;
  line->u.vector.length = 31;
  line->u.vector.vector = vec;
  line->u.vector.id = id;

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE(
      "Write Vector node at addr %#x, vec 0x%08x, base %#x, id %#x, dev %d",
      addr,
      vec,
      base,
      id,
      dev);

  return;
}

void mc_mgr_rdm_write_l1_end(
    int sid, bf_dev_id_t dev, uint32_t addr, uint32_t l2, uint16_t rid) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    return;
  }

  mc_mgr_rdm_node_type_e node_type = mc_mgr_rdm_node_type_end;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[addr & 1] &&
      mc_mgr_rdm_node_type_end != line->type[addr & 1]) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[addr & 1] ||
                  mc_mgr_rdm_node_type_end == line->type[addr & 1]);
    return;
  }
  /* Ensure the line is not occupied by a full width node. */
  if (!line_is_shareable(line)) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(line_is_shareable(line));
    return;
  }

  /* Fill in the structure representation. */
  line->type[addr & 1] = node_type;
  line->u.end[addr & 1].next_l2 = l2;
  line->u.end[addr & 1].rid = rid;

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE("Write L1End node at addr %#x, rid %#x, l2 %#x", addr, rid, l2);
  return;
}

void mc_mgr_rdm_write_port72(int sid,
                             bf_dev_id_t dev,
                             uint32_t addr,
                             uint8_t pipe,
                             bool last,
                             bf_bitset_t *ports) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr || addr & 1) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[1] ||
      (mc_mgr_rdm_node_type_port72 != line->type[0] &&
       mc_mgr_rdm_node_type_invalid != line->type[0])) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[1]);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[0] ||
                  mc_mgr_rdm_node_type_port72 == line->type[0]);
    return;
  }

  /* Fill in the structure representation. */
  line->type[0] = mc_mgr_rdm_node_type_port72;
  line->u.port72.pipe = pipe;
  line->u.port72.last = last;
  line->u.port72.spv = bf_bs_get_word(ports, 64, 8);
  line->u.port72.ports = bf_bs_get_word(ports, 0, 64);

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE("Write Port72 0x%02x_%016" PRIx64
            " pipe %d last %d node at addr %#x 0x%04" PRIx64 "_%016" PRIx64,
            line->u.port72.spv,
            line->u.port72.ports,
            line->u.port72.pipe,
            line->u.port72.last,
            addr,
            line->data[1],
            line->data[0]);
  return;
}

void mc_mgr_rdm_write_port18(int sid,
                             bf_dev_id_t dev,
                             uint8_t pipe,
                             uint32_t addr,
                             bool last,
                             bf_bitset_t *ports) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    return;
  }

  mc_mgr_rdm_node_type_e node_type = mc_mgr_rdm_node_type_port18;

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[addr & 1] &&
      node_type != line->type[addr & 1]) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[addr & 1] ||
                  node_type == line->type[addr & 1]);
    return;
  }
  /* Ensure the line is not occupied by a full width node. */
  if (!line_is_shareable(line)) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(line_is_shareable(line));
    return;
  }

  /* Fill in the structure representation. */
  line->type[addr & 1] = node_type;
  line->u.port18[addr & 1].pipe = pipe;
  line->u.port18[addr & 1].last = last;
  line->u.port18[addr & 1].spv = 0;
  line->u.port18[addr & 1].ports = 0;

  uint64_t spv = bf_bs_get_word(ports, 64, 8);
  for (int i = 0; spv && i < 8; i += 4) {
    if (spv & (1u << i)) line->u.port18[addr & 1].spv |= (1u << i / 4);
  }
  uint64_t npv = bf_bs_get_word(ports, 0, 64);
  for (int i = 0; npv && i < 64; i += 4) {
    if (npv & (1ULL << i)) line->u.port18[addr & 1].ports |= (1ULL << i / 4);
  }

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE(
      "Write Port18 0x%02x_%04x pipe %d last %d node at addr 0x%x "
      "0x%04" PRIx64 "_%016" PRIx64,
      line->u.port18[addr & 1].spv,
      line->u.port18[addr & 1].ports,
      line->u.port18[addr & 1].pipe,
      line->u.port18[addr & 1].last,
      addr,
      line->data[1],
      line->data[0]);
  return;
}

void mc_mgr_rdm_write_lag(
    int sid, bf_dev_id_t dev, uint32_t addr, uint8_t lag_id, uint32_t next_l2) {
  if (MC_MGR_INVALID_DEV(dev)) {
    LOG_ERROR("Invalid device %d at %s:%d", dev, __func__, __LINE__);
    MC_MGR_DBGCHK(!MC_MGR_INVALID_DEV(dev));
    return;
  }
  if (!addr) {
    LOG_ERROR("Invalid RDM address %#x at %s:%d", addr, __func__, __LINE__);
    MC_MGR_DBGCHK(addr);
    MC_MGR_DBGCHK(0 == (addr & 1));
    return;
  }

  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  mc_mgr_rdm_line_t *line = &rdm_map->rdm[addr / 2];
  /* Can either write a new node or modify a node of the same type. */
  if (mc_mgr_rdm_node_type_invalid != line->type[addr & 1] &&
      mc_mgr_rdm_node_type_lag != line->type[addr & 1]) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(mc_mgr_rdm_node_type_invalid == line->type[addr & 1] ||
                  mc_mgr_rdm_node_type_lag == line->type[addr & 1]);
    return;
  }
  /* Ensure the line is not occupied by a full width node. */
  if (!line_is_shareable(line)) {
    LOG_ERROR("Overwritting existing node (%s/%s) dev %d, addr %#x at %s:%d",
              MC_MGR_RDM_NODE_TYPE_STR(line->type[0]),
              MC_MGR_RDM_NODE_TYPE_STR(line->type[1]),
              dev,
              addr,
              __func__,
              __LINE__);
    MC_MGR_DBGCHK(line_is_shareable(line));
    return;
  }

  /* Fill in the structure representation. */
  line->type[addr & 1] = mc_mgr_rdm_node_type_lag;
  line->u.lag[addr & 1].next_l2 = next_l2;
  line->u.lag[addr & 1].lag_id = lag_id;

  /* Fill in the bit vector representation. */
  encode_line(dev, line);

  /* Append to the write list. */
  mc_mgr_set_rdm_wrl(sid, dev, addr / 2, line->data[1], line->data[0]);
  LOG_TRACE(
      "Write LAG node at addr %#x with id %3d and next node %#x "
      "0x%04" PRIx64 "_%016" PRIx64,
      addr,
      lag_id,
      next_l2,
      line->data[1],
      line->data[0]);
  return;
}

bf_status_t mc_mgr_rdm_decode_line(bf_dev_id_t dev, mc_mgr_rdm_line_t *line) {
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      return tof_mc_mgr_rdm_decode_line(line);
    case BF_DEV_FAMILY_TOFINO2:
      return tof2_mc_mgr_rdm_decode_line(line);
    case BF_DEV_FAMILY_TOFINO3:
      return tof3_mc_mgr_rdm_decode_line(line);

    case BF_DEV_FAMILY_UNKNOWN:
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  return BF_INVALID_ARG;
}

static void encode_line(bf_dev_id_t dev, mc_mgr_rdm_line_t *line) {
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      tof_mc_mgr_rdm_encode_line(line);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      tof2_mc_mgr_rdm_encode_line(line);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      tof3_mc_mgr_rdm_encode_line(line);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      MC_MGR_DBGCHK(0);
      break;
  }
}

static bool is_rdm_addr_valid(bf_dev_id_t dev, uint32_t addr) {
  bool is_addr_valid;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    is_addr_valid = tof_rdm_addr_valid(addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    is_addr_valid = tof2_rdm_addr_valid(addr);
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    is_addr_valid = tof3_rdm_addr_valid(addr);
  } else {
    return false;
  }
  return is_addr_valid;
}
static bool get_l2_block_size(int sid,
                              bf_dev_id_t dev,
                              mc_mgr_rdm_t *rdm_map,
                              uint32_t l2_addr,
                              bool read_hw,
                              int *size,
                              int *cnt) {
  *size = 0;
  *cnt = 0;
  while (is_rdm_addr_valid(dev, l2_addr)) {
    /* Read the line from HW if needed. */
    int line = rdm_addr_to_line(dev, l2_addr);
    if (read_hw && rdm_line_invalid(&rdm_map->rdm[line])) {
      mc_mgr_get_rdm_reg(sid,
                         dev,
                         line,
                         &rdm_map->rdm[line].data[1],
                         &rdm_map->rdm[line].data[0]);
      mc_mgr_rdm_decode_line(dev, &rdm_map->rdm[line]);
    }
    /* Decode based on node type.
     * LAG nodes are always size one.
     * Port nodes can be of size one or two, they are also strung together into
     * blocks based on a "last" bit in the node.
     * Other node types are ignored. */
    mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(rdm_map, l2_addr);
    if (mc_mgr_rdm_node_type_lag == t) {
      if (*size) {
        return false;
      } else {
        *size = 1;
        *cnt = 1;
        break;
      }
    } else if (mc_mgr_rdm_node_type_port18 == t) {
      *size += 1;
      *cnt += 1;
      if (rdm_map->rdm[line].u.port18[l2_addr & 1].last) break;
      l2_addr += 1;
    } else if (mc_mgr_rdm_node_type_port72 == t) {
      *size += 2;
      *cnt += 1;
      if (rdm_map->rdm[line].u.port72.last) break;
      l2_addr += 2;
    } else {
      return false;
    }
  }
  return true;
}

bool sync_l2_chain(int sid,
                   bf_dev_id_t dev,
                   int pipe,
                   struct mc_mgr_dev_hw_state *st,
                   mc_mgr_rdm_addr_list_t **addr_list,
                   bf_bitset_t *ports,
                   bf_bitset_t *lags,
                   uint32_t l1_addr) {
  bool okay = true;
  /* Get the head of the L2 chain. */
  uint32_t l2_addr = mc_mgr_rdm_next_l2(st->rdm_map, l1_addr, true);
  while (is_rdm_addr_valid(dev, l2_addr)) {
    /* Get the size of this L2 node allocation, it could be a block of port
     * nodes or it could be a single LAG node.  This will return false if the
     * address isn't occupied by an L2 node or if it is not correctly formed
     * for example a port node without "last" set followed by a LAG node. */
    int l2_blk_len = 0, l2_blk_sz = 0;
    okay = get_l2_block_size(
        sid, dev, st->rdm_map, l2_addr, true, &l2_blk_sz, &l2_blk_len);
    if (!okay) break;
    /* Make sure this L2 chain isn't looping. */
    okay = !mc_mgr_rdm_addr_in(*addr_list, l2_addr);
    if (!okay) break;
    /* Log this address so it can be reserved later. */
    mc_mgr_rdm_addr_append(addr_list, l2_addr);
    /* Save the next L2 pointer. */
    uint32_t l2_next = mc_mgr_rdm_next_l2(st->rdm_map, l2_addr, true);
    /* If this is a LAG node add the LAG to the member list.
     * If this is a port node add the ports to the member list. */
    mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, l2_addr);
    if (mc_mgr_rdm_node_type_lag == t) {
      int id = st->rdm_map->rdm[rdm_addr_to_line(dev, l2_addr)]
                   .u.lag[l2_addr & 1]
                   .lag_id;
      /* Cannot have a member twice in the same pipe! */
      okay = !bf_bs_get(lags, id);
      bf_bs_set(lags, id, 1);
      LOG_DBG(
          "Syncing Mbr : dev %d.%d node-addr %#x mbr-addr %#x lag-id %d, sts "
          "%c next %#x",
          dev,
          pipe,
          l1_addr,
          l2_addr,
          id,
          okay ? 'T' : 'F',
          l2_next);
    } else if (mc_mgr_rdm_node_type_port18 == t) {
      okay = extract_port18_ports(dev, st->rdm_map, l2_addr, ports);
      LOG_DBG(
          "Syncing Mbr : dev %d.%d node-addr %#x mbr-addr %#x port18, sts %c "
          "next %#x",
          dev,
          pipe,
          l1_addr,
          l2_addr,
          okay ? 'T' : 'F',
          l2_next);
      int node_pipe = get_pipe_from_port_node(st->rdm_map, l2_addr);
      if (okay && pipe != node_pipe) {
        /* Nodes are for a different pipe, make sure that at least one port set
         * has a backup port in this pipe.  Note that it is at least one rather
         * than all because we simply copy the entire port membership of the
         * remote pipe rather than picking wich members have backup pipes here
         * and writting only those. */
        int local_port = -1;
        bool is_node_bkup_correct = false;
        for (local_port = bf_bs_first_set(&ports[node_pipe], local_port);
             local_port != -1;
             local_port = bf_bs_first_set(&ports[node_pipe], local_port)) {
          bf_dev_port_t dev_port =
              mc_make_dev_port(dev, node_pipe, 4 * local_port);
          uint16_t port_index = mc_dev_port_to_bit_idx(dev, dev_port);
/* GCC 4.9.2 gives a false positive warning for "array subscript is above array
 * bounds".  Likely related to this bug:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56273
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
          if ((unsigned int)pipe ==
              mc_bit_idx_to_pipe(dev, st->bkup_ports[port_index])) {
            is_node_bkup_correct = true;
            break;
          }
#pragma GCC diagnostic pop
        }
        okay = is_node_bkup_correct;
      }
    } else if (mc_mgr_rdm_node_type_port72 == t) {
      okay = extract_port72_ports(dev, st->rdm_map, l2_addr, ports);
      LOG_DBG(
          "Syncing Mbr : dev %d.%d node-addr %#x mbr-addr %#x port72, sts %c "
          "next %#x P0 %02" PRIx64 "_%016" PRIx64 " P1 %02" PRIx64
          "_%016" PRIx64 " P2 %02" PRIx64 "_%016" PRIx64 " P3 %02" PRIx64
          "_%016" PRIx64,
          dev,
          pipe,
          l1_addr,
          l2_addr,
          okay ? 'T' : 'F',
          l2_next,
          ports[0].bs[1],
          ports[0].bs[0],
          ports[1].bs[1],
          ports[1].bs[0],
          ports[2].bs[1],
          ports[2].bs[0],
          ports[3].bs[1],
          ports[3].bs[0]);
      int node_pipe = get_pipe_from_port_node(st->rdm_map, l2_addr);
      if (okay && pipe != node_pipe) {
        /* Nodes are for a different pipe, make sure that at least one port set
         * has a backup port in this pipe.  Note that it is at least one rather
         * than all because we simply copy the entire port membership of the
         * remote pipe rather than picking wich members have backup pipes here
         * and writting only those. */
        int local_port = -1;
        bool is_node_bkup_correct = false;
        for (local_port = bf_bs_first_set(&ports[node_pipe], local_port);
             local_port != -1;
             local_port = bf_bs_first_set(&ports[node_pipe], local_port)) {
          bf_dev_port_t dev_port = mc_make_dev_port(dev, node_pipe, local_port);
          uint16_t port_index = mc_dev_port_to_bit_idx(dev, dev_port);
/* GCC 4.9.2 gives a false positive warning for "array subscript is above array
 * bounds".  Likely related to this bug:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56273
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
          if ((unsigned int)pipe ==
              mc_bit_idx_to_pipe(dev, st->bkup_ports[port_index])) {
            is_node_bkup_correct = true;
            break;
          }
#pragma GCC diagnostic pop
        }
        okay = is_node_bkup_correct;
      }
    } else {
      LOG_DBG("Unexpected node type dev %d.%d node-addr %#x mbr-addr %#x",
              dev,
              pipe,
              l1_addr,
              l2_addr);
      okay = false;
    }
    if (!okay) break;
    /* Follow the next L2 pointer. */
    l2_addr = l2_next;
  }
  return okay;
}

static mc_ecmp_grp_t *sync_ecmp_grp(int sid,
                                    bf_dev_id_t dev,
                                    int pipe,
                                    struct mc_mgr_dev_hw_state *st,
                                    bf_map_t *ecmp_grps,
                                    uint32_t vec_0_addr,
                                    uint32_t vec_1_addr) {
  int i = 0, j = 0;
  int mbrs_added_to_rdm_map = 0;
  bool rsrvd_v0 = false;
  bool rsrvd_v1 = false;
  bool is_addrs_invalid;

  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO) {
    is_addrs_invalid =
        (!tof_rdm_addr_valid(vec_0_addr) || !tof_rdm_addr_valid(vec_1_addr));
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2) {
    is_addrs_invalid =
        (!tof2_rdm_addr_valid(vec_0_addr) || !tof2_rdm_addr_valid(vec_1_addr));
  } else if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    is_addrs_invalid =
        (!tof3_rdm_addr_valid(vec_0_addr) || !tof3_rdm_addr_valid(vec_1_addr));
  } else {
    is_addrs_invalid = true;
  }
  /* Addresses must be valid. */
  if (is_addrs_invalid) {
    LOG_DBG("Invalid vector addresses (%#x %#x) for ECMP group, dev %d.%d",
            vec_0_addr,
            vec_1_addr,
            dev,
            pipe);
    return NULL;
  }

  /* Read the two vector nodes if they haven't been read from hardware yet. */
  int line_0 = rdm_addr_to_line(dev, vec_0_addr);
  int line_1 = rdm_addr_to_line(dev, vec_1_addr);
  if (mc_mgr_rdm_node_type_vector != mc_mgr_rdm_type(st->rdm_map, vec_0_addr) ||
      mc_mgr_rdm_node_type_vector != mc_mgr_rdm_type(st->rdm_map, vec_1_addr)) {
    mc_mgr_get_rdm_reg(sid,
                       dev,
                       line_0,
                       &st->rdm_map->rdm[line_0].data[1],
                       &st->rdm_map->rdm[line_0].data[0]);
    mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[line_0]);
    mc_mgr_get_rdm_reg(sid,
                       dev,
                       line_1,
                       &st->rdm_map->rdm[line_1].data[1],
                       &st->rdm_map->rdm[line_1].data[0]);
    mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[line_1]);
    if (mc_mgr_rdm_node_type_vector !=
        mc_mgr_rdm_type(st->rdm_map, vec_0_addr)) {
      LOG_DBG(
          "Unexpected node type %s on dev %d.%d for vec0 at RDM #%x",
          MC_MGR_RDM_NODE_TYPE_STR(mc_mgr_rdm_type(st->rdm_map, vec_0_addr)),
          dev,
          pipe,
          vec_0_addr);
      return NULL;
    } else if (mc_mgr_rdm_node_type_vector !=
               mc_mgr_rdm_type(st->rdm_map, vec_1_addr)) {
      LOG_DBG(
          "Unexpected node type %s on dev %d.%d for vec1 at RDM #%x",
          MC_MGR_RDM_NODE_TYPE_STR(mc_mgr_rdm_type(st->rdm_map, vec_1_addr)),
          dev,
          pipe,
          vec_1_addr);
      return NULL;
    }
  }

  /* Get the ECMP group Id from the vector nodes.  It must be the same in both
   * vector nodes. */
  uint16_t ecmp_id = mc_mgr_rdm_get_id_from_vec(st->rdm_map, vec_0_addr);
  uint16_t ecmp_id2 = mc_mgr_rdm_get_id_from_vec(st->rdm_map, vec_1_addr);
  if (ecmp_id != ecmp_id2) {
    LOG_DBG("ECMP vectors mismatch, %#x != %#x, vec0/1 %#x %#x on dev %d.%d",
            ecmp_id,
            ecmp_id2,
            vec_0_addr,
            vec_1_addr,
            dev,
            pipe);
    return NULL;
  }

  /* Check if this group has already been read for a different MGID associated
   * L1-ECMP node in the same pipe. */
  mc_ecmp_grp_t *g = NULL;
  bf_map_sts_t s = bf_map_get(ecmp_grps, ecmp_id, (void **)&g);
  if (s == BF_MAP_OK) {
    /* We've seen the group before, make sure the vector nodes have the correct
     * addresses. */
    if (g->vector_node_addr[0][pipe] == vec_0_addr &&
        g->vector_node_addr[1][pipe] == vec_1_addr) {
      return g;
    } else {
      LOG_DBG(
          "Vector nodes (%#x %#x) don't agree w/ group %#x %#x, grp-id %#x on "
          "dev %d.%d",
          vec_0_addr,
          vec_1_addr,
          g->vector_node_addr[0][pipe],
          g->vector_node_addr[1][pipe],
          g->ecmp_id,
          dev,
          pipe);
      return NULL;
    }
  } else if (s == BF_MAP_NO_KEY) {
    /* This is a new ECMP group. */
  } else {
    /* Some other problem... */
    return NULL;
  }

  /* Skip invalid nodes (base-l1 is zero or members is zero). */
  uint32_t mbrs = st->rdm_map->rdm[line_0].u.vector.vector;
  uint32_t base = st->rdm_map->rdm[line_0].u.vector.base_l1;
  if (!base || !mbrs) {
    LOG_DBG("Dropping bad ECMP group, mbrship 0x%08x, base %#x, on dev %d.%d",
            mbrs,
            base,
            dev,
            pipe);
    return NULL;
  }

  /* Allocate a new group. */
  g = MC_MGR_MALLOC(sizeof(mc_ecmp_grp_t));
  if (!g) {
    LOG_ERROR(
        "Failed to allocate memory for new ECMP group on dev %d.%d", dev, pipe);
    return NULL;
  }
  MC_MGR_MEMSET(g, 0, sizeof(mc_ecmp_grp_t));
  g->dev = dev;
  g->ecmp_id = ecmp_id;
  g->base[pipe] = base;
  g->valid_map = mbrs;
  g->vector_node_addr[0][pipe] = vec_0_addr;
  g->vector_node_addr[1][pipe] = vec_1_addr;

  /* Reserve memory for the vector nodes. */
  bool okay;
  int vec_0_sz = node_width(mc_mgr_rdm_type(st->rdm_map, vec_0_addr));
  int vec_1_sz = node_width(mc_mgr_rdm_type(st->rdm_map, vec_1_addr));
  okay = mc_mgr_mark_addr_used(
      st->rdm_map, dev, vec_0_addr, pipe, vec_0_sz, false);
  if (!okay) {
    LOG_DBG(
        "Dropping bad group %#x, RDM %#x is is not available for vec0 on dev "
        "%d.%d",
        g->ecmp_id,
        vec_0_addr,
        dev,
        pipe);
    goto free_group;
  }
  rsrvd_v0 = true;
  okay = mc_mgr_mark_addr_used(
      st->rdm_map, dev, vec_1_addr, pipe, vec_1_sz, false);
  if (!okay) {
    LOG_DBG(
        "Dropping bad group %#x, RDM %#x is is not available for vec1 on dev "
        "%d.%d",
        g->ecmp_id,
        vec_1_addr,
        dev,
        pipe);
    goto free_group;
  }
  rsrvd_v1 = true;

  /* Check for empty groups is not needed now as ecmp_grp vector is not present
   * in all pipes.  */
  mc_mgr_rdm_addr_list_t *l2_addrs = NULL;
  /* For each member (bit set in the vector) check that the L1 node exists and
   * check its L2 chain. */
  bool l1_okay = true, l2_okay = true;
  for (i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
    if (!(mbrs & (1u << i))) continue;
    j = i;
    uint32_t mbr_addr = base + i;
    int mbr_line = rdm_addr_to_line(dev, mbr_addr);
    mc_mgr_get_rdm_reg(sid,
                       dev,
                       mbr_line,
                       &st->rdm_map->rdm[mbr_line].data[1],
                       &st->rdm_map->rdm[mbr_line].data[0]);
    mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[mbr_line]);
    mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, mbr_addr);
    if (mc_mgr_rdm_node_type_end != t) {
      LOG_DBG(
          "Dropping bad group %#x, mbr %d has unexpected type %s at %#x on "
          "dev %d.%d",
          g->ecmp_id,
          i,
          MC_MGR_RDM_NODE_TYPE_STR(t),
          mbr_addr,
          dev,
          pipe);
      l1_okay = false;
      break;
    }
    /* Allocate the L1 member node. */
    g->mbrs[i] = MC_MGR_MALLOC(sizeof(mc_l1_node_t));
    if (!g->mbrs[i]) {
      LOG_ERROR(
          "Failed to allocate memory for ECMP mbr %d in grp %#x on dev %d.%d",
          i,
          g->ecmp_id,
          dev,
          pipe);
      goto free_group;
    }
    MC_MGR_MEMSET(g->mbrs[i], 0, sizeof(mc_l1_node_t));
    g->mbrs[i]->rid = st->rdm_map->rdm[mbr_line].u.end[mbr_addr & 1].rid;
    g->mbrs[i]->dev = dev;
    g->mbrs[i]->mgid = -1;
    g->mbrs[i]->ecmp_grp = g;

    /* Read the L2 chain. */
    bf_bitset_t ports[MC_MGR_NUM_PIPES];
    int p;
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      bf_bs_init(&ports[p], BF_PIPE_PORT_COUNT, g->mbrs[i]->l2_chain.ports[p]);
    }
    bf_bitset_t lags;
    bf_bs_init(&lags, BF_LAG_COUNT, g->mbrs[i]->l2_chain.lags);
    l2_okay =
        sync_l2_chain(sid, dev, pipe, st, &l2_addrs, ports, &lags, mbr_addr);
    if (!l2_okay) {
      LOG_DBG(
          "Dropping bad group %#x, mbr %d at %#x has unexpected L2-chain on "
          "dev %d.%d",
          g->ecmp_id,
          i,
          mbr_addr,
          dev,
          pipe);
      break;
    }
    LOG_DBG("Syncing ECMP Mbr: dev %d.%d ecmp-id %#x mbr %d addr %#x RID %#x",
            dev,
            pipe,
            g->ecmp_id,
            i,
            mbr_addr,
            g->mbrs[i]->rid);
  }

  /* Add the members to the RDM-to-Node mapping. */
  for (i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
    if (!g->mbrs[i]) continue;
    s = bf_map_add(&st->rdm_to_node, base + i, g->mbrs[i]);
    if (s != BF_MAP_OK) {
      LOG_ERROR(
          "Failed to cache ECMP mbr node; group %#x mbr %d at %#x on dev "
          "%d.%d",
          g->ecmp_id,
          i,
          base + i,
          dev,
          pipe);
      MC_MGR_DBGCHK(s == BF_MAP_OK);
      goto free_group;
    }
    mbrs_added_to_rdm_map++;
  }

  if (!l1_okay || !l2_okay) goto free_group;

  /* Reserve memory for the L1 node block. */
  g->allocated_sz = j + 1;
  okay = mc_mgr_mark_addr_used(
      st->rdm_map, dev, base, pipe, g->allocated_sz, false);
  if (!okay) {
    LOG_DBG(
        "Dropping bad group %#x, RDM block %#x to %#x is not available on "
        "dev %d.%d",
        g->ecmp_id,
        base,
        base + g->allocated_sz - 1,
        dev,
        pipe);
    goto free_group;
  }
  /* Reserve memory for the L2 nodes. */
  mc_mgr_rdm_addr_list_t *x;
  for (x = l2_addrs; x; x = x->next) {
    /* Get the size of each L2 node block so it can be reserved. */
    int l2_size = 0, l2_len = 0;
    get_l2_block_size(sid, dev, st->rdm_map, x->addr, false, &l2_size, &l2_len);
    okay =
        mc_mgr_mark_addr_used(st->rdm_map, dev, x->addr, pipe, l2_size, true);
    if (!okay) {
      LOG_DBG(
          "Dropping bad group %#x, L2 addr %#x is not available on dev %d.%d",
          g->ecmp_id,
          x->addr,
          dev,
          pipe);
      /* Release all the L2 node addrs already reserved. */
      mc_mgr_rdm_addr_list_t *y;
      for (y = l2_addrs; y != x; y = y->next) {
        mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, y->addr, pipe);
      }
      /* Release the L1 node addr. */
      mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, base, pipe);
      goto free_group;
    }
  }
  /* If the bases are different or the vectors are different then assume v0 is
   * correct. */
  if (st->rdm_map->rdm[line_0].u.vector.vector !=
          st->rdm_map->rdm[line_1].u.vector.vector ||
      st->rdm_map->rdm[line_0].u.vector.length !=
          st->rdm_map->rdm[line_1].u.vector.length ||
      st->rdm_map->rdm[line_0].u.vector.base_l1 !=
          st->rdm_map->rdm[line_1].u.vector.base_l1) {
    st->rdm_map->rdm[line_1].u.vector.vector =
        st->rdm_map->rdm[line_0].u.vector.vector;
    st->rdm_map->rdm[line_1].u.vector.length =
        st->rdm_map->rdm[line_0].u.vector.length;
    st->rdm_map->rdm[line_1].u.vector.base_l1 =
        st->rdm_map->rdm[line_0].u.vector.base_l1;
    bf_map_add(&st->dirty_rdm, vec_1_addr, NULL);
  }

  /* Add the group to the list of all ECMP groups read from hardware. */
  bf_map_add(ecmp_grps, ecmp_id, (void *)g);
  LOG_DBG(
      "Syncing ECMP Grp: dev %d.%d ecmp-id %#x mbr-map 0x%08x addrs: v0 %#x v1 "
      "%#x mbr at %#x-%#x sz %d",
      dev,
      pipe,
      g->ecmp_id,
      g->valid_map,
      g->vector_node_addr[0][pipe],
      g->vector_node_addr[1][pipe],
      g->base[pipe],
      g->base[pipe] + g->allocated_sz - (g->allocated_sz ? 1 : 0),
      g->allocated_sz);
  return g;

free_group:
  /* Release the vector node addrs. */
  if (rsrvd_v0) mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, vec_0_addr, pipe);
  if (rsrvd_v1) mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, vec_1_addr, pipe);
  for (i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
    if (g->mbrs[i]) {
      MC_MGR_FREE(g->mbrs[i]);
      g->mbrs[i] = NULL;
    }
  }
  for (i = 0; i < MC_ECMP_GROUP_MAX_MBR && mbrs_added_to_rdm_map; ++i) {
    if (!g->mbrs[i]) continue;
    bf_map_rmv(&st->rdm_to_node, base + i);
    mbrs_added_to_rdm_map--;
  }
  MC_MGR_FREE(g);
  uint32_t junk;
  while (l2_addrs) mc_mgr_rdm_addr_pop(&l2_addrs, &junk);
  return NULL;
}

/* Return ECMP ptr in "node", also optionally add a new grp to ecmp_grps. */
static bool sync_l1_ecmp_node(int sid,
                              bf_dev_id_t dev,
                              int pipe,
                              int mgid,
                              struct mc_mgr_dev_hw_state *st,
                              bf_map_t *ecmp_grps,
                              mc_l1_node_t **node,
                              uint32_t l1_addr) {
  /* Create placeholders for the state that will be read from the node. */
  mc_l1_node_t *n = MC_MGR_MALLOC(sizeof(mc_l1_node_t));
  if (!n) {
    LOG_ERROR(
        "Failed to allocate memory for ecmp-ptr node at RDM %#x for dev %d.%d "
        "mgid %#x",
        l1_addr,
        dev,
        pipe,
        mgid);
    return false;
  }
  *node = n;
  MC_MGR_MEMSET(n, 0, sizeof(mc_l1_node_t));
  n->mgid = mgid;
  n->dev = dev;
  n->rid = 0xDEAD;
  n->hw_nodes[pipe].rdm_addr = l1_addr;
  n->hw_nodes[pipe].sw_node = n;

  /* Get the XID and vector addresses from the L1 pointer node. */
  int line = rdm_addr_to_line(dev, l1_addr);
  uint32_t addr_0 = 0, addr_1 = 0;
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, l1_addr);
  int width = node_width(t);
  if (mc_mgr_rdm_node_type_ecmp == t) {
    addr_0 = st->rdm_map->rdm[line].u.ecmp.vector0;
    addr_1 = st->rdm_map->rdm[line].u.ecmp.vector1;
  } else if (mc_mgr_rdm_node_type_ecmp_xid == t) {
    addr_0 = st->rdm_map->rdm[line].u.ecmp_xid.vector0;
    addr_1 = st->rdm_map->rdm[line].u.ecmp_xid.vector1;
    n->xid = st->rdm_map->rdm[line].u.ecmp_xid.xid;
    n->xid_valid = true;
  } else {
    LOG_DBG(
        "Expected ECMP ptr node but got %s at RDM %#x for dev %d.%d mgid %#x",
        MC_MGR_RDM_NODE_TYPE_STR(t),
        l1_addr,
        dev,
        pipe,
        mgid);
    goto not_okay;
  }

  /* Use the vector pointers to read the ECMP group.  This may be a new group
   * or an existing group that is referenced from multiple L1 chains. */
  n->ecmp_grp = sync_ecmp_grp(sid, dev, pipe, st, ecmp_grps, addr_0, addr_1);
  if (!n->ecmp_grp) {
    LOG_DBG(
        "No group for ecmp-ptr node at RDM %#x for dev %d.%d mgid %#x, vec0/1 "
        "%#x %#x",
        l1_addr,
        dev,
        pipe,
        mgid,
        addr_0,
        addr_1);
    goto not_okay;
  }

  /* Add the ECMP pointer node to the RDM-to-node map. */
  bf_map_sts_t s = bf_map_add(&st->rdm_to_node, l1_addr, n);
  if (s != BF_MAP_OK) {
    LOG_ERROR(
        "Failed to cache ecmp-ptr node at RDM %#x for dev %d.%d mgid %#x grp "
        "%#x vec0/1 %#x %#x sts %d",
        l1_addr,
        dev,
        pipe,
        mgid,
        n->ecmp_grp->ecmp_id,
        addr_0,
        addr_1,
        s);
    MC_MGR_FREE(n);
    *node = NULL;
    MC_MGR_DBGCHK(s == BF_MAP_OK);
    return false;
  }

  /* Reserve L1 address. */
  bool okay =
      mc_mgr_mark_addr_used(st->rdm_map, dev, l1_addr, pipe, width, false);
  if (!okay) {
    LOG_DBG("Syncing ECMP Node: dev %d.%d mgid %#x addr %#x RDM conflict",
            dev,
            pipe,
            mgid,
            l1_addr);
    goto not_okay;
  }

  /* Link the ECMP pointer onto the group. */
  BF_LIST_DLL_PP(n->ecmp_grp->refs, n, ecmp_next, ecmp_prev);

  LOG_DBG(
      "Syncing ECMP Ptr: dev %d.%d mgid %#x addr %#x XID %#x XID-valid %d "
      "ecmp-id %#x",
      dev,
      pipe,
      mgid,
      l1_addr,
      n->xid,
      n->xid_valid,
      n->ecmp_grp->ecmp_id);
  return true;

not_okay:
  set_node_invalid(dev, st->rdm_map, l1_addr);
  bf_map_rmv(&st->rdm_to_node, l1_addr);
  MC_MGR_FREE(n);
  *node = NULL;
  return false;
}
static bool sync_l1_node(int sid,
                         bf_dev_id_t dev,
                         int pipe,
                         int mgid,
                         struct mc_mgr_dev_hw_state *st,
                         bf_map_t *ecmp_grps,
                         mc_l1_node_t **node,
                         uint32_t l1_addr) {
  /* Make sure we have not seen this node before. */
  void *dont_care = NULL;
  bf_map_sts_t s = bf_map_get(&st->rdm_to_node, l1_addr, &dont_care);
  if (s != BF_MAP_NO_KEY) {
    /* We have already sync this node!  This indicates either a loop or two
     * multicast trees converging both of which are not right. */
    return false;
  }
  /* Read the RDM line and decode the node type. */
  int line = rdm_addr_to_line(dev, l1_addr);
  mc_mgr_get_rdm_reg(sid,
                     dev,
                     line,
                     &st->rdm_map->rdm[line].data[1],
                     &st->rdm_map->rdm[line].data[0]);
  mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[line]);
  mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, l1_addr);

  /* ECMP nodes get special handling. */
  if (mc_mgr_rdm_node_type_ecmp == t || mc_mgr_rdm_node_type_ecmp_xid == t) {
    bool r =
        sync_l1_ecmp_node(sid, dev, pipe, mgid, st, ecmp_grps, node, l1_addr);
    return r;
  }

  /* Handle normal L1 nodes, any other node type is unexpected. */
  if (!node_is_first_class_l1(t)) {
    set_node_invalid(dev, st->rdm_map, l1_addr);
    return false;
  }

  /* Create placeholders for the state that will be read from the node. */
  mc_l1_node_t *n = MC_MGR_MALLOC(sizeof(mc_l1_node_t));
  *node = n;
  MC_MGR_MEMSET(n, 0, sizeof(mc_l1_node_t));
  n->mgid = mgid;
  n->dev = dev;
  mc_mgr_rdm_addr_list_t *l2_addrs = NULL;
  bf_bitset_t ports[MC_MGR_NUM_PIPES];
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    bf_bs_init(&ports[p], BF_PIPE_PORT_COUNT, n->l2_chain.ports[p]);
  }
  bf_bitset_t lags;
  bf_bs_init(&lags, BF_LAG_COUNT, n->l2_chain.lags);

  /* Get the RID/XID from the L1 node. */
  int width = node_width(t);
  switch (t) {
    case mc_mgr_rdm_node_type_end:
      n->rid = st->rdm_map->rdm[line].u.end[l1_addr & 1].rid;
      LOG_DBG("Syncing Node: dev %d.%d mgid %#x addr %#x RidEnd %#x L2 %#x",
              dev,
              pipe,
              mgid,
              l1_addr,
              n->rid,
              st->rdm_map->rdm[line].u.end[l1_addr & 1].next_l2);
      break;
    case mc_mgr_rdm_node_type_rid:
      n->rid = st->rdm_map->rdm[line].u.rid.rid;
      LOG_DBG(
          "Syncing Node: dev %d.%d mgid %#x addr %#x Rid %#x L2 %#x nextL1 %#x",
          dev,
          pipe,
          mgid,
          l1_addr,
          n->rid,
          st->rdm_map->rdm[line].u.rid.next_l2,
          st->rdm_map->rdm[line].u.rid.next_l1);
      break;
    case mc_mgr_rdm_node_type_xid:
      n->rid = st->rdm_map->rdm[line].u.xid.rid;
      n->xid = st->rdm_map->rdm[line].u.xid.xid;
      n->xid_valid = true;
      LOG_DBG(
          "Syncing Node: dev %d.%d mgid %#x addr %#x Rid %#x Xid %#x L2 %#x "
          "nextL1 %#x",
          dev,
          pipe,
          mgid,
          l1_addr,
          n->rid,
          n->xid,
          st->rdm_map->rdm[line].u.xid.next_l2,
          st->rdm_map->rdm[line].u.xid.next_l1);
      break;
    default:
      set_node_invalid(dev, st->rdm_map, l1_addr);
      goto not_okay;
      break;
  }

  /* Check the L2 chain. */
  bool okay;
  okay = sync_l2_chain(sid, dev, pipe, st, &l2_addrs, ports, &lags, l1_addr);
  if (!okay) {
    LOG_DBG("Syncing Node: dev %d.%d mgid %#x addr %#x with unexpected L2",
            dev,
            pipe,
            mgid,
            l1_addr);
    set_node_invalid(dev, st->rdm_map, l1_addr);
    if (l2_addrs) {
      for (mc_mgr_rdm_addr_list_t *x = l2_addrs; x; x = x->next) {
        set_node_invalid(dev, st->rdm_map, x->addr);
      }
    }
    goto not_okay;
  }
  if (!l2_addrs) {
    /* Shouldn't have empty nodes. */
    LOG_DBG("Syncing Node: dev %d.%d mgid %#x addr %#x with no mbrs",
            dev,
            pipe,
            mgid,
            l1_addr);
    set_node_invalid(dev, st->rdm_map, l1_addr);
    goto not_okay;
  }

  /* Reserve L1 address. */
  okay = mc_mgr_mark_addr_used(st->rdm_map, dev, l1_addr, pipe, width, false);
  if (!okay) {
    LOG_DBG("Syncing Node: dev %d.%d mgid %#x addr %#x RDM conflict",
            dev,
            pipe,
            mgid,
            l1_addr);
    goto not_okay;
  }

  /* Reserve L2 address. */
  mc_mgr_rdm_addr_list_t *x;
  for (x = l2_addrs; x; x = x->next) {
    /* Get the size of each L2 node block so it can be reserved. */
    int l2_size = 0, l2_len = 0;
    get_l2_block_size(sid, dev, st->rdm_map, x->addr, false, &l2_size, &l2_len);
    okay =
        mc_mgr_mark_addr_used(st->rdm_map, dev, x->addr, pipe, l2_size, true);
    if (!okay) {
      /* Release all the L2 node addrs already reserved. */
      mc_mgr_rdm_addr_list_t *y;
      for (y = l2_addrs; y != x; y = y->next) {
        mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, y->addr, pipe);
      }
      /* Release the L1 node addr. */
      mc_mgr_rdm_mark_addr_free(st->rdm_map, dev, l1_addr, pipe);

      LOG_DBG(
          "Syncing Node: dev %d.%d mgid %#x addr %#x Mbr RDM conflict %#x sz "
          "%d ln %d",
          dev,
          pipe,
          mgid,
          l1_addr,
          x->addr,
          l2_size,
          l2_len);
      goto not_okay;
    }
  }

  n->hw_nodes[pipe].sw_node = n;
  n->hw_nodes[pipe].rdm_addr = l1_addr;
  bf_map_add(&st->rdm_to_node, l1_addr, n);
  return true;

not_okay:
  MC_MGR_FREE(n);
  *node = NULL;
  return false;
}
bool mc_mgr_rdm_sync_is_tail_addr(struct mc_mgr_dev_hw_state *st,
                                  int pipe,
                                  uint32_t addr) {
  uint32_t base_addr = st->tail_info->tail_base[pipe];
  uint32_t l1_cnt = st->tail_info->num_tails;
  return addr >= base_addr && addr < base_addr + l1_cnt;
}

bool sync_l1_chain(int sid,
                   bf_dev_id_t dev,
                   int pipe,
                   int mgid,
                   struct mc_mgr_dev_hw_state *st,
                   bf_map_t *ecmp_grps,
                   uint32_t l1_addr) {
  bool ret = false;
  uint32_t last_l1 = 0;
  LOG_DBG("Syncing Node Chain: dev %d.%d mgid %#x start %#x",
          dev,
          pipe,
          mgid,
          l1_addr);

  while (is_rdm_addr_valid(dev, l1_addr)) {
    mc_l1_node_t *n = NULL;
    bool l1_okay =
        sync_l1_node(sid, dev, pipe, mgid, st, ecmp_grps, &n, l1_addr);
    if (!l1_okay) {
      if (last_l1) {
        LOG_DBG("Trimming Node Chain: dev %d.%d mgid %#x start %#x",
                dev,
                pipe,
                mgid,
                l1_addr);
        mc_mgr_rdm_set_next_l1(st->rdm_map, last_l1, 0);
        bf_map_add(&st->dirty_rdm, last_l1, NULL);
      } else {
        LOG_DBG("Shortening Node Chain: dev %d.%d mgid %#x start %#x",
                dev,
                pipe,
                mgid,
                l1_addr);
      }
      break;
    } else {
      /* At least one good L1 node, return true to indicate that the address
       * points to at least a partially good L1 chain.  Any later bad nodes
       * will be cut off the chain. */
      ret = true;
    }
    /* Follow the next L1 pointer. */
    last_l1 = l1_addr;
    l1_addr = mc_mgr_rdm_next_l1(st->rdm_map, l1_addr);
    if (mc_mgr_rdm_sync_is_tail_addr(st, pipe, l1_addr)) break;
  }
  return ret;
}

bool mc_mgr_rdm_sync_tails(int sid,
                           bf_dev_id_t dev,
                           int pipe,
                           struct mc_mgr_dev_hw_state *st) {
  int base_addr = st->tail_info->tail_base[pipe];
  int l1_cnt = st->tail_info->num_tails;
  int l2_base_addr = st->tail_info->tail_l2_addr[pipe];
  int l2_cnt = st->tail_info->tail_l2_size;

  /* Read l1_cnt consecutive addresses starting at base_addr, they should all
   * be L1-End nodes. */
  for (int l1_addr = base_addr; l1_addr < base_addr + l1_cnt; ++l1_addr) {
    int line = rdm_addr_to_line(dev, l1_addr);
    mc_mgr_get_rdm_reg(sid,
                       dev,
                       line,
                       &st->rdm_map->rdm[line].data[1],
                       &st->rdm_map->rdm[line].data[0]);
    mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[line]);
    mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, l1_addr);
    if (t != mc_mgr_rdm_node_type_end) {
      LOG_ERROR(
          "%s: Dev %d sid %d pipe %d base 0x%x cnt %d, addr %x was %s not End",
          __func__,
          dev,
          sid,
          pipe,
          base_addr,
          l1_cnt,
          l1_addr,
          MC_MGR_RDM_NODE_TYPE_STR(t));
      goto failure;
    }
  }
  /* Read l2_cnt consecutive addresses starting at l2_base_addr, they should all
   * be LAG nodes. */
  for (int l2_addr = l2_base_addr; l2_addr < l2_base_addr + l2_cnt; ++l2_addr) {
    int line = rdm_addr_to_line(dev, l2_addr);
    mc_mgr_get_rdm_reg(sid,
                       dev,
                       line,
                       &st->rdm_map->rdm[line].data[1],
                       &st->rdm_map->rdm[line].data[0]);
    mc_mgr_rdm_decode_line(dev, &st->rdm_map->rdm[line]);
    mc_mgr_rdm_node_type_e t = mc_mgr_rdm_type(st->rdm_map, l2_addr);
    if (t != mc_mgr_rdm_node_type_lag) {
      LOG_ERROR(
          "%s: Dev %d sid %d pipe %d base 0x%x cnt %d, addr %x was %s not Lag",
          __func__,
          dev,
          sid,
          pipe,
          l2_base_addr,
          l2_cnt,
          l2_addr,
          MC_MGR_RDM_NODE_TYPE_STR(t));
      goto failure;
    }
  }
  mc_mgr_mark_addr_used(st->rdm_map, dev, base_addr, pipe, l1_cnt, false);
  mc_mgr_mark_addr_used(st->rdm_map, dev, l2_base_addr, pipe, l2_cnt, true);

  return true;

failure:
  return false;
}
