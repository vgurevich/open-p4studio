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
 * @file pipe_mgr_stat_tbl_init.c
 * @date
 *
 *
 * Contains initialization code for statistics tables
 */

/* Global header includes */

/* Module header includes */
#include <stdbool.h>
#include <sched.h>
#include <pipe_mgr/pipe_mgr_intf.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stat_mgr_int.h"
#include "pipe_mgr_stat_tbl_init.h"
#include "pipe_mgr_tbl.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_phy_mem_map.h"

static pipe_status_t block_write_one_map_ram(pipe_sess_hdl_t sess_hdl,
                                             rmt_dev_info_t *dev_info,
                                             pipe_tbl_dir_t gress,
                                             int pipe_mask,
                                             int unit_ram_id,
                                             int vpn_id,
                                             int stage) {
  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;
  mem_id_t map_ram_id = dev_cfg->map_ram_from_unit_ram(unit_ram_id);
  uint64_t map_phy_addr = dev_cfg->get_full_phy_addr(
      gress, 0, stage, map_ram_id, 0, pipe_mem_type_map_ram);
  uint8_t map_ram_data[4] = {~vpn_id & 0x3F, 0, 0, 0};
  pipe_status_t x = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                             dev_info,
                                             4,
                                             TOF_MAP_RAM_UNIT_DEPTH,
                                             1,
                                             map_phy_addr,
                                             pipe_mask,
                                             map_ram_data);
  PIPE_MGR_DBGCHK(PIPE_SUCCESS == x);
  return x;
}

static pipe_status_t pipe_mgr_stat_mgr_init_ram_alloc_info(
    pipe_mgr_stat_ram_alloc_info_t *stat_ram_alloc_info,
    rmt_tbl_bank_map_t *bank_map) {
  if (stat_ram_alloc_info == NULL) {
    LOG_ERROR("%s: Ram alloc info passed in is NULL", __func__);
    return PIPE_INVALID_ARG;
  }

  stat_ram_alloc_info->num_wide_word_blks = bank_map->num_tbl_word_blks;

  stat_ram_alloc_info->tbl_word_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_CALLOC(
      stat_ram_alloc_info->num_wide_word_blks, sizeof(rmt_tbl_word_blk_t));

  if (stat_ram_alloc_info->tbl_word_blk == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  PIPE_MGR_MEMCPY(
      stat_ram_alloc_info->tbl_word_blk,
      bank_map->tbl_word_blk,
      sizeof(rmt_tbl_word_blk_t) * stat_ram_alloc_info->num_wide_word_blks);

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_stat_tbl_stage_init(
    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info,
    rmt_tbl_info_t *rmt_tbl_info) {
  pipe_status_t status = PIPE_SUCCESS;

  if (stat_tbl_stage_info == NULL) {
    LOG_ERROR("%s : Stat table stage info passed in is Null", __func__);
    return PIPE_INVALID_ARG;
  }

  PIPE_MGR_ASSERT(rmt_tbl_info->type == RMT_TBL_TYPE_STATS);
  stat_tbl_stage_info->stage_id = rmt_tbl_info->stage_id;
  stat_tbl_stage_info->num_entries = rmt_tbl_info->num_entries;
  stat_tbl_stage_info->num_entries_per_line =
      rmt_tbl_info->pack_format.entries_per_tbl_word;
  stat_tbl_stage_info->stage_table_handle = rmt_tbl_info->handle;

  /* Some sanity check on well-known constraints/facts about statistics
   * on Tofino.
   */
  PIPE_MGR_ASSERT(rmt_tbl_info->pack_format.mem_units_per_tbl_word == 1);
  PIPE_MGR_ASSERT(rmt_tbl_info->num_tbl_banks == 1);

  /* Initialize RAM allocation info */
  stat_tbl_stage_info->stat_ram_alloc_info =
      (pipe_mgr_stat_ram_alloc_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_stat_ram_alloc_info_t));

  if (stat_tbl_stage_info->stat_ram_alloc_info == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  status = pipe_mgr_stat_mgr_init_ram_alloc_info(
      stat_tbl_stage_info->stat_ram_alloc_info, rmt_tbl_info->bank_map);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing ram allocation info for stage id %d"
        " error %s",
        __func__,
        stat_tbl_stage_info->stage_id,
        pipe_str_err(status));
    goto err_cleanup;
  }

  /* Sanity check on the information published.  The number of entries should
   * agree with the number of RAMs published. */
  if (stat_tbl_stage_info->stat_ram_alloc_info->num_wide_word_blks * 1024u *
          stat_tbl_stage_info->num_entries_per_line !=
      stat_tbl_stage_info->num_entries) {
    PIPE_MGR_DBGCHK(
        stat_tbl_stage_info->stat_ram_alloc_info->num_wide_word_blks * 1024u *
            stat_tbl_stage_info->num_entries_per_line ==
        stat_tbl_stage_info->num_entries);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }

  return PIPE_SUCCESS;

err_cleanup:
  if (stat_tbl_stage_info->stat_ram_alloc_info) {
    PIPE_MGR_FREE(stat_tbl_stage_info->stat_ram_alloc_info);
    stat_tbl_stage_info->stat_ram_alloc_info = NULL;
  }
  return status;
}

static pipe_status_t pipe_mgr_stat_tbl_instance_init(
    pipe_mgr_stat_tbl_t *stat_tbl,
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance,
    pipe_stat_tbl_info_t *stat_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t pipe = 0;
  uint32_t num_entries = 0;

  int dev_pipes = stat_tbl->dev_info->num_active_pipes;
  int dev_stages = stat_tbl->dev_info->num_active_mau;

  stat_tbl_instance->pipe_id = pipe_id;
  stat_tbl_instance->lowest_pipe_id = PIPE_BITMAP_GET_FIRST_SET(pipe_bmp);
  PIPE_BITMAP_INIT(&stat_tbl_instance->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&stat_tbl_instance->pipe_bmp, pipe_bmp);
  stat_tbl_instance->capacity = stat_tbl_info->size;
  stat_tbl_instance->num_stages = stat_tbl_info->num_rmt_info;

  /* Now initialize the stage info */
  stat_tbl_instance->stat_tbl_stage_info = PIPE_MGR_CALLOC(
      stat_tbl_instance->num_stages, sizeof(pipe_mgr_stat_tbl_stage_info_t));
  if (stat_tbl_instance->stat_tbl_stage_info == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  uint32_t running_offset = 0;
  for (unsigned i = 0; i < stat_tbl_instance->num_stages; i++) {
    status =
        pipe_mgr_stat_tbl_stage_init(&stat_tbl_instance->stat_tbl_stage_info[i],
                                     &stat_tbl_info->rmt_info[i]);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Dev %d error initializing stat tbl %s (0x%x) stage %d info, "
          "error %s",
          __func__,
          stat_tbl->device_id,
          stat_tbl->name,
          stat_tbl->stat_tbl_hdl,
          stat_tbl_info->rmt_info[i].stage_id,
          pipe_str_err(status));
      return status;
    }
    stat_tbl_instance->stat_tbl_stage_info[i].ent_idx_offset = running_offset;
    running_offset += stat_tbl_instance->stat_tbl_stage_info[i].num_entries;
  }

  /* The total number of entries in the instance is the sum total of the number
   * of entries ALLOCATED.
   */
  if (stat_tbl_info->ref_type == PIPE_TBL_REF_TYPE_DIRECT) {
    stat_tbl_instance->num_entries = running_offset;
  } else {
    stat_tbl_instance->num_entries = stat_tbl_instance->capacity;
  }

  stat_tbl_instance->trace_idx = 0;
  stat_tbl_instance->trace = PIPE_MGR_CALLOC(PIPE_MGR_STAT_TRACE_SIZE,
                                             sizeof *stat_tbl_instance->trace);
  if (!stat_tbl_instance->trace) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_LOCK_INIT(stat_tbl_instance->trace_mtx);

  stat_tbl_instance->ent_idx_info =
      PIPE_MGR_CALLOC(dev_pipes, sizeof *stat_tbl_instance->ent_idx_info);
  if (!stat_tbl_instance->ent_idx_info) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_BITMAP_ITER(pipe_bmp, pipe) {
    stat_tbl_instance->ent_idx_info[pipe] = PIPE_MGR_CALLOC(
        dev_stages, sizeof *stat_tbl_instance->ent_idx_info[pipe]);
    if (!stat_tbl_instance->ent_idx_info[pipe]) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    for (unsigned j = 0; j < stat_tbl_instance->num_stages; j++) {
      dev_stage_t stage_id = stat_tbl_instance->stat_tbl_stage_info[j].stage_id;
      num_entries = stat_tbl_instance->stat_tbl_stage_info[j].num_entries;
      stat_tbl_instance->ent_idx_info[pipe][stage_id] =
          PIPE_MGR_CALLOC(num_entries, sizeof(pipe_mgr_stat_ent_idx_info_t));
      if (stat_tbl_instance->ent_idx_info[pipe][stage_id] == NULL) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      for (unsigned k = 0; k < num_entries; k++) {
        pipe_mgr_stat_ent_idx_info_t *ent_idx_info =
            &stat_tbl_instance->ent_idx_info[pipe][stage_id][k];
        PIPE_MGR_LOCK_INIT(ent_idx_info->entry_info.mtx);
      }
    }
  }

  if (stat_tbl->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    /* The per entry index count is used only for the indirect
     * counters, since for direct addressed counters, this is held
     * in the per entry handle info. This is to hold the "user set"
     * count value, which will be added to live count values and reported
     * in queries.  */
    stat_tbl_instance->user_idx_count = PIPE_MGR_CALLOC(
        stat_tbl_instance->num_entries, sizeof(pipe_stat_data_t));
    if (stat_tbl_instance->user_idx_count == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  stat_tbl_instance->barrier_list =
      PIPE_MGR_CALLOC(dev_pipes, sizeof *stat_tbl_instance->barrier_list);
  if (!stat_tbl_instance->barrier_list) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  PIPE_MGR_LOCK_INIT(stat_tbl_instance->barrier_data_mtx);
  stat_tbl_instance->def_barrier_node =
      PIPE_MGR_CALLOC(dev_pipes, sizeof *stat_tbl_instance->def_barrier_node);
  PIPE_BITMAP_ITER(pipe_bmp, pipe) {
    stat_tbl_instance->def_barrier_node[pipe] = PIPE_MGR_CALLOC(
        dev_stages, sizeof *stat_tbl_instance->def_barrier_node[pipe]);
  }

  bf_map_init(&stat_tbl_instance->ent_hdl_loc);
  PIPE_MGR_LOCK_INIT(stat_tbl_instance->ent_hdl_loc_mtx);

  if (stat_tbl->lrt_enabled) {
    stat_tbl_instance->ent_idx_lrt_dbg_info = PIPE_MGR_CALLOC(
        dev_pipes, sizeof(*stat_tbl_instance->ent_idx_lrt_dbg_info));
    if (!stat_tbl_instance->ent_idx_lrt_dbg_info) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    PIPE_BITMAP_ITER(pipe_bmp, pipe) {
      stat_tbl_instance->ent_idx_lrt_dbg_info[pipe] = PIPE_MGR_CALLOC(
          dev_stages, sizeof(*stat_tbl_instance->ent_idx_lrt_dbg_info[pipe]));
      if (!stat_tbl_instance->ent_idx_lrt_dbg_info[pipe]) {
        LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
        return PIPE_NO_SYS_RESOURCES;
      }
      for (unsigned j = 0; j < stat_tbl_instance->num_stages; j++) {
        dev_stage_t stage_id =
            stat_tbl_instance->stat_tbl_stage_info[j].stage_id;
        num_entries = stat_tbl_instance->stat_tbl_stage_info[j].num_entries;
        stat_tbl_instance->ent_idx_lrt_dbg_info[pipe][stage_id] =
            PIPE_MGR_CALLOC(num_entries, sizeof(pipe_mgr_stat_lrt_dbg_info_t));
        if (stat_tbl_instance->ent_idx_lrt_dbg_info[pipe][stage_id] == NULL) {
          LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
          return PIPE_NO_SYS_RESOURCES;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

static pipe_status_t pipe_mgr_stat_tbl_init_instances(
    pipe_mgr_stat_tbl_t *stat_tbl, pipe_stat_tbl_info_t *stat_tbl_info) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id = 0;

  stat_tbl->stat_tbl_instances = PIPE_MGR_CALLOC(
      stat_tbl->num_instances, sizeof *stat_tbl->stat_tbl_instances);
  if (stat_tbl->stat_tbl_instances == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (unsigned i = 0; i < stat_tbl->num_instances; i++) {
    pipe_bitmap_t local_pipe_bmp;
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(stat_tbl->scope_pipe_bmp[i],
                                    &local_pipe_bmp);
    if (stat_tbl->symmetric == false) {
      pipe_id = pipe_mgr_get_lowest_pipe_in_scope(stat_tbl->scope_pipe_bmp[i]);
    } else {
      /* For symmetric tables, the table is present in ALL pipes */
      pipe_id = BF_DEV_PIPE_ALL;
    }
    status = pipe_mgr_stat_tbl_instance_init(stat_tbl,
                                             &stat_tbl->stat_tbl_instances[i],
                                             stat_tbl_info,
                                             pipe_id,
                                             &local_pipe_bmp);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s : Error in initializing stat tbl instances, tbl %d, device id %d",
          __func__,
          stat_tbl->stat_tbl_hdl,
          stat_tbl->device_id);
      return status;
    }
  }

  return PIPE_SUCCESS;
}

static void pipe_mgr_stat_tbl_lkup_set(pipe_mgr_stat_tbl_t *stat_tbl,
                                       bool add) {
  /* For each pipe, and each stage in which the table is present, add a mapping
   * if the "add" argument is true otherwise we remove the mapping by having the
   * key map to NULL. */
  bf_dev_id_t dev_id = stat_tbl->device_id;

  for (uint32_t i = 0; i < stat_tbl->num_instances; i++) {
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance =
        &stat_tbl->stat_tbl_instances[i];

    bf_dev_pipe_t pipe_id = 0;
    PIPE_BITMAP_ITER(&stat_tbl_instance->pipe_bmp, pipe_id) {
      for (uint8_t stage_idx = 0; stage_idx < stat_tbl_instance->num_stages;
           stage_idx++) {
        pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info =
            &stat_tbl_instance->stat_tbl_stage_info[stage_idx];
        dev_stage_t stage_id = stat_tbl_stage_info->stage_id;
        /* The stage table handle is also known as the logical table id */
        uint8_t ltbl_id = stat_tbl_stage_info->stage_table_handle;
        /* Add/update the mapping */
        pipe_mgr_stat_mgr_update_tbl_hdl(
            dev_id, pipe_id, stage_id, ltbl_id, add ? stat_tbl : NULL);
      }
    }
  }
}

pipe_status_t pipe_mgr_stat_tbl_init(pipe_sess_hdl_t sess_hdl,
                                     bf_dev_id_t dev_id,
                                     pipe_stat_tbl_hdl_t stat_tbl_hdl,
                                     profile_id_t profile_id,
                                     pipe_bitmap_t *pipe_bmp) {
  LOG_TRACE("Entering %s for dev id %d, tbl_hdl 0x%x, profile %d",
            __func__,
            dev_id,
            stat_tbl_hdl,
            profile_id);

  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_stat_tbl_info_t *stat_tbl_info = NULL;
  bf_map_sts_t map_sts;
  unsigned long htbl_key = 0;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  uint32_t q = 0;

  if (!dev_info) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* First, check if the table already exists */
  stat_tbl = pipe_mgr_stat_tbl_get(dev_id, stat_tbl_hdl);

  if (stat_tbl != NULL) {
    LOG_ERROR(
        "%s:%d Attempt to initialize Stat table 0x%x on device %d which "
        "already exists",
        __func__,
        __LINE__,
        stat_tbl_hdl,
        dev_id);
    return PIPE_ALREADY_EXISTS;
  }

  stat_tbl_info =
      pipe_mgr_get_stat_tbl_info(dev_id, stat_tbl_hdl, __func__, __LINE__);

  if (stat_tbl_info == NULL) {
    LOG_ERROR("%s : Stat tbl info not found for tbl hdl %d, device id %d",
              __func__,
              stat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  stat_tbl =
      (pipe_mgr_stat_tbl_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_stat_tbl_t));

  if (stat_tbl == NULL) {
    LOG_ERROR("%s/%d : Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  stat_tbl->dev_info = dev_info;
  stat_tbl->name = bf_sys_strdup(stat_tbl_info->name);
  stat_tbl->direction = stat_tbl_info->direction;
  stat_tbl->device_id = dev_id;
  stat_tbl->symmetric = stat_tbl_info->symmetric;
  stat_tbl->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!stat_tbl->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  /* Set the scope info */
  if (stat_tbl->symmetric) {
    stat_tbl->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) { stat_tbl->scope_pipe_bmp[0] |= (1 << q); }
  } else {
    stat_tbl->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      stat_tbl->scope_pipe_bmp[q] |= (1 << q);
      stat_tbl->num_scopes += 1;
    }
  }

  stat_tbl->stat_tbl_hdl = stat_tbl_hdl;
  stat_tbl->profile_id = profile_id;
  /* For now setting the counter resolution to 64 bits
   * The compiler will publish this info at the table level
   */
  stat_tbl->packet_counter_resolution =
      stat_tbl_info->packet_counter_resolution;
  stat_tbl->byte_counter_resolution = stat_tbl_info->byte_counter_resolution;
  stat_tbl->counter_type = stat_tbl_info->stat_type;
  stat_tbl->ref_type = stat_tbl_info->ref_type;
  stat_tbl->enable_per_flow_enable = stat_tbl_info->enable_per_flow_enable;
  stat_tbl->per_flow_enable_bit_position =
      stat_tbl_info->per_flow_enable_bit_position;
  PIPE_BITMAP_INIT(&stat_tbl->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&stat_tbl->pipe_bmp, pipe_bmp);

  if (stat_tbl->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    stat_tbl->over_allocated = true;
  }

  /* LR(t) is enabled only for counter resolution < 64 bits */
  switch (stat_tbl->counter_type) {
    case PACKET_COUNT:
      if (stat_tbl->packet_counter_resolution != 64) {
        stat_tbl->lrt_enabled = true;
      }
      break;
    case BYTE_COUNT:
      if (stat_tbl->byte_counter_resolution != 64) {
        stat_tbl->lrt_enabled = true;
      }
      break;
    case PACKET_AND_BYTE_COUNT:
      if (stat_tbl->packet_counter_resolution != 64 ||
          stat_tbl->byte_counter_resolution != 64) {
        stat_tbl->lrt_enabled = true;
      }

      break;
    default:
      PIPE_MGR_DBGCHK(0);
  }

  stat_tbl->num_instances = stat_tbl->num_scopes;

  if (!pipe_mgr_is_device_virtual(dev_id) &&
      !pipe_mgr_hitless_warm_init_in_progress(dev_id)) {
    /* Block write the mapRAMs. */

    /* First, build a bit mask of pipes for the block write. */
    bf_dev_pipe_t p = 0;
    int pipe_mask = 0;
    PIPE_BITMAP_ITER(pipe_bmp, p) { pipe_mask |= 1 << p; }
    uint32_t stage_idx;
    /* For each stage, block write each mapRAM assocaited with each unit RAM the
     * table is using. */
    for (stage_idx = 0; stage_idx < stat_tbl_info->num_rmt_info; ++stage_idx) {
      rmt_tbl_info_t *stage_info = stat_tbl_info->rmt_info + stage_idx;
      int stage = stage_info->stage_id;
      int bank, block, ram_id, vpn_id;
      for (bank = 0; bank < stage_info->num_tbl_banks; ++bank) {
        rmt_tbl_bank_map_t *bm = stage_info->bank_map + bank;
        for (block = 0; block < bm->num_tbl_word_blks; ++block) {
          ram_id = bm->tbl_word_blk[block].mem_id[0];
          vpn_id = bm->tbl_word_blk[block].vpn_id[0];
          status = block_write_one_map_ram(sess_hdl,
                                           dev_info,
                                           stat_tbl_info->direction,
                                           pipe_mask,
                                           ram_id,
                                           vpn_id,
                                           stage);
          if (PIPE_SUCCESS != status) goto err_cleanup;
        }
      }
      /* Also do the spare ram! */
      for (block = 0; block < stage_info->num_spare_rams; block++) {
        ram_id = stage_info->spare_rams[block];
        status = block_write_one_map_ram(sess_hdl,
                                         dev_info,
                                         stat_tbl_info->direction,
                                         pipe_mask,
                                         ram_id,
                                         0x3F,
                                         stage);
        if (PIPE_SUCCESS != status) goto err_cleanup;
      }
    }
    /* Also notify the shadow manager of this table's RAM allocation so it can
     * initialize the memory. */
    for (stage_idx = 0; stage_idx < stat_tbl_info->num_rmt_info; ++stage_idx) {
      rmt_tbl_info_t *stage_info = stat_tbl_info->rmt_info + stage_idx;
      int stage = stage_info->stage_id;
      int bank, block, ram_id;
      for (bank = 0; bank < stage_info->num_tbl_banks; ++bank) {
        rmt_tbl_bank_map_t *bm = stage_info->bank_map + bank;
        for (block = 0; block < bm->num_tbl_word_blks; ++block) {
          ram_id = bm->tbl_word_blk[block].mem_id[0];
          status =
              pipe_mgr_phy_mem_map_symmetric_mode_set(dev_id,
                                                      stat_tbl_info->direction,
                                                      pipe_bmp,
                                                      stage,
                                                      pipe_mem_type_unit_ram,
                                                      ram_id,
                                                      true);
          if (PIPE_SUCCESS != status) goto err_cleanup;
        }
      }
      for (block = 0; block < stage_info->num_spare_rams; block++) {
        status = pipe_mgr_phy_mem_map_symmetric_mode_set(
            dev_id,
            stat_tbl_info->direction,
            pipe_bmp,
            stage,
            pipe_mem_type_unit_ram,
            stage_info->spare_rams[block],
            true);
        if (PIPE_SUCCESS != status) goto err_cleanup;
      }
    }
  }

  /* Now initialize all the managed instances */
  status = pipe_mgr_stat_tbl_init_instances(stat_tbl, stat_tbl_info);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s : Error in initializing stat tbl instances, for table with"
        " handle %d, device id %d",
        __func__,
        stat_tbl_hdl,
        dev_id);
    goto err_cleanup;
  }

  htbl_key = stat_tbl_hdl;

  /* Insert the table info into the hash map */
  map_sts = pipe_mgr_stat_tbl_map_add(dev_id, htbl_key, (void *)stat_tbl);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s : Error in inserting the stat table struct into the hash"
        " table for table handle %d, device id %d",
        __func__,
        stat_tbl_hdl,
        dev_id);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }

  pipe_mgr_stat_tbl_lkup_set(stat_tbl, true);

  LOG_TRACE("Exiting %s successfully for dev_id %d, tbl_hdl 0x%x",
            __func__,
            dev_id,
            stat_tbl_hdl);
err_cleanup:
  if (status != PIPE_SUCCESS) {
    pipe_mgr_stat_mgr_tbl_delete(dev_id, stat_tbl);
  }
  return status;
}

void pipe_mgr_stat_mgr_tbl_cleanup(bf_dev_id_t device_id,
                                   pipe_stat_tbl_hdl_t stat_tbl_hdl) {
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long key = 0;

  key = stat_tbl_hdl;

  stat_tbl = pipe_mgr_stat_tbl_get(device_id, stat_tbl_hdl);

  if (stat_tbl == NULL) {
    return;
  }

  pipe_mgr_stat_mgr_tbl_delete(device_id, stat_tbl);

  map_sts = pipe_mgr_stat_tbl_map_rmv(device_id, key);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing stat tbl %d, device id %d"
        " from the hash table",
        __func__,
        __LINE__,
        stat_tbl_hdl,
        device_id);
  }
  if (stat_tbl->scope_pipe_bmp) {
    PIPE_MGR_FREE(stat_tbl->scope_pipe_bmp);
  }

  PIPE_MGR_FREE(stat_tbl);

  return;
}

static void pipe_mgr_stat_tbl_delete_stage_info(
    pipe_mgr_stat_tbl_stage_info_t *stat_tbl_stage_info) {
  if (stat_tbl_stage_info && stat_tbl_stage_info->stat_ram_alloc_info) {
    if (stat_tbl_stage_info->stat_ram_alloc_info->tbl_word_blk) {
      PIPE_MGR_FREE(stat_tbl_stage_info->stat_ram_alloc_info->tbl_word_blk);
      stat_tbl_stage_info->stat_ram_alloc_info->tbl_word_blk = NULL;
    }

    PIPE_MGR_FREE(stat_tbl_stage_info->stat_ram_alloc_info);
    stat_tbl_stage_info->stat_ram_alloc_info = NULL;
  }
}

static void pipe_mgr_stat_tbl_delete_instance(
    pipe_mgr_stat_tbl_instance_t *stat_tbl_instance) {
  if (stat_tbl_instance == NULL) {
    /* Nothing to do */
    return;
  }
  bf_dev_pipe_t pipe = 0;
  pipe_bitmap_t *pipe_bmp;
  pipe_bmp = &stat_tbl_instance->pipe_bmp;

  if (stat_tbl_instance->ent_idx_lrt_dbg_info) {
    PIPE_BITMAP_ITER(pipe_bmp, pipe) {
      if (stat_tbl_instance->ent_idx_lrt_dbg_info[pipe]) {
        for (unsigned j = 0; j < stat_tbl_instance->num_stages; j++) {
          dev_stage_t stage_id =
              stat_tbl_instance->stat_tbl_stage_info[j].stage_id;
          if (stat_tbl_instance->ent_idx_lrt_dbg_info[pipe][stage_id]) {
            PIPE_MGR_FREE(
                stat_tbl_instance->ent_idx_lrt_dbg_info[pipe][stage_id]);
            stat_tbl_instance->ent_idx_lrt_dbg_info[pipe][stage_id] = NULL;
          }
        }
        PIPE_MGR_FREE(stat_tbl_instance->ent_idx_lrt_dbg_info[pipe]);
        stat_tbl_instance->ent_idx_lrt_dbg_info[pipe] = NULL;
      }
    }
    PIPE_MGR_FREE(stat_tbl_instance->ent_idx_lrt_dbg_info);
    stat_tbl_instance->ent_idx_lrt_dbg_info = NULL;
  }

  if (stat_tbl_instance->ent_hdl_loc) {
    pipe_mgr_stat_mgr_destroy_ent_hdl_loc(stat_tbl_instance);
    PIPE_MGR_LOCK_DESTROY(&stat_tbl_instance->ent_hdl_loc_mtx);
  }

  if (stat_tbl_instance->barrier_list) {
    PIPE_MGR_FREE(stat_tbl_instance->barrier_list);
    stat_tbl_instance->barrier_list = NULL;
    PIPE_MGR_LOCK_DESTROY(&stat_tbl_instance->barrier_data_mtx);
  }
  if (stat_tbl_instance->def_barrier_node) {
    PIPE_BITMAP_ITER(pipe_bmp, pipe) {
      if (stat_tbl_instance->def_barrier_node[pipe]) {
        PIPE_MGR_FREE(stat_tbl_instance->def_barrier_node[pipe]);
        stat_tbl_instance->def_barrier_node[pipe] = NULL;
      }
    }
    PIPE_MGR_FREE(stat_tbl_instance->def_barrier_node);
    stat_tbl_instance->def_barrier_node = NULL;
  }

  if (stat_tbl_instance->user_idx_count) {
    PIPE_MGR_FREE(stat_tbl_instance->user_idx_count);
    stat_tbl_instance->user_idx_count = NULL;
  }

  if (stat_tbl_instance->ent_idx_info) {
    PIPE_BITMAP_ITER(pipe_bmp, pipe) {
      for (unsigned j = 0; j < stat_tbl_instance->num_stages; j++) {
        dev_stage_t stage_id =
            stat_tbl_instance->stat_tbl_stage_info[j].stage_id;
        uint32_t num_entries =
            stat_tbl_instance->stat_tbl_stage_info[j].num_entries;
        unsigned k = 0;
        for (k = 0; k < num_entries; k++) {
          pipe_mgr_stat_ent_idx_info_t *ent_idx_info =
              stat_tbl_instance->ent_idx_info[pipe][stage_id];

          PIPE_MGR_LOCK_DESTROY(&ent_idx_info[k].entry_info.mtx);
        }
        PIPE_MGR_FREE(stat_tbl_instance->ent_idx_info[pipe][stage_id]);
        stat_tbl_instance->ent_idx_info[pipe][stage_id] = NULL;
      }
      if (stat_tbl_instance->ent_idx_info[pipe]) {
        PIPE_MGR_FREE(stat_tbl_instance->ent_idx_info[pipe]);
        stat_tbl_instance->ent_idx_info[pipe] = NULL;
      }
    }
    PIPE_MGR_FREE(stat_tbl_instance->ent_idx_info);
    stat_tbl_instance->ent_idx_info = NULL;
  }

  if (stat_tbl_instance->stat_tbl_stage_info) {
    for (unsigned i = 0; i < stat_tbl_instance->num_stages; i++) {
      pipe_mgr_stat_tbl_delete_stage_info(
          &stat_tbl_instance->stat_tbl_stage_info[i]);
    }
    PIPE_MGR_FREE(stat_tbl_instance->stat_tbl_stage_info);
    stat_tbl_instance->stat_tbl_stage_info = NULL;
  }

  if (stat_tbl_instance->trace) {
    PIPE_MGR_LOCK_DESTROY(&stat_tbl_instance->trace_mtx);
    PIPE_MGR_FREE(stat_tbl_instance->trace);
    stat_tbl_instance->trace = NULL;
  }

  return;
}

void pipe_mgr_stat_mgr_tbl_delete(bf_dev_id_t device_id,
                                  pipe_mgr_stat_tbl_t *stat_tbl) {
  if (stat_tbl == NULL) {
    /* Nothing to do */
    return;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (!dev_info) return;

  unsigned i = 0;

  /* Delete the mapping from the logical table id in a given stage, pipe
   * and for a device for this table.
   */
  pipe_mgr_stat_tbl_lkup_set(stat_tbl, false);

  for (i = 0; i < stat_tbl->num_instances; i++) {
    pipe_mgr_stat_tbl_delete_instance(&stat_tbl->stat_tbl_instances[i]);
  }
  PIPE_MGR_FREE(stat_tbl->stat_tbl_instances);
  stat_tbl->stat_tbl_instances = NULL;

  if (stat_tbl->name) {
    PIPE_MGR_FREE(stat_tbl->name);
    stat_tbl->name = NULL;
  }

  return;
}

pipe_status_t pipe_mgr_stat_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mgr_stat_tbl_t *tbl = pipe_mgr_stat_tbl_get(dev_id, tbl_hdl);
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

pipe_status_t pipe_mgr_stat_tbl_set_symmetric_mode(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_stat_tbl_info_t *stat_tbl_info = NULL;

  stat_tbl_info =
      pipe_mgr_get_stat_tbl_info(device_id, tbl_hdl, __func__, __LINE__);
  if (stat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Stat tbl info not found for tbl hdl 0x%x, device id %d",
              __func__,
              __LINE__,
              tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }
  stat_tbl = pipe_mgr_stat_tbl_get(device_id, tbl_hdl);
  if (stat_tbl == NULL) {
    LOG_ERROR("%s:%d Stat table for device id %d, tbl hdl 0x%x not found",
              __func__,
              __LINE__,
              device_id,
              tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Check if the scope has changed */
  if (!pipe_mgr_tbl_is_scope_different(device_id,
                                       tbl_hdl,
                                       symmetric,
                                       num_scopes,
                                       scope_pipe_bmp,
                                       stat_tbl->symmetric,
                                       stat_tbl->num_scopes,
                                       &stat_tbl->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              stat_tbl->name,
              stat_tbl->symmetric,
              stat_tbl->num_scopes);
    /* Nothing to be done */
    return PIPE_SUCCESS;
  }

  /* Before cleaning up the table instances, need to ensure all pending messages
   * from the hardware are processed. This involves checking the necessary state
   * that is maintained to indicate pending responses. It includes two things
   * 1. pending_resp_cnt : This will be non-zero if we are waiting on a barrier
   *    acks for barriers sent as part of an entry dump or table dump and lock
   *    acks sent for LRT enabled stats tables.
   * 2. pending_barrier_responses : This will be non-zero if we are waiting on
   *    a barrier response for barriers sent as part of ZEROing out an entry.
   *    in hardware which is done for a new match entry.
   * When any response is pending, we service the LRT Rx DR inline in the hope
   * of draining all responses from hardware. Now, there can be another
   * dedicated thread which drains the responses. Hence at the end of each
   * loop over all pipe-ids and stage-ids in which the table exists, this thread
   * yields, so that the thread (if it exists) can finish draining. Once there
   * are no more pending responses, we are done draining and can safely clean
   * up the table instances.
   */
  pipe_mgr_stat_mgr_complete_operations(device_id, stat_tbl->stat_tbl_hdl);

  /* Delete the stat table instances to recreate the appropriate number */
  for (unsigned i = 0; i < stat_tbl->num_instances; i++) {
    pipe_mgr_stat_tbl_delete_instance(&stat_tbl->stat_tbl_instances[i]);
  }
  PIPE_MGR_FREE(stat_tbl->stat_tbl_instances);
  stat_tbl->symmetric = symmetric;
  /* Copy the new scope info */
  stat_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(stat_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  stat_tbl->stat_tbl_instances = NULL;
  if (symmetric) {
    PIPE_MGR_DBGCHK(stat_tbl->num_scopes == 1);
  }
  stat_tbl->num_instances = stat_tbl->num_scopes;

  status = pipe_mgr_stat_tbl_init_instances(stat_tbl, stat_tbl_info);
  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in initializing stat table instances for tbl 0x%x"
        " device id %d, err %s",
        __func__,
        __LINE__,
        tbl_hdl,
        device_id,
        pipe_str_err(status));
    return status;
  }

  return PIPE_SUCCESS;
}
