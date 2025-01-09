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
#include <math.h>
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>
#include "mc_mgr.h"
#include "mc_mgr_int.h"
#include <tofino_regs/pipe_top_level.h>
#include "mc_mgr_bh.h"
#include "mc_mgr_handle.h"
#include "lld/lld_sku.h"
#include <tof3_regs/tof3_mem_addr.h>

/* Macro Definitions */
#define M1(a, i) (a + i * MC_MGR_NUM_PIPES)
#define M2(a, k) (a + k)
#define M3(a, j, k) ((M1(a, j) + k))
#define M4(a, j, k) (M2(a, j * 3) + k)

uint8_t mc_mgr_get_tvt(int dev, int mgid);

void mc_mgr_one_at_a_time_begin() { MC_MGR_LOCK_R(mc_mgr_ctx_lock()); }

bf_status_t mc_mgr_one_at_a_time_begin_try() {
  int err;
  MC_MGR_TRYLOCK_R(mc_mgr_ctx_lock(), err);
  if (err != 0) {
    return BF_IN_USE;
  }
  return BF_SUCCESS;
}

void mc_mgr_one_at_a_time_end() { MC_MGR_UNLOCK_R(mc_mgr_ctx_lock()); }

mc_l1_node_t *mc_mgr_lookup_l1_node(bf_dev_id_t dev,
                                    bf_mc_node_hdl_t h,
                                    const char *where,
                                    const int line) {
  mc_l1_node_t *x = NULL;
  bf_map_sts_t y = bf_map_get(mc_mgr_ctx_db_l1(dev), h, (void *)&x);
  if (BF_MAP_OK != y) {
    LOG_ERROR(
        "Failed to find node %#x in DB (%d) from %s:%d", h, y, where, line);
    return NULL;
  }
  if (!x) {
    LOG_ERROR("Node %#x is NULL in the DB; from %s:%d", h, where, line);
    return NULL;
  }

  if (h != x->handle) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }

  return x;
}

mc_ecmp_grp_t *mc_mgr_lookup_ecmp(bf_dev_id_t dev,
                                  bf_mc_ecmp_hdl_t h,
                                  const char *where,
                                  const int line) {
  mc_ecmp_grp_t *x = NULL;
  bf_map_sts_t y = bf_map_get(mc_mgr_ctx_db_ecmp(dev), h, (void *)&x);
  if (BF_MAP_OK != y) {
    LOG_ERROR(
        "Failed to find ecmp %#x in DB (%d) from %s:%d", h, y, where, line);
    return NULL;
  }
  if (!x) {
    LOG_ERROR("ECMP %#x is NULL in the DB; from %s:%d", h, where, line);
    return NULL;
  }
  return x;
}

int mc_mgr_validate_session(bf_mc_session_hdl_t hdl,
                            const char *where,
                            const int line) {
  int sid = -1;
  if (!mc_mgr_decode_sess_hdl(hdl, &sid)) {
    LOG_ERROR("Invalid session handle type (%#x) from %s:%d", hdl, where, line);
  } else {
    if (MC_MGR_INVALID_SID(sid)) {
      sid = -1;
      LOG_ERROR(
          "Session handle out of range (0x%x) from %s:%d", hdl, where, line);
    } else if (!mc_mgr_ctx_session_state(sid)->valid) {
      sid = -1;
      LOG_ERROR("Invalid session handle (0x%x) from %s:%d", hdl, where, line);
    }
  }

  return sid;
}
bool mc_mgr_validate_dev(bf_dev_id_t dev, const char *where, const int line) {
  bool ret = true;
  if (MC_MGR_INVALID_DEV(dev)) {
    ret = false;
    LOG_ERROR("Device %d is invalid from %s:%d", dev, where, line);
  } else if (!mc_mgr_dev_present(dev)) {
    ret = false;
    LOG_ERROR("Device %d is not present from %s:%d", dev, where, line);
  }
  return ret;
}

bool mc_mgr_get_mgid_map_bit(bf_dev_id_t dev, int grp) {
  int word = grp / MC_MGID_WIDTH;
  uint64_t bit = grp % MC_MGID_WIDTH;
  return mc_mgr_ctx_mgid_blk(dev, word) & (UINT64_C(1) << bit);
}
void mc_mgr_set_mgid_map_bit(bf_dev_id_t dev, int grp, bool val) {
  int word = grp / MC_MGID_WIDTH;
  uint64_t bit = grp % MC_MGID_WIDTH;
  if (val) {
    uint64_t x = mc_mgr_ctx_mgid_blk(dev, word);
    mc_mgr_ctx_mgid_blk_set(dev, word, x | (UINT64_C(1) << bit));
  } else {
    uint64_t x = mc_mgr_ctx_mgid_blk(dev, word);
    mc_mgr_ctx_mgid_blk_set(dev, word, x & ~(UINT64_C(1) << bit));
  }
}

bool mc_mgr_l1_node_contains_port(mc_l1_node_t *node, bf_dev_port_t port_id) {
  int i = 0;
  bf_dev_port_t dev_port = 0;
  int pipe = DEV_PORT_TO_PIPE(port_id);

  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    bf_bitset_t ports;

    if (i != pipe) continue;

    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[i]);

    if (bf_bs_all_0s(&ports)) {
      continue;
    }
    int j = -1;
    while (-1 != (j = bf_bs_first_set(&ports, j))) {
      dev_port = mc_bit_idx_to_dev_port(node->dev, 72 * i + j);
      if (port_id == dev_port) {
        return true;
      }
    }
  }
  return false;
}

static bool mc_mgr_l1_node_has_400g_200g_100g_port(mc_l1_node_t *node) {
  int i = 0;
  bf_dev_port_t dev_port = 0;
  mc_mgr_port_info_t *port_info = NULL;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    bf_dev_id_t dev = node->dev;
    bf_dev_pipe_t pipe = i;
    bf_bitset_t ports;
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[i]);
#if defined(EMU_2DIE_USING_SW_2DEV)
    if (pipe >= BF_SUBDEV_PIPE_COUNT) {
      pipe = pipe % BF_SUBDEV_PIPE_COUNT;
      if (dev == 0) {
        dev = 1;
      } else {
        dev = 0;
      }
    }
#endif

    if (bf_bs_all_0s(&ports)) {
      continue;
    }
    int j = -1;
    while (-1 != (j = bf_bs_first_set(&ports, j))) {
      dev_port = mc_make_dev_port(node->dev, pipe, j);
      port_info = mc_mgr_get_port_info(dev, dev_port);
      if (!port_info) {
        continue;
      }
      if ((port_info->speed == BF_SPEED_400G) ||
          (port_info->speed == BF_SPEED_200G) ||
          (port_info->speed == BF_SPEED_100G)) {
        return true;
      }
    }
  }
  return false;
}

static bool mc_mgr_grp_has_400g_200g_100g_port(bf_dev_id_t dev, int mgid) {
  bf_map_sts_t s;
  mc_l1_node_t *node = NULL;
  unsigned long not_used;
  /* Find all nodes associated with this MGID */
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (mgid == node->mgid) {
      if (mc_mgr_l1_node_has_400g_200g_100g_port(node)) {
        return true;
      }
    }
  }

  return false;
}

static int calculate_port_pipe_mask_per_pipe(int primary_pipe,
                                             mc_l1_node_t *node) {
  // Go through all ports on the node.  If any ports are in the primary pipe
  // add it to the mask.  If any ports which are not in the primary pipe have
  // backup ports in the primary pipe, add their pipe to the mask.
  int mask = 0, i;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    bf_bitset_t ports;
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[i]);

    if (i == primary_pipe) {
      if (!bf_bs_all_0s(&ports)) {
        mask |= (1 << primary_pipe);
      }
    } else {
      int j = -1;
      while (-1 != (j = bf_bs_first_set(&ports, j))) {
        if ((unsigned int)primary_pipe ==
            mc_bit_idx_to_pipe(
                node->dev,
                mc_mgr_ctx_bkup_port(
                    node->dev,
                    mc_dev_port_to_bit_idx(
                        node->dev, mc_make_dev_port(node->dev, i, j))))) {
          mask |= (1 << i);
          break;
        }
      }
    }
  }
  return mask;
}

bf_status_t mc_mgr_update_lag_to_l1_nodes(int sid,
                                          bf_dev_id_t dev,
                                          int lag_id) {
  bf_status_t sts = BF_SUCCESS;
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > lag_id || BF_LAG_COUNT < lag_id) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  bf_map_sts_t s;

  /* Get the MGID map for the lag idx.*/
  bf_map_t *map = mc_mgr_ctx_lag_to_node_map(dev, lag_id);
  if (!map) {
    LOG_DBG("No LAG map found for lag_id:%d %s %d", lag_id, __func__, __LINE__);
    return BF_NOT_IMPLEMENTED;
  }

  /* Iterate through map and find every node that uses the same lag_id */
  unsigned long key;
  void *unused;
  for (s = bf_map_get_first(map, &key, &unused); BF_MAP_OK == s;
       s = bf_map_get_next(map, &key, &unused)) {
    bf_mc_node_hdl_t node_hdl = key;
    mc_l1_node_t *sw_node =
        mc_mgr_lookup_l1_node(dev, node_hdl, __func__, __LINE__);
    if (!sw_node) {
      LOG_ERROR("No node found for hdl:%#x for lag_id:%d", node_hdl, lag_id);
      continue;
    }

    LOG_TRACE("Node hdl:%#x found for lag :%d", node_hdl, lag_id);
    MC_MGR_ASSERT(node_hdl == sw_node->handle);

    /* Node must be associated to a MGID. Skip nodes which are not associated
     * with both MGID and ecmp */
    if (!mgid_associated(sw_node) && !ecmp_associated(sw_node)) continue;

    /* The nodes allowed are l1_node associated with MGID or an ecmp mbr nodes.
     */
    sts = mc_mgr_l1_write(sid, sw_node);
    if (sts != BF_SUCCESS) {
      LOG_ERROR("Failed to push lag(%d) update to node_hdl:0x%x %s %d",
                lag_id,
                sw_node->handle,
                __func__,
                __LINE__);
      return sts;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_lag_to_node_map_rmv(bf_dev_id_t dev,
                                       int lag_id,
                                       mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev || !node) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > lag_id || BF_LAG_COUNT <= lag_id) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Get the MGID map for the lag idx.*/
  bf_map_t *map = mc_mgr_ctx_lag_to_node_map(dev, lag_id);
  if (!map) {
    LOG_DBG("No LAG map found for lag_id:%d %s %d", lag_id, __func__, __LINE__);
    return BF_NOT_IMPLEMENTED;
  }

  void *unused;
  bf_map_sts_t map_sts = bf_map_get_rmv(map, node->handle, &unused);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR("Removed failed node_hdl:%#x lag_id:%d sts:%d %s %d",
              node->handle,
              lag_id,
              sts,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_lag_to_node_map_add(bf_dev_id_t dev,
                                       int lag_id,
                                       mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev || !node) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > lag_id || BF_LAG_COUNT <= lag_id) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Get the MGID map for the lag idx.*/
  bf_map_t *map = mc_mgr_ctx_lag_to_node_map(dev, lag_id);
  if (!map) {
    LOG_DBG("No LAG map found for lag_id:%d %s %d", lag_id, __func__, __LINE__);
    return BF_NOT_IMPLEMENTED;
  }

  bf_map_sts_t map_sts = bf_map_add(map, node->handle, NULL);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR("Add failed node_hdl:%#x lag_id:%d sts:%d %s %d",
              node->handle,
              lag_id,
              sts,
              __func__,
              __LINE__);
    return BF_INVALID_ARG;
  }

  return BF_SUCCESS;
}

int mc_mgr_calculate_lag2pipe_mask(bf_dev_id_t dev, int lag_id) {
  int mask = 0, pipe;

  /* Iterate through every pipe, create the mask for the lag member which
   * includes pipe masks of both primiary pipe as well as backup pipe. */
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    bf_bitset_t *lag_bset = mc_mgr_ctx_lit(dev, lag_id, pipe);

    /* Continue if there are no members in the current pipe. */
    if (bf_bs_all_0s(lag_bset)) {
      continue;
    }

    int port_idx = -1;
    mask |= 1 << pipe;
    while (-1 != (port_idx = bf_bs_first_set(lag_bset, port_idx))) {
      int backup_pipe = mc_bit_idx_to_pipe(
          dev,
          mc_mgr_ctx_bkup_port(
              dev,
              mc_dev_port_to_bit_idx(dev,
                                     mc_make_dev_port(dev, pipe, port_idx))));

      /* Set the mask from the backup pipe. */
      mask |= 1 << backup_pipe;
    }
  }

  return mask;
}

int mc_mgr_calculate_backup_mod_pm(mc_l1_node_t *node,
                                   int old_backup_idx,
                                   int new_backup_idx) {
  int mask = 0, p;

  /* Old and new back pipes. */
  int old_backup_pipe = mc_bit_idx_to_pipe(node->dev, old_backup_idx);
  int new_backup_pipe = mc_bit_idx_to_pipe(node->dev, new_backup_idx);

  /* Compute the pipe mask for this node considering the backup port
   * change. The new_pipe_mask has already taken into consideration the
   * new lag pipe mask of the node. */
  int new_pipe_mask = mc_mgr_calculate_pipe_mask(node);
  int old_pipe_mask = mc_mgr_current_pipe_mask(node);

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++p) {
    /* Check if there are no changes required in this pipe. */
    if ((((~new_pipe_mask & (1 << p)) && (~old_pipe_mask & (1 << p))) ||
         (((new_pipe_mask & (1 << p)) && (old_pipe_mask & (1 << p))))) &&
        (p != old_backup_pipe && p != new_backup_pipe))
      continue;
    mask |= (1 << p);
  }
  return mask;
}
int mc_mgr_calculate_backup_l1_add_mask(mc_l1_node_t *node,
                                        int old_backup_idx,
                                        int new_backup_idx) {
  int mask = 0, p;

  /* Compute the pipe mask for this node considering the backup port
   * change. The new_pipe_mask has already taken into consideration the
   * new lag pipe mask of the node. */
  int new_pipe_mask = mc_mgr_calculate_pipe_mask(node);
  int old_pipe_mask = mc_mgr_current_pipe_mask(node);
  int mod_pipe_mask =
      mc_mgr_calculate_backup_mod_pm(node, old_backup_idx, new_backup_idx);

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++p) {
    if (!(mod_pipe_mask & (1 << p))) continue;
    if ((new_pipe_mask & (1 << p)) && (~old_pipe_mask & (1 << p)))
      mask |= (1 << p);
  }
  return mask;
}

int mc_mgr_calculate_backup_del_mask(mc_l1_node_t *node,
                                     int old_backup_idx,
                                     int new_backup_idx) {
  int mask = 0, p;

  /* Compute the pipe mask for this node considering the backup port
   * change. The new_pipe_mask has already taken into consideration the
   * new lag pipe mask and port mask according to the backup table update
   */
  int new_pipe_mask = mc_mgr_calculate_pipe_mask(node);
  int old_pipe_mask = mc_mgr_current_pipe_mask(node);
  int mod_pipe_mask =
      mc_mgr_calculate_backup_mod_pm(node, old_backup_idx, new_backup_idx);

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++p) {
    if (!(mod_pipe_mask & (1 << p))) continue;
    if ((~new_pipe_mask & (1 << p)) && (old_pipe_mask & (1 << p)))
      mask |= (1 << p);
  }
  return mask;
}

int mc_mgr_calculate_pipe_mask(mc_l1_node_t *node) {
  int mask = 0, i;

  /* Check if this node has any ports in each pipe.
   * Also check if there lag nodes in each pipe. The
   * final is mask is created by '|' port mask and
   * lag mask. */
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    if (calculate_port_pipe_mask_per_pipe(i, node)) mask |= 1 << i;
  }

  /* Iterate through each node's LAG pipe mask and create the final
   * mask.*/
  int lag_id = -1;
  bf_bitset_t lags;
  bf_bs_init(&lags, BF_LAG_COUNT, node->l2_chain.lags);
  while (-1 != (lag_id = bf_bs_first_set(&lags, lag_id)))
    mask |= mc_mgr_ctx_lag2pipe_mask(node->dev, lag_id);
  return mask;
}

int mc_mgr_current_pipe_mask(mc_l1_node_t *node) {
  int i, mask = 0;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++i) {
    if (node->hw_nodes[i].rdm_addr) {
      mask |= 1 << i;
    }
  }
  return mask;
}

uint16_t mc_mgr_current_ecmp_pipe_mask(mc_ecmp_grp_t *grp) {
  int i, mask = 0;
  for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(grp->dev); ++i) {
    if (grp->base[i]) {
      mask |= 1 << i;
    }
  }
  return mask;
}

bool mc_mgr_is_prot_port_node_lags(bf_dev_id_t dev,
                                   mc_l1_node_t *node,
                                   bf_dev_port_t port) {
  if (!node) {
    MC_MGR_DBGCHK(0);
    return false;
  }

  int port_bit_idx = mc_dev_port_to_bit_idx(node->dev, port);
  int pipe = mc_bit_idx_to_pipe(dev, port_bit_idx);
  int local_port = mc_bit_idx_to_local_port(dev, port_bit_idx);

  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(dev)) {
    MC_MGR_DBGCHK(0);
    return false;
  }

  /* Iterate through the node LAGs and check if the protected port is member
   * of its LAGs. */
  bf_bitset_t node_lags;
  bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);
  int lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
    if (!bf_bs_get(mc_mgr_ctx_lit(dev, lag_id, pipe), local_port)) continue;
    return true;
  }

  return false;
}

bf_status_t mc_mgr_update_pvt(int sid,
                              bf_dev_id_t dev,
                              int mgid,
                              bool batch,
                              const char *where,
                              const int line) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (0 > mgid || BF_MGID_COUNT <= mgid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint8_t cur_pvt = mc_mgr_get_pvt(dev, mgid);
  uint8_t new_pvt = 0;
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (mc_mgr_ctx_tree(dev, p, mgid)) {
      new_pvt |= 1 << p;
    }
  }
  if (new_pvt != cur_pvt) {
    if (BF_SUCCESS !=
        mc_mgr_program_pvt(
            sid, dev, mgid, new_pvt, MC_PVT_MASK_ALL, batch, where, line)) {
      return BF_NO_SYS_RESOURCES;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_update_tvt(
    int sid, int dev, int mgid, bool batch, const char *where, const int line) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* TVT table only exists on tofino3 */
  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }

  if (0 > mgid || BF_MGID_COUNT <= mgid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  uint8_t cur_tvt = mc_mgr_get_tvt(dev, mgid);
  uint8_t new_tvt = 0;
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (mc_mgr_ctx_tree(dev, p, mgid)) {
      if (p < BF_SUBDEV_PIPE_COUNT) {
        /* left die present */
        new_tvt |= 0x1;
      } else {
        /* right die present */
        new_tvt |= 0x2;
      }
    }
  }

  /* New TVT value can never be zero. If no egress pipe is mapped for a MGID
   * then intialize with default value of 1 such that packet gets dropped
   * later Our HW/Model expects default value to be always 1. */
  if (new_tvt == 0) {
    new_tvt = 1;
  }

  if (new_tvt != cur_tvt) {
    if (BF_SUCCESS !=
        mc_mgr_program_tvt(sid, dev, mgid, new_tvt, batch, where, line)) {
      return BF_NO_SYS_RESOURCES;
    }
  }
  return BF_SUCCESS;
}

void mc_mgr_delete_l1(mc_l1_hw_node_t *n) {
  if (n == NULL) {
    MC_MGR_DBGCHK(0);
    return;
  }

  if (n->rdm_addr) {
    MC_MGR_DBGCHK(0);
    return;
  }

  n->sw_node = NULL;
}

static void get_new_l1(mc_l1_node_t *node, int pipe, uint32_t rdm_addr) {
  mc_l1_hw_node_t *n = &node->hw_nodes[pipe];
  n->prev = NULL;
  n->next = NULL;
  n->sw_node = node;
  n->rdm_addr = rdm_addr;
}
static inline void link_l1(mc_l1_hw_node_t *n,
                           bf_dev_id_t dev,
                           int pipe,
                           int mgid) {
  mc_l1_hw_node_t **list = mc_mgr_ctx_tree_ref(dev, pipe, mgid);
  if (list) {
    BF_LIST_DLL_PP(*list, n, next, prev);
  } else {
    MC_MGR_DBGCHK(list);
  }
}
static inline void unlink_l1(mc_l1_hw_node_t *n,
                             bf_dev_id_t dev,
                             int pipe,
                             int mgid) {
  mc_l1_hw_node_t **list = mc_mgr_ctx_tree_ref(dev, pipe, mgid);
  if (list) {
    BF_LIST_DLL_REM(*list, n, next, prev);
  } else {
    MC_MGR_DBGCHK(list);
  }
}
uint32_t mc_mgr_get_mit(bf_dev_id_t dev, int pipe, int mgid) {
  return mc_mgr_ctx_tree(dev, pipe, mgid)
             ? mc_mgr_ctx_tree(dev, pipe, mgid)->rdm_addr
             : 0;
}

inline uint32_t mc_mgr_get_bit_mask(int bit_cnt) {
  uint32_t mask = 0;

  if (bit_cnt == 0) {
    mask = 0;
  } else if (bit_cnt >= 32) {
    mask = 0xffffffff;
  } else {
    mask = (1 << bit_cnt) - 1;
  }

  return mask;
}

/* Repair ECC error in MIT entry.
 * 'address' is the table index (mgid) from the err_log. */
bf_status_t mc_mgr_ecc_correct_mit(bf_dev_id_t dev,
                                   int pipe,
                                   uint32_t address) {
  int mgid = 0, sid = 0;
  int bit_cnt = log2(BF_MGID_COUNT);
  bf_status_t sts = BF_SUCCESS;

  mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);

  /* Use only required no of bits from address */
  mgid = address & mc_mgr_get_bit_mask(bit_cnt);

  LOG_TRACE("MIT ECC error, dev %d, mgid %d  ", dev, mgid);

  sts = mc_mgr_set_mit_wrl(sid, dev, pipe, mgid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("MIT ecc update failed (%d) from %s:%d", sts, __func__, __LINE__);
    return sts;
  }

  /* Push the write list to correct ecc */
  return mc_mgr_drv_wrl_send(sid, false);
}

mc_mgr_pvt_entry_t mc_mgr_get_pvt_row(bf_dev_id_t dev, int row) {
  return mc_mgr_ctx_pvt_row(dev, row);
}
uint8_t mc_mgr_get_pvt(bf_dev_id_t dev, int mgid) {
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      /* Packs 8 MGIDs into a 32 bit word. */
      return (mc_mgr_get_pvt_row(dev, mgid >> 3).d >> (4 * (mgid % 8))) & 0xF;
    case BF_DEV_FAMILY_TOFINO2:
      /* Packs 4 MGIDs into a 32 bit word, 5 bit masks where the top bit is
       * always set to one. */
      return (mc_mgr_get_pvt_row(dev, mgid >> 2).d >> (5 * (mgid % 4))) & 0x1F;
    case BF_DEV_FAMILY_TOFINO3:
      /* Packs 8 MGIDs (8+1 bits) into a 64 bit word. */
      return (mc_mgr_get_pvt_row(dev, mgid >> 3).d >> (8 * (mgid % 8))) & 0xFF;
    default:
      MC_MGR_DBGCHK(0);
  }
  return 0;
}

mc_mgr_tvt_entry_t mc_mgr_get_tvt_row(int dev, int row) {
  return mc_mgr_ctx_tvt_row(dev, row);
}

uint8_t mc_mgr_get_tvt(int dev, int mgid) {
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO3:
      /* Packs 4 MGIDs (4 bits each) into a 16 bit word. */
      return (mc_mgr_get_tvt_row(dev, mgid >> 2).diemap >> (4 * (mgid % 4))) &
             0xF;
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    default:
      MC_MGR_DBGCHK(0);
  }
  return 0;
}

uint8_t mc_mgr_get_pvt_hw(bf_dev_id_t dev, int pipe, int mgid) {
  uint32_t masks = 0;
  uint8_t r = 0;
  bool log2phy = false;
  pipe_status_t s;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      s = pipe_mgr_mc_pipe_msk_get(dev, mgid >> 3, pipe, &masks);
      r = (masks >> (4 * (mgid & 0x7))) & 0xF;
      break;
    case BF_DEV_FAMILY_TOFINO3:
    case BF_DEV_FAMILY_TOFINO2: {
      bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
      lld_sku_get_chip_part_revision_number(dev, &rev);
      log2phy = rev != BF_SKU_CHIP_PART_REV_A0;
      s = pipe_mgr_mc_pipe_msk_get(dev, mgid >> 2, pipe, &masks);
      r = (masks >> (5 * (mgid & 0x3))) & 0x1F;
      break;
    }
    default:
      s = PIPE_UNEXPECTED;
      MC_MGR_DBGCHK(0);
  }
  if (s != PIPE_SUCCESS) return r;

  /* If we had programmed a logical pipe vector convert it back to physical
   * here since that is what the caller expects. */
  if (log2phy) {
    uint32_t phy_mask = 0;
    for (int i = 0; mc_mgr_ctx_num_max_pipes(dev); ++i) {
      if (~r & (1u << i)) continue;
      bf_dev_pipe_t phy_pipe = i;
      lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &phy_pipe);
      phy_mask |= 1u << phy_pipe;
    }
    /* Preserve the 5th bit. */
    r = (r & 0x10) | phy_mask;
  }

  return r;
}

uint8_t mc_mgr_get_tvt_hw(bf_dev_id_t dev, int pipe, int mgid) {
  uint32_t masks = 0;
  uint8_t r = 0;
  pipe_status_t s;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO3: {
      s = pipe_mgr_mc_tvt_msk_get(dev, mgid >> 2, pipe, &masks);
      r = (masks >> (4 * (mgid & 0x3))) & 0xF;
      break;
    }
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    default:
      s = PIPE_UNEXPECTED;
      MC_MGR_DBGCHK(0);
  }

  if (s != PIPE_SUCCESS)
    return 0;
  else
    return r;
}

bf_status_t mc_mgr_c2c_pipe_msk_get_hw(bf_dev_id_t dev,
                                       int pipe,
                                       uint32_t *mask) {
  bf_status_t s = pipe_mgr_mc_c2c_pipe_msk_get(dev, pipe, mask);
  bool log2phy = false;
  bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
  lld_sku_get_chip_part_revision_number(dev, &rev);
  log2phy = mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO2 &&
            rev != BF_SKU_CHIP_PART_REV_A0;

  /* If we had programmed a logical pipe vector convert it back to physical
   * here since that is what the caller expects. */
  if (log2phy) {
    uint32_t phy_mask = 0;
    for (unsigned int i = 0; i < mc_mgr_ctx_num_max_pipes(dev); ++i) {
      if (~*mask & (1u << i)) continue;
      bf_dev_pipe_t phy_pipe = i;
      lld_sku_map_pipe_id_to_phy_pipe_id(dev, i, &phy_pipe);
      phy_mask |= 1u << phy_pipe;
    }
    *mask = phy_mask;
  }

  return s;
}

bool mc_mgr_get_tbl_ver(bf_dev_id_t dev) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    return false;
  }
  return mc_mgr_ctx_tbl_ver(dev);
}

bf_status_t mc_mgr_wait_for_ver_drain(bf_dev_id_t dev, int ver) {
  if (0 > dev || MC_MGR_NUM_DEVICES <= dev) {
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    return BF_INVALID_ARG;
  }
  bf_status_t sts = BF_SUCCESS;

  int pipe;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    uint32_t cnt = 1;
    for (int i = 0; i < 5000 && cnt; ++i) {
      sts = mc_mgr_get_ver_cnt_reg(dev, pipe, ver, &cnt);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        return sts;
      }
    }
    if (cnt) {
      LOG_WARN("Phy pipe %d had count of %u during %s", pipe, cnt, __func__);
    }
  }
  return BF_SUCCESS;
}

void mc_mgr_collect_l1s_l2_chain_addrs(bf_dev_id_t dev,
                                       uint32_t l1_rdm_addr,
                                       mc_mgr_rdm_addr_list_t **addrs) {
  mc_mgr_rdm_t *rdm_map = mc_mgr_ctx_rdm_map(dev);
  if (!node_is_l1(mc_mgr_rdm_type(rdm_map, l1_rdm_addr))) {
    MC_MGR_DBGCHK(0);
    return;
  }
  uint32_t cur_l2_addr = 0;
  cur_l2_addr = mc_mgr_rdm_l1_node_get_l2_ptr(rdm_map, l1_rdm_addr);
  while (cur_l2_addr) {
    mc_mgr_rdm_addr_append(addrs, cur_l2_addr);
    cur_l2_addr = mc_mgr_rdm_next_l2(rdm_map, cur_l2_addr, true);
  }
}

void mc_mgr_delete_l1s_l2_chain(int sid,
                                bf_dev_id_t dev,
                                uint32_t l1_rdm_addr) {
  mc_mgr_rdm_addr_list_t *to_free = NULL;
  mc_mgr_collect_l1s_l2_chain_addrs(dev, l1_rdm_addr, &to_free);
  while (to_free) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_free, &x);
    mc_mgr_rdm_map_enqueue_free(sid, dev, x);
  }
  mc_mgr_rdm_update_next_l2(sid, dev, l1_rdm_addr, 0);
}

void mc_mgr_delete_l1_node_from_pipe(int sid,
                                     mc_l1_node_t *node,
                                     int pipe,
                                     mc_mgr_rdm_addr_list_t **to_free) {
  if (node == NULL) {
    MC_MGR_DBGCHK(0);
    return;
  }

  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(node->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }

  /* Not safe for ECMP member nodes as this function update the MIT. */
  if (node->ecmp_grp && node->mgid == -1) {
    MC_MGR_DBGCHK(0);
    return;
  }

  mc_l1_hw_node_t *n = &node->hw_nodes[pipe];
  if (!n->rdm_addr) {
    MC_MGR_DBGCHK(0);
    return;
  }

  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  // Clean up L2 nodes (add to free list).
  mc_mgr_collect_l1s_l2_chain_addrs(dev, n->rdm_addr, to_free);

  // Unlink L1 node
  mc_l1_hw_node_t *pn = n->prev;
  if (pn == NULL) {  // Even a list of 1 element has a previous
    MC_MGR_DBGCHK(0);
    return;
  }

  if (pn->next) { /* Not head of list */
    MC_MGR_DBGCHK(n->rdm_addr ==
                  mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), pn->rdm_addr));
  }
  bool first = mc_mgr_get_mit(dev, pipe, mgid) == n->rdm_addr;
  uint32_t next_l1 = mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), n->rdm_addr);

  /* Unlink it from the list of nodes on the mgid. */
  unlink_l1(n, dev, pipe, mgid);

  if (!first) {
    mc_mgr_rdm_update_next_l1(sid, dev, pn->rdm_addr, next_l1);
  } else {
    /* If first node was removed, update the MIT. */
    if (BF_SUCCESS != mc_mgr_set_mit_wrl(sid, dev, pipe, mgid)) {
      MC_MGR_DBGCHK(0);
      return;
    }
  }

  LOG_DBG("Freed L1-RDM:%#x pipe:%d node_handle:%#x",
          n->rdm_addr,
          pipe,
          node->handle);

  /* Add the L1 node's address to the list of addresses to free. */
  mc_mgr_rdm_addr_append(to_free, n->rdm_addr);
  n->rdm_addr = 0;
  /* Update sw object for the L1. */
  mc_mgr_delete_l1(n);
  node->l2_count[pipe] = 0;
}

static void mc_mgr_delete_tree(int sid, mc_l1_node_t *node, int pipe) {
  if (node == NULL) {
    MC_MGR_DBGCHK(0);
    return;
  }

  if (pipe < 0 || pipe >= (int)mc_mgr_ctx_num_max_pipes(node->dev)) {
    MC_MGR_DBGCHK(0);
    return;
  }

  /* Not safe for ECMP nodes as this function update the MIT. */
  if (node->ecmp_grp) {
    MC_MGR_DBGCHK(0);
    return;
  }

  mc_l1_hw_node_t *n = &node->hw_nodes[pipe];
  if (!n->rdm_addr) {
    MC_MGR_DBGCHK(0);
    return;
  }

  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  // Clean up L2 nodes (add to free list).
  mc_mgr_delete_l1s_l2_chain(sid, dev, n->rdm_addr);

  // Unlink L1 node
  mc_l1_hw_node_t *pn = n->prev;
  if (pn == NULL) {  // Even a list of 1 element has a previous
    MC_MGR_DBGCHK(0);
    return;
  }

  if (pn->next) { /* Not head of list */
    MC_MGR_DBGCHK(n->rdm_addr ==
                  mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), pn->rdm_addr));
  }
  bool first = mc_mgr_get_mit(dev, pipe, mgid) == n->rdm_addr;
  uint32_t next_l1 = mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), n->rdm_addr);

  /* Unlink it from the list of nodes on the mgid. */
  unlink_l1(n, dev, pipe, mgid);

  if (!first) {
    mc_mgr_rdm_update_next_l1(sid, dev, pn->rdm_addr, next_l1);
  } else {
    /* If first node was removed, update the MIT. */
    if (BF_SUCCESS != mc_mgr_set_mit_wrl(sid, dev, pipe, mgid)) {
      MC_MGR_DBGCHK(0);
      return;
    }
  }

  /* Start the RDM delete. */
  mc_mgr_rdm_map_enqueue_free(sid, dev, n->rdm_addr);
  n->rdm_addr = 0;
  /* Update sw object allocated for it. */
  mc_mgr_delete_l1(n);
}

static void mc_mgr_delete_all_trees(int sid, mc_l1_node_t *node) {
  int pipe;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++pipe) {
    if (node->hw_nodes[pipe].rdm_addr) {
      mc_mgr_delete_tree(sid, node, pipe);
      MC_MGR_DBGCHK(!node->hw_nodes[pipe].rdm_addr);
    }
  }
}

void mc_mgr_get_l2_chain_sz(int pipe,
                            mc_l1_node_t *node,
                            int *port_cnt,
                            int *lag_cnt) {
  /* Iterate through every lag index and check if the current pipe
   * bit is set. */
  bf_bitset_t lags;
  bf_bs_init(&lags, BF_LAG_COUNT, node->l2_chain.lags);
  int lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&lags, lag_id))) {
    if ((1 << pipe) & mc_mgr_ctx_lag2pipe_mask(node->dev, lag_id)) {
      (*lag_cnt)++;
    }
  }

  /* Return the port count from the current pipe.  */
  int pipe_mask = calculate_port_pipe_mask_per_pipe(pipe, node);
  *port_cnt = __builtin_popcount(pipe_mask);
}

void mc_mgr_get_port_node_size(bf_dev_id_t dev,
                               bf_dev_pipe_t pipe,
                               mc_l1_node_t *node,
                               int *cnt,
                               uint8_t *mask) {
  uint8_t x = calculate_port_pipe_mask_per_pipe(pipe, node);
  if (mc_mgr_ctx_num_max_pipes(dev) != BF_SUBDEV_PIPE_COUNT) {
    int i = 0, node_cnt = 0;
    for (i = 0; i < (int)mc_mgr_ctx_num_max_pipes(dev); i++) {
      if (x & (0x1u << i)) {
        node_cnt++;
      }
    }
    if (cnt) *cnt = node_cnt;
  } else {
    MC_MGR_DBGCHK(0 == (x & 0xF0));
    int node_cnt[16] = {
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4}; /* popcount lookup
                                                            table. */
    if (cnt) *cnt = node_cnt[x];
  }
  if (mask) *mask = x;
}

int mc_mgr_write_l2_port_node(int sid,
                              mc_l1_node_t *node,
                              uint32_t rdm_addr,
                              uint8_t mask) {
  bf_dev_id_t dev = node->dev;

  int node_count = 0;
  int x, y = 0;
  for (x = 0; x < (int)mc_mgr_ctx_num_max_pipes(dev); ++x) {
    if (~mask & (1 << x)) continue;
    mask = mask & ~(1 << x);
    bf_bitset_t ports;
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[x]);
    mc_mgr_rdm_write_port72(sid, dev, rdm_addr + y, x, !mask, &ports);
    y += 2;
    ++node_count;
  }
  return node_count;
}

bf_status_t mc_mgr_allocate_l2_chain(int sid,
                                     bf_dev_id_t dev,
                                     int pipe,
                                     mc_l1_node_t *node,
                                     uint32_t *l2_length,
                                     mc_mgr_rdm_addr_list_t **port_addrs,
                                     mc_mgr_rdm_addr_list_t **lag_addrs) {
  int port_sz = 0;
  int lag_cnt = 0;
  int port_addr_cnt = 0;
  int lag_addr_cnt = 0;
  int i;

  if (!port_addrs) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (!lag_addrs) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  mc_mgr_get_l2_chain_sz(pipe, node, &port_sz, &lag_cnt);
  *l2_length = port_sz + lag_cnt;

  if (port_sz) {
    uint32_t x = mc_mgr_rdm_map_get(
        sid, node->dev, pipe, mc_mgr_rdm_node_type_port72, port_sz);
    if (!x) goto not_enough_nodes;
    ++port_addr_cnt;
    mc_mgr_rdm_addr_append(port_addrs, x);
  }
  for (i = 0; i < lag_cnt; ++i) {
    uint32_t x =
        mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_lag, 1);
    if (!x) goto not_enough_nodes;
    ++lag_addr_cnt;
    mc_mgr_rdm_addr_append(lag_addrs, x);
  }

  LOG_TRACE("Allocated L2 Ports dev:%d port_node_sz:%d lag_cnt:%d pipe:%d %#x",
            dev,
            port_sz,
            lag_cnt,
            pipe,
            node->handle);

  return BF_SUCCESS;

not_enough_nodes:
  for (i = 0; i < port_addr_cnt; ++i) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(port_addrs, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  for (i = 0; i < lag_addr_cnt; ++i) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(lag_addrs, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  *l2_length = 0;
  return BF_NO_SYS_RESOURCES;
}

static bf_status_t write_l2_chain(int sid,
                                  int pipe,
                                  mc_l1_node_t *node,
                                  mc_mgr_rdm_addr_list_t **lag_addrs,
                                  mc_mgr_rdm_addr_list_t **port_addrs,
                                  uint32_t *first_l2) {
  bf_dev_id_t dev = node->dev;
  bf_bitset_t lags;
  bf_bs_init(&lags, BF_LAG_COUNT, node->l2_chain.lags);

  int l2_length = 0;
  uint8_t port_mask;
  int node_block_size;
  mc_mgr_get_port_node_size(dev, pipe, node, &node_block_size, &port_mask);
  bool has_ports = port_mask != 0;

  *first_l2 = 0;
  uint32_t cur_node_addr = 0;
  uint32_t prev_node_addr = 0;

  /* Write the port node first; either the LAG nodes or L1 node can
   * point to it. */
  if (has_ports) {
    if (!port_mask || !node_block_size || !(*port_addrs)) {
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
    }
    mc_mgr_rdm_addr_pop(port_addrs, &cur_node_addr);
    if (!cur_node_addr) {
      /* Lost track of how many RDM nodes were needed! */
      MC_MGR_DBGCHK(cur_node_addr);
      return BF_NO_SYS_RESOURCES;
    }
    l2_length += mc_mgr_write_l2_port_node(sid, node, cur_node_addr, port_mask);
    prev_node_addr = cur_node_addr;
    cur_node_addr = 0;
  }

  LOG_TRACE(
      "Written L2 Port node dev:%d port_node_sz:%d pipe:%d node_handle:%#x",
      dev,
      l2_length,
      pipe,
      node->handle);

  /* Write a LAG node for each LAG the node has. */
  int lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&lags, lag_id))) {
    if ((1 << pipe) & mc_mgr_ctx_lag2pipe_mask(dev, lag_id)) {
      mc_mgr_rdm_addr_pop(lag_addrs, &cur_node_addr);
      if (!cur_node_addr) {
        /* Lost track of how many RDM nodes were needed! */
        MC_MGR_DBGCHK(cur_node_addr);
        return BF_NO_SYS_RESOURCES;
      }
      mc_mgr_rdm_write_lag(sid, dev, cur_node_addr, lag_id, prev_node_addr);
      prev_node_addr = cur_node_addr;
      cur_node_addr = 0;
      ++l2_length;
      LOG_TRACE("Written L2 Lag node dev:%d lad_id:%d pipe:%d node_handle:%#x",
                dev,
                lag_id,
                pipe,
                node->handle);
    }
  }

  *first_l2 = prev_node_addr;
  node->l2_count[pipe] = l2_length;
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_l1_tail(
    int sid, bf_dev_id_t dev, int pipe, int mgid, int len) {
  struct mc_mgr_tail_info *ti = mc_mgr_ctx_tail_info(dev);
  if (!ti->num_tails) {
    /* No tail info for this device, tree balancing is not required (e.g. this
     * is not Tofino-1). */
    return BF_SUCCESS;
  }
  mc_l1_hw_node_t *head = mc_mgr_ctx_tree(dev, pipe, mgid);
  /* For trees of length zero don't bother. */
  if (!head) return BF_SUCCESS;
  uint32_t last_node_rdm_addr = head->prev->rdm_addr;
  /* For trees of length one or depth one don't bother, but clear any tail
   * that was set. */
  if (head->prev == head || len <= 1) {
    if (mc_mgr_rdm_get_next_l1(dev, last_node_rdm_addr)) {
      mc_mgr_rdm_update_next_l1(sid, dev, last_node_rdm_addr, 0);
    }
    return BF_SUCCESS;
  }
  unsigned int i;
  for (i = 0; i < ti->num_tails - 1; ++i) {
    if (ti->tail_size[i] >= len) break;
  }
  if (len > ti->tail_size[i]) {
    LOG_ERROR("Dev %d pipe %d mgid 0x%x using short tail %d instead of %d",
              dev,
              pipe,
              mgid,
              len,
              ti->tail_size[i]);
  }
  mc_mgr_rdm_update_next_l1(
      sid, dev, last_node_rdm_addr, ti->tail_base[pipe] + i);
  return BF_SUCCESS;
}

bool mc_mgr_update_len(int sid, bf_dev_id_t dev, int pipe, int mgid, int len) {
  struct mc_mgr_tree_size_t *t = NULL;
  bool result = true;
  int subdev = 0;
  for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev); subdev++) {
    mc_mgr_ctx_tree_len_lock(dev, subdev);
    bf_map_sts_t s =
        bf_map_get(mc_mgr_ctx_tree_len(dev, subdev), mgid, (void *)&t);
    if (BF_MAP_OK != s) {
      mc_mgr_ctx_tree_len_unlock(dev, subdev);
      MC_MGR_DBGCHK(BF_MAP_OK == s);
      result &= false;
      continue;
    }
    if (!t) {
      mc_mgr_ctx_tree_len_unlock(dev, subdev);
      MC_MGR_DBGCHK(t);
      result &= false;
      continue;
    }
    /* t is an array of pipes, advance to the specified pipe. */
    t += pipe;

    if (len >= t->hw_len) {
      t->hw_len = t->sw_len = len;
      t->buf = NULL;
      mc_mgr_ctx_tree_len_unlock(dev, subdev);
      mc_mgr_set_l1_tail(sid, dev, pipe, mgid, len);
      mc_mgr_ctx_tree_len_lock(dev, subdev);
      mc_mgr_ctx_clear_len_update(sid, dev, subdev, pipe, mgid);
      mc_mgr_ctx_tree_len_unlock(dev, subdev);
      result &= true;
    } else {
      /* Tail is being shortened, store a pending update to be associated with
       * the DMA completion of the buffer which is modifying this tree.
       * However, if the device is locked then there is no traffic and the
       * update can happen immediately. */
      if (mc_mgr_ctx_dev_locked(dev)) {
        t->hw_len = t->sw_len = len;
        t->buf = NULL;
        mc_mgr_ctx_tree_len_unlock(dev, subdev);
        mc_mgr_set_l1_tail(sid, dev, pipe, mgid, len);
        result &= true;
      } else {
        t->sw_len = len;
        t->buf = NULL;
        mc_mgr_ctx_mark_len_update(sid, dev, subdev, pipe, mgid);
        mc_mgr_ctx_tree_len_unlock(dev, subdev);
        result &= true;
      }
    }
  }
  return result;
}
int mc_mgr_compute_longest_l2(bf_dev_id_t dev, int pipe, int mgid) {
  struct mc_l1_hw_node_t *h = mc_mgr_ctx_tree(dev, pipe, mgid);
  uint32_t l2_len = 0;
  while (h) {
    /* In case of an ECMP each member must be checked. */
    if (h->sw_node->ecmp_grp) {
      if (!h->sw_node->ecmp_grp) {
        /* The group is empty so the L2 length is zero. */
      } else {
        for (int m = 0; m < MC_ECMP_GROUP_MAX_MBR; ++m) {
          if (h->sw_node->ecmp_grp->valid_map & (1u << m)) {
            if (l2_len < h->sw_node->ecmp_grp->mbrs[m]->l2_count[pipe]) {
              l2_len = h->sw_node->ecmp_grp->mbrs[m]->l2_count[pipe];
            }
          }
        }
      }
    } else {
      if (l2_len < h->sw_node->l2_count[pipe]) {
        l2_len = h->sw_node->l2_count[pipe];
      }
    }
    h = h->next;
  }
  return l2_len;
}

bf_status_t mc_mgr_l1_remove(int sid, mc_l1_node_t *node) {
  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  mc_mgr_delete_all_trees(sid, node);
  node->mgid = -1;

  for (int pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    /* Get the length of the longest L2 chain in the pipe. */
    uint32_t l2_len = mc_mgr_compute_longest_l2(dev, pipe, mgid);
    mc_mgr_update_len(sid, dev, pipe, mgid, l2_len);
  }

  return BF_SUCCESS;
}

bf_status_t mc_mgr_ecmp_ptr_remove(int sid, mc_l1_node_t *node) {
  if (node == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (node->ecmp_grp == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  /* Find the pipe mask for the L1-Ecmp node. */
  uint16_t curr_mask = mc_mgr_current_pipe_mask(node);

  /* The ecmp group mask and the L1-ECMP node pipe mask should match. */
  MC_MGR_DBGCHK(curr_mask == mc_mgr_current_ecmp_pipe_mask(node->ecmp_grp));

  int pipe;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    uint16_t pipe_mask = 1 << pipe;
    if (!(pipe_mask & curr_mask)) continue;

    mc_l1_hw_node_t *n = &node->hw_nodes[pipe];
    if (!n->rdm_addr) {
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
    }

    // Unlink L1 node
    mc_l1_hw_node_t *pn = n->prev;
    if (pn == NULL) {  // Even a list of 1 element has a previous.
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
    }
    if (pn->next) { /* Not head of list */
      MC_MGR_DBGCHK(n->rdm_addr ==
                    mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), pn->rdm_addr));
    }
    bool first = mc_mgr_get_mit(dev, pipe, mgid) == n->rdm_addr;
    uint32_t next_l1 = mc_mgr_rdm_next_l1(mc_mgr_ctx_rdm_map(dev), n->rdm_addr);

    /* Unlink it from the list of nodes on the mgid. */
    unlink_l1(n, dev, pipe, mgid);

    if (!first) {
      mc_mgr_rdm_update_next_l1(sid, dev, pn->rdm_addr, next_l1);
    } else {
      /* If first node was removed, update the MIT. */
      bf_status_t sts = mc_mgr_set_mit_wrl(sid, dev, pipe, mgid);
      if (BF_SUCCESS != sts) return sts;
    }

    /* Start the RDM delete. */
    mc_mgr_rdm_map_enqueue_free(sid, dev, n->rdm_addr);
    n->rdm_addr = 0;
    /* Update sw object allocated for it. */
    mc_mgr_delete_l1(n);
  }

  return BF_SUCCESS;
}

/* Inserts an ECMP pointer node into a tree. */
bf_status_t mc_mgr_ecmp_ptr_write(int sid, mc_l1_node_t *node) {
  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;
  int p;

  mc_ecmp_grp_t *grp = node->ecmp_grp;
  if (!grp) {
    LOG_ERROR("Dev %d ecmp_grp is NULL for node_hdl:%x", dev, node->handle);
    return BF_INVALID_ARG;
  }

  /* Find the pipe mask where ECMP group has associated L1-End block. */
  uint16_t curr_mask = mc_mgr_current_ecmp_pipe_mask(grp);

  /* Allocate L1-End nodes where pipe_mask != 0. */
  uint32_t rdm_addrs[MC_MGR_NUM_PIPES] = {0};
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(node->dev); ++p) {
    uint16_t pipe_mask = 1 << p;
    if (!(pipe_mask & curr_mask)) continue;

    rdm_addrs[p] =
        mc_mgr_rdm_map_get(sid, dev, p, mc_mgr_rdm_node_type_ecmp, 1);
    if (!rdm_addrs[p]) {
      goto not_enough_nodes;
    }
  }

  /* Write L1-ECMP address with the vector node addresses where the
   * group has L1-End vector. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pipe_mask = 1 << p;
    if (!(pipe_mask & curr_mask)) continue;

    /* Allocate a new L1 node. */
    get_new_l1(node, p, rdm_addrs[p]);
    if (!node->hw_nodes[p].rdm_addr) {
      MC_MGR_DBGCHK(node->hw_nodes[p].rdm_addr);
      return BF_NO_SYS_RESOURCES;
    }

    /* Write the node to the RDM at the head of the L1 list. */
    uint32_t next_l1_addr = mc_mgr_get_mit(dev, p, mgid);
    uint32_t p0 = node->ecmp_grp->vector_node_addr[0][p];
    uint32_t p1 = node->ecmp_grp->vector_node_addr[1][p];
    mc_mgr_rdm_write_ptr(sid,
                         dev,
                         node->hw_nodes[p].rdm_addr,
                         p0,
                         p1,
                         next_l1_addr,
                         node->xid,
                         node->xid_valid);

    /* Link the L1 node to the MIT chain (updates SW structures only). */
    link_l1(&node->hw_nodes[p], dev, p, mgid);

    /* Get the size of this tree and update the tail if needed. */
    int l2_len = mc_mgr_compute_longest_l2(dev, p, mgid);
    mc_mgr_update_len(sid, dev, p, mgid, l2_len);

    /* Program the MIT. */
    if (BF_SUCCESS != mc_mgr_set_mit_wrl(sid, dev, p, mgid)) {
      return BF_NO_SYS_RESOURCES;
    }
  }

  return BF_SUCCESS;

not_enough_nodes:
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (rdm_addrs[p]) {
      mc_mgr_rdm_map_return(dev, rdm_addrs[p]);
    }
  }
  return BF_NO_SYS_RESOURCES;
}

void mc_mgr_write_l1_rid_node(int sid,
                              int pipe,
                              mc_l1_node_t *node,
                              uint32_t l1_rdm_addr,
                              uint32_t l2_rdm_addr) {
  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  /* Setup new L1 node. */
  get_new_l1(node, pipe, l1_rdm_addr);

  /* Write the node to the RDM at the head of the L1 list. */
  uint32_t next_l1_addr = mc_mgr_get_mit(dev, pipe, mgid);
  mc_mgr_rdm_write_l1(sid,
                      dev,
                      node->hw_nodes[pipe].rdm_addr,
                      next_l1_addr,
                      l2_rdm_addr,
                      node->rid,
                      node->xid,
                      node->xid_valid);

  /* Link the L1 node to the MIT chain, but no packets will see this node since
   * it is inserted at the head and the MIT has not yet been updated. */
  link_l1(&node->hw_nodes[pipe], dev, pipe, mgid);
  /* Terminate the tree (in this pipe) with an appropriately sized tail before
   * the MIT is updated. */
  uint32_t l2_len = mc_mgr_compute_longest_l2(dev, pipe, mgid);
  mc_mgr_update_len(sid, dev, pipe, mgid, l2_len);
  /* Program the MIT. */
  (void)mc_mgr_set_mit_wrl(sid, dev, pipe, mgid);
}

bf_status_t mc_mgr_ecmp_first_mbr_add(int sid,
                                      bf_dev_id_t dev,
                                      mc_ecmp_grp_t *g,
                                      mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;

  /* Which pipes the node needs to be in. */
  uint8_t add_mask = mc_mgr_calculate_pipe_mask(node);
  /* Add mask can be both zero and non zero. The add_mask == 0 is a
   * scenario, the member node has associated LAGs where LAGs have
   * no member ports. */
  MC_MGR_DBGCHK(add_mask || !add_mask);

  int used_mgid_count = 0;
  mc_l1_node_t *l1_ecmp_node = g->refs;
  while (l1_ecmp_node) {
    used_mgid_count += 1;
    l1_ecmp_node = l1_ecmp_node->ecmp_next;
  }

  /* Allocate placeholder to hold the MGIDs that the grp is associated with.*/
  int *mgids = MC_MGR_CALLOC(used_mgid_count, sizeof(int));
  if (!mgids) goto not_enough_memory_for_mgdids;

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int m;
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    mgids[m] = l1_ecmp_node->mgid;
  }

  /* Allocate placeholder to hold L1-ECMP RDM memories */
  mc_mgr_rdm_addr_list_t **to_write_l1_ecmp = MC_MGR_CALLOC(
      used_mgid_count * MC_MGR_NUM_PIPES, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write_l1_ecmp) goto not_enough_memory_to_write_l1_ecmp;

  /* Allocate placeholder to hold L1-End, L2-Port and Lag RDM memories */
  mc_mgr_rdm_addr_list_t **to_write =
      MC_MGR_CALLOC(MC_MGR_NUM_PIPES * 3, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write) goto not_enough_memory_to_write;

  /* Allocate an array of mgid_counts to hold the max l2 length. */
  uint32_t *l2_len = MC_MGR_CALLOC(MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!l2_len) goto not_enough_memory_l2_len;

  int idx_l1 = 0;  /* Index to_write for L1 ecmp addresses. */
  int idx_lag = 1; /* Index to_write for lag addresses. */
  int idx_l2 = 2;  /* Index to_write for port addresses. */
  int p;

  /* Iterate through MGIDs and allocate L1-ECMP nodes for all MGIDs that the
   * ecmp is associated with */
  for (m = 0; m < used_mgid_count; m++) {
    /* Get L1-ECMP nodes for pipes where add_mask != 0. This allocates
     * nodes only for a single mgid.*/
    sts = mc_mgr_get_l1_ecmps(sid, dev, add_mask, M1(to_write_l1_ecmp, m));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Allocate nodes to hold the L2 chain in each pipe. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(pm & add_mask)) continue;
    sts = mc_mgr_allocate_l2_chain(sid,
                                   dev,
                                   p,
                                   node,
                                   M2(l2_len, p),
                                   M3(to_write, idx_l2, p),
                                   M3(to_write, idx_lag, p));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Allocate L1-End block node for pipes where add_mask !=0. As a ecmp_grp
   * can be associated to more than one MGID, iterating through all pipes
   * should be sufficient.*/
  sts = mc_mgr_get_ecmp_blocks(sid, dev, 1, add_mask, M2(to_write, idx_l1));
  if (BF_SUCCESS != sts) goto not_enough_nodes;

  /* Assign the node to the group as its memeber. */
  g->mbrs[0] = node;
  g->valid_map = 1;

  /* Add the ecmp members to the pipe where add_mask != 0. */
  sts = mc_mgr_ecmp_add_block_n_one_mbr(sid,
                                        dev,
                                        0,
                                        add_mask,
                                        node,
                                        g,
                                        M1(to_write, idx_l1),
                                        M1(to_write, idx_l2),
                                        M1(to_write, idx_lag));
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to add mbr_hdl:%x to the group:%x with add_mask:%x",
              node->handle,
              g->handle,
              add_mask);
    goto not_enough_nodes;
  }

  /* The ECMP group has not been associated with any L1-ECMP node,
   * hence we can update the vector map w/o updating PVT/TVT. */

  /* Update the standby vector. */
  mc_mgr_write_stby_vector(sid, dev, g, 1, g->base);
  /* Update the active vector. */
  mc_mgr_write_actv_vector(sid, dev, g, 1, g->base);

  g->allocated_sz = 1;
  node->ecmp_grp = g;

  /* Iterate through each MGID and associate the group to each MGID L1 chain
   * where add_mask != 0. As vector nodes are already updated, if there
   * is an exisiting traffic in new_pipes, post addition of L1-ECMP nodes,
   * traffic will see the new member node. So we will never run into the
   * HW constrainst where L1-ECMP pointer if valid, but no valid L1-END
   * addresses. */
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    MC_MGR_DBGCHK(mgids[m] == l1_ecmp_node->mgid);
    /* Associate the new L1  */
    sts = mc_mgr_write_one_ecmp_mbr_l1_ecmps(
        sid, dev, add_mask, l1_ecmp_node, g, M1(to_write_l1_ecmp, m));
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to associate mbr_hdl:0x%x to the group:0x%x with "
          "add_mask:%x",
          node->handle,
          g->handle,
          add_mask);
      MC_MGR_DBGCHK(0);
      goto not_enough_nodes;
    }
  }

  /* As ecmp grp may got associated to l1-ecmp nodes on some pipes, we update
   * the TVT/PVT table. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);
    }
  }

  /* Send all pending RDM writes to HW. */
  sts = mc_mgr_drv_wrl_send(sid, true);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
  }

  MC_MGR_FREE(l2_len);
  MC_MGR_FREE(to_write);
  MC_MGR_FREE(to_write_l1_ecmp);
  MC_MGR_FREE(mgids);
  return sts;
not_enough_nodes:
  for (m = 0; m < used_mgid_count; m++) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      while (*M3(to_write_l1_ecmp, m, p)) {
        uint32_t x = 0;
        mc_mgr_rdm_addr_pop(M3(to_write_l1_ecmp, m, p), &x);
        mc_mgr_rdm_map_return(dev, x);
      }
    }
  }

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (*M3(to_write, idx_l1, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l1, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_lag, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_lag, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_l2, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l2, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }

  MC_MGR_FREE(l2_len);
not_enough_memory_l2_len:
  MC_MGR_FREE(to_write);
not_enough_memory_to_write:
  MC_MGR_FREE(to_write_l1_ecmp);
not_enough_memory_to_write_l1_ecmp:
  MC_MGR_FREE(mgids);
not_enough_memory_for_mgdids:
  return BF_NO_SYS_RESOURCES;
}

bf_status_t mc_mgr_ecmp_mbr_add_to_hole(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g,
                                        mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;

  /* Which pipes the node needs to be in. */
  uint8_t pipe_mask = mc_mgr_calculate_pipe_mask(node);
  /* Find the pipe mask where ECMP group has associated L1-End nodes. */
  uint16_t curr_mask = mc_mgr_current_ecmp_pipe_mask(g);
  /* Add L1-ECMP node to these pipes. */
  uint8_t add_mask = ~curr_mask & pipe_mask;
  /* Update node in these pipes. */
  int8_t upd_mask = curr_mask & pipe_mask;

  /* Proceed ahead even if the add_mask or upd_mask is zero. The scenario
   * occurs if the node has no LAG ports or simple ports. */
  LOG_TRACE("%s pipe_mask:%d curr_mask:%d add_mask:%d upd_mask:%d",
            __func__,
            pipe_mask,
            curr_mask,
            add_mask,
            upd_mask);

  int used_mgid_count = 0;
  mc_l1_node_t *l1_ecmp_node = g->refs;
  while (l1_ecmp_node) {
    used_mgid_count += 1;
    l1_ecmp_node = l1_ecmp_node->ecmp_next;
  }

  int *mgids = MC_MGR_CALLOC(used_mgid_count, sizeof(int));
  if (!mgids) goto not_enough_memory_for_mgdids;

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int m;
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    mgids[m] = l1_ecmp_node->mgid;
  }

  /* Allocate placeholder to hold L1-ECMP RDM memories */
  mc_mgr_rdm_addr_list_t **to_write_l1_ecmp = MC_MGR_CALLOC(
      used_mgid_count * MC_MGR_NUM_PIPES, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write_l1_ecmp) goto not_enough_memory_to_write_l1_ecmp;

  /* Allocate placeholder to hold L1-End, L2-Port and Lag RDM memories */
  mc_mgr_rdm_addr_list_t **to_write =
      MC_MGR_CALLOC(MC_MGR_NUM_PIPES * 3, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write) goto not_enough_memory_to_write;

  /* Allocate an array of mgid_counts to hold the max l2 length. */
  uint32_t *l2_len = MC_MGR_CALLOC(MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!l2_len) goto not_enough_memory_l2_len;

  int idx_l1 = 0;  /* Index to_write for L1 ecmp addresses. */
  int idx_lag = 1; /* Index to_write for lag addresses. */
  int idx_l2 = 2;  /* Index to_write for port addresses. */
  int p;

  /* Check for any holes in the existing allocation. */
  int offset;
  for (offset = 0; offset < g->allocated_sz && g->mbrs[offset]; ++offset)
    ;

  /* Iterate through MGIDs and allocate L1-ECMP nodes for all MGIDs that the
   * ecmp is associated with and allocate nodes where the add_mask != 0. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_get_l1_ecmps(sid, dev, add_mask, M1(to_write_l1_ecmp, m));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Allocate nodes to hold the L2 chain in each pipe. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(pm & add_mask) && !(pm & upd_mask)) continue;
    sts = mc_mgr_allocate_l2_chain(sid,
                                   dev,
                                   p,
                                   node,
                                   M2(l2_len, p),
                                   M3(to_write, idx_l2, p),
                                   M3(to_write, idx_lag, p));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Allocate L1-End array node for pipes where add_mask !=0. As a ecmp_grp
   * can be associated to more than one MGID, iterating through all pipes
   * should be sufficient.*/
  sts = mc_mgr_get_ecmp_blocks(
      sid, dev, g->allocated_sz, add_mask, M2(to_write, idx_l1));
  if (BF_SUCCESS != sts) goto not_enough_nodes;

  /* Write the ecmp member to the pipe where add_mask != 0. This also
   * updates the vector nodes with L1-END block address in new pipes.*/
  sts = mc_mgr_ecmp_add_block_n_one_mbr(sid,
                                        dev,
                                        offset,
                                        add_mask,
                                        node,
                                        g,
                                        M1(to_write, idx_l1),
                                        M1(to_write, idx_l2),
                                        M1(to_write, idx_lag));
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to add mbr_hdl:%x to the group:%x with add_mask:%x",
              node->handle,
              g->handle,
              add_mask);
    goto not_enough_nodes;
  }

  /* As the new member is written to the L1-End block, assign the mbr to grp. */
  uint32_t new_vec = g->valid_map | (1u << offset);
  g->mbrs[offset] = node;
  g->valid_map = new_vec;
  node->ecmp_grp = g;

  /* The curr_mask gives the pipe bits for the existing pipes where L2 chain
   * for the new node needs to be added. The added L2 chain will
   * not see any traffic until unless the vector map of the group
   * is updated. */
  sts = mc_mgr_ecmp_upd_one_mbr(sid,
                                dev,
                                offset,
                                curr_mask,
                                node,
                                g,
                                M1(to_write, idx_l2),
                                M1(to_write, idx_lag));

  /* As the group software structures are all updated update the HW tail or
   * rebalance the tree. */
  mc_mgr_ecmp_update_tail_reevaluate(sid, dev, g);

  /* Iterate through each MGID and associate the group to each MGID L1 chain
   * where add_mask != 0. */
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    MC_MGR_DBGCHK(mgids[m] == l1_ecmp_node->mgid);
    /* Associate the new L1  */
    sts = mc_mgr_write_one_ecmp_mbr_l1_ecmps(
        sid, dev, add_mask, l1_ecmp_node, g, M1(to_write_l1_ecmp, m));
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to associate mbr_hdl:0x%x to the group:0x%x with "
          "add_mask:%x",
          node->handle,
          g->handle,
          add_mask);
      goto not_enough_nodes;
    }
  }

  /* HW is still programmed with old vector map. The traffic may
   * reach the new pipe block from new L1-ECMP pointer nodes
   * as vector node is already programmed with new block address
   * and TVT/PVT may be pre-programmed with the presence of
   * other existing L1-RID/XID or L1-ECMP nodes. Hence, traffic
   * if reaches the new pipe blocks, it will always gets hashed
   * to the one of the old members of the group. Now update the
   * vector nodes with the new vector map.
   * */
  mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  /* Update the active vector. */
  if ((g->refs) && (mc_mgr_versioning_on(sid, dev))) {
    /* Switch versions. */
    sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    mc_mgr_drv_cmplt_operations(sid, dev);
    /* Flip table version. */
    bool ver = mc_mgr_ctx_tbl_ver(dev);
    sts = mc_mgr_program_tbl_ver(sid, dev, !ver);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    /* Drain old version. */
    sts = mc_mgr_wait_for_ver_drain(dev, ver);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    /* Write old-active vector to use a new mask. */
    mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  } else {
    /* Write active vector to use a new mask. */
    mc_mgr_write_actv_vector(sid, dev, g, new_vec, g->base);
    sts = mc_mgr_drv_wrl_send(sid, false);
  }

  /* Update the TVT/PVT such that traffic sees new pipes if added.*/
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);
    }
  }

  MC_MGR_FREE(l2_len);
  MC_MGR_FREE(to_write);
  MC_MGR_FREE(to_write_l1_ecmp);
  MC_MGR_FREE(mgids);
  return sts;
not_enough_nodes:
  for (m = 0; m < used_mgid_count; m++) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      while (*M3(to_write_l1_ecmp, m, p)) {
        uint32_t x = 0;
        mc_mgr_rdm_addr_pop(M3(to_write_l1_ecmp, m, p), &x);
        mc_mgr_rdm_map_return(dev, x);
      }
    }
  }

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (*M3(to_write, idx_l1, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l1, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_lag, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_lag, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_l2, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l2, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }

  MC_MGR_FREE(l2_len);
not_enough_memory_l2_len:
  MC_MGR_FREE(to_write);
not_enough_memory_to_write:
  MC_MGR_FREE(to_write_l1_ecmp);
not_enough_memory_to_write_l1_ecmp:
  MC_MGR_FREE(mgids);
not_enough_memory_for_mgdids:
  return BF_NO_SYS_RESOURCES;
}

bf_status_t mc_mgr_ecmp_mbr_add_to_end(int sid,
                                       bf_dev_id_t dev,
                                       mc_ecmp_grp_t *g,
                                       mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;

  /* Which pipes the node needs to be in. */
  uint8_t pipe_mask = mc_mgr_calculate_pipe_mask(node);
  /* Find the pipe mask where ECMP group has associated L1-End nodes. */
  uint16_t curr_mask = mc_mgr_current_ecmp_pipe_mask(g);
  /* Add L1-ECMP node to these pipes. */
  uint8_t add_mask = ~curr_mask & pipe_mask;
  /* Update node in these pipes. */
  uint8_t upd_mask = curr_mask & pipe_mask;

  /* Proceed ahead even if the add_mask or upd_mask is zero. The scenario
   * occurs if the node has no LAG ports or simple ports. */
  LOG_TRACE("%s pipe_mask:%d curr_mask:%d add_mask:%d upd_mask:%d",
            __func__,
            pipe_mask,
            curr_mask,
            add_mask,
            upd_mask);

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int used_mgid_count = 0;
  mc_l1_node_t *l1_ecmp_node = g->refs;
  while (l1_ecmp_node) {
    used_mgid_count += 1;
    l1_ecmp_node = l1_ecmp_node->ecmp_next;
  }

  int *mgids = MC_MGR_CALLOC(used_mgid_count, sizeof(int));
  if (!mgids) goto not_enough_memory_for_mgdids;

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int m;
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    mgids[m] = l1_ecmp_node->mgid;
  }

  /* Allocate placeholder to hold L1-ECMP RDM memories */
  mc_mgr_rdm_addr_list_t **to_write_l1_ecmp = MC_MGR_CALLOC(
      used_mgid_count * MC_MGR_NUM_PIPES, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write_l1_ecmp) goto not_enough_memory_to_write_l1_ecmp;

  /* Allocate placeholder to hold L1-End, L2-Port and Lag RDM memories */
  mc_mgr_rdm_addr_list_t **to_write =
      MC_MGR_CALLOC(MC_MGR_NUM_PIPES * 3, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write) goto not_enough_memory_to_write;

  /* Allocate an array of mgid_counts to hold the max l2 length. */
  uint32_t *l2_len = MC_MGR_CALLOC(MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!l2_len) goto not_enough_memory_l2_len;

  int idx_l1 = 0;  /* Index to_write for L1 ecmp addresses. */
  int idx_lag = 1; /* Index to_write for lag addresses. */
  int idx_l2 = 2;  /* Index to_write for port addresses. */
  int p;

  /* Iterate through MGIDs and allocate L1-ECMP nodes for all MGIDs that the
   * ecmp is associated with and allocate nodes where the add_mask != 0. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_get_l1_ecmps(sid, dev, add_mask, M1(to_write_l1_ecmp, m));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Allocate nodes to hold the L2 chain in each pipe. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(pm & add_mask) && !(pm & upd_mask)) continue;
    sts = mc_mgr_allocate_l2_chain(sid,
                                   dev,
                                   p,
                                   node,
                                   M2(l2_len, p),
                                   M3(to_write, idx_l2, p),
                                   M3(to_write, idx_lag, p));
    if (BF_SUCCESS != sts) goto not_enough_nodes;
  }

  /* Add the L1+L2 at the RDM address of base+x. */
  uint32_t new_vec = g->valid_map | (1u << g->allocated_sz);
  int new_sz = g->allocated_sz + 1;

  /* Allocate L1-End blocks for pipes where (add_mask | curr_mask)!=0.
   * We are OR'ing curr_mask with add mask such that we can copy the
   * existing L2 chains from old blocks to new blocks.
   * */
  sts = mc_mgr_get_ecmp_blocks(
      sid, dev, new_sz, add_mask | curr_mask, M2(to_write, idx_l1));
  if (BF_SUCCESS != sts) goto not_enough_nodes;

  uint32_t old_base[MC_MGR_NUM_PIPES] = {0};

  /* Saves the backup of old block address in "old_base" to be freed
   * later. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    old_base[p] = g->base[p];
  }

  /* Move the existing members node L2 chains from old L1-End block
   * to the new L1-End allocated blocks. In-flight traffic still follows
   * the old blocks as vector nodes have not been updated with addresses
   * of new blocks. Through the below copy API we are making the new
   * L1-End block hold another references to the existing pipes L2 chains.
   */
  mc_mgr_move_mbr_block(sid, dev, g, M1(to_write, idx_l1));

  /* Add the ecmp members to the pipe where add_mask != 0. This also
   * updates the vector nodes with L1-END block address in new pipes
   * but with an older vector map. */
  sts = mc_mgr_ecmp_add_block_n_one_mbr(sid,
                                        dev,
                                        new_sz - 1,
                                        add_mask,
                                        node,
                                        g,
                                        M1(to_write, idx_l1),
                                        M1(to_write, idx_l2),
                                        M1(to_write, idx_lag));

  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to add mbr_hdl:%x to the group:%x with add_mask:%x",
              node->handle,
              g->handle,
              add_mask);
    goto not_enough_nodes;
  }

  /* As the new member is written to the L1-End block, assign the mbr to grp. */
  g->mbrs[new_sz - 1] = node;
  g->valid_map = new_vec;
  g->allocated_sz = new_sz;
  node->ecmp_grp = g;

  /* Add new member L2 chains for the existing pipes.
   * NOTE: This assigns the L2 chains for the new mbr to the new
   * L1-End blocks. The old L1-END blocks still also points to
   * the L2 pointer other member l2 chains. */
  sts = mc_mgr_ecmp_upd_one_mbr(sid,
                                dev,
                                new_sz - 1,
                                curr_mask,
                                node,
                                g,
                                M1(to_write, idx_l2),
                                M1(to_write, idx_lag));

  /* As the software structures are all updated update the HW tail or
   * rebalance the tree. */
  mc_mgr_ecmp_update_tail_reevaluate(sid, dev, g);

  /* At this point we have modified the group with the following:
   * 1. Added new L1-End blocks for new and existing pipes.
   * 2. Added L2 chain for the new member in new pipes and
   *    copied other members L2 chains for existing pipes
   *    from old blocks to new blocks.
   * 3. Added L2 chains for the new mbr in existing pipes.
   * 4. Change the sw structure to hold the address of the
   *    new blocks.
   */

  /* Iterate through each MGID and associate the group to each MGID L1 chain
   * where add_mask != 0.*/
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    MC_MGR_DBGCHK(mgids[m] == l1_ecmp_node->mgid);
    /* Associate the new L1  */
    sts = mc_mgr_write_one_ecmp_mbr_l1_ecmps(
        sid, dev, add_mask, l1_ecmp_node, g, M1(to_write_l1_ecmp, m));
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "Failed to associate mbr_hdl:0x%x to the group:0x%x with "
          "add_mask:%x",
          node->handle,
          g->handle,
          add_mask);
      goto not_enough_nodes;
    }
  }

  /* HW is still programmed with old vector map. The traffic may
   * reach the new pipe block from new L1-ECMP pointer nodes
   * as vector node is already programmed with new block address
   * and TVT/PVT may be pre-programmed with the presence of
   * other existing L1-RID/XID or L1-ECMP nodes. Hence, traffic
   * if reaches the new pipe blocks, it will always gets hashed
   * to the one of the old members of the group. Now update the
   * vector nodes with the new vector map.
   * */
  mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  /* Update the active vector. */
  if ((g->refs) && (mc_mgr_versioning_on(sid, dev))) {
    /* Switch versions. */
    sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    mc_mgr_drv_cmplt_operations(sid, dev);
    /* Flip table version. */
    bool ver = mc_mgr_ctx_tbl_ver(dev);
    sts = mc_mgr_program_tbl_ver(sid, dev, !ver);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    /* Drain old version. */
    sts = mc_mgr_wait_for_ver_drain(dev, ver);
    if (BF_SUCCESS != sts) goto not_enough_nodes;
    /* Write old-active vector to use a new mask. */
    mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  } else {
    /* Write active vector to use a new mask. */
    mc_mgr_write_actv_vector(sid, dev, g, new_vec, g->base);
    sts = mc_mgr_drv_wrl_send(sid, false);
  }

  /* As ecmp grp may got associated to l1-ecmp nodes on some pipes, we update
   * the TVT/PVT table. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);
    }
  }

  /* The vector nodes are updated with new block addresses. We
   * can free now the old L1-End block addresses.*/
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    if (old_base[p]) mc_mgr_rdm_map_enqueue_free(sid, dev, old_base[p]);
    MC_MGR_DBGCHK(!*M3(to_write, idx_l1, p));
    MC_MGR_DBGCHK(!*M3(to_write, idx_l2, p));
    MC_MGR_DBGCHK(!*M3(to_write, idx_lag, p));
  }

  MC_MGR_FREE(l2_len);
  MC_MGR_FREE(to_write);
  MC_MGR_FREE(to_write_l1_ecmp);
  MC_MGR_FREE(mgids);
  return sts;
not_enough_nodes:
  for (m = 0; m < used_mgid_count; m++) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      while (*M3(to_write_l1_ecmp, m, p)) {
        uint32_t x = 0;
        mc_mgr_rdm_addr_pop(M3(to_write_l1_ecmp, m, p), &x);
        mc_mgr_rdm_map_return(dev, x);
      }
    }
  }

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (*M3(to_write, idx_l1, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l1, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_lag, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_lag, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M3(to_write, idx_l2, p)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M3(to_write, idx_l2, p), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }

  MC_MGR_FREE(l2_len);
not_enough_memory_l2_len:
  MC_MGR_FREE(to_write);
not_enough_memory_to_write:
  MC_MGR_FREE(to_write_l1_ecmp);
not_enough_memory_to_write_l1_ecmp:
  MC_MGR_FREE(mgids);
not_enough_memory_for_mgdids:
  return BF_NO_SYS_RESOURCES;
}

bf_status_t mc_mgr_ecmp_mbr_rem_one_mbr(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g,
                                        mc_l1_node_t *node) {
  bf_status_t sts = BF_SUCCESS;

  /* There are two cases below:
   * 1) Deleting the last member of the group will free the L1-Blocks,
   *    L1-ECMP pointer nodes and the group member nodes from existing
   *    pipes.
   * 2) Non last member, just invalidate the member and update
   *    the membership vectors. Also free the L1-END blocks and
   *    L1-ECMP pointers from pipes where the removed node has only
   *    L2-Chains.
   */
  mc_mgr_rdm_addr_list_t *to_clean = NULL;

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int used_mgid_count = 0;
  mc_l1_node_t *l1_ecmp_node = g->refs;
  while (l1_ecmp_node) {
    used_mgid_count += 1;
    l1_ecmp_node = l1_ecmp_node->ecmp_next;
  }

  /* Allocate placeholder to hold the MGIDs that the grp is associated with.*/
  int *mgids = MC_MGR_CALLOC(used_mgid_count, sizeof(int));
  if (!mgids) {
    sts = BF_NO_SYS_RESOURCES;
    goto not_enough_memory_for_mgdids;
  }

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int m;
  for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    mgids[m] = l1_ecmp_node->mgid;
  }

  uint32_t x = 0;
  int p = 0;
  for (; x < 32 && node != g->mbrs[x]; ++x)
    ;
  MC_MGR_DBGCHK(x != 32);
  uint32_t new_vec = g->valid_map & ~(1u << x);

  /* As we are removing the node from the group, the del_mask will
   * be same as the node 's pipe mask. */
  uint8_t del_mask = mc_mgr_current_pipe_mask(node);

  /* Determine the pipe mask for all other member nodes. If its the last
   * node then other_mbrs_pipe_mask will always be zero. */
  int other_mbrs_pipe_mask = 0;
  for (int mbr = 0; mbr < 32; ++mbr) {
    mc_l1_node_t *l1_end_node = g->mbrs[mbr];
    if (!l1_end_node || (l1_end_node == node)) continue;
    other_mbrs_pipe_mask |= mc_mgr_calculate_pipe_mask(l1_end_node);
  }

  /* There are two scenarios if a node is getting removed.
   * case 1: Removed node pipe mask is not equal to other member pipe mask.
   *         In this case we remove the node and invalidate the idx by updating
   *         the vector map. Also free the L1-END block and L1-ECMP ptrs from
   *         pipes where the removed node has explicitly pipe bits set and
   *         no other members have their pipe bits set.
   *
   *         This also covers the scenario if removed node is last member of the
   *         group.
   *
   * case 2: The removed node pipe mask is equal to the other members pipe
   *         mask. In this case, we remove the node and invalidate the idx
   *         by updating the vector map. No L1-END blocks and L1-ECMP ptrs
   *         are removed.
   */
  if (del_mask != other_mbrs_pipe_mask) {
    for (m = 0, l1_ecmp_node = g->refs; m < used_mgid_count && l1_ecmp_node;
         m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
      for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
        int pm = 1 << p;
        if (!(del_mask & pm)) continue;
        /* Diassociate ecmp group and  l1-ecmp nodes from the pipes where
         * the deleted member node is only the member that has non zero L2
         * chains. */
        if (other_mbrs_pipe_mask & pm) continue;
        mc_mgr_delete_l1_node_from_pipe(sid, l1_ecmp_node, p, &to_clean);
      }
    }
  }

  /* Rewrite the standby vector node to use a new membership vector which
   * does not include the removed member. If last member of the group is
   * removed, its okay to update the vector node with old member L1-blocks. */
  mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  /* If the group is associated to any MGIDs a version update must
   * be done to ensure no traffic loss or duplication. */
  if ((g->refs) && (mc_mgr_versioning_on(sid, dev))) {
    sts = mc_mgr_drv_wrl_send(sid, false);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    /* Flip table version. */
    bool ver = mc_mgr_ctx_tbl_ver(dev);
    sts = mc_mgr_program_tbl_ver(sid, dev, !ver);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    /* Drain old version. */
    sts = mc_mgr_wait_for_ver_drain(dev, ver);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    /* Write old-active vector to use new mask. */
    mc_mgr_write_stby_vector(sid, dev, g, new_vec, g->base);
  } else {
    mc_mgr_write_actv_vector(sid, dev, g, new_vec, g->base);
    sts = mc_mgr_drv_wrl_send(sid, false);
  }

  /* Go ahead and safely free the L1-End block and the associated L2
   * chains of the removed node. */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(del_mask & pm)) continue;

    /* Free the L2 chain only and the tree will be still balanced as we might
     * have shortened the MGID tree post deleteion of L2 chain.. */
    mc_mgr_delete_l1s_l2_chain(sid, dev, g->base[p] + x);
    node->hw_nodes[p].rdm_addr = 0;
    mc_mgr_delete_l1(&node->hw_nodes[p]);

    /* If the L1-End node is the last member in this pipe then free the
     * L1-End block too. If other members have L2 chains then skip the
     * deletion of the L1-End block. */
    LOG_DBG("Freed L1-Block RDM:%#x pipe:%d mbr_node_handle:%#x grp_hdl:%#x",
            g->base[p],
            p,
            node->handle,
            g->handle);
    if (other_mbrs_pipe_mask & pm) continue;
    mc_mgr_rdm_addr_append(&to_clean, g->base[p]);
    g->base[p] = 0;

    /* Mark the other members of the group with zero rdm address for this
     * current pipe. */
    int mbr;
    for (mbr = 0; mbr < 32; ++mbr) {
      mc_l1_node_t *l1_end_node = g->mbrs[mbr];
      if (!l1_end_node || (l1_end_node == node)) continue;
      l1_end_node->hw_nodes[p].rdm_addr = 0;
      mc_mgr_delete_l1(&l1_end_node->hw_nodes[p]);
    }
  }

  /* For non-last member:
   * When members are added to the group we reserve a block of N entries.
   * If additional members are added in the future and fit within
   * the N allocated RDM entries we use that memory. If not though we get a
   * new larger block, and garbage collect the old. If entries are removed
   * from the ECMP group we maintain the same RDM foot
   * print, and if entries are add back into the group in the future there
   * will be RDM space ready to use.
   * TODO: Currently we do not have is a compaction routine.
   * A compaction routine would go over all the groups and reallocate them
   * to new RDM allocations to reclaim any RDM space used by a group that has
   * extra RDM reserved. It would also move nodes and groups around
   * in the RDM in order to free up blocks.  */
  g->mbrs[x] = NULL;
  g->valid_map = new_vec;
  g->allocated_sz = new_vec ? g->allocated_sz : 0;

  node->ecmp_grp = NULL;

  /* Re-evaluate the tail nodes of any trees using this ECMP group as they may
   * be able to use a shorter tail now.  Note that this processing will be
   * deferred until an RDM change is signaled so we are sure no packets are
   * using the removed member. */
  mc_mgr_ecmp_update_tail_reevaluate(sid, dev, g);

  /* First update the TVT/PVT such and then push the pending write list
   * commands to HW in to order to have hitless. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);
    }
  }

  /* The freed entries are marked to be freed post RDM-CHANGE. */
  while (to_clean) {
    x = 0;
    mc_mgr_rdm_addr_pop(&to_clean, &x);
    mc_mgr_rdm_map_enqueue_free(sid, dev, x);
  }

  /* Push the write list for "Delete/Update" such that HW does not see the new
   * nodes after TVT/PVT update . Also intiate the RDM change for the freed
   * buffers and tail update. */
  sts = mc_mgr_drv_wrl_send(sid, true);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
  }

  MC_MGR_FREE(mgids);
not_enough_memory_for_mgdids:
  return sts;
}

bf_status_t mc_mgr_set_backup_ecmps(int sid,
                                    bf_dev_id_t dev,
                                    int *old_lag_pm,
                                    int *new_lag_pm,
                                    int protected_bit_idx,
                                    int new_backup_idx,
                                    int old_backup_idx,
                                    bf_bitset_t *prot_port_lags) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t addr = 0;
  unsigned long not_used;
  int p;

  mc_l1_node_t *node = NULL;
  mc_mgr_rdm_addr_list_t *to_clean = NULL;
  mc_mgr_rdm_addr_list_t *to_write_lags = NULL;
  mc_mgr_rdm_addr_list_t *to_write_l2_port = NULL;
  mc_mgr_rdm_addr_list_t *to_write_l1_block[MC_MGR_NUM_PIPES] = {0};
  mc_mgr_rdm_addr_list_t *to_write_l1_ecmp[MC_MGR_NUM_PIPES] = {0};

  int *mgids = MC_MGR_CALLOC(BF_MGID_COUNT, sizeof(int));
  if (!mgids) goto not_enough_memory_for_mgids;

  /* Walk L1 nodes and count number of L1 and L2 nodes to alloc. */
  bf_map_sts_t s;
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (node == NULL) {
      MC_MGR_DBGCHK(0);
      goto not_enough_nodes;
    }

    /* Skip L1-RID nodes which are not asssociated with any ECMP group. */
    if (!node->ecmp_grp) continue;

    /* Skip L1-ECMP nodes which are associated with MGID. Thus we are only
     * handling L1-End nodes or ecmp member nodes. */
    if (node->mgid != -1) continue;

    /* Skip L1 nodes which are not using the protected port as they will
     * not change. */
    bf_bitset_t ports;
    bf_dev_pipe_t pipe = mc_bit_idx_to_pipe(dev, protected_bit_idx);
    if (pipe >= MC_MGR_NUM_PIPES) {
      LOG_ERROR("%s:%d Pipe id %d out of range for bit index %d device id %d",
                __func__,
                __LINE__,
                pipe,
                protected_bit_idx,
                dev);
      MC_MGR_DBGCHK(0);
      goto not_enough_nodes;
    }

    /* Here, we are considering only nodes where the protected port is a member
     * of the node. Also check if the protected port is a member of ode's
     * lags.*/
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[pipe]);
    if (!bf_bs_get(&ports, mc_bit_idx_to_local_port(dev, protected_bit_idx)) &&
        !mc_mgr_is_prot_port_node_lags(
            dev, node, mc_bit_idx_to_dev_port(dev, protected_bit_idx)))
      continue;

    /* Count all MGIDs for the node associated with an ECMP group. */
    int used_mgid_count = 0;
    mc_ecmp_grp_t *ecmp_grp = node->ecmp_grp;
    mc_l1_node_t *l1_ecmp_node = ecmp_grp->refs;
    while (l1_ecmp_node) {
      used_mgid_count += 1;
      l1_ecmp_node = l1_ecmp_node->ecmp_next;
    }

    int l1_add_mask = mc_mgr_calculate_backup_l1_add_mask(
        node, old_backup_idx, new_backup_idx);

    /* Check if we need to allocate L1-ECMP nodes. Iterate through all MGIDs
     * that the group is associated and allocate L1-ECMP node for each of the
     * group. */
    int m;
    for (m = 0; m < used_mgid_count; m++) {
      sts = mc_mgr_get_l1_ecmps(sid, dev, l1_add_mask, &to_write_l1_ecmp[0]);
      if (BF_SUCCESS != sts) goto not_enough_nodes;
    }

    /* Check if we are required to allocate L1-End block. */
    sts = mc_mgr_get_ecmp_blocks(
        sid, dev, ecmp_grp->allocated_sz, l1_add_mask, &to_write_l1_block[0]);
    if (BF_SUCCESS != sts) goto not_enough_nodes;

    /* Assume the port node will be blindly re-written, therefore
     * allocate a block of RDM nodes to hold the new port node(s). */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      int block_sz;
      mc_mgr_get_port_node_size(dev, p, node, &block_sz, NULL);
      if (block_sz) {
        uint32_t new_l2_rdm_addr = mc_mgr_rdm_map_get(
            sid, dev, p, mc_mgr_rdm_node_type_port72, block_sz);
        if (!new_l2_rdm_addr) {
          sts = BF_NO_SYS_RESOURCES;
          goto not_enough_nodes;
        }
        mc_mgr_rdm_addr_append(&to_write_l2_port, new_l2_rdm_addr);
      }

      /* Check if we have any LAG nodes to be allocated in this pipe.
       * This pipe can be either new pipe or existing pipe. We are
       * not allocating the whole existing LAGs, we are only allocating
       * new LAG nodes for changed backup map. */
      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);
      int lag_id = -1;
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        if (!bf_bs_get(prot_port_lags, lag_id)) continue;
        if (!((new_lag_pm[lag_id] & (1 << p)) &&
              (~old_lag_pm[lag_id] & (1 << p))))
          continue;
        uint32_t x =
            mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_lag, 1);
        if (!x) {
          sts = BF_NO_SYS_RESOURCES;
          goto not_enough_nodes;
        }
        mc_mgr_rdm_addr_append(&to_write_lags, x);
      }
    }
  }

  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (node == NULL) {
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
    }

    /* Skip L1-RID nodes which are not asssociated with any ECMP group. */
    if (!node->ecmp_grp) continue;

    /* Skip L1-ECMP nodes which are associated with MGID. Thus we are only
     * handling L1-End nodes. */
    if (node->mgid != -1) continue;

    /* Find the offset of the mbr node. */
    int offset = 0;
    mc_ecmp_grp_t *ecmp_grp = node->ecmp_grp;
    for (offset = 0; offset < 32 && node != ecmp_grp->mbrs[offset]; ++offset)
      ;
    MC_MGR_DBGCHK(offset != 32);

    /* Skip L1 nodes which are not using the protected port as they will
     * not change. */
    bf_bitset_t ports;
    bf_dev_pipe_t pipe = mc_bit_idx_to_pipe(dev, protected_bit_idx);
    if (pipe >= MC_MGR_NUM_PIPES) {
      LOG_ERROR("%s:%d Pipe id %d out of range for bit index %d device id %d",
                __func__,
                __LINE__,
                pipe,
                protected_bit_idx,
                dev);
      MC_MGR_DBGCHK(0);
      goto not_enough_nodes;
    }

    /* Here, we are considering only nodes where the protected port is a member
     * of the node. Also check if the protected port is a member of node's
     * lags.*/
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[pipe]);
    if (!bf_bs_get(&ports, mc_bit_idx_to_local_port(dev, protected_bit_idx)) &&
        !mc_mgr_is_prot_port_node_lags(
            dev, node, mc_bit_idx_to_dev_port(dev, protected_bit_idx)))
      continue;

    /* Collect all MGIDs for the node associated with an ECMP group. */
    int used_mgid_count = 0;
    mc_l1_node_t *l1_ecmp_node = ecmp_grp->refs;
    while (l1_ecmp_node) {
      mgids[used_mgid_count] = l1_ecmp_node->mgid;
      used_mgid_count += 1;
      l1_ecmp_node = l1_ecmp_node->ecmp_next;
    }

    /* Calculate pipe mask which includes old and new backup pipe mask. */
    int backup_mod_pm =
        mc_mgr_calculate_backup_mod_pm(node, old_backup_idx, new_backup_idx);
    /* Calculate the pipe mask where L1-RID nodes need to be allocated. */
    int l1_add_mask = mc_mgr_calculate_backup_l1_add_mask(
        node, old_backup_idx, new_backup_idx);
    /* Calculate the l1-l2 chain delete pipe mask. */
    int del_mask =
        mc_mgr_calculate_backup_del_mask(node, old_backup_idx, new_backup_idx);

    LOG_DBG(
        "L1 mod_pm:%x add_pm:%x port_bit:%d old_backup_p:%d new_backup_p:%d",
        backup_mod_pm,
        l1_add_mask,
        protected_bit_idx,
        old_backup_idx,
        new_backup_idx);

    /* Add the L1-End blocks for pipes where add_mask !=0. As no L2
     * chains are added only pass NULL and invalid offset of 32.*/
    sts = mc_mgr_ecmp_add_block_n_one_mbr(sid,
                                          dev,
                                          32,
                                          l1_add_mask,
                                          node,
                                          ecmp_grp,
                                          &to_write_l1_block[0],
                                          NULL,
                                          NULL);
    if (sts != BF_SUCCESS) {
      LOG_ERROR("Failed to add mbr_hdl:%x to the group:%x with add_mask:%x",
                node->handle,
                ecmp_grp->handle,
                l1_add_mask);
      MC_MGR_DBGCHK(0);
      goto not_enough_nodes;
    }

    /* Iterate through pipes and handle pipes where new ports/lags
     * need to be allocated. */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (!(backup_mod_pm & (1 << p))) continue;

      /* Check if there is any existing L1-Node in this chain. If not
       * so then there is no L2 chain to be modify.  */
      if (!node->hw_nodes[p].rdm_addr) continue;

      /* Check if we have any LAG nodes to be allocated in this pipe.
       * This adds lag nodes to both new and existing pipes. These
       * lag nodes are getting added due to changed port mbrship.*/
      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);

      /* Get the address of the RDM node pointing to the port node
       * and previous node which is either a L1 or a LAG node. */
      uint32_t port_prev_addr = 0;
      mc_mgr_rdm_get_port_node_from_l1(
          node->dev, node->hw_nodes[p].rdm_addr, &addr, &port_prev_addr);
      if (addr) {
        /* If a port node already exists keep track of its address to
         * free later. */
        mc_mgr_rdm_addr_append(&to_clean, addr);
      }

      /* Check the size needed for the new port node. Write the new
       * port node. Delay the update of the prev node's next with the
       * new port node because the size of the port node might have
       * increased, so we may need to balance the tree. */
      int block_sz;
      uint8_t mask = 0;
      mc_mgr_get_port_node_size(dev, p, node, &block_sz, &mask);
      uint32_t new_l2_rdm_addr = 0;
      if (block_sz) {
        mc_mgr_rdm_addr_pop(&to_write_l2_port, &new_l2_rdm_addr);
        mc_mgr_write_l2_port_node(sid, node, new_l2_rdm_addr, mask);
      }

      uint32_t l2_head_rdm = mc_mgr_rdm_l1_node_get_l2_ptr(
          mc_mgr_ctx_rdm_map(dev), node->hw_nodes[p].rdm_addr);

      /* Check if the port node is head of l2 chain, if so then assign the new
       * port node to head of the L2 chain. */
      if (port_prev_addr == node->hw_nodes[p].rdm_addr) {
        l2_head_rdm = new_l2_rdm_addr;
      }

      /* Check if we have any LAG nodes to be allocated in this pipe. */
      int lag_id = -1;
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        if (!bf_bs_get(prot_port_lags, lag_id)) {
          continue;
        }

        if (!((new_lag_pm[lag_id] & (1 << p)) &&
              (~old_lag_pm[lag_id] & (1 << p)))) {
          continue;
        }

        uint32_t new_lag_rdm_addr = 0;
        mc_mgr_rdm_addr_pop(&to_write_lags, &new_lag_rdm_addr);
        mc_mgr_rdm_write_lag(sid, dev, new_lag_rdm_addr, lag_id, l2_head_rdm);
        l2_head_rdm = new_lag_rdm_addr;
      }

      /* Write the new port node and add it to the tree. */
      /* block_sz is the number of port nodes in this pipe.  Also count the
       * number of LAG nodes in this pipe so we can adjust the length stored
       * against the node struct. */
      int lag_sz = 0, port_cnt = 0;
      mc_mgr_get_l2_chain_sz(pipe, node, &port_cnt, &lag_sz);
      node->l2_count[p] = block_sz + lag_sz;

      /* Before pointing the node to the new L2 chain, check if the tail should
       * be updated. The node is an ECMP member then many groups may need to
       * be updated. */
      mc_mgr_ecmp_update_tail_reevaluate(sid, dev, ecmp_grp);

      /* port_prev_addr can point to either L1 or an existing Lag node. Thus
       * update the previous port with new port node. */
      if (port_prev_addr != node->hw_nodes[p].rdm_addr)
        mc_mgr_rdm_update_next_l2(sid, dev, port_prev_addr, new_l2_rdm_addr);

      mc_mgr_rdm_update_next_l2(
          sid, dev, node->hw_nodes[p].rdm_addr, l2_head_rdm);
    }

    /* We don't need versioning switch as we are not modifying existing
     * L1-ECMP's vector. Above, L2 chains are added new pipes. Updated
     * lags and port nodes are added on existing pipes */
    mc_mgr_write_stby_vector(
        sid, dev, ecmp_grp, ecmp_grp->valid_map, ecmp_grp->base);
    mc_mgr_write_actv_vector(
        sid, dev, ecmp_grp, ecmp_grp->valid_map, ecmp_grp->base);

    /* Associate the new L1-End vector nodes to the L1-ECMP nodes associated
     * with the ECMP group. */
    int m;
    for (m = 0, l1_ecmp_node = ecmp_grp->refs;
         m < used_mgid_count && l1_ecmp_node;
         m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
      sts = mc_mgr_write_one_ecmp_mbr_l1_ecmps(
          sid, dev, l1_add_mask, l1_ecmp_node, ecmp_grp, &to_write_l1_ecmp[0]);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Failed to associate mbr_hdl:0x%x to the group:0x%x with "
            "add_mask:%x",
            node->handle,
            ecmp_grp->handle,
            l1_add_mask);
        MC_MGR_DBGCHK(0);
      }
    }

    /* Pushing the pending add/RDM changes from the mc-mgr write list. */
    sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }

    /* Update the TVT/PVT accordingly. As the node RDM are already modified it
     * is not safe to return from here. Hence add an assert in error case. */
    for (m = 0; m < used_mgid_count; m++) {
      sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
      sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
                  node->handle,
                  node->mgid);
        MC_MGR_DBGCHK(0);
      }
    }

    /* Determine the pipe mask for all other member nodes. */
    int other_mbrs_pm = 0;
    int mbr;
    for (mbr = 0; mbr < 32; ++mbr) {
      mc_l1_node_t *l1_end_node = ecmp_grp->mbrs[mbr];
      if (!l1_end_node || l1_end_node == node) continue;
      other_mbrs_pm |= mc_mgr_calculate_pipe_mask(l1_end_node);
    }

    /* Remove L1-ECMP ptrs from all mgids that the group is associated
     * with if the grp does have any members on it in pipe "p".  */
    for (m = 0, l1_ecmp_node = ecmp_grp->refs;
         m < used_mgid_count && l1_ecmp_node;
         m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
      for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
        if (!(del_mask & (1 << p))) continue;
        /* Skip L1-End nodes array deletion if some other member node
         * has non zero l2 chain. */
        if (other_mbrs_pm & (1 << p)) continue;
        mc_mgr_delete_l1_node_from_pipe(sid, l1_ecmp_node, p, &to_clean);
      }
    }

    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (!(backup_mod_pm & (1 << p))) continue;
      /* Check if an L1-End has any L2 chain to be removed. */
      if ((del_mask & (1 << p))) {
        mc_mgr_delete_l1s_l2_chain(sid, dev, ecmp_grp->base[p] + offset);

        /* If the L1-End node is the last member in this pipe then free the
         * L1-End array too. */
        if ((other_mbrs_pm & (1 << p))) continue;
        LOG_DBG(
            "Freed L1-Block RDM:%#x pipe:%d mbr_node_handle:%#x grp_hdl:%#x",
            ecmp_grp->base[p],
            p,
            node->handle,
            ecmp_grp->handle);

        mc_mgr_rdm_addr_append(&to_clean, ecmp_grp->base[p]);
        ecmp_grp->base[p] = 0;

        /* Mark the other members of the group with zero rdm address for this
         * current pipe. */
        for (mbr = 0; mbr < 32; ++mbr) {
          mc_l1_node_t *l1_end_node = ecmp_grp->mbrs[mbr];
          if (!l1_end_node) continue;
          l1_end_node->hw_nodes[p].rdm_addr = 0;
          mc_mgr_delete_l1(&l1_end_node->hw_nodes[p]);
        }
        continue;
      }

      /* Check if we have LAG nodes to be removed in this pipe. These are those
       * LAG Nodes whose member port's (protected port) backup changed from
       * current pipe to some other pipe. This only modified the lags from
       * existing pipes. */
      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);

      int lag_id = -1;
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        /* Check if the LAG id is being used by the protected port */
        if (!bf_bs_get(prot_port_lags, lag_id)) continue;
        if (!((~new_lag_pm[lag_id] & (1 << p)) &&
              (old_lag_pm[lag_id] & (1 << p))))
          continue;
        uint32_t lag_rdm_addr = 0;
        mc_mgr_rdm_unlink_and_get_lag_node_by_lag_id(
            sid, node->dev, node->hw_nodes[p].rdm_addr, &lag_rdm_addr, lag_id);
        if (!lag_rdm_addr) continue;
        mc_mgr_rdm_addr_append(&to_clean, lag_rdm_addr);
        node->l2_count[p] -= 1;
      }

      /* Port nodes to be removed due to backup mbrship change, is already
       * added to clean list. */
    }

    /* Re-evaluate the group. The tree will be balanced post RDM change. This
     * might shortens the tree. */
    mc_mgr_ecmp_update_tail_reevaluate(sid, dev, ecmp_grp);

    /* Update the TVT/PVT accordingly. As the node RDM are already modified it
     * is not safe to return from here. Hence add an assert in error case. */
    for (m = 0; m < used_mgid_count; m++) {
      sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
      sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
                  node->handle,
                  node->mgid);
        MC_MGR_DBGCHK(0);
      }
    }

    /* Free the old entries. Mark them to be free post RDM change. */
    while (to_clean) {
      uint32_t old_rdm_addr = 0;
      mc_mgr_rdm_addr_pop(&to_clean, &old_rdm_addr);
      mc_mgr_rdm_map_enqueue_free(sid, dev, old_rdm_addr);
    }

    /* Pushing the pending add/RDM changes from the mc-mgr write list.
     * This also intiates the RDM change post completion, Hence, we
     * get a grace period for the inflight packets to be flushed before
     * deletion of L1 and L2 nodes. */
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }
  }

  MC_MGR_FREE(mgids);
  return sts;
not_enough_nodes:
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (to_write_l1_ecmp[p]) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(&to_write_l1_ecmp[p], &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (to_write_l1_block[p]) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(&to_write_l1_block[p], &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }
  while (to_write_lags) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_write_lags, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  while (to_write_l2_port) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_write_l2_port, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  MC_MGR_FREE(mgids);
not_enough_memory_for_mgids:
  return sts;
}

bf_status_t mc_mgr_ecmp_l1_write(int sid, mc_l1_node_t *node) {
  /* Check if the node is associated with a MGID */
  if (-1 != node->mgid) {
    LOG_ERROR("Node %#x is already a member of multicast group %#x",
              node->handle,
              node->mgid);
    return BF_INVALID_ARG;
  }

  bf_status_t sts = BF_SUCCESS;
  bf_dev_id_t dev = node->dev;

  /* Determine the index where the node belongs in the ecmp group. */
  mc_ecmp_grp_t *ecmp_grp = node->ecmp_grp;
  int offset = 0;
  for (offset = 0; offset < 32 && node != ecmp_grp->mbrs[offset]; ++offset)
    ;
  MC_MGR_DBGCHK(offset != 32);

  /* Which pipes the node needs to be in. */
  uint8_t pipe_mask = mc_mgr_calculate_pipe_mask(node);
  /* Which pipes the node is currently in. */
  uint8_t curr_mask = mc_mgr_current_pipe_mask(node);
  MC_MGR_DBGCHK(curr_mask == mc_mgr_current_ecmp_pipe_mask(ecmp_grp));
  /* Remove node from these pipes. */
  uint8_t del_mask = curr_mask & ~pipe_mask;
  /* Add node to these pipes. */
  uint8_t add_mask = ~curr_mask & pipe_mask;
  /* Update node in these pipes. */
  uint8_t upd_mask = curr_mask & pipe_mask;
  bool has_work = del_mask || add_mask || upd_mask;
  if (!has_work) {
    return BF_SUCCESS;
  }

  LOG_TRACE("%s pipe_mask:%d curr_mask:%d add_mask:%d upd_mask:%d",
            __func__,
            pipe_mask,
            curr_mask,
            add_mask,
            upd_mask);

  int used_mgid_count = 0;
  mc_l1_node_t *l1_ecmp_node = ecmp_grp->refs;
  while (l1_ecmp_node) {
    used_mgid_count += 1;
    l1_ecmp_node = l1_ecmp_node->ecmp_next;
  }

  /* Allocate placeholder to hold the MGIDs that the grp is associated with.*/
  int *mgids = MC_MGR_CALLOC(used_mgid_count, sizeof(int));
  if (!mgids) goto not_enough_memory_for_mgdids;

  /* Collect all MGIDs for the node associated with an ECMP group. */
  int m;
  for (m = 0, l1_ecmp_node = ecmp_grp->refs;
       m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    mgids[m] = l1_ecmp_node->mgid;
  }

  /* Allocate placeholder to hold L1-ECMP RDM memories */
  mc_mgr_rdm_addr_list_t **to_write_l1_ecmp = MC_MGR_CALLOC(
      used_mgid_count * MC_MGR_NUM_PIPES, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write_l1_ecmp) goto not_enough_memory_to_write_l1_ecmp;

  /* Allocate placeholder to hold L1-End block, L2-Port and Lag RDM memories
   * Here "3" signifies palce holder for L1-END block , LAG-RDM, and PORT-RDM
   * for each pipe. */
  mc_mgr_rdm_addr_list_t **to_write =
      MC_MGR_CALLOC(MC_MGR_NUM_PIPES * 3, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write) goto not_enough_memory_to_write;

  /* Allocate an array of mgid_counts to hold the max l2 length. */
  uint32_t *l2_len =
      MC_MGR_CALLOC(used_mgid_count * MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!l2_len) goto not_enough_memory_l2_len;

  uint32_t *len_before_update =
      MC_MGR_CALLOC(used_mgid_count * MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!len_before_update) goto not_enough_memory_l2_before_update;

  uint32_t *len_after_update =
      MC_MGR_CALLOC(used_mgid_count * MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!len_after_update) goto not_enough_memory_l2_after_update;
  int idx_l1 = 0;  /* Index to_write for L1 ecmp addresses. */
  int idx_lag = 1; /* Index to_write for lag addresses. */
  int idx_l2 = 2;  /* Index to_write for port addresses. */
  int p;
  int l1_end_block_sz = ecmp_grp->allocated_sz;

  /* Iterate through each pipe and allocate the following nodes
   *   - Vector nodes
   *   - L1-End array
   *   - L2 chain of nodes
   */
  for (m = 0; m < used_mgid_count; m++) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      int pm = 1 << p;
      if (add_mask & pm) {
        /* Adding the tree to this pipe; get an L1 node. */
        uint32_t new_l1_ecmp_addr =
            mc_mgr_rdm_map_get(sid, dev, p, mc_mgr_rdm_node_type_ecmp, 1);
        if (!new_l1_ecmp_addr) {
          goto not_enough_nodes;
        }
        mc_mgr_rdm_addr_append(M3(to_write_l1_ecmp, m, p), new_l1_ecmp_addr);
      }
    }
  }

  /* Allocate L1-End block node for pipes where add_mask !=0. As an ecmp_grp
   * can be associated to more than one MGID, iteratnig through all pipes
   * should be sufficient.*/
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (add_mask & pm) {
      uint32_t new_l1_end_addr = mc_mgr_rdm_map_get(
          sid, dev, p, mc_mgr_rdm_node_type_end, l1_end_block_sz);
      if (!new_l1_end_addr) {
        goto not_enough_nodes;
      }
      mc_mgr_rdm_addr_append(M4(to_write, p, idx_l1), new_l1_end_addr);
    }

    /* For update scenario we make it simple by reallocating whole L2 chain of
     * the L1-End node. */
    if ((add_mask & pm) || (upd_mask & pm)) {
      sts = mc_mgr_allocate_l2_chain(sid,
                                     dev,
                                     p,
                                     node,
                                     M2(l2_len, p),
                                     M4(to_write, p, idx_l2),
                                     M4(to_write, p, idx_lag));
      if (BF_SUCCESS != sts) goto not_enough_nodes;
    }
  }

  /* Start with the adding nodes and change PVT/TVT post addition of the nodes.
   */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(add_mask & pm)) continue;
    uint32_t l1_end_rdm_addr = 0;
    mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l1), &l1_end_rdm_addr);

    /* Iterate through every member node and assoiate the HW of this pipe
     * to the software node. */
    int mbr = 0;
    for (mbr = 0; mbr < 32; ++mbr) {
      mc_l1_node_t *l1_end_node = ecmp_grp->mbrs[mbr];
      if (!l1_end_node) continue;
      if (l1_end_node == node) {
        /* Write the L2 nodes first keeping track of the RDM address of the
         * first L2 node. */
        uint32_t l2_rdm_ptr = 0;
        write_l2_chain(sid,
                       p,
                       l1_end_node,
                       M4(to_write, p, idx_lag),
                       M4(to_write, p, idx_l2),
                       &l2_rdm_ptr);
        /* Write the L1-End node possibly updating the tree's tail. */
        get_new_l1(l1_end_node, p, l1_end_rdm_addr + mbr);
        mc_mgr_rdm_write_l1_end(
            sid, dev, l1_end_rdm_addr + offset, l2_rdm_ptr, l1_end_node->rid);
      } else {
        /* Since this node is NOT the node being added/modified we know
         * it does not have membership in the pipe (otherwise the group
         * wouldn't be in the add mask). */
        /* There is no L2 chain for these L1-End nodes here. */
        get_new_l1(l1_end_node, p, l1_end_rdm_addr + mbr);
        mc_mgr_rdm_write_l1_end(
            sid, dev, l1_end_rdm_addr + mbr, 0, l1_end_node->rid);
      }
    }
    ecmp_grp->base[p] = l1_end_rdm_addr;
    /* Only L1-End block exist, hence put an assert to check leftover. */
    MC_MGR_DBGCHK(!*M4(to_write, p, idx_l1));
  }

  /* We don't need versioning switch as we are not modifying existing
   * L1-ECMP's vector address. Rather we are adding a whole new L1-End
   * block to both the versions. */
  mc_mgr_write_stby_vector(
      sid, dev, ecmp_grp, ecmp_grp->valid_map, ecmp_grp->base);
  mc_mgr_write_actv_vector(
      sid, dev, ecmp_grp, ecmp_grp->valid_map, ecmp_grp->base);

  /* Associate the L1-End block to new L1-ECMP nodes for each pipes
   * where add_mask != 0. This iteration is equivalent to associating a
   * ECMP group to L1-ECMP node/MGID. */
  for (m = 0, l1_ecmp_node = ecmp_grp->refs;
       m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      int pm = 1 << p;
      if (!(add_mask & pm)) continue;

      uint32_t l1_ecmp_rdm_addr = 0;
      mc_mgr_rdm_addr_pop(M3(to_write_l1_ecmp, m, p), &l1_ecmp_rdm_addr);
      get_new_l1(l1_ecmp_node, p, l1_ecmp_rdm_addr);

      /* Update the L1-ECMP node with vector node addresses.
       * Write L1-ECMP node to the RDM at the head of the L1 list. */
      uint32_t next_l1_addr = mc_mgr_get_mit(dev, p, l1_ecmp_node->mgid);
      uint32_t p0 = node->ecmp_grp->vector_node_addr[0][p];
      uint32_t p1 = node->ecmp_grp->vector_node_addr[1][p];
      mc_mgr_rdm_write_ptr(sid,
                           dev,
                           l1_ecmp_node->hw_nodes[p].rdm_addr,
                           p0,
                           p1,
                           next_l1_addr,
                           l1_ecmp_node->xid,
                           l1_ecmp_node->xid_valid);

      /* Link the L1-ECMP node to the MIT chain (updates SW structures only).
       */
      link_l1(&l1_ecmp_node->hw_nodes[p], dev, p, l1_ecmp_node->mgid);

      /* At HW level we have modified the entry of l1_ecmp_rdm, but as this is
       * not yet associated to the MGID table, hence all above modification
       * does not impact the traffic. As Software structures are all modified
       * we are good to rebalance the tree. */

      /* Get the length of the longest L2 chain in the pipe. */
      uint32_t length_after_add =
          mc_mgr_compute_longest_l2(dev, p, l1_ecmp_node->mgid);
      /* Set the tail based on that length.This is done before MGID update
       * such that HW sees the latest balanced tree. */
      mc_mgr_update_len(sid, dev, p, l1_ecmp_node->mgid, length_after_add);

      /* Program the MIT. Assert if it fails. */
      if (BF_SUCCESS != mc_mgr_set_mit_wrl(sid, dev, p, l1_ecmp_node->mgid)) {
        MC_MGR_DBGCHK(0);
      }

      /* Only one L1-ECMP node exist per group and per pipe. Hence, put
       * an assert to check for left-over nodes. */
      MC_MGR_DBGCHK(!*M3(to_write_l1_ecmp, m, p));
    }
  }

  /* Push the write list for "ADD" such that HW can see the new nodes after
   * TVT/PVT update. Don't return incase of failure, as we will retry after
   * update or delete mask. */
  sts = mc_mgr_drv_wrl_send(sid, false);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
    MC_MGR_DBGCHK(0);
  }

  /* Program the PVT if it has changed. If it fails continue as we retry
   * after delete and update operations. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);
      MC_MGR_DBGCHK(0);
    }
  }

  /* Queue up the tree updates in a write list using the pre-allocated RDM
   * addresses. */
  mc_mgr_rdm_addr_list_t *to_clean = NULL;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(upd_mask & pm)) continue;

    /* Collect the longest L2 chain for a pipe for all MGIDs. */
    for (m = 0; m < used_mgid_count; m++)
      len_before_update[m] = mc_mgr_compute_longest_l2(dev, p, mgids[m]);

    /* For simplicity, write a whole new L2 chain, link it to the L1 node,
     * and free the old L2 chain. */
    uint32_t l2_rdm_ptr = 0;
    write_l2_chain(sid,
                   p,
                   node,
                   M4(to_write, p, idx_lag),
                   M4(to_write, p, idx_l2),
                   &l2_rdm_ptr);

    /* add all the rdm addresses from the old l2 chain to the list to
     * be reclaimed. */
    mc_mgr_collect_l1s_l2_chain_addrs(
        dev, node->hw_nodes[p].rdm_addr, &to_clean);

    for (m = 0; m < used_mgid_count; m++) {
      /* Check the length of the tree again now that we have updated the sw
       * structure's l2 chain length. If the new length is greater than the
       * older length, first balance the tree for the new length before
       * updating L1 RDM with L2 addresses, such that HW always sees the
       * balanced tree */
      len_after_update[m] = mc_mgr_compute_longest_l2(dev, p, mgids[m]);
      if (len_before_update[m] < len_after_update[m]) {
        mc_mgr_update_len(sid, dev, p, mgids[m], *M2(l2_len, p));
      }
    }

    /* Update the L1 with the new L2 pointer. */
    mc_mgr_rdm_update_next_l2(sid, dev, node->hw_nodes[p].rdm_addr, l2_rdm_ptr);

    for (m = 0; m < used_mgid_count; m++) {
      if (len_before_update[m] > len_after_update[m]) {
        /* This update decreased the length of the longest L2 chain so we can
         * safely update the tree to use a shorter tail. */
        mc_mgr_update_len(sid, dev, p, mgids[m], len_after_update[m]);
      }
    }

    /* Ensure there are no left over nodes that we intended to use in this
     * pipe. */
    MC_MGR_DBGCHK(!*M4(to_write, p, idx_lag));
    MC_MGR_DBGCHK(!*M4(to_write, p, idx_l2));
  }

  /* Delete L2 chain from  a pipe, there can be 2 cases.
   * 1. The L1-End node is one of the mbr of the ECMP group, in
   *    that case, free the L2-chain from the pipe.
   * 2. The L1-End node or mbr is only the member that has
   *    non-zero l2 chain, in that case free the L1-End block
   *    post removal of the member's L2 chain. */

  /* Determine the pipe mask for all other member nodes. */
  int l1_end_other_mbrs_pm = 0;
  int mbr;
  for (mbr = 0; mbr < 32; ++mbr) {
    mc_l1_node_t *l1_end_node = ecmp_grp->mbrs[mbr];
    if (!l1_end_node) continue;
    if (l1_end_node == node) continue;
    l1_end_other_mbrs_pm |= mc_mgr_calculate_pipe_mask(l1_end_node);
  }

  for (m = 0, l1_ecmp_node = ecmp_grp->refs;
       m < used_mgid_count && l1_ecmp_node;
       m++, l1_ecmp_node = l1_ecmp_node->ecmp_next) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      int pm = 1 << p;
      if (!(del_mask & pm)) continue;

      /* Skip the L1-Ecmp nodes deletion from the current pipe, if any other
       * member nodes has L2 chains in this pipe. */
      if (l1_end_other_mbrs_pm & pm) continue;
      mc_mgr_delete_l1_node_from_pipe(sid, l1_ecmp_node, p, &to_clean);
    }
  }

  /* As we have removed the L1-ECMP node safely,
   * Re-evaluate the tail nodes and balance the tree. */
  mc_mgr_ecmp_update_tail_reevaluate(sid, dev, ecmp_grp);

  /* Go ahead and safely free the L1-End array and the associated L2 chains.
   */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(del_mask & pm)) continue;

    /* As ECMP pointers have been removed, we can live with the vector
     * nodes with stale L1-End block address. ALso we came here, means
     * we may be going to free the L1-End block too. */
    mc_mgr_delete_l1s_l2_chain(sid, dev, ecmp_grp->base[p] + offset);

    /* If the L1-End node is the last member in this pipe then free the
     * L1-End blcok too. If other member nodes have L2 chain in this pipe
     * then skip the freeing of L1-End block. */
    if ((l1_end_other_mbrs_pm & pm)) continue;
    LOG_DBG("Freed L1-Block RDM:%#x pipe:%d node_handle:%#x grp_hdl:%#x",
            ecmp_grp->base[p],
            p,
            node->handle,
            ecmp_grp->handle);
    mc_mgr_rdm_addr_append(&to_clean, ecmp_grp->base[p]);
    ecmp_grp->base[p] = 0;

    /* Mark the other members of the group with zero rdm address for this
     * current pipe. */
    for (mbr = 0; mbr < 32; ++mbr) {
      mc_l1_node_t *l1_end_node = ecmp_grp->mbrs[mbr];
      if (!l1_end_node) continue;
      l1_end_node->hw_nodes[p].rdm_addr = 0;
      mc_mgr_delete_l1(&l1_end_node->hw_nodes[p]);
    }
  }

  /* Re-evaluate the tail nodes of any trees using this ECMP group as they may
   * be able to use a shorter tail now.  Note that this processing will be
   * deferred until an RDM change is signaled so we are sure no packets are
   * using the removed member .*/
  mc_mgr_ecmp_update_tail_reevaluate(sid, dev, ecmp_grp);

  /* First update the TVT/PVT and then push the pending write list
   * commands to HW in to order to have hitless. */
  for (m = 0; m < used_mgid_count; m++) {
    sts = mc_mgr_update_pvt(sid, dev, mgids[m], false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, mgids[m], false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to update ECMP TVT/PVT for L1-End node 0x%x in group 0x%x",
          node->handle,
          m);

      MC_MGR_DBGCHK(0);
    }
  }

  /* Now free any nodes which are no longer used. These entries will be
   * actually freed post RDM change. */
  while (to_clean) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_clean, &x);
    mc_mgr_rdm_map_enqueue_free(sid, dev, x);
  }

  /* Push the write list for "Delete/Update" such that HW does not see the old
   * nodes removed after TVT/PVT update */
  sts = mc_mgr_drv_wrl_send(sid, true);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);

    MC_MGR_DBGCHK(0);
  }

  MC_MGR_FREE(l2_len);
  MC_MGR_FREE(len_after_update);
  MC_MGR_FREE(len_before_update);
  MC_MGR_FREE(to_write);
  MC_MGR_FREE(to_write_l1_ecmp);
  MC_MGR_FREE(mgids);

  return BF_SUCCESS;
not_enough_nodes:
  for (m = 0; m < used_mgid_count; m++) {
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      while (*M3(to_write_l1_ecmp, m, p)) {
        uint32_t x = 0;
        mc_mgr_rdm_addr_pop(M3(to_write_l1_ecmp, m, p), &x);
        mc_mgr_rdm_map_return(dev, x);
      }
    }
  }

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (*M4(to_write, p, idx_l1)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l1), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M4(to_write, p, idx_lag)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_lag), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M4(to_write, p, idx_l2)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l2), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }

  MC_MGR_FREE(len_after_update);
not_enough_memory_l2_after_update:
  MC_MGR_FREE(len_before_update);
not_enough_memory_l2_before_update:
  MC_MGR_FREE(l2_len);
not_enough_memory_l2_len:
  MC_MGR_FREE(to_write);
not_enough_memory_to_write:
  MC_MGR_FREE(to_write_l1_ecmp);
not_enough_memory_to_write_l1_ecmp:
  MC_MGR_FREE(mgids);
not_enough_memory_for_mgdids:
  return BF_NO_SYS_RESOURCES;
}

bf_status_t mc_mgr_l1_write(int sid, mc_l1_node_t *node) {
  if (!MC_MGR_VALID_DEV(node->dev)) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Check if the node is associated with ECMP */
  if (ecmp_associated(node)) {
    return mc_mgr_ecmp_l1_write(sid, node);
  }

  bf_dev_id_t dev = node->dev;
  int mgid = node->mgid;

  bf_status_t sts = BF_SUCCESS;

  /* Which pipes the node needs to be in. */
  uint8_t pipe_mask = mc_mgr_calculate_pipe_mask(node);
  /* Which pipes the node is currently in. */
  uint8_t curr_mask = mc_mgr_current_pipe_mask(node);
  /* Remove node from these pipes. */
  uint8_t del_mask = curr_mask & ~pipe_mask;
  /* Add node to these pipes. */
  uint8_t add_mask = ~curr_mask & pipe_mask;
  /* Update node in these pipes. */
  uint8_t upd_mask = curr_mask & pipe_mask;
  bool has_work = del_mask || add_mask || upd_mask;
  if (!has_work) {
    return BF_SUCCESS;
  }

  /* Pre-allocate the RDM resoruces for add mask. First handle
   * L1 nodes which are not associated with ECMP.
   *  - Determine if an L1 needs to be allocated
   *  - How many port nodes need to be allocatd(up to 4 with backup ports)
   */
  mc_mgr_rdm_addr_list_t **to_write =
      MC_MGR_CALLOC(MC_MGR_NUM_PIPES * 3, sizeof(mc_mgr_rdm_addr_list_t *));
  if (!to_write) goto not_enough_memory_to_write;
  uint32_t *l2_len = MC_MGR_CALLOC(MC_MGR_NUM_PIPES, sizeof(uint32_t));
  if (!l2_len) goto not_enough_memory_l2_len;
  int idx_l1 = 0;  /* Index to_write for L1 addresses. */
  int idx_lag = 1; /* Index to_write for lag addresses. */
  int idx_l2 = 2;  /* Index to_write for port addresses. */
  int p;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (add_mask & pm) {
      /* Adding the tree to this pipe; get an L1 node. */
      uint32_t new_l1_addr =
          mc_mgr_rdm_map_get(sid, dev, p, mc_mgr_rdm_node_type_rid, 1);
      if (!new_l1_addr) {
        goto not_enough_nodes;
      }
      mc_mgr_rdm_addr_append(M4(to_write, p, idx_l1), new_l1_addr);
    }
    if ((add_mask & pm) || (upd_mask & pm)) {
      /* Will need to write an L2 chain for this node, get enough L2
       * nodes. For simplicity we are reallocating L2 nodes for update
       * scenario. */
      sts = mc_mgr_allocate_l2_chain(sid,
                                     dev,
                                     p,
                                     node,
                                     M2(l2_len, p),
                                     M4(to_write, p, idx_l2),
                                     M4(to_write, p, idx_lag));
      if (BF_SUCCESS != sts) goto not_enough_nodes;
    }
  }

  /* start with the adding nodes and change PVT/TVT post addition of the nodes
   */
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (add_mask & pm) {
      MC_MGR_ASSERT(mgid_associated(node));
      /*
       * The node is going to be added to this pipe.
       */
      /* Write the L2 nodes first keeping track of the RDM address of the
       * first L2 node. */
      uint32_t l2_rdm_ptr = 0;
      write_l2_chain(sid,
                     p,
                     node,
                     M4(to_write, p, idx_lag),
                     M4(to_write, p, idx_l2),
                     &l2_rdm_ptr);

      /* Write the L1 node possibly updating the tree's tail. */
      uint32_t l1_rdm_addr = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l1), &l1_rdm_addr);
      mc_mgr_write_l1_rid_node(sid, p, node, l1_rdm_addr, l2_rdm_ptr);
    }
  }

  /* Push the write list for "ADD" such that HW can see the new nodes after
   * TVT/PVT update. Don't return if there is a failure to send write list.
   * It will be send again post updates/del commands. */
  sts = mc_mgr_drv_wrl_send(sid, false);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
    MC_MGR_DBGCHK(0);
  }

  /* Program the PVT if it has changed. If it fails continue as we retry
   * after delete and update operations. */
  sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
  sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
    MC_MGR_DBGCHK(0);
  }

  /* Queue up the tree updates in a write list using the pre-allocated RDM
   * addresses. */
  mc_mgr_rdm_addr_list_t *to_clean = NULL;
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (del_mask & pm) {
      MC_MGR_ASSERT(mgid_associated(node));
      /* The node is being removed from this pipe. */
      /* Free L2/L1 RDM nodes for this pipe. */
      mc_mgr_delete_l1_node_from_pipe(sid, node, p, &to_clean);

      uint32_t len_after_removal = mc_mgr_compute_longest_l2(dev, p, mgid);
      mc_mgr_update_len(sid, dev, p, mgid, len_after_removal);
    } else if (upd_mask & pm) {
      /* calculate the correct update len for each mgid
       * for mgid case, we only need to update that specifc mgid*/
      uint32_t len_before_update = 0;
      uint32_t len_after_update = 0;
      len_before_update = mc_mgr_compute_longest_l2(dev, p, mgid);
      /* For simplicity, write a whole new L2 chain, link it to the L1 node,
       * and free the old L2 chain. */
      uint32_t l2_rdm_ptr = 0;
      write_l2_chain(sid,
                     p,
                     node,
                     M4(to_write, p, idx_lag),
                     M4(to_write, p, idx_l2),
                     &l2_rdm_ptr);

      /* Add all the RDM addresses from the old L2 chain to the list to
       * be reclaimed. */
      mc_mgr_collect_l1s_l2_chain_addrs(
          dev, node->hw_nodes[p].rdm_addr, &to_clean);

      /* Check the length of the tree again now that we have updated the sw
       * structure's l2 chain length. */
      len_after_update = mc_mgr_compute_longest_l2(dev, p, mgid);
      if (len_before_update < len_after_update) {
        /* This update will increase the length of the longest L2 chain so
         * update the tree to use a longer tail before making the
         * modifications to the L1 node. */
        mc_mgr_update_len(sid, dev, p, mgid, *M2(l2_len, p));
      }

      /* Update the L1 with the new L2 pointer. */
      mc_mgr_rdm_update_next_l2(
          sid, dev, node->hw_nodes[p].rdm_addr, l2_rdm_ptr);

      if (len_before_update > len_after_update) {
        /* This update decreased the length of the longest L2 chain so
         * update the tree to use a shorter tail. */
        mc_mgr_update_len(sid, dev, p, mgid, len_after_update);
      }
      /* Ensure there are no left over nodes that we intended to use in this
       * pipe. */
      MC_MGR_DBGCHK(!*M4(to_write, p, idx_l1));
      MC_MGR_DBGCHK(!*M4(to_write, p, idx_lag));
      MC_MGR_DBGCHK(!*M4(to_write, p, idx_l2));
    }
  }

  /* Program the PVT if it has changed. */
  if (sts == BF_SUCCESS) {
    sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }
  }

  /* Now free any nodes which are no longer used. Mark these nodes for
   * RDM change too. */
  while (to_clean) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_clean, &x);
    mc_mgr_rdm_map_enqueue_free(sid, dev, x);
  }

  /* Push the write list for "Delete/Update" such that HW can see the new
   * nodes after TVT/PVT update */
  sts = mc_mgr_drv_wrl_send(sid, true);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
              node->handle,
              node->mgid);
    MC_MGR_DBGCHK(0);
  }

  MC_MGR_FREE(l2_len);
  MC_MGR_FREE(to_write);
  return sts;

not_enough_nodes:

  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    while (*M4(to_write, p, idx_l1)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l1), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M4(to_write, p, idx_lag)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_lag), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
    while (*M4(to_write, p, idx_l2)) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(M4(to_write, p, idx_l2), &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }

  MC_MGR_FREE(l2_len);
not_enough_memory_l2_len:
  MC_MGR_FREE(to_write);
not_enough_memory_to_write:
  return BF_NO_SYS_RESOURCES;
}

static void write_vector(int sid,
                         bf_dev_id_t dev,
                         mc_ecmp_grp_t *g,
                         uint32_t new_vec,
                         uint32_t *new_base,
                         bool ver) {
  int pipe = 0;
  uint16_t id = g->ecmp_id;
  (void)mc_mgr_decode_ecmp_hdl(g->handle, __func__, __LINE__);
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    (void)mc_mgr_rdm_write_vec(
        sid, dev, g->vector_node_addr[ver][pipe], new_vec, new_base[pipe], id);
  }
}
void mc_mgr_write_actv_vector(int sid,
                              bf_dev_id_t dev,
                              mc_ecmp_grp_t *g,
                              uint32_t new_vec,
                              uint32_t *new_base) {
  bool ver = mc_mgr_ctx_tbl_ver(dev);
  write_vector(sid, dev, g, new_vec, new_base, ver);
}
void mc_mgr_write_stby_vector(int sid,
                              bf_dev_id_t dev,
                              mc_ecmp_grp_t *g,
                              uint32_t new_vec,
                              uint32_t *new_base) {
  bool ver = !mc_mgr_ctx_tbl_ver(dev);
  write_vector(sid, dev, g, new_vec, new_base, ver);
}

void mc_mgr_ecmp_update_tail_reevaluate(int sid,
                                        bf_dev_id_t dev,
                                        mc_ecmp_grp_t *g) {
  /* For each MGID this group is associated to. */
  for (mc_l1_node_t *i = g->refs; i; i = i->ecmp_next) {
    /* For each pipe. */
    for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      /* Get the length of the longest L2 chain in the pipe. */
      uint32_t l2_len = mc_mgr_compute_longest_l2(dev, p, i->mgid);
      /* Set the tail based on that length. */
      mc_mgr_update_len(sid, dev, p, i->mgid, l2_len);
    }
  }
}

bf_status_t mc_mgr_get_l1_ecmps(
    int sid,
    bf_dev_id_t dev,
    int add_mask,
    mc_mgr_rdm_addr_list_t *l1_ecmp_addrs[MC_MGR_NUM_PIPES]) {
  int pipe = 0;
  int l1_ecmp_addr_cnt[MC_MGR_NUM_PIPES] = {0};

  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    int pm_mask = 1 << pipe;
    if (!(add_mask & pm_mask)) continue;
    uint32_t new_l1_ecmp_addr =
        mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_ecmp, 1);
    if (!new_l1_ecmp_addr) goto failure;
    l1_ecmp_addr_cnt[pipe]++;
    mc_mgr_rdm_addr_append(&l1_ecmp_addrs[pipe], new_l1_ecmp_addr);
  }
  return BF_SUCCESS;

failure:
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    for (int i = 0; i < l1_ecmp_addr_cnt[pipe]; ++i) {
      uint32_t x = 0;
      mc_mgr_rdm_addr_pop(l1_ecmp_addrs + pipe, &x);
      mc_mgr_rdm_map_return(dev, x);
    }
  }
  return BF_NO_SYS_RESOURCES;
}

bf_status_t mc_mgr_write_one_ecmp_mbr_l1_ecmps(
    int sid,
    bf_dev_id_t dev,
    int add_mask,
    mc_l1_node_t *l1_ecmp_node,
    mc_ecmp_grp_t *grp,
    mc_mgr_rdm_addr_list_t **l1_ecmp_addrs) {
  bf_status_t sts = BF_SUCCESS;
  int p;

  /* We are associating the MGIDs with the group by inserting L1-ECMP
   * pointer nodes into the MGID's trees which point to the group's
   * vector nodes.*/
  for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int pm = 1 << p;
    if (!(add_mask & pm)) continue;

    uint32_t l1_ecmp_rdm_addr = 0;
    mc_mgr_rdm_addr_pop(l1_ecmp_addrs + p, &l1_ecmp_rdm_addr);
    get_new_l1(l1_ecmp_node, p, l1_ecmp_rdm_addr);

    /* Update the L1-ECMP node with vector node addresses.
     * Write L1-ECMP node to the RDM at the head of the L1 list. */
    uint32_t next_l1_addr = mc_mgr_get_mit(dev, p, l1_ecmp_node->mgid);
    uint32_t p0 = grp->vector_node_addr[0][p];
    uint32_t p1 = grp->vector_node_addr[1][p];
    mc_mgr_rdm_write_ptr(sid,
                         dev,
                         l1_ecmp_node->hw_nodes[p].rdm_addr,
                         p0,
                         p1,
                         next_l1_addr,
                         l1_ecmp_node->xid,
                         l1_ecmp_node->xid_valid);

    /* Link the L1-ECMP node to the MIT chain (updates SW structures only). */
    link_l1(&l1_ecmp_node->hw_nodes[p], dev, p, l1_ecmp_node->mgid);

    /* At HW level we have modified the entry of l1_ecmp_rdm, but as this is
     * not yet associated to the MGID table, hence all above modification does
     * not impact the traffic.
     * As Software structures are all modified we are good to rebalance the
     * tree. */

    /* Get the length of the longest L2 chain in the pipe. */
    uint32_t length_after_add =
        mc_mgr_compute_longest_l2(dev, p, l1_ecmp_node->mgid);
    /* Set the tail based on that length. This is done before MGID update
     * such that HW sees the latest balanced tree. */
    mc_mgr_update_len(sid, dev, p, l1_ecmp_node->mgid, length_after_add);

    /* Program the MIT. TODO: Is the roll back safe here? */
    if (BF_SUCCESS != mc_mgr_set_mit_wrl(sid, dev, p, l1_ecmp_node->mgid)) {
      return BF_NO_SYS_RESOURCES;
    }

    MC_MGR_DBGCHK(!l1_ecmp_addrs[p]);
  }

  return sts;
}

bf_status_t mc_mgr_ecmp_add_block_n_one_mbr(
    int sid,
    bf_dev_id_t dev,
    uint32_t offset,
    int add_mask,
    mc_l1_node_t *new_mbr,
    mc_ecmp_grp_t *grp,
    mc_mgr_rdm_addr_list_t **l1_end_addrs,
    mc_mgr_rdm_addr_list_t **port_addrs,
    mc_mgr_rdm_addr_list_t **lag_addrs) {
  bf_status_t sts = BF_SUCCESS;

  int pipe = 0;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    int pm = 1 << pipe;
    if (!(add_mask & pm)) continue;
    uint32_t l1_end_rdm_addr = 0;
    mc_mgr_rdm_addr_pop(l1_end_addrs + pipe, &l1_end_rdm_addr);

    /* Add the block addresses to the vector nodes with the current vector map
     */
    mc_mgr_rdm_write_vec(sid,
                         dev,
                         grp->vector_node_addr[0][pipe],
                         grp->valid_map,
                         l1_end_rdm_addr,
                         grp->ecmp_id);
    mc_mgr_rdm_write_vec(sid,
                         dev,
                         grp->vector_node_addr[1][pipe],
                         grp->valid_map,
                         l1_end_rdm_addr,
                         grp->ecmp_id);

    /* Iterate through every member node and assoiate the HW structures to
     * the new member of the group. */
    uint32_t mbr = 0;
    for (mbr = 0; mbr < 32; ++mbr) {
      if (mbr == offset) {
        /* Write the L2 nodes first keeping track of the RDM address of the
         * first L2 node. */
        uint32_t l2_rdm_ptr = 0;
        write_l2_chain(sid,
                       pipe,
                       new_mbr,
                       lag_addrs + pipe,
                       port_addrs + pipe,
                       &l2_rdm_ptr);
        /* Write the L1 node possibly updating the tree's tail. */
        get_new_l1(new_mbr, pipe, l1_end_rdm_addr + mbr);
        mc_mgr_rdm_write_l1_end(
            sid, dev, l1_end_rdm_addr + offset, l2_rdm_ptr, new_mbr->rid);
      } else {
        mc_l1_node_t *l1_end_node = grp->mbrs[mbr];
        if (!l1_end_node) continue;
        /* There is no L2 chain for these L1-End nodes here. */
        get_new_l1(l1_end_node, pipe, l1_end_rdm_addr + mbr);
        mc_mgr_rdm_write_l1_end(
            sid, dev, l1_end_rdm_addr + mbr, 0, l1_end_node->rid);
      }
    }
    grp->base[pipe] = l1_end_rdm_addr;
  }

  return sts;
}

bf_status_t mc_mgr_ecmp_upd_one_mbr(int sid,
                                    bf_dev_id_t dev,
                                    uint32_t offset,
                                    int upd_mask,
                                    mc_l1_node_t *node,
                                    mc_ecmp_grp_t *grp,
                                    mc_mgr_rdm_addr_list_t **port_addrs,
                                    mc_mgr_rdm_addr_list_t **lag_addrs) {
  bf_status_t sts = BF_SUCCESS;

  /* In update scenario, ECMP block already exist. */
  int pipe = 0;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    int pm = 1 << pipe;
    if (!(upd_mask & pm)) continue;
    uint32_t l2_rdm_ptr = 0;
    write_l2_chain(
        sid, pipe, node, lag_addrs + pipe, port_addrs + pipe, &l2_rdm_ptr);

    /* Write the L1 node possibly updating the tree's tail. */
    get_new_l1(node, pipe, grp->base[pipe] + offset);
    mc_mgr_rdm_write_l1_end(
        sid, dev, grp->base[pipe] + offset, l2_rdm_ptr, node->rid);
  }

  return sts;
}

bf_status_t mc_mgr_write_one_ecmp_mbr(int sid,
                                      bf_dev_id_t dev,
                                      uint32_t *addr,
                                      uint32_t offset,
                                      mc_l1_node_t *mbr,
                                      mc_ecmp_grp_t *grp,
                                      mc_mgr_rdm_addr_list_t **port_addrs,
                                      mc_mgr_rdm_addr_list_t **lag_addrs) {
  bf_status_t sts = BF_SUCCESS;

  int pipe = 0;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    uint32_t l2_addr = 0;
    sts = write_l2_chain(
        sid, pipe, mbr, &(lag_addrs[pipe]), &(port_addrs[pipe]), &l2_addr);

    mbr->hw_nodes[pipe].rdm_addr = addr[pipe] + offset;
    mbr->hw_nodes[pipe].sw_node = mbr;
    mbr->hw_nodes[pipe].prev = NULL;
    mbr->hw_nodes[pipe].next = NULL;
    mc_mgr_rdm_write_l1_end(sid, dev, addr[pipe] + offset, l2_addr, mbr->rid);
  }

  /* The group would be NULL for the dummy ecmp members. */
  if (grp) {
    mc_mgr_ecmp_update_tail_reevaluate(sid, dev, grp);
  }
  return sts;
}

bf_status_t mc_mgr_get_ecmp_blocks(
    int sid,
    bf_dev_id_t dev,
    int size,
    int add_mask,
    mc_mgr_rdm_addr_list_t *l1_end_addrs[MC_MGR_NUM_PIPES]) {
  int pipe = 0;

  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    if (!(add_mask & (1 << pipe))) continue;
    uint32_t l1_end_addr =
        mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_end, size);
    if (!l1_end_addr) goto failure;
    mc_mgr_rdm_addr_append(&l1_end_addrs[pipe], l1_end_addr);
  }
  return BF_SUCCESS;

failure:
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(l1_end_addrs + pipe, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  return BF_NO_SYS_RESOURCES;
}

void mc_mgr_move_mbr_block(int sid,
                           bf_dev_id_t dev,
                           mc_ecmp_grp_t *g,
                           mc_mgr_rdm_addr_list_t *l1_dst[MC_MGR_NUM_PIPES]) {
  int pipe = 0;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    if (!g->base[pipe]) continue;

    MC_MGR_DBGCHK(l1_dst[pipe]);

    /* Retrieve the address the of the new block address. */
    uint32_t l1_block_addr = 0;
    mc_mgr_rdm_addr_pop(&l1_dst[pipe], &l1_block_addr);
    if (!l1_block_addr) {
      MC_MGR_DBGCHK(0);
      return;
    }

    /* Moves the L2 chains from each L1 members from its old L1-block
     * to the new L1 block. */
    int mbr = 0;
    for (mbr = 0; mbr < MC_ECMP_GROUP_MAX_MBR; ++mbr) {
      if (!g->mbrs[mbr]) continue;
      if (mbr >= g->allocated_sz) {
        MC_MGR_DBGCHK(0);
        return;
      }

      if (!g->mbrs[mbr]->hw_nodes[pipe].rdm_addr) continue;

      /* Get start of L2 chain from existing node. */
      uint32_t l2_addr = mc_mgr_rdm_l1_node_get_l2_ptr(
          mc_mgr_ctx_rdm_map(dev), g->mbrs[mbr]->hw_nodes[pipe].rdm_addr);
      /* Update the L1 with it's new address. */
      g->mbrs[mbr]->hw_nodes[pipe].rdm_addr = l1_block_addr + mbr;
      /* Write the L1 at the new address. */
      mc_mgr_rdm_write_l1_end(
          sid, dev, l1_block_addr + mbr, l2_addr, g->mbrs[mbr]->rid);
      LOG_TRACE("Moved group:%#x L2 chains from L1-Block:%#x to L1-Block:%#x",
                g->handle,
                g->base[pipe],
                l1_block_addr);
    }

    /* Note: This is replacing the old lock address with new address, caller
     * is expected to save the old address to return to RDM. */
    g->base[pipe] = l1_block_addr;
  }
}

void mc_mgr_free_mbr_block(int sid, bf_dev_id_t dev, mc_ecmp_grp_t *g) {
  int pipe = 0;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    mc_mgr_rdm_map_enqueue_free(sid, dev, g->base[pipe]);
  }
  g->allocated_sz = 0;
}

/* Repair ECC error in PMT entry.
 * 'address' is the table index from the err_log. */
bf_status_t mc_mgr_ecc_correct_pmt(bf_dev_id_t dev, int ver, uint32_t address) {
  int yid = 0, sid = 0;
  int bit_cnt = 9;
  bf_status_t sts;

  (void)ver;

  mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);

  if (dev >= MC_MGR_NUM_DEVICES) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  yid = address & mc_mgr_get_bit_mask(bit_cnt);
  if (yid >= (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev))) {
    /* An invalid yid lookup could cause an ecc interrupt, ignore the ecc */
    LOG_ERROR(" Invalid yid %d in PMT ECC correct (dev %d)", yid, dev);
    return BF_INVALID_ARG;
  }

  LOG_TRACE("PMT ECC error, dev %d, port %d  ", dev, yid);

  /* Write to both PMT0 and PMT1, the interrupt is thrown for only one even if
     both are corrupt
  */
  sts = mc_mgr_set_pmt_wrl(sid, dev, 0, yid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("PMT update failed (%d) from %s:%d", sts, __func__, __LINE__);
    return sts;
  }
  sts = mc_mgr_set_pmt_wrl(sid, dev, 1, yid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("PMT update failed (%d) from %s:%d", sts, __func__, __LINE__);
    return sts;
  }

  /* Push the write list to correct ecc */
  return mc_mgr_drv_wrl_send(sid, false);
}

void mc_mgr_program_pvt_shadow(bf_dev_id_t dev, uint16_t mgid, uint8_t mask) {
  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return;
  }
  int shift, row;
  mc_mgr_pvt_entry_t pvt_mask;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      /* Calculate the row in the PVT table. */
      row = mgid >> 3;
      /* Calculate the bit shift to reach the desired entry in a word. */
      shift = 4 * (mgid & 0x7);
      /* Calculate a mask to extract the desired entry from the word. */
      pvt_mask.d = 0xFu << shift;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      /* Calculate the row in the PVT table. */
      row = mgid >> 2;
      /* Calculate the bit shift to reach the desired entry in a word. */
      shift = 5 * (mgid & 0x3);
      /* Calculate a mask to extract the desired entry from the word. */
      pvt_mask.d = 0x1Fu << shift;
      /* Include the 5th bit unconditionally. */
      mask |= 0x10;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* Calculate the row in the PVT table. */
      row = mgid >> 3;
      /* Calculate the bit shift to reach the desired entry in a word. */
      shift = 8 * (mgid & 0x7);
      /* Calculate a mask to extract the desired entry from the word. */
      pvt_mask.d = (0xFFllu) << shift;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return;
  }

  /* Construct a new word containing the new value at the correct position. */
  mc_mgr_pvt_entry_t pvt_update = {.d = mask};
  pvt_update.d <<= shift;
  /* Clear the existing entry. */
  mc_mgr_pvt_entry_t x = mc_mgr_ctx_pvt_row(dev, row);
  x.d &= ~pvt_mask.d;
  /* Add the new entry. */
  x.d |= pvt_update.d;
  /* Enable 4 -cell ph for 200G and 100G also. This increases MC latency
     but avoids this performance degradation for MC1:2 513B to ~610B
  */
  bool has_400g_port = mc_mgr_grp_has_400g_200g_100g_port(dev, mgid);
#if defined(EMU_2DIE_USING_SW_2DEV)
  has_400g_port = true;
#endif
  /* Set the 400g map if it has a 400g port */
  if (has_400g_port) {
    x.has_400g_map |= (0x1u << (mgid & 0x7));
  } else {
    x.has_400g_map &= ~(0x1u << (mgid & 0x7));
  }

  mc_mgr_ctx_pvt_set(dev, row, x);
}

/* Mc-mgr sets the PVT mask for tofino3 only
   Pipe-mgr sets the pvt mask for tofino1/tofino2
*/
static bf_status_t mc_mgr_pvt_msk_set(int sid,
                                      uint8_t dev,
                                      int mgid_grp,
                                      mc_mgr_pvt_entry_t pvt_entry,
                                      bool push,
                                      bool complete) {
  bf_status_t bf_sts = BF_SUCCESS;
  uint32_t pipe_cnt = 0;
#if defined(EMU_2DIE_USING_SW_2DEV)
  pipe_cnt = mc_mgr_ctx_num_max_pipes(dev);
#else
  lld_sku_get_num_active_pipes(dev, &pipe_cnt);
#endif

  (void)complete;
  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }

  /* Each subdevice view their local pipes as [0..3]. Hence, we need to
   * change each MGID pvt_value(8bits) to local view of each subdevice.
   * TODO: Change the shift to subdev basis later. */
  mc_mgr_pvt_entry_t masks[BF_MAX_SUBDEV_COUNT] = {0};
  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev); subdev++) {
    uint8_t shift = BF_SUBDEV_PIPE_COUNT * 0;
    for (uint32_t idx = 0; idx < 8; ++idx) {
      /* 0xFF is because of 8MGIDs per pvt_entry */
      uint64_t pvt_mask = 0xFFlu << (8 * idx);
      uint64_t pvt_val = (pvt_entry.d & pvt_mask) >> (8 * idx);
      uint64_t subdev_val =
          (((pvt_val & 0x0F) << (shift)) | ((pvt_val & 0xF0) >> shift)) &
          0xFFlu;
      masks[subdev].d |= subdev_val << (8 * idx);
      masks[subdev].has_400g_map = pvt_entry.has_400g_map;
    }
  }

  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev); subdev++) {
    for (uint32_t pipe = 0; pipe < pipe_cnt; ++pipe) {
      uint64_t tbl0_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pvt_table_mgid1_tbl(
              pipe, mgid_grp);
      uint64_t tbl1_addr =
          tof3_mem_tm_tm_wac_wac_pipe_mem_csr_memory_wac_pvt_table_mgid2_tbl(
              pipe, mgid_grp);

      // convert software presentation into hardware presentation
      uint64_t data_lo = 0;

      // The last mgid bit 0 will be 63 bit in data_lo, bits 1-8 will be
      // 64-71 (8-bits) in data_hi

      // The last mgid bit 0
      data_lo |= ((masks[subdev].d >> (8 * 7)) & 0x1) << 63;
      uint64_t data_hi =
          (((masks[subdev].d >> (8 * 7 + 1)) & 0x7f) |
           (((masks[subdev].has_400g_map >> (1 * 7)) & 0x1) << 7)) &
          0xff;

      for (int i = 0; i < 7; i++) {
        uint64_t mgidmap =
            (((masks[subdev].d >> (8 * i)) & 0xff) |
             (((masks[subdev].has_400g_map >> (1 * i)) & 0x1) << 8)) &
            0x1ff;
        data_lo |= mgidmap << (9 * i);
      }

      if (mc_mgr_drv_wrl_append(dev,
                                subdev,
                                sid,
                                16,
                                tbl0_addr,
                                data_hi,
                                data_lo,
                                __func__,
                                __LINE__)) {
        MC_MGR_DBGCHK(0);
        return BF_NO_SYS_RESOURCES;
      }

      if (mc_mgr_drv_wrl_append(dev,
                                subdev,
                                sid,
                                16,
                                tbl1_addr,
                                data_hi,
                                data_lo,
                                __func__,
                                __LINE__)) {
        MC_MGR_DBGCHK(0);
        return BF_NO_SYS_RESOURCES;
      }
    }
  }

  if (push) {
    bf_sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != bf_sts) {
      LOG_ERROR("Failed to push mc_mgr_set-pvt-mask instruction list (%d)",
                bf_sts);
      MC_MGR_DBGCHK(BF_SUCCESS == bf_sts);
      goto done;
    }
  }
done:
  return bf_sts;
}

bf_status_t mc_mgr_program_pvt(int sid,
                               bf_dev_id_t dev,
                               uint16_t mgid,
                               uint8_t mask,
                               uint32_t tbl_msk,
                               bool batch,
                               const char *where,
                               const int line) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (tbl_msk & ~(MC_PVT_MASK_ALL)) {
    return BF_INVALID_ARG;
  }

  /* Save original value in case of failure. */
  uint8_t old_pvt = mc_mgr_get_pvt(dev, mgid);

  /* Update the software shadow. */
  mc_mgr_program_pvt_shadow(dev, mgid, mask);

  /* Update hardware if needed. */
  if (!mc_mgr_ctx_dev_locked(dev) || mc_mgr_ctx_rebuilding(dev)) {
    int mgid_per_row = BF_MGID_COUNT / mc_mgr_ctx_pvt_sz(dev);
    int pvt_row = mgid / mgid_per_row;

    bf_status_t sts = mc_mgr_pvt_write_row_from_shadow(
        dev, sid, BF_DEV_PIPE_ALL, pvt_row, tbl_msk, batch);

    /* If the HW update fails, roll back our update to the shadow as well. */
    if (BF_SUCCESS != sts) {
      LOG_ERROR("PVT update failed (%d) from %s:%d", sts, where, line);
      mc_mgr_program_pvt_shadow(dev, mgid, old_pvt);
      return sts;
    }
  }
  return BF_SUCCESS;
}

void mc_mgr_program_tvt_shadow(uint8_t dev, uint16_t mgid, uint8_t mask) {
  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return;
  }
  int shift, row;
  mc_mgr_tvt_entry_t tvt_mask;
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO3:
      /* Calculate the row in the tvt table. */
      row = mgid >> 2;
      /* Calculate the bit shift to reach the desired entry in a word. */
      shift = 4 * (mgid % 4);
      /* Calculate a mask to extract the desired entry from the word. */
      tvt_mask.diemap = 0xFu << shift;
      break;
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    default:
      MC_MGR_DBGCHK(0);
      return;
  }

  /* Construct a new word containing the new value at the correct position. */
  mc_mgr_tvt_entry_t tvt_update = {.diemap = mask};
  tvt_update.diemap <<= shift;
  /* Clear the existing entry. */
  mc_mgr_tvt_entry_t x = mc_mgr_ctx_tvt_row(dev, row);
  x.diemap &= ~tvt_mask.diemap;
  /* Add the new entry. */
  x.diemap |= tvt_update.diemap;
  mc_mgr_ctx_tvt_set(dev, row, x);
}

bf_status_t mc_mgr_program_tvt(int sid,
                               uint8_t dev,
                               uint16_t mgid,
                               uint8_t mask,
                               bool batch,
                               const char *where,
                               const int line) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }


  if (mgid == 0 && (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3)) {
    mask = 0x1;
  }

  /* Save original value in case of failure. */
  uint8_t old_tvt = mc_mgr_get_tvt(dev, mgid);

  /* Update the software shadow. */
  mc_mgr_program_tvt_shadow(dev, mgid, mask);

  if (!mc_mgr_ctx_dev_locked(dev) || mc_mgr_ctx_rebuilding(dev)) {
    mc_mgr_tvt_entry_t row_data = {0};
    int row;
    row = mgid >> 2;
    row_data = mc_mgr_get_tvt_row(dev, row);

    /* Write the new table entry to hardware. */
    pipe_status_t sts;
    dev_target_t dev_tgt = {dev, DEV_PIPE_ALL};
    sts = pipe_mgr_mc_pipe_tvt_msk_set(mc_mgr_ctx_pipe_sess(),
                                       dev_tgt,
                                       row,
                                       row_data,
                                       !batch,
                                       true,
                                       mc_mgr_ctx_rebuilding(dev));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("TVT update failed (%d) from %s:%d", sts, where, line);
      mc_mgr_program_tvt_shadow(dev, mgid, old_tvt);
      return BF_NO_SYS_RESOURCES;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_pvt_write_row_from_shadow(bf_dev_id_t dev,
                                             int sid,
                                             int pipe,
                                             uint16_t row,
                                             uint32_t tbl_msk,
                                             bool batch) {
  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  mc_mgr_pvt_entry_t row_data = {.d = 0};
  switch (mc_mgr_ctx_dev_family(dev)) {
    case BF_DEV_FAMILY_TOFINO:
      row_data.d = mc_mgr_get_pvt_row(dev, row & 0x0FFF).d |
                   mc_mgr_get_pvt_row(dev, row | 0x1000).d;
      row &= 0xFFF;
      break;
    case BF_DEV_FAMILY_TOFINO2: {
      /* Tofino2 will use logical pipe ids in the PVT. */
      row_data = mc_mgr_get_pvt_row(dev, row);
      /* For each of the four MGIDs in the PVT row... */
      for (int i = 0; i < 4; ++i) {
        uint32_t phy_entry = (row_data.d >> (5 * i)) & 0x1F;
        uint32_t log_entry = 0;
        /* Check if a physical pipe is set, if so convert it to logical. */
        for (int p = 0; p < MC_MGR_NUM_PIPES; ++p) {
          if ((phy_entry & (1u << p)) == 0) continue;
          bf_dev_pipe_t log_pipe = p;
          lld_sku_map_phy_pipe_id_to_pipe_id(dev, p, &log_pipe);
          log_entry |= 1u << log_pipe;
        }
        /* Preserve the 5th bit of each entry. */
        log_entry |= phy_entry & 0x10;

        row_data.d |= log_entry << (i * 5);
      }
      break;
    }
    case BF_DEV_FAMILY_TOFINO3: {
      bf_sku_chip_part_rev_t rev = BF_SKU_CHIP_PART_REV_B0;
      lld_sku_get_chip_part_revision_number(dev, &rev);
      row_data = mc_mgr_get_pvt_row(dev, row);
      break;
    }
    default:
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
  }

  bf_status_t sts = BF_SUCCESS;
  /* Write the new table entry to hardware. */
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    sts = mc_mgr_pvt_msk_set(sid, dev, row, row_data, !batch, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("PVT update failed (%d) ", sts);
      mc_mgr_program_pvt_shadow(dev, row, row_data.d);
      return BF_NO_SYS_RESOURCES;
    }
  } else {
    dev_target_t dev_tgt = {dev, pipe};
    sts = pipe_mgr_mc_pipe_msk_set(mc_mgr_ctx_pipe_sess(),
                                   dev_tgt,
                                   row,
                                   row_data,
                                   tbl_msk,
                                   !batch,
                                   false,
                                   mc_mgr_ctx_rebuilding(dev));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("PVT row write failed (%d) ", sts);
      return BF_NO_SYS_RESOURCES;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_tvt_write_row_from_shadow(bf_dev_id_t dev,
                                             bf_dev_pipe_t pipe,
                                             uint16_t row,
                                             bool batch) {
  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }

  mc_mgr_tvt_entry_t row_data = {0};
  row_data = mc_mgr_get_tvt_row(dev, row);

  /* Write the new table entry to hardware. */
  pipe_status_t sts;
  dev_target_t dev_tgt = {dev, pipe};
  sts = pipe_mgr_mc_pipe_tvt_msk_set(mc_mgr_ctx_pipe_sess(),
                                     dev_tgt,
                                     row,
                                     row_data,
                                     !batch,
                                     true,
                                     mc_mgr_ctx_rebuilding(dev));
  if (PIPE_SUCCESS != sts) {
    LOG_ERROR("TVT row write failed (%d)", sts);
    return BF_NO_SYS_RESOURCES;
  }
  return BF_SUCCESS;
}

/* Repair ECC error in PVT entry. */
bf_status_t mc_mgr_ecc_correct_pvt(bf_dev_id_t dev,
                                   bf_dev_pipe_t pipe,
                                   uint16_t row,
                                   bool batch) {
  int sid = 0;
  if (mc_mgr_ctx_dev_family(dev) == BF_DEV_FAMILY_TOFINO3) {
    mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);
  } else {
    sid = mc_mgr_ctx_pipe_sess();
  }
  return mc_mgr_pvt_write_row_from_shadow(
      dev, sid, pipe, row, MC_PVT_MASK_ALL, batch);
}

/* Repair ECC error in LIT_BM entry.
 * 'address' is the table index (lag_id) from the err_log. */
bf_status_t mc_mgr_ecc_correct_lit_bm(bf_dev_id_t dev,
                                      int ver,
                                      uint32_t address) {
  bf_status_t sts = BF_SUCCESS;
  int lag_id = 0, sid = 0;
  int bit_cnt = log2(BF_LAG_COUNT);

  mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);

  if (dev >= MC_MGR_NUM_DEVICES) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (ver != 0 && ver != 1) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Use only required no of bits from address */
  lag_id = address & mc_mgr_get_bit_mask(bit_cnt);
  if ((lag_id < 0) || (lag_id >= BF_LAG_COUNT)) {
    /* An invalid lag-id lookup could cause an ecc interrupt, ignore the ecc
     */
    LOG_ERROR(" Invalid lagid %d in LIT BM ECC correct (dev %d)", lag_id, dev);
    return BF_INVALID_ARG;
  }

  LOG_TRACE("LIT BM ECC error, dev %d, lag_id %d  ", dev, lag_id);

  sts = mc_mgr_set_lit_wrl(sid, dev, ver, lag_id);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program from shadow (lag %d) %s:%d",
              lag_id,
              __func__,
              __LINE__);
    return sts;
  }

  /* Push the write list to correct ecc */
  return mc_mgr_drv_wrl_send(sid, false);
}

/* Repair ECC error in LIT_NP entry.
 * 'address' is the table index (lag_id) from the err_log. */
bf_status_t mc_mgr_ecc_correct_lit_np(bf_dev_id_t dev,
                                      int ver,
                                      uint32_t address) {
  int lag_id = 0, sid = 0;
  bf_status_t sts;
  int bit_cnt = log2(BF_LAG_COUNT);

  mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid);

  if (dev >= MC_MGR_NUM_DEVICES) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (ver != 0 && ver != 1) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  lag_id = address & mc_mgr_get_bit_mask(bit_cnt);
  if ((lag_id < 0) || (lag_id >= BF_LAG_COUNT)) {
    /* An invalid lag-id lookup could cause an ecc interrupt, ignore the ecc
     */
    LOG_ERROR(" Invalid lagid %d in LIT NP ECC correct (dev %d)", lag_id, dev);
    return BF_INVALID_ARG;
  }

  LOG_TRACE("LIT NP ECC error, dev %d, lag_id %d  ", dev, lag_id);

  sts = mc_mgr_set_lit_np_wrl(sid, dev, ver, lag_id);
  if (sts != BF_SUCCESS) {
    LOG_ERROR(
        "ECC: LIT NP update failed (%d) at %s:%d", sts, __func__, __LINE__);
    return sts;
  }

  /* Push the write list to correct ecc */
  return mc_mgr_drv_wrl_send(sid, false);
}

bf_status_t mc_mgr_program_tbl_ver(int sid, bf_dev_id_t dev, bool ver) {
  if (0 > sid || MC_MGR_NUM_SESSIONS <= sid) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (MC_MGR_NUM_DEVICES <= dev) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (!mc_mgr_ctx_dev_locked(dev) || mc_mgr_ctx_rebuilding(dev)) {
    pipe_status_t sts;
    dev_target_t dev_tgt = {dev, DEV_PIPE_ALL};
    sts = pipe_mgr_mc_tbl_ver_set(
        mc_mgr_ctx_pipe_sess(), dev_tgt, ver, mc_mgr_ctx_rebuilding(dev));
    if (PIPE_SUCCESS != sts) {
      LOG_ERROR("Tbl Ver update failed (%d) at %s:%d", sts, __func__, __LINE__);
      return BF_NO_SYS_RESOURCES;
    }
  }
  mc_mgr_ctx_tbl_ver_set(dev, ver);
  return BF_SUCCESS;
}
