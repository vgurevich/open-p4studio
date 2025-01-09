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


/* Module header files */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>

/* Local header files */
#include "pipe_mgr_alpm.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_tcam_hw.h"
#include "pipe_mgr_tcam_ha.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_rmt_cfg.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_hitless_ha.h"

/* Global header files */
#include <math.h>
#include <netinet/in.h>
static alpm_mgr_ctx_t alpm_ctx;
static alpm_mgr_ctx_t *alpm_ctx_p;
static bool enable_free_inactive_nodes = false;

static alpm_mgr_ctx_t *alpm_mgr_ctx(void) { return alpm_ctx_p; }

pipe_mat_ent_hdl_t pipe_mgr_alpm_allocate_handle(alpm_pipe_tbl_t *pipe_tbl) {
  int ent_hdl;
  ent_hdl = bf_id_allocator_allocate(pipe_tbl->ent_hdl_allocator);
  if (ent_hdl == -1) {
    return PIPE_ALPM_INVALID_ENT_HDL;
  }

  if (pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
    ent_hdl = PIPE_SET_HDL_PIPE(ent_hdl, pipe_tbl->pipe_id);
  }
  return (pipe_mat_ent_hdl_t)ent_hdl;
}

/*
 * Usage: find_covering_prefix(node)
 * ---------------------------------
 * Finds and returns the covering prefix of the given node, or NULL if
 * none are found.
 */
static trie_node_t *find_covering_prefix(trie_node_t *node) {
  trie_node_t *curr = node->parent;
  while (curr && !curr->entry) {
    curr = curr->parent;
  }
  return curr;
}
static trie_node_t *find_covering_prefix_with_node(trie_node_t *node,
                                                   trie_node_t *stop) {
  trie_node_t *curr = node->parent;
  while (curr && !curr->entry && curr != stop) {
    curr = curr->parent;
  }
  return curr;
}

/******************************************************
 *           Data Structure Duplication Fns           *
 ******************************************************/
static alpm_entry_t *copy_alpm_entry(alpm_entry_t *entry) {
  if (!entry) {
    return NULL;
  }

  alpm_entry_t *new_entry = PIPE_MGR_CALLOC(1, sizeof(alpm_entry_t));
  PIPE_MGR_MEMCPY(new_entry, entry, sizeof(alpm_entry_t));
  new_entry->match_spec = NULL;
  new_entry->act_spec = NULL;
  new_entry->match_spec =
      pipe_mgr_tbl_copy_match_spec(new_entry->match_spec, entry->match_spec);
  new_entry->act_spec =
      pipe_mgr_tbl_copy_action_spec(new_entry->act_spec, entry->act_spec);

  return new_entry;
}

static trie_subtree_t *copy_subtree(trie_subtree_t *subtree) {
  if (!subtree) {
    return NULL;
  }

  trie_subtree_t *new_subtree = PIPE_MGR_CALLOC(1, sizeof(trie_subtree_t));
  PIPE_MGR_MEMCPY(new_subtree, subtree, sizeof(trie_subtree_t));
  new_subtree->cov_pfx_entry = copy_alpm_entry(subtree->cov_pfx_entry);

  return new_subtree;
}

static trie_node_t **copy_node_array(trie_node_t **arr, uint32_t count) {
  if (count == 0) {
    return NULL;
  }

  trie_node_t **new_arr = PIPE_MGR_CALLOC(count, sizeof(trie_node_t *));
  PIPE_MGR_MEMCPY(new_arr, arr, sizeof(trie_node_t *) * count);
  return new_arr;
}

static trie_node_t *copy_node(trie_node_t *dst, trie_node_t *src) {
  if (!src) {
    return NULL;
  }

  trie_node_t *new_node = dst;
  if (!new_node) {
    new_node = PIPE_MGR_CALLOC(1, sizeof(trie_node_t));
  }

  PIPE_MGR_MEMCPY(new_node, src, sizeof(trie_node_t));
  new_node->entry = copy_alpm_entry(src->entry);
  new_node->subtree = copy_subtree(src->subtree);
  new_node->cov_pfx_subtree_nodes =
      copy_node_array(src->cov_pfx_subtree_nodes, src->cov_pfx_arr_size);

  return new_node;
}

static partition_info_t *copy_partition_info(partition_info_t *ptn,
                                             uint32_t subtrees_arr_size) {
  if (!ptn) {
    return NULL;
  }

  partition_info_t *new_ptn = PIPE_MGR_CALLOC(1, sizeof(partition_info_t));

  PIPE_MGR_MEMCPY(new_ptn, ptn, sizeof(partition_info_t));
  new_ptn->subtree_nodes =
      copy_node_array(ptn->subtree_nodes, subtrees_arr_size);

  return new_ptn;
}

/******************************************************
 *               Data Structure Free Fns              *
 ******************************************************/
static void free_match_spec(pipe_tbl_match_spec_t *match_spec) {
  if (match_spec) {
    if (match_spec->match_mask_bits) {
      PIPE_MGR_FREE(match_spec->match_mask_bits);
    }

    if (match_spec->match_value_bits) {
      PIPE_MGR_FREE(match_spec->match_value_bits);
    }

    PIPE_MGR_FREE(match_spec);
  }
}

static void free_action_spec(pipe_action_spec_t *act_spec) {
  if (act_spec) {
    if (act_spec->act_data.action_data_bits) {
      PIPE_MGR_FREE(act_spec->act_data.action_data_bits);
    }

    PIPE_MGR_FREE(act_spec);
  }
}

static void free_entry(alpm_entry_t *entry) {
  if (entry) {
    free_match_spec(entry->match_spec);
    free_action_spec(entry->act_spec);

    PIPE_MGR_FREE(entry);
  }
}

static void free_subtree(trie_subtree_t *subtree) {
  if (subtree) {
    free_entry(subtree->cov_pfx_entry);
    PIPE_MGR_FREE(subtree);
  }
}

static void free_node_internal(trie_node_t *node) {
  if (node) {
    if (node->entry) {
      free_entry(node->entry);
      node->entry = NULL;
    }

    if (node->subtree) {
      free_subtree(node->subtree);
      node->subtree = NULL;
    }

    if (node->cov_pfx_subtree_nodes) {
      PIPE_MGR_FREE(node->cov_pfx_subtree_nodes);
      node->cov_pfx_subtree_nodes = NULL;
    }
  }
}

static void free_node(trie_node_t *node) {
  if (node) {
    free_node_internal(node);
    PIPE_MGR_FREE(node);
  }
}

/******************************************************
 *              Data Structure Backup Fns             *
 ******************************************************/
static void backup_node(alpm_pipe_tbl_t *pipe_tbl, trie_node_t *node) {
  if (!ALPM_SESS_IS_TXN(pipe_tbl) || !node || node->backed_up) {
    return;
  }
  node->backed_up = true;

  backup_node_t *backup = PIPE_MGR_CALLOC(1, sizeof(backup_node_t));
  backup->node = node;
  backup->backup_node = copy_node(NULL, node);

  backup->next = pipe_tbl->backup_node_list;
  pipe_tbl->backup_node_list = backup;
}

static void backup_ptn(alpm_pipe_tbl_t *pipe_tbl, partition_info_t *p_info) {
  if (!ALPM_SESS_IS_TXN(pipe_tbl) || !p_info || p_info->backed_up) {
    return;
  }
  /* Overall partition list backups */
  if (pipe_tbl->backup_partitions_in_use == -1) {
    pipe_tbl->backup_partitions_in_use = pipe_tbl->partitions_in_use;
    pipe_tbl->backup_ptn_in_use_list = pipe_tbl->ptn_in_use_list;
    pipe_tbl->backup_ptn_free_list = pipe_tbl->ptn_free_list;
  }

  p_info->backed_up = true;

  backup_ptn_t *backup = PIPE_MGR_CALLOC(1, sizeof(backup_ptn_t));
  backup->backup = copy_partition_info(
      p_info, pipe_tbl->alpm_tbl_info->max_subtrees_per_partition);

  /*
   * TODO: Add code to deep copy the ID allocator while doing backup of
   * partition. Or use different mechanism for subtree id management
   */

  backup->next = pipe_tbl->backup_ptn_list;
  pipe_tbl->backup_ptn_list = backup;
}

/*
 * Usage: add_subtree_to_cp_list(pipe_tbl, cp_node, subtree)
 * ---------------------------------------------------------
 * Assigns the subtree to its associated covering prefix node.
 */
static void add_subtree_to_cp_list(alpm_pipe_tbl_t *pipe_tbl,
                                   trie_node_t *cp,
                                   trie_subtree_t *subtree) {
  if (!pipe_tbl || !cp || !subtree) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return;
  }

  backup_node(pipe_tbl, cp);
  backup_node(pipe_tbl, subtree->node);

  subtree->cov_pfx = cp;
  subtree->cov_pfx_entry = copy_alpm_entry(cp->entry);
  if (subtree->cov_pfx_entry) {
    if (subtree->cov_pfx_entry->match_spec &&
        pipe_tbl->alpm_tbl_info->atcam_subset_key_width) {
      subtree->cov_pfx_entry->match_spec->version_bits =
          pipe_tbl->alpm_tbl_info->cp_ver_bits;
    }
    subtree->cov_pfx_entry->sram_entry_hdl = 0;
    subtree->cov_pfx_entry->ttl = 0;
  }

  if (cp->cov_pfx_count == cp->cov_pfx_arr_size) {
    cp->cov_pfx_arr_size++;
    cp->cov_pfx_subtree_nodes = (trie_node_t **)PIPE_MGR_REALLOC(
        cp->cov_pfx_subtree_nodes,
        cp->cov_pfx_arr_size * sizeof(trie_node_t *));
  }
  cp->cov_pfx_subtree_nodes[cp->cov_pfx_count] = subtree->node;
  cp->cov_pfx_count++;
}

/*
 * Usage: remove_subtree_from_cp_list(pipe_tbl, cp_node, subtree)
 * --------------------------------------------------------------
 * Detaches the subtree from its associated covering prefix node.
 */
static void remove_subtree_from_cp_list(alpm_pipe_tbl_t *pipe_tbl,
                                        trie_node_t *cp,
                                        trie_subtree_t *subtree) {
  backup_node(pipe_tbl, cp);
  backup_node(pipe_tbl, subtree->node);

  uint32_t i;
  bool found = false;
  for (i = 0; i < cp->cov_pfx_count; i++) {
    if (!found) {
      if (cp->cov_pfx_subtree_nodes[i] == subtree->node) {
        found = true;
      }
    } else {
      cp->cov_pfx_subtree_nodes[i - 1] = cp->cov_pfx_subtree_nodes[i];
    }
  }

  if (found) {
    cp->cov_pfx_count--;
  }

  if (subtree->cov_pfx == cp) {
    free_entry(subtree->cov_pfx_entry);
    subtree->cov_pfx = NULL;
    subtree->cov_pfx_entry = NULL;
  }
}

/*
 * Usage: create_node(parent_node, is_left)
 * ----------------------------------------
 * Allocates a new node and inserts it into the trie.
 */
static trie_node_t *create_node(trie_node_t *parent, bool is_left_child) {
  trie_node_t *node = (trie_node_t *)PIPE_MGR_CALLOC(1, sizeof(trie_node_t));

  if (parent) {
    node->parent = parent;
    node->depth = parent->depth + 1;
    if (is_left_child) {
      parent->left_child = node;
    } else {
      parent->right_child = node;
    }
  }

  return node;
}

/*
 * Usage: create_subtree(pipe_tbl, subtree_node, p_info)
 * -----------------------------------------------------
 * Allocates a new subtree root at the given node and sets up corresponding
 * trie information.
 */
static trie_subtree_t *create_subtree(alpm_pipe_tbl_t *pipe_tbl,
                                      trie_node_t *node,
                                      partition_info_t *p_info) {
  trie_subtree_t *new_subtree =
      (trie_subtree_t *)PIPE_MGR_CALLOC(1, sizeof(trie_subtree_t));
  new_subtree->node = node;
  new_subtree->partition = p_info;

  if (node) {
    PIPE_MGR_DBGCHK(!node->subtree);

    node->subtree = new_subtree;
    if (!node->entry) {
      trie_node_t *cp = find_covering_prefix(node);
      if (cp) {
        add_subtree_to_cp_list(pipe_tbl, cp, new_subtree);
      }
    }
  }

  return new_subtree;
}

/*
 * Usage: remove_subtree_from_partition(pipe_tbl, p_info, subtree)
 * ---------------------------------------------------------------
 * Removes the subtree from the given partition's list of subtrees.
 */
static void remove_subtree_from_partition(alpm_pipe_tbl_t *pipe_tbl,
                                          partition_info_t *p_info,
                                          trie_subtree_t *subtree) {
  backup_node(pipe_tbl, subtree->node);
  backup_ptn(pipe_tbl, p_info);

  uint32_t i;
  bool found = false;
  for (i = 0; i < p_info->num_subtrees; i++) {
    if (!found) {
      if (p_info->subtree_nodes[i] == subtree->node) {
        found = true;
      }
    } else {
      p_info->subtree_nodes[i - 1] = p_info->subtree_nodes[i];
    }
  }

  if (found) {
    p_info->num_subtrees--;
  }
  subtree->partition = NULL;
}

/*
 * Usage: remove_subtree(pipe_tbl, subtree)
 * ----------------------------------------
 * Removes a subtree root from any relevant lists.
 */
static void remove_subtree(alpm_pipe_tbl_t *pipe_tbl, trie_subtree_t *subtree) {
  backup_node(pipe_tbl, subtree->node);

  if (subtree->partition) {
    remove_subtree_from_partition(pipe_tbl, subtree->partition, subtree);
  }

  if (subtree->cov_pfx_entry) {
    remove_subtree_from_cp_list(pipe_tbl, subtree->cov_pfx, subtree);
  }
}

/*
 * Usage: create_partition_info(pipe_tbl, subtree)
 * -----------------------------------------------
 * Creates a data structure for a new partition and prepares it for use.
 */
static partition_info_t *create_partition_info(alpm_pipe_tbl_t *pipe_tbl,
                                               trie_subtree_t *subtree) {
  partition_info_t *new_partition = pipe_tbl->ptn_free_list;
  PIPE_MGR_DBGCHK(new_partition);
  PIPE_MGR_DBGCHK(new_partition->size == 0 && new_partition->num_subtrees == 0);

  backup_ptn(pipe_tbl, new_partition);
  backup_ptn(pipe_tbl, pipe_tbl->ptn_in_use_list);

  pipe_tbl->ptn_free_list = new_partition->next;
  new_partition->next = pipe_tbl->ptn_in_use_list;
  if (pipe_tbl->ptn_in_use_list) {
    pipe_tbl->ptn_in_use_list->prev = new_partition;
  }
  pipe_tbl->ptn_in_use_list = new_partition;

  pipe_tbl->partitions_in_use++;

  if (subtree) {
    new_partition->num_subtrees = 1;
    new_partition->subtree_nodes[0] = subtree->node;
    subtree->partition = new_partition;
  }

  return new_partition;
}

/*
 * Usage: remove_partition_info(pipe_tbl, p_info)
 * ----------------------------------------------
 * Moves the given partition from the in-use list to the free list.
 * Partition must be empty, with no subtrees pointing to it.
 */
static void remove_partition_info(alpm_pipe_tbl_t *pipe_tbl,
                                  partition_info_t *p_info) {
  PIPE_MGR_DBGCHK(p_info->size == 0 && p_info->num_subtrees == 0);
  backup_ptn(pipe_tbl, p_info);
  backup_ptn(pipe_tbl, p_info->prev);
  backup_ptn(pipe_tbl, p_info->next);

  if (p_info->prev) {
    p_info->prev->next = p_info->next;
  } else {
    pipe_tbl->ptn_in_use_list = p_info->next;
  }
  if (p_info->next) {
    p_info->next->prev = p_info->prev;
  }

  p_info->next = pipe_tbl->ptn_free_list;
  p_info->prev = NULL;
  pipe_tbl->ptn_free_list = p_info;

  pipe_tbl->partitions_in_use--;
}

static inline alpm_pipe_tbl_t *get_pipe_tbl_by_pipe_id(
    alpm_tbl_info_t *tbl_info, bf_dev_pipe_t pipe_id) {
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  uint32_t i;

  for (i = 0; i < tbl_info->num_pipes; i++) {
    pipe_tbl = tbl_info->pipe_tbls[i];
    if (pipe_tbl->pipe_id == pipe_id) {
      return pipe_tbl;
    }
  }
  return NULL;
}

static inline alpm_pipe_tbl_t *get_pipe_tbl_instance(alpm_tbl_info_t *tbl_info,
                                                     bf_dev_pipe_t pipe_id,
                                                     const char *where,
                                                     const int line) {
  alpm_pipe_tbl_t *pipe_tbl = NULL;

  if (tbl_info->is_symmetric) {
    pipe_tbl = tbl_info->pipe_tbls[0];
  } else {
    pipe_tbl = get_pipe_tbl_by_pipe_id(tbl_info, pipe_id);
    if (!pipe_tbl) {
      LOG_ERROR("%s:%d Pipe table instance with id %d not found",
                where,
                line,
                pipe_id);
    }
  }
  return pipe_tbl;
}

alpm_pipe_tbl_t *get_pipe_tbl_inst_log(alpm_tbl_info_t *tbl_info,
                                       bf_dev_pipe_t pipe_id,
                                       const char *where,
                                       const int line) {
  alpm_pipe_tbl_t *pipe_tbl = NULL;

  if (tbl_info->is_symmetric) {
    if (pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid request to get an asymmetric entry (pipe %d)"
          " in a symmetric table %d device %d ",
          where,
          line,
          pipe_id,
          tbl_info->alpm_tbl_hdl,
          tbl_info->dev_id);
    }
    pipe_tbl = tbl_info->pipe_tbls[0];
  } else {
    if (pipe_id == BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d ALPM tbl 0x%x, Invalid pipe %d specified for asymmetric tbl, "
          "dev %d",
          where,
          line,
          tbl_info->alpm_tbl_hdl,
          pipe_id,
          tbl_info->dev_id);
      return NULL;
    }
    pipe_tbl = get_pipe_tbl_by_pipe_id(tbl_info, pipe_id);
  }

  if (!pipe_tbl) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Pipe table instance with id %d not found",
              where,
              line,
              tbl_info->name,
              tbl_info->dev_id,
              tbl_info->alpm_tbl_hdl,
              pipe_id);
  }

  return pipe_tbl;
}

/*
 * Usage: pipe_mgr_alpm_tbl_info_get(device_id, mat_tbl_hdl)
 * ---------------------------------------------------------
 * Fetches the alpm tbl info associated with the handle from the global
 * context map.
 */
alpm_tbl_info_t *pipe_mgr_alpm_tbl_info_get(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t tbl_hdl) {
  alpm_tbl_info_t *alpm_tbl_info = NULL;

  if (dev_id >= PIPE_MGR_NUM_DEVICES) {
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  bf_map_sts_t msts;
  msts = bf_map_get(&alpm_mgr_ctx()->tbl_hdl_to_tbl_map[dev_id],
                    tbl_hdl,
                    (void **)&alpm_tbl_info);

  if (msts != BF_MAP_OK) {
    return NULL;
  }

  return alpm_tbl_info;
}

alpm_tbl_info_t *pipe_mgr_alpm_tbl_info_get_from_ll_hdl(
    bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t ll_tbl_hdl) {
  bf_map_sts_t msts = BF_MAP_OK;
  unsigned long key;
  alpm_tbl_info_t *alpm_tbl_info = NULL;

  msts = bf_map_get_first(&alpm_mgr_ctx()->tbl_hdl_to_tbl_map[dev_id],
                          &key,
                          (void **)&alpm_tbl_info);
  while (msts == BF_MAP_OK && alpm_tbl_info) {
    if (alpm_tbl_info->preclass_tbl_hdl == ll_tbl_hdl ||
        alpm_tbl_info->atcam_tbl_hdl == ll_tbl_hdl) {
      break;
    }
    msts = bf_map_get_next(&alpm_mgr_ctx()->tbl_hdl_to_tbl_map[dev_id],
                           &key,
                           (void **)&alpm_tbl_info);
  }
  if (msts != BF_MAP_OK) {
    return NULL;
  }

  return alpm_tbl_info;
}

static void build_alpm_move_list_hdr(alpm_pipe_tbl_t *pipe_tbl,
                                     enum pipe_mat_update_type op,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     pipe_mgr_move_list_t *ml_head,
                                     pipe_mat_ent_hdl_t atcam_ent_hdl,
                                     pipe_mat_ent_hdl_t alpm_ent_hdl,
                                     bool cov_pfx) {
  if (!ml_head) {
    return;
  }
  pipe_mgr_alpm_move_list_hdr_t *ml_hdr =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_alpm_move_list_hdr_t));
  ml_hdr->op = op;
  ml_hdr->tbl_hdl = tbl_hdl;
  ml_hdr->ml_head = ml_head;
  ml_hdr->atcam_ent_hdl = atcam_ent_hdl;
  ml_hdr->alpm_ent_hdl = alpm_ent_hdl;
  ml_hdr->cov_pfx = cov_pfx;

  if (!pipe_tbl->ml_hdr) {
    pipe_tbl->ml_hdr = ml_hdr;
    pipe_tbl->ml_hdr_tail = ml_hdr;
  } else {
    pipe_tbl->ml_hdr_tail->next = ml_hdr;
    pipe_tbl->ml_hdr_tail = ml_hdr;
  }
}

/* Update the local state corresponding to the stuff that is not read from
 * hardware - entry priority, ttl values based on the API replay
 */
static pipe_status_t pipe_mgr_alpm_entry_update_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t msts = BF_MAP_OK;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node = NULL;
  bf_dev_pipe_t pipe_id;
  pipe_mat_ent_hdl_t ent_hdl = move_list_node->entry_hdl;
  pipe_tbl_match_spec_t *match_spec =
      unpack_mat_ent_data_ms(move_list_node->data);
  uint32_t ttl = unpack_mat_ent_data_ttl(move_list_node->data);
  uint32_t i = 0;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d Alpm table not found for handle 0x%x device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_id = PIPE_GET_HDL_PIPE(ent_hdl);
  pipe_tbl = get_pipe_tbl_instance(tbl_info, pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  msts = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, ent_hdl, (void **)&node);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d Unable to find alpm entry %d in table 0x%x device %d",
              __func__,
              __LINE__,
              ent_hdl,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  node->entry->match_spec->priority = match_spec->priority;
  node->entry->ttl = ttl;

  // Update state of atcam entry
  move_list_node->entry_hdl = node->entry->sram_entry_hdl;
  sts = pipe_mgr_tcam_entry_update_state(
      dev_id, tbl_info->atcam_tbl_hdl, move_list_node, NULL);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Unable to update atcam entry state for alpm entry %d "
        "tbl 0x%x device %d",
        __func__,
        __LINE__,
        ent_hdl,
        mat_tbl_hdl,
        dev_id);
    return sts;
  }
  for (i = 0; i < node->cov_pfx_count; i++) {
    move_list_node->entry_hdl =
        node->cov_pfx_subtree_nodes[i]->subtree->cov_pfx_entry->sram_entry_hdl;
    // Set proper priority for cp entries
    move_list_node->data->match_spec.priority = tbl_info->trie_depth + 1;
    sts = pipe_mgr_tcam_entry_update_state(
        dev_id, tbl_info->atcam_tbl_hdl, move_list_node, NULL);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Unable to update atcam entry state for alpm entry %d "
          "tbl 0x%x device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          dev_id);
      return sts;
    }
  }

  if (pipe_mgr_mat_tbl_has_idle(dev_id, mat_tbl_hdl)) {
    /* Here, generate MOVE-LIST to indicate to LLP that the TTL needs to be
     * updated.
     */
    pipe_mgr_move_list_t *head = *move_head_p;
    pipe_mgr_move_list_t *move_node =
        alloc_move_list(head, PIPE_MAT_UPDATE_ADD_IDLE, move_list_node->pipe);
    move_node->entry_hdl = node->entry->sram_entry_hdl;
    move_node->data = make_mat_ent_data(NULL, NULL, 0, ttl, 0, 0, 0);
    if (*move_head_p) {
      (*move_head_p)->next = move_node;
    } else {
      *move_head_p = move_node;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_ADD_IDLE,
                             tbl_info->atcam_tbl_hdl,
                             move_node,
                             node->entry->sram_entry_hdl,
                             node->entry->alpm_entry_hdl,
                             false);
  }

  return PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_alpm_pipe_tbl_create(alpm_tbl_info, mat_tbl_info, pipe_id)
 * --------------------------------------------------------------------------
 * Allocates and populates an alpm pipe table structure.
 */
alpm_pipe_tbl_t *pipe_mgr_alpm_pipe_tbl_create(
    alpm_tbl_info_t *tbl_info,
    pipe_mat_tbl_info_t *mat_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *pipe_bmp) {
  alpm_pipe_tbl_t *pipe_tbl;
  partition_info_t *ptn;
  uint32_t ptn_idx;

  pipe_tbl = PIPE_MGR_CALLOC(1, sizeof(alpm_pipe_tbl_t));
  pipe_tbl->alpm_tbl_info = tbl_info;
  if (tbl_info->is_symmetric) {
    pipe_tbl->pipe_id = BF_DEV_PIPE_ALL;
  } else {
    pipe_tbl->pipe_id = pipe_id;
  }
  PIPE_BITMAP_INIT(&(pipe_tbl->pipe_bmp), PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&(pipe_tbl->pipe_bmp), pipe_bmp);
  pipe_tbl->root = create_node(NULL, false);
  pipe_tbl->partitions =
      PIPE_MGR_CALLOC(tbl_info->num_partitions, sizeof(partition_info_t));
  pipe_tbl->ptn_free_list = pipe_tbl->partitions;
  for (ptn_idx = 0; ptn_idx < tbl_info->num_partitions; ptn_idx++) {
    ptn = &pipe_tbl->partitions[ptn_idx];
    /*
     * If ATCAM subset key width optimization or exclude MSB bits is used, then
     * partition 0 has to be reserved and shouldn't be used. This is needed
     * because for covering prefix of subset width and prefix len < exclude MSB
     * bits, the route entry that is added is all 0's for key and mask meaning
     * it's a catch-all entry. But preclassifier miss would result in ATCAM
     * partition 0 lookup, so this will result in catch-all entry being hit in
     * ATCAM (if partition 0 is used) instead of miss & hitting
     * default action. To avoid this, partition 0 is reserved and not used for
     * any routes installation. Num partitions is already reduced by 1,
     * so adjust the partition index here so that partition 0 doesn't get used.
     */
    if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
      ptn->ptn_index = ptn_idx + 1;
    } else {
      ptn->ptn_index = ptn_idx;
    }

    /*
     * If ALPM key width optimization is used and max subtrees per partition
     * is more than 1, initialize the subtree id allocator
     */
    if (tbl_info->atcam_subset_key_width &&
        tbl_info->max_subtrees_per_partition > 1) {
      ptn->subtree_id_allocator =
          bf_id_allocator_new(tbl_info->max_subtrees_per_partition, true);
    }
    ptn->subtree_nodes = (trie_node_t **)PIPE_MGR_CALLOC(
        tbl_info->max_subtrees_per_partition, sizeof(trie_node_t *));
    if (ptn_idx < tbl_info->num_partitions - 1) {
      ptn->next = &pipe_tbl->partitions[ptn_idx + 1];
    }
  }
  pipe_tbl->backup_partitions_in_use = -1;

  pipe_tbl->ent_hdl_allocator = bf_id_allocator_new(tbl_info->size << 1, false);
  bf_id_allocator_set(pipe_tbl->ent_hdl_allocator, PIPE_ALPM_DEFAULT_ENT_HDL);

  pipe_tbl->spec_map.dev_tgt.device_id = tbl_info->dev_id;
  pipe_tbl->spec_map.dev_tgt.dev_pipe_id = pipe_id;
  pipe_tbl->spec_map.mat_tbl_hdl = tbl_info->alpm_tbl_hdl;
  pipe_tbl->spec_map.tbl_info = mat_tbl_info;

  pipe_tbl->spec_map.entry_place_with_hdl_fn =
      pipe_mgr_alpm_entry_place_with_hdl;
  pipe_tbl->spec_map.entry_modify_fn = pipe_mgr_alpm_ent_set_action;
  pipe_tbl->spec_map.entry_delete_fn = pipe_mgr_alpm_entry_del;
  pipe_tbl->spec_map.entry_update_fn = pipe_mgr_alpm_entry_update_state;

  return pipe_tbl;
}

/*
 * Usage: free_trie(node)
 * ----------------------
 * Frees the trie structure through a recursive DFS
 */
static void free_trie(trie_node_t *node) {
  if (!node) {
    return;
  }
  free_trie(node->left_child);
  free_trie(node->right_child);

  free_node(node);
}

/*
 * Usage: free_partitions(pipe_tbl)
 * --------------------------------
 * Frees all atcam partitions, along with all subtrees for these partitions.
 */
static void free_partitions(alpm_pipe_tbl_t *pipe_tbl) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  partition_info_t *curr_ptn_info;
  uint32_t ptn_idx;

  for (ptn_idx = 0; ptn_idx < tbl_info->num_partitions; ptn_idx++) {
    curr_ptn_info = &pipe_tbl->partitions[ptn_idx];
    if (curr_ptn_info->subtree_nodes) {
      PIPE_MGR_FREE(curr_ptn_info->subtree_nodes);
    }

    /*
     * If ALPM key width optimization is used and max subtrees per partiton
     * is more than 1, free the subtree id allocator.
     */
    if (tbl_info->atcam_subset_key_width &&
        tbl_info->max_subtrees_per_partition > 1) {
      bf_id_allocator_destroy(curr_ptn_info->subtree_id_allocator);
    }
  }
  PIPE_MGR_FREE(pipe_tbl->partitions);
}

/*
 * Usage: pipe_mgr_alpm_pipe_tbl_destroy(pipe_tbl)
 * -----------------------------------------------
 * Frees all memory associated with an alpm pipe table structure.
 */
void pipe_mgr_alpm_pipe_tbl_destroy(alpm_pipe_tbl_t *pipe_tbl) {
  free_trie(pipe_tbl->root);
  free_partitions(pipe_tbl);
  bf_id_allocator_destroy(pipe_tbl->ent_hdl_allocator);
  bf_map_destroy(&pipe_tbl->alpm_entry_hdl_map);
  bf_map_destroy(&pipe_tbl->atcam_entry_hdl_map);
  free_match_spec(pipe_tbl->match_spec_template);
  free_action_spec(pipe_tbl->act_spec_template);

  PIPE_MGR_FREE(pipe_tbl);
}

/*
 * Usage: pipe_mgr_alpm_tbl_info_destroy(alpm_tbl_info)
 * ----------------------------------------------------
 * Frees all memory associated with an alpm table structure.
 */
void pipe_mgr_alpm_tbl_info_destroy(alpm_tbl_info_t *tbl_info) {
  if (!tbl_info) {
    return;
  }

  uint32_t i;
  for (i = 0; i < tbl_info->num_pipes; i++) {
    pipe_mgr_alpm_pipe_tbl_destroy(tbl_info->pipe_tbls[i]);
  }
  PIPE_MGR_FREE(tbl_info->pipe_tbls);

  PIPE_MGR_FREE(tbl_info->name);
  if (tbl_info->scope_pipe_bmp) {
    PIPE_MGR_FREE(tbl_info->scope_pipe_bmp);
  }

  PIPE_MGR_FREE(tbl_info);
}

/*
 * Usage: pipe_mgr_alpm_init()
 * ---------------------------
 * Initializes the global data for alpm management.
 */
pipe_status_t pipe_mgr_alpm_init(void) {
  if (alpm_mgr_ctx()) {
    return PIPE_SUCCESS;
  }

  alpm_ctx_p = &alpm_ctx;
  return PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_alpm_tbl_create(device_id, mat_tbl_hdl, profile_id)
 * -------------------------------------------------------------------
 * Creates and populates a logical alpm table structure.
 */
pipe_status_t pipe_mgr_alpm_tbl_create(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl,
                                       profile_id_t profile_id,
                                       pipe_bitmap_t *pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_map_sts_t msts;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  alpm_tbl_info_t *alpm_tbl_info = NULL;
  uint32_t i = 0, j = 0, q = 0;
  uint32_t byte_loc, num_bytes, bit_loc, num_bits;

  (void)pipe_bmp;
  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table 0x%x not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  alpm_tbl_info =
      (alpm_tbl_info_t *)PIPE_MGR_CALLOC(1, sizeof(alpm_tbl_info_t));
  alpm_tbl_info->name = bf_sys_strdup(mat_tbl_info->name);
  alpm_tbl_info->dev_id = dev_id;
  alpm_tbl_info->dev_info = pipe_mgr_get_dev_info(dev_id);
  alpm_tbl_info->profile_id = profile_id;
  alpm_tbl_info->size = mat_tbl_info->size;
  alpm_tbl_info->alpm_tbl_hdl = tbl_hdl;
  alpm_tbl_info->preclass_tbl_hdl = mat_tbl_info->alpm_info->preclass_handle;
  alpm_tbl_info->atcam_tbl_hdl = mat_tbl_info->alpm_info->atcam_handle;

  alpm_tbl_info->num_partitions = mat_tbl_info->num_partitions;
  // Save 1 entry space for entry replacements
  alpm_tbl_info->partition_depth = mat_tbl_info->alpm_info->partition_depth - 1;
  alpm_tbl_info->max_subtrees_per_partition =
      mat_tbl_info->alpm_info->max_subtrees_per_partition;

  alpm_tbl_info->num_excluded_bits = mat_tbl_info->alpm_info->num_excluded_bits;
  alpm_tbl_info->ptn_idx_byte_offset =
      mat_tbl_info->alpm_info->partition_idx_start_bit / 8;
  alpm_tbl_info->num_ptn_idx_bits =
      mat_tbl_info->alpm_info->partition_idx_field_width;
  alpm_tbl_info->act_fn_hdls = mat_tbl_info->alpm_info->act_fn_hdls;
  alpm_tbl_info->num_act_fn_hdls = mat_tbl_info->alpm_info->num_act_fn_hdls;

  alpm_tbl_info->num_fields = mat_tbl_info->alpm_info->num_fields;
  alpm_tbl_info->field_info = mat_tbl_info->alpm_info->field_info;

  for (i = 0; i < alpm_tbl_info->num_fields; i++) {
    num_bits = alpm_tbl_info->field_info[i].bit_width;
    num_bytes = (num_bits + 7) / 8;
    byte_loc = alpm_tbl_info->field_info[i].byte_offset;
    for (j = 0; j < alpm_tbl_info->field_info[i].num_slices; j++) {
      bit_loc = alpm_tbl_info->field_info[i].slice_offset[j] +
                alpm_tbl_info->field_info[i].slice_width[j] - 1;
      byte_loc = byte_loc + num_bytes - bit_loc / 8 - 1;
      /* Last field slice is LPM field, remember its byte offset */
      alpm_tbl_info->lpm_field_byte_offset = byte_loc;
      alpm_tbl_info->trie_depth += alpm_tbl_info->field_info[i].slice_width[j];
    }
  }

  alpm_tbl_info->atcam_subset_key_width =
      mat_tbl_info->alpm_info->atcam_subset_key_width;
  alpm_tbl_info->lpm_field_key_width =
      mat_tbl_info->alpm_info->lpm_field_key_width;
  alpm_tbl_info->shift_granularity = mat_tbl_info->alpm_info->shift_granularity;

  alpm_tbl_info->is_cp_restore = false;
  alpm_tbl_info->cp_ml_head = alpm_tbl_info->cp_ml_tail = NULL;
  alpm_tbl_info->cp_ver_bits = 0xD;

  /* Make sure trie depth is >= LPM key field width */
  if (alpm_tbl_info->trie_depth < alpm_tbl_info->lpm_field_key_width) {
    LOG_ERROR(
        "%s:%d, ALPM trie depth %d is less than LPM field key width %d for "
        "ALPM table hdl 0x%x table name %s",
        __func__,
        __LINE__,
        alpm_tbl_info->trie_depth,
        alpm_tbl_info->lpm_field_key_width,
        alpm_tbl_info->alpm_tbl_hdl,
        alpm_tbl_info->name);
    return PIPE_INVALID_ARG;
  }

  /*
   * If LPM field key width is equal to ATCAM subset key width,
   * then ALPM subset key width optimization shouldn't be used.
   * So, set ATCAM subset key width to 0. Also, set the atcam subset
   * key width to 0 in pipe manager's match table.
   */
  if (alpm_tbl_info->lpm_field_key_width ==
      alpm_tbl_info->atcam_subset_key_width) {
    alpm_tbl_info->atcam_subset_key_width = 0;
    mat_tbl_info->alpm_info->atcam_subset_key_width = 0;
  }

  /*
   * ALPM subset key width optimization and excluded fields are mutually
   * exclusive. Return error if both need to be supported.
   */
  if (alpm_tbl_info->atcam_subset_key_width &&
      alpm_tbl_info->num_excluded_bits) {
    LOG_ERROR(
        "%s:%d: ALPM subset key width optimization and excluded fields are "
        "mutually exclusive. Both can't be supported at the same time for "
        "ALPM table hdl 0x%x table name %s",
        __func__,
        __LINE__,
        alpm_tbl_info->alpm_tbl_hdl,
        alpm_tbl_info->name);
    return PIPE_INVALID_ARG;
  }

  /*
   * If ALPM subset key width optimization is used, make sure
   * LPM field key width is > ATCAM subset key width
   */
  if (alpm_tbl_info->atcam_subset_key_width &&
      (alpm_tbl_info->lpm_field_key_width <=
       alpm_tbl_info->atcam_subset_key_width)) {
    LOG_ERROR(
        "%s:%d, ALPM LPM key field width %d is less than or same as ATCAM "
        "subset key width %d for ALPM table hdl 0x%x table name %s",
        __func__,
        __LINE__,
        alpm_tbl_info->lpm_field_key_width,
        alpm_tbl_info->atcam_subset_key_width,
        alpm_tbl_info->alpm_tbl_hdl,
        alpm_tbl_info->name);
    return PIPE_INVALID_ARG;
  }

  /* Calculate the non LPM (exact match) fields' width if any */
  alpm_tbl_info->exm_fields_key_width =
      alpm_tbl_info->trie_depth - alpm_tbl_info->lpm_field_key_width;

  /*
   * If ALPM subset key width optimization is used, validate
   * the shift_granularity
   */
  if (alpm_tbl_info->atcam_subset_key_width &&
      (alpm_tbl_info->shift_granularity < 1 ||
       (alpm_tbl_info->shift_granularity >
        alpm_tbl_info->atcam_subset_key_width) ||
       ((alpm_tbl_info->atcam_subset_key_width %
         alpm_tbl_info->shift_granularity) != 0))) {
    LOG_ERROR(
        "%s:%d, ALPM shift granularity %d is invalid for "
        "ATCAM subset key width %d of "
        "ALPM table hdl 0x%x table name %s",
        __func__,
        __LINE__,
        alpm_tbl_info->shift_granularity,
        alpm_tbl_info->atcam_subset_key_width,
        alpm_tbl_info->alpm_tbl_hdl,
        alpm_tbl_info->name);
    return PIPE_INVALID_ARG;
  }

  /*
   * If ALPM subset key width optimization is used, validate
   * the number of preclassifier action handles
   */
  if (alpm_tbl_info->atcam_subset_key_width) {
    uint8_t expected_num_act_fn_hdls = 0;
    uint8_t extra_num_act_fn_hdls = 0;
    /*
     * If the subset key width is not a multiple of shift granularity, then
     * account for another extra action handle for the remaining bits
     * at the end.
     */
    extra_num_act_fn_hdls = (alpm_tbl_info->atcam_subset_key_width %
                             alpm_tbl_info->shift_granularity) == 0
                                ? 1
                                : 2;
    expected_num_act_fn_hdls = ((alpm_tbl_info->lpm_field_key_width -
                                 alpm_tbl_info->atcam_subset_key_width) /
                                alpm_tbl_info->shift_granularity) +
                               extra_num_act_fn_hdls;
    if (alpm_tbl_info->num_act_fn_hdls != expected_num_act_fn_hdls) {
      LOG_ERROR(
          "%s:%d, ALPM num act hdls %d is invalid, expected %d for LPM field "
          "key width %d, "
          "ATCAM subset key width %d and shift granularity %d for "
          "ALPM table hdl 0x%x table name %s",
          __func__,
          __LINE__,
          alpm_tbl_info->num_act_fn_hdls,
          expected_num_act_fn_hdls,
          alpm_tbl_info->lpm_field_key_width,
          alpm_tbl_info->atcam_subset_key_width,
          alpm_tbl_info->shift_granularity,
          alpm_tbl_info->alpm_tbl_hdl,
          alpm_tbl_info->name);
      return PIPE_INVALID_ARG;
    }
  }

  /*
   * If ALPM subset key width optimization is used, validate the
   * the partition_depth if non-default shift_granularity is used.
   */
  if (alpm_tbl_info->atcam_subset_key_width &&
      alpm_tbl_info->shift_granularity > 1 &&
      alpm_tbl_info->partition_depth < 3) {
    LOG_ERROR(
        "%s:%d, ALPM partition depth %d is NOT supported for "
        "ATCAM subset key width %d and shift granularity %d of "
        "ALPM table hdl 0x%x table name %s",
        __func__,
        __LINE__,
        alpm_tbl_info->partition_depth,
        alpm_tbl_info->atcam_subset_key_width,
        alpm_tbl_info->shift_granularity,
        alpm_tbl_info->alpm_tbl_hdl,
        alpm_tbl_info->name);
    return PIPE_INVALID_ARG;
  }

  /*
   * If ATCAM subset key width optimization or exclude MSB bits is used, then
   * partition 0 has to be reserved and shouldn't be used. This is needed
   * because for covering prefix of subset width and prefix len < exclude MSB
   * bits, the route entry that is added is all 0's for key and mask meaning
   * it's a catch-all entry. But preclassifier miss would result in ATCAM
   * partition 0 lookup, so this will result in catch-all entry being hit in
   * ATCAM (if partition 0 is used) instead of miss & hitting
   * default action. To avoid this, partition 0 is reserved and not used for any
   * routes installation.
   */
  if (ALPM_IS_SCALE_OPT_ENB(alpm_tbl_info)) {
    alpm_tbl_info->num_partitions--;
  }

  LOG_DBG(
      "%s: %d, trie depth = %d, LPM field width %d, exm fields width "
      "%d, ATCAM subset key width %d, num act hdls %d for "
      "ALPM tabl hdl 0x%x table name %s",
      __func__,
      __LINE__,
      alpm_tbl_info->trie_depth,
      alpm_tbl_info->lpm_field_key_width,
      alpm_tbl_info->exm_fields_key_width,
      alpm_tbl_info->atcam_subset_key_width,
      alpm_tbl_info->num_act_fn_hdls,
      alpm_tbl_info->alpm_tbl_hdl,
      alpm_tbl_info->name);

  alpm_tbl_info->is_symmetric = mat_tbl_info->symmetric;
  alpm_tbl_info->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!alpm_tbl_info->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (alpm_tbl_info->is_symmetric) {
    alpm_tbl_info->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      alpm_tbl_info->scope_pipe_bmp[0] |= (1 << q);
    }
  } else {
    alpm_tbl_info->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      alpm_tbl_info->scope_pipe_bmp[q] |= (1 << q);
      alpm_tbl_info->num_scopes += 1;
    }
  }
  alpm_tbl_info->num_pipes = alpm_tbl_info->num_scopes;
  alpm_tbl_info->pipe_tbls =
      PIPE_MGR_CALLOC(alpm_tbl_info->num_pipes, sizeof(alpm_pipe_tbl_t *));
  for (i = 0; i < alpm_tbl_info->num_pipes; i++) {
    pipe_bitmap_t local_pipe_bmp;
    bf_dev_pipe_t pipe_id = 0;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(alpm_tbl_info->scope_pipe_bmp[i]);
    pipe_mgr_convert_scope_pipe_bmp(alpm_tbl_info->scope_pipe_bmp[i],
                                    &local_pipe_bmp);
    alpm_tbl_info->pipe_tbls[i] = pipe_mgr_alpm_pipe_tbl_create(
        alpm_tbl_info, mat_tbl_info, pipe_id, &local_pipe_bmp);
  }

  msts = bf_map_add(&alpm_mgr_ctx()->tbl_hdl_to_tbl_map[dev_id],
                    tbl_hdl,
                    (void *)alpm_tbl_info);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding alpm table to map sts %d",
              __func__,
              __LINE__,
              alpm_tbl_info->name,
              alpm_tbl_info->dev_id,
              alpm_tbl_info->alpm_tbl_hdl,
              msts);
    rc = PIPE_INIT_ERROR;
    pipe_mgr_alpm_tbl_info_destroy(alpm_tbl_info);
  }

  return rc;
}

/*
 * Usage: pipe_mgr_alpm_tbl_delete(device_id, mat_tbl_hdl)
 * -------------------------------------------------------
 * Deletes the alpm table data structure associated with the given handle.
 */
pipe_status_t pipe_mgr_alpm_tbl_delete(bf_dev_id_t dev_id,
                                       pipe_mat_tbl_hdl_t tbl_hdl) {
  alpm_tbl_info_t *tbl_info = NULL;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_TRACE("%s:%d Alpm table not found for handle 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  bf_map_sts_t msts;
  msts = bf_map_rmv(&alpm_mgr_ctx()->tbl_hdl_to_tbl_map[dev_id], tbl_hdl);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting alpm table sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->dev_id,
              tbl_info->alpm_tbl_hdl,
              msts);
    return PIPE_UNEXPECTED;
  }

  pipe_mgr_alpm_tbl_info_destroy(tbl_info);

  return PIPE_SUCCESS;
}

/*
 * Usage: get_partition_depth(alpm_tbl_info, p_info)
 * -------------------------------------------------
 * Returns the partition depth of the given partition.
 * Sets one spot aside for the default entry in partition 0.
 */
static uint32_t get_partition_depth(alpm_tbl_info_t *tbl_info,
                                    partition_info_t *p_info) {
  return (p_info->ptn_index == 0) ? (tbl_info->partition_depth - 1)
                                  : tbl_info->partition_depth;
}

/*
 * Usage: get_split_cutoff(alpm_tbl_info, p_info)
 * ----------------------------------------------
 * Calculates and returns the cutoff for the free space in a
 * partition that's acceptable for a split-then-move operation.
 * To keep subtrees at reasonable sizes, we will only consider
 * partitions where at least 10% of entry slots are free. There
 * also must be at least 3 free slots for a successful move.
 */
static uint32_t get_split_cutoff(alpm_tbl_info_t *tbl_info,
                                 partition_info_t *p_info) {
  uint32_t ptn_depth = get_partition_depth(tbl_info, p_info);
  return (ptn_depth * 0.9 < ptn_depth - 3) ? (uint32_t)(ptn_depth * 0.9)
                                           : (ptn_depth - 3);
}

/*
 * Usage: get_subtree_size(node)
 * -----------------------------
 * Calculates and returns the size of a subtree rooted at the given node.
 */
static uint32_t get_subtree_size(trie_node_t *node) {
  uint32_t size = node->count;
  if ((!node->entry || !node->entry->sram_entry_hdl) &&
      find_covering_prefix(node)) {
    size++;
  }
  return size;
}

/*
 * Usage: too_large(node, space_left)
 * ----------------------------------
 * Checks and returns whether a subtree rooted at this node can fit in
 * the space given.
 */
static bool too_large(trie_node_t *node, uint32_t space_left) {
  return get_subtree_size(node) > space_left;
}

/*
 * Usage: get_match_spec_node_depth(alpm_tbl_info, match_spec)
 * -----------------------------------------------------------
 * Calculates and returns the depth of the node corresponding to
 * this match_spec.
 */
static uint32_t get_match_spec_node_depth(alpm_tbl_info_t *tbl_info,
                                          pipe_tbl_match_spec_t *match_spec) {
  uint32_t depth = 0, byte_loc, num_bytes, bit_loc, num_bits;
  uint32_t i, j;
  uint8_t *byte;

  for (i = 0; i < tbl_info->num_fields; i++) {
    num_bits = tbl_info->field_info[i].bit_width;
    num_bytes = (num_bits + 7) / 8;
    byte_loc = tbl_info->field_info[i].byte_offset;
    for (j = 0; j < tbl_info->field_info[i].num_slices; j++) {
      bit_loc = tbl_info->field_info[i].slice_offset[j] +
                tbl_info->field_info[i].slice_width[j] - 1;
      byte_loc = byte_loc + num_bytes - bit_loc / 8 - 1;
      byte = match_spec->match_mask_bits + byte_loc;
      while (bit_loc >= tbl_info->field_info[i].slice_offset[j]) {
        if ((*byte & (1 << (bit_loc % 8))) == 0) {
          return depth;
        }
        depth++;
        if (bit_loc == 0) {
          break;
        }
        if (bit_loc % 8 == 0) {
          byte++;
        }
        bit_loc--;
      }
    }
  }

  return depth;
}

/*
 * Usage: build_match_spec_bits(alpm_tbl_info, match_spec, node)
 * -------------------------------------------------------------
 * Constructs the value and mask byte arrays for the match spec
 * corresponding to the given node.
 */
static void build_match_spec_bits(alpm_tbl_info_t *tbl_info,
                                  pipe_tbl_match_spec_t *match_spec,
                                  trie_node_t *node) {
  uint32_t depth, num_bytes, num_bits, byte_loc, bit_loc;
  int i, j;
  uint8_t *value_byte, *mask_byte;
  trie_node_t *curr = node;
  depth = tbl_info->trie_depth;

  PIPE_MGR_MEMSET(match_spec->match_value_bits, 0, match_spec->num_match_bytes);
  PIPE_MGR_MEMSET(match_spec->match_mask_bits, 0, match_spec->num_match_bytes);

  /* Traverse up the trie and construct the byte arrays one bit at a time */
  for (i = tbl_info->num_fields - 1; i >= 0; i--) {
    num_bits = tbl_info->field_info[i].bit_width;
    num_bytes = (num_bits + 7) / 8;
    byte_loc = tbl_info->field_info[i].byte_offset;
    for (j = tbl_info->field_info[i].num_slices - 1; j >= 0; j--) {
      if (depth > tbl_info->field_info[i].slice_width[j] &&
          depth - tbl_info->field_info[i].slice_width[j] > node->depth) {
        depth -= tbl_info->field_info[i].slice_width[j];
        continue;
      }
      bit_loc = tbl_info->field_info[i].slice_offset[j];
      byte_loc = byte_loc + num_bytes - bit_loc / 8 - 1;
      value_byte = match_spec->match_value_bits + byte_loc;
      mask_byte = match_spec->match_mask_bits + byte_loc;
      while (bit_loc < tbl_info->field_info[i].slice_offset[j] +
                           tbl_info->field_info[i].slice_width[j]) {
        if (depth <= node->depth) {
          *mask_byte |= (1 << (bit_loc % 8));
          if (curr->parent->right_child == curr) {
            *value_byte |= (1 << (bit_loc % 8));
          }
          curr = curr->parent;
        }
        bit_loc++;
        if (bit_loc % 8 == 0) {
          value_byte--;
          mask_byte--;
        }
        depth--;
      }
    }
  }

  return;
}

/*
 * Usage: build_match_spec(alpm_tbl_info, node)
 * --------------------------------------------
 * Build the match spec for the given node. Used for entry additions
 * into the preclassifier table.
 */
static pipe_tbl_match_spec_t *build_match_spec(alpm_tbl_info_t *tbl_info,
                                               alpm_pipe_tbl_t *pipe_tbl,
                                               trie_node_t *node) {
  pipe_tbl_match_spec_t *match_spec = NULL;

  if (!tbl_info || !node) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return NULL;
  }

  match_spec =
      pipe_mgr_tbl_copy_match_spec(match_spec, pipe_tbl->match_spec_template);

  if (match_spec) {
    match_spec->partition_index = 0;
    match_spec->priority = tbl_info->trie_depth - node->depth;
    build_match_spec_bits(tbl_info, match_spec, node);
  }

  return match_spec;
}

/*
 * Usage: build_action_data_spec_bits(alpm_tbl_info, act_spec, ptn_index)
 * ----------------------------------------------------------------------
 * Constructs the action data spec byte array by setting the partition index
 * field appropriately.
 */
static void build_action_data_spec_bits(alpm_tbl_info_t *tbl_info,
                                        pipe_action_spec_t *act_spec,
                                        uint32_t ptn_index,
                                        uint8_t subtree_id) {
  uint16_t num_ptn_idx_bytes = (tbl_info->num_ptn_idx_bits + 7) / 8;
  uint32_t ptn_index_big_endian = htonl(ptn_index);
  uint8_t *act_data_ptn_index_bits =
      act_spec->act_data.action_data_bits + tbl_info->ptn_idx_byte_offset;
  uint16_t padding;

  PIPE_MGR_DBGCHK(num_ptn_idx_bytes > 0);

  switch (num_ptn_idx_bytes) {
    case 1:
      PIPE_MGR_MEMCPY(
          act_data_ptn_index_bits, ((char *)&ptn_index_big_endian) + 3, 1);
      break;
    case 2:
      PIPE_MGR_MEMCPY(
          act_data_ptn_index_bits, ((char *)&ptn_index_big_endian) + 2, 2);
      break;
    case 3:
      PIPE_MGR_MEMCPY(
          act_data_ptn_index_bits, ((char *)&ptn_index_big_endian) + 1, 3);
      break;
    default:
      padding = num_ptn_idx_bytes - 4;
      PIPE_MGR_MEMCPY(
          act_data_ptn_index_bits + padding, &ptn_index_big_endian, 4);
  }

  /*
   * If ALPM key width optimization is used and max subtrees per partition
   * is more than 1, set the subtree id in action data.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    uint8_t *act_data_subtree_id_bits = act_spec->act_data.action_data_bits +
                                        tbl_info->ptn_idx_byte_offset +
                                        num_ptn_idx_bytes;

    /* Never mind the endianess as the subtree id never exceeds a byte */
    *act_data_subtree_id_bits = subtree_id;
  }
}

static uint32_t read_action_spec_ptn_idx(alpm_tbl_info_t *tbl_info,
                                         pipe_action_spec_t *act_spec,
                                         uint8_t *subtree_id) {
  uint16_t num_ptn_idx_bytes = (tbl_info->num_ptn_idx_bits + 7) / 8;
  uint32_t ptn_index_big_endian = 0;
  uint8_t *act_data_ptn_index_bits =
      act_spec->act_data.action_data_bits + tbl_info->ptn_idx_byte_offset;
  uint16_t padding;

  PIPE_MGR_DBGCHK(num_ptn_idx_bytes > 0);

  switch (num_ptn_idx_bytes) {
    case 1:
      PIPE_MGR_MEMCPY(
          ((char *)&ptn_index_big_endian) + 3, act_data_ptn_index_bits, 1);
      break;
    case 2:
      PIPE_MGR_MEMCPY(
          ((char *)&ptn_index_big_endian) + 2, act_data_ptn_index_bits, 2);
      break;
    case 3:
      PIPE_MGR_MEMCPY(
          ((char *)&ptn_index_big_endian) + 1, act_data_ptn_index_bits, 3);
      break;
    default:
      padding = num_ptn_idx_bytes - 4;
      PIPE_MGR_MEMCPY(
          &ptn_index_big_endian, act_data_ptn_index_bits + padding, 4);
  }
  /* In case of scale optimization fetch subtree id for the subset key feature
   * as atcam contains partial key only.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    uint8_t *act_data_subtree_id_bits = act_spec->act_data.action_data_bits +
                                        tbl_info->ptn_idx_byte_offset +
                                        num_ptn_idx_bytes;

    /* Endianess is ignored as the subtree id never exceeds a byte */
    *subtree_id = *act_data_subtree_id_bits;
  } else {
    *subtree_id = 0;
  }

  return ntohl(ptn_index_big_endian);
}

/*
 * Usage: build_act_spec(tbl_info, ptn_index)
 * ------------------------------------------
 * Builds the action spec for a preclassifier entry that points to the given
 * partition index.
 */
static pipe_action_spec_t *build_act_spec(alpm_tbl_info_t *tbl_info,
                                          alpm_pipe_tbl_t *pipe_tbl,
                                          uint32_t ptn_index,
                                          uint8_t subtree_id) {
  pipe_action_spec_t *act_spec = NULL;
  act_spec =
      pipe_mgr_tbl_copy_action_spec(act_spec, pipe_tbl->act_spec_template);
  build_action_data_spec_bits(tbl_info, act_spec, ptn_index, subtree_id);

  return act_spec;
}

/*
 * Usage: pipe_mgr_set_alpm_tbl_match_act_info(alpm_tbl_info,
      match_spec, act_spec)
 * ----------------------------------------------------------
 * Sets up the info necessary to construct preclassifier entry match and
 * action specs.
 */
pipe_status_t pipe_mgr_set_alpm_tbl_match_act_info(
    alpm_tbl_info_t *tbl_info,
    alpm_pipe_tbl_t *pipe_tbl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec) {
  uint16_t num_ptn_idx_bytes;

  if (match_spec && !pipe_tbl->match_spec_template) {
    pipe_tbl->match_spec_template =
        pipe_mgr_tbl_copy_match_spec(pipe_tbl->match_spec_template, match_spec);
  }

  if (act_spec && !pipe_tbl->act_spec_template) {
    pipe_tbl->act_spec_template =
        pipe_mgr_tbl_copy_action_spec(pipe_tbl->act_spec_template, act_spec);

    if (!pipe_tbl->act_spec_template) {
      return PIPE_UNEXPECTED;
    }

    /* Note: The preclassifier action data contains the partition index first */
    num_ptn_idx_bytes = (tbl_info->num_ptn_idx_bits + 7) / 8;
    pipe_tbl->act_spec_template->act_data.num_valid_action_data_bits =
        tbl_info->num_ptn_idx_bits;
    pipe_tbl->act_spec_template->act_data.num_action_data_bytes =
        num_ptn_idx_bytes;

    /*
     * If ALPM key width optimization is used and if the max subtrees
     * per partition is > 1, then preclassifier action data would have
     * the subtree id as well.
     */
    if (tbl_info->atcam_subset_key_width &&
        tbl_info->max_subtrees_per_partition > 1) {
      uint8_t num_subtree_id_bits = log2(tbl_info->max_subtrees_per_partition);
      pipe_tbl->act_spec_template->act_data.num_valid_action_data_bits +=
          num_subtree_id_bits;
      pipe_tbl->act_spec_template->act_data.num_action_data_bytes +=
          (num_subtree_id_bits + 7) / 8;
    }

    pipe_tbl->act_spec_template->act_data.action_data_bits = PIPE_MGR_REALLOC(
        pipe_tbl->act_spec_template->act_data.action_data_bits,
        pipe_tbl->act_spec_template->act_data.num_action_data_bytes);

    /* The preclassifier uses a direct action with no resources */
    pipe_tbl->act_spec_template->pipe_action_datatype_bmap =
        PIPE_ACTION_DATA_TYPE;
    pipe_tbl->act_spec_template->resource_count = 0;
  }

  return PIPE_SUCCESS;
}

/*
 * Usage: should_go_left(alpm_tbl_info, match_spec, depth)
 * -------------------------------------------------------
 * Calculates the direction to travel from the given depth in order to
 * arrive at the node that corresponds to the given match spec.
 */
static bool should_go_left(alpm_tbl_info_t *tbl_info,
                           pipe_tbl_match_spec_t *match_spec,
                           uint32_t depth) {
  uint32_t num_bytes, num_bits, byte_loc, bit_loc, i, j;
  uint8_t match_byte;

  for (i = 0; i < tbl_info->num_fields; i++) {
    num_bits = tbl_info->field_info[i].bit_width;
    num_bytes = (num_bits + 7) / 8;
    byte_loc = tbl_info->field_info[i].byte_offset;
    for (j = 0; j < tbl_info->field_info[i].num_slices; j++) {
      if (depth >= tbl_info->field_info[i].slice_width[j]) {
        depth -= tbl_info->field_info[i].slice_width[j];
        continue;
      }
      bit_loc = tbl_info->field_info[i].slice_offset[j] +
                tbl_info->field_info[i].slice_width[j] - 1 - depth;
      byte_loc = byte_loc + num_bytes - bit_loc / 8 - 1;
      match_byte = match_spec->match_value_bits[byte_loc];
      return (match_byte & (1 << (bit_loc % 8))) == 0;
    }
  }

  return false;
}

/*
 * Usage: find_subtree(node)
 * -------------------------
 * Finds the subtree to which the given node belongs.
 */
static trie_subtree_t *find_subtree(trie_node_t *node) {
  trie_node_t *curr = node;

  while (!curr->subtree) {
    curr = curr->parent;
  }

  return curr->subtree;
}

/*
 * Usage: walk_child_subtrees(subtree_list, ref_node, node, num_found_subtrees)
 * -------------------------------------------------------------------
 * Walk down the node's child nodes to see if underlying subtrees require
 * a covering prefix.
 *
 */
static void walk_child_subtrees(trie_subtree_t **subtree_list,
                                trie_node_t *ref_node,
                                trie_node_t *node,
                                uint32_t *num_found_subtrees) {
  trie_node_t *lc;
  trie_node_t *rc;

  if (!node || node->entry) {
    return;
  }

  lc = node->left_child;
  if (lc) {
    if (lc->subtree && lc->subtree->partition && !lc->subtree->node->entry &&
        !lc->subtree->cov_pfx &&
        ref_node ==
            find_covering_prefix_with_node(lc->subtree->node, ref_node)) {
      /* Covering prefix needed */
      subtree_list[*num_found_subtrees] = lc->subtree;
      *num_found_subtrees += 1;
    }
    walk_child_subtrees(subtree_list, ref_node, lc, num_found_subtrees);
  }

  rc = node->right_child;
  if (rc) {
    if (rc->subtree && rc->subtree->partition && !rc->subtree->node->entry &&
        !rc->subtree->cov_pfx &&
        ref_node ==
            find_covering_prefix_with_node(rc->subtree->node, ref_node)) {
      /* Covering prefix needed */
      subtree_list[*num_found_subtrees] = rc->subtree;
      *num_found_subtrees += 1;
    }
    walk_child_subtrees(subtree_list, ref_node, rc, num_found_subtrees);
  }
}

/*
 * Usage: update_counts(pipe_tbl, node, is_add)
 * --------------------------------------------
 * Updates the counts for the subtree the given node belongs to.
 */
static void update_counts(alpm_pipe_tbl_t *pipe_tbl,
                          trie_node_t *node,
                          bool is_add) {
  trie_node_t *curr = node;

  while (!curr->subtree) {
    backup_node(pipe_tbl, curr);
    curr->count += (is_add) ? 1 : -1;
    curr = curr->parent;
  }

  backup_node(pipe_tbl, curr);
  curr->count += (is_add) ? 1 : -1;
}

/*
 * Usage: get_larger_child(node)
 * -----------------------------
 * Returns the child of the given node with the higher count.
 */
static trie_node_t *get_larger_child(trie_node_t *node) {
  trie_node_t *left = node->left_child;
  trie_node_t *right = node->right_child;

  if (!left && !right) {
    return NULL;
  }
  if (left && right && left->subtree && right->subtree) {
    return NULL;
  }
  if (!left || left->subtree) {
    return right;
  }
  if (!right || right->subtree) {
    return left;
  }
  if (left->count > right->count) {
    return left;
  } else {
    return right;
  }
}

static void set_move_list(pipe_mgr_move_list_t **move_head_p,
                          pipe_mgr_move_list_t **move_tail_p,
                          pipe_mgr_move_list_t *move_list) {
  if (!*move_head_p) {
    *move_head_p = move_list;
    *move_tail_p = move_list;
  } else {
    (*move_tail_p)->next = move_list;
  }
  while (move_list) {
    *move_tail_p = move_list;
    move_list = move_list->next;
  }
}

static void clean_alpm_move_list_hdr(alpm_pipe_tbl_t *pipe_tbl) {
  pipe_mgr_alpm_move_list_hdr_t *ml_hdr, *ml_hdr_next;

  ml_hdr = pipe_tbl->ml_hdr;
  while (ml_hdr) {
    ml_hdr_next = ml_hdr->next;
    PIPE_MGR_FREE(ml_hdr);
    ml_hdr = ml_hdr_next;
  }
  pipe_tbl->ml_hdr = NULL;
  pipe_tbl->ml_hdr_tail = NULL;
}

static uint8_t get_byte(uint8_t *stream, uint8_t offset, uint8_t num_bits) {
  if (offset == 0) {
    return *stream;
  } else if (offset + num_bits <= 8) {
    return (*stream << offset);
  } else {
    return ((*stream << offset) | (*(stream + 1) >> (8 - offset)));
  }
}

static void set_byte(uint8_t *stream,
                     uint8_t offset,
                     uint8_t value,
                     uint8_t num_bits) {
  if (offset == 0) {
    *stream = value;
  } else if (offset + num_bits <= 8) {
    *stream |= (value >> offset);
  } else {
    *stream |= (value >> offset);
    *(stream + 1) |= (value << (8 - offset));
  }
}

static void setup_atcam_mspec(alpm_tbl_info_t *tbl_info,
                              pipe_tbl_match_spec_t *atcam_mspec,
                              pipe_tbl_match_spec_t *entry_mspec,
                              uint32_t depth) {
  /* Sanity check: Depth should be greater than EXM fields bit width */
  if (depth <= tbl_info->exm_fields_key_width) {
    /* We should never hit this as subtree is getting rooted in EXM fields */
    PIPE_MGR_DBGCHK(depth > tbl_info->exm_fields_key_width);
    return;
  }

  uint8_t atcam_offset = depth - tbl_info->exm_fields_key_width - 1;
  uint32_t atcam_subset_key_bits = tbl_info->atcam_subset_key_width;
  uint32_t num_subtree_id_bytes = 0, num_subtree_id_bits = 0;

  /*
   * If max subtrees per partition
   * is more than 1, account for subtree id in match spec.
   */
  if (tbl_info->max_subtrees_per_partition > 1) {
    num_subtree_id_bits = log2(tbl_info->max_subtrees_per_partition);
    num_subtree_id_bytes = (num_subtree_id_bits + 7) / 8;
  }

  /* Copy the entry_mspec subset to atcam_mspec */
  uint8_t val = 0;
  uint8_t mask = 0;
  uint8_t *src = entry_mspec->match_value_bits +
                 tbl_info->lpm_field_byte_offset + atcam_offset / 8;
  uint8_t *dst = atcam_mspec->match_value_bits + num_subtree_id_bytes;
  uint8_t *mask_src = entry_mspec->match_mask_bits +
                      tbl_info->lpm_field_byte_offset + atcam_offset / 8;
  uint8_t *mask_dst = atcam_mspec->match_mask_bits + num_subtree_id_bytes;
  int prefix_len = atcam_subset_key_bits;
  int dst_offset =
      (atcam_subset_key_bits % 8) ? (8 - atcam_subset_key_bits % 8) : 0;

  while (prefix_len > 0) {
    uint8_t num_bits = prefix_len >= 8 ? 8 : prefix_len;
    val = get_byte(src, atcam_offset % 8, num_bits);
    mask = get_byte(mask_src, atcam_offset % 8, num_bits);

    set_byte(dst, dst_offset, val, num_bits);
    set_byte(mask_dst, dst_offset, mask, num_bits);

    src++;
    dst++;
    mask_src++;
    mask_dst++;
    prefix_len -= 8;
  }
}

static void setup_atcam_exclude_msb_bits_mspec(
    alpm_tbl_info_t *tbl_info,
    pipe_tbl_match_spec_t *atcam_mspec,
    pipe_tbl_match_spec_t *entry_mspec) {
  uint8_t atcam_offset =
      tbl_info->num_excluded_bits - tbl_info->exm_fields_key_width;
  uint32_t num_atcam_bits = tbl_info->trie_depth - tbl_info->num_excluded_bits;

  /* Copy the entry_mspec subset to atcam_mspec */
  uint8_t val = 0;
  uint8_t mask = 0;
  uint8_t *src = entry_mspec->match_value_bits +
                 tbl_info->lpm_field_byte_offset + atcam_offset / 8;
  uint8_t *dst = atcam_mspec->match_value_bits + atcam_offset / 8;
  uint8_t *mask_src = entry_mspec->match_mask_bits +
                      tbl_info->lpm_field_byte_offset + atcam_offset / 8;
  uint8_t *mask_dst = atcam_mspec->match_mask_bits + atcam_offset / 8;
  int prefix_len = num_atcam_bits;
  int dst_offset = (num_atcam_bits % 8) ? (8 - num_atcam_bits % 8) : 0;

  while (prefix_len > 0) {
    uint8_t num_bits = prefix_len >= 8 ? 8 : prefix_len;
    val = get_byte(src, atcam_offset % 8, num_bits);
    mask = get_byte(mask_src, atcam_offset % 8, num_bits);

    set_byte(dst, dst_offset, val, num_bits);
    set_byte(mask_dst, dst_offset, mask, num_bits);

    src++;
    dst++;
    mask_src++;
    mask_dst++;
    prefix_len -= 8;
  }
}

static pipe_status_t pipe_mgr_alpm_atcam_entry_place(
    alpm_tbl_info_t *tbl_info,
    alpm_pipe_tbl_t *pipe_tbl,
    alpm_entry_t *entry,
    trie_subtree_t *subtree,
    bool is_covering_prefix,
    pipe_mgr_move_list_t **move_list) {
  pipe_status_t sts = PIPE_SUCCESS;

  if (!ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    // None of optimizations are enabled
    sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                    tbl_info->atcam_tbl_hdl,
                                    entry->match_spec,
                                    entry->act_fn_hdl,
                                    entry->act_spec,
                                    entry->ttl,
                                    pipe_tbl->sess_flags,
                                    &entry->sram_entry_hdl,
                                    move_list);
  } else if (tbl_info->num_excluded_bits) {
    uint32_t num_atcam_bytes = (tbl_info->lpm_field_key_width + 7) / 8;
    pipe_tbl_match_spec_t atcam_mspec = {0};
    uint8_t val[num_atcam_bytes];
    uint8_t mask[num_atcam_bytes];
    PIPE_MGR_MEMSET(val, 0, num_atcam_bytes);
    PIPE_MGR_MEMSET(mask, 0, num_atcam_bytes);

    PIPE_MGR_MEMCPY(
        &atcam_mspec, entry->match_spec, sizeof(pipe_tbl_match_spec_t));
    atcam_mspec.match_value_bits = val;
    atcam_mspec.match_mask_bits = mask;
    atcam_mspec.num_valid_match_bits =
        tbl_info->trie_depth - tbl_info->num_excluded_bits;
    atcam_mspec.num_match_bytes = num_atcam_bytes;

    setup_atcam_exclude_msb_bits_mspec(
        tbl_info, &atcam_mspec, entry->match_spec);

    sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                    tbl_info->atcam_tbl_hdl,
                                    &atcam_mspec,
                                    entry->act_fn_hdl,
                                    entry->act_spec,
                                    entry->ttl,
                                    pipe_tbl->sess_flags,
                                    &entry->sram_entry_hdl,
                                    move_list);
  } else {
    uint32_t subtree_root_depth = subtree->node->depth;
    uint32_t num_subset_key_bytes = (tbl_info->atcam_subset_key_width + 7) / 8;
    uint32_t num_subtree_id_bytes = 0;
    uint32_t num_subtree_id_bits = 0;
    /*
     * If max subtrees per partition
     * is more than 1, account for subtree id in match spec.
     */
    if (tbl_info->max_subtrees_per_partition > 1) {
      num_subtree_id_bits = log2(tbl_info->max_subtrees_per_partition);
      num_subtree_id_bytes = (num_subtree_id_bits + 7) / 8;
    }
    uint32_t num_atcam_bytes = num_subset_key_bytes + num_subtree_id_bytes;
    pipe_tbl_match_spec_t atcam_mspec = {0};
    uint8_t val[num_atcam_bytes];
    uint8_t mask[num_atcam_bytes];
    PIPE_MGR_MEMSET(val, 0, num_atcam_bytes);
    PIPE_MGR_MEMSET(mask, 0, num_atcam_bytes);

    PIPE_MGR_MEMCPY(
        &atcam_mspec, entry->match_spec, sizeof(pipe_tbl_match_spec_t));
    atcam_mspec.match_value_bits = val;
    atcam_mspec.match_mask_bits = mask;
    atcam_mspec.num_valid_match_bits =
        tbl_info->atcam_subset_key_width + num_subtree_id_bits;
    atcam_mspec.num_match_bytes = num_atcam_bytes;

    if (subtree_root_depth >
        (tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1)) {
      subtree_root_depth =
          tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1;
    }

    if (!is_covering_prefix &&
        subtree_root_depth > tbl_info->exm_fields_key_width) {
      setup_atcam_mspec(
          tbl_info, &atcam_mspec, entry->match_spec, subtree_root_depth);
    } else if (is_covering_prefix) {
      atcam_mspec.priority = tbl_info->trie_depth + 1;
    }

    /*
     * If max subtrees per partition
     * is more than 1, set the subtree id in match spec.
     */
    if (tbl_info->max_subtrees_per_partition > 1) {
      atcam_mspec.match_value_bits[0] = subtree->subtree_id;
      atcam_mspec.match_mask_bits[0] = 0xFF;  // Mask is always 0xFF
    }

    sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                    tbl_info->atcam_tbl_hdl,
                                    &atcam_mspec,
                                    entry->act_fn_hdl,
                                    entry->act_spec,
                                    entry->ttl,
                                    pipe_tbl->sess_flags,
                                    &entry->sram_entry_hdl,
                                    move_list);
  }

  return sts;
}

static pipe_status_t setup_covering_prefix(alpm_pipe_tbl_t *pipe_tbl,
                                           trie_subtree_t *subtree,
                                           pipe_mgr_move_list_t **move_head_p,
                                           pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  alpm_entry_t *entry = subtree->cov_pfx_entry;
  pipe_mgr_move_list_t *move_list = NULL;

  if (entry) {
    entry->match_spec->partition_index = subtree->partition->ptn_index;
    if (pipe_tbl->alpm_tbl_info->atcam_subset_key_width) {
      entry->match_spec->version_bits = pipe_tbl->alpm_tbl_info->cp_ver_bits;
    }
    sts = pipe_mgr_alpm_atcam_entry_place(
        tbl_info, pipe_tbl, entry, subtree, true, &move_list);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error adding covering prefix to alpm table 0x%x",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_ADD,
                             tbl_info->atcam_tbl_hdl,
                             move_list,
                             entry->sram_entry_hdl,
                             subtree->cov_pfx->entry->alpm_entry_hdl,
                             true);
    subtree->partition->size++;
  }

  return sts;
}

/*
 * Usage: cut_trie(pipe_tbl, new_subtree_node, orig_subtree)
 * ---------------------------------------------------------
 * Splits the given subtree into two, with the given node as the new root
 */
static trie_subtree_t *cut_trie(alpm_pipe_tbl_t *pipe_tbl,
                                trie_node_t *curr,
                                trie_subtree_t *subtree,
                                pipe_mgr_move_list_t **move_head_p,
                                pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  trie_subtree_t *new_subtree;
  trie_node_t *parent;

  if (curr == subtree->node) {
    return subtree;
  }

  /* Update the counts */
  for (parent = curr->parent; parent != subtree->node;
       parent = parent->parent) {
    backup_node(pipe_tbl, parent);
    parent->count -= curr->count;
  }
  backup_node(pipe_tbl, parent);
  parent->count -= curr->count;

  /* Creates the new subtree and adds its covering prefix if needed */
  backup_node(pipe_tbl, curr);
  backup_ptn(pipe_tbl, subtree->partition);

  new_subtree = create_subtree(pipe_tbl, curr, subtree->partition);
  sts = setup_covering_prefix(pipe_tbl, new_subtree, move_head_p, move_tail_p);
  return sts == PIPE_SUCCESS ? new_subtree : NULL;
}

/*
 * Usage: fill_subtree_entries(subtree_entry_nodes, node, start_index)
 * -------------------------------------------------------------------
 * Populates the given array with the entries that live under the given node.
 */
static void fill_subtree_entries(trie_node_t **subtree_entry_nodes,
                                 trie_node_t *node,
                                 uint32_t start_index) {
  trie_node_t *lc;
  trie_node_t *rc;
  uint32_t left_start_index = start_index;
  uint32_t right_start_index = 0;

  if (!node || node->count == 0) {
    return;
  }
  if (node->entry && node->entry->sram_entry_hdl) {
    subtree_entry_nodes[start_index] = node;
    left_start_index++;
  }
  right_start_index = left_start_index;

  lc = node->left_child;
  if (lc && !lc->subtree && lc->count > 0) {
    fill_subtree_entries(subtree_entry_nodes, lc, left_start_index);
    right_start_index += lc->count;
  }

  rc = node->right_child;
  if (rc && !rc->subtree && rc->count > 0) {
    fill_subtree_entries(subtree_entry_nodes, rc, right_start_index);
  }
}

uint8_t get_pre_class_act_fn_hdl_index(alpm_tbl_info_t *tbl_info,
                                       uint32_t subtree_root_depth) {
  if (tbl_info->atcam_subset_key_width == 0 ||
      subtree_root_depth <= tbl_info->exm_fields_key_width) {
    return 0;
  }

  uint8_t pre_act_fn_hdl_index = 0;
  uint8_t lpm_node_depth = subtree_root_depth - tbl_info->exm_fields_key_width;
  if (lpm_node_depth <=
      (tbl_info->lpm_field_key_width - tbl_info->atcam_subset_key_width)) {
    pre_act_fn_hdl_index = (lpm_node_depth - 1) / tbl_info->shift_granularity;
  } else {
    pre_act_fn_hdl_index = tbl_info->num_act_fn_hdls - 1;
  }

  return pre_act_fn_hdl_index;
}

/*
 * Usage: move_subtree(pipe_tbl, subtree, dst_ptn, move_head_p, move_tail_p)
 * -------------------------------------------------------------------------
 * Moves all entries in the given subtree to the given destination partition
 * and performs the appropriate preclassifier table changes and bookkeeping
 * updates. This is done in a make-then-break fashion to avoid traffic hits.
 */
static pipe_status_t move_subtree(alpm_pipe_tbl_t *pipe_tbl,
                                  trie_subtree_t *subtree,
                                  partition_info_t *dst,
                                  pipe_mgr_move_list_t **move_head_p,
                                  pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  partition_info_t *src = subtree->partition;
  uint32_t subtree_size = get_subtree_size(subtree->node);
  uint32_t arr_size = subtree_size ? subtree_size : 1;
  trie_node_t *subtree_entry_nodes[arr_size];
  pipe_mat_ent_hdl_t old_ent_hdls[arr_size];
  trie_node_t *node = NULL;
  alpm_entry_t *entry = NULL;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_action_spec_t *act_spec = NULL;
  pipe_mgr_move_list_t *move_list = NULL;
  uint32_t i = 0;

  PIPE_MGR_DBGCHK(dst->size + subtree_size <=
                  get_partition_depth(tbl_info, dst));

  backup_node(pipe_tbl, subtree->node);
  backup_ptn(pipe_tbl, src);
  backup_ptn(pipe_tbl, dst);

  for (uint32_t j = 0; j < arr_size; j++) {
    subtree_entry_nodes[j] = NULL;
    old_ent_hdls[j] = 0;
  }

  /* Organize all subtree entries, including the covering prefix */
  fill_subtree_entries(subtree_entry_nodes, subtree->node, 0);
  if (!subtree->node->entry && subtree->cov_pfx) {
    subtree_entry_nodes[subtree->node->count] = subtree->cov_pfx;
  } else {
    if (subtree_size > subtree->node->count) {
      subtree_entry_nodes[subtree->node->count] = NULL;
    }
  }

  /*
   * If ALPM key width optimization is used and max subtrees per partition
   * is more than 1, allocate the subtree id.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    int subtree_id = bf_id_allocator_allocate(dst->subtree_id_allocator);
    if (subtree_id == -1 ||
        subtree_id >= (int)tbl_info->max_subtrees_per_partition) {
      LOG_ERROR(
          "%s:%d, Failed to allocate subtree id for partition index %d, ALPM "
          "table 0x%x",
          __func__,
          __LINE__,
          dst->ptn_index,
          tbl_info->alpm_tbl_hdl);
      return PIPE_UNEXPECTED;
    }
    subtree->subtree_id = subtree_id;
  }

  /* Add all entries to the destination partition */
  for (i = 0; i < subtree_size; i++) {
    bool is_covering_prefix = false;
    node = subtree_entry_nodes[i];
    if (node) {
      if (node == subtree->cov_pfx) {
        entry = subtree->cov_pfx_entry;
        if (tbl_info->atcam_subset_key_width) {
          entry->match_spec->version_bits = tbl_info->cp_ver_bits;
        }
        is_covering_prefix = true;
      } else {
        entry = node->entry;
        entry->match_spec->version_bits = 0;
        backup_node(pipe_tbl, node);
      }
      entry->match_spec->partition_index = dst->ptn_index;
      old_ent_hdls[i] = entry->sram_entry_hdl;
      if (entry->sram_entry_hdl == 0) {
        continue;
      }
      sts = pipe_mgr_alpm_atcam_entry_place(
          tbl_info, pipe_tbl, entry, subtree, is_covering_prefix, &move_list);
      set_move_list(move_head_p, move_tail_p, move_list);
      if (sts != PIPE_SUCCESS) {
        goto cleanup;
      }
      if (entry == node->entry) {
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_ADD,
                                 tbl_info->atcam_tbl_hdl,
                                 move_list,
                                 entry->sram_entry_hdl,
                                 entry->alpm_entry_hdl,
                                 false);
      } else {
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_ADD,
                                 tbl_info->atcam_tbl_hdl,
                                 move_list,
                                 entry->sram_entry_hdl,
                                 node->entry->alpm_entry_hdl,
                                 true);
      }
      move_list = NULL;
      dst->size++;
    }
  }

  act_spec =
      build_act_spec(tbl_info, pipe_tbl, dst->ptn_index, subtree->subtree_id);
  if (!act_spec) {
    goto cleanup;
  }

  uint8_t pre_act_fn_hdl_index = 0;

  /*
   * If ALPM subset key width optimization is used, set the
   * correct preclassifier action handle
   */
  if (tbl_info->atcam_subset_key_width) {
    pre_act_fn_hdl_index =
        get_pre_class_act_fn_hdl_index(tbl_info, subtree->node->depth);
  }

  pipe_act_fn_hdl_t pre_act_fn_hdl =
      tbl_info->act_fn_hdls[pre_act_fn_hdl_index];

  if (subtree->tcam_entry_hdl) {
    /* Updates the preclassifier entry if it already exists */
    sts = pipe_mgr_tcam_ent_set_action(pipe_tbl->dev_tgt.device_id,
                                       tbl_info->preclass_tbl_hdl,
                                       subtree->tcam_entry_hdl,
                                       pre_act_fn_hdl,
                                       act_spec,
                                       pipe_tbl->sess_flags,
                                       &move_list);
    free_action_spec(act_spec);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      goto cleanup;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_MOD,
                             tbl_info->preclass_tbl_hdl,
                             move_list,
                             0,
                             0,
                             false);
    move_list = NULL;
  } else {
    /* Create a new preclassifier entry otherwise */
    entry = subtree->node->entry;
    if (entry) {
      match_spec = pipe_mgr_tbl_copy_match_spec(match_spec, entry->match_spec);
      if (match_spec) {
        match_spec->partition_index = 0;
      }
    } else {
      match_spec = build_match_spec(tbl_info, pipe_tbl, subtree->node);
    }
    sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                    tbl_info->preclass_tbl_hdl,
                                    match_spec,
                                    pre_act_fn_hdl,
                                    act_spec,
                                    0,
                                    pipe_tbl->sess_flags,
                                    &subtree->tcam_entry_hdl,
                                    &move_list);
    free_match_spec(match_spec);
    free_action_spec(act_spec);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      goto cleanup;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_ADD,
                             tbl_info->preclass_tbl_hdl,
                             move_list,
                             0,
                             0,
                             false);
    move_list = NULL;
  }
  remove_subtree_from_partition(pipe_tbl, src, subtree);
  subtree->partition = dst;
  dst->subtree_nodes[dst->num_subtrees] = subtree->node;
  dst->num_subtrees++;

  /* Delete all entries from the source partition */
  for (i = 0; i < subtree_size; i++) {
    if (subtree_entry_nodes[i] && old_ent_hdls[i]) {
      sts = pipe_mgr_tcam_entry_del(pipe_tbl->dev_tgt.device_id,
                                    tbl_info->atcam_tbl_hdl,
                                    old_ent_hdls[i],
                                    pipe_tbl->sess_flags,
                                    &move_list);
      set_move_list(move_head_p, move_tail_p, move_list);
      if (sts != PIPE_SUCCESS) {
        goto cleanup;
      }
      if (subtree_entry_nodes[i] != subtree->cov_pfx) {
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_DEL,
                                 tbl_info->atcam_tbl_hdl,
                                 move_list,
                                 old_ent_hdls[i],
                                 subtree_entry_nodes[i]->entry->alpm_entry_hdl,
                                 false);
      } else {
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_DEL,
                                 tbl_info->atcam_tbl_hdl,
                                 move_list,
                                 old_ent_hdls[i],
                                 0,
                                 true);
      }
      move_list = NULL;
      src->size--;
    }
  }

  return sts;

cleanup:
  LOG_ERROR("%s:%d %s(%d - 0x%x) Error moving alpm subtree sts %d",
            __func__,
            __LINE__,
            tbl_info->name,
            pipe_tbl->dev_tgt.device_id,
            tbl_info->alpm_tbl_hdl,
            sts);
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_basic_subtree_split(pipe_tbl, subtree, head, tail)
 * -----------------------------------------------------------------------
 * Perform a basic subtree split operation by splitting the given subtree
 * as evenly as possible and moving the lower half to a new partition.
 */
static pipe_status_t pipe_mgr_alpm_basic_subtree_split(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  trie_node_t *curr = NULL;
  trie_subtree_t *new_subtree = NULL;
  alpm_tbl_info_t *tbl_info = NULL;
  uint32_t ptn_depth = 0;

  if (!pipe_tbl || !subtree || !move_head_p || !move_tail_p) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  tbl_info = pipe_tbl->alpm_tbl_info;
  ptn_depth = get_partition_depth(tbl_info, subtree->partition);

  if (pipe_tbl->partitions_in_use >= tbl_info->num_partitions) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_NO_SPACE;
  }

  if ((ptn_depth / 2) > get_subtree_size(subtree->node)) {
    /*
     * If ALPM key width optimization is used and max subtrees per partiton
     * is more than 1, then free the subtree id in the current partition as new
     * subtree id will be created in dst partition when all the entries
     * get moved.
     */
    if (tbl_info->atcam_subset_key_width &&
        tbl_info->max_subtrees_per_partition > 1) {
      bf_id_allocator_release(subtree->partition->subtree_id_allocator,
                              subtree->subtree_id);
    }
    new_subtree = subtree;
  } else {
    /* Find and cut out the new subtree */
    curr = subtree->node;
    while (curr->count == subtree->node->count ||
           too_large(curr, ptn_depth / 2)) {
      curr = get_larger_child(curr);
      if (!curr) {
        return PIPE_UNEXPECTED;
      }
    }
    if (curr != subtree->node) {
      /*
       * If ALPM key width optimization is used and non-default
       * shift granularity is used, any subtree root should be chosen
       * based on the available preclassifier shift actions.
       */
      if (tbl_info->atcam_subset_key_width &&
          tbl_info->shift_granularity != 1) {
        while (curr) {
          uint8_t lpm_node_depth =
              curr->depth - tbl_info->exm_fields_key_width - 1;
          if ((lpm_node_depth % tbl_info->shift_granularity) == 0 ||
              (lpm_node_depth > (tbl_info->lpm_field_key_width -
                                 tbl_info->atcam_subset_key_width))) {
            /*
             * New subtree can be rooted at correct depth based on the
             * available shift actions
             */
            break;
          }
          /* Subtree can't be rooted at current depth, try walking down */
          curr = get_larger_child(curr);
        }
        if (!curr) {
          /*
           * The subtree can't be split. So, move to new partition.
           * For this, subtree id needs to be freed in case max subtrees per
           * partition is more than 1.
           */
          if (tbl_info->max_subtrees_per_partition > 1) {
            bf_id_allocator_release(subtree->partition->subtree_id_allocator,
                                    subtree->subtree_id);
          }
          new_subtree = subtree;
        }
      }
      if (curr) {
        new_subtree =
            cut_trie(pipe_tbl, curr, subtree, move_head_p, move_tail_p);
        if (!new_subtree) {
          return PIPE_UNEXPECTED;
        }
      }
    }
  }

  if (!new_subtree) {
    return PIPE_UNEXPECTED;
  }

  /* Create a new partition and move the new subtree into it */
  partition_info_t *p_info = create_partition_info(pipe_tbl, NULL);
  return move_subtree(pipe_tbl, new_subtree, p_info, move_head_p, move_tail_p);
}

/*
 * Usage: pipe_mgr_alpm_subtree_swap(pipe_tbl, swap, orig, head, tail)
 * -------------------------------------------------------------------
 * Performs a subtree swap by temporarily using an empty partition.
 * This sequence allows the swap to be performed without a traffic hit.
 */
static pipe_status_t pipe_mgr_alpm_subtree_swap(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *small_subtree,
    trie_subtree_t *large_subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  partition_info_t *large_ptn = large_subtree->partition;
  partition_info_t *small_ptn = small_subtree->partition;

  partition_info_t *free_partition = create_partition_info(pipe_tbl, NULL);

  /*
   * If ALPM key width optimization is used and max subtrees per partiton
   * is more than 1, then free the subtree ids in source partiton as new
   * subtree ids will be created in dst partition when the entries get moved.
   */
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    bf_id_allocator_release(large_ptn->subtree_id_allocator,
                            large_subtree->subtree_id);
    bf_id_allocator_release(small_ptn->subtree_id_allocator,
                            small_subtree->subtree_id);
  }
  sts = move_subtree(
      pipe_tbl, large_subtree, free_partition, move_head_p, move_tail_p);

  /*
   * Now free the subtree id in free_partition as well as those entries will
   * still be moved
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    bf_id_allocator_release(free_partition->subtree_id_allocator,
                            large_subtree->subtree_id);
  }

  sts |= move_subtree(
      pipe_tbl, small_subtree, large_ptn, move_head_p, move_tail_p);
  sts |= move_subtree(
      pipe_tbl, large_subtree, small_ptn, move_head_p, move_tail_p);

  remove_partition_info(pipe_tbl, free_partition);
  return sts;
}

static bool has_fully_excluded_entry(alpm_tbl_info_t *tbl_info,
                                     trie_subtree_t *subtree) {
  if (tbl_info->num_excluded_bits == 0) {
    return false;
  }
  /* Check if the root is fully excluded */
  if (subtree->node->depth <= tbl_info->num_excluded_bits) {
    return true;
  }

  /* Check if subtree uses a fully excluded entry as a covering prefix */
  if (subtree->cov_pfx &&
      subtree->cov_pfx->depth <= tbl_info->num_excluded_bits) {
    return true;
  }

  return false;
}

static bool is_subset(alpm_tbl_info_t *tbl_info,
                      trie_node_t *high,
                      trie_node_t *low,
                      bool *left_child) {
  /* Need to propagate check down to children if node is excluded */
  if (high->depth < tbl_info->num_excluded_bits) {
    if (!high->left_child ||
        (!high->left_child->subtree &&
         is_subset(tbl_info, high->left_child, low, left_child + 1))) {
      return true;
    }
    if (!high->right_child ||
        (!high->right_child->subtree &&
         is_subset(tbl_info, high->right_child, low, left_child + 1))) {
      return true;
    }
  }

  uint32_t index = 0;
  while (high->depth > low->depth) {
    high = left_child[index] ? high->left_child : high->right_child;
    if (!high) {
      return true;
    }
    if (high->subtree) {
      return false;
    }
    index++;
  }

  return high->subtree == NULL;
}

/* Check if two subtrees differ only in the atcam excluded bits. These cannot
 * live in the same partition.
 */
static bool overlapping_subtrees(alpm_tbl_info_t *tbl_info,
                                 trie_subtree_t *a,
                                 trie_subtree_t *b) {
  trie_node_t *a_node = a->cov_pfx ? a->cov_pfx : a->node;
  trie_node_t *b_node = b->cov_pfx ? b->cov_pfx : b->node;

  trie_node_t *high = a_node->depth < b_node->depth ? a_node : b_node;
  trie_node_t *low = a_node->depth > b_node->depth ? a_node : b_node;
  trie_node_t *curr = low;

  if (curr->depth > high->depth) {
    bool left_child[curr->depth - high->depth];
    PIPE_MGR_MEMSET(left_child, 0, (curr->depth - high->depth) * sizeof(bool));
    while (curr->depth > high->depth) {
      left_child[curr->depth - high->depth - 1] =
          curr == curr->parent->left_child;
      curr = curr->parent;
    }

    /* Not overlapping if one subtree is a subset of the other */
    if (curr == high && curr->depth > tbl_info->num_excluded_bits) {
      return false;
    }

    /* Check for disparity between the node depths */
    if (is_subset(tbl_info, high, low, left_child)) {
      return true;
    }
  }

  /* Check for disparity up to the most significant included bit */
  while (curr->depth > tbl_info->num_excluded_bits) {
    if ((curr->parent->left_child == curr &&
         high->parent->right_child == high) ||
        (curr->parent->right_child == curr &&
         high->parent->left_child == high)) {
      return false;
    }
    curr = curr->parent;
    high = high->parent;
  }

  return true;
}

static bool subtree_compatible_with_partition(alpm_tbl_info_t *tbl_info,
                                              trie_subtree_t *subtree,
                                              partition_info_t *p_info,
                                              uint32_t dest_index) {
  if (tbl_info->num_excluded_bits == 0) {
    return true;
  }

  /* Fully excluded subtrees need to have its own partition */
  if (has_fully_excluded_entry(tbl_info, subtree)) {
    return false;
  }

  trie_subtree_t *curr_subtree;
  for (uint32_t i = 0; i < p_info->num_subtrees; i++) {
    /* Only check subtrees that won't be swapped out */
    if (i == dest_index) {
      continue;
    }

    curr_subtree = p_info->subtree_nodes[i]->subtree;
    if (has_fully_excluded_entry(tbl_info, curr_subtree)) {
      return false;
    }
    if (overlapping_subtrees(tbl_info, subtree, curr_subtree)) {
      return false;
    }
  }

  return true;
}

/*
 * Usage: find_subtree_to_swap(pipe_tbl, subtree)
 * ----------------------------------------------
 * Iterates through all partitions in use and looks for a subtree that
 * fulfills the swapping conditions with the given subtree:
 *   -The second subtree must not be larger in size
 *   -Its corresponding partition must have free space after the swap
 *   -The resulting partitions must have at most one fully excluded entry
 *   -No pair of subtrees can overlap
 */
static trie_subtree_t *find_subtree_to_swap(alpm_pipe_tbl_t *pipe_tbl,
                                            trie_subtree_t *subtree) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  trie_subtree_t *curr_subtree;
  partition_info_t *p_info = subtree->partition;
  uint32_t curr_subtree_size;
  uint32_t subtree_size = get_subtree_size(subtree->node);
  uint32_t i, src_index = 0;

  for (i = 0; i < p_info->num_subtrees; i++) {
    if (subtree == p_info->subtree_nodes[i]->subtree) {
      src_index = i;
      break;
    }
  }
  if (i == p_info->num_subtrees) {
    return NULL;
  }

  for (p_info = pipe_tbl->ptn_in_use_list; p_info; p_info = p_info->next) {
    if (p_info == subtree->partition) {
      continue;
    }

    for (i = 0; i < p_info->num_subtrees; i++) {
      curr_subtree = p_info->subtree_nodes[i]->subtree;
      curr_subtree_size = get_subtree_size(curr_subtree->node);
      if (curr_subtree_size < subtree_size &&
          p_info->size - curr_subtree_size + subtree_size <
              get_partition_depth(tbl_info, p_info) - 1 &&
          subtree_compatible_with_partition(tbl_info, subtree, p_info, i) &&
          subtree_compatible_with_partition(
              tbl_info, curr_subtree, subtree->partition, src_index)) {
        return curr_subtree;
      }
    }
  }

  return NULL;
}

/*
 * Usage: pipe_mgr_alpm_subtree_split_move(pipe_tbl, subtree, head, tail)
 * ----------------------------------------------------------------------
 * Performs a split-then-move operation by searching for a partition in use
 * with an adequate amount of free space, or grabbing a fresh partition
 * if none are found. The given subtree is then split according to the
 * free space available and the new subtree is moved into the partition.
 */
static pipe_status_t pipe_mgr_alpm_subtree_split_move(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = NULL;
  trie_node_t *curr = NULL;
  trie_subtree_t *new_subtree = NULL;
  uint32_t subtree_size = 0;
  uint32_t space_left = 0;
  uint32_t free_space;
  partition_info_t *p_iter = NULL;
  partition_info_t *p_info = NULL;

  if (!pipe_tbl || !subtree || !move_head_p || !move_tail_p) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  tbl_info = pipe_tbl->alpm_tbl_info;
  subtree_size = get_subtree_size(subtree->node);

  /* Search for a partition with free space */
  for (p_iter = pipe_tbl->ptn_in_use_list; p_iter; p_iter = p_iter->next) {
    if (p_iter != subtree->partition &&
        get_partition_depth(tbl_info, p_iter) > 3 &&
        p_iter->num_subtrees < tbl_info->max_subtrees_per_partition &&
        p_iter->size <= get_split_cutoff(tbl_info, p_iter) &&
        subtree_compatible_with_partition(
            tbl_info, subtree, p_iter, p_iter->num_subtrees)) {
      free_space = get_partition_depth(tbl_info, p_iter) - p_iter->size;
      if (free_space > space_left) {
        space_left = free_space;
        p_info = p_iter;
        if (space_left > subtree_size) {
          break;
        }
      }
    }
  }
  /* Try to use a new partition if none are found */
  if (space_left == 0) {
    if (pipe_tbl->partitions_in_use == tbl_info->num_partitions) {
      return PIPE_NO_SPACE;
    }
    p_info = create_partition_info(pipe_tbl, NULL);
    space_left = get_partition_depth(tbl_info, p_info);
  }
  backup_ptn(pipe_tbl, p_info);

  if (space_left > subtree_size) {
    /*
     * If ALPM key width optimization is used and max subtrees per partiton
     * is more than 1, then free the subtree id in the current partition as new
     * subtree id will be created in dst partition when all the entries
     * get moved.
     */
    if (tbl_info->atcam_subset_key_width &&
        tbl_info->max_subtrees_per_partition > 1) {
      bf_id_allocator_release(subtree->partition->subtree_id_allocator,
                              subtree->subtree_id);
    }
    new_subtree = subtree;
  } else {
    /* Cut the subtree and move the new one to the second partition */
    curr = subtree->node;
    while (curr->count == subtree->node->count ||
           too_large(curr, space_left - 1)) {
      curr = get_larger_child(curr);
      if (!curr) {
        return PIPE_UNEXPECTED;
      }
    }

    if (curr != subtree->node) {
      /*
       * If ALPM key width optimization is used and non-default
       * shift granularity is used, any subtree root should be chosen
       * based on the available preclassifier shift actions.
       */
      if (tbl_info->atcam_subset_key_width &&
          tbl_info->shift_granularity != 1) {
        while (curr) {
          uint8_t lpm_node_depth =
              curr->depth - tbl_info->exm_fields_key_width - 1;
          if ((lpm_node_depth % tbl_info->shift_granularity) == 0 ||
              (lpm_node_depth > (tbl_info->lpm_field_key_width -
                                 tbl_info->atcam_subset_key_width))) {
            /*
             * New subtree can be rooted at correct depth based on the available
             * shift actions
             */
            break;
          }
          /* Subtree can't be rooted at current depth, try walking down */
          curr = get_larger_child(curr);
        }
        if (!curr) {
          /*
           * The subtree can't be split due to shift granularity restriction.
           * Also, there is no partition available to move the entire subtree.
           * Return error as the new entry can't be added.
           */
          LOG_ERROR("%s : %d, No enough space to add new ALPM entry",
                    __func__,
                    __LINE__);
          return PIPE_NO_SPACE;
        }
      }
      new_subtree = cut_trie(pipe_tbl, curr, subtree, move_head_p, move_tail_p);
      if (!new_subtree) {
        return PIPE_UNEXPECTED;
      }
    }
  }

  if (!new_subtree) {
    return PIPE_UNEXPECTED;
  }

  return move_subtree(pipe_tbl, new_subtree, p_info, move_head_p, move_tail_p);
}

static pipe_status_t pipe_mgr_alpm_is_preclass_full(alpm_pipe_tbl_t *pipe_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  dev_target_t dev_tgt = {.device_id = tbl_info->dev_id,
                          .dev_pipe_id = pipe_tbl->pipe_id};
  uint32_t count = 0;

  sts = pipe_mgr_tcam_get_placed_entry_count(
      dev_tgt, tbl_info->preclass_tbl_hdl, &count);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d: Failed to get preclassifier entry count for alpm table 0x%x "
        "pipe %d device %d",
        __func__,
        __LINE__,
        tbl_info->alpm_tbl_hdl,
        pipe_tbl->pipe_id,
        tbl_info->dev_id);
    return sts;
  }

  if (count >=
      (tbl_info->num_partitions * tbl_info->max_subtrees_per_partition - 2)) {
    LOG_ERROR("%s:%d: Preclassifier full for alpm table 0x%x pipe %d device %d",
              __func__,
              __LINE__,
              tbl_info->alpm_tbl_hdl,
              pipe_tbl->pipe_id,
              tbl_info->dev_id);
    return PIPE_NO_SPACE;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_alpm_make_space(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p,
    bool covering_prefix) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;

  sts = pipe_mgr_alpm_is_preclass_full(pipe_tbl);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d: Unable to make space in alpm table 0x%x pipe %d device %d",
        __func__,
        __LINE__,
        tbl_info->alpm_tbl_hdl,
        pipe_tbl->pipe_id,
        tbl_info->dev_id);
    return sts;
  }

  /* If we have not used most of the partitions, do a basic subtree split */
  if (tbl_info->partition_depth > 4 &&
      pipe_tbl->partitions_in_use < tbl_info->num_partitions * 0.95) {
    return pipe_mgr_alpm_basic_subtree_split(
        pipe_tbl, subtree, move_head_p, move_tail_p);
  }

  /* If we have at least one free partition and the subtree is small enough,
   * try to do a subtree swap. Note that we cannot perform swaps when making
   * room for a covering prefix, since the subtree state will be disrupted.
   */
  if (!covering_prefix &&
      pipe_tbl->partitions_in_use < tbl_info->num_partitions &&
      get_subtree_size(subtree->node) <
          get_split_cutoff(tbl_info, subtree->partition)) {
    trie_subtree_t *swap_subtree = find_subtree_to_swap(pipe_tbl, subtree);
    if (swap_subtree) {
      return pipe_mgr_alpm_subtree_swap(
          pipe_tbl, swap_subtree, subtree, move_head_p, move_tail_p);
    }
  }

  /* Default: try to split the subtree and move it to another partition */
  sts = pipe_mgr_alpm_subtree_split_move(
      pipe_tbl, subtree, move_head_p, move_tail_p);
  if (sts == PIPE_NO_SPACE) {
    LOG_ERROR("%s: Exceeded maximum number of partitions allowed", __func__);
  }

  return sts;
}

static uint32_t number_excluded_subtrees(alpm_tbl_info_t *tbl_info,
                                         partition_info_t *p_info) {
  uint32_t count = 0;
  for (uint32_t i = 0; i < p_info->num_subtrees; i++) {
    if (has_fully_excluded_entry(tbl_info, p_info->subtree_nodes[i]->subtree)) {
      count++;
    }
  }
  return count;
}

static partition_info_t *find_compatible_partition(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    partition_info_t **used_partitions,
    uint32_t num_used_partitions) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  uint32_t subtree_size = get_subtree_size(subtree->node);
  partition_info_t *p_info;
  uint32_t i;
  bool used;
  for (p_info = pipe_tbl->ptn_in_use_list; p_info; p_info = p_info->next) {
    if (p_info != subtree->partition &&
        get_partition_depth(tbl_info, p_info) > 3 &&
        p_info->num_subtrees < tbl_info->max_subtrees_per_partition &&
        (p_info->size + subtree_size) < get_partition_depth(tbl_info, p_info) &&
        subtree_compatible_with_partition(
            tbl_info, subtree, p_info, p_info->num_subtrees)) {
      /* Check if the partition has already been claimed by another
       * excluded subtree
       */
      used = false;
      for (i = 0; i < num_used_partitions; i++) {
        if (used_partitions[i] == p_info) {
          used = true;
          break;
        }
      }
      if (!used) {
        return p_info;
      }
    }
  }
  return NULL;
}

/*
 * Usage: check_excluded_entries(pipe_tbl, node)
 * ----------------------------------------------------------------------
 * Check that every partition can have compatible subtrees if there are
 * excluded bits.
 */
static bool check_excluded_entries(alpm_pipe_tbl_t *pipe_tbl,
                                   trie_node_t *node) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  partition_info_t *p_info = NULL;
  trie_subtree_t *subtree = NULL;
  uint32_t num_open_partitions = 0;
  uint32_t total_subtrees =
      tbl_info->num_partitions * tbl_info->max_subtrees_per_partition;
  bool found = true;
  uint32_t i = 0;

  if (tbl_info->num_excluded_bits > 0 &&
      node->depth <= tbl_info->num_excluded_bits && total_subtrees > 0) {
    trie_subtree_t *excluded_subtrees[total_subtrees];
    partition_info_t *used_partitions[pipe_tbl->partitions_in_use];
    uint32_t excluded_subtrees_size = 0, num_excluded_subtrees = 0;
    uint32_t num_used_partitions = 0;
    num_open_partitions =
        tbl_info->num_partitions - pipe_tbl->partitions_in_use;
    partition_info_t *p_swap = NULL;

    for (p_info = pipe_tbl->ptn_in_use_list; p_info; p_info = p_info->next) {
      num_excluded_subtrees = number_excluded_subtrees(tbl_info, p_info);
      for (i = 0; i < p_info->num_subtrees; i++) {
        subtree = p_info->subtree_nodes[i]->subtree;
        if (!subtree->node->entry && !subtree->cov_pfx &&
            node == find_covering_prefix(subtree->node)) {
          /* Temporarily mark this subtree with its covering prefix */
          subtree->cov_pfx = node;
          excluded_subtrees[excluded_subtrees_size] = subtree;
          excluded_subtrees_size++;
          num_excluded_subtrees++;
          if (num_excluded_subtrees > 1) {
            if (num_open_partitions) {
              num_open_partitions--;
            } else {
              p_swap = find_compatible_partition(
                  pipe_tbl, subtree, used_partitions, num_used_partitions);
              if (p_swap) {
                used_partitions[num_used_partitions] = p_swap;
                num_used_partitions++;
              } else {
                found = false;
              }
            }
          }
        }
        if (!found) {
          break;
        }
      }
      if (!found) {
        break;
      }
    }
    /* Clean up temporary subtree state */
    for (i = 0; i < excluded_subtrees_size; i++) {
      excluded_subtrees[i]->cov_pfx = NULL;
    }
  }

  return found;
}

/*
 * Usage: check_covering_prefixes_add(pipe_tbl, node)
 * ----------------------------------------------------------------------
 * Check to ensure all relevant underlying subtrees under the given node
 * can support a covering prefix
 */
static bool check_covering_prefixes_add(alpm_pipe_tbl_t *pipe_tbl,
                                        trie_node_t *node) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  uint32_t num_unused_partitions =
      (tbl_info->num_partitions - pipe_tbl->partitions_in_use) *
      tbl_info->max_subtrees_per_partition;
  uint32_t num_open_partitions = 0;
  uint32_t num_subtrees_found = 0;
  uint32_t num_full_subtrees = 0;
  partition_info_t *p_info = NULL;
  uint32_t total_subtrees =
      tbl_info->num_partitions * tbl_info->max_subtrees_per_partition;
  trie_subtree_t *subtree_list[total_subtrees];
  uint32_t num_cp_subtrees;
  uint32_t i;

  /* Walk down the node's child nodes.
   *
   * - num_subtrees_found is the number of underlying subtrees found.
   */
  walk_child_subtrees(subtree_list, node, node, &num_subtrees_found);

  /* For the partition of each subtree found we could potentially need space for
   * up to max_subtrees_per_partition covering prefix.
   */
  if (num_subtrees_found &&
      (num_subtrees_found * tbl_info->max_subtrees_per_partition >=
       num_unused_partitions)) {
    /* Walk through all partitions in use to determine the number of open
     * partitions and check that each overflowing partition of the underlying
     * subtrees can spread out its entries.
     */
    for (p_info = pipe_tbl->ptn_in_use_list; p_info; p_info = p_info->next) {
      num_cp_subtrees = 0;
      i = 0;
      while (i < num_subtrees_found) {
        if (p_info->ptn_index == subtree_list[i++]->partition->ptn_index)
          num_cp_subtrees++;
      }

      if (p_info->size + num_cp_subtrees >=
          get_partition_depth(tbl_info, p_info)) {
        num_full_subtrees += num_cp_subtrees;
      } else {
        if (get_partition_depth(tbl_info, p_info) > 3 &&
            p_info->num_subtrees < tbl_info->max_subtrees_per_partition &&
            p_info->size + num_cp_subtrees <
                get_split_cutoff(tbl_info, p_info)) {
          num_open_partitions++;
        }
      }
    }

    /* Do we have enough open partitions to install new covering prefix */
    if ((num_open_partitions + num_unused_partitions) <= num_full_subtrees)
      return false;
  }

  /* Check excluded entries if necessary */
  if (tbl_info->num_excluded_bits > 0 &&
      node->depth <= tbl_info->num_excluded_bits && total_subtrees > 0)
    return check_excluded_entries(pipe_tbl, node);

  return true;
}

static pipe_status_t pipe_mgr_alpm_move_subtree(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  partition_info_t *p_info = NULL;

  /* Try to use a new partition */
  if (pipe_tbl->partitions_in_use < tbl_info->num_partitions) {
    p_info = create_partition_info(pipe_tbl, NULL);
  }

  /* Search for a partition with free space if all partitions are used */
  if (!p_info) {
    p_info = find_compatible_partition(pipe_tbl, subtree, NULL, 0);
    if (!p_info) {
      LOG_ERROR(
          "%s:%d No compatible partition found for subtree in alpm table 0x%x",
          __func__,
          __LINE__,
          tbl_info->alpm_tbl_hdl);
      return PIPE_NO_SPACE;
    }
  }

  backup_ptn(pipe_tbl, p_info);

  /*
   * If ALPM key width optimization is used and max subtrees per partiton
   * is more than 1, then free the subtree id in source partiton as new
   * subtree id will be created in dst partition when the entries get moved.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    bf_id_allocator_release(subtree->partition->subtree_id_allocator,
                            subtree->subtree_id);
  }

  return move_subtree(pipe_tbl, subtree, p_info, move_head_p, move_tail_p);
}

/*
 * Usage: update_subtree_covering_prefix(pipe_tbl, subtree, node, head, tail)
 * -------------------------------------------------------------------
 * Performs the necessary subtree covering prefix update.
 */
static pipe_status_t update_subtree_covering_prefix(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    trie_node_t *node,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  pipe_mgr_move_list_t *move_list = NULL;
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_entry_t *entry;

  backup_ptn(pipe_tbl, subtree->partition);
  add_subtree_to_cp_list(pipe_tbl, node, subtree);

  if (number_excluded_subtrees(tbl_info, subtree->partition) > 1) {
    sts =
        pipe_mgr_alpm_move_subtree(pipe_tbl, subtree, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to move subtree in alpm table 0x%x",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  /* Make space if the partition is already full */
  if (subtree->partition->size ==
      get_partition_depth(tbl_info, subtree->partition)) {
    sts = pipe_mgr_alpm_make_space(
        pipe_tbl, subtree, move_head_p, move_tail_p, true);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to make space in alpm table 0x%x",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  entry = subtree->cov_pfx_entry;
  if (tbl_info->atcam_subset_key_width) {
    entry->match_spec->version_bits = tbl_info->cp_ver_bits;
  }
  entry->match_spec->partition_index = subtree->partition->ptn_index;
  sts = pipe_mgr_alpm_atcam_entry_place(
      tbl_info, pipe_tbl, entry, subtree, true, &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);
  if (sts != PIPE_SUCCESS) {
    return sts;
  } else {
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_ADD,
                             tbl_info->atcam_tbl_hdl,
                             move_list,
                             entry->sram_entry_hdl,
                             node->entry->alpm_entry_hdl,
                             true);
    move_list = NULL;
    subtree->partition->size++;
  }

  return sts;
}

/*
 * Usage: check_underlying_subtrees(pipe_tbl, cp_node, node, head, tail)
 * -------------------------------------------------------------------
 * Walk down the node's child nodes to see if underlying subtrees require
 * a covering prefix and perform the necessary covering prefix updates when
 * needed.
 */
static pipe_status_t check_underlying_subtrees(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_node_t *cp_node,
    trie_node_t *node,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  trie_node_t *lc;
  trie_node_t *rc;

  if (!node || (node->entry && node != cp_node)) {
    return sts;
  }

  lc = node->left_child;
  if (lc) {
    if (lc->subtree && lc->subtree->partition && !lc->subtree->node->entry &&
        !lc->subtree->cov_pfx &&
        cp_node == find_covering_prefix_with_node(lc->subtree->node, cp_node)) {
      /* Covering prefix needed */
      sts = update_subtree_covering_prefix(
          pipe_tbl, lc->subtree, cp_node, move_head_p, move_tail_p);
      if (sts != PIPE_SUCCESS) return sts;
    }
    sts = check_underlying_subtrees(
        pipe_tbl, cp_node, lc, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) return sts;
  }

  rc = node->right_child;
  if (rc) {
    if (rc->subtree && rc->subtree->partition && !rc->subtree->node->entry &&
        !rc->subtree->cov_pfx &&
        cp_node == find_covering_prefix_with_node(rc->subtree->node, cp_node)) {
      /* Covering prefix needed */
      sts = update_subtree_covering_prefix(
          pipe_tbl, rc->subtree, cp_node, move_head_p, move_tail_p);
      if (sts != PIPE_SUCCESS) return sts;
    }
    sts = check_underlying_subtrees(
        pipe_tbl, cp_node, rc, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) return sts;
  }

  return sts;
}

/*
 * Usage: update_covering_prefixes_add(pipe_tbl, added_node, head, tail)
 * ---------------------------------------------------------------------
 * Search for and perform the necessary covering prefix updates when an entry
 * is added. This may be the case for subtrees that use this node's covering
 * prefix as their covering prefixes, as we may have added our new node along
 * that path. If this node does not have a covering prefix, we must default to
 * searching through all subtrees.
 */
static pipe_status_t update_covering_prefixes_add(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_node_t *node,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  trie_node_t *cp = NULL;
  trie_node_t *new_cp;
  trie_subtree_t *subtree;
  alpm_entry_t *entry;
  pipe_mat_ent_hdl_t old_sram_hdl;
  pipe_mgr_move_list_t *move_list = NULL;
  uint32_t i = 0;
  pipe_status_t sts = PIPE_SUCCESS;

  cp = find_covering_prefix(node);

  if (cp) {
    /* Covering prefix found, check only these subtrees */
    while (i < cp->cov_pfx_count) {
      subtree = cp->cov_pfx_subtree_nodes[i]->subtree;
      new_cp = find_covering_prefix(subtree->node);
      if (new_cp != cp) {
        /* Note that new_cp must be the added node here */
        old_sram_hdl = subtree->cov_pfx_entry->sram_entry_hdl;
        backup_ptn(pipe_tbl, subtree->partition);
        remove_subtree_from_cp_list(pipe_tbl, cp, subtree);
        if (subtree->node != new_cp) {
          add_subtree_to_cp_list(pipe_tbl, new_cp, subtree);
          entry = subtree->cov_pfx_entry;
          if (tbl_info->atcam_subset_key_width) {
            entry->match_spec->version_bits = tbl_info->cp_ver_bits;
          }
          if (subtree->partition) {
            entry->match_spec->partition_index = subtree->partition->ptn_index;
            sts = pipe_mgr_alpm_atcam_entry_place(
                tbl_info, pipe_tbl, entry, subtree, true, &move_list);
            set_move_list(move_head_p, move_tail_p, move_list);
            if (sts != PIPE_SUCCESS) {
              return sts;
            }
            build_alpm_move_list_hdr(pipe_tbl,
                                     PIPE_MAT_UPDATE_ADD,
                                     tbl_info->atcam_tbl_hdl,
                                     move_list,
                                     entry->sram_entry_hdl,
                                     new_cp->entry->alpm_entry_hdl,
                                     true);
            move_list = NULL;
            subtree->partition->size++;
          } else {
            return PIPE_UNEXPECTED;
          }
        }

        sts = pipe_mgr_tcam_entry_del(pipe_tbl->dev_tgt.device_id,
                                      tbl_info->atcam_tbl_hdl,
                                      old_sram_hdl,
                                      pipe_tbl->sess_flags,
                                      &move_list);
        set_move_list(move_head_p, move_tail_p, move_list);
        if (sts != PIPE_SUCCESS) {
          return sts;
        }
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_DEL,
                                 tbl_info->atcam_tbl_hdl,
                                 move_list,
                                 old_sram_hdl,
                                 0,
                                 true);
        move_list = NULL;
        if (subtree->partition) {
          subtree->partition->size--;
        }
      } else {
        i++;
      }
    }
  } else {
    /* No covering prefix, check node's underlying subtrees */
    sts = check_underlying_subtrees(
        pipe_tbl, node, node, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to update subtree in alpm table 0x%x",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
    }
  }
  return sts;
}

static pipe_status_t setup_root(alpm_pipe_tbl_t *pipe_tbl,
                                trie_subtree_t *subtree,
                                pipe_mgr_move_list_t **move_head_p,
                                pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  partition_info_t *p_info;
  pipe_mgr_move_list_t *move_list = NULL;

  /* Preemptive check */
  if (!subtree->tcam_entry_hdl) {
    sts = pipe_mgr_alpm_is_preclass_full(pipe_tbl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Failed to set up root preclassifier entry for alpm tbl 0x%x",
          __func__,
          __LINE__,
          tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  /* Set up the root subtree's partition info */
  if (!subtree->partition) {
    backup_node(pipe_tbl, subtree->node);

    if (pipe_tbl->partitions_in_use < tbl_info->num_partitions) {
      create_partition_info(pipe_tbl, subtree);
    } else {
      for (p_info = pipe_tbl->ptn_in_use_list; p_info; p_info = p_info->next) {
        if (p_info->num_subtrees < tbl_info->max_subtrees_per_partition &&
            p_info->size < (get_partition_depth(tbl_info, p_info) - 1) &&
            subtree_compatible_with_partition(
                tbl_info, subtree, p_info, p_info->num_subtrees)) {
          backup_ptn(pipe_tbl, p_info);
          subtree->partition = p_info;
          p_info->subtree_nodes[p_info->num_subtrees] = subtree->node;
          p_info->num_subtrees++;
          break;
        }
      }
      if (!subtree->partition) {
        LOG_ERROR("%s: Exceeded maximum number of partitions allowed",
                  __func__);
        return PIPE_NO_SPACE;
      }
    }
  }

  /*
   * If ALPM key width optimization is used and max subtrees per partition
   * is more than 1, allocate the subtree id.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    int subtree_id =
        bf_id_allocator_allocate(subtree->partition->subtree_id_allocator);
    if (subtree_id == -1 ||
        subtree_id >= (int)tbl_info->max_subtrees_per_partition) {
      LOG_ERROR(
          "%s:%d, Failed to allocate subtree id for partition index %d, ALPM "
          "table 0x%x",
          __func__,
          __LINE__,
          subtree->partition->ptn_index,
          tbl_info->alpm_tbl_hdl);
      return PIPE_UNEXPECTED;
    }
    subtree->subtree_id = subtree_id;
  }

  /* Set up the root subtree's covering prefix and preclassifier entry */
  if (!subtree->tcam_entry_hdl) {
    sts = setup_covering_prefix(pipe_tbl, subtree, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to set up covering prefix in alpm table 0x%x",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
    backup_node(pipe_tbl, subtree->node);

    pipe_tbl_match_spec_t *subtree_match_spec =
        build_match_spec(tbl_info, pipe_tbl, subtree->node);
    pipe_action_spec_t *subtree_act_spec = build_act_spec(
        tbl_info, pipe_tbl, subtree->partition->ptn_index, subtree->subtree_id);

    uint8_t pre_act_fn_hdl_index = 0;

    /*
     * If ALPM subset key width optimization is used, set the
     * correct preclassifier action handle
     */
    if (tbl_info->atcam_subset_key_width) {
      pre_act_fn_hdl_index =
          get_pre_class_act_fn_hdl_index(tbl_info, subtree->node->depth);
    }

    pipe_act_fn_hdl_t pre_act_fn_hdl =
        tbl_info->act_fn_hdls[pre_act_fn_hdl_index];

    sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                    tbl_info->preclass_tbl_hdl,
                                    subtree_match_spec,
                                    pre_act_fn_hdl,
                                    subtree_act_spec,
                                    0,
                                    pipe_tbl->sess_flags,
                                    &subtree->tcam_entry_hdl,
                                    &move_list);
    free_match_spec(subtree_match_spec);
    free_action_spec(subtree_act_spec);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(%d - 0x%x) Error adding alpm preclassifier entry sts %d",
          __func__,
          __LINE__,
          tbl_info->name,
          pipe_tbl->dev_tgt.device_id,
          tbl_info->alpm_tbl_hdl,
          sts);

      return sts;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_ADD,
                             tbl_info->preclass_tbl_hdl,
                             move_list,
                             0,
                             0,
                             false);
  }

  return sts;
}

static pipe_status_t replace_covering_prefix(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    alpm_entry_t *entry,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  trie_node_t *node = subtree->node;
  pipe_mgr_move_list_t *move_list = NULL;

  entry->match_spec->version_bits = 0;  // reset version bits
  sts = pipe_mgr_alpm_atcam_entry_place(
      tbl_info, pipe_tbl, entry, subtree, false, &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);
  if (sts != PIPE_SUCCESS) {
    return sts;
  }
  node->entry = entry;
  build_alpm_move_list_hdr(pipe_tbl,
                           PIPE_MAT_UPDATE_ADD,
                           tbl_info->atcam_tbl_hdl,
                           move_list,
                           entry->sram_entry_hdl,
                           entry->alpm_entry_hdl,
                           false);
  move_list = NULL;

  sts = pipe_mgr_tcam_entry_del(tbl_info->dev_id,
                                tbl_info->atcam_tbl_hdl,
                                subtree->cov_pfx_entry->sram_entry_hdl,
                                pipe_tbl->sess_flags,
                                &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting covering prefix sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              pipe_tbl->dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    return sts;
  } else {
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_DEL,
                             tbl_info->atcam_tbl_hdl,
                             move_list,
                             subtree->cov_pfx_entry->sram_entry_hdl,
                             0,
                             true);
    remove_subtree_from_cp_list(pipe_tbl, subtree->cov_pfx, subtree);
  }

  return sts;
}

static pipe_status_t remove_subtree_from_trie(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_subtree_t *subtree,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  pipe_mgr_move_list_t *move_list = NULL;
  partition_info_t *p_info = subtree->partition;

  if (subtree->cov_pfx_entry) {
    /* Remove the covering prefix from the empty subtree */
    sts = pipe_mgr_tcam_entry_del(tbl_info->dev_id,
                                  tbl_info->atcam_tbl_hdl,
                                  subtree->cov_pfx_entry->sram_entry_hdl,
                                  pipe_tbl->sess_flags,
                                  &move_list);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting covering prefix sts %d",
                __func__,
                __LINE__,
                tbl_info->name,
                pipe_tbl->dev_tgt.device_id,
                tbl_info->alpm_tbl_hdl,
                sts);
      clean_alpm_move_list_hdr(pipe_tbl);
      return sts;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_DEL,
                             tbl_info->atcam_tbl_hdl,
                             move_list,
                             subtree->cov_pfx_entry->sram_entry_hdl,
                             0,
                             true);
    move_list = NULL;
    remove_subtree_from_cp_list(pipe_tbl, subtree->cov_pfx, subtree);
    p_info->size--;
  }

  /* Delete this subtree and its corresponding preclassifier entry */
  sts = pipe_mgr_tcam_entry_del(tbl_info->dev_id,
                                tbl_info->preclass_tbl_hdl,
                                subtree->tcam_entry_hdl,
                                pipe_tbl->sess_flags,
                                &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting preclassifier entry sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->dev_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    clean_alpm_move_list_hdr(pipe_tbl);
    return sts;
  }
  build_alpm_move_list_hdr(pipe_tbl,
                           PIPE_MAT_UPDATE_DEL,
                           tbl_info->preclass_tbl_hdl,
                           move_list,
                           0,
                           0,
                           false);
  move_list = NULL;

  /*
   * If ALPM key width optimization is used and max subtrees per partition
   * is more than 1, free the subtree id.
   */
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    bf_id_allocator_release(p_info->subtree_id_allocator, subtree->subtree_id);
  }

  remove_subtree(pipe_tbl, subtree);
  subtree->tcam_entry_hdl = 0;

  /* Clear the partition structure if it's empty */
  if (p_info->size == 0 && p_info->num_subtrees == 0) {
    remove_partition_info(pipe_tbl, p_info);
  }

  return sts;
}

/* Usage: find_lower_node(root_node)
 * ---------------------------------
 * Find a node in the same subtree below the given node.
 */
static trie_node_t *find_lower_node(trie_node_t *node) {
  if (node->left_child && !node->left_child->subtree &&
      node->left_child->count > 0) {
    if (node->left_child->entry) {
      return node->left_child;
    } else {
      return find_lower_node(node->left_child);
    }
  }

  if (node->right_child && !node->right_child->subtree &&
      node->right_child->count > 0) {
    if (node->right_child->entry) {
      return node->right_child;
    } else {
      return find_lower_node(node->right_child);
    }
  }

  return NULL;
}

/*
 * Usage: restructure_subtree(pipe_tbl, prev_subtree, curr, root,
 *                            &subtree, &move_head_p, &move_tail_p)
 * ----------------------------------------------------------------
 * Instead of creating a new subtree at deep nodes, try to restructure the
 * existing subtree (if it exists) or try to create new subtree at the
 * possible top most node to improve the utilization.
 */
trie_node_t *restructure_subtree(alpm_pipe_tbl_t *pipe_tbl,
                                 trie_subtree_t *prev_subtree,
                                 trie_node_t *curr,
                                 trie_node_t *root,
                                 trie_subtree_t **subtree,
                                 pipe_mgr_move_list_t **move_head_p,
                                 pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  pipe_status_t sts = PIPE_SUCCESS;

  /*
   * This function should never be called if ATCAM key
   * width optimization is not used.
   */
  if (tbl_info->atcam_subset_key_width == 0) {
    LOG_ERROR(
        "%s %d, restructuring of subtrees can be done only if "
        "ATCAM key width optimization is used for tbl %s, hdl 0x%x",
        __func__,
        __LINE__,
        tbl_info->name,
        tbl_info->alpm_tbl_hdl);
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  if (subtree == NULL || move_head_p == NULL || move_tail_p == NULL) {
    LOG_ERROR(
        "%s %d, NULL arguments to restructure the subtrees for "
        "ALPM tbl %s, hdl 0x%x",
        __func__,
        __LINE__,
        tbl_info->name,
        tbl_info->alpm_tbl_hdl);
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  /*
   * Instead of creating a new subtree at deep nodes, try to restructure the
   * existing subtree (if it exists) or try to create new subtree at the
   * possible top most node to improve the utilization.
   *
   * NOTE:
   *    Based on current code, restructuring of subtrees
   *    for each route entry add doesn't seem to impact
   *    performance much but in case this function needs to be
   *    optimized still, the below items can be optimized -
   *    - Caching the list of parent nodes until subset width length while
   *      building/walking down the trie
   *    - If the in-use partitions can be sorted based on the max available
   *      space, then iterating through all the partitions to find less used
   *      one can be avoided
   */
  if (prev_subtree) {
    /*
     * Restructure the existing subtree -
     *    - Find a new subtree root at the top most node for the current
     *      node that is getting added
     *    - Move the new subtree's route nodes to a different partition
     *    - Delete the moved route nodes from old subtree/partition
     *    - If no more route nodes exist in prev/old subtree, delete
     *      the subtree and its preclassifier entry
     */

    /* Find the possible new subtree root at top most level from curr node */
    trie_node_t *new_subtree_root = curr, *temp = curr;
    int level = tbl_info->atcam_subset_key_width;
    while (level-- > 1) {
      uint8_t lpm_node_depth = temp->depth - tbl_info->exm_fields_key_width - 1;
      if ((lpm_node_depth % tbl_info->shift_granularity) == 0 ||
          (lpm_node_depth >= (tbl_info->lpm_field_key_width -
                              tbl_info->atcam_subset_key_width))) {
        new_subtree_root = temp;
      }
      temp = temp->parent;
    }

    /*
     * If the new subtree root is out of coverage from previous subtree or
     * the new top most subtree doesn't have any route nodes, then no
     * restructuring needs to be done to prev subtree. New subtree needs
     * to be created and returned.
     */
    if (new_subtree_root->count == 0 ||
        new_subtree_root->depth >=
            prev_subtree->node->depth + tbl_info->atcam_subset_key_width) {
      *subtree = create_subtree(pipe_tbl, new_subtree_root, NULL);
      return curr;
    }

    uint32_t space_left = 0;
    uint32_t free_space = 0;
    partition_info_t *p_iter = NULL;
    partition_info_t *p_info = NULL;
    partition_info_t *max_space_p_info = NULL;

    /* Search for a partition with max free space */
    for (p_iter = pipe_tbl->ptn_in_use_list; p_iter; p_iter = p_iter->next) {
      if (p_iter != prev_subtree->partition &&
          p_iter->num_subtrees < tbl_info->max_subtrees_per_partition &&
          p_iter->size <= get_split_cutoff(tbl_info, p_iter)) {
        free_space = get_partition_depth(tbl_info, p_iter) - p_iter->size;
        if (free_space > space_left) {
          space_left = free_space;
          max_space_p_info = p_iter;
          if (space_left > new_subtree_root->count + 1) {
            p_info = p_iter;
            break;
          }
        }
      }
    }

    if (p_info == NULL) {
      /* Try to select a new subtree root based on max space available */
      trie_node_t *top_most_subtree_root = new_subtree_root;
      temp = curr;
      while (temp->parent != top_most_subtree_root &&
             space_left > temp->parent->count + 1) {
        temp = temp->parent;
        uint8_t lpm_node_depth =
            temp->depth - tbl_info->exm_fields_key_width - 1;
        if ((lpm_node_depth % tbl_info->shift_granularity) == 0 ||
            (lpm_node_depth >= (tbl_info->lpm_field_key_width -
                                tbl_info->atcam_subset_key_width))) {
          new_subtree_root = temp;
        }
      }

      /*
       * Since the new subtree root is changed, again check
       * if the new subtree root is out of coverage from previous subtree or
       * the new top most subtree doesn't have any route nodes. If so, then no
       * restructuring needs to be done to prev subtree. New subtree needs
       * to be created and returned.
       */
      if (new_subtree_root->count == 0 ||
          new_subtree_root->depth >=
              prev_subtree->node->depth + tbl_info->atcam_subset_key_width) {
        *subtree = create_subtree(pipe_tbl, new_subtree_root, NULL);
        return curr;
      }

      /* Use a new partition if none of the in-use partitions can be used */
      if (space_left <= new_subtree_root->count + 1) {
        if (pipe_tbl->partitions_in_use == tbl_info->num_partitions) {
          *subtree = create_subtree(pipe_tbl, root, NULL);
          return curr;
        }
        p_info = create_partition_info(pipe_tbl, NULL);
      } else {
        p_info = max_space_p_info;
      }
    }
    backup_ptn(pipe_tbl, p_info);

    trie_subtree_t *new_subtree = NULL;

    /* Create the new subtree by cutting it off from previous subtree */
    new_subtree = cut_trie(
        pipe_tbl, new_subtree_root, prev_subtree, move_head_p, move_tail_p);
    if (!new_subtree) {
      LOG_ERROR(
          "%s %d, New subtree creation off existing subtree failed for "
          "ALPM tbl %s, hdl 0x%x",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl);
      return NULL;
    }

    /* Move the new subtree's route nodes to its partition */
    sts = move_subtree(pipe_tbl, new_subtree, p_info, move_head_p, move_tail_p);
    if (sts) {
      LOG_ERROR(
          "%s %d, Moving part of the existing subtree nodes failed for "
          "ALPM tbl %s, hdl 0x%x",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl);
      return NULL;
    }

    /* Delete the previous subtree in case it doesn't have any route nodes */
    if (prev_subtree->node->count == 0) {
      sts = remove_subtree_from_trie(
          pipe_tbl, prev_subtree, move_head_p, move_tail_p);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Failed to remove empty subtree in alpm tbl 0x%x",
                  __func__,
                  __LINE__,
                  tbl_info->alpm_tbl_hdl);
        clean_alpm_move_list_hdr(pipe_tbl);
        return NULL;
      }
      prev_subtree->node->subtree = NULL;
      free_subtree(prev_subtree);
    }

    *subtree = new_subtree;
  } else {
    /*
     * Create the new subtree at the possible top most
     * node from the current node that is getting added.
     */
    trie_node_t *temp = curr, *new_root = root;

    for (uint32_t i = 0; i < tbl_info->atcam_subset_key_width - 1 &&
                         temp->depth > tbl_info->exm_fields_key_width + 1;
         i++) {
      temp = temp->parent;
      if (temp->count) {
        break;
      }
      uint8_t lpm_node_depth = temp->depth - tbl_info->exm_fields_key_width - 1;
      if ((lpm_node_depth % tbl_info->shift_granularity) == 0 ||
          (lpm_node_depth >= (tbl_info->lpm_field_key_width -
                              tbl_info->atcam_subset_key_width))) {
        new_root = temp;
      }
    }

    root = new_root;
    *subtree = create_subtree(pipe_tbl, root, NULL);
  }

  return curr;
}

/*
 * Usage: find_node(pipe_tbl, match_spec, &subtree, &move_head_p, &move_tail_p)
 * ------------------------------------------------
 * Traverses the trie of the given pipe table to find and return the node
 * that corresponds to the given match spec. Saves the subtree of the node
 * into the provided subtree pointer argument.
 */
trie_node_t *find_node(alpm_pipe_tbl_t *pipe_tbl,
                       pipe_tbl_match_spec_t *match_spec,
                       bool is_cp_restore,
                       trie_subtree_t **subtree,
                       pipe_mgr_move_list_t **move_head_p,
                       pipe_mgr_move_list_t **move_tail_p) {
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  uint32_t node_depth;
  bool is_left;
  trie_node_t *curr = pipe_tbl->root, *root = pipe_tbl->root;
  trie_node_t *child;
  trie_subtree_t *prev_subtree = NULL;

  node_depth = get_match_spec_node_depth(tbl_info, match_spec);

  /*
   * If ALPM subset key width optimization is used, subtrees shouldn't start
   * in EXM fields depth, so set to NULL at the begining.
   */
  if (tbl_info->atcam_subset_key_width) {
    root = NULL;
  }

  // Restore in scale opt case, we know where to start.
  if (subtree && *subtree && (*subtree)->node) curr = (*subtree)->node;

  while (curr->depth < node_depth) {
    /* If we've found a more specific subtree, don't create a new one */
    if (subtree && curr->subtree) {
      *subtree = curr->subtree;
      root = NULL;
      prev_subtree = NULL;
    }

    is_left = should_go_left(tbl_info, match_spec, curr->depth);
    if (is_left) {
      child = curr->left_child;
    } else {
      child = curr->right_child;
    }
    if (!child) {
      child = create_node(curr, is_left);
    }
    curr = child;

    /*
     * Once we get past the EXM fields, set the root if
     * ATCAM subset key width optimization is used
     */
    if (subtree && tbl_info->atcam_subset_key_width &&
        curr->depth == tbl_info->exm_fields_key_width + 1) {
      root = curr;
      *subtree = NULL;
      prev_subtree = NULL;
    }

    /* If there are excluded bits, new subtrees should start from the first
     * included bit
     */
    if (subtree && tbl_info->num_excluded_bits &&
        curr->depth == tbl_info->num_excluded_bits + 1) {
      root = curr;
      *subtree = NULL;
    }

    /*
     * If ATCAM subset key width optimization is used, subtrees depth
     * shouldn't be greater than subset key width
     */
    if (subtree && tbl_info->atcam_subset_key_width) {
      if ((*subtree &&
           (*subtree)->node->depth + tbl_info->atcam_subset_key_width <=
               curr->depth) ||
          (root &&
           root->depth + tbl_info->atcam_subset_key_width <= curr->depth)) {
        /*
         * Remember the current subtree (if valid) to check
         * for possible restructuring later
         */
        if (*subtree && tbl_info->atcam_subset_key_width) {
          prev_subtree = *subtree;
        } else {
          prev_subtree = NULL;
        }
        root = curr;
        *subtree = NULL;
      }
    }
  }

  if (subtree) {
    if (curr->subtree) {
      *subtree = curr->subtree;
      root = NULL;
    }

    /* Fully excluded entries require their own subtrees */
    if ((tbl_info->num_excluded_bits &&
         curr->depth <= tbl_info->num_excluded_bits) ||
        tbl_info->partition_depth == 1) {
      root = curr;
    }

    /*
     * If ATCAM key width optimization is used and if the prefix length for
     * this route is 0, then this entry requires its own subtree
     */
    if (tbl_info->atcam_subset_key_width &&
        curr->depth <= tbl_info->exm_fields_key_width) {
      root = curr;
    }

    if (root && root->subtree == NULL) {
      /*
       * If ATCAM key width optimization is used, try to restructure the
       * existing subtree or create the subtree at possible top most node
       * to improve utilization. This needs to be done only for non-zero
       * prefix length routes.
       */
      if (tbl_info->atcam_subset_key_width &&
          curr->depth > tbl_info->exm_fields_key_width) {
        return restructure_subtree(pipe_tbl,
                                   prev_subtree,
                                   curr,
                                   root,
                                   subtree,
                                   move_head_p,
                                   move_tail_p);
      }

      *subtree = create_subtree(pipe_tbl, root, NULL);
    }
  }

  // For covering prefix restore, we must find actual node that this match spec
  // covers. This requires covering prefixes to be proccessed last in restore.
  // Valid only for scale opt case.
  if (is_cp_restore) {
    while (!curr->entry) curr = curr->parent;
  }

  return curr;
}

pipe_status_t pipe_mgr_alpm_get_inactive_node_delete(bool *enable) {
  pipe_status_t rc = PIPE_SUCCESS;
  *enable = enable_free_inactive_nodes;
  return rc;
}

pipe_status_t pipe_mgr_alpm_set_inactive_node_delete(bool enable) {
  pipe_status_t rc = PIPE_SUCCESS;
  enable_free_inactive_nodes = enable;
  return rc;
}

/*
 * Usage: Delete Internal inactive node (pipe_tbl, node)
 * ------------------------------------------------
 * Traverses the trie of the given pipe table to find and delete nodes
 * that have the count as zero ie. entry deleted and without subtree.
 */
bool free_internal_inactive_node(trie_node_t *node_to_delete) {
  trie_node_t *curr = node_to_delete;
  trie_node_t *parent = NULL;
  bool pivot_with_child = false, is_node_deleted = false;
  if (!curr) {
    return is_node_deleted;
  }
  /* Node with even one child cannot be accommodated. */
  if (curr->left_child || curr->right_child) {
    pivot_with_child = true;
  }
  while (!curr->count && !pivot_with_child) {
    pivot_with_child = false;
    if (curr->parent) {
      /*
       * If the parent has both children then we do not traverse
       * the parent in the next iteration as the other subtrie can
       * have valid covering prefix/subtrie.
       */
      if (curr->parent->left_child && curr->parent->right_child) {
        pivot_with_child = true;
      }
      if (curr->parent->left_child == curr) {
        curr->parent->left_child = NULL;
      }
      if (curr->parent->right_child == curr) {
        curr->parent->right_child = NULL;
      }
      parent = curr->parent;
      PIPE_MGR_FREE(curr);
      is_node_deleted = true;
      curr = parent;
    } else {
      return is_node_deleted;
    }
  }
  return is_node_deleted;
}

static void free_inactive_node_if_not_txn(alpm_pipe_tbl_t *pipe_tbl,
                                          trie_node_t *node) {
  if (!ALPM_SESS_IS_TXN(pipe_tbl)) {
    if (free_internal_inactive_node(node)) {
      LOG_TRACE("%s: able to reduce trie footprint", __func__);
    }
  }
}

/*
 * Usage: pipe_mgr_alpm_entry_add_internal(...)
 * --------------------------------------------
 * Adds the given entry to the alpm table:
 *   -Adds the entry to the trie
 *   -Change the subtree structure if necessary
 *   -Updates the preclassifier table if necessary
 *   -Adds the entry to the correct SRAM partition
 */
pipe_status_t pipe_mgr_alpm_entry_add_internal(
    alpm_pipe_tbl_t *pipe_tbl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    pipe_mat_ent_hdl_t alpm_entry_hdl,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t msts = BF_MAP_OK;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_entry_t *entry = NULL;
  trie_node_t *node = NULL;
  trie_subtree_t *subtree = NULL;
  partition_info_t *p_info = NULL;
  trie_node_t *cp = NULL;
  alpm_entry_t *cp_entry = NULL;
  pipe_mgr_move_list_t *move_list = NULL;

  if (!pipe_tbl || !match_spec || !act_spec || !move_head_p || !move_tail_p) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  tbl_info = pipe_tbl->alpm_tbl_info;

  /* Finds the subtree and node corresponding to the given match spec */
  node = find_node(
      pipe_tbl, match_spec, false, &subtree, move_head_p, move_tail_p);
  if (node == NULL) {
    LOG_ERROR("%s:%d Invalid match spec for ALPM table 0x%x entry add",
              __func__,
              __LINE__,
              tbl_info->alpm_tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  if (!subtree) {
    return PIPE_UNEXPECTED;
  }
  if (node->entry) {
    return PIPE_ALREADY_EXISTS;
  }
  backup_node(pipe_tbl, node);

  /* Check to ensure all relevant subtrees can support a covering prefix */
  if (tbl_info->partition_depth >= 3 &&
      !check_covering_prefixes_add(pipe_tbl, node)) {
    LOG_ERROR("%s:%d Not enough space for covering prefixes in alpm table 0x%x",
              __func__,
              __LINE__,
              tbl_info->alpm_tbl_hdl);
    return PIPE_NO_SPACE;
  }

  if (!subtree->partition || !subtree->tcam_entry_hdl) {
    sts = setup_root(pipe_tbl, subtree, move_head_p, move_tail_p);
    if (sts != PIPE_SUCCESS) {
      // Indicates its a new subtree, which must be removed from covering
      // prefix subtree list.
      remove_subtree(pipe_tbl, subtree);
      LOG_ERROR(
          "%s:%d Failed to setup the root preclassifier for alpm table 0x%x",
          __func__,
          __LINE__,
          tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  p_info = subtree->partition;
  backup_ptn(pipe_tbl, p_info);
  match_spec->partition_index = p_info->ptn_index;

  /* Allocate and set our entry structure */
  entry = (alpm_entry_t *)PIPE_MGR_CALLOC(1, sizeof(alpm_entry_t));
  if (!entry) {
    LOG_ERROR("%s:%d %s(0x%x) Entry malloc failed",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl);
    free_inactive_node_if_not_txn(pipe_tbl, node);
    return PIPE_NO_SYS_RESOURCES;
  }
  entry->act_fn_hdl = act_fn_hdl;
  entry->ttl = ttl;
  entry->match_spec =
      pipe_mgr_tbl_copy_match_spec(entry->match_spec, match_spec);
  entry->act_spec = pipe_mgr_tbl_copy_action_spec(entry->act_spec, act_spec);

  if (!entry->match_spec || !entry->act_spec) {
    LOG_ERROR("%s:%d %s(0x%x) Entry match_spec or act_spec copy failed",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl);
    free_entry(entry);
    free_inactive_node_if_not_txn(pipe_tbl, node);
    return PIPE_UNEXPECTED;
  }

  /* If we are adding the root, replace the covering prefix if it exists */
  /* Note that we cannot guarantee the partition will not overflow otherwise */
  if (node->subtree) {
    PIPE_MGR_DBGCHK(node->subtree == subtree);
    if (subtree->cov_pfx && subtree->cov_pfx_entry) {
      entry->alpm_entry_hdl = alpm_entry_hdl;
      sts = replace_covering_prefix(
          pipe_tbl, subtree, entry, move_head_p, move_tail_p);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Failed to replace covering prefix in alpm tbl 0x%x",
                  __func__,
                  __LINE__,
                  tbl_info->alpm_tbl_hdl);
        free_entry(entry);
        free_inactive_node_if_not_txn(pipe_tbl, node);
        return sts;
      }
      goto done;
    }
  }

  /* Make space if our partition is already full */
  if (p_info->size >= get_partition_depth(tbl_info, p_info)) {
    if (tbl_info->partition_depth < 3) {
      /* Edge case for very small partitions */
      trie_subtree_t *new_subtree = NULL;
      trie_node_t *new_subtree_node = NULL;
      if (pipe_tbl->partitions_in_use == tbl_info->num_partitions) {
        free_entry(entry);
        free_inactive_node_if_not_txn(pipe_tbl, node);
        return PIPE_NO_SPACE;
      }

      p_info = create_partition_info(pipe_tbl, NULL);
      if (node->count == 2) {
        new_subtree_node = find_lower_node(node);
        if (!new_subtree_node) {
          free_entry(entry);
          free_inactive_node_if_not_txn(pipe_tbl, node);
          return PIPE_UNEXPECTED;
        }
      } else {
        node->entry = entry;
        new_subtree_node = node;
      }

      new_subtree = cut_trie(
          pipe_tbl, new_subtree_node, subtree, move_head_p, move_tail_p);
      node->entry = NULL;
      if (!new_subtree) {
        free_entry(entry);
        free_inactive_node_if_not_txn(pipe_tbl, node);
        return PIPE_UNEXPECTED;
      }

      sts =
          move_subtree(pipe_tbl, new_subtree, p_info, move_head_p, move_tail_p);
      if (sts != PIPE_SUCCESS) {
        free_entry(entry);
        node->entry = NULL;

        /*
         * If ALPM key width optimization is used and max subtrees per partition
         * is more than 1, free the subtree id.
         */
        if (tbl_info->atcam_subset_key_width &&
            tbl_info->max_subtrees_per_partition > 1) {
          bf_id_allocator_release(p_info->subtree_id_allocator,
                                  new_subtree->subtree_id);
        }

        remove_subtree(pipe_tbl, new_subtree);
        PIPE_MGR_FREE(new_subtree);
        new_subtree_node->subtree = NULL;
        free_inactive_node_if_not_txn(pipe_tbl, node);
        return sts;
      }
    } else {
      /* General case */
      sts = pipe_mgr_alpm_make_space(
          pipe_tbl, subtree, move_head_p, move_tail_p, false);
      if (sts != PIPE_SUCCESS) {
        free_entry(entry);
        free_inactive_node_if_not_txn(pipe_tbl, node);
        return sts;
      }
    }
  }

  /* Add the entry to the free partition */
  subtree = find_subtree(node);
  p_info = subtree->partition;
  backup_ptn(pipe_tbl, p_info);
  entry->match_spec->partition_index = p_info->ptn_index;
  entry->match_spec->version_bits = 0;
  sts = pipe_mgr_alpm_atcam_entry_place(
      tbl_info, pipe_tbl, entry, subtree, false, &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);
  if (sts != PIPE_SUCCESS) {
    free_entry(entry);
    free_inactive_node_if_not_txn(pipe_tbl, node);
    return sts;
  }
  p_info->size++;
  node->entry = entry;
  build_alpm_move_list_hdr(pipe_tbl,
                           PIPE_MAT_UPDATE_ADD,
                           tbl_info->atcam_tbl_hdl,
                           move_list,
                           entry->sram_entry_hdl,
                           alpm_entry_hdl,
                           false);
  move_list = NULL;

  /* If we have added the root of our new subtree, remove the temporary
   * covering prefix
   */
  if (subtree->node == node) {
    cp = subtree->cov_pfx;
    cp_entry = subtree->cov_pfx_entry;
    if (cp && cp_entry) {
      sts = pipe_mgr_tcam_entry_del(pipe_tbl->dev_tgt.device_id,
                                    tbl_info->atcam_tbl_hdl,
                                    cp_entry->sram_entry_hdl,
                                    pipe_tbl->sess_flags,
                                    &move_list);
      set_move_list(move_head_p, move_tail_p, move_list);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d %s(%d - 0x%x) Error removing covering prefix sts %d",
                  __func__,
                  __LINE__,
                  tbl_info->name,
                  pipe_tbl->dev_tgt.device_id,
                  tbl_info->alpm_tbl_hdl,
                  sts);
        return sts;
      }
      build_alpm_move_list_hdr(pipe_tbl,
                               PIPE_MAT_UPDATE_DEL,
                               tbl_info->atcam_tbl_hdl,
                               move_list,
                               cp_entry->sram_entry_hdl,
                               0,
                               true);
      move_list = NULL;
      remove_subtree_from_cp_list(pipe_tbl, cp, subtree);
      p_info->size--;
    }
  }

done:
  update_counts(pipe_tbl, node, true);
  entry->alpm_entry_hdl = alpm_entry_hdl;
  sts = update_covering_prefixes_add(pipe_tbl, node, move_head_p, move_tail_p);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error updating covering prefixes sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              pipe_tbl->dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    return sts;
  }
  msts =
      bf_map_add(&pipe_tbl->alpm_entry_hdl_map, alpm_entry_hdl, (void *)node);
  if (msts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding alpm entry to map sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              pipe_tbl->dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              msts);
    return PIPE_NO_SYS_RESOURCES;
  }
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_entry_place(...)
 * -------------------------------------
 * API to add an entry to the ALPM table.
 */
pipe_status_t pipe_mgr_alpm_entry_place(dev_target_t dev_tgt,
                                        pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                        pipe_tbl_match_spec_t *match_spec,
                                        pipe_act_fn_hdl_t act_fn_hdl,
                                        pipe_action_spec_t *act_spec,
                                        uint32_t ttl,
                                        uint32_t pipe_api_flags,
                                        pipe_mat_ent_hdl_t *ent_hdl_p,
                                        pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mgr_move_list_t *tail = *move_head_p;
  pipe_mat_ent_hdl_t alpm_entry_hdl;
  uint32_t i;

  /* Retrieves the alpm table structure corresponding to the given handle */
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((tbl_info->is_symmetric) && (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install an asymmetric entry"
        " in a symmetric table %d device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if ((!tbl_info->is_symmetric) && (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install a symmetric entry"
        " in an asymmetric table %d device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  pipe_tbl->dev_tgt = dev_tgt;
  pipe_tbl->sess_flags = pipe_api_flags;

  /* Fetch preclassifier info if necessary */
  if (!pipe_tbl->match_spec_template) {
    sts = pipe_mgr_set_alpm_tbl_match_act_info(
        tbl_info, pipe_tbl, match_spec, act_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error getting ALPM table 0x%x preclassifier info",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  alpm_entry_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);

  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    pipe_mat_ent_hdl_t ha_entry_hdl = -1;

    sts = pipe_mgr_hitless_ha_lookup_spec(&pipe_tbl->spec_map,
                                          match_spec,
                                          act_spec,
                                          act_fn_hdl,
                                          alpm_entry_hdl,
                                          &ha_entry_hdl,
                                          ttl);
    if (sts != PIPE_SUCCESS) {
      bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                              PIPE_GET_HDL_VAL(alpm_entry_hdl));
      return sts;
    }
    if (alpm_entry_hdl != ha_entry_hdl) {
      /* Free the entry-hdl */
      bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                              PIPE_GET_HDL_VAL(alpm_entry_hdl));
      alpm_entry_hdl = ha_entry_hdl;
    }

    trie_node_t *node = NULL;
    trie_subtree_t *subtree = NULL;
    bf_map_sts_t msts = bf_map_get(
        &pipe_tbl->alpm_entry_hdl_map, alpm_entry_hdl, (void **)&node);
    if (msts != BF_MAP_OK || !node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x) Error looking up alpm entry 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                alpm_entry_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (act_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE) {
      pipe_mgr_tcam_update_sel_hlp_state(dev_tgt,
                                         tbl_info->atcam_tbl_hdl,
                                         node->entry->sram_entry_hdl,
                                         act_spec->sel_grp_hdl);
      for (i = 0; i < node->cov_pfx_count; i++) {
        subtree = node->cov_pfx_subtree_nodes[i]->subtree;
        pipe_mgr_tcam_update_sel_hlp_state(
            dev_tgt,
            tbl_info->atcam_tbl_hdl,
            subtree->cov_pfx_entry->sram_entry_hdl,
            act_spec->sel_grp_hdl);
      }
    }
  } else {
    sts = pipe_mgr_alpm_entry_add_internal(pipe_tbl,
                                           match_spec,
                                           act_fn_hdl,
                                           act_spec,
                                           ttl,
                                           alpm_entry_hdl,
                                           move_head_p,
                                           &tail);
  }

  if (sts == PIPE_SUCCESS) {
    *ent_hdl_p = alpm_entry_hdl;
  } else {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding alpm entry sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              pipe_tbl->dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    clean_alpm_move_list_hdr(pipe_tbl);
    bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                            PIPE_GET_HDL_VAL(alpm_entry_hdl));
  }
  match_spec->partition_index = 0;
  return sts;
}

pipe_status_t pipe_mgr_alpm_entry_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mgr_move_list_t *tail = *move_head_p;

  /* Retrieves the alpm table structure corresponding to the given handle */
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((tbl_info->is_symmetric) && (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install an asymmetric entry"
        " in a symmetric table %d device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if ((!tbl_info->is_symmetric) && (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL)) {
    LOG_ERROR(
        "%s:%d Invalid request to install a symmetric entry"
        " in an asymmetric table %d device %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  pipe_tbl->dev_tgt = dev_tgt;
  pipe_tbl->sess_flags = pipe_api_flags;

  /* Fetch preclassifier info if necessary */
  if (!pipe_tbl->match_spec_template) {
    sts = pipe_mgr_set_alpm_tbl_match_act_info(
        tbl_info, pipe_tbl, match_spec, act_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error getting ALPM table 0x%x preclassifier info",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  bf_id_allocator_set(pipe_tbl->ent_hdl_allocator, PIPE_GET_HDL_VAL(ent_hdl));
  sts = pipe_mgr_alpm_entry_add_internal(pipe_tbl,
                                         match_spec,
                                         act_fn_hdl,
                                         act_spec,
                                         ttl,
                                         ent_hdl,
                                         move_head_p,
                                         &tail);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding alpm entry sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              pipe_tbl->dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    clean_alpm_move_list_hdr(pipe_tbl);
    bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                            PIPE_GET_HDL_VAL(ent_hdl));
  }

  match_spec->partition_index = 0;
  return sts;
}

/*
 * Usage: update_covering_prefixes_delete(pipe_tbl, deleted_node, head, tail)
 * --------------------------------------------------------------------------
 * Search for and perform the necessary covering prefix updates when an entry
 * is deleted. For all subtrees using this node as a covering prefix, replace
 * with this node's covering prefix instead. If this node has no covering
 * prefix, simply remove this node from all relevant subtrees.
 */
static pipe_status_t update_covering_prefixes_delete(
    alpm_pipe_tbl_t *pipe_tbl,
    trie_node_t *node,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = pipe_tbl->alpm_tbl_info;
  trie_node_t *cp;
  trie_subtree_t *subtree;
  alpm_entry_t *entry;
  pipe_mat_ent_hdl_t old_sram_hdl;
  pipe_mgr_move_list_t *move_list = NULL;
  int i;

  cp = find_covering_prefix(node);

  for (i = (int)node->cov_pfx_count - 1; i >= 0; i--) {
    subtree = node->cov_pfx_subtree_nodes[i]->subtree;
    backup_ptn(pipe_tbl, subtree->partition);
    old_sram_hdl = subtree->cov_pfx_entry->sram_entry_hdl;
    remove_subtree_from_cp_list(pipe_tbl, node, subtree);
    if (cp) {
      /* Add the new covering prefix if it exists */
      add_subtree_to_cp_list(pipe_tbl, cp, subtree);
      entry = subtree->cov_pfx_entry;
      if (tbl_info->atcam_subset_key_width) {
        entry->match_spec->version_bits = tbl_info->cp_ver_bits;
      }
      entry->match_spec->partition_index = subtree->partition->ptn_index;
      sts = pipe_mgr_alpm_atcam_entry_place(
          tbl_info, pipe_tbl, entry, subtree, true, &move_list);
      set_move_list(move_head_p, move_tail_p, move_list);
      if (sts != PIPE_SUCCESS) {
        return sts;
      }
      build_alpm_move_list_hdr(pipe_tbl,
                               PIPE_MAT_UPDATE_ADD,
                               tbl_info->atcam_tbl_hdl,
                               move_list,
                               entry->sram_entry_hdl,
                               cp->entry->alpm_entry_hdl,
                               true);
      move_list = NULL;
      subtree->partition->size++;
    }
    /* Delete this node regardless */
    sts = pipe_mgr_tcam_entry_del(pipe_tbl->dev_tgt.device_id,
                                  tbl_info->atcam_tbl_hdl,
                                  old_sram_hdl,
                                  pipe_tbl->sess_flags,
                                  &move_list);
    set_move_list(move_head_p, move_tail_p, move_list);
    if (sts != PIPE_SUCCESS) {
      return sts;
    }
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_DEL,
                             tbl_info->atcam_tbl_hdl,
                             move_list,
                             old_sram_hdl,
                             0,
                             true);
    move_list = NULL;
    subtree->partition->size--;
  }
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_entry_del(...)
 * -----------------------------------
 * Removes the entry from the alpm table:
 *   -Removes the entry from the underlying SRAM partition
 *   -Marks the trie node as invalid
 *   -Remove the subtree from the trie if empty
 *     -Remove the corresponding preclassifier entry
 *   -Free the partition info structure if empty
 */
pipe_status_t pipe_mgr_alpm_entry_del(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      uint32_t pipe_api_flags,
                                      pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t st = BF_MAP_OK;
  alpm_entry_t *entry;
  trie_node_t *node = NULL;
  trie_node_t *covering_prefix = NULL;
  alpm_entry_t *cp_entry;
  trie_subtree_t *subtree;
  partition_info_t *p_info;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  pipe_mgr_move_list_t *tail = *move_head_p;
  pipe_mgr_move_list_t *move_list = NULL;
  uint32_t pipe_idx;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  tbl_info->dev_id = dev_id;

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    pipe_tbl->dev_tgt.device_id = dev_id;
    pipe_tbl->sess_flags = pipe_api_flags;
    if (tbl_info->is_symmetric) {
      pipe_tbl->dev_tgt.dev_pipe_id = BF_DEV_PIPE_ALL;
    } else {
      pipe_tbl->dev_tgt.dev_pipe_id = pipe_tbl->pipe_id;
    }
    st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl, (void **)&node);
    if (st == BF_MAP_NO_KEY) {
      continue;
    }
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error looking up alpm entry"
          " 0x%x ",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    entry = node->entry;
    subtree = find_subtree(node);
    p_info = subtree->partition;
    backup_node(pipe_tbl, node);
    backup_ptn(pipe_tbl, p_info);
    if (subtree->node->count == 1) {
      /* Removing the last entry */
      sts = pipe_mgr_tcam_entry_del(dev_id,
                                    tbl_info->atcam_tbl_hdl,
                                    entry->sram_entry_hdl,
                                    pipe_api_flags,
                                    &move_list);
      set_move_list(move_head_p, &tail, move_list);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting alpm entry sts %d",
                  __func__,
                  __LINE__,
                  tbl_info->name,
                  dev_id,
                  tbl_info->alpm_tbl_hdl,
                  sts);
        clean_alpm_move_list_hdr(pipe_tbl);
        return sts;
      }
      build_alpm_move_list_hdr(pipe_tbl,
                               PIPE_MAT_UPDATE_DEL,
                               tbl_info->atcam_tbl_hdl,
                               move_list,
                               entry->sram_entry_hdl,
                               entry->alpm_entry_hdl,
                               false);
      move_list = NULL;
      p_info->size--;

      sts = remove_subtree_from_trie(pipe_tbl, subtree, move_head_p, &tail);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Failed to remove empty subtree in alpm tbl 0x%x",
                  __func__,
                  __LINE__,
                  tbl_info->alpm_tbl_hdl);
        clean_alpm_move_list_hdr(pipe_tbl);
        return sts;
      }
    } else {
      if (node->subtree) {
        if (tbl_info->partition_depth == 2 && node != pipe_tbl->root) {
          /* Move the subtree root to the lower node */
          trie_node_t *lower_node = find_lower_node(node);
          trie_subtree_t *new_subtree =
              create_subtree(pipe_tbl, lower_node, p_info);

          if (!lower_node || !new_subtree) {
            LOG_ERROR("%s:%d Either lower_node find or subtree_cretion failed",
                      __func__,
                      __LINE__);
            clean_alpm_move_list_hdr(pipe_tbl);
            return PIPE_OBJ_NOT_FOUND;
          }

          uint8_t pre_act_fn_hdl_index = 0;

          /*
           * If ALPM subset key width optimization is used, set the
           * correct preclassifier action handle and also assign the same
           * subtree id to new subtree.
           */
          if (tbl_info->atcam_subset_key_width) {
            new_subtree->subtree_id = node->subtree->subtree_id;
            pre_act_fn_hdl_index =
                get_pre_class_act_fn_hdl_index(tbl_info, subtree->node->depth);
          }

          pipe_act_fn_hdl_t pre_act_fn_hdl =
              tbl_info->act_fn_hdls[pre_act_fn_hdl_index];

          pipe_tbl_match_spec_t *subtree_match_spec =
              build_match_spec(tbl_info, pipe_tbl, lower_node);
          pipe_action_spec_t *subtree_act_spec = build_act_spec(
              tbl_info, pipe_tbl, p_info->ptn_index, new_subtree->subtree_id);

          sts = pipe_mgr_tcam_entry_place(pipe_tbl->dev_tgt,
                                          tbl_info->preclass_tbl_hdl,
                                          subtree_match_spec,
                                          pre_act_fn_hdl,
                                          subtree_act_spec,
                                          0,
                                          pipe_tbl->sess_flags,
                                          &new_subtree->tcam_entry_hdl,
                                          &move_list);
          free_match_spec(subtree_match_spec);
          free_action_spec(subtree_act_spec);
          set_move_list(move_head_p, &tail, move_list);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d %s(%d - 0x%x) Error adding alpm preclassifier entry sts "
                "%d",
                __func__,
                __LINE__,
                tbl_info->name,
                pipe_tbl->dev_tgt.device_id,
                tbl_info->alpm_tbl_hdl,
                sts);
            clean_alpm_move_list_hdr(pipe_tbl);
            return sts;
          }
          build_alpm_move_list_hdr(pipe_tbl,
                                   PIPE_MAT_UPDATE_ADD,
                                   tbl_info->preclass_tbl_hdl,
                                   move_list,
                                   0,
                                   0,
                                   false);
          move_list = NULL;
          lower_node->subtree = new_subtree;
          update_counts(pipe_tbl, lower_node->parent, false);

          sts = pipe_mgr_tcam_entry_del(dev_id,
                                        tbl_info->preclass_tbl_hdl,
                                        node->subtree->tcam_entry_hdl,
                                        pipe_api_flags,
                                        &move_list);
          set_move_list(move_head_p, &tail, move_list);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d %s(%d - 0x%x) Error deleting preclassifier entry sts %d",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->dev_id,
                tbl_info->alpm_tbl_hdl,
                sts);
            clean_alpm_move_list_hdr(pipe_tbl);
            return sts;
          }
          build_alpm_move_list_hdr(pipe_tbl,
                                   PIPE_MAT_UPDATE_DEL,
                                   tbl_info->preclass_tbl_hdl,
                                   move_list,
                                   0,
                                   0,
                                   false);
          move_list = NULL;
          remove_subtree(pipe_tbl, node->subtree);
          p_info->subtree_nodes[0] = lower_node;
          p_info->num_subtrees = 1;
        } else {
          /* Try to replace the subtree root with a covering prefix */
          covering_prefix = find_covering_prefix(node);
          if (covering_prefix) {
            add_subtree_to_cp_list(pipe_tbl, covering_prefix, subtree);
            cp_entry = subtree->cov_pfx_entry;
            if (tbl_info->atcam_subset_key_width) {
              entry->match_spec->version_bits = tbl_info->cp_ver_bits;
            }
            cp_entry->match_spec->partition_index =
                subtree->partition->ptn_index;
            sts = pipe_mgr_alpm_atcam_entry_place(
                tbl_info, pipe_tbl, cp_entry, subtree, true, &move_list);
            set_move_list(move_head_p, &tail, move_list);
            if (sts != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d %s(%d - 0x%x) Error adding covering prefix sts %d",
                  __func__,
                  __LINE__,
                  tbl_info->name,
                  dev_id,
                  tbl_info->alpm_tbl_hdl,
                  sts);
              clean_alpm_move_list_hdr(pipe_tbl);
              return sts;
            }
            build_alpm_move_list_hdr(pipe_tbl,
                                     PIPE_MAT_UPDATE_ADD,
                                     tbl_info->atcam_tbl_hdl,
                                     move_list,
                                     cp_entry->sram_entry_hdl,
                                     covering_prefix->entry->alpm_entry_hdl,
                                     true);
            move_list = NULL;
            p_info->size++;
          }
        }
      }
      sts = pipe_mgr_tcam_entry_del(dev_id,
                                    tbl_info->atcam_tbl_hdl,
                                    entry->sram_entry_hdl,
                                    pipe_api_flags,
                                    &move_list);
      set_move_list(move_head_p, &tail, move_list);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting alpm entry sts %d",
                  __func__,
                  __LINE__,
                  tbl_info->name,
                  tbl_info->dev_id,
                  tbl_info->alpm_tbl_hdl,
                  sts);
        clean_alpm_move_list_hdr(pipe_tbl);
        return sts;
      }
      build_alpm_move_list_hdr(pipe_tbl,
                               PIPE_MAT_UPDATE_DEL,
                               tbl_info->atcam_tbl_hdl,
                               move_list,
                               entry->sram_entry_hdl,
                               entry->alpm_entry_hdl,
                               false);
      move_list = NULL;
      p_info->size--;
    }

    sts = update_covering_prefixes_delete(pipe_tbl, node, move_head_p, &tail);
    if (sts != PIPE_SUCCESS) {
      clean_alpm_move_list_hdr(pipe_tbl);
      return sts;
    }
    st = bf_map_rmv(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl);
    if (st != BF_MAP_OK) {
      LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting alpm entry from map sts %d",
                __func__,
                __LINE__,
                tbl_info->name,
                dev_id,
                tbl_info->alpm_tbl_hdl,
                st);
      clean_alpm_move_list_hdr(pipe_tbl);
      return PIPE_UNEXPECTED;
    }

    free_entry(entry);
    node->entry = NULL;
    update_counts(pipe_tbl, node, false);
    if (subtree->node->count == 0) {
      subtree->node->subtree = NULL;
      PIPE_MGR_FREE(subtree);
    }
    if (sts == PIPE_SUCCESS) {
      /* Delete any cache for nodes submitted for entry delete.*/
      if (enable_free_inactive_nodes) {
        free_inactive_node_if_not_txn(pipe_tbl, node);
      }
    }
    bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                            PIPE_GET_HDL_VAL(mat_ent_hdl));
  }
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_default_ent_place(...)
 * -------------------------------------------
 * Adds the given default action to the alpm table.
 */
pipe_status_t pipe_mgr_alpm_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mgr_move_list_t *move_tail = *move_head_p;
  pipe_mgr_move_list_t *move_list = NULL;
  pipe_mat_ent_hdl_t alpm_ent_hdl = 0;
  pipe_mat_ent_hdl_t atcam_ent_hdl;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) && (!tbl_info->is_symmetric)) {
    LOG_ERROR(
        "%s:%d ALPM tbl 0x%x, Invalid pipe %d specified for asymmetric tbl, "
        "dev %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  if (!pipe_tbl->default_alpm_ent_hdl) {
    alpm_ent_hdl = PIPE_ALPM_DEFAULT_ENT_HDL;
    if (pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
      alpm_ent_hdl = PIPE_SET_HDL_PIPE(alpm_ent_hdl, pipe_tbl->pipe_id);
    }
  }

  /* Set the atcam default entry */
  sts = pipe_mgr_tcam_default_ent_place(dev_tgt,
                                        tbl_info->atcam_tbl_hdl,
                                        act_fn_hdl,
                                        act_spec,
                                        pipe_api_flags,
                                        &atcam_ent_hdl,
                                        &move_list);
  set_move_list(move_head_p, &move_tail, move_list);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error adding default alpm entry sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    clean_alpm_move_list_hdr(pipe_tbl);
    return sts;
  }
  if (!alpm_ent_hdl) {
    atcam_ent_hdl = 0;
  }
  build_alpm_move_list_hdr(pipe_tbl,
                           PIPE_MAT_UPDATE_SET_DFLT,
                           tbl_info->atcam_tbl_hdl,
                           move_list,
                           atcam_ent_hdl,
                           alpm_ent_hdl,
                           false);

  if ((pipe_api_flags & PIPE_MGR_TBL_API_TXN) &&
      !pipe_tbl->backup_default_ent_hdl_set) {
    pipe_tbl->backup_default_alpm_ent_hdl = pipe_tbl->default_alpm_ent_hdl;
    pipe_tbl->backup_default_atcam_ent_hdl = pipe_tbl->default_atcam_ent_hdl;
    pipe_tbl->backup_default_ent_hdl_set = true;
  }
  if (!pipe_tbl->default_alpm_ent_hdl) {
    pipe_tbl->default_alpm_ent_hdl = alpm_ent_hdl;
    pipe_tbl->default_atcam_ent_hdl = atcam_ent_hdl;
  }
  *ent_hdl_p = pipe_tbl->default_alpm_ent_hdl;

  return sts;
}

/*
 * Usage: pipe_mgr_alpm_default_ent_hdl_get(...)
 * -------------------------------------------
 * Get the default entry handle
 */
pipe_status_t pipe_mgr_alpm_default_ent_hdl_get(dev_target_t dev_tgt,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                pipe_mat_ent_hdl_t *ent_hdl_p) {
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_inst_log(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  *ent_hdl_p = 0;
  if (!pipe_tbl->default_alpm_ent_hdl) {
    return PIPE_OBJ_NOT_FOUND;
  }

  *ent_hdl_p = pipe_tbl->default_alpm_ent_hdl;

  return PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_alpm_cleanup_default_entry(...)
 * -----------------------------------------------
 * Removes the given default action from the alpm table.
 */
pipe_status_t pipe_mgr_alpm_cleanup_default_entry(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mgr_move_list_t *move_tail = *move_head_p;
  pipe_mgr_move_list_t *move_list = NULL;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  sts = pipe_mgr_tcam_cleanup_default_entry(
      dev_tgt, tbl_info->atcam_tbl_hdl, pipe_api_flags, &move_list);
  set_move_list(move_head_p, &move_tail, move_list);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d %s(%d - 0x%x) Error deleting default alpm entry sts %d",
              __func__,
              __LINE__,
              tbl_info->name,
              dev_tgt.device_id,
              tbl_info->alpm_tbl_hdl,
              sts);
    clean_alpm_move_list_hdr(pipe_tbl);
    return sts;
  }
  build_alpm_move_list_hdr(pipe_tbl,
                           PIPE_MAT_UPDATE_CLR_DFLT,
                           tbl_info->atcam_tbl_hdl,
                           move_list,
                           pipe_tbl->default_atcam_ent_hdl,
                           pipe_tbl->default_alpm_ent_hdl,
                           false);

  if ((pipe_api_flags & PIPE_MGR_TBL_API_TXN) &&
      !pipe_tbl->backup_default_ent_hdl_set) {
    pipe_tbl->backup_default_alpm_ent_hdl = pipe_tbl->default_alpm_ent_hdl;
    pipe_tbl->backup_default_atcam_ent_hdl = pipe_tbl->default_atcam_ent_hdl;
    pipe_tbl->backup_default_ent_hdl_set = true;
  }
  pipe_tbl->default_alpm_ent_hdl = 0;
  pipe_tbl->default_atcam_ent_hdl = 0;

  return sts;
}

/*
 * Usage: set_internal_action(...)
 * -------------------------------
 * Updates the action of the given entry to the given action spec and
 * function handle.
 */
static pipe_status_t set_internal_action(uint8_t device_id,
                                         alpm_pipe_tbl_t *pipe_tbl,
                                         alpm_entry_t *entry,
                                         pipe_action_spec_t *act_spec,
                                         pipe_act_fn_hdl_t act_fn_hdl,
                                         uint32_t pipe_api_flags,
                                         pipe_mgr_move_list_t **move_head_p,
                                         pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts;
  pipe_mgr_move_list_t *move_list = NULL;

  sts = pipe_mgr_tcam_ent_set_action(device_id,
                                     pipe_tbl->alpm_tbl_info->atcam_tbl_hdl,
                                     entry->sram_entry_hdl,
                                     act_fn_hdl,
                                     act_spec,
                                     pipe_api_flags,
                                     &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);

  if (sts == PIPE_SUCCESS) {
    free_action_spec(entry->act_spec);
    entry->act_spec = NULL;
    entry->act_spec = pipe_mgr_tbl_copy_action_spec(entry->act_spec, act_spec);
    entry->act_fn_hdl = act_fn_hdl;

    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_MOD,
                             pipe_tbl->alpm_tbl_info->atcam_tbl_hdl,
                             move_list,
                             0,
                             0,
                             false);
  }
  return sts;
}

static int get_resource_idx(alpm_entry_t *entry, pipe_res_hdl_t tbl_hdl) {
  int i;
  pipe_res_spec_t *res;
  for (i = 0; i < entry->act_spec->resource_count; i++) {
    res = &entry->act_spec->resources[i];
    if ((res->tag == PIPE_RES_ACTION_TAG_ATTACHED) &&
        (res->tbl_hdl == tbl_hdl)) {
      return i;
    }
  }
  return -1;
}

static void remove_resource_from_alpm_entry(alpm_entry_t *entry,
                                            pipe_res_hdl_t tbl_hdl) {
  int i;
  pipe_res_spec_t *res;
  bool found = false;
  for (i = 0; i < entry->act_spec->resource_count; i++) {
    res = &entry->act_spec->resources[i];
    if (!found) {
      if ((res->tag == PIPE_RES_ACTION_TAG_ATTACHED) &&
          (res->tbl_hdl == tbl_hdl)) {
        found = true;
      }
    } else {
      PIPE_MGR_MEMCPY(
          &entry->act_spec->resources[i - 1], res, sizeof(pipe_res_spec_t));
    }
  }
  if (found) {
    entry->act_spec->resources[i - 1].tag = PIPE_RES_ACTION_TAG_DETACHED;
    entry->act_spec->resource_count--;
  }
}

/*
 * Usage: update_resources(alpm_entry, resources, resource_count)
 * --------------------------------------------------------------
 * Updates the resource list of the given alpm entry metadata according to
 * the given resource list.
 */
static void update_resources(alpm_entry_t *entry,
                             pipe_res_spec_t *resources,
                             int resource_count) {
  pipe_res_spec_t *res, *entry_res = NULL;
  int i, res_idx;
  for (i = 0; i < resource_count; i++) {
    res = &resources[i];
    switch (res->tag) {
      case PIPE_RES_ACTION_TAG_ATTACHED: {
        res_idx = get_resource_idx(entry, res->tbl_hdl);
        if (res_idx == -1) {
          res_idx = entry->act_spec->resource_count;
          PIPE_MGR_DBGCHK(res_idx < PIPE_NUM_TBL_RESOURCES);
          entry->act_spec->resource_count++;
        }
        entry_res = &entry->act_spec->resources[res_idx];
        PIPE_MGR_MEMCPY(entry_res, res, sizeof(pipe_res_spec_t));
      } break;
      case PIPE_RES_ACTION_TAG_DETACHED: {
        remove_resource_from_alpm_entry(entry, res->tbl_hdl);
      } break;
      case PIPE_RES_ACTION_TAG_NO_CHANGE:
        break;
    }
  }
}

/*
 * Usage: set_internal_resources(...)
 * ----------------------------------
 * Updates the resource list of the given entry to the given resource list.
 */
static pipe_status_t set_internal_resources(
    uint8_t device_id,
    alpm_pipe_tbl_t *pipe_tbl,
    alpm_entry_t *entry,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p,
    pipe_mgr_move_list_t **move_tail_p) {
  pipe_status_t sts;
  pipe_mgr_move_list_t *move_list = NULL;

  sts = pipe_mgr_tcam_ent_set_resource(device_id,
                                       pipe_tbl->alpm_tbl_info->atcam_tbl_hdl,
                                       entry->sram_entry_hdl,
                                       resources,
                                       resource_count,
                                       pipe_api_flags,
                                       &move_list);
  set_move_list(move_head_p, move_tail_p, move_list);

  if (sts == PIPE_SUCCESS) {
    update_resources(entry, resources, resource_count);
    build_alpm_move_list_hdr(pipe_tbl,
                             PIPE_MAT_UPDATE_MOD,
                             pipe_tbl->alpm_tbl_info->atcam_tbl_hdl,
                             move_list,
                             0,
                             0,
                             false);
  }
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_ent_set_action_internal(...)
 * -------------------------------------------------
 * Modifies the alpm entry corresponding to the given handle with the
 * given action spec and/or resources.
 */
pipe_status_t pipe_mgr_alpm_ent_set_action_internal(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  bf_map_sts_t st = BF_MAP_OK;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node = NULL;
  alpm_entry_t *entry = NULL;
  pipe_mgr_move_list_t *move_tail = NULL;
  uint32_t pipe_id, i;

  if (!move_head_p) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }
  if (!act_spec && !resources) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  move_tail = *move_head_p;

  tbl_info = pipe_mgr_alpm_tbl_info_get(device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_id = 0; pipe_id < tbl_info->num_pipes; pipe_id++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_id];

    pipe_tbl->dev_tgt.device_id = device_id;
    pipe_tbl->sess_flags = pipe_api_flags;

    /* Set default entry action */
    if (mat_ent_hdl == pipe_tbl->default_alpm_ent_hdl) {
      if (act_spec) {
        sts = pipe_mgr_tcam_ent_set_action(device_id,
                                           tbl_info->atcam_tbl_hdl,
                                           pipe_tbl->default_atcam_ent_hdl,
                                           act_fn_hdl,
                                           act_spec,
                                           pipe_api_flags,
                                           move_head_p);
      } else {
        sts = pipe_mgr_tcam_ent_set_resource(device_id,
                                             tbl_info->atcam_tbl_hdl,
                                             pipe_tbl->default_atcam_ent_hdl,
                                             resources,
                                             resource_count,
                                             pipe_api_flags,
                                             move_head_p);
      }

      if (sts == PIPE_SUCCESS) {
        build_alpm_move_list_hdr(pipe_tbl,
                                 PIPE_MAT_UPDATE_MOD,
                                 pipe_tbl->alpm_tbl_info->atcam_tbl_hdl,
                                 *move_head_p,
                                 0,
                                 0,
                                 false);
      }
      return sts;
    }

    st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl, (void **)&node);
    if (st == BF_MAP_NO_KEY) {
      continue;
    }
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error looking up alpm entry"
          " 0x%x ",
          __func__,
          __LINE__,
          tbl_info->name,
          mat_tbl_hdl,
          mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
                __func__,
                __LINE__,
                tbl_info->name,
                mat_tbl_hdl,
                device_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    backup_node(pipe_tbl, node);
    entry = node->entry;
    if (!entry->sram_entry_hdl) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) Can't modify alpm entry %d with tcam entry hdl %d",
          __func__,
          __LINE__,
          tbl_info->name,
          mat_tbl_hdl,
          device_id,
          entry->alpm_entry_hdl,
          entry->sram_entry_hdl);
    } else {
      if (act_spec) {
        sts = set_internal_action(device_id,
                                  pipe_tbl,
                                  entry,
                                  act_spec,
                                  act_fn_hdl,
                                  pipe_api_flags,
                                  move_head_p,
                                  &move_tail);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s(0x%x - %d) Error updating the action for alpm entry "
              "0x%x pipe %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_info->name,
              mat_tbl_hdl,
              device_id,
              mat_ent_hdl,
              pipe_tbl->pipe_id,
              sts);
          clean_alpm_move_list_hdr(pipe_tbl);
          return sts;
        }
      }
      if (resources) {
        sts = set_internal_resources(device_id,
                                     pipe_tbl,
                                     entry,
                                     resources,
                                     resource_count,
                                     pipe_api_flags,
                                     move_head_p,
                                     &move_tail);
        if (sts != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d %s(0x%x - %d) Error updating the resources for alpm entry "
              "0x%x pipe %d rc 0x%x",
              __func__,
              __LINE__,
              tbl_info->name,
              mat_tbl_hdl,
              device_id,
              mat_ent_hdl,
              pipe_tbl->pipe_id,
              sts);
          clean_alpm_move_list_hdr(pipe_tbl);
          return sts;
        }
      }
    }

    for (i = 0; i < node->cov_pfx_count; i++) {
      if (!node->cov_pfx_subtree_nodes[i]->subtree) {
        continue;
      }
      backup_node(pipe_tbl, node->cov_pfx_subtree_nodes[i]);
      entry = node->cov_pfx_subtree_nodes[i]->subtree->cov_pfx_entry;
      if (entry == NULL || entry->match_spec == NULL) {
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      if (tbl_info->atcam_subset_key_width) {
        entry->match_spec->version_bits = tbl_info->cp_ver_bits;
      }
      if (!entry->sram_entry_hdl) {
        LOG_ERROR(
            "%s:%d %s(0x%x-%d) Can't modify alpm entry %d with cp tcam entry "
            "hdl %d",
            __func__,
            __LINE__,
            tbl_info->name,
            mat_tbl_hdl,
            device_id,
            node->entry->alpm_entry_hdl,
            entry->sram_entry_hdl);
      } else {
        if (act_spec) {
          sts = set_internal_action(device_id,
                                    pipe_tbl,
                                    entry,
                                    act_spec,
                                    act_fn_hdl,
                                    pipe_api_flags,
                                    move_head_p,
                                    &move_tail);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d %s(0x%x - %d) Error updating the action for alpm entry "
                "0x%x pipe %d rc 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                mat_tbl_hdl,
                device_id,
                mat_ent_hdl,
                pipe_tbl->pipe_id,
                sts);
            clean_alpm_move_list_hdr(pipe_tbl);
            return sts;
          }
        }
        if (resources) {
          sts = set_internal_resources(device_id,
                                       pipe_tbl,
                                       entry,
                                       resources,
                                       resource_count,
                                       pipe_api_flags,
                                       move_head_p,
                                       &move_tail);
          if (sts != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d %s(0x%x - %d) Error updating the resources for alpm "
                "entry "
                "0x%x pipe %d rc 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                mat_tbl_hdl,
                device_id,
                mat_ent_hdl,
                pipe_tbl->pipe_id,
                sts);
            clean_alpm_move_list_hdr(pipe_tbl);
            return sts;
          }
        }
      }
    }
  }
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_ent_set_action(...)
 * ----------------------------------------
 * Modifies the alpm entry corresponding to the given handle with the
 * given action spec.
 */
pipe_status_t pipe_mgr_alpm_ent_set_action(bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_act_fn_hdl_t act_fn_hdl,
                                           pipe_action_spec_t *act_spec,
                                           uint32_t pipe_api_flags,
                                           pipe_mgr_move_list_t **move_head_p) {
  return pipe_mgr_alpm_ent_set_action_internal(device_id,
                                               mat_tbl_hdl,
                                               mat_ent_hdl,
                                               act_fn_hdl,
                                               act_spec,
                                               NULL,
                                               0,
                                               pipe_api_flags,
                                               move_head_p);
}

/*
 * Usage: pipe_mgr_alpm_ent_set_resource(...)
 * ------------------------------------------
 * Modifies the alpm entry corresponding to the given handle with the
 * given resources.
 */
pipe_status_t pipe_mgr_alpm_ent_set_resource(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **move_head_p) {
  return pipe_mgr_alpm_ent_set_action_internal(device_id,
                                               mat_tbl_hdl,
                                               mat_ent_hdl,
                                               0,
                                               NULL,
                                               resources,
                                               resource_count,
                                               pipe_api_flags,
                                               move_head_p);
}

uint32_t get_pipe_tbl_usage(alpm_pipe_tbl_t *pipe_tbl) {
  partition_info_t *ptn;
  uint32_t count = 0;
  uint32_t pi, si;

  for (pi = 0; pi < pipe_tbl->alpm_tbl_info->num_partitions; pi++) {
    ptn = &pipe_tbl->partitions[pi];
    for (si = 0; si < ptn->num_subtrees; si++) {
      count += ptn->subtree_nodes[si]->count;
    }
  }

  return count;
}

/*
 * Usage: pipe_mgr_alpm_get_programmed_entry_count(dev_tgt, mat_tbl_hdl, &count)
 * -----------------------------------------------------------------------------
 * Returns the number of valid entries in the alpm table corresponding
 * to the given table handle.
 */
pipe_status_t pipe_mgr_alpm_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t tbl_hdl, uint32_t *count_p) {
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  uint32_t pipe_idx;
  uint32_t count = 0;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric alpm tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == pipe_tbl->pipe_id) {
      count += get_pipe_tbl_usage(pipe_tbl);
    }
  }

  *count_p = count;
  return PIPE_SUCCESS;
}

/*
 * Usage: is_entry_hdl_alpm_default(entry_hdl)
 * ------------------------------------------------------------------------
 * Return true if the given entry handle is the default entry for the alpm
 * table.
 */
static bool is_entry_hdl_alpm_default(pipe_mat_ent_hdl_t entry_hdl) {
  return PIPE_GET_HDL_VAL(entry_hdl) == PIPE_ALPM_DEFAULT_ENT_HDL;
}
/*
 * Usage: pipe_mgr_alpm_get_first_entry_handle(tbl_hdl, dev_tgt, &entry_hdl)
 * -------------------------------------------------------------------------
 * Retrieves the first entry handle for the given alpm table.
 */
pipe_status_t pipe_mgr_alpm_get_first_entry_handle(pipe_mat_tbl_hdl_t tbl_hdl,
                                                   dev_target_t dev_tgt,
                                                   int *entry_hdl) {
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  uint32_t pipe_idx;
  void *node = NULL;

  *entry_hdl = -1;
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tbl_info->is_symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric alpm tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        pipe_tbl->pipe_id != dev_tgt.dev_pipe_id) {
      continue;
    }
    *entry_hdl = bf_id_allocator_get_first(pipe_tbl->ent_hdl_allocator);
    if (*entry_hdl != -1) {
      if (pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
        *entry_hdl = PIPE_SET_HDL_PIPE(*entry_hdl, pipe_tbl->pipe_id);
      }
      if (is_entry_hdl_alpm_default(*entry_hdl) ||
          bf_map_get(&pipe_tbl->alpm_entry_hdl_map,
                     (uint32_t)*entry_hdl,
                     &node) == BF_MAP_NO_KEY) {
        int next_hdl = -1;
        pipe_status_t rc = pipe_mgr_alpm_get_next_entry_handles(
            tbl_hdl, dev_tgt, *entry_hdl, 1, &next_hdl);
        if (PIPE_SUCCESS != rc) {
          *entry_hdl = -1;
        } else {
          *entry_hdl = next_hdl;
        }
      }
      break;
    }
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_alpm_get_next_entry_handles(...)
 * ------------------------------------------------
 * Retrieves the next n entry handles for the given alpm table. If there are
 * less than n entry handles left, the end is marked with -1.
 */
pipe_status_t pipe_mgr_alpm_get_next_entry_handles(pipe_mat_tbl_hdl_t tbl_hdl,
                                                   dev_target_t dev_tgt,
                                                   pipe_mat_ent_hdl_t entry_hdl,
                                                   int n,
                                                   int *next_entry_handles) {
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mat_ent_hdl_t hdl = 0;
  bf_dev_pipe_t pipe_id;
  uint32_t pipe_idx = 0;
  int new_hdl = 0;
  int i = 0;
  void *node = NULL;

  if (n) {
    next_entry_handles[0] = -1;
  }
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (tbl_info->is_symmetric) {
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for symmetric alpm tbl with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    pipe_idx = 0;
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != pipe_id) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d for entry hdl %d passed for "
          "asymmetric alpm tbl with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
      if (tbl_info->pipe_tbls[pipe_idx]->pipe_id == pipe_id) {
        break;
      }
    }
    if (pipe_idx == tbl_info->num_pipes) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "alpm table for pipe %d not found",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  pipe_tbl = tbl_info->pipe_tbls[pipe_idx];

  hdl = PIPE_GET_HDL_VAL(entry_hdl);
  i = 0;
  while (i < n) {
    new_hdl = bf_id_allocator_get_next(pipe_tbl->ent_hdl_allocator, hdl);
    if (new_hdl == -1) {
      if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
        // We've reached the end of this pipe, so exit
        break;
      }
      while (pipe_idx < tbl_info->num_pipes - 1) {
        pipe_idx++;
        pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
        new_hdl = bf_id_allocator_get_first(pipe_tbl->ent_hdl_allocator);
        if (new_hdl != -1) {
          break;
        }
      }
      if (new_hdl == -1) {
        next_entry_handles[i] = new_hdl;
        break;
      }
    }
    if (pipe_tbl->pipe_id != BF_DEV_PIPE_ALL) {
      new_hdl = PIPE_SET_HDL_PIPE(new_hdl, pipe_tbl->pipe_id);
    }
    if (is_entry_hdl_alpm_default(new_hdl) ||
        bf_map_get(&pipe_tbl->alpm_entry_hdl_map, (uint32_t)new_hdl, &node) ==
            BF_MAP_NO_KEY) {
      hdl = PIPE_GET_HDL_VAL(new_hdl);
      continue;
    }

    next_entry_handles[i] = new_hdl;
    hdl = PIPE_GET_HDL_VAL(new_hdl);
    i++;
  }
  if (i < n) {
    next_entry_handles[i] = -1;
  }

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls) {
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  uint32_t pipe_id;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  *num_def_hdls = 0;

  for (pipe_id = 0; pipe_id < tbl_info->num_pipes; pipe_id++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_id];
    default_hdls[pipe_id] = pipe_tbl->default_alpm_ent_hdl;
    if (pipe_tbl->default_alpm_ent_hdl) {
      (*num_def_hdls)++;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_default_ent_get(pipe_sess_hdl_t sess_hdl,
                                            dev_target_t dev_tgt,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            pipe_action_spec_t *action_spec,
                                            pipe_act_fn_hdl_t *act_fn_hdl,
                                            bool from_hw) {
  alpm_tbl_info_t *tbl_info =
      pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  return pipe_mgr_tcam_default_ent_get(sess_hdl,
                                       dev_tgt,
                                       tbl_info->atcam_tbl_hdl,
                                       action_spec,
                                       act_fn_hdl,
                                       from_hw);
}

void build_alpm_full_mspec(alpm_tbl_info_t *tbl_info,
                           pipe_tbl_match_spec_t *entry_mspec,
                           pipe_tbl_match_spec_t *atcam_mspec,
                           uint32_t depth) {
  uint8_t entry_offset = depth - tbl_info->exm_fields_key_width - 1;
  uint32_t atcam_subset_key_bits = tbl_info->atcam_subset_key_width;
  uint32_t num_subtree_id_bytes = 0, num_subtree_id_bits = 0;

  /*
   * If max subtrees per partition
   * is more than 1, account for subtree id in match spec.
   */
  if (tbl_info->max_subtrees_per_partition > 1) {
    num_subtree_id_bits = log2(tbl_info->max_subtrees_per_partition);
    num_subtree_id_bytes = (num_subtree_id_bits + 7) / 8;
  }

  /* Copy the atcam_mspec subset to entry_mspec */
  uint8_t val = 0;
  uint8_t mask = 0;
  uint8_t *src = atcam_mspec->match_value_bits + num_subtree_id_bytes;
  uint8_t *dst = entry_mspec->match_value_bits +
                 tbl_info->lpm_field_byte_offset + entry_offset / 8;
  uint8_t *mask_src = atcam_mspec->match_mask_bits + num_subtree_id_bytes;
  uint8_t *mask_dst = entry_mspec->match_mask_bits +
                      tbl_info->lpm_field_byte_offset + entry_offset / 8;
  int prefix_len = atcam_subset_key_bits;
  int src_offset =
      (atcam_subset_key_bits % 8) ? (8 - atcam_subset_key_bits % 8) : 0;

  while (prefix_len > 0) {
    uint8_t num_bits = prefix_len >= 8 ? 8 : prefix_len;
    val = get_byte(src, src_offset, num_bits);
    mask = get_byte(mask_src, src_offset, num_bits);

    set_byte(dst, entry_offset % 8, val, num_bits);
    set_byte(mask_dst, entry_offset % 8, mask, num_bits);

    src++;
    dst++;
    mask_src++;
    mask_dst++;
    prefix_len -= 8;
  }
}

void build_alpm_exclude_msb_bits_full_mspec(
    alpm_tbl_info_t *tbl_info,
    pipe_tbl_match_spec_t *entry_mspec,
    pipe_tbl_match_spec_t *atcam_mspec) {
  uint8_t entry_offset =
      tbl_info->num_excluded_bits - tbl_info->exm_fields_key_width;
  uint32_t num_atcam_bits = tbl_info->trie_depth - tbl_info->num_excluded_bits;

  /* Copy the atcam_mspec subset to entry_mspec */
  uint8_t val = 0;
  uint8_t mask = 0;
  uint8_t *src = atcam_mspec->match_value_bits + entry_offset / 8;
  uint8_t *dst = entry_mspec->match_value_bits +
                 tbl_info->lpm_field_byte_offset + entry_offset / 8;
  uint8_t *mask_src = atcam_mspec->match_mask_bits + entry_offset / 8;
  uint8_t *mask_dst = entry_mspec->match_mask_bits +
                      tbl_info->lpm_field_byte_offset + entry_offset / 8;
  int prefix_len = num_atcam_bits;
  int src_offset = (num_atcam_bits % 8) ? (8 - num_atcam_bits % 8) : 0;

  while (prefix_len > 0) {
    uint8_t num_bits = prefix_len >= 8 ? 8 : prefix_len;
    val = get_byte(src, src_offset, num_bits);
    mask = get_byte(mask_src, src_offset, num_bits);

    set_byte(dst, entry_offset % 8, val, num_bits);
    set_byte(mask_dst, entry_offset % 8, mask, num_bits);

    src++;
    dst++;
    mask_src++;
    mask_dst++;
    prefix_len -= 8;
  }
}

/*
 * Usage: pipe_mgr_alpm_get_entry(...)
 * -----------------------------------
 * Retrieves the information corresponding to the given entry.
 */
pipe_status_t pipe_mgr_alpm_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                      dev_target_t dev_tgt,
                                      pipe_mat_ent_hdl_t entry_hdl,
                                      pipe_tbl_match_spec_t *pipe_match_spec,
                                      pipe_action_spec_t *pipe_action_spec,
                                      pipe_act_fn_hdl_t *act_fn_hdl,
                                      bool from_hw) {
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  bf_map_sts_t st = BF_MAP_OK;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  bf_dev_pipe_t pipe_id;
  trie_node_t *node;
  alpm_entry_t *entry;
  (void)from_hw;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_tgt.device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
  pipe_tbl = get_pipe_tbl_instance(tbl_info, pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  if (!tbl_info->is_symmetric) {
    /* Make sure the table has been locked for the proper pipe. For a symmetric
     * table, the table has been locked for all pipes. */
    sts = pipe_mgr_is_pipe_in_bmp(&pipe_tbl->pipe_bmp, dev_tgt.dev_pipe_id);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Entry with handle %d with pipe id %d does not match requested "
          "pipe id %d in alpm match table with handle %d, device_id %d",
          __func__,
          entry_hdl,
          PIPE_GET_HDL_PIPE(entry_hdl),
          dev_tgt.dev_pipe_id,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  }

  if (entry_hdl == pipe_tbl->default_alpm_ent_hdl) {
    sts = pipe_mgr_tcam_get_entry(tbl_info->atcam_tbl_hdl,
                                  dev_tgt,
                                  pipe_tbl->default_atcam_ent_hdl,
                                  pipe_match_spec,
                                  pipe_action_spec,
                                  act_fn_hdl);
    return sts;
  }

  st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, entry_hdl, (void **)&node);
  if (st == BF_MAP_NO_KEY) {
    LOG_TRACE("%s:%d %s(0x%x) Could not find alpm entry 0x%x",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl,
              entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error looking up alpm entry"
        " 0x%x ",
        __func__,
        __LINE__,
        tbl_info->name,
        tbl_info->alpm_tbl_hdl,
        entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (!node || !node->entry) {
    LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
              __func__,
              __LINE__,
              tbl_info->name,
              tbl_info->alpm_tbl_hdl,
              dev_tgt.device_id,
              entry_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  entry = node->entry;
  /*
   * If ATCAM subset key width optimization or
   * exclude MSB bits optimization is used, get both
   * the preclassifier entry and ATCAM entry to form
   * the full match spec that needs to be returned
   */
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    trie_subtree_t *subtree = NULL;
    subtree = find_subtree(node);
    if (subtree == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Subtree doesn't exist for entry 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_tgt.device_id,
                entry_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    pipe_action_spec_t *preclass_action_spec = NULL;
    preclass_action_spec = pipe_mgr_tbl_copy_action_spec(
        preclass_action_spec, pipe_tbl->act_spec_template);
    if (preclass_action_spec == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x - malloc failure",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_tgt.device_id,
                entry_hdl);
      return PIPE_NO_SYS_RESOURCES;
    }

    pipe_act_fn_hdl_t preclass_act_fn_hdl;
    sts = pipe_mgr_tcam_get_entry(tbl_info->preclass_tbl_hdl,
                                  dev_tgt,
                                  subtree->tcam_entry_hdl,
                                  pipe_match_spec,
                                  preclass_action_spec,
                                  &preclass_act_fn_hdl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) Failed to get subtree root entry 0x%x for ATCAM "
          "entry 0x%x",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          dev_tgt.device_id,
          subtree->tcam_entry_hdl,
          entry_hdl);
      free_action_spec(preclass_action_spec);
      return sts;
    }
    pipe_match_spec->partition_index = 0;

    pipe_tbl_match_spec_t *atcam_match_spec = NULL;
    atcam_match_spec = pipe_mgr_tbl_copy_match_spec(
        atcam_match_spec, pipe_tbl->match_spec_template);
    if (atcam_match_spec == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x - malloc failure",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_tgt.device_id,
                entry_hdl);
      free_action_spec(preclass_action_spec);
      return PIPE_NO_SYS_RESOURCES;
    }

    sts = pipe_mgr_tcam_get_entry(tbl_info->atcam_tbl_hdl,
                                  dev_tgt,
                                  entry->sram_entry_hdl,
                                  atcam_match_spec,
                                  pipe_action_spec,
                                  act_fn_hdl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Failed to get entry 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_tgt.device_id,
                entry_hdl);
      free_action_spec(preclass_action_spec);
      free_match_spec(atcam_match_spec);
      return sts;
    }

    /*
     * Build the full match spec from preclassifier match spec and
     * ATCAM subset key match spec. This step needs to be bypassed
     * if prefix length is 0 for ATCAM subset key width optimization as
     * ATCAM portion wouldn't have any valid bits.
     */
    uint32_t subtree_root_depth = subtree->node->depth;
    if (tbl_info->atcam_subset_key_width &&
        subtree_root_depth > tbl_info->exm_fields_key_width) {
      /* ATCAM subset key width optimization case and prefix length > 0 */
      if (subtree_root_depth >
          (tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1)) {
        subtree_root_depth =
            tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1;
      }
      build_alpm_full_mspec(
          tbl_info, pipe_match_spec, atcam_match_spec, subtree_root_depth);
    } else if (tbl_info->num_excluded_bits) {
      /* Exclude MSB bits optimization case */
      build_alpm_exclude_msb_bits_full_mspec(
          tbl_info, pipe_match_spec, atcam_match_spec);
    }
    pipe_match_spec->priority = atcam_match_spec->priority;

    /* Free the preclassifier action spec and ATCAM match spec */
    free_action_spec(preclass_action_spec);
    free_match_spec(atcam_match_spec);
  } else {
    sts = pipe_mgr_tcam_get_entry(tbl_info->atcam_tbl_hdl,
                                  dev_tgt,
                                  entry->sram_entry_hdl,
                                  pipe_match_spec,
                                  pipe_action_spec,
                                  act_fn_hdl);
    if (sts != PIPE_SUCCESS) return sts;
    pipe_match_spec->partition_index = 0;
  }

  return sts;
}

/*
 * Usage: pipe_mgr_alpm_entry_get_location(...)
 * --------------------------------------------
 * Retrives location information for the given entry.
 */
pipe_status_t pipe_mgr_alpm_entry_get_location(bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl,
                                               uint32_t subindex,
                                               bf_dev_pipe_t *pipe_id_p,
                                               uint8_t *stage_id_p,
                                               rmt_tbl_hdl_t *stage_table_hdl_p,
                                               uint32_t *index_p) {
  pipe_status_t sts = PIPE_OBJ_NOT_FOUND;
  bf_map_sts_t st = BF_MAP_OK;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  uint32_t pipe_id;
  trie_node_t *node;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_id = 0; pipe_id < tbl_info->num_pipes; pipe_id++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_id];

    st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl, (void **)&node);
    if (st == BF_MAP_NO_KEY) {
      if (mat_ent_hdl == pipe_tbl->default_alpm_ent_hdl) {
        sts = pipe_mgr_tcam_entry_get_programmed_location(
            dev_id,
            tbl_info->atcam_tbl_hdl,
            pipe_tbl->default_atcam_ent_hdl,
            subindex,
            pipe_id_p,
            stage_id_p,
            stage_table_hdl_p,
            index_p);
        break;
      } else {
        continue;
      }
    }
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error looking up alpm entry"
          " 0x%x ",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                dev_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    sts =
        pipe_mgr_tcam_entry_get_programmed_location(dev_id,
                                                    tbl_info->atcam_tbl_hdl,
                                                    node->entry->sram_entry_hdl,
                                                    subindex,
                                                    pipe_id_p,
                                                    stage_id_p,
                                                    stage_table_hdl_p,
                                                    index_p);
    break;
  }
  return sts;
}

pipe_status_t pipe_mgr_alpm_entry_hdl_from_stage_idx(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint8_t stage,
    uint8_t logical_tbl_id,
    uint32_t hit_addr,
    pipe_mat_ent_hdl_t *entry_hdl) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info;
  pipe_mat_ent_hdl_t x = 0;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  // Check if hit is from preclassifier
  sts = pipe_mgr_tcam_entry_hdl_from_stage_idx(dev_id,
                                               pipe,
                                               tbl_info->preclass_tbl_hdl,
                                               stage,
                                               logical_tbl_id,
                                               hit_addr,
                                               &x);
  if (sts != PIPE_SUCCESS) {
    // Check if hit is from atcam
    sts = pipe_mgr_tcam_entry_hdl_from_stage_idx(dev_id,
                                                 pipe,
                                                 tbl_info->atcam_tbl_hdl,
                                                 stage,
                                                 logical_tbl_id,
                                                 hit_addr,
                                                 &x);
    if (sts != PIPE_SUCCESS) {
      return sts;
    }

    // Convert lower-level atcam handle to application-level alpm handle
    sts = pipe_mgr_alpm_ent_hdl_atcam_to_alpm(
        dev_id, mat_tbl_hdl, pipe, &x, true);
    if (sts != PIPE_SUCCESS) {
      return sts;
    }
  }

  *entry_hdl = x;
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_tbl_set_symmetric_mode(...)
 * ------------------------------------------------
 * Sets the symmetric mode of the ALPM table according to the boolean flag.
 * This is only allowed when the table is empty.
 */
pipe_status_t pipe_mgr_alpm_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_bitmap_t pipe_bmp;
  bf_dev_pipe_t pipe_id = BF_INVALID_PIPE;
  uint32_t usage;
  uint32_t i;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("%s: Table %s, Change to symmetric mode %d ",
            __func__,
            tbl_info->name,
            symmetric);

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       tbl_info->is_symmetric,
                                       tbl_info->num_scopes,
                                       &tbl_info->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              tbl_info->name,
              tbl_info->is_symmetric,
              tbl_info->num_scopes);
    return rc;
  }

  rc = pipe_mgr_alpm_get_programmed_entry_count(dev_tgt, tbl_hdl, &usage);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to get entry count for alpm table 0x%x",
              __func__,
              __LINE__,
              tbl_hdl);
    return rc;
  }
  if (usage > 0) {
    LOG_TRACE(
        "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d ",
        __func__,
        tbl_info->name,
        tbl_info->is_symmetric,
        usage);
    return PIPE_NOT_SUPPORTED;
  }

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Match table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  // cleanup first
  for (i = 0; i < tbl_info->num_pipes; i++) {
    pipe_mgr_alpm_pipe_tbl_destroy(tbl_info->pipe_tbls[i]);
  }

  tbl_info->is_symmetric = symmetric;
  /* Copy the new scope info */
  tbl_info->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(tbl_info->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  tbl_info->num_pipes = tbl_info->num_scopes;
  tbl_info->pipe_tbls = PIPE_MGR_REALLOC(
      tbl_info->pipe_tbls, tbl_info->num_pipes * sizeof(alpm_pipe_tbl_t *));

  rc = pipe_mgr_get_pipe_bmp_for_profile(
      tbl_info->dev_info, tbl_info->profile_id, &pipe_bmp, __func__, __LINE__);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d - %s (%d - 0x%x) "
        "Error getting the pipe-bmp for profile-id %d "
        " rc 0x%x",
        __func__,
        __LINE__,
        tbl_info->name,
        tbl_info->dev_id,
        tbl_info->alpm_tbl_hdl,
        tbl_info->profile_id,
        rc);
    PIPE_MGR_DBGCHK(0);
    return rc;
  }

  if (tbl_info->is_symmetric) {
    tbl_info->pipe_tbls[0] =
        pipe_mgr_alpm_pipe_tbl_create(tbl_info,
                                      mat_tbl_info,
                                      PIPE_BITMAP_GET_FIRST_SET(&pipe_bmp),
                                      &pipe_bmp);
  } else {
    for (i = 0; i < tbl_info->num_pipes; i++) {
      pipe_bitmap_t local_pipe_bmp;

      PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
      pipe_id = pipe_mgr_get_lowest_pipe_in_scope(tbl_info->scope_pipe_bmp[i]);
      pipe_mgr_convert_scope_pipe_bmp(tbl_info->scope_pipe_bmp[i],
                                      &local_pipe_bmp);

      tbl_info->pipe_tbls[i] = pipe_mgr_alpm_pipe_tbl_create(
          tbl_info, mat_tbl_info, pipe_id, &local_pipe_bmp);
    }
  }

  rc = pipe_mgr_tbl_set_scope(sess_hdl,
                              dev_id,
                              tbl_info->preclass_tbl_hdl,
                              symmetric,
                              tbl_info->num_scopes,
                              &tbl_info->scope_pipe_bmp[0],
                              false);
  rc |= pipe_mgr_tbl_set_scope(sess_hdl,
                               dev_id,
                               tbl_info->atcam_tbl_hdl,
                               symmetric,
                               tbl_info->num_scopes,
                               &tbl_info->scope_pipe_bmp[0],
                               false);

  return rc;
}

/*
 * Usage: pipe_mgr_alpm_process_move_list(...)
 * -------------------------------------------
 * Process the hardware move list built by the hlp entry placement functions.
 */
pipe_status_t pipe_mgr_alpm_process_move_list(pipe_sess_hdl_t sess_hdl,
                                              bf_dev_id_t dev_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_mgr_move_list_t *move_list,
                                              uint32_t *success_count) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  pipe_mgr_move_list_t *head, *curr, *last;
  pipe_mgr_alpm_move_list_hdr_t *ml_hdr, *ml_hdr_next = NULL;
  hdl_info_t *hdl_info = NULL;
  pipe_mat_ent_hdl_t alpm_hdl;
  pipe_mat_ent_hdl_t atcam_hdl;
  bool is_preclass;
  uint32_t tbl_success_count;

  if (!move_list) {
    return PIPE_SUCCESS;
  }

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              mat_tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_inst_log(tbl_info, move_list->pipe, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  if (pipe_tbl->ml_hdr == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  ml_hdr = pipe_tbl->ml_hdr;
  while (ml_hdr) {
    ml_hdr_next = ml_hdr->next;
    head = ml_hdr->ml_head;
    if (!head) {
      ml_hdr = ml_hdr_next;
      continue;
    }
    curr = head;
    last = NULL;
    is_preclass = (ml_hdr->tbl_hdl == tbl_info->preclass_tbl_hdl);

    if (ml_hdr_next) {
      while (curr && (curr->next != ml_hdr_next->ml_head)) {
        curr = curr->next;
      }
      if (!curr) {
        ml_hdr->next = NULL;
      } else {
        last = curr;
        last->next = NULL;
      }
    }

    pipe_tbl = get_pipe_tbl_inst_log(tbl_info, head->pipe, __func__, __LINE__);
    if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

    /*
     * The atcam_entry_hdl_map is used by the idletime table to translate
     * atcam entry handles back to alpm entry handles. Since the idletime APIs
     * are called during move list processing in the atcam table, we must
     * ensure that all state exists at this time, which requires us to update
     * all adds before process_move_list is called and update all deletes after
     * process_move_list returns.
     */
    if (!is_preclass && (ml_hdr->op == PIPE_MAT_UPDATE_ADD ||
                         ml_hdr->op == PIPE_MAT_UPDATE_SET_DFLT)) {
      if (ml_hdr->alpm_ent_hdl) {
        PIPE_MGR_DBGCHK(ml_hdr->atcam_ent_hdl);

        alpm_hdl = ml_hdr->alpm_ent_hdl;
        atcam_hdl = ml_hdr->atcam_ent_hdl;
        hdl_info = PIPE_MGR_MALLOC(sizeof(hdl_info_t));
        hdl_info->alpm_hdl = alpm_hdl;
        hdl_info->cp_hdl = 0;

        if (ml_hdr->cov_pfx) {
          /* Covering prefix add, must allocate a dummy alpm handle to
           * ensure no unintentional collision with an actual alpm entry.
           */
          hdl_info->cp_hdl = pipe_mgr_alpm_allocate_handle(pipe_tbl);
        }

        bf_map_add(&pipe_tbl->atcam_entry_hdl_map, atcam_hdl, (void *)hdl_info);
      }
    }

    tbl_success_count = 0;
    sts = pipe_mgr_tcam_process_move_list(
        sess_hdl, dev_id, ml_hdr->tbl_hdl, head, &tbl_success_count);
    (*success_count) += tbl_success_count;
    if (sts != PIPE_SUCCESS) {
      clean_alpm_move_list_hdr(pipe_tbl);
      return sts;
    }

    if (!is_preclass && (ml_hdr->op == PIPE_MAT_UPDATE_DEL ||
                         ml_hdr->op == PIPE_MAT_UPDATE_CLR_DFLT)) {
      atcam_hdl = ml_hdr->atcam_ent_hdl;
      bf_map_get_rmv(
          &pipe_tbl->atcam_entry_hdl_map, atcam_hdl, (void **)&hdl_info);
      if (hdl_info->cp_hdl) {
        bf_id_allocator_release(pipe_tbl->ent_hdl_allocator,
                                PIPE_GET_HDL_VAL(hdl_info->cp_hdl));
      }
      PIPE_MGR_FREE(hdl_info);
    }

    if (last) {
      last->next = ml_hdr->next->ml_head;
    }
    ml_hdr = ml_hdr->next;
  }

  clean_alpm_move_list_hdr(pipe_tbl);
  pipe_tbl->ml_hdr = ml_hdr_next;
  return sts;
}

/*
 * Usage: reset_entry_handles(pipe_tbl)
 * -------------------------------------
 * Iterate through the backup list, releasing all new entry handles
 * on the first pass and re-setting the old entry handles on the second.
 */
static void reset_entry_handles(alpm_pipe_tbl_t *pipe_tbl) {
  backup_node_t *curr;

  /* Pass 1: Release all entry handles that have been changed */
  for (curr = pipe_tbl->backup_node_list; curr != NULL; curr = curr->next) {
    if (curr->node->entry && curr->node->entry->alpm_entry_hdl &&
        (!curr->backup_node->entry ||
         curr->node->entry->alpm_entry_hdl !=
             curr->backup_node->entry->alpm_entry_hdl)) {
      bf_id_allocator_release(
          pipe_tbl->ent_hdl_allocator,
          PIPE_GET_HDL_VAL(curr->node->entry->alpm_entry_hdl));
      if (bf_map_rmv(&pipe_tbl->alpm_entry_hdl_map,
                     curr->node->entry->alpm_entry_hdl) != BF_MAP_OK) {
        PIPE_MGR_DBGCHK(0);
      }
    }
  }

  /* Pass 2: Recover all original entry handles */
  for (curr = pipe_tbl->backup_node_list; curr != NULL; curr = curr->next) {
    if (curr->backup_node->entry && curr->backup_node->entry->alpm_entry_hdl &&
        (!curr->node->entry || curr->node->entry->alpm_entry_hdl !=
                                   curr->backup_node->entry->alpm_entry_hdl)) {
      bf_id_allocator_set(
          pipe_tbl->ent_hdl_allocator,
          PIPE_GET_HDL_VAL(curr->backup_node->entry->alpm_entry_hdl));
      if (bf_map_add(&pipe_tbl->alpm_entry_hdl_map,
                     curr->backup_node->entry->alpm_entry_hdl,
                     (void *)curr->node) != BF_MAP_OK) {
        PIPE_MGR_DBGCHK(0);
      }
    }
  }
}

/*
 * Usage: restore_node_backup_state(pipe_tbl)
 * ------------------------------------------
 * Restores the backup node states for the given pipe table.
 */
static void restore_node_backup_state(alpm_pipe_tbl_t *pipe_tbl) {
  backup_node_t *curr, *next;

  curr = pipe_tbl->backup_node_list;
  while (curr) {
    free_node_internal(curr->node);
    PIPE_MGR_MEMCPY(curr->node, curr->backup_node, sizeof(trie_node_t));
    curr->node->backed_up = false;

    next = curr->next;
    PIPE_MGR_FREE(curr->backup_node);
    PIPE_MGR_FREE(curr);
    curr = next;
  }
  pipe_tbl->backup_node_list = NULL;
}

/*
 * Usage: restore_ptn_backup_state(pipe_tbl)
 * -----------------------------------------
 * Restores the backup partition states for the given pipe table.
 */
static void restore_ptn_backup_state(alpm_pipe_tbl_t *pipe_tbl) {
  backup_ptn_t *curr, *next;
  partition_info_t *p_info;

  curr = pipe_tbl->backup_ptn_list;
  while (curr) {
    p_info = &pipe_tbl->partitions[curr->backup->ptn_index];
    if (p_info->subtree_nodes) {
      PIPE_MGR_FREE(p_info->subtree_nodes);
      p_info->subtree_nodes = NULL;
    }

    PIPE_MGR_MEMCPY(p_info, curr->backup, sizeof(partition_info_t));
    p_info->backed_up = false;

    next = curr->next;
    PIPE_MGR_FREE(curr->backup);
    PIPE_MGR_FREE(curr);
    curr = next;
  }
  pipe_tbl->backup_ptn_list = NULL;
}

/*
 * Usage: pipe_mgr_alpm_abort(device_id, alpm_tbl_hdl)
 * ---------------------------------------------------
 * Aborts the current transaction and restores the state from our backups.
 */
pipe_status_t pipe_mgr_alpm_abort(bf_dev_id_t dev_id,
                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                  bf_dev_pipe_t *pipes_list,
                                  unsigned nb_pipes) {
  pipe_status_t sts = PIPE_SUCCESS;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl;
  unsigned i, pipe;
  bool symmetric;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    pipe_tbl = get_pipe_tbl_by_pipe_id(tbl_info, pipe);
    if (!pipe_tbl) continue;

    reset_entry_handles(pipe_tbl);
    restore_node_backup_state(pipe_tbl);
    restore_ptn_backup_state(pipe_tbl);

    if (pipe_tbl->backup_partitions_in_use >= 0) {
      pipe_tbl->partitions_in_use = pipe_tbl->backup_partitions_in_use;
      pipe_tbl->ptn_in_use_list = pipe_tbl->backup_ptn_in_use_list;
      pipe_tbl->ptn_free_list = pipe_tbl->backup_ptn_free_list;

      pipe_tbl->backup_partitions_in_use = -1;
      pipe_tbl->backup_ptn_in_use_list = NULL;
      pipe_tbl->backup_ptn_free_list = NULL;
    }

    if (pipe_tbl->backup_default_ent_hdl_set) {
      pipe_tbl->default_alpm_ent_hdl = pipe_tbl->backup_default_alpm_ent_hdl;
      pipe_tbl->default_atcam_ent_hdl = pipe_tbl->backup_default_atcam_ent_hdl;
      pipe_tbl->backup_default_alpm_ent_hdl = 0;
      pipe_tbl->backup_default_atcam_ent_hdl = 0;
      pipe_tbl->backup_default_ent_hdl_set = false;
    }

    clean_alpm_move_list_hdr(pipe_tbl);
  }

  sts = pipe_mgr_tcam_abort(
      dev_id, tbl_info->preclass_tbl_hdl, pipes_list, nb_pipes);
  sts |= pipe_mgr_tcam_abort(
      dev_id, tbl_info->atcam_tbl_hdl, pipes_list, nb_pipes);

  return sts;
}

/*
 * Usage: discard_node_backup_state(pipe_tbl)
 * ------------------------------------------
 * Discards the backup node states for the given pipe table.
 */
static void discard_node_backup_state(alpm_pipe_tbl_t *pipe_tbl) {
  backup_node_t *curr, *next;

  curr = pipe_tbl->backup_node_list;
  while (curr) {
    curr->node->backed_up = false;

    next = curr->next;
    free_node(curr->backup_node);
    if (enable_free_inactive_nodes) {
      if (free_internal_inactive_node(curr->node)) {
        LOG_TRACE("%s: able to reduce trie footprint", __func__);
      }
    }
    PIPE_MGR_FREE(curr);
    curr = next;
  }
  pipe_tbl->backup_node_list = NULL;
}

/*
 * Usage: discard_ptn_backup_state(pipe_tbl)
 * -----------------------------------------
 * Discards the backup partition states for the given pipe table.
 */
static void discard_ptn_backup_state(alpm_pipe_tbl_t *pipe_tbl) {
  backup_ptn_t *curr, *next;

  curr = pipe_tbl->backup_ptn_list;
  while (curr) {
    pipe_tbl->partitions[curr->backup->ptn_index].backed_up = false;

    next = curr->next;
    if (curr->backup->subtree_nodes) {
      PIPE_MGR_FREE(curr->backup->subtree_nodes);
    }
    PIPE_MGR_FREE(curr->backup);
    PIPE_MGR_FREE(curr);
    curr = next;
  }
  pipe_tbl->backup_ptn_list = NULL;
}

/*
 * Usage: pipe_mgr_alpm_commit_int(tbl_info)
 * -----------------------------------------
 * Discards all backup states of the alpm table manager.
 */
pipe_status_t pipe_mgr_alpm_commit_int(alpm_tbl_info_t *tbl_info,
                                       bf_dev_pipe_t *pipes_list,
                                       unsigned nb_pipes) {
  alpm_pipe_tbl_t *pipe_tbl;
  unsigned i, pipe;
  bool symmetric;

  /* Process only table session's related pipes. */
  i = 0;
  symmetric = false;
  while (i < nb_pipes && !symmetric) {
    if (tbl_info->is_symmetric) {
      /* For symmetric table, we loop only once here. */
      symmetric = true;
      pipe = BF_DEV_PIPE_ALL;
    } else {
      pipe = pipes_list[i++];
    }

    pipe_tbl = get_pipe_tbl_by_pipe_id(tbl_info, pipe);
    if (!pipe_tbl) continue;

    discard_node_backup_state(pipe_tbl);
    discard_ptn_backup_state(pipe_tbl);

    pipe_tbl->backup_partitions_in_use = -1;
    pipe_tbl->backup_ptn_in_use_list = NULL;
    pipe_tbl->backup_ptn_free_list = NULL;

    /* Discard default entry backup */
    pipe_tbl->backup_default_alpm_ent_hdl = 0;
    pipe_tbl->backup_default_atcam_ent_hdl = 0;
    pipe_tbl->backup_default_ent_hdl_set = false;
  }

  return PIPE_SUCCESS;
}

/*
 * Usage: pipe_mgr_alpm_commit(device_id, alpm_tbl_hdl)
 * ----------------------------------------------------
 * Locks in the current transaction and discards all backup states.
 */
pipe_status_t pipe_mgr_alpm_commit(bf_dev_id_t dev_id,
                                   pipe_mat_tbl_hdl_t tbl_hdl,
                                   bf_dev_pipe_t *pipes_list,
                                   unsigned nb_pipes) {
  pipe_status_t sts;
  alpm_tbl_info_t *tbl_info;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sts = pipe_mgr_alpm_commit_int(tbl_info, pipes_list, nb_pipes);
  sts |= pipe_mgr_tcam_commit(
      dev_id, tbl_info->preclass_tbl_hdl, pipes_list, nb_pipes);
  sts |= pipe_mgr_tcam_commit(
      dev_id, tbl_info->atcam_tbl_hdl, pipes_list, nb_pipes);

  return sts;
}

/*
 * Usage: pipe_mgr_alpm_atom_cleanup(device_id, alpm_tbl_hdl)
 * ----------------------------------------------------------
 * Locks in the current atomic transaction and discards all backup states.
 * The actual atomic actions will be performed by the corresponding tcam
 * functions.
 */
pipe_status_t pipe_mgr_alpm_atom_cleanup(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl,
                                         bf_dev_pipe_t *pipes_list,
                                         unsigned nb_pipes) {
  pipe_status_t sts;
  alpm_tbl_info_t *tbl_info;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  sts = pipe_mgr_alpm_commit_int(tbl_info, pipes_list, nb_pipes);
  sts |= pipe_mgr_tcam_atom_cleanup(
      dev_id, tbl_info->preclass_tbl_hdl, pipes_list, nb_pipes);
  sts |= pipe_mgr_tcam_atom_cleanup(
      dev_id, tbl_info->atcam_tbl_hdl, pipes_list, nb_pipes);
  return sts;
}

/*
 * Usage: pipe_mgr_alpm_print_match_spec(...)
 * ------------------------------------------
 * Prints the given match spec assigned to this alpm table to the given
 * buffer. Returns a string of length 0 if the table is not found.
 */
void pipe_mgr_alpm_print_match_spec(bf_dev_id_t device_id,
                                    pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                    pipe_tbl_match_spec_t *match_spec,
                                    char *buf,
                                    size_t buf_len) {
  alpm_tbl_info_t *tbl_info = NULL;
  size_t bytes_written = 0;

  tbl_info = pipe_mgr_alpm_tbl_info_get(device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    buf[0] = '\0';
    return;
  }

  pipe_mgr_entry_format_print_match_spec(device_id,
                                         tbl_info->profile_id,
                                         tbl_info->atcam_tbl_hdl,
                                         match_spec,
                                         buf,
                                         buf_len,
                                         &bytes_written);

  return;
}

pipe_status_t pipe_mgr_alpm_get_full_match_spec(
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    bf_dev_id_t device_id,
    bf_dev_pipe_t *pipe_id,
    pipe_tbl_match_spec_t **match_spec) {
  bf_map_sts_t st;
  bf_status_t sts;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node = NULL;
  uint32_t pipe_idx;

  tbl_info = pipe_mgr_alpm_tbl_info_get(device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle %d, device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /*
   * This API should only be called if the ALPM table uses
   * ATCAM subset key width optimization or
   * exclude MSB bits optimization
   */
  if (!ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    LOG_ERROR(
        "%s : %d ALPM match table table with handle %d "
        "doesn't use ATCAM subset key width optimization or "
        "exclude MSB bits optimization, device id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl, (void **)&node);
    if (st == BF_MAP_NO_KEY) {
      continue;
    }
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error looking up alpm entry"
          " 0x%x ",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                device_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* First get the match spec for subtree root */
    pipe_tbl_match_spec_t *preclass_match_spec = NULL;
    trie_subtree_t *subtree = NULL;
    subtree = find_subtree(node);
    if (subtree == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Subtree doesn't exist for entry 0x%x",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                device_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    sts = pipe_mgr_tcam_get_match_spec(subtree->tcam_entry_hdl,
                                       tbl_info->preclass_tbl_hdl,
                                       device_id,
                                       pipe_id,
                                       &preclass_match_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) Failed to get match spec of subtree root entry "
          "0x%x for ATCAM entry 0x%x",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          device_id,
          subtree->tcam_entry_hdl,
          mat_ent_hdl);
      return sts;
    }

    /* Next, get the match spec for ATCAM entry */
    pipe_tbl_match_spec_t *atcam_match_spec = NULL;
    sts = pipe_mgr_tcam_get_match_spec(node->entry->sram_entry_hdl,
                                       tbl_info->atcam_tbl_hdl,
                                       device_id,
                                       pipe_id,
                                       &atcam_match_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) Failed to get match spec of ATCAM entry 0x%x",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          device_id,
          mat_ent_hdl);
      return sts;
    }

    /*
     * Copy the preclassifier match spec
     * If memory is allocated in this step, it's caller's responsibility to FREE
     * the memory
     */
    *match_spec =
        pipe_mgr_tbl_copy_match_spec(*match_spec, preclass_match_spec);
    if (*match_spec == NULL) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Malloc failure",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                device_id);
      return PIPE_NO_SYS_RESOURCES;
    }

    /*
     * Build the full match spec from preclassifier match spec and
     * ATCAM subset key match spec. This step needs to be bypassed
     * if prefix length is 0 for ATCAM subset key width optimization as
     * ATCAM portion wouldn't have any valid bits.
     */
    uint32_t subtree_root_depth = subtree->node->depth;
    if (tbl_info->atcam_subset_key_width &&
        subtree_root_depth > tbl_info->exm_fields_key_width) {
      /* ATCAM subset key width optimization case and prefix length > 0 */
      if (subtree_root_depth >
          (tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1)) {
        subtree_root_depth =
            tbl_info->trie_depth - tbl_info->atcam_subset_key_width + 1;
      }
      build_alpm_full_mspec(
          tbl_info, *match_spec, atcam_match_spec, subtree_root_depth);
    } else if (tbl_info->num_excluded_bits) {
      /* Exclude MSB bits optimization case */
      build_alpm_exclude_msb_bits_full_mspec(
          tbl_info, *match_spec, atcam_match_spec);
    }
    (*match_spec)->partition_index = 0;
    (*match_spec)->priority = atcam_match_spec->priority;

    return PIPE_SUCCESS;
  }

  LOG_ERROR("%s:%d Entry data for %d in ALPM tbl %d not found",
            __func__,
            __LINE__,
            mat_ent_hdl,
            mat_tbl_hdl);
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_alpm_get_match_spec(pipe_mat_ent_hdl_t mat_ent_hdl,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           bf_dev_id_t device_id,
                                           bf_dev_pipe_t *pipe_id,
                                           pipe_tbl_match_spec_t **match_spec) {
  bf_map_sts_t st;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node = NULL;
  uint32_t pipe_idx;

  tbl_info = pipe_mgr_alpm_tbl_info_get(device_id, mat_tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle %d, device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    st = bf_map_get(&pipe_tbl->alpm_entry_hdl_map, mat_ent_hdl, (void **)&node);
    if (st == BF_MAP_NO_KEY) {
      continue;
    }
    if (st != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s(0x%x) Error looking up alpm entry"
          " 0x%x ",
          __func__,
          __LINE__,
          tbl_info->name,
          tbl_info->alpm_tbl_hdl,
          mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    if (!node || !node->entry) {
      LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x does not exist",
                __func__,
                __LINE__,
                tbl_info->name,
                tbl_info->alpm_tbl_hdl,
                device_id,
                mat_ent_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }

    /*
     * If ATCAM subset key width optimization or
     * exclude MSB bits optimization is used,
     * ATCAM table wouldn't have the full match spec. So, instead
     * return the local match spec copy.
     * Note: This function shouldn't be called while processing entry
     * delete as the local copy would be deleted during entry delete.
     * For entry delete, pipe_mgr_alpm_get_full_match_spec()
     * should be called if ALPM subset key width optimization or
     * exclude MSB bits optimization is used.
     */
    if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
      if (node->entry->match_spec != NULL) {
        *match_spec = node->entry->match_spec;
        return PIPE_SUCCESS;
      } else {
        LOG_ERROR("%s:%d %s(0x%x-%d) Entry 0x%x match spec does not exist",
                  __func__,
                  __LINE__,
                  tbl_info->name,
                  tbl_info->alpm_tbl_hdl,
                  device_id,
                  mat_ent_hdl);
        return PIPE_OBJ_NOT_FOUND;
      }
    } else {
      return pipe_mgr_tcam_get_match_spec(node->entry->sram_entry_hdl,
                                          tbl_info->atcam_tbl_hdl,
                                          device_id,
                                          pipe_id,
                                          match_spec);
    }
  }

  LOG_ERROR("%s:%d Entry data for ALPM tbl %d not found",
            __func__,
            __LINE__,
            mat_tbl_hdl);
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_alpm_ent_hdl_atcam_to_alpm(bf_dev_id_t device_id,
                                                  pipe_mat_tbl_hdl_t tbl_hdl,
                                                  bf_dev_pipe_t pipe_id,
                                                  pipe_mat_ent_hdl_t *ent_hdl,
                                                  bool from_user) {
  bf_map_sts_t st;
  alpm_tbl_info_t *tbl_info = NULL;
  hdl_info_t *hdl_info = NULL;
  alpm_pipe_tbl_t *pipe_tbl;

  tbl_info = pipe_mgr_alpm_tbl_info_get(device_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl = get_pipe_tbl_inst_log(tbl_info, pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  st = bf_map_get(&pipe_tbl->atcam_entry_hdl_map, *ent_hdl, (void **)&hdl_info);
  if (st != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d %s(0x%x) Error looking up alpm entry"
        " with atcam handle %d",
        __func__,
        __LINE__,
        tbl_info->name,
        tbl_info->alpm_tbl_hdl,
        *ent_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (hdl_info->cp_hdl && !from_user) {
    *ent_hdl = hdl_info->cp_hdl;
  } else {
    *ent_hdl = hdl_info->alpm_hdl;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_log_state(bf_dev_id_t dev_id,
                                      pipe_mat_tbl_info_t *mat_info,
                                      cJSON *match_tbls) {
  pipe_status_t sts;
  bf_map_sts_t st;
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *alpm_pipe_tbl = NULL;
  trie_node_t *node = NULL;
  trie_subtree_t *subtree = NULL;
  alpm_entry_t *entry_info = NULL;
  hdl_info_t *hdl_info = NULL;
  pipe_mat_tbl_hdl_t tbl_hdl;
  unsigned long entry_hdl;
  uint32_t pipe_idx;
  uint32_t cp_idx;
  cJSON *match_tbl, *pipe_tbls, *pipe_tbl, *mat_ents, *mat_ent;
  cJSON *cps, *cp;

  tbl_hdl = mat_info->handle;
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  cJSON_AddItemToArray(match_tbls, match_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(match_tbl, "name", tbl_info->name);
  cJSON_AddNumberToObject(match_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(match_tbl, "symmetric", tbl_info->is_symmetric);
  cJSON_AddBoolToObject(
      match_tbl, "duplicate_entry_check", mat_info->duplicate_entry_check);
  cJSON_AddItemToObject(
      match_tbl, "pipe_tbls", pipe_tbls = cJSON_CreateArray());

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    alpm_pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    cJSON_AddItemToArray(pipe_tbls, pipe_tbl = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe_tbl, "pipe_id", alpm_pipe_tbl->pipe_id);
    if (alpm_pipe_tbl->default_alpm_ent_hdl) {
      cJSON_AddNumberToObject(
          pipe_tbl, "default_ent_hdl", alpm_pipe_tbl->default_alpm_ent_hdl);
      cJSON_AddNumberToObject(pipe_tbl,
                              "default_atcam_ent_hdl",
                              alpm_pipe_tbl->default_atcam_ent_hdl);
    }

    cJSON_AddItemToObject(
        pipe_tbl, "match_entries", mat_ents = cJSON_CreateArray());
    st = bf_map_get_first(
        &alpm_pipe_tbl->alpm_entry_hdl_map, &entry_hdl, (void **)&node);
    while (st == BF_MAP_OK) {
      entry_info = node->entry;
      if (entry_hdl != alpm_pipe_tbl->default_alpm_ent_hdl) {
        cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
        cJSON_AddNumberToObject(mat_ent, "entry_hdl", entry_hdl);
        cJSON_AddNumberToObject(
            mat_ent, "atcam_hdl", entry_info->sram_entry_hdl);
        cJSON_AddNumberToObject(mat_ent, "ttl", entry_info->ttl);

        cJSON_AddItemToObject(
            mat_ent, "covering_prefixes", cps = cJSON_CreateArray());
        for (cp_idx = 0; cp_idx < node->cov_pfx_count; cp_idx++) {
          subtree = node->cov_pfx_subtree_nodes[cp_idx]->subtree;
          st = bf_map_get(&alpm_pipe_tbl->atcam_entry_hdl_map,
                          subtree->cov_pfx_entry->sram_entry_hdl,
                          (void **)&hdl_info);
          if (st == BF_MAP_OK) {
            PIPE_MGR_DBGCHK(hdl_info->alpm_hdl == entry_hdl);
            cJSON_AddItemToArray(cps, cp = cJSON_CreateObject());
            cJSON_AddNumberToObject(cp, "cp_hdl", hdl_info->cp_hdl);
            cJSON_AddNumberToObject(
                cp, "atcam_hdl", subtree->cov_pfx_entry->sram_entry_hdl);
          }
        }
      }
      st = bf_map_get_next(
          &alpm_pipe_tbl->alpm_entry_hdl_map, &entry_hdl, (void **)&node);
    }
  }

  sts = pipe_mgr_tcam_log_state(
      dev_id, NULL, tbl_info->preclass_tbl_hdl, match_tbls);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Preclassifier table 0x%x for alpm table 0x%x log failed",
              __func__,
              __LINE__,
              tbl_info->preclass_tbl_hdl,
              tbl_hdl);
    return sts;
  }
  sts = pipe_mgr_tcam_log_state(
      dev_id, NULL, tbl_info->atcam_tbl_hdl, match_tbls);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Atcam table 0x%x for alpm table 0x%x log failed",
              __func__,
              __LINE__,
              tbl_info->atcam_tbl_hdl,
              tbl_hdl);
    return sts;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  alpm_tbl_info_t *tbl_info = NULL;

  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR("%s:%d ALPM table with handle 0x%x not found",
              __func__,
              __LINE__,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("%s: Table %s, symmetric mode get ", __func__, tbl_info->name);

  *symmetric = tbl_info->is_symmetric;
  *num_scopes = tbl_info->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  tbl_info->scope_pipe_bmp,
                  tbl_info->num_scopes * sizeof(scope_pipes_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_preclass_restore_int(
    alpm_tbl_info_t *tbl_info,
    alpm_pipe_tbl_t *pipe_tbl,
    pipe_mat_ent_hdl_t ll_ent_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec) {
  pipe_status_t sts = PIPE_SUCCESS;
  trie_node_t *node;
  trie_subtree_t *subtree;
  partition_info_t *p_info = NULL;
  uint32_t ptn_idx;

  if (!pipe_tbl->match_spec_template) {
    sts = pipe_mgr_set_alpm_tbl_match_act_info(
        tbl_info, pipe_tbl, match_spec, act_spec);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Error setting ALPM table 0x%x preclassifier info",
                __func__,
                __LINE__,
                tbl_info->alpm_tbl_hdl);
      return sts;
    }
  }

  uint8_t subtree_id = 0;
  ptn_idx = read_action_spec_ptn_idx(tbl_info, act_spec, &subtree_id);
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    p_info = &pipe_tbl->partitions[ptn_idx - 1];
  } else {
    p_info = &pipe_tbl->partitions[ptn_idx];
  }
  if (!p_info) {
    LOG_ERROR(
        "%s:%d Error getting ALPM table partition info", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(p_info->ptn_index == ptn_idx);
  node = find_node(pipe_tbl, match_spec, false, NULL, NULL, NULL);
  if (!node) {
    LOG_ERROR("%s:%d Error getting ALPM table node", __func__, __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }
  subtree = create_subtree(pipe_tbl, node, p_info);
  subtree->tcam_entry_hdl = ll_ent_hdl;
  subtree->subtree_id = subtree_id;
  if (tbl_info->atcam_subset_key_width &&
      tbl_info->max_subtrees_per_partition > 1) {
    bf_id_allocator_set(p_info->subtree_id_allocator, subtree_id);
  }
  p_info->subtree_nodes[p_info->num_subtrees] = node;
  p_info->num_subtrees++;

  // Restore the priority of the match spec
  match_spec->priority = tbl_info->trie_depth - node->depth;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_preclass_restore(dev_target_t dev_tgt,
                                             pipe_mat_tbl_info_t *mat_info,
                                             pipe_mat_tbl_hdl_t ll_tbl_hdl,
                                             pipe_mat_ent_hdl_t ll_ent_hdl,
                                             pipe_tbl_match_spec_t *match_spec,
                                             pipe_act_fn_hdl_t act_fn_hdl,
                                             pipe_action_spec_t *act_spec) {
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;

  (void)mat_info;
  (void)act_fn_hdl;

  tbl_info =
      pipe_mgr_alpm_tbl_info_get_from_ll_hdl(dev_tgt.device_id, ll_tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with preclass handle 0x%x, device id %d",
        __func__,
        ll_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  return pipe_mgr_alpm_preclass_restore_int(
      tbl_info, pipe_tbl, ll_ent_hdl, match_spec, act_spec);
}

pipe_status_t pipe_mgr_alpm_atcam_restore_int(alpm_tbl_info_t *tbl_info,
                                              alpm_pipe_tbl_t *pipe_tbl,
                                              pipe_mat_ent_hdl_t ll_ent_hdl,
                                              trie_node_t *node,
                                              partition_info_t *p_info,
                                              uint8_t subtree_id,
                                              pipe_tbl_match_spec_t *match_spec,
                                              pipe_act_fn_hdl_t act_fn_hdl,
                                              pipe_action_spec_t *act_spec,
                                              bool is_cp) {
  pipe_status_t sts = PIPE_SUCCESS;
  cp_restore_t *cp_restore;

  if (is_cp) {
    if (node->cov_pfx_restore_count == node->cov_pfx_arr_size) {
      node->cov_pfx_arr_size++;
      node->cov_pfx_subtree_nodes =
          PIPE_MGR_REALLOC(node->cov_pfx_subtree_nodes,
                           node->cov_pfx_arr_size * sizeof(trie_node_t *));
    }
    cp_restore = PIPE_MGR_MALLOC(sizeof(cp_restore_t));
    cp_restore->ptn_idx = match_spec->partition_index;
    cp_restore->ent_hdl = ll_ent_hdl;
    cp_restore->subtree_id = subtree_id;
    node->cov_pfx_subtree_nodes[node->cov_pfx_arr_size - 1] =
        (trie_node_t *)cp_restore;
    node->cov_pfx_restore_count++;
  } else {
    if (!pipe_tbl->match_spec_template) {
      PIPE_MGR_DBGCHK(0);  // This means preclassifier was not processed
      sts = pipe_mgr_set_alpm_tbl_match_act_info(
          tbl_info, pipe_tbl, match_spec, act_spec);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error setting ALPM table 0x%x preclassifier info",
                  __func__,
                  __LINE__,
                  tbl_info->alpm_tbl_hdl);
        return sts;
      }
    }

    node->entry->match_spec =
        pipe_mgr_tbl_copy_match_spec(node->entry->match_spec, match_spec);
    node->entry->act_spec =
        pipe_mgr_tbl_copy_action_spec(node->entry->act_spec, act_spec);
    node->entry->act_fn_hdl = act_fn_hdl;

    p_info->size++;
    update_counts(pipe_tbl, node, true);
    bf_map_add(&pipe_tbl->alpm_entry_hdl_map,
               node->entry->alpm_entry_hdl,
               (void *)node);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_alpm_atcam_restore(dev_target_t dev_tgt,
                                          pipe_mat_tbl_info_t *mat_info,
                                          pipe_mat_tbl_hdl_t ll_tbl_hdl,
                                          pipe_mat_ent_hdl_t ll_ent_hdl,
                                          pipe_tbl_match_spec_t *match_spec,
                                          pipe_act_fn_hdl_t act_fn_hdl,
                                          pipe_action_spec_t *act_spec) {
  bf_map_sts_t msts;
  alpm_tbl_info_t *tbl_info;
  alpm_pipe_tbl_t *pipe_tbl = NULL;
  trie_node_t *node;
  alpm_entry_t *ent_info;
  hdl_info_t *hdl_info;
  pipe_mat_ent_hdl_t alpm_ent_hdl;
  partition_info_t *p_info;

  tbl_info =
      pipe_mgr_alpm_tbl_info_get_from_ll_hdl(dev_tgt.device_id, ll_tbl_hdl);
  if (!tbl_info) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with atcam handle 0x%x, device id %d",
        __func__,
        ll_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_tbl =
      get_pipe_tbl_instance(tbl_info, dev_tgt.dev_pipe_id, __func__, __LINE__);
  if (!pipe_tbl) return PIPE_OBJ_NOT_FOUND;

  msts = bf_map_get(
      &pipe_tbl->atcam_entry_hdl_map, ll_ent_hdl, (void **)&hdl_info);
  if (msts != BF_MAP_OK) {
    return PIPE_UNEXPECTED;
  }

  node = find_node(pipe_tbl, match_spec, false, NULL, NULL, NULL);
  if (!node) {
    return PIPE_UNEXPECTED;
  }
  if (!hdl_info->cp_hdl) {
    // Normal entry
    alpm_ent_hdl = hdl_info->alpm_hdl;
    bf_map_get_rmv(
        &pipe_tbl->alpm_entry_hdl_map, alpm_ent_hdl, (void **)&ent_info);
    node->entry = ent_info;
    pipe_mgr_mat_tbl_key_insert(tbl_info->dev_id,
                                mat_info,
                                match_spec,
                                alpm_ent_hdl,
                                pipe_tbl->pipe_id,
                                false);
  }
  if (ALPM_IS_SCALE_OPT_ENB(tbl_info)) {
    p_info = &pipe_tbl->partitions[match_spec->partition_index - 1];
  } else {
    p_info = &pipe_tbl->partitions[match_spec->partition_index];
  }

  return pipe_mgr_alpm_atcam_restore_int(tbl_info,
                                         pipe_tbl,
                                         ll_ent_hdl,
                                         node,
                                         p_info,
                                         0,
                                         match_spec,
                                         act_fn_hdl,
                                         act_spec,
                                         (hdl_info->cp_hdl > 0));
}

void pipe_mgr_alpm_restore_cp(alpm_tbl_info_t *tbl_info) {
  alpm_pipe_tbl_t *pipe_tbl;
  partition_info_t *p_info, *last_free_ptn;
  trie_node_t *node, *cp;
  trie_subtree_t *subtree;
  cp_restore_t *cp_restore;
  uint32_t pipe_idx, ptn_idx, subtree_idx;

  for (pipe_idx = 0; pipe_idx < tbl_info->num_pipes; pipe_idx++) {
    pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    pipe_tbl->ptn_in_use_list = NULL;
    pipe_tbl->ptn_free_list = NULL;
    last_free_ptn = NULL;
    for (ptn_idx = 0; ptn_idx < tbl_info->num_partitions; ptn_idx++) {
      p_info = &pipe_tbl->partitions[ptn_idx];
      if (p_info->size == 0) {
        if (!pipe_tbl->ptn_free_list) {
          pipe_tbl->ptn_free_list = p_info;
        }
        if (last_free_ptn) {
          last_free_ptn->next = p_info;
        }
        last_free_ptn = p_info;
      } else {
        p_info->next = pipe_tbl->ptn_in_use_list;
        p_info->prev = NULL;
        if (pipe_tbl->ptn_in_use_list) {
          pipe_tbl->ptn_in_use_list->prev = p_info;
        }
        pipe_tbl->ptn_in_use_list = p_info;
        pipe_tbl->partitions_in_use++;

        for (subtree_idx = 0; subtree_idx < p_info->num_subtrees;
             subtree_idx++) {
          node = p_info->subtree_nodes[subtree_idx];
          if (!node->entry) {
            cp = find_covering_prefix(node);
            if (cp) {
              subtree = node->subtree;
              cp_restore = NULL;
              bool found = false;
              uint32_t i;
              for (i = 0; i < cp->cov_pfx_restore_count; i++) {
                cp_restore = (cp_restore_t *)cp->cov_pfx_subtree_nodes[i];
                if (cp_restore->ptn_idx == p_info->ptn_index &&
                    cp_restore->subtree_id == subtree->subtree_id) {
                  found = true;
                  break;
                }
              }
              if (cp->cov_pfx_restore_count <= 0 || found != true ||
                  !cp_restore || p_info != subtree->partition) {
                LOG_ERROR(
                    "%s:%d Could not find covering prefix info for table "
                    "with handle 0x%x, partition %d, subtree %d",
                    __func__,
                    __LINE__,
                    tbl_info->alpm_tbl_hdl,
                    p_info->ptn_index,
                    subtree->subtree_id);
                PIPE_MGR_DBGCHK(cp->cov_pfx_restore_count > 0);
                PIPE_MGR_DBGCHK(p_info == subtree->partition);
                PIPE_MGR_DBGCHK(found == true);
                return;
              }

              subtree->cov_pfx = cp;
              subtree->cov_pfx_entry = copy_alpm_entry(cp->entry);
              if (subtree->cov_pfx_entry) {
                subtree->cov_pfx_entry->sram_entry_hdl = cp_restore->ent_hdl;
                subtree->cov_pfx_entry->ttl = 0;
                if (subtree->cov_pfx_entry->match_spec) {
                  subtree->cov_pfx_entry->match_spec->partition_index =
                      p_info->ptn_index;
                  if (tbl_info->atcam_subset_key_width) {
                    subtree->cov_pfx_entry->match_spec->version_bits =
                        tbl_info->cp_ver_bits;
                  }
                }
              }
              subtree->partition->size++;

              if (cp_restore) PIPE_MGR_FREE(cp_restore);
              cp->cov_pfx_subtree_nodes[i] = node;
              cp->cov_pfx_count++;
              // Reset the restore count once all nodes are restored
              if (cp->cov_pfx_count == cp->cov_pfx_restore_count) {
                cp->cov_pfx_restore_count = 0;
              }
            }
          }
        }
      }
    }
  }
}

pipe_status_t pipe_mgr_alpm_restore_state(bf_dev_id_t dev_id,
                                          pipe_mat_tbl_info_t *mat_info,
                                          cJSON *alpm_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  alpm_tbl_info_t *tbl_info = NULL;
  alpm_pipe_tbl_t *alpm_pipe_tbl = NULL;
  alpm_entry_t *ent_info = NULL;
  hdl_info_t *hdl_info = NULL;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t alpm_ent_hdl;
  pipe_mat_ent_hdl_t atcam_ent_hdl;
  uint32_t pipe_idx;
  bool symmetric;
  cJSON *pipe_tbls, *pipe_tbl, *def_ent, *mat_ents, *mat_ent;
  cJSON *cps, *cp;
  cJSON *preclass_tbl, *atcam_tbl;
  scope_pipes_t scopes = 0xf;

  tbl_hdl = cJSON_GetObjectItem(alpm_tbl, "handle")->valueint;
  tbl_info = pipe_mgr_alpm_tbl_info_get(dev_id, tbl_hdl);
  if (tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the ALPM match table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  symmetric = (cJSON_GetObjectItem(alpm_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != tbl_info->is_symmetric) {
    sts = pipe_mgr_alpm_tbl_set_symmetric_mode(
        sess_hdl, dev_id, tbl_hdl, symmetric, 1, &scopes);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, alpm tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      return sts;
    }
  }
  mat_info->duplicate_entry_check =
      (cJSON_GetObjectItem(alpm_tbl, "duplicate_entry_check")->type ==
       cJSON_True);

  pipe_tbls = cJSON_GetObjectItem(alpm_tbl, "pipe_tbls");
  for (pipe_tbl = pipe_tbls->child, pipe_idx = 0; pipe_tbl;
       pipe_tbl = pipe_tbl->next, pipe_idx++) {
    alpm_pipe_tbl = tbl_info->pipe_tbls[pipe_idx];
    PIPE_MGR_DBGCHK(
        alpm_pipe_tbl->pipe_id ==
        (uint32_t)cJSON_GetObjectItem(pipe_tbl, "pipe_id")->valueint);

    def_ent = cJSON_GetObjectItem(pipe_tbl, "default_ent_hdl");
    if (def_ent) {
      alpm_pipe_tbl->default_alpm_ent_hdl = def_ent->valuedouble;
      alpm_pipe_tbl->default_atcam_ent_hdl =
          cJSON_GetObjectItem(pipe_tbl, "default_atcam_ent_hdl")->valuedouble;

      hdl_info = PIPE_MGR_MALLOC(sizeof(hdl_info_t));
      hdl_info->alpm_hdl = def_ent->valuedouble;
      hdl_info->cp_hdl = 0;
      bf_map_add(&alpm_pipe_tbl->atcam_entry_hdl_map,
                 alpm_pipe_tbl->default_atcam_ent_hdl,
                 (void *)hdl_info);
    }

    mat_ents = cJSON_GetObjectItem(pipe_tbl, "match_entries");
    for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
      alpm_ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;
      atcam_ent_hdl = cJSON_GetObjectItem(mat_ent, "atcam_hdl")->valuedouble;

      ent_info = PIPE_MGR_CALLOC(1, sizeof(alpm_entry_t));
      ent_info->alpm_entry_hdl = alpm_ent_hdl;
      ent_info->sram_entry_hdl = atcam_ent_hdl;
      ent_info->ttl = cJSON_GetObjectItem(mat_ent, "ttl")->valuedouble;
      bf_map_add(
          &alpm_pipe_tbl->alpm_entry_hdl_map, alpm_ent_hdl, (void *)ent_info);

      hdl_info = PIPE_MGR_MALLOC(sizeof(hdl_info_t));
      hdl_info->alpm_hdl = alpm_ent_hdl;
      hdl_info->cp_hdl = 0;
      bf_map_add(
          &alpm_pipe_tbl->atcam_entry_hdl_map, atcam_ent_hdl, (void *)hdl_info);
      bf_id_allocator_set(alpm_pipe_tbl->ent_hdl_allocator,
                          PIPE_GET_HDL_VAL(alpm_ent_hdl));

      cps = cJSON_GetObjectItem(mat_ent, "covering_prefixes");
      for (cp = cps->child; cp; cp = cp->next) {
        hdl_info = PIPE_MGR_MALLOC(sizeof(hdl_info_t));
        hdl_info->alpm_hdl = alpm_ent_hdl;
        hdl_info->cp_hdl = cJSON_GetObjectItem(cp, "cp_hdl")->valuedouble;
        bf_map_add(&alpm_pipe_tbl->atcam_entry_hdl_map,
                   cJSON_GetObjectItem(cp, "atcam_hdl")->valuedouble,
                   (void *)hdl_info);
        bf_id_allocator_set(alpm_pipe_tbl->ent_hdl_allocator,
                            PIPE_GET_HDL_VAL(hdl_info->cp_hdl));
      }
    }
  }

  preclass_tbl = alpm_tbl->next;
  PIPE_MGR_DBGCHK(
      tbl_info->preclass_tbl_hdl ==
      (uint32_t)cJSON_GetObjectItem(preclass_tbl, "handle")->valueint);
  atcam_tbl = preclass_tbl->next;
  PIPE_MGR_DBGCHK(tbl_info->atcam_tbl_hdl ==
                  (uint32_t)cJSON_GetObjectItem(atcam_tbl, "handle")->valueint);

  sts = pipe_mgr_tcam_restore_state(
      dev_id, mat_info, preclass_tbl, pipe_mgr_alpm_preclass_restore);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Preclassifier table 0x%x for alpm table 0x%x restore failed",
        __func__,
        __LINE__,
        tbl_info->preclass_tbl_hdl,
        tbl_hdl);
    return sts;
  }

  sts = pipe_mgr_tcam_restore_state(
      dev_id, mat_info, atcam_tbl, pipe_mgr_alpm_atcam_restore);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Atcam table 0x%x for alpm table 0x%x restore failed",
              __func__,
              __LINE__,
              tbl_info->atcam_tbl_hdl,
              tbl_hdl);
    return sts;
  }

  pipe_mgr_alpm_restore_cp(tbl_info);
  return sts;
}
