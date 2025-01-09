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
 * Exact-match table initialization. Contains per exact-match table
 * initialization code. This is called at the table create time.
 */

/* Standard header includes */
#include <netinet/in.h>
#include <math.h>
#include <netinet/in.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_exm_tbl_init.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tbl.h"
#include "cuckoo_move_init.h"
#include "pipe_mgr_move_list.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_hw_dump.h"

static int pipe_mgr_exm_proxy_hash_cmp(const void *arg, const void *key1) {
  PIPE_MGR_ASSERT(key1 != NULL && arg != NULL);
  uint64_t proxy_hash1 = *(uint64_t *)arg;
  uint64_t proxy_hash2;
  pipe_mgr_exm_proxy_hash_record_node_t *key2 = bf_hashtbl_get_cmp_data(key1);
  if (key1 == NULL || key2 == NULL) {
    return -1;
  }
  proxy_hash2 = key2->proxy_hash;
  if (proxy_hash1 == proxy_hash2) {
    return 0;
  }
  return 1;
}

static void pipe_mgr_exm_tbl_cleanup_ent_hdl_mgr(
    pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr) {
  if (ent_hdl_mgr == NULL) {
    return;
  }

  if (ent_hdl_mgr->ent_hdl_allocator) {
    bf_id_allocator_destroy(ent_hdl_mgr->ent_hdl_allocator);
    ent_hdl_mgr->ent_hdl_allocator = NULL;
  }

  if (ent_hdl_mgr->backup_ent_hdl_allocator) {
    bf_id_allocator_destroy(ent_hdl_mgr->backup_ent_hdl_allocator);
    ent_hdl_mgr->backup_ent_hdl_allocator = NULL;
  }

  return;
}

static void pipe_mgr_exm_tbl_cleanup_adt_tbl_refs(
    pipe_tbl_ref_t *adt_tbl_refs) {
  if (adt_tbl_refs == NULL) {
    return;
  }

  PIPE_MGR_FREE(adt_tbl_refs);

  return;
}

static void pipe_mgr_exm_tbl_cleanup_sel_tbl_refs(
    pipe_tbl_ref_t *sel_tbl_refs) {
  if (sel_tbl_refs == NULL) {
    return;
  }

  PIPE_MGR_FREE(sel_tbl_refs);

  return;
}

static void pipe_mgr_exm_tbl_cleanup_ram_alloc_info(
    pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info) {
  if (ram_alloc_info == NULL) {
    return;
  }

  if (ram_alloc_info->tbl_word_blk) {
    PIPE_MGR_FREE(ram_alloc_info->tbl_word_blk);
  }

  PIPE_MGR_FREE(ram_alloc_info);

  return;
}
uint32_t get_unit_ram_depth(pipe_mgr_exm_tbl_t *exm_tbl) {
  if (!exm_tbl) {
    return ~0;
  }
  rmt_dev_info_t *dev_info = exm_tbl->dev_info;
  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return dev_cfg->stage_cfg.sram_unit_depth;








    default:
      PIPE_MGR_ASSERT(0);
      return ~0;
  }
}
static void pipe_mgr_exm_tbl_cleanup_txn_state(pipe_mgr_exm_tbl_t *exm_tbl) {
  if (exm_tbl == NULL) {
    return;
  }
  unsigned long key = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  pipe_mgr_exm_txn_node_t *txn_node = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  unsigned i = 0;
  pipe_mgr_exm_idx_txn_node_t *txn_node1 = NULL;

  for (i = 0; i < exm_tbl->num_tbls && exm_tbl->exm_tbl_data; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    while ((map_sts = bf_map_get_first_rmv(&exm_tbl_data->dirtied_ent_hdl_htbl,
                                           &key,
                                           (void **)&txn_node)) == BF_MAP_OK) {
      if (txn_node->entry_info) {
        PIPE_MGR_FREE(txn_node->entry_info);
      }
      PIPE_MGR_FREE(txn_node);
    }
    bf_map_destroy(&exm_tbl_data->dirtied_ent_hdl_htbl);

    while ((map_sts = bf_map_get_first_rmv(&exm_tbl_data->dirtied_ent_idx_htbl,
                                           &key,
                                           (void **)&txn_node1)) == BF_MAP_OK) {
      PIPE_MGR_FREE(txn_node1);
    }
    bf_map_destroy(&exm_tbl_data->dirtied_ent_idx_htbl);
  }
  return;
}

static void pipe_mgr_exm_tbl_cleanup_hashway_data(
    pipe_mgr_exm_hash_way_data_t *exm_hash_way_data,
    uint32_t num_entries_per_wide_word) {
  if (exm_hash_way_data == NULL) {
    return;
  }

  if (exm_hash_way_data->ram_alloc_info) {
    pipe_mgr_exm_tbl_cleanup_ram_alloc_info(exm_hash_way_data->ram_alloc_info);
    exm_hash_way_data->ram_alloc_info = NULL;
  }
  if (exm_hash_way_data->num_ram_units) {
    PIPE_MGR_FREE(exm_hash_way_data->num_ram_units);
  }
  if (exm_hash_way_data->ram_unit_present) {
    for (uint32_t i = 0; i < num_entries_per_wide_word; ++i) {
      if (exm_hash_way_data->ram_unit_present[i]) {
        PIPE_MGR_FREE(exm_hash_way_data->ram_unit_present[i]);
      }
    }
    PIPE_MGR_FREE(exm_hash_way_data->ram_unit_present);
  }

  return;
}

static void pipe_mgr_exm_tbl_cleanup_pack_format(
    pipe_mgr_exm_pack_format_t *pack_format) {
  if (pack_format == NULL) {
    return;
  }

  PIPE_MGR_FREE(pack_format);

  return;
}

static void pipe_mgr_exm_tbl_cleanup_stage_data(
    pipe_mgr_exm_stage_info_t *exm_stage_info) {
  uint32_t i = 0, num_entries = 0;

  if (exm_stage_info == NULL) {
    return;
  }

  num_entries = exm_stage_info->pack_format
                    ? exm_stage_info->pack_format->num_entries_per_wide_word
                    : 0;
  for (i = 0; i < exm_stage_info->num_hash_ways; i++) {
    pipe_mgr_exm_tbl_cleanup_hashway_data(&exm_stage_info->hashway_data[i],
                                          num_entries);
  }

  PIPE_MGR_FREE(exm_stage_info->hashway_data);

  /* Free stash memory */
  if (exm_stage_info->stash) {
    for (i = 0; i < exm_stage_info->stash->num_stash_entries; i++) {
      PIPE_MGR_FREE(exm_stage_info->stash->stash_entries[i]);
    }
    PIPE_MGR_FREE(exm_stage_info->stash->stash_entries);

    for (i = 0; i < exm_stage_info->stash->num_rams_per_stash; i++) {
      PIPE_MGR_FREE(exm_stage_info->stash->shadow_ptr_arr[i]);
    }
    PIPE_MGR_FREE(exm_stage_info->stash->shadow_ptr_arr);
    PIPE_MGR_FREE(exm_stage_info->stash->wide_word_indices);

    PIPE_MGR_FREE(exm_stage_info->stash);
    exm_stage_info->stash = NULL;
  }

  if (exm_stage_info->pack_format) {
    pipe_mgr_exm_tbl_cleanup_pack_format(exm_stage_info->pack_format);
    exm_stage_info->pack_format = NULL;
  }

  if (exm_stage_info->cuckoo_move_graph) {
    cuckoo_move_graph_cleanup(exm_stage_info->cuckoo_move_graph);
    exm_stage_info->cuckoo_move_graph = NULL;
  }

  if (exm_stage_info->PJ1Array) {
    for (i = 0; i < exm_stage_info->num_entries; ++i) {
      if (exm_stage_info->PJ1Array[i]) {
        Word_t bytes_freed;
        J1FA(bytes_freed, exm_stage_info->PJ1Array[i]);
        (void)bytes_freed;
      }
    }
    PIPE_MGR_FREE(exm_stage_info->PJ1Array);
    exm_stage_info->PJ1Array = NULL;
  }

  if (exm_stage_info->edge_container) {
    if (exm_stage_info->edge_container->entries) {
      PIPE_MGR_FREE(exm_stage_info->edge_container->entries);
    }
    PIPE_MGR_FREE(exm_stage_info->edge_container);
  }

  if (exm_stage_info->proxy_hash_tbl) {
    bf_hashtbl_delete(exm_stage_info->proxy_hash_tbl);
    PIPE_MGR_FREE(exm_stage_info->proxy_hash_tbl);
  }
  if (exm_stage_info->log_idx_to_ent_hdl_htbl) {
    bf_map_destroy(&exm_stage_info->log_idx_to_ent_hdl_htbl);
  }
  if (exm_stage_info->log_idx_to_occ) {
    bf_map_destroy(&exm_stage_info->log_idx_to_occ);
  }
  if (exm_stage_info->mem_id_arr) {
    PIPE_MGR_FREE(exm_stage_info->mem_id_arr);
  }
  if (exm_stage_info->shadow_ptr_arr) {
    PIPE_MGR_FREE(exm_stage_info->shadow_ptr_arr);
  }
  if (exm_stage_info->wide_word_indices) {
    PIPE_MGR_FREE(exm_stage_info->wide_word_indices);
  }
}

static void pipe_mgr_exm_tbl_cleanup_tbl_data(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data, uint32_t num_instances) {
  uint32_t i = 0;
  uint32_t j = 0;

  for (i = 0; i < num_instances; i++) {
    for (j = 0; j < exm_tbl_data[i].num_stages; j++) {
      pipe_mgr_exm_tbl_cleanup_stage_data(&exm_tbl_data[i].exm_stage_info[j]);
    }
    PIPE_MGR_FREE(exm_tbl_data[i].exm_stage_info);
    exm_tbl_data[i].exm_stage_info = NULL;
    /* Cleanup pipe-level HA state */
    if (exm_tbl_data[i].ha_hlp_info) {
      PIPE_MGR_FREE(exm_tbl_data[i].ha_hlp_info);
      exm_tbl_data[i].ha_hlp_info = NULL;
    }
    if (exm_tbl_data[i].ha_llp_info) {
      bf_id_allocator_destroy(exm_tbl_data[i].ha_llp_info->ent_hdl_allocator);
      PIPE_MGR_FREE(exm_tbl_data[i].ha_llp_info);
      exm_tbl_data[i].ha_llp_info = NULL;
    }
    if (exm_tbl_data[i].ent_hdl_mgr) {
      pipe_mgr_exm_tbl_cleanup_ent_hdl_mgr(exm_tbl_data[i].ent_hdl_mgr);
      PIPE_MGR_FREE(exm_tbl_data[i].ent_hdl_mgr);
      exm_tbl_data[i].ent_hdl_mgr = NULL;
    }
    if (exm_tbl_data[i].stage_info_ptrs) {
      PIPE_MGR_FREE(exm_tbl_data[i].stage_info_ptrs);
      exm_tbl_data->stage_info_ptrs = NULL;
    }
    if (exm_tbl_data[i].hash_action_dflt_act_spec) {
      pipe_mgr_tbl_destroy_action_spec(
          &exm_tbl_data[i].hash_action_dflt_act_spec);
    }

    pipe_tbl_match_spec_t *m_spec = NULL;
    unsigned long unused_key;
    pipe_mgr_exm_phy_entry_info_t *phy_entry_info = NULL;
    while (BF_MAP_OK ==
           bf_map_get_first_rmv(&exm_tbl_data[i].entry_phy_info_htbl,
                                &unused_key,
                                (void **)&phy_entry_info)) {
      if (phy_entry_info->mem_id) {
        PIPE_MGR_FREE(phy_entry_info->mem_id);
      }
      PIPE_MGR_FREE(phy_entry_info);
    }
    bf_map_destroy(&exm_tbl_data[i].entry_phy_info_htbl);
    while (BF_MAP_OK ==
           bf_map_get_first_rmv(&exm_tbl_data[i].proxy_hash_llp_hdl_to_mspec,
                                &unused_key,
                                (void **)&m_spec)) {
      pipe_mgr_tbl_destroy_match_spec(&m_spec);
    }
    bf_map_destroy(&exm_tbl_data[i].proxy_hash_llp_hdl_to_mspec);

    pipe_mgr_exm_entry_info_t *entry_info = NULL;
    while (BF_MAP_OK == bf_map_get_first_rmv(&exm_tbl_data[i].entry_info_htbl,
                                             &unused_key,
                                             (void **)&entry_info)) {
      if (entry_info->entry_data) {
        PIPE_MGR_FREE(entry_info->entry_data);
      }
      PIPE_MGR_FREE(entry_info);
    }
    bf_map_destroy(&exm_tbl_data[i].entry_info_htbl);
    // Cleanup structs allocated for entry move stats
    pipe_mgr_exm_ent_mov_stats_t *entry_stats = &exm_tbl_data[i].entry_stats;
    if (entry_stats->stage_stats) PIPE_MGR_FREE(entry_stats->stage_stats);
    if (entry_stats->failed) PIPE_MGR_FREE(entry_stats->failed);
  }

  PIPE_MGR_FREE(exm_tbl_data);
}

static pipe_status_t pipe_mgr_exm_ram_alloc_info_init(
    pipe_mgr_exm_ram_alloc_info_t *exm_ram_alloc_info,
    rmt_mem_pack_format_t *pack_format,
    rmt_tbl_bank_map_t *bank_map) {
  uint32_t tbl_word_blk_idx = 0;
  uint8_t num_tbl_word_blks = 0;
  uint8_t mem_units_per_tbl_word = 0;
  uint8_t entries_per_tbl_word = 0;

  num_tbl_word_blks = bank_map->num_tbl_word_blks;

  exm_ram_alloc_info->num_wide_word_blks = num_tbl_word_blks;
  mem_units_per_tbl_word = pack_format->mem_units_per_tbl_word;
  entries_per_tbl_word = pack_format->entries_per_tbl_word;

  if (num_tbl_word_blks > 0) {
    exm_ram_alloc_info->tbl_word_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_CALLOC(
        num_tbl_word_blks, sizeof(rmt_tbl_word_blk_t));

    if (exm_ram_alloc_info->tbl_word_blk == NULL) {
      LOG_ERROR("%s : Could not allocate memory for holding wide word blocks",
                __func__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  for (tbl_word_blk_idx = 0; tbl_word_blk_idx < num_tbl_word_blks;
       tbl_word_blk_idx++) {
    /* Populate the mem_ids in the reverse order in which they are populated
     * in the rmt cfg db. This is because, the compiler publishes them in
     * the MSB-LSB order and all the logic in exm tbl mgr interprets in
     * the reverse order.
     */
    unsigned i = 0;
    unsigned j = 0;
    for (i = 0, j = mem_units_per_tbl_word - 1; i < mem_units_per_tbl_word;
         i++, j--) {
      exm_ram_alloc_info->tbl_word_blk[tbl_word_blk_idx].mem_id[i] =
          bank_map->tbl_word_blk[tbl_word_blk_idx].mem_id[j];
    }

    /* Copy over the VPNs, which are as many as the number of entries packed
     * in the wide-word in a similar fashion.
     */
    for (i = 0; i < entries_per_tbl_word; i++) {
      exm_ram_alloc_info->tbl_word_blk[tbl_word_blk_idx].vpn_id[i] =
          bank_map->tbl_word_blk[tbl_word_blk_idx].vpn_id[i];
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_hash_way_init(
    pipe_mgr_exm_hash_way_data_t *hashway_data,
    rmt_mem_pack_format_t *pack_format,
    rmt_tbl_bank_map_t *bank_map,
    pipe_mgr_exm_tbl_t *exm_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  rmt_tbl_hash_sel_bits_t *hash_sel_bits = NULL;
  uint32_t entries_per_tbl_word = 0;
  uint32_t num_wide_word_blks = 0;
  uint32_t num_entries_per_wide_word_blk = 0;

  if (hashway_data == NULL) {
    LOG_ERROR("%s : Passed in hashway_data is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  if (pack_format == NULL) {
    LOG_ERROR("%s : Passed in packing format is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  /* Derive the number of entries in this hash-way from the memory allocated
   * and the packing format.
   */
  entries_per_tbl_word = pack_format->entries_per_tbl_word;
  num_wide_word_blks = bank_map->num_tbl_word_blks;

  num_entries_per_wide_word_blk =
      entries_per_tbl_word * TOF_UNIT_RAM_DEPTH(exm_tbl);

  hashway_data->num_entries =
      num_entries_per_wide_word_blk * num_wide_word_blks;

  hashway_data->ram_alloc_info =
      (pipe_mgr_exm_ram_alloc_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_ram_alloc_info_t));

  if (hashway_data->ram_alloc_info == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for holding ram allocation "
        "info within the hashway ",
        __func__);
    return PIPE_NO_SYS_RESOURCES;
  }

  status = pipe_mgr_exm_ram_alloc_info_init(
      hashway_data->ram_alloc_info, pack_format, bank_map);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing ram allocation information for "
        " a hashway",
        __func__);
    return status;
  }

  hash_sel_bits = &bank_map->hash_bits;

  /* Initialize hash-bits related info */
  hashway_data->num_ram_select_bits = hash_sel_bits->num_ram_select_bits;
  hashway_data->num_ram_line_bits = hash_sel_bits->num_ram_line_bits;
  hashway_data->ram_select_start_offset = hash_sel_bits->ram_unit_select_bit_lo;
  hashway_data->ram_line_start_offset = hash_sel_bits->ram_line_select_bit_lo;
  hashway_data->hash_function_id = hash_sel_bits->function;
  hashway_data->num_ram_units =
      (uint8_t *)PIPE_MGR_CALLOC(entries_per_tbl_word, sizeof(uint8_t));
  if (hashway_data->num_ram_units == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  hashway_data->ram_unit_present =
      (bool **)PIPE_MGR_CALLOC(entries_per_tbl_word, sizeof(bool *));
  if (hashway_data->ram_unit_present == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  hashway_data->num_subword_bits = hash_sel_bits->num_subword_bits;
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_pack_format_init(
    pipe_mgr_exm_pack_format_t *exm_pack_format,
    rmt_mem_pack_format_t *pack_format) {
  if (exm_pack_format == NULL) {
    return PIPE_INVALID_ARG;
  }

  if (pack_format == NULL) {
    return PIPE_INVALID_ARG;
  }

  exm_pack_format->num_entries_per_wide_word =
      pack_format->entries_per_tbl_word;

  exm_pack_format->num_rams_in_wide_word = pack_format->mem_units_per_tbl_word;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_stash_pack_format_init(
    pipe_mgr_exm_stage_info_t *exm_stage_info, rmt_tbl_stash_t *rmt_stash) {
  uint32_t i = 0, j = 0;
  /* No stash info */
  if (rmt_stash == NULL) {
    return PIPE_SUCCESS;
  }

  exm_stage_info->stash = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_exm_stash_t));
  if (exm_stage_info->stash == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for packing format for "
        "exact match table's stash in stage %d",
        __func__,
        exm_stage_info->stage_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Copy pack format */
  pipe_mgr_exm_stash_t *stash = exm_stage_info->stash;
  stash->pack_format.num_entries_per_wide_word =
      rmt_stash->pack_format.entries_per_tbl_word;
  stash->pack_format.num_rams_in_wide_word =
      rmt_stash->pack_format.mem_units_per_tbl_word;

  /* Copy entry locations */
  stash->num_stash_entries = rmt_stash->num_stash_entries;
  stash->num_rams_per_stash = rmt_stash->number_memory_units_per_table_word;

  stash->stash_entries = PIPE_MGR_CALLOC(rmt_stash->num_stash_entries,
                                         sizeof(*stash->stash_entries));

  for (i = 0; i < rmt_stash->num_stash_entries; i++) {
    stash->stash_entries[i] =
        PIPE_MGR_CALLOC(rmt_stash->number_memory_units_per_table_word,
                        sizeof(*stash->stash_entries[i]));
    for (j = 0; j < rmt_stash->number_memory_units_per_table_word; j++) {
      stash->stash_entries[i][j].stash_id =
          rmt_stash->stash_entries[i][j].stash_id;
      stash->stash_entries[i][j].stash_match_data_select =
          rmt_stash->stash_entries[i][j].stash_match_data_select;
      stash->stash_entries[i][j].stash_hashbank_select =
          rmt_stash->stash_entries[i][j].stash_hashbank_select;
      stash->stash_entries[i][j].hash_function_id =
          rmt_stash->stash_entries[i][j].hash_function_id;
    }
  }

  stash->shadow_ptr_arr =
      (uint8_t **)PIPE_MGR_CALLOC(stash->num_rams_per_stash, sizeof(uint8_t *));
  if (stash->shadow_ptr_arr == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    return PIPE_NO_SYS_RESOURCES;
  }
  stash->wide_word_indices =
      (uint8_t *)PIPE_MGR_CALLOC(stash->num_rams_per_stash, sizeof(uint8_t));
  if (stash->wide_word_indices == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_DBGCHK(0);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (i = 0; i < stash->num_rams_per_stash; i++) {
    stash->shadow_ptr_arr[i] =
        PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
    if (stash->shadow_ptr_arr[i] == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      PIPE_MGR_DBGCHK(0);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  return PIPE_SUCCESS;
}

/* Get the first free stash-id in the first memory unit.
   We use only the first stash entry always for atomic moves.
   In future, Need management of stash entries to know which is free
   and which is used
 */
static pipe_status_t pipe_mgr_stash_first_free_id_get(
    pipe_mgr_exm_stage_info_t *exm_stage_info, uint32_t *stash_id) {
  if (!exm_stage_info->stash) {
    return PIPE_INVALID_ARG;
  }
  if (exm_stage_info->stash->num_stash_entries == 0) {
    return PIPE_INVALID_ARG;
  }
  *stash_id = exm_stage_info->stash->stash_entries[0][0].stash_id;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_stage_init(
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    rmt_tbl_info_t *rmt_tbl_info,
    uint32_t stage_offset,
    pipe_mgr_exm_tbl_t *exm_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_hashtbl_sts_t htbl_sts = BF_HASHTBL_OK;
  uint32_t hash_way_idx = 0;
  uint32_t num_hash_ways = 0;
  uint32_t total_num_wide_word_blks = 0;
  uint32_t i = 0;

  if (exm_stage_info == NULL) {
    return PIPE_INVALID_ARG;
  }

  exm_stage_info->stage_id = rmt_tbl_info->stage_id;
  exm_stage_info->direction = rmt_tbl_info->direction;
  exm_stage_info->stage_table_handle = rmt_tbl_info->handle;
  /* The number of entries in the stage is based on the amount of SRAM
   * allocated. The amount of SRAM allocated will be in the unit SRAM depth
   * boundary, which implies that there will be cases where the amount of
   * memory allocated will exceed the amount of entries that have been
   * decided to be placed by the compiler. However, data structures at the
   * stage level are indexed by the entry index and need to be allocated based
   * on the SRAM allocation. However the number of entries, set aside by the
   * compiler still need to be recorded to do checks and not allow more
   * entries than what has been set aside by the compiler.
   */
  exm_stage_info->num_entries = rmt_tbl_info->num_entries;

  for (i = 0; i < rmt_tbl_info->num_tbl_banks; i++) {
    exm_stage_info->capacity +=
        ((rmt_tbl_info->pack_format.entries_per_tbl_word) *
         (rmt_tbl_info->bank_map[i].num_tbl_word_blks) *
         TOF_UNIT_RAM_DEPTH(exm_tbl));
  }
  if (rmt_tbl_info->stash) {
    exm_stage_info->stash_capacity =
        rmt_tbl_info->stash->pack_format.entries_per_tbl_word *
        rmt_tbl_info->stash->num_stash_entries;
  }

  num_hash_ways = rmt_tbl_info->num_tbl_banks;
  exm_stage_info->num_hash_ways = num_hash_ways;
  exm_stage_info->stage_offset = stage_offset;

  /* Always initialize the default miss entry index as the invalid
   * entry index. Only the last stage of the table will have a valid
   * value for the default miss entry index, as only in the last stage
   * of a table's existence the default entry is programmed. The valid
   * value will be initialized by the per-pipe initializer routine
   * above
   */
  exm_stage_info->default_miss_entry_idx = PIPE_MAT_ENT_INVALID_ENTRY_INDEX;
  exm_stage_info->default_stash_id = 0; /* invalid value */

  exm_stage_info->edge_container =
      PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_exm_edge_container_t));

  if (exm_stage_info->edge_container == NULL) {
    LOG_ERROR("%s/%d : Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_stage_info->edge_container->entries =
      PIPE_MGR_CALLOC(num_hash_ways, sizeof(pipe_mat_ent_idx_t));

  if (exm_stage_info->edge_container->entries == NULL) {
    LOG_ERROR("%s/%d : Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_stage_info->edge_container->num_entries = num_hash_ways;

  if (num_hash_ways > 0) {
    exm_stage_info->hashway_data =
        (pipe_mgr_exm_hash_way_data_t *)PIPE_MGR_CALLOC(
            num_hash_ways, sizeof(pipe_mgr_exm_hash_way_data_t));

    if (exm_stage_info->hashway_data == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for holding per-hashway data"
          " for stage id %d",
          __func__,
          exm_stage_info->stage_id);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  uint32_t hashway_accum = 0;
  for (hash_way_idx = 0; hash_way_idx < num_hash_ways; hash_way_idx++) {
    status =
        pipe_mgr_exm_hash_way_init(&exm_stage_info->hashway_data[hash_way_idx],
                                   &rmt_tbl_info->pack_format,
                                   &rmt_tbl_info->bank_map[hash_way_idx],
                                   exm_tbl);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in initializing per-hashway data for hash-way"
          " %d, stage id %d",
          __func__,
          hash_way_idx,
          exm_stage_info->stage_id);
      return status;
    }
    exm_stage_info->hashway_data[hash_way_idx].offset = hashway_accum;
    hashway_accum += exm_stage_info->hashway_data[hash_way_idx].num_entries;
  }

  exm_stage_info->pack_format = (pipe_mgr_exm_pack_format_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_exm_pack_format_t));

  if (exm_stage_info->pack_format == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for packing format for "
        "exact match table in stage %d",
        __func__,
        exm_stage_info->stage_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  status = pipe_mgr_exm_pack_format_init(exm_stage_info->pack_format,
                                         &rmt_tbl_info->pack_format);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing the packing format for "
        "exact match table in stage %d",
        __func__,
        exm_stage_info->stage_id);
    return status;
  }

  status =
      pipe_mgr_exm_stash_pack_format_init(exm_stage_info, rmt_tbl_info->stash);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing the packing format for "
        "exact match table's stash in stage %d",
        __func__,
        exm_stage_info->stage_id);
    return status;
  }

  for (hash_way_idx = 0; hash_way_idx < exm_stage_info->num_hash_ways;
       hash_way_idx++) {
    pipe_mgr_exm_hash_way_data_t *hashway_data =
        &exm_stage_info->hashway_data[hash_way_idx];
    total_num_wide_word_blks +=
        hashway_data->ram_alloc_info->num_wide_word_blks;
  }

  /* Calculate the hash way per function id */
  uint8_t hashway_within_function_id = 0;
  for (hash_way_idx = 0; hash_way_idx < exm_stage_info->num_hash_ways;
       hash_way_idx++) {
    pipe_mgr_exm_hash_way_data_t *hashway_data =
        &exm_stage_info->hashway_data[hash_way_idx];

    if (hash_way_idx == 0) {
      hashway_within_function_id = 0;
    } else {
      if (hashway_data->ram_line_start_offset == 0) {
        /* Update hashway_within_function_id from 5 for hashword1 */
        hashway_within_function_id = 5;
      } else {
        hashway_within_function_id++;
      }
    }
    /* This hashway is used when programming the hash_addr_select in stashes */
    hashway_data->hashway_within_function_id = hashway_within_function_id;
  }

  exm_stage_info->shadow_ptr_arr = (uint8_t **)PIPE_MGR_CALLOC(
      exm_stage_info->pack_format->num_rams_in_wide_word, sizeof(uint8_t *));
  if (exm_stage_info->shadow_ptr_arr == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_ASSERT(0);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_stage_info->mem_id_arr = (mem_id_t *)PIPE_MGR_CALLOC(
      exm_stage_info->pack_format->num_rams_in_wide_word, sizeof(mem_id_t));
  if (exm_stage_info->mem_id_arr == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_ASSERT(0);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_stage_info->wide_word_indices = (uint8_t *)PIPE_MGR_CALLOC(
      exm_stage_info->pack_format->num_rams_in_wide_word, sizeof(uint8_t));
  if (exm_stage_info->wide_word_indices == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    PIPE_MGR_ASSERT(0);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Allocate the array of Judy1 arrays for managing sub-entries */
  exm_stage_info->PJ1Array =
      PIPE_MGR_CALLOC(exm_stage_info->num_entries, sizeof(Pvoid_t));

  if (exm_stage_info->PJ1Array == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for Judy1 arrays for exact "
        "match table for stage id %d",
        __func__,
        exm_stage_info->stage_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_stage_info->proxy_hash_tbl =
      (bf_hashtable_t *)PIPE_MGR_CALLOC(1, sizeof(bf_hashtable_t));

  if (exm_stage_info->proxy_hash_tbl == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  htbl_sts = bf_hashtbl_init(exm_stage_info->proxy_hash_tbl,
                             pipe_mgr_exm_proxy_hash_cmp,
                             pipe_mgr_free_key_htbl_node,
                             sizeof(uint64_t),
                             sizeof(pipe_mgr_exm_proxy_hash_record_node_t),
                             0x98762132);

  if (htbl_sts != BF_HASHTBL_OK) {
    LOG_ERROR("%s:%d Error in initializing hash table for proxy hash",
              __func__,
              __LINE__);
    return PIPE_UNEXPECTED;
  }

  bf_map_sts_t msts;
  msts = bf_map_init(&exm_stage_info->log_idx_to_ent_hdl_htbl);
  if (msts != BF_MAP_OK) return PIPE_NO_SYS_RESOURCES;
  msts = bf_map_init(&exm_stage_info->log_idx_to_occ);
  if (msts != BF_MAP_OK) return PIPE_NO_SYS_RESOURCES;

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_tbl_ent_hdl_mgr_init(
    pipe_mgr_exm_ent_hdl_mgmt_t *exm_ent_hdl_mgr, uint32_t num_entries) {
  bool ZERO_BASED_ALLOCATOR = false;

  if (exm_ent_hdl_mgr == NULL) {
    return PIPE_INVALID_ARG;
  }

  /* Allocate an entry handle allocator */
  exm_ent_hdl_mgr->ent_hdl_allocator =
      bf_id_allocator_new(num_entries, ZERO_BASED_ALLOCATOR);

  if (exm_ent_hdl_mgr->ent_hdl_allocator == NULL) {
    LOG_ERROR("%s : Error in allocating an entry handle allocator", __func__);
    return PIPE_NO_SYS_RESOURCES;
  }
  bf_id_allocator_set(exm_ent_hdl_mgr->ent_hdl_allocator,
                      PIPE_MGR_EXM_DEF_ENTRY_HDL);

  /* Do not allocate the backup entry handle allocator yet.
   * It will allocated and freed on a need-basis
   */

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_tbl_data_init(
    bf_dev_id_t dev_id,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mat_tbl_info_t *mat_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *pipe_bmp,
    bool *idle_present_p,
    bool *hash_action,
    bool *proxy_hash,
    bool *default_entry_reserved,
    bool *stash_available,
    pipe_mgr_exm_tbl_t *exm_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t tbl_idx = 0;
  uint32_t curr_stage_idx = 0;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  uint8_t stage_id = 0;
  uint32_t accum = 0;
  int Rc_int = 0;
  Word_t Rc_word = 0;

  if (exm_tbl_data == NULL) {
    return PIPE_INVALID_ARG;
  }

  if (mat_tbl_info == NULL) {
    return PIPE_INVALID_ARG;
  }

  *hash_action = false;
  *proxy_hash = false;

  exm_tbl_data->pipe_id = pipe_id;

  PIPE_BITMAP_INIT(&exm_tbl_data->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&exm_tbl_data->pipe_bmp, pipe_bmp);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  uint32_t num_stages = dev_info->num_active_mau;
  exm_tbl_data->stage_info_ptrs = (pipe_mgr_exm_stage_info_t **)PIPE_MGR_CALLOC(
      num_stages, sizeof(pipe_mgr_exm_stage_info_t *));

  if (exm_tbl_data->stage_info_ptrs == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (tbl_idx = 0; tbl_idx < mat_tbl_info->num_rmt_info; tbl_idx++) {
    if (mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_HASH_MATCH) {
      PIPE_MGR_ASSERT(*hash_action == false);
      PIPE_MGR_ASSERT(*proxy_hash == false);
      exm_tbl_data->num_stages++;
    }

    if (mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_HASH_ACTION) {
      exm_tbl_data->num_stages++;
      *hash_action = true;
    }
    if (mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_PROXY_HASH) {
      PIPE_MGR_ASSERT(*hash_action == false);
      exm_tbl_data->num_stages++;
      *proxy_hash = true;
    }
  }

  if (exm_tbl_data->num_stages == 0) {
    LOG_ERROR(
        "%s : Attempt to initialize an exact match table with no "
        "presence in any stages",
        __func__);
    return PIPE_INVALID_ARG;
  }

  exm_tbl_data->num_entries = mat_tbl_info->size;

  if (exm_tbl_data->num_stages > 0) {
    exm_tbl_data->exm_stage_info = (pipe_mgr_exm_stage_info_t *)PIPE_MGR_CALLOC(
        exm_tbl_data->num_stages, sizeof(pipe_mgr_exm_stage_info_t));

    if (exm_tbl_data->exm_stage_info == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for holding the exact match"
          " stage data for pipe_id %x",
          __func__,
          pipe_id);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  for (tbl_idx = 0; tbl_idx < mat_tbl_info->num_rmt_info; tbl_idx++) {
    if (mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_HASH_MATCH ||
        mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_HASH_ACTION ||
        mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_PROXY_HASH) {
      status =
          pipe_mgr_exm_stage_init(&exm_tbl_data->exm_stage_info[curr_stage_idx],
                                  &mat_tbl_info->rmt_info[tbl_idx],
                                  accum,
                                  exm_tbl);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in initializing stage level data for "
            "stage id %d, error %s",
            __func__,
            mat_tbl_info->rmt_info[tbl_idx].stage_id,
            pipe_str_err(status));
        /* Responsibility of the caller to free memory already
         * allocated.
         */
        return status;
      }
      stage_id = mat_tbl_info->rmt_info[tbl_idx].stage_id;
      exm_tbl_data->stage_info_ptrs[stage_id] =
          &exm_tbl_data->exm_stage_info[curr_stage_idx];

      accum += mat_tbl_info->rmt_info[tbl_idx].num_entries;
      curr_stage_idx++;
    } else if (mat_tbl_info->rmt_info[tbl_idx].type == RMT_TBL_TYPE_IDLE_TMO) {
      *idle_present_p = true;
    }
  }

  /* Get the stage info for the last stage of the table to use for default entry
   * checks.  The default entry will always be only in the last stage. */
  exm_stage_info = &exm_tbl_data->exm_stage_info[mat_tbl_info->num_stages - 1];

  /* Check if the table requires an index to be reserved for the default entry.
   * If the default entry CAN use direct resources a reservation is required. */
  bool make_reservation = false;
  status = pipe_mgr_tbl_default_entry_needs_reserve(
      dev_info, mat_tbl_info, *idle_present_p, &make_reservation);
  if (PIPE_SUCCESS != status) {
    return status;
  }
  if (make_reservation) {
    uint8_t num_entries_per_wide_word =
        exm_stage_info->pack_format->num_entries_per_wide_word;
    if (!(*hash_action)) {
      pipe_mat_ent_idx_t subentry = 0, edge_idx = 0;
      if (exm_stage_info->num_hash_ways == 1) {
        /* If there is only one hash-way, then it implies that it is
         * a direct indexed table, and reserving a default miss entry
         * index becomes tricky. The default miss entry index is then
         * chosen as the last entry index.
         */
        exm_stage_info->default_miss_entry_idx =
            exm_stage_info->num_entries - 1;
      } else {
        /* Simply use the 0th index as the default miss entry index */
        exm_stage_info->default_miss_entry_idx = 0;
      }
      subentry =
          exm_stage_info->default_miss_entry_idx % num_entries_per_wide_word;
      edge_idx = exm_stage_info->default_miss_entry_idx - subentry;
      J1S(Rc_int, exm_stage_info->PJ1Array[edge_idx], subentry);
      if (Rc_int == JERR) {
        return PIPE_NO_SYS_RESOURCES;
      }
      J1C(Rc_word, exm_stage_info->PJ1Array[edge_idx], 0, -1);
      if (Rc_word == num_entries_per_wide_word) {
        cuckoo_move_graph_t *x =
            pipe_mgr_exm_get_or_create_cuckoo_move_graph(exm_stage_info);
        if (!x) return PIPE_NO_SYS_RESOURCES;
        cuckoo_mark_edge_occupied(x, edge_idx);
      }

      *default_entry_reserved = true;
      if (exm_tbl_data->num_entries < accum) {
        /* Try to expand the exm table by one if we need to
         * reserve default entry
         */
        exm_tbl_data->num_entries++;
      }
    }
  } else {
    /* Even though there is no need to reserve the default entry, set aside
     * an index, since this is used by the default entry add logic to talk
     * to a common state update function which figures out the operation
     * by looking at the cuckoo move list src and dst nodes.
     */
    exm_stage_info->default_miss_entry_idx = PIPE_MGR_EXM_UNUSED_ENTRY_IDX;
  }

  /* Get default stash entry locaton */
  for (curr_stage_idx = 0; curr_stage_idx < exm_tbl_data->num_stages;
       curr_stage_idx++) {
    exm_stage_info = &(exm_tbl_data->exm_stage_info[curr_stage_idx]);
    if (pipe_mgr_exm_reserve_stash_entry(mat_tbl_info) &&
        (exm_stage_info->stash)) {
      if (!(*hash_action)) {
        status = pipe_mgr_stash_first_free_id_get(
            exm_stage_info, &(exm_stage_info->default_stash_id));
        PIPE_MGR_ASSERT(status == PIPE_SUCCESS);
        *stash_available = true;
      }
    }
  }

  /* Allocate the entry handle manager */
  exm_tbl_data->ent_hdl_mgr = (pipe_mgr_exm_ent_hdl_mgmt_t *)PIPE_MGR_CALLOC(
      1, sizeof(pipe_mgr_exm_ent_hdl_mgmt_t));

  if (exm_tbl_data->ent_hdl_mgr == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for exact match entry handle"
        " manager for table with handle %d, pipe %d device id %d",
        __func__,
        mat_tbl_info->handle,
        pipe_id,
        dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  /* The number of entries for which an entry handle is assigned is the number
   * of entries in the tables' declaration plus a default entry for exm tables
   * which do not have any directly referenced resource tables (in each pipe).
   */
  status = pipe_mgr_exm_tbl_ent_hdl_mgr_init(exm_tbl_data->ent_hdl_mgr,
                                             exm_tbl_data->num_entries + 1);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing the exact match table entry handle"
        " manager for table with handle %d, pipe %d device id %d",
        __func__,
        mat_tbl_info->handle,
        pipe_id,
        dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  // Initialize structs required for entry move stats
  pipe_mgr_exm_ent_mov_stats_t *entry_stats = &(exm_tbl_data->entry_stats);
  entry_stats->failed = PIPE_MGR_CALLOC(num_stages, sizeof(uint32_t));
  entry_stats->total_failed = 0;
  entry_stats->stage_stats =
      PIPE_MGR_CALLOC(num_stages, sizeof(pipe_mgr_exm_stage_moves_t));
  if (!entry_stats->failed || !entry_stats->stage_stats) {
    if (entry_stats->stage_stats) PIPE_MGR_FREE(entry_stats->stage_stats);
    if (entry_stats->failed) PIPE_MGR_FREE(entry_stats->failed);
    entry_stats->stage_stats = NULL;
    entry_stats->failed = NULL;
    LOG_ERROR(
        "%s : Error in initializing the exact match table entry move stats"
        "  for table with handle 0x%x, pipe %d device id %d",
        __func__,
        mat_tbl_info->handle,
        pipe_id,
        dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_tbl_init_tbl_refs(
    pipe_mgr_exm_tbl_t *exm_tbl, pipe_mat_tbl_info_t *mat_tbl_info) {
  if (exm_tbl == NULL || mat_tbl_info == NULL) {
    return PIPE_INVALID_ARG;
  }

  exm_tbl->num_adt_refs = mat_tbl_info->num_adt_tbl_refs;
  /* FIXME : Number of selector table refs need to be populated in rmt cfg */

  if (exm_tbl->num_adt_refs > 0) {
    exm_tbl->adt_tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_adt_refs, sizeof(pipe_tbl_ref_t));

    if (exm_tbl->adt_tbl_refs == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for holding action data"
          " table refs for exact match table",
          __func__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMCPY(exm_tbl->adt_tbl_refs,
                    mat_tbl_info->adt_tbl_ref,
                    sizeof(pipe_tbl_ref_t) * exm_tbl->num_adt_refs);
  }

  exm_tbl->num_sel_tbl_refs = mat_tbl_info->num_sel_tbl_refs;

  if (exm_tbl->num_sel_tbl_refs > 0) {
    exm_tbl->sel_tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_sel_tbl_refs, sizeof(pipe_tbl_ref_t));

    if (exm_tbl->sel_tbl_refs == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for holding selection table "
          " refs for exact match table",
          __func__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMCPY(exm_tbl->sel_tbl_refs,
                    mat_tbl_info->sel_tbl_ref,
                    sizeof(pipe_tbl_ref_t) * exm_tbl->num_sel_tbl_refs);
  }

  exm_tbl->num_stat_tbl_refs = mat_tbl_info->num_stat_tbl_refs;

  if (exm_tbl->num_stat_tbl_refs > 0) {
    exm_tbl->stat_tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_stat_tbl_refs, sizeof(pipe_tbl_ref_t));

    if (exm_tbl->stat_tbl_refs == NULL) {
      LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMCPY(exm_tbl->stat_tbl_refs,
                    mat_tbl_info->stat_tbl_ref,
                    sizeof(pipe_tbl_ref_t) * exm_tbl->num_stat_tbl_refs);
  }

  /* Meter table references */
  exm_tbl->num_meter_tbl_refs = mat_tbl_info->num_meter_tbl_refs;

  if (exm_tbl->num_meter_tbl_refs > 0) {
    exm_tbl->meter_tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_meter_tbl_refs, sizeof(pipe_tbl_ref_t));

    if (exm_tbl->meter_tbl_refs == NULL) {
      LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMCPY(exm_tbl->meter_tbl_refs,
                    mat_tbl_info->meter_tbl_ref,
                    sizeof(pipe_tbl_ref_t) * exm_tbl->num_meter_tbl_refs);
  }

  /* Stful table references */
  exm_tbl->num_stful_tbl_refs = mat_tbl_info->num_stful_tbl_refs;

  if (exm_tbl->num_stful_tbl_refs > 0) {
    exm_tbl->stful_tbl_refs = (pipe_tbl_ref_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_stful_tbl_refs, sizeof(pipe_tbl_ref_t));

    if (exm_tbl->stful_tbl_refs == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    PIPE_MGR_MEMCPY(exm_tbl->stful_tbl_refs,
                    mat_tbl_info->stful_tbl_ref,
                    sizeof(pipe_tbl_ref_t) * exm_tbl->num_stful_tbl_refs);
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_stage_shadow_mem_init(
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_tbl_stage_info,
    pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  pipe_mgr_exm_ram_alloc_info_t *ram_alloc_info = NULL;
  pipe_mgr_exm_pack_format_t *exm_pack_format = NULL;

  unsigned i = 0;
  unsigned j = 0;
  unsigned k = 0;
  pipe_mem_type_t mem_type = pipe_mem_type_unit_ram;



  for (i = 0; i < exm_tbl_stage_info->num_hash_ways; i++) {
    exm_hashway_data = &exm_tbl_stage_info->hashway_data[i];
    ram_alloc_info = exm_hashway_data->ram_alloc_info;
    exm_pack_format = exm_tbl_stage_info->pack_format;
    for (j = 0; j < ram_alloc_info->num_wide_word_blks; j++) {
      for (k = 0; k < exm_pack_format->num_rams_in_wide_word; k++) {
        status = pipe_mgr_phy_mem_map_symmetric_mode_set(
            exm_tbl->dev_id,
            exm_tbl->direction,
            pipe_bmp,
            exm_tbl_stage_info->stage_id,
            mem_type,
            ram_alloc_info->tbl_word_blk[j].mem_id[k],
            exm_tbl->symmetric);

        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in setting pipe list for"
              " shadow mem for tbl 0x%x, device id %d"
              " err %s",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
          return status;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_shadow_mem_init(pipe_mgr_exm_tbl_t *exm_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_tbl_stage_info = NULL;
  unsigned i = 0;
  unsigned j = 0;

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];
    for (j = 0; j < exm_tbl_data->num_stages; j++) {
      exm_tbl_stage_info = &exm_tbl_data->exm_stage_info[j];
      status = pipe_mgr_exm_stage_shadow_mem_init(
          exm_tbl, exm_tbl_stage_info, &exm_tbl_data->pipe_bmp);
      if (status != PIPE_SUCCESS) {
        return status;
      }
    }
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_tbl_pipe_ha_init(
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_tbl_info_t *mat_tbl_info) {
  /* TODO : Find a way to figure out that we are LLP only to avoid initing
   * HLP HA info.
   */

  (void)device_id;
  exm_tbl_data->ha_hlp_info =
      (pipe_mgr_exm_pipe_hlp_ha_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_exm_pipe_hlp_ha_info_t));
  if (exm_tbl_data->ha_hlp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_mgr_spec_map_t *spec_map = &exm_tbl_data->ha_hlp_info->spec_map;
  spec_map->dev_tgt.device_id = device_id;
  spec_map->dev_tgt.dev_pipe_id = exm_tbl_data->pipe_id;
  spec_map->mat_tbl_hdl = mat_tbl_hdl;
  spec_map->tbl_info = mat_tbl_info;
  spec_map->entry_place_with_hdl_fn = pipe_mgr_exm_ent_place_with_hdl;
  spec_map->entry_modify_fn = pipe_mgr_exm_ent_set_action;
  spec_map->entry_update_fn = pipe_mgr_exm_entry_update_state;
  spec_map->entry_delete_fn = pipe_mgr_exm_entry_del;

  if (!pipe_mgr_is_device_virtual(device_id)) {
    exm_tbl_data->ha_llp_info =
        (pipe_mgr_exm_pipe_llp_ha_info_t *)PIPE_MGR_CALLOC(
            1, sizeof(pipe_mgr_exm_pipe_llp_ha_info_t));
    if (exm_tbl_data->ha_llp_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    exm_tbl_data->ha_llp_info->ent_hdl_allocator =
        bf_id_allocator_new(exm_tbl_data->num_entries + 1, false);
    /* Reserve the default handle */
    bf_id_allocator_set(exm_tbl_data->ha_llp_info->ent_hdl_allocator,
                        PIPE_MGR_EXM_DEF_ENTRY_HDL);
  }
  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_exm_tbl_alloc(pipe_mgr_exm_tbl_t *exm_tbl,
                                            pipe_mat_tbl_info_t *mat_tbl_info,
                                            pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t tbl_idx = 0;
  bool idle_present = false;
  bool hash_action = false;
  bool proxy_hash = false;
  bool default_entry_reserved = false;
  bool stash_available = false;

  (void)pipe_bmp;
  if (exm_tbl->num_tbls > 0) {
    exm_tbl->exm_tbl_data = (pipe_mgr_exm_tbl_data_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_tbls, sizeof(pipe_mgr_exm_tbl_data_t));

    if (exm_tbl->exm_tbl_data == NULL) {
      LOG_ERROR(
          "%s : Could not allocate memory for exact match table data "
          "for table with device id %d",
          __func__,
          exm_tbl->dev_id);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  bf_dev_pipe_t pipe_id = 0;
  bf_dev_pipe_t lowest_pipe_id = 0;
  /* The num of exm table instances is equal to the number of scopes
    In symmetric case, num of scopes will be one.
    In non-symmetric case, num of scopes will be equal to num of
    active pipes in profile.
    In user-defined scope case, num of scopes is based on user config. User
    could add more than one pipe in one scope. In that case, the lowest pipe-id
    is the representative pipe in that scope and user is expected to pass this
    lowest pipe in all PD api calls.
  */
  for (tbl_idx = 0; tbl_idx < exm_tbl->num_tbls; tbl_idx++) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);

    lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(exm_tbl->scope_pipe_bmp[tbl_idx]);
    if (exm_tbl->symmetric == true) {
      pipe_id = BF_DEV_PIPE_ALL;
    } else {
      pipe_id = lowest_pipe_id;
    }
    pipe_mgr_convert_scope_pipe_bmp(exm_tbl->scope_pipe_bmp[tbl_idx],
                                    &local_pipe_bmp);

    stash_available = false;
    status = pipe_mgr_exm_tbl_data_init(exm_tbl->dev_id,
                                        &exm_tbl->exm_tbl_data[tbl_idx],
                                        mat_tbl_info,
                                        pipe_id,
                                        &local_pipe_bmp,
                                        &idle_present,
                                        &hash_action,
                                        &proxy_hash,
                                        &default_entry_reserved,
                                        &stash_available,
                                        exm_tbl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error initializing table instance for table %s (0x%x) on "
          "device id %d, error %s",
          __func__,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }

    bf_map_init(&exm_tbl->exm_tbl_data[tbl_idx].proxy_hash_llp_hdl_to_mspec);

    /* Init default entry handle */
    exm_tbl->exm_tbl_data[tbl_idx].default_entry_hdl =
        PIPE_SET_HDL_PIPE(PIPE_MGR_EXM_DEF_ENTRY_HDL, lowest_pipe_id);

    /* Init HA related stuff at the pipe-level */
    status = pipe_mgr_exm_tbl_pipe_ha_init(&exm_tbl->exm_tbl_data[tbl_idx],
                                           exm_tbl->dev_id,
                                           exm_tbl->mat_tbl_hdl,
                                           mat_tbl_info);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing pipe level HA info, for tbl %s 0x%x, pipe id "
          "%x, device id %d",
          __func__,
          __LINE__,
          exm_tbl->name,
          exm_tbl->mat_tbl_hdl,
          pipe_id,
          exm_tbl->dev_id);
      return status;
    }
  }
  exm_tbl->idle_present = idle_present;
  exm_tbl->hash_action = hash_action;
  exm_tbl->proxy_hash = proxy_hash;
  exm_tbl->default_entry_reserved = default_entry_reserved;
  exm_tbl->stash_available = stash_available;

  if (!pipe_mgr_is_device_virtual(exm_tbl->dev_id)) {
    /* Do some initialization for shadow memory */
    status = pipe_mgr_exm_shadow_mem_init(exm_tbl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing shadow memory metadata for tbl"
          " 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  }
  /* Store the number of match spec bits and bytes */
  exm_tbl->num_match_spec_bits = mat_tbl_info->num_match_bits;
  exm_tbl->num_match_spec_bytes = mat_tbl_info->num_match_bytes;
  /* Init action function handle info */
  exm_tbl->num_actions = mat_tbl_info->num_actions;
  if (!exm_tbl->act_fn_hdl_info) {
    exm_tbl->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
        exm_tbl->num_actions, sizeof(pipe_act_fn_info_t));
    if (exm_tbl->act_fn_hdl_info == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_MGR_MEMCPY(exm_tbl->act_fn_hdl_info,
                    mat_tbl_info->act_fn_hdl_info,
                    sizeof(pipe_act_fn_info_t) * exm_tbl->num_actions);
    unsigned i = 0;
    uint32_t max_bytes = 0;
    for (i = 0; i < exm_tbl->num_actions; i++) {
      if (exm_tbl->act_fn_hdl_info[i].num_bytes > max_bytes) {
        max_bytes = exm_tbl->act_fn_hdl_info[i].num_bytes;
      }
    }
    exm_tbl->max_act_data_size = max_bytes;
  }
  return status;
}

/** \brief pipe_mgr_exm_tbl_init:
 *         Performs initialization for the given table handle.
 *
 *   This function allocates memory for all the required state associated
 *   with an exact match table and initializes them with values as per the
 *   parameters with which the table is configured to operate.
 *
 *  \param pipe_mat_tbl_hdl Table handle of the table for which the
 *         initialization is being done.
 *  \return pipe_status_t The status of the operation.
 */

pipe_status_t pipe_mgr_exm_tbl_init(bf_dev_id_t dev_id,
                                    pipe_mat_tbl_hdl_t pipe_mat_tbl_hdl,
                                    profile_id_t profile_id,
                                    pipe_bitmap_t *pipe_bmp) {
  LOG_TRACE("Entering %s for dev_id %d, tbl_hdl 0x%x, profile %d ",
            __func__,
            dev_id,
            pipe_mat_tbl_hdl,
            profile_id);

  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  unsigned long htbl_key = 0UL;

  pipe_status_t status = PIPE_SUCCESS;
  bf_map_sts_t map_sts;

  uint32_t num_entries = 0, q = 0;

  /* First, check if the table already exists */
  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, pipe_mat_tbl_hdl);
  if (exm_tbl != NULL) {
    LOG_ERROR(
        "%s : Attempt to initialize an exact match table which "
        "already exists, with table handle 0x%x, device id %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, pipe_mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting mat table info in table with handle 0x%x, "
        "device id %d",
        __func__,
        __LINE__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  num_entries = mat_tbl_info->size;

  exm_tbl =
      (pipe_mgr_exm_tbl_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_exm_tbl_t));

  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for exact match table with"
        " handle 0x%x for device %d",
        __func__,
        pipe_mat_tbl_hdl,
        dev_id);
    return PIPE_NO_SYS_RESOURCES;
  }

  exm_tbl->name = bf_sys_strdup(mat_tbl_info->name);
  exm_tbl->direction = mat_tbl_info->direction;
  exm_tbl->dev_id = dev_id;
  exm_tbl->dev_info = pipe_mgr_get_dev_info(dev_id);
  PIPE_MGR_ASSERT(exm_tbl->dev_info != NULL);
  exm_tbl->mat_tbl_info = mat_tbl_info;
  exm_tbl->symmetric = mat_tbl_info->symmetric;
  exm_tbl->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!exm_tbl->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (exm_tbl->symmetric) {
    exm_tbl->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) { exm_tbl->scope_pipe_bmp[0] |= (1 << q); }
  } else {
    exm_tbl->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      exm_tbl->scope_pipe_bmp[q] |= (1 << q);
      exm_tbl->num_scopes += 1;
    }
  }
  exm_tbl->num_entries = num_entries;
  exm_tbl->match_key_width = mat_tbl_info->match_key_width;
  exm_tbl->mat_tbl_hdl = pipe_mat_tbl_hdl;
  exm_tbl->profile_id = profile_id;


  LOG_TRACE("%s: Table %s, Pipe bitmap count %d ",
            __func__,
            exm_tbl->name,
            PIPE_BITMAP_COUNT(pipe_bmp));
  exm_tbl->lock_type = LOCK_ID_TYPE_INVALID;
  rmt_dev_profile_info_t *profile = pipe_mgr_get_profile(
      exm_tbl->dev_info, exm_tbl->profile_id, __func__, __LINE__);
  PIPE_MGR_ASSERT(profile);

  exm_tbl->num_tbls = exm_tbl->num_scopes;
  if (exm_tbl->symmetric) {
    exm_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(exm_tbl->scope_pipe_bmp[0]);
  }

  status = pipe_mgr_exm_tbl_alloc(exm_tbl, mat_tbl_info, &profile->pipe_bmp);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in tbl alloc for table %s (0x%x), device id %d, error %s",
        __func__,
        exm_tbl->name,
        pipe_mat_tbl_hdl,
        dev_id,
        pipe_str_err(status));
    goto err_cleanup;
  }

  /* Populate the references from this table */
  status = pipe_mgr_exm_tbl_init_tbl_refs(exm_tbl, mat_tbl_info);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error initializing references for exact match table %s (0x%x) on "
        "device id %d",
        __func__,
        exm_tbl->name,
        pipe_mat_tbl_hdl,
        dev_id);
    goto err_cleanup;
  }

  /* Insert the exm table reference into the global hash-table which maps
   * device-id, tbl_hdl to tbl info.
   */
  htbl_key = pipe_mat_tbl_hdl;

  map_sts = pipe_mgr_exm_tbl_map_add(dev_id, htbl_key, (void *)exm_tbl);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "Error creating exact match table %s (0x%x) on device %d, status %d",
        exm_tbl->name,
        pipe_mat_tbl_hdl,
        dev_id,
        map_sts);
    return PIPE_UNEXPECTED;
  }

  /* Update the stats table lock type */
  unsigned i = 0;
  pipe_tbl_ref_t *stat_tbl_ref = NULL;
  pipe_stat_tbl_info_t *stat_tbl_info = NULL;
  for (i = 0; i < exm_tbl->num_stat_tbl_refs; i++) {
    stat_tbl_ref = &exm_tbl->stat_tbl_refs[i];
    if (stat_tbl_ref->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
      stat_tbl_info = pipe_mgr_get_stat_tbl_info(
          exm_tbl->dev_id, stat_tbl_ref->tbl_hdl, __func__, __LINE__);
      PIPE_MGR_ASSERT(stat_tbl_info);

      if (stat_tbl_info->lrt_enabled == true) {
        status = pipe_mgr_exm_update_lock_type(
            exm_tbl->dev_id, exm_tbl->mat_tbl_hdl, false, true, true);

        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in updating stat lock type for"
              " exm tbl %s 0x%x, device id %d, err %s",
              __func__,
              __LINE__,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl,
              exm_tbl->dev_id,
              pipe_str_err(status));
        }
      }
    }
  }

  LOG_TRACE("Exiting %s successfully for dev_id %d, table %s 0x%x",
            __func__,
            dev_id,
            exm_tbl->name,
            pipe_mat_tbl_hdl);

  return PIPE_SUCCESS;

err_cleanup:
  if (exm_tbl) {
    pipe_mgr_exm_tbl_delete(dev_id, pipe_mat_tbl_hdl);
  }
  return status;
}

void pipe_mgr_exm_tbl_delete(bf_dev_id_t device_id,
                             pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  LOG_TRACE("Entering %s for table handle 0x%x, device id %d",
            __func__,
            mat_tbl_hdl,
            device_id);

  bf_map_sts_t map_sts;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  unsigned long key = 0;
  uint32_t num_instances = 0;

  /* Walk over this structure and de-allocate heap memory allocated
   * for all the sub-structures associated with this structure and
   * ultimately free the top-level structure.
   */

  exm_tbl = pipe_mgr_exm_tbl_get(device_id, mat_tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_TRACE(
        "%s : Request to delete a non-existent table with handle 0x%x"
        " device id %d",
        __func__,
        mat_tbl_hdl,
        device_id);
    /* Nothing to free */
    return;
  }

  /* The substructures allocated as part of this structure are
   *   1. Txn state.
   *   2. Array of table data, one per managed instance of the table. This
   *      will be 1 for symmetric tables, number of pipe-lines for asymmetric
   *      tables.
   *   3. An entry handle manager.
   *   4. An array of action data table references.
   *   5. An array of selection table references.
   */

  num_instances = exm_tbl->num_tbls;

  /* Item #1 */
  pipe_mgr_exm_tbl_cleanup_txn_state(exm_tbl);

  /* Clean up resource table references. */
  if (exm_tbl->exm_tbl_data) {
    pipe_mgr_exm_tbl_cleanup_tbl_data(exm_tbl->exm_tbl_data, num_instances);
    exm_tbl->exm_tbl_data = NULL;
  }
  if (exm_tbl->adt_tbl_refs) {
    pipe_mgr_exm_tbl_cleanup_adt_tbl_refs(exm_tbl->adt_tbl_refs);
    exm_tbl->adt_tbl_refs = NULL;
  }
  if (exm_tbl->sel_tbl_refs) {
    pipe_mgr_exm_tbl_cleanup_sel_tbl_refs(exm_tbl->sel_tbl_refs);
    exm_tbl->sel_tbl_refs = NULL;
  }
  if (exm_tbl->stat_tbl_refs) {
    PIPE_MGR_FREE(exm_tbl->stat_tbl_refs);
    exm_tbl->stat_tbl_refs = NULL;
  }
  if (exm_tbl->meter_tbl_refs) {
    PIPE_MGR_FREE(exm_tbl->meter_tbl_refs);
    exm_tbl->meter_tbl_refs = NULL;
  }
  if (exm_tbl->stful_tbl_refs) {
    PIPE_MGR_FREE(exm_tbl->stful_tbl_refs);
    exm_tbl->stful_tbl_refs = NULL;
  }

  /* Now that all of the sub-structures are de-allocated, do the work of
   * freeing the table structure, but, before that clean up the reference to
   * this table in the hash-table mapping <table_handle, device_id> to the
   * table structure.
   */

  key = mat_tbl_hdl;
  map_sts = pipe_mgr_exm_tbl_map_rmv(device_id, key);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s : Error in removing the exact match table from the "
        " entry handle to table structure hash table, table handle 0x%x",
        __func__,
        mat_tbl_hdl);
  }

  exm_tbl->mat_tbl_info = NULL;

  if (exm_tbl->scope_pipe_bmp) {
    PIPE_MGR_FREE(exm_tbl->scope_pipe_bmp);
  }

  PIPE_MGR_FREE(exm_tbl->name);
  if (exm_tbl->act_fn_hdl_info) {
    PIPE_MGR_FREE(exm_tbl->act_fn_hdl_info);
    exm_tbl->act_fn_hdl_info = NULL;
  }
  PIPE_MGR_FREE(exm_tbl);
  LOG_TRACE("Exiting %s for table handle 0x%x, device id %d successfully",
            __func__,
            mat_tbl_hdl,
            device_id);

  return;
}

pipe_status_t pipe_mgr_exm_tbl_set_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  rmt_dev_profile_info_t *profile = NULL;
  uint32_t usage = 0, i = 0;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl info for tbl 0x%x device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    PIPE_MGR_ASSERT(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  profile = pipe_mgr_get_profile(
      exm_tbl->dev_info, exm_tbl->profile_id, __func__, __LINE__);
  if (profile == NULL) {
    LOG_ERROR("%s:%d Profile info for profile id %d, dev id %d not found",
              __func__,
              __LINE__,
              exm_tbl->profile_id,
              dev_id);
    PIPE_MGR_ASSERT(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  LOG_TRACE("%s: Table %s, Change to symmetric mode %d ",
            __func__,
            exm_tbl->name,
            symmetric);

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       exm_tbl->symmetric,
                                       exm_tbl->num_scopes,
                                       &exm_tbl->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              exm_tbl->name,
              exm_tbl->symmetric,
              exm_tbl->num_scopes);
    return rc;
  }

  if (exm_tbl->symmetric) {
    dev_target_t dev_tgt = {.device_id = dev_id,
                            .dev_pipe_id = BF_DEV_PIPE_ALL};
    rc = pipe_mgr_exm_tbl_get_placed_entry_count(dev_tgt, tbl_hdl, &usage);
    if (PIPE_SUCCESS != rc) {
      LOG_ERROR(
          "%s:%d Error in getting exm tbl usage, for tbl %d, device id %d"
          " err %s",
          __func__,
          __LINE__,
          tbl_hdl,
          dev_id,
          pipe_str_err(rc));
      return rc;
    }
    if (usage > 0) {
      LOG_ERROR(
          "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d ",
          __func__,
          exm_tbl->name,
          exm_tbl->symmetric,
          usage);
      return PIPE_NOT_SUPPORTED;
    }

  } else {
    bf_dev_pipe_t pipe_id = 0;
    dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = 0};
    for (i = 0; i < exm_tbl->num_tbls; i++) {
      pipe_id = exm_tbl->exm_tbl_data[i].pipe_id;
      dev_tgt.dev_pipe_id = pipe_id;
      usage = 0;
      rc = pipe_mgr_exm_tbl_get_placed_entry_count(dev_tgt, tbl_hdl, &usage);
      if (PIPE_SUCCESS != rc) {
        LOG_ERROR(
            "%s:%d Error in getting exm tbl usage, for tbl %d, device id %d, "
            "pipe %d"
            " err %s",
            __func__,
            __LINE__,
            tbl_hdl,
            dev_id,
            pipe_id,
            pipe_str_err(rc));
        return rc;
      }
      if (usage > 0) {
        LOG_ERROR(
            "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d "
            "in pipe %d",
            __func__,
            exm_tbl->name,
            exm_tbl->symmetric,
            usage,
            pipe_id);
        return PIPE_NOT_SUPPORTED;
      }
    }
  }

  mat_tbl_info = pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  PIPE_MGR_ASSERT(mat_tbl_info != NULL);

  /* Default entry should not be placed (ignore static default entries) */
  bool static_def_ent = false;
  for (i = 0; i < mat_tbl_info->num_static_entries; i++) {
    if (mat_tbl_info->static_entries[i].default_entry) {
      static_def_ent = true;
    }
  }
  if (!static_def_ent) {
    for (i = 0; i < exm_tbl->num_tbls; i++) {
      if (pipe_mgr_exm_is_default_ent_placed(&exm_tbl->exm_tbl_data[i])) {
        LOG_ERROR(
            "%s: ERROR: Table %s, Cannot change symmetric mode to %d, default "
            "entry exists on pipe %u",
            __func__,
            exm_tbl->name,
            exm_tbl->symmetric,
            exm_tbl->exm_tbl_data[i].pipe_id);
        return PIPE_NOT_SUPPORTED;
      }
    }
  }

  // cleanup first
  pipe_mgr_exm_tbl_cleanup_tbl_data(exm_tbl->exm_tbl_data, exm_tbl->num_tbls);
  exm_tbl->exm_tbl_data = NULL;

  exm_tbl->symmetric = symmetric;
  mat_tbl_info->symmetric = symmetric;
  /* Copy the new scope info */
  exm_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(exm_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  exm_tbl->num_tbls = exm_tbl->num_scopes;
  if (exm_tbl->symmetric) {
    PIPE_MGR_ASSERT(exm_tbl->num_scopes == 1);
    exm_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(exm_tbl->scope_pipe_bmp[0]);
  }
  // allocate with new setting
  rc = pipe_mgr_exm_tbl_alloc(exm_tbl, mat_tbl_info, &profile->pipe_bmp);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s: Table %s, alloc with new settings failed ",
              __func__,
              exm_tbl->name);
    return rc;
  }

  if (!pipe_mgr_is_device_virtual(exm_tbl->dev_id)) {
    /* Re-init the shadow memory stuff, since the symmetricity of the table
     * changed.
     */
    rc = pipe_mgr_exm_shadow_mem_init(exm_tbl);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing shadow memory data for tbl"
          " 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          pipe_str_err(rc));
      return rc;
    }
  }

  return rc;
}

static inline bool is_entry_hdl_default(pipe_mgr_exm_tbl_t *tbl_info,
                                        pipe_mat_ent_hdl_t ent_hdl,
                                        bool is_hlp) {
  int i = 0, I = tbl_info->num_tbls;
  for (; i < I; ++i) {
    if (is_hlp && tbl_info->exm_tbl_data[i].default_entry_hdl == ent_hdl) {
      return true;
    }
    if (!is_hlp && tbl_info->exm_tbl_data[i].default_entry_hdl == ent_hdl) {
      return true;
    }
  }
  return false;
}

pipe_status_t pipe_mgr_exm_get_first_placed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;
  uint32_t pipe_idx;

  *entry_hdl = -1;
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
  if (exm_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric exm tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < exm_tbl->num_tbls; pipe_idx++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        exm_tbl_data->pipe_id != dev_tgt.dev_pipe_id) {
      continue;
    }
    ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;
    *entry_hdl = bf_id_allocator_get_first(ent_hdl_mgr->ent_hdl_allocator);
    if (*entry_hdl != -1) {
      if (exm_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
        *entry_hdl = PIPE_SET_HDL_PIPE(*entry_hdl, exm_tbl_data->pipe_id);
      }
      if (is_entry_hdl_default(exm_tbl, *entry_hdl, true)) {
        int next_hdl = -1;
        pipe_status_t rc = pipe_mgr_exm_get_next_placed_entry_handles(
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

pipe_status_t pipe_mgr_exm_get_next_placed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_ent_hdl_mgmt_t *ent_hdl_mgr = NULL;
  bf_dev_pipe_t pipe_id;
  uint32_t pipe_idx = 0;
  pipe_mat_ent_hdl_t hdl = 0;
  int new_hdl = 0;
  int i = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
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
  if (exm_tbl->symmetric) {
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for symmetric exm tbl with "
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
          "asymmetric exm tbl with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    for (pipe_idx = 0; pipe_idx < exm_tbl->num_tbls; pipe_idx++) {
      if (exm_tbl->exm_tbl_data[pipe_idx].pipe_id == pipe_id) {
        break;
      }
    }
    if (pipe_idx == exm_tbl->num_tbls) {
      LOG_ERROR(
          "%s:%d %s(0x%x-%d) "
          "exm table for pipe %d not found",
          __func__,
          __LINE__,
          exm_tbl->name,
          tbl_hdl,
          dev_tgt.device_id,
          pipe_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
  ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;

  hdl = PIPE_GET_HDL_VAL(entry_hdl);
  i = 0;
  while (i < n) {
    new_hdl = bf_id_allocator_get_next(ent_hdl_mgr->ent_hdl_allocator, hdl);
    if (new_hdl == -1) {
      if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
        // We've reached the end of this pipe, so exit
        break;
      }
      while (pipe_idx < exm_tbl->num_tbls - 1) {
        pipe_idx++;
        exm_tbl_data = &exm_tbl->exm_tbl_data[pipe_idx];
        ent_hdl_mgr = exm_tbl_data->ent_hdl_mgr;
        new_hdl = bf_id_allocator_get_first(ent_hdl_mgr->ent_hdl_allocator);
        if (new_hdl != -1) {
          break;
        }
      }
      if (new_hdl == -1) {
        next_entry_handles[i] = new_hdl;
        break;
      }
    }
    if (exm_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
      new_hdl = PIPE_SET_HDL_PIPE(new_hdl, exm_tbl_data->pipe_id);
    }
    if (is_entry_hdl_default(exm_tbl, new_hdl, true)) {
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

pipe_status_t pipe_mgr_exm_get_first_programmed_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  bool first_entry_found = false;
  bf_map_sts_t map_sts;
  unsigned long key = 0;
  uint32_t i;

  *entry_hdl = -1;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);

  if (!exm_tbl) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
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
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &exm_tbl->exm_tbl_data[i];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != exm_tbl_data->pipe_id)
      continue;

    map_sts = bf_map_get_first(
        &exm_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
    if (map_sts != BF_MAP_OK) {
      *entry_hdl = -1;
      if (dev_tgt.dev_pipe_id == exm_tbl_data->pipe_id) {
        /* No entry in the requested pipe_id */
        break;
      } else {
        /* Keep looking in other table instances */
        continue;
      }
    }

    while (map_sts == BF_MAP_OK) {
      *entry_hdl = key;

      if (!pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, *entry_hdl)) {
        first_entry_found = true;
        break;
      } else {
        map_sts = bf_map_get_next(
            &exm_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
      }
    }

    if (first_entry_found) break;

    *entry_hdl = -1;
  }

  return *entry_hdl == -1 ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_next_programmed_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info = NULL;
  pipe_mgr_exm_tbl_data_t *first_tbl_data;
  pipe_mgr_exm_tbl_data_t *cur_tbl_data;
  unsigned long key = 0;
  bf_map_sts_t map_sts;
  uint32_t first_tbl_idx, idx, t;
  bool done = false;
  int i = 0;

  if (n) next_entry_handles[0] = -1;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (!exm_tbl) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
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
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  /* Start with the entry's table instance. */
  first_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, entry_hdl, __func__, __LINE__);
  if (!first_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Determine the table instance index of this first table. */
  for (t = 0; t < exm_tbl->num_tbls; t++) {
    if (first_tbl_data == &exm_tbl->exm_tbl_data[t]) {
      first_tbl_idx = t;
      break;
    }
  }

  /* Sanity check. */
  if (t >= exm_tbl->num_tbls) {
    LOG_ERROR("%s:%d Unexpected error for Exm tbl 0x%x, entry hdl %d",
              __func__,
              __LINE__,
              exm_tbl->mat_tbl_hdl,
              entry_hdl);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  key = entry_hdl;
  for (idx = first_tbl_idx; idx < exm_tbl->num_tbls && i < n && !done; idx++) {
    cur_tbl_data = &exm_tbl->exm_tbl_data[idx];

    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != cur_tbl_data->pipe_id)
      continue;

    if (cur_tbl_data != first_tbl_data) {
      /* Look in next table instance. */
      map_sts = bf_map_get_first(
          &cur_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
      if (map_sts == BF_MAP_OK) {
        if (!pipe_mgr_exm_is_ent_hdl_default(cur_tbl_data, key)) {
          /* Entry present */
          next_entry_handles[i++] = key;
        }
      } else {
        /* Keep looking in other table instances. */
        continue;
      }
    }

    while (i < n) {
      map_sts = bf_map_get_next(
          &cur_tbl_data->entry_phy_info_htbl, &key, (void **)&entry_info);
      if (map_sts != BF_MAP_OK) {
        next_entry_handles[i] = -1;
        if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
          /* There is no more entry for the requested pipe. */
          done = true;
        }
        break;
      }
      if (pipe_mgr_exm_is_ent_hdl_default(cur_tbl_data, key)) continue;
      next_entry_handles[i] = key;
      i++;
    }
  }

  if (i < n) next_entry_handles[i] = -1;

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_default_entry_handles(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t *default_hdls,
    uint32_t *num_def_hdls) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  uint32_t i;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);

  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl %d, for device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  *num_def_hdls = 0;

  for (i = 0; i < exm_tbl->num_tbls; i++) {
    exm_tbl_data = &(exm_tbl->exm_tbl_data[i]);
    if (exm_tbl_data->default_entry_placed ||
        exm_tbl_data->default_entry_installed) {
      default_hdls[i] = exm_tbl_data->default_entry_hdl;
      (*num_def_hdls)++;
    } else {
      default_hdls[i] = 0;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_default_ent_get(pipe_sess_hdl_t sess_hdl,
                                           dev_target_t dev_tgt,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_action_spec_t *action_spec,
                                           pipe_act_fn_hdl_t *act_fn_hdl,
                                           bool from_hw) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_entry_info_t *entry_info = NULL;

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

  /* There is no default entry in hardware for Hash Action tables; in these
   * tables all entries are valid at all times.  Fall back to a SW read here
   * instead of returning an error for the get request. */
  if (exm_tbl->hash_action) from_hw = false;

  if (from_hw)
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_any_pipe(
        exm_tbl, dev_tgt.dev_pipe_id);
  else
    exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, dev_tgt.dev_pipe_id);
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

  if (pipe_mgr_exm_is_default_ent_placed(exm_tbl_data)) {
    entry_info =
        pipe_mgr_exm_get_entry_info(exm_tbl, exm_tbl_data->default_entry_hdl);
  }

  if (from_hw) {
    exm_stage_info =
        &exm_tbl_data->exm_stage_info[exm_tbl_data->num_stages - 1];
    pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
    rc = pipe_mgr_exm_execute_def_entry_get(sess_hdl,
                                            dev_tgt,
                                            exm_tbl,
                                            exm_tbl_data,
                                            exm_stage_info,
                                            act_fn_hdl,
                                            &indirect_ptrs,
                                            action_spec);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting the match overhead for the default entry "
          "in exact match table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_tgt.device_id);
      return rc;
    }
    if (*act_fn_hdl == 0) {
      /* Default action not set */
      return PIPE_OBJ_NOT_FOUND;
    }
    rc = pipe_mgr_exm_decode_act_spec_for_entry(
        dev_tgt,
        exm_tbl,
        exm_tbl_data,
        exm_stage_info,
        *act_fn_hdl,
        exm_stage_info->default_miss_entry_idx,
        action_spec,
        &indirect_ptrs,
        true,
        NULL /* sess_hdl */);

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding action spec for def entry in tbl 0x%x, "
          "device id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      return rc;
    }

    /* Next, recover the indirect index by decoding the virtual addresses
     * read from hw (in the indirect_ptrs) and populate them in the
     * resource specs. */
    rc = pipe_mgr_exm_build_indirect_resources_from_hw(dev_tgt,
                                                       exm_tbl,
                                                       exm_tbl_data,
                                                       exm_stage_info,
                                                       action_spec,
                                                       &indirect_ptrs);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in building resources for def entry in tbl 0x%x, device "
          "id %d, pipe id %d, stage id %d, err %s",
          __func__,
          __LINE__,
          exm_tbl->mat_tbl_hdl,
          exm_tbl->dev_id,
          exm_tbl_data->pipe_id,
          exm_stage_info->stage_id,
          pipe_str_err(rc));
      return rc;
    }

    if (action_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE &&
        entry_info) {
      action_spec->sel_grp_hdl =
          unpack_mat_ent_data_as(entry_info->entry_data)->sel_grp_hdl;
    }
  } else {
    if (entry_info) {
      pipe_action_spec_t *def_act_spec =
          unpack_mat_ent_data_as(entry_info->entry_data);
      if (def_act_spec) {
        pipe_mgr_tbl_copy_action_spec(action_spec, def_act_spec);
      }
      *act_fn_hdl = unpack_mat_ent_data_afun_hdl(entry_info->entry_data);
    } else {
      return pipe_mgr_tbl_get_init_default_entry(
          dev_tgt.device_id, mat_tbl_hdl, action_spec, act_fn_hdl);
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_entry(pipe_mat_tbl_hdl_t tbl_hdl,
                                     dev_target_t dev_tgt,
                                     pipe_mat_ent_hdl_t entry_hdl,
                                     pipe_tbl_match_spec_t *pipe_match_spec,
                                     pipe_action_spec_t *pipe_action_spec,
                                     pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_tbl_match_spec_t *match_spec;
  pipe_action_spec_t *action_spec;
  pipe_mgr_exm_entry_info_t *entry_info;

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

  entry_info = pipe_mgr_exm_get_entry_info(exm_tbl, entry_hdl);
  if (entry_info == NULL) {
    LOG_ERROR(
        "%s : Could not find the entry info for entry with handle"
        " %d in exact match table with handle %d, device_id %d",
        __func__,
        entry_hdl,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      entry_info->pipe_id != dev_tgt.dev_pipe_id) {
    LOG_TRACE(
        "%s : Entry with handle %d with pipe id %d does not match requested "
        "pipe id  %d in exact match table with handle %d, device_id %d",
        __func__,
        entry_hdl,
        entry_info->pipe_id,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  /* Check if the entry handle passed in a valid one */
  exm_tbl_data = pipe_mgr_exm_tbl_get_instance(exm_tbl, entry_info->pipe_id);
  if (exm_tbl_data == NULL) {
    LOG_ERROR(
        "%s:%d Exm tbl instance for tbl 0x%x, pipe id %d"
        " not found",
        __func__,
        __LINE__,
        tbl_hdl,
        entry_info->pipe_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  if (pipe_mgr_exm_tbl_is_ent_hdl_valid(exm_tbl_data, entry_hdl) == false) {
    LOG_ERROR(
        "%s : Exm Grp info get failed for "
        " table with handle %d, entry handle %d",
        __func__,
        exm_tbl->mat_tbl_hdl,
        entry_hdl);
    return PIPE_INVALID_ARG;
  }

  match_spec = unpack_mat_ent_data_ms(entry_info->entry_data);
  action_spec = unpack_mat_ent_data_as(entry_info->entry_data);

  /* Copy the match spec */
  pipe_match_spec = pipe_mgr_tbl_copy_match_spec(pipe_match_spec, match_spec);
  if (!pipe_match_spec) {
    return PIPE_NO_SYS_RESOURCES;
  }

  LOG_TRACE("%s: dev_id %d, match_bits %d, num_match_bytes %d ",
            __func__,
            dev_tgt.device_id,
            match_spec->num_valid_match_bits,
            match_spec->num_match_bytes);

  /* Copy the action spec */
  pipe_action_spec =
      pipe_mgr_tbl_copy_action_spec(pipe_action_spec, action_spec);
  if (!pipe_action_spec) {
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Copy the action function hdl */
  *act_fn_hdl = unpack_mat_ent_data_afun_hdl(entry_info->entry_data);

  return rc;
}

pipe_status_t pipe_mgr_exm_get_entry_llp_from_hw(
    pipe_mat_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec,
    pipe_action_spec_t *pipe_action_spec,
    pipe_act_fn_hdl_t *act_fn_hdl) {
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;
  pipe_mgr_exm_tbl_data_t *exm_tbl_data = NULL;
  pipe_mgr_exm_phy_entry_info_t *entry_info;
  pipe_mgr_exm_stage_info_t *exm_stage_info = NULL;
  pipe_mgr_exm_hash_way_data_t *exm_hashway_data = NULL;
  uint8_t hashway = 0;
  uint8_t num_rams_in_wide_word = 0;
  uint32_t entry_position = 0;
  uint8_t num_ram_units = 0;
  bf_dev_pipe_t pipe_id = 0, phy_pipe_id = 0;
  bool *ram_unit_present = NULL;
  mem_id_t *mem_id_arr = NULL;
  pipe_mgr_indirect_ptrs_t indirect_ptrs = {0};
  pipe_mgr_exm_hash_info_for_decode_t hash_info;
  unsigned i = 0;
  uint64_t *addrs = NULL;
  uint8_t *phy_addrs_map = NULL;
  uint8_t **wide_word_ptrs = NULL;
  bf_dev_pipe_t default_pipe_id = 0;
  pipe_mem_type_t pipe_mem_type;
  exm_tbl = pipe_mgr_exm_tbl_get(dev_tgt.device_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table  with handle "
        "0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  rmt_dev_info_t *dev_info = exm_tbl->dev_info;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mem_type = pipe_mem_type_unit_ram;
      break;







    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  PIPE_MGR_MEMSET(&hash_info, 0, sizeof(pipe_mgr_exm_hash_info_for_decode_t));

  /* Special handling for hash action tables */
  if (exm_tbl->hash_action) {
    return pipe_mgr_exm_hash_action_decode_entry(tbl_hdl,
                                                 dev_tgt,
                                                 entry_hdl,
                                                 pipe_match_spec,
                                                 pipe_action_spec,
                                                 act_fn_hdl);
  }

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
        "%s:%d Error in getting table data for exact "
        "match table with handle 0x%x, pipe id %d, device id %d",
        __func__,
        __LINE__,
        tbl_hdl,
        entry_info->pipe_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (exm_tbl_data->pipe_id == BF_DEV_PIPE_ALL)
    default_pipe_id = exm_tbl->lowest_pipe_id;
  else
    default_pipe_id = exm_tbl_data->pipe_id;

  rc = pipe_mgr_get_pipe_id(
      &exm_tbl_data->pipe_bmp, dev_tgt.dev_pipe_id, default_pipe_id, &pipe_id);
  if (rc != PIPE_SUCCESS) {
    LOG_TRACE(
        "%s:%d Invalid request to access pipe %x for table %s "
        "0x%x device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        exm_tbl->name,
        tbl_hdl,
        dev_tgt.device_id);
    return rc;
  }
  rc =
      pipe_mgr_map_pipe_id_log_to_phy(exm_tbl->dev_info, pipe_id, &phy_pipe_id);
  if (PIPE_SUCCESS != rc) {
    LOG_ERROR("%s:%d Failed to map logical pipe %d to phy pipe on dev %d (%s)",
              __func__,
              __LINE__,
              pipe_id,
              dev_tgt.device_id,
              pipe_str_err(rc));
    return rc;
  }

  exm_stage_info = pipe_mgr_exm_tbl_get_stage_info(
      exm_tbl, entry_info->pipe_id, entry_info->stage_id);
  if (exm_stage_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting stage info in exact "
        "match table with handle 0x%x, pipe id %d, stage id %d, device id %d",
        __func__,
        __LINE__,
        tbl_hdl,
        entry_info->pipe_id,
        entry_info->stage_id,
        dev_tgt.device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_exm_is_ent_hdl_default(exm_tbl_data, entry_hdl)) {
    pipe_sess_hdl_t sess_hdl = 0;
    rc = pipe_mgr_exm_execute_def_entry_get(sess_hdl,
                                            dev_tgt,
                                            exm_tbl,
                                            exm_tbl_data,
                                            exm_stage_info,
                                            act_fn_hdl,
                                            &indirect_ptrs,
                                            pipe_action_spec);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in getting the match overhead for the default entry "
          "with handle %d in exact "
          "match table with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return rc;
    }
  } else {
    entry_position = entry_info->entry_idx %
                     exm_stage_info->pack_format->num_entries_per_wide_word;
    hashway =
        pipe_mgr_exm_get_entry_hashway(exm_stage_info, entry_info->entry_idx);

    exm_hashway_data = &exm_stage_info->hashway_data[hashway];

    num_rams_in_wide_word = exm_stage_info->pack_format->num_rams_in_wide_word;
    pipe_mgr_exm_compute_entry_details_from_location(exm_tbl,
                                                     exm_stage_info,
                                                     exm_hashway_data,
                                                     entry_info->entry_idx,
                                                     &hash_info,
                                                     NULL,
                                                     NULL);

    /* Calculate the ram units that this entry uses */
    num_ram_units = entry_info->num_ram_units;
    ram_unit_present = PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(bool));
    if (ram_unit_present == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    mem_id_arr = PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(mem_id_t));
    if (mem_id_arr == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    pipe_mgr_exm_get_mem_ids_for_entry(
        exm_tbl, exm_stage_info, entry_info->entry_idx, mem_id_arr, NULL, NULL);
    uint32_t idx = 0;
    for (; idx < num_ram_units; idx++) {
      for (i = 0; i < num_rams_in_wide_word; i++) {
        if (entry_info->mem_id[idx] == mem_id_arr[i]) {
          ram_unit_present[i] = true;
          break;
        }
      }
    }

    addrs = (uint64_t *)PIPE_MGR_CALLOC(num_ram_units, sizeof(uint64_t));
    if (addrs == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    phy_addrs_map = (uint8_t *)PIPE_MGR_CALLOC(num_ram_units, sizeof(uint8_t));
    if (phy_addrs_map == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }

    wide_word_ptrs =
        (uint8_t **)PIPE_MGR_CALLOC(num_rams_in_wide_word, sizeof(uint8_t *));
    if (wide_word_ptrs == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    unsigned map_idx = 0;
    for (i = 0; i < num_rams_in_wide_word; i++) {
      if (ram_unit_present[i]) {
        wide_word_ptrs[i] =
            (uint8_t *)PIPE_MGR_CALLOC(TOF_BYTES_IN_RAM_WORD, sizeof(uint8_t));
        if (wide_word_ptrs[i] == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
        phy_addrs_map[map_idx++] = i;
      }
    }
    uint32_t mem_addr =
        (entry_info->entry_idx /
         exm_stage_info->pack_format->num_entries_per_wide_word) %
        TOF_UNIT_RAM_DEPTH(exm_tbl);
    for (i = 0; i < entry_info->num_ram_units; i++) {
      addrs[i] =
          exm_tbl->dev_info->dev_cfg.get_full_phy_addr(exm_tbl->direction,
                                                       phy_pipe_id,
                                                       entry_info->stage_id,
                                                       entry_info->mem_id[i],
                                                       mem_addr,
                                                       pipe_mem_type);
    }
    bf_subdev_id_t subdev = pipe_mgr_subdev_id_from_pipe(phy_pipe_id);
    rc = pipe_mgr_dump_any_tbl_by_addr(dev_tgt.device_id,
                                       subdev,
                                       exm_tbl->mat_tbl_hdl,
                                       exm_stage_info->stage_table_handle,
                                       entry_info->stage_id,
                                       RMT_TBL_TYPE_HASH_MATCH,
                                       addrs,
                                       entry_info->num_ram_units,
                                       entry_position,
                                       0,
                                       NULL,
                                       NULL,
                                       0,
                                       wide_word_ptrs,
                                       phy_addrs_map,
                                       NULL /*sess_hdl*/);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in dumping the entry %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          entry_hdl,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(rc));
      goto err;
    }

    bool valid = false;
    rc = pipe_mgr_exm_decode_entry(exm_tbl,
                                   exm_tbl_data,
                                   exm_stage_info,
                                   &hash_info,
                                   pipe_match_spec,
                                   pipe_action_spec,
                                   act_fn_hdl,
                                   entry_position,
                                   wide_word_ptrs,
                                   &indirect_ptrs,
                                   entry_info->entry_idx,
                                   &valid,
                                   NULL /* proxy_hash */);

    if (exm_tbl->proxy_hash && rc == PIPE_SUCCESS) {
      rc = pipe_mgr_exm_get_proxy_hash_match_spec_by_entry_handle(
          tbl_hdl, dev_tgt.device_id, entry_hdl, pipe_match_spec);
    }

    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in decoding entry read from hardware to match spec, for "
          "entry hdl %d, tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          entry_hdl,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id,
          pipe_str_err(rc));
      goto err;
    }

    /* Entry has to be valid */
    if (valid != true) {
      LOG_ERROR(
          "%s:%d Error in decoding entry read from hardware to match spec, for "
          "entry hdl %d, tbl 0x%x, device id %d",
          __func__,
          __LINE__,
          entry_hdl,
          exm_tbl->mat_tbl_hdl,
          dev_tgt.device_id);
      PIPE_MGR_DBGCHK(0);
      rc = PIPE_UNEXPECTED;
      goto err;
    }
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
    goto err;
  }

  /* Next, recover the indirect index by decoding the virtual addresses
   * read from hw (in the indirect_ptrs) and populate them in the
   * resource specs. */
  rc = pipe_mgr_exm_build_indirect_resources_from_hw(dev_tgt,
                                                     exm_tbl,
                                                     exm_tbl_data,
                                                     exm_stage_info,
                                                     pipe_action_spec,
                                                     &indirect_ptrs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in build resources from hw for entry %d, tbl 0x%x, device "
        "id %d, pipe id %d, stage id %d, err %s",
        __func__,
        __LINE__,
        entry_hdl,
        exm_tbl->mat_tbl_hdl,
        exm_tbl->dev_id,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        pipe_str_err(rc));
    goto err;
  }

  /* For match entries pointing to selectors, try to restore the grp hdl
   * through HLP state.
   */
  if (pipe_action_spec->pipe_action_datatype_bmap == PIPE_SEL_GRP_HDL_TYPE) {
    pipe_mgr_exm_entry_info_t *hlp_entry_info =
        pipe_mgr_exm_get_entry_info(exm_tbl, entry_hdl);
    if (hlp_entry_info) {
      pipe_action_spec->sel_grp_hdl =
          unpack_mat_ent_data_as(hlp_entry_info->entry_data)->sel_grp_hdl;
    }
  }

err:
  if (phy_addrs_map) {
    PIPE_MGR_FREE(phy_addrs_map);
  }
  if (addrs) {
    PIPE_MGR_FREE(addrs);
  }
  if (mem_id_arr) {
    PIPE_MGR_FREE(mem_id_arr);
  }
  if (ram_unit_present) {
    PIPE_MGR_FREE(ram_unit_present);
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

/* Add default entry for hash-action tbl - Helper API */
static pipe_status_t pipe_mgr_hash_action_add_default_entry_helper(
    pipe_sess_hdl_t sess_hdl,
    pipe_mat_tbl_info_t *mat_tbl_info,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_idx_t logical_idx,
    pipe_mgr_move_list_t *node,
    struct pipe_mgr_mat_data *node_data,
    bool is_recovery) {
  pipe_status_t ret = PIPE_SUCCESS;
  pipe_mat_ent_idx_t dst_stage_ent_idx = 0;

  if (!exm_tbl->hash_action) {
    return PIPE_INVALID_ARG;
  }
  if (!mat_tbl_info) {
    return PIPE_INVALID_ARG;
  }
  if (!mat_tbl_info->hash_action_info) {
    return PIPE_INVALID_ARG;
  }
  if (!exm_tbl_data->hash_action_dflt_act_spec) {
    LOG_ERROR("Missing dflt entry action spec.  Dev %d pipe %x Tbl %s 0x%x",
              exm_tbl->dev_id,
              exm_tbl_data->pipe_id,
              exm_tbl->name,
              exm_tbl->mat_tbl_hdl);
    PIPE_MGR_DBGCHK(exm_tbl_data->hash_action_dflt_act_spec);
    return PIPE_UNEXPECTED;
  }
  pipe_mgr_action_entry_t *action_entry =
      &mat_tbl_info->default_info->action_entry;
  dst_stage_ent_idx = logical_idx - exm_stage_info->stage_offset;

  /* Initialize pointers */
  node->data = node_data;
  /* Use a shallow copy of the default entry's action spec. */
  node_data->action_spec = *exm_tbl_data->hash_action_dflt_act_spec;

  /* Populate the node */
  node->u.single.logical_idx = logical_idx;
  node->next = NULL;
  node->op = PIPE_MAT_UPDATE_ADD;
  node->pipe = exm_tbl_data->pipe_id;
  node->entry_hdl = PIPE_MAT_ENT_HDL_INVALID_HDL;
  node->logical_sel_idx = 0;
  node->logical_action_idx = 0;
  node->selector_len = 0;
  node_data->act_fn_hdl = action_entry->act_fn_hdl;

  /* Add the entry */
  ret = pipe_mgr_exm_execute_entry_add(sess_hdl,
                                       exm_tbl,
                                       exm_tbl_data,
                                       exm_stage_info,
                                       dst_stage_ent_idx,
                                       node,
                                       false,
                                       false,
                                       is_recovery);
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s Failed to add default hash-action entry at logical-idx 0x%x "
        "(ent_idx 0x%x) pipe %d, stage %d for tbl 0x%x",
        __func__,
        logical_idx,
        dst_stage_ent_idx,
        exm_tbl_data->pipe_id,
        exm_stage_info->stage_id,
        exm_tbl->mat_tbl_hdl);
  }

  return ret;
}

/* Add default entry for hash-action tbl */
pipe_status_t pipe_mgr_hash_action_add_default_entry(
    pipe_sess_hdl_t sess_hdl,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_tbl_data_t *exm_tbl_data,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    pipe_idx_t logical_idx,
    bool is_recovery) {
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

  /* Call the helper add api */
  ret = pipe_mgr_hash_action_add_default_entry_helper(sess_hdl,
                                                      mat_tbl_info,
                                                      exm_tbl,
                                                      exm_tbl_data,
                                                      exm_stage_info,
                                                      logical_idx,
                                                      node,
                                                      node_data,
                                                      is_recovery);

  PIPE_MGR_FREE(node);
  PIPE_MGR_FREE(node_data);

  return ret;
}

pipe_status_t pipe_mgr_exm_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mgr_exm_tbl_t *exm_tbl = NULL;

  exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR("%s:%d Exm tbl info for tbl 0x%x device id %d not found",
              __func__,
              __LINE__,
              tbl_hdl,
              dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  *symmetric = exm_tbl->symmetric;
  *num_scopes = exm_tbl->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  exm_tbl->scope_pipe_bmp,
                  exm_tbl->num_scopes * sizeof(scope_pipes_t));

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_exm_get_proxy_hash_match_spec_by_entry_handle(
    pipe_mat_tbl_hdl_t tbl_hdl,
    bf_dev_id_t dev_id,
    pipe_mat_ent_hdl_t entry_hdl,
    pipe_tbl_match_spec_t *pipe_match_spec) {
  pipe_mgr_exm_tbl_data_t *exm_tbl_data;
  pipe_tbl_match_spec_t *mspec = NULL;
  bf_map_sts_t map_sts;

  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, tbl_hdl);
  if (exm_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the exact match table info for table  with handle "
        "0x%x, device id %d",
        __func__,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  exm_tbl_data = pipe_mgr_exm_tbl_get_instance_from_entry(
      exm_tbl, entry_hdl, __func__, __LINE__);
  if (!exm_tbl_data) {
    return PIPE_OBJ_NOT_FOUND;
  }

  map_sts = bf_map_get(
      &exm_tbl_data->proxy_hash_llp_hdl_to_mspec, entry_hdl, (void **)&mspec);

  if (map_sts == BF_MAP_NO_KEY) {
    LOG_ERROR(
        "%s : Could not find the match spec for entry with handle"
        " %d in exact match table %s with handle 0x%x, device_id %d",
        __func__,
        entry_hdl,
        exm_tbl->name,
        tbl_hdl,
        dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in getting match spec for entry with handle %d in exact "
        "match table %s with handle 0x%x, device id %d",
        __func__,
        __LINE__,
        entry_hdl,
        exm_tbl->name,
        tbl_hdl,
        dev_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  pipe_match_spec = pipe_mgr_tbl_copy_match_spec(pipe_match_spec, mspec);

  return PIPE_SUCCESS;
}
