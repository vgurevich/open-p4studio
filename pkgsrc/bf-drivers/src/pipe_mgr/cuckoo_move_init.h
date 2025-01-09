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
 * Definitions relating to the cuckoo move initialization process.
 */

#ifndef _CUCKOO_MOVE_INIT_H
#define _CUCKOO_MOVE_INIT_H

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

/* Structure to hold per-hash way initialization data */
typedef struct cuckoo_move_hash_way_init_data_ {
  uint32_t num_entries;

} cuckoo_move_hash_way_init_t;

/* Structure to hold "per-stage" initialization information of
 * exact match hash entry table.
 */
typedef struct cuckoo_move_graph_stage_init_ {
  /*!< Factoring for a different number of entries per-stage */
  uint32_t num_entries;
  /*!< Factoring for a different hash-way per-stage */
  uint8_t num_hash_ways;
  /*!< Number of entries packed per wide-word */
  uint8_t num_entries_per_wide_word;
  /*!< A pointer to per-hashway data for this stage */
  cuckoo_move_hash_way_init_t *hash_way_data;

} cuckoo_move_graph_stage_init_t;

/* Structure for cuckoo_move_graph initialization */

typedef struct cuckoo_move_graph_init_ {
  bf_dev_id_t dev_id; /*!< Device id to which the table belongs to */
  pipe_mat_tbl_hdl_t
      tbl_hdl; /*!< Table handle with which the graph is associated with */
  uint8_t num_stages;   /*!< Number of stages that the table is present in */
  uint32_t num_entries; /*!< Total number of entries in the table */
  cuckoo_move_graph_stage_init_t
      *stage_init; /*!< A pointer to an array of per-stage data */

} cuckoo_move_graph_init_t;

pipe_status_t cuckoo_move_data_init(cuckoo_move_graph_init_t *init_data);

void cuckoo_move_stage_init_data_cleanup(
    cuckoo_move_graph_stage_init_t *stage_data);

pipe_status_t cuckoo_move_graph_init(uint32_t num_entries,
                                     uint32_t num_hash_ways,
                                     uint8_t num_entries_per_wide_word,
                                     cuckoo_move_graph_t *cuckoo_move_graph);

#ifdef __cplusplus
}
#endif /* C++ */

#endif
