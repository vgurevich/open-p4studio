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
 * @file cuckoo_move_bfs.c
 * @date
 *
 * Implementation of the bfs algorithm for cuckoo move.
 */

/* Standard includes */
#include <stdbool.h>
#include <math.h>

#define JUDYERROR(CallerFile, CallerLine, JudyFunc, JudyErrno, JudyErrID) \
  {                                                                       \
    if ((JudyErrno) != JU_ERRNO_NOMEM) /* ! a malloc() failure */         \
    {                                                                     \
      (void)fprintf(stderr,                                               \
                    "File '%s', line %d: %s(), "                          \
                    "JU_ERRNO_* == %d, ID == %d\n",                       \
                    CallerFile,                                           \
                    CallerLine,                                           \
                    JudyFunc,                                             \
                    JudyErrno,                                            \
                    JudyErrID);                                           \
    }                                                                     \
  }

/* Module header files */

#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <target-utils/third-party/judy-1.0.5/src/Judy.h>
/* Local header files */
#include "pipe_mgr_log.h"
#include "pipe_mgr_exm_hash.h"
#include "cuckoo_move.h"

/* Some basic accessor routines defined on the logical entry container */
/* FIXME: Need to move these accessor routines out of the cuckoo_move part
 * to the "general" regions of the pipeline manager.
 */
uint32_t pipe_mat_tbl_logical_entry_container_len(
    pipe_mat_tbl_log_entry_container_t *container) {
  if (container == NULL) {
    return 0;
  }

  return container->num_entries;
}

pipe_mat_ent_idx_t pipe_mat_tbl_get_log_entry(
    pipe_mat_tbl_log_entry_container_t *container, uint32_t idx) {
  if (container == NULL) {
    return PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  }

  if (idx >= container->num_entries) {
    return PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  }

  return container->entries[idx];
}

/** \brief cuckoo_move_graph_cleanup:
 *         Given a pointer to the cuckoo_move_graph, cleanup the graph
 *         and free the memory occupied by the graph. The cuckoo move graph
 *         is a composite "object", hence requires custom clean-up.
 *
 *
 *
 * \param cuckoo_graph A pointer to the cuckoo move graph that needs to be
 *        cleaned up.
 * \return Nothing
 */

void cuckoo_move_graph_cleanup(cuckoo_move_graph_t *cuckoo_graph) {
  uint32_t i = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  pipe_mat_ent_idx_t *ent_idx = NULL;
  for (i = 0; i < cuckoo_graph->num_nodes; i++) {
    cuckoo_move_graph_node_cleanup(&cuckoo_graph->nodes[i]);
  }
  PIPE_MGR_FREE(cuckoo_graph->nodes);
  PIPE_MGR_FREE(cuckoo_graph->edges);
  PIPE_MGR_FREE(cuckoo_graph->bfs_queue.nodes);
  PIPE_MGR_FREE(cuckoo_graph->cache_bfs_queue.nodes);
  PIPE_MGR_FREE(cuckoo_graph->move_list);
  if (cuckoo_graph->dirtied_ent_idx_htbl) {
    while ((map_sts = bf_map_get_first_rmv(&cuckoo_graph->dirtied_ent_idx_htbl,
                                           &key,
                                           (void **)&ent_idx)) == BF_MAP_OK) {
      PIPE_MGR_FREE(ent_idx);
    }
    bf_map_destroy(&cuckoo_graph->dirtied_ent_idx_htbl);
  }

  PIPE_MGR_FREE(cuckoo_graph);

  return;
}

static pipe_status_t cuckoo_move_graph_node_backup(
    cuckoo_move_graph_t *cuckoo_graph, pipe_mat_ent_idx_t node_idx) {
  cuckoo_graph_node_data_t *node_data = NULL;
  cuckoo_graph_node_data_t *backup = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = node_idx;

  node_data = &((cuckoo_graph->nodes[node_idx]).node_data);

  if (cuckoo_graph->nodes[node_idx].backup == NULL) {
    /* Make a backup */
    cuckoo_graph->nodes[node_idx].backup = backup =
        PIPE_MGR_MALLOC(sizeof(cuckoo_graph_node_data_t));

    PIPE_MGR_MEMSET(cuckoo_graph->nodes[node_idx].backup,
                    0,
                    sizeof(cuckoo_graph_node_data_t));

    if (node_data != NULL) {
      PIPE_MGR_MEMCPY(cuckoo_graph->nodes[node_idx].backup,
                      node_data,
                      sizeof(cuckoo_graph_node_data_t));
    }

    /* Reset the fwd_edges */
    backup->fwd_edges = NULL;

    if (backup->occupied == true) {
      backup->fwd_edges =
          PIPE_MGR_MALLOC(backup->num_edges * sizeof(pipe_mat_ent_idx_t));

      if (backup->fwd_edges == NULL) {
        LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }

      if (node_data != NULL) {
        PIPE_MGR_MEMCPY(backup->fwd_edges,
                        node_data->fwd_edges,
                        backup->num_edges * sizeof(pipe_mat_ent_idx_t));
      }
    }
    pipe_mat_ent_idx_t *entry_idx = NULL;
    entry_idx =
        (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mat_ent_idx_t));
    if (entry_idx == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    *entry_idx = node_idx;
    map_sts =
        bf_map_add(&cuckoo_graph->dirtied_ent_idx_htbl, key, (void *)entry_idx);
    PIPE_MGR_DBGCHK(map_sts != BF_MAP_KEY_EXISTS);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR("%s:%d Error in inserting entry idx %d into htbl, err 0x%x",
                __func__,
                __LINE__,
                *entry_idx,
                map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }

  return PIPE_SUCCESS;
}

static void cuckoo_move_graph_node_txn_commit(
    cuckoo_move_graph_t *cuckoo_move_graph, uint32_t entry_idx) {
  cuckoo_move_graph_node_t *node = NULL;
  cuckoo_graph_node_data_t *backup = NULL;

  node = &cuckoo_move_graph->nodes[entry_idx];

  backup = node->backup;

  /* Check if the backup exists */
  if (backup != NULL) {
    if (backup->fwd_edges != NULL) {
      PIPE_MGR_FREE(backup->fwd_edges);
      backup->fwd_edges = NULL;
    }

    PIPE_MGR_FREE(backup);

    node->backup = NULL;
  }

  return;
}

static void cuckoo_move_graph_node_txn_abort(
    cuckoo_move_graph_t *cuckoo_move_graph, uint32_t entry_idx) {
  cuckoo_move_graph_node_t *node = NULL;
  cuckoo_graph_node_data_t *node_data = NULL;
  cuckoo_graph_node_data_t *backup = NULL;
  pipe_mat_ent_idx_t *fwd_edges = NULL;

  node = &cuckoo_move_graph->nodes[entry_idx];
  node_data = &node->node_data;
  backup = node->backup;
  /* Check if the backup exists */
  if (backup != NULL) {
    if (backup->fwd_edges != NULL) {
      if (node_data->fwd_edges == NULL) {
        node_data->fwd_edges =
            PIPE_MGR_CALLOC(backup->num_edges, sizeof(pipe_mat_ent_idx_t));

        if (node_data->fwd_edges == NULL) {
          LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
          return;
        }
      }
      PIPE_MGR_MEMCPY(node_data->fwd_edges,
                      backup->fwd_edges,
                      backup->num_edges * sizeof(pipe_mat_ent_idx_t));
      /* Cache the fwd_edges */
      fwd_edges = node_data->fwd_edges;
    } else {
      /* The node did not have any edges before the transaction */
      if (node_data->fwd_edges) {
        PIPE_MGR_FREE(node_data->fwd_edges);
        node_data->fwd_edges = NULL;
        node_data->num_edges = 0;
      }
    }
    PIPE_MGR_MEMCPY(node_data, backup, sizeof(cuckoo_graph_node_data_t));
    node_data->fwd_edges = fwd_edges;
    if (backup->fwd_edges != NULL) {
      PIPE_MGR_FREE(backup->fwd_edges);
    }
    PIPE_MGR_FREE(backup);
    node->backup = NULL;
  }
  return;
}

static pipe_status_t cuckoo_move_txn_state_update(
    cuckoo_move_graph_t *cuckoo_graph, cuckoo_move_list_t *move_list) {
  pipe_status_t status;
  cuckoo_move_list_t *traverser = NULL;
  pipe_mat_ent_idx_t entry_idx;

  traverser = move_list;

  /* Walk over the move list and check if a backup exists for each node, if
   * not, create one, else, just move on.
   */

  while (traverser) {
    if (traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      entry_idx = traverser->dst_entry;
    } else if (traverser->src_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      entry_idx = traverser->src_entry;
    } else {
      LOG_ERROR(
          "%s/%d : Invalid entry in the cuckoo move list", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
    status = cuckoo_move_graph_node_backup(cuckoo_graph, entry_idx);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d : Error in backing up cuckoo move graph node "
          "index %d, error %s",
          __func__,
          __LINE__,
          entry_idx,
          pipe_str_err(status));
      return status;
    }
    traverser = traverser->next;
  }
  return PIPE_SUCCESS;
}

void cuckoo_move_graph_node_cleanup(
    cuckoo_move_graph_node_t *cuckoo_graph_node) {
  if (cuckoo_graph_node == NULL) {
    return;
  }

  if (cuckoo_graph_node->node_data.fwd_edges) {
    PIPE_MGR_FREE((cuckoo_graph_node->node_data).fwd_edges);
  }

  if (cuckoo_graph_node->backup) {
    if (cuckoo_graph_node->backup->fwd_edges) {
      PIPE_MGR_FREE(cuckoo_graph_node->backup->fwd_edges);
    }

    PIPE_MGR_FREE(cuckoo_graph_node->backup);
  }
}

pipe_status_t cuckoo_program_new_entry(
    cuckoo_move_list_t **move_list,
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mgr_exm_edge_container_t *edge_container) {
  pipe_status_t status = PIPE_SUCCESS;

  if (cuckoo_graph == NULL) {
    LOG_ERROR("%s : Passed in cuckoo graph instance is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  /* At the mercy of what the search says */
  status = cuckoo_move_bfs(cuckoo_graph, move_list, edge_container);
  return status;
}

pipe_status_t cuckoo_compute_move_list(
    cuckoo_move_list_t **move_list,
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mgr_exm_edge_container_t *edge_container) {
  pipe_status_t status = PIPE_SUCCESS;

  if (edge_container == NULL) {
    LOG_ERROR(
        "%s: Candidate entries not given for the new entry to "
        "be programmed",
        __func__);

    return PIPE_INVALID_ARG;
  }

  if (cuckoo_graph == NULL) {
    LOG_ERROR("%s : Passed in cuckoo graph instance is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  /* At the mercy of what the search says */
  status = cuckoo_move_bfs(cuckoo_graph, move_list, edge_container);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s : The new entry could not be accommodated %s",
              __func__,
              pipe_str_err(status));

    *move_list = PIPE_MGR_CALLOC(1, sizeof(cuckoo_move_list_t));

    (*move_list)->src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
    (*move_list)->dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;

    return status;
  }

  return status;
}

void cuckoo_delete_entry(cuckoo_move_graph_t *cuckoo_graph,
                         pipe_mat_ent_idx_t entry_idx,
                         bool isTxn) {
  pipe_status_t status = PIPE_SUCCESS;
  cuckoo_graph_node_data_t *cuckoo_node_data = NULL;

  cuckoo_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, entry_idx);

  if (isTxn) {
    status = cuckoo_move_graph_node_backup(cuckoo_graph, entry_idx);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d : Error in backing up cuckoo graph node index %d"
          " for transaction purposes, erro %s",
          __func__,
          __LINE__,
          entry_idx,
          pipe_str_err(status));
      return;
    }
  }

  if (cuckoo_node_data == NULL) {
    LOG_ERROR(
        "%s : Could not find the data for the cuckoo graph node for "
        " entry index %d",
        __func__,
        entry_idx);
    return;
  }

  /* Delete involves clearing out the edges and marking the node as
   * not occupied.
   */
  if (cuckoo_node_data->fwd_edges) {
    PIPE_MGR_FREE(cuckoo_node_data->fwd_edges);
    cuckoo_node_data->fwd_edges = NULL;
  }
  cuckoo_node_data->num_edges = 0;
  cuckoo_node_data->occupied = false;

  if (!isTxn) {
    /* If this operation is not done as part of the transaction, effect
     * the changes on the scratch space as well.
     */
    cuckoo_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, entry_idx);
    if (cuckoo_node_data == NULL) {
      LOG_ERROR(
          "%s:%d Could not find the data for the cuckoo graph node with "
          " entry index %d",
          __func__,
          __LINE__,
          entry_idx);
      return;
    }

    if (cuckoo_node_data->fwd_edges) {
      PIPE_MGR_FREE(cuckoo_node_data->fwd_edges);
      cuckoo_node_data->fwd_edges = NULL;
    }
    cuckoo_node_data->num_edges = 0;
    cuckoo_node_data->occupied = false;
  }
  return;
}

/** \brief cuckoo_move_graph_get_node
 *         Given a cuckoo move graph and an index of the nodes
 *         return a pointer to the node.
 *
 * \param cuckoo_graph A pointer to the cuckoo move graph
 * \param entry_idx Index of the node needed.
 * \return cuckoo_move_graph_node_t* A pointer to the requested node,
 *         if found, NULL otherwise.
 */

cuckoo_graph_node_data_t *cuckoo_move_graph_get_node_data(
    cuckoo_move_graph_t *cuckoo_graph, pipe_mat_ent_idx_t entry_idx) {
  cuckoo_graph_node_data_t *ret_node = NULL;

  if (cuckoo_graph == NULL) {
    LOG_ERROR("%s: cuckoo move graph non-existent", __func__);
    return NULL;
  }

  if (entry_idx >= cuckoo_graph->num_nodes) {
    LOG_ERROR("%s: Requested node idx %d greater than what the graph holds",
              __func__,
              entry_idx);
    return NULL;
  }

  ret_node = &cuckoo_graph->nodes[entry_idx].node_data;

  return ret_node;
}

/** \brief cuckoo_move_graph_execute_moves
 *         Given a cuckoo move list, execute the moves at the cuckoo graph
 *         level.
 *         This only updates the state of the cuckoo move graph. The ACTUAL
 *         moving of entries at the device level is in the realm of the caller.
 *
 * \param cuckoo_graph A pointer to the cuckoo move graph on which the moves
 *        are to be executed.
 * \param move_list A pointer to the cuckoo move list on the basis of which
 *        the entries are to be moved.
 * \param candidate_entries The candidate locations of the entry, whose
 *        insertion triggered the cuckoo moves.
 * \return Nothing.
 */

pipe_status_t cuckoo_move_graph_execute_moves(
    cuckoo_move_graph_t *cuckoo_graph,
    cuckoo_move_list_t *move_list,
    pipe_mgr_exm_edge_container_t *candidate_entries,
    bool isTxn) {
  cuckoo_move_list_t *traverser = NULL;

  pipe_status_t status = PIPE_SUCCESS;

  if (cuckoo_graph == NULL) {
    LOG_ERROR("%s: NULL cuckoo graph passed", __func__);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (move_list == NULL) {
    LOG_ERROR("%s: NULL move list passed", __func__);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (candidate_entries == NULL) {
    LOG_ERROR("%s: List of candidate locations not passed", __func__);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* If this operation is done as part of a transaction, take backup of
   * relevant nodes in order to be able to restore the state.
   */

  if (isTxn) {
    status = cuckoo_move_txn_state_update(cuckoo_graph, move_list);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s/%d : Error in updating transaction state for cuckoo"
          " move graph",
          __func__,
          __LINE__);
      return status;
    }
  }

  /* Need to execute the moves in the reverse order of the
   * move_list. Head over to the tail.
   */

  traverser = move_list;

  while (traverser->next) {
    traverser = traverser->next;
  }

  while (traverser) {
    if (traverser->src_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      status = cuckoo_move_graph_move_node(
          cuckoo_graph, traverser->dst_entry, traverser->src_entry, isTxn);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Moving node at index %d to destination %d"
            " failed\n",
            __func__,
            traverser->src_entry,
            traverser->dst_entry);

        return status;
      }
    } else if (traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      status = cuckoo_move_graph_insert_node(
          cuckoo_graph, traverser->dst_entry, candidate_entries);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR("%s: Insertion of node into index %d failed",
                  __func__,
                  traverser->dst_entry);

        return status;
      }
    }
    traverser = traverser->previous;
  }

  return PIPE_SUCCESS;
}

/** \brief cuckoo_move_graph_move_node
 *         Given a cuckoo graph, a src index and a dst index, move the node.
 *         from the src_idx to dst_idx.
 *
 * \param cuckoo_graph A pointer to the cuckoo move graph on which the moves
 *        are to be executed.
 * \param src_idx Index of the source entry.
 * \param dst_idx Index of the dst entry.
 * \return Nothing.
 */

pipe_status_t cuckoo_move_graph_move_node(cuckoo_move_graph_t *cuckoo_graph,
                                          pipe_mat_ent_idx_t dst_idx,
                                          pipe_mat_ent_idx_t src_idx,
                                          bool isTxn) {
  cuckoo_graph_node_data_t *src_node_data = NULL;
  cuckoo_graph_node_data_t *dst_node_data = NULL;
  uint32_t num_edges = 0;

  if (cuckoo_graph == NULL) {
    LOG_ERROR("%s: Cuckoo graph passed is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  src_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, src_idx);

  if (src_node_data == NULL) {
    LOG_ERROR(
        "%s : Cuckoo graph node data not found for idx %d", __func__, src_idx);
    return PIPE_INVALID_ARG;
  }

  dst_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, dst_idx);

  if (dst_node_data == NULL) {
    LOG_ERROR(
        "%s : Cuckoo graph node data not found for idx %d", __func__, dst_idx);
    return PIPE_INVALID_ARG;
  }

  num_edges = cuckoo_move_graph_node_get_num_edges(src_node_data);

  if (dst_node_data->fwd_edges == NULL) {
    /* Allocate the edges */
    dst_node_data->fwd_edges = (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(
        num_edges, sizeof(pipe_mat_ent_idx_t));
  }

  PIPE_MGR_MEMCPY(dst_node_data->fwd_edges,
                  src_node_data->fwd_edges,
                  sizeof(pipe_mat_ent_idx_t) * num_edges);

  dst_node_data->num_edges = num_edges;
  dst_node_data->occupied = true;

  if (!isTxn) {
    src_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, src_idx);

    dst_node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, dst_idx);

    if (src_node_data == NULL) {
      LOG_ERROR("%s : Cuckoo graph src node data not found for idx %d",
                __func__,
                src_idx);
      return PIPE_INVALID_ARG;
    }

    if (dst_node_data == NULL) {
      LOG_ERROR("%s : Cuckoo graph dst node data not found for idx %d",
                __func__,
                dst_idx);
      return PIPE_INVALID_ARG;
    }

    /* If this operation is not done as part of a transaction, effect the
     * modifications on the scratch-space too since all queries/
     * modifications for transaction-based operations are done on the
     * scratch-space.
     */

    if (dst_node_data->fwd_edges == NULL) {
      /* Allocate the edges */
      dst_node_data->fwd_edges = (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(
          num_edges, sizeof(pipe_mat_ent_idx_t));
    }

    PIPE_MGR_MEMCPY(dst_node_data->fwd_edges,
                    src_node_data->fwd_edges,
                    sizeof(pipe_mat_ent_idx_t) * num_edges);

    dst_node_data->num_edges = num_edges;
    dst_node_data->occupied = true;
  }

  return PIPE_SUCCESS;
}

/** \brief cuckoo_move_graph_insert_node
 *         Insert a new node into the given cuckoo graph at the given location.
 *
 *
 * \param cuckoo_graph A pointer to the cuckoo move graph into which the node
 *        is to be inserted.
 * \param dst_idx Index of the destination into which the insertion should
 *        happen.
 * \param candidate_entries A pointer to the list of candidate locations
 *        (which form the edges).
 * \return pipe_status_t Status of the operation.
 */

pipe_status_t cuckoo_move_graph_insert_node(
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mat_ent_idx_t idx,
    pipe_mgr_exm_edge_container_t *edge_container) {
  uint32_t num_edges = 0;

  cuckoo_graph_node_data_t *node_data = NULL;

  if (edge_container == NULL) {
    LOG_ERROR("%s: List of edges not passed", __func__);
    return PIPE_INVALID_ARG;
  }

  if (idx >= cuckoo_graph->num_nodes) {
    LOG_ERROR("%s : Node idx %d exceeds the number of entries present",
              __func__,
              idx);
    return PIPE_INVALID_ARG;
  }

  node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, idx);

  if (node_data == NULL) {
    LOG_ERROR(
        "%s : Could not find the cuckoo move graph node data for idx"
        " %d",
        __func__,
        idx);
    return PIPE_INVALID_ARG;
  }

  num_edges = edge_container->num_entries;

  /* Allocate the edges */

  if (node_data->fwd_edges == NULL) {
    node_data->fwd_edges = (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(
        num_edges, sizeof(pipe_mat_ent_idx_t));

    if (node_data->fwd_edges == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for the edges of the "
          "cuckoo move graph node, at idx %d",
          __func__,
          idx);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  PIPE_MGR_MEMCPY(node_data->fwd_edges,
                  edge_container->entries,
                  num_edges * sizeof(pipe_mat_ent_idx_t));

  node_data->num_edges = num_edges;

  /* Mark the node as occupied */
  node_data->occupied = true;

  return PIPE_SUCCESS;
}

void cuckoo_move_list_cleanup(cuckoo_move_list_t *move_list) {
  cuckoo_move_list_t *traverser = move_list;
  cuckoo_move_list_t *traverser1 = move_list;

  while (traverser) {
    traverser = traverser1->next;
    PIPE_MGR_FREE(traverser1);
    traverser1 = traverser;
  }

  return;
}

bool cuckoo_graph_node_is_occupied(cuckoo_move_graph_t *cuckoo_graph,
                                   uint32_t node_idx) {
  cuckoo_graph_node_data_t *node_data = NULL;

  node_data = cuckoo_move_graph_get_node_data(cuckoo_graph, node_idx);

  if (node_data == NULL) {
    return false;
  }

  return (node_data->occupied);
}

void cuckoo_move_graph_txn_commit(cuckoo_move_graph_t *cuckoo_move_graph) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  pipe_mat_ent_idx_t *ent_idx = NULL;
  if (cuckoo_move_graph->dirtied_ent_idx_htbl == NULL) {
    return;
  }
  while (
      (map_sts = bf_map_get_first_rmv(&cuckoo_move_graph->dirtied_ent_idx_htbl,
                                      &key,
                                      (void **)&ent_idx)) == BF_MAP_OK) {
    cuckoo_move_graph_node_txn_commit(cuckoo_move_graph, *ent_idx);
    PIPE_MGR_FREE(ent_idx);
  }
  return;
}

void cuckoo_move_graph_txn_abort(cuckoo_move_graph_t *cuckoo_move_graph) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;
  pipe_mat_ent_idx_t *ent_idx = NULL;
  if (cuckoo_move_graph->dirtied_ent_idx_htbl == NULL) {
    return;
  }
  while (
      (map_sts = bf_map_get_first_rmv(&cuckoo_move_graph->dirtied_ent_idx_htbl,
                                      &key,
                                      (void **)&ent_idx)) == BF_MAP_OK) {
    cuckoo_move_graph_node_txn_abort(cuckoo_move_graph, *ent_idx);
    PIPE_MGR_FREE(ent_idx);
  }
  return;
}

pipe_status_t cuckoo_move_bfs(cuckoo_move_graph_t *cuckoo_graph,
                              cuckoo_move_list_t **move_list,
                              pipe_mgr_exm_edge_container_t *edge_container) {
  Pvoid_t visited_PJ1Array = NULL;
  bool path_found = false;
  uint32_t i, j = 0;
  uint32_t curr_depth = 0;
  // Word_t   Rc_word         = 0;
  int Rc_int = 0;
  cuckoo_bfs_queue_node_t *elem = NULL;
  cuckoo_bfs_queue_t *queue = &cuckoo_graph->bfs_queue;
  cuckoo_bfs_queue_t *cache_queue = &cuckoo_graph->cache_bfs_queue;

  cuckoo_graph_node_data_t *curr_node = NULL;

  pipe_mat_ent_idx_t curr_edge = 0;
  pipe_mat_ent_idx_t this_edge = 0;

  QUEUE_EMPTY(queue);
  QUEUE_EMPTY(cache_queue);

  for (i = 0; i < edge_container->num_entries; i++) {
    if (cuckoo_graph_edge_is_occupied(cuckoo_graph,
                                      edge_container->entries[i]) == false) {
      cuckoo_graph->move_list->next = NULL;
      cuckoo_graph->move_list->previous = NULL;
      cuckoo_graph->move_list->src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
      cuckoo_graph->move_list->dst_entry = edge_container->entries[i];
      *move_list = cuckoo_graph->move_list;
      return PIPE_SUCCESS;
    }
  }

  for (i = 0; i < edge_container->num_entries; i++) {
    QUEUE_ELEM(queue, edge_container->entries[i], 0, -1, -1);

    J1S(Rc_int, visited_PJ1Array, edge_container->entries[i]);
  }

  while (QUEUE_NOT_EMPTY(queue)) {
    DEQUEUE_ELEM(queue, elem);
    QUEUE_EMPTY(cache_queue);

    while (elem) {
      curr_edge = elem->edge_idx;

      curr_depth = elem->depth;

      if (cuckoo_graph_edge_is_occupied(cuckoo_graph, curr_edge) == false) {
        /* Found the empty node */
        /* Build the path from the root node */

        cuckoo_construct_bfs_path(
            cuckoo_graph, move_list, queue, curr_edge, (queue->head) - 1);
        path_found = true;
        break;
      }

      if (QUEUE_FULL(cache_queue)) {
        break;
      } else {
        QUEUE_ELEM(cache_queue, curr_edge, 0, -1, QUEUE_HEAD(queue) - 1);
      }

      if (QUEUE_NOT_EMPTY(queue)) {
        DEQUEUE_ELEM(queue, elem);
      } else {
        elem = NULL;
      }
    }

    if (path_found == true) {
      break;
    }

    if (curr_depth + 1 > CUCKOO_MAX_NUM_MOVES) {
      path_found = false;
      break;
    }

    DEQUEUE_ELEM(cache_queue, elem)

    while (elem) {
      curr_edge = elem->edge_idx;

      for (i = 0; i < cuckoo_graph->num_entries_in_a_node; i++) {
        curr_node =
            cuckoo_move_graph_get_node_data(cuckoo_graph, curr_edge + i);
        if (!curr_node) {
          LOG_ERROR(
              "%s:%d Error in getting graph node data", __func__, __LINE__);
          return PIPE_OBJ_NOT_FOUND;
        }

        for (j = 0; j < curr_node->num_edges; j++) {
          this_edge = curr_node->fwd_edges[j];

          if (this_edge == curr_edge) {
            continue;
          }

          J1T(Rc_int, visited_PJ1Array, this_edge);

          if (Rc_int == 1) {
            continue;
          } else if (Rc_int == 0) {
            J1S(Rc_int, visited_PJ1Array, this_edge);
          }

          /* If the queue is full, we want to try and find an empty node in the
             already queued nodes. This gives better results compared to the
             approach where we terminate the search on a queue full.
          */
          if (QUEUE_FULL(queue)) {
            break;
          } else {
            QUEUE_ELEM(queue,
                       curr_node->fwd_edges[j],
                       curr_depth + 1,
                       curr_edge + i,
                       elem->parent_pos);
          }
        }
      }
      if (QUEUE_NOT_EMPTY(cache_queue)) {
        DEQUEUE_ELEM(cache_queue, elem);
      } else {
        elem = NULL;
      }
    }
  }

  if (visited_PJ1Array) {
    Word_t bytes_freed;
    J1FA(bytes_freed, visited_PJ1Array);
    (void)bytes_freed;
  }
  if (path_found != true) {
    return PIPE_NO_SPACE;
  }
  return PIPE_SUCCESS;
}

pipe_status_t cuckoo_construct_bfs_path(cuckoo_move_graph_t *cuckoo_graph,
                                        cuckoo_move_list_t **move_list,
                                        cuckoo_bfs_queue_t *queue,
                                        pipe_mat_ent_idx_t start_idx,
                                        uint32_t parent_pos) {
  pipe_mat_ent_idx_t path_arr[CUCKOO_MAX_NUM_MOVES + 1];

  cuckoo_move_list_t *traverser = NULL;
  cuckoo_bfs_queue_node_t *elem = NULL;
  int i = 0, k = 0;
  int j = 0;
  int parent = 0;

  if (move_list == NULL) {
    LOG_ERROR("%s:Move list passed is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  parent = start_idx;

  while (parent != -1) {
    path_arr[i++] = parent;
    QUEUE_GET(queue, parent_pos, elem);
    parent = elem->parent;
    parent_pos = elem->parent_pos;
  }

  *move_list = cuckoo_graph->move_list;

  if (*move_list == NULL) {
    LOG_ERROR("%s %d : Could not allocate memory", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  traverser = *move_list;

  for (j = (i - 1), k = 0; j >= 0; j--, k++) {
    if (j == (i - 1)) {
      traverser[k].src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
      traverser[k].dst_entry = path_arr[j];
    } else {
      traverser[k].src_entry = path_arr[j + 1];
      traverser[k].dst_entry = path_arr[j];
    }

    if (k == (i - 1)) {
      traverser[k].next = NULL;
    } else {
      traverser[k].next = &traverser[k + 1];
    }

    if (k == 0) {
      traverser[k].previous = NULL;
    } else {
      traverser[k].previous = &traverser[k - 1];
    }
  }

  return PIPE_SUCCESS;
}

void cuckoo_mark_edge_occupied(cuckoo_move_graph_t *cuckoo_graph,
                               pipe_mat_ent_idx_t candidate_idx) {
  if (cuckoo_graph == NULL) {
    return;
  }

  cuckoo_graph->edges[candidate_idx].occupied = true;

  return;
}

void cuckoo_mark_edge_free(cuckoo_move_graph_t *cuckoo_graph,
                           pipe_mat_ent_idx_t candidate_idx) {
  if (cuckoo_graph == NULL) {
    return;
  }

  cuckoo_graph->edges[candidate_idx].occupied = false;

  return;
}

bool cuckoo_graph_edge_is_occupied(cuckoo_move_graph_t *cuckoo_graph,
                                   pipe_mat_ent_idx_t candidate_idx) {
  if (cuckoo_graph == NULL) {
    return false;
  }

  return cuckoo_graph->edges[candidate_idx].occupied;
}

/** \brief cuckoo_move_graph_node_get_num_edges
 *         Given a cuckoo move graph node,
 *         return the number of edges.
 *
 * \param this_node A pointer to the cuckoo move graph node.
 * \return uint32_t Number of edges for this node.
 */

uint32_t cuckoo_move_graph_node_get_num_edges(
    cuckoo_graph_node_data_t *this_node) {
  if (this_node == NULL) {
    return 0;
  }

  return this_node->num_edges;
}
