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


#include <stdbool.h>
#include <bf_types/bf_types.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include "mc_mgr.h"
#include "mc_mgr_bh.h"
#include "mc_mgr_reg.h"
#include "mc_mgr_mem.h"

/* Sanity checks
 * - Does PVT match non-zero MITs
 * - Does MIT match L1 database (if MIT=0, L1-DB doesn't have nodes for the mgid
 * and vise versa)
 * - Are all L1 nodes present in the trees (for each L1, walk rdm starting at
 * MIT)
 */

struct mc_mgr_ctx *mc_mgr_get_ctx() {
  return mc_mgr_ctx();
}
mc_mgr_rdm_t *mc_mgr_get_rdm_map(bf_dev_id_t dev) {
  return mc_mgr_ctx_rdm_map(dev);
}
bool mc_mgr_is_dma_pending(bf_dev_id_t dev) {
  int sid;
  for (sid = 0; sid < MC_MGR_NUM_SESSIONS; ++sid)
    if (mc_mgr_ctx_dev(dev)->write_list[sid][0].bufList) return true;
  return false;
}

void mc_mgr_tree_size(
    bf_dev_id_t dev, int pipe, int mgid, int *l1_cnt, int *l2_cnt) {
  *l1_cnt = 0;
  *l2_cnt = 0;

  // printf("TreeSize(%d, %d, %d)\n", dev, pipe, mgid);
  /* Walk all L1 nodes and find the ones associated with the MGID. */
  unsigned long hdl;
  mc_l1_node_t *n = NULL;
  bf_map_sts_t s;
  s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &hdl, (void **)&n);
  while (BF_MAP_OK == s) {
    MC_MGR_ASSERT(n);
    // printf("  Node %#lx with mgid %d [%p %p %p %p]\n", hdl, n->mgid,
    // n->hw_nodes[0], n->hw_nodes[1], n->hw_nodes[2], n->hw_nodes[3]);
    if (n->mgid == mgid) {
      if (n->hw_nodes[pipe].rdm_addr) {
        ++(*l1_cnt);
        // printf("    Found L1 RDM node %p at %#x\n", n->hw_nodes[pipe],
        // n->hw_nodes[pipe]->rdm_addr);
      }
    }
    s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &hdl, (void **)&n);
  }

  /* Walk the RDM and ensure the same number of L1 nodes. */
  int l1_cnt_rdm = 0;
  mc_l1_hw_node_t *first_node = mc_mgr_ctx_tree(dev, pipe, mgid);
  uint32_t l1_addr = first_node ? first_node->rdm_addr : 0;
  while (l1_addr) {
    // printf("Walked L1 RDM node at %#x\n", l1_addr);
    ++l1_cnt_rdm;
    l1_addr = mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), l1_addr);
    /* If we walked to a L1 End node then we hit the tail, don't count it. */
    mc_mgr_rdm_node_type_e t =
        mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), l1_addr);
    if (t == mc_mgr_rdm_node_type_end) break;
  }

  if (l1_cnt_rdm != *l1_cnt) {
    LOG_ERROR("RDM Walk Count: %d, DB Walk Count: %d", l1_cnt_rdm, *l1_cnt);
    MC_MGR_ASSERT(l1_cnt_rdm == *l1_cnt);
  }

  /* For each L1 node, count the L2s under it. */
  l1_addr = first_node ? first_node->rdm_addr : 0;
  while (l1_addr) {
    uint32_t l2_addr =
        mc_mgr_rdm_next_l2(mc_mgr_ctx_rdm_map(dev), l1_addr, false);
    while (l2_addr) {
      ++(*l2_cnt);
      l2_addr = mc_mgr_rdm_next_l2(mc_mgr_ctx_rdm_map(dev), l2_addr, false);
    }
    l1_addr = mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), l1_addr);
    /* If we walked to a L1 End node then we hit the tail, don't count it. */
    mc_mgr_rdm_node_type_e t =
        mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), l1_addr);
    if (t == mc_mgr_rdm_node_type_end) break;
  }
}

bf_mc_node_hdl_t mc_mgr_tree_first_handle(bf_dev_id_t dev, int pipe, int mgid) {
  bf_mc_node_hdl_t ret = -1;
  mc_l1_hw_node_t *n = mc_mgr_ctx_tree(dev, pipe, mgid);
  if (n) {
    ret = n->sw_node->handle;
  }
  return ret;
}

int mc_mgr_rdm_alloc_cnt(bf_dev_id_t dev, int size) {
  int blk;
  int count = 0;
  for (blk = 0; blk < TOF_MC_MGR_RDM_BLK_COUNT; ++blk) {
    count += power2_allocator_alloc_count_by_size(
        mc_mgr_ctx_rdm_map(dev)->blocks[blk].p2a, size);
  }

  /* Index zero of block zero is reserved, so we should always have at least
   * one allocation because of it. */
  if (size == 1) {
    MC_MGR_ASSERT(1 <= count);
    --count;
  }
  return count;
}

void mc_mgr_rdm_alloc_log(bf_dev_id_t dev) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  for (int i = 0; i < TOF_MC_MGR_RDM_BLK_COUNT; ++i) {
    for (int x = power2_allocator_first_alloc(rdm_map->blocks[i].p2a); x != -1;
         x = power2_allocator_next_alloc(rdm_map->blocks[i].p2a, x)) {
      LOG_TRACE("Dev %d blk %3d Idx %d sz %d",
                dev,
                i,
                x,
                power2_allocator_get_index_size(rdm_map->blocks[i].p2a, x));
    }
  }
}

uint32_t mc_mgr_l1_hdl_to_rdm_addr(bf_dev_id_t dev,
                                   int pipe,
                                   bf_mc_node_hdl_t node_hdl) {
  MC_MGR_ASSERT(0 <= pipe && BF_SUBDEV_PIPE_COUNT > pipe);
  mc_l1_node_t *node = mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);
  if (!node) return 0;
  return node->hw_nodes[pipe].rdm_addr;
}

bool mc_mgr_get_port72_last(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_line_t line = {{0}};
  line.data[0] = mc_mgr_ctx_rdm_map(dev)->rdm[addr / 2].data[0];
  line.data[1] = mc_mgr_ctx_rdm_map(dev)->rdm[addr / 2].data[1];
  mc_mgr_rdm_decode_line(dev, &line);
  if (mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr) !=
      mc_mgr_rdm_node_type_port72) {
    LOG_ERROR("Dev %d, Addr %#x, node isn't port72, %s",
              dev,
              addr,
              MC_MGR_RDM_NODE_TYPE_STR(
                  mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr)));
    MC_MGR_ASSERT(mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr) ==
                  mc_mgr_rdm_node_type_port72);
  }
  return line.u.port72.last;
}

int mc_mgr_get_port72_pipe(bf_dev_id_t dev, uint32_t addr) {
  mc_mgr_rdm_line_t line = {{0}};
  line.data[0] = mc_mgr_ctx_rdm_map(dev)->rdm[addr / 2].data[0];
  line.data[1] = mc_mgr_ctx_rdm_map(dev)->rdm[addr / 2].data[1];
  mc_mgr_rdm_decode_line(dev, &line);
  if (mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr) !=
      mc_mgr_rdm_node_type_port72) {
    LOG_ERROR("Dev %d, Addr %#x, node isn't port72, %s",
              dev,
              addr,
              MC_MGR_RDM_NODE_TYPE_STR(
                  mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr)));
    MC_MGR_ASSERT(mc_mgr_rdm_type(mc_mgr_ctx_rdm_map(dev), addr) ==
                  mc_mgr_rdm_node_type_port72);
  }
  return line.u.port72.pipe;
}

int mc_mgr_test_misc(bf_dev_id_t dev) {
  bf_status_t sts = BF_SUCCESS;
  int sid = 0;

  uint32_t blk_id;
  sts = mc_mgr_get_blk_id_grp_hw(dev, 0, &blk_id);
  if (BF_SUCCESS != sts) return -1;

  bf_bitset_t lit_hw[MC_MGR_NUM_PIPES];
  uint64_t lit_hw_[MC_MGR_NUM_PIPES][BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
  int p;
  for (p = 0; p < BF_SUBDEV_PIPE_COUNT; ++p) {
    bf_bs_init(&lit_hw[p], BF_PIPE_PORT_COUNT, lit_hw_[p]);
  }
  int lag, v;
  for (v = 0; v < 2; ++v) {
    for (lag = 0; lag < 256; ++lag) {
      sts = mc_mgr_get_lag_hw(dev, v, lag, lit_hw);
      if (BF_SUCCESS != sts) return -1;

      int l, r;
      sts = mc_mgr_get_lag_np_hw(dev, v, lag, &l, &r);
      if (BF_SUCCESS != sts) return -1;
      sts = mc_mgr_set_lit_np_wrl(sid, dev, v, lag);
      if (BF_SUCCESS != sts) return -1;
    }
  }

  int mgid;
  for (mgid = 0; mgid < 64 * 1024; ++mgid) {
    uint32_t m0 = mc_mgr_get_mit(dev, 0, mgid);
    uint32_t m1 = mc_mgr_get_mit(dev, 1, mgid);
    uint32_t m2 = mc_mgr_get_mit(dev, 2, mgid);
    uint32_t m3 = mc_mgr_get_mit(dev, 3, mgid);
    mc_mgr_get_mit_row_reg(dev, 0, mgid >> 3, &m0, &m1, &m2, &m3);
    mc_mgr_get_mit_row_reg(dev, 1, mgid >> 3, &m0, &m1, &m2, &m3);
    mc_mgr_get_mit_row_reg(dev, 2, mgid >> 3, &m0, &m1, &m2, &m3);
    mc_mgr_get_mit_row_reg(dev, 3, mgid >> 3, &m0, &m1, &m2, &m3);
  }

  for (v = 0; v < 2; ++v) {
    for (p = 0; p < 288; ++p) {
      sts = mc_mgr_get_port_mask_reg(dev, v, p, NULL, NULL);
      if (BF_SUCCESS != sts) return -1;
      sts = mc_mgr_set_port_mask_reg(dev, v, p);
      if (BF_SUCCESS != sts) return -1;
    }
  }

  mc_mgr_set_comm_ctrl_wrl(sid, dev);
  if (BF_SUCCESS != sts) return -1;

  for (v = 0; v < 2; ++v) {
    for (p = 0; p < 288; ++p) {
      int b;
      sts = mc_mgr_get_bkup_port_reg(dev, v, p, &b);
      if (BF_SUCCESS != sts) return -1;
    }
  }

  for (p = 0; p < 288; ++p) {
    bool down;
    sts = mc_mgr_get_port_down_reg(dev, p, &down);
    if (BF_SUCCESS != sts) return -1;
    sts = mc_mgr_clr_port_down_reg(dev, p);
    if (BF_SUCCESS != sts) return -1;
  }

  for (p = 0; p < 4; ++p) {
    sts = mc_mgr_set_pre_ctrl_wrl(sid, dev);
    if (BF_SUCCESS != sts) return -1;

    int count = 0;
    sts = mc_mgr_get_max_l1_reg(dev, p, &count);
    if (BF_SUCCESS != sts) return -1;
    sts = mc_mgr_set_max_l1_wrl(sid, dev);
    if (BF_SUCCESS != sts) return -1;

    sts = mc_mgr_get_max_l2_reg(dev, p, &count);
    if (BF_SUCCESS != sts) return -1;
    sts = mc_mgr_set_max_l2_wrl(sid, dev);
    if (BF_SUCCESS != sts) return -1;
  }
  return 0;
}
