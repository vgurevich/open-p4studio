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
 * @file pipe_mgr_meter_tbl_init.c
 * @date
 *
 * Meter table initialization code.
 */

/* Standard header includes */

/* Module header includes */
#include <dvm/bf_dma_types.h>
#include <lld/bf_dma_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_meter_tbl_init.h"
#include "pipe_mgr_meter_tbl.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tbl.h"
#include <tofino_regs/tofino.h>

static pipe_status_t pipe_mgr_meter_mgr_init_mapram(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_meter_tbl_info_t *meter_tbl_info,
    pipe_bitmap_t pipe_bmp,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t stage_id = 0;
  int pipe_mask = 0;
  uint32_t blk_idx = 0;
  uint8_t spare_ram_idx;
  bf_dev_pipe_t pipe_id = 0;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_dev_cfg_t *dev_cfg = &dev_info->dev_cfg;

  if (meter_tbl_stage_info == NULL) {
    return PIPE_INVALID_ARG;
  }
  stage_id = meter_tbl_stage_info->stage_id;
  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  if (ram_alloc_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  PIPE_BITMAP_ITER(&pipe_bmp, pipe_id) { pipe_mask |= 1 << pipe_id; }

  for (blk_idx = 0; blk_idx < ram_alloc_info->num_wide_word_blks; blk_idx++) {
    uint64_t phy_addr = 0;
    mem_id_t ram_id = ram_alloc_info->tbl_word_blk[blk_idx].mem_id[0];
    vpn_id_t vpn_id = ram_alloc_info->tbl_word_blk[blk_idx].vpn_id[0];
    mem_id_t map_ram_id = dev_cfg->map_ram_from_unit_ram(ram_id);

    phy_addr = dev_cfg->get_full_phy_addr(meter_tbl_info->direction,
                                          0,
                                          stage_id,
                                          map_ram_id,
                                          0,
                                          pipe_mem_type_map_ram);

    uint8_t map_ram_data[4] = {~vpn_id & 0x3F, 0, 0, 0};
    status = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                      dev_info,
                                      4,
                                      TOF_MAP_RAM_UNIT_DEPTH,
                                      1,
                                      phy_addr,
                                      pipe_mask,
                                      map_ram_data);
    if (status != PIPE_SUCCESS) {
      PIPE_MGR_DBGCHK(0);
      return status;
    }
  }
  /* Zero out the Spare mapram VPN */
  for (spare_ram_idx = 0; spare_ram_idx < meter_tbl_stage_info->num_spare_rams;
       spare_ram_idx++) {
    uint64_t phy_addr = 0;
    mem_id_t ram_id = meter_tbl_stage_info->spare_rams[spare_ram_idx];
    vpn_id_t vpn_id = 0;
    mem_id_t map_ram_id = dev_cfg->map_ram_from_unit_ram(ram_id);
    if (map_ram_id == 0xffff) {
      LOG_ERROR("ERROR Invalid map_ram_id %u for table %s at stage %d",
                ram_id,
                meter_tbl_info->name,
                meter_tbl_stage_info->stage_id);
    }
    phy_addr = dev_cfg->get_full_phy_addr(meter_tbl_info->direction,
                                          0,
                                          stage_id,
                                          map_ram_id,
                                          0,
                                          pipe_mem_type_map_ram);

    uint8_t map_ram_data[4] = {vpn_id & 0x3F, 0, 0, 0};
    status = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                      dev_info,
                                      4,
                                      TOF_MAP_RAM_UNIT_DEPTH,
                                      1,
                                      phy_addr,
                                      pipe_mask,
                                      map_ram_data);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
  }

  return status;
}

static pipe_status_t pipe_mgr_meter_mgr_init_color_mapram(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_meter_tbl_info_t *meter_tbl_info,
    pipe_bitmap_t pipe_bmp,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info) {
  pipe_status_t status = PIPE_SUCCESS;
  uint8_t stage_id = 0;
  int pipe_mask = 0;
  uint32_t blk_idx = 0;
  bf_dev_pipe_t pipe_id = 0;
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;
  rmt_dev_cfg_t *dev_cfg = NULL;

  if (!dev_info || !meter_tbl_stage_info ||
      !meter_tbl_stage_info->ram_alloc_info) {
    return PIPE_INVALID_ARG;
  }

  dev_cfg = &dev_info->dev_cfg;
  /* LPF and WRED tables do not have color map ram, nothing to do here */
  if (!meter_tbl_stage_info->ram_alloc_info->color_tbl_word_blk) {
    return PIPE_SUCCESS;
  }

  stage_id = meter_tbl_stage_info->stage_id;
  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  if (ram_alloc_info == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  PIPE_BITMAP_ITER(&pipe_bmp, pipe_id) { pipe_mask |= 1 << pipe_id; }

  for (blk_idx = 0; blk_idx < ram_alloc_info->num_color_wide_word_blks;
       blk_idx++) {
    uint64_t phy_addr = 0;
    mem_id_t map_ram_id = ram_alloc_info->color_tbl_word_blk[blk_idx].mem_id[0];

    phy_addr = dev_cfg->get_full_phy_addr(meter_tbl_info->direction,
                                          0,
                                          stage_id,
                                          map_ram_id,
                                          0,
                                          pipe_mem_type_map_ram);

    uint8_t map_ram_data[4] = {0, 0, 0, 0};
    status = pipe_mgr_drv_blk_wr_data(&sess_hdl,
                                      dev_info,
                                      4,
                                      TOF_MAP_RAM_UNIT_DEPTH,
                                      1,
                                      phy_addr,
                                      pipe_mask,
                                      map_ram_data);
    PIPE_MGR_DBGCHK(PIPE_SUCCESS == status);
  }
  return status;
}

static pipe_status_t pipe_mgr_meter_tbl_instance_init(
    pipe_sess_hdl_t sess_hdl,
    rmt_dev_info_t *dev_info,
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance,
    pipe_meter_tbl_info_t *meter_tbl_info,
    bf_dev_pipe_t pipe_id,
    pipe_bitmap_t *pipe_bmp,
    bool hw_init) {
  pipe_status_t status = PIPE_SUCCESS;

  meter_tbl_instance->pipe_id = pipe_id;
  meter_tbl_instance->num_stages = meter_tbl_info->num_rmt_info;
  PIPE_BITMAP_INIT(&meter_tbl_instance->pipe_bmp, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&meter_tbl_instance->pipe_bmp, pipe_bmp);

  meter_tbl_instance->meter_tbl_stage_info =
      (pipe_mgr_meter_tbl_stage_info_t *)PIPE_MGR_CALLOC(
          meter_tbl_instance->num_stages,
          sizeof(pipe_mgr_meter_tbl_stage_info_t));

  if (meter_tbl_instance->meter_tbl_stage_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  unsigned i = 0;
  uint32_t offset = 0;

  /* During hitless HA all meter writes will be cached and applied during the
   * push-delta changes.  Setup a map to hold onto the specs. */
  bf_map_init(&meter_tbl_instance->replayed);

  for (i = 0; i < meter_tbl_instance->num_stages; i++) {
    status = pipe_mgr_meter_tbl_stage_init(
        &meter_tbl_instance->meter_tbl_stage_info[i],
        &meter_tbl_info->rmt_info[i]);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Dev %d error initializing meter table %s (0x%x) stage %d "
          "info, error %s",
          __func__,
          __LINE__,
          dev_info->dev_id,
          meter_tbl_info->name,
          meter_tbl_info->handle,
          meter_tbl_info->rmt_info[i].stage_id,
          pipe_str_err(status));
      return status;
    }

    meter_tbl_instance->meter_tbl_stage_info[i].ent_idx_offset = offset;
    offset += meter_tbl_instance->meter_tbl_stage_info[i].num_entries;

    /* The hardware (memories) must be initialized during the initial table
     * creation in device add.  If the tables are recreated later, say for a
     * symmetric mode change the hardware does not need to be re-initialized.
     * However, during hitless HA, even the initial table creation does not need
     * to touch the memories since they were initialized prior to the hitless ha
     * event. */
    if (hw_init && !pipe_mgr_is_device_virtual(dev_info->dev_id) &&
        !pipe_mgr_hitless_warm_init_in_progress(dev_info->dev_id)) {
      status = pipe_mgr_meter_mgr_init_mapram(
          sess_hdl,
          dev_info,
          meter_tbl_info,
          meter_tbl_instance->pipe_bmp,
          &meter_tbl_instance->meter_tbl_stage_info[i]);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initializing map ram for meter tbl 0x%x"
            " device id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            meter_tbl_info->handle,
            dev_info->dev_id,
            meter_tbl_info->rmt_info[i].stage_id,
            pipe_str_err(status));
        return status;
      }

      status = pipe_mgr_meter_mgr_init_color_mapram(
          sess_hdl,
          dev_info,
          meter_tbl_info,
          meter_tbl_instance->pipe_bmp,
          &meter_tbl_instance->meter_tbl_stage_info[i]);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initializing color map ram for meter tbl 0x%x"
            " device id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            meter_tbl_info->handle,
            dev_info->dev_id,
            meter_tbl_info->rmt_info[i].stage_id,
            pipe_str_err(status));
        return status;
      }
    }
  }

  if (meter_tbl_info->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    meter_tbl_instance->num_entries = meter_tbl_info->size;
  } else {
    /* For direct addressed table, the number of entries should be the
     * "capacity" of the meter table.
     */
    meter_tbl_instance->num_entries = offset;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_tbl_stage_init(
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    rmt_tbl_info_t *rmt_info) {
  pipe_mgr_meter_ram_alloc_info_t *ram_alloc_info = NULL;

  meter_tbl_stage_info->stage_id = rmt_info->stage_id;
  meter_tbl_stage_info->stage_table_handle = rmt_info->handle;
  meter_tbl_stage_info->num_entries = rmt_info->num_entries;
  meter_tbl_stage_info->default_lower_huffman_bits =
      rmt_info->params.meter_params.default_lower_huffman_bits;
  meter_tbl_stage_info->num_alu_ids =
      rmt_info->params.meter_params.num_alu_indexes;
  for (int i = 0; i < meter_tbl_stage_info->num_alu_ids; i++)
    meter_tbl_stage_info->alu_ids[i] =
        rmt_info->params.meter_params.alu_indexes[i];

  PIPE_MGR_DBGCHK(rmt_info->num_tbl_banks == 1);

  meter_tbl_stage_info->ram_alloc_info =
      (pipe_mgr_meter_ram_alloc_info_t *)PIPE_MGR_CALLOC(
          1, sizeof(pipe_mgr_meter_ram_alloc_info_t));

  if (meter_tbl_stage_info->ram_alloc_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;
  ram_alloc_info->num_wide_word_blks = rmt_info->bank_map->num_tbl_word_blks;
  ram_alloc_info->tbl_word_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_CALLOC(
      ram_alloc_info->num_wide_word_blks, sizeof(rmt_tbl_word_blk_t));
  PIPE_MGR_MEMCPY(
      ram_alloc_info->tbl_word_blk,
      rmt_info->bank_map->tbl_word_blk,
      ram_alloc_info->num_wide_word_blks * sizeof(rmt_tbl_word_blk_t));
  meter_tbl_stage_info->num_spare_rams = rmt_info->num_spare_rams;
  if (meter_tbl_stage_info->num_spare_rams > 0)
    PIPE_MGR_MEMCPY(meter_tbl_stage_info->spare_rams,
                    rmt_info->spare_rams,
                    meter_tbl_stage_info->num_spare_rams * sizeof(mem_id_t));

  /* A couple of sanity checks, the number of spare rams should equal the number
   * of ALUs.  The number of non-spare RAMs should equal num_entries divided by
   * 1024 (the number of meter entries per RAM). */
  if (meter_tbl_stage_info->num_spare_rams !=
      meter_tbl_stage_info->num_alu_ids) {
    PIPE_MGR_DBGCHK(meter_tbl_stage_info->num_spare_rams ==
                    meter_tbl_stage_info->num_alu_ids);
    return PIPE_UNEXPECTED;
  }
  if (ram_alloc_info->num_wide_word_blks * 1024 !=
      meter_tbl_stage_info->num_entries) {
    PIPE_MGR_DBGCHK(ram_alloc_info->num_wide_word_blks * 1024 ==
                    meter_tbl_stage_info->num_entries);
    return PIPE_UNEXPECTED;
  }

  if (rmt_info->color_bank_map) {
    /* The color bank map info will be present only for regular meters and not
     * for LPF/WRED
     */
    ram_alloc_info->num_color_wide_word_blks =
        rmt_info->color_bank_map->num_tbl_word_blks;
    ram_alloc_info->color_tbl_word_blk = (rmt_tbl_word_blk_t *)PIPE_MGR_CALLOC(
        ram_alloc_info->num_color_wide_word_blks, sizeof(rmt_tbl_word_blk_t));
    PIPE_MGR_MEMCPY(
        ram_alloc_info->color_tbl_word_blk,
        rmt_info->color_bank_map->tbl_word_blk,
        ram_alloc_info->num_color_wide_word_blks * sizeof(rmt_tbl_word_blk_t));
  }

  return PIPE_SUCCESS;
}

static pipe_status_t meter_tbl_set_dflt_val(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id,
                                            pipe_mgr_meter_tbl_t *meter_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  dev_target_t dt;
  dt.device_id = dev_id;
  for (unsigned i = 0; i < meter_tbl->num_tbl_instances; i++) {
    dt.dev_pipe_id = meter_tbl->meter_tbl_instances[i].pipe_id;
    struct pipe_mgr_meter_op_list_t *ml = NULL;
    if (meter_tbl->type == PIPE_METER_TYPE_STANDARD) {
      status =
          pipe_mgr_meter_mgr_meter_reset(dt, meter_tbl->meter_tbl_hdl, &ml);
    } else if (meter_tbl->type == PIPE_METER_TYPE_WRED) {
      status = pipe_mgr_meter_mgr_wred_reset(dt, meter_tbl->meter_tbl_hdl, &ml);
    } else if (meter_tbl->type == PIPE_METER_TYPE_LPF) {
      status = pipe_mgr_meter_mgr_lpf_reset(dt, meter_tbl->meter_tbl_hdl, &ml);
    } else {
      status = PIPE_UNEXPECTED;
      PIPE_MGR_DBGCHK(0);
    }
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Dev %d pipe %x meter tbl %s 0x%x, failed to init, err %s",
                dev_id,
                dt.dev_pipe_id,
                meter_tbl->name,
                meter_tbl->meter_tbl_hdl,
                pipe_str_err(status));
      return status;
    }
    uint32_t unused = 0;
    status = pipe_mgr_meter_process_op_list(
        sess_hdl, dev_id, meter_tbl->meter_tbl_hdl, ml, &unused);
    if (PIPE_SUCCESS != status) {
      LOG_ERROR("Dev %d pipe %x meter tbl %s 0x%x err %s processing init",
                dev_id,
                dt.dev_pipe_id,
                meter_tbl->name,
                meter_tbl->meter_tbl_hdl,
                pipe_str_err(status));
      return status;
    }
    pipe_mgr_meter_free_ops(&ml);
  }
  return status;
}

pipe_status_t pipe_mgr_meter_tbl_init(pipe_sess_hdl_t sess_hdl,
                                      bf_dev_id_t device_id,
                                      pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                      profile_id_t profile_id,
                                      pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t pipe_id = 0;
  rmt_dev_info_t *dev_info = NULL;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_meter_tbl_info_t *meter_tbl_info = NULL;
  pipe_bitmap_t local_pipe_bmp;
  unsigned long htbl_key = 0;
  bf_map_sts_t map_sts = BF_MAP_OK;
  uint32_t q = 0;

  dev_info = pipe_mgr_get_dev_info(device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d Device info for device id %d not found",
              __func__,
              __LINE__,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl_info =
      pipe_mgr_get_meter_tbl_info(device_id, meter_tbl_hdl, __func__, __LINE__);

  if (meter_tbl_info == NULL) {
    LOG_ERROR("%s:%d No information found for meter tbl %d, device id %d",
              __func__,
              __LINE__,
              meter_tbl_hdl,
              device_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* First check if a meter table already exists */
  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);
  if (meter_tbl != NULL) {
    LOG_ERROR(
        "%s:%d Attempt to initialize Meter table 0x%x on device %d which "
        "already exists",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        device_id);
    return PIPE_ALREADY_EXISTS;
  }

  meter_tbl =
      (pipe_mgr_meter_tbl_t *)PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_meter_tbl_t));

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  meter_tbl->dev_info = dev_info;
  meter_tbl->symmetric = meter_tbl_info->symmetric;
  meter_tbl->scope_pipe_bmp =
      PIPE_MGR_CALLOC(PIPE_BITMAP_COUNT(pipe_bmp), sizeof(scope_pipes_t));
  if (!meter_tbl->scope_pipe_bmp) {
    LOG_ERROR("%s:%d Malloc failed", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  if (meter_tbl->symmetric) {
    meter_tbl->num_scopes = 1;
    PIPE_BITMAP_ITER(pipe_bmp, q) { meter_tbl->scope_pipe_bmp[0] |= (1 << q); }
  } else {
    meter_tbl->num_scopes = 0;
    PIPE_BITMAP_ITER(pipe_bmp, q) {
      meter_tbl->scope_pipe_bmp[q] |= (1 << q);
      meter_tbl->num_scopes += 1;
    }
  }
  meter_tbl->name = bf_sys_strdup(meter_tbl_info->name);
  meter_tbl->direction = meter_tbl_info->direction;
  meter_tbl->meter_tbl_hdl = meter_tbl_info->handle;
  meter_tbl->num_entries = meter_tbl_info->size;
  meter_tbl->profile_id = profile_id;
  meter_tbl->enable_color_aware = meter_tbl_info->enable_color_aware;
  meter_tbl->enable_per_flow_enable = meter_tbl_info->enable_per_flow_enable;
  meter_tbl->per_flow_enable_bit_position =
      meter_tbl_info->per_flow_enable_bit_position;
  meter_tbl->enable_color_aware_per_flow_enable =
      meter_tbl_info->enable_color_aware_per_flow_enable;
  meter_tbl->color_aware_per_flow_enable_address_type_bit_position =
      meter_tbl_info->color_aware_per_flow_enable_address_type_bit_position;
  meter_tbl->ref_type = meter_tbl_info->ref_type;
  meter_tbl->type = meter_tbl_info->meter_type;

  if (meter_tbl->type == PIPE_METER_TYPE_STANDARD) {
    meter_tbl->meter_granularity = meter_tbl_info->meter_granularity;
  } else {
    meter_tbl->meter_granularity = PIPE_METER_GRANULARITY_INVALID;
  }

  if (meter_tbl->ref_type == PIPE_TBL_REF_TYPE_INDIRECT) {
    meter_tbl->over_allocated = true;
  } else {
    meter_tbl->over_allocated = false;
  }

  if (meter_tbl->symmetric) {
    PIPE_MGR_DBGCHK(meter_tbl->num_scopes == 1);
    meter_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(meter_tbl->scope_pipe_bmp[0]);
  }

  meter_tbl->num_tbl_instances = meter_tbl->num_scopes;

  meter_tbl->meter_tbl_instances =
      (pipe_mgr_meter_tbl_instance_t *)PIPE_MGR_CALLOC(
          meter_tbl->num_tbl_instances, sizeof(pipe_mgr_meter_tbl_instance_t));

  if (meter_tbl->meter_tbl_instances == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  unsigned i = 0;
  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    if (meter_tbl->symmetric == false) {
      pipe_id = pipe_mgr_get_lowest_pipe_in_scope(meter_tbl->scope_pipe_bmp[i]);
    } else {
      pipe_id = BF_DEV_PIPE_ALL;
    }
    PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(meter_tbl->scope_pipe_bmp[i],
                                    &local_pipe_bmp);
    PIPE_BITMAP_AND(&local_pipe_bmp, pipe_bmp);

    status =
        pipe_mgr_meter_tbl_instance_init(sess_hdl,
                                         dev_info,
                                         &meter_tbl->meter_tbl_instances[i],
                                         meter_tbl_info,
                                         pipe_id,
                                         &local_pipe_bmp,
                                         true);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing meter table instances, with handle %d"
          " device id %d",
          __func__,
          __LINE__,
          meter_tbl_hdl,
          device_id);
      return status;
    }
  }

  htbl_key = meter_tbl_hdl;

  /* Insert the table info into the hash map */
  map_sts = pipe_mgr_meter_tbl_map_add(device_id, htbl_key, (void *)meter_tbl);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in inserting the meter table info into the hash map"
        " meter tbl %d, device id %d",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        device_id);
    status = PIPE_UNEXPECTED;
    goto err_cleanup;
  }

  if (!pipe_mgr_is_device_virtual(device_id)) {
    /* Init some shadow memory stuff */
    status = pipe_mgr_meter_init_shadow_mem(meter_tbl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error initing shadow memory for tbl 0x%x device id %d err %s",
          __func__,
          __LINE__,
          meter_tbl->meter_tbl_hdl,
          meter_tbl->dev_info->dev_id,
          pipe_str_err(status));
      return status;
    }

    /* Set the default value in the table for non-hitless HA cases.  For hitless
     * the table will be updated during the compute/push-delta steps.  */
    if (!pipe_mgr_hitless_warm_init_in_progress(device_id)) {
      status = meter_tbl_set_dflt_val(sess_hdl, device_id, meter_tbl);
      if (PIPE_SUCCESS != status) {
        goto err_cleanup;
      }
    }
  }

err_cleanup:
  if (status != PIPE_SUCCESS) {
    pipe_mgr_meter_mgr_tbl_delete(device_id, meter_tbl_hdl, meter_tbl);
    if (meter_tbl) PIPE_MGR_FREE(meter_tbl);
  }

  return status;
}

void pipe_mgr_meter_tbl_delete_stage(
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info) {
  if (meter_tbl_stage_info == NULL) {
    return;
  }

  if (meter_tbl_stage_info->ram_alloc_info) {
    if (meter_tbl_stage_info->ram_alloc_info->tbl_word_blk) {
      PIPE_MGR_FREE(meter_tbl_stage_info->ram_alloc_info->tbl_word_blk);
    }
    if (meter_tbl_stage_info->ram_alloc_info->color_tbl_word_blk) {
      PIPE_MGR_FREE(meter_tbl_stage_info->ram_alloc_info->color_tbl_word_blk);
    }
    PIPE_MGR_FREE(meter_tbl_stage_info->ram_alloc_info);
  }

  return;
}

void pipe_mgr_meter_tbl_delete_instance(
    pipe_mgr_meter_tbl_instance_t *meter_tbl_instance) {
  if (meter_tbl_instance == NULL) {
    return;
  }

  unsigned i = 0;
  for (i = 0; i < meter_tbl_instance->num_stages; i++) {
    pipe_mgr_meter_tbl_delete_stage(
        &meter_tbl_instance->meter_tbl_stage_info[i]);
  }

  if (meter_tbl_instance->meter_tbl_stage_info) {
    PIPE_MGR_FREE(meter_tbl_instance->meter_tbl_stage_info);
    unsigned long unused;
    void *spec;
    while (BF_MAP_OK == bf_map_get_first_rmv(
                            &meter_tbl_instance->replayed, &unused, &spec)) {
      PIPE_MGR_FREE(spec);
    }
    bf_map_destroy(&meter_tbl_instance->replayed);
  }
}

void pipe_mgr_meter_mgr_tbl_cleanup(bf_dev_id_t device_id,
                                    pipe_meter_tbl_hdl_t meter_tbl_hdl) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);

  if (meter_tbl == NULL) {
    return;
  }

  pipe_mgr_meter_mgr_tbl_delete(device_id, meter_tbl_hdl, meter_tbl);

  PIPE_MGR_FREE(meter_tbl);
}

void pipe_mgr_meter_mgr_tbl_delete(bf_dev_id_t device_id,
                                   pipe_meter_tbl_hdl_t meter_tbl_hdl,
                                   pipe_mgr_meter_tbl_t *tbl) {
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  bf_map_sts_t map_sts = BF_MAP_OK;
  unsigned long htbl_key = meter_tbl_hdl;

  if (tbl == NULL) {
    meter_tbl = pipe_mgr_meter_tbl_get(device_id, meter_tbl_hdl);
    if (meter_tbl == NULL) {
      /* Nothing to do */
      LOG_ERROR(
          "%s:%d Attempt to delete a non-existent meter table, handle %d"
          " device id %d",
          __func__,
          __LINE__,
          meter_tbl_hdl,
          device_id);
      return;
    }
  } else {
    meter_tbl = tbl;
  }

  unsigned i = 0;
  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    pipe_mgr_meter_tbl_delete_instance(&meter_tbl->meter_tbl_instances[i]);
  }

  if (meter_tbl->meter_tbl_instances) {
    PIPE_MGR_FREE(meter_tbl->meter_tbl_instances);
    meter_tbl->meter_tbl_instances = NULL;
  }

  if (meter_tbl->scope_pipe_bmp) {
    PIPE_MGR_FREE(meter_tbl->scope_pipe_bmp);
    meter_tbl->scope_pipe_bmp = NULL;
  }

  if (meter_tbl->name) {
    PIPE_MGR_FREE(meter_tbl->name);
    meter_tbl->name = NULL;
  }

  map_sts = pipe_mgr_meter_tbl_map_rmv(device_id, htbl_key);

  if (map_sts != BF_MAP_OK) {
    LOG_ERROR(
        "%s:%d Error in removing meter table with handle %d, device id %d"
        " from the hash map",
        __func__,
        __LINE__,
        meter_tbl_hdl,
        device_id);
  }

  return;
}

pipe_status_t pipe_mgr_meter_tbl_get_symmetric_mode(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t tbl_hdl,
    bool *symmetric,
    scope_num_t *num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_mgr_meter_tbl_t *tbl = pipe_mgr_meter_tbl_get(dev_id, tbl_hdl);
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

pipe_status_t pipe_mgr_meter_tbl_set_symmetric_mode(
    pipe_sess_hdl_t sess_hdl,
    bf_dev_id_t device_id,
    pipe_meter_tbl_hdl_t tbl_hdl,
    bool symmetric,
    scope_num_t num_scopes,
    scope_pipes_t *scope_pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;
  pipe_meter_tbl_info_t *meter_tbl_info = NULL;
  rmt_dev_profile_info_t *profile = NULL;
  pipe_bitmap_t pipe_bmp;
  bf_dev_pipe_t pipe_id;

  meter_tbl_info =
      pipe_mgr_get_meter_tbl_info(device_id, tbl_hdl, __func__, __LINE__);

  if (meter_tbl_info == NULL) {
    LOG_ERROR(
        "%s:%d Error in getting meter table info for device id %d"
        " table handle 0x%x",
        __func__,
        __LINE__,
        device_id,
        tbl_hdl);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl = pipe_mgr_meter_tbl_get(device_id, tbl_hdl);

  if (meter_tbl == NULL) {
    LOG_ERROR("%s:%d Meter table for device id %d, tbl hdl 0x%x not found",
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
                                       meter_tbl->symmetric,
                                       meter_tbl->num_scopes,
                                       &meter_tbl->scope_pipe_bmp[0])) {
    LOG_TRACE("%s: Table %s, No change to symmetric mode %d, Num-scopes %d ",
              __func__,
              meter_tbl->name,
              meter_tbl->symmetric,
              meter_tbl->num_scopes);
    return PIPE_SUCCESS;
  }

  meter_tbl->symmetric = symmetric;
  /* Copy the new scope info */
  meter_tbl->num_scopes = num_scopes;
  PIPE_MGR_MEMCPY(meter_tbl->scope_pipe_bmp,
                  scope_pipe_bmp,
                  num_scopes * sizeof(scope_pipes_t));

  /* Delete the existing meter table instances, since we will be creating the
   * required number of new instances as per the symmetricity of the table.
   */
  unsigned i = 0;
  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    pipe_mgr_meter_tbl_delete_instance(&meter_tbl->meter_tbl_instances[i]);
  }

  if (meter_tbl->meter_tbl_instances) {
    PIPE_MGR_FREE(meter_tbl->meter_tbl_instances);
    meter_tbl->meter_tbl_instances = NULL;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(device_id);
  if (dev_info == NULL) {
    LOG_ERROR("%s:%d No device info found for device id %d",
              __func__,
              __LINE__,
              device_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  profile =
      pipe_mgr_get_profile(dev_info, meter_tbl->profile_id, __func__, __LINE__);

  if (profile == NULL) {
    LOG_ERROR("%s:%d Profile info not found for device id %d, profile %d",
              __func__,
              __LINE__,
              device_id,
              meter_tbl->profile_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  meter_tbl->num_tbl_instances = meter_tbl->num_scopes;
  if (meter_tbl->symmetric == true) {
    PIPE_MGR_DBGCHK(meter_tbl->num_scopes == 1);
    meter_tbl->lowest_pipe_id =
        pipe_mgr_get_lowest_pipe_in_scope(meter_tbl->scope_pipe_bmp[0]);
  }

  meter_tbl->meter_tbl_instances =
      (pipe_mgr_meter_tbl_instance_t *)PIPE_MGR_CALLOC(
          meter_tbl->num_tbl_instances, sizeof(pipe_mgr_meter_tbl_instance_t));

  if (meter_tbl->meter_tbl_instances == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    if (meter_tbl->symmetric == false) {
      pipe_id = pipe_mgr_get_lowest_pipe_in_scope(meter_tbl->scope_pipe_bmp[i]);
    } else {
      pipe_id = BF_DEV_PIPE_ALL;
    }
    PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
    pipe_mgr_convert_scope_pipe_bmp(meter_tbl->scope_pipe_bmp[i], &pipe_bmp);

    status =
        pipe_mgr_meter_tbl_instance_init(sess_hdl,
                                         meter_tbl->dev_info,
                                         &meter_tbl->meter_tbl_instances[i],
                                         meter_tbl_info,
                                         pipe_id,
                                         &pipe_bmp,
                                         false);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initializing meter table instance for device id %d"
          " table handle 0x%x",
          __func__,
          __LINE__,
          device_id,
          tbl_hdl);
      return PIPE_OBJ_NOT_FOUND;
    }
  }

  if (!pipe_mgr_is_device_virtual(device_id)) {
    /* Re-init the shadow memory metadata since the symmetricity of the table
     * has changed.
     */
    status = pipe_mgr_meter_init_shadow_mem(meter_tbl);
    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing shadow memory for tbl 0x%x device id %d"
          " err %s",
          __func__,
          __LINE__,
          meter_tbl->meter_tbl_hdl,
          meter_tbl->dev_info->dev_id,
          pipe_str_err(status));
      return status;
    }

    /* Set the default value in the table for non-hitless HA cases.  For hitless
     * the table will be updated during the compute/push-delta steps.  */
    if (!pipe_mgr_hitless_warm_init_in_progress(device_id)) {
      status = meter_tbl_set_dflt_val(sess_hdl, device_id, meter_tbl);
      if (PIPE_SUCCESS != status) {
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_init_shadow_mem(pipe_mgr_meter_tbl_t *meter_tbl) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_instance_t *meter_tbl_instance = NULL;
  pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info = NULL;

  unsigned i = 0;
  unsigned j = 0;

  for (i = 0; i < meter_tbl->num_tbl_instances; i++) {
    meter_tbl_instance = &meter_tbl->meter_tbl_instances[i];
    for (j = 0; j < meter_tbl_instance->num_stages; j++) {
      meter_tbl_stage_info = &meter_tbl_instance->meter_tbl_stage_info[j];
      status = pipe_mgr_meter_stage_init_shadow_mem(
          meter_tbl, meter_tbl_stage_info, &meter_tbl_instance->pipe_bmp);
      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in initing meter stage shadow mem metadata"
            " for tbl 0x%x, dev id %d, stage id %d, err %s",
            __func__,
            __LINE__,
            meter_tbl->meter_tbl_hdl,
            meter_tbl->dev_info->dev_id,
            meter_tbl_stage_info->stage_id,
            pipe_str_err(status));
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_meter_stage_init_shadow_mem(
    pipe_mgr_meter_tbl_t *meter_tbl,
    pipe_mgr_meter_tbl_stage_info_t *meter_tbl_stage_info,
    pipe_bitmap_t *pipe_bmp) {
  pipe_status_t status = PIPE_SUCCESS;
  pipe_mgr_meter_ram_alloc_info_t *meter_ram_alloc_info = NULL;
  unsigned i = 0;
  meter_ram_alloc_info = meter_tbl_stage_info->ram_alloc_info;

  for (i = 0; i < meter_ram_alloc_info->num_wide_word_blks; i++) {
    status = pipe_mgr_phy_mem_map_symmetric_mode_set(
        meter_tbl->dev_info->dev_id,
        meter_tbl->direction,
        pipe_bmp,
        meter_tbl_stage_info->stage_id,
        pipe_mem_type_unit_ram,
        meter_ram_alloc_info->tbl_word_blk[i].mem_id[0],
        meter_tbl->symmetric);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing meter shadow mem metadata for tbl 0x%x dev "
          "id %d, stage id %d, ram id %d err %s",
          __func__,
          __LINE__,
          meter_tbl->meter_tbl_hdl,
          meter_tbl->dev_info->dev_id,
          meter_tbl_stage_info->stage_id,
          meter_ram_alloc_info->tbl_word_blk[i].mem_id[0],
          pipe_str_err(status));
      return status;
    }
  }
  for (i = 0; i < meter_tbl_stage_info->num_spare_rams; i++) {
    status = pipe_mgr_phy_mem_map_symmetric_mode_set(
        meter_tbl->dev_info->dev_id,
        meter_tbl->direction,
        pipe_bmp,
        meter_tbl_stage_info->stage_id,
        pipe_mem_type_unit_ram,
        meter_tbl_stage_info->spare_rams[i],
        meter_tbl->symmetric);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in initing meter shadow mem metadata for tbl 0x%x dev "
          "id %d, stage id %d, ram id %d err %s",
          __func__,
          __LINE__,
          meter_tbl->meter_tbl_hdl,
          meter_tbl->dev_info->dev_id,
          meter_tbl_stage_info->stage_id,
          meter_tbl_stage_info->spare_rams[i],
          pipe_str_err(status));
    }
  }

  return status;
}

pipe_status_t pipe_mgr_meter_sweep_control(pipe_sess_hdl_t sess_hdl,
                                           rmt_dev_info_t *dev_info,
                                           bool enable) {
  pipe_status_t sts = PIPE_SUCCESS;
  pipe_mgr_meter_tbl_t *meter_tbl = NULL;

  /* Go through all meter tables on the device and enable sweeps in each pipe
   * the table is present in. */
  meter_tbl = pipe_mgr_meter_tbl_get_first(dev_info->dev_id);
  for (; meter_tbl; meter_tbl = pipe_mgr_meter_tbl_get_next(meter_tbl)) {
    /* Get the pipe bitmap and config value from the profile. */
    pipe_bitmap_t *pbm =
        &dev_info->profile_info[meter_tbl->profile_id]->pipe_bmp;
    struct pipe_config_cache_meter_sweep_t *cc = NULL;
    bf_map_sts_t msts =
        bf_map_get(&dev_info->profile_info[meter_tbl->profile_id]->config_cache,
                   pipe_cck_meter_sweep_ctrl,
                   (void **)&cc);
    if (BF_MAP_OK != msts || !cc) {
      PIPE_MGR_DBGCHK(cc);
      continue;
    }
    /* Program in each stage. */
    pipe_mgr_meter_tbl_instance_t *first_inst =
        &meter_tbl->meter_tbl_instances[0];
    for (uint8_t i = 0; i < first_inst->num_stages; ++i) {
      int stage_id = first_inst->meter_tbl_stage_info[i].stage_id;
      for (uint8_t j = 0; j < first_inst->meter_tbl_stage_info[i].num_alu_ids;
           j++) {
        int alu_id = first_inst->meter_tbl_stage_info[i].alu_ids[j];
        uint32_t val = cc->val[stage_id][alu_id];
        pipe_instr_write_reg_t instr;
        pipe_status_t rc;
        switch (dev_info->dev_family) {
          case BF_DEV_FAMILY_TOFINO:
            setp_meter_sweep_ctl_meter_sweep_en(&val, enable);
            construct_instr_reg_write(
                dev_info->dev_id,
                &instr,
                offsetof(Tofino,
                         pipes[0]
                             .mau[stage_id]
                             .rams.match.adrdist.meter_sweep_ctl[alu_id]),
                val);
            rc = pipe_mgr_drv_ilist_add(&sess_hdl,
                                        dev_info,
                                        pbm,
                                        stage_id,
                                        (uint8_t *)&instr,
                                        sizeof instr);
            if (PIPE_SUCCESS != rc) {
              LOG_ERROR(
                  "Failed to post meter sweep en instruction, dev %d, table "
                  "0x%x, stage %d, %s",
                  dev_info->dev_id,
                  meter_tbl->meter_tbl_hdl,
                  stage_id,
                  pipe_str_err(rc));
              sts = rc;
            }
            break;
          case BF_DEV_FAMILY_TOFINO2:
            setp_tof2_meter_sweep_ctl_meter_sweep_en(&val, enable);
            construct_instr_reg_write(
                dev_info->dev_id,
                &instr,
                offsetof(tof2_reg,
                         pipes[0]
                             .mau[stage_id]
                             .rams.match.adrdist.meter_sweep_ctl[alu_id]),
                val);
            rc = pipe_mgr_drv_ilist_add(&sess_hdl,
                                        dev_info,
                                        pbm,
                                        stage_id,
                                        (uint8_t *)&instr,
                                        sizeof instr);
            if (PIPE_SUCCESS != rc) {
              LOG_ERROR(
                  "Failed to post meter sweep en instruction, dev %d, table "
                  "0x%x, stage %d, %s",
                  dev_info->dev_id,
                  meter_tbl->meter_tbl_hdl,
                  stage_id,
                  pipe_str_err(rc));
              sts = rc;
            }
            break;
          case BF_DEV_FAMILY_TOFINO3:
            setp_tof3_meter_sweep_ctl_meter_sweep_en(&val, enable);
            construct_instr_reg_write(
                dev_info->dev_id,
                &instr,
                offsetof(tof3_reg,
                         pipes[0]
                             .mau[stage_id]
                             .rams.match.adrdist.meter_sweep_ctl[alu_id]),
                val);
            rc = pipe_mgr_drv_ilist_add(&sess_hdl,
                                        dev_info,
                                        pbm,
                                        stage_id,
                                        (uint8_t *)&instr,
                                        sizeof instr);
            if (PIPE_SUCCESS != rc) {
              LOG_ERROR(
                  "Failed to post meter sweep en instruction, dev %d, table "
                  "0x%x, stage %d, %s",
                  dev_info->dev_id,
                  meter_tbl->meter_tbl_hdl,
                  stage_id,
                  pipe_str_err(rc));
              sts = rc;
            }
            break;
          default:
            PIPE_MGR_DBGCHK(0);
            return PIPE_UNEXPECTED;
        }
      }
    }
  }
  return sts;
}
