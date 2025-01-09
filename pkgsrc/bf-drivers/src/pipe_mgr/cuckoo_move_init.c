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
 * @file cuckoo_move_init.c
 * @date
 *
 * Initialization code for the cuckoo move. Includes instantiation of cuckoo
 * move graphs for exact match hash tables, and other such required
 * information per pipe-line stage, in which the table exists.
 */

/* Standard includes */

/* Module header includes */
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header files */

#include "cuckoo_move.h"
#include "cuckoo_move_init.h"
#include "pipe_mgr_log.h"

pipe_status_t cuckoo_move_graph_init(uint32_t num_entries,
                                     uint32_t num_hash_ways,
                                     uint8_t num_entries_per_wide_word,
                                     cuckoo_move_graph_t *cuckoo_move_graph) {
  // PWord_t    PValue;
  // uint32_t   num_elems_in_bfs;

  if (cuckoo_move_graph == NULL) {
    LOG_ERROR("%s : Cuckoo move graph passed is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  if (num_entries == 0) {
    LOG_ERROR("%s : Cuckoo move graph initialization with ZERO entries",
              __func__);
    return PIPE_INVALID_ARG;
  }

  cuckoo_move_graph->num_nodes = num_entries;
  cuckoo_move_graph->num_hash_ways = num_hash_ways;
  cuckoo_move_graph->num_entries_in_a_node = num_entries_per_wide_word;

  cuckoo_move_graph->nodes = (cuckoo_move_graph_node_t *)PIPE_MGR_CALLOC(
      num_entries, sizeof(cuckoo_move_graph_node_t));

  if (cuckoo_move_graph->nodes == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for holding the cuckoo graph"
        " nodes",
        __func__);
    return PIPE_NO_SYS_RESOURCES;
  }

  unsigned i = 0;

  for (i = 0; i < num_entries; i++) {
    cuckoo_move_graph->nodes[i].node_data.fwd_edges =
        PIPE_MGR_CALLOC(num_hash_ways, sizeof(pipe_mat_ent_idx_t));
  }

  cuckoo_move_graph->edges = (cuckoo_move_graph_edge_t *)PIPE_MGR_CALLOC(
      num_entries, sizeof(cuckoo_move_graph_edge_t));

  if (cuckoo_move_graph->edges == NULL) {
    LOG_ERROR(
        "%s: Could not allocate memory for holding the cuckoo graph"
        " edges",
        __func__);
    return PIPE_NO_SYS_RESOURCES;
  }

  // num_elems_in_bfs = num_entries_per_wide_word*num_hash_ways;

  cuckoo_move_graph->bfs_queue.nodes =
      PIPE_MGR_CALLOC(PIPE_MGR_BFS_QUEUE_LEN, sizeof(cuckoo_bfs_queue_node_t));
  cuckoo_move_graph->cache_bfs_queue.nodes =
      PIPE_MGR_CALLOC(PIPE_MGR_BFS_QUEUE_LEN, sizeof(cuckoo_bfs_queue_node_t));

  cuckoo_move_graph->move_list =
      PIPE_MGR_CALLOC(CUCKOO_MAX_NUM_MOVES + 1, sizeof(cuckoo_move_list_t));
  return PIPE_SUCCESS;
}
