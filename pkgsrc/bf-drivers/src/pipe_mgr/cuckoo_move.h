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
 * @file cuckoo_move_bfs.h
 * @date
 *
 * Definitions relating to the bfs algorithm for cuckoo move for exact match
 *hash-table
 * management.
 */

#ifndef _CUCKOO_MOVE_H
#define _CUCKOO_MOVE_H

/* Standard includes */
#include <sys/types.h>

/* Module includes */
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr.h"

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

#define CUCKOO_MAX_NUM_MOVES \
  8 /*!< Max number of cuckoo moves that we will incur */

/* FIXME : For now fix the Stash capacity here, but
 * this actually needs to come from the rmt_config part
 * of the pipe_mgr which is built at boot-up time.
 */
#define CUCKOO_STASH_CAPACITY 8
#define PIPE_MGR_BFS_QUEUE_LEN 50000

#define QUEUE_ELEM(                                                 \
    __queue__, __edge_idx__, __depth__, __parent__, __parent_pos__) \
  __queue__->nodes[__queue__->tail].edge_idx = __edge_idx__;        \
  __queue__->nodes[__queue__->tail].depth = __depth__;              \
  __queue__->nodes[__queue__->tail].parent = __parent__;            \
  __queue__->nodes[__queue__->tail].parent_pos = __parent_pos__;    \
  __queue__->tail++;

#define DEQUEUE_ELEM(__queue__, __elem__)              \
  PIPE_MGR_DBGCHK(__queue__->head != __queue__->tail); \
  __elem__ = &(__queue__->nodes[__queue__->head]);     \
  __queue__->head++;

#define QUEUE_NOT_EMPTY(__queue__) (__queue__->head != __queue__->tail) ? 1 : 0

#define QUEUE_EMPTY(__queue__) __queue__->head = __queue__->tail = 0;

#define QUEUE_HEAD(__queue__) __queue__->head

#define QUEUE_GET(__queue__, __pos__, __elem__) \
  __elem__ = &__queue__->nodes[__pos__];

#define QUEUE_FULL(__queue__) (__queue__->tail >= PIPE_MGR_BFS_QUEUE_LEN)

typedef struct cuckoo_bfs_queue_node_ {
  uint32_t depth;
  int parent;
  uint32_t parent_pos;
  uint32_t edge_idx;

} cuckoo_bfs_queue_node_t;

typedef struct cuckoo_bfs_queue_ {
  uint32_t head;
  uint32_t tail;
  cuckoo_bfs_queue_node_t *nodes;

} cuckoo_bfs_queue_t;

typedef struct cuckoo_move_list_ {
  pipe_mat_ent_idx_t src_entry;
  pipe_mat_ent_idx_t dst_entry;
  struct cuckoo_move_list_ *next;
  struct cuckoo_move_list_ *previous;
  void *entry_data;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  uint8_t *shadow_ptr_arr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];
  pipe_idx_t logical_action_idx;
  pipe_idx_t logical_sel_idx;
  pipe_adt_ent_hdl_t adt_ent_hdl;
  uint32_t selector_len;
  uint32_t ttl;
  uint64_t proxy_hash;
} cuckoo_move_list_t;

/* Structure for cuckoo graph node data */
typedef struct cuckoo_graph_node_data_ {
  uint32_t num_edges;            /*!< Number of edges (hash-ways) */
  bool occupied;                 /*!< Occupied flag */
  pipe_mat_ent_idx_t *fwd_edges; /*!< Pointer to an array of fwd_edges */
} cuckoo_graph_node_data_t;

/* Structure for the node in the cuckoo move graph */
typedef struct cuckoo_move_graph_node {
  cuckoo_graph_node_data_t node_data;

  cuckoo_graph_node_data_t *backup;

} cuckoo_move_graph_node_t;

typedef struct cuckoo_move_graph_edge_ {
  bool occupied;

} cuckoo_move_graph_edge_t;

typedef struct cukoo_move_graph {
  uint32_t num_nodes; /*!< Number of nodes in the graph */
  cuckoo_move_graph_node_t
      *nodes; /*!< Pointer to an array containing the nodes of the graph */
  cuckoo_move_graph_edge_t
      *edges; /*!< Pointer to an array containing the edges of the graph */
  pipe_mat_tbl_hdl_t tbl_hdl; /*!< Table handle of the table with which this
                                 graph is associated with */
  uint32_t num_hash_ways; /*!< Number of hash-ways that make up this table */
  uint32_t num_entries_in_a_node; /*!< Number of sub-entries per node */

  cuckoo_bfs_queue_t bfs_queue;
  cuckoo_bfs_queue_t cache_bfs_queue;
  cuckoo_move_list_t *move_list;
  bf_map_t dirtied_ent_idx_htbl;
} cuckoo_move_graph_t;

/* cuckoo_program_new_entry : Handles a request to program a
 * new entry into a hash-based match-entry table.
 */
pipe_status_t cuckoo_program_new_entry(
    cuckoo_move_list_t **move_list,
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mgr_exm_edge_container_t *edge_container);

/* cuckoo_delete_entry : Handles a request to delete a entry from the
 * given exact match table.
 */
void cuckoo_delete_entry(cuckoo_move_graph_t *cuckoo_graph,
                         pipe_mat_ent_idx_t entry_idx,
                         bool isTxn);

/* cuckoo_graph_move_node : Move a node from src_idx to dst_idx in
 * the given cuckoo move graph.
 */
pipe_status_t cuckoo_move_graph_move_node(cuckoo_move_graph_t *cuckoo_graph,
                                          pipe_mat_ent_idx_t dst_idx,
                                          pipe_mat_ent_idx_t src_idx,
                                          bool isTxn);

/* cuckoo_move_graph_execute_moves : Execute a list of moves in
 * the given cuckoo move graph.
 */
pipe_status_t cuckoo_move_graph_execute_moves(
    cuckoo_move_graph_t *cuckoo_graph,
    cuckoo_move_list_t *move_list,
    pipe_mat_tbl_log_entry_container_t *candidate_entries,
    bool isTxn);

/* cuckoo_move_graph_insert_node : Insert a new node into the cuckoo graph */
pipe_status_t cuckoo_move_graph_insert_node(
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mat_ent_idx_t idx,
    pipe_mgr_exm_edge_container_t *edge_container);

/* cuckoo_move_graph_get_node_data : Given a cuckoo move graph and a node index
 * return the pointer to the graph node.
 */
cuckoo_graph_node_data_t *cuckoo_move_graph_get_node_data(
    cuckoo_move_graph_t *cuckoo_graph, pipe_mat_ent_idx_t entry_idx);

uint32_t cuckoo_move_graph_node_get_num_edges(
    cuckoo_graph_node_data_t *this_node);

void cuckoo_mark_edge_occupied(cuckoo_move_graph_t *cuckoo_graph,
                               pipe_mat_ent_idx_t candidate_idx);

void cuckoo_mark_edge_free(cuckoo_move_graph_t *cuckoo_graph,
                           pipe_mat_ent_idx_t candidate_idx);

bool cuckoo_graph_edge_is_occupied(cuckoo_move_graph_t *cuckoo_graph,
                                   pipe_mat_ent_idx_t candidate_idx);

pipe_status_t cuckoo_compute_move_list(
    cuckoo_move_list_t **move_list,
    cuckoo_move_graph_t *cuckoo_graph,
    pipe_mgr_exm_edge_container_t *edge_container);

void cuckoo_move_graph_cleanup(cuckoo_move_graph_t *cuckoo_graph);

void cuckoo_move_graph_node_cleanup(
    cuckoo_move_graph_node_t *cuckoo_graph_node);

void cuckoo_move_list_cleanup(cuckoo_move_list_t *move_list);

bool cuckoo_graph_node_is_occupied(cuckoo_move_graph_t *cuckoo_graph,
                                   uint32_t node_idx);

void cuckoo_move_graph_txn_commit(cuckoo_move_graph_t *cuckoo_move_graph);

void cuckoo_move_graph_txn_abort(cuckoo_move_graph_t *cuckoo_move_graph);

pipe_status_t cuckoo_move_bfs(cuckoo_move_graph_t *cuckoo_graph,
                              cuckoo_move_list_t **move_list,
                              pipe_mgr_exm_edge_container_t *edge_container);

pipe_status_t cuckoo_construct_bfs_path(cuckoo_move_graph_t *cuckoo_graph,
                                        cuckoo_move_list_t **move_list,
                                        cuckoo_bfs_queue_t *queue,
                                        pipe_mat_ent_idx_t parent,
                                        uint32_t parent_pos);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _CUCKOO_MOVE_BFS_H */
