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


#include <string.h>
#include "mc_mgr_int.h"
#include "mc_mgr_bh.h"
#include "mc_mgr_handle.h"

mc_l1_node_t *mc_mgr_node_alloc(bf_dev_id_t dev, bf_mc_rid_t rid) {
  bf_mc_node_hdl_t node_hdl;

  /* Allocate the handle. */
  if (BF_SUCCESS != mc_mgr_encode_l1_node_hdl(dev, &node_hdl)) {
    LOG_ERROR("Out of node handles at %s:%d", __func__, __LINE__);
    return NULL;
  }

  /* Allocate the node. */
  mc_l1_node_t *n = MC_MGR_MALLOC(sizeof(mc_l1_node_t));
  if (!n) {
    mc_mgr_delete_l1_node_hdl(dev, node_hdl);
    return NULL;
  }
  /* Initialize fields. */
  MC_MGR_MEMSET(n, 0, sizeof(mc_l1_node_t));
  n->mgid = -1;
  n->dev = dev;
  n->rid = rid;
  n->handle = node_hdl;
  n->ecmp_grp = NULL;

  /* Insert the node into the DB. */
  bf_map_sts_t s = bf_map_add(mc_mgr_ctx_db_l1(dev), node_hdl, n);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  return n;
}

bf_status_t mc_mgr_node_free(bf_dev_id_t dev, bf_mc_node_hdl_t hdl) {
  if (!mc_mgr_decode_l1_node_hdl(hdl, NULL, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }

  mc_l1_node_t *n = mc_mgr_lookup_l1_node(dev, hdl, __func__, __LINE__);
  if (!n) return BF_INVALID_ARG;

  int i;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); ++i) {
    if (n->hw_nodes[i].rdm_addr) {
      LOG_ERROR("%#x %#x %#x %#x",
                n->hw_nodes[0].rdm_addr,
                n->hw_nodes[1].rdm_addr,
                n->hw_nodes[2].rdm_addr,
                n->hw_nodes[3].rdm_addr);
      /* HW resources still exist. */
      LOG_ERROR("Cannot delete node (handle %#x), resources still exist", hdl);
      return BF_IN_USE;
    }
  }

  /* Remove it from the L1 node database. */
  bf_map_sts_t s = bf_map_rmv(mc_mgr_ctx_db_l1(dev), hdl);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  /* Release the node handle. */
  mc_mgr_delete_l1_node_hdl(dev, n->handle);

  /* Release memory allocated for the node. */
  MC_MGR_FREE(n);

  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_node_membership(int sid,
                                       mc_l1_node_t *node,
                                       bf_mc_port_map_t ports,
                                       bf_mc_lag_map_t lags) {
  bf_status_t sts = BF_SUCCESS;

  int i, j;
  bool has_ports = false;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    bf_bitset_t bs_p;
    bf_bs_init(&bs_p, BF_PIPE_PORT_COUNT, node->l2_chain.ports[i]);
    int step = BF_PIPE_PORT_COUNT / 8;
    for (j = 0; j < step; ++j) {
      bf_bs_set_word(&bs_p, 8 * j, 8, ports[j + i * step]);
    }
    has_ports = !bf_bs_all_0s(&bs_p) || has_ports;
  }
  /* Copy the LAGs into the node. */
  bf_bitset_t bs_l;
  bf_bs_init(&bs_l, BF_LAG_COUNT, node->l2_chain.lags);
  for (i = 0; i < BF_MC_LAG_ARRAY_SIZE; ++i) {
    bf_bs_set_word(&bs_l, 8 * i, 8, lags[i]);
  }

  /* Rewrite the node in HW if it has associated with mgid/ecmp*/
  if (mgid_associated(node) || ecmp_associated(node)) {
    sts = mc_mgr_l1_write(sid, node);
  } else {
    /*no update to the HW*/
  }
  return sts;
}

mc_l1_node_t *mc_mgr_split_node(mc_l1_node_t *n, int pipe_mask) {
  if (n->ecmp_grp || n->ecmp_next || n->ecmp_prev) {
    MC_MGR_DBGCHK(NULL == n->ecmp_grp);
    MC_MGR_DBGCHK(NULL == n->ecmp_next);
    MC_MGR_DBGCHK(NULL == n->ecmp_prev);
    return NULL;
  }

  mc_l1_node_t *nn = MC_MGR_MALLOC(sizeof(mc_l1_node_t));
  if (!nn) return NULL;
  memcpy(nn, n, sizeof *nn);

  uint64_t rmv_lags[BF_BITSET_ARRAY_SIZE(BF_LAG_COUNT)] = {0};

  /* Calcualte the lag counts per pipe for node "n". */
  bf_bitset_t bs_lags;
  bf_bs_init(&bs_lags, BF_LAG_COUNT, n->l2_chain.lags);

  bf_bitset_t bs_rmv_lags;
  bf_bs_init(&bs_rmv_lags, BF_LAG_COUNT, rmv_lags);

  /* Check if the lag is present in the "pipe-mask". We may set the bit
   * for a lag_idx more than once if its present in multtiple pipes. */
  int lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&bs_lags, lag_id))) {
    if (mc_mgr_ctx_lag2pipe_mask(n->dev, lag_id) & pipe_mask) {
      if (mc_mgr_ctx_lag2pipe_mask(n->dev, lag_id) & ~pipe_mask) {
        return NULL;
      }

      bf_bs_set(&bs_rmv_lags, lag_id, 1);
    }
  }

  /* Now iter again and remove the lags which are present in remove lag map. */
  lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&bs_rmv_lags, lag_id))) {
    bf_bs_set(&bs_lags, lag_id, 0);
  }

  /* The node 'n' is getting splitted, hence clear the L2 port mbrs
   * from pipes where pipe_mask bits are set. */
  for (int i = 0; i < (int)mc_mgr_ctx_num_max_pipes(n->dev); ++i) {
    if (pipe_mask & (1u << i)) {
      bf_bitset_t bs;
      bf_bs_init(&bs, BF_PIPE_PORT_COUNT, n->l2_chain.ports[i]);
      bf_bs_set_all(&bs, 0);
      n->l2_count[i] = 0;
      nn->hw_nodes[i].sw_node = nn;
    } else {
      /* The new splitted node "nn" has same data as that of the original
       * node "n". Hence, clear the l2 port mbrs from new splitted node
       * hwere pipe_mask bits are not set.  */
      bf_bitset_t bs;
      bf_bs_init(&bs, BF_PIPE_PORT_COUNT, nn->l2_chain.ports[i]);
      bf_bs_set_all(&bs, 0);
      nn->l2_count[i] = 0;
      memset(nn->hw_nodes + i, 0, sizeof nn->hw_nodes[i]);
    }
  }
  return nn;
}

bf_status_t mc_mgr_node_set_lags_map(mc_l1_node_t *node) {
  if (!node) {
    LOG_ERROR("Node is NULL %s %d", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_id_t dev = node->dev;

  int lag_id = -1;
  bf_bitset_t bs_l;
  bf_bs_init(&bs_l, BF_LAG_COUNT, node->l2_chain.lags);
  while (-1 != (lag_id = bf_bs_first_set(&bs_l, lag_id))) {
    mc_mgr_lag_to_node_map_add(node->dev, lag_id, node);
  }

  LOG_TRACE("Dev: %d Node hdl:%#x added to lags_map", dev, node->handle);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_node_reset_lags_map(mc_l1_node_t *node) {
  if (!node) {
    LOG_ERROR("Node is NULL %s %d", __func__, __LINE__);
    return BF_INVALID_ARG;
  }

  bf_dev_id_t dev = node->dev;

  int lag_id = -1;
  bf_bitset_t bs_l;
  bf_bs_init(&bs_l, BF_LAG_COUNT, node->l2_chain.lags);
  while (-1 != (lag_id = bf_bs_first_set(&bs_l, lag_id))) {
    mc_mgr_lag_to_node_map_rmv(node->dev, lag_id, node);
  }
  LOG_TRACE("Dev: %d Node hdl:%#x removed lags_map", dev, node->handle);
  return BF_SUCCESS;
}
