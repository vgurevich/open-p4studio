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
 * @file pipe_exm_tbl_mgr.c
 * @date
 *
 * Exact-match table manager, which includes all aspects of management such as
 * table additions/deletions/modifications and all of these for other memory
 * types a single exact match entry table might reference..
 */

/* Standard header includes */
#include <math.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <target-utils/id/id.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_exm_hash.h"
#include "pipe_mgr_exm_tbl_mgr.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_drv_workflows.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_select_tbl.h"
#include "pipe_mgr_select_ha.h"
#include "pipe_mgr_act_tbl.h"
#include "cuckoo_move.h"
#include "cuckoo_move_init.h"
#include "pipe_mgr_exm_transaction.h"
#include "pipe_mgr_stats_tbl.h"
#include "pipe_mgr_idle.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_stful_tbl_mgr.h"
#include "pipe_mgr_exm_tof.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hw_dump.h"

static pipe_status_t pipe_mgr_exm_proxy_hash_entry_insert(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    uint64_t proxy_hash);

cuckoo_move_graph_t *pipe_mgr_exm_get_or_create_cuckoo_move_graph(
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  if (exm_stage_info->cuckoo_move_graph == NULL &&
      exm_stage_info->capacity > 0) {
    exm_stage_info->cuckoo_move_graph =
        (cuckoo_move_graph_t *)PIPE_MGR_CALLOC(1, sizeof(cuckoo_move_graph_t));

    if (exm_stage_info->cuckoo_move_graph == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for cuckoo move graph for "
          "exact match table in stage %d",
          __func__,
          exm_stage_info->stage_id);
      return NULL;
    }
    /* For hash-action tables, no need for the cuckoo move graph to be
     * instantiated, since there is no match table.  */
    cuckoo_move_graph_init(
        exm_stage_info->capacity,
        exm_stage_info->num_hash_ways,
        exm_stage_info->pack_format->num_entries_per_wide_word,
        exm_stage_info->cuckoo_move_graph);
  }
  return exm_stage_info->cuckoo_move_graph;
}

static void pipe_mgr_exm_get_resource_delta(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_res_spec_t *resources,
    int resource_count,
    pipe_mgr_act_spec_delta_t *act_spec_delta);

static inline bool pipe_mgr_exm_is_default_ent_installed(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data == NULL) {
    return false;
  }
  return (exm_tbl_data->default_entry_installed);
}

static inline void pipe_mgr_exm_set_def_ent_installed(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data != NULL) {
    exm_tbl_data->default_entry_installed = true;
  }
  return;
}

static inline void pipe_mgr_exm_reset_def_ent_installed(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data == NULL) {
    return;
  }
  exm_tbl_data->default_entry_installed = false;
  return;
}

static void populate_exm_addr_node(pipe_mgr_exm_phy_entry_info_t *entry,
                                   cJSON *entry_node) {
  pipe_mgr_indirect_ptrs_t *ptrs = &(entry->indirect_ptrs);
  cJSON *ptrs_node;

  cJSON_AddItemToObject(entry_node, "ptrs", ptrs_node = cJSON_CreateObject());
  cJSON_AddNumberToObject(ptrs_node, "action", ptrs->adt_ptr);
  cJSON_AddNumberToObject(ptrs_node, "stats", ptrs->stats_ptr);
  cJSON_AddNumberToObject(ptrs_node, "meter", ptrs->meter_ptr);
  cJSON_AddNumberToObject(ptrs_node, "stful", ptrs->stfl_ptr);
  cJSON_AddNumberToObject(ptrs_node, "sel", ptrs->sel_ptr);

  return;
}

static void restore_exm_addr_resources(pipe_mgr_indirect_ptrs_t *ptrs,
                                       cJSON *entry_node) {
  if (ptrs) {
    cJSON *ptrs_node = cJSON_GetObjectItem(entry_node, "ptrs");

    ptrs->stats_ptr = cJSON_GetObjectItem(ptrs_node, "stats")->valuedouble;
    ptrs->meter_ptr = cJSON_GetObjectItem(ptrs_node, "meter")->valuedouble;
    ptrs->stfl_ptr = cJSON_GetObjectItem(ptrs_node, "stful")->valuedouble;
  }

  return;
}

uint8_t pipe_mgr_exm_get_stage_id_from_idx(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data, pipe_mat_ent_idx_t entry_idx) {
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  unsigned i = 0;
  for (i = 0; i < exm_tbl_data->num_stages; i++) {
    exm_stage_info = &exm_tbl_data->exm_stage_info[i];
    if (entry_idx >= exm_stage_info->stage_offset &&
        entry_idx <
            (exm_stage_info->stage_offset + exm_stage_info->num_entries)) {
      return exm_stage_info->stage_id;
    }
  }
  PIPE_MGR_DBGCHK(0);
  return 0xff;
}

pipe_status_t pipe_mgr_exm_update_cuckoo_edge_state(
    pipe_mgr_exm_stage_info_t *exm_stage_info, cuckoo_move_list_t *move_list) {
  cuckoo_move_list_t *traverser = move_list;
  Word_t Index1 = 0;
  Word_t Index2 = -1;
  Word_t Rc_word = 0;
  int Rc_int = 0;
  uint8_t num_entries_per_wide_word = 0;
  cuckoo_move_graph_t *cuckoo_move_graph =
      pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);

  num_entries_per_wide_word =
      exm_stage_info->pack_format->num_entries_per_wide_word;

  while (traverser) {
    pipe_mat_ent_idx_t dst_entry =
        traverser->dst_entry -
        (traverser->dst_entry % num_entries_per_wide_word);
    if (traverser->next == NULL &&
        traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      PIPE_MGR_DBGCHK(traverser->dst_entry !=
                      exm_stage_info->default_miss_entry_idx);
      J1S(Rc_int,
          exm_stage_info->PJ1Array[dst_entry],
          traverser->dst_entry % num_entries_per_wide_word);
      PIPE_MGR_DBGCHK(Rc_int == 1);
      J1C(Rc_word, exm_stage_info->PJ1Array[dst_entry], Index1, Index2);
      if (Rc_word == num_entries_per_wide_word) {
        cuckoo_mark_edge_occupied(cuckoo_move_graph, dst_entry);
      }
    } else {
      PIPE_MGR_DBGCHK(0);
    }
    traverser = traverser->next;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_transform_cuckoo_move_list(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    cuckoo_move_graph_t *cuckoo_move_graph,
    cuckoo_move_list_t *move_list,
    bool *edge_updated,
    uint32_t ent_subword_loc) {
  cuckoo_move_list_t *traverser = move_list;
  pipe_mat_ent_idx_t dst_entry = 0;
  pipe_mat_ent_idx_t src_entry = 0;
  int Rc_int = 0;
  Word_t Index = 0;
  Word_t Index1 = 0;
  Word_t Index2 = -1;
  Word_t Rc_word = 0;
  uint8_t num_entries_per_wide_word = 0;

  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  num_entries_per_wide_word =
      exm_stage_info->pack_format->num_entries_per_wide_word;
  if (edge_updated) *edge_updated = true;
  while (traverser) {
    Index = 0;

    if (traverser->next == NULL &&
        traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      dst_entry = traverser->dst_entry;
      if (dst_entry == exm_stage_info->default_miss_entry_idx &&
          exm_tbl->default_entry_reserved) {
        /* Default Entry index cannot be programmed with a regular
         * match entry.
         */
        J1S(Rc_int, exm_stage_info->PJ1Array[dst_entry], dst_entry);
      }
      if (exm_tbl->restore_ent_node) {
        /* State restore case. Calculate the word and subindex from
         * the stored entry index.
         */
        Index = dst_entry % num_entries_per_wide_word;
        dst_entry -= Index;
      } else {
        if (ent_subword_loc != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
          /*if sub entry index was caluclated from hash function, directly use
           * it and set the judy array for that sub entry location
           */
          Index = ent_subword_loc;
          PIPE_MGR_DBGCHK(Index < num_entries_per_wide_word)
        } else {
          J1FE(Rc_int, exm_stage_info->PJ1Array[dst_entry], Index);
        }
        /* No free entry */
        if (Index >= num_entries_per_wide_word) {
          LOG_ERROR(
              "%s : Could not find a free entry in exact match table with "
              "handle 0x%x. Table full.",
              __func__,
              exm_tbl->mat_tbl_hdl);
          return PIPE_NO_SPACE;
        }
        traverser->dst_entry = dst_entry + Index;
      }
      J1S(Rc_int, exm_stage_info->PJ1Array[dst_entry], Index);

      J1C(Rc_word, exm_stage_info->PJ1Array[dst_entry], Index1, Index2);

      if (Rc_word == num_entries_per_wide_word) {
        cuckoo_mark_edge_occupied(cuckoo_move_graph, dst_entry);
      }
    } else if (traverser->src_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX &&
               traverser->dst_entry == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      /* Delete case */
      src_entry = traverser->src_entry -
                  (traverser->src_entry % num_entries_per_wide_word);
      J1U(Rc_int,
          exm_stage_info->PJ1Array[src_entry],
          traverser->src_entry - src_entry);
      PIPE_MGR_DBGCHK(Rc_int == 1);
      cuckoo_mark_edge_free(cuckoo_move_graph, src_entry);
    }
    traverser = traverser->next;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_rollback_cuckoo_ml_transform(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    cuckoo_move_list_t *move_list) {
  cuckoo_move_list_t *traverser = move_list;
  int Rc_int = 0;
  uint8_t num_entries_per_wide_word = 0;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  cuckoo_move_graph_t *cuckoo_move_graph =
      pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);
  num_entries_per_wide_word =
      exm_stage_info->pack_format->num_entries_per_wide_word;
  while (traverser) {
    pipe_mat_ent_idx_t dst_entry =
        traverser->dst_entry -
        (traverser->dst_entry % num_entries_per_wide_word);
    PIPE_MGR_DBGCHK(traverser->dst_entry !=
                    exm_stage_info->default_miss_entry_idx);
    J1U(Rc_int,
        exm_stage_info->PJ1Array[dst_entry],
        traverser->dst_entry % num_entries_per_wide_word);
    PIPE_MGR_DBGCHK(Rc_int == 1);
    if (traverser->src_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      J1S(Rc_int,
          exm_stage_info
              ->PJ1Array[traverser->src_entry -
                         (traverser->src_entry % num_entries_per_wide_word)],
          traverser->src_entry % num_entries_per_wide_word);
    }
    PIPE_MGR_DBGCHK(Rc_int == 1);
    /* mark the edge of the last node as free because the last
       node in move list is the one going to a new location
       and leave the rest as occupied as before this operation*/
    if (traverser->next == NULL &&
        traverser->dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      cuckoo_mark_edge_free(cuckoo_move_graph, dst_entry);
    }
    traverser = traverser->next;
  }
  return PIPE_SUCCESS;
}

pipe_mgr_exm_edge_container_t *pipe_mgr_exm_expand_to_logical_entries(
    pipe_mgr_exm_tbl_t *exm_tbl,
    bf_dev_pipe_t pipe_id,
    uint8_t stage_id,
    pipe_exm_hash_t *hash_container,
    uint32_t num_hashes,
    uint32_t *subword_loc) {
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_hash_way_data_t *hashway_data = NULL;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  uint32_t hash_way_idx = 0;
  uint32_t hashway_hash = 0;
  uint32_t num_edges = 0;
  uint32_t accum = 0;
  pipe_mat_ent_idx_t logical_entry_idx_start = 0;
  pipe_mgr_exm_edge_container_t *edge_container = NULL;

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return NULL;
  }

  exm_pack_format = exm_stage_info->pack_format;

  edge_container = exm_stage_info->edge_container;

  PIPE_MGR_DBGCHK(num_hashes <= 2);

  /* Based on the packing format, transform the set of hashes into a list
   * entry indices in the given stage.
   */
  if (exm_tbl->hash_action == false) {
    for (hash_way_idx = 0; hash_way_idx < exm_stage_info->num_hash_ways;
         hash_way_idx++) {
      hashway_data = &exm_stage_info->hashway_data[hash_way_idx];

      if (hashway_data->hash_function_id == 0) {
        hashway_hash = pipe_mgr_exm_extract_per_hashway_hash(
            &hash_container[0], hashway_data, subword_loc);
      } else {
        hashway_hash = pipe_mgr_exm_extract_per_hashway_hash(
            &hash_container[1], hashway_data, subword_loc);
      }
      /* From the computed hash, expand to logical entries */
      logical_entry_idx_start =
          (hashway_hash * exm_pack_format->num_entries_per_wide_word) + accum;

      edge_container->entries[num_edges++] = logical_entry_idx_start;

      accum += hashway_data->num_entries;
    }
  } else {
    edge_container = (pipe_mgr_exm_edge_container_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_exm_edge_container_t));
    if (edge_container == NULL) {
      return NULL;
    }
    edge_container->entries =
        (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mat_ent_idx_t));
    if (edge_container->entries == NULL) {
      return NULL;
    }
    edge_container->entries[num_edges++] = hash_container->hash_value;
    edge_container->num_entries = 1;
  }

  return edge_container;
}

pipe_status_t pipe_mgr_exm_find_empty_entry(pipe_mgr_exm_tbl_t *exm_tbl,
                                            bf_dev_pipe_t pipe_id,
                                            uint8_t stage_id,
                                            cuckoo_move_graph_t *cuckoo_graph,
                                            pipe_mat_ent_idx_t *entry_idx) {
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_hash_way_data_t *hashway_data = NULL;
  uint32_t hash_way_idx = 0, idx = 0;
  uint32_t accum = 0;
  pipe_mat_ent_idx_t logical_entry_idx = 0;

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (exm_tbl->hash_action) {
    return PIPE_INVALID_ARG;
  }

  for (hash_way_idx = 0; hash_way_idx < exm_stage_info->num_hash_ways;
       hash_way_idx++) {
    hashway_data = &exm_stage_info->hashway_data[hash_way_idx];

    for (idx = 1; idx < hashway_data->num_entries; idx++) {
      logical_entry_idx = idx + accum;
      if (cuckoo_graph_edge_is_occupied(cuckoo_graph, logical_entry_idx) ==
          false) {
        // found empty entry
        *entry_idx = logical_entry_idx;
        return PIPE_SUCCESS;
      }
    }
    accum += hashway_data->num_entries;
  }

  return PIPE_NO_SPACE;
}

static pipe_status_t pipe_mgr_exm_proxy_hash_entry_remove(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint64_t proxy_hash) {
  bf_hashtable_t *htbl = NULL;
  pipe_mgr_exm_proxy_hash_record_node_t *htbl_node = NULL;
  PIPE_MGR_DBGCHK(exm_tbl->proxy_hash == true);
  htbl = exm_stage_info->proxy_hash_tbl;
  if (htbl == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  htbl_node = bf_hashtbl_get_remove(htbl, &proxy_hash);
  if (htbl_node == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  } else {
    PIPE_MGR_FREE(htbl_node);
  }
  return PIPE_SUCCESS;
}

static void pipe_mgr_exm_populate_pipe_move_list(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_entry_info_t *entry_info,
    pipe_mgr_move_list_t *pipe_move_list_node,
    struct pipe_mgr_mat_data *old_entry_data) {
  pipe_mgr_exm_stage_info_t *exm_stage_info;

  exm_stage_info = exm_tbl_data->stage_info_ptrs[entry_info->stage_id];

  /* Populate appropriate data in the pipe move list node */
  if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, entry_info->mat_ent_hdl)) {
    if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data) &&
        !exm_tbl->restore_ent_node) {
      pipe_move_list_node->op = PIPE_MAT_UPDATE_MOD;
      /* Populate the old match data */
      pipe_move_list_node->old_data = old_entry_data;
    } else {
      pipe_move_list_node->op = PIPE_MAT_UPDATE_SET_DFLT;
    }
  } else {
    pipe_move_list_node->op = PIPE_MAT_UPDATE_ADD;
    /* The entry index in the pipe move list is a global entry index */
    pipe_move_list_node->u.single.logical_idx =
        entry_info->entry_idx + exm_stage_info->stage_offset;
  }
  pipe_move_list_node->pipe = exm_tbl_data->pipe_id;
  pipe_move_list_node->data = entry_info->entry_data;
  pipe_move_list_node->entry_hdl = entry_info->mat_ent_hdl;
  if (entry_info->adt_ent_hdl_valid) {
    pipe_move_list_node->adt_ent_hdl = entry_info->adt_ent_hdl;
    pipe_move_list_node->adt_ent_hdl_valid = true;
  }
  pipe_move_list_node->logical_sel_idx = entry_info->logical_sel_idx;
  pipe_move_list_node->logical_action_idx = entry_info->logical_action_idx;
  pipe_move_list_node->selector_len = entry_info->selector_len;
}

pipe_status_t pipe_mgr_exm_update_state_for_new_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_hdl_t ent_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    cuckoo_move_list_t *move_list_node,
    pipe_mgr_move_list_t *pipe_move_list_node,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    bool populate_move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  unsigned long key = ent_hdl;
  unsigned long key1 = entry_idx + exm_stage_info->stage_offset;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_entry_info_t *htbl_data = NULL;
  uintptr_t htbl_data1 = (uintptr_t)ent_hdl;
  uintptr_t old_htbl_data1 = 0;
  struct pipe_mgr_mat_data *old_entry_data = NULL;
  uint64_t indirect_action_ptr = move_list_node->logical_action_idx;
  uint64_t indirect_selector_ptr = move_list_node->logical_sel_idx;
  uint64_t selector_len = move_list_node->selector_len;
  bool new_entry_idx = false;
  bool existing_default_entry = false;
  bool default_entry = false;
  bool isTxn = false;
  bool has_direct_stful = false;
  bool htbl_data_added = false;
  bool proxy_hash_update = false;
  bool rollback_idx_to_hdl = false;
  bool idx_to_hdl_updated = false;
  uint32_t stful_seq_nu = 0;
  struct pipe_mgr_mat_data *new_ent_data = NULL;

  isTxn = (exm_tbl_data->api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, ent_hdl)) {
    default_entry = true;
    if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
      existing_default_entry = true;
    }
  }

  /* If this is the first time the default entry is coming it will not yet be in
   * the entry_info_htbl map so go ahead and insert it now.  Otherwise the key
   * (entry handle) should already be in the map and we simply look it up. */
  if (!existing_default_entry) {
    htbl_data = (pipe_mgr_exm_entry_info_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_exm_entry_info_t));
    if (htbl_data == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    /* Insert into the hash table */
    map_sts =
        bf_map_add(&exm_tbl_data->entry_info_htbl, key, (void *)htbl_data);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in inserting entry info for entry hdl %u tbl %s 0x%x "
          "device id %d pipe %x err 0x%x",
          __func__,
          __LINE__,
          ent_hdl,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      PIPE_MGR_FREE(htbl_data);
      return PIPE_UNEXPECTED;
    }
    htbl_data_added = true;
  } else {
    map_sts =
        bf_map_get(&exm_tbl_data->entry_info_htbl, key, (void **)&htbl_data);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR("%s:%d %s for entry %u, tbl %s 0x%x, dev %d, err 0x%x",
                __func__,
                __LINE__,
                map_sts == BF_MAP_NO_KEY ? "Entry info not found"
                                         : "Error getting entry info",
                ent_hdl,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_tbl->dev_id,
                map_sts);
      PIPE_MGR_DBGCHK(0);
      return map_sts == BF_MAP_NO_KEY ? PIPE_OBJ_NOT_FOUND : PIPE_UNEXPECTED;
    }
    /* Cache the old entry data, which needs to be populated in the move list */
    old_entry_data = htbl_data->entry_data;
    has_direct_stful =
        exm_tbl->stful_tbl_refs
            ? exm_tbl->stful_tbl_refs->ref_type == PIPE_TBL_REF_TYPE_DIRECT
            : false;
    if (has_direct_stful) {
      stful_seq_nu = 1 + unpack_mat_ent_data_stful(htbl_data->entry_data);
    }
  }

  /* Allocate a new mat_entry_data object for this entry. */
  new_ent_data = make_mat_ent_data(match_spec,
                                   act_spec,
                                   act_fn_hdl,
                                   move_list_node->ttl,
                                   selector_len,
                                   stful_seq_nu,
                                   move_list_node->proxy_hash);
  if (!new_ent_data) {
    LOG_ERROR(
        "%s:%d Error allocating entry data for entry handle %u tbl %s 0x%x, "
        "device_id %d",
        __func__,
        __LINE__,
        ent_hdl,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    status = PIPE_NO_SYS_RESOURCES;
    goto err_cleanup;
  }

  /* For normal entries in a proxy hash table we need to update our hash table
   * tracking which proxy hash values are present in the stage. */
  if (!default_entry && exm_tbl->proxy_hash) {
    status = pipe_mgr_exm_proxy_hash_entry_insert(
        exm_tbl, exm_stage_info, move_list_node->proxy_hash);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in inserting proxy hash for tbl 0x%x, device id %d, "
          "pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto err_cleanup;
    }
    proxy_hash_update = true;
  }

  /* Now add a mapping from entry index to entry handle.  If there is an
   * existing mapping replace it by removing the current mapping and adding a
   * new one.
   *  */
  if (!move_list_node->next && !default_entry) {
    new_entry_idx = true;
  }
  if (existing_default_entry || !new_entry_idx) {
    map_sts = bf_map_get_rmv(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void **)&old_htbl_data1);
    if (map_sts == BF_MAP_OK) {
      rollback_idx_to_hdl = true;
    }
  }
  map_sts = bf_map_add(
      &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void *)htbl_data1);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error updating index-to-handle map, entry handle %u index %u "
        "table %s 0x%x dev %d sts %d",
        __func__,
        __LINE__,
        ent_hdl,
        (int)key1,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  idx_to_hdl_updated = true;

  /* Save state if this is a transaction. */
  if (isTxn) {
    status = pipe_mgr_exm_ent_hdl_txn_add(exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          ent_hdl,
                                          NULL,
                                          PIPE_MGR_EXM_OPERATION_ADD,
                                          key1,
                                          PIPE_MAT_ENT_INVALID_ENTRY_INDEX);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in recording entry add operation for entry %d, tbl "
          "0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto err_cleanup;
    }
  }

  if (!default_entry) {
    exm_stage_info->num_entries_placed++;
    exm_tbl_data->num_entries_placed++;
  }

  /* Update the entry data which has been saved in the entry_info_htbl map. */
  htbl_data->entry_data = new_ent_data;
  htbl_data->mat_ent_hdl = ent_hdl;
  htbl_data->pipe_id = exm_tbl_data->pipe_id;
  htbl_data->stage_id = exm_stage_info->stage_id;
  htbl_data->entry_idx = entry_idx;
  htbl_data->logical_action_idx = indirect_action_ptr;
  htbl_data->logical_sel_idx = indirect_selector_ptr;
  htbl_data->selector_len = selector_len;

  if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    htbl_data->adt_ent_hdl = act_spec->adt_ent_hdl;
    htbl_data->adt_ent_hdl_valid = true;
  }

  if (populate_move_list_node && pipe_move_list_node) {
    pipe_mgr_exm_populate_pipe_move_list(
        exm_tbl, exm_tbl_data, htbl_data, pipe_move_list_node, old_entry_data);
  }

  return PIPE_SUCCESS;
err_cleanup:

  /* Remove the index to handle mapping if it was set. */
  if (idx_to_hdl_updated) {
    bf_map_rmv(&exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1);
  }
  /* Rollback the index to handle map update if needed. */
  if (rollback_idx_to_hdl) {
    bf_map_rmv(&exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1);
    bf_map_add(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void *)old_htbl_data1);
  }
  /* Rollback the proxy hash update if it was performed. */
  if (proxy_hash_update) {
    pipe_mgr_exm_proxy_hash_entry_remove(
        exm_tbl, exm_stage_info, move_list_node->proxy_hash);
  }
  /* Free the new entry data if one was allocated. */
  if (new_ent_data) {
    free_mat_ent_data(new_ent_data);
  }
  /* Rollback the entry_info_htlb data if it was updated. */
  if (old_entry_data) {
    htbl_data->entry_data = old_entry_data;
  }
  if (htbl_data_added) {
    bf_map_rmv(&exm_tbl_data->entry_info_htbl, key);
    PIPE_MGR_FREE(htbl_data);
  }

  return status;
}

static pipe_status_t pipe_mgr_exm_update_state_for_entry_move(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_idx_t src_entry,
    pipe_mat_ent_idx_t dst_entry,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    cuckoo_move_list_t *move_list_node,
    pipe_mgr_move_list_t *pipe_move_list_node) {
  /* Update entry handle location and the mapping from entry index to entry
   * handle.
   */
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_hdl_t mat_ent_hdl = pipe_mgr_exm_get_ent_hdl_from_ent_idx(
      src_entry, exm_tbl_data, exm_stage_info);
  PIPE_MGR_DBGCHK(mat_ent_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL);
  pipe_mgr_exm_entry_info_t *htbl_data = NULL;
  uintptr_t htbl_data1 = mat_ent_hdl;
  uintptr_t old_htbl_data1 = 0;
  bool restore_htbl_data1 = false;
  unsigned long key = mat_ent_hdl;
  bool isTxn = false;

  if (!pipe_move_list_node) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  isTxn = (exm_tbl_data->api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  map_sts =
      bf_map_get(&exm_tbl_data->entry_info_htbl, key, (void **)&htbl_data);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s for hdl %u tbl %s 0x%x dev %d err 0x%x",
              __func__,
              __LINE__,
              map_sts == BF_MAP_NO_KEY ? "Entry info not found"
                                       : "Error getting entry info",
              mat_ent_hdl,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              map_sts);
    return map_sts == BF_MAP_NO_KEY ? PIPE_OBJ_NOT_FOUND : PIPE_UNEXPECTED;
  }

  /* Now, update the mapping from the dst entry index to the handle */
  /* Based on the move_list node being either the last node in the move-list
   * or the only node in the move list, we find out if the entry index will be
   * present in the entry index to entry hdl map. We use this avoid a lookup
   * into the map if it turns out the entry index is new.  */
  unsigned long key1 = dst_entry + exm_stage_info->stage_offset;
  bool new_entry_idx = false;
  if (move_list_node->next == NULL) {
    new_entry_idx = true;
  }
  if (!new_entry_idx) {
    map_sts = bf_map_get_rmv(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void **)&old_htbl_data1);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d %s (%ld) in index to handle map, tbl %s 0x%x dev %d err %d",
          __func__,
          __LINE__,
          map_sts == BF_MAP_NO_KEY ? "Entry index not found"
                                   : "Error getting entry index",
          key1,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    restore_htbl_data1 = true;
  }
  map_sts = bf_map_add(
      &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void *)htbl_data1);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in inserting entry index to entry handle mapping for "
        "index %ld hdl %d, tbl 0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        key1,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }
  if (isTxn) {
    status =
        pipe_mgr_exm_ent_hdl_txn_add(exm_tbl,
                                     exm_tbl_data,
                                     exm_stage_info,
                                     mat_ent_hdl,
                                     htbl_data,
                                     PIPE_MGR_EXM_OPERATION_MOVE,
                                     src_entry + exm_stage_info->stage_offset,
                                     dst_entry + exm_stage_info->stage_offset);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in recording entry move for entry %u, tbl 0x%x, device "
          "id %d, err %s",
          __func__,
          __LINE__,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto err_cleanup;
    }
  }
  /* Modify the location */
  htbl_data->pipe_id = exm_tbl_data->pipe_id;
  htbl_data->stage_id = exm_stage_info->stage_id;
  htbl_data->entry_idx = dst_entry;
  /* Populate pipe move list info */
  pipe_move_list_node->data = htbl_data->entry_data;
  pipe_move_list_node->entry_hdl = mat_ent_hdl;
  pipe_move_list_node->adt_ent_hdl = htbl_data->adt_ent_hdl;
  pipe_move_list_node->adt_ent_hdl_valid = htbl_data->adt_ent_hdl_valid;
  pipe_move_list_node->logical_sel_idx = htbl_data->logical_sel_idx;
  pipe_move_list_node->logical_action_idx = htbl_data->logical_action_idx;
  pipe_move_list_node->selector_len = htbl_data->selector_len;
  pipe_move_list_node->op = PIPE_MAT_UPDATE_MOV;
  pipe_move_list_node->u.single.logical_idx = key1;
  return PIPE_SUCCESS;

err_cleanup:
  bf_map_rmv(&exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1);
  if (restore_htbl_data1) {
    bf_map_add(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void *)old_htbl_data1);
  }
  return status;
}

static pipe_status_t pipe_mgr_exm_update_state_for_entry_delete(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *pipe_move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  unsigned long key = mat_ent_hdl;
  unsigned long key1 = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_entry_info_t *htbl_data = NULL;
  uintptr_t htbl_data1 = 0;
  uint64_t proxy_hash = 0;
  cuckoo_move_list_t move_list;
  struct pipe_mgr_mat_data *entry_data = NULL;
  cuckoo_move_graph_t *cuckoo_graph = NULL;
  bool default_entry = false;
  bool isTxn = false;
  bool idx_to_hdl_updated = false;
  bool proxy_hash_updated = false;

  if (!pipe_move_list_node) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  isTxn = (exm_tbl_data->api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  map_sts =
      bf_map_get_rmv(&exm_tbl_data->entry_info_htbl, key, (void **)&htbl_data);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR("%s:%d %s entry handle %u tbl %s 0x%x dev %d err 0x%x",
              __func__,
              __LINE__,
              map_sts == BF_MAP_NO_KEY ? "Entry info not found"
                                       : "Error getting entry info",
              mat_ent_hdl,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              map_sts);
    PIPE_MGR_DBGCHK(0);
    return map_sts == BF_MAP_NO_KEY ? PIPE_OBJ_NOT_FOUND : PIPE_UNEXPECTED;
  }

  key1 = htbl_data->entry_idx + exm_stage_info->stage_offset;
  proxy_hash = unpack_mat_ent_data_phash(htbl_data->entry_data);
  /* Cache the entry data to be populated in the move list */
  entry_data = htbl_data->entry_data;
  /* Create a cuckoo move-list node for a delete operation. */
  PIPE_MGR_MEMSET(&move_list, 0, sizeof(cuckoo_move_list_t));
  move_list.src_entry = htbl_data->entry_idx;
  move_list.dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;

  map_sts = bf_map_get_rmv(
      &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void **)&htbl_data1);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Entry index to entry handle mapping not found for idx %ld, "
        "handle %u, tbl %s 0x%x, device id %d, map-status %d",
        __func__,
        __LINE__,
        key1,
        mat_ent_hdl,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    status = PIPE_OBJ_NOT_FOUND;
    goto err_cleanup;
  } else if (mat_ent_hdl != (pipe_mat_ent_hdl_t)htbl_data1) {
    LOG_WARN(
        "%s:%d Table %s, 0x%x, pipe %x removing entry %u from stage %d index "
        "%d (%d) but found entry %u (0x%x)",
        __func__,
        __LINE__,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl_data->pipe_id,
        mat_ent_hdl,
        exm_stage_info->stage_id,
        htbl_data->entry_idx,
        htbl_data->entry_idx + exm_stage_info->stage_offset,
        (pipe_mat_ent_hdl_t)htbl_data1,
        (pipe_mat_ent_hdl_t)htbl_data1);
  }
  idx_to_hdl_updated = true;

  /* Populate pipe move list stuff */
  if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, mat_ent_hdl)) {
    default_entry = true;
    pipe_move_list_node->op = PIPE_MAT_UPDATE_CLR_DFLT;
  } else {
    pipe_move_list_node->op = PIPE_MAT_UPDATE_DEL;
    if (exm_tbl->proxy_hash) {
      status = pipe_mgr_exm_proxy_hash_entry_remove(
          exm_tbl, exm_stage_info, proxy_hash);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error removing proxy hash value from hash table, for entry "
            "%u tbl %s 0x%x dev %d, err %s",
            __func__,
            __LINE__,
            mat_ent_hdl,
            exm_tbl->name,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        goto err_cleanup;
      }
      proxy_hash_updated = true;
    }
  }
  pipe_move_list_node->data = entry_data;
  pipe_move_list_node->entry_hdl = mat_ent_hdl;

  if (isTxn) {
    status = pipe_mgr_exm_ent_hdl_txn_add(exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          mat_ent_hdl,
                                          htbl_data,
                                          PIPE_MGR_EXM_OPERATION_DELETE,
                                          key1,
                                          PIPE_MAT_ENT_INVALID_ENTRY_INDEX);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error recording entry delete for entry %u tbl %s 0x%x dev %d, "
          "err %s",
          __func__,
          __LINE__,
          mat_ent_hdl,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      goto err_cleanup;
    }
  }

  /* Update cuckoo placement stuff */
  if (exm_tbl->hash_action == false && !default_entry) {
    cuckoo_graph = pipe_mgr_exm_get_cuckoo_graph(
        exm_tbl, exm_tbl_data->pipe_id, exm_stage_info->stage_id);

    status = pipe_mgr_exm_transform_cuckoo_move_list(
        exm_tbl,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        cuckoo_graph,
        &move_list,
        NULL,
        PIPE_MAT_ENT_INVALID_ENTRY_INDEX);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in transforming the cuckoo move list, for"
          " exact match table entry %d, for table with handle %d",
          __func__,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl);
      return status;
    }
    cuckoo_delete_entry(cuckoo_graph, move_list.src_entry, false);
  }
  if (!default_entry) {
    if (exm_stage_info->num_entries_placed > 0) {
      exm_stage_info->num_entries_placed--;
    }
    if (exm_tbl_data->num_entries_placed > 0) {
      exm_tbl_data->num_entries_placed--;
    }
  }
  PIPE_MGR_FREE(htbl_data);
  return PIPE_SUCCESS;

err_cleanup:
  /* Restore proxy hash if required. */
  if (proxy_hash_updated) {
    pipe_mgr_exm_proxy_hash_entry_insert(exm_tbl, exm_stage_info, proxy_hash);
  }
  if (idx_to_hdl_updated) {
    /* Restore the index to handle mapping. */
    bf_map_add(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key1, (void *)htbl_data1);
  }
  /* Restore the entry in the entry-info map. */
  bf_map_add(&exm_tbl_data->entry_info_htbl, key, (void *)htbl_data);
  return status;
}

static pipe_status_t pipe_mgr_exm_update_state_for_action_modify(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mgr_exm_entry_info_t *entry_info,
    pipe_mgr_move_list_t **pipe_move_list,
    pipe_idx_t action_idx,
    pipe_idx_t selector_idx,
    uint32_t selector_len) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_move_list_t *move_list = NULL;
  bool isTxn = false;
  bool has_direct_stful = false;
  uint32_t stful_seq_nu = 0;
  int i = 0;

  isTxn = (exm_tbl_data->api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;
  move_list = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOD, exm_tbl_data->pipe_id);
  if (move_list == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* If we have a direct stateful table and we have a stateful spec specified
   * in the action spec then compute a new stateful sequence number to cause
   * the stateful table entry to be updated along with the match entry. */
  stful_seq_nu = unpack_mat_ent_data_stful(entry_info->entry_data);
  has_direct_stful =
      exm_tbl->stful_tbl_refs
          ? exm_tbl->stful_tbl_refs->ref_type == PIPE_TBL_REF_TYPE_DIRECT
          : false;
  if (has_direct_stful) {
    for (i = 0; i < act_spec->resource_count; ++i) {
      if (PIPE_GET_HDL_TYPE(act_spec->resources[i].tbl_hdl) ==
              PIPE_HDL_TYPE_STFUL_TBL &&
          act_spec->resources[i].tag != PIPE_RES_ACTION_TAG_NO_CHANGE) {
        stful_seq_nu++;
        break;
      }
    }
  }
  if (isTxn) {
    status = pipe_mgr_exm_ent_hdl_txn_add(exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          mat_ent_hdl,
                                          entry_info,
                                          PIPE_MGR_EXM_OPERATION_MODIFY,
                                          PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                          PIPE_MAT_ENT_INVALID_ENTRY_INDEX);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in recording entry modify for entry %d, tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }

  if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    move_list->adt_ent_hdl = act_spec->adt_ent_hdl;
    move_list->adt_ent_hdl_valid = true;
    entry_info->adt_ent_hdl = act_spec->adt_ent_hdl;
    entry_info->adt_ent_hdl_valid = true;
  }
  move_list->op = PIPE_MAT_UPDATE_MOD;
  move_list->entry_hdl = mat_ent_hdl;
  move_list->logical_sel_idx = selector_idx;
  move_list->selector_len = selector_len;
  move_list->logical_action_idx = action_idx;
  /* Update the entry info */
  entry_info->logical_action_idx = action_idx;
  entry_info->logical_sel_idx = selector_idx;
  /* Cache the old entry data in the move-list */
  move_list->old_data = entry_info->entry_data;
  /* Make new entry data */
  pipe_tbl_match_spec_t *match_spec =
      unpack_mat_ent_data_ms(entry_info->entry_data);
  move_list->data = entry_info->entry_data =
      make_mat_ent_data(match_spec,
                        act_spec,
                        act_fn_hdl,
                        0,
                        selector_len,
                        stful_seq_nu,
                        unpack_mat_ent_data_phash(entry_info->entry_data));
  if (move_list->data == NULL) {
    LOG_ERROR(
        "%s:%d Error in allocating match entry data for entry handle %d, tbl "
        "0x%x, device id %d",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  *pipe_move_list = move_list;
  entry_info->logical_action_idx = action_idx;
  entry_info->logical_sel_idx = selector_idx;
  entry_info->selector_len = selector_len;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_update_state_for_set_resource(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_mgr_act_spec_delta_t *act_spec_delta,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_action_spec_t act_spec;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_move_list_t *move_list = NULL;
  pipe_res_spec_t *stats_res_spec = NULL;
  pipe_res_spec_t *meter_res_spec = NULL;
  pipe_res_spec_t *stful_res_spec = NULL;
  uint32_t stful_seq_nu = 0;

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, mat_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  move_list = alloc_move_list(NULL, PIPE_MAT_UPDATE_MOD, entry_info->pipe_id);
  if (move_list == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMSET(&act_spec, 0, sizeof(pipe_action_spec_t));
  /* First copy the current action spec in whole */
  PIPE_MGR_MEMCPY(&act_spec,
                  unpack_mat_ent_data_as(entry_info->entry_data),
                  sizeof(pipe_action_spec_t));
  act_spec.act_data.action_data_bits = (uint8_t *)PIPE_MGR_CALLOC(
      act_spec.act_data.num_action_data_bytes, sizeof(uint8_t));
  if (act_spec.act_data.action_data_bits == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(
      act_spec.act_data.action_data_bits,
      unpack_mat_ent_data_as(entry_info->entry_data)->act_data.action_data_bits,
      act_spec.act_data.num_action_data_bytes);
  /* Now fold in the delta */
  if (act_spec_delta->stats_operate) {
    /* Iterate through the current resources, replace stats if it exists, else
     * add
     */
    int i = 0;
    bool stats_found = false;
    for (i = 0; i < act_spec.resource_count; i++) {
      if (PIPE_GET_HDL_TYPE(act_spec.resources[i].tbl_hdl) ==
          PIPE_HDL_TYPE_STAT_TBL) {
        stats_found = true;
        stats_res_spec = &act_spec.resources[i];
        break;
      }
    }
    if (!stats_found) {
      stats_res_spec = &act_spec.resources[act_spec.resource_count];
      act_spec.resource_count++;
    }
    PIPE_MGR_MEMCPY(stats_res_spec,
                    &act_spec_delta->stats_op.stat_spec,
                    sizeof(pipe_res_spec_t));
  }
  if (act_spec_delta->meters_operate) {
    int i = 0;
    bool meters_found = false;
    for (i = 0; i < act_spec.resource_count; i++) {
      if (PIPE_GET_HDL_TYPE(act_spec.resources[i].tbl_hdl) ==
          PIPE_HDL_TYPE_METER_TBL) {
        meters_found = true;
        meter_res_spec = &act_spec.resources[i];
        break;
      }
    }
    if (!meters_found) {
      meter_res_spec = &act_spec.resources[act_spec.resource_count];
      act_spec.resource_count++;
    }
    PIPE_MGR_MEMCPY(meter_res_spec,
                    &act_spec_delta->meters_op.meter_res,
                    sizeof(pipe_res_spec_t));
  }
  if (act_spec_delta->stful_operate) {
    int i = 0;
    bool stful_found = false;
    bool has_direct_stful = false;
    for (i = 0; i < act_spec.resource_count; i++) {
      if (PIPE_GET_HDL_TYPE(act_spec.resources[i].tbl_hdl) ==
          PIPE_HDL_TYPE_STFUL_TBL) {
        stful_found = true;
        stful_res_spec = &act_spec.resources[i];
        break;
      }
    }
    if (!stful_found) {
      stful_res_spec = &act_spec.resources[act_spec.resource_count];
      act_spec.resource_count++;
    }
    PIPE_MGR_MEMCPY(stful_res_spec,
                    &act_spec_delta->stful_op.stful_res,
                    sizeof(pipe_res_spec_t));
    has_direct_stful =
        exm_tbl->stful_tbl_refs
            ? exm_tbl->stful_tbl_refs->ref_type == PIPE_TBL_REF_TYPE_DIRECT
            : false;
    if (has_direct_stful) {
      stful_seq_nu = 1 + unpack_mat_ent_data_stful(entry_info->entry_data);
    }
  }
  /* Now, the action spec is correctly formed */
  /* Cache the old entry data to be populated in the pipe move list before
   * allocating a new one
   */
  move_list->old_data = entry_info->entry_data;
  move_list->data = entry_info->entry_data =
      make_mat_ent_data(unpack_mat_ent_data_ms(entry_info->entry_data),
                        &act_spec,
                        unpack_mat_ent_data_afun_hdl(entry_info->entry_data),
                        unpack_mat_ent_data_ttl(entry_info->entry_data),
                        unpack_mat_ent_data_sel(entry_info->entry_data),
                        stful_seq_nu,
                        unpack_mat_ent_data_phash(entry_info->entry_data));
  if (move_list->data == NULL) {
    LOG_ERROR(
        "%s:%d Error in allocating match entry data for entry handle %d, tbl "
        "0x%x device id %d",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    if (act_spec.act_data.action_data_bits) {
      PIPE_MGR_FREE(act_spec.act_data.action_data_bits);
    }
    return PIPE_UNEXPECTED;
  }
  /* Populate required info into the pipe move list */
  move_list->op = PIPE_MAT_UPDATE_MOD;
  move_list->entry_hdl = mat_ent_hdl;
  move_list->adt_ent_hdl = entry_info->adt_ent_hdl;
  move_list->adt_ent_hdl_valid = entry_info->adt_ent_hdl_valid;
  move_list->logical_sel_idx = entry_info->logical_sel_idx;
  move_list->selector_len = entry_info->selector_len;
  move_list->logical_action_idx = entry_info->logical_action_idx;

  *pipe_move_list = move_list;

  if (act_spec.act_data.action_data_bits) {
    PIPE_MGR_FREE(act_spec.act_data.action_data_bits);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_verify_indices(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_action_spec_t *action_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  int i = 0;
  pipe_hdl_type_t hdl_type;
  pipe_tbl_hdl_t res_hdl = 0;
  pipe_idx_t res_idx = 0;
  for (i = 0; i < action_spec->resource_count; i++) {
    res_hdl = action_spec->resources[i].tbl_hdl;
    res_idx = action_spec->resources[i].tbl_idx;
    hdl_type = PIPE_GET_HDL_TYPE(res_hdl);
    switch (hdl_type) {
      case PIPE_HDL_TYPE_STAT_TBL:
        if (exm_tbl->num_stat_tbl_refs > 0 &&
            exm_tbl->stat_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
          if (exm_tbl->stat_tbl_refs[0].tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid stats tbl hdl 0x%x for exm tbl 0x%x device %d, "
                "which expects stats tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                exm_tbl->mat_tbl_hdl,
                exm_tbl->dev_id,
                exm_tbl->stat_tbl_refs[0].tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_stat_mgr_verify_idx(
                exm_tbl->dev_id, exm_tbl_data->pipe_id, res_hdl, res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying stats idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "stat tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  exm_tbl->mat_tbl_hdl,
                  exm_tbl->dev_id,
                  exm_tbl_data->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      case PIPE_HDL_TYPE_METER_TBL:
        if (exm_tbl->num_meter_tbl_refs > 0 &&
            exm_tbl->meter_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
          if (exm_tbl->meter_tbl_refs[0].tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid meter tbl hdl 0x%x for exm tbl 0x%x device %d, "
                "which expects meter tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                exm_tbl->mat_tbl_hdl,
                exm_tbl->dev_id,
                exm_tbl->meter_tbl_refs[0].tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_meter_verify_idx(
                exm_tbl->dev_id, exm_tbl_data->pipe_id, res_hdl, res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying meter idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "meter tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  exm_tbl->mat_tbl_hdl,
                  exm_tbl->dev_id,
                  exm_tbl_data->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      case PIPE_HDL_TYPE_STFUL_TBL:
        if (exm_tbl->num_stful_tbl_refs > 0 &&
            exm_tbl->stful_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
          if (exm_tbl->stful_tbl_refs[0].tbl_hdl != res_hdl) {
            LOG_ERROR(
                "%s:%d Invalid stful tbl hdl 0x%x for exm tbl 0x%x device %d, "
                "which expects stful tbl hdl 0x%x",
                __func__,
                __LINE__,
                res_hdl,
                exm_tbl->mat_tbl_hdl,
                exm_tbl->dev_id,
                exm_tbl->stful_tbl_refs[0].tbl_hdl);
            PIPE_MGR_DBGCHK(0);
            return PIPE_INVALID_ARG;
          }
          if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
            status = pipe_mgr_stful_verify_idx(
                exm_tbl->dev_id, exm_tbl_data->pipe_id, res_hdl, res_idx);
            if (status != PIPE_SUCCESS) {
              LOG_ERROR(
                  "%s:%d Error in verifying stful idx %d for "
                  "match tbl 0x%x, device id %d, pipe id %d, "
                  "stful tbl 0x%x, err %s",
                  __func__,
                  __LINE__,
                  res_idx,
                  exm_tbl->mat_tbl_hdl,
                  exm_tbl->dev_id,
                  exm_tbl_data->pipe_id,
                  res_hdl,
                  pipe_str_err(status));
              return status;
            }
          }
        }
        break;
      default:
        LOG_ERROR(
            "%s:%d Invalid resource handle type for resource tbl 0x%x "
            "for exm tbl 0x%x device id %d",
            __func__,
            __LINE__,
            res_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id);
        return PIPE_INVALID_ARG;
    }
  }
  return status;
}

static pipe_status_t pipe_mgr_exm_alloc_indirect_resources(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_action_spec_t *action_spec,
    pipe_idx_t *action_idx,
    pipe_idx_t *selector_idx,
    pipe_idx_t *selector_len) {
  pipe_sess_hdl_t sess_hdl = 0;
  pipe_status_t status = PIPE_SUCCESS;
  *action_idx = PIPE_MGR_LOGICAL_ACT_IDX_INVALID;
  *selector_idx = 0;
  *selector_len = 0;

  /* Verify resource indices */
  status = pipe_mgr_exm_verify_indices(exm_tbl, exm_tbl_data, action_spec);
  if (PIPE_SUCCESS != status) {
    return status;
  }

  if (exm_tbl->num_sel_tbl_refs != 0 && IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    pipe_sel_tbl_hdl_t sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;
    status = rmt_sel_grp_activate_stage(sess_hdl,
                                        exm_tbl->dev_id,
                                        exm_tbl_data->pipe_id,
                                        exm_tbl->mat_tbl_hdl,
                                        mat_ent_hdl,
                                        sel_tbl_hdl,
                                        action_spec->sel_grp_hdl,
                                        exm_stage_info->stage_id,
                                        (rmt_virt_addr_t *)action_idx,
                                        selector_len,
                                        selector_idx,
                                        exm_tbl_data->api_flags);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in allocating selector group for grp handle %d, "
          "selector table 0x%x, for match table 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          action_spec->sel_grp_hdl,
          sel_tbl_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  } else if (exm_tbl->num_adt_refs != 0 &&
             IS_ACTION_SPEC_ACT_DATA_HDL(action_spec)) {
    pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
    if (exm_tbl->adt_tbl_refs[0].ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
      PIPE_MGR_DBGCHK(0);
      return (PIPE_INVALID_ARG);
    }
    status = rmt_adt_ent_place(exm_tbl->dev_id,
                               exm_tbl_data->pipe_id,
                               adt_tbl_hdl,
                               action_spec->adt_ent_hdl,
                               exm_stage_info->stage_id,
                               action_idx,
                               exm_tbl_data->api_flags);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in placing action entry handle %d for action tbl "
          "0x%x, for entry in match table 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          action_spec->adt_ent_hdl,
          adt_tbl_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_dealloc_indirect_resources(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_entry_info_t *entry_info) {
  pipe_sess_hdl_t sess_hdl = 0;
  pipe_status_t status = PIPE_SUCCESS;
  pipe_action_spec_t *act_spec = unpack_mat_ent_data_as(entry_info->entry_data);
  if (exm_tbl->num_sel_tbl_refs != 0 && IS_ACTION_SPEC_SEL_GRP(act_spec)) {
    pipe_sel_tbl_hdl_t sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;
    pipe_sel_grp_hdl_t sel_grp_hdl = act_spec->sel_grp_hdl;
    status = rmt_sel_grp_deactivate_stage(sess_hdl,
                                          exm_tbl->dev_id,
                                          entry_info->pipe_id,
                                          exm_tbl->mat_tbl_hdl,
                                          entry_info->mat_ent_hdl,
                                          sel_tbl_hdl,
                                          sel_grp_hdl,
                                          entry_info->stage_id,
                                          exm_tbl_data->api_flags);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in deactivating sel grp %d, for entry %d, tbl 0x%x, "
          "device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          sel_grp_hdl,
          entry_info->mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          entry_info->pipe_id,
          entry_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  } else if (exm_tbl->num_adt_refs != 0 &&
             IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    if (exm_tbl->adt_tbl_refs[0].ref_type != PIPE_TBL_REF_TYPE_INDIRECT) {
      PIPE_MGR_DBGCHK(0);
      return (PIPE_INVALID_ARG);
    }
    pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
    pipe_adt_ent_hdl_t adt_ent_hdl = act_spec->adt_ent_hdl;
    status = rmt_adt_ent_remove(exm_tbl->dev_id,
                                adt_tbl_hdl,
                                adt_ent_hdl,
                                entry_info->stage_id,
                                exm_tbl_data->api_flags);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Dev %d Pipe %x Tbl %s (0x%x) entry %d (0x%x) stage %d, error "
          "%s removing ADT mbr %d (0x%x) from ADT 0x%x",
          __func__,
          __LINE__,
          exm_tbl->dev_id,
          entry_info->pipe_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          entry_info->mat_ent_hdl,
          entry_info->mat_ent_hdl,
          entry_info->stage_id,
          pipe_str_err(status),
          adt_ent_hdl,
          adt_ent_hdl,
          adt_tbl_hdl);
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_exm_ent_update_state:
 *         Helper routine used to inflict changes in the state maintained by
 *         by the exact match table manager for managing entries.
 *
 *
 * \param mat_tbl_hdl Handle associated with the match entry table.
 * \param match_spec Pointer to the match spec associated with the entry.
 * \param act_data_spec Pointer to the action spec associated with the entry.
 * \param move_list A pointer to the list of moves that needs to be done in
 *        order to accommodate the new entry.
 * \return pipe_status_t Status of this operation.
 */
static pipe_status_t pipe_mgr_exm_update_state(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_data_spec,
    cuckoo_move_list_t *move_list,
    pipe_mgr_move_list_t **pipe_move_list,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  cuckoo_move_list_t *traverser = NULL;
  pipe_mat_ent_idx_t src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  pipe_mat_ent_idx_t dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  pipe_mgr_move_list_t *pipe_move_list_node = NULL;

  /* Consume the move_list and inflict the state changes. The move list is
   * consumed in reverse order, since the last entry of the move list will
   * be an entry moving to an empty entry.
   */

  traverser = move_list;

  while (traverser->next) {
    traverser = traverser->next;
  }
  /* Iterate through each of the move_list nodes */
  while (traverser) {
    if (pipe_move_list) {
      if (*pipe_move_list == NULL) {
        *pipe_move_list =
            alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, exm_tbl_data->pipe_id);
        if (*pipe_move_list == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
        pipe_move_list_node = *pipe_move_list;
      } else {
        pipe_move_list_node = alloc_move_list(
            pipe_move_list_node, PIPE_MAT_UPDATE_ADD, exm_tbl_data->pipe_id);
        if (pipe_move_list_node == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
      }
    }
    src_entry = traverser->src_entry;
    dst_entry = traverser->dst_entry;

    if (src_entry == PIPE_MAT_ENT_INVALID_ENTRY_INDEX &&
        dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      /* A new entry getting programmed into the location at
       * dst_entry.
       */
      status = pipe_mgr_exm_update_state_for_new_entry(exm_tbl,
                                                       mat_ent_hdl,
                                                       match_spec,
                                                       act_data_spec,
                                                       act_fn_hdl,
                                                       traverser,
                                                       pipe_move_list_node,
                                                       dst_entry,
                                                       exm_tbl_data,
                                                       exm_stage_info,
                                                       true);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in updating state for new entry add for "
            "exact match table with handle %d, for pipe_id %d"
            " in stage id %d, error %s",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      traverser->mat_ent_hdl = mat_ent_hdl;
    } else if (src_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX &&
               dst_entry != PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      /* Entry move from src_entry to dst_entry */
      status = pipe_mgr_exm_update_state_for_entry_move(exm_tbl,
                                                        src_entry,
                                                        dst_entry,
                                                        exm_tbl_data,
                                                        exm_stage_info,
                                                        traverser,
                                                        pipe_move_list_node);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in updating state for entry move for "
            " exact match table with handle %d, for pipe_id %d"
            " in stage id %d, error %s",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
    } else if (src_entry == PIPE_MAT_ENT_INVALID_ENTRY_INDEX &&
               dst_entry == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
      /* Entry delete */
      status = pipe_mgr_exm_update_state_for_entry_delete(exm_tbl,
                                                          mat_ent_hdl,
                                                          exm_tbl_data,
                                                          exm_stage_info,
                                                          pipe_move_list_node);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in updating state for entry delete for "
            " exact match table with handle %d, for pipe id %d"
            " in stage id %d, error %s",
            __func__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      traverser->mat_ent_hdl = mat_ent_hdl;
    }

    traverser = traverser->previous;
  }

  return PIPE_SUCCESS;
}

static pipe_mat_ent_idx_t pipe_mgr_exm_get_entry_loc(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *pipe_id,
    dev_stage_t *stage_id) {
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;

  entry_info = pipe_mgr_exm_get_phy_entry_info(exm_tbl, mat_ent_hdl);
  if (entry_info == NULL) return PIPE_MAT_ENT_INVALID_ENTRY_INDEX;

  *pipe_id = entry_info->pipe_id;
  *stage_id = entry_info->stage_id;
  return entry_info->entry_idx;
}

static inline pipe_mat_ent_hdl_t pipe_mgr_exm_get_def_ent_hdl(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data != NULL) {
    return exm_tbl_data->default_entry_hdl;
  }
  return PIPE_MAT_ENT_HDL_INVALID_HDL;
}

pipe_mat_ent_idx_t pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx) {
  /* These two table locations are reserved numbers which do not have a
   * corresponding direct-resource index. */
  PIPE_MGR_DBGCHK(entry_idx != PIPE_MGR_EXM_UNUSED_ENTRY_IDX);
  PIPE_MGR_DBGCHK(entry_idx != PIPE_MGR_EXM_UNUSED_STASH_ENTRY_IDX);
  /* Based on the packing format the entry index will get translated
   * appropriately. This is for direct addressing only. The address
   * generated will factor in the per-entry VPN.
   */

  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  uint32_t hashway = 0;
  uint32_t ram_line_num = 0;
  uint32_t wide_word_blk_idx = 0;
  uint32_t hash_way_idx = 0;
  uint32_t subword_idx = 0;
  uint32_t num_entries_in_wide_word_blk = 0;
  uint32_t num_ram_line_bits = 0;
  uint32_t offset = 0;

  num_ram_line_bits = log2(TOF_UNIT_RAM_DEPTH(exm_tbl));

  /* The direct address that gets generated is composed of two components
   * the ram line number and the entry VPN, which is then shifted by
   * appropriate amounts as per the packing of the action data table to
   * address the equivalent logical entry index.
   *
   * The logical entry index of the exact match RAM is not the logical entry
   * index of the action data table because of the fact that there can be
   * multiple entries packed in the exact match table and the sub-entry
   * number should thus be factored in the direct address generated. A VPN
   * will be allocated for each sub-entry of the exact match table in a way
   */
  if (exm_tbl->hash_action == false) {
    exm_pack_format = exm_stage_info->pack_format;

    hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, entry_idx);

    num_entries_in_wide_word_blk = exm_pack_format->num_entries_per_wide_word *
                                   TOF_UNIT_RAM_DEPTH(exm_tbl);

    /* The logical entry index will be scaled up by the pack format. Based on
     * the direct action data entry addressing scheme, the logical entry index
     * of the match table has to be decomposed into the the ram line number and
     * scaled up by the entry VPN of the match entry. The entry VPN resides in
     * the MSB of the direct address generated from the match entry. Hence,
     * a scale down of the logical entry index by the pack format is necessary
     * to get the ram_line_num. Masking is required since the logical entry
     * index will have the ram-select bits in the MSB.
     */
    ram_line_num = (entry_idx / exm_pack_format->num_entries_per_wide_word) &
                   ((1 << num_ram_line_bits) - 1);

    /* The entry index which is passed here is the global entry index in this
     * stage. Calculate the relative entry index in the hashway in which it
     * is present for any calculations which are relevant within the hashway.
     */

    for (hash_way_idx = 0;
         hash_way_idx < exm_stage_info->num_hash_ways && hash_way_idx < hashway;
         hash_way_idx++) {
      exm_hashway_data = &exm_stage_info->hashway_data[hash_way_idx];
      if (exm_hashway_data) {
        offset += exm_hashway_data->num_entries;
        entry_idx -= exm_hashway_data->num_entries;
      }
    }

    wide_word_blk_idx = entry_idx / num_entries_in_wide_word_blk;

    subword_idx = entry_idx % exm_pack_format->num_entries_per_wide_word;

    /* The entry VPN is populated per tbl_word_blk of the hashway. There are
     * as many VPNs as there are number of entries packed in a wide-word. One
     * per sub-entry of a line.
     */

    /* The VPN assignment can be arbitrary based on various constraints. For
     * calculating the logical entry index of the directly addressed table, need
     * to scale up the ram_line_num by the subword. For each subword jump of
     * the exact match entry, there is a jump of 1k (TOF_SRAM_UNIT_DEPTH) in
     * the directly addressed memory.
     */

    return (((subword_idx << num_ram_line_bits) | ram_line_num) +
            (wide_word_blk_idx * num_entries_in_wide_word_blk) + offset);
  } else {
    return entry_idx;
  }
}

static pipe_status_t pipe_mgr_exm_get_ram_shadow_copy(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    uint8_t **shadow_ptrs) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t hashway = 0;
  uint8_t num_rams_in_wide_word = 0;
  uint8_t wide_word_blk_idx = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t accum = 0;
  mem_id_t mem_id = 0;
  uint32_t ram_line_num = 0;
  uint32_t num_entries_per_wide_word = 0;
  pipe_mem_type_t mem_type;
  PIPE_MGR_DBGCHK(exm_tbl != NULL);
  rmt_dev_info_t *dev_info = exm_tbl->dev_info;
  PIPE_MGR_DBGCHK(dev_info != NULL);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      mem_type = pipe_mem_type_unit_ram;
      break;







    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  bf_dev_pipe_t pipe_id;
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }
  pipe_mat_ent_idx_t hashway_ent_idx = 0;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;

  hashway = pipe_mgr_exm_get_entry_hashway(exm_tbl_stage_info, entry_idx);

  exm_hashway_data = &exm_tbl_stage_info->hashway_data[hashway];
  ram_alloc_info = exm_hashway_data->ram_alloc_info;

  exm_pack_format = exm_tbl_stage_info->pack_format;

  num_rams_in_wide_word = exm_pack_format->num_rams_in_wide_word;
  num_entries_per_wide_word = exm_pack_format->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_UNIT_RAM_DEPTH(exm_tbl);
  unsigned i = 0;

  accum = exm_tbl_stage_info->hashway_data[hashway].offset;
  hashway_ent_idx = entry_idx - accum;
  wide_word_blk_idx = hashway_ent_idx / num_entries_per_wide_word_blk;
  ram_line_num = (hashway_ent_idx / num_entries_per_wide_word) %
                 TOF_UNIT_RAM_DEPTH(exm_tbl);
  if (exm_tbl_data->pipe_id == BF_DEV_PIPE_ALL) {
    pipe_id = exm_tbl->lowest_pipe_id;
  } else {
    pipe_id = exm_tbl_data->pipe_id;
  }

  for (i = 0; i < num_rams_in_wide_word; i++) {
    mem_id = ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id[i];
    status = pipe_mgr_phy_mem_map_get_ref(exm_tbl->dev_id,
                                          exm_tbl->direction,
                                          mem_type,
                                          pipe_id,
                                          exm_tbl_stage_info->stage_id,
                                          mem_id,
                                          ram_line_num,
                                          &shadow_ptrs[i],
                                          false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting shadow memory ref for mem id %d, stage id %d"
          " error %s",
          __func__,
          __LINE__,
          mem_id,
          exm_tbl_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_compute_ram_shadow_copy_vv(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    uint8_t **shadow_ptr_arr,
    uint8_t *wide_word_index,
    mem_id_t *mem_id_arr,
    bool valid,
    bool is_stash,
    uint32_t *ret_version_valid_bits) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t version_valid_bits = 0;
  uint8_t entry_position = 0;
  uint8_t hashway = 0;
  pipe_mat_ent_idx_t accum = 0;
  bool ram_words_updated[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD];
  uint32_t num_mem_ids = 0;
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }
  PIPE_MGR_MEMSET(ram_words_updated, 0, sizeof(ram_words_updated));
  if (!valid) {
    RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(RMT_EXM_ENTRY_VERSION_INVALID,
                                         RMT_EXM_ENTRY_VERSION_INVALID,
                                         version_valid_bits);
  } else {
    RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                         RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                         version_valid_bits);
  }
  *ret_version_valid_bits = version_valid_bits;

  if (is_stash) {
    entry_position =
        stage_ent_idx %
        exm_stage_info->stash->pack_format.num_entries_per_wide_word;
  } else {
    entry_position =
        stage_ent_idx % exm_stage_info->pack_format->num_entries_per_wide_word;
  }

  status = pipe_mgr_entry_format_tof_exm_tbl_ent_set_vv(
      exm_tbl->dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_table_handle,
      entry_position,
      version_valid_bits,
      shadow_ptr_arr,
      ram_words_updated,
      is_stash);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error when encoding the entry contents for entry delete"
        " for exact match table with handle %d, stage id %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id);
    return status;
  }

  pipe_mgr_exm_get_mem_ids_for_entry(exm_tbl,
                                     exm_stage_info,
                                     stage_ent_idx,
                                     mem_id_arr,
                                     ram_words_updated,
                                     &num_mem_ids);

  *wide_word_index = 0;
  unsigned i = 0;
  unsigned j = 0;
  for (i = 0; i < exm_stage_info->pack_format->num_rams_in_wide_word; i++) {
    if (ram_words_updated[i] == false) {
      continue;
    }
    *wide_word_index = i;
    /* For just modifying the version/valid bits of the entry only one
     * word in the wide-word would be modified.
     */
    break;
  }
  rmt_dev_cfg_t *cfg = &exm_tbl->dev_info->dev_cfg;
  PIPE_MGR_DBGCHK(*wide_word_index < cfg->stage_cfg.num_sram_rows);

  hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, stage_ent_idx);

  for (i = 0; i < exm_stage_info->num_hash_ways; i++) {
    if (hashway == i) {
      break;
    }
    accum += exm_stage_info->hashway_data[i].num_entries;
  }

  /* Log the Encoded entry words */
  for (i = 0; i < exm_stage_info->pack_format->num_rams_in_wide_word; i++) {
    if (ram_words_updated[i] == false) {
      continue;
    }

    char buf[3 * (TOF_SRAM_UNIT_WIDTH / 8)];
    for (j = 0; j < TOF_SRAM_UNIT_WIDTH / 8; j++) {
      int n1 = *(shadow_ptr_arr[i] + j) >> 4; /* Top nibble */
      int n0 = *(shadow_ptr_arr[i] + j) & 15; /* Bottom nibble */
      buf[3 * j + 0] = (n1 < 10 ? '0' : 'A' - 10) + n1;
      buf[3 * j + 1] = (n0 < 10 ? '0' : 'A' - 10) + n0;
      buf[3 * j + 2] = ' ';
    }
    buf[sizeof buf - 1] = '\0';
    LOG_DBG("Word %d: %s", i, buf);
    break;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_ent_place_with_hdl_internal(
    dev_target_t dev_tgt,
    void *exm_tbl_p,
    void *exm_tbl_data_p,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *ms,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t new_ent_hdl,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = exm_tbl_p;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = exm_tbl_data_p;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_exm_hash_t *hash_container = NULL;
  cuckoo_move_list_t *move_list = NULL;
  cuckoo_move_list_t movelist;
  bool isTxn = false;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t num_hashes = 0;
  uint64_t proxy_hash = 0;
  pipe_tbl_match_spec_t *dkm_match_spec = NULL;
  pipe_tbl_match_spec_t *match_spec = NULL;
  pipe_tbl_match_spec_t exm_matchspec;
  uint8_t match_value_bits[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD *
                           TOF_BYTES_IN_RAM_WORD];

  cuckoo_move_graph_t *cuckoo_graph = NULL;

  pipe_mgr_exm_edge_container_t *edge_container = NULL;
  /*Subword location is new from compiler and hashing module will give this
   * info*/
  uint32_t ent_subword_loc = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  bool edge_updated = false;
  LOG_TRACE("Entering %s", __func__);

  match_spec = ms;

  PIPE_MGR_MEMSET(&movelist, 0, sizeof(cuckoo_move_list_t));

  /* Always expect a valid match-spec and action spec */

  if (match_spec->num_valid_match_bits == 0) {
    LOG_ERROR("%s:%d No match spec passed for tbl %d, device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  isTxn = (pipe_api_flags & PIPE_MGR_TBL_API_TXN) ? true : false;

  if (!exm_tbl) {
    exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);

    if (exm_tbl == NULL) {
      LOG_ERROR(
          "%s : Could not find the exact match table info for table with"
          " handle %d for device id %d",
          __func__,
          mat_tbl_hdl,
          dev_tgt.device_id);
      status = PIPE_OBJ_NOT_FOUND;
      goto err_cleanup;
    }
  }

  LOG_DBG("%s Request to install EXM entry tbl 0x%x dev %d pipe %x match spec",
          exm_tbl->name,
          mat_tbl_hdl,
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id);
  pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                       BF_LOG_DBG,
                                       exm_tbl->profile_id,
                                       mat_tbl_hdl,
                                       match_spec);
  pipe_mgr_entry_format_log_action_spec(dev_tgt.device_id,
                                        BF_LOG_DBG,
                                        exm_tbl->profile_id,
                                        act_data_spec,
                                        act_fn_hdl);

  if (exm_tbl->symmetric == true) {
    pipe_id = BF_DEV_PIPE_ALL;
  } else if (dev_tgt.dev_pipe_id == DEV_PIPE_ALL) {
    pipe_id = BF_DEV_PIPE_ALL;
  } else {
    pipe_id = dev_tgt.dev_pipe_id;
  }

  if (!exm_tbl_data) {
    if (exm_tbl->symmetric) {
      if (dev_tgt.dev_pipe_id != DEV_PIPE_ALL) {
        LOG_ERROR(
            "%s:%d Incorrect pipe id %d passed for exm tbl 0x%x, device id %d",
            __func__,
            __LINE__,
            dev_tgt.dev_pipe_id,
            exm_tbl->mat_tbl_hdl,
            dev_tgt.device_id);
        return PIPE_INVALID_ARG;
      }
    }
    if (pipe_id == BF_DEV_PIPE_ALL) {
      exm_tbl_data = &exm_tbl->exm_tbl_data[0];
    } else {
      exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
    }
    if (exm_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
          "not found",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    }
    if (pipe_mgr_exm_is_tbl_full(exm_tbl, exm_tbl_data)) {
      LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d, pipe id %d is full",
                __func__,
                __LINE__,
                exm_tbl->mat_tbl_hdl,
                dev_tgt.device_id,
                pipe_id);
      return PIPE_NO_SPACE;
    }
  }
  exm_tbl_data->api_flags = pipe_api_flags;

  /* Since we are dealing with an exact match table, the mask in the match
   * spec is completely ignored. However if the EXM table can be applied with
   * match key mask at run time, then match key mask maintained per exm table,
   * has to be applied on match-spec value bits before placing the entry.
   */
  pipe_mat_tbl_info_t *mat_tbl_info = exm_tbl->mat_tbl_info;
  exm_matchspec.match_value_bits = match_value_bits;
  dkm_match_spec = &exm_matchspec;
  if ((pipe_mgr_get_exm_key_with_dkm_mask(
          mat_tbl_info, match_spec, dkm_match_spec)) == PIPE_SUCCESS) {
    // EXM table has dynamic key mask property. Change key to include bits as
    // per key-mask.
    match_spec = dkm_match_spec;
  }

  if (exm_tbl->restore_ent_node) {
    /* State restore case. Construct the edge container and populate it with
     * the entry index stored in the logfile.
     */
    pipe_mat_ent_idx_t ent_idx =
        cJSON_GetObjectItem(exm_tbl->restore_ent_node, "entry_idx")
            ->valuedouble;
    stage_id = pipe_mgr_exm_get_stage_id_from_idx(exm_tbl_data, ent_idx);
    exm_stage_info = exm_tbl_data->stage_info_ptrs[stage_id];

    edge_container = (pipe_mgr_exm_edge_container_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_exm_edge_container_t));
    if (edge_container == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    edge_container->entries =
        (pipe_mat_ent_idx_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mat_ent_idx_t));
    if (edge_container->entries == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_FREE(edge_container);
      return PIPE_NO_SYS_RESOURCES;
    }
    edge_container->entries[0] = ent_idx - exm_stage_info->stage_offset;
    edge_container->num_entries = 1;
  } else {
    /* First, figure out the stage-id in which the entry should be placed in.
     * This is based on a bunch of heuristics at this point, one being occupancy
     * level. Until a certain level of occupancy, a stage is picked, beyond
     * that, the least occupied stage will be picked.
     */

    stage_id = pipe_mgr_exm_get_stage_for_new_entry(exm_tbl, pipe_id);

    /* Invoke the hash computation block, which takes in the match spec and
     * the table handle, computes the "required number of hashes.
     * This is invoked even for direct-indexed tables.
     */

    /* Allocating memory sufficient to hold TWO 64 bit hashes. It is deemed
     * sufficient at this point.
     */

    hash_container = PIPE_MGR_CALLOC(2, sizeof(pipe_exm_hash_t));

    status = pipe_mgr_exm_hash_compute(dev_tgt.device_id,
                                       exm_tbl->profile_id,
                                       mat_tbl_hdl,
                                       match_spec,
                                       stage_id,
                                       hash_container,
                                       &num_hashes);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in computing the hashes for a new entry exact"
          " match hash table add for table with handle %d",
          __func__,
          mat_tbl_hdl);
      goto err_cleanup;
    }

    /* Expand the computed hash to a set of logical entry indices in the stage
     * in which the entry will be placed.
     */
    edge_container = pipe_mgr_exm_expand_to_logical_entries(exm_tbl,
                                                            pipe_id,
                                                            stage_id,
                                                            hash_container,
                                                            num_hashes,
                                                            &ent_subword_loc);

    if (!edge_container) {
      LOG_ERROR(
          "%s:%d edge_container for tbl 0x%x, device id %d, pipe id %d, "
          "stage_id %d not found",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_id,
          stage_id);
      PIPE_MGR_DBGCHK(0);
      status = PIPE_OBJ_NOT_FOUND;
      goto err_cleanup;
    }

    if (pipe_mgr_exm_entry_exists(
            exm_tbl, pipe_id, stage_id, edge_container, match_spec) == true) {
      LOG_ERROR(
          "%s:%d Exm entry add for tbl 0x%x device id %d pipe %x with the "
          "passed key, already exists",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      LOG_ERROR("%s:%d Duplicate key", __func__, __LINE__);
      pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                           BF_LOG_ERR,
                                           exm_tbl->profile_id,
                                           mat_tbl_hdl,
                                           match_spec);
      goto err_cleanup;
    }
  }

  if (exm_tbl->hash_action == false) {
    /* Now that the hashes are computed, invoke the exact match hash scheme
     * implementation to take the computed hashes for this table and figure out
     * a location to place the entry.
     */

    if (exm_tbl->proxy_hash == true) {
      /* If this is a proxy hash table need to compute the proxy hash digest */

      /* Assuming the proxy hash to be the same across all stages, the
       * proxy hash compute is called only once.
       */
      status = pipe_mgr_exm_proxy_hash_compute(dev_tgt.device_id,
                                               exm_tbl->profile_id,
                                               mat_tbl_hdl,
                                               match_spec,
                                               stage_id,
                                               &proxy_hash);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in computing proxy hash for tbl 0x%x, device id %d"
            " error %s",
            __func__,
            __LINE__,
            mat_tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(status));
        return status;
      }
    }

    cuckoo_graph = pipe_mgr_exm_get_cuckoo_graph(exm_tbl, pipe_id, stage_id);

    status = cuckoo_program_new_entry(&move_list, cuckoo_graph, edge_container);

    if (status == PIPE_SUCCESS && exm_tbl->proxy_hash == true) {
      /* We are here because the entry can be successfully placed in the chosen
       * stage of the pipe-line. However since the table is a proxy hash table
       * we need to check if another entry with the same proxy hash is present
       * in the same stage. If it is present, we will not be housing this
       * entry in the chosen stage.
       */
      if (pipe_mgr_exm_proxy_hash_entry_exists(
              exm_tbl, pipe_id, stage_id, proxy_hash)) {
        status = PIPE_NO_SPACE;
      }
    }

    if (status != PIPE_SUCCESS) {
      /* The stage where the entry was chosen to be placed could not accommodate
       * the new entry or the table is a proxy hash table and there exists
       * another entry in this stage with the same proxy hash. Now just attempt
       * placing it in any other stage.
       */
      unsigned i = 0;
      unsigned num_tries = 0;
      pipe_mgr_exm_stage_info_t *stage_info = NULL;
      for (i = 0; i < exm_tbl_data->num_stages; i++) {
        stage_info = &exm_tbl_data->exm_stage_info[i];
        if (stage_info->stage_id == stage_id) {
          num_tries++;
          exm_tbl_data->entry_stats.failed[stage_id]++;
          if (num_tries == exm_tbl_data->num_stages) {
            break;
          } else {
            continue;
          }
        }
        cuckoo_graph = pipe_mgr_exm_get_cuckoo_graph(
            exm_tbl, pipe_id, stage_info->stage_id);

        /* Attempt programming in this stage now */
        status = pipe_mgr_exm_hash_compute(dev_tgt.device_id,
                                           exm_tbl->profile_id,
                                           mat_tbl_hdl,
                                           match_spec,
                                           stage_info->stage_id,
                                           hash_container,
                                           &num_hashes);

        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s : Error in computing the hashes for a new entry exact"
              " match hash table add for table with handle %d",
              __func__,
              mat_tbl_hdl);
          goto err_cleanup;
        }

        edge_container =
            pipe_mgr_exm_expand_to_logical_entries(exm_tbl,
                                                   pipe_id,
                                                   stage_info->stage_id,
                                                   hash_container,
                                                   num_hashes,
                                                   &ent_subword_loc);

        if (!edge_container) {
          LOG_ERROR(
              "%s:%d edge_container for tbl 0x%x, device id %d, pipe id %d, "
              "stage_id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              dev_tgt.device_id,
              pipe_id,
              stage_info->stage_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto err_cleanup;
        }

        if (pipe_mgr_exm_entry_exists(exm_tbl,
                                      pipe_id,
                                      stage_info->stage_id,
                                      edge_container,
                                      match_spec) == true) {
          LOG_ERROR(
              "%s:%d Exm entry add for tbl 0x%x, device id %d pipe %d with the "
              "passed key, already exists",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              dev_tgt.device_id,
              pipe_id);
          LOG_ERROR("%s:%d Duplicate key", __func__, __LINE__);
          pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                               BF_LOG_ERR,
                                               exm_tbl->profile_id,
                                               mat_tbl_hdl,
                                               match_spec);
          goto err_cleanup;
        }

        status =
            cuckoo_program_new_entry(&move_list, cuckoo_graph, edge_container);

        if (status != PIPE_SUCCESS) {
          num_tries++;
          exm_tbl_data->entry_stats.failed[stage_info->stage_id]++;
          if (num_tries == exm_tbl_data->num_stages) {
            break;
          }
        } else {
          if (exm_tbl->proxy_hash == true) {
            if (pipe_mgr_exm_proxy_hash_entry_exists(
                    exm_tbl, pipe_id, stage_info->stage_id, proxy_hash)) {
              status = PIPE_NO_SPACE;
            } else {
              /* We have successfully placed the entry in this stage id.
               * Record the stage id where the entry was placed.
               */
              stage_id = stage_info->stage_id;
            }
          } else {
            /* We have successfully placed the entry in this stage id.
             * Record the stage id where the entry was placed.
             */
            stage_id = stage_info->stage_id;
            break;
          }
        }
      }
    }

    /* Unable to place entry, check if entry can be placed in stash */
    if (0) {
      pipe_mat_ent_idx_t free_entry_idx = 0;
      stage_id = pipe_mgr_exm_get_stage_for_new_entry(exm_tbl, pipe_id);
      /* Do when stash is implemented */
      pipe_mgr_exm_find_empty_entry(
          exm_tbl, pipe_id, stage_id, cuckoo_graph, &free_entry_idx);
    }

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Could not place new entry in exact match table %s (0x%x), "
          "device id %d, pipe id %x with match spec:",
          __func__,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id);
      pipe_mgr_entry_format_log_match_spec(exm_tbl->dev_id,
                                           BF_LOG_ERR,
                                           exm_tbl->profile_id,
                                           exm_tbl->mat_tbl_hdl,
                                           match_spec);
      LOG_ERROR("Action spec:");
      pipe_mgr_entry_format_log_action_spec(exm_tbl->dev_id,
                                            BF_LOG_ERR,
                                            exm_tbl->profile_id,
                                            act_data_spec,
                                            act_fn_hdl);
      goto err_cleanup;
    }

    status = pipe_mgr_exm_transform_cuckoo_move_list(exm_tbl,
                                                     pipe_id,
                                                     stage_id,
                                                     cuckoo_graph,
                                                     move_list,
                                                     &edge_updated,
                                                     ent_subword_loc);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in transforming the cuckoo move list for "
          "exact match table with handle %d",
          __func__,
          mat_tbl_hdl);
      goto err_cleanup;
    }

    /* We have successfully found a evict chain of match entries in a stage to
     * place the new match entry. But before moving ahead with this move list
     * check if the required indirect resources can be obtained. If not, do not
     * go ahead with the move.
     */
    exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
        exm_tbl, exm_tbl_data->pipe_id, stage_id);
    if (exm_stage_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
      goto err_cleanup;
    }

    /* Handle indirect action/selector placement */
    move_list->ttl = ttl;
    /* Populate the proxy hash in the move list */
    move_list->proxy_hash = proxy_hash;
    if (exm_tbl->restore_ent_node) {
      move_list->logical_action_idx =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "action_idx")
              ->valuedouble;
      move_list->logical_sel_idx =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_idx")
              ->valuedouble;
      move_list->selector_len =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_len")
              ->valuedouble;
    } else {
      status =
          pipe_mgr_exm_alloc_indirect_resources(exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                new_ent_hdl,
                                                act_data_spec,
                                                &move_list->logical_action_idx,
                                                &move_list->logical_sel_idx,
                                                &move_list->selector_len);
      if (status != PIPE_SUCCESS) {
        pipe_status_t cur_status = PIPE_SUCCESS;
        char buf[1000];
        size_t bytes_written = 0;
        cur_status =
            pipe_mgr_entry_format_print_match_spec(exm_tbl->dev_id,
                                                   exm_tbl->profile_id,
                                                   exm_tbl->mat_tbl_hdl,
                                                   match_spec,
                                                   buf,
                                                   sizeof(buf),
                                                   &bytes_written);
        LOG_ERROR(
            "%s:%d Error in placing indirect resource for table 0x%x, device id"
            " %d, with match spec %s",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            buf);
        (void)cur_status;
        goto err_cleanup;
      }
    }

    status = cuckoo_move_graph_execute_moves(
        cuckoo_graph, move_list, edge_container, isTxn);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in executing the move list for action data entries"
          " for exm tbl with handle %d, pipe id %d, stage id %d, error %s",
          __func__,
          exm_tbl->mat_tbl_hdl,
          pipe_id,
          stage_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
  } else {
    /*
     * Hash action table handling.
     */
    exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
        exm_tbl, exm_tbl_data->pipe_id, stage_id);
    if (exm_stage_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
      goto err_cleanup;
    }
    if (exm_tbl->restore_ent_node) {
      movelist.logical_action_idx =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "action_idx")
              ->valuedouble;
      movelist.logical_sel_idx =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_idx")
              ->valuedouble;
      movelist.selector_len =
          cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_len")
              ->valuedouble;
    } else {
      /* Handle indirect action/selector placement */
      status =
          pipe_mgr_exm_alloc_indirect_resources(exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                new_ent_hdl,
                                                act_data_spec,
                                                &movelist.logical_action_idx,
                                                &movelist.logical_sel_idx,
                                                &movelist.selector_len);
      if (status != PIPE_SUCCESS) {
        pipe_status_t cur_status = PIPE_SUCCESS;
        char buf[1000];
        size_t bytes_written = 0;
        cur_status =
            pipe_mgr_entry_format_print_match_spec(exm_tbl->dev_id,
                                                   exm_tbl->profile_id,
                                                   exm_tbl->mat_tbl_hdl,
                                                   match_spec,
                                                   buf,
                                                   sizeof(buf),
                                                   &bytes_written);
        LOG_ERROR(
            "%s:%d Error in placing indirect resource for table 0x%x, device id"
            " %d, with match spec %s",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            buf);
        (void)cur_status;
        goto err_cleanup;
      }
    }
    /* For a hash-action table, there is no entry moves involved unlike
     * regular exact match entry adds. Just cook up a move-list which is
     * like a new entry add, i,e.. containing just one entry, with source
     * as invalid entry index and destination the new entry index.
     */

    /* Assert if the number of candidate locations to install the entry
     * was more than 1. Since hash action tables must be direct indexed
     * hence 1-way hash-tables.
     */
    PIPE_MGR_DBGCHK(edge_container->num_entries == 1);
    movelist.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
    movelist.dst_entry = edge_container->entries[0];
    movelist.ttl = ttl;

    move_list = &movelist;
  }
  if (IS_ACTION_SPEC_ACT_DATA_HDL(act_data_spec)) {
    move_list->adt_ent_hdl = act_data_spec->adt_ent_hdl;
  }
  pipe_mgr_exm_set_ent_hdl(exm_tbl_data, new_ent_hdl);

  /* The entry has been placed sucessfully, save state */
  /* When programming match spec into EXM table,
   * for tables with dynamic key mask attribute or
   * regular exm table, always program match spec  *without*
   * key mask applied.
   */
  match_spec = ms;
  status = pipe_mgr_exm_update_state(exm_tbl,
                                     exm_tbl_data,
                                     exm_stage_info,
                                     match_spec,
                                     act_data_spec,
                                     move_list,
                                     pipe_move_list,
                                     new_ent_hdl,
                                     act_fn_hdl);

  if (exm_tbl->restore_ent_node) {
    PIPE_MGR_FREE(edge_container->entries);
    edge_container->entries = NULL;
    PIPE_MGR_FREE(edge_container);
    edge_container = NULL;
  }

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in saving placement state for new entry hdl 0x%x, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        new_ent_hdl,
        mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(status));
    /* Deallocate the allocated entry handle */
    pipe_mgr_exm_deallocate_entry_hdl(exm_tbl, exm_tbl_data, new_ent_hdl);
    goto err_cleanup;
  }
err_cleanup:
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error adding the new entry into the table with"
        " handle %d  error: %s",
        __func__,
        mat_tbl_hdl,
        pipe_str_err(status));
    exm_tbl_data->entry_stats.total_failed++;
  }

  if (hash_container) {
    PIPE_MGR_FREE(hash_container);
  }
  if (status != PIPE_SUCCESS && edge_updated) {
    pipe_status_t rc = pipe_mgr_exm_rollback_cuckoo_ml_transform(
        exm_tbl, pipe_id, stage_id, move_list);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Cuckoo edge state rollback error", __func__, __LINE__);
    }
  }

  if (exm_tbl != NULL) {
    if (exm_tbl->hash_action == true) {
      if (edge_container) {
        if (edge_container->entries) {
          PIPE_MGR_FREE(edge_container->entries);
        }
        PIPE_MGR_FREE(edge_container);
        edge_container = NULL;
      }
    }
  }

  if (status == PIPE_SUCCESS) {
    // initiate the moves idx to -1 as add op does not
    // count as move.
    int moves = -1;
    PIPE_MGR_EXM_TRACE(dev_tgt.device_id,
                       exm_tbl->name,
                       new_ent_hdl,
                       dev_tgt.dev_pipe_id,
                       stage_id,
                       " Entry move-list for ent hdl 0x%x",
                       new_ent_hdl);
    cuckoo_move_list_t *traverser = move_list;
    while (traverser) {
      LOG_TRACE("\t\t Src entry %d, Dst entry %d",
                traverser->src_entry,
                traverser->dst_entry);
      traverser = traverser->next;
      moves++;
    }
    if (moves > CUCKOO_MAX_NUM_MOVES) {
      LOG_ERROR(
          "Entry add generated moves %d greater than %d which is not expected",
          moves,
          CUCKOO_MAX_NUM_MOVES);
    } else {
      exm_tbl_data->entry_stats.stage_stats[stage_id].moves[moves]++;
    }
    PIPE_MGR_EXM_TRACE(
        dev_tgt.device_id,
        exm_tbl->name,
        new_ent_hdl,
        dev_tgt.dev_pipe_id,
        stage_id,
        "Successfully added exm entry with handle %d, action function hdl 0x%x",
        new_ent_hdl,
        act_fn_hdl);
    LOG_TRACE("Exiting %s successfully", __func__);
  }

  return status;
}

/** \brief pipe_mgr_exm_ent_add:
 *         Installs a new entry into the given exact match table.
 *
 *
 * \param sess_hdl Handle of the session associated with the client.
 * \param dev_tgt Device id of the device onto which this entry should be
 *installed.
 * \param mat_tbl_hdl Handle associated with the match entry table.
 * \param match_spec Pointer to the match spec associated with the entry.
 * \param act_fn_hdl Handle of the action function associated with the entry.
 * \param act_data_spec Pointer to the action spec associated with the entry.
 * \param pipe_api_flags API flags asscociated with this request.
 * \return pipe_status_t Status of this operation.
 */
pipe_status_t pipe_mgr_exm_ent_place(dev_target_t dev_tgt,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_tbl_match_spec_t *match_spec,
                                     pipe_act_fn_hdl_t act_fn_hdl,
                                     pipe_action_spec_t *act_data_spec,
                                     uint32_t ttl,
                                     uint32_t pipe_api_flags,
                                     pipe_mat_ent_hdl_t *ent_hdl_p,
                                     pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mat_ent_hdl_t new_ent_hdl = 0;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  bf_dev_pipe_t pipe_id = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (exm_tbl->symmetric == true) {
    pipe_id = BF_DEV_PIPE_ALL;
    if (dev_tgt.dev_pipe_id != DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for a symmetric exm tbl, with "
          "handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          mat_tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
  } else if (dev_tgt.dev_pipe_id == DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id of all pipes passed for a asymmetric exm tbl, "
        "with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  } else {
    pipe_id = dev_tgt.dev_pipe_id;
  }
  if (pipe_id == BF_DEV_PIPE_ALL) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[0];
  } else {
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  }
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, tbl_data does not exist",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_id);
    return PIPE_INVALID_ARG;
  }
  status = pipe_mgr_exm_verify_indices(exm_tbl, exm_tbl_data, act_data_spec);
  if (PIPE_SUCCESS != status) {
    return status;
  }
  /* Allocate a new entry handle.  Note, if this is hitless HA and the table was
   * full before HA then all entries would have been restored to the table and
   * no more handles will be available.  Wait until we lookup the spec and use
   * that handle. */
  new_ent_hdl = pipe_mgr_exm_allocate_new_ent_hdl(exm_tbl_data);
  if (new_ent_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
    if (false == pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
      LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d, is full",
                __func__,
                __LINE__,
                mat_tbl_hdl,
                dev_tgt.device_id);
      return PIPE_NO_SPACE;
    }
  }
  /* If warm init is in progress - i,e.. API replay, then need to lookup if this
   * entry is already present.
   */
  if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
    LOG_DBG("%s Replay EXM entry tbl 0x%x dev %d pipe %x match spec",
            exm_tbl->name,
            mat_tbl_hdl,
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id);
    pipe_mgr_entry_format_log_match_spec(dev_tgt.device_id,
                                         BF_LOG_DBG,
                                         exm_tbl->profile_id,
                                         mat_tbl_hdl,
                                         match_spec);
    pipe_mgr_entry_format_log_action_spec(dev_tgt.device_id,
                                          BF_LOG_DBG,
                                          exm_tbl->profile_id,
                                          act_data_spec,
                                          act_fn_hdl);
    pipe_mat_ent_hdl_t ha_entry_hdl = -1;
    status =
        pipe_mgr_hitless_ha_lookup_spec(&exm_tbl_data->ha_hlp_info->spec_map,
                                        match_spec,
                                        act_data_spec,
                                        act_fn_hdl,
                                        new_ent_hdl,
                                        &ha_entry_hdl,
                                        ttl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in processing match spec/action spec during API replay "
          "at HLP for tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(status));
      return status;
    }
    if (new_ent_hdl != ha_entry_hdl) {
      /* This implies that an existing entry was matched to this new
       * entry.  Release the entry handle we just allocated for it. */
      pipe_mgr_exm_deallocate_entry_hdl(exm_tbl, exm_tbl_data, new_ent_hdl);
      new_ent_hdl = ha_entry_hdl;
      /* Set the matched up entry hdl */
      pipe_mgr_exm_set_ent_hdl(exm_tbl_data, ha_entry_hdl);
    }
    if (new_ent_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
      LOG_ERROR(
          "Dev %d pipe %x table %s (0x%x): Replay of new entry to full table "
          "unsupported.",
          dev_tgt.device_id,
          dev_tgt.dev_pipe_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl);
      PIPE_MGR_DBGCHK(new_ent_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL);
      return PIPE_NO_SPACE;
    }

    if (exm_tbl->num_sel_tbl_refs) {
      pipe_sel_tbl_hdl_t sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;
      pipe_mgr_exm_entry_info_t *entry_info =
          pipe_mgr_exm_get_entry_info(exm_tbl, new_ent_hdl);
      if (entry_info) {
        if (act_data_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE) {
          pipe_mgr_sel_hlp_update_state(dev_tgt.device_id,
                                        dev_tgt.dev_pipe_id,
                                        exm_tbl->mat_tbl_hdl,
                                        entry_info->mat_ent_hdl,
                                        sel_tbl_hdl,
                                        act_data_spec->sel_grp_hdl,
                                        entry_info->logical_sel_idx);
          unpack_mat_ent_data_as(entry_info->entry_data)->sel_grp_hdl =
              act_data_spec->sel_grp_hdl;
        }
      }
    }
  } else {
    if (pipe_mgr_exm_is_tbl_full(exm_tbl, exm_tbl_data)) {
      LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d, pipe id %d is full",
                __func__,
                __LINE__,
                mat_tbl_hdl,
                dev_tgt.device_id,
                pipe_id);
      /* Deallocate the allocated entry handle */
      pipe_mgr_exm_deallocate_entry_hdl(exm_tbl, exm_tbl_data, new_ent_hdl);
      return PIPE_NO_SPACE;
    }

    status = pipe_mgr_exm_ent_place_with_hdl_internal(dev_tgt,
                                                      exm_tbl,
                                                      exm_tbl_data,
                                                      mat_tbl_hdl,
                                                      match_spec,
                                                      act_fn_hdl,
                                                      act_data_spec,
                                                      ttl,
                                                      pipe_api_flags,
                                                      new_ent_hdl,
                                                      pipe_move_list);
    if (status != PIPE_SUCCESS) {
      /* Deallocate the allocated entry handle */
      pipe_mgr_exm_deallocate_entry_hdl(exm_tbl, exm_tbl_data, new_ent_hdl);
      return status;
    }
  }
  *ent_hdl_p = new_ent_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_plcmt_data(bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_mgr_move_list_t **move_list) {
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mgr_move_list_t *ml_head = NULL, *ml_tail = NULL;
  unsigned long key = 0;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *tbl_data;
  bf_map_t *htbl;
  uint32_t i;

  /* Walk through all table instances */
  for (i = 0; i < exm_tbl->num_tbls; i++) {
    tbl_data = &exm_tbl->exm_tbl_data[i];
    htbl = &tbl_data->entry_info_htbl;

    for (bf_map_sts_t ms = bf_map_get_first(htbl, &key, (void **)&entry_info);
         ms == BF_MAP_OK;
         ms = bf_map_get_next(htbl, &key, (void **)&entry_info)) {
      pipe_mat_ent_hdl_t mat_ent_hdl = key;
      bf_dev_pipe_t pipe_id = entry_info->pipe_id;

      bool is_dflt = pipe_mgr_exm_is_ent_hdl_default(tbl_data, mat_ent_hdl);
      pipe_mgr_move_list_t *ml = alloc_move_list(
          NULL,
          is_dflt ? PIPE_MAT_UPDATE_SET_DFLT : PIPE_MAT_UPDATE_ADD,
          pipe_id);
      if (!ml) {
        /* Clean up anything already allocated. */
        while (ml_head) {
          pipe_mgr_move_list_t *x = ml_head;
          ml_head = ml_head->next;
          PIPE_MGR_FREE(x);
        }
        return PIPE_NO_SYS_RESOURCES;
      }
      if (!is_dflt) {
        pipe_mgr_exm_stage_info_t *exm_stage_info;
        exm_stage_info = tbl_data->stage_info_ptrs[entry_info->stage_id];
        ml->u.single.logical_idx =
            entry_info->entry_idx + exm_stage_info->stage_offset;
      }
      ml->entry_hdl = mat_ent_hdl;
      ml->data = entry_info->entry_data;

      if (entry_info->adt_ent_hdl_valid) {
        ml->adt_ent_hdl = entry_info->adt_ent_hdl;
        ml->adt_ent_hdl_valid = true;
      }
      ml->logical_sel_idx = entry_info->logical_sel_idx;
      ml->logical_action_idx = entry_info->logical_action_idx;
      ml->selector_len = entry_info->selector_len;
      if (!ml_head) {
        ml_head = ml;
      } else {
        ml_tail->next = ml;
      }
      ml_tail = ml;
    }
  }

  *move_list = ml_head;
  return PIPE_SUCCESS;
}

// return PIPE_SUCCESS if table has dynamic key mask else returns
// PIPE_NOT_SUPPORTED if table is not dynamic key mask.
pipe_status_t pipe_mgr_get_exm_key_with_dkm_mask(
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_tbl_match_spec_t *match_spec,
    pipe_tbl_match_spec_t *dkm_applied_ms) {
  if (mat_tbl_info->dynamic_key_mask_table) {
    // When EXM table has dynamic key mask construct
    // apply dynamic key mask on match spec before
    // placing the entry
    if (mat_tbl_info->match_key_mask_width) {
      // modify match_spec->value_bits to include only those match-bits
      // into hash computation that are specified by match-key-mask
      PIPE_MGR_MEMCPY(
          dkm_applied_ms, match_spec, sizeof(pipe_tbl_match_spec_t));
      PIPE_MGR_DBGCHK(dkm_applied_ms->num_match_bytes ==
                      mat_tbl_info->match_key_mask_width);
      for (int i = 0; i < dkm_applied_ms->num_match_bytes; i++) {
        dkm_applied_ms->match_value_bits[i] = match_spec->match_value_bits[i];
        // apply match-key-mask
        dkm_applied_ms->match_value_bits[i] &= mat_tbl_info->match_key_mask[i];
      }
      return (PIPE_SUCCESS);
    } else {
      // Although dynamic key mask table, no mask has been set so far.
      // return PIPE_NOT_SUPPORTED so that caller of the API
      // can continue to use match-spec without applying mask.
      return (PIPE_NOT_SUPPORTED);
    }
  }
  return (PIPE_NOT_SUPPORTED);
}

pipe_status_t pipe_mgr_exm_ent_place_with_hdl(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *ms,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_data_spec,
    uint32_t ttl,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t new_ent_hdl,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl =
      pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d EXM tbl with handle 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data =
      pipe_mgr_exm_tbl_get_instance(exm_tbl, dev_tgt.dev_pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl info for tbl 0x%x pipe id %d, device id %d not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (pipe_mgr_exm_is_tbl_full(exm_tbl, exm_tbl_data)) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d, pipe id %d is full",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id,
              dev_tgt.dev_pipe_id);
    return PIPE_NO_SPACE;
  }
  status = pipe_mgr_exm_ent_place_with_hdl_internal(dev_tgt,
                                                    exm_tbl,
                                                    exm_tbl_data,
                                                    exm_tbl->mat_tbl_hdl,
                                                    ms,
                                                    act_fn_hdl,
                                                    act_data_spec,
                                                    ttl,
                                                    pipe_api_flags,
                                                    new_ent_hdl,
                                                    pipe_move_list);
  return status;
}

pipe_status_t pipe_mgr_exm_default_ent_place(
    dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mat_ent_hdl_t *ent_hdl_p,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mat_ent_idx_t def_ent_idx = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  pipe_mat_ent_hdl_t def_ent_hdl = 0;
  uint8_t stage_id = 0;
  bf_dev_pipe_t pipe_id = 0;
  cuckoo_move_list_t move_list;
  uint32_t num_stages = 0;
  pipe_mgr_exm_entry_info_t *existing_entry_info = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);

  PIPE_MGR_MEMSET(&move_list, 0, sizeof(cuckoo_move_list_t));

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    status = PIPE_OBJ_NOT_FOUND;
    return status;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) && (!exm_tbl->symmetric)) {
    LOG_ERROR(
        "%s:%d Dev %d Exm tbl %s 0x%x, pipe %x specified for asymmetric tbl",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, dev_tgt.dev_pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Dev %d Exm tbl %s 0x%x pipe %x tbl instance not found",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_data->api_flags = pipe_api_flags;
  def_ent_hdl = pipe_mgr_exm_get_def_ent_hdl(exm_tbl_data);

  LOG_DBG("%s Request to install EXM default entry", exm_tbl->name);
  LOG_DBG("%s : EXM entry Action spec", exm_tbl->name);
  pipe_mgr_entry_format_log_action_spec(
      dev_tgt.device_id, BF_LOG_DBG, exm_tbl->profile_id, act_spec, act_fn_hdl);

  if (exm_tbl->hash_action) {
    /* If this is a hitless HA replay just save the default entry in the table's
     * spec map so it can be used in the compute-delta phase later. */
    if (pipe_mgr_hitless_warm_init_in_progress(dev_tgt.device_id)) {
      pipe_mgr_spec_map_t *spec_map = &exm_tbl_data->ha_hlp_info->spec_map;
      pipe_action_spec_t *aspec_copy = NULL;
      if (!pipe_mgr_tbl_copy_action_spec(aspec_copy, act_spec)) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      if (spec_map->def_act_spec) {
        pipe_mgr_tbl_destroy_action_spec(&spec_map->def_act_spec);
      }
      spec_map->def_act_spec = aspec_copy;
      spec_map->def_act_fn_hdl = act_fn_hdl;
      *ent_hdl_p = def_ent_hdl;
      return PIPE_SUCCESS;
    }

    /* Create a set_default move node. This will be expanded into a series of
     * adds in the LLP to populate all unoccupied entries. */
    pipe_mgr_move_list_t *move_node =
        alloc_move_list(NULL, PIPE_MAT_UPDATE_SET_DFLT, exm_tbl_data->pipe_id);
    if (move_node == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    if (*pipe_move_list) {
      (*pipe_move_list)->next = move_node;
    } else {
      *pipe_move_list = move_node;
    }
    move_node->entry_hdl = def_ent_hdl;
    cuckoo_move_list_t exm_move_node = {0};
    exm_move_node.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
    exm_move_node.dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
    exm_move_node.mat_ent_hdl = def_ent_hdl;
    pipe_mgr_exm_update_state_for_new_entry(
        exm_tbl,
        def_ent_hdl,
        NULL,
        act_spec,
        act_fn_hdl,
        &exm_move_node,
        move_node,
        PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
        exm_tbl_data,
        &exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1],
        true);
    pipe_mgr_exm_set_def_ent_placed(exm_tbl_data);
    *ent_hdl_p = def_ent_hdl;
    return PIPE_SUCCESS;
  }

  if (exm_tbl->symmetric == true) {
    /* The default entry is always installed in the last stage the table exists
     */
    num_stages = exm_tbl_data->num_stages;
    pipe_id = BF_DEV_PIPE_ALL;
    exm_stage_info = &exm_tbl_data->exm_stage_info[num_stages - 1];
    stage_id = exm_stage_info->stage_id;
  } else {
    pipe_id = dev_tgt.dev_pipe_id;
    num_stages = exm_tbl_data->num_stages;
    exm_stage_info = &exm_tbl_data->exm_stage_info[num_stages - 1];
    stage_id = exm_stage_info->stage_id;
  }

  def_ent_idx = exm_stage_info->default_miss_entry_idx;

  /* Prime a cuckoo move list with the reserved default entry index as the
   * entry which got programmed.
   */
  move_list.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  move_list.dst_entry = def_ent_idx;

  PIPE_MGR_DBGCHK(def_ent_idx != PIPE_MAT_ENT_INVALID_ENTRY_INDEX);

  /* In order to support the "update" operation, check if the default entry
   * is already installed, if it is, then delete the existing default entry
   * and re-install the current one.
   */

  if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
    existing_entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, def_ent_hdl);
    if (!existing_entry_info) {
      LOG_ERROR(
          "%s:%d Unable to find placed default entry %d in exm table 0x%x "
          "pipe %d device %d",
          __func__,
          __LINE__,
          def_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.dev_pipe_id,
          dev_tgt.device_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    /* De-allocate indirect resources attached */
    status = pipe_mgr_exm_dealloc_indirect_resources(
        exm_tbl, exm_tbl_data, existing_entry_info);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in deallocating indirect resources for entry %d, tbl "
          "0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          def_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  }

  if (!exm_tbl->restore_ent_node) {
    /* Re-allocate indirect resources */
    status =
        pipe_mgr_exm_alloc_indirect_resources(exm_tbl,
                                              exm_tbl_data,
                                              exm_stage_info,
                                              def_ent_hdl,
                                              act_spec,
                                              &move_list.logical_action_idx,
                                              &move_list.logical_sel_idx,
                                              &move_list.selector_len);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in allocating indirect resources for tbl 0x%x, device "
          "id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  } else {
    move_list.logical_action_idx =
        cJSON_GetObjectItem(exm_tbl->restore_ent_node, "action_idx")
            ->valuedouble;
    move_list.logical_sel_idx =
        cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_idx")->valuedouble;
    move_list.selector_len =
        cJSON_GetObjectItem(exm_tbl->restore_ent_node, "sel_len")->valuedouble;
  }

  if (IS_ACTION_SPEC_ACT_DATA_HDL(act_spec)) {
    move_list.adt_ent_hdl = act_spec->adt_ent_hdl;
  }
  /* If this is a modification of an existing default entry use the state update
   * function for modify.  However, if we are in a state restore use the update
   * state for new entries since we do not expect entry modification in state
   * restore cases. */
  if (existing_entry_info && !exm_tbl->restore_ent_node) {
    status = pipe_mgr_exm_update_state_for_action_modify(
        exm_tbl,
        exm_tbl_data,
        exm_stage_info,
        def_ent_hdl,
        act_spec,
        act_fn_hdl,
        existing_entry_info,
        pipe_move_list,
        move_list.logical_action_idx,
        move_list.logical_sel_idx,
        move_list.selector_len);
  } else {
    status = pipe_mgr_exm_update_state(exm_tbl,
                                       exm_tbl_data,
                                       exm_stage_info,
                                       NULL,
                                       act_spec,
                                       &move_list,
                                       pipe_move_list,
                                       def_ent_hdl,
                                       act_fn_hdl);
  }
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  pipe_mgr_exm_set_def_ent_placed(exm_tbl_data);

  PIPE_MGR_EXM_TRACE(dev_tgt.device_id,
                     exm_tbl->name,
                     def_ent_hdl,
                     pipe_id,
                     stage_id,
                     "Successfully added default entry with handle 0x%x action "
                     "function handle 0x%x",
                     def_ent_hdl,
                     act_fn_hdl);

  *ent_hdl_p = def_ent_hdl;
  return PIPE_SUCCESS;
}

/* Get default entry handle */
pipe_status_t pipe_mgr_exm_default_ent_hdl_get(dev_target_t dev_tgt,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                               pipe_mat_ent_hdl_t *ent_hdl_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL) && (!exm_tbl->symmetric)) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, Invalid pipe %d specified for asymmetric tbl, dev "
        "%d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  exm_tbl_data =
      pipe_mgr_exm_tbl_get_instance_from_any_pipe(exm_tbl, dev_tgt.dev_pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl info for tbl 0x%x pipe id %d, device id %d not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  *ent_hdl_p = 0;
  if (pipe_mgr_is_device_virtual(dev_tgt.device_id)) {
    if (!pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
      return PIPE_OBJ_NOT_FOUND;
    }

    *ent_hdl_p = pipe_mgr_exm_get_def_ent_hdl(exm_tbl_data);
  } else {
    if (!pipe_mgr_exm_is_default_ent_installed(exm_tbl_data)) {
      return PIPE_OBJ_NOT_FOUND;
    }

    *ent_hdl_p = pipe_mgr_exm_get_def_ent_hdl(exm_tbl_data);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_entry_del(bf_dev_id_t device_id,
                                     pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                     pipe_mat_ent_hdl_t mat_ent_hdl,
                                     uint32_t pipe_api_flags,
                                     pipe_mgr_move_list_t **pipe_move_list) {
  uint8_t stage_id = 0;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_status_t status = PIPE_SUCCESS;
  cuckoo_move_list_t move_list;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  dev_target_t dev_tgt;

  LOG_TRACE("Entering %s", __func__);
  PIPE_MGR_MEMSET(&move_list, 0, sizeof(cuckoo_move_list_t));
  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  LOG_DBG("%s Request to delete EXM entry with match spec", exm_tbl->name);
  /* Get entry info */
  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, mat_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  dev_tgt.device_id = device_id;
  dev_tgt.dev_pipe_id = entry_info->pipe_id;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl info for tbl 0x%x, pipe id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              entry_info->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_data->api_flags = pipe_api_flags;

  /* Check if the entry handle passed in a valid one */
  if (pipe_mgr_exm_tbl_is_ent_hdl_valid(exm_tbl_data, mat_ent_hdl) == false) {
    LOG_ERROR(
        "%s : Invalid entry handle passed for exact match entry delete"
        " for table with handle %d, entry handle %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        mat_ent_hdl);
    return PIPE_INVALID_ARG;
  }
  if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
    if (mat_ent_hdl == pipe_mgr_exm_get_def_ent_hdl(exm_tbl_data)) {
      LOG_ERROR(
          "%s:%d ERROR : Entry delete API called on default entry handle %d "
          "for tbl 0x%x, device id %d, pipe id %d",
          __func__,
          __LINE__,
          mat_ent_hdl,
          mat_tbl_hdl,
          device_id,
          dev_tgt.dev_pipe_id);
      return PIPE_INVALID_ARG;
    }
  }
  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, stage id "
        "%d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        entry_info->pipe_id,
        entry_info->stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  LOG_DBG("%s Request to delete EXM entry with handle %d",
          exm_tbl->name,
          mat_ent_hdl);
  /* First, deallocate any indirectly addressed resources */
  status = pipe_mgr_exm_dealloc_indirect_resources(
      exm_tbl, exm_tbl_data, entry_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in deallocating indirect addressed resources for entry "
        "%d, tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  /* For the entry delete case, prime a cuckoo move list with just one
   * node, with both src entry and the dst entry as an
   * INVALID entry index. This is to convey the delete.
   */
  move_list.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  move_list.dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;

  /* Update state */
  status = pipe_mgr_exm_update_state(exm_tbl,
                                     exm_tbl_data,
                                     exm_stage_info,
                                     NULL,
                                     NULL,
                                     &move_list,
                                     pipe_move_list,
                                     mat_ent_hdl,
                                     0);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry handle %d, tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  /* De-allocate the entry handle */
  pipe_mgr_exm_deallocate_entry_hdl(exm_tbl, exm_tbl_data, mat_ent_hdl);
  PIPE_MGR_EXM_TRACE(
      dev_tgt.device_id,
      exm_tbl->name,
      mat_ent_hdl,
      dev_tgt.dev_pipe_id,
      stage_id,
      "Successfully deleted exm entry handle %d from tbl %d, device id %d",
      mat_ent_hdl,
      mat_tbl_hdl,
      dev_tgt.device_id);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_ent_set_action(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *act_spec,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_idx_t action_idx = 0;
  pipe_idx_t selector_idx = 0;
  uint32_t selector_len = 0;
  uint8_t *shadow_ptr_arr[RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK];

  PIPE_MGR_MEMSET(shadow_ptr_arr,
                  0,
                  sizeof(uint8_t *) * RMT_MAX_MEM_UNITS_PER_TBL_WORD_BLK);

  /* First, validate the new action spec to see if it conforms to the constraint
   * of how many indirect resources an action can address.
   */
  if (act_spec != NULL &&
      pipe_mgr_exm_ent_validate_action_spec(act_spec) == false) {
    LOG_ERROR(
        "%s:%d Invalid action spec passed for exm entry hdl %d, for tbl 0x%x"
        " device id %d",
        __func__,
        __LINE__,
        mat_ent_hdl,
        mat_tbl_hdl,
        device_id);
    return PIPE_INVALID_ARG;
  }

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s:Could not find the exact match table info for table handle 0x%x "
        "device %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, mat_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* This is entry refresh call, get data from SW state */
  if (act_spec == NULL) {
    act_spec = unpack_mat_ent_data_as(entry_info->entry_data);
    act_fn_hdl = unpack_mat_ent_data_afun_hdl(entry_info->entry_data);
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm table data for table 0x%x device id %d, pipe id %d not "
        "found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        entry_info->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_data->api_flags = pipe_api_flags;

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm table stage info not found for tbl 0x%x, device id %d, pipe "
        "id %d stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        entry_info->pipe_id,
        entry_info->stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_DBG(
      "%s Request to modify EXM entry handle %d", exm_tbl->name, mat_ent_hdl);
  pipe_mgr_entry_format_log_action_spec(
      device_id, BF_LOG_DBG, exm_tbl->profile_id, act_spec, act_fn_hdl);

  status = pipe_mgr_exm_verify_indices(exm_tbl, exm_tbl_data, act_spec);
  if (PIPE_SUCCESS != status) {
    return status;
  }
  /* First de-allocate indirect resources */
  status = pipe_mgr_exm_dealloc_indirect_resources(
      exm_tbl, exm_tbl_data, entry_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in de-allocating indirect resources for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  /* Now, re-allocate the indirect resources as per the latest action spec */
  status = pipe_mgr_exm_alloc_indirect_resources(exm_tbl,
                                                 exm_tbl_data,
                                                 exm_stage_info,
                                                 entry_info->mat_ent_hdl,
                                                 act_spec,
                                                 &action_idx,
                                                 &selector_idx,
                                                 &selector_len);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in allocating indirect resources for entry %d, table "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  /* Update state required for this modify */
  status = pipe_mgr_exm_update_state_for_action_modify(exm_tbl,
                                                       exm_tbl_data,
                                                       exm_stage_info,
                                                       mat_ent_hdl,
                                                       act_spec,
                                                       act_fn_hdl,
                                                       entry_info,
                                                       pipe_move_list,
                                                       action_idx,
                                                       selector_idx,
                                                       selector_len);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for action modify for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }

  PIPE_MGR_EXM_TRACE(device_id,
                     exm_tbl->name,
                     mat_ent_hdl,
                     -1,
                     -1,
                     "Successfully set the action for exm entry hdl 0x%x to "
                     "action function 0x%x",
                     mat_ent_hdl,
                     act_fn_hdl);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_ent_set_resource(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    pipe_res_spec_t *resources,
    int resource_count,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_act_spec_delta_t act_spec_delta = {0};
  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exact match table %d, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, mat_ent_hdl, __func__, __LINE__);
  if (!exm_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_data->api_flags = pipe_api_flags;

  /* Check if the entry handle passed in a valid one */
  if (pipe_mgr_exm_tbl_is_ent_hdl_valid(exm_tbl_data, mat_ent_hdl) == false) {
    LOG_ERROR(
        "%s : Invalid entry handle passed "
        " for table with handle %d, entry handle %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        mat_ent_hdl);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_exm_get_resource_delta(
      exm_tbl, resources, resource_count, &act_spec_delta);

  status = pipe_mgr_exm_update_state_for_set_resource(
      exm_tbl, mat_ent_hdl, &act_spec_delta, pipe_move_list);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for set resource for entry handle %d, "
        "table 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_tbl_mgr_entry_activate(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  bool is_stash = false, version_valid = true;
  uint32_t version_valid_bits = 0;
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;

  pipe_mat_ent_idx_t stage_ent_idx =
      pipe_mgr_exm_get_entry_loc(exm_tbl, mat_ent_hdl, &pipe_id, &stage_id);
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        device_id,
        pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  status = pipe_mgr_exm_get_ram_shadow_copy(exm_tbl,
                                            exm_tbl_data,
                                            exm_stage_info,
                                            stage_ent_idx,
                                            exm_stage_info->shadow_ptr_arr);
  if (status != PIPE_SUCCESS) {
    return status;
  }
  status =
      pipe_mgr_exm_compute_ram_shadow_copy_vv(exm_tbl,
                                              exm_stage_info,
                                              stage_ent_idx,
                                              exm_stage_info->shadow_ptr_arr,
                                              exm_stage_info->wide_word_indices,
                                              exm_stage_info->mem_id_arr,
                                              version_valid,
                                              is_stash,
                                              &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    return status;
  }
  uint8_t vv_word_index = *exm_stage_info->wide_word_indices;
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      stage_ent_idx,
      1,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      false);
  return status;
}

pipe_status_t pipe_mgr_exm_tbl_mgr_entry_deactivate(
    pipe_sess_hdl_t sess_hdl,
    uint8_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  bool is_stash = false, version_valid = false;
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint32_t version_valid_bits = 0;
  pipe_mat_ent_idx_t stage_ent_idx =
      pipe_mgr_exm_get_entry_loc(exm_tbl, mat_ent_hdl, &pipe_id, &stage_id);
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        device_id,
        pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }
  status = pipe_mgr_exm_get_ram_shadow_copy(exm_tbl,
                                            exm_tbl_data,
                                            exm_stage_info,
                                            stage_ent_idx,
                                            exm_stage_info->shadow_ptr_arr);
  if (status != PIPE_SUCCESS) {
    return status;
  }
  status =
      pipe_mgr_exm_compute_ram_shadow_copy_vv(exm_tbl,
                                              exm_stage_info,
                                              stage_ent_idx,
                                              exm_stage_info->shadow_ptr_arr,
                                              exm_stage_info->wide_word_indices,
                                              exm_stage_info->mem_id_arr,
                                              version_valid,
                                              is_stash,
                                              &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    return status;
  }
  uint8_t vv_word_index = *exm_stage_info->wide_word_indices;
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      stage_ent_idx,
      1,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      false);
  return status;
}

/** \brief pipe_mgr_exm_tbl_get:
 *         Get a pointer to the shadow copy associate with the given table
 *         handle.
 */

pipe_mgr_exm_tbl_t *pipe_mgr_exm_tbl_get(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  bf_map_sts_t status;
  unsigned long key = 0;
  key = mat_tbl_hdl;
  status = pipe_mgr_exm_tbl_map_get(dev_id, key, (void **)&exm_tbl);
  if (status != BF_MAP_OK) {
    return NULL;
  }

  return exm_tbl;
}

/** \brief pipe_mgr_exm_init:
 *         Performs initialization required by the exact match table manager.
 */

pipe_status_t pipe_mgr_exm_init(void) {
  LOG_TRACE("Entering %s", __func__);

  pipe_status_t status = PIPE_SUCCESS;

  status = pipe_mgr_hash_init();

  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s : Error in initializing the pipe mgr hash module", __func__);
    return PIPE_INIT_ERROR;
  }

  LOG_TRACE("Exiting %s successfully", __func__);

  return PIPE_SUCCESS;
}

uint8_t pipe_mgr_exm_get_stage_for_new_entry(pipe_mgr_exm_tbl_t *exm_tbl,
                                             bf_dev_pipe_t pipe_id) {
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  float curr_occupancy = 0.0;
  float min_occupancy = 0.0;
  uint8_t min_stage_id = 0;

  if (pipe_id == BF_DEV_PIPE_ALL) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[0];
  } else {
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  }
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl instance for tbl 0x%x, pipe id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              pipe_id);
    PIPE_MGR_DBGCHK(0);
    return 0;
  }

  unsigned i = 0;
  min_occupancy = (float)(exm_tbl_data->exm_stage_info[0].num_entries_placed /
                          (float)exm_tbl_data->exm_stage_info[0].capacity);
  min_stage_id = exm_tbl_data->exm_stage_info[0].stage_id;

  for (i = 0; i < exm_tbl_data->num_stages; i++) {
    exm_stage_info = &exm_tbl_data->exm_stage_info[i];
    curr_occupancy = (float)(exm_stage_info->num_entries_placed /
                             (float)exm_stage_info->capacity);

    if (curr_occupancy < 0.7) {
      return exm_stage_info->stage_id;
    } else {
      if (curr_occupancy < min_occupancy) {
        min_occupancy = curr_occupancy;
        min_stage_id = exm_stage_info->stage_id;
      }
    }
  }

  return min_stage_id;
}

pipe_mat_ent_hdl_t pipe_mgr_exm_get_ent_hdl_from_ent_idx(
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  bf_map_sts_t map_sts;
  uintptr_t htbl_data = 0;
  unsigned long key;
  key = entry_idx + exm_stage_info->stage_offset;
  map_sts = bf_map_get(
      &exm_tbl_data->ent_idx_to_ent_hdl_htbl, key, (void **)&htbl_data);

  if (map_sts != BF_MAP_OK) {
    return PIPE_MAT_ENT_HDL_INVALID_HDL;
  }
  return (pipe_mat_ent_hdl_t)htbl_data;
}
pipe_mat_ent_hdl_t pipe_mgr_exm_get_ent_hdl_from_dir_addr(
    pipe_mat_ent_idx_t entry_idx, pipe_mgr_exm_stage_info_t *exm_stage_info) {
  bf_map_sts_t s;
  uintptr_t htbl_data = 0;

  s = bf_map_get(
      &exm_stage_info->log_idx_to_ent_hdl_htbl, entry_idx, (void **)&htbl_data);
  if (s != BF_MAP_OK) {
    return PIPE_MAT_ENT_HDL_INVALID_HDL;
  }
  return (pipe_mat_ent_hdl_t)htbl_data;
}

cuckoo_move_graph_t *pipe_mgr_exm_get_cuckoo_graph(pipe_mgr_exm_tbl_t *exm_tbl,
                                                   bf_dev_pipe_t pipe_id,
                                                   uint8_t stage_id) {
  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;

  if (exm_tbl == NULL) {
    return NULL;
  }

  exm_tbl_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);

  if (exm_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table stage info for "
        " exact match table with handle %d, pipe id %d, stage id %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return NULL;
  }

  return pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_tbl_stage_info);
}

pipe_mgr_exm_stage_info_t *pipe_mgr_exm_tbl_get_stage_info(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id, uint8_t stage_id) {
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;

  if (exm_tbl == NULL) {
    return NULL;
  }

  if (pipe_id == BF_DEV_PIPE_ALL) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[0];
  } else {
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  }

  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl instance for tbl 0x%x, pipe id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              pipe_id);
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  return exm_tbl_data->stage_info_ptrs[stage_id];
}

pipe_mat_ent_hdl_t pipe_mgr_exm_allocate_new_ent_hdl(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  int new_ent_hdl = 0;
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;

  if (exm_tbl_data == NULL) {
    return PIPE_MAT_ENT_HDL_INVALID_HDL;
  }
  ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;
  /* Allocate the entry handle */
  new_ent_hdl = bf_id_allocator_allocate(ent_hdl_mgr->ent_hdl_allocator);
  if (new_ent_hdl == -1) {
    return PIPE_MAT_ENT_HDL_INVALID_HDL;
  }
  if (exm_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
    new_ent_hdl = PIPE_SET_HDL_PIPE(new_ent_hdl, exm_tbl_data->pipe_id);
  }
  return (pipe_mat_ent_hdl_t)new_ent_hdl;
}

void pipe_mgr_exm_set_ent_hdl(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                              pipe_mat_ent_hdl_t mat_ent_hdl) {
  if (exm_tbl_data == NULL) {
    return;
  }
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;
  ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;
  bf_id_allocator_set(ent_hdl_mgr->ent_hdl_allocator,
                      PIPE_GET_HDL_VAL(mat_ent_hdl));
  return;
}

static pipe_status_t pipe_mgr_exm_compute_ram_shadow_copy(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mgr_move_list_t *move_list_node,
    uint8_t **shadow_ptr_arr,
    uint32_t *num_ram_units,
    mem_id_t *mem_id_arr,
    uint8_t *wide_word_indices,
    uint8_t *vv_word_index,
    bool is_stash,
    uint32_t *ret_version_valid_bits) {
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }
  pipe_tbl_match_spec_t *match_spec =
      unpack_mat_ent_data_ms(move_list_node->data);
  pipe_action_spec_t *act_data_spec =
      unpack_mat_ent_data_as(move_list_node->data);
  int version_index = 0;
  uint32_t i = 0;
  uint32_t idx = 0;
  uint32_t num_mem_ids = 0;
  bool ram_words_updated[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD] = {false};
  (void)exm_tbl;
  (void)indirect_ptrs;
  uint8_t entry_position = 0;
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t version_valid_bits = 0;
  PIPE_MGR_MEMSET(ram_words_updated, 0, sizeof(ram_words_updated));

/* Based on the number of words that got updated as a result of this new
 * entry add, compute the mem_id_arr that needs to be returned and fill in
 * the wide-word indices accordingly. Wide-word indices will enable us to
 * pick up a "wide-word" and know which of the wide-words contain a
 * particular entry. Here is an illustration
 *  -------------------------------------------------------
 * |      0        |         1          |        2         |
 *  -------------------------------------------------------
 *
 * Suppose a wide-word is made up of 3 ram-units, and an entry is split
 * across two of those ram-units. Wide-word indices for that entry will
 * contain two entries (for example 0 and 1, if the entry is split across
 * ram-unit 0 and 1). This would enable to pick the wide-word and
 * be able to access the right words within that to operate upon the entry.
 */
#ifndef UTEST
  if (!indirect_ptrs) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  if (is_stash) {
    entry_position =
        entry_idx %
        exm_stage_info->stash->pack_format.num_entries_per_wide_word;
  } else {
    entry_position =
        entry_idx % exm_stage_info->pack_format->num_entries_per_wide_word;
  }
  /* Compute the version/valid bits. Since this is not atomic, the
   * version/valid bits for this entry is don't care. There are two sets of
   * version/valid bits per exact match entry. Set both of them to don't care.
   */
  RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                       RMT_EXM_ENTRY_VERSION_DONT_CARE,
                                       version_valid_bits);
  *ret_version_valid_bits = version_valid_bits;

  status = pipe_mgr_entry_format_tof_exm_tbl_ent_update(
      exm_tbl->dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_table_handle,
      version_valid_bits,
      match_spec,
      unpack_mat_ent_data_afun_hdl(move_list_node->data),
      &act_data_spec->act_data,
      indirect_ptrs->adt_ptr,
      indirect_ptrs->stats_ptr,
      indirect_ptrs->meter_ptr,
      indirect_ptrs->stfl_ptr,
      indirect_ptrs->sel_ptr,
      unpack_mat_ent_data_sel(move_list_node->data),
      entry_position,
      unpack_mat_ent_data_phash(move_list_node->data),
      shadow_ptr_arr,
      ram_words_updated,
      &version_index,
      is_stash);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in encoding the exact match entry for table with"
        " handle %d, stage id %d, error %s",
        __func__,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    return status;
  } else {
    unsigned j = 0;

    LOG_DBG("Encoded Exact match entry : ");

    /* Log the Encoded entry words */
    for (i = 0; i < exm_stage_info->pack_format->num_rams_in_wide_word; i++) {
      if (ram_words_updated[i] == false) {
        continue;
      }

      char buf[3 * (TOF_SRAM_UNIT_WIDTH / 8)];
      for (j = 0; j < TOF_SRAM_UNIT_WIDTH / 8; j++) {
        int n1 = *(shadow_ptr_arr[i] + j) >> 4; /* Top nibble */
        int n0 = *(shadow_ptr_arr[i] + j) & 15; /* Bottom nibble */
        buf[3 * j + 0] = (n1 < 10 ? '0' : 'A' - 10) + n1;
        buf[3 * j + 1] = (n0 < 10 ? '0' : 'A' - 10) + n0;
        buf[3 * j + 2] = ' ';
      }
      buf[sizeof buf - 1] = '\0';
      LOG_DBG("Word %d: %s", i, buf);
    }
  }

  /* Given the entry location, get the ram unit(s) associated with that entry.
   * The ram unit(s) associated with it is that of the entire wide-word.
   */
  if (!is_stash) {
    pipe_mgr_exm_get_mem_ids_for_entry(exm_tbl,
                                       exm_stage_info,
                                       entry_idx,
                                       mem_id_arr,
                                       ram_words_updated,
                                       &num_mem_ids);
  } else {
    num_mem_ids = exm_stage_info->stash->num_rams_per_stash;
  }

#else
  (void)exm_stage_info;
  (void)entry_idx;
  (void)mem_id_arr;
  (void)match_spec;
  (void)act_data_spec;
  (void)shadow_ptr_arr;
  (void)version_valid_bits;
  (void)status;
  (void)entry_position;
  (void)is_stash;
  (void)ret_version_valid_bits;
  (void)vv_word_index;
#endif /* !UTEST */

  if (wide_word_indices != NULL) {
    *vv_word_index = version_index;
    for (i = 0; i < num_mem_ids; i++) {
      if (ram_words_updated[i] == true) {
        wide_word_indices[idx] = i;
        idx++;
      }
    }
  }

  if (num_ram_units != NULL) {
    *num_ram_units = idx;
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_exm_get_mem_ids_for_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    mem_id_t *wide_word_mem_id,
    bool *ram_words_updated,
    uint32_t *num_mem_ids) {
  uint8_t hashway = 0; /* Which hashway the entry belongs to */
  uint32_t wide_word_blk_idx = 0;
  uint32_t i = 0;
  uint32_t ram_line_num = 0;
  uint8_t entry_position = 0;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_mat_ent_idx_t hashway_ent_idx = 0;
  pipe_mat_ent_idx_t accum = 0;
  uint32_t num_ram_units = 0;

  hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, entry_idx);

  for (i = 0; i < exm_stage_info->num_hash_ways; i++) {
    if (hashway == i) {
      break;
    }
    accum += exm_stage_info->hashway_data[i].num_entries;
  }
  /* For economy of expression */
  exm_pack_format = exm_stage_info->pack_format;
  exm_hashway_data = &exm_stage_info->hashway_data[hashway];
  ram_alloc_info = exm_hashway_data->ram_alloc_info;

  hashway_ent_idx = entry_idx - accum;
  ram_line_num = hashway_ent_idx / exm_pack_format->num_entries_per_wide_word;

  wide_word_blk_idx = ram_line_num / TOF_UNIT_RAM_DEPTH(exm_tbl);

  entry_position =
      entry_idx % exm_stage_info->pack_format->num_entries_per_wide_word;

  if (ram_words_updated &&
      !exm_hashway_data->ram_unit_present[entry_position]) {
    for (i = 0; i < exm_pack_format->num_rams_in_wide_word; i++) {
      if (ram_words_updated[i]) {
        num_ram_units++;
      }
    }
    exm_hashway_data->num_ram_units[entry_position] = num_ram_units;
    exm_hashway_data->ram_unit_present[entry_position] =
        (bool *)PIPE_MGR_CALLOC(exm_pack_format->num_rams_in_wide_word,
                                sizeof(bool));
    if (exm_hashway_data->ram_unit_present[entry_position] == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return;
    }
    PIPE_MGR_MEMCPY(exm_hashway_data->ram_unit_present[entry_position],
                    ram_words_updated,
                    (sizeof(bool) * exm_pack_format->num_rams_in_wide_word));
  }
  for (i = 0; i < exm_pack_format->num_rams_in_wide_word; i++) {
    wide_word_mem_id[i] =
        ram_alloc_info->tbl_word_blk[wide_word_blk_idx].mem_id[i];
  }
  if (num_mem_ids) {
    *num_mem_ids = exm_pack_format->num_rams_in_wide_word;
  }
  return;
}

uint8_t pipe_mgr_exm_get_entry_hashway(
    pipe_mgr_exm_stage_info_t *exm_stage_info, pipe_mat_ent_idx_t entry_idx) {
  uint32_t accum = 0;
  int i = 0;

  if (!exm_stage_info->hashway_data) {
    /* Return invalid value */
    return (exm_stage_info->num_hash_ways + 1);
  }

  accum = exm_stage_info->hashway_data[0].num_entries;

  for (i = 0; i < exm_stage_info->num_hash_ways; i++) {
    if (entry_idx < accum) {
      return i;
    }

    if (i != ((exm_stage_info->num_hash_ways) - 1)) {
      accum += exm_stage_info->hashway_data[i + 1].num_entries;
    }
  }

  /* An invalid hash-way would be any hash-way greater than or equal to the
   * number of hash-ways in the stage.
   */
  return (exm_stage_info->num_hash_ways);
}

void pipe_mgr_exm_deallocate_entry_hdl(pipe_mgr_exm_tbl_t *exm_tbl,
                                       pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                       pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;

  ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;

  /* Deallocate the entry handle */
  bf_id_allocator_release(ent_hdl_mgr->ent_hdl_allocator,
                          PIPE_GET_HDL_VAL(mat_ent_hdl));
  PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                     exm_tbl->name,
                     mat_ent_hdl,
                     exm_tbl_data->pipe_id,
                     0xff,
                     "Deallocating entry handle 0x%x",
                     mat_ent_hdl);

  return;
}

pipe_status_t pipe_mgr_exm_gen_lock_id(bf_dev_id_t device_id,
                                       pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                       pipe_mgr_lock_id_type_e lock_id_type,
                                       lock_id_t *lock_id_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  return pipe_mgr_exm_tbl_alloc_lock_id(exm_tbl, lock_id_type, lock_id_p);
}

pipe_status_t pipe_mgr_exm_tbl_alloc_lock_id(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_lock_id_type_e lock_id_type,
    lock_id_t *lock_id_p) {
  lock_id_t lock_id = exm_tbl->idle_lock_id_allocator++;

  PIPE_MGR_FORM_LOCK_ID(*lock_id_p, lock_id_type, lock_id);

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_dir_ent_idx(bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mat_ent_hdl_t mat_ent_hdl,
                                           bf_dev_pipe_t *pipe_id_p,
                                           dev_stage_t *stage_id_p,
                                           rmt_tbl_hdl_t *stage_table_hdl_p,
                                           uint32_t *index_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exact match table not found for table %d, device id %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  pipe_mat_ent_idx_t entry_idx =
      pipe_mgr_exm_get_entry_loc(exm_tbl, mat_ent_hdl, pipe_id_p, stage_id_p);

  if (entry_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
    LOG_ERROR(
        "%s:%d Location info for ent hdl %d, tbl 0x%x, device id %d not"
        " found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        mat_tbl_hdl,
        device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, *pipe_id_p, *stage_id_p);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm stage info for tbl 0x%x, pipe id %d, stage id %d not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        *pipe_id_p,
        *stage_id_p);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (stage_table_hdl_p) {
    *stage_table_hdl_p = exm_stage_info->stage_table_handle;
  }

  if (index_p) {
    *index_p = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, entry_idx);
  }

  return PIPE_SUCCESS;
}

bool pipe_mgr_exm_tbl_is_ent_hdl_valid(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                       pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;

  if (exm_tbl_data->pipe_id != BF_DEV_PIPE_ALL &&
      exm_tbl_data->pipe_id != PIPE_GET_HDL_PIPE(mat_ent_hdl)) {
    return false;
  }

  ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;
  if (bf_id_allocator_is_set(ent_hdl_mgr->ent_hdl_allocator,
                             PIPE_GET_HDL_VAL(mat_ent_hdl)) == 1) {
    return true;
  }

  return false;
}

pipe_status_t pipe_mgr_exm_update_lock_type(bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                            bool idle,
                                            bool stat,
                                            bool add_lock) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  PIPE_MGR_DBGCHK(idle || stat);

  pipe_mgr_lock_id_type_e ltype = exm_tbl->lock_type;

  bool idle_locked = false, stat_locked = false;

  switch (ltype) {
    case LOCK_ID_TYPE_IDLE_LOCK:
      idle_locked = true;
      break;
    case LOCK_ID_TYPE_STAT_LOCK:
      stat_locked = true;
      break;
    case LOCK_ID_TYPE_ALL_LOCK:
      idle_locked = true;
      stat_locked = true;
      break;
    default:
      idle_locked = false;
      stat_locked = false;
      break;
  }

  bool lock_stat = false, lock_idle = false;
  if (add_lock) {
    lock_stat = (stat || stat_locked) ? true : false;
    lock_idle = (idle || idle_locked) ? true : false;
  } else {
    lock_stat = (stat ? false : (stat_locked ? true : false));
    lock_idle = (idle ? false : (idle_locked ? true : false));
  }

  if (lock_idle && lock_stat) {
    exm_tbl->lock_type = LOCK_ID_TYPE_ALL_LOCK;
  } else if (lock_idle) {
    exm_tbl->lock_type = LOCK_ID_TYPE_IDLE_LOCK;
  } else if (lock_stat) {
    exm_tbl->lock_type = LOCK_ID_TYPE_STAT_LOCK;
  } else {
    exm_tbl->lock_type = LOCK_ID_TYPE_INVALID;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_update_idle_init_val(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl->idle_init_val_for_ttl_0 = idle_init_val_for_ttl_0;

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_tbl_get_placed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t *count_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  unsigned i = 0;
  uint32_t exm_count = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, for device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (exm_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric exm tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == exm_tbl->exm_tbl_data[i].pipe_id) {
      exm_count += exm_tbl->exm_tbl_data[i].num_entries_placed;
    }
  }

  *count_p = exm_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_tbl_get_programmed_entry_count(
    dev_target_t dev_tgt, pipe_mat_tbl_hdl_t mat_tbl_hdl, uint32_t *count_p) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  unsigned i = 0;
  uint32_t exm_count = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, for device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (exm_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric exm tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    if (dev_tgt.dev_pipe_id == BF_DEV_PIPE_ALL ||
        dev_tgt.dev_pipe_id == exm_tbl->exm_tbl_data[i].pipe_id) {
      exm_count += exm_tbl->exm_tbl_data[i].num_entries_occupied;
    }
  }

  *count_p = exm_count;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_entry_hdl_from_stage_idx(
    bf_dev_id_t dev_id,
    bf_dev_pipe_t pipe_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    uint8_t stage,
    uint8_t logical_tbl_id,
    pipe_mat_ent_idx_t ent_idx,
    pipe_mat_ent_hdl_t *ent_hdl) {
  unsigned int i;
  /* Look up the table based on device and table handle. */
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Confirm that this table is in the requested stage with the requested
   * logical table id. */
  for (i = 0; i < exm_tbl->mat_tbl_info->num_rmt_info; ++i) {
    if (stage == exm_tbl->mat_tbl_info->rmt_info[i].stage_id &&
        logical_tbl_id == exm_tbl->mat_tbl_info->rmt_info[i].handle) {
      break;
    }
  }
  if (i == exm_tbl->mat_tbl_info->num_rmt_info) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Look up the table instance based on the pipe-id.  Note that we are not
   * using pipe_mgr_exm_tbl_get_instance here because that API expects the
   * lowest pipe-id and here we just have a pipe-id.  So if there was a profile
   * across two pipes the pipe-id we have might not be the lowest. */
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  if (exm_tbl->symmetric) {
    exm_tbl_data = exm_tbl->exm_tbl_data;
  } else {
    for (i = 0; i < exm_tbl->num_tbls; ++i) {
      if (PIPE_BITMAP_GET(&exm_tbl->exm_tbl_data[i].pipe_bmp, pipe_id)) {
        exm_tbl_data = &exm_tbl->exm_tbl_data[i];
        break;
      }
    }
  }
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s : Could not find the exact match table data for table with"
        " handle 0x%x for device id %d pipe id %d",
        __func__,
        tbl_hdl,
        dev_id,
        pipe_id);
    PIPE_MGR_DBGCHK(exm_tbl_data);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Look up the stage info by indexing using the stage id. */
  pipe_mgr_exm_stage_info_t *exm_stage_info =
      exm_tbl_data->stage_info_ptrs[stage];
  if (!exm_stage_info) {
    LOG_ERROR(
        "%s : Could not find the exact match table stage for table with"
        " handle 0x%x for device id %d pipe id %d stage %d",
        __func__,
        tbl_hdl,
        dev_id,
        pipe_id,
        stage);
    PIPE_MGR_DBGCHK(exm_stage_info);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Map the index to an entry handle and return it. */
  pipe_mat_ent_hdl_t h =
      pipe_mgr_exm_get_ent_hdl_from_dir_addr(ent_idx, exm_stage_info);
  if (h == PIPE_MAT_ENT_HDL_INVALID_HDL) return PIPE_OBJ_NOT_FOUND;
  *ent_hdl = h;
  return PIPE_SUCCESS;
}

static pipe_status_t handle_direct_action(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_dir_ent_idx,
    pipe_mat_ent_idx_t src_dir_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_action_spec_t *action_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_action_data_spec_t *act_data_spec = &action_spec->act_data;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = 0;
  for (unsigned int i = 0; i < exm_tbl->num_adt_refs; ++i) {
    if (exm_tbl->adt_tbl_refs[i].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      adt_tbl_hdl = exm_tbl->adt_tbl_refs[i].tbl_hdl;
      break;
    }
  }
  if (0 == adt_tbl_hdl) return PIPE_SUCCESS;

  enum pipe_mat_update_type op = move_list_node->op;
  switch (op) {
    case PIPE_MAT_UPDATE_ADD:
    case PIPE_MAT_UPDATE_SET_DFLT:
      /* New entry */
      status = rmt_adt_ent_add(
          sess_hdl,
          exm_tbl->dev_id,
          adt_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          act_fn_hdl,
          act_data_spec,
          dst_dir_ent_idx,
          exm_stage_info->stage_table_handle,
          entry_info ? (&entry_info->indirect_ptrs.adt_ptr) : NULL,
          exm_tbl->hash_action /* Hash action requires atomic updates */);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in adding action data entry for entry %d, match tbl "
            "0x%x device id %d, stage id %d",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_stage_info->stage_id);
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         " Action data entry add at idx %d tbl 0x%x",
                         dst_dir_ent_idx,
                         adt_tbl_hdl);
      break;

    case PIPE_MAT_UPDATE_MOD:
      status = rmt_adt_ent_add(
          sess_hdl,
          exm_tbl->dev_id,
          adt_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          act_fn_hdl,
          act_data_spec,
          dst_dir_ent_idx,
          exm_stage_info->stage_table_handle,
          entry_info ? &entry_info->indirect_ptrs.adt_ptr : NULL,
          true);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in setting action data for entry %d, match tbl 0x%x, "
            "device id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      break;
    case PIPE_MAT_UPDATE_MOV:
      /* Entry move */
      status = rmt_adt_ent_add(
          sess_hdl,
          exm_tbl->dev_id,
          adt_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          act_fn_hdl,
          act_data_spec,
          dst_dir_ent_idx,
          exm_stage_info->stage_table_handle,
          entry_info ? &entry_info->indirect_ptrs.adt_ptr : NULL,
          false);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in moving action data entry from idx %d to %d for "
            "match tbl 0x%x, device id %d, action tbl 0x%x",
            __func__,
            __LINE__,
            src_dir_ent_idx,
            dst_dir_ent_idx,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            adt_tbl_hdl);
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         " Action data entry move from %d to %d, tbl 0x%x",
                         src_dir_ent_idx,
                         dst_dir_ent_idx,
                         adt_tbl_hdl);
      break;
    case PIPE_MAT_UPDATE_DEL:
    case PIPE_MAT_UPDATE_CLR_DFLT:
    case PIPE_MAT_UPDATE_ADD_IDLE:
      /* Supported by EXM Manager but nothing to do here... */
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_ADD_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      /* NOT supported by EXM Manager. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t handle_direct_stats(
    pipe_sess_hdl_t sess_hdl,
    pipe_stat_tbl_hdl_t stat_tbl_hdl,
    enum pipe_res_action_tag res_tag,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_dir_ent_idx,
    pipe_mat_ent_idx_t src_dir_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    bool end_of_move_add,
    pipe_stat_data_t *stat_data) {
  pipe_status_t status = PIPE_SUCCESS;
  dev_target_t dev_tgt;
  dev_tgt.dev_pipe_id = exm_tbl_data->pipe_id;
  dev_tgt.device_id = exm_tbl->dev_id;

  enum pipe_mat_update_type op = move_list_node->op;
  switch (op) {
    case PIPE_MAT_UPDATE_SET_DFLT:
    /* Fall through */
    case PIPE_MAT_UPDATE_ADD:
      if (res_tag == PIPE_RES_ACTION_TAG_ATTACHED) {
        bool hw_stats_init = end_of_move_add ? false : true;

        status = pipe_mgr_stat_mgr_add_entry(
            sess_hdl,
            dev_tgt,
            move_list_node->entry_hdl,
            stat_tbl_hdl,
            exm_stage_info->stage_id,
            dst_dir_ent_idx,
            hw_stats_init,
            stat_data,
            entry_info ? &entry_info->indirect_ptrs.stats_ptr : NULL);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in adding entry for entry hdl 0x%x, stats index %d "
              "for stat tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              move_list_node->entry_hdl,
              dst_dir_ent_idx,
              stat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          return status;
        }
        PIPE_MGR_EXM_TRACE(
            exm_tbl->dev_id,
            exm_tbl->name,
            move_list_node->entry_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            " New stat entry init for idx %d tbl 0x%x, pkts %" PRIu64
            " bytes %" PRIu64,
            dst_dir_ent_idx,
            stat_tbl_hdl,
            stat_data ? stat_data->packets : 0,
            stat_data ? stat_data->bytes : 0);
      }
      break;
    case PIPE_MAT_UPDATE_MOV:
      /* Entry move */
      status = rmt_stat_mgr_stat_ent_move(exm_tbl->dev_id,
                                          stat_tbl_hdl,
                                          exm_tbl_data->pipe_id,
                                          move_list_node->entry_hdl,
                                          exm_stage_info->stage_id,
                                          exm_stage_info->stage_id,
                                          src_dir_ent_idx,
                                          dst_dir_ent_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in moving stat entry idx %d to %d, for tbl 0x%x "
            "device id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            src_dir_ent_idx,
            dst_dir_ent_idx,
            stat_tbl_hdl,
            exm_tbl->dev_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         " Stat entry move from idx %d to %d, tbl 0x%x",
                         src_dir_ent_idx,
                         dst_dir_ent_idx,
                         stat_tbl_hdl);
      break;
    case PIPE_MAT_UPDATE_DEL:
    case PIPE_MAT_UPDATE_CLR_DFLT: {
      /* Only pass the entry handle to stats_mgr for deletion if it was added
       * to stats_mgr.  Default entries are not required to use direct stats
       * tables so stats_mgr may not have been informed of this entry handle. */
      if (!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                           move_list_node->entry_hdl) ||
          (entry_info && entry_info->indirect_ptrs.stats_ptr)) {
        status = pipe_mgr_stat_mgr_delete_entry(dev_tgt,
                                                exm_stage_info->stage_id,
                                                move_list_node->entry_hdl,
                                                dst_dir_ent_idx,
                                                stat_tbl_hdl);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in deleting stats entry for entry %d, tbl 0x%x, "
              "device id %d, err %s",
              __func__,
              __LINE__,
              move_list_node->entry_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_ASSERT(0);
          return status;
        }
        PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                           exm_tbl->name,
                           move_list_node->entry_hdl,
                           exm_tbl_data->pipe_id,
                           exm_stage_info->stage_id,
                           " Stat entry delete from idx %d, tbl 0x%x",
                           dst_dir_ent_idx,
                           stat_tbl_hdl);
      }
      break;
    }
    case PIPE_MAT_UPDATE_MOD:
      /* Set-default can land here when an existing default entry is being
       * modified.  If the original default had stats and the new default
       * doesn't we need to remove the handle from stats manager.  If the
       * original default did not have stats and the new one did then add the
       * entry handle to stats manager.
       * For non-default entries there should be no change in the resource tag.
       */
      if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                          move_list_node->entry_hdl)) {
        if (res_tag == PIPE_RES_ACTION_TAG_ATTACHED &&
            (entry_info && !entry_info->indirect_ptrs.stats_ptr)) {
          bool hw_stats_init = true;
          status =
              pipe_mgr_stat_mgr_add_entry(sess_hdl,
                                          dev_tgt,
                                          move_list_node->entry_hdl,
                                          stat_tbl_hdl,
                                          exm_stage_info->stage_id,
                                          dst_dir_ent_idx,
                                          hw_stats_init,
                                          stat_data,
                                          &entry_info->indirect_ptrs.stats_ptr);
        } else if (res_tag == PIPE_RES_ACTION_TAG_DETACHED &&
                   (entry_info && entry_info->indirect_ptrs.stats_ptr)) {
          pipe_mgr_stat_mgr_delete_entry(dev_tgt,
                                         exm_stage_info->stage_id,
                                         move_list_node->entry_hdl,
                                         dst_dir_ent_idx,
                                         stat_tbl_hdl);
          entry_info->indirect_ptrs.stats_ptr = 0;
        }
      }
      break;
    case PIPE_MAT_UPDATE_ADD_IDLE:
      /* Supported by EXM Manager but nothing to do here... */
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_ADD_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      /* NOT supported by EXM Manager. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t handle_direct_meter(
    pipe_sess_hdl_t sess_hdl,
    pipe_meter_tbl_hdl_t meter_tbl_hdl,
    enum pipe_res_action_tag res_tag,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_dir_ent_idx,
    pipe_mat_ent_idx_t src_dir_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    pipe_meter_spec_t *meter_spec,
    pipe_lpf_spec_t *lpf_spec,
    pipe_wred_spec_t *wred_spec) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_meter_impl_type_e meter_type;
  if (!pipe_mgr_get_meter_impl_type(
          meter_tbl_hdl, exm_tbl->dev_id, &meter_type)) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  enum pipe_mat_update_type op = move_list_node->op;
  switch (op) {
    case PIPE_MAT_UPDATE_ADD:
    case PIPE_MAT_UPDATE_MOD:
    case PIPE_MAT_UPDATE_SET_DFLT: {
      if (res_tag == PIPE_RES_ACTION_TAG_ATTACHED) {
        switch (meter_type) {
          case PIPE_METER_TYPE_STANDARD:
            status = rmt_meter_mgr_direct_meter_attach(
                sess_hdl,
                exm_tbl->dev_id,
                meter_tbl_hdl,
                dst_dir_ent_idx,
                exm_tbl_data->pipe_id,
                exm_stage_info->stage_id,
                meter_spec,
                entry_info ? &entry_info->indirect_ptrs.meter_ptr : NULL);
            if (status == PIPE_SUCCESS) {
              PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                                 exm_tbl->name,
                                 move_list_node->entry_hdl,
                                 exm_tbl_data->pipe_id,
                                 exm_stage_info->stage_id,
                                 " Direct meter attach at idx %d, tbl 0x%x",
                                 dst_dir_ent_idx,
                                 meter_tbl_hdl);
            }
            break;
          case PIPE_METER_TYPE_LPF:
            status = rmt_meter_mgr_direct_lpf_attach(
                sess_hdl,
                exm_tbl->dev_id,
                meter_tbl_hdl,
                dst_dir_ent_idx,
                exm_tbl_data->pipe_id,
                exm_stage_info->stage_id,
                lpf_spec,
                entry_info ? &entry_info->indirect_ptrs.meter_ptr : NULL);
            if (status == PIPE_SUCCESS) {
              PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                                 exm_tbl->name,
                                 move_list_node->entry_hdl,
                                 exm_tbl_data->pipe_id,
                                 exm_stage_info->stage_id,
                                 " Direct LPF attach at idx %d, tbl 0x%x",
                                 dst_dir_ent_idx,
                                 meter_tbl_hdl);
            }
            break;
          case PIPE_METER_TYPE_WRED:
            status = rmt_meter_mgr_direct_wred_attach(
                sess_hdl,
                exm_tbl->dev_id,
                meter_tbl_hdl,
                dst_dir_ent_idx,
                exm_tbl_data->pipe_id,
                exm_stage_info->stage_id,
                wred_spec,
                entry_info ? &entry_info->indirect_ptrs.meter_ptr : NULL);
            if (status == PIPE_SUCCESS) {
              PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                                 exm_tbl->name,
                                 move_list_node->entry_hdl,
                                 exm_tbl_data->pipe_id,
                                 exm_stage_info->stage_id,
                                 "Direct WRED attach at idx %d, tbl 0x%x",
                                 dst_dir_ent_idx,
                                 meter_tbl_hdl);
            }
            break;
          default:
            PIPE_MGR_ASSERT(0);
        }
      }
      break;
    }
    case PIPE_MAT_UPDATE_MOV:
      status = pipe_mgr_meter_mgr_move_meter_spec(sess_hdl,
                                                  exm_tbl->dev_id,
                                                  exm_tbl_data->pipe_id,
                                                  exm_stage_info->stage_id,
                                                  exm_stage_info->stage_id,
                                                  meter_tbl_hdl,
                                                  src_dir_ent_idx,
                                                  dst_dir_ent_idx);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in moving meter spec from idx %d to %d for meter tbl "
            "0x%x, stage id %d, device id %d, err %s",
            __func__,
            __LINE__,
            src_dir_ent_idx,
            dst_dir_ent_idx,
            meter_tbl_hdl,
            exm_stage_info->stage_id,
            exm_tbl->dev_id,
            pipe_str_err(status));
        return status;
      }
      break;
    case PIPE_MAT_UPDATE_DEL:
    case PIPE_MAT_UPDATE_CLR_DFLT:
    case PIPE_MAT_UPDATE_ADD_IDLE:
      /* Supported by EXM Manager but nothing to do here... */
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_ADD_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      /* NOT supported by EXM Manager. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t handle_direct_stful(
    pipe_sess_hdl_t sess_hdl,
    pipe_stful_tbl_hdl_t stful_tbl_hdl,
    enum pipe_res_action_tag res_tag,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_dir_ent_idx,
    pipe_mat_ent_idx_t src_dir_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_stful_mem_spec_t *stful_spec) {
  pipe_status_t status = PIPE_SUCCESS;

  enum pipe_mat_update_type op = move_list_node->op;
  bool new_add = false;
  switch (op) {
    case PIPE_MAT_UPDATE_ADD:
    case PIPE_MAT_UPDATE_SET_DFLT:
      new_add = true;
    /* Fall Through */
    case PIPE_MAT_UPDATE_MOD:
      if (res_tag == PIPE_RES_ACTION_TAG_ATTACHED) {
        if (new_add || (entry_info &&
                        entry_info->direct_stful_seq_nu !=
                            unpack_mat_ent_data_stful(move_list_node->data))) {
          if (entry_info) {
            entry_info->direct_stful_seq_nu =
                unpack_mat_ent_data_stful(move_list_node->data);
          }
          status =
              pipe_mgr_stful_direct_word_write_at(sess_hdl,
                                                  exm_tbl->dev_id,
                                                  exm_tbl->mat_tbl_hdl,
                                                  move_list_node->entry_hdl,
                                                  exm_tbl_data->pipe_id,
                                                  exm_stage_info->stage_id,
                                                  dst_dir_ent_idx,
                                                  stful_spec,
                                                  0);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in programming stateful spec for entry 0x%x, tbl"
                " 0x%x stful tbl 0x%x, device id %d, stage id %d, err %s",
                __func__,
                __LINE__,
                move_list_node->entry_hdl,
                exm_tbl->mat_tbl_hdl,
                stful_tbl_hdl,
                exm_tbl->dev_id,
                exm_stage_info->stage_id,
                pipe_str_err(status));
            return status;
          }
        }
        /* Get the addr of the installed entry (Needed for default entry) */
        if (entry_info) {
          status = pipe_mgr_stful_get_indirect_ptr(
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_stage_info->stage_id,
              act_fn_hdl,
              stful_tbl_hdl,
              dst_dir_ent_idx,
              &entry_info->indirect_ptrs.stfl_ptr);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Error in getting stful indirect ptr for tbl 0x%x entry "
                "0x%x, stful tbl 0x%x, device id %d, err %s",
                __func__,
                __LINE__,
                exm_tbl->mat_tbl_hdl,
                move_list_node->entry_hdl,
                stful_tbl_hdl,
                exm_tbl->dev_id,
                pipe_str_err(status));
            return status;
          }
        }
        PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                           exm_tbl->name,
                           move_list_node->entry_hdl,
                           exm_tbl_data->pipe_id,
                           exm_stage_info->stage_id,
                           "Stateful entry add at idx %d, tbl 0x%x",
                           dst_dir_ent_idx,
                           stful_tbl_hdl);
      }
      break;
    case PIPE_MAT_UPDATE_MOV:
      /* Entry move */
      status = pipe_mgr_stful_move(sess_hdl,
                                   exm_tbl->dev_id,
                                   stful_tbl_hdl,
                                   exm_tbl_data->pipe_id,
                                   exm_stage_info->stage_id,
                                   exm_stage_info->stage_id,
                                   src_dir_ent_idx,
                                   dst_dir_ent_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in moving stateful shadow from idx %d to %d stage id "
            "%d for stful tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            src_dir_ent_idx,
            dst_dir_ent_idx,
            exm_stage_info->stage_id,
            stful_tbl_hdl,
            exm_tbl->dev_id,
            pipe_str_err(status));
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         "Stateful entry move from idx %d to %d, tbl 0x%x",
                         src_dir_ent_idx,
                         dst_dir_ent_idx,
                         stful_tbl_hdl);
      break;
    case PIPE_MAT_UPDATE_DEL:
    case PIPE_MAT_UPDATE_CLR_DFLT:
      /* Only pass the entry handle to stful_mgr for deletion if it was added
       * to stful_mgr.  Default entries are not required to use direct stful
       * tables so stful_mgr may not have been informed of this entry handle. */
      if (!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                           move_list_node->entry_hdl) ||
          (entry_info && entry_info->indirect_ptrs.stfl_ptr)) {
        status = pipe_mgr_stful_dir_ent_del(exm_tbl->dev_id,
                                            stful_tbl_hdl,
                                            exm_tbl_data->pipe_id,
                                            move_list_node->entry_hdl);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in deleting stful entry for entry %d, tbl 0x%x, "
              "device id %d, err %s",
              __func__,
              __LINE__,
              move_list_node->entry_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          return status;
        }
      }
      break;
    case PIPE_MAT_UPDATE_ADD_IDLE:
      /* Supported by EXM Manager but nothing to do here... */
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_ADD_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      /* NOT supported by EXM Manager. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t handle_direct_idle(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_dir_ent_idx,
    pipe_mat_ent_idx_t src_dir_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    bool end_of_move_add) {
  pipe_status_t status = PIPE_SUCCESS;
  enum pipe_mat_update_type op = move_list_node->op;
  switch (op) {
    case PIPE_MAT_UPDATE_ADD: {
      uint32_t ttl = unpack_mat_ent_data_ttl(move_list_node->data);
      status = rmt_idle_add_entry(sess_hdl,
                                  exm_tbl->dev_id,
                                  exm_tbl->mat_tbl_hdl,
                                  move_list_node->entry_hdl,
                                  exm_tbl_data->pipe_id,
                                  exm_stage_info->stage_id,
                                  exm_stage_info->stage_table_handle,
                                  dst_dir_ent_idx,
                                  ttl,
                                  end_of_move_add,
                                  0);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in adding idle entry at idx %d, for entry 0x%x, tbl "
            "0x%x device id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            dst_dir_ent_idx,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         "Idle entry add at idx %d",
                         dst_dir_ent_idx);
      break;
    }
    case PIPE_MAT_UPDATE_MOV:
      /* Entry move */
      /* Call the idle time API to move the idle time entry */
      status = rmt_idle_move_entry(sess_hdl,
                                   exm_tbl->dev_id,
                                   exm_tbl->mat_tbl_hdl,
                                   move_list_node->entry_hdl,
                                   exm_tbl_data->pipe_id,
                                   exm_stage_info->stage_id,
                                   exm_stage_info->stage_table_handle,
                                   exm_stage_info->stage_id,
                                   exm_stage_info->stage_table_handle,
                                   src_dir_ent_idx,
                                   dst_dir_ent_idx,
                                   0);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in moving idle time entry for entry %d from %d to %d, "
            "tbl %d, device id %d, pipe id %d stage id %d, err %s",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            src_dir_ent_idx,
            dst_dir_ent_idx,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         "Idle entry move from idx %d to %d",
                         src_dir_ent_idx,
                         dst_dir_ent_idx);
      break;
    case PIPE_MAT_UPDATE_DEL:
      status = rmt_idle_delete_entry(sess_hdl,
                                     exm_tbl->dev_id,
                                     exm_tbl->mat_tbl_hdl,
                                     move_list_node->entry_hdl,
                                     exm_tbl_data->pipe_id,
                                     exm_stage_info->stage_id,
                                     exm_stage_info->stage_table_handle,
                                     dst_dir_ent_idx,
                                     0);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in deleting idle entry for entry %d, tbl 0x%x, pipe "
            "id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        PIPE_MGR_ASSERT(0);
        return status;
      }
      PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                         exm_tbl->name,
                         move_list_node->entry_hdl,
                         exm_tbl_data->pipe_id,
                         exm_stage_info->stage_id,
                         "Idle entry delete from idx %d",
                         dst_dir_ent_idx);
      break;
    case PIPE_MAT_UPDATE_SET_DFLT:
    case PIPE_MAT_UPDATE_MOD:
    case PIPE_MAT_UPDATE_CLR_DFLT:
    case PIPE_MAT_UPDATE_ADD_IDLE:
      /* Supported by EXM Manager but nothing to do here... */
      break;
    case PIPE_MAT_UPDATE_MOV_MOD:
    case PIPE_MAT_UPDATE_ADD_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI:
    case PIPE_MAT_UPDATE_MOV_MULTI_MOD:
      /* NOT supported by EXM Manager. */
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_handle_direct_resources(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t dst_stage_ent_idx,
    pipe_mat_ent_idx_t src_stage_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    bool end_of_move_add,
    bool update_action,
    bool is_recovery) {
  /* Since we are handling direct resources here let's make sure we have
   * actual indexes for them. */
  PIPE_MGR_DBGCHK(dst_stage_ent_idx != PIPE_MGR_EXM_UNUSED_ENTRY_IDX);
  PIPE_MGR_DBGCHK(src_stage_ent_idx != PIPE_MGR_EXM_UNUSED_ENTRY_IDX);
  PIPE_MGR_DBGCHK(dst_stage_ent_idx != PIPE_MGR_EXM_UNUSED_STASH_ENTRY_IDX);
  PIPE_MGR_DBGCHK(src_stage_ent_idx != PIPE_MGR_EXM_UNUSED_STASH_ENTRY_IDX);
  pipe_status_t status = PIPE_SUCCESS;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_action_spec_t *action_spec = NULL;
  pipe_mat_ent_idx_t src_dir_ent_idx = 0;
  pipe_mat_ent_idx_t dst_dir_ent_idx = 0;
  enum pipe_mat_update_type op = move_list_node->op;

  /* Delete and Clear-Default operations might not have the MAT entry data
   * populated so check for this before taking the action spec and action
   * handle from it. */
  if (move_list_node->data) {
    action_spec = unpack_mat_ent_data_as(move_list_node->data);
    act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_list_node->data);
  }

  if (op == PIPE_MAT_UPDATE_MOV) {
    src_dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, src_stage_ent_idx);
  }

  dst_dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
      exm_tbl, exm_stage_info, dst_stage_ent_idx);

  if (update_action) {
    status = handle_direct_action(sess_hdl,
                                  exm_tbl,
                                  exm_tbl_data,
                                  exm_stage_info,
                                  dst_dir_ent_idx,
                                  src_dir_ent_idx,
                                  move_list_node,
                                  entry_info,
                                  act_fn_hdl,
                                  action_spec);
    if (status != PIPE_SUCCESS) return status;
  }

  if (exm_tbl->restore_ent_node) {
    // If in restore mode, just restore the indices.
    // The resource tables will restore themselves.
    if (entry_info == NULL) {
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    restore_exm_addr_resources(&entry_info->indirect_ptrs,
                               exm_tbl->restore_ent_node);
    return PIPE_SUCCESS;
  }

  if (move_list_node->entry_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
    /* Don't do direct resource management for hash-action default entries,
     * since its not an actual match-entry. We use the entry add code-path to
     * install the default action into the action RAM. That use case is
     * indicated by an invalid entry handle.
     */
    return PIPE_SUCCESS;
  }
  if (is_recovery) {
    /* Don't do direct resource management for entries that are being recovered.
     */
    return PIPE_SUCCESS;
  }

  /* Handle direct stats. */
  for (uint32_t i = 0; i < exm_tbl->num_stat_tbl_refs; ++i) {
    if (exm_tbl->stat_tbl_refs[i].ref_type != PIPE_TBL_REF_TYPE_DIRECT)
      continue;
    /* Get the right resources table index. */
    int rsrc_idx;
    enum pipe_res_action_tag res_tag = PIPE_RES_ACTION_TAG_NO_CHANGE;
    pipe_stat_data_t *st_data = NULL;
    if (action_spec) { /* Delete cases do not need to pass the action spec. */
      for (rsrc_idx = 0; rsrc_idx < action_spec->resource_count; rsrc_idx++) {
        if (action_spec->resources[rsrc_idx].tbl_hdl ==
            exm_tbl->stat_tbl_refs[i].tbl_hdl) {
          res_tag = action_spec->resources[rsrc_idx].tag;
          st_data = &action_spec->resources[rsrc_idx].data.counter;
          break;
        }
      }
    }
    status = handle_direct_stats(sess_hdl,
                                 exm_tbl->stat_tbl_refs[i].tbl_hdl,
                                 res_tag,
                                 exm_tbl,
                                 exm_tbl_data,
                                 exm_stage_info,
                                 dst_dir_ent_idx,
                                 src_dir_ent_idx,
                                 move_list_node,
                                 entry_info,
                                 end_of_move_add,
                                 st_data);
    if (status != PIPE_SUCCESS) return status;
  }

  /* Handle direct meters. */
  for (uint32_t i = 0; i < exm_tbl->num_meter_tbl_refs; ++i) {
    if (exm_tbl->meter_tbl_refs[i].ref_type != PIPE_TBL_REF_TYPE_DIRECT)
      continue;
    pipe_meter_spec_t *meter_spec = NULL;
    pipe_lpf_spec_t *lpf_spec = NULL;
    pipe_wred_spec_t *wred_spec = NULL;
    enum pipe_res_action_tag res_tag = PIPE_RES_ACTION_TAG_NO_CHANGE;
    if (action_spec) { /* Delete cases do not need to pass the action spec. */
      for (int j = 0; j < action_spec->resource_count; ++j) {
        if (exm_tbl->meter_tbl_refs[i].tbl_hdl ==
            action_spec->resources[j].tbl_hdl) {
          res_tag = action_spec->resources[j].tag;
          meter_spec = &action_spec->resources[j].data.meter;
          lpf_spec = &action_spec->resources[j].data.lpf;
          wred_spec = &action_spec->resources[j].data.red;
          break;
        }
      }
    }
    status = handle_direct_meter(sess_hdl,
                                 exm_tbl->meter_tbl_refs[i].tbl_hdl,
                                 res_tag,
                                 exm_tbl,
                                 exm_tbl_data,
                                 exm_stage_info,
                                 dst_dir_ent_idx,
                                 src_dir_ent_idx,
                                 move_list_node,
                                 entry_info,
                                 meter_spec,
                                 lpf_spec,
                                 wred_spec);
    if (status != PIPE_SUCCESS) return status;
  }

  /* Handle direct stateful. */
  for (uint32_t i = 0; i < exm_tbl->num_stful_tbl_refs; ++i) {
    if (exm_tbl->stful_tbl_refs[i].ref_type != PIPE_TBL_REF_TYPE_DIRECT)
      continue;

    pipe_stful_mem_spec_t *stful_spec = NULL;
    enum pipe_res_action_tag res_tag = PIPE_RES_ACTION_TAG_NO_CHANGE;
    if (action_spec) { /* Delete cases do not need to pass the action spec. */
      for (int j = 0; j < action_spec->resource_count; ++j) {
        if (exm_tbl->stful_tbl_refs[i].tbl_hdl ==
            action_spec->resources[j].tbl_hdl) {
          res_tag = action_spec->resources[j].tag;
          stful_spec = &action_spec->resources[j].data.stful;
          break;
        }
      }
    }
    status = handle_direct_stful(sess_hdl,
                                 exm_tbl->stful_tbl_refs[i].tbl_hdl,
                                 res_tag,
                                 exm_tbl,
                                 exm_tbl_data,
                                 exm_stage_info,
                                 dst_dir_ent_idx,
                                 src_dir_ent_idx,
                                 move_list_node,
                                 entry_info,
                                 act_fn_hdl,
                                 stful_spec);
    if (status != PIPE_SUCCESS) return status;
  }

  /* Finally handle idletime as well. */
  if (exm_tbl->idle_present == true) {
    status = handle_direct_idle(sess_hdl,
                                exm_tbl,
                                exm_tbl_data,
                                exm_stage_info,
                                dst_dir_ent_idx,
                                src_dir_ent_idx,
                                move_list_node,
                                end_of_move_add);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_attach_indirect_resources(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    bool update_action,
    bool update_atomic,
    bool is_recovery) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_action_spec_t *action_spec = NULL;
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_idx_t logical_action_idx;
  action_spec = unpack_mat_ent_data_as(move_list_node->data);
  act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_list_node->data);
  logical_action_idx = move_list_node->logical_action_idx;
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }

  if (update_action) {
    if (exm_tbl->num_sel_tbl_refs != 0 && IS_ACTION_SPEC_SEL_GRP(action_spec)) {
      pipe_sel_tbl_hdl_t sel_tbl_hdl = exm_tbl->sel_tbl_refs[0].tbl_hdl;
      if (!indirect_ptrs) {
        LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
        return PIPE_INVALID_ARG;
      }
      status =
          pipe_mgr_sel_logical_idx_to_vaddr(exm_tbl->dev_id,
                                            sel_tbl_hdl,
                                            exm_stage_info->stage_id,
                                            move_list_node->logical_sel_idx,
                                            &indirect_ptrs->sel_ptr);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting selection address for idx %d, for entry "
            "%d, "
            "match tbl 0x%x, device id %d, pipe id %d, stage id %d, sel tbl "
            "0x%x, err %s",
            __func__,
            __LINE__,
            move_list_node->logical_sel_idx,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            sel_tbl_hdl,
            pipe_str_err(status));
        PIPE_MGR_DBGCHK(0);
        return status;
      }
      pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
      status = rmt_adt_ent_get_addr(exm_tbl->dev_id,
                                    exm_tbl_data->pipe_id,
                                    adt_tbl_hdl,
                                    exm_stage_info->stage_id,
                                    logical_action_idx,
                                    &indirect_ptrs->adt_ptr);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in getting action address for idx %d, for entry %d, "
            "match tbl 0x%x, device id %d, pipe id %d, stage id %d, action tbl "
            "0x%x, err %s",
            __func__,
            __LINE__,
            logical_action_idx,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            adt_tbl_hdl,
            pipe_str_err(status));
        PIPE_MGR_DBGCHK(0);
        return status;
      }
    } else if (exm_tbl->num_adt_refs > 0 && exm_tbl->adt_tbl_refs[0].ref_type ==
                                                PIPE_TBL_REF_TYPE_INDIRECT) {
      if (!indirect_ptrs) {
        LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
        return PIPE_INVALID_ARG;
      }

      if (exm_tbl->restore_ent_node) {
        cJSON *ptrs_node =
            cJSON_GetObjectItem(exm_tbl->restore_ent_node, "ptrs");
        indirect_ptrs->adt_ptr =
            cJSON_GetObjectItem(ptrs_node, "action")->valuedouble;
      } else {
        pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
        pipe_adt_ent_hdl_t adt_ent_hdl = action_spec->adt_ent_hdl;
        pipe_adt_ent_idx_t entry_idx = logical_action_idx;
        if (!adt_ent_hdl) {
          /* No ADT entry, currently this can only happen for default entries
           * but perhaps in the future normal match entries which do not require
           * action entries can also have a zero ADT entry handle. */
          indirect_ptrs->adt_ptr = 0;
        } else if (!is_recovery) {
          status = rmt_adt_ent_activate_stage(sess_hdl,
                                              exm_tbl->dev_id,
                                              exm_tbl_data->pipe_id,
                                              adt_tbl_hdl,
                                              adt_ent_hdl,
                                              exm_stage_info->stage_id,
                                              entry_idx,
                                              &indirect_ptrs->adt_ptr,
                                              update_atomic);
        }
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in activating action entry hdl %d for match tbl "
              "0x%x, device id %d, pipe id %d, stage id %d, action tbl 0x%x, "
              "err %s",
              __func__,
              __LINE__,
              adt_ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_stage_info->stage_id,
              adt_tbl_hdl,
              pipe_str_err(status));
          return status;
        }
      }
    }
  }

  /* Stats, Meters, Stful address */
  if (exm_tbl->restore_ent_node) {
    restore_exm_addr_resources(indirect_ptrs, exm_tbl->restore_ent_node);
  } else {
    int i = 0;
    pipe_hdl_type_t hdl_type;
    for (i = 0; i < action_spec->resource_count; i++) {
      hdl_type = PIPE_GET_HDL_TYPE(action_spec->resources[i].tbl_hdl);
      switch (hdl_type) {
        case PIPE_HDL_TYPE_STAT_TBL:
          if (exm_tbl->num_stat_tbl_refs > 0 &&
              exm_tbl->stat_tbl_refs[0].ref_type ==
                  PIPE_TBL_REF_TYPE_INDIRECT) {
            pipe_stat_tbl_hdl_t stat_tbl_hdl =
                exm_tbl->stat_tbl_refs[0].tbl_hdl;
            pipe_stat_ent_idx_t stat_ent_idx =
                action_spec->resources[i].tbl_idx;
            if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
              if (!indirect_ptrs) {
                LOG_ERROR(
                    "%s:%d Null pointer arguments passed", __func__, __LINE__);
                return PIPE_INVALID_ARG;
              }
              status = rmt_stat_mgr_stat_ent_attach(exm_tbl->dev_id,
                                                    exm_tbl_data->pipe_id,
                                                    exm_stage_info->stage_id,
                                                    stat_tbl_hdl,
                                                    stat_ent_idx,
                                                    &indirect_ptrs->stats_ptr);
              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in attaching stats idx %d, for entry %d, "
                    "match "
                    "tbl 0x%x, device id %d, pipe id %d, stage id %d, stat tbl "
                    "0x%x, err %s",
                    __func__,
                    __LINE__,
                    stat_ent_idx,
                    move_list_node->entry_hdl,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    exm_tbl_data->pipe_id,
                    exm_stage_info->stage_id,
                    stat_tbl_hdl,
                    pipe_str_err(status));
                PIPE_MGR_DBGCHK(0);
                return status;
              }
            }
          }
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          if (exm_tbl->num_meter_tbl_refs > 0 &&
              exm_tbl->meter_tbl_refs[0].ref_type ==
                  PIPE_TBL_REF_TYPE_INDIRECT) {
            pipe_meter_tbl_hdl_t meter_tbl_hdl =
                exm_tbl->meter_tbl_refs[0].tbl_hdl;
            pipe_meter_idx_t meter_idx = action_spec->resources[i].tbl_idx;
            if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
              if (!indirect_ptrs) {
                LOG_ERROR(
                    "%s:%d Null pointer arguments passed", __func__, __LINE__);
                return PIPE_INVALID_ARG;
              }
              status = rmt_meter_mgr_meter_attach(exm_tbl->dev_id,
                                                  exm_tbl_data->pipe_id,
                                                  exm_stage_info->stage_id,
                                                  meter_tbl_hdl,
                                                  meter_idx,
                                                  &indirect_ptrs->meter_ptr);
              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in attaching meter idx %d, for entry %d, "
                    "match "
                    "tbl 0x%x, device id %d, pipe id %d, stage id %d, stat tbl "
                    "0x%x, err %s",
                    __func__,
                    __LINE__,
                    meter_idx,
                    move_list_node->entry_hdl,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    exm_tbl_data->pipe_id,
                    exm_stage_info->stage_id,
                    meter_tbl_hdl,
                    pipe_str_err(status));
                PIPE_MGR_DBGCHK(0);
                return status;
              }
            }
          }
          break;
        case PIPE_HDL_TYPE_STFUL_TBL:
          if (exm_tbl->num_stful_tbl_refs > 0 &&
              exm_tbl->stful_tbl_refs[0].ref_type ==
                  PIPE_TBL_REF_TYPE_INDIRECT) {
            pipe_stful_tbl_hdl_t stful_tbl_hdl =
                exm_tbl->stful_tbl_refs[0].tbl_hdl;
            pipe_stful_mem_idx_t stful_idx = action_spec->resources[i].tbl_idx;
            if (action_spec->resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
              if (!indirect_ptrs) {
                LOG_ERROR(
                    "%s:%d Null pointer arguments passed", __func__, __LINE__);
                return PIPE_INVALID_ARG;
              }
              status =
                  pipe_mgr_stful_get_indirect_ptr(exm_tbl->dev_id,
                                                  exm_tbl_data->pipe_id,
                                                  exm_stage_info->stage_id,
                                                  act_fn_hdl,
                                                  stful_tbl_hdl,
                                                  stful_idx,
                                                  &indirect_ptrs->stfl_ptr);
              if (status != PIPE_SUCCESS) {
                LOG_ERROR(
                    "%s:%d Error in attaching stful idx %d, for entry %d, "
                    "match tbl 0x%x, device id %d, pipe id %d, stage id %d, "
                    "stat tbl 0x%x, err %s",
                    __func__,
                    __LINE__,
                    stful_idx,
                    move_list_node->entry_hdl,
                    exm_tbl->mat_tbl_hdl,
                    exm_tbl->dev_id,
                    exm_tbl_data->pipe_id,
                    exm_stage_info->stage_id,
                    stful_tbl_hdl,
                    pipe_str_err(status));
                PIPE_MGR_DBGCHK(0);
                return status;
              }
            }
          }
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          return PIPE_INVALID_ARG;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_detach_indirect_resources(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_phy_entry_info_t *entry_info,
    bool update_action) {
  pipe_status_t status = PIPE_SUCCESS;
  if (exm_tbl->hash_action) {
    return PIPE_SUCCESS;
  }
  /* Action */
  if (update_action) {
    if (!entry_info->selector_enabled) {
      if (exm_tbl->num_adt_refs > 0 &&
          exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT &&
          entry_info->indirect_ptrs.adt_ptr) {
        pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
        status = rmt_adt_ent_deactivate_stage(exm_tbl_data->pipe_id,
                                              exm_tbl->dev_id,
                                              adt_tbl_hdl,
                                              entry_info->adt_ent_hdl,
                                              exm_stage_info->stage_id,
                                              entry_info->action_idx);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d Pipe %x Tbl %s (0x%x) stage %d, error %s removing "
              "ADT mbr %d (0x%x) from ADT 0x%x",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_stage_info->stage_id,
              pipe_str_err(status),
              entry_info->adt_ent_hdl,
              entry_info->adt_ent_hdl,
              adt_tbl_hdl);
          PIPE_MGR_DBGCHK(0);
          return status;
        }
        entry_info->indirect_ptrs.adt_ptr = 0;
      }
    } else {
      PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs > 0);
      entry_info->indirect_ptrs.sel_ptr = 0;
      entry_info->indirect_ptrs.sel_len = 0;
    }
  }

  if (exm_tbl->num_stat_tbl_refs > 0 &&
      exm_tbl->stat_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    entry_info->indirect_ptrs.stats_ptr = 0;
  }
  if (exm_tbl->num_meter_tbl_refs > 0 &&
      exm_tbl->meter_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    entry_info->indirect_ptrs.meter_ptr = 0;
  }
  if (exm_tbl->num_stful_tbl_refs > 0 &&
      exm_tbl->stful_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    entry_info->indirect_ptrs.stfl_ptr = 0;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_lock_dir_tbls(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  pipe_status_t status = PIPE_SUCCESS;
  bool idle_locked = false;
  bool stats_locked = false;
  lock_id_t lock_id;
  PIPE_MGR_DBGCHK(exm_stage_info->idle_locked == false);
  PIPE_MGR_DBGCHK(exm_stage_info->stats_locked == false);
  if (exm_tbl->lock_type == LOCK_ID_TYPE_INVALID) {
    return PIPE_SUCCESS;
  }
  status =
      pipe_mgr_exm_tbl_alloc_lock_id(exm_tbl, exm_tbl->lock_type, &lock_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in generating lock id for tbl 0x%x"
        " device id %d, stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_stage_info->stage_id);
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  status = pipe_mgr_exm_issue_lock_instr(sess_hdl,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         lock_id,
                                         &stats_locked,
                                         &idle_locked);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in issuing lock instruction for tbl 0x%x, device id %d, "
        "pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  if (idle_locked) {
    /* Call the idle time lock API to inform the idle time manager
     * of the table lock.
     */
    status = rmt_idle_tbl_lock(sess_hdl,
                               exm_tbl->dev_id,
                               exm_tbl->mat_tbl_hdl,
                               lock_id,
                               exm_tbl_data->pipe_id,
                               exm_stage_info->stage_id,
                               exm_stage_info->stage_table_handle,
                               0);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in locking idle table for mat tbl 0x%x"
          " device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
    exm_stage_info->idle_locked = true;
    exm_stage_info->idle_tbl_lock_id = lock_id;
    PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                       exm_tbl->name,
                       -1,
                       exm_tbl_data->pipe_id,
                       exm_stage_info->stage_id,
                       "Locking idle table with lock id 0x%x",
                       lock_id);
  }

  if (stats_locked) {
    /* Call the stats lock API to inform the stats table manager
     * of the table lock.
     */
    dev_target_t dev_tgt;
    dev_tgt.device_id = exm_tbl->dev_id;
    dev_tgt.dev_pipe_id = exm_tbl_data->pipe_id;
    pipe_stat_tbl_hdl_t stat_tbl_hdl;
    stat_tbl_hdl = exm_tbl->stat_tbl_refs[0].tbl_hdl;
    status = rmt_stat_mgr_tbl_lock(
        dev_tgt, stat_tbl_hdl, exm_stage_info->stage_id, lock_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in locking stats table for mat tbl 0x%x"
          " device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
    exm_stage_info->stats_locked = true;
    exm_stage_info->stats_tbl_lock_id = lock_id;
    PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                       exm_tbl->name,
                       -1,
                       exm_tbl_data->pipe_id,
                       exm_stage_info->stage_id,
                       "Locking stats table with lock id 0x%x",
                       lock_id);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_unlock_dir_tbls(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  if (exm_tbl->lock_type == LOCK_ID_TYPE_INVALID) {
    return PIPE_SUCCESS;
  }
  pipe_status_t status = PIPE_SUCCESS;
  bool unlock_stats = false;
  bool unlock_idle = false;
  lock_id_t lock_id;
  PIPE_MGR_DBGCHK(exm_stage_info->idle_locked || exm_stage_info->stats_locked);
  /* Passing either of the lock ids. Both are the same */
  if (exm_stage_info->idle_locked) {
    lock_id = exm_stage_info->idle_tbl_lock_id;
  } else if (exm_stage_info->stats_locked) {
    lock_id = exm_stage_info->stats_tbl_lock_id;
  } else {
    PIPE_MGR_DBGCHK(0);
    lock_id = PIPE_MGR_INVALID_LOCK_ID;
  }
  status = pipe_mgr_exm_issue_unlock_instr(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           lock_id,
                                           &unlock_stats,
                                           &unlock_idle);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in issuing unlock instr for tbl 0x%x device id %d, pipe "
        "id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  if (unlock_idle) {
    /* Call the idle time lock API to inform the idle time manager
     * of the table lock.
     */
    PIPE_MGR_DBGCHK(exm_stage_info->idle_locked);
    status = rmt_idle_tbl_unlock(sess_hdl,
                                 exm_tbl->dev_id,
                                 exm_tbl->mat_tbl_hdl,
                                 lock_id,
                                 exm_tbl_data->pipe_id,
                                 exm_stage_info->stage_id,
                                 exm_stage_info->stage_table_handle,
                                 0);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in unlocking idle table for mat tbl 0x%x"
          " device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
    exm_stage_info->idle_locked = false;
    exm_stage_info->idle_tbl_lock_id = PIPE_MGR_INVALID_LOCK_ID;
    PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                       exm_tbl->name,
                       -1,
                       exm_tbl_data->pipe_id,
                       exm_stage_info->stage_id,
                       "Unlocking idle table with lock id 0x%x",
                       lock_id);
  }

  if (unlock_stats) {
    PIPE_MGR_DBGCHK(exm_stage_info->stats_locked);
    dev_target_t dev_tgt;
    dev_tgt.device_id = exm_tbl->dev_id;
    dev_tgt.dev_pipe_id = exm_tbl_data->pipe_id;
    pipe_stat_tbl_hdl_t stat_tbl_hdl;
    stat_tbl_hdl = exm_tbl->stat_tbl_refs[0].tbl_hdl;
    /* Call the stats lock API to inform the stats table manager
     * of the table lock.
     */
    status = rmt_stat_mgr_tbl_unlock(
        dev_tgt, stat_tbl_hdl, exm_stage_info->stage_id, lock_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in unlocking stats table for mat tbl 0x%x"
          " device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
    exm_stage_info->stats_locked = false;
    exm_stage_info->stats_tbl_lock_id = PIPE_MGR_INVALID_LOCK_ID;
    PIPE_MGR_EXM_TRACE(exm_tbl->dev_id,
                       exm_tbl->name,
                       -1,
                       exm_tbl_data->pipe_id,
                       exm_stage_info->stage_id,
                       "Unlocking stats table with lock id 0x%x",
                       lock_id);
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_reset_idle(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      bf_dev_pipe_t pipe_id,
                                      uint8_t stage_id,
                                      mem_id_t mem_id,
                                      uint32_t mem_offset) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t msts = BF_MAP_OK;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  rmt_tbl_info_t *rmt_info = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mat_ent_idx_t ent_idx = 0;
  pipe_mat_ent_hdl_t ent_hdl = 0;
  uint32_t i = 0, j = 0;
  uint32_t entries_per_word = 0;
  uint32_t ttl;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (!exm_tbl->idle_present) {
    return PIPE_SUCCESS;
  }
  mat_tbl_info = exm_tbl->mat_tbl_info;

  if (exm_tbl->symmetric) {
    exm_tbl_data = exm_tbl->exm_tbl_data;
  } else {
    for (i = 0; i < exm_tbl->num_tbls; ++i) {
      if (PIPE_BITMAP_GET(&exm_tbl->exm_tbl_data[i].pipe_bmp, pipe_id)) {
        exm_tbl_data = &exm_tbl->exm_tbl_data[i];
        break;
      }
    }
  }
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Could not find the exact match table data for table with"
        " handle 0x%x for device id %d pipe id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        device_id,
        pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_stage_info = exm_tbl_data->stage_info_ptrs[stage_id];
  if (!exm_stage_info) {
    LOG_ERROR(
        "%s:%d Could not find the exact match table stage for table with"
        " handle 0x%x for device id %d pipe id %d stage %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        device_id,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < mat_tbl_info->num_rmt_info; ++i) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    if (rmt_info->stage_id == stage_id &&
        rmt_info->type == RMT_TBL_TYPE_IDLE_TMO) {
      entries_per_word = rmt_info->pack_format.entries_per_tbl_word;
      for (j = 0; j < rmt_info->bank_map->num_tbl_word_blks; j++) {
        if (mem_id == rmt_info->bank_map->tbl_word_blk[j].mem_id[0]) {
          ent_idx += mem_offset * entries_per_word;
          break;
        } else {
          ent_idx += TOF_MAP_RAM_UNIT_DEPTH * entries_per_word;
        }
      }
      break;
    }
  }
  if (i == mat_tbl_info->num_rmt_info ||
      j == rmt_info->bank_map->num_tbl_word_blks) {
    LOG_ERROR("%s:%d Idletime rmt info not found for exm tbl 0x%x device %d",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  sts = pipe_mgr_exm_lock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in locking direct addressed tables for tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(sts));
    return sts;
  }

  for (i = 0; i < entries_per_word; i++) {
    ent_hdl =
        pipe_mgr_exm_get_ent_hdl_from_dir_addr(ent_idx + i, exm_stage_info);
    if (ent_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
      // No valid entry at this index, move onto the next one
      continue;
    }
    if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, ent_hdl)) {
      // Default entries do not have idletime state
      continue;
    }

    msts = bf_map_get(
        &exm_tbl_data->entry_phy_info_htbl, ent_hdl, (void **)&entry_info);
    if (msts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in getting exm entry for hdl %d, tbl 0x%x, pipe id %d "
          "device id %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          pipe_id,
          device_id);
      return PIPE_OBJ_NOT_FOUND;
    }

    sts = pipe_mgr_idle_get_init_ttl(device_id,
                                     mat_tbl_hdl,
                                     ent_hdl,
                                     pipe_id,
                                     stage_id,
                                     rmt_info->handle,
                                     &ttl);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error retrieving ttl for entry %d in exm tbl 0x%x device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }

    sts = rmt_idle_delete_entry(sess_hdl,
                                device_id,
                                mat_tbl_hdl,
                                ent_hdl,
                                exm_tbl_data->pipe_id,
                                stage_id,
                                rmt_info->handle,
                                ent_idx + i,
                                0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error deleting idletime state for entry %d in exm tbl 0x%x "
          "device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }
    sts = rmt_idle_add_entry(sess_hdl,
                             device_id,
                             mat_tbl_hdl,
                             ent_hdl,
                             exm_tbl_data->pipe_id,
                             stage_id,
                             rmt_info->handle,
                             ent_idx + i,
                             ttl,
                             false,
                             0);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error re-adding idletime state for entry %d in exm tbl 0x%x "
          "device %d",
          __func__,
          __LINE__,
          ent_hdl,
          mat_tbl_hdl,
          device_id);
      return sts;
    }
  }

  sts = pipe_mgr_exm_unlock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in unlocking direct addressed tables for tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(sts));
    return sts;
  }
  pipe_mgr_drv_ilist_push(&sess_hdl, NULL, NULL);

  return PIPE_SUCCESS;
}

static void pipe_mgr_exm_get_resource_delta(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_res_spec_t *resources,
    int resource_count,
    pipe_mgr_act_spec_delta_t *act_spec_delta) {
  pipe_tbl_ref_type_t stats_ref_type;
  pipe_tbl_ref_type_t meters_ref_type;
  pipe_tbl_ref_type_t stful_ref_type;

  if (resource_count > 0) {
    int i = 0;
    unsigned j = 0;
    for (i = 0; i < resource_count; i++) {
      if (resources[i].tag == PIPE_RES_ACTION_TAG_NO_CHANGE) {
        continue;
      }
      if (PIPE_GET_HDL_TYPE(resources[i].tbl_hdl) == PIPE_HDL_TYPE_STAT_TBL) {
        for (j = 0; j < exm_tbl->num_stat_tbl_refs; j++) {
          if (resources[i].tbl_hdl == exm_tbl->stat_tbl_refs[j].tbl_hdl) {
            stats_ref_type = exm_tbl->stat_tbl_refs[j].ref_type;
            act_spec_delta->stats_op.ref_type = stats_ref_type;

            if (stats_ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
              act_spec_delta->stats_operate = true;
              act_spec_delta->stats_op.stat_tbl_hdl =
                  exm_tbl->stat_tbl_refs[j].tbl_hdl;

              if (resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
                act_spec_delta->stats_op.type = ACT_SPEC_DELTA_STATS_ADD;

                act_spec_delta->stats_op.u.entry_add.stat_idx =
                    resources[i].tbl_idx;

              } else if (resources[i].tag == PIPE_RES_ACTION_TAG_DETACHED) {
                act_spec_delta->stats_op.type = ACT_SPEC_DELTA_STATS_DELETE;
              }
              act_spec_delta->stats_op.stat_spec = resources[i];
            } else {
              /* Nothing to be done for direct referenced stats */
              act_spec_delta->stats_operate = false;
              if (resources[i].tag != PIPE_RES_ACTION_TAG_ATTACHED) {
                PIPE_MGR_DBGCHK(0);
                return;
              }
            }
          }
        }
      } else if (PIPE_GET_HDL_TYPE(resources[i].tbl_hdl) ==
                 PIPE_HDL_TYPE_METER_TBL) {
        for (j = 0; j < exm_tbl->num_meter_tbl_refs; j++) {
          if (resources[i].tbl_hdl == exm_tbl->meter_tbl_refs[j].tbl_hdl) {
            meters_ref_type = exm_tbl->meter_tbl_refs[j].ref_type;
            act_spec_delta->meters_op.ref_type = meters_ref_type;

            if (meters_ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
              act_spec_delta->meters_operate = true;
              act_spec_delta->meters_op.meter_tbl_hdl =
                  exm_tbl->meter_tbl_refs[j].tbl_hdl;

              if (resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
                act_spec_delta->meters_op.type = ACT_SPEC_DELTA_METERS_ADD;
                act_spec_delta->meters_op.u.entry_add.meter_idx =
                    resources[i].tbl_idx;

              } else if (resources[i].tag == PIPE_RES_ACTION_TAG_DETACHED) {
                act_spec_delta->meters_op.type = ACT_SPEC_DELTA_METERS_DELETE;
              }
            } else {
              PIPE_MGR_DBGCHK(resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED);

              act_spec_delta->meters_operate = true;
              act_spec_delta->meters_op.meter_tbl_hdl =
                  exm_tbl->meter_tbl_refs[j].tbl_hdl;
              act_spec_delta->meters_op.u.entry_add.meter_spec =
                  &resources[i].data.meter;
            }
            act_spec_delta->meters_op.meter_res = resources[i];
          }
        }
      } else if (PIPE_GET_HDL_TYPE(resources[i].tbl_hdl) ==
                 PIPE_HDL_TYPE_STFUL_TBL) {
        for (j = 0; j < exm_tbl->num_stful_tbl_refs; j++) {
          if (resources[i].tbl_hdl == exm_tbl->stful_tbl_refs[j].tbl_hdl) {
            stful_ref_type = exm_tbl->stful_tbl_refs[j].ref_type;
            act_spec_delta->stful_op.ref_type = stful_ref_type;

            if (stful_ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
              act_spec_delta->stful_operate = true;
              act_spec_delta->stful_op.stful_tbl_hdl =
                  exm_tbl->stful_tbl_refs[j].tbl_hdl;

              if (resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED) {
                act_spec_delta->stful_op.type = ACT_SPEC_DELTA_STFUL_ADD;
                act_spec_delta->stful_op.u.entry_add.stful_idx =
                    resources[i].tbl_idx;

              } else if (resources[i].tag == PIPE_RES_ACTION_TAG_DETACHED) {
                act_spec_delta->stful_op.type = ACT_SPEC_DELTA_STFUL_DELETE;
              }
            } else {
              PIPE_MGR_DBGCHK(resources[i].tag == PIPE_RES_ACTION_TAG_ATTACHED);

              act_spec_delta->stful_operate = true;
              act_spec_delta->stful_op.stful_tbl_hdl =
                  exm_tbl->stful_tbl_refs[j].tbl_hdl;
              act_spec_delta->stful_op.u.entry_add.stful_spec =
                  &resources[i].data.stful;
            }
            act_spec_delta->stful_op.stful_res = resources[i];
          }
        }
      }
    }
  }

  return;
}

bool pipe_mgr_exm_entry_exists(pipe_mgr_exm_tbl_t *exm_tbl,
                               bf_dev_pipe_t pipe_id,
                               uint8_t stage_id,
                               pipe_mgr_exm_edge_container_t *edge_container,
                               pipe_tbl_match_spec_t *match_spec) {
#ifdef PIPE_MGR_DO_EXM_DUPLICATE_CHECKS
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mat_ent_hdl_t ent_hdl;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl instance for tbl 0x%x, pipe id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_tbl_stage_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return false;
  }
  exm_pack_format = exm_tbl_stage_info->pack_format;
  unsigned i = 0;
  unsigned j = 0;

  for (i = 0; i < edge_container->num_entries; i++) {
    for (j = 0; j < exm_pack_format->num_entries_per_wide_word; j++) {
      pipe_mat_ent_idx_t entry_idx =
          exm_tbl_stage_info->stage_offset + edge_container->entries[i] + j;

      /* Get the entry handle corresponding to the entry index */
      ent_hdl = pipe_mgr_exm_get_ent_hdl_from_ent_idx(
          entry_idx, exm_tbl_data, exm_tbl_stage_info);

      if (ent_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
        continue;
      }

      /* There is an entry at this index, compare the match spec contents */
      entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, ent_hdl);
      if (entry_info == NULL) {
        PIPE_MGR_DBGCHK(0);
        return false;
      }
      if (pipe_mgr_exm_cmp_match_spec(
              unpack_mat_ent_data_ms(entry_info->entry_data), match_spec) ==
          true) {
        return true;
      }
    }
  }
  return false;
#else
  (void)exm_tbl;
  (void)pipe_id;
  (void)stage_id;
  (void)edge_container;
  (void)match_spec;
  return false;
#endif
}

bool pipe_mgr_exm_cmp_match_spec(pipe_tbl_match_spec_t *src_spec,
                                 pipe_tbl_match_spec_t *dst_spec) {
  if (src_spec == NULL || dst_spec == NULL) {
    return false;
  }

  if (src_spec->partition_index != dst_spec->partition_index) {
    return false;
  }

  if (src_spec->num_valid_match_bits != dst_spec->num_valid_match_bits) {
    return false;
  }

  if (src_spec->priority != dst_spec->priority) {
    return false;
  }

  if (src_spec->match_value_bits == NULL ||
      dst_spec->match_value_bits == NULL) {
    return false;
  }

  if (PIPE_MGR_MEMCMP(src_spec->match_value_bits,
                      dst_spec->match_value_bits,
                      sizeof((src_spec->num_valid_match_bits + 7) / 8))) {
    return false;
  }

  return true;
}

bool pipe_mgr_exm_ent_validate_action_spec(pipe_action_spec_t *act_spec) {
  (void)act_spec;

  return true;
}

uint32_t pipe_mgr_exm_compute_ram_line_num(
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    pipe_mat_ent_idx_t entry_idx) {
  /* This API returns a RAM line number of TOF_SRAM_UNIT_DEPTH in any error
   * scenario.
   */
  uint32_t offset = 0;
  uint8_t hashway = 0;
  unsigned hash_way_idx = 0;
  uint8_t num_entries_per_wide_word = 0;

  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;

  hashway = pipe_mgr_exm_get_entry_hashway(exm_tbl_stage_info, entry_idx);

  if (hashway >= exm_tbl_stage_info->num_hash_ways) {
    return TOF_SRAM_UNIT_DEPTH;
  }

  for (hash_way_idx = 0; hash_way_idx < hashway; hash_way_idx++) {
    exm_hashway_data = &exm_tbl_stage_info->hashway_data[hash_way_idx];
    offset += exm_hashway_data->num_entries;
    entry_idx -= exm_hashway_data->num_entries;
  }

  exm_pack_format = exm_tbl_stage_info->pack_format;

  if (exm_pack_format == NULL) {
    return TOF_SRAM_UNIT_DEPTH;
  }

  num_entries_per_wide_word = exm_pack_format->num_entries_per_wide_word;

  if (num_entries_per_wide_word == 0) {
    return TOF_SRAM_UNIT_DEPTH;
  }

  return ((entry_idx / num_entries_per_wide_word) % TOF_SRAM_UNIT_DEPTH);
}

bool pipe_mgr_exm_proxy_hash_entry_exists(pipe_mgr_exm_tbl_t *exm_tbl,
                                          bf_dev_pipe_t pipe_id,
                                          uint8_t stage_id,
                                          uint64_t proxy_hash) {
  bf_hashtable_t *htbl = NULL;
  pipe_mgr_exm_proxy_hash_record_node_t *htbl_node = NULL;

  PIPE_MGR_DBGCHK(exm_tbl->proxy_hash == true);

  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;

  exm_tbl_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);

  if (exm_tbl_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting exm tbl stage info for tbl 0x%d, pipe id %d"
        " stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return false;
  }

  htbl = exm_tbl_stage_info->proxy_hash_tbl;
  if (htbl == NULL) {
    PIPE_MGR_DBGCHK(0);
    return false;
  }

  /* Search the proxy hash in the hash table */
  htbl_node = bf_hashtbl_search(htbl, &proxy_hash);

  if (htbl_node != NULL) {
    return true;
  }

  return false;
}

static pipe_status_t pipe_mgr_exm_proxy_hash_entry_insert(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    uint64_t proxy_hash) {
  bf_hashtable_t *htbl = NULL;
  bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
  pipe_mgr_exm_proxy_hash_record_node_t *htbl_node = NULL;

  PIPE_MGR_DBGCHK(exm_tbl->proxy_hash == true);

  htbl = exm_tbl_stage_info->proxy_hash_tbl;
  if (htbl == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  htbl_node = (pipe_mgr_exm_proxy_hash_record_node_t *)PIPE_MGR_CALLOC(
      1, sizeof *htbl_node);
  if (htbl_node == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  htbl_node->proxy_hash = proxy_hash;

  /* Insert the entry into the hash table */
  htbl_sts = bf_hashtbl_insert(htbl, htbl_node, &proxy_hash);
  if (htbl_sts != BF_HASHTBL_OK) {
    PIPE_MGR_FREE(htbl_node);
    return PIPE_UNEXPECTED;
  }

  return PIPE_SUCCESS;
}

static inline int pipe_mgr_exm_proxy_hash_record_cmp(const void *key1,
                                                     const void *key2) {
  return PIPE_MGR_MEMCMP(key1, key2, sizeof(uint64_t));
}

pipe_tbl_ref_t *pipe_mgr_exm_get_tbl_ref(pipe_mgr_exm_tbl_t *exm_tbl,
                                         pipe_tbl_hdl_t tbl_hdl) {
  unsigned i = 0;

  switch (PIPE_GET_HDL_TYPE(tbl_hdl)) {
    case PIPE_HDL_TYPE_ADT_TBL:
      for (i = 0; i < exm_tbl->num_adt_refs; i++) {
        if (exm_tbl->adt_tbl_refs[i].tbl_hdl == tbl_hdl) {
          return &exm_tbl->adt_tbl_refs[i];
        }
      }
      break;
    case PIPE_HDL_TYPE_STAT_TBL:
      for (i = 0; i < exm_tbl->num_stat_tbl_refs; i++) {
        if (exm_tbl->stat_tbl_refs[i].tbl_hdl == tbl_hdl) {
          return &exm_tbl->stat_tbl_refs[i];
        }
      }
      break;
    case PIPE_HDL_TYPE_METER_TBL:
      for (i = 0; i < exm_tbl->num_meter_tbl_refs; i++) {
        if (exm_tbl->meter_tbl_refs[i].tbl_hdl == tbl_hdl) {
          return &exm_tbl->meter_tbl_refs[i];
        }
      }
      break;
    case PIPE_HDL_TYPE_STFUL_TBL:
      for (i = 0; i < exm_tbl->num_stful_tbl_refs; i++) {
        if (exm_tbl->stful_tbl_refs[i].tbl_hdl == tbl_hdl) {
          return &exm_tbl->stful_tbl_refs[i];
        }
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return NULL;
  }
  return NULL;
}

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance_from_any_pipe(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id) {
  if (pipe_id == BF_DEV_PIPE_ALL) return &exm_tbl->exm_tbl_data[0];
  for (unsigned i = 0; i < exm_tbl->num_scopes; i++) {
    if (exm_tbl->scope_pipe_bmp[i] & (1u << pipe_id)) {
      return &exm_tbl->exm_tbl_data[i];
    }
  }
  return NULL;
}

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance(
    pipe_mgr_exm_tbl_t *exm_tbl, bf_dev_pipe_t pipe_id) {
  for (unsigned i = 0; i < exm_tbl->num_tbls; i++) {
    if (exm_tbl->exm_tbl_data[i].pipe_id == pipe_id) {
      return &exm_tbl->exm_tbl_data[i];
    }
  }
  return NULL;
}

pipe_mgr_exm_tbl_data_t *pipe_mgr_exm_tbl_get_instance_from_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    int entry_hdl,
    const char *where,
    const int line) {
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  bf_dev_pipe_t pipe_id;

  if (exm_tbl->symmetric) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[0];
  } else {
    pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
    if (!exm_tbl_data) {
      LOG_ERROR(
          "%s:%d Exm tbl instance for tbl 0x%x, entry hdl %d device id %d not "
          "found",
          where,
          line,
          exm_tbl->mat_tbl_hdl,
          entry_hdl,
          exm_tbl->dev_id);
    }
  }
  return exm_tbl_data;
}

pipe_status_t pipe_mgr_exm_cleanup_default_entry(
    bf_dev_target_t dev_tgt,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t pipe_api_flags,
    pipe_mgr_move_list_t **pipe_move_list) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mat_ent_hdl_t def_ent_hdl;
  cuckoo_move_list_t move_list;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  uint8_t stage_id = 0;

  PIPE_MGR_MEMSET(&move_list, 0, sizeof(cuckoo_move_list_t));
  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not find the exact match table info for table with"
        " handle 0x%x for device id %d",
        __func__,
        mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, dev_tgt.dev_pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl instance for tbl 0x%x, pipe id %x not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              dev_tgt.dev_pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  exm_tbl_data->api_flags = pipe_api_flags;

  if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data) == false) {
    return PIPE_SUCCESS;
  }
  def_ent_hdl = pipe_mgr_exm_get_def_ent_hdl(exm_tbl_data);

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, def_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry %d, tbl %s 0x%x, device %d pipe %x not "
        "found",
        __func__,
        __LINE__,
        def_ent_hdl,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  stage_id = entry_info->stage_id;
  exm_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, entry_info->pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm stage info for tbl 0x%x, pipe id %d, stage id %d not "
        "found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        entry_info->pipe_id,
        stage_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Deallocate any indirectly addressed resources */
  status = pipe_mgr_exm_dealloc_indirect_resources(
      exm_tbl, exm_tbl_data, entry_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in deallocating indirect resources for default entry with "
        "handle %d for tbl %s 0x%x device %d pipe %x err %s",
        __func__,
        __LINE__,
        def_ent_hdl,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Prime a cuckoo move list indicating delete */
  move_list.src_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  move_list.dst_entry = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  /* Update state */
  status = pipe_mgr_exm_update_state(exm_tbl,
                                     exm_tbl_data,
                                     exm_stage_info,
                                     NULL,
                                     NULL,
                                     &move_list,
                                     pipe_move_list,
                                     def_ent_hdl,
                                     0);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in updating state for entry hdl %d, tbl 0x%x, device id "
        "%d, err %s",
        __func__,
        __LINE__,
        def_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  pipe_mgr_exm_reset_def_ent_placed(exm_tbl_data);
  PIPE_MGR_EXM_TRACE(dev_tgt.device_id,
                     exm_tbl->name,
                     def_ent_hdl,
                     dev_tgt.dev_pipe_id,
                     stage_id,
                     "Clearing the default entry, with handle %d",
                     def_ent_hdl);
  return PIPE_SUCCESS;
}

/** \brief pipe_mgr_exm_get_phy_entry_info:
 *         Retrieves a physical entry info for the provided entry handle.
 *
 * \param exm_tbl Pointer to the exact match table.
 * \param mat_ent_hdl Handle associated to the match entry.
 *
 * \return pipe_mgr_exm_phy_entry_info_t Pointer to the physical entry info.
 */
pipe_mgr_exm_phy_entry_info_t *pipe_mgr_exm_get_phy_entry_info(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  bf_map_sts_t map_sts;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, mat_ent_hdl, __func__, __LINE__);
  if (!exm_tbl_data) {
    return NULL;
  }

  map_sts = bf_map_get(
      &exm_tbl_data->entry_phy_info_htbl, mat_ent_hdl, (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting physical entry info for entry handle %d, "
        "tbl 0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    return NULL;
  }

  return entry_info;
}

/** \brief pipe_mgr_exm_get_entry_info:
 *         Retrieves an entry info for the provided entry handle.
 *
 * \param exm_tbl Pointer to the exact match table.
 * \param mat_ent_hdl Handle associated to the match entry.
 *
 * \return pipe_mgr_exm_entry_info_t Pointer to the physical entry info.
 */
pipe_mgr_exm_entry_info_t *pipe_mgr_exm_get_entry_info(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_ent_hdl_t mat_ent_hdl) {
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  bf_map_sts_t map_sts;

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, mat_ent_hdl, __func__, __LINE__);
  if (!exm_tbl_data) {
    return NULL;
  }

  map_sts = bf_map_get(
      &exm_tbl_data->entry_info_htbl, mat_ent_hdl, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    return NULL;
  }
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry handle info for entry handle %d, tbl "
        "0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    PIPE_MGR_DBGCHK(0);
    return NULL;
  }

  return entry_info;
}

pipe_status_t pipe_mgr_exm_get_match_spec(pipe_mat_ent_hdl_t mat_ent_hdl,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          bf_dev_id_t device_id,
                                          bf_dev_pipe_t *pipe_id,
                                          pipe_tbl_match_spec_t **match_spec) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, mat_ent_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  *pipe_id = entry_info->pipe_id;
  *match_spec = unpack_mat_ent_data_ms(entry_info->entry_data);

  return PIPE_SUCCESS;
}

static bool pipe_mgr_exm_validate_entry_add(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    pipe_mgr_move_list_t *move_list_node) {
  (void)exm_tbl;
  (void)exm_tbl_data;
  (void)exm_stage_info;
  (void)stage_ent_idx;
  (void)move_list_node;
  return true;
}

pipe_status_t pipe_mgr_exm_update_llp_state(
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    mem_id_t *mem_id_arr,
    uint32_t num_mem_ids) {
  pipe_action_spec_t *action_spec =
      unpack_mat_ent_data_as(move_list_node->data);
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  if (move_list_node->op == PIPE_MAT_UPDATE_ADD) {
    if (move_list_node->entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL) {
      entry_info = (pipe_mgr_exm_phy_entry_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_phy_entry_info_t));
      if (entry_info == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_NO_SYS_RESOURCES;
      }
      map_sts = bf_map_add(&exm_tbl_data->entry_phy_info_htbl,
                           move_list_node->entry_hdl,
                           entry_info);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in inserting entry phy info for entry hdl %d, tbl "
            "0x%x, pipe id %d, stage id %d, err 0x%x",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      pipe_mat_ent_idx_t entry_idx =
          move_list_node->u.single.logical_idx - exm_stage_info->stage_offset;
      entry_info->adt_ent_hdl = action_spec->adt_ent_hdl;
      entry_info->action_idx = move_list_node->logical_action_idx;
      entry_info->act_fn_hdl =
          unpack_mat_ent_data_afun_hdl(move_list_node->data);
      exm_stage_info->num_entries_occupied++;
      exm_tbl_data->num_entries_occupied++;
      entry_info->entry_idx = entry_idx;
      entry_info->pipe_id = exm_tbl_data->pipe_id;
      entry_info->stage_id = exm_stage_info->stage_id;
      if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
        PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs);
        entry_info->selector_enabled = true;
      } else {
        entry_info->selector_enabled = false;
      }
      entry_info->num_ram_units = num_mem_ids;
      if (mem_id_arr) {
        entry_info->mem_id =
            (mem_id_t *)PIPE_MGR_CALLOC(num_mem_ids, sizeof(mem_id_t));
        if (entry_info->mem_id == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          PIPE_MGR_FREE(entry_info);
          return PIPE_NO_SYS_RESOURCES;
        }
        PIPE_MGR_MEMCPY(
            entry_info->mem_id, mem_id_arr, num_mem_ids * sizeof(mem_id_t));
      }
      PIPE_MGR_MEMCPY(&entry_info->indirect_ptrs,
                      indirect_ptrs,
                      sizeof(pipe_mgr_indirect_ptrs_t));
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_write_atomic_mod_sram_instr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    uint32_t stage_id) {
  if (exm_tbl->dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Tofino doesn't have atomic mod registers, return */
    return PIPE_SUCCESS;
  }




  pipe_atomic_mod_sram_instr_t instr;
  construct_instr_atomic_mod_sram(exm_tbl->dev_id, &instr, exm_tbl->direction);
  return pipe_mgr_drv_ilist_add(&sess_hdl,
                                exm_tbl->dev_info,
                                &exm_tbl_data->pipe_bmp,
                                stage_id,
                                (uint8_t *)&instr,
                                sizeof(pipe_atomic_mod_sram_instr_t));
}

pipe_status_t pipe_mgr_exm_execute_entry_add(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_move_list_t *move_list_node,
    bool end_of_move_add,
    bool is_stash,
    bool is_recovery) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_action_spec_t *action_spec = NULL;
  uint8_t **shadow_ptr_arr = NULL;
  uint8_t *wide_word_indices = NULL;
  uint8_t vv_word_index = 0;
  uint32_t num_ram_units = 0, version_valid_bits = 0;
  mem_id_t *mem_id_arr = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_idx_t src_stage_ent_idx = 0;
  pipe_mat_ent_idx_t dir_entry_idx = 0;
  uintptr_t ent_hdl_p = 0;

  /* First compute ram shadow copy */
  if (is_stash) {
    shadow_ptr_arr = exm_stage_info->stash->shadow_ptr_arr;
    wide_word_indices = exm_stage_info->stash->wide_word_indices;
  } else {
    shadow_ptr_arr = exm_stage_info->shadow_ptr_arr;
    mem_id_arr = exm_stage_info->mem_id_arr;
    wide_word_indices = exm_stage_info->wide_word_indices;
  }

  action_spec = unpack_mat_ent_data_as(move_list_node->data);
  if (move_list_node->op == PIPE_MAT_UPDATE_ADD) {
    /* Entry handle could be invalid for cases where default entries are
     * being added to hash-action tables.
     * Don't allocate entries during recovery.
     */
    if (move_list_node->entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL &&
        !is_recovery) {
      entry_info = (pipe_mgr_exm_phy_entry_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_phy_entry_info_t));
      if (entry_info == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        PIPE_MGR_DBGCHK(0);
        return PIPE_NO_SYS_RESOURCES;
      }
      map_sts = bf_map_add(&exm_tbl_data->entry_phy_info_htbl,
                           move_list_node->entry_hdl,
                           entry_info);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in inserting entry phy info for entry hdl %d, tbl "
            "0x%x, "
            "pipe id %d, stage id %d, err 0x%x",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
      /* Store the action entry handle associated with this match entry. This is
       * applicable only for indirectly addressed action tables. It will not be
       * intepreted/used for directly addressed action tables.
       */
      entry_info->adt_ent_hdl = action_spec->adt_ent_hdl;
      entry_info->action_idx = move_list_node->logical_action_idx;
      entry_info->act_fn_hdl =
          unpack_mat_ent_data_afun_hdl(move_list_node->data);
      exm_stage_info->num_entries_occupied++;
      exm_tbl_data->num_entries_occupied++;
    } else if (move_list_node->entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL &&
               is_recovery) {
      map_sts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                           move_list_node->entry_hdl,
                           (void **)&entry_info);
      if (map_sts != BF_MAP_OK) {
        LOG_ERROR(
            "%s:%d Error in retrieving entry phy info for entry hdl %d, tbl "
            "0x%x, "
            "pipe id %d, stage id %d, err 0x%x",
            __func__,
            __LINE__,
            move_list_node->entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            map_sts);
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
      }
    }
  } else if (move_list_node->op == PIPE_MAT_UPDATE_MOV) {
    map_sts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                         move_list_node->entry_hdl,
                         (void **)&entry_info);
    if (map_sts == BF_MAP_NO_KEY) {
      LOG_ERROR(
          "%s:%d Entry info for entry %d, tbl 0x%x, device id %d not found",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    } else if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in getting entry info for entry %d, tbl 0x%x, device id "
          "%d, err %d",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    /* Cache the current location */
    src_stage_ent_idx = entry_info->entry_idx;
  }
  if (entry_info) {
    entry_info->entry_idx = entry_idx;
    entry_info->stage_id = exm_stage_info->stage_id;
    entry_info->pipe_id = exm_tbl_data->pipe_id;
    if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
      PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs);
      entry_info->selector_enabled = true;
    } else {
      entry_info->selector_enabled = false;
    }
  }
  /* Get the shadow memory pointers to the wide-word in which this entry lives.
   */
  if (!is_stash) {
    status = pipe_mgr_exm_get_ram_shadow_copy(
        exm_tbl, exm_tbl_data, exm_stage_info, entry_idx, shadow_ptr_arr);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting ram shadow copy for entry handle %d, idx %d, "
          "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  /* Handle any direct addressed tables that needs to be programmed. This needs.
   * to be done before the match entry is programmed.
   */

  status = pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                                exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                entry_idx,
                                                src_stage_ent_idx,
                                                move_list_node,
                                                entry_info,
                                                end_of_move_add,
                                                true,
                                                is_recovery);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in handling direct addressed resources for entry %d, "
        "tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    goto err_cleanup;
  }

  if (move_list_node->op == PIPE_MAT_UPDATE_ADD) {
    /* Get any indirect addressed pointers to be encoded as part of match
     * overhead
     */
    status = pipe_mgr_exm_attach_indirect_resources(
        sess_hdl,
        exm_tbl,
        exm_tbl_data,
        exm_stage_info,
        move_list_node,
        entry_info ? (&entry_info->indirect_ptrs) : NULL,
        true,
        false,
        is_recovery);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting indirect ptrs for entry hdl %d, tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
  }

  status = pipe_mgr_exm_compute_ram_shadow_copy(
      exm_tbl,
      exm_stage_info,
      entry_idx,
      entry_info ? (&entry_info->indirect_ptrs) : NULL,
      move_list_node,
      shadow_ptr_arr,
      &num_ram_units,
      mem_id_arr,
      wide_word_indices,
      &vv_word_index,
      is_stash,
      &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing ram shadow copy for entry handle %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Invoke the driver workflow function to program this entry down to the
   * hardware.
   */
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      entry_idx,
      num_ram_units,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in adding entry %d for tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  if (exm_tbl->proxy_hash && !is_recovery) {
    pipe_tbl_match_spec_t *m_spec = NULL;
    m_spec = pipe_mgr_tbl_copy_match_spec(
        m_spec, unpack_mat_ent_data_ms(move_list_node->data));
    if (m_spec == NULL) {
      LOG_ERROR("%s:%d Copy match_spec failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    map_sts = bf_map_add(&exm_tbl_data->proxy_hash_llp_hdl_to_mspec,
                         move_list_node->entry_hdl,
                         (void *)m_spec);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in inserting match spec for entry hdl %d, tbl "
          "0x%x, pipe id %x, err 0x%x",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }

  /* Hash action tables require atomic modify of the action ram.
   * Send the go instruction here.
   */
  if (exm_tbl->hash_action) {
    status = pipe_mgr_exm_write_atomic_mod_sram_instr(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error sending atomic mod instr for entry %d in exm tbl 0x%x "
          "device id %d err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }

  if (entry_info && !is_stash && !is_recovery) {
    entry_info->num_ram_units = num_ram_units;
    if (entry_info->mem_id) {
      PIPE_MGR_FREE(entry_info->mem_id);
      entry_info->mem_id = NULL;
    }
    entry_info->mem_id =
        (mem_id_t *)PIPE_MGR_CALLOC(num_ram_units, sizeof(mem_id_t));
    if (entry_info->mem_id == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    unsigned i = 0;
    for (i = 0; i < num_ram_units; i++) {
      entry_info->mem_id[i] = mem_id_arr[wide_word_indices[i]];
    }
  }

  if (move_list_node->entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL &&
      !is_recovery) {
    dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, entry_idx);
    ent_hdl_p = (uintptr_t)move_list_node->entry_hdl;
    map_sts = bf_map_add(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                         dir_entry_idx,
                         (void *)ent_hdl_p);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in inserting log dir idx to entry hdl mapping for idx "
          "%d, hdl 0x%x, tbl 0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          entry_idx,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      return PIPE_UNEXPECTED;
    }
  }
  if (move_list_node->op == PIPE_MAT_UPDATE_MOV) {
    dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, src_stage_ent_idx);
    map_sts = bf_map_get_rmv(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                             dir_entry_idx,
                             (void **)&ent_hdl_p);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in removing log dir idx to entry hdl mapping for idx "
          "%d, tbl 0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          src_stage_ent_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;

err_cleanup:
  if (move_list_node->op == PIPE_MAT_UPDATE_ADD &&
      move_list_node->entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL &&
      !is_recovery) {
    map_sts = bf_map_get_rmv(&exm_tbl_data->entry_phy_info_htbl,
                             move_list_node->entry_hdl,
                             (void **)&entry_info);
    PIPE_MGR_DBGCHK(map_sts == BF_MAP_OK);
    if (entry_info->mem_id) {
      PIPE_MGR_FREE(entry_info->mem_id);
    }
    PIPE_MGR_FREE(entry_info);
  }
  return status;
}

static pipe_status_t pipe_mgr_exm_write_atomic_mod_csr(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    uint32_t stage_id,
    bool start) {
  if (exm_tbl->dev_info->dev_family != BF_DEV_FAMILY_TOFINO2 &&
      exm_tbl->dev_info->dev_family != BF_DEV_FAMILY_TOFINO3) {
    /* Tofino doesn't have atomic mod registers, return */

    return PIPE_SUCCESS;
  }

  pipe_atomic_mod_csr_instr_t instr;
  construct_instr_atomic_mod_csr(
      exm_tbl->dev_id, &instr, exm_tbl->direction, start, true);
  return pipe_mgr_drv_ilist_add(&sess_hdl,
                                exm_tbl->dev_info,
                                &exm_tbl_data->pipe_bmp,
                                stage_id,
                                (uint8_t *)&instr,
                                sizeof(pipe_atomic_mod_csr_instr_t));
}

static pipe_status_t pipe_mgr_exm_write_atomic_mod_sram_reg(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    uint32_t stage_id) {
  if (exm_tbl->dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Tofino doesn't have atomic mod registers, return */
    return PIPE_SUCCESS;
  }

  uint32_t reg_addr = 0, reg_data = 0;
  pipe_instr_write_reg_t instr;

  switch (exm_tbl->dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      reg_addr = offsetof(tof2_reg,
                          pipes[0]
                              .mau[stage_id]
                              .rams.match.adrdist
                              .atomic_mod_sram_go_pending[exm_tbl->direction]);
      setp_tof2_atomic_mod_sram_go_pending_atomic_mod_sram_go_pending(&reg_data,
                                                                      1);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      reg_addr = offsetof(tof3_reg,
                          pipes[0]
                              .mau[stage_id]
                              .rams.match.adrdist
                              .atomic_mod_sram_go_pending[exm_tbl->direction]);
      setp_tof3_atomic_mod_sram_go_pending_atomic_mod_sram_go_pending(&reg_data,
                                                                      1);
      break;



    default:
      return PIPE_SUCCESS;
  }

  construct_instr_reg_write(exm_tbl->dev_id, &instr, reg_addr, reg_data);
  return pipe_mgr_drv_ilist_add(&sess_hdl,
                                exm_tbl->dev_info,
                                &exm_tbl_data->pipe_bmp,
                                stage_id,
                                (uint8_t *)&instr,
                                sizeof(pipe_instr_write_reg_t));
}

static pipe_status_t pipe_mgr_exm_execute_hash_action_def_entry_set(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node) {
  pipe_status_t sts = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_idx_t entry_idx = 0, dir_entry_idx = 0;
  pipe_mgr_exm_entry_info_t *htbl_data = NULL;
  pipe_action_spec_t *action_spec, *tmp_aspec;
  action_spec = unpack_mat_ent_data_as(move_list_node->data);
  if (!action_spec) {
    LOG_ERROR("Dev %d pipe %x Tbl %s 0x%x, no action spec provided to %s",
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              __func__);
    return PIPE_INVALID_ARG;
  }
  /* Make a copy of the action spec to be saved against the table. */
  tmp_aspec = pipe_mgr_tbl_copy_action_spec(NULL, action_spec);
  if (!tmp_aspec) {
    return PIPE_NO_SYS_RESOURCES;
  }

  /* The current move list node is either a SET_DFLT or CLR_DFLT operation and
   * may be on a long list with additional operations (i.e. nodes) after the
   * current SET/CLR_DFLT.  Alter the current node in place so we can reuse it
   * to perform a "dummy add" of an entry using the action spec provided in the
   * SET/CLR_DFLT operation. */
  pipe_mgr_move_list_t *next = move_list_node->next;
  uint8_t op = move_list_node->op;
  pipe_ent_hdl_t hdl = move_list_node->entry_hdl;
  move_list_node->next = NULL;
  move_list_node->op = PIPE_MAT_UPDATE_ADD;
  move_list_node->entry_hdl = PIPE_MAT_ENT_HDL_INVALID_HDL;

  /* Go over all entries in the stage, if the entry has not be programmed
   * program it with the default action data. */
  for (entry_idx = 0; entry_idx < exm_stage_info->num_entries; entry_idx++) {
    dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, entry_idx);
    map_sts = bf_map_get(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                         dir_entry_idx,
                         (void **)&htbl_data);

    /* If no match entry exists at this index, set the default data here */
    if (map_sts == BF_MAP_NO_KEY) {
      move_list_node->u.single.logical_idx = entry_idx;
      sts = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           entry_idx,
                                           move_list_node,
                                           false,
                                           false,
                                           false /* is_recovery */);
      if (sts != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Failed to set default hash-action entry at index 0x%x "
            "pipe %d, stage %d for tbl %s 0x%x, %s",
            __func__,
            __LINE__,
            entry_idx,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            exm_tbl->name,
            exm_tbl->mat_tbl_hdl,
            pipe_str_err(sts));
        break;
      }
    }
  }

  /* Restore the move node to the original state. */
  move_list_node->next = next;
  move_list_node->op = op;
  move_list_node->entry_hdl = hdl;

  if (sts != PIPE_SUCCESS) {
    pipe_mgr_tbl_destroy_action_spec(&tmp_aspec);
    return sts;
  }

  /* Save the default info for future entry delete operations. */
  if (exm_tbl_data->hash_action_dflt_act_spec)
    pipe_mgr_tbl_destroy_action_spec(&exm_tbl_data->hash_action_dflt_act_spec);
  exm_tbl_data->hash_action_dflt_act_spec = tmp_aspec;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_execute_def_entry_program(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_action_spec_t *action_spec =
      unpack_mat_ent_data_as(move_list_node->data);
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_hdl_t mat_ent_hdl = move_list_node->entry_hdl;
  bool existing_default_entry =
      pipe_mgr_exm_is_default_ent_installed(exm_tbl_data);
  pipe_mat_ent_idx_t dir_entry_idx = 0;
  uintptr_t ent_hdl_p = 0;

  /* Hash action is a special case that programs unoccupied match indices
   * instead of the default registers. Handle this case separately. */
  if (exm_tbl->hash_action) {
    status = pipe_mgr_exm_execute_hash_action_def_entry_set(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, move_list_node);
    if (PIPE_SUCCESS == status)
      pipe_mgr_exm_set_def_ent_installed(exm_tbl_data);
    return status;
  }

  /* We need a phy_entry_info for the default entry.  If this is a modification
   * of the existing default entry just lookup it up from the map using its
   * entry handle.  If this is a new entry, allocate a phy_entry_info and add it
   * to the map. */
  if (existing_default_entry) {
    map_sts = bf_map_get(
        &exm_tbl_data->entry_phy_info_htbl, mat_ent_hdl, (void **)&entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u Error %d getting phy info",
          __func__,
          __LINE__,
          exm_tbl->dev_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          mat_ent_hdl,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    PIPE_MGR_DBGCHK(move_list_node->entry_hdl ==
                    exm_tbl_data->default_entry_hdl);
  } else {
    entry_info = (pipe_mgr_exm_phy_entry_info_t *)PIPE_MGR_CALLOC(
        1, sizeof(pipe_mgr_exm_phy_entry_info_t));
    if (entry_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
    entry_info->pipe_id = exm_tbl_data->pipe_id;
    entry_info->stage_id = exm_stage_info->stage_id;
    entry_info->entry_idx = exm_stage_info->default_miss_entry_idx;
    map_sts =
        bf_map_add(&exm_tbl_data->entry_phy_info_htbl, mat_ent_hdl, entry_info);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u Error %d adding phy info",
          __func__,
          __LINE__,
          exm_tbl->dev_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          mat_ent_hdl,
          map_sts);
      PIPE_MGR_FREE(entry_info);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
  }
  if (existing_default_entry) {
    status = pipe_mgr_exm_detach_indirect_resources(
        exm_tbl, exm_tbl_data, exm_stage_info, entry_info, true);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in detaching indirect resources for default entry, "
          "tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      return status;
    }
    PIPE_MGR_DBGCHK(pipe_mgr_exm_is_default_ent_installed(exm_tbl_data));
  } else {
    pipe_mgr_exm_set_def_ent_installed(exm_tbl_data);
  }
  /* Handle any direct addressed tables that needs to be programmed. This needs.
   * to be done before the match entry is programmed.  This should be skipped if
   * the default entry does not use direct resources.  */
  if (exm_stage_info->default_miss_entry_idx != PIPE_MGR_EXM_UNUSED_ENTRY_IDX) {
    status =
        pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                             exm_tbl,
                                             exm_tbl_data,
                                             exm_stage_info,
                                             entry_info->entry_idx,
                                             PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                             move_list_node,
                                             entry_info,
                                             false,
                                             true,
                                             false /* is_recovery */);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in handling direct addressed resources for entry %d, "
          "tbl "
          "0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      goto err_cleanup;
    }
  }
  /* Get any indirect addressed pointers to be encoded as part of match overhead
   */
  status = pipe_mgr_exm_attach_indirect_resources(sess_hdl,
                                                  exm_tbl,
                                                  exm_tbl_data,
                                                  exm_stage_info,
                                                  move_list_node,
                                                  &entry_info->indirect_ptrs,
                                                  true,
                                                  existing_default_entry,
                                                  false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting indirect ptrs for entry hdl %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    goto err_cleanup;
  }

  if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs);
    entry_info->selector_enabled = true;
  } else {
    entry_info->selector_enabled = false;
  }
  entry_info->adt_ent_hdl = action_spec->adt_ent_hdl;
  entry_info->action_idx = move_list_node->logical_action_idx;
  entry_info->act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_list_node->data);
  entry_info->indirect_ptrs.sel_len =
      unpack_mat_ent_data_sel(move_list_node->data);

  if (existing_default_entry) {
    /* Update atomically if we are modifying the default entry */
    status = pipe_mgr_exm_write_atomic_mod_csr(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id, true);
    if (status != PIPE_SUCCESS) {
      goto err_cleanup;
    }

    /* Csr atomic modify requires atomic sram updates through a register
     * write. Note that this must be the first csr to be written after
     * the atomic start.
     */
    status = pipe_mgr_exm_write_atomic_mod_sram_reg(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id);
    if (status != PIPE_SUCCESS) {
      goto err_cleanup;
    }
  }

  status = pipe_mgr_exm_program_default_entry(sess_hdl,
                                              exm_tbl,
                                              exm_tbl_data,
                                              exm_stage_info,
                                              entry_info->act_fn_hdl,
                                              &action_spec->act_data,
                                              &entry_info->indirect_ptrs);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in programming default entry for tbl %d, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    goto err_cleanup;
  }

  if (existing_default_entry) {
    status = pipe_mgr_exm_write_atomic_mod_csr(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id, false);
    if (status != PIPE_SUCCESS) {
      goto err_cleanup;
    }
  }

  if (!existing_default_entry) {
    if (entry_info->entry_idx == PIPE_MGR_EXM_UNUSED_ENTRY_IDX) {
      dir_entry_idx = PIPE_MGR_EXM_UNUSED_ENTRY_IDX;
    } else {
      dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
          exm_tbl, exm_stage_info, entry_info->entry_idx);
    }
    ent_hdl_p = (uintptr_t)move_list_node->entry_hdl;
    map_sts = bf_map_add(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                         dir_entry_idx,
                         (void *)ent_hdl_p);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in inserting log dir idx to entry hdl mapping for idx "
          "%d, hdl 0x%x, tbl 0x%x, device id %d, err 0x%x",
          __func__,
          __LINE__,
          entry_info->entry_idx,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          map_sts);
      return PIPE_UNEXPECTED;
    }
  }

  return PIPE_SUCCESS;
err_cleanup:
  if (!existing_default_entry) {
    map_sts = bf_map_get_rmv(
        &exm_tbl_data->entry_phy_info_htbl, mat_ent_hdl, (void **)&entry_info);
    PIPE_MGR_DBGCHK(map_sts == BF_MAP_OK);
    PIPE_MGR_FREE(entry_info);
  }
  return status;
}

/* Default entry get from HW. */
pipe_status_t pipe_mgr_exm_execute_def_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_action_spec_t *action_spec) {
  pipe_status_t status;

  status = pipe_mgr_exm_get_default_entry(sess_hdl,
                                          dev_tgt,
                                          exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          act_fn_hdl,
                                          &action_spec->act_data,
                                          indirect_ptrs);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in reading default entry for tbl 0x%x, device_id %d, err "
        "%s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_execute_entry_modify(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node) {
  if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                      move_list_node->entry_hdl)) {
    return pipe_mgr_exm_execute_def_entry_program(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, move_list_node);
  }
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  uint8_t **shadow_ptr_arr = NULL;
  uint32_t num_ram_units = 0, version_valid_bits = 0;
  bool is_stash = false;
  mem_id_t *mem_id_arr = NULL;
  uint8_t *wide_word_indices = NULL;
  uint8_t vv_word_index = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  bf_dev_pipe_t pipe_id;
  dev_stage_t stage_id;
  pipe_action_spec_t *action_spec = NULL;
  action_spec = unpack_mat_ent_data_as(move_list_node->data);
  pipe_mat_ent_idx_t stage_ent_idx = pipe_mgr_exm_get_entry_loc(
      exm_tbl, move_list_node->entry_hdl, &pipe_id, &stage_id);
  if (stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
    LOG_ERROR(
        "%s:%d Entry location for entry handle %d, tbl 0x%x, device id %d not "
        "found",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* First compute ram shadow copy */
  shadow_ptr_arr = exm_stage_info->shadow_ptr_arr;
  mem_id_arr = exm_stage_info->mem_id_arr;
  wide_word_indices = exm_stage_info->wide_word_indices;
  /* Get the shadow memory pointers to the wide-word in which this entry lives.
   */
  status = pipe_mgr_exm_get_ram_shadow_copy(
      exm_tbl, exm_tbl_data, exm_stage_info, stage_ent_idx, shadow_ptr_arr);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting ram shadow copy for entry handle %d, idx %d, "
        "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_FREE(shadow_ptr_arr);
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  map_sts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                       move_list_node->entry_hdl,
                       (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting phy entry info for entry %d, tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  /* Handle any direct addressed tables that needs to be programmed. This needs.
   * to be done before the match entry is programmed.
   */
  status =
      pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           stage_ent_idx,
                                           PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                           move_list_node,
                                           entry_info,
                                           false,
                                           true,
                                           false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in handling direct addressed resources for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Detach existing indirect resources */
  status = pipe_mgr_exm_detach_indirect_resources(
      exm_tbl, exm_tbl_data, exm_stage_info, entry_info, true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in detaching indirect resource for entry %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  entry_info->adt_ent_hdl = action_spec->adt_ent_hdl;
  entry_info->action_idx = move_list_node->logical_action_idx;
  entry_info->act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_list_node->data);
  if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs);
    entry_info->selector_enabled = true;
  } else {
    entry_info->selector_enabled = false;
  }
  /* Get any indirect addressed pointers to be encoded as part of match overhead
   */
  status = pipe_mgr_exm_attach_indirect_resources(sess_hdl,
                                                  exm_tbl,
                                                  exm_tbl_data,
                                                  exm_stage_info,
                                                  move_list_node,
                                                  &entry_info->indirect_ptrs,
                                                  true,
                                                  true,
                                                  false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting indirect ptrs for entry hdl %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Store the new action entry handle */

  status = pipe_mgr_exm_compute_ram_shadow_copy(exm_tbl,
                                                exm_stage_info,
                                                stage_ent_idx,
                                                &entry_info->indirect_ptrs,
                                                move_list_node,
                                                shadow_ptr_arr,
                                                &num_ram_units,
                                                mem_id_arr,
                                                wide_word_indices,
                                                &vv_word_index,
                                                is_stash,
                                                &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing ram shadow copy for entry handle %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Invoke the driver workflow function to program this entry down to the
   * hardware.
   */
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      stage_ent_idx,
      num_ram_units,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in modifying entry %d for tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  return pipe_mgr_exm_write_atomic_mod_sram_instr(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id);
}

static pipe_status_t pipe_mgr_exm_invalidate_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    bool is_stash) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t **shadow_ptr_arr = NULL;
  mem_id_t *mem_id_arr = NULL;
  uint8_t *wide_word_indices = NULL;
  bool version_valid = false;
  uint32_t version_valid_bits = 0;
  shadow_ptr_arr = exm_stage_info->shadow_ptr_arr;
  mem_id_arr = exm_stage_info->mem_id_arr;
  wide_word_indices = exm_stage_info->wide_word_indices;
  /* Get the shadow memory pointers to the wide-word in which this entry lives.
   */
  status = pipe_mgr_exm_get_ram_shadow_copy(
      exm_tbl, exm_tbl_data, exm_stage_info, stage_ent_idx, shadow_ptr_arr);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting ram shadow copy for idx %d, "
        "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Compute the new ram shadow copy with just flipping the version valid bits
   * in the entry to invalid.
   */
  status = pipe_mgr_exm_compute_ram_shadow_copy_vv(exm_tbl,
                                                   exm_stage_info,
                                                   stage_ent_idx,
                                                   shadow_ptr_arr,
                                                   wide_word_indices,
                                                   mem_id_arr,
                                                   version_valid,
                                                   is_stash,
                                                   &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing ram shadow copy for tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Invoke the driver workflow function to program this entry down to the
   * hardware.
   */
  uint8_t vv_word_index = *wide_word_indices;
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      stage_ent_idx,
      1,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in invalidating entry %d for tbl 0x%x, device id %d, err "
        "%s",
        __func__,
        __LINE__,
        stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_execute_entry_delete(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t stage_ent_idx,
    pipe_mgr_move_list_t *move_list_node,
    bool is_stash) {
  pipe_status_t status = PIPE_SUCCESS;
  bool version_valid = false;
  uint32_t version_valid_bits = 0;
  uint8_t **shadow_ptr_arr = NULL;
  mem_id_t *mem_id_arr = NULL;
  uint8_t *wide_word_indices = NULL;
  pipe_mat_ent_hdl_t mat_ent_hdl = move_list_node->entry_hdl;
  pipe_mat_ent_idx_t dir_entry_idx = 0;
  uintptr_t ent_hdl_p = 0;

  /* First compute ram shadow copy */
  shadow_ptr_arr = exm_stage_info->shadow_ptr_arr;
  mem_id_arr = exm_stage_info->mem_id_arr;
  wide_word_indices = exm_stage_info->wide_word_indices;
  bf_map_sts_t map_sts = BF_MAP_OK;
  /* Get the shadow memory pointers to the wide-word in which this entry lives.
   */
  status = pipe_mgr_exm_get_ram_shadow_copy(
      exm_tbl, exm_tbl_data, exm_stage_info, stage_ent_idx, shadow_ptr_arr);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting ram shadow copy for entry handle %d, idx %d, "
        "tbl 0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Compute the new ram shadow copy with just flipping the version valid bits
   * in the entry to invalid.
   */
  status = pipe_mgr_exm_compute_ram_shadow_copy_vv(exm_tbl,
                                                   exm_stage_info,
                                                   stage_ent_idx,
                                                   shadow_ptr_arr,
                                                   wide_word_indices,
                                                   mem_id_arr,
                                                   version_valid,
                                                   is_stash,
                                                   &version_valid_bits);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in computing ram shadow copy for entry handle %d tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  /* Delete the entry phy info */
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  map_sts = bf_map_get_rmv(
      &exm_tbl_data->entry_phy_info_htbl, mat_ent_hdl, (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing entry phy info for entry %d, tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        mat_ent_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  status =
      pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           entry_info->entry_idx,
                                           PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                           move_list_node,
                                           entry_info,
                                           false,
                                           true,
                                           false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in handling direct addressed resource for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Detach indirect resources */
  status = pipe_mgr_exm_detach_indirect_resources(
      exm_tbl, exm_tbl_data, exm_stage_info, entry_info, true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in detaching indirect resource for entry %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  if (entry_info->mem_id) {
    PIPE_MGR_FREE(entry_info->mem_id);
  }
  PIPE_MGR_FREE(entry_info);
  /* Invoke the driver workflow function to program this entry down to the
   * hardware.
   */
  uint8_t vv_word_index = *wide_word_indices;
  status = pipe_mgr_exm_entry_program(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      stage_ent_idx,
      1,
      version_valid_bits,
      vv_word_index,
      is_stash,
      is_stash ? pipe_mgr_default_stash_id_get(exm_stage_info) : 0,
      false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in deleting entry %d for tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  if (exm_stage_info->num_entries_occupied > 0) {
    exm_stage_info->num_entries_occupied--;
  }
  if (exm_tbl_data->num_entries_occupied > 0) {
    exm_tbl_data->num_entries_occupied--;
  }

  dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
      exm_tbl, exm_stage_info, stage_ent_idx);
  map_sts = bf_map_get_rmv(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                           dir_entry_idx,
                           (void **)&ent_hdl_p);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing log dir idx to entry hdl mapping for idx "
        "%d, tbl 0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    return PIPE_UNEXPECTED;
  }

  if (exm_tbl->proxy_hash) {
    pipe_tbl_match_spec_t *mspec = NULL;
    map_sts = bf_map_get_rmv(&exm_tbl_data->proxy_hash_llp_hdl_to_mspec,
                             mat_ent_hdl,
                             (void **)&mspec);
    if (map_sts != BF_MAP_OK) {
      LOG_ERROR(
          "%s:%d Error in removing match_spec for entry hdl %d, tbl "
          "0x%x, pipe id %x, err 0x%x",
          __func__,
          __LINE__,
          mat_ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          map_sts);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    pipe_mgr_tbl_destroy_match_spec(&mspec);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_reset_default_entry_int(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t entry_idx,
    bool init) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  pipe_mgr_action_entry_t *action_entry = NULL;
  pipe_mat_ent_idx_t dir_ent_idx = 0;
  pipe_act_fn_hdl_t act_fn_hdl = 0;
  pipe_action_data_spec_t *act_data_spec = NULL;
  pipe_mgr_ind_res_info_t *ind_res = NULL;
  pipe_hdl_type_t tbl_type;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  uint32_t i = 0;

  mat_tbl_info = pipe_mgr_get_tbl_info(
      exm_tbl->dev_id, exm_tbl->mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Cannot find match table info for exm tbl 0x%x device %d",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (mat_tbl_info->default_info) {
    if (exm_stage_info->default_miss_entry_idx !=
        PIPE_MGR_EXM_UNUSED_ENTRY_IDX) {
      dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
          exm_tbl, exm_stage_info, entry_idx);
    }
    action_entry = &mat_tbl_info->default_info->action_entry;
    act_fn_hdl = action_entry->act_fn_hdl;

    if (action_entry->num_act_data) {
      /* Create action spec */
      act_data_spec = PIPE_MGR_CALLOC(1, sizeof(pipe_action_data_spec_t));
      if (act_data_spec == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      status = pipe_mgr_create_action_data_spec(
          action_entry->act_data, action_entry->num_act_data, act_data_spec);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Failed to create action data spec for default entry for tbl "
            "0x%x ",
            __func__,
            __LINE__,
            mat_tbl_info->handle);
        goto cleanup;
      }
    }

    if (exm_stage_info->default_miss_entry_idx !=
            PIPE_MGR_EXM_UNUSED_ENTRY_IDX &&
        exm_tbl->num_adt_refs != 0 &&
        exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      status = rmt_adt_ent_add(sess_hdl,
                               exm_tbl->dev_id,
                               exm_tbl->adt_tbl_refs[0].tbl_hdl,
                               exm_tbl_data->pipe_id,
                               exm_stage_info->stage_id,
                               act_fn_hdl,
                               act_data_spec,
                               dir_ent_idx,
                               exm_stage_info->stage_table_handle,
                               &indirect_ptrs.adt_ptr,
                               !init);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Error writing default action for exm table 0x%x",
                  __func__,
                  __LINE__,
                  exm_tbl->mat_tbl_hdl);
        goto cleanup;
      }
    }

    for (i = 0; i < mat_tbl_info->default_info->action_entry.num_ind_res; i++) {
      ind_res = &mat_tbl_info->default_info->action_entry.ind_res[i];
      tbl_type = PIPE_GET_HDL_TYPE(ind_res->handle);
      switch (tbl_type) {
        case PIPE_HDL_TYPE_STAT_TBL:
          status = rmt_stat_mgr_stat_ent_attach(exm_tbl->dev_id,
                                                exm_tbl_data->pipe_id,
                                                exm_stage_info->stage_id,
                                                ind_res->handle,
                                                ind_res->idx,
                                                &indirect_ptrs.stats_ptr);
          if (PIPE_SUCCESS != status) {
            LOG_ERROR(
                "%s:%d Error attaching stat tbl 0x%x idx %d for exm tbl 0x%x "
                "pipe %d device %d, %s",
                __func__,
                __LINE__,
                ind_res->handle,
                ind_res->idx,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                exm_tbl->dev_id,
                pipe_str_err(status));
            goto cleanup;
          }
          break;
        case PIPE_HDL_TYPE_METER_TBL:
          status = rmt_meter_mgr_meter_attach(exm_tbl->dev_id,
                                              exm_tbl_data->pipe_id,
                                              exm_stage_info->stage_id,
                                              ind_res->handle,
                                              ind_res->idx,
                                              &indirect_ptrs.meter_ptr);
          if (PIPE_SUCCESS != status) {
            LOG_ERROR(
                "%s:%d Error attaching meter tbl 0x%x idx %d for exm tbl 0x%x "
                "pipe %d device %d, %s",
                __func__,
                __LINE__,
                ind_res->handle,
                ind_res->idx,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                exm_tbl->dev_id,
                pipe_str_err(status));
            goto cleanup;
          }
          break;
        case PIPE_HDL_TYPE_STFUL_TBL:
          status = pipe_mgr_stful_get_indirect_ptr(exm_tbl->dev_id,
                                                   exm_tbl_data->pipe_id,
                                                   exm_stage_info->stage_id,
                                                   act_fn_hdl,
                                                   ind_res->handle,
                                                   ind_res->idx,
                                                   &indirect_ptrs.stfl_ptr);
          if (PIPE_SUCCESS != status) {
            LOG_ERROR(
                "%s:%d Error attaching stful tbl 0x%x idx %d for exm tbl 0x%x "
                "pipe %d device %d, %s",
                __func__,
                __LINE__,
                ind_res->handle,
                ind_res->idx,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                exm_tbl->dev_id,
                pipe_str_err(status));
            goto cleanup;
          }
          break;
        default:
          status = PIPE_INVALID_ARG;
          goto cleanup;
      }
    }

    /* Add default meter spec if the table uses a direct meter */
    if (exm_tbl->num_meter_tbl_refs != 0 &&
        exm_tbl->meter_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      pipe_meter_tbl_hdl_t meter_tbl_hdl = exm_tbl->meter_tbl_refs[0].tbl_hdl;
      pipe_meter_tbl_info_t *meter_tbl_info = pipe_mgr_get_meter_tbl_info(
          exm_tbl->dev_id, meter_tbl_hdl, __func__, __LINE__);
      if (!meter_tbl_info) {
        LOG_ERROR(
            "%s:%d Failed to find meter info for direct meter 0x%x "
            "attached to exm table 0x%x device %d",
            __func__,
            __LINE__,
            meter_tbl_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id);
        status = PIPE_OBJ_NOT_FOUND;
        goto cleanup;
      }
      if (meter_tbl_info->meter_type == PIPE_METER_TYPE_STANDARD) {
        pipe_meter_spec_t meter = {0};
        meter.meter_type = meter_tbl_info->enable_color_aware
                               ? METER_TYPE_COLOR_AWARE
                               : METER_TYPE_COLOR_UNAWARE;
        switch (meter_tbl_info->meter_granularity) {
          case PIPE_METER_GRANULARITY_BYTES:
            meter.cir.type = METER_RATE_TYPE_KBPS;
            meter.cir.value.kbps = meter_tbl_info->max_rate;
            meter.pir.type = METER_RATE_TYPE_KBPS;
            meter.pir.value.kbps = meter_tbl_info->max_rate;
            break;
          case PIPE_METER_GRANULARITY_PACKETS:
            meter.cir.type = METER_RATE_TYPE_PPS;
            meter.cir.value.pps = meter_tbl_info->max_rate;
            meter.pir.type = METER_RATE_TYPE_PPS;
            meter.pir.value.pps = meter_tbl_info->max_rate;
            break;
          default:
            LOG_ERROR(
                "%s:%d Invalid meter granularity type %d for meter tbl 0x%x "
                "attached to exm tbl 0x%x device %d",
                __func__,
                __LINE__,
                meter_tbl_info->meter_granularity,
                meter_tbl_hdl,
                exm_tbl->mat_tbl_hdl,
                exm_tbl->dev_id);
            status = PIPE_UNEXPECTED;
            goto cleanup;
        }
        meter.cburst = meter_tbl_info->max_burst_size;
        meter.pburst = meter_tbl_info->max_burst_size;
        rmt_meter_mgr_direct_meter_attach(sess_hdl,
                                          exm_tbl->dev_id,
                                          meter_tbl_hdl,
                                          dir_ent_idx,
                                          exm_tbl_data->pipe_id,
                                          exm_stage_info->stage_id,
                                          &meter,
                                          &indirect_ptrs.meter_ptr);
      }
    }
  }

  /* Update registers atomically if we are not in any sort of init phase */
  if (!init) {
    status = pipe_mgr_exm_write_atomic_mod_csr(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id, true);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod csr start for exm tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_tbl->dev_id);
      goto cleanup;
    }
    status = pipe_mgr_exm_write_atomic_mod_sram_reg(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod sram reg for exm tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_tbl->dev_id);
      goto cleanup;
    }
  }
  status = pipe_mgr_exm_program_default_entry(sess_hdl,
                                              exm_tbl,
                                              exm_tbl_data,
                                              exm_stage_info,
                                              act_fn_hdl,
                                              act_data_spec,
                                              &indirect_ptrs);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in programming default entry for tbl %d, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    goto cleanup;
  }
  if (!init) {
    status = pipe_mgr_exm_write_atomic_mod_csr(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info->stage_id, false);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error writing atomic mod csr end for exm tbl 0x%x pipe %d "
          "device %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_tbl->dev_id);
      goto cleanup;
    }
  }

cleanup:
  if (act_data_spec) {
    if (act_data_spec->action_data_bits) {
      PIPE_MGR_FREE(act_data_spec->action_data_bits);
      act_data_spec->action_data_bits = NULL;
    }
    PIPE_MGR_FREE(act_data_spec);
    act_data_spec = NULL;
  }
  return status;
}

pipe_status_t pipe_mgr_exm_reset_default_entry(pipe_sess_hdl_t sess_hdl,
                                               bf_dev_id_t dev_id,
                                               pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  uint32_t i = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s:%d Could not get the exact match table info for table "
        " with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    exm_stage_info =
        &exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1];
    status = pipe_mgr_exm_reset_default_entry_int(
        sess_hdl,
        exm_tbl,
        exm_tbl_data,
        exm_stage_info,
        exm_stage_info->default_miss_entry_idx,
        true);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error resetting default entry for exm table 0x%x on pipe %d "
          "device %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          dev_id);
      return status;
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_execute_def_entry_clear(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mat_ent_idx_t dir_entry_idx = 0;
  uintptr_t ent_hdl_p = 0;

  /* Hash action is a special case that programs unoccupied match indices
   * instead of the default registers. Handle this case separately. */
  if (exm_tbl->hash_action) {
    pipe_mat_tbl_info_t *mat_tbl_info = pipe_mgr_get_tbl_info(
        exm_tbl->dev_id, exm_tbl->mat_tbl_hdl, __func__, __LINE__);
    if (!mat_tbl_info) {
      LOG_ERROR(
          "%s:%d Unable to find match table info for exm table 0x%x device %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id);
      return PIPE_INVALID_ARG;
    }
    pipe_mgr_action_entry_t *action_entry =
        &mat_tbl_info->default_info->action_entry;

    /* Populate move node with default action spec */
    move_list_node->data = PIPE_MGR_CALLOC(sizeof(struct pipe_mgr_mat_data), 1);
    if (!move_list_node->data) {
      LOG_ERROR("%s:%d move list node allocation for tbl 0x%x",
                __func__,
                __LINE__,
                exm_tbl->mat_tbl_hdl);
      return PIPE_NO_SYS_RESOURCES;
    }
    status = pipe_mgr_create_action_spec(
        exm_tbl->dev_id, action_entry, &move_list_node->data->action_spec);
    if (status != PIPE_SUCCESS) {
      PIPE_MGR_FREE(move_list_node->data);
      return status;
    }
    move_list_node->data->act_fn_hdl = action_entry->act_fn_hdl;

    status = pipe_mgr_exm_execute_hash_action_def_entry_set(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, move_list_node);
    PIPE_MGR_FREE(move_list_node->data->action_spec.act_data.action_data_bits);
    move_list_node->data->action_spec.act_data.action_data_bits = NULL;
    PIPE_MGR_FREE(move_list_node->data);
    move_list_node->data = NULL;
    return status;
  }

  /* Delete the entry phy info */
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  map_sts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                       move_list_node->entry_hdl,
                       (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting entry phy info for entry %d, tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  /* Lock direct tables since this is a entry delete */
  status = pipe_mgr_exm_lock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Take care of direct resources only if the default entry is using them. */
  if (exm_stage_info->default_miss_entry_idx != PIPE_MGR_EXM_UNUSED_ENTRY_IDX) {
    status =
        pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                             exm_tbl,
                                             exm_tbl_data,
                                             exm_stage_info,
                                             entry_info->entry_idx,
                                             PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                             move_list_node,
                                             entry_info,
                                             false,
                                             true,
                                             false /* is_recovery */);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in handling direct resources for entry %d, tbl 0x%x, "
          "device id %d, err %s",
          __func__,
          __LINE__,
          move_list_node->entry_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      PIPE_MGR_DBGCHK(0);
      pipe_mgr_exm_unlock_dir_tbls(
          sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
      return status;
    }
  }
  status = pipe_mgr_exm_detach_indirect_resources(
      exm_tbl, exm_tbl_data, exm_stage_info, entry_info, true);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in detaching indirect resources for default entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  status = pipe_mgr_exm_reset_default_entry_int(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      entry_info->entry_idx,
      pipe_mgr_is_device_locked(exm_tbl->dev_id));
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in resetting default entry for tbl %d, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Unlock direct tables */
  status = pipe_mgr_exm_unlock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in unlocking direct addressed tables for tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  if (exm_stage_info->default_miss_entry_idx == PIPE_MGR_EXM_UNUSED_ENTRY_IDX) {
    dir_entry_idx = exm_stage_info->default_miss_entry_idx;
  } else {
    dir_entry_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
        exm_tbl, exm_stage_info, entry_info->entry_idx);
  }
  map_sts = bf_map_get_rmv(&exm_stage_info->log_idx_to_ent_hdl_htbl,
                           dir_entry_idx,
                           (void **)&ent_hdl_p);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing log dir idx to entry hdl mapping for idx "
        "%d, tbl 0x%x, device id %d, err 0x%x",
        __func__,
        __LINE__,
        entry_info->entry_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        map_sts);
    return PIPE_UNEXPECTED;
  }

  /* Safe to remove the entry */
  map_sts = bf_map_get_rmv(&exm_tbl_data->entry_phy_info_htbl,
                           move_list_node->entry_hdl,
                           (void **)&entry_info);
  PIPE_MGR_DBGCHK(map_sts == BF_MAP_OK);
  PIPE_MGR_FREE(entry_info);
  pipe_mgr_exm_reset_def_ent_installed(exm_tbl_data);
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_analyze_next_node(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_move_list_t *curr_node,
    pipe_mgr_move_list_t *next_node,
    bf_dev_pipe_t curr_pipe_id,
    pipe_mgr_exm_stage_info_t *curr_stage_info,
    pipe_mat_ent_idx_t src_stage_ent_idx,
    bool *move_in_progress) {
  bf_dev_pipe_t next_pipe_id = 0;
  dev_stage_t next_stage_id = 0;
  pipe_mgr_exm_tbl_data_t *next_exm_tbl_data = NULL;
  pipe_mat_ent_idx_t next_dst_stage_ent_idx;
  pipe_mat_ent_idx_t next_src_stage_ent_idx;
  switch (next_node->op) {
    case PIPE_MAT_UPDATE_ADD:
      next_pipe_id = get_move_list_pipe(next_node);
      next_exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, next_pipe_id);
      if (next_exm_tbl_data == NULL) {
        LOG_ERROR(
            "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
            "not found",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id,
            next_pipe_id);
        PIPE_MGR_DBGCHK(0);
        return PIPE_OBJ_NOT_FOUND;
      }
      next_stage_id = pipe_mgr_exm_get_stage_id_from_idx(
          next_exm_tbl_data, curr_node->u.single.logical_idx);
      if (next_stage_id == 0xff) {
        LOG_ERROR("%s:%d Invalid idx %d in the move list",
                  __func__,
                  __LINE__,
                  curr_node->u.single.logical_idx);
        return PIPE_INVALID_ARG;
      }
      /* If the new entry add is being added to the source location
       * of this move, then it is part of the same move chain
       */
      if (next_pipe_id == curr_pipe_id &&
          next_stage_id == curr_stage_info->stage_id) {
        next_dst_stage_ent_idx =
            next_node->u.single.logical_idx - curr_stage_info->stage_offset;
        if (next_dst_stage_ent_idx == src_stage_ent_idx) {
          *move_in_progress = true;
        } else {
          /* The next node is an unrelated entry add, which implies
           * that this move
           * should flush the move-reg addresses.
           */
          *move_in_progress = false;
        }
      } else {
        /* The next entry add is in a different stage/pipe id,
         * hence do the work of
         * unlocking if any direct addressed tables were locked.
         */
        *move_in_progress = false;
      }
      break;
    case PIPE_MAT_UPDATE_MOV:
      next_src_stage_ent_idx = pipe_mgr_exm_get_entry_loc(
          exm_tbl, next_node->entry_hdl, &next_pipe_id, &next_stage_id);
      if (next_src_stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
        return PIPE_OBJ_NOT_FOUND;
      }
      if (next_pipe_id == curr_pipe_id &&
          next_stage_id == curr_stage_info->stage_id) {
        next_dst_stage_ent_idx =
            next_node->u.single.logical_idx - curr_stage_info->stage_offset;
        if (next_dst_stage_ent_idx == src_stage_ent_idx) {
          *move_in_progress = true;
        } else {
          *move_in_progress = false;
        }
      } else {
        /* Next move is in an unrelated pipe/stage, gracefully
         * terminate the move-chain.
         */
        *move_in_progress = false;
      }
      break;
    case PIPE_MAT_UPDATE_DEL:
    /* Fallthrough */
    case PIPE_MAT_UPDATE_MOD:
    case PIPE_MAT_UPDATE_SET_DFLT:
    case PIPE_MAT_UPDATE_CLR_DFLT:
      *move_in_progress = false;
      break;
    default:
      LOG_ERROR("%s:%d Invalid operation %d in the move list",
                __func__,
                __LINE__,
                next_node->op);
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

/* The terminate move list functions are broken into two parts. When ending
 * any move-chain, two POPS are required to be done on the movereg registers.
 * This will remove the hit-inhibiting mechanism which is required "during" the
 * process of moving an entry from one location to the other. This first part
 * of the function just does the first pop, after which the new entry is written
 * Then a second POP completes the process. The new entry add is required to
 * be done in between two POPs, because if done afterwards, it leaves open a
 * window of time wherein two match entries are active which causes issues for
 * direct addressed resources and what not!
 */
static pipe_status_t pipe_mgr_exm_terminate_move_list_1(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t ttl) {
  pipe_status_t status = PIPE_SUCCESS;
  /* First POP */
  status = pipe_mgr_exm_issue_pop_instr(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, ttl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in issuing POP move addr instr for tbl 0x%x "
        "device id %d pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_terminate_move_list_2(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t ttl) {
  pipe_status_t status = PIPE_SUCCESS;
  /* Second POP */
  status = pipe_mgr_exm_issue_pop_instr(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, ttl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in issuing POP move addr instr for tbl 0x%x "
        "device id %d pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Unlock direct addressed tables */
  status = pipe_mgr_exm_unlock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in unlocking direct addressed tables for tbl 0x%x, device "
        "id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_initiate_move_list(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t src_stage_ent_idx,
    pipe_mat_ent_idx_t dst_stage_ent_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  /* First, lock the direct addressed tables */
  status = pipe_mgr_exm_lock_dir_tbls(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Push destination next */
  status = pipe_mgr_exm_issue_push_instr(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, dst_stage_ent_idx);
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Push Source next */
  status = pipe_mgr_exm_issue_push_instr(
      sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, src_stage_ent_idx);
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_execute_add_idle(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    idle_tbl_ha_cookie_t **ha_cookie,
    bf_dev_id_t device_id,
    pipe_mgr_move_list_t *move_list_node) {
  /* For hitless HA, when PD API replay happens, even for a fully reconciled
   * entry, the HLP sends a move-list node of PIPE_MAT_ENT_ADD_IDLE, indicating
   * the LLP to add an idle table entry for this entry handle. This is required
   * since TTL is not availabe during Hardware restore AND we don't know if the
   * idle table is "enabled" on the match table. It is expected that the
   * idle timeout enable is replayed at the very beginning during API replay.
   */
  pipe_status_t status = PIPE_SUCCESS;
  bool get_cookie = false;

  bf_dev_pipe_t pipe_id = get_move_list_pipe(move_list_node);

  pipe_mgr_exm_tbl_data_t *exm_tbl_data =
      pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR("%s:%d Exm tbl instance for tbl 0x%x, pipe id %d not found",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  uint8_t stage_id = pipe_mgr_exm_get_stage_id_from_idx(
      exm_tbl_data, move_list_node->u.single.logical_idx);

  pipe_mgr_exm_stage_info_t *exm_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (*ha_cookie == NULL) {
    *ha_cookie = (idle_tbl_ha_cookie_t *)PIPE_MGR_CALLOC(
        1, sizeof(idle_tbl_ha_cookie_t));
    if (*ha_cookie == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    (*ha_cookie)->stage_info = (idle_tbl_stage_info_t **)PIPE_MGR_CALLOC(
        pipe_mgr_get_num_active_pipes(exm_tbl->dev_id), sizeof(void *));
    if ((*ha_cookie)->stage_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    get_cookie = true;
  } else {
    if ((*ha_cookie)->pipe_id != pipe_id ||
        (*ha_cookie)->stage_id != stage_id) {
      get_cookie = true;
    }
  }
  if (get_cookie) {
    status = rmt_idle_get_cookie(device_id,
                                 exm_tbl->mat_tbl_hdl,
                                 pipe_id,
                                 stage_id,
                                 exm_stage_info->stage_table_handle,
                                 *ha_cookie);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting idle HA cookie for tbl hdl 0x%x, device id "
          "%d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          device_id,
          pipe_str_err(status));
      if (*ha_cookie) {
        PIPE_MGR_FREE(*ha_cookie);
      }
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }

  pipe_mat_ent_idx_t dir_entry_idx =
      pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
          exm_tbl,
          exm_stage_info,
          move_list_node->u.single.logical_idx - exm_stage_info->stage_offset);

  status = rmt_idle_add_entry(sess_hdl,
                              exm_tbl->dev_id,
                              exm_tbl->mat_tbl_hdl,
                              move_list_node->entry_hdl,
                              pipe_id,
                              stage_id,
                              exm_stage_info->stage_table_handle,
                              dir_entry_idx,
                              unpack_mat_ent_data_ttl(move_list_node->data),
                              false,
                              0);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in Adding idle entry for entry hdl %d, tbl 0x%x, device "
        "id %d, "
        "err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        device_id,
        pipe_str_err(status));
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_move_into_stash(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mat_ent_idx_t src_stage_ent_idx,
    pipe_mat_ent_idx_t dst_stage_ent_idx,
    uint32_t stash_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bool is_src_stash = false, is_dst_stash = true;

  /* Move to stash */
  /* Program the stash match address and control */
  status = pipe_mgr_stash_match_addr_control_program(sess_hdl,
                                                     exm_tbl,
                                                     exm_tbl_data,
                                                     exm_stage_info,
                                                     dst_stage_ent_idx,
                                                     stash_id,
                                                     move_list_node->data);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in programming stash match addr at stash-id %d "
        "ent_idx %d, for tbl %x device id %d, pipe id %d, stage id %d, "
        "err "
        "%s",
        __func__,
        __LINE__,
        stash_id,
        dst_stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  /* Init move */
  status = pipe_mgr_exm_initiate_move_list(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           src_stage_ent_idx,
                                           dst_stage_ent_idx);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in taking actions to initiate the move from idx "
        "%d to idx %d, tbl 0x%x, device id %d, pipe id %d, stage id "
        "%d, err %s",
        __func__,
        __LINE__,
        src_stage_ent_idx,
        dst_stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  /* Now execute the entry add at the new location */
  status = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                          exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          dst_stage_ent_idx,
                                          move_list_node,
                                          true,
                                          is_dst_stash,
                                          false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in executing entry move for entry handle %d from "
        "%d "
        "to %d for tbl %x device id %d, pipe id %d, stage id %d, err "
        "%s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        src_stage_ent_idx,
        dst_stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  status = pipe_mgr_exm_terminate_move_list_1(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      unpack_mat_ent_data_ttl(move_list_node->data));
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  /* Between the two pops, invalidate the source entry. This is
   * essential so that we don't leave two matching entries in the
   * tables.
   */
  status = pipe_mgr_exm_invalidate_entry(sess_hdl,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         src_stage_ent_idx,
                                         is_src_stash);
  if (status != PIPE_SUCCESS) {
    /* This error is pretty bad */
    LOG_ERROR(
        "%s:%d Error in invalidating entry at idx %d, stage id %d, "
        "pipe id %d, tbl 0x%x, device id %d as part of a stray move "
        "operation",
        __func__,
        __LINE__,
        src_stage_ent_idx,
        exm_stage_info->stage_id,
        exm_tbl_data->pipe_id,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  status = pipe_mgr_exm_terminate_move_list_2(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      unpack_mat_ent_data_ttl(move_list_node->data));
  if (status != PIPE_SUCCESS) {
    return status;
  }

  return status;
}

pipe_status_t pipe_mgr_exm_move_out_of_stash(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mat_ent_idx_t dst_stage_ent_idx,  // src and dst are reversed here
    pipe_mat_ent_idx_t src_stage_ent_idx,
    uint32_t stash_id) {
  pipe_status_t status = PIPE_SUCCESS;
  bool is_src_stash = true, is_dst_stash = false;

#ifdef STASHES_UNIT_TEST
  /* For unit testing stashes, move entry from sram to the stash,
     but do not bring it back to the sram.
     This allows us to test hits on stashes.
  */
  return PIPE_SUCCESS;
#endif

  (void)stash_id;
  /* Move back from stash */
  /* Init move */
  status = pipe_mgr_exm_initiate_move_list(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           src_stage_ent_idx,
                                           dst_stage_ent_idx);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in taking actions to initiate the move from idx "
        "%d to idx %d, tbl 0x%x, device id %d, pipe id %d, stage id "
        "%d, err %s",
        __func__,
        __LINE__,
        src_stage_ent_idx,
        dst_stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Now execute the entry add at the new location */
  status = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                          exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          dst_stage_ent_idx,
                                          move_list_node,
                                          true,
                                          is_dst_stash,
                                          false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in executing entry move for entry handle %d from "
        "%d "
        "to %d for tbl %x device id %d, pipe id %d, stage id %d, err "
        "%s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        src_stage_ent_idx,
        dst_stage_ent_idx,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return status;
  }

  status = pipe_mgr_exm_terminate_move_list_1(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      unpack_mat_ent_data_ttl(move_list_node->data));
  if (status != PIPE_SUCCESS) {
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  /* Between the two pops, invalidate the source entry. This is
   * essential so that we don't leave two matching entries in the
   * tables.
   */
  status = pipe_mgr_exm_invalidate_entry(sess_hdl,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         src_stage_ent_idx,
                                         is_src_stash);
  if (status != PIPE_SUCCESS) {
    /* This error is pretty bad */
    LOG_ERROR(
        "%s:%d Error in invalidating entry at idx %d, stage id %d, "
        "pipe id %d, tbl 0x%x, device id %d as part of a stray move "
        "operation",
        __func__,
        __LINE__,
        src_stage_ent_idx,
        exm_stage_info->stage_id,
        exm_tbl_data->pipe_id,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  status = pipe_mgr_exm_terminate_move_list_2(
      sess_hdl,
      exm_tbl,
      exm_tbl_data,
      exm_stage_info,
      unpack_mat_ent_data_ttl(move_list_node->data));
  if (status != PIPE_SUCCESS) {
    return status;
  }

  return status;
}

static pipe_status_t pipe_mgr_exm_update_resources(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mat_ent_idx_t ent_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;

  map_sts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                       move_list_node->entry_hdl,
                       (void **)&entry_info);
  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting phy entry info for entry %d, tbl 0x%x, device "
        "id %d",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  // Direct resources
  status =
      pipe_mgr_exm_handle_direct_resources(sess_hdl,
                                           exm_tbl,
                                           exm_tbl_data,
                                           exm_stage_info,
                                           ent_idx,
                                           PIPE_MAT_ENT_INVALID_ENTRY_INDEX,
                                           move_list_node,
                                           entry_info,
                                           false,
                                           false,
                                           false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in handling direct addressed resources for entry %d, tbl "
        "0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }

  // Detach indirect resources
  status = pipe_mgr_exm_detach_indirect_resources(
      exm_tbl, exm_tbl_data, exm_stage_info, entry_info, false);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in detaching indirect resource for entry %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }

  // Attach new indirect resources
  status = pipe_mgr_exm_attach_indirect_resources(sess_hdl,
                                                  exm_tbl,
                                                  exm_tbl_data,
                                                  exm_stage_info,
                                                  move_list_node,
                                                  &entry_info->indirect_ptrs,
                                                  false,
                                                  false,
                                                  false /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in getting indirect ptrs for entry hdl %d, tbl 0x%x, "
        "device id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}

/* Use stash to execute an entry modify */
static pipe_status_t pipe_mgr_exm_execute_entry_modify_using_stash(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mat_ent_idx_t stash_ent_idx) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mat_ent_idx_t stage_ent_idx = 0;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  uint8_t stash_id = 0;

  /* Use stashes if available as this helps in atomic entry modify */
  if (!exm_tbl->stash_available) {
    return PIPE_INVALID_ARG;
  }

  stage_ent_idx = pipe_mgr_exm_get_entry_loc(
      exm_tbl, move_list_node->entry_hdl, &pipe_id, &stage_id);
  if (stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
    LOG_ERROR(
        "%s:%d Error in getting entry loc for entry handle %d, tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  bf_map_sts_t msts = BF_MAP_OK;
  msts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                    move_list_node->entry_hdl,
                    (void **)&entry_info);
  if (msts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting exm entry for hdl %d, tbl 0x%x, device "
        "id %d",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  /* Entry will be moved to stash and back */
  /* Change operation type to move so that existing entry add api works */
  move_list_node->op = PIPE_MAT_UPDATE_MOV;

  /* Use the reserved stash match address */
  stash_id = pipe_mgr_default_stash_id_get(exm_stage_info);

  /* Move new entry into stash */
  status = pipe_mgr_exm_move_into_stash(sess_hdl,
                                        exm_tbl,
                                        exm_tbl_data,
                                        exm_stage_info,
                                        move_list_node,
                                        stage_ent_idx,
                                        stash_ent_idx,
                                        stash_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in move into stash for entry handle %d, tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }

  /* Update resources in the stash entry */
  move_list_node->op = PIPE_MAT_UPDATE_MOD;
  status = pipe_mgr_exm_update_resources(sess_hdl,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         move_list_node,
                                         stash_ent_idx);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error updating resources in stash for entry handle %d, tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }

  move_list_node->op = PIPE_MAT_UPDATE_MOV;
  /* Move entry out of stash */
  status = pipe_mgr_exm_move_out_of_stash(sess_hdl,
                                          exm_tbl,
                                          exm_tbl_data,
                                          exm_stage_info,
                                          move_list_node,
                                          stage_ent_idx,
                                          stash_ent_idx,
                                          stash_id);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in move out of stash for entry handle %d, tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    PIPE_MGR_DBGCHK(0);
    goto cleanup;
  }

  pipe_action_spec_t *action_spec = NULL;
  action_spec = unpack_mat_ent_data_as(move_list_node->data);
  entry_info->adt_ent_hdl = action_spec->adt_ent_hdl;
  entry_info->action_idx = move_list_node->logical_action_idx;
  entry_info->act_fn_hdl = unpack_mat_ent_data_afun_hdl(move_list_node->data);
  if (IS_ACTION_SPEC_SEL_GRP(action_spec)) {
    PIPE_MGR_DBGCHK(exm_tbl->num_sel_tbl_refs);
    entry_info->selector_enabled = true;
  } else {
    entry_info->selector_enabled = false;
  }

cleanup:
  /* Change back the operation type to modify */
  move_list_node->op = PIPE_MAT_UPDATE_MOD;

  return status;
}

/* Does this entry update need an atomic modify */
static bool pipe_mgr_exm_entry_modify_needs_atomic_change(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_move_list_t *move_list_node) {
  bool atomic_modify = false;
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t num_ram_units = 0;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  pipe_mat_ent_idx_t stage_ent_idx = 0;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  bf_map_sts_t msts = BF_MAP_OK;

#ifdef STASHES_UNIT_TEST
  /* For unit testing stashes, move entry from sram to the stash,
     but do not bring it back to the sram.
     This allows us to test hits on stashes.
  */
  return true;
#endif

  pipe_act_fn_hdl_t act_fn_hdl =
      unpack_mat_ent_data_afun_hdl(move_list_node->data);
  stage_ent_idx = pipe_mgr_exm_get_entry_loc(
      exm_tbl, move_list_node->entry_hdl, &pipe_id, &stage_id);
  if (stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
    LOG_ERROR(
        "%s:%d Error in getting entry loc for entry handle %d, tbl "
        "0x%x, device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    return false;
  }

  msts = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                    move_list_node->entry_hdl,
                    (void **)&entry_info);
  if (msts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting exm entry for hdl %d, tbl 0x%x, device "
        "id %d",
        __func__,
        __LINE__,
        move_list_node->entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    return false;
  }

  /*  No need for atomic modify if there is there is no direct action ram */
  if (!exm_tbl->num_adt_refs ||
      exm_tbl->adt_tbl_refs->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    return false;
  }

  /* Invalidate the entry if the table uses direct wide action rams */
  status = pipe_mgr_adt_get_num_ram_units(exm_tbl->dev_id,
                                          exm_tbl->adt_tbl_refs->tbl_hdl,
                                          exm_tbl_data->pipe_id,
                                          exm_stage_info->stage_id,
                                          &num_ram_units);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Failed to get action ram info for exm tbl 0x%x in pipe %d "
        "stage %d device %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        exm_tbl->dev_id);
    return status;
  }
  if (num_ram_units > 1) {
    atomic_modify = true;
  }

  if (!atomic_modify) {
    if (act_fn_hdl != entry_info->act_fn_hdl) {
      /* Invalidate the entry if the action itself changed */
      atomic_modify = true;
    } else {
      /* Invalidate the entry if the table uses immediate data */
      status =
          pipe_mgr_entry_format_tof_tbl_uses_imm_data(exm_tbl->dev_id,
                                                      exm_tbl->profile_id,
                                                      exm_stage_info->stage_id,
                                                      exm_tbl->mat_tbl_hdl,
                                                      act_fn_hdl,
                                                      false,
                                                      &atomic_modify);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Failed to get entry encoding info for exm table 0x%x in "
            "pipe %d stage %d device %d",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            exm_stage_info->stage_id,
            exm_tbl->dev_id);
        return status;
      }
    }
  }

  return atomic_modify;
}

static void exm_llp_idx_to_occ_free(pipe_mgr_exm_stage_info_t *stage_info,
                                    pipe_mat_ent_idx_t entry_idx) {
  bf_map_rmv(&stage_info->log_idx_to_occ, entry_idx);
}
static void exm_llp_idx_to_occ_update(pipe_mgr_exm_stage_info_t *stage_info,
                                      pipe_mat_ent_idx_t entry_idx) {
  bf_map_add(&stage_info->log_idx_to_occ, entry_idx, (void *)(uintptr_t)1);
}
pipe_status_t pipe_mgr_exm_tbl_process_move_list(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t dev_id,
    pipe_tbl_hdl_t tbl_hdl,
    pipe_mgr_move_list_t *pipe_move_list,
    uint32_t *num_performed) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_move_list_t *traverser = NULL;
  bool move_in_progress = false;
  bool failure = false;
  enum pipe_mat_update_type op_type;
  bf_dev_pipe_t pipe_id = 0;
  dev_stage_t stage_id = 0;
  bool valid = false;
  uint32_t num_successful = 0;
  idle_tbl_ha_cookie_t *idle_ha_cookie = NULL;
  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    PIPE_MGR_DBGCHK(0);
    status = PIPE_OBJ_NOT_FOUND;
    goto cleanup;
  }
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mat_ent_idx_t src_stage_ent_idx;
  pipe_mat_ent_idx_t dst_stage_ent_idx;
  traverser = pipe_move_list;
  while (traverser) {
    if (failure) {
      /* Terminate the processing on first failure */
      break;
    }
    op_type = traverser->op;
    LOG_TRACE("Dev %d pipe %x Tbl %s 0x%x entry %d op %s",
              dev_id,
              traverser->pipe,
              exm_tbl->name,
              tbl_hdl,
              traverser->entry_hdl,
              pipe_mgr_move_list_op_str(op_type));
    /* For hash action tables MOD operations on the default entry are handled
     * as part of the SET_DFLT logic. */
    if (exm_tbl->hash_action && op_type == PIPE_MAT_UPDATE_MOD) {
      pipe_mgr_exm_tbl_data_t *inst =
          pipe_mgr_exm_tbl_get_instance(exm_tbl, get_move_list_pipe(traverser));
      if (pipe_mgr_exm_get_def_ent_hdl(inst) == traverser->entry_hdl) {
        op_type = PIPE_MAT_UPDATE_SET_DFLT;
      }
    }
    switch (op_type) {
      case PIPE_MAT_UPDATE_ADD:
        pipe_id = get_move_list_pipe(traverser);
        valid = pipe_mgr_exm_validate_entry_add(exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                traverser->u.single.logical_idx,
                                                traverser);
        if (!valid) {
          /* Break */
          failure = true;
          /* Need to disable the previous source of the move */
          PIPE_MGR_DBGCHK(0);
          break;
        }
        if (move_in_progress) {
          /* This add terminates a move chain. At the beginning of the move
           * chain, following things would be done
           *  1. Lock IDLE/STATS tables if any associated with the match table.
           *  2. Set up the move-reg mechanism.
           * These must be undone. #2 is undone by popping the move-reg twice
           * to flush it out and 1 is undone by unlocking the locked tables.
           */
          /* First, validate the add */
          PIPE_MGR_DBGCHK(!exm_tbl->hash_action);
          status = pipe_mgr_exm_terminate_move_list_1(
              sess_hdl,
              exm_tbl,
              exm_tbl_data,
              exm_stage_info,
              unpack_mat_ent_data_ttl(traverser->data));
          if (status != PIPE_SUCCESS) {
            PIPE_MGR_DBGCHK(0);
            goto cleanup;
          }
        } else {
          exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
          if (exm_tbl_data == NULL) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u instance not found",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                pipe_id,
                traverser->entry_hdl);
            PIPE_MGR_DBGCHK(0);
            status = PIPE_OBJ_NOT_FOUND;
            goto cleanup;
          }
          stage_id = pipe_mgr_exm_get_stage_id_from_idx(
              exm_tbl_data, traverser->u.single.logical_idx);
          if (stage_id == 0xff) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u invalid idx %u",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                pipe_id,
                traverser->entry_hdl,
                traverser->u.single.logical_idx);
            PIPE_MGR_DBGCHK(0);
            status = PIPE_INVALID_ARG;
            goto cleanup;
          }
          exm_stage_info =
              pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
          if (exm_stage_info == NULL) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u idx %u stage %d "
                "info not found",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                pipe_id,
                traverser->entry_hdl,
                traverser->u.single.logical_idx,
                stage_id);
            PIPE_MGR_DBGCHK(0);
            status = PIPE_OBJ_NOT_FOUND;
            goto cleanup;
          }
        }
        dst_stage_ent_idx =
            traverser->u.single.logical_idx - exm_stage_info->stage_offset;
        /* Execute the entry add */
        status = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                                exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                dst_stage_ent_idx,
                                                traverser,
                                                move_in_progress,
                                                false,
                                                false /* is_recovery */);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u idx %u stage %d "
              "add failed, error %s",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              traverser->u.single.logical_idx,
              stage_id,
              pipe_str_err(status));
          goto cleanup;
        }
        if (move_in_progress) {
          status = pipe_mgr_exm_terminate_move_list_2(
              sess_hdl,
              exm_tbl,
              exm_tbl_data,
              exm_stage_info,
              unpack_mat_ent_data_ttl(traverser->data));
          if (status != PIPE_SUCCESS) {
            return status;
          }
        }
        if (!exm_tbl->hash_action) {
          /* On entry add mark the table location as occupied. */
          exm_llp_idx_to_occ_update(exm_stage_info, dst_stage_ent_idx);
        }
        move_in_progress = false;
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      case PIPE_MAT_UPDATE_MOV:
        /* If a move is in progress, then just push the source in the move-addr
           register, else need to push both.
         */
        PIPE_MGR_DBGCHK(!exm_tbl->hash_action);
        src_stage_ent_idx = pipe_mgr_exm_get_entry_loc(
            exm_tbl, traverser->entry_hdl, &pipe_id, &stage_id);
        if (src_stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
          /* Gracefully terminate the move process */
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        if (move_in_progress) {
          status = pipe_mgr_exm_issue_push_instr(sess_hdl,
                                                 exm_tbl,
                                                 exm_tbl_data,
                                                 exm_stage_info,
                                                 src_stage_ent_idx);
          if (status != PIPE_SUCCESS) {
            PIPE_MGR_DBGCHK(0);
            goto cleanup;
          }
          dst_stage_ent_idx =
              traverser->u.single.logical_idx - exm_stage_info->stage_offset;
        } else {
          exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
          if (exm_tbl_data == NULL) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u instance not found",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                pipe_id,
                traverser->entry_hdl);
            PIPE_MGR_DBGCHK(0);
            status = PIPE_OBJ_NOT_FOUND;
            goto cleanup;
          }
          exm_stage_info =
              pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
          if (exm_stage_info == NULL) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u idx %u stage %d "
                "info not found",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                pipe_id,
                traverser->entry_hdl,
                traverser->u.single.logical_idx,
                stage_id);
            PIPE_MGR_DBGCHK(0);
            status = PIPE_OBJ_NOT_FOUND;
            goto cleanup;
          }
          dst_stage_ent_idx =
              traverser->u.single.logical_idx - exm_stage_info->stage_offset;
          status = pipe_mgr_exm_initiate_move_list(sess_hdl,
                                                   exm_tbl,
                                                   exm_tbl_data,
                                                   exm_stage_info,
                                                   src_stage_ent_idx,
                                                   dst_stage_ent_idx);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d move %d to "
                "%d, error %s",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                traverser->entry_hdl,
                stage_id,
                src_stage_ent_idx,
                dst_stage_ent_idx,
                pipe_str_err(status));
            PIPE_MGR_DBGCHK(0);
            return status;
          }
        }
        /* Now execute the entry add at the new location */
        status = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                                exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                dst_stage_ent_idx,
                                                traverser,
                                                true,
                                                false,
                                                false /* is_recovery */);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d move %d to "
              "%d, error %s",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              stage_id,
              src_stage_ent_idx,
              dst_stage_ent_idx,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        /* Analyze the next node in the move list to figure out if its part
         * of the same move chain.
         */
        if (traverser->next) {
          status = pipe_mgr_exm_analyze_next_node(exm_tbl,
                                                  traverser,
                                                  traverser->next,
                                                  pipe_id,
                                                  exm_stage_info,
                                                  src_stage_ent_idx,
                                                  &move_in_progress);
        }
        /* Need to terminate the move-list if the next node does not fall
         * in the same move-chain OR if there is no next node.
         */
        if (!traverser->next || !move_in_progress || status != PIPE_SUCCESS) {
          /* The move chain is ending OR there was an error in analyzing
           * the next node. Both needs a graceful cleanup.
           * 1. Unlock direct addressed tables, if they were locked.
           * 2. Flush the move reg adr registers (POP & POP).
           */
          pipe_status_t cur_status = pipe_mgr_exm_terminate_move_list_1(
              sess_hdl,
              exm_tbl,
              exm_tbl_data,
              exm_stage_info,
              unpack_mat_ent_data_ttl(traverser->data));
          if (cur_status != PIPE_SUCCESS) {
            PIPE_MGR_DBGCHK(0);
            goto cleanup;
          }
          /* Between the two pops, invalidate the source entry. This is
           * essential so that we don't leave two matching entries in the
           * tables. If the move-chain continued, the next move would have
           * pushed a new source address making the current source address
           * the destination, thereby inhibiting the current source. Since
           * the move-list is not continuing, we need to invalidate the entry
           */
          cur_status = pipe_mgr_exm_invalidate_entry(sess_hdl,
                                                     exm_tbl,
                                                     exm_tbl_data,
                                                     exm_stage_info,
                                                     src_stage_ent_idx,
                                                     false);
          if (cur_status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %u idx %d "
                "error %s invalidating",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                traverser->entry_hdl,
                exm_stage_info->stage_id,
                src_stage_ent_idx,
                pipe_str_err(cur_status));
            PIPE_MGR_DBGCHK(0);
            status = status ? status : cur_status;
            goto cleanup;
          }
          cur_status = pipe_mgr_exm_terminate_move_list_2(
              sess_hdl,
              exm_tbl,
              exm_tbl_data,
              exm_stage_info,
              unpack_mat_ent_data_ttl(traverser->data));
          if (cur_status != PIPE_SUCCESS) {
            status = status ? status : cur_status;
            return status;
          }
          if (!exm_tbl->hash_action) {
            /* This is a move without a terminating add so mark the move source
             * location as free now. */
            exm_llp_idx_to_occ_free(exm_stage_info, src_stage_ent_idx);
          }
        }
        if (status != PIPE_SUCCESS) {
          return status;
        }
        if (!exm_tbl->hash_action) {
          /* Mark the move destination as occupied. */
          exm_llp_idx_to_occ_update(exm_stage_info, dst_stage_ent_idx);
        }
        break;
      case PIPE_MAT_UPDATE_MOD:
        pipe_mgr_exm_get_entry_loc(
            exm_tbl, traverser->entry_hdl, &pipe_id, &stage_id);
        exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
        if (exm_tbl_data == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u instance not found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        exm_stage_info =
            pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
        if (exm_stage_info == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d info not "
              "found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl,
              stage_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }

        /* Use stashes if available for atomic modify */
        /* Skip stashes for default entry as it cannot be used for it */
        pipe_mat_ent_idx_t stash_ent_idx = 0;
        if ((!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data,
                                              traverser->entry_hdl)) &&
            exm_tbl->stash_available && exm_stage_info->stash &&
            pipe_mgr_exm_entry_modify_needs_atomic_change(
                exm_tbl, exm_tbl_data, exm_stage_info, traverser) &&
            (pipe_mgr_exm_tbl_free_entry_idx_get(
                 exm_tbl, exm_stage_info, &stash_ent_idx) == PIPE_SUCCESS)) {
          LOG_TRACE(
              "%s:%d Found free index %d to use for stash for atomic modify "
              "tbl 0x%x, entry_hdl %d, device id %d, pipe id %d, stage id %d,",
              __func__,
              __LINE__,
              stash_ent_idx,
              exm_tbl->mat_tbl_hdl,
              traverser->entry_hdl,
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_stage_info->stage_id);
          status = pipe_mgr_exm_execute_entry_modify_using_stash(sess_hdl,
                                                                 exm_tbl,
                                                                 exm_tbl_data,
                                                                 exm_stage_info,
                                                                 traverser,
                                                                 stash_ent_idx);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d mod w/ "
                "stash error %s",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                traverser->entry_hdl,
                exm_stage_info->stage_id,
                pipe_str_err(status));
            PIPE_MGR_DBGCHK(0);
            return status;
          }
          num_successful++;
          if (pipe_mgr_sess_in_batch(sess_hdl)) {
            pipe_mgr_drv_ilist_chkpt(sess_hdl);
          }
        } else {
          status = pipe_mgr_exm_execute_entry_modify(
              sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, traverser);
          if (status != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d mod error "
                "%s",
                __func__,
                __LINE__,
                exm_tbl->dev_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_tbl_data->pipe_id,
                traverser->entry_hdl,
                exm_stage_info->stage_id,
                pipe_str_err(status));
            PIPE_MGR_DBGCHK(0);
            return status;
          }
          num_successful++;
          if (pipe_mgr_sess_in_batch(sess_hdl)) {
            pipe_mgr_drv_ilist_chkpt(sess_hdl);
          }
        }
        break;
      case PIPE_MAT_UPDATE_DEL:
        dst_stage_ent_idx = pipe_mgr_exm_get_entry_loc(
            exm_tbl, traverser->entry_hdl, &pipe_id, &stage_id);
        if (dst_stage_ent_idx == PIPE_MAT_ENT_INVALID_ENTRY_INDEX) {
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
        if (exm_tbl_data == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u instance not found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        exm_stage_info =
            pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
        if (exm_stage_info == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d info not "
              "found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl,
              stage_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        /* Lock direct addressed IDLE/STATS tables if any/if needed */
        status = pipe_mgr_exm_lock_dir_tbls(
            sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d error %s "
              "locking direct resources",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              exm_stage_info->stage_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        /* Now execute the delete */
        status = pipe_mgr_exm_execute_entry_delete(sess_hdl,
                                                   exm_tbl,
                                                   exm_tbl_data,
                                                   exm_stage_info,
                                                   dst_stage_ent_idx,
                                                   traverser,
                                                   false);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d error %s "
              "deleting",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              exm_stage_info->stage_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        /* Unlock the direct addressed tables */
        status = pipe_mgr_exm_unlock_dir_tbls(
            sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d error %s "
              "unlocking direct resources",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              exm_stage_info->stage_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        /* If hash-action, add the Default entry */
        if (exm_tbl->hash_action) {
          pipe_mgr_hash_action_add_default_entry(
              sess_hdl,
              exm_tbl,
              exm_tbl_data,
              exm_stage_info,
              dst_stage_ent_idx + exm_stage_info->stage_offset,
              false /* is_recovery */);
        } else {
          exm_llp_idx_to_occ_free(exm_stage_info, dst_stage_ent_idx);
        }
        break;
      case PIPE_MAT_UPDATE_SET_DFLT:
        pipe_id = get_move_list_pipe(traverser);
        exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
        if (exm_tbl_data == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u instance not found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        stage_id =
            exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1].stage_id;
        exm_stage_info =
            pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
        if (exm_stage_info == NULL) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d info not "
              "found",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              pipe_id,
              traverser->entry_hdl,
              stage_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        status = pipe_mgr_exm_execute_def_entry_program(
            sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, traverser);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x entry %u stage %d error %s "
              "dflt-set",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              traverser->entry_hdl,
              exm_stage_info->stage_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      case PIPE_MAT_UPDATE_CLR_DFLT:
        pipe_id = get_move_list_pipe(traverser);
        exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
        if (exm_tbl_data == NULL) {
          LOG_ERROR("%s:%d Dev %d tbl %s 0x%x pipe %x instance not found",
                    __func__,
                    __LINE__,
                    exm_tbl->dev_id,
                    exm_tbl->name,
                    exm_tbl->mat_tbl_hdl,
                    pipe_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        stage_id =
            exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1].stage_id;
        exm_stage_info =
            pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, stage_id);
        if (exm_stage_info == NULL) {
          LOG_ERROR("%s:%d Dev %d tbl %s 0x%x pipe %x stage %d info not found",
                    __func__,
                    __LINE__,
                    exm_tbl->dev_id,
                    exm_tbl->name,
                    exm_tbl->mat_tbl_hdl,
                    pipe_id,
                    stage_id);
          PIPE_MGR_DBGCHK(0);
          status = PIPE_OBJ_NOT_FOUND;
          goto cleanup;
        }
        status = pipe_mgr_exm_execute_def_entry_clear(
            sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, traverser);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Dev %d tbl %s 0x%x pipe %x stage %d error %s dflt-clr",
              __func__,
              __LINE__,
              exm_tbl->dev_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl_data->pipe_id,
              exm_stage_info->stage_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        num_successful++;
        if (pipe_mgr_sess_in_batch(sess_hdl)) {
          pipe_mgr_drv_ilist_chkpt(sess_hdl);
        }
        break;
      case PIPE_MAT_UPDATE_ADD_IDLE:
        status = pipe_mgr_exm_execute_add_idle(
            sess_hdl, exm_tbl, &idle_ha_cookie, dev_id, traverser);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in setting TTL for tbl 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              dev_id,
              pipe_str_err(status));
          PIPE_MGR_DBGCHK(0);
          goto cleanup;
        }
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        status = PIPE_NOT_SUPPORTED;
        goto cleanup;
        break;
    }
    traverser = traverser->next;
  }
cleanup:
  *num_performed = num_successful;
  if (idle_ha_cookie) {
    if (idle_ha_cookie->stage_info) {
      PIPE_MGR_FREE(idle_ha_cookie->stage_info);
    }
    PIPE_MGR_FREE(idle_ha_cookie);
  }
  return status;
}

void pipe_mgr_exm_print_match_spec(bf_dev_id_t device_id,
                                   pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                   pipe_tbl_match_spec_t *match_spec,
                                   char *buf,
                                   size_t buf_len) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  size_t bytes_written = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    if (buf && buf_len > 0) {
      buf[0] = '\0';
    }
    return;
  }

  pipe_mgr_entry_format_print_match_spec(device_id,
                                         exm_tbl->profile_id,
                                         mat_tbl_hdl,
                                         match_spec,
                                         buf,
                                         buf_len,
                                         &bytes_written);

  return;
}

pipe_status_t pipe_mgr_exm_log_state(bf_dev_id_t dev_id,
                                     pipe_mat_tbl_info_t *mat_info,
                                     pipe_mat_tbl_hdl_t tbl_hdl,
                                     cJSON *match_tbls) {
  bf_map_sts_t st;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_phy_entry_info_t *phy_info = NULL;
  pipe_mat_ent_hdl_t ent_hdl;
  uintptr_t ent_hdl_p = 0;
  unsigned long ent_hdl_key;
  unsigned long ent_idx;
  uint32_t pipe_idx;
  cJSON *match_tbl, *pipe_tbls, *pipe_tbl, *llp;
  cJSON *def_ent, *mat_ents, *mat_ent;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  cJSON_AddItemToArray(match_tbls, match_tbl = cJSON_CreateObject());
  cJSON_AddStringToObject(match_tbl, "name", exm_tbl->name);
  cJSON_AddNumberToObject(match_tbl, "handle", tbl_hdl);
  cJSON_AddBoolToObject(match_tbl, "symmetric", exm_tbl->symmetric);
  if (mat_info) {
    cJSON_AddBoolToObject(
        match_tbl, "duplicate_entry_check", mat_info->duplicate_entry_check);
  }
  cJSON_AddItemToObject(
      match_tbl, "pipe_tbls", pipe_tbls = cJSON_CreateArray());

  for (pipe_idx = 0; pipe_idx < exm_tbl->num_tbls; pipe_idx++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
    cJSON_AddItemToArray(pipe_tbls, pipe_tbl = cJSON_CreateObject());
    cJSON_AddNumberToObject(pipe_tbl, "pipe_id", exm_tbl_data->pipe_id);
    cJSON_AddItemToObject(
        pipe_tbl, "default_entry", def_ent = cJSON_CreateObject());
    /* Do not log state for the default entry of a hash action table.  There is
     * no entry-info created for hash-action default entries. */
    if (exm_tbl_data->default_entry_placed && !exm_tbl->hash_action) {
      cJSON_AddNumberToObject(
          def_ent, "entry_hdl", exm_tbl_data->default_entry_hdl);

      entry_info =
          pipe_mgr_exm_get_entry_info(exm_tbl, exm_tbl_data->default_entry_hdl);
      if (entry_info == NULL) {
        LOG_ERROR(
            "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d "
            "not found",
            __func__,
            __LINE__,
            exm_tbl_data->default_entry_hdl,
            exm_tbl->mat_tbl_hdl,
            exm_tbl->dev_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      cJSON_AddNumberToObject(
          def_ent, "action_idx", entry_info->logical_action_idx);
      cJSON_AddNumberToObject(def_ent, "sel_idx", entry_info->logical_sel_idx);
      cJSON_AddNumberToObject(def_ent, "sel_len", entry_info->selector_len);
      pipe_mgr_tbl_log_specs(dev_id,
                             exm_tbl->profile_id,
                             tbl_hdl,
                             entry_info->entry_data,
                             def_ent,
                             true);
    }
    if (exm_tbl_data->default_entry_installed) {
      if (!exm_tbl_data->default_entry_placed) {
        cJSON_AddNumberToObject(
            def_ent, "entry_hdl", exm_tbl_data->default_entry_hdl);
      }
      st = bf_map_get(&exm_tbl_data->entry_phy_info_htbl,
                      exm_tbl_data->default_entry_hdl,
                      (void **)&phy_info);
      if (st == BF_MAP_OK) {
        populate_exm_addr_node(phy_info, def_ent);
      }
    }

    cJSON_AddItemToObject(
        pipe_tbl, "match_entries", mat_ents = cJSON_CreateArray());
    st = bf_map_get_first(
        &exm_tbl_data->ent_idx_to_ent_hdl_htbl, &ent_idx, (void **)&ent_hdl_p);
    while (st == BF_MAP_OK) {
      ent_hdl = (pipe_mat_ent_hdl_t)ent_hdl_p;
      if (!exm_tbl_data->default_entry_placed ||
          ent_hdl != exm_tbl_data->default_entry_hdl) {
        st = bf_map_get(
            &exm_tbl_data->entry_info_htbl, ent_hdl, (void **)&entry_info);
        if (st == BF_MAP_OK) {
          cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
          cJSON_AddNumberToObject(mat_ent, "entry_hdl", ent_hdl);
          cJSON_AddNumberToObject(mat_ent, "entry_idx", ent_idx);
          cJSON_AddNumberToObject(
              mat_ent, "action_idx", entry_info->logical_action_idx);
          cJSON_AddNumberToObject(
              mat_ent, "sel_idx", entry_info->logical_sel_idx);
          cJSON_AddNumberToObject(mat_ent, "sel_len", entry_info->selector_len);
          pipe_mgr_tbl_log_specs(dev_id,
                                 exm_tbl->profile_id,
                                 tbl_hdl,
                                 entry_info->entry_data,
                                 mat_ent,
                                 false);
        }
      }
      st = bf_map_get_next(&exm_tbl_data->ent_idx_to_ent_hdl_htbl,
                           &ent_idx,
                           (void **)&ent_hdl_p);
    }
  }

  cJSON_AddItemToObject(match_tbl, "llp", llp = cJSON_CreateObject());
  cJSON_AddItemToObject(llp, "match_entries", mat_ents = cJSON_CreateArray());

  /* Walk through all table instances */
  for (pipe_idx = 0; pipe_idx < exm_tbl->num_tbls; pipe_idx++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
    st = bf_map_get_first(
        &exm_tbl_data->entry_phy_info_htbl, &ent_hdl_key, (void **)&phy_info);
    while (st == BF_MAP_OK) {
      cJSON_AddItemToArray(mat_ents, mat_ent = cJSON_CreateObject());
      cJSON_AddNumberToObject(mat_ent, "entry_hdl", ent_hdl_key);
      populate_exm_addr_node(phy_info, mat_ent);
      st = bf_map_get_next(
          &exm_tbl_data->entry_phy_info_htbl, &ent_hdl_key, (void **)&phy_info);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_restore_state(bf_dev_id_t dev_id,
                                         pipe_mat_tbl_info_t *mat_info,
                                         cJSON *match_tbl) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_sess_hdl_t sess_hdl = pipe_mgr_get_int_sess_hdl();
  dev_target_t dev_tgt;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_move_list_t *move_node;
  pipe_tbl_match_spec_t ms = {0};
  pipe_action_spec_t as = {0};
  pipe_act_fn_hdl_t act_fn_hdl;
  pipe_mat_tbl_hdl_t tbl_hdl;
  pipe_mat_ent_hdl_t ent_hdl;
  uint32_t pipe_idx;
  bool symmetric;
  uint32_t success_count = 0;
  cJSON *pipe_tbls, *pipe_tbl, *llp;
  cJSON *def_ent, *mat_ents, *mat_ent;
  scope_pipes_t scopes = 0xf;

  dev_tgt.device_id = dev_id;
  tbl_hdl = cJSON_GetObjectItem(match_tbl, "handle")->valueint;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  symmetric = (cJSON_GetObjectItem(match_tbl, "symmetric")->type == cJSON_True);
  if (symmetric != exm_tbl->symmetric) {
    sts = pipe_mgr_exm_tbl_set_symmetric_mode(
        dev_id, tbl_hdl, symmetric, 1, &scopes);
    if (sts != PIPE_SUCCESS) {
      LOG_ERROR("Failed to set %ssymmetric mode on dev %u, exm tbl 0x%x",
                symmetric ? "" : "non-",
                dev_id,
                tbl_hdl);
      goto done;
    }
  }
  if (mat_info && tbl_hdl == mat_info->handle) {
    mat_info->duplicate_entry_check =
        (cJSON_GetObjectItem(match_tbl, "duplicate_entry_check")->type ==
         cJSON_True);
  }

  pipe_tbls = cJSON_GetObjectItem(match_tbl, "pipe_tbls");
  for (pipe_tbl = pipe_tbls->child, pipe_idx = 0; pipe_tbl;
       pipe_tbl = pipe_tbl->next, pipe_idx++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
    dev_tgt.dev_pipe_id =
        (uint32_t)cJSON_GetObjectItem(pipe_tbl, "pipe_id")->valueint;
    PIPE_MGR_DBGCHK(exm_tbl_data->pipe_id == dev_tgt.dev_pipe_id);

    def_ent = cJSON_GetObjectItem(pipe_tbl, "default_entry");
    exm_tbl->restore_ent_node = def_ent;
    if (cJSON_GetObjectItem(def_ent, "entry_hdl")) {
      ent_hdl = cJSON_GetObjectItem(def_ent, "entry_hdl")->valuedouble;
      if (cJSON_GetObjectItem(def_ent, "act_spec")) {
        pipe_mgr_tbl_restore_specs(dev_id,
                                   exm_tbl->profile_id,
                                   tbl_hdl,
                                   def_ent,
                                   &ms,
                                   &as,
                                   &act_fn_hdl);
        sts = pipe_mgr_exm_default_ent_place(
            dev_tgt, tbl_hdl, act_fn_hdl, &as, 0, &ent_hdl, NULL);
        if (as.act_data.action_data_bits) {
          PIPE_MGR_FREE(as.act_data.action_data_bits);
        }
        if (sts != PIPE_SUCCESS) {
          goto done;
        }
      }

      if (cJSON_GetObjectItem(def_ent, "ptrs")) {
        entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, ent_hdl);
        if (entry_info == NULL) {
          LOG_ERROR(
              "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d "
              "not found",
              __func__,
              __LINE__,
              ent_hdl,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id);
          return PIPE_OBJ_NOT_FOUND;
        }
        move_node = alloc_move_list(
            NULL, PIPE_MAT_UPDATE_SET_DFLT, exm_tbl_data->pipe_id);
        if (move_node == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          sts = PIPE_NO_SYS_RESOURCES;
          goto done;
        }
        pipe_mgr_exm_populate_pipe_move_list(
            exm_tbl, exm_tbl_data, entry_info, move_node, NULL);
        success_count = 0;
        sts = pipe_mgr_exm_tbl_process_move_list(
            sess_hdl, dev_id, tbl_hdl, move_node, &success_count);
        free_move_list(&move_node, true);
        if (sts != PIPE_SUCCESS) {
          goto done;
        }
      }
    }

    mat_ents = cJSON_GetObjectItem(pipe_tbl, "match_entries");
    for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
      exm_tbl->restore_ent_node = mat_ent;
      ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;

      pipe_mgr_tbl_restore_specs(
          dev_id, exm_tbl->profile_id, tbl_hdl, mat_ent, &ms, &as, &act_fn_hdl);
      sts = pipe_mgr_exm_ent_place_with_hdl_internal(dev_tgt,
                                                     exm_tbl,
                                                     exm_tbl_data,
                                                     tbl_hdl,
                                                     &ms,
                                                     act_fn_hdl,
                                                     &as,
                                                     0,
                                                     0,
                                                     ent_hdl,
                                                     NULL);
      if (sts == PIPE_SUCCESS && mat_info) {
        sts = pipe_mgr_mat_tbl_key_insert(
            dev_id, mat_info, &ms, ent_hdl, exm_tbl_data->pipe_id, false);
      }
      PIPE_MGR_FREE(ms.match_value_bits);
      PIPE_MGR_FREE(ms.match_mask_bits);
      PIPE_MGR_FREE(as.act_data.action_data_bits);
      if (sts != PIPE_SUCCESS) {
        goto done;
      }
    }
  }

  llp = cJSON_GetObjectItem(match_tbl, "llp");
  mat_ents = cJSON_GetObjectItem(llp, "match_entries");
  for (mat_ent = mat_ents->child; mat_ent; mat_ent = mat_ent->next) {
    exm_tbl->restore_ent_node = mat_ent;
    ent_hdl = cJSON_GetObjectItem(mat_ent, "entry_hdl")->valuedouble;
    entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, ent_hdl);
    if (entry_info == NULL) {
      LOG_ERROR(
          "%s:%d Entry info for entry handle %d, tbl 0x%x, device id %d "
          "not found",
          __func__,
          __LINE__,
          ent_hdl,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id);
      sts = PIPE_OBJ_NOT_FOUND;
      goto done;
    }
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
    if (exm_tbl_data == NULL) {
      LOG_ERROR(
          "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
          "not found",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          entry_info->pipe_id);
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_OBJ_NOT_FOUND;
      goto done;
    }

    move_node = alloc_move_list(NULL, PIPE_MAT_UPDATE_ADD, entry_info->pipe_id);
    if (move_node == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      sts = PIPE_NO_SYS_RESOURCES;
      goto done;
    }
    pipe_mgr_exm_populate_pipe_move_list(
        exm_tbl, exm_tbl_data, entry_info, move_node, NULL);

    success_count = 0;
    sts = pipe_mgr_exm_tbl_process_move_list(
        sess_hdl, dev_id, tbl_hdl, move_node, &success_count);
    free_move_list(&move_node, true);
    if (sts != PIPE_SUCCESS) {
      goto done;
    }
  }

done:
  exm_tbl->restore_ent_node = NULL;
  return sts;
}

bool pipe_mgr_exm_is_ent_hdl_default(pipe_mgr_exm_tbl_data_t *exm_tbl_data,
                                     pipe_mat_ent_hdl_t mat_ent_hdl) {
  return (mat_ent_hdl == exm_tbl_data->default_entry_hdl);
}

bool pipe_mgr_exm_is_default_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data == NULL) {
    return false;
  }
  return (exm_tbl_data->default_entry_placed);
}

void pipe_mgr_exm_set_def_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data != NULL) {
    exm_tbl_data->default_entry_placed = true;
  }
  return;
}

void pipe_mgr_exm_reset_def_ent_placed(pipe_mgr_exm_tbl_data_t *exm_tbl_data) {
  if (exm_tbl_data == NULL) {
    return;
  }
  exm_tbl_data->default_entry_placed = false;
  return;
}

void pipe_mgr_exm_compute_entry_details_from_location(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_hash_way_data_t *exm_hashway_data,
    pipe_mat_ent_idx_t entry_idx,
    pipe_mgr_exm_hash_info_for_decode_t *hash_info,
    bool **ram_unit_present,
    uint8_t *num_ram_units) {
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;
  uint64_t hash = 0;
  uint64_t mem_addr = 0;
  uint8_t wide_word_blk_idx = 0;
  uint32_t num_entries_per_wide_word_blk = 0;
  uint32_t num_entries_per_wide_word = 0;
  uint32_t accum = 0;
  uint8_t entry_position = 0;
  uint32_t hashway_ent_idx = 0;

  exm_pack_format = exm_stage_info->pack_format;
  mem_addr = (entry_idx / exm_pack_format->num_entries_per_wide_word) %
             TOF_UNIT_RAM_DEPTH(exm_tbl);

  num_entries_per_wide_word = exm_pack_format->num_entries_per_wide_word;
  num_entries_per_wide_word_blk =
      num_entries_per_wide_word * TOF_UNIT_RAM_DEPTH(exm_tbl);

  /* Get the wide word block index in which the entry is present */
  accum = exm_hashway_data->offset;
  hashway_ent_idx = entry_idx - accum;
  wide_word_blk_idx = hashway_ent_idx / num_entries_per_wide_word_blk;
  entry_position = entry_idx % num_entries_per_wide_word;

  /* Based on the entry position, which is one of the possible sub-entry
   * positions packed in the wide word, get the array of RAM units in which, the
   * entry is present, numbe of them and also, a boolean array, an element of
   * which is set to TRUE if the entry lives in that ram-unit idx within the
   * wide-word.
   */
  if (num_ram_units) {
    *num_ram_units = exm_hashway_data->num_ram_units[entry_position];
  }
  if (ram_unit_present) {
    *ram_unit_present = exm_hashway_data->ram_unit_present[entry_position];
  }

  hash |= (mem_addr << exm_hashway_data->ram_line_start_offset);
  hash |= ((wide_word_blk_idx &
            ((1ULL << exm_hashway_data->num_ram_select_bits) - 1))
           << exm_hashway_data->ram_select_start_offset);
  if (exm_hashway_data->num_subword_bits) {
    // Add entry position to the hash if present
    hash = (hash << exm_hashway_data->num_subword_bits | entry_position);
  }
  hash_info->hash = hash;
  hash_info->hash_bit_lo = exm_hashway_data->ram_line_start_offset;
  hash_info->hash_bit_hi = (exm_hashway_data->ram_line_start_offset +
                            exm_hashway_data->num_ram_line_bits - 1);
  hash_info->num_ram_select_bits = exm_hashway_data->num_ram_select_bits;
  hash_info->ram_select_lo = exm_hashway_data->ram_select_start_offset;
  hash_info->ram_select_hi = (exm_hashway_data->ram_select_start_offset +
                              exm_hashway_data->num_ram_select_bits - 1);
  hash_info->wide_hash_idx = exm_hashway_data->hash_function_id;
  hash_info->num_subword_bits = exm_hashway_data->num_subword_bits;
  return;
}

/* Update the local state corresponding to the stuff that is not read from
 * hardware - ttl value based on the API replay
 */
pipe_status_t pipe_mgr_exm_entry_update_state(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mgr_move_list_t *move_list_node,
    pipe_mgr_move_list_t **move_head_p) {
  pipe_mgr_exm_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  pipe_mat_ent_hdl_t mat_ent_hdl;
  bf_dev_pipe_t pipe_id;
  bf_map_sts_t map_sts;

  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl with hdl 0x%x, device id %d not found",
              __func__,
              __LINE__,
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  mat_ent_hdl = move_list_node->entry_hdl;
  pipe_id = get_move_list_pipe(move_list_node);

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        dev_id,
        pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  map_sts = bf_map_get(
      &exm_tbl_data->entry_info_htbl, mat_ent_hdl, (void **)&entry_info);
  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR(
        "%s:%d Entry info for entry hdl %d, tbl 0x%x, device id %d not found",
        __func__,
        __LINE__,
        mat_ent_hdl,
        mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_exm_stage_info_t *exm_stage_info =
      pipe_mgr_exm_tbl_get_stage_info(exm_tbl, pipe_id, entry_info->stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        entry_info->stage_id);
    PIPE_MGR_DBGCHK(exm_stage_info);
    return PIPE_OBJ_NOT_FOUND;
  }

  set_mat_ent_data_ttl(entry_info->entry_data,
                       unpack_mat_ent_data_ttl(move_list_node->data));

  if (move_head_p && exm_tbl->idle_present) {
    /* Here, generate MOVE-LIST to indicate to the LLP that the
     * TTL needs to be updated.
     */
    pipe_mgr_move_list_t *head = *move_head_p;
    pipe_mgr_move_list_t *move_node =
        alloc_move_list(head, PIPE_MAT_UPDATE_ADD_IDLE, pipe_id);
    if (!move_node) {
      LOG_ERROR(
          "%s:%d Fail on move list node allocation for tbl 0x%x, pipe_id %d",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          pipe_id);
      return PIPE_NO_SYS_RESOURCES;
    }
    move_node->entry_hdl = mat_ent_hdl;
    move_node->data = make_mat_ent_data(
        NULL, NULL, 0, unpack_mat_ent_data_ttl(move_list_node->data), 0, 0, 0);
    if (*move_head_p) {
      (*move_head_p)->next = move_node;
    } else {
      *move_head_p = move_node;
    }
    move_node->u.single.logical_idx =
        entry_info->entry_idx + exm_stage_info->stage_offset;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_hash_action_decode_entry(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *act_fn_hdl = exm_tbl->act_fn_hdl_info[0].act_fn_hdl;

  /* Check if the entry handle passed in a valid one */
  entry_info = pipe_mgr_exm_get_phy_entry_info(exm_tbl, entry_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Could not find the entry info for entry with handle"
        " %d in exact match table with handle %d, device_id %d",
        __func__,
        __LINE__,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl instance for tbl 0x%x, device id %d, pipe id %d "
        "not found",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id,
        entry_info->pipe_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  if ((dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) &&
      !PIPE_BITMAP_GET(&exm_tbl_data->pipe_bmp, dev_tgt.dev_pipe_id)) {
    LOG_TRACE(
        "%s:%d Invalid request to access pipe %x for table %s "
        "0x%x device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        entry_info->pipe_id,
        entry_info->stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Initialize the match spec */
  PIPE_MGR_MEMSET(pipe_match_spec->match_value_bits,
                  0,
                  sizeof(uint8_t) * exm_tbl->num_match_spec_bytes);
  /* Recover the match spec from the entry index */
  rc = bf_hash_mat_entry_hash_action_match_spec_decode_from_hash(
      exm_tbl->dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      entry_info->entry_idx,
      false /*proxy hash*/,
      pipe_match_spec);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding match spec from entry idx %d, tbl 0x%x, "
        "stage id %d, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        entry_info->entry_idx,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        exm_tbl->dev_id,
        pipe_str_err(rc));
  }

  if (!exm_tbl->num_adt_refs) {
    pipe_action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    return PIPE_SUCCESS;
  }

  /* Next, read action data params if any */
  rc = pipe_mgr_exm_decode_act_spec_for_entry(dev_tgt,
                                              exm_tbl,
                                              exm_tbl_data,
                                              exm_stage_info,
                                              *act_fn_hdl,
                                              entry_info->entry_idx,
                                              pipe_action_spec,
                                              &indirect_ptrs,
                                              true,
                                              NULL /* sess_hdl */);

  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding action data spec for entry hdl %d, tbl 0x%x, "
        "device id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(rc));
  }

  return rc;
}

/* Is entry idx a stash entry */
bool pipe_mgr_entry_idx_is_stash(pipe_mgr_exm_stage_info_t *exm_stage_info,
                                 pipe_mat_ent_idx_t stage_ent_idx) {
  // Entries are not stored in stashes currently. Only atomic modify
  // uses stashes
  (void)exm_stage_info;
  (void)stage_ent_idx;
  return false;
}

pipe_status_t pipe_mgr_exm_tbl_free_entry_idx_get(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mat_ent_idx_t *free_entry_idx) {
  /* Locate a free index in the table by finding a key that is NOT present in
   * the map. */
  int Rc_int;
  Word_t judy_index = exm_stage_info->num_entries;
  JLPE(Rc_int, exm_stage_info->log_idx_to_occ, judy_index);
  pipe_mat_ent_idx_t entry_idx = judy_index;
  if (Rc_int != 0 && exm_tbl->default_entry_reserved &&
      (entry_idx == exm_stage_info->default_miss_entry_idx)) {
    /* The seach was successful but found an entry reserved for the default
     * entry.  Search again to find the next free location. */
    JLPE(Rc_int, exm_stage_info->log_idx_to_occ, judy_index);
    entry_idx = judy_index;
  }

  if (!Rc_int) {
    return PIPE_NO_SPACE;
  }
  if (entry_idx >= exm_stage_info->num_entries) {
    return PIPE_NO_SPACE;
  }
  *free_entry_idx = entry_idx;
  return PIPE_SUCCESS;
}

uint32_t pipe_mgr_default_stash_id_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  return exm_stage_info->default_stash_id;
}

/* Get stash-info for a particular stash-id */
pipe_status_t pipe_mgr_stash_info_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t stash_id,
    pipe_mgr_exm_stash_entry_info_t **stash_ent_info) {
  uint32_t i = 0, j = 0;
  *stash_ent_info = NULL;

  if (!exm_stage_info->stash) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (exm_stage_info->stash->num_stash_entries == 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < exm_stage_info->stash->num_stash_entries; i++) {
    for (j = 0; j < exm_stage_info->stash->num_rams_per_stash; j++) {
      if (stash_id == exm_stage_info->stash->stash_entries[i][j].stash_id) {
        *stash_ent_info = &(exm_stage_info->stash->stash_entries[i][j]);
        return PIPE_SUCCESS;
      }
    }
  }

  return PIPE_INVALID_ARG;
}

/* Get stash-info at a particular mem index (for wide words)
   given the start stash-id
*/
pipe_status_t pipe_mgr_stash_info_at_wide_word_index_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t stash_id,
    uint32_t mem_index,
    pipe_mgr_exm_stash_entry_info_t **stash_ent_info) {
  uint32_t i = 0;
  *stash_ent_info = NULL;

  if (!exm_stage_info->stash) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (exm_stage_info->stash->num_stash_entries == 0) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (mem_index >= exm_stage_info->stash->num_rams_per_stash) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < exm_stage_info->stash->num_stash_entries; i++) {
    if (stash_id == exm_stage_info->stash->stash_entries[i][0].stash_id) {
      *stash_ent_info = &(exm_stage_info->stash->stash_entries[i][mem_index]);
      return PIPE_SUCCESS;
    }
  }

  return PIPE_INVALID_ARG;
}

pipe_status_t pipe_mgr_exm_decode_entry(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_hash_info_for_decode_t *hash_info,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    uint8_t entry_position,
    uint8_t **wide_word_ptrs,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    pipe_mat_ent_idx_t stage_ent_idx,
    bool *valid,
    uint64_t *proxy_hash) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t version_valid_bits = 0;

  status = pipe_mgr_entry_format_tof_exm_tbl_ent_decode_to_components(
      exm_tbl->dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      exm_stage_info->stage_table_handle,
      entry_position,
      &version_valid_bits,
      match_spec,
      &action_spec->act_data,
      wide_word_ptrs,
      hash_info,
      indirect_ptrs,
      NULL,
      act_fn_hdl,
      NULL,
      pipe_mgr_entry_idx_is_stash(exm_stage_info, stage_ent_idx),
      proxy_hash);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding exm entry for tbl 0x%x, device id %d, pipe_id "
        "%d, stage id %d, err %s",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(status));
    return status;
  }

  if (version_valid_bits == RMT_EXM_ENTRY_VERSION_INVALID) {
    *valid = false;
  } else {
    *valid = true;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_decode_act_spec_for_entry(
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_act_fn_hdl_t act_fn_hdl,
    pipe_mat_ent_idx_t entry_idx,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs,
    bool from_hw,
    pipe_sess_hdl_t *sess_hdl) {
  rmt_virt_addr_t virt_addr = 0;

  if (!exm_tbl->num_adt_refs) {
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    return PIPE_SUCCESS;
  }

  bool is_dflt = false;
  if (entry_idx == exm_stage_info->default_miss_entry_idx &&
      exm_stage_info->stage_id ==
          exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1].stage_id) {
    is_dflt = true;
  }

  if (exm_tbl->num_sel_tbl_refs && indirect_ptrs->sel_len > 0) {
    /* Since group handle is not saved in LLP state, populate the field
     * with the selector logical index instead. If we are in single-process
     * mode and HLP state exists, this value will be later replaced with
     * the actual group handle.
     */
    virt_addr = indirect_ptrs->sel_ptr;
    pipe_mgr_sel_vaddr_to_logical_idx(exm_tbl->dev_id,
                                      exm_tbl->sel_tbl_refs[0].tbl_hdl,
                                      exm_stage_info->stage_id,
                                      virt_addr,
                                      &action_spec->sel_grp_hdl);
    action_spec->pipe_action_datatype_bmap = PIPE_SEL_GRP_HDL_TYPE;
    return PIPE_SUCCESS;
  }

  pipe_status_t status = PIPE_SUCCESS;
  bool sharable = true;
  pipe_adt_tbl_hdl_t adt_tbl_hdl = exm_tbl->adt_tbl_refs[0].tbl_hdl;
  if (exm_tbl->adt_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    bool has_adt = false;
    status = pipe_mgr_entry_format_adt_tbl_used(exm_tbl->dev_info,
                                                exm_tbl->profile_id,
                                                exm_stage_info->stage_id,
                                                adt_tbl_hdl,
                                                act_fn_hdl,
                                                &has_adt);
    if (status != PIPE_SUCCESS) {
      return status;
    }
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
    if (!has_adt) {
      return PIPE_SUCCESS;
    }
    pipe_adt_ent_idx_t adt_entry_idx =
        pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
            exm_tbl, exm_stage_info, entry_idx);
    status = pipe_mgr_adt_mgr_decode_to_act_data_spec(
        dev_tgt,
        adt_tbl_hdl,
        exm_stage_info->stage_table_handle,
        adt_entry_idx,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        act_fn_hdl,
        &action_spec->act_data,
        NULL,
        from_hw,
        sess_hdl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding action spec for exm entry at idx %d, tbl "
          "0x%x, device id %d, pipe_id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
  } else {
    status =
        pipe_mgr_adt_mgr_get_ent_hdl_from_location(exm_tbl->dev_id,
                                                   adt_tbl_hdl,
                                                   exm_tbl_data->pipe_id,
                                                   exm_stage_info->stage_id,
                                                   indirect_ptrs->adt_ptr,
                                                   sharable,
                                                   &action_spec->adt_ent_hdl);
    if (is_dflt && status == PIPE_OBJ_NOT_FOUND) {
      /* Default entries may have been installed automatically without an action
       * entry. */
      action_spec->adt_ent_hdl = 0;
      status = PIPE_SUCCESS;
    } else if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting action entry hdl from location for exm entry "
          "at idx %d, tbl 0x%x, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(status));
      return status;
    }
    action_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_HDL_TYPE;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_build_indirect_resources_from_hw(
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_action_spec_t *action_spec,
    pipe_mgr_indirect_ptrs_t *indirect_ptrs) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id = 0;

  status = pipe_mgr_get_pipe_id(&exm_tbl_data->pipe_bmp,
                                dev_tgt.dev_pipe_id,
                                exm_tbl_data->pipe_id,
                                &pipe_id);
  if (status != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s:%d Invalid request to access pipe %x for table %s "
        "0x%x device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        exm_tbl->name,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id);
    return status;
  }

  /* Stats first */
  if (exm_tbl->num_stat_tbl_refs) {
    if (exm_tbl->stat_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      bool pfe = false, pfe_defaulted = false;
      pipe_res_spec_t *res_spec =
          &action_spec->resources[action_spec->resource_count];
      res_spec->tbl_hdl = exm_tbl->stat_tbl_refs[0].tbl_hdl;
      status =
          pipe_mgr_stat_mgr_decode_virt_addr(exm_tbl->dev_id,
                                             exm_tbl->stat_tbl_refs[0].tbl_hdl,
                                             pipe_id,
                                             exm_stage_info->stage_id,
                                             indirect_ptrs->stats_ptr,
                                             &pfe,
                                             &pfe_defaulted,
                                             &res_spec->tbl_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding stats addr 0x%x, for tbl 0x%x, device "
            "id %d, pipe id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->stats_ptr,
            exm_tbl->stat_tbl_refs[0].tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      if (pfe || pfe_defaulted) {
        res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
      } else {
        res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
      }
      // Restore the stat table handle
      res_spec->tbl_hdl = exm_tbl->stat_tbl_refs[0].tbl_hdl;
      action_spec->resource_count++;
    }
  }

  /* Stful next */
  if (exm_tbl->num_stful_tbl_refs) {
    if (exm_tbl->stful_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      bool pfe = false, pfe_defaulted = false;
      pipe_res_spec_t *res_spec =
          &action_spec->resources[action_spec->resource_count];
      res_spec->tbl_hdl = exm_tbl->stful_tbl_refs[0].tbl_hdl;
      status = pipe_mgr_stful_mgr_decode_virt_addr(
          exm_tbl->dev_id,
          exm_tbl->stful_tbl_refs[0].tbl_hdl,
          pipe_id,
          exm_stage_info->stage_id,
          indirect_ptrs->stfl_ptr,
          &pfe,
          &pfe_defaulted,
          &res_spec->tbl_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding stful addr 0x%x, for tbl 0x%x, device "
            "id %d, pipe id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->stfl_ptr,
            exm_tbl->stful_tbl_refs[0].tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      if (pfe || pfe_defaulted) {
        /* Stfl is enabled */
        res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
      } else {
        res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
      }
      // Restore the stful table handle
      res_spec->tbl_hdl = exm_tbl->stful_tbl_refs[0].tbl_hdl;
      action_spec->resource_count++;
    }
  }

  /* Meters next */
  if (exm_tbl->num_meter_tbl_refs) {
    if (exm_tbl->meter_tbl_refs[0].ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      bool pfe = false, pfe_defaulted = false;
      pipe_res_spec_t *res_spec =
          &action_spec->resources[action_spec->resource_count];
      res_spec->tbl_hdl = exm_tbl->meter_tbl_refs[0].tbl_hdl;
      status = pipe_mgr_meter_mgr_decode_virt_addr(
          exm_tbl->dev_id,
          exm_tbl->meter_tbl_refs[0].tbl_hdl,
          pipe_id,
          exm_stage_info->stage_id,
          indirect_ptrs->meter_ptr,
          &pfe,
          &pfe_defaulted,
          &res_spec->tbl_idx);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in decoding meter addr 0x%x, for tbl 0x%x, device "
            "id %d, pipe id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            indirect_ptrs->meter_ptr,
            exm_tbl->meter_tbl_refs[0].tbl_hdl,
            exm_tbl->dev_id,
            pipe_id,
            exm_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
      if (pfe || pfe_defaulted) {
        res_spec->tag = PIPE_RES_ACTION_TAG_ATTACHED;
      } else {
        res_spec->tag = PIPE_RES_ACTION_TAG_DETACHED;
      }
      // Restore the meter table handle
      res_spec->tbl_hdl = exm_tbl->meter_tbl_refs[0].tbl_hdl;
      action_spec->resource_count++;
    }
  }

  return PIPE_SUCCESS;
}

/* Add entry. Currently used for recovery. */
static pipe_status_t exm_tbl_add_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_entry_info_t *entry_info);

static pipe_status_t exm_tbl_hash_action_raw_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    uint32_t entry_idx,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    bool err_correction,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    pipe_mat_ent_hdl_t *entry_hdl,
    bool *is_default) {
  pipe_status_t rc;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_action_spec_t *action_spec;
  pipe_mgr_exm_entry_info_t *entry_info;
  *act_fn_hdl = exm_tbl->act_fn_hdl_info[0].act_fn_hdl;
  /* Initialize the match spec */
  PIPE_MGR_MEMSET(match_spec->match_value_bits,
                  0,
                  sizeof(uint8_t) * exm_tbl->num_match_spec_bytes);
  /* Initialize the action spec. */
  act_spec->act_data.num_valid_action_data_bits =
      exm_tbl->act_fn_hdl_info[0].num_bits;
  act_spec->act_data.num_action_data_bytes =
      exm_tbl->act_fn_hdl_info[0].num_bytes;
  /* Recover the match spec from the entry index */
  rc = bf_hash_mat_entry_hash_action_match_spec_decode_from_hash(
      exm_tbl->dev_id,
      exm_tbl->profile_id,
      exm_stage_info->stage_id,
      exm_tbl->mat_tbl_hdl,
      entry_idx,
      false /*proxy hash*/,
      match_spec);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding match spec from entry idx %d, tbl 0x%x, "
        "stage id %d, device id %d, "
        "err %s",
        __func__,
        __LINE__,
        entry_idx,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        exm_tbl->dev_id,
        pipe_str_err(rc));
    return rc;
  }

  if (!exm_tbl->num_adt_refs) {
    act_spec->pipe_action_datatype_bmap = PIPE_ACTION_DATA_TYPE;
  } else {
    /* Next, read action data params if any */
    rc = pipe_mgr_exm_decode_act_spec_for_entry(dev_tgt,
                                                exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                *act_fn_hdl,
                                                entry_idx,
                                                act_spec,
                                                &indirect_ptrs,
                                                true,
                                                &sess_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding action data spec for entry_idx %d, tbl 0x%x,"
          " device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      return rc;
    }
  }
  *entry_hdl =
      pipe_mgr_exm_get_ent_hdl_from_dir_addr(entry_idx, exm_stage_info);

  if (*entry_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
    /* This must be a default entry. */
    *is_default = true;
    *entry_hdl = exm_tbl_data->default_entry_hdl;
    if (!exm_tbl_data->hash_action_dflt_act_spec) {
      LOG_TRACE("Missing dflt entry action spec. Dev %d pipe %x Tbl %s 0x%x",
                exm_tbl->dev_id,
                exm_tbl_data->pipe_id,
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
    action_spec = exm_tbl_data->hash_action_dflt_act_spec;
    if (pipe_mgr_tbl_compare_action_specs(act_spec, action_spec)) {
      if (err_correction) {
        /* Recover the default entry installation. */
        rc = pipe_mgr_hash_action_add_default_entry(
            sess_hdl,
            exm_tbl,
            exm_tbl_data,
            exm_stage_info,
            entry_idx + exm_stage_info->stage_offset,
            true /* is_recovery */);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in correcting the default for entry_idx %d in exact "
              "match table with handle %d, stage %d,device id %d",
              __func__,
              __LINE__,
              entry_idx,
              exm_tbl->mat_tbl_hdl,
              exm_stage_info->stage_id,
              dev_tgt.device_id);
          return rc;
        }
      }
      rc = PIPE_INTERNAL_ERROR;
    }
    return rc;
  }
  /* This must be a non-default entry. */
  *is_default = false;
  if (!err_correction) return rc;

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, *entry_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting entry info for entry with handle %d in exact "
        "match table with handle %d, device id %d",
        __func__,
        __LINE__,
        *entry_hdl,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id);
    return PIPE_UNEXPECTED;
  }
  action_spec = unpack_mat_ent_data_as(entry_info->entry_data);

  if (pipe_mgr_tbl_compare_action_specs(act_spec, action_spec)) {
    /* For non-default entry, re-add the entry. */
    rc = exm_tbl_add_entry(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, entry_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Dev %d tbl %s 0x%x pipe %x entry hdl %u stage %u"
          "error %s adding",
          __func__,
          __LINE__,
          exm_tbl->dev_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          entry_info->mat_ent_hdl,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      PIPE_MGR_DBGCHK(0);
      return rc;
    }
    rc = PIPE_INTERNAL_ERROR;
  }
  return rc;
}

/* Add entry. Currently used for recovery. */
static pipe_status_t exm_tbl_add_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_mgr_exm_entry_info_t *entry_info) {
  pipe_status_t ret = PIPE_SUCCESS;

  pipe_mgr_move_list_t *node = NULL;
  struct pipe_mgr_mat_data *node_data = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;

  mat_tbl_info = pipe_mgr_get_tbl_info(
      exm_tbl->dev_id, exm_tbl->mat_tbl_hdl, __func__, __LINE__);
  if (!mat_tbl_info) {
    return PIPE_INVALID_ARG;
  }

  /* Allocate memory for node */
  node = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_move_list_t));
  node_data = PIPE_MGR_CALLOC(1, sizeof(struct pipe_mgr_mat_data));
  if (!node || !node_data) {
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_mgr_exm_populate_pipe_move_list(
      exm_tbl, exm_tbl_data, entry_info, node, NULL);
  /* Add the entry */
  ret = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                       exm_tbl,
                                       exm_tbl_data,
                                       exm_stage_info,
                                       entry_info->entry_idx,
                                       node,
                                       false,
                                       false,
                                       true /* is_recovery */);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s Failed to add entry at logical-idx 0x%x "
        "(ent_idx 0x%x) pipe %d, stage %d for tbl 0x%x",
        __func__,
        entry_info->entry_idx + exm_stage_info->stage_offset,
        entry_info->entry_idx,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        exm_tbl->mat_tbl_hdl);
  }

  PIPE_MGR_FREE(node);
  PIPE_MGR_FREE(node_data);

  return ret;
}

static pipe_status_t exm_tbl_allocate_match_and_action_spec_bits(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec) {
  if (match_spec->match_value_bits == NULL) {
    match_spec->match_value_bits = (uint8_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_match_spec_bytes, sizeof(uint8_t));
    if (match_spec->match_value_bits == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    match_spec->num_valid_match_bits = exm_tbl->num_match_spec_bits;
    match_spec->num_match_bytes = exm_tbl->num_match_spec_bytes;
  }
  if (match_spec->match_mask_bits == NULL) {
    match_spec->match_mask_bits = (uint8_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_match_spec_bytes, sizeof(uint8_t));
    if (match_spec->match_mask_bits == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    match_spec->num_valid_match_bits = exm_tbl->num_match_spec_bits;
    match_spec->num_match_bytes = exm_tbl->num_match_spec_bytes;
  }
  if (act_spec->act_data.action_data_bits == NULL &&
      exm_tbl->max_act_data_size) {
    act_spec->act_data.action_data_bits =
        (uint8_t *)PIPE_MGR_CALLOC(exm_tbl->max_act_data_size, sizeof(uint8_t));
    if (act_spec->act_data.action_data_bits == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t exm_tbl_hw_entry_get(
    pipe_sess_hdl_t sess_hdl,
    dev_target_t dev_tgt,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint32_t tbl_index,
    pipe_tbl_match_spec_t *match_spec,
    pipe_action_spec_t *act_spec,
    pipe_act_fn_hdl_t *act_fn_hdl,
    bool *valid,
    uint64_t *proxy_hash) {
  pipe_mat_ent_idx_t entry_idx;
  uint32_t entry_position = 0;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  uint8_t hashway = 0;
  uint8_t num_rams_in_wide_word = 0;
  pipe_mgr_exm_hash_info_for_decode_t hash_info;
  uint64_t *addrs = NULL;
  uint8_t *phy_addrs_map = NULL;
  uint8_t **wide_word_ptrs = NULL;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  bf_dev_pipe_t phy_pipe_id = 0;
  pipe_status_t rc;
  unsigned int i;
  pipe_act_fn_info_t *act_fn_info = NULL;

  entry_idx = tbl_index - exm_stage_info->stage_offset;
  entry_position =
      entry_idx % exm_stage_info->pack_format->num_entries_per_wide_word;

  if (exm_stage_info->num_hash_ways == 0) {
    /* No hash info is available. */
    return PIPE_INVALID_ARG;
  }
  hashway = pipe_mgr_exm_get_entry_hashway(exm_stage_info, entry_idx);

  exm_hashway_data = &exm_stage_info->hashway_data[hashway];

  num_rams_in_wide_word = exm_stage_info->pack_format->num_rams_in_wide_word;
  pipe_mgr_exm_compute_entry_details_from_location(exm_tbl,
                                                   exm_stage_info,
                                                   exm_hashway_data,
                                                   entry_idx,
                                                   &hash_info,
                                                   NULL,
                                                   NULL);

  rc = pipe_mgr_map_pipe_id_log_to_phy(
      exm_tbl->dev_info, dev_tgt.dev_pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              dev_tgt.dev_pipe_id,
              dev_tgt.device_id,
              pipe_str_err(rc));
    return rc;
  }
  addrs = (uint64_t *)PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(uint64_t));
  if (addrs == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  phy_addrs_map =
      (uint8_t *)PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(uint8_t));
  if (phy_addrs_map == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  wide_word_ptrs =
      (uint8_t **)PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(uint8_t *));
  if (wide_word_ptrs == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (i = 0; i < num_rams_in_wide_word; i++) {
    wide_word_ptrs[i] =
        (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
    if (wide_word_ptrs[i] == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    phy_addrs_map[i] = i;
  }
  /* Given the entry location, get the ram unit(s) associated with that entry.
   * The ram unit(s) associated with it is that of the entire wide-word.
   */
  pipe_mgr_exm_get_mem_ids_for_entry(exm_tbl,
                                     exm_stage_info,
                                     entry_idx,
                                     exm_stage_info->mem_id_arr,
                                     NULL,
                                     NULL);

  uint32_t mem_addr =
      (entry_idx / exm_stage_info->pack_format->num_entries_per_wide_word) %
      TOF_UNIT_RAM_DEPTH(exm_tbl);
  for (i = 0; i < num_rams_in_wide_word; i++) {
    addrs[i] = exm_tbl->dev_info->dev_cfg.get_full_phy_addr(
        exm_tbl->direction,
        phy_pipe_id,
        exm_stage_info->stage_id,
        exm_stage_info->mem_id_arr[i],
        mem_addr,
        pipe_mem_type_unit_ram);
  }
  bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
  rc = pipe_mgr_dump_any_tbl_by_addr(dev_tgt.device_id,
                                     subdev,
                                     exm_tbl->mat_tbl_hdl,
                                     exm_stage_info->stage_table_handle,
                                     exm_stage_info->stage_id,
                                     RMT_TBL_TYPE_HASH_MATCH,
                                     addrs,
                                     num_rams_in_wide_word,
                                     entry_position,
                                     0,
                                     NULL,
                                     NULL,
                                     0,
                                     wide_word_ptrs,
                                     phy_addrs_map,
                                     &sess_hdl);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in dumping the tbl_index %d, tbl 0x%x, device id %d, err "
        "%s",
        __func__,
        __LINE__,
        tbl_index,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(rc));
    goto cleanup;
  }

  *valid = false;
  rc = pipe_mgr_exm_decode_entry(exm_tbl,
                                 exm_tbl_data,
                                 exm_stage_info,
                                 &hash_info,
                                 match_spec,
                                 act_spec,
                                 act_fn_hdl,
                                 entry_position,
                                 wide_word_ptrs,
                                 &indirect_ptrs,
                                 entry_idx,
                                 valid,
                                 proxy_hash);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in decoding entry read from hardware to match spec, for "
        "tbl_index %d, tbl 0x%x, device id %d, err %s",
        __func__,
        __LINE__,
        tbl_index,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.device_id,
        pipe_str_err(rc));
    goto cleanup;
  }

  if (*valid) {
    act_fn_info = exm_tbl->act_fn_hdl_info;
    /* Compute the number of bits and bytes for action data spec. */
    for (i = 0; i < exm_tbl->num_actions; i++) {
      if (act_fn_info[i].act_fn_hdl == *act_fn_hdl) {
        act_spec->act_data.num_valid_action_data_bits = act_fn_info[i].num_bits;
        act_spec->act_data.num_action_data_bytes = act_fn_info[i].num_bytes;
        break;
      }
    }
    /* Next, read action data params if any */
    rc = pipe_mgr_exm_decode_act_spec_for_entry(dev_tgt,
                                                exm_tbl,
                                                exm_tbl_data,
                                                exm_stage_info,
                                                *act_fn_hdl,
                                                entry_idx,
                                                act_spec,
                                                &indirect_ptrs,
                                                true,
                                                &sess_hdl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding action data spec for entry idx %d, tbl"
          "0x%x, device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      goto cleanup;
    }
    /* Next, recover the indirect index by decoding the virtual addresses
     * read from hw (in the indirect_ptrs) and populate them in the
     * resource specs. */
    rc = pipe_mgr_exm_build_indirect_resources_from_hw(dev_tgt,
                                                       exm_tbl,
                                                       exm_tbl_data,
                                                       exm_stage_info,
                                                       act_spec,
                                                       &indirect_ptrs);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in build resources from hw for entry idx %d, tbl 0x%x, "
          "device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          entry_idx,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      goto cleanup;
    }
  } else {
    /* No decoding took place, because entry was not programmed. */
    match_spec->num_match_bytes = 0;
    match_spec->num_valid_match_bits = 0;
    act_spec->act_data.num_action_data_bytes = 0;
    act_spec->act_data.num_valid_action_data_bits = 0;
  }
cleanup:
  if (phy_addrs_map) {
    PIPE_MGR_FREE(phy_addrs_map);
  }
  if (addrs) {
    PIPE_MGR_FREE(addrs);
  }
  if (wide_word_ptrs) {
    for (i = 0; i < num_rams_in_wide_word; i++) {
      if (wide_word_ptrs[i]) {
        PIPE_MGR_FREE(wide_word_ptrs[i]);
      }
    }
    PIPE_MGR_FREE(wide_word_ptrs);
  }
  return rc;
}

pipe_status_t pipe_mgr_exm_tbl_raw_entry_get(pipe_sess_hdl_t sess_hdl,
                                             dev_target_t dev_tgt,
                                             pipe_mat_tbl_hdl_t tbl_hdl,
                                             uint32_t tbl_index,
                                             bool err_correction,
                                             pipe_tbl_match_spec_t *match_spec,
                                             pipe_action_spec_t *act_spec,
                                             pipe_act_fn_hdl_t *act_fn_hdl,
                                             pipe_mat_ent_hdl_t *entry_hdl,
                                             bool *is_default,
                                             uint32_t *next_index) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mat_ent_idx_t entry_idx;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mat_ent_idx_t dir_ent_idx = 0;
  pipe_mgr_exm_hash_info_for_decode_t hash_info;
  pipe_mgr_exm_entry_info_t *entry_info;
  pipe_status_t rc;
  dev_stage_t stage_id = 0;
  uint64_t proxy_hash, proxy_hash_sw;
  bool valid = false;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  PIPE_MGR_MEMSET(&hash_info, 0, sizeof(pipe_mgr_exm_hash_info_for_decode_t));
  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Request for a non-existent table with handle 0x%x device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  exm_tbl_data =
      pipe_mgr_exm_tbl_get_instance_from_any_pipe(exm_tbl, dev_tgt.dev_pipe_id);
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, tbl_data does not exist",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }
  stage_id = pipe_mgr_exm_get_stage_id_from_idx(exm_tbl_data, tbl_index);
  if (stage_id == 0xff || stage_id >= dev_info->num_active_mau) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, invalid tbl_index 0x%x",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id,
        tbl_index);
    return PIPE_INVALID_ARG;
  }
  exm_stage_info = exm_tbl_data->stage_info_ptrs[stage_id];
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        dev_tgt.dev_pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_idx = tbl_index - exm_stage_info->stage_offset;
  dir_ent_idx = pipe_mgr_exm_compute_log_ent_idx_for_dir_tbls(
      exm_tbl, exm_stage_info, entry_idx);

  *next_index = tbl_index + 1;
  rc = exm_tbl_allocate_match_and_action_spec_bits(
      exm_tbl, match_spec, act_spec);
  if (rc) return rc;
  if (exm_tbl->hash_action)
    return exm_tbl_hash_action_raw_entry_get(sess_hdl,
                                             dev_tgt,
                                             exm_tbl,
                                             exm_tbl_data,
                                             entry_idx,
                                             exm_stage_info,
                                             err_correction,
                                             match_spec,
                                             act_spec,
                                             act_fn_hdl,
                                             entry_hdl,
                                             is_default);
  *entry_hdl =
      pipe_mgr_exm_get_ent_hdl_from_dir_addr(dir_ent_idx, exm_stage_info);

  *is_default = false;

  rc = exm_tbl_hw_entry_get(sess_hdl,
                            dev_tgt,
                            exm_tbl,
                            exm_tbl_data,
                            exm_stage_info,
                            tbl_index,
                            match_spec,
                            act_spec,
                            act_fn_hdl,
                            &valid,
                            &proxy_hash);
  if (rc) {
    LOG_ERROR(
        "%s:%d Failed to read entry from hw Exm tbl 0x%x, device id %d, pipe "
        "id %d, tbl_index 0x%x",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id,
        tbl_index);
    return rc;
  }

  /* For match entries pointing to selectors, try to restore the grp hdl
   * through HLP state.
   */
  if (*entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL &&
      act_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE) {
    entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, *entry_hdl);
    if (entry_info == NULL) {
      LOG_ERROR(
          "%s:%d Error in getting entry info for entry with handle %d in exact "
          "match table with handle %d, device id %d",
          __func__,
          __LINE__,
          *entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    act_spec->sel_grp_hdl =
        unpack_mat_ent_data_as(entry_info->entry_data)->sel_grp_hdl;
  }
  if (valid && exm_tbl->proxy_hash) {
    /* Validate the hash read from the hardware. */
    bool hash_valid = pipe_mgr_exm_proxy_hash_entry_exists(
        exm_tbl, exm_tbl_data->pipe_id, stage_id, proxy_hash);
    if (!hash_valid && !err_correction) return PIPE_SUCCESS;

    if (hash_valid && *entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL) {
      pipe_tbl_match_spec_t *mspec = NULL;
      bf_map_sts_t map_sts =
          bf_map_get(&exm_tbl_data->proxy_hash_llp_hdl_to_mspec,
                     *entry_hdl,
                     (void **)&mspec);
      if (map_sts == BF_MAP_NO_KEY) {
        LOG_ERROR(
            "%s : Could not find the match spec for entry with handle"
            " %d in exact match table %s with handle 0x%x, device_id %d",
            __func__,
            *entry_hdl,
            exm_tbl->name,
            tbl_hdl,
            dev_tgt.device_id);
        return PIPE_OBJ_NOT_FOUND;
      }
      rc = pipe_mgr_exm_proxy_hash_compute(dev_tgt.device_id,
                                           exm_tbl->profile_id,
                                           exm_tbl->mat_tbl_hdl,
                                           mspec,
                                           stage_id,
                                           &proxy_hash_sw);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in computing proxy hash for tbl 0x%x, device id %d"
            " error %s",
            __func__,
            __LINE__,
            exm_tbl->mat_tbl_hdl,
            dev_tgt.device_id,
            pipe_str_err(rc));
        return rc;
      }
      if (proxy_hash != proxy_hash_sw) {
        LOG_ERROR("%s:%d Error hw proxy hash %016" PRIx64
                  " differs from sw proxy hash %016" PRIx64
                  " tbl 0x%x, device id %d",
                  __func__,
                  __LINE__,
                  proxy_hash,
                  proxy_hash_sw,
                  exm_tbl->mat_tbl_hdl,
                  dev_tgt.device_id);
        return PIPE_INTERNAL_ERROR;
      }
      match_spec = pipe_mgr_tbl_copy_match_spec(match_spec, mspec);
    }
  }
  if (valid && *entry_hdl == PIPE_MAT_ENT_HDL_INVALID_HDL) {
    if (err_correction) {
      rc = pipe_mgr_exm_invalidate_entry(
          sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, entry_idx, false);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Dev %d tbl %s 0x%x pipe %x entry idx %u stage %u"
            "error %s invalidating",
            __func__,
            __LINE__,
            exm_tbl->dev_id,
            exm_tbl->name,
            exm_tbl->mat_tbl_hdl,
            exm_tbl_data->pipe_id,
            entry_idx,
            exm_stage_info->stage_id,
            pipe_str_err(rc));
        PIPE_MGR_DBGCHK(0);
        return rc;
      }
    }
    return PIPE_INTERNAL_ERROR;
  }

  if (!valid && *entry_hdl != PIPE_MAT_ENT_HDL_INVALID_HDL) {
    /* Invalid HW state but valid SW state. */
    if (!err_correction) return PIPE_INTERNAL_ERROR;
    entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, *entry_hdl);
    if (entry_info == NULL) {
      LOG_ERROR(
          "%s:%d Error in getting entry info for entry with handle %d in exact "
          "match table with handle %d, device id %d",
          __func__,
          __LINE__,
          *entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }
    rc = exm_tbl_add_entry(
        sess_hdl, exm_tbl, exm_tbl_data, exm_stage_info, entry_info);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Dev %d tbl %s 0x%x pipe %x entry hdl %u stage %u"
          "error %s adding",
          __func__,
          __LINE__,
          exm_tbl->dev_id,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl_data->pipe_id,
          entry_info->mat_ent_hdl,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      PIPE_MGR_DBGCHK(0);
    }
    return PIPE_INTERNAL_ERROR;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_last_index(dev_target_t dev_tgt,
                                          pipe_mat_tbl_hdl_t tbl_hdl,
                                          uint32_t *last_index) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  unsigned i = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Request for a non-existent table with handle 0x%x device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  exm_tbl_data =
      pipe_mgr_exm_tbl_get_instance_from_any_pipe(exm_tbl, dev_tgt.dev_pipe_id);
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, tbl_data does not exist",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_tgt.device_id,
        dev_tgt.dev_pipe_id);
    return PIPE_INVALID_ARG;
  }
  *last_index = 0;
  for (i = 0; i < exm_tbl_data->num_stages; i++)
    *last_index += exm_tbl_data->exm_stage_info[i].num_entries;
  *last_index -= 1;
  return PIPE_SUCCESS;
}

static pipe_status_t hash_action_invalidate_idx(pipe_sess_hdl_t sess_hdl,
                                                bf_dev_id_t dev_id,
                                                bf_dev_pipe_t pipe_id,
                                                pipe_tbl_hdl_t tbl_hdl,
                                                uint32_t tbl_index) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mat_ent_idx_t entry_idx;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_status_t status;
  unsigned int stage_id = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  if (pipe_id == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Request for a non-existent table with handle 0x%x device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_INVALID_ARG;
  }
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_any_pipe(exm_tbl, pipe_id);
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, tbl_data does not exist",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id);
    return PIPE_INVALID_ARG;
  }
  stage_id = pipe_mgr_exm_get_stage_id_from_idx(exm_tbl_data, tbl_index);
  if (stage_id == 0xff || stage_id >= dev_info->num_active_mau) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, invalid tbl_index 0x%x",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id,
        tbl_index);
    return PIPE_INVALID_ARG;
  }
  exm_stage_info = exm_tbl_data->stage_info_ptrs[stage_id];
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_idx = tbl_index - exm_stage_info->stage_offset;
  /* The default entry installation. */
  status = pipe_mgr_hash_action_add_default_entry(sess_hdl,
                                                  exm_tbl,
                                                  exm_tbl_data,
                                                  exm_stage_info,
                                                  tbl_index,
                                                  true /* is_recovery */);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in setting the default for entry_idx %d in exact "
        "match table with handle %d, stage %d,device id %d",
        __func__,
        __LINE__,
        entry_idx,
        exm_tbl->mat_tbl_hdl,
        exm_stage_info->stage_id,
        dev_id);
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_invalidate_exm_idx(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          bf_dev_pipe_t pipe_id,
                                          pipe_tbl_hdl_t tbl_hdl,
                                          uint32_t tbl_index) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mat_ent_idx_t entry_idx;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_status_t status;
  unsigned int stage_id = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }

  if (pipe_id == BF_DEV_PIPE_ALL) {
    return PIPE_INVALID_ARG;
  }

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Request for a non-existent table with handle 0x%x device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_INVALID_ARG;
  }
  if (exm_tbl->hash_action)
    return hash_action_invalidate_idx(
        sess_hdl, dev_id, pipe_id, tbl_hdl, tbl_index);
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_any_pipe(exm_tbl, pipe_id);
  if (!exm_tbl_data) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, tbl_data does not exist",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id);
    return PIPE_INVALID_ARG;
  }
  stage_id = pipe_mgr_exm_get_stage_id_from_idx(exm_tbl_data, tbl_index);
  if (stage_id == 0xff || stage_id >= dev_info->num_active_mau) {
    LOG_ERROR(
        "%s:%d Exm tbl 0x%x, device id %d, pipe id %d, invalid tbl_index 0x%x",
        __func__,
        __LINE__,
        tbl_hdl,
        dev_id,
        pipe_id,
        tbl_index);
    return PIPE_INVALID_ARG;
  }
  exm_stage_info = exm_tbl_data->stage_info_ptrs[stage_id];
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl stage info not found for tbl 0x%x, pipe id %d, "
        "stage id %d",
        __func__,
        __LINE__,
        exm_tbl->mat_tbl_hdl,
        pipe_id,
        stage_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  entry_idx = tbl_index - exm_stage_info->stage_offset;
  status = pipe_mgr_exm_invalidate_entry(sess_hdl,
                                         exm_tbl,
                                         exm_tbl_data,
                                         exm_stage_info,
                                         entry_idx,
                                         false /* stash */);
  if (status != PIPE_SUCCESS) {
    /* This error is pretty bad */
    LOG_ERROR(
        "%s:%d Error in invalidating entry at idx %d, stage id %d, "
        "pipe id %d, tbl 0x%x, device id %d as part of a stray move "
        "operation",
        __func__,
        __LINE__,
        entry_idx,
        exm_stage_info->stage_id,
        exm_tbl_data->pipe_id,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id);
    PIPE_MGR_DBGCHK(0);
    return status;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_tbl_update(dev_target_t dev_tgt,
                                      pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                      pipe_mat_ent_hdl_t mat_ent_hdl,
                                      uint32_t pipe_api_flags,
                                      pipe_mgr_move_list_t **move_head_p) {
  return pipe_mgr_exm_ent_set_action(dev_tgt.device_id,
                                     mat_tbl_hdl,
                                     mat_ent_hdl,
                                     0,
                                     NULL,
                                     pipe_api_flags,
                                     move_head_p);
}
