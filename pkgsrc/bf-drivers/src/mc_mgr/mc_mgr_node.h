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


#ifndef __MC_MGR_NODE_H__
#define __MC_MGR_NODE_H__
#include <target-utils/bitset/bitset.h>
#include <mc_mgr/mc_mgr_config.h>
#include <mc_mgr/mc_mgr_types.h>
#include "mc_mgr_int.h"
#include "mc_mgr_rdm.h"

typedef struct mc_ecmp_grp_t mc_ecmp_grp_t;

typedef struct mc_l1_node_t mc_l1_node_t;
typedef struct mc_l1_hw_node_t mc_l1_hw_node_t;

struct mc_l1_hw_node_t {
  /* Link together the HW nodes on the same L1 chain.
   * Note that for nodes in ECMP groups these are NULL. */
  mc_l1_hw_node_t *prev;
  mc_l1_hw_node_t *next;
  uint32_t rdm_addr;
  mc_l1_node_t *sw_node;
};

struct mc_l1_node_t {
  bf_mc_node_hdl_t handle;
  bf_mc_rid_t rid;
  int mgid;
  bf_dev_id_t dev;
  bf_mc_l1_xid_t xid;
  bool xid_valid;
  mc_ecmp_grp_t *ecmp_grp;
  struct {
    uint64_t ports[MC_MGR_NUM_PIPES][BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
    uint64_t lags[BF_BITSET_ARRAY_SIZE(BF_LAG_COUNT)];
  } l2_chain;
  uint32_t l2_count[MC_MGR_NUM_PIPES];
  mc_l1_hw_node_t hw_nodes[MC_MGR_NUM_PIPES];
  /* DLL pointers to store the L1 pointer nodes in a list against an ecmp group.
   */
  mc_l1_node_t *ecmp_next;
  mc_l1_node_t *ecmp_prev;
};

mc_l1_node_t *mc_mgr_node_alloc(bf_dev_id_t dev, bf_mc_rid_t rid);
bf_status_t mc_mgr_node_free(bf_dev_id_t dev, bf_mc_node_hdl_t hdl);
mc_l1_node_t *mc_mgr_split_node(mc_l1_node_t *n, int pipe_mask);
bf_status_t mc_mgr_node_set_lags_map(mc_l1_node_t *n);
bf_status_t mc_mgr_node_reset_lags_map(mc_l1_node_t *n);

static inline bool ecmp_associated(mc_l1_node_t *n) {
  return n->ecmp_grp != NULL;
}
static inline bool mgid_associated(mc_l1_node_t *n) { return n->mgid != -1; }
static inline bool node_mbrship_equal(mc_l1_node_t *a, mc_l1_node_t *b) {
  bf_bitset_t x, y;
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(a->dev); ++i) {
    bf_bs_init(&x, BF_PIPE_PORT_COUNT, a->l2_chain.ports[i]);
    bf_bs_init(&y, BF_PIPE_PORT_COUNT, b->l2_chain.ports[i]);
    if (!bf_bs_equal(&x, &y)) return false;
  }
  bf_bs_init(&x, BF_LAG_COUNT, a->l2_chain.lags);
  bf_bs_init(&y, BF_LAG_COUNT, b->l2_chain.lags);
  return bf_bs_equal(&x, &y);
}
static inline bool node_mbrship_subset(mc_l1_node_t *a, mc_l1_node_t *b) {
  bf_bitset_t x, y;
  int a_pm = 0;
  int b_pm = 0;
  /* TBD: Do we need to consider the backup ports? */
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(a->dev); ++i) {
    bf_bs_init(&x, BF_PIPE_PORT_COUNT, a->l2_chain.ports[i]);
    bf_bs_init(&y, BF_PIPE_PORT_COUNT, b->l2_chain.ports[i]);
    if (bf_bs_all_0s(&x) && !bf_bs_all_0s(&y)) {
      b_pm |= 1 << i;
    }
    if (!bf_bs_equal(&x, &y)) return false;
    a_pm |= 1 << i;
    b_pm |= 1 << i;
  }

  /* As node "a" has subset of port of node "b". Lets remove the pipe
   * mask from node "b" where node "a" ports are present for clean
   * subset consideration. */
  b_pm &= ~a_pm;

  bf_bs_init(&x, BF_LAG_COUNT, a->l2_chain.lags);
  bf_bs_init(&y, BF_LAG_COUNT, b->l2_chain.lags);
  /* As LAGs are not present in all pipes, there
   * should not be any lags from "a" not present in
   * node "b" as we are checking only subset check.  */
  int i = -1;
  while (-1 != (i = bf_bs_first_set(&x, i))) {
    if (!bf_bs_get(&y, i)) return false;
    a_pm |= mc_mgr_ctx_lag2pipe_mask(a->dev, i);
  }

  /* We are looking for clean subsets. Hence lag pipe-mask
   * of node "a" should be subset of lag pipe-mask
   * of node "b".*/
  i = -1;
  while (-1 != (i = bf_bs_first_set(&y, i))) {
    if (bf_bs_get(&x, i)) continue;
    b_pm |= mc_mgr_ctx_lag2pipe_mask(b->dev, i);
  }

  return (a_pm & b_pm) ? false : true;
}
static inline bool node_is_in_rdm(mc_l1_node_t *n) {
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++i)
    if (n->hw_nodes[i].rdm_addr) return true;
  return false;
}
static inline void clear_hw_locations(mc_l1_node_t *n) {
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++i)
    n->hw_nodes[i].rdm_addr = 0;
}
static inline bool nodes_equal(mc_l1_node_t *a, mc_l1_node_t *b) {
  return a->rid == b->rid && a->xid_valid == b->xid_valid && a->xid == b->xid &&
         a->mgid == b->mgid && a->dev == b->dev && node_mbrship_equal(a, b);
}
/* Checks if 'a' is a subset of 'b'.  Returns true if, for every pipe 'a' has
 * ports in 'b' has the same ports.  So, for every pipe 'a' is in, 'b' is also
 * in the same pipe with the same membership. */
static inline bool node_is_subset(mc_l1_node_t *a, mc_l1_node_t *b) {
  return a->rid == b->rid && a->xid_valid == b->xid_valid && a->xid == b->xid &&
         a->mgid == b->mgid && a->dev == b->dev && node_mbrship_subset(a, b);
}
static inline bool node_has_port_mbrs_in_pipe(mc_l1_node_t *n, int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(n->dev)) {
    MC_MGR_DBGCHK(pipe >= 0 && pipe < (int)mc_mgr_ctx_num_max_pipes(n->dev));
    return false;
  }
  bf_bitset_t x;
  bf_bs_init(&x, BF_PIPE_PORT_COUNT, n->l2_chain.ports[pipe]);
  return !bf_bs_all_0s(&x);
}
static inline bool node_has_lag_mbrs_in_pipe(mc_l1_node_t *n, int pipe) {
  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(n->dev)) {
    MC_MGR_DBGCHK(pipe >= 0 && pipe < (int)mc_mgr_ctx_num_max_pipes(n->dev));
    return false;
  }

  int lag_id = -1;
  bf_bitset_t lags;
  bf_bs_init(&lags, BF_LAG_COUNT, n->l2_chain.lags);
  while (-1 != (lag_id = bf_bs_first_set(&lags, lag_id)))
    if (mc_mgr_ctx_lag2pipe_mask(n->dev, lag_id) & (1 << pipe)) return true;
  return false;
}

bf_status_t mc_mgr_set_node_membership(int sid,
                                       mc_l1_node_t *node,
                                       bf_mc_port_map_t logical_ports,
                                       bf_mc_lag_map_t lags);
static inline mc_l1_node_t *node_from_addr(bf_map_t *node_to_addr,
                                           uint32_t rdm_addr) {
  mc_l1_node_t *n = NULL;
  bf_map_sts_t s = bf_map_get(node_to_addr, rdm_addr, (void **)&n);
  return s == BF_MAP_OK ? n : NULL;
}
static inline mc_l1_node_t *next_node_from_addr(bf_map_t *node_to_addr,
                                                mc_mgr_rdm_t *rdm_map,
                                                uint32_t rdm_addr) {
  mc_l1_node_t *n = NULL;
  rdm_addr = mc_mgr_rdm_next_l1(rdm_map, rdm_addr);
  if (!rdm_addr) return NULL;
  bf_map_sts_t s = bf_map_get(node_to_addr, rdm_addr, (void **)&n);
  return s == BF_MAP_OK ? n : NULL;
}
static inline bool node_has_mbr(mc_l1_node_t *node, bf_dev_port_t mbr) {
  bf_dev_pipe_t pipe = mc_dev_port_to_pipe(node->dev, mbr);
  int local_port = mc_dev_port_to_local_port(node->dev, mbr);
  bf_bitset_t x;
  bf_bs_init(&x, BF_PIPE_PORT_COUNT, node->l2_chain.ports[pipe]);
  return bf_bs_get(&x, local_port);
}
#endif
