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


#include <bf_types/bf_types.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_bh.h"

static bf_status_t sync_rdm_contents(int sid,
                                     bf_dev_id_t dev,
                                     struct mc_mgr_dev_hw_state *st);

bf_status_t mc_mgr_read_hw_state(int sid,
                                 bf_dev_id_t dev,
                                 struct mc_mgr_dev_hw_state *st) {
  int i, v, p, P;
  bf_status_t psts[MC_MGR_NUM_PIPES];
  bf_dev_family_t dev_family = mc_mgr_ctx_dev_family(dev);

  st->dev = dev;
  mc_mgr_rdm_map_init(
      &st->rdm_map, dev, dev_family, mc_mgr_ctx_num_max_pipes(dev));
  bf_map_init(&st->ecmp_grps);

  /* Read L1 Slice and CPU port register. */
  bool c2c_en[MC_MGR_NUM_PIPES] = {0};
  int c2c_port[MC_MGR_NUM_PIPES] = {0};
  uint8_t l1_slice[MC_MGR_NUM_PIPES] = {0};
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    mc_mgr_get_pre_ctrl_reg(dev, p, c2c_en + p, c2c_port + p, l1_slice + p);
  }
  /* Combine register values from pipes to get state for Max L1 Timeslice.  The
   * value read across all pipes must be the same. */
  st->max_l1_slice_valid = true;
  st->max_l1_slice = l1_slice[0];
  for (p = 1; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    st->max_l1_slice_valid =
        st->max_l1_slice_valid && (l1_slice[0] == l1_slice[p]);
  }
  /* Combine register values from pipes to get state for CPU Port.  At most
   * only a single pipe can have the port enabled. */
  st->cpu_port_valid = true;
  st->cpu_port_en = false;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (c2c_en[p] && !st->cpu_port_en) {
      st->cpu_port_en = true;
      st->cpu_port = mc_make_dev_port(dev, p, c2c_port[p]);
    } else if (c2c_en[p] && st->cpu_port_en) {
      st->cpu_port_valid = false;
    }
  }

  /* Read Max L1/L2 nodes register. */
  int max_l1_nodes[MC_MGR_NUM_PIPES] = {0};
  int max_l2_nodes[MC_MGR_NUM_PIPES] = {0};
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    mc_mgr_get_max_l1_reg(dev, p, max_l1_nodes + p);
    mc_mgr_get_max_l2_reg(dev, p, max_l2_nodes + p);
  }
  /* Combine register values from pipes to get state for max node configuration.
   * The value must the same across all pipes. */
  st->max_l1_nodes_valid = true;
  st->max_l2_nodes_valid = true;
  st->max_l1_nodes = max_l1_nodes[0];
  st->max_l2_nodes = max_l2_nodes[0];
  for (p = 1; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    st->max_l1_nodes_valid =
        st->max_l1_nodes_valid && (max_l1_nodes[0] == max_l1_nodes[p]);
    st->max_l2_nodes_valid =
        st->max_l2_nodes_valid && (max_l2_nodes[0] == max_l2_nodes[p]);
  }

  /* Read Fast Failover state (hardware port liveness enable) and Backup Port
   * Table enable. */
  uint8_t pipe_sel;
  mc_mgr_get_comm_ctrl_reg(dev, &pipe_sel, &st->ff_en, &st->bkup_en);

  /* Read Global RID. */
  mc_mgr_get_global_rid_reg(dev, &st->g_rid);

  /* Read RDM Block Map. */
  int blk_idx = 0;
  int mc_mgr_rdm_blk_count = 0;

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      mc_mgr_rdm_blk_count = TOF_MC_MGR_RDM_BLK_COUNT;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      mc_mgr_rdm_blk_count = TOF2_MC_MGR_RDM_BLK_COUNT;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      mc_mgr_rdm_blk_count = TOF3_MC_MGR_RDM_BLK_COUNT;
      break;
    default:
      MC_MGR_DBGCHK(0);
  }
  for (blk_idx = 0; blk_idx < (mc_mgr_rdm_blk_count / 16); ++blk_idx) {
    mc_mgr_get_rdm_blk_id_grp_reg(dev, blk_idx, &st->rdm_map->blk_ids[blk_idx]);
  }

  /* Read the software port mask. */
  st->port_fwd_state_valid = true;
  for (i = 0;
       i < (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(mc_mgr_ctx_num_max_ports(dev));
       ++i) {
    uint32_t pfs;
    mc_mgr_get_port_mask_reg(dev, 0, i * 32, &st->port_fwd_state[i], NULL);
    mc_mgr_get_port_mask_reg(dev, 1, i * 32, &pfs, NULL);
    st->port_fwd_state_valid =
        st->port_fwd_state_valid && pfs == st->port_fwd_state[i];
  }

  /* Read backup port table. */
  st->bkup_ports_valid = true;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(dev); ++i) {
    int v0, v1;
    mc_mgr_get_bkup_port_reg(dev, 0, i, &v0);
    mc_mgr_get_bkup_port_reg(dev, 1, i, &v1);
    st->bkup_ports_valid = st->bkup_ports_valid && (v0 == v1);
    st->bkup_ports[i] = v0;
  }

  /* Read LIT L and R counts. */
  st->lit_cnt_valid = true;
  for (i = 0; i < BF_LAG_COUNT; ++i) {
    for (v = 0; v < 2; ++v) {
      int l, r;
      mc_mgr_get_lit_np_reg(dev, v, i, &l, &r);
      st->lit_cnt_l[v][i] = l;
      st->lit_cnt_r[v][i] = r;
    }
    st->lit_cnt_valid = st->lit_cnt_valid &&
                        st->lit_cnt_l[0][i] == st->lit_cnt_l[1][i] &&
                        st->lit_cnt_r[0][i] == st->lit_cnt_r[1][i];
  }
  /* Read LIT */
  st->lit_valid = true;
  for (i = 0; i < BF_LAG_COUNT; ++i) {
    bf_bitset_t lit_hw[MC_MGR_NUM_PIPES];
    uint64_t lit_hw_[MC_MGR_NUM_PIPES]
                    [BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)] = {0};
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      bf_bs_init(&st->lit[i][p], BF_PIPE_PORT_COUNT, st->lit_[i][p]);
      bf_bs_init(&lit_hw[p], BF_PIPE_PORT_COUNT, lit_hw_[p]);
    }
    mc_mgr_get_lag_hw(dev, 0, i, st->lit[i]);
    mc_mgr_get_lag_hw(dev, 1, i, lit_hw);

    p = (int)mc_mgr_ctx_num_max_pipes(dev);
    do {
      p--;
      st->lit_valid = st->lit_valid && bf_bs_equal(&st->lit[i][p], &lit_hw[p]);
    } while (st->lit_valid && p > 0);
  }
  /* Read PMT */
  st->pmt_valid = true;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(dev); ++i) {
    bf_bs_init(&st->pmt[i], mc_mgr_ctx_num_max_ports(dev), st->pmt_[i]);
    int seg;
    BF_BITSET(v0, BF_PIPE_PORT_COUNT);
    BF_BITSET(v1, BF_PIPE_PORT_COUNT);
    for (seg = 0; seg < 4; ++seg) {
      mc_mgr_get_pmt_seg_reg(dev, 0, i, seg, &v0);
      mc_mgr_get_pmt_seg_reg(dev, 1, i, seg, &v1);
      st->pmt_valid = st->pmt_valid && bf_bs_equal(&v0, &v1);
      bf_bs_copy_range(
          &st->pmt[i], seg * BF_PIPE_PORT_COUNT, &v0, 0, BF_PIPE_PORT_COUNT);
    }
  }

  /* Read MIT */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    for (i = 0; i < BF_MGID_COUNT; i += 4) {
      mc_mgr_get_mit_row_reg(dev,
                             p,
                             i >> 2,
                             &st->mit[p][i + 0],
                             &st->mit[p][i + 1],
                             &st->mit[p][i + 2],
                             &st->mit[p][i + 3]);
    }
  }

  /* Block read deparser PVT 0 and 1.
   * Note that BF_INVALID_ARG will be returned if the pipe doesn't exist on
   * this SKU. */
  st->pvt_valid = true;
  for (i = 0; i < (BF_MGID_COUNT / 8); ++i) {
    mc_mgr_pvt_entry_t v0[MC_MGR_NUM_PIPES];
    mc_mgr_pvt_entry_t v1[MC_MGR_NUM_PIPES];
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      psts[p] = mc_mgr_get_pvt_reg(dev, p, 0, i, v0 + p);
      mc_mgr_get_pvt_reg(dev, p, 1, i, v1 + p);
    }
    /* Check both tables are the same. */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      st->pvt_valid =
          st->pvt_valid && (psts[p] == BF_INVALID_ARG || (v0[p].d == v1[p].d));
    }
    /* Check all pipes are the same. */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (BF_INVALID_ARG == psts[p]) continue;
      st->pvt[i].d = v0[p].d;
      for (P = p; P < (int)mc_mgr_ctx_num_max_pipes(dev); ++P) {
        if (BF_INVALID_ARG == psts[P]) continue;
        st->pvt_valid = st->pvt_valid && (v0[p].d == v0[P].d);
      }
    }
  }
  /* Read deparser packet version. */
  int tbl_ver[MC_MGR_NUM_PIPES] = {0};
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    psts[p] = mc_mgr_get_tbl_ver_reg(dev, p, &tbl_ver[p]);
    if (BF_INVALID_ARG == psts[p]) {
      tbl_ver[p] = 0;
    }
  }
  /* Check all pipes are the same. */
  st->tbl_ver_valid = true;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (BF_INVALID_ARG == psts[p]) continue;
    st->tbl_ver = tbl_ver[p];
    for (P = p; P < (int)mc_mgr_ctx_num_max_pipes(dev); ++P) {
      if (BF_INVALID_ARG == psts[P]) continue;
      st->tbl_ver_valid = st->tbl_ver_valid && (tbl_ver[p] == tbl_ver[P]);
    }
  }

  /* Read C2C deparser pipe mask. */
  uint32_t c2c_msk[MC_MGR_NUM_PIPES] = {0};
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    psts[p] = mc_mgr_c2c_pipe_msk_get_hw(dev, p, &c2c_msk[p]);
    if (BF_INVALID_ARG == psts[p]) {
      c2c_msk[p] = 0;
    }
  }
  /* Check all pipes are the same. */
  st->c2c_pipe_mask_valid = true;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (BF_INVALID_ARG == psts[p]) continue;
    st->c2c_pipe_mask = c2c_msk[p];
    for (P = p; P < (int)mc_mgr_ctx_num_max_pipes(dev); ++P) {
      if (BF_INVALID_ARG == psts[P]) continue;
      st->c2c_pipe_mask_valid =
          st->c2c_pipe_mask_valid && (c2c_msk[p] == c2c_msk[P]);
    }
  }

  sync_rdm_contents(sid, dev, st);
  return BF_SUCCESS;
}

/* Compare two ECMP groups read from two different pipes/PREs and decide if
 * they are two instances of the same group or not. */
static bool are_grps_same(mc_ecmp_grp_t *a, mc_ecmp_grp_t *b) {
  /* Groups are expected to be same if they have same ecmp_id, same
   * vector-map, same L1-End block size, each mbr having same LAG ID
   * same */
  if (a->ecmp_id != b->ecmp_id) return false;
  if (a->valid_map != b->valid_map) return false;
  if (a->allocated_sz != b->allocated_sz) return false;
  for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
    if (0 == (a->valid_map & (1u << i))) continue;
    if (a->mbrs[i]->rid != b->mbrs[i]->rid) return false;
    /* Port membership is only programmed in pipes where the ports exist so it
     * cannot be compared across pipes.
     *
     * Similarly LAG membership cannot be comapred as LAGs are programmed in
     * pipes where LAGs have their associated ports, so it cannot be compared
     * across pipes. */
  }
  return true;
}

static bf_status_t combine_non_ecmp_nodes(struct mc_mgr_dev_hw_state *st,
                                          bf_map_t *bad_nodes,
                                          int mgid) {
  int p, P;
  bf_map_sts_t s;
  /*
   * Combine nodes read from the individual pipes.  Just a double for loop over
   * the pipes to check if a node in an early pipe can be combined with nodes in
   * later pipes.  Start at the address given by the MIT, lookup the node for
   * that address, then follow its L1-next pointer to find the next node in the
   * chain. */

  /* A map to keep track of each node we've processed so we don't work on it
   * multiple times.  For example, if a node is in all PREs then the first pass
   * through will merge them into a single node.  The second pass will see the
   * node again but it doesn't need to check if it can be combined again since
   * the work was already done. */
  bf_map_t nodes_processed;
  bf_map_init(&nodes_processed);

  /* Outside loop over PREs.  Take a node in this PREs chain and match it
   * against nodes in later PREs chains to try and combine. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++p) {
    mc_l1_node_t *a = node_from_addr(&st->rdm_to_node, st->mit[p][mgid]);
    while (a) {
      if (a->mgid != mgid) {
        MC_MGR_DBGCHK(a->mgid == mgid);
        return BF_UNEXPECTED;
      }
      mc_l1_node_t *a_next = next_node_from_addr(
          &st->rdm_to_node, st->rdm_map, a->hw_nodes[p].rdm_addr);
      mc_l1_node_t *node_set[MC_MGR_NUM_PIPES] = {0};
      node_set[p] = a;
      if (!a->hw_nodes[p].rdm_addr) {
        MC_MGR_DBGCHK(a->hw_nodes[p].rdm_addr);
        return BF_UNEXPECTED;
      }
      void *dont_care = NULL;
      s = bf_map_get(&nodes_processed, (unsigned long)a, &dont_care);
      if (BF_MAP_OK == s) {
        /* We've already seen this node, go ahead and skip it.  This can happen,
         * for example, if a node is in multiple pipes.  Initially it is nodes
         * A, B, and C in pipes 0, 1 and 2.  After the first pass through of the
         * outer loop the nodes combine so that it is nodes A, A and A in pipes
         * 0, 1 and 2 so we can skip any further processing since it is already
         * combined. */
        a = a_next;
        continue;
      } else if (a->ecmp_grp) {
        /* This is an ECMP pointer node.  Deal with it later when combining the
         * ECMP groups. */
        a = a_next;
        continue;
      }
      /* Inner loop over PREs to look for a matching node.  Walk the L1 chain
       * for this mgid in each later PRE to find a match. */
      for (P = p + 1; P < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++P) {
        mc_l1_node_t *b = node_from_addr(&st->rdm_to_node, st->mit[P][mgid]);
        while (b) {
          if (b->mgid != mgid) {
            MC_MGR_DBGCHK(b->mgid == mgid);
            return BF_UNEXPECTED;
          }
          if (!b->hw_nodes[P].rdm_addr) {
            MC_MGR_DBGCHK(b->hw_nodes[P].rdm_addr);
            return BF_UNEXPECTED;
          }
          mc_l1_node_t *b_next = next_node_from_addr(
              &st->rdm_to_node, st->rdm_map, b->hw_nodes[P].rdm_addr);
          s = bf_map_get(&nodes_processed, (unsigned long)b, &dont_care);
          if (BF_MAP_OK == s) {
            /* We've already seen this node, go ahead and skip it.  Since we
             * have already processed it we know it cannot combine with "a".
             */
          } else if (b->ecmp_grp) {
            /* This is an ECMP pointer node.  Deal with it later when combining
             * the ECMP groups. */
          } else if (a->rid == b->rid && a->xid == b->xid &&
                     a->xid_valid == b->xid_valid) {
            bf_bitset_t a_lags, b_lags;
            bf_bs_init(&a_lags, BF_LAG_COUNT, a->l2_chain.lags);
            bf_bs_init(&b_lags, BF_LAG_COUNT, b->l2_chain.lags);
            if (bf_bs_equal(&a_lags, &b_lags)) {
              /* A and B are instances of the same L1 node in two different
               * pipes so add the node to the set to combine and move onto the
               * next PRE. */
              node_set[P] = b;
              break;
            }
          }
          b = b_next;
        }
      }
      /* Before merging nodes, check if LAG-IDs of node 'a' has remote pipe-bits
       * set in lag pm If set then check if there are any nodes in node_set of
       * remote pipe. */
      bool node_is_bad = false;
      for (P = 0; !node_is_bad && P < (int)mc_mgr_ctx_num_max_pipes(st->dev);
           ++P) {
        if (!node_set[P]) continue;

        int lag_id = -1;
        bf_bitset_t a_lags;
        bf_bs_init(&a_lags, BF_LAG_COUNT, node_set[P]->l2_chain.lags);

        /* Iterating through LAGIDs of the node 'a'.*/
        for (lag_id = bf_bs_first_set(&a_lags, lag_id);
             !node_is_bad && lag_id != -1;
             lag_id = bf_bs_first_set(&a_lags, lag_id)) {
          int pm = mc_mgr_ctx_lag2pipe_mask(node_set[P]->dev, lag_id);
          for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node_set[P]->dev);
               i++) {
            if (i == P) continue;

            /* Check if the node is present in this pipe or not. */
            mc_l1_node_t *b = node_set[i];
            if (!b) continue;

            bf_bitset_t b_lags;
            bf_bs_init(&b_lags, BF_LAG_COUNT, b->l2_chain.lags);

            /* Add to bad nodes map, if node 'a' lag has pm !=0 in the current
             * pipe 'i' but node 'b' which is present in the node-set in pipe
             * 'i'  has no lag node with the same lag id or vice-versa.*/
            if (!(pm & (1 << i))) continue;
            if ((pm & (1 << i) && !bf_bs_get(&b_lags, lag_id)) ||
                (!(pm & (1 << i)) && bf_bs_get(&b_lags, lag_id))) {
              LOG_DBG(
                  "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
                  "XID %#x XID-Val %d (Lag-Mismatch)",
                  (void *)a,
                  a->dev,
                  a->mgid,
                  a->hw_nodes[0].rdm_addr,
                  a->hw_nodes[1].rdm_addr,
                  a->hw_nodes[2].rdm_addr,
                  a->hw_nodes[3].rdm_addr,
                  a->rid,
                  a->xid,
                  a->xid_valid);

              bf_map_add(bad_nodes, (unsigned long)a, NULL);
              node_is_bad = true;
              break;
            }
          }
        }
      }

      /* As we have already added 'a' to the bad_nodes, it not needed to match
       * it with any other node from node_set. Continue with 'a_next' node. */
      if (node_is_bad) {
        a = a_next;
        continue;
      }

      /* The lag map of all lag ids in all pipes have been verified. Now,
       * Before merging the nodes make sure that the backup ports make sense.
       * When syncing the nodes we verified that ports in remote pipes agreed
       * with the backup port table, but now that we have the node in all pipes
       * we can verify that it isn't missing any backup ports. */
      for (P = 0; P < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++P) {
        if (!node_set[P]) continue;
        /* Node is present in pipe P.  If there are any ports present for pipe P
         * that have backup ports in remote pipes then there must be a node in
         * that pipe and that node must have the port in pipe P as a member. */
        int local_port = -1;
        bf_bitset_t ports;
        bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node_set[P]->l2_chain.ports[P]);
        for (local_port = bf_bs_first_set(&ports, local_port); local_port != -1;
             local_port = bf_bs_first_set(&ports, local_port)) {
/* GCC 4.9.2 gives a false positive warning for "array subscript is above array
 * bounds".  Likely related to this bug:
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56273
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
          bf_dev_port_t dev_port = mc_make_dev_port(a->dev, P, local_port);
          uint16_t port_index = mc_dev_port_to_bit_idx(a->dev, dev_port);
          uint16_t bkup_port_index = st->bkup_ports[port_index];
          int bkup_pipe = mc_bit_idx_to_pipe(a->dev, bkup_port_index);
#pragma GCC diagnostic pop
          if (!node_set[bkup_pipe]) {
            /* No node for this backup port!!!  Put the "a" node into the set of
             * bad nodes since that is the node which will survive the merging.
             */
            LOG_DBG(
                "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
                "XID %#x XID-Val %d (Bkup port node)",
                (void *)a,
                a->dev,
                a->mgid,
                a->hw_nodes[0].rdm_addr,
                a->hw_nodes[1].rdm_addr,
                a->hw_nodes[2].rdm_addr,
                a->hw_nodes[3].rdm_addr,
                a->rid,
                a->xid,
                a->xid_valid);
            bf_map_add(bad_nodes, (unsigned long)a, NULL);
            break;
          }
          if (!node_has_mbr(node_set[bkup_pipe], dev_port)) {
            /* Backup port config wasn't present in hardware!!!  Put the "a"
             * node into the set of bad nodes since that is the node which will
             * survive the merging. */
            LOG_DBG(
                "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
                "XID %#x XID-Val %d (Bkup port mbr)",
                (void *)a,
                a->dev,
                a->mgid,
                a->hw_nodes[0].rdm_addr,
                a->hw_nodes[1].rdm_addr,
                a->hw_nodes[2].rdm_addr,
                a->hw_nodes[3].rdm_addr,
                a->rid,
                a->xid,
                a->xid_valid);
            bf_map_add(bad_nodes, (unsigned long)a, NULL);
            break;
          }
        }
      }

      /* Combine the nodes in the set, move all state into the lowest array
       * index that is populated, index "p" where we have node "a" and delete
       * the other nodes. */
      for (P = p + 1; P < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++P) {
        mc_l1_node_t *b = node_set[P];
        if (!b) continue;
        /* For each "b" node, copy the RDM address and port membership in b's
         * pipe over to the "a" node. */
        bf_bitset_t a_ports, b_ports;
        bf_bs_init(&a_ports, BF_PIPE_PORT_COUNT, a->l2_chain.ports[P]);
        a->hw_nodes[P].rdm_addr = b->hw_nodes[P].rdm_addr;
        a->hw_nodes[P].sw_node = a;
        bf_bs_init(&b_ports, BF_PIPE_PORT_COUNT, b->l2_chain.ports[P]);
        bf_bs_copy(&a_ports, &b_ports);

        /* Start copy node "b" lags which are not present in node 'a' lags. */
        int b_lag_id = -1;
        bf_bitset_t a_lags, b_lags;
        bf_bs_init(&a_lags, BF_LAG_COUNT, a->l2_chain.lags);
        bf_bs_init(&b_lags, BF_LAG_COUNT, b->l2_chain.lags);

        /* Iterating through LAGIDs of the node 'b'. Merge 'b' LAGs with 'a'. */
        for (b_lag_id = bf_bs_first_set(&b_lags, b_lag_id); b_lag_id != -1;
             b_lag_id = bf_bs_first_set(&b_lags, b_lag_id)) {
          bf_bs_set(&a_lags, b_lag_id, 1);
        }

        /* Remove the "b" node from the address-to-node map (since it is being
         * deleted) and replace it with the "a" node. */
        s = bf_map_rmv(&st->rdm_to_node, b->hw_nodes[P].rdm_addr);
        if (s != BF_MAP_OK) {
          MC_MGR_DBGCHK(s == BF_MAP_OK);
          return BF_UNEXPECTED;
        }
        s = bf_map_add(&st->rdm_to_node, a->hw_nodes[P].rdm_addr, a);
        if (s != BF_MAP_OK) {
          MC_MGR_DBGCHK(s == BF_MAP_OK);
          return BF_UNEXPECTED;
        }
        LOG_DBG(
            "Merging nodes %p (PRE %d) %p (PRE %d): Dev %d MGID %#x RDM [%#x "
            "%#x %#x %#x] [%#x %#x %#x %#x] RID %#x XID %#x XID-Val %d",
            (void *)a,
            p,
            (void *)b,
            P,
            a->dev,
            a->mgid,
            a->hw_nodes[0].rdm_addr,
            a->hw_nodes[1].rdm_addr,
            a->hw_nodes[2].rdm_addr,
            a->hw_nodes[3].rdm_addr,
            b->hw_nodes[0].rdm_addr,
            b->hw_nodes[1].rdm_addr,
            b->hw_nodes[2].rdm_addr,
            b->hw_nodes[3].rdm_addr,
            a->rid,
            a->xid,
            a->xid_valid);
        MC_MGR_FREE(b);
      }

      /* Add the node to the list of processed nodes. */
      s = bf_map_add(&nodes_processed, (unsigned long)a, NULL);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to save combined node");
        return BF_NO_SYS_RESOURCES;
      }
      a = a_next;
    }
  }
  bf_map_destroy(&nodes_processed);
  return BF_SUCCESS;
}

static void combine_ecmp_groups_and_nodes(struct mc_mgr_dev_hw_state *st,
                                          bf_map_t ecmp_grps[MC_MGR_NUM_PIPES],
                                          bf_map_t *bad_groups,
                                          bf_map_t *bad_nodes) {
  int P, p, m;
  mc_ecmp_grp_t *grp = NULL;
  mc_l1_node_t *a, *b;
  unsigned long ecmp_grp_id = 0;
  bf_map_sts_t s;

  /* Combine the ECMP groups read out from the different PREs.  ECMP groups
   * is not mandatory to be present in each PRE with the exact same membership.
   * The groups are provided to us in an array of maps, one per PRE where they
   * were read from, where the key is the ECMP id that was encoded in the
   * ECMP vector node. */
  for (P = 0; P < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++P) {
    /* Walk through all groups in the first PRE matching it with groups in each
     * of the other PREs. */
    s = bf_map_get_first(&ecmp_grps[P], &ecmp_grp_id, (void **)&grp);
    while (BF_MAP_OK == s && grp) {
      bool group_is_bad = false;
      if (grp->ecmp_id != ecmp_grp_id) {
        MC_MGR_DBGCHK(grp->ecmp_id == ecmp_grp_id);
        return;
      }

      /* Extract the next group from pipe "P". */
      mc_ecmp_grp_t *grp_next;
      unsigned long ecmp_grp_nxt_id = ecmp_grp_id;
      bf_map_sts_t grp_sts =
          bf_map_get_next(&ecmp_grps[P], &ecmp_grp_nxt_id, (void **)&grp_next);

      mc_ecmp_grp_t *grp_set[MC_MGR_NUM_PIPES] = {0};
      grp_set[P] = grp;

      void *dont_care = NULL;
      s = bf_map_get(bad_groups, (unsigned long)grp, &dont_care);
      if (BF_MAP_OK == s) {
        /* We have already seen the group in this pipe, and its already
         * marked as bad group, skip it and move to the next group. */
        s = grp_sts;
        grp = grp_next;
        ecmp_grp_id = ecmp_grp_nxt_id;
        continue;
      }

      /* Check the remote pipe("p") groups if they can be merged with the
       * current group. If it can merged, then store in the group set. The
       * group set's group will be merged later. */
      for (p = P + 1; p < (int)mc_mgr_ctx_num_max_pipes(grp->dev); ++p) {
        mc_ecmp_grp_t *g = NULL;
        if (bf_map_get(&ecmp_grps[p], ecmp_grp_id, (void **)&g) != BF_MAP_OK)
          continue;

        if (!g) continue;

        /* The "g" ecmp_id should match with the "grp" ecmp_id.*/
        if (g->ecmp_id != ecmp_grp_id) {
          MC_MGR_DBGCHK(g->ecmp_id == ecmp_grp_id);
          return;
        }

        /* Check if both the groups are same, if not mark the group "grp" as
         * bad group and put in bad_grp map.*/
        if (g && are_grps_same(grp, g)) {
          grp_set[p] = g;
        } else {
          /* Since the groups are not the same across pipes even though they
           * have same ecmp_id, just forget about the groups and let the replay
           * give us the correct information. */
          group_is_bad = true;
          LOG_DBG(
              "ECMP group 0x%lx in phy-pipe %d doesn't match group in phy-pipe "
              "%d, "
              "marking for cleanup",
              ecmp_grp_id,
              p,
              P);
          LOG_DBG("Id 0x%x/0x%x valid-map 0x%x/0x%x alloc-sz %d/%d",
                  grp->ecmp_id,
                  g->ecmp_id,
                  grp->valid_map,
                  g->valid_map,
                  grp->allocated_sz,
                  g->allocated_sz);
          for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
            if (0 == (grp->valid_map & (1u << i))) continue;
            LOG_DBG("  Mbr %d RID 0x%x/0x%x",
                    i,
                    grp->mbrs[i]->rid,
                    g->mbrs[i]->rid);
          }
          for (mc_l1_node_t *r = grp->refs; r; r = r->ecmp_next) {
            LOG_DBG("  Ref to MGID 0x%x (node handle 0x%x %p)",
                    r->mgid,
                    r->handle,
                    (void *)r);
          }
          /* Add "g" with the same ecmp_id to the bad_Group map. */
          bf_map_add(bad_groups, ecmp_grp_id, NULL);
          break;
        }
      }

      /* Check if "grp" is bad which means that group in pipe "P" did
       * not match with group "g" from other pipes, even though they
       * have the same ecmp_id. */
      if (group_is_bad) {
        s = grp_sts;
        grp = grp_next;
        ecmp_grp_id = ecmp_grp_nxt_id;
        continue;
      }

      /* TODO: Do we need checks if the members/groups in the group_set
       * matches the lag memberships and the ports membership along with
       * back up ports like we do for non-ecmp nodes?  */

      /* Iterate through the other pipe and combine the groups from
       * group set. */
      for (p = P + 1; p < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++p) {
        mc_ecmp_grp_t *g = grp_set[p];
        if (!g) continue;
        for (m = 0; m < MC_ECMP_GROUP_MAX_MBR; ++m) {
          if (!grp->mbrs[m]) continue;
          a = grp->mbrs[m];
          b = g->mbrs[m];
          a->hw_nodes[p].sw_node = g->mbrs[m];
          a->hw_nodes[p].rdm_addr = g->base[p] + m;
          bf_bitset_t a_ports, b_ports;
          bf_bs_init(&a_ports, BF_PIPE_PORT_COUNT, a->l2_chain.ports[p]);
          bf_bs_init(&b_ports, BF_PIPE_PORT_COUNT, b->l2_chain.ports[p]);
          bf_bs_copy(&a_ports, &b_ports);
          /* Start copy node "b" lags which are not present in node 'a' lags. */
          int b_lag_id = -1;
          bf_bitset_t a_lags, b_lags;
          bf_bs_init(&a_lags, BF_LAG_COUNT, a->l2_chain.lags);
          bf_bs_init(&b_lags, BF_LAG_COUNT, b->l2_chain.lags);

          /* Iterating through LAGIDs of the node 'b'. Merge 'b' LAGs with 'a'.
           */
          for (b_lag_id = bf_bs_first_set(&b_lags, b_lag_id); b_lag_id != -1;
               b_lag_id = bf_bs_first_set(&b_lags, b_lag_id)) {
            bf_bs_set(&a_lags, b_lag_id, 1);
          }
          bf_map_rmv(&st->rdm_to_node, g->base[p] + m);
          bf_map_add(&st->rdm_to_node, g->base[p] + m, grp->mbrs[m]);
          LOG_DBG(
              "Merging grp-id %#x mbr %d PRE%d %p PRE%d %p: Dev %d RDM [%#x "
              "%#x %#x %#x] [%#x %#x %#x %#x] RID %#x",
              grp->ecmp_id,
              m,
              P,
              (void *)grp->mbrs[m],
              p,
              (void *)g->mbrs[m],
              grp->dev,
              grp->base[0] + m,
              grp->base[1] + m,
              grp->base[2] + m,
              grp->base[3] + m,
              g->base[0] + m,
              g->base[1] + m,
              g->base[2] + m,
              g->base[3] + m,
              grp->mbrs[m]->rid);
          MC_MGR_FREE(g->mbrs[m]);
        }
        /* Copy over the base and vector addresses from this pipe. */
        grp->base[p] = g->base[p];
        grp->vector_node_addr[0][p] = g->vector_node_addr[0][p];
        grp->vector_node_addr[1][p] = g->vector_node_addr[1][p];
        /* Combine the L1-ECMP pointer nodes in the two groups. */
        for (a = grp->refs; a; a = a->ecmp_next) {
          bool matched = false;
          for (b = g->refs; b; b = b->ecmp_next) {
            if (a->mgid == b->mgid) {
              a->hw_nodes[p].rdm_addr = b->hw_nodes[p].rdm_addr;
              a->hw_nodes[p].sw_node = a;
              bf_map_rmv(&st->rdm_to_node, b->hw_nodes[p].rdm_addr);
              bf_map_add(&st->rdm_to_node, a->hw_nodes[p].rdm_addr, a);
              BF_LIST_DLL_REM(g->refs, b, ecmp_next, ecmp_prev);
              matched = true;
              LOG_DBG(
                  "Merging ptr grp-id %#x MGID %#x PRE%d %p PRE%d %p: Dev %d "
                  "RDM [%#x %#x "
                  "%#x %#x] [%#x %#x %#x %#x]",
                  grp->ecmp_id,
                  a->mgid,
                  P,
                  (void *)a,
                  p,
                  (void *)b,
                  grp->dev,
                  a->hw_nodes[0].rdm_addr,
                  a->hw_nodes[1].rdm_addr,
                  a->hw_nodes[2].rdm_addr,
                  a->hw_nodes[3].rdm_addr,
                  b->hw_nodes[0].rdm_addr,
                  b->hw_nodes[1].rdm_addr,
                  b->hw_nodes[2].rdm_addr,
                  b->hw_nodes[3].rdm_addr);
              MC_MGR_FREE(b);
              break;
            }
          }
          /* If no match was found then it indicates the group was associated
           * in the first PRE but not the later PRE.  Delete the association so
           * that it can be re-added properly, to all PREs, by the replay. */
          if (!matched) {
            LOG_DBG(
                "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
                "XID %#x XID-Val %d (ECMP-Ptr unmatched)",
                (void *)a,
                a->dev,
                a->mgid,
                a->hw_nodes[0].rdm_addr,
                a->hw_nodes[1].rdm_addr,
                a->hw_nodes[2].rdm_addr,
                a->hw_nodes[3].rdm_addr,
                a->rid,
                a->xid,
                a->xid_valid);
            bf_map_add(bad_nodes, (unsigned long)a, NULL);
          }
        }
        /* There should not be any more L1-ECMP pointer nodes on the second
         * group, they all should have matched pointer nodes on the first group
         * and should have been merged.  If there are left over pointer nodes it
         * indicates that node was associated in some PREs but not others.  We
         * will delete these associations and let the replay add them to all
         * PREs if they are needed. */
        while (g->refs) {
          b = g->refs;
          BF_LIST_DLL_REM(g->refs, b, ecmp_next, ecmp_prev);
          LOG_DBG(
              "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
              "XID %#x XID-Val %d (ECMP-Ptr extra)",
              (void *)b,
              b->dev,
              b->mgid,
              b->hw_nodes[0].rdm_addr,
              b->hw_nodes[1].rdm_addr,
              b->hw_nodes[2].rdm_addr,
              b->hw_nodes[3].rdm_addr,
              b->rid,
              b->xid,
              b->xid_valid);
          bf_map_add(bad_nodes, (unsigned long)b, NULL);
        }
        /* Remove the group from the group map of this pipe. */
        bf_map_rmv(&ecmp_grps[p], ecmp_grp_id);
        MC_MGR_FREE(g);
      }
      /* If we could combine the group in all PREs then add it to the set of
       * good groups in our state. */
      bf_map_add(&st->ecmp_grps, ecmp_grp_id, grp);

      /* Assign the next group to be processed from pipe "P" and copy the next
       * group "get" status. The status is copied as we have reused status "s"
       * for different map APIs.*/
      grp = grp_next;
      s = grp_sts;
    }
  }
}

static void clean_up_bad_groups(struct mc_mgr_dev_hw_state *st,
                                bf_map_t ecmp_grps[MC_MGR_NUM_PIPES],
                                bf_map_t *bad_groups,
                                bf_map_t *bad_nodes) {
  int p;
  void *dc;
  unsigned long ecmp_grp_id;
  while (BF_MAP_OK == bf_map_get_first_rmv(bad_groups, &ecmp_grp_id, &dc)) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++p) {
      mc_ecmp_grp_t *grp = NULL;
      bf_map_sts_t s = bf_map_get(&ecmp_grps[p], ecmp_grp_id, (void **)&grp);
      if (!grp || s != BF_MAP_OK) continue;
      /* Free the vector nodes in the RDM. */
      if (grp->vector_node_addr[0][p])
        mc_mgr_rdm_mark_addr_free(
            st->rdm_map, grp->dev, grp->vector_node_addr[0][p], p);
      if (grp->vector_node_addr[1][p])
        mc_mgr_rdm_mark_addr_free(
            st->rdm_map, grp->dev, grp->vector_node_addr[1][p], p);
      /* Clean up each of the member nodes. */
      int mbr;
      for (mbr = 0; mbr < MC_ECMP_GROUP_MAX_MBR; ++mbr) {
        if (!grp->mbrs[mbr]) continue;
        uint32_t mbr_addr = grp->mbrs[mbr]->hw_nodes[p].rdm_addr;
        if (!mbr_addr) continue;
        uint32_t l2_addr = mc_mgr_rdm_l1_node_get_l2_ptr(st->rdm_map, mbr_addr);
        while (l2_addr) {
          mc_mgr_rdm_mark_addr_free(st->rdm_map, grp->dev, l2_addr, p);
          l2_addr = mc_mgr_rdm_next_l2(st->rdm_map, l2_addr, true);
        }
      }
      /* Free the member block in the RDM. */
      mc_mgr_rdm_mark_addr_free(st->rdm_map, grp->dev, grp->base[p], p);

      /* Add all the ECMP pointer nodes to the list of bad nodes. */
      while (grp->refs) {
        mc_l1_node_t *ptr_node = grp->refs;
        BF_LIST_DLL_REM(grp->refs, ptr_node, ecmp_next, ecmp_prev);
        LOG_DBG(
            "Bad Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x "
            "XID %#x XID-Val %d (ECMP-Grp)",
            (void *)ptr_node,
            ptr_node->dev,
            ptr_node->mgid,
            ptr_node->hw_nodes[0].rdm_addr,
            ptr_node->hw_nodes[1].rdm_addr,
            ptr_node->hw_nodes[2].rdm_addr,
            ptr_node->hw_nodes[3].rdm_addr,
            ptr_node->rid,
            ptr_node->xid,
            ptr_node->xid_valid);
        bf_map_add(bad_nodes, (unsigned long)ptr_node, NULL);
      }
      /* Free memory for the group. */
      MC_MGR_FREE(grp);
    }
  }
}

static void free_bad_node(struct mc_mgr_dev_hw_state *st, mc_l1_node_t *node) {
  int p;
  LOG_DBG(
      "Cleaning Node %p: Dev %d MGID %#x RDM %#x %#x %#x %#x RID %#x XID %#x "
      "XID-Val %d",
      (void *)node,
      node->dev,
      node->mgid,
      node->hw_nodes[0].rdm_addr,
      node->hw_nodes[1].rdm_addr,
      node->hw_nodes[2].rdm_addr,
      node->hw_nodes[3].rdm_addr,
      node->rid,
      node->xid,
      node->xid_valid);
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++p) {
    uint32_t rdm_addr = node->hw_nodes[p].rdm_addr;
    /* If the node isn't in this PRE then move on. */
    if (!rdm_addr) continue;
    MC_MGR_DBGCHK(!node->hw_nodes[p].next && !node->hw_nodes[p].prev);
    /* Free this node's RDM resources; the entire L2 chain and the space the
     * node itself occupies. */
    uint32_t l2_addr = mc_mgr_rdm_l1_node_get_l2_ptr(st->rdm_map, rdm_addr);
    mc_mgr_rdm_mark_addr_free(st->rdm_map, node->dev, rdm_addr, p);
    while (l2_addr) {
      mc_mgr_rdm_mark_addr_free(st->rdm_map, node->dev, l2_addr, p);
      l2_addr = mc_mgr_rdm_next_l2(st->rdm_map, l2_addr, true);
    }
    /* Remove the node from the address-to-node mapping. */
    bf_map_rmv(&st->rdm_to_node, rdm_addr);
  }
  /* Free the memory for this node. */
  MC_MGR_DBGCHK(!node->ecmp_next && !node->ecmp_prev);
  MC_MGR_FREE(node);
}

static bf_status_t rebuild_l1_chains(struct mc_mgr_dev_hw_state *st,
                                     bf_map_t *bad_nodes) {
  int i, p;
  void *dc;
  bf_map_sts_t s;
  for (i = 0; i < BF_MGID_COUNT; ++i) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(st->dev); ++p) {
      /* Start at the MIT. */
      uint32_t l1_addr = st->mit[p][i];
      if (!l1_addr) continue;
      mc_l1_node_t *n = NULL;
      s = bf_map_get(&st->rdm_to_node, l1_addr, (void **)&n);
      if (BF_MAP_OK != s) {
        MC_MGR_DBGCHK(BF_MAP_OK == s);
        return BF_UNEXPECTED;
      }
      MC_MGR_DBGCHK(n);
      MC_MGR_DBGCHK(st->htrees[p][i] == NULL);
      /* If the first node is a bad node then we'll need to update the MIT as
       * well with the proper valid L1 address. */
      while (BF_MAP_OK == bf_map_get(bad_nodes, (unsigned long)n, &dc)) {
        l1_addr = mc_mgr_rdm_next_l1(st->rdm_map, n->hw_nodes[p].rdm_addr);
        st->mit[p][i] = l1_addr;
        bf_map_add(&st->mit_dirty[p], i, NULL);
        if (l1_addr) {
          s = bf_map_get(&st->rdm_to_node, l1_addr, (void **)&n);
          if (BF_MAP_OK != s) {
            MC_MGR_DBGCHK(BF_MAP_OK == s);
            return BF_UNEXPECTED;
          }
        } else {
          n = NULL;
          break;
        }
      }
      /* If all the nodes in the chain were bad then we don't have anything to
       * do here; move onto the next PRE or MGID. */
      if (!n) continue;

      /* A node survived the bad-nodes check!  Put it at the head of the L1
       * chain for this PRE. */
      BF_LIST_DLL_AP(st->htrees[p][i], &n->hw_nodes[p], next, prev);

      /* Follow the node's L1-next pointer to the end of the chain. */
      l1_addr = mc_mgr_rdm_next_l1(st->rdm_map, l1_addr);
      if (mc_mgr_rdm_sync_is_tail_addr(st, p, l1_addr)) l1_addr = 0;
      while (l1_addr) {
        /* Get the node at this address. */
        s = bf_map_get(&st->rdm_to_node, l1_addr, (void **)&n);
        if (BF_MAP_OK != s) {
          MC_MGR_DBGCHK(BF_MAP_OK == s);
          return BF_UNEXPECTED;
        }
        MC_MGR_DBGCHK(n);
        MC_MGR_DBGCHK(n->hw_nodes[p].rdm_addr);

        /* Skip over any bad nodes. */
        bool skipped_some_nodes = false;
        while (BF_MAP_OK == bf_map_get(bad_nodes, (unsigned long)n, &dc)) {
          skipped_some_nodes = true;
          l1_addr = mc_mgr_rdm_next_l1(st->rdm_map, l1_addr);
          if (mc_mgr_rdm_sync_is_tail_addr(st, p, l1_addr)) l1_addr = 0;
          if (!l1_addr) {
            n = NULL;
            break;
          }
          s = bf_map_get(&st->rdm_to_node, l1_addr, (void **)&n);
          if (BF_MAP_OK != s) {
            MC_MGR_DBGCHK(BF_MAP_OK == s);
            return BF_UNEXPECTED;
          }
          MC_MGR_DBGCHK(n);
          MC_MGR_DBGCHK(n->hw_nodes[p].rdm_addr);
        }
        /* If we skipped over nodes then the L1-next pointer of the last good
         * node needs to be updated to point to the valid address we found. */
        if (skipped_some_nodes) {
          uint32_t tail_addr = st->htrees[p][i]->prev->rdm_addr;
          mc_mgr_rdm_set_next_l1(st->rdm_map, tail_addr, l1_addr);
          bf_map_add(&st->dirty_rdm, tail_addr, NULL);
        }
        /* If we have a good node then append it onto the chain and get the
         * address of the next L1.  If the last nodes in the chain were all bad
         * nodes it is possible we won't have a node here. */
        if (n) {
          MC_MGR_DBGCHK(l1_addr);
          BF_LIST_DLL_AP(st->htrees[p][i], &n->hw_nodes[p], next, prev);
          l1_addr = mc_mgr_rdm_next_l1(st->rdm_map, l1_addr);
          if (mc_mgr_rdm_sync_is_tail_addr(st, p, l1_addr)) l1_addr = 0;
        } else {
          MC_MGR_DBGCHK(!l1_addr);
          break;
        }
      }
    }
  }
  return BF_SUCCESS;
}

static bf_status_t sync_rdm_contents(int sid,
                                     bf_dev_id_t dev,
                                     struct mc_mgr_dev_hw_state *st) {
  int i, p;
  bf_map_t bad_nodes, bad_groups, ecmp_grps[MC_MGR_NUM_PIPES];
  void *dc;

  bf_map_init(&bad_nodes);
  bf_map_init(&bad_groups);
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p)
    bf_map_init(&ecmp_grps[p]);

  /* Setup tail info. */
  st->tail_info = mc_mgr_ctx_tail_info(dev);
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    bool okay = mc_mgr_rdm_sync_tails(sid, dev, p, st);
    if (!okay) {
      LOG_ERROR("Error syncing multicast tails, dev %d pipe %d", dev, p);
      return BF_UNEXPECTED;
    }
  }

  /*
   * Read RDM by going through each MGID in each pipe/PRE using the MIT as the
   * first RDM address and then following the Next-L1 pointers to read each
   * multicast tree.  Once all pipes/PREs for a given MGID have been read we
   * will combine the data read back for the nodes into as few structures as
   * possible.  For example, if a node had ports in three pipes we would have
   * read back three separate node structures that are then combined into a
   * single node structure containing the combined port membership.
   * */

  for (i = 0; i < BF_MGID_COUNT; ++i) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      /* Use each entry in each MIT table as a starting point. */
      uint32_t addr = st->mit[p][i];
      if (!addr) continue;
      /* Read all the L1 and L2 nodes on this chain as well as any ECMP groups
       * the chain points to. */
      bool okay = sync_l1_chain(sid, dev, p, i, st, &ecmp_grps[p], addr);
      if (!okay) {
        /* The address pointed to by the MIT doesn't contain a valid node in
         * the RDM or the first node had an error.  Correct the MIT shadow and
         * mark it as dirty. */
        st->mit[p][i] = 0;
        bf_map_add(&st->mit_dirty[p], i, NULL);
      }
    }

    /* We can combine nodes after going through all pipes for an MGID since a
     * node can be in at most one MGID. */
    combine_non_ecmp_nodes(st, &bad_nodes, i);
  }

  /* Combine ECMP groups read from different pipes.  Each ECMP group should be
   * in all pipes. */
  combine_ecmp_groups_and_nodes(st, ecmp_grps, &bad_groups, &bad_nodes);

  /* Clean up all the bad ECMP groups; we only track the bad groups by id so
   * try to pull out a group with each bad id from the array of maps that hold
   * the groups read from hardware. The groups are never partially merged from
   * different PRE. */
  clean_up_bad_groups(st, ecmp_grps, &bad_groups, &bad_nodes);

  /* We are done with these temporary maps so clean them up. */
  bf_map_destroy(&bad_groups);
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p)
    bf_map_destroy(&ecmp_grps[p]);

  /* Rebuild the hardware trees from the data read from hardware. */
  rebuild_l1_chains(st, &bad_nodes);

  /* Clean up the bad nodes. */
  mc_l1_node_t *n = NULL;
  while (BF_MAP_OK ==
         bf_map_get_first_rmv(&bad_nodes, (unsigned long *)&n, &dc)) {
    free_bad_node(st, n);
  }
  MC_MGR_DBGCHK(!bf_map_count(&bad_nodes));
  bf_map_destroy(&bad_nodes);

  return BF_SUCCESS;
}
