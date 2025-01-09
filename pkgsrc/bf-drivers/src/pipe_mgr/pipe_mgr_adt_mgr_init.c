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
 * @file pipe_adt_mgr_init.c
 * @date
 *
 * Contains action data table manager related initialization. Initializes
 * all global data structures plus per-table structures.
 *
 */

/* Standard header includes */
#include <sys/types.h>

/* Module header includes */
#include <pipe_mgr/pipe_mgr_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <target-utils/id/id.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_adt_init.h"
#include "pipe_mgr_adt_transaction.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_adt_mgr_ha_int.h"
#include "pipe_mgr_tbl.h"

static pipe_status_t pipe_mgr_adt_tbl_get_num_entries(bf_dev_id_t dev,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p);

pipe_status_t pipe_mgr_adt_tbl_init(bf_dev_id_t device_id,
                                    pipe_adt_tbl_hdl_t adt_tbl_hdl,
                                    profile_id_t profile_id,
                                    pipe_bitmap_t *pipe_bmp) {
  LOG_TRACE("Entering %s for device id %d, tbl_hdl %d",
            __func__,
            device_id,
            adt_tbl_hdl);
  pipe_status_t status = PIPE_SUCCESS;
  pipe_adt_tbl_info_t *adt_tbl_info = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;

  unsigned long key = 0;
  pipe_mgr_adt_t *adt = NULL;
  uint32_t num_tbl_instances = 0;
  uint32_t num_entries = 0, q = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt != NULL) {
    LOG_ERROR(
        "%s:%d Attempt to initialize ADT table 0x%x on device %d which already "
        "exists",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_ALREADY_EXISTS;
  }

  adt_tbl_info =
      pipe_mgr_get_adt_tbl_info(device_id, adt_tbl_hdl, __func__, __LINE__);

  if (adt_tbl_info == NULL) {
    LOG_ERROR(
        "%s : Could not get the action data table info from the "
        " rmt cfg database for table with handle %d, device id "
        " %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_INIT_ERROR;
  }

  /* Allocate the top-level structure for action data table */
  adt = (pipe_mgr_adt_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_adt_t));

  if (adt == NULL) {
    LOG_ERROR(
        "%s : Could not allocate the action data table structure for"
        " table with handle %d and device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    return PIPE_INIT_ERROR;
  }

  num_entries = adt_tbl_info->size;
  adt->name = bf_sys_strdup(adt_tbl_info->name);
  adt->direction = adt_tbl_info->direction;
  adt->adt_tbl_hdl = adt_tbl_hdl;
  adt->dev_id = device_id;
  adt->dev_info = pipe_mgr_get_dev_info(device_id);
  PIPE_MGR_ASSERT(adt->dev_info != NULL);
  adt->symmetric = adt_tbl_info->symmetric;
  adt->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!adt->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (adt->symmetric) {
    adt->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) { adt->scope_pipe_bmp[0] |= (1 << q); }
  } else {
    adt->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      adt->scope_pipe_bmp[q] |= (1 << q);
      adt->num_scopes += 1;
    }
  }
  adt->num_entries = num_entries;
  adt->profile_id = profile_id;
  PIPE_BITMAP_INIT(&adt->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&adt->pipe_bmp, pipe_bmp);
  if (adt->symmetric) {
    PIPE_MGR_ASSERT(adt->num_scopes == 1);
    adt->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(adt->scope_pipe_bmp[0]);
  }
  adt->ref_type = adt_tbl_info->ref_type;

  LOG_TRACE("%s: Table %s, Pipe bitmap count %d ",
            __func__,
            adt->name,
            PIPE_BITMAP_COUNT(&adt->pipe_bmp));

  if (adt->symmetric) {
    num_tbl_instances = 1;
  } else {
    num_tbl_instances = adt->num_scopes;
  }
  adt->num_tbls = num_tbl_instances;

  /* Init action function handle info */
  adt->num_actions = adt_tbl_info->num_actions;
  adt->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
      adt->num_actions, sizeof(pipe_act_fn_info_t));
  if (adt->act_fn_hdl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_MEMCPY(adt->act_fn_hdl_info,
                  adt_tbl_info->act_fn_hdl_info,
                  sizeof(pipe_act_fn_info_t) * adt->num_actions);
  unsigned i = 0;
  uint32_t max_bytes = 0;
  for (i = 0; i < adt->num_actions; i++) {
    if (adt->act_fn_hdl_info[i].num_bytes > max_bytes) {
      max_bytes = adt->act_fn_hdl_info[i].num_bytes;
    }
  }
  adt->max_act_data_size = max_bytes;

  status = pipe_mgr_adt_init_tbl_data(adt, num_tbl_instances, adt_tbl_info);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initialzing action data table data for "
        " table with handle %d, device id %d",
        __func__,
        adt_tbl_hdl,
        device_id);
    goto err_cleanup;
  }

  key = adt_tbl_hdl;

  /* Insert the reference to the action data table state in the hash table */
  map_sts = pipe_mgr_adt_tbl_map_add(device_id, key, (void *)adt);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in inserting the action data table structure into"
        " the hash table with table handle %d, device id %d",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_UNEXPECTED;
  }

  if (!pipe_mgr_is_device_virtual(device_id)) {
    /* Do some shadow memory metadata initialization */
    status = pipe_mgr_adt_shadow_mem_init(adt);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing shadow memory metadata for "
          "tbl 0x%x, device id %d, err %s",
          __func__,
          __LINE__,
          adt->adt_tbl_hdl,
          adt->dev_id,
          pipe_str_err(status));
      return status;
    }
  }

  LOG_TRACE("Exiting %s successfully for device id %d, tbl_hdl %d",
            __func__,
            device_id,
            adt_tbl_hdl);
  return PIPE_SUCCESS;

err_cleanup:
  pipe_mgr_adt_cleanup_tbl(adt);

  return status;
}

static pipe_status_t pipe_mgr_adt_tbl_pipe_ha_init(
    pipe_mgr_adt_data_t *adt_tbl_data) {
  adt_tbl_data->ha_hlp_info =
      (pipe_mgr_adt_pipe_ha_hlp_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_adt_pipe_ha_hlp_info_t));
  if (adt_tbl_data->ha_hlp_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  return PIPE_SUCCESS;
}
static void pipe_mgr_adt_tbl_pipe_ha_destroy(
    pipe_mgr_adt_data_t *adt_tbl_data) {
  if (adt_tbl_data->ha_hlp_info) {
    PIPE_MGR_FREE(adt_tbl_data->ha_hlp_info);
  }
}

pipe_status_t pipe_mgr_adt_init_tbl_data(pipe_mgr_adt_t *adt,
                                         uint32_t num_tbl_instances,
                                         pipe_adt_tbl_info_t *adt_tbl_info) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t tbl_idx = 0;
  uint32_t num_stages = 0;

  adt->adt_tbl_data = (pipe_mgr_adt_data_t *)PIPE_MGR_CALLOC(
      num_tbl_instances, sizeof(pipe_mgr_adt_data_t));

  if (adt->adt_tbl_data == NULL) {
    LOG_ERROR("%s : Could not allocate memory for action data table data",
              __func__);
    return PIPE_INIT_ERROR;
  }
  bool virtual_device = pipe_mgr_is_device_virtual(adt->dev_id);

  num_stages = adt_tbl_info->num_stages;

  for (tbl_idx = 0; tbl_idx < num_tbl_instances; tbl_idx++) {
    adt->adt_tbl_data[tbl_idx].adt = adt;
    adt->adt_tbl_data[tbl_idx].num_stages = num_stages;
    adt->adt_tbl_data[tbl_idx].num_entries = adt->num_entries;
    PIPE_BITMAP_INIT(&((adt->adt_tbl_data[tbl_idx]).pipe_bmp), PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(adt->scope_pipe_bmp[tbl_idx],
                                    &((adt->adt_tbl_data[tbl_idx]).pipe_bmp));
    if (adt->symmetric == true) {
      /* If the table is symmetric, then the pipe_id is ALL pipes */
      adt->adt_tbl_data[tbl_idx].pipe_id = BF_DEV_PIPE_ALL;
    } else {
      adt->adt_tbl_data[tbl_idx].pipe_id =
          pipe_mgr_get_lowest_pipe_in_scope(adt->scope_pipe_bmp[tbl_idx]);
    }
    adt->adt_tbl_data[tbl_idx].num_entries_occupied = 0;
    adt->adt_tbl_data[tbl_idx].ent_hdl_allocator =
        bf_id_allocator_new(adt->num_entries, false);
    bf_map_init(&adt->adt_tbl_data[tbl_idx].mbr_id_map);
    bf_map_init(&adt->adt_tbl_data[tbl_idx].adt_ent_refs);
    bf_map_init(&adt->adt_tbl_data[tbl_idx].entry_info_htbl);
    bf_map_init(&adt->adt_tbl_data[tbl_idx].entry_phy_info_htbl);
    bf_map_init(&adt->adt_tbl_data[tbl_idx].const_init_entries);

    status = pipe_mgr_adt_init_stage_data(&adt->adt_tbl_data[tbl_idx],
                                          adt_tbl_info->num_rmt_info,
                                          adt_tbl_info->rmt_info,
                                          adt->ref_type,
                                          virtual_device);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in initializing the stage data for action "
          "data table",
          __func__);
      return PIPE_INIT_ERROR;
    }

    adt->adt_tbl_data[tbl_idx].txn_state =
        (pipe_mgr_adt_txn_state_t *)PIPE_MGR_CALLOC(
            1, sizeof(pipe_mgr_adt_txn_state_t));
    if (!adt->adt_tbl_data[tbl_idx].txn_state) {
      LOG_ERROR(
          "%s : Could not allocate memory for action data table txn "
          "state for table with handle %d, device id %d",
          __func__,
          adt->adt_tbl_hdl,
          adt->dev_id);
      return PIPE_INIT_ERROR;
    }

    if (!virtual_device) {
      if (adt->max_act_data_size) {
        /* Do it only for action data tables with actual action data */
        status =
            pipe_mgr_adt_pipe_ha_llp_info_init(&adt->adt_tbl_data[tbl_idx]);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in initializing the pipe HA data at LLP for tbl "
              "0x%x, "
              "device id %d, err %s",
              __func__,
              __LINE__,
              adt->adt_tbl_hdl,
              adt->dev_id,
              pipe_str_err(status));
          return status;
        }
      }
    }

    if (!pipe_mgr_is_device_virtual_dev_slave(adt->dev_id)) {
      status = pipe_mgr_adt_tbl_pipe_ha_init(&adt->adt_tbl_data[tbl_idx]);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initing adt pipe HA HLP data for tbl 0x%x, device "
            "id "
            "%d, err %s",
            __func__,
            __LINE__,
            adt->adt_tbl_hdl,
            adt->dev_id,
            pipe_str_err(status));
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_init_stage_data(pipe_mgr_adt_data_t *adt_tbl_data,
                                           uint32_t num_rmt_info,
                                           rmt_tbl_info_t *rmt_info,
                                           pipe_tbl_ref_type_t ref_type,
                                           bool virtual_device) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;
  uint32_t stage_idx = 0;
  uint32_t num_entries = 0;
  uint32_t offset = 0;

  adt_tbl_data->adt_stage_info = (pipe_mgr_adt_stage_info_t *)PIPE_MGR_CALLOC(
      num_rmt_info, sizeof(pipe_mgr_adt_stage_info_t));

  if (adt_tbl_data->adt_stage_info == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for per-stage action data"
        " table info",
        __func__);
    return PIPE_INIT_ERROR;
  }

  adt_stage_info = adt_tbl_data->adt_stage_info;

  unsigned i = 0;
  uint8_t curr_stage_id = 0;
  uint8_t next_stage_id = 0xff;
  bool new_stage = true;
  uint8_t sub_tbl_idx = 0;
  uint32_t accum = 0;

  /* For algorithmic TCAM purposes a single P4 action data table may be
   * internally broken down into many logical tables at the stage level.
   * This is represented in the context JSON as multiple "stage tables"
   * with the same stage id, but with different RAM allocation and such.
   * This is absorbed into the stage level data structure here. The
   * different logical tables within the same stage appears as one table
   * internally.
   */

  for (i = 0; i < num_rmt_info; i++) {
    curr_stage_id = rmt_info[i].stage_id;
    /* The number of entries per stage is not what gets driven by the
     * size specified by the P4 program, but actually the number of entries
     * that the allocated memory for this stage can hold (i,e.. capacity).
     * This is especially important when action data memories are direct
     * addressed by the match tables, since the generated address from the
     * hit entry can lie anywhere in the address-space. The compiler would
     * have/ allocated enough memory to fit the entire address-space.
     */
    num_entries = rmt_info[i].bank_map[0].num_tbl_word_blks *
                  rmt_info[i].pack_format.entries_per_tbl_word *
                  TOF_SRAM_UNIT_DEPTH;

    if (new_stage) {
      adt_stage_info[stage_idx].stage_id = curr_stage_id;
      adt_stage_info[stage_idx].num_entries = num_entries;
      adt_stage_info[stage_idx].stage_offset = accum;
      accum += num_entries;
      if (!virtual_device) {
        status =
            pipe_mgr_adt_stage_ha_llp_info_init(&adt_stage_info[stage_idx]);
        if (status != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Error in initializing stage ha llp info for stagei d %d, "
              "err %s",
              __func__,
              __LINE__,
              curr_stage_id,
              pipe_str_err(status));
          return status;
        }
      }
    } else {
      PIPE_MGR_ASSERT(adt_stage_info[stage_idx].stage_id == curr_stage_id);
      adt_stage_info[stage_idx].num_entries += num_entries;
    }

    adt_stage_info[stage_idx].num_sub_tbls++;

    if ((i + 1) == num_rmt_info) {
      next_stage_id = 0xff;
    } else {
      next_stage_id = rmt_info[i + 1].stage_id;
    }

    PIPE_MGR_ASSERT((num_entries % 128) == 0);

    /* Entry allocator */

    /* An entry allocator which is used as a placement engine is required
     * only for indirectly referenced action data tables. Since for directly
     * referenced action data tables the match table manager dictates the
     * placement of the action data entry.
     */

    if (new_stage == true) {
      if (ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
        /* For indirect referenced action data tables, the entry allocator
         * has a special requirement of allocating chunks of aligned entry
         * indices, for selector table purposes. Because of this requirement
         * the entry allocator is a special allocator, which gets used for
         * the entire action data table, regardless of whether the action
         * table is associated with a selector table or not.
         */
        adt_stage_info[stage_idx].entry_allocator =
            (void *)power2_allocator_create(
                128, adt_stage_info[stage_idx].num_entries / 128);
      }

      /* Ram allocation information */
      status = pipe_mgr_adt_init_ram_alloc_info(&adt_stage_info[stage_idx],
                                                &rmt_info[i]);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s : Error in initializing ram allocation information "
            "for action data table",
            __func__);
        return PIPE_INIT_ERROR;
      }
      adt_stage_info[stage_idx].sub_tbl_offsets =
          PIPE_MGR_CALLOC(1, sizeof(struct sub_tbl_offset_t));
      if (adt_stage_info[stage_idx].sub_tbl_offsets == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_INIT_ERROR;
      }
      adt_stage_info[stage_idx].sub_tbl_offsets[0].offset = 0;
      adt_stage_info[stage_idx].sub_tbl_offsets[0].stage_table_handle =
          rmt_info[i].handle;
      offset = num_entries;
      adt_stage_info[stage_idx].shadow_ptr_arr = (uint8_t **)PIPE_MGR_CALLOC(
          adt_stage_info[stage_idx].ram_alloc_info->num_rams_in_wide_word,
          sizeof(uint8_t *));
      if (adt_stage_info[stage_idx].shadow_ptr_arr == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      adt_stage_info[stage_idx].mem_id_arr = (mem_id_t *)PIPE_MGR_CALLOC(
          adt_stage_info[stage_idx].ram_alloc_info->num_rams_in_wide_word,
          sizeof(mem_id_t));
      if (adt_stage_info[stage_idx].mem_id_arr == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
    } else {
      status = pipe_mgr_adt_add_ram_alloc_info(&adt_stage_info[stage_idx],
                                               &rmt_info[i]);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initializing ram allocation information "
            " for action data table",
            __func__,
            __LINE__);
        return PIPE_INIT_ERROR;
      }
      sub_tbl_idx = adt_stage_info[stage_idx].num_sub_tbls - 1;
      adt_stage_info[stage_idx].sub_tbl_offsets =
          PIPE_MGR_REALLOC(adt_stage_info[stage_idx].sub_tbl_offsets,
                           adt_stage_info[stage_idx].num_sub_tbls *
                               sizeof(struct sub_tbl_offset_t));
      adt_stage_info[stage_idx].sub_tbl_offsets[sub_tbl_idx].offset = offset;
      adt_stage_info[stage_idx]
          .sub_tbl_offsets[sub_tbl_idx]
          .stage_table_handle = rmt_info[i].handle;

      offset += num_entries;
    }

    if (next_stage_id > curr_stage_id && next_stage_id != 0xff) {
      new_stage = true;
      stage_idx++;
    } else {
      new_stage = false;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_init_ram_alloc_info(
    pipe_mgr_adt_stage_info_t *adt_stage_info, rmt_tbl_info_t *rmt_tbl_info) {
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_mem_pack_format_t *pack_format = NULL;
  rmt_tbl_bank_map_t *bank_map = NULL;

  uint32_t blk_idx = 0;
  uint32_t num_wide_word_blks = 0;
  uint8_t mem_units_per_tbl_word = 0;

  adt_stage_info->ram_alloc_info =
      (pipe_mgr_adt_ram_alloc_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_adt_ram_alloc_info_t));

  if (adt_stage_info->ram_alloc_info == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for ram allocation info for"
        " action data table",
        __func__);
    return PIPE_INIT_ERROR;
  }

  ram_alloc_info = adt_stage_info->ram_alloc_info;
  /* Action data tables have only one bank */
  bank_map = &rmt_tbl_info->bank_map[0];

  pack_format = &(rmt_tbl_info->pack_format);

  mem_units_per_tbl_word = pack_format->mem_units_per_tbl_word;

  ram_alloc_info->num_wide_word_blks = bank_map->num_tbl_word_blks;
  ram_alloc_info->num_entries_per_wide_word = pack_format->entries_per_tbl_word;
  /* FIXME: Assuming a uniform number of rams per wide-word throughout
   * the stage. Probably a fair assumption. Need to check with the compiler
   * folks.
   */
  ram_alloc_info->num_rams_in_wide_word = mem_units_per_tbl_word;

  num_wide_word_blks = ram_alloc_info->num_wide_word_blks;

  ram_alloc_info->tbl_word_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_CALLOC(
      num_wide_word_blks, sizeof(rmt_tbl_word_blk_t));

  if (ram_alloc_info->tbl_word_blk == NULL) {
    LOG_ERROR(
        "%s : Could not allocate memory for tbl word block inside "
        " ram allocation information for action data table",
        __func__);
    return PIPE_INIT_ERROR;
  }

  for (blk_idx = 0; blk_idx < num_wide_word_blks; blk_idx++) {
    /* Populate the mem_ids in the reverse order in which they are populated
     * in the rmt cfg db. This is because, the compiler publishes them in
     * the MSB-LSB order and all the logic in action data tbl mgr interprets in
     * the reverse order.
     */
    unsigned i = 0;
    unsigned j = 0;
    for (i = 0, j = mem_units_per_tbl_word - 1; i < mem_units_per_tbl_word;
         i++, j--) {
      ram_alloc_info->tbl_word_blk[blk_idx].mem_id[i] =
          bank_map->tbl_word_blk[blk_idx].mem_id[j];
    }

    /* Copy over the VPNs, which are as many as the number of wide
     * word blocks in a similar fashion.
     */

    ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0] =
        bank_map->tbl_word_blk[blk_idx].vpn_id[0];
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_add_ram_alloc_info(
    pipe_mgr_adt_stage_info_t *adt_stage_info, rmt_tbl_info_t *rmt_tbl_info) {
  pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_mem_pack_format_t *pack_format = NULL;
  rmt_tbl_bank_map_t *bank_map = NULL;

  uint32_t blk_idx = 0;
  uint32_t local_blk_idx = 0;
  uint32_t num_wide_word_blks = 0;
  uint8_t mem_units_per_tbl_word = 0;

  ram_alloc_info = adt_stage_info->ram_alloc_info;
  /* Action data tables have only one bank */
  bank_map = &rmt_tbl_info->bank_map[0];

  pack_format = &(rmt_tbl_info->pack_format);

  mem_units_per_tbl_word = pack_format->mem_units_per_tbl_word;
  PIPE_MGR_ASSERT(mem_units_per_tbl_word ==
                  ram_alloc_info->num_rams_in_wide_word);

  num_wide_word_blks = ram_alloc_info->num_wide_word_blks;
  ram_alloc_info->num_wide_word_blks += bank_map->num_tbl_word_blks;
  /* FIXME: Assuming a uniform number of rams per wide-word throughout
   * the stage. Probably a fair assumption. Need to check with the compiler
   * folks.
   */
  ram_alloc_info->tbl_word_blk = PIPE_MGR_REALLOC(
      ram_alloc_info->tbl_word_blk,
      ram_alloc_info->num_wide_word_blks * sizeof(rmt_tbl_word_blk_t));

  if (ram_alloc_info->tbl_word_blk == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  rmt_tbl_word_blk_t *ptr =
      ram_alloc_info->tbl_word_blk +
      (ram_alloc_info->num_wide_word_blks - bank_map->num_tbl_word_blks);

  PIPE_MGR_MEMSET(
      ptr, 0, sizeof(rmt_tbl_word_blk_t) * bank_map->num_tbl_word_blks);

  for (blk_idx = num_wide_word_blks;
       blk_idx < ram_alloc_info->num_wide_word_blks;
       blk_idx++) {
    local_blk_idx = blk_idx - num_wide_word_blks;
    /* Populate the mem_ids in the reverse order in which they are populated
     * in the rmt cfg db. This is because, the compiler publishes them in
     * the MSB-LSB order and all the logic in action data tbl mgr interprets in
     * the reverse order.
     */
    unsigned i = 0;
    unsigned j = 0;
    for (i = 0, j = mem_units_per_tbl_word - 1; i < mem_units_per_tbl_word;
         i++, j--) {
      ram_alloc_info->tbl_word_blk[blk_idx].mem_id[i] =
          bank_map->tbl_word_blk[local_blk_idx].mem_id[j];
    }

    /* Copy over the VPNs, which are as many as the number of wide
     * word blocks in a similar fashion.
     */

    ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0] =
        bank_map->tbl_word_blk[local_blk_idx].vpn_id[0];

    local_blk_idx++;
  }

  return PIPE_SUCCESS;
}

void pipe_mgr_adt_cleanup_ram_alloc_info(
    pipe_mgr_adt_ram_alloc_info_t *ram_alloc_info) {
  PIPE_MGR_FREE(ram_alloc_info->tbl_word_blk);
  PIPE_MGR_FREE(ram_alloc_info);
}

void pipe_mgr_adt_cleanup_stage_data(pipe_mgr_adt_stage_info_t *adt_stage_info,
                                     pipe_tbl_ref_type_t ref_type) {
  power2_allocator_t *power2_allocator = NULL;

  if (adt_stage_info->entry_allocator) {
    if (ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      power2_allocator = (power2_allocator_t *)adt_stage_info->entry_allocator;

      power2_allocator_destroy(power2_allocator);
      adt_stage_info->entry_allocator = NULL;
    }
  }

  if (adt_stage_info->backup_entry_allocator) {
    if (ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
      power2_allocator =
          (power2_allocator_t *)adt_stage_info->backup_entry_allocator;

      power2_allocator_destroy(power2_allocator);
      adt_stage_info->backup_entry_allocator = NULL;
    }
  }

  pipe_mgr_adt_cleanup_ram_alloc_info(adt_stage_info->ram_alloc_info);

  if (adt_stage_info->sub_tbl_offsets) {
    PIPE_MGR_FREE(adt_stage_info->sub_tbl_offsets);
  }
  if (adt_stage_info->shadow_ptr_arr) {
    PIPE_MGR_FREE(adt_stage_info->shadow_ptr_arr);
  }
  if (adt_stage_info->mem_id_arr) {
    PIPE_MGR_FREE(adt_stage_info->mem_id_arr);
  }
  pipe_mgr_adt_stage_ha_llp_info_destroy(adt_stage_info);
  return;
}

void pipe_mgr_adt_cleanup_tbl_data(pipe_mgr_adt_data_t *adt_tbl_data,
                                   pipe_tbl_ref_type_t ref_type) {
  unsigned i = 0;

  for (i = 0; i < adt_tbl_data->num_stages; i++) {
    pipe_mgr_adt_cleanup_stage_data(&adt_tbl_data->adt_stage_info[i], ref_type);
  }
  if (adt_tbl_data->ent_hdl_allocator) {
    bf_id_allocator_destroy(adt_tbl_data->ent_hdl_allocator);
  }
  PIPE_MGR_FREE(adt_tbl_data->adt_stage_info);
  adt_tbl_data->adt_stage_info = NULL;
  pipe_mgr_adt_pipe_ha_llp_info_destroy(adt_tbl_data);
  pipe_mgr_adt_tbl_pipe_ha_destroy(adt_tbl_data);

  pipe_mgr_adt_entry_info_t *entry;
  unsigned long key;
  while (BF_MAP_OK == bf_map_get_first_rmv(&adt_tbl_data->entry_info_htbl,
                                           &key,
                                           (void **)&entry)) {
    if (entry->entry_data) {
      free_adt_ent_data(entry->entry_data);
      entry->entry_data = NULL;
    }
    pipe_mgr_adt_stage_location_t *location = entry->sharable_stage_location;
    while (location) {
      pipe_mgr_adt_stage_location_t *next = location->next;
      PIPE_MGR_FREE(location);
      location = next;
    }
    entry->sharable_stage_location = NULL;
    PIPE_MGR_FREE(entry);
  }
  bf_map_destroy(&adt_tbl_data->entry_info_htbl);

  pipe_mgr_adt_entry_phy_info_t *phy_entry;
  while (BF_MAP_OK == bf_map_get_first_rmv(&adt_tbl_data->entry_phy_info_htbl,
                                           &key,
                                           (void **)&phy_entry)) {
    if (phy_entry->entry_data) {
      free_adt_ent_data(phy_entry->entry_data);
      phy_entry->entry_data = NULL;
    }

    pipe_mgr_adt_stage_location_t *location =
        phy_entry->sharable_stage_location;
    while (location) {
      pipe_mgr_adt_stage_location_t *next = location->next;
      PIPE_MGR_FREE(location);
      location = next;
    }
    phy_entry->sharable_stage_location = NULL;

    location = phy_entry->non_sharable_stage_location;
    while (location) {
      pipe_mgr_adt_stage_location_t *next = location->next;
      PIPE_MGR_FREE(location);
      location = next;
    }
    phy_entry->non_sharable_stage_location = NULL;
    PIPE_MGR_FREE(phy_entry);
  }
  bf_map_destroy(&adt_tbl_data->entry_phy_info_htbl);

  /* Member index map just shadows entry_info_tbl map per scope,
   * can be safetly purged. */
  bf_map_destroy(&adt_tbl_data->mbr_id_map);
  bf_map_destroy(&adt_tbl_data->adt_ent_refs);
  bf_map_destroy(&adt_tbl_data->const_init_entries);
}

void pipe_mgr_adt_cleanup_txn_state(pipe_mgr_adt_data_t *adt_tbl_data) {
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key;
  pipe_mgr_adt_entry_info_t *entry;

  if (adt_tbl_data->txn_state) {
    map_sts = bf_map_get_first_rmv(
        &adt_tbl_data->txn_state->dirtied_ent_hdl_htbl, &key, (void **)&entry);
    while (map_sts == BF_MAP_OK) {
      pipe_mgr_adt_entry_cleanup(entry);
      map_sts =
          bf_map_get_first_rmv(&adt_tbl_data->txn_state->dirtied_ent_hdl_htbl,
                               &key,
                               (void **)&entry);
    }

    PIPE_MGR_FREE(adt_tbl_data->txn_state);
    adt_tbl_data->txn_state = NULL;
  }

  return;
}

pipe_status_t pipe_mgr_adt_cleanup_all_tbl_data(pipe_mgr_adt_t *adt) {
  for (unsigned i = 0; i < adt->num_tbls; i++) {
    pipe_mgr_adt_data_t *adt_tbl_data = &adt->adt_tbl_data[i];
    pipe_mgr_adt_cleanup_tbl_data(adt_tbl_data, adt->ref_type);
    pipe_mgr_adt_cleanup_ent_refs(adt_tbl_data);
    pipe_mgr_adt_cleanup_txn_state(adt_tbl_data);
  }
  PIPE_MGR_FREE(adt->adt_tbl_data);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_cleanup_tbl(pipe_mgr_adt_t *adt) {
  pipe_mgr_adt_cleanup_all_tbl_data(adt);

  if (adt->name) {
    PIPE_MGR_FREE(adt->name);
    adt->name = NULL;
  }

  if (adt->act_fn_hdl_info) {
    PIPE_MGR_FREE(adt->act_fn_hdl_info);
    adt->act_fn_hdl_info = NULL;
  }

  if (adt->scope_pipe_bmp) {
    PIPE_MGR_FREE(adt->scope_pipe_bmp);
    adt->scope_pipe_bmp = NULL;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mgr_adt_t *tbl = pipe_mgr_adt_get(dev_id, tbl_hdl);
  if (!tbl) {
    PIPE_MGR_DBGCHK(tbl);
    return PIPE_UNEXPECTED;
  }

  *symmetric = tbl->symmetric;
  *num_scopes = tbl->num_scopes;
  PIPE_MGR_MEMCPY(scope_pipe_bmp,
                  tbl->scope_pipe_bmp,
                  tbl->num_scopes * sizeof tbl->scope_pipe_bmp[0]);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_tbl_set_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_adt_tbl_info_t *adt_tbl_info = NULL;
  uint32_t usage = 0;

  adt_tbl = pipe_mgr_adt_get(dev_id, tbl_hdl);
  PIPE_MGR_ASSERT(adt_tbl != NULL);

  LOG_TRACE("%s: Table %s, Change to symmetric mode %d ",
            __func__,
            adt_tbl->name,
            symmetric);

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(dev_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       adt_tbl->symmetric,
                                       adt_tbl->num_scopes,
                                       &adt_tbl->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              adt_tbl->name,
              adt_tbl->symmetric,
              adt_tbl->num_scopes);
    return status;
  }

  pipe_mgr_adt_tbl_get_num_entries(dev_id, tbl_hdl, &usage);
  if (usage > 0) {
    LOG_TRACE(
        "%s: ERROR: Table %s, Cannot change symmetric mode to %d, usage %d ",
        __func__,
        adt_tbl->name,
        adt_tbl->symmetric,
        usage);
    return PIPE_NOT_SUPPORTED;
  }

  adt_tbl_info = pipe_mgr_get_adt_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
  if (adt_tbl_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  // cleanup first
  /* All the stuff which has state needs to get cleaned up and recreated. */
  status = pipe_mgr_adt_cleanup_all_tbl_data(adt_tbl);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Table %s, Cleanup failed ", __func__, adt_tbl->name);
    return status;
  }

  adt_tbl->symmetric = symmetric;
  /* Copy the new scope info */
  adt_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(adt_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));
  adt_tbl->num_tbls = adt_tbl->num_scopes;
  if (adt_tbl->symmetric) {
    PIPE_MGR_DBGCHK(adt_tbl->num_scopes == 1);
    adt_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(adt_tbl->scope_pipe_bmp[0]);
  }
  // allocate with new setting
  status = pipe_mgr_adt_init_tbl_data(adt_tbl, adt_tbl->num_tbls, adt_tbl_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR("%s: Table %s, alloc with new settings failed ",
              __func__,
              adt_tbl->name);
    return status;
  }

  if (!pipe_mgr_is_device_virtual(dev_id)) {
    /* Re-init the shadow memory since the symmetricity of the table has
     * changed.
     */
    status = pipe_mgr_adt_shadow_mem_init(adt_tbl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing shadow memory for tbl 0x%x"
          " device id %d, err %s",
          __func__,
          __LINE__,
          adt_tbl->adt_tbl_hdl,
          adt_tbl->dev_id,
          pipe_str_err(status));
      return status;
    }
  }

  return status;
}

static inline bool adt_hlp_is_mbr_hdl_const(pipe_mgr_adt_data_t *inst,
                                            pipe_adt_ent_hdl_t hdl) {
  unsigned long key = hdl;
  pipe_mgr_adt_entry_info_t *entry_info = NULL;
  bf_map_sts_t map_sts =
      bf_map_get(&inst->entry_info_htbl, key, (void **)&entry_info);
  if (map_sts != BF_MAP_OK) return false;
  if (!entry_info) return false;
  if (!entry_info->entry_data) return false;
  return !!entry_info->entry_data->is_const;
}

pipe_status_t pipe_mgr_adt_get_first_placed_entry_handle(
    pipe_adt_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  uint32_t pipe_idx;

  *entry_hdl = -1;
  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the adt table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (pipe_idx = 0; pipe_idx < adt_tbl->num_tbls; pipe_idx++) {
    adt_tbl_data = &adt_tbl->adt_tbl_data[pipe_idx];
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        adt_tbl_data->pipe_id != dev_tgt.dev_pipe_id) {
      continue;
    }
    int id = bf_id_allocator_get_first(adt_tbl_data->ent_hdl_allocator);

    for (; id != -1;
         id = bf_id_allocator_get_next(adt_tbl_data->ent_hdl_allocator, id)) {
      pipe_adt_ent_hdl_t hdl;
      if (adt_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
        hdl = PIPE_SET_HDL_PIPE(id, adt_tbl_data->pipe_id);
      } else {
        hdl = id;
      }
      if (!adt_hlp_is_mbr_hdl_const(adt_tbl_data, hdl)) {
        *entry_hdl = hdl;
        return PIPE_SUCCESS;
      }
    }
  }

  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_adt_get_next_placed_entry_handles(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  bf_dev_pipe_t pipe_id;
  uint32_t pipe_idx = 0;
  int i = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the adt table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt_tbl->symmetric) {
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
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
          "asymmetric adt tbl with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    for (pipe_idx = 0; pipe_idx < adt_tbl->num_tbls; pipe_idx++) {
      if (adt_tbl->adt_tbl_data[pipe_idx].pipe_id == pipe_id) {
        break;
      }
    }
    if (pipe_idx == adt_tbl->num_tbls) {
      LOG_ERROR(
          "%s : Could not get the adt pipe table info for table "
          " with handle %d, pipe %d device id %d",
          __func__,
          tbl_hdl,
          pipe_id,
          dev_tgt.device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  adt_tbl_data = &adt_tbl->adt_tbl_data[pipe_idx];

  int id = PIPE_GET_HDL_VAL(entry_hdl);
  i = 0;
  while (i < n) {
    id = bf_id_allocator_get_next(adt_tbl_data->ent_hdl_allocator, id);
    if (id == -1) {
      /* No more handles in this instance.  If this is pipe-all (all instances)
       * move on to the next instance (which has handles) and get the first
       * handle there to continue with.  If this is for a single instance then
       * the walk is over. */
      if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
        break;
      }
      for (++pipe_idx; pipe_idx < adt_tbl->num_tbls; ++pipe_idx) {
        adt_tbl_data = &adt_tbl->adt_tbl_data[pipe_idx];
        id = bf_id_allocator_get_first(adt_tbl_data->ent_hdl_allocator);
        if (id != -1) {
          break;
        }
      }
      if (id == -1) {
        break;
      }
    }
    pipe_adt_ent_hdl_t hdl;
    if (adt_tbl_data->pipe_id != BF_DEV_PIPE_ALL) {
      hdl = PIPE_SET_HDL_PIPE(id, adt_tbl_data->pipe_id);
    } else {
      hdl = id;
    }

    /* If the entry is const (i.e. a default entry) skip over it. */
    if (adt_hlp_is_mbr_hdl_const(adt_tbl_data, hdl)) continue;

    next_entry_handles[i++] = hdl;
  }
  if (i < n) {
    next_entry_handles[i] = -1;
  }

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_get_first_programmed_entry_handle(
    pipe_adt_tbl_hdl_t tbl_hdl, dev_target_t dev_tgt, int *entry_hdl) {
  pipe_mgr_adt_data_t *adt_tbl_data;
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;
  unsigned long key = 0;

  *entry_hdl = -1;
  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the adt table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }
  if (adt_tbl->symmetric && dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
    LOG_ERROR(
        "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
        "handle 0x%x, device id %d",
        __func__,
        __LINE__,
        dev_tgt.dev_pipe_id,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  for (uint32_t i = 0; i < adt_tbl->num_tbls; i++) {
    adt_tbl_data = &adt_tbl->adt_tbl_data[i];
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != adt_tbl_data->pipe_id) {
      continue;
    }
    bf_map_t *map = &adt_tbl_data->entry_phy_info_htbl;

    for (bf_map_sts_t map_sts =
             bf_map_get_first(map, &key, (void **)&entry_info);
         map_sts == BF_MAP_OK;
         map_sts = bf_map_get_next(map, &key, (void **)&entry_info)) {
      /* Skip over const (init time default) entries. */
      if (entry_info->entry_data->is_const) continue;
      /* Even if the entry is not programmed in hardware return it, otherwise
       * table walks at the LLP will not return all entries known. */
      /* if (!entry_info->sharable_stage_location &&
            !entry_info->non_sharable_stage_location)
          continue; */

      *entry_hdl = key;
      return PIPE_SUCCESS;
    }
  }

  *entry_hdl = -1;
  return PIPE_OBJ_NOT_FOUND;
}

pipe_status_t pipe_mgr_adt_get_next_programmed_entry_handles(
    pipe_adt_tbl_hdl_t tbl_hdl,
    dev_target_t dev_tgt,
    pipe_adt_ent_hdl_t entry_hdl,
    int n,
    int *next_entry_handles) {
  pipe_mgr_adt_t *adt_tbl = NULL;
  pipe_mgr_adt_data_t *adt_tbl_data;
  uint32_t pipe_idx = 0;

  if (n) {
    next_entry_handles[0] = -1;
  }
  adt_tbl = pipe_mgr_adt_get(dev_tgt.device_id, tbl_hdl);
  if (adt_tbl == NULL) {
    LOG_ERROR(
        "%s : Could not get the adt table info for table "
        " with handle %d, device id %d",
        __func__,
        tbl_hdl,
        dev_tgt.device_id);
    return PIPE_INVALID_ARG;
  }

  if (adt_tbl->symmetric) {
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d passed for symmetric adt tbl with "
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
    bf_dev_pipe_t pipe_id = PIPE_GET_HDL_PIPE(entry_hdl);
    if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
        dev_tgt.dev_pipe_id != pipe_id) {
      LOG_ERROR(
          "%s:%d Invalid pipe id %d for entry hdl %d passed for "
          "asymmetric adt tbl with handle 0x%x, device id %d",
          __func__,
          __LINE__,
          dev_tgt.dev_pipe_id,
          entry_hdl,
          tbl_hdl,
          dev_tgt.device_id);
      return PIPE_INVALID_ARG;
    }
    for (pipe_idx = 0; pipe_idx < adt_tbl->num_tbls; pipe_idx++) {
      if (adt_tbl->adt_tbl_data[pipe_idx].pipe_id == pipe_id) {
        break;
      }
    }
    if (pipe_idx == adt_tbl->num_tbls) {
      LOG_ERROR(
          "%s : Could not get the adt pipe table info for table "
          " with handle %d, pipe %d device id %d",
          __func__,
          tbl_hdl,
          pipe_id,
          dev_tgt.device_id);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  adt_tbl_data = &adt_tbl->adt_tbl_data[pipe_idx];
  bf_map_t *map = &adt_tbl_data->entry_phy_info_htbl;
  unsigned long key = entry_hdl;
  pipe_mgr_adt_entry_phy_info_t *entry_info = NULL;

  int i = 0;
  while (i < n) {
    bf_map_sts_t map_sts = bf_map_get_next(map, &key, (void **)&entry_info);
    if (map_sts != BF_MAP_OK) {
      /* No more handles in this instance.  If this is pipe-all (all instances)
       * move on to the next instance (which has handles) and get the first
       * handle there to continue with.  If this is for a single instance then
       * the walk is over. */
      if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL) {
        break;
      }
      for (++pipe_idx; pipe_idx < adt_tbl->num_tbls; ++pipe_idx) {
        adt_tbl_data = &adt_tbl->adt_tbl_data[pipe_idx];
        map = &adt_tbl_data->entry_phy_info_htbl;
        map_sts = bf_map_get_first(map, &key, (void **)&entry_info);
        if (map_sts == BF_MAP_OK) {
          /* This instance has a handle, stop searching for a new instance. */
          break;
        }
      }
    }
    if (map_sts != BF_MAP_OK) {
      break;
    }

    /* Skip over const (init time default) entries. */
    if (entry_info->entry_data->is_const) continue;
    /* Even if the entry is not programmed in hardware return it, otherwise
     * table walks at the LLP will not return all entries known. */
    /* if (!entry_info->sharable_stage_location &&
     *    !entry_info->non_sharable_stage_location)
     *  continue; */

    next_entry_handles[i++] = key;
  }

  if (i < n) next_entry_handles[i] = -1;

  /* If there are no handles being returned then give an error.  If at least
   * one handle is there then return success. */
  return !i ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

void pipe_mgr_adt_cleanup_ent_refs(pipe_mgr_adt_data_t *adt_tbl_data) {
  /* Walk the map, remove, free each element */
  unsigned long unused_key = 0;
  uint32_t *num_refs = NULL;
  while (BF_MAP_OK == bf_map_get_first_rmv(&adt_tbl_data->adt_ent_refs,
                                           &unused_key,
                                           (void **)&num_refs)) {
    if (num_refs) {
      PIPE_MGR_FREE(num_refs);
    }
  }
  adt_tbl_data->adt_ent_refs = NULL;
  return;
}

pipe_status_t pipe_mgr_adt_tbl_delete(bf_dev_id_t device_id,
                                      pipe_adt_tbl_hdl_t adt_tbl_hdl) {
  pipe_mgr_adt_t *adt = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;

  adt = pipe_mgr_adt_get(device_id, adt_tbl_hdl);

  if (adt == NULL) {
    /* Nothing to do */
    return PIPE_OBJ_NOT_FOUND;
  }
  pipe_mgr_adt_cleanup_tbl(adt);
  /* Remove the adt tbl reference from the hash table */
  key = adt_tbl_hdl;
  map_sts = pipe_mgr_adt_tbl_map_rmv(device_id, key);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing the action data table reference for tbl %d"
        " device id %d from the hash table",
        __func__,
        __LINE__,
        adt_tbl_hdl,
        device_id);
    return PIPE_UNEXPECTED;
  }
  PIPE_MGR_FREE(adt);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_adt_tbl_get_num_entries(bf_dev_id_t dev,
                                                      pipe_tbl_hdl_t tbl_hdl,
                                                      uint32_t *count_p) {
  pipe_mgr_adt_t *adt_tbl = NULL;
  uint32_t usage = 0;

  adt_tbl = pipe_mgr_adt_get(dev, tbl_hdl);
  if (adt_tbl == NULL) {
    return PIPE_OBJ_NOT_FOUND;
  }

  if (pipe_mgr_is_device_virtual(dev)) {
    usage = pipe_mgr_adt_get_num_hlp_entries(adt_tbl);
  } else {
    usage = pipe_mgr_adt_get_num_llp_entries(adt_tbl);
  }
  *count_p = usage;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_shadow_mem_init(pipe_mgr_adt_t *adt) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_data_t *adt_tbl_data = NULL;
  pipe_mgr_adt_stage_info_t *adt_stage_info = NULL;

  unsigned i = 0;
  unsigned j = 0;
  for (i = 0; i < adt->num_tbls; i++) {
    adt_tbl_data = &adt->adt_tbl_data[i];
    for (j = 0; j < adt_tbl_data->num_stages; j++) {
      adt_stage_info = &adt_tbl_data->adt_stage_info[j];
      status = pipe_mgr_adt_stage_shadow_mem_init(
          adt, adt_stage_info, &adt_tbl_data->pipe_bmp, adt_tbl_data->pipe_id);
      if (status != PIPE_SUCCESS) {
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_adt_stage_shadow_mem_init(
    pipe_mgr_adt_t *adt,
    pipe_mgr_adt_stage_info_t *adt_stage_info,
    pipe_bitmap_t *pipe_bmp,
    bf_dev_pipe_t pipe_id) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_adt_ram_alloc_info_t *adt_ram_alloc_info = NULL;
  unsigned i = 0;
  unsigned j = 0;
  (void)pipe_id;

  adt_ram_alloc_info = adt_stage_info->ram_alloc_info;
  for (i = 0; i < adt_ram_alloc_info->num_wide_word_blks; i++) {
    for (j = 0; j < adt_ram_alloc_info->num_rams_in_wide_word; j++) {
      status = pipe_mgr_phy_mem_map_symmetric_mode_set(
          adt->dev_id,
          adt->direction,
          pipe_bmp,
          adt_stage_info->stage_id,
          pipe_mem_type_unit_ram,
          adt_ram_alloc_info->tbl_word_blk[i].mem_id[j],
          adt->symmetric);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in setting pipe list for shadow mem"
            " for tbl 0x%x, device id %d, err %s",
            __func__,
            __LINE__,
            adt->adt_tbl_hdl,
            adt->dev_id,
            pipe_str_err(status));
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}
