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
 * @file pipe_exm_tbl_utest.c
 * @date
 *
 * Exact-match table manager, routines used over thrift for the purposes of
 * exact match unit-test.
 */

/* Standard header includes */
#include <sys/time.h>
#include <time.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_utest.h"

bf_map_t exm_utest_htbl;

pipe_status_t pipe_mgr_exm_ent_get_candidate_locations(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mat_tbl_log_entry_container_t **candidate_entries) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  dev_stage_t stage_id = 0;
  uint32_t num_hashes = 0;
  uint32_t num_hash_ways = 0;
  uint32_t num_entries_per_wide_word;
  uint32_t num_entries = 0;
  uint32_t entry_idx = 0;
  uint32_t i = 0;
  pipe_exm_hash_t *hash_container = NULL;
  pipe_mgr_exm_edge_container_t *edge_container = NULL;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  stage_id = exm_tbl->exm_tbl_data[0].exm_stage_info[0].stage_id;

  exm_pack_format = exm_tbl->exm_tbl_data[0].exm_stage_info[0].pack_format;

  num_hash_ways = exm_tbl->exm_tbl_data[0].exm_stage_info[0].num_hash_ways;
  num_entries_per_wide_word = exm_pack_format->num_entries_per_wide_word;

  hash_container = PIPE_MGR_CALLOC(2, sizeof(pipe_exm_hash_t));
  status = pipe_mgr_exm_hash_compute(device_id,
                                     exm_tbl->profile_id,
                                     mat_tbl_hdl,
                                     match_spec,
                                     stage_id,
                                     hash_container,
                                     &num_hashes);

  if (status != PIPE_SUCCESS) {
    return PIPE_OBJ_NOT_FOUND;
  }

  edge_container = pipe_mgr_exm_expand_to_logical_entries(
      exm_tbl, BF_DEV_PIPE_ALL, stage_id, hash_container, num_hashes, NULL);
  if (edge_container == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  *candidate_entries =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mat_tbl_log_entry_container_t));
  if (*candidate_entries == NULL) {
    return PIPE_NO_SYS_RESOURCES;
  }

  (*candidate_entries)->entries = PIPE_MGR_CALLOC(
      num_hash_ways * num_entries_per_wide_word, sizeof(pipe_mat_ent_idx_t));

  for (i = 0; i < edge_container->num_entries; i++) {
    for (entry_idx = 0; entry_idx < num_entries_per_wide_word; entry_idx++) {
      (*candidate_entries)->entries[num_entries++] =
          edge_container->entries[i] + entry_idx;
    }
  }

  (*candidate_entries)->num_entries = num_entries;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_compute_cuckoo_move_list(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    dev_stage_t *stage_id,
    cuckoo_move_list_t **move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_exm_hash_t *hash_container = NULL;
  cuckoo_move_graph_t *cuckoo_graph = NULL;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL; /* Assuming symmetric tables */
  uint32_t num_hashes = 0;
  pipe_mgr_exm_edge_container_t *edge_container = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle %d for device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* First, figure out the stage-id in which the entry should be placed in.
   * This is based on a bunch of heuristics at this point, one being occupancy
   * level. Until a certain level of occupancy, a stage is picked, beyond
   * that, the least occupied stage will be picked.
   */

  *stage_id = pipe_mgr_exm_get_stage_for_new_entry(exm_tbl, pipe_id);

  hash_container = PIPE_MGR_CALLOC(2, sizeof(pipe_exm_hash_t));
  status = pipe_mgr_exm_hash_compute(device_id,
                                     exm_tbl->profile_id,
                                     mat_tbl_hdl,
                                     match_spec,
                                     *stage_id,
                                     hash_container,
                                     &num_hashes);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:Error in computing the hashes for new exact match entry"
        " for table with handle %d",
        __func__,
        mat_tbl_hdl);
    return status;
  }

  /* Expand the computed hash to a set of logical entry indices in the stage
   * in which the entry will be placed.
   */
  edge_container = pipe_mgr_exm_expand_to_logical_entries(
      exm_tbl, pipe_id, *stage_id, hash_container, num_hashes, NULL);

  if (edge_container == NULL) {
    LOG_ERROR(
        "%s : Could not expand the computed hash to a list of logical"
        " entries for table with handle %d in stage %d",
        __func__,
        mat_tbl_hdl,
        *stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Now that the hashes are computed, invoke the exact match hash scheme
   * implementation to take the computed hashes for this table and figure out
   * a location to place the entry.
   */

  cuckoo_graph = pipe_mgr_exm_get_cuckoo_graph(exm_tbl, pipe_id, *stage_id);

  cuckoo_compute_move_list(move_list, cuckoo_graph, edge_container);

  status = pipe_mgr_exm_transform_cuckoo_move_list_utest(
      exm_tbl, pipe_id, *stage_id, cuckoo_graph, *move_list);

  return status;
}

pipe_mgr_exm_utest_cached_entry_t *pipe_mgr_exm_utest_get_create_cached_entry(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_utest_htbl_entry_t *data = NULL;
  pipe_mgr_exm_utest_cached_entry_t *data1 = NULL;

  map_sts = bf_map_get(&exm_utest_htbl, exm_tbl->mat_tbl_hdl, (void **)&data);

  if (map_sts != BF_MAP_OK) {
    /* Create the entry in the hash table */
    data = PIPE_MGR_CALLOC(1, sizeof(bf_map_t));

    bf_map_add(&exm_utest_htbl, exm_tbl->mat_tbl_hdl, data);
  }

  map_sts = bf_map_get(&data->ent_hdl_htbl, mat_ent_hdl, (void **)&data1);

  if (map_sts != BF_MAP_OK) {
    data1 = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_exm_utest_cached_entry_t));

    bf_map_add(&data->ent_hdl_htbl, mat_ent_hdl, data1);
  }

  return (pipe_mgr_exm_utest_cached_entry_t *)data1;
}

pipe_status_t pipe_mgr_exm_txn_compare_ent_hdl_state(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  (void)device_id;
  (void)mat_tbl_hdl;
  (void)mat_ent_hdl;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_verify_txn_commit_abort(
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  /* Ensure all backups are destroyed. This has to be called after a
   * txn commit or abort.
   */
  (void)device_id;
  (void)mat_tbl_hdl;
  (void)mat_ent_hdl;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_txn_cache_entry_hdl(uint8_t device_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t mat_ent_hdl) {
  (void)device_id;
  (void)mat_tbl_hdl;
  (void)mat_ent_hdl;
  // pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  // pipe_mgr_exm_utest_cached_entry_t *cached_entry = NULL;
  // pipe_mgr_exm_tbl_entry_data_t *exm_entry_data = NULL;
  // pipe_mgr_exm_entry_loc_info_t *entry_loc_info = NULL;

  // exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  // exm_entry_data =
  //    pipe_mgr_exm_get_create_entry_data(exm_tbl, mat_ent_hdl, false);

  // cached_entry =
  //    pipe_mgr_exm_utest_get_create_cached_entry(exm_tbl, mat_ent_hdl);

  // PIPE_MGR_MEMCPY(&cached_entry->entry_data,
  //       exm_entry_data,
  //       sizeof(pipe_mgr_exm_tbl_entry_data_t));

  // if (cached_entry->entry_data.occupied == false) {
  //  return PIPE_SUCCESS;
  //}

  // cached_entry->entry_data.match_spec.match_value_bits = NULL;
  // cached_entry->entry_data.match_spec.match_mask_bits = NULL;
  // cached_entry->entry_data.action_spec.act_data.action_data_bits = NULL;

  // pipe_mgr_exm_deep_copy_match_spec(&((cached_entry->entry_data).match_spec),
  //                                  &exm_entry_data->match_spec);

  // pipe_mgr_exm_deep_copy_action_spec(&((cached_entry->entry_data).action_spec),
  //                                   &exm_entry_data->action_spec);

  // entry_loc_info = pipe_mgr_exm_get_ent_loc_info(exm_tbl, mat_ent_hdl);

  ///* Cache the entry location info */

  // PIPE_MGR_MEMCPY(&cached_entry->entry_loc_info,
  //       entry_loc_info,
  //       sizeof(pipe_mgr_exm_entry_loc_info_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_transform_cuckoo_move_list_utest(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    cuckoo_move_graph_t *cuckoo_move_graph,
    cuckoo_move_list_t *move_list) {
  (void)cuckoo_move_graph;
  cuckoo_move_list_t *traverser = move_list;
  pipe_mat_ent_idx_t dst_entry = 0;
  int Rc_int = 0;
  Word_t Index = 0;
  uint8_t num_entries_per_wide_word = 0;

  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Could not get exact match table stage info for"
        " for pipe_id %d, stage_id %d",
        __func__,
        __LINE__,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  num_entries_per_wide_word =
      exm_stage_info->pack_format->num_entries_per_wide_word;

  while (traverser) {
    Index = 0;

    if (traverser->next == NULL &&
        traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      dst_entry = traverser->dst_entry;
      if (dst_entry == PIPE_MGR_EXM_DEF_MISS_ENTRY_IDX) {
        Index = Index + 1;
      }

      J1FE(Rc_int, exm_stage_info->PJ1Array[dst_entry], Index);
      PIPE_MGR_DBGCHK(Rc_int != JERR);

      PIPE_MGR_ASSERT(Index < num_entries_per_wide_word);
      traverser->dst_entry = dst_entry + Index;
    }

    traverser = traverser->next;
  }

  return PIPE_SUCCESS;
}
