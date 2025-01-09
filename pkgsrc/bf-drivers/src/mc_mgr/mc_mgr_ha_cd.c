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


#include <mc_mgr/mc_mgr_intf.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include "mc_mgr_bh.h"

static bf_status_t check_simple_mem_and_reg(bf_dev_id_t dev_id,
                                            bf_mc_session_hdl_t shdl,
                                            struct mc_mgr_dev_hw_state *st) {
  bf_status_t sts = BF_SUCCESS;
  int sid = mc_mgr_validate_session(shdl, __func__, __LINE__);
  int d = dev_id;
  int i, v, p;

  /* Reset the L1 time slice if it has changed. */
  if (!st->max_l1_slice_valid || st->max_l1_slice != mc_mgr_ctx_max_l1_ts(d)) {
    sts =
        bf_mc_set_max_nodes_before_yield(shdl, dev_id, mc_mgr_ctx_max_l1_ts(d));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "max-nodes-before-yield",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Copy-to-CPU CPU port if it has changed.
   * This also reset the C2C mask if it was incorrect. */
  uint32_t c2c_mask = mc_mgr_ctx_cpu_port_en(d)
                          ? 1u << mc_dev_port_to_pipe(d, mc_mgr_ctx_cpu_port(d))
                          : 0;
  if (!st->cpu_port_valid || st->cpu_port != mc_mgr_ctx_cpu_port(d) ||
      st->cpu_port_en != mc_mgr_ctx_cpu_port_en(d) ||
      st->c2c_pipe_mask != c2c_mask) {
    sts = bf_mc_set_copy_to_cpu(
        dev_id, mc_mgr_ctx_cpu_port_en(d), mc_mgr_ctx_cpu_port(d));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "cpu-port",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Max-L1 and Max-L2 limits if they have changed. */
  if (!st->max_l1_nodes_valid || st->max_l1_nodes != mc_mgr_ctx_max_l1(d) ||
      !st->max_l2_nodes_valid || st->max_l2_nodes != mc_mgr_ctx_max_l2(d)) {
    sts = bf_mc_set_max_node_threshold(
        shdl, dev_id, mc_mgr_ctx_max_l1(d), mc_mgr_ctx_max_l2(d));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "max-node-threshold",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Fast-Fail-Over mode if it has changed. */
  if (st->ff_en != mc_mgr_ctx_ff_en(d)) {
    if (mc_mgr_ctx_ff_en(d))
      sts = bf_mc_enable_port_fast_failover(shdl, dev_id);
    else
      sts = bf_mc_disable_port_fast_failover(shdl, dev_id);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "Port-Fast-Failover state",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Backup-Port mode if it has changed. */
  if (st->bkup_en != mc_mgr_ctx_bkup_port_en(d)) {
    if (mc_mgr_ctx_bkup_port_en(d))
      sts = bf_mc_enable_port_protection(shdl, dev_id);
    else
      sts = bf_mc_disable_port_protection(shdl, dev_id);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "Port-Protection state",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Global-RID if it has changed. */
  if (st->g_rid != mc_mgr_ctx_glb_rid(d)) {
    sts = bf_mc_set_global_exclusion_rid(shdl, dev_id, mc_mgr_ctx_glb_rid(d));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "Global-RID",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  /* Reset the Port-Forward-State if it has changed.  The top level function to
   * do this works on one port at a time but actually writes 32 ports to the
   * hardware.  As a small optimization we can use the lower level function to
   * just write in blocks of 32. */
  for (i = 0;
       st->port_fwd_state_valid &&
       i < (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(mc_mgr_ctx_num_max_ports(d));
       ++i) {
    st->port_fwd_state_valid =
        st->port_fwd_state[i] == mc_mgr_ctx_port_fwd_state(d, i);
  }
  if (!st->port_fwd_state_valid) {
    for (v = 0; v < 2; ++v) {
      for (i = 0; i < (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(
                          mc_mgr_ctx_num_max_ports(d));
           i += 32) {
        sts = mc_mgr_set_port_mask_wrl(sid, d, v, i);
        if (BF_SUCCESS != sts) {
          LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                    "Port-Forwarding-State",
                    dev_id,
                    bf_err_str(sts));
          return sts;
        }
      }
    }
  }

  /* Reset the LAG Remote Left and Right Counts if they have changed. */
  for (i = 0; st->lit_cnt_valid && i < BF_LAG_COUNT; ++i) {
    st->lit_cnt_valid = st->lit_cnt_l[0][i] == mc_mgr_ctx_lit_np_l(d, i) &&
                        st->lit_cnt_r[0][i] == mc_mgr_ctx_lit_np_r(d, i);
  }
  if (!st->lit_cnt_valid) {
    for (i = 0; i < BF_LAG_COUNT; ++i) {
      sts = bf_mc_set_remote_lag_member_count(shdl,
                                              dev_id,
                                              i,
                                              mc_mgr_ctx_lit_np_l(d, i),
                                              mc_mgr_ctx_lit_np_r(d, i));
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                  "Remote-Lag-Mbr-Counts",
                  dev_id,
                  bf_err_str(sts));
        return sts;
      }
    }
  }

  /* Reset the LAG table if it has changed.  Note that we are using a lower
   * level API here since the higher level API takes a logical port map which
   * we no longer have (we can construct it but why bother). */
  for (i = 0; st->lit_valid && i < BF_LAG_COUNT; ++i) {
    for (p = 0; st->lit_valid && p < (int)mc_mgr_ctx_num_max_pipes(d); ++p) {
      st->lit_valid = bf_bs_equal(&st->lit[i][p], mc_mgr_ctx_lit(d, i, p));
    }
  }
  if (!st->lit_valid) {
    for (v = 0; v < 2; ++v) {
      for (i = 0; i < BF_LAG_COUNT; ++i) {
        sts = mc_mgr_set_lit_wrl(sid, d, v, i);
        if (BF_SUCCESS != sts) {
          LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                    "LAG-Mbrship",
                    dev_id,
                    bf_err_str(sts));
          return sts;
        }
      }
    }
  }

  /* Reset the Port Prune Table (PMT) if it has changed.  Again, we are using a
   * lower level API here since the high level API takes a logical port map
   * which we don't want to reconstruct. */
  for (i = 0; st->pmt_valid && i < (int)mc_mgr_ctx_num_max_ports(d); ++i) {
    st->pmt_valid = bf_bs_equal(&st->pmt[i], mc_mgr_ctx_pmt(d, i));
  }
  if (!st->pmt_valid) {
    for (v = 0; v < 2; ++v) {
      for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(d); ++i) {
        sts = mc_mgr_set_pmt_wrl(sid, d, v, i);
        if (BF_SUCCESS != sts) {
          LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                    "Port-Prune-Table",
                    dev_id,
                    bf_err_str(sts));
          return sts;
        }
      }
    }
  }

  /* Write the backup port table if the two versions were out of sync. */
  if (!st->bkup_ports_valid) {
    for (v = 0; v < 2; ++v) {
      for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(d); ++i) {
        sts = mc_mgr_set_bkup_port_wrl(sid, d, v, i, st->bkup_ports[i]);
        if (BF_SUCCESS != sts) {
          LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                    "Backup-Port-Table",
                    dev_id,
                    bf_err_str(sts));
          return sts;
        }
      }
    }
  }

  /* Fix up the Table Version. */
  if (!st->tbl_ver_valid || (st->tbl_ver != mc_mgr_ctx_tbl_ver(d))) {
    sts = mc_mgr_program_tbl_ver(sid, d, st->tbl_ver);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct %s on dev %d, sts %s",
                "MCast-Pkt-Version",
                dev_id,
                bf_err_str(sts));
      return sts;
    }
  }

  return BF_SUCCESS;
}

static void free_l2_chain(mc_l1_node_t *n, struct mc_mgr_dev_hw_state *st) {
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++p) {
    mc_l1_hw_node_t *h = &n->hw_nodes[p];
    if (!h->rdm_addr) continue;

    mc_mgr_rdm_line_t *line = &st->rdm_map->rdm[h->rdm_addr / 2];
    /* Continue if node is valid */
    if (mc_mgr_rdm_node_type_invalid != line->type[h->rdm_addr & 1]) {
      uint32_t l2_addr =
          mc_mgr_rdm_l1_node_get_l2_ptr(st->rdm_map, h->rdm_addr);
      mc_mgr_rdm_mark_addr_free(st->rdm_map, n->dev, h->rdm_addr, p);
      while (l2_addr) {
        mc_mgr_rdm_mark_addr_free(st->rdm_map, n->dev, l2_addr, p);
        l2_addr = mc_mgr_rdm_next_l2(st->rdm_map, l2_addr, true);
      }
    }
  }
}
static void rmv_node(mc_l1_node_t *n, struct mc_mgr_dev_hw_state *st) {
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++p) {
    mc_l1_hw_node_t *h = &n->hw_nodes[p];
    if (!h->rdm_addr) continue;

    /* Take it out of the RDM address to node mapping. */
    bf_map_rmv(&st->rdm_to_node, h->rdm_addr);

    /* Free the RDM resources, the L1 node and the entire L2 chain.
     * This function also marks L1-RDM address to be freed. */
    free_l2_chain(n, st);
    /* Check if this is the first node in the tree. */
    if (h->rdm_addr == st->mit[p][n->mgid]) {
      /* First in the chain, update MIT. Add the mgid to the dirty
       * map such that later updates can be sent to HW to update
       * the MIT table.*/
      st->mit[p][n->mgid] = h->next ? h->next->rdm_addr : 0;
      bf_map_add(&st->mit_dirty[p], n->mgid, NULL);
    } else {
      /* Middle or end of chain, update previous node's next. */
      mc_l1_hw_node_t *prev = h->prev;
      uint32_t next_l1_addr = h->next ? h->next->rdm_addr : 0;
      mc_mgr_rdm_set_next_l1(st->rdm_map, prev->rdm_addr, next_l1_addr);
      bf_map_add(&st->dirty_rdm, prev->rdm_addr, NULL);
    }
    bf_map_add(&st->dirty_rdm, h->rdm_addr, NULL);
    h->rdm_addr = 0;
    /* Remove the node from the list if the lists are populated. */
    if (st->htrees[p][n->mgid]) {
      BF_LIST_DLL_REM(st->htrees[p][n->mgid], h, next, prev);
    }
  }
}
static void rmv_ecmp(mc_ecmp_grp_t **g, struct mc_mgr_dev_hw_state *st) {
  mc_ecmp_grp_t *grp = *g;
  /* Remove the ECMP pointer nodes. */
  while (grp->refs) {
    mc_l1_node_t *n = grp->refs;
    BF_LIST_DLL_REM(grp->refs, n, ecmp_next, ecmp_prev);
    rmv_node(n, st);
    MC_MGR_FREE(n);
  }
  /* Clean up each member. */
  for (int m = 0; grp->mbrs[0] != st->dummy && m < MC_ECMP_GROUP_MAX_MBR; ++m) {
    if (!grp->mbrs[m]) continue;
    /* Free L2 nodes in the RDM. */
    free_l2_chain(grp->mbrs[m], st);
    /* Free the memory for the member node. */
    MC_MGR_FREE(grp->mbrs[m]);
    grp->mbrs[m] = NULL;
  }
  /* Free the block of L1 member nodes in the RDM and the vector nodes. */
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(grp->dev); ++p) {
    if (grp->base[p]) {
      mc_mgr_rdm_mark_addr_free(st->rdm_map, grp->dev, grp->base[p], p);
      grp->base[p] = 0;
    }
    mc_mgr_rdm_mark_addr_free(
        st->rdm_map, grp->dev, grp->vector_node_addr[0][p], p);
    mc_mgr_rdm_mark_addr_free(
        st->rdm_map, grp->dev, grp->vector_node_addr[1][p], p);
    grp->vector_node_addr[0][p] = 0;
    grp->vector_node_addr[1][p] = 0;
  }
  bf_map_rmv(&st->ecmp_grps, grp->ecmp_id);
  MC_MGR_FREE(grp);
  *g = NULL;
}
struct ecmp_mgid {
  struct ecmp_mgid *next;
  mc_ecmp_grp_t *ecmp;
  bf_mc_grp_id_t mgid;
  bf_mc_l1_xid_t xid;
  bool use_xid;
};

static bf_status_t match_nodes(bf_dev_id_t dev,
                               struct mc_mgr_dev_hw_state *st,
                               bf_map_t *matched_hw_l1,
                               bf_map_t *extra_l1) {
  mc_l1_node_t *sw_node, *hw_node;
  bf_map_sts_t s;
  unsigned long not_used;
  int m, p;
  bf_map_t maybe_extra_l1;
  bf_map_init(&maybe_extra_l1);

  /* Pass 1, go through all replayed nodes matching them to the nodes read back
   * from hardware.  Mark the read-back nodes with the assigned handles for the
   * ones that match. */
  bf_map_t *l1_db = mc_mgr_ctx_db_l1(dev);
  for (s = bf_map_get_first(l1_db, &not_used, (void **)&sw_node);
       BF_MAP_OK == s;
       s = bf_map_get_next(l1_db, &not_used, (void **)&sw_node)) {
    hw_node = NULL;
    /* Node must be associated to a MGID.  Software only nodes we can ignore
     * and ECMP L1 ptr nodes will be checked later.
     * Note, this will also skip nodes that are members of ECMP groups since
     * they are associated to ECMP groups and not MGIDs. */
    if (!mgid_associated(sw_node) || ecmp_associated(sw_node)) continue;

    /* Find a matching node. Go through all L1 nodes from each pipe and
     * break when the first matching node is found. */
    for (p = 0; !hw_node && p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      mc_l1_hw_node_t *h;
      for (h = st->htrees[p][sw_node->mgid]; h; h = h->next) {
        mc_l1_node_t *n = h->sw_node;
        /* Skip the node if it was already mapped to a SW node. */
        if (n->handle) continue;

        /* TBD: As the rebuilding the chain, don't we need check for L1-ecmp
         * prts? */

        /* Note: Node comparision will consider Port members and LAG members
         * from all the PRE/pipes. */
        if (nodes_equal(n, sw_node)) {
          hw_node = n;
          n->handle = sw_node->handle;
          break;
        }
      }
    }
    /* Check if any matched node is found, if not then add it to the extra-map.
     */
    if (!hw_node) {
      LOG_DBG(
          "Caching extra node %#x for partial match for mgid %#x RID %#x dev "
          "%d ecmp-id %#x",
          sw_node->handle,
          sw_node->mgid,
          sw_node->rid,
          sw_node->dev,
          sw_node->ecmp_grp ? sw_node->ecmp_grp->ecmp_id : 0);
      s = bf_map_add(&maybe_extra_l1, sw_node->handle, sw_node);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to cache node %#x during reconcile (%d)",
                  sw_node->handle,
                  s);
        bf_map_destroy(&maybe_extra_l1);
        return BF_NO_SYS_RESOURCES;
      }
    } else {
      LOG_DBG(
          "Caching matched %#x for partial match for mgid %#x RID %#x dev "
          "%d ecmp-id %#x",
          sw_node->handle,
          sw_node->mgid,
          sw_node->rid,
          sw_node->dev,
          sw_node->ecmp_grp ? sw_node->ecmp_grp->ecmp_id : 0);

      s = bf_map_add(matched_hw_l1, hw_node->handle, hw_node);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to cache hw node %#x during reconcile (%d)",
                  hw_node->handle,
                  s);
        bf_map_destroy(&maybe_extra_l1);
        return BF_NO_SYS_RESOURCES;
      }
    }
  }

  /* Pass 1.5, go through the unmatched replayed nodes and match them against
   * the remaining hardware nodes.  None of them will match completely, but if
   * they match for a subset of the pipes split the HW node and match it to the
   * SW node.  This covers cases where the HW restore node shows a RID-x with
   * ports y and z and there are two SW nodes one with RID-x and port y and
   * another with RID-x and port z.  So we can split the HW node into two nodes
   * (ports x and y are not in the same pipe) and match them to the SW nodes.
   *
   * NOTE: We are splitting HW nodes which have L2 port nodes only. */
  for (s = bf_map_get_first(&maybe_extra_l1, &not_used, (void **)&sw_node);
       BF_MAP_OK == s;
       s = bf_map_get_next(&maybe_extra_l1, &not_used, (void **)&sw_node)) {
    hw_node = NULL;
    /* Just as above, skip ECMP nodes. */
    if (!mgid_associated(sw_node)) continue;

    /* Find a matching node. */
    for (p = 0; !hw_node && p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      mc_l1_hw_node_t *h, *next_h;
      for (h = st->htrees[p][sw_node->mgid]; h; h = next_h) {
        next_h = h->next;
        mc_l1_node_t *n = h->sw_node;
        /* Skip the node if it was already mapped to a SW node/replayed node. */
        if (n->handle) continue;

        if (node_is_subset(sw_node, n)) {
          /* Split the HW node into a node which completely matches the SW node
           * and a node which has the remaining pipes. The "pmask" is port
           * pipe-mask and lag pipe mask.
           *
           * TBD: shall we not consider the backup port pipe mask. */
          int pmask = 0;
          for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
            if (node_has_port_mbrs_in_pipe(sw_node, i)) pmask |= 1u << i;
            if (node_has_lag_mbrs_in_pipe(sw_node, i)) pmask |= 1u << i;
          }

          /* The split function will create a new hw_node for the pipes that
           * match the SW node and remove all config from the original hw_node
           * for those pipes.  This means the next pointer in the original node
           * is no longer part of this list but since we saved a copy of the
           * pointer we can continue to walk the list. */
          hw_node = mc_mgr_split_node(n, pmask);
          if (!hw_node) {
            LOG_ERROR("Failed to allocate node during reconcile");
            bf_map_destroy(&maybe_extra_l1);
            return BF_NO_SYS_RESOURCES;
          }
          /* We need to clean up the htree lists to take care of the new node.
           * When we split out the new node it got its own array of
           * mc_l1_hw_node_t's that should be linked into the htree list but
           * right now only the original node is linked in the list but the new
           * node has a copy of the same next/prev pointers. */
          for (int P = 0; P < (int)mc_mgr_ctx_num_max_pipes(dev); ++P) {
            if (pmask & (1u << P)) {
              /* The new node is in the pipe, replace all references to the old
               * node with references to the new node. */
              struct mc_l1_hw_node_t *itr, *itr_next;
              for (itr = st->htrees[P][sw_node->mgid]; itr; itr = itr_next) {
                itr_next = itr->next;
                if (itr->prev == &n->hw_nodes[P])
                  itr->prev = hw_node->hw_nodes + P;
                if (itr->next == &n->hw_nodes[P])
                  itr->next = hw_node->hw_nodes + P;
              }
              /* Handle the case of a single node list where the node's prev
               * refers back to itself. */
              if (hw_node->hw_nodes[P].prev == &n->hw_nodes[P])
                hw_node->hw_nodes[P].prev = hw_node->hw_nodes + P;
              /* Clear the mc_l1_hw_node_t in the original node since it isn't
               * in this pipe anymore. */
              memset(n->hw_nodes + P, 0, sizeof n->hw_nodes[P]);
            } else {
              /* The new node isn't in this pipe, clear its mc_l1_hw_node_t
               * information. */
              memset(hw_node->hw_nodes + P, 0, sizeof hw_node->hw_nodes[P]);
            }
          }

          hw_node->handle = sw_node->handle;
          /* We potentially need to update st->htrees after splitting the node.
           * If the split node was at the head of the list then st->htrees will
           * now be pointing at something invalid and should be updated to use
           * the new node. */
          for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
            if (~pmask & 1u << i) continue;
            if (st->htrees[i][sw_node->mgid] == &n->hw_nodes[i]) {
              st->htrees[i][sw_node->mgid] = &hw_node->hw_nodes[i];
            }
          }
          break;
        }
      }
    }
    if (!hw_node) {
      LOG_DBG(
          "Caching split extra node %#x for mgid %#x RID %#x dev %d ecmp-id "
          "%#x",
          sw_node->handle,
          sw_node->mgid,
          sw_node->rid,
          sw_node->dev,
          sw_node->ecmp_grp ? sw_node->ecmp_grp->ecmp_id : 0);
      s = bf_map_add(extra_l1, sw_node->handle, sw_node);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to cache node %#x during reconcile (%d)",
                  sw_node->handle,
                  s);
        bf_map_destroy(&maybe_extra_l1);
        return BF_NO_SYS_RESOURCES;
      }
    } else {
      LOG_ERROR(
          "Caching split matched %#x for partial match for mgid %#x RID %#x "
          "dev "
          "%d ecmp-id %#x",
          sw_node->handle,
          sw_node->mgid,
          sw_node->rid,
          sw_node->dev,
          sw_node->ecmp_grp ? sw_node->ecmp_grp->ecmp_id : 0);

      s = bf_map_add(matched_hw_l1, hw_node->handle, hw_node);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to cache hw node %#x during reconcile (%d)",
                  hw_node->handle,
                  s);
        bf_map_destroy(&maybe_extra_l1);
        return BF_NO_SYS_RESOURCES;
      }
    }
  }

  /* Destroy the map. The extra map already has the nodes which
   * are in "maybe_extra_l1" map. We will add these extra replayed
   * nodes later. */
  bf_map_destroy(&maybe_extra_l1);

  /* Pass 2, go through all the read-back nodes and remove the ones don't match
   * any of the replayed nodes.  Need to update MIT, L1-Next, and hardware node
   * previous/next pointers to take out the extra nodes. */
  for (m = 0; m < BF_MGID_COUNT; ++m) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      mc_l1_hw_node_t *next_h = NULL, *h;
      for (h = st->htrees[p][m]; h; h = next_h) {
        next_h = h->next;
        mc_l1_node_t *n = h->sw_node;
        /* Skip the node if it has a corresponding replayed node. */
        if (n->handle) continue;
        /* Skip the node if it is an ECMP node. */
        if (n->ecmp_grp) continue;

        /* This node was read from hardware but not replayed to us, it needs to
         * removed from hardware. */
        rmv_node(n, st);
        MC_MGR_FREE(n);
      }
    }
  }
  return BF_SUCCESS;
}

static bf_status_t match_ecmp_groups(int d,
                                     struct mc_mgr_dev_hw_state *st,
                                     bf_map_t *extra_ecmp,
                                     struct ecmp_mgid **ecmp_to_associate,
                                     bf_map_t *matched_hw_l1) {
  mc_l1_node_t *sw_node;
  mc_ecmp_grp_t *sw_ecmp, *hw_ecmp;
  unsigned long not_used, ecmp_grp_id;
  bf_map_sts_t s;

  /* Go through all the replayed ECMP groups and match them to the synced
   * groups.  Mark the matching groups with the handle assigned to the
   * replayed group. */
  for (s = bf_map_get_first(
           mc_mgr_ctx_db_ecmp(d), &not_used, (void **)&sw_ecmp);
       BF_MAP_OK == s;
       s = bf_map_get_next(
           mc_mgr_ctx_db_ecmp(d), &not_used, (void **)&sw_ecmp)) {
    MC_MGR_DBGCHK(sw_ecmp);

    /* Look for a matching group that was read back from HW. */
    bool matched_group = false;
    for (s = bf_map_get_first(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp);
         s == BF_MAP_OK && !matched_group;
         s = bf_map_get_next(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp)) {
      MC_MGR_DBGCHK(hw_ecmp);
      if (!hw_ecmp) continue;
      /* Skip as the group has already been matched or replayed. */
      if (hw_ecmp->handle) continue;

      /* Groups must have the same number of members. */
      if (__builtin_popcount(sw_ecmp->valid_map) !=
          __builtin_popcount(hw_ecmp->valid_map)) {
        continue;
      }
      /* Groups must have the same membership. */
      uint32_t matched = 0;
      for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
        if (~sw_ecmp->valid_map & (1u << i)) continue;
        MC_MGR_DBGCHK(sw_ecmp->mbrs[i]);
        if (!sw_ecmp->mbrs[i]) continue;
        bool matched_mbr = false;
        for (int j = 0; j < MC_ECMP_GROUP_MAX_MBR; ++j) {
          if (matched & (1u << j)) continue;
          if (~hw_ecmp->valid_map & (1u << j)) continue;
          MC_MGR_DBGCHK(hw_ecmp->mbrs[j]);

          if (!hw_ecmp->mbrs[j]) continue;
          matched_mbr = nodes_equal(sw_ecmp->mbrs[i], hw_ecmp->mbrs[j]);

          if (!matched_mbr) continue;
          /* Generally the handle should be zero, but if this is the dummy node
           * then it might have already been set. */
          MC_MGR_DBGCHK(hw_ecmp->mbrs[j]->handle == 0 ||
                        (hw_ecmp->mbrs[j]->handle = sw_ecmp->mbrs[i]->handle));
          hw_ecmp->mbrs[j]->handle = sw_ecmp->mbrs[i]->handle;
          matched |= 1u << j;
          break;
        }
        if (!matched_mbr) break;
      }
      if (matched != hw_ecmp->valid_map) continue;

      /* These two groups are equal to each other.  Mark group with a handle
       * so we know it has been matched. */
      hw_ecmp->handle = sw_ecmp->handle;
      matched_group = true;
      break;
    }
    if (!matched_group) {
      /* The ECMP group did not match any groups read back from HW.  Record it
       * in a map to be added later and record any associations it has. */
      s = bf_map_add(extra_ecmp, sw_ecmp->handle, sw_ecmp);
      if (BF_MAP_OK != s) {
        LOG_ERROR("Failed to cache %s group %#x during reconcile (%d)",
                  matched_group ? "matched" : "unmatched",
                  sw_ecmp->handle,
                  s);
        return BF_NO_SYS_RESOURCES;
      }
      LOG_DBG("Dev %d: Replayed ECMP group 0x%x not found in HW.",
              d,
              sw_ecmp->handle);
      for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
        if (sw_ecmp->valid_map & (1u << i)) {
          LOG_DBG("     : Mbr idx %d Node 0x%x", i, sw_ecmp->mbrs[i]->handle);
        }
      }
      for (struct mc_l1_node_t *r = sw_ecmp->refs; r; r = r->ecmp_next) {
        if (r->xid_valid) {
          LOG_DBG("     : Ref MGID 0x%x Xid 0x%x", r->mgid, r->xid);
        } else {
          LOG_DBG("     : Ref MGID 0x%x no Xid", r->mgid);
        }
      }
      for (sw_node = sw_ecmp->refs; sw_node;) {
        mc_l1_node_t *sw_node_next = sw_node->ecmp_next;
        struct mc_mgr_grp_info *mgrp_info = NULL;

        bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(d);
        s = bf_map_get(grpinfo_map, sw_node->mgid, (void **)&mgrp_info);
        if (s != BF_MAP_OK || !mgrp_info) {
          LOG_WARN("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
                   sw_node->mgid,
                   d,
                   s);
          return BF_OBJECT_NOT_FOUND;
        }

        /* Remove the umatched replayed L1 ptr from the members map. These
         * will pushed to HW later. */
        bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
        s = bf_map_rmv(mbrs_map, sw_node->handle);
        MC_MGR_DBGCHK(BF_MAP_OK == s);
        mc_mgr_mgrp_ecmp_dec_l1_count(d, mgrp_info);

        BF_LIST_DLL_REM(sw_ecmp->refs, sw_node, ecmp_next, ecmp_prev);
        struct ecmp_mgid *x = MC_MGR_MALLOC(sizeof *x);
        if (!x) {
          LOG_ERROR("Failed to cache ecmp association during reconcile");
          return BF_NO_SYS_RESOURCES;
        }

        x->next = *ecmp_to_associate;
        *ecmp_to_associate = x;
        x->ecmp = sw_ecmp;
        x->mgid = sw_node->mgid;
        x->xid = sw_node->xid;
        x->use_xid = sw_node->xid_valid;
        clear_hw_locations(sw_node);
        bf_status_t sts = mc_mgr_node_free(d, sw_node->handle);
        if (BF_SUCCESS != sts) {
          MC_MGR_DBGCHK(BF_SUCCESS == sts);
          return sts;
        }
        sw_node = sw_node_next;
      }
    } else {
      /* Add the mbrs of the HW ECMP group to the matched_hw_l1 db if they are
       * real members (i.e. not the dummy node). */
      if (hw_ecmp->mbrs[0] != st->dummy) {
        for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
          if (~hw_ecmp->valid_map & (1u << i)) continue;
          mc_l1_node_t *mbr = hw_ecmp->mbrs[i];
          s = bf_map_add(matched_hw_l1, mbr->handle, mbr);
          if (BF_MAP_OK != s) {
            LOG_ERROR(
                "Failed to cache ecmp mbr %d hw node %#x grp %#x id %#x during "
                "reconcile (%d)",
                i,
                mbr->handle,
                hw_ecmp->handle,
                hw_ecmp->ecmp_id,
                s);
            return BF_NO_SYS_RESOURCES;
          }
        }
      }
    }
  }

  /* Go through all the read back ECMP groups and delete the ones which were
   * not replayed. */
  bf_map_t ecmps_to_del;
  bf_map_init(&ecmps_to_del);
  for (s = bf_map_get_first(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp);
       s == BF_MAP_OK;
       s = bf_map_get_next(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp)) {
    MC_MGR_DBGCHK(hw_ecmp);
    if (!hw_ecmp) continue;
    MC_MGR_DBGCHK(hw_ecmp->ecmp_id == ecmp_grp_id);
    if (hw_ecmp->handle) continue;
    bf_map_add(&ecmps_to_del, ecmp_grp_id, hw_ecmp);
  }
  while (BF_MAP_OK ==
         bf_map_get_first_rmv(&ecmps_to_del, &ecmp_grp_id, (void **)&hw_ecmp)) {
    rmv_ecmp(&hw_ecmp, st);
  }
  bf_map_destroy(&ecmps_to_del);

  /* Go through all the ECMP groups and mark their IDs as used in the id
   * allocator. */
  MC_MGR_DBGCHK(!st->ecmp_hw_id_gen);
  st->ecmp_hw_id_gen = bf_id_allocator_new(0xFFFF, false);
  for (s = bf_map_get_first(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp);
       s == BF_MAP_OK;
       s = bf_map_get_next(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp)) {
    bf_id_allocator_set(st->ecmp_hw_id_gen, ecmp_grp_id);
  }

  return BF_SUCCESS;
}

static bf_status_t match_ecmp_associations(int d,
                                           struct mc_mgr_dev_hw_state *st,
                                           struct ecmp_mgid **ecmp_to_associate,
                                           bf_map_t *matched_hw_l1) {
  (void)d;
  bf_status_t sts;
  mc_ecmp_grp_t *sw_ecmp, *hw_ecmp;
  mc_l1_node_t *sw_node, *hw_node;
  unsigned long ecmp_grp_id;
  bf_map_sts_t s;

  /* Reconcile the ECMP group to MGID associations.  For each matched group
   * find the replayed group and walk through all the replayed associations
   * matching them to the read back associations. */
  struct mc_mgr_grp_info *mgrp_info = NULL;
  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(d);
  for (s = bf_map_get_first(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp);
       s == BF_MAP_OK;
       s = bf_map_get_next(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp)) {
    /* As the ecmp group has been has been reconciled with the replayed node.
     * Look up the replayed ECMP group that corresponds to the read back group.
     * We use the read back  group key to find the replayed node.
     */
    s = bf_map_get(mc_mgr_ctx_db_ecmp(d), hw_ecmp->handle, (void **)&sw_ecmp);
    if (s != BF_MAP_OK) {
      MC_MGR_DBGCHK(s == BF_MAP_OK);
      return BF_OBJECT_NOT_FOUND;
    }
    /* Walk the list of associations on the replayed group and find a matching
     * association on the recovered group.  If there is a match give the node
     * the new handle.  If there is no match then save the node so we can add
     * the association again later. */
    for (sw_node = sw_ecmp->refs; sw_node;) {
      mc_l1_node_t *sw_node_next = sw_node->ecmp_next;
      for (hw_node = hw_ecmp->refs; hw_node; hw_node = hw_node->ecmp_next) {
        if (sw_node->mgid == hw_node->mgid) {
          hw_node->handle = sw_node->handle;
          break;
        }
      }
      if (!hw_node) {
        s = bf_map_get(grpinfo_map, sw_node->mgid, (void **)&mgrp_info);
        if (s != BF_MAP_OK || !mgrp_info) {
          LOG_WARN("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
                   sw_node->mgid,
                   d,
                   s);
          return BF_OBJECT_NOT_FOUND;
        }

        /* Remove the umatched replayed L1 ptr from the members map. These
         * will pushed to HW later. */
        bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
        s = bf_map_rmv(mbrs_map, sw_node->handle);
        MC_MGR_DBGCHK(BF_MAP_OK == s);
        mc_mgr_mgrp_ecmp_dec_l1_count(d, mgrp_info);

        /* We didn't find a matching hardware node so save a record so we know
         * that this MGID and ECMP group need to be associated. */
        struct ecmp_mgid *x = MC_MGR_MALLOC(sizeof *x);
        if (!x) {
          LOG_ERROR("Failed to cache ecmp association during reconcile");
          return BF_NO_SYS_RESOURCES;
        }
        x->next = *ecmp_to_associate;
        *ecmp_to_associate = x;
        x->ecmp = hw_ecmp;
        x->mgid = sw_node->mgid;
        x->xid = sw_node->xid;
        x->use_xid = sw_node->xid_valid;

        /* Unlink it from the group. */
        BF_LIST_DLL_REM(sw_ecmp->refs, sw_node, ecmp_next, ecmp_prev);

        clear_hw_locations(sw_node);
        sts = mc_mgr_node_free(d, sw_node->handle);
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
      } else {
        /* Add the matched L1 pointer to the matched_hw_l1 map. */
        s = bf_map_add(matched_hw_l1, hw_node->handle, hw_node);
        if (BF_MAP_OK != s) {
          LOG_ERROR(
              "Failed to cache sw_node %#x hw node %#x grp %#x id %#x during "
              "reconcile (%d)",
              sw_node->handle,
              hw_node->handle,
              hw_ecmp->handle,
              hw_ecmp->ecmp_id,
              s);
          return BF_NO_SYS_RESOURCES;
        }
      }
      sw_node = sw_node_next;
    }
    /* Remove any extra associations that were not replayed. These extra
     * associations need to be removed to match HW read ecmp group's
     * associated L1-ECMP nodes and replayed ecmp group's associated
     * L1-ECMP nodes. */
    for (hw_node = hw_ecmp->refs; hw_node;) {
      mc_l1_node_t *hw_node_next = hw_node->ecmp_next;
      if (!hw_node->handle) {
        BF_LIST_DLL_REM(hw_ecmp->refs, hw_node, ecmp_next, ecmp_prev);
        rmv_node(hw_node, st);
        MC_MGR_FREE(hw_node);
      }
      hw_node = hw_node_next;
    }
  }
  return BF_SUCCESS;
}

static bf_status_t migrate_state(bf_dev_id_t dev_id,
                                 int sid,
                                 struct mc_mgr_dev_hw_state *st,
                                 bf_map_t *matched_hw_l1) {
  bf_status_t sts = BF_SUCCESS;
  int d = dev_id;
  int i, j;
  unsigned long not_used, ecmp_grp_id;
  mc_ecmp_grp_t *sw_ecmp, *hw_ecmp;
  mc_l1_node_t *sw_node, *hw_node;
  bf_map_sts_t s;
  int num_blks = st->rdm_map->rdm_blk_count;
  /* Two bits per block id, they are packed 16 blocks to an array element. */
  int num_blk_id_elements = num_blks / 16;

  /* Rewrite the RDM block assignments if they are different. */
  for (i = 0; i < num_blk_id_elements; ++i) {
    if (st->rdm_map->blk_ids[i] == mc_mgr_ctx_rdm_map(d)->blk_ids[i]) continue;
    mc_mgr_ctx_rdm_map(d)->blk_ids[i] = st->rdm_map->blk_ids[i];
    sts = mc_mgr_set_rdm_blk_id_grp_wrl(sid, d, i);
    if (sts != BF_SUCCESS) {
      LOG_ERROR("Failed to set block id.  Dev %d blk %d id %d, sts %s",
                d,
                i,
                st->rdm_map->blk_ids[i],
                bf_err_str(sts));
      MC_MGR_DBGCHK(sts == BF_SUCCESS);
      return sts;
    }
  }

  /* Replace the RDM management structures with what we have read back from the
   * hardware. */
  mc_mgr_rdm_map_cleanup(&mc_mgr_ctx_dev(dev_id)->rdm_map);
  mc_mgr_ctx_dev(dev_id)->rdm_map = st->rdm_map;
  st->rdm_map = NULL;

  /* Replace the ECMP HW Id allocator. */
  bf_id_allocator_destroy(mc_mgr_ctx_ecmp_hw_id_gen(d));
  mc_mgr_ctx_dev(d)->ecmp_hw_id_gen = st->ecmp_hw_id_gen;
  st->ecmp_hw_id_gen = NULL;

  /* Replace the L1 node structures that were created during replay with the
   * ones created during the hardware sync. */
  struct mc_mgr_grp_info *mgrp_info = NULL;
  bf_map_t *mbrs_map = NULL;
  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(d);
  for (s = bf_map_get_first(matched_hw_l1, &not_used, (void **)&hw_node);
       BF_MAP_OK == s;
       s = bf_map_get_next(matched_hw_l1, &not_used, (void **)&hw_node)) {
    /* Find the corresponding node from the software replay. */
    s = bf_map_get_rmv(mc_mgr_ctx_db_l1(d), hw_node->handle, (void **)&sw_node);
    if (BF_MAP_OK != s || !sw_node) {
      LOG_ERROR(
          "Failed to clean up duplicate state for multicast node %#x on dev %d",
          hw_node->handle,
          dev_id);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
      MC_MGR_DBGCHK(sw_node);
      return BF_OBJECT_NOT_FOUND;
    }

    /* sw_node here can be 1. L1 ptrs node, 2. L1 node, 3. L1 ecmp mbr node. */
    int mgid = sw_node->mgid;
    if (mgid != -1) {
      s = bf_map_get(grpinfo_map, sw_node->mgid, (void **)&mgrp_info);
      if (s != BF_MAP_OK || !mgrp_info) {
        LOG_ERROR("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
                  mgid,
                  sw_node->dev,
                  s);
        return BF_OBJECT_NOT_FOUND;
      }

      /* First remove the member from the group map. */
      mbrs_map = &mgrp_info->node_mbrs;
      s = bf_map_rmv(mbrs_map, sw_node->handle);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
    }

    /* Delete it. */
    MC_MGR_FREE(sw_node);

    /* Replace it with the node recovered from hardware. */
    s = bf_map_add(mc_mgr_ctx_db_l1(d), hw_node->handle, hw_node);
    if (BF_MAP_OK != s) {
      LOG_ERROR("Failed to update state for multicast node %#x on dev %d",
                hw_node->handle,
                dev_id);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
      return BF_NO_SYS_RESOURCES;
    }

    /* Add to the member map if its l1 node. */
    if (mgid != -1) {
      s = bf_map_add(mbrs_map, hw_node->handle, hw_node);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
    }
  }

  /* Replace the ECMP group structures that were created during the replay with
   * the ones created during the hardware sync. */
  for (s = bf_map_get_first(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp);
       s == BF_MAP_OK;
       s = bf_map_get_next(&st->ecmp_grps, &ecmp_grp_id, (void **)&hw_ecmp)) {
    /* Find the corresponding group from the software replay. */
    s = bf_map_get_rmv(
        mc_mgr_ctx_db_ecmp(d), hw_ecmp->handle, (void **)&sw_ecmp);
    if (BF_MAP_OK != s || !sw_ecmp) {
      LOG_ERROR(
          "Failed to clean up duplicate state for multicast ecmp %#x on dev %d",
          hw_ecmp->handle,
          dev_id);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
      MC_MGR_DBGCHK(sw_ecmp);
      return BF_OBJECT_NOT_FOUND;
    }
    /* Delete it. */
    MC_MGR_FREE(sw_ecmp);
    /* Replace it with the group recovered from hardware. */
    s = bf_map_add(mc_mgr_ctx_db_ecmp(d), hw_ecmp->handle, hw_ecmp);
    if (BF_MAP_OK != s) {
      LOG_ERROR("Failed to update state for multicast ecmp %#x on dev %d",
                hw_ecmp->handle,
                dev_id);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
      return BF_NO_SYS_RESOURCES;
    }
  }

  /* Bring over the correct pointers to the MIT shadow since we've just delete
   * most everything they currently point to. */
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(d); ++i) {
    for (j = 0; j < BF_MGID_COUNT; ++j) {
      struct mc_l1_hw_node_t **mc_l1_ptr = mc_mgr_ctx_tree_ref(d, i, j);
      if (!mc_l1_ptr) continue;

      *mc_l1_ptr = st->htrees[i][j];
    }
  }

  return BF_SUCCESS;
}

static bf_status_t add_missing_l1_nodes(int sid,
                                        bf_dev_id_t dev,
                                        bf_map_t *extra_l1) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_l1_node_t *sw_node = NULL;
  unsigned long not_used;
  for (s = bf_map_get_first(extra_l1, &not_used, (void **)&sw_node);
       BF_MAP_OK == s;
       s = bf_map_get_next(extra_l1, &not_used, (void **)&sw_node)) {
    /* Zero out the old placement for good measure. */
    clear_hw_locations(sw_node);
    /* Call the standard function to write the node to hardware. */
    sts = mc_mgr_l1_write(sid, sw_node);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to resolve HW location for dev %d node %#x",
                dev,
                sw_node->handle);
      MC_MGR_DBGCHK(sts == BF_SUCCESS);
      return sts;
    }
  }
  return BF_SUCCESS;
}

static bf_status_t add_missing_ecmp(int sid,
                                    bf_dev_id_t dev,
                                    struct ecmp_mgid **ecmp_to_associate,
                                    bf_map_t *extra_ecmp) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  unsigned long ecmp_grp_id;
  mc_ecmp_grp_t *sw_ecmp = NULL;
  int i, j, v, p;

  /* First add the missing L1-ECMP associates for the groups that we already
   * have. */
  while (*ecmp_to_associate) {
    struct ecmp_mgid *x = *ecmp_to_associate;
    *ecmp_to_associate = x->next;
    sts = mc_mgr_ecmp_associate(sid, dev, x->mgid, x->ecmp, x->xid, x->use_xid);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to recreate ECMP association, dev %d grpId %#x ecmp handle "
          "%#x",
          dev,
          x->mgid,
          x->ecmp->handle);
      MC_MGR_DBGCHK(sts == BF_SUCCESS);
      return sts;
    }
    MC_MGR_FREE(x);
  }

  /* Second, add the missing ECMP groups with their members and then assocaite
   * them to any MGIDs. */
  for (s = bf_map_get_first(extra_ecmp, &ecmp_grp_id, (void **)&sw_ecmp);
       s == BF_MAP_OK;
       s = bf_map_get_next(extra_ecmp, &ecmp_grp_id, (void **)&sw_ecmp)) {
    /* Allocate a block for the members in each pipe.  First pack the members
     * to ensure there are no holes, then allocate RDM space in each pipe.
     * Note, this compaction logic sure could be improved but the double-for-
     * loop solution is simple and works. Also calculate the pipe masks of the
     * member nodes. */
    int mbr_cnt = 0, mbrs_pm = 0;
    ;
    for (i = 0; i < MC_ECMP_GROUP_MAX_MBR; ++i) {
      if (!sw_ecmp->mbrs[i]) {
        for (j = i + 1; j < MC_ECMP_GROUP_MAX_MBR; ++j) {
          if (sw_ecmp->mbrs[j]) {
            sw_ecmp->mbrs[i] = sw_ecmp->mbrs[j];
            sw_ecmp->mbrs[j] = NULL;
            mbr_cnt++;
            break;
          }
        }
        if (!sw_ecmp->mbrs[i]) break;
      } else {
        mbrs_pm |= mc_mgr_calculate_pipe_mask(sw_ecmp->mbrs[i]);
        mbr_cnt++;
      }
    }
    sw_ecmp->valid_map =
        mbr_cnt == MC_ECMP_GROUP_MAX_MBR ? 0xFFFFFFFF : ((1u << mbr_cnt) - 1);
    sw_ecmp->allocated_sz = mbr_cnt;

    /* Get the L1-End blocks. */
    mc_mgr_rdm_addr_list_t *l1_end_block_addr[MC_MGR_NUM_PIPES] = {0};
    sts = mc_mgr_get_ecmp_blocks(
        sid, dev, mbr_cnt, mbrs_pm, &l1_end_block_addr[0]);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to allocate RDM resources for ECMP group %#x, sz %d on dev "
          "%d",
          sw_ecmp->handle,
          mbr_cnt,
          dev);
      MC_MGR_DBGCHK(sts == BF_SUCCESS);
      return sts;
    }

    /* Allocate and write the vector nodes for all pipes. */
    for (v = 0; v < 2; ++v) {
      for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
        uint32_t a;
        a = mc_mgr_rdm_map_get(sid, dev, p, mc_mgr_rdm_node_type_vector, 1);
        if (!a) {
          LOG_ERROR(
              "Failed to allocate RDM resources for ECMP group %#x (vec) on "
              "dev %d",
              sw_ecmp->handle,
              dev);
          MC_MGR_DBGCHK(a);
          return BF_NO_SYS_RESOURCES;
        }
        sw_ecmp->vector_node_addr[v][p] = a;
      }
    }

    /* Add only empty L1-End blocks to vector nodes in pipes
     * where mbrs_pm != 0.  */
    sts = mc_mgr_ecmp_add_block_n_one_mbr(
        sid,
        dev,
        32,  // max-offset
        mbrs_pm,
        NULL,  // mbr
        sw_ecmp,
        &l1_end_block_addr[0],  // only L1-End block
        NULL,                   // port
        NULL);                  // lag

    /* Write the members in each pipe. */
    for (i = 0; i < MC_ECMP_GROUP_MAX_MBR && sw_ecmp->mbrs[i]; ++i) {
      mc_l1_node_t *mbr = sw_ecmp->mbrs[i];
      if (!mbr) continue;
      mc_mgr_rdm_addr_list_t *l2_port_addrs[MC_MGR_NUM_PIPES] = {NULL};
      mc_mgr_rdm_addr_list_t *l2_lag_addrs[MC_MGR_NUM_PIPES] = {NULL};
      uint32_t l2_len[MC_MGR_NUM_PIPES];

      /* get the individual member pipe-mask. */
      int mbr_pm = mc_mgr_calculate_pipe_mask(mbr);
      for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
        if (!(mbr_pm & (1 << p))) continue;
        sts = mc_mgr_allocate_l2_chain(
            sid, dev, p, mbr, &l2_len[p], &l2_port_addrs[p], &l2_lag_addrs[p]);
        if (BF_SUCCESS != sts) {
          LOG_ERROR(
              "Failed to allocate RDM resources for ECMP group %#x mbr %#x on "
              "dev "
              "%d",
              sw_ecmp->handle,
              sw_ecmp->mbrs[i]->handle,
              dev);
          MC_MGR_DBGCHK(sts == BF_SUCCESS);
          return sts;
        }
      }
      /* Update the L1-End block with mbr L2 chain. */
      sts = mc_mgr_ecmp_upd_one_mbr(sid,
                                    dev,
                                    i,
                                    mbr_pm,  // mbr pipe-mask
                                    mbr,
                                    sw_ecmp,
                                    &l2_port_addrs[0],  // mbr port
                                    &l2_lag_addrs[0]);  // mbr lag
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "Failed to enqueue write for ECMP group %#x mbr %#x on dev %d",
            sw_ecmp->handle,
            sw_ecmp->mbrs[i]->handle,
            dev);
        MC_MGR_DBGCHK(sts == BF_SUCCESS);
        return sts;
      }
    }

    /* Write the vector nodes. No versioning is needed as we are adding for
     * the first time.  */
    mc_mgr_write_actv_vector(
        sid, dev, sw_ecmp, sw_ecmp->valid_map, sw_ecmp->base);
    mc_mgr_write_stby_vector(
        sid, dev, sw_ecmp, sw_ecmp->valid_map, sw_ecmp->base);

    /* Create the group-to-mgid associations. */
    LOG_DBG("Reapplying ECMP group 0x%x associations", sw_ecmp->handle);
    struct ecmp_mgid *to_associate = NULL;
    while (sw_ecmp->refs) {
      LOG_DBG("  Ref %p", (void *)sw_ecmp->refs);
      LOG_DBG("  Ref next %p", (void *)sw_ecmp->refs->ecmp_next);
      mc_l1_node_t *n = sw_ecmp->refs;
      BF_LIST_DLL_REM(sw_ecmp->refs, n, ecmp_next, ecmp_prev);
      struct ecmp_mgid *x = MC_MGR_MALLOC(sizeof *x);
      if (!x) {
        LOG_ERROR("Failed to cache ecmp re-association during reconcile");
        MC_MGR_DBGCHK(x);
        return BF_NO_SYS_RESOURCES;
      }
      x->next = to_associate;
      to_associate = x;
      x->ecmp = sw_ecmp;
      x->mgid = n->mgid;
      x->xid = n->xid;
      x->use_xid = n->xid_valid;
      clear_hw_locations(n);
      sts = mc_mgr_node_free(dev, n->handle);
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
    }
    while (to_associate) {
      struct ecmp_mgid *x = to_associate;
      to_associate = x->next;
      sts =
          mc_mgr_ecmp_associate(sid, dev, x->mgid, x->ecmp, x->xid, x->use_xid);
      if (BF_SUCCESS != sts) {
        LOG_ERROR(
            "Failed to recreate ECMP association, dev %d grpId %#x ecmp handle "
            "%#x",
            dev,
            x->mgid,
            x->ecmp->handle);
        MC_MGR_DBGCHK(sts == BF_SUCCESS);
        return sts;
      }
      MC_MGR_FREE(x);
    }
  }

  return BF_SUCCESS;
}

void mc_mgr_free_ha_hw_state(struct mc_mgr_dev_hw_state *st) {
  for (unsigned int i = 0; i < sizeof st->mit_dirty / sizeof st->mit_dirty[0];
       ++i) {
    bf_map_destroy(&st->mit_dirty[i]);
  }
  bf_map_destroy(&st->rdm_to_node);
  bf_map_destroy(&st->ecmp_grps);
  bf_map_destroy(&st->dirty_rdm);
  if (st->rdm_map) {
    mc_mgr_rdm_map_cleanup(&st->rdm_map);
  }
  bf_sys_free(st);
}

bf_status_t mc_mgr_compute_delta_changes(bf_dev_id_t dev_id,
                                         bool disable_input_pkts) {
  (void)disable_input_pkts;
  if (MC_MGR_INVALID_DEV(dev_id)) {
    LOG_ERROR("Invalid device %u at %s", dev_id, __func__);
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_ctx()->dev_ctx[dev_id]) {
    /* We don't know this device, hence nothing to do */
    return BF_SUCCESS;
  }
  bf_mc_session_hdl_t shdl = mc_mgr_ctx_int_sess();
  int sid = mc_mgr_validate_session(shdl, __func__, __LINE__);
  int d = dev_id;
  bf_status_t sts = BF_SUCCESS;
  int i, p;

  /* Get the data read from HW to compare with our new state.  Reset the state
   * saved in the device context as it isn't needed anymore. */
  struct mc_mgr_dev_hw_state *st = *mc_mgr_ctx_ha_state(dev_id);
  *mc_mgr_ctx_ha_state(dev_id) = NULL;

  /* Switch from syncing to locked as we want to start queueing up DMA
   * operations. */
  mc_mgr_ctx_dev_locked_set(dev_id, true);
  mc_mgr_ctx_rebuilding_set(dev_id);
  mc_mgr_ctx_syncing_clr(dev_id);

  /* First reconcile a few of the simple memories and registers since they are
   * very straightforward.  */
  sts = check_simple_mem_and_reg(dev_id, shdl, st);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(sts == BF_SUCCESS);
    goto done;
  }

  /* Reconcile the non-ECMP nodes replayed to us with the non-ECMP nodes read
   * from hardware. */
  bf_map_t extra_l1, matched_hw_l1;
  bf_map_init(&extra_l1);      /* Replayed but not read from HW. */
  bf_map_init(&matched_hw_l1); /* Read from HW and matched a replayed node. */
  sts = match_nodes(d, st, &matched_hw_l1, &extra_l1);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(sts == BF_SUCCESS);
    goto done;
  }

  /* Reconcile the ECMP groups replayed to us with the ECMP groups read from
   * hardware. */
  struct ecmp_mgid *ecmp_to_associate = NULL;
  bf_map_t extra_ecmp;
  bf_map_init(&extra_ecmp); /* Replayed but not read from HW. */
  sts =
      match_ecmp_groups(d, st, &extra_ecmp, &ecmp_to_associate, &matched_hw_l1);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(sts == BF_SUCCESS);
    goto done;
  }

  /* Reconcile the ECMP Group to MGID associations. */
  sts = match_ecmp_associations(d, st, &ecmp_to_associate, &matched_hw_l1);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(sts == BF_SUCCESS);
    goto done;
  }

  /*
   * At this point we've diffed the replayed nodes and ECMP groups with the HW
   * contents.  The replayed configuration which matched the HW has been matched
   * up and the extra nodes and ECMP groups have been removed.  The next step is
   * to add any nodes and ECMP groups that were not present in the hardware.
   * Before doing that though combine the two software states (the state
   * recovered from hardware and the state rebuilt from replays) so that we can
   * use the usual functions to add nodes and groups.
   */
  sts = migrate_state(dev_id, sid, st, &matched_hw_l1);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(sts == BF_SUCCESS);
    goto done;
  }
  bf_map_destroy(&matched_hw_l1);

  /* Enqueue the MIT corrections onto the DMA write list. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev_id); ++p) {
    unsigned long key;
    void *unused;
    while (BF_MAP_OK ==
           bf_map_get_first_rmv(&st->mit_dirty[p], &key, &unused)) {
      sts = mc_mgr_set_mit_wrl(sid, d, p, key);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to enqueue MIT update on dev %d for %#lx %d sts %s",
                  dev_id,
                  key,
                  p,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }

  /* Enqueue the RDM corrections onto the DMA write list. */
  unsigned long rdm_key = 0;
  void *unused_rdm_data;
  while (BF_MAP_OK ==
         bf_map_get_first_rmv(&st->dirty_rdm, &rdm_key, &unused_rdm_data)) {
    int rdm_line = rdm_key >> 1;
    sts = mc_mgr_set_rdm_wrl(sid,
                             d,
                             rdm_line,
                             mc_mgr_ctx_rdm_map(d)->rdm[rdm_line].data[1],
                             mc_mgr_ctx_rdm_map(d)->rdm[rdm_line].data[0]);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to enqueue RDM update on dev %d for line %#x sts %s",
                dev_id,
                rdm_line,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }

  /* Enqueue the PVT corrections onto a DMA instruction list. */
  for (i = 0; (unsigned int)i < sizeof(st->pvt) / sizeof(st->pvt[0]); ++i) {
    if (st->pvt_valid && (st->pvt[i].d == mc_mgr_ctx_pvt_row(d, i).d)) continue;
    mc_mgr_ctx_pvt_set(d, i, st->pvt[i]);
    sts = mc_mgr_program_pvt(sid,
                             d,
                             i << 3,
                             st->pvt[i].d & 0xF,
                             MC_PVT_MASK_ALL,
                             true,
                             __func__,
                             __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to enqueue PVT update on dev %d for pvt-group %#x sts %s",
          dev_id,
          i,
          bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }

  /* Add the missing L1 nodes. */
  sts = add_missing_l1_nodes(sid, d, &extra_l1);
  bf_map_destroy(&extra_l1);
  if (BF_SUCCESS != sts) {
    goto done;
  }

  /* Add the missing ECMP configuration. */
  sts = add_missing_ecmp(sid, d, &ecmp_to_associate, &extra_ecmp);
  if (BF_SUCCESS != sts) {
    goto done;
  }

  bf_map_destroy(&extra_ecmp);

  /* And finally the backup ports.  Now that the state is basically reconciled
   * we will use the normal APIs to correct any differences in the backup port
   * configuration. */
  for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(dev_id); ++i) {
    if (st->bkup_ports[i] == mc_mgr_ctx_bkup_port(d, i)) continue;
    sts = mc_mgr_set_backup_port(sid, d, i, mc_mgr_ctx_bkup_port(d, i));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to correct backup port, index %d, dev %d, status %s",
                i,
                d,
                bf_err_str(sts));
      MC_MGR_DBGCHK(sts == BF_SUCCESS);
      goto done;
    }
  }

done:
  mc_mgr_free_ha_hw_state(st);
  return sts;
}
