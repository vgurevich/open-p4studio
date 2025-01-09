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


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_tof_db.h"
#include "pipe_mgr_tof2_db.h"
#include "pipe_mgr_tof3_db.h"
#include "pipe_mgr_phy_mem_map.h"
#include "pipe_mgr_tcam.h"
#include "pipe_mgr_idle.h"

pipe_mgr_db_t *pipe_db[PIPE_MGR_NUM_DEVICES];
pipe_mgr_sel_shadow_db_t *pipe_sel_shadow_db[PIPE_MGR_NUM_DEVICES];

extern const char *pipe_mgr_mem_type2str(pipe_mem_type_t mem_type);

pipe_status_t pipe_mgr_prsr_db_init(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_status_t sts = PIPE_SUCCESS;
  bool db_init = false;
  int dev_pipes = dev_info->dev_cfg.num_pipelines;
  if (!pipe_db[dev]) {
    pipe_db[dev] = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_db_t));
    if (!pipe_db[dev]) return PIPE_NO_SYS_RESOURCES;
    db_init = true;
  }
  pipe_db[dev]->prsr_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->prsr_db));
  if (!pipe_db[dev]->prsr_db) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (int p = 0; p < dev_pipes; ++p) {
    // One map per gress (2) per pipe.
    pipe_db[dev]->prsr_db[p] =
        PIPE_MGR_CALLOC(2, sizeof(*pipe_db[dev]->prsr_db[p]));
    if (!pipe_db[dev]->prsr_db[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    for (int g = 0; g < 2; g++) {
      bf_map_init(&(pipe_db[dev]->prsr_db[p][g]));
    }
  }
  pipe_db[dev]->prsr_base_addr =
      PIPE_MGR_CALLOC(PIPE_DIR_MAX, sizeof(*pipe_db[dev]->prsr_base_addr));
  // init base_address
  /* All for pipe0 parser 0 */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_prsr_db_init(dev);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_prsr_db_init(dev);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_prsr_db_init(dev);
      break;





    default:
      LOG_ERROR("%s: Invalid dev_family %d", __func__, dev_info->dev_family);
      return PIPE_INVALID_ARG;
  }

  return PIPE_SUCCESS;
cleanup:
  if (pipe_db[dev]) {
    for (int p = 0; p < dev_pipes; ++p) {
      if (pipe_db[dev]->prsr_db[p]) PIPE_MGR_FREE(pipe_db[dev]->prsr_db[p]);
    }
    if (pipe_db[dev]->prsr_db) PIPE_MGR_FREE(pipe_db[dev]->prsr_db);
    if (pipe_db[dev]->prsr_base_addr)
      PIPE_MGR_FREE(pipe_db[dev]->prsr_base_addr);
    if (db_init) {
      PIPE_MGR_FREE(pipe_db[dev]);
      pipe_db[dev] = NULL;
    }
  }
  return sts;
}

/* Interrupt DB init */
pipe_status_t pipe_mgr_db_init(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_status_t sts = PIPE_SUCCESS;
  bool db_init = false;
  int dev_pipes = dev_info->dev_cfg.num_pipelines;
  int dev_stages = dev_info->num_active_mau;
  int dev_prsrs = dev_info->dev_cfg.num_prsrs;
  if (!pipe_db[dev]) {
    pipe_db[dev] = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_db_t));
    if (!pipe_db[dev]) return PIPE_NO_SYS_RESOURCES;
    db_init = true;
  }
  pipe_db[dev]->imem_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->imem_db));
  pipe_db[dev]->gfm_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->gfm_db));
  pipe_db[dev]->map_ram =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->map_ram));
  pipe_db[dev]->prsr_tcam_db =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->prsr_tcam_db));
  if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
      (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
    pipe_db[dev]->mirrtbl =
        PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->mirrtbl));
    if (!pipe_db[dev]->mirrtbl) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    // tof2 prsr group
    dev_prsrs /= 4;
  }
  pipe_db[dev]->tbl_hdl =
      PIPE_MGR_CALLOC(dev_pipes, sizeof(*pipe_db[dev]->tbl_hdl));
  if (!pipe_db[dev]->imem_db || !pipe_db[dev]->map_ram ||
      !pipe_db[dev]->tbl_hdl || !pipe_db[dev]->gfm_db ||
      !pipe_db[dev]->prsr_tcam_db) {
    sts = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  int p;
  for (p = 0; p < dev_pipes; ++p) {
    pipe_db[dev]->imem_db[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*pipe_db[dev]->imem_db[p]));
    if (!pipe_db[dev]->imem_db[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pipe_db[dev]->gfm_db[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*pipe_db[dev]->gfm_db[p]));
    if (!pipe_db[dev]->gfm_db[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pipe_db[dev]->map_ram[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*pipe_db[dev]->map_ram[p]));
    if (!pipe_db[dev]->map_ram[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pipe_db[dev]->tbl_hdl[p] =
        PIPE_MGR_CALLOC(dev_stages, sizeof(*pipe_db[dev]->tbl_hdl[p]));
    if (!pipe_db[dev]->tbl_hdl[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pipe_db[dev]->prsr_tcam_db[p] =
        PIPE_MGR_CALLOC(dev_prsrs, sizeof(*pipe_db[dev]->prsr_tcam_db[p]));
    if (!pipe_db[dev]->prsr_tcam_db[p]) {
      sts = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_DB_DATA(dev).dev = dev;
  PIPE_BITMAP_INIT(&(PIPE_DB_DATA(dev).gfm_hash_parity_enabled), PIPE_BMP_SIZE);

  /* Save the base address of the imem sections in each pipe and MAU. */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_interrupt_db_init(dev_info);
      if (PIPE_SUCCESS != sts) goto cleanup;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_interrupt_db_init(dev_info);
      if (PIPE_SUCCESS != sts) goto cleanup;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_interrupt_db_init(dev_info);
      if (PIPE_SUCCESS != sts) goto cleanup;
      break;





    default:
      PIPE_MGR_DBGCHK(0);
      sts = PIPE_UNEXPECTED;
      goto cleanup;
  }
  return PIPE_SUCCESS;
cleanup:
  if (pipe_db[dev]) {
    for (p = 0; p < dev_pipes; ++p) {
      if (pipe_db[dev]->imem_db && pipe_db[dev]->imem_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->imem_db[p]);
      if (pipe_db[dev]->gfm_db && pipe_db[dev]->gfm_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->gfm_db[p]);
      if (pipe_db[dev]->map_ram && pipe_db[dev]->map_ram[p])
        PIPE_MGR_FREE(pipe_db[dev]->map_ram[p]);
      if (pipe_db[dev]->tbl_hdl && pipe_db[dev]->tbl_hdl[p])
        PIPE_MGR_FREE(pipe_db[dev]->tbl_hdl[p]);
      if (pipe_db[dev]->prsr_db && pipe_db[dev]->prsr_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->prsr_db[p]);
      if (pipe_db[dev]->prsr_tcam_db && pipe_db[dev]->prsr_tcam_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->prsr_tcam_db[p]);
    }
    if (pipe_db[dev]->imem_db) PIPE_MGR_FREE(pipe_db[dev]->imem_db);
    if (pipe_db[dev]->gfm_db) PIPE_MGR_FREE(pipe_db[dev]->gfm_db);
    if (pipe_db[dev]->map_ram) PIPE_MGR_FREE(pipe_db[dev]->map_ram);
    if (pipe_db[dev]->mirrtbl) PIPE_MGR_FREE(pipe_db[dev]->mirrtbl);
    if (pipe_db[dev]->tbl_hdl) PIPE_MGR_FREE(pipe_db[dev]->tbl_hdl);
    if (pipe_db[dev]->prsr_db) PIPE_MGR_FREE(pipe_db[dev]->prsr_db);
    if (pipe_db[dev]->prsr_tcam_db) PIPE_MGR_FREE(pipe_db[dev]->prsr_tcam_db);
    if (db_init) {
      PIPE_MGR_FREE(pipe_db[dev]);
      pipe_db[dev] = NULL;
    }
  }
  return sts;
}

void pipe_mgr_db_cleanup(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (dev_info == NULL) return;
  if (pipe_db[dev]) {
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        pipe_mgr_tof_interrupt_db_cleanup(dev_info);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        pipe_mgr_tof2_interrupt_db_cleanup(dev_info);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        pipe_mgr_tof3_interrupt_db_cleanup(dev_info);
        break;
      default:
        PIPE_MGR_DBGCHK(0);
        return;
    }
    int dev_pipes = dev_info->num_active_pipes;
    for (int p = 0; p < dev_pipes; ++p) {
      for (int g = 0; g < 2; g++) {
        // Walk through prsr_db map, free pvs_hdl inside the node and node
        // itself
        bf_map_sts_t sts;
        struct pipe_mgr_prsr_instance_t *instance_info;
        unsigned long key;
        while (
            (sts = bf_map_get_first_rmv(
                 &PIPE_PRSR_DATA(dev, p, g), &key, (void **)&instance_info)) ==
            BF_MAP_OK) {
          if (instance_info != NULL) {
            if (instance_info->pvs_hdl != NULL) {
              PIPE_MGR_FREE(instance_info->pvs_hdl);
            }
            if (instance_info->name != NULL) {
              PIPE_MGR_FREE(instance_info->name);
            }
            PIPE_MGR_FREE(instance_info);
          }
        }
        bf_map_destroy(&PIPE_PRSR_DATA(dev, p, g));
      }
      if (pipe_db[dev]->prsr_db && pipe_db[dev]->prsr_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->prsr_db[p]);
      if (pipe_db[dev]->map_ram && pipe_db[dev]->map_ram[p])
        PIPE_MGR_FREE(pipe_db[dev]->map_ram[p]);
      if (pipe_db[dev]->tbl_hdl && pipe_db[dev]->tbl_hdl[p])
        PIPE_MGR_FREE(pipe_db[dev]->tbl_hdl[p]);
      if (pipe_db[dev]->gfm_db && pipe_db[dev]->gfm_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->gfm_db[p]);
      if (pipe_db[dev]->prsr_tcam_db && pipe_db[dev]->prsr_tcam_db[p])
        PIPE_MGR_FREE(pipe_db[dev]->prsr_tcam_db[p]);
    }
    if (pipe_db[dev]->imem_db) PIPE_MGR_FREE(pipe_db[dev]->imem_db);
    if (pipe_db[dev]->gfm_db) PIPE_MGR_FREE(pipe_db[dev]->gfm_db);
    if (pipe_db[dev]->map_ram) PIPE_MGR_FREE(pipe_db[dev]->map_ram);
    if (pipe_db[dev]->prsr_db) PIPE_MGR_FREE(pipe_db[dev]->prsr_db);
    if (pipe_db[dev]->mirrtbl) PIPE_MGR_FREE(pipe_db[dev]->mirrtbl);
    if (pipe_db[dev]->tbl_hdl) PIPE_MGR_FREE(pipe_db[dev]->tbl_hdl);
    if (pipe_db[dev]->prsr_tcam_db) PIPE_MGR_FREE(pipe_db[dev]->prsr_tcam_db);
    if (pipe_db[dev]->prsr_base_addr)
      PIPE_MGR_FREE(pipe_db[dev]->prsr_base_addr);
    PIPE_MGR_FREE(pipe_db[dev]);
    pipe_db[dev] = NULL;
  }
}

/* Set the GFM hash parity enabled */
pipe_status_t pipe_mgr_set_gfm_hash_parity_enable(bf_dev_id_t dev,
                                                  pipe_bitmap_t *pipe_bmp,
                                                  bool enabled) {
  bf_dev_pipe_t pipe = 0;

  PIPE_BITMAP_ITER(pipe_bmp, pipe) {
    if (enabled) {
      PIPE_BITMAP_SET(&(PIPE_DB_DATA(dev).gfm_hash_parity_enabled), pipe);
    } else {
      PIPE_BITMAP_CLR(&(PIPE_DB_DATA(dev).gfm_hash_parity_enabled), pipe);
    }
  }
  return PIPE_SUCCESS;
}

/* Recalculate the GFM parity bit */
pipe_status_t pipe_mgr_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
                                               rmt_dev_info_t *dev_info,
                                               pipe_bitmap_t *pipe_bmp,
                                               dev_stage_t stage,
                                               bool skip_write) {
  pipe_bitmap_t pipes_enabled;

  /* If parity check is not enabled by compiler: Then the 51st bit has not
     been reserved for parity, do not recalculate parity or write it.
  */
  PIPE_BITMAP_INIT(&pipes_enabled, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&pipes_enabled, pipe_bmp);
  PIPE_BITMAP_AND(&pipes_enabled,
                  &(PIPE_DB_DATA(dev_info->dev_id).gfm_hash_parity_enabled));

  if (PIPE_BITMAP_COUNT(&pipes_enabled) == 0) {
    return PIPE_SUCCESS;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_recalc_write_gfm_parity(
          sess_hdl, dev_info, &pipes_enabled, stage, skip_write);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_recalc_write_gfm_parity(
          sess_hdl, dev_info, &pipes_enabled, stage, skip_write);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_recalc_write_gfm_parity(
          sess_hdl, dev_info, &pipes_enabled, stage, skip_write);

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

/* Recalculate the seed */
pipe_status_t pipe_mgr_recalc_write_seed_parity(pipe_sess_hdl_t sess_hdl,
                                                rmt_dev_info_t *dev_info,
                                                pipe_bitmap_t *pipe_bmp,
                                                dev_stage_t stage) {
  pipe_bitmap_t pipes_enabled;

  /* If parity check is not enabled by compiler: Then the 51st bit has not
     been reserved for parity, do not recalculate parity or write it.
  */
  PIPE_BITMAP_INIT(&pipes_enabled, PIPE_BMP_SIZE);
  PIPE_BITMAP_ASSIGN(&pipes_enabled, pipe_bmp);
  PIPE_BITMAP_AND(&pipes_enabled,
                  &(PIPE_DB_DATA(dev_info->dev_id).gfm_hash_parity_enabled));

  if (PIPE_BITMAP_COUNT(&pipes_enabled) == 0) {
    return PIPE_SUCCESS;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_recalc_write_seed(
          sess_hdl, dev_info, &pipes_enabled, stage);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_recalc_write_seed(
          sess_hdl, dev_info, &pipes_enabled, stage);
    case BF_DEV_FAMILY_TOFINO3:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

/* Write GFM block from shadow */
pipe_status_t pipe_mgr_write_gfm_from_shadow(pipe_sess_hdl_t shdl,
                                             bf_dev_id_t dev,
                                             bf_dev_pipe_t log_pipe,
                                             dev_stage_t stage,
                                             int grp,
                                             bool ingress) {
  {
    bf_dev_pipe_t phy_pipe = 0;
    pipe_status_t status = PIPE_SUCCESS;
    pipe_bitmap_t pipe_bmp;
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
    if (!dev_info) {
      PIPE_MGR_DBGCHK(dev_info);
      return PIPE_UNEXPECTED;
    }
    (void)grp;
    (void)ingress;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    /* Recalculate the GFM parity again to make sure it is current
       Do not write parity right now, we will do block write later
    */
    PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
    PIPE_BITMAP_SET(&pipe_bmp, log_pipe);
    pipe_mgr_recalc_write_gfm_parity(shdl, dev_info, &pipe_bmp, stage, true);

    /* Write the entire GFM block as it is not possible to determine the hash
       functions in a group. Compiler does not publish this information.
    */
    status = pipe_mgr_api_enter(shdl);
    /* Write the seed as an ilist add only since it is going to be
     * only 1 ilist msg as it is one pipe, one stage
     */
    pipe_mgr_recalc_write_seed_parity(shdl, dev_info, &pipe_bmp, stage);
    if (PIPE_SUCCESS != status) return status;

    int bwr_size = pipe_mgr_drv_buf_size(dev, PIPE_MGR_DRV_BUF_BWR);
    pipe_mgr_drv_ses_state_t *st =
        pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
    if (NULL == st) {
      return PIPE_INVALID_ARG;
    }

    int entry_width = 4;
    int log_pipe_mask = 1 << log_pipe;
    pipe_mgr_drv_buf_t *b = NULL;
    /* Allocate a DMA buffer, fill it with the gfm data. Then issue a block
       write to correct the gfm contents.
    */
    b = pipe_mgr_drv_buf_alloc(
        st->sid, dev, bwr_size, PIPE_MGR_DRV_BUF_BWR, true);
    if (!b) {
      pipe_mgr_api_exit(shdl);
      LOG_ERROR("Failed to correct GFM error, dev %d pipe %d stage %d",
                dev,
                log_pipe,
                stage);
      return PIPE_NO_SYS_RESOURCES;
    }
    uint32_t pcie_addr = PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.base_addr;
    uint8_t *data_ptr =
        (uint8_t *)&(PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data[0][0]);
    int data_len = PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data_len;
    PIPE_MGR_MEMCPY(b->addr, data_ptr, data_len);
    int num_entries = data_len / 4;
    status = pipe_mgr_drv_blk_wr(
        &shdl,
        entry_width,
        num_entries,
        4,
        dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(pcie_addr),
        log_pipe_mask,
        b);
    if (PIPE_SUCCESS != status) {
      pipe_mgr_api_exit(shdl);
      LOG_ERROR(
          "Failed to correct gfm error, dev %d log-pipe %d stage %d, sts %s",
          dev,
          log_pipe,
          stage,
          pipe_str_err(status));
      return PIPE_NO_SYS_RESOURCES;
    }
  }

  pipe_mgr_api_exit(shdl);

  return PIPE_SUCCESS;
}

/* Write one mirror table entry from shadow */
pipe_status_t pipe_mgr_write_mirrtbl_entry_from_shadow(pipe_sess_hdl_t shdl,
                                                       bf_dev_id_t dev,
                                                       bf_dev_pipe_t log_pipe,
                                                       uint32_t entry,
                                                       bool direction) {
  uint32_t pcie_addr;
  uint32_t *data;
  int i, slice, tbl_word_width = 0;
  pipe_status_t status = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return PIPE_UNEXPECTED;
  }
  if ((dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) &&
      (dev_info->dev_family != BF_DEV_FAMILY_TOFINO3))
    return PIPE_INVALID_ARG;
  if (entry > 15) return PIPE_INVALID_ARG;
  if (log_pipe >= dev_info->num_active_pipes) return PIPE_INVALID_ARG;
  status = pipe_mgr_api_enter(shdl);
  if (PIPE_SUCCESS != status) return status;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      pcie_addr = PIPE_INTR_MIRR_DATA(dev, log_pipe)
                      .tof2[direction][entry]
                      .base_address;
      data = PIPE_INTR_MIRR_DATA(dev, log_pipe).tof2[direction][entry].data;
      tbl_word_width = PIPE_MGR_TOF2_MIRRTBL_WORD_WIDTH;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      pcie_addr = PIPE_INTR_MIRR_DATA(dev, log_pipe)
                      .tof3[direction][entry]
                      .base_address;
      data = PIPE_INTR_MIRR_DATA(dev, log_pipe).tof3[direction][entry].data;
      tbl_word_width = PIPE_MGR_TOF3_MIRRTBL_WORD_WIDTH;
      break;
    case BF_DEV_FAMILY_TOFINO:

    case BF_DEV_FAMILY_UNKNOWN:
    default:
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
      goto cleanup;
  }

  for (slice = 0; slice < 4; slice++) {
    for (i = 0; i < tbl_word_width; i++) {
      // configure one entry for 4 slice
      pipe_mgr_write_register(dev,
                              log_pipe / BF_SUBDEV_PIPE_COUNT,
                              (pcie_addr + (0x2000u * slice) + (0x4u * i)),
                              data[i]);
    }
  }

cleanup:
  pipe_mgr_api_exit(shdl);

  return status;
}

/* Rewrite the entire mirror table from our shadow. */
pipe_status_t pipe_mgr_mirrtbl_write(pipe_sess_hdl_t shdl,
                                     rmt_dev_info_t *dev_info) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* The deparser mirror table is not shadowed on Tofino-1. */
      return PIPE_SUCCESS;
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_mirrtbl_write(shdl, dev_info);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_mirrtbl_write(shdl, dev_info);




    case BF_DEV_FAMILY_UNKNOWN:
    default:
      PIPE_MGR_DBGCHK(0);
  }
  return PIPE_UNEXPECTED;
}

/* Get GFM bit */
pipe_status_t pipe_mgr_gfm_shadow_entry_get(rmt_dev_info_t *dev_info,
                                            bf_dev_pipe_t log_pipe,
                                            uint32_t stage,
                                            uint32_t row,
                                            uint32_t col,
                                            uint32_t *val) {
  bf_dev_pipe_t phy_pipe = 0;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *val = PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
                 .tof.data[row][col];
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *val = PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
                 .tof2.data[row][col];
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *val = PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
                 .tof3.data[row][col];
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return PIPE_SUCCESS;
}

/* Update the shadow GFM DB */
pipe_status_t pipe_mgr_update_gfm_shadow(rmt_dev_info_t *dev_info,
                                         pipe_bitmap_t *pipe_bmp,
                                         uint32_t stage,
                                         uint32_t row,
                                         uint32_t col,
                                         uint32_t val) {
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  PIPE_BITMAP_ITER(pipe_bmp, pipe) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
            .tof.data[row][col] = val;
        break;
      case BF_DEV_FAMILY_TOFINO2:
        PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
            .tof2.data[row][col] = val;
        break;
      case BF_DEV_FAMILY_TOFINO3:
        PIPE_INTR_GFM_DATA(dev_info->dev_id, phy_pipe, stage)
            .tof3.data[row][col] = val;
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        PIPE_MGR_DBGCHK(0);
        return PIPE_UNEXPECTED;
    }
  }
  return PIPE_SUCCESS;
}

/* Set Map Ram type */
static pipe_status_t pipe_mgr_map_ram_type_set(rmt_dev_info_t *dev_info,
                                               pipe_bitmap_t *pipe_bmp,
                                               dev_stage_t stage,
                                               mem_id_t mem_id,
                                               pipe_tbl_hdl_t tbl_hdl,
                                               rmt_tbl_type_t type) {
  bf_dev_id_t dev = dev_info->dev_id;
  int logical_pipe_mask = 0;
  int pipe = 0;

  if (stage >= dev_info->num_active_mau) {
    return PIPE_INVALID_ARG;
  }

  for (pipe = PIPE_BITMAP_GET_FIRST_SET(pipe_bmp); pipe != -1;
       pipe = PIPE_BITMAP_GET_NEXT_BIT(pipe_bmp, pipe)) {
    logical_pipe_mask |= 1u << pipe;
    PIPE_INTR_MAP_RAM_TYPE(dev, pipe, stage, mem_id) = type;
    PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id) = tbl_hdl;
  }

  LOG_TRACE("Dev %d log-pipe-mask 0x%x stage %d mapRAM %d is type %s (%d)",
            dev,
            logical_pipe_mask,
            stage,
            mem_id,
            pipe_mgr_rmt_tbl_type2str(type),
            type);

  return PIPE_SUCCESS;
}

/* Get Map Ram type */
rmt_tbl_type_t pipe_mgr_map_ram_type_get(bf_dev_id_t dev,
                                         bf_dev_pipe_t pipe,
                                         dev_stage_t stage,
                                         mem_id_t mem_id) {
  rmt_tbl_type_t type = -1;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return type;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return (type);
  }

  if (pipe >= dev_info->num_active_pipes) {
    return type;
  }
  if (stage >= dev_info->num_active_mau) {
    return type;
  }

  return (PIPE_INTR_MAP_RAM_TYPE(dev, pipe, stage, mem_id));
}

/* Set the map ram type for all mem-ids's */
pipe_status_t pipe_mgr_set_all_map_ram_type(bf_dev_id_t dev,
                                            pipe_mat_tbl_hdl_t tbl_hdl,
                                            pipe_bitmap_t *pipe_bmp) {
  pipe_mat_tbl_info_t *mat_tbl_info = NULL;
  rmt_tbl_info_t *rmt_info = NULL;
  uint32_t i = 0;
  int j = 0;

  mat_tbl_info = pipe_mgr_get_tbl_info(dev, tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("%s:%d Table %d not found in RMT database for device %d",
              __func__,
              __LINE__,
              tbl_hdl,
              dev);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Do some sanity */
  if (mat_tbl_info->size == 0) {
    return PIPE_INVALID_ARG;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return (PIPE_INVALID_ARG);
  }

  for (i = 0; i < mat_tbl_info->num_rmt_info; i++) {
    rmt_info = &mat_tbl_info->rmt_info[i];
    /* Only idletime is using map ram */
    if (rmt_info->type != RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }
    int stage = rmt_info->stage_id;
    if (stage >= dev_info->num_active_mau) return PIPE_INVALID_ARG;
    /* Go over all mem-ids  */
    for (j = 0; j < rmt_info->bank_map[0].num_tbl_word_blks; j++) {
      mem_id_t mem_id = rmt_info->bank_map[0].tbl_word_blk[j].mem_id[0];
      /* Set the type */
      pipe_mgr_map_ram_type_set(
          dev_info, pipe_bmp, stage, mem_id, tbl_hdl, rmt_info->type);
    }
  }

  return PIPE_SUCCESS;
}

/* Set mem-id table hdl */
static pipe_status_t pipe_mgr_mem_id_tbl_hdl_set(rmt_dev_info_t *dev_info,
                                                 uint8_t gress,
                                                 pipe_bitmap_t *pipe_bmp,
                                                 dev_stage_t stage,
                                                 mem_id_t mem_id,
                                                 pipe_tbl_hdl_t hdl,
                                                 rmt_tbl_type_t tbl_type) {
  int pipe = 0;
  int index = 0;
  int set_count = 0;
  int logical_pipe_mask = 0;
  pipe_mem_type_t mem_type = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  if (stage >= dev_info->num_active_mau) {
    return PIPE_INVALID_ARG;
  }
  /* Multiple tables could map to same mem-id */
  for (pipe = PIPE_BITMAP_GET_FIRST_SET(pipe_bmp); pipe != -1;
       pipe = PIPE_BITMAP_GET_NEXT_BIT(pipe_bmp, pipe)) {
    logical_pipe_mask |= 1u << pipe;
    for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
      if (PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) == 0) {
        PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) = hdl;
        PIPE_INTR_TBL_TYPE(dev, pipe, stage, mem_id, index) = tbl_type;
        pipe_mgr_rmt_tbl_type_to_mem_type(tbl_type, &mem_type);
        PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) = mem_type;
        if (mem_type == pipe_mem_type_tcam) {
          bf_dev_pipe_t phy_pipe = 0;
          pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
          PIPE_INTR_TBL_TCAM_BASE_ADDR(dev, pipe, stage, mem_id, index) =
              dev_info->dev_cfg.get_full_phy_addr(
                  gress, phy_pipe, stage, mem_id, 0, pipe_mem_type_tcam);
        }
        ++set_count;
        break;
      }
    }
  }
  if (set_count != PIPE_BITMAP_COUNT(pipe_bmp)) {
    LOG_ERROR(
        "Dev %d tbl 0x%x stage %d mem_id %d log-pipe-mask 0x%x registered %d "
        "pipes but required "
        "%d",
        dev,
        hdl,
        stage,
        mem_id,
        logical_pipe_mask,
        set_count,
        PIPE_BITMAP_COUNT(pipe_bmp));
    return PIPE_INVALID_ARG;
  } else {
    LOG_DBG(
        "Dev %d stage %d mem_id %d logical pipe mask 0x%x owned by tbl 0x%x",
        dev,
        stage,
        mem_id,
        logical_pipe_mask,
        hdl);
  }

  return PIPE_SUCCESS;
}

/* Set the mapping from mem-ids's to tbl-hdl */
pipe_status_t pipe_mgr_set_mem_id_to_tbl_hdl_mapping(bf_dev_id_t dev,
                                                     rmt_tbl_info_t *rmt_info_p,
                                                     uint32_t num_rmt_info,
                                                     pipe_tbl_hdl_t tbl_hdl,
                                                     pipe_bitmap_t *pipe_bmp) {
  if (num_rmt_info == 0) {
    return PIPE_SUCCESS;
  }
  uint32_t bank, rmt_tbl, word_blk, v;
  rmt_tbl_info_t *rmt_info;
  bool is_s2p = pipe_mgr_rmt_tbl_type_is_s2p(rmt_info_p->type);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return (PIPE_INVALID_ARG);
  }

  for (rmt_tbl = 0; rmt_tbl < num_rmt_info; rmt_tbl++) {
    rmt_info = &rmt_info_p[rmt_tbl];
    /* Don't register idletime mem_ids */
    if (rmt_info->type == RMT_TBL_TYPE_IDLE_TMO) {
      continue;
    }
    uint32_t num_mem_ids = 1;
    num_mem_ids = rmt_info->pack_format.mem_units_per_tbl_word;

    for (bank = 0; bank < rmt_info->num_tbl_banks; bank++) {
      rmt_tbl_bank_map_t *bank_map = &rmt_info->bank_map[bank];
      for (word_blk = 0; word_blk < bank_map->num_tbl_word_blks; word_blk++) {
        rmt_tbl_word_blk_t *tbl_word_blk = &bank_map->tbl_word_blk[word_blk];
        for (v = 0; v < num_mem_ids; v++) {
          mem_id_t mem_id = tbl_word_blk->mem_id[v];
          /* Go over all pipes with this cfg */
          pipe_mgr_mem_id_tbl_hdl_set(dev_info,
                                      rmt_info->direction,
                                      pipe_bmp,
                                      rmt_info->stage_id,
                                      mem_id,
                                      tbl_hdl,
                                      rmt_info->type);
          /* S2P tables also have a map ram for each unit ram. */
          if (is_s2p) {
            mem_id_t map_ram_id =
                dev_info->dev_cfg.map_ram_from_unit_ram(mem_id);
            pipe_mgr_map_ram_type_set(dev_info,
                                      pipe_bmp,
                                      rmt_info->stage_id,
                                      map_ram_id,
                                      tbl_hdl,
                                      rmt_info->type);
          }
        }
      }
    }
    /* S2P tables also have a spare ram. */
    if (is_s2p) {
      for (v = 0; v < rmt_info->num_spare_rams; v++) {
        pipe_mgr_mem_id_tbl_hdl_set(dev_info,
                                    rmt_info->direction,
                                    pipe_bmp,
                                    rmt_info->stage_id,
                                    rmt_info->spare_rams[v],
                                    tbl_hdl,
                                    rmt_info->type);
        mem_id_t map_ram_id =
            dev_info->dev_cfg.map_ram_from_unit_ram(rmt_info->spare_rams[v]);
        pipe_mgr_map_ram_type_set(dev_info,
                                  pipe_bmp,
                                  rmt_info->stage_id,
                                  map_ram_id,
                                  tbl_hdl,
                                  rmt_info->type);
      }
    }
  }

  return PIPE_SUCCESS;
}

/* Get table hdl from mem-id */
pipe_status_t pipe_mgr_get_mem_id_to_tbl_hdl_mapping(bf_dev_id_t dev,
                                                     bf_dev_pipe_t pipe,
                                                     dev_stage_t stage,
                                                     mem_id_t mem_id,
                                                     pipe_mem_type_t mem_type,
                                                     pipe_tbl_hdl_t *hdl,
                                                     rmt_tbl_type_t *tbl_type) {
  int index = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  const rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, dev);
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  if (pipe >= dev_info->num_active_pipes) {
    return PIPE_INVALID_ARG;
  }
  if (stage >= dev_info->num_active_mau) {
    return PIPE_INVALID_ARG;
  }

  /* Multiple tables could map to same mem-id */
  for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
    if ((PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) != 0) &&
        (PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index) == mem_type)) {
      *hdl = PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index);
      *tbl_type = PIPE_INTR_TBL_TYPE(dev, pipe, stage, mem_id, index);
      return PIPE_SUCCESS;
    }
  }

  return PIPE_INVALID_ARG;
}

/* Print table hdl to mem-id mapping info */
void pipe_mgr_dump_mem_id_to_tbl_hdl_mapping(ucli_context_t *uc,
                                             bf_dev_id_t dev,
                                             bf_dev_pipe_t in_pipe,
                                             dev_stage_t in_stage,
                                             mem_id_t in_mem_id) {
  int index = 0;
  dev_stage_t stage = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int mem_id = 0;
  pipe_mem_type_t mem_type = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  const rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  if ((in_pipe != 0xffff) && (in_pipe >= dev_info->num_active_pipes)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", in_pipe);
    return;
  }
  if ((in_stage != 0xff) && (in_stage >= dev_info->num_active_mau)) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", in_stage);
    return;
  }

  aim_printf(&uc->pvs,
             "%-5s %-8s %-5s %-6s %-3s %-3s %-12s %-9s %-10s\n",
             "Pipe",
             "Phy-pipe",
             "Stage",
             "Mem-id",
             "Row",
             "Col",
             "Tbl-Handle",
             "Mem-Type",
             "Type");
  aim_printf(&uc->pvs,
             "-----------------------------------------------------------------"
             "-----\n");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    if ((in_pipe != 0xffff) && (in_pipe != pipe)) {
      continue;
    }
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);

    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      if ((in_stage != 0xff) && (in_stage != stage)) {
        continue;
      }

      for (mem_id = 0; mem_id < PIPE_MGR_MAX_MEM_ID; mem_id++) {
        if ((in_mem_id != 0xff) && (in_mem_id != mem_id)) {
          continue;
        }

        /* Dump Sram, tcam */
        /* Multiple tables could map to same mem-id */
        for (index = 0; index < PIPE_MGR_MAX_HDL_PER_MEM_ID; index++) {
          if (PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index) != 0) {
            mem_type = PIPE_INTR_TBL_MEM_TYPE(dev, pipe, stage, mem_id, index);
            aim_printf(&uc->pvs,
                       "%-5d %-8d %-5d %-6d %-3d %-3d 0x%-10x %-9s %-10s\n",
                       pipe,
                       phy_pipe,
                       stage,
                       mem_id,
                       dev_info->dev_cfg.mem_id_to_row(mem_id, mem_type),
                       dev_info->dev_cfg.mem_id_to_col(mem_id, mem_type) -
                           (mem_type == pipe_mem_type_map_ram ? 6 : 0),
                       PIPE_INTR_TBL_HDL(dev, pipe, stage, mem_id, index),
                       pipe_mgr_mem_type2str(mem_type),
                       pipe_mgr_rmt_tbl_type2str(PIPE_INTR_TBL_TYPE(
                           dev, pipe, stage, mem_id, index)));
          }
        }  // index
        /* Dump map-rams */
        if (PIPE_INTR_MAP_RAM_TYPE(dev, pipe, stage, mem_id) != 0) {
          /* We're searching the mapRam type DB so of course the memtype must
           * be MAPRAM. */
          mem_type = pipe_mem_type_map_ram;
          aim_printf(&uc->pvs,
                     "%-5d %-8d %-5d %-6d %-3d %-3d 0x%-10x %-9s %-10s\n",
                     pipe,
                     phy_pipe,
                     stage,
                     mem_id,
                     dev_info->dev_cfg.mem_id_to_row(mem_id, mem_type),
                     dev_info->dev_cfg.mem_id_to_col(mem_id, mem_type) - 6,
                     PIPE_INTR_MAP_RAM_TBL_HDL(dev, pipe, stage, mem_id),
                     pipe_mgr_mem_type2str(mem_type),
                     pipe_mgr_rmt_tbl_type2str(
                         PIPE_INTR_MAP_RAM_TYPE(dev, pipe, stage, mem_id)));
        }

      }  // mem_id
      aim_printf(&uc->pvs, "\n");
    }  // stage
    aim_printf(&uc->pvs, "\n");
  }  // pipe

  return;
}
/* lookup for imem memory errors and cache state */
pipe_status_t pipe_mgr_lookup_cache_imem_register_val(rmt_dev_info_t *dev_info,
                                                      uint32_t log_pipe_mask,
                                                      dev_stage_t stage,
                                                      uint32_t base_address,
                                                      uint8_t *data,
                                                      int data_len,
                                                      bool *shadowed) {
  if (stage >= dev_info->num_active_mau) {
    return PIPE_INVALID_ARG;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* Always report NOT shadowed on TF1.  This will cause imem configuration
       * to be posted during the tofino.bin parsing. */
      *shadowed = false;
      return pipe_mgr_tof_interrupt_cache_imem_val(
          dev_info, log_pipe_mask, stage, base_address, data, data_len);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_interrupt_cache_imem_val(dev_info,
                                                    log_pipe_mask,
                                                    stage,
                                                    base_address,
                                                    data,
                                                    data_len,
                                                    shadowed);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_interrupt_cache_imem_val(
          dev_info, log_pipe_mask, stage, base_address, data, data_len);
      break;



    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_imem_write(pipe_sess_hdl_t shdl,
                                  rmt_dev_info_t *dev_info,
                                  bf_dev_pipe_t phy_pipe_filter,
                                  int stage_filter,
                                  bool chip_init) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      /* Not needed today as tofino.bin download writes imem and parity errors
       * are handled by another function. */
      return PIPE_SUCCESS;
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_imem_write(
          shdl, dev_info, phy_pipe_filter, stage_filter, chip_init);
    case BF_DEV_FAMILY_TOFINO3:
      return PIPE_SUCCESS;





    case BF_DEV_FAMILY_UNKNOWN:
      return PIPE_UNEXPECTED;
  }
  return PIPE_UNEXPECTED;
}

/* Lookup mirr tbl register to cache for ecc errors */
pipe_status_t pipe_mgr_lookup_cache_mirrtbl_register_content(
    rmt_dev_info_t *dev_info,
    uint32_t log_pipe_mask,
    uint32_t base_address,
    uint8_t *data,
    int data_len) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_interrupt_cache_mirrtbl_val(
          dev_info, log_pipe_mask, base_address, data, data_len);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_interrupt_cache_mirrtbl_val(
          dev_info, log_pipe_mask, base_address, data, data_len);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

/* Lookup GFM (Galois field matrix) memory to cache for ecc errors */
pipe_status_t pipe_mgr_lookup_cache_gfm(rmt_dev_info_t *dev_info,
                                        uint32_t log_pipe_mask,
                                        dev_stage_t stage,
                                        uint32_t address,
                                        uint8_t *data,
                                        int data_len) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_cache_gfm(
          dev_info, log_pipe_mask, stage, address, data, data_len);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_cache_gfm(
          dev_info, log_pipe_mask, stage, address, data, data_len);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_cache_gfm(
          dev_info, log_pipe_mask, stage, address, data, data_len);


    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

/* Parser instance info operations */
static pipe_status_t pipe_mgr_get_prsr_instance(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    struct pipe_mgr_prsr_instance_t **instance_info) {
  bf_map_sts_t sts = bf_map_get(&PIPE_PRSR_DATA(dev, pipeid, gress),
                                (unsigned long)prsr_instance_hdl,
                                (void **)instance_info);
  if (sts != BF_MAP_OK) {
    return PIPE_INVALID_ARG;
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_get_pvs_hdls(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t **pvs_hdl,
    uint32_t *pvs_hdl_numb) {
  if (pvs_hdl_numb == NULL) return PIPE_INVALID_ARG;
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  *pvs_hdl = instance_info->pvs_hdl;
  *pvs_hdl_numb = instance_info->pvs_hdl_numb;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_get_phase0_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t *phase0_hdl) {
  if (phase0_hdl == NULL) return PIPE_INVALID_ARG;
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  *phase0_hdl = instance_info->phase0_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_get_bin_cfg(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    union pipe_parser_bin_config_t **bin_cfg) {
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  *bin_cfg = &(instance_info->bin_cfg);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_get_default_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t *prsr_map) {
  if (prsr_map == NULL) return PIPE_INVALID_ARG;
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  *prsr_map = instance_info->prsr_map_default;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_get_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t *prsr_map) {
  if (prsr_map == NULL) return PIPE_INVALID_ARG;
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  *prsr_map = instance_info->prsr_map;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_set_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t prsr_map) {
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  instance_info->prsr_map = prsr_map;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_prsr_instance_reset_profile(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl) {
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  instance_info->prsr_map = instance_info->prsr_map_default;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_prsr_instance_hdl_from_name(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    char *prsr_instance_name,
    pipe_prsr_instance_hdl_t *prsr_instance_hdl) {
  bf_map_t *prsr_map_p = &PIPE_PRSR_DATA(dev, pipeid, gress);
  unsigned long key;
  struct pipe_mgr_prsr_instance_t *instance_info;
  bf_map_sts_t sts;
  sts = bf_map_get_first(prsr_map_p, &key, (void **)&instance_info);
  if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  while (strcmp(prsr_instance_name, instance_info->name) != 0) {
    sts = bf_map_get_next(prsr_map_p, &key, (void **)&instance_info);
    if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  }
  *prsr_instance_hdl = (pipe_prsr_instance_hdl_t)key;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_prsr_instance_name_from_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    char *prsr_instance_name) {
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  strcpy(prsr_instance_name, instance_info->name);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_prsr_instance_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    uint8_t prsr_id,
    pipe_prsr_instance_hdl_t *prsr_instance_hdl) {
  if (prsr_instance_hdl == NULL) return PIPE_INVALID_ARG;
  bf_map_t *prsr_map_p = &PIPE_PRSR_DATA(dev, pipeid, gress);
  unsigned long key;
  struct pipe_mgr_prsr_instance_t *instance_info;
  bf_map_sts_t sts;
  sts = bf_map_get_first(prsr_map_p, &key, (void **)&instance_info);
  if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  while (!((instance_info->prsr_map & (1 << prsr_id)) != 0)) {
    sts = bf_map_get_next(prsr_map_p, &key, (void **)&instance_info);
    if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  }
  *prsr_instance_hdl = (pipe_prsr_instance_hdl_t)key;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_get_prsr_default_instance_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    uint8_t prsr_id,
    pipe_prsr_instance_hdl_t *default_prsr_instance_hdl) {
  if (default_prsr_instance_hdl == NULL) return PIPE_INVALID_ARG;
  bf_map_t *prsr_map_p = &PIPE_PRSR_DATA(dev, pipeid, gress);
  unsigned long key;
  struct pipe_mgr_prsr_instance_t *instance_info;
  bf_map_sts_t sts;
  sts = bf_map_get_first(prsr_map_p, &key, (void **)&instance_info);
  if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  while (!((instance_info->prsr_map_default & (1 << prsr_id)) != 0)) {
    sts = bf_map_get_next(prsr_map_p, &key, (void **)&instance_info);
    if (sts != BF_MAP_OK) return PIPE_INVALID_ARG;
  }
  *default_prsr_instance_hdl = (pipe_prsr_instance_hdl_t)key;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_set_prsr_instance_phase0_hdl(
    bf_dev_id_t dev,
    bf_dev_pipe_t pipeid,
    uint8_t gress,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint32_t phase0_table_hdl) {
  struct pipe_mgr_prsr_instance_t *instance_info;
  pipe_status_t sts;
  sts = pipe_mgr_get_prsr_instance(
      dev, pipeid, gress, prsr_instance_hdl, &instance_info);
  if (sts != PIPE_SUCCESS) return sts;
  if (instance_info->phase0_hdl != 0) {
    LOG_ERROR(
        "Fail in adding Phase0 table handler 0x%x to parser instance 0x%x, "
        "table has already assigned, dev %d, pipe %d",
        instance_info->phase0_hdl,
        prsr_instance_hdl,
        dev,
        pipeid);
    return PIPE_INVALID_ARG;
  }
  instance_info->phase0_hdl = phase0_table_hdl;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_lookup_cache_parser_bin_reg_cfg(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint32_t address,
    uint32_t data,
    bool *shadowed) {
  pipe_status_t status = PIPE_SUCCESS;
  /* Check first */
  if (dev_info->dev_id >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  /* If parser address, need to return prsr_mask */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      status = pipe_mgr_tof_cache_prsr_reg_val(
          dev_info, prsr_instance_hdl, prof_id, address, data, shadowed);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      status = pipe_mgr_tof2_cache_prsr_reg_val(
          dev_info, prsr_instance_hdl, prof_id, address, data, shadowed);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      status = pipe_mgr_tof3_cache_prsr_reg_val(
          dev_info, prsr_instance_hdl, prof_id, address, data, shadowed);
      break;



    default:
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
  }
  return status;
}

/* Lookup parser memory to cache */
pipe_status_t pipe_mgr_lookup_cache_parser_bin_cfg(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint64_t address,
    uint8_t *data,
    int data_len,
    bool *shadowed) {
  pipe_status_t status = PIPE_SUCCESS;
  /* Check first */
  if (dev_info->dev_id >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }
  /* is this the parser address range */
  uint32_t prsr_stage = 0;
  if (LLD_OK != lld_sku_get_prsr_stage(dev_info->dev_id, &prsr_stage)) {
    LOG_ERROR("%s:%d Failed to get prsr stage for device %d",
              __func__,
              __LINE__,
              dev_info->dev_id);
    return PIPE_INVALID_ARG;
  }
  uint32_t this_stage = dev_info->dev_cfg.stage_id_from_addr(address);
  if (prsr_stage != this_stage) {
    return PIPE_SUCCESS;
  }
  /* If parser address, need to return prsr_mask */
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      status = pipe_mgr_tof_cache_prsr_val(dev_info,
                                           prof_id,
                                           prsr_instance_hdl,
                                           address,
                                           data,
                                           data_len,
                                           shadowed);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      status = pipe_mgr_tof2_cache_prsr_val(dev_info,
                                            prof_id,
                                            prsr_instance_hdl,
                                            address,
                                            data,
                                            data_len,
                                            shadowed);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      status = pipe_mgr_tof3_cache_prsr_val(dev_info,
                                            prof_id,
                                            prsr_instance_hdl,
                                            address,
                                            data,
                                            data_len,
                                            shadowed);
      break;



    default:
      PIPE_MGR_DBGCHK(0);
      status = PIPE_UNEXPECTED;
  }
  return status;
}

/* Update parser TCAM contents. */
pipe_status_t pipe_mgr_set_parser_tcam_shadow(rmt_dev_info_t *dev_info,
                                              bf_dev_pipe_t pipe,
                                              bool ing0_egr1,
                                              int prsr_id,
                                              int tcam_index,
                                              uint8_t data_len,
                                              uint8_t *word0,
                                              uint8_t *word1) {
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_interrupt_set_parser_tcam_shadow(dev_info,
                                                           pipe,
                                                           ing0_egr1,
                                                           prsr_id,
                                                           tcam_index,
                                                           data_len,
                                                           word0,
                                                           word1);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_interrupt_set_parser_tcam_shadow(dev_info,
                                                            pipe,
                                                            ing0_egr1,
                                                            prsr_id,
                                                            tcam_index,
                                                            data_len,
                                                            word0,
                                                            word1);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_interrupt_set_parser_tcam_shadow(dev_info,
                                                            pipe,
                                                            ing0_egr1,
                                                            prsr_id,
                                                            tcam_index,
                                                            data_len,
                                                            word0,
                                                            word1);
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
}

pipe_status_t pipe_mgr_gfm_test(pipe_sess_hdl_t shdl,
                                bf_dev_target_t dev_tgt,
                                bf_dev_direction_t gress,
                                dev_stage_t stage_id,
                                int num_patterns,
                                uint64_t *row_patterns,
                                uint64_t *row_bad_parity) {
  bf_dev_id_t dev_id = dev_tgt.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return BF_INVALID_ARG;
  }

  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      dev_tgt.dev_pipe_id >= dev_info->num_active_pipes) {
    LOG_ERROR("%s: Invalid pipe-id requested, %d, dev %d has %d pipes active",
              __func__,
              dev_tgt.dev_pipe_id,
              dev_id,
              dev_info->num_active_pipes);
    return PIPE_INVALID_ARG;
  }

  if (stage_id != 0xFF && stage_id >= dev_info->num_active_mau) {
    LOG_ERROR("%s: Invalid stage-id requested, %d, dev %d has %d stages active",
              __func__,
              stage_id,
              dev_id,
              dev_info->num_active_mau);
    return PIPE_INVALID_ARG;
  }

  if (num_patterns < 0 || num_patterns > 1024) return PIPE_INVALID_ARG;
  if (!row_patterns) return PIPE_INVALID_ARG;
  if (gress != BF_DEV_DIR_INGRESS && gress != BF_DEV_DIR_EGRESS)
    return PIPE_INVALID_ARG;

  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) return PIPE_NOT_SUPPORTED;

  return pipe_mgr_tof2_gfm_test_pattern(shdl,
                                        dev_info,
                                        dev_tgt.dev_pipe_id,
                                        gress,
                                        stage_id,
                                        num_patterns,
                                        row_patterns,
                                        row_bad_parity);
}

pipe_status_t pipe_mgr_gfm_test_col(pipe_sess_hdl_t shdl,
                                    bf_dev_target_t dev_tgt,
                                    bf_dev_direction_t gress,
                                    dev_stage_t stage_id,
                                    int column,
                                    uint16_t col_data[64]) {
  bf_dev_id_t dev_id = dev_tgt.device_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s: Invalid device id %d", __func__, dev_id);
    return BF_INVALID_ARG;
  }

  if (dev_tgt.dev_pipe_id != BF_DEV_PIPE_ALL &&
      dev_tgt.dev_pipe_id >= dev_info->num_active_pipes) {
    LOG_ERROR("%s: Invalid pipe-id requested, %d, dev %d has %d pipes active",
              __func__,
              dev_tgt.dev_pipe_id,
              dev_id,
              dev_info->num_active_pipes);
    return PIPE_INVALID_ARG;
  }

  if (stage_id != 0xFF && stage_id >= dev_info->num_active_mau) {
    LOG_ERROR("%s: Invalid stage-id requested, %d, dev %d has %d stages active",
              __func__,
              stage_id,
              dev_id,
              dev_info->num_active_mau);
    return PIPE_INVALID_ARG;
  }

  if (column < 0 || column > 51) return PIPE_INVALID_ARG;
  if (gress != BF_DEV_DIR_INGRESS && gress != BF_DEV_DIR_EGRESS)
    return PIPE_INVALID_ARG;

  if (dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) return PIPE_NOT_SUPPORTED;

  return pipe_mgr_tof2_gfm_test_col(
      shdl, dev_info, dev_tgt.dev_pipe_id, gress, stage_id, column, col_data);
}

void pipe_mgr_dump_gfm_shadow(ucli_context_t *uc,
                              bf_dev_id_t dev,
                              bf_dev_pipe_t in_pipe,
                              dev_stage_t in_stage) {
  dev_stage_t stage = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int row = 0, column = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  const rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  if ((in_pipe != 0xffff) && (in_pipe >= dev_info->num_active_pipes)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", in_pipe);
    return;
  }
  if ((in_stage != 0xff) && (in_stage >= dev_info->num_active_mau)) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", in_stage);
    return;
  }

  aim_printf(&uc->pvs,
             "%-5s:%-8s %-5s %-6s %-3s %-12s\n",
             "Pipe",
             "Phy-pipe",
             "Stage",
             "Row",
             "Col",
             "Data");
  aim_printf(&uc->pvs,
             "-----------------------------------------------------------------"
             "-----\n");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    if ((in_pipe != 0xffff) && (in_pipe != pipe)) {
      continue;
    }
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      if ((in_stage != 0xff) && (in_stage != stage)) {
        continue;
      }
      union pipe_gfm_shadow_db stage_gfm_db = pipe_db[dev]->gfm_db[pipe][stage];
      // This is the same for all tofinos, one row index has 16 rows bits.
      for (row = 0; row < PIPE_MGR_TOF_MAX_GFM_ROWS; row++) {
        for (column = 0; column < PIPE_MGR_TOF_MAX_GFM_COLS; column++) {
          // Print only non-zero values for now.
          if (stage_gfm_db.tof.data[row][column] == 0) continue;
          aim_printf(&uc->pvs,
                     "%-5d:%-8d %-5d %-6d %-3d 0x%-10x\n",
                     pipe,
                     phy_pipe,
                     stage,
                     row,
                     column,
                     stage_gfm_db.tof.data[row][column]);
        }  // column
      }    // row
      aim_printf(&uc->pvs, "\n");
    }  // stage
    aim_printf(&uc->pvs, "\n");
  }  // pipe

  return;
}

void pipe_mgr_dump_hash_seed_shadow(ucli_context_t *uc,
                                    bf_dev_id_t dev,
                                    bf_dev_pipe_t in_pipe,
                                    dev_stage_t in_stage) {
  dev_stage_t stage = 0;
  bf_dev_pipe_t pipe = 0, phy_pipe = 0;
  int bit = 0, column = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  const rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev);
    return;
  }
  if ((in_pipe != 0xffff) && (in_pipe >= dev_info->num_active_pipes)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", in_pipe);
    return;
  }
  if ((in_stage != 0xff) && (in_stage >= dev_info->num_active_mau)) {
    aim_printf(&uc->pvs, "Invalid stage %d\n", in_stage);
    return;
  }

  aim_printf(&uc->pvs,
             "%-4s:%-8s %-5s %-3s %-12s\n",
             "Pipe",
             "Phy-pipe",
             "Stage",
             "Col",
             "Seed");
  aim_printf(&uc->pvs,
             "-----------------------------------------------------------------"
             "-----\n");

  for (pipe = 0; pipe < dev_info->num_active_pipes; pipe++) {
    if ((in_pipe != 0xffff) && (in_pipe != pipe)) {
      continue;
    }
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe);
    for (stage = 0; stage < dev_info->num_active_mau; stage++) {
      if ((in_stage != 0xff) && (in_stage != stage)) {
        continue;
      }
      pipe_mgr_sel_hash_seed_db_t *seed_db =
          &(pipe_sel_shadow_db[dev]->seed_db[pipe][stage]);
      uint64_t hash_seed_out_grp[8] = {0};
      for (column = 0; column < PIPE_SEL_HASH_SEEDS_NUM; column++) {
        // Print only non-zero values for now.
        if (seed_db->hash_seed[column] == 0) continue;
        for (bit = 0; bit < 8; bit++) {
          hash_seed_out_grp[bit] |=
              (((uint64_t)seed_db->hash_seed[column] >> bit) & 0x1) << column;
        }
        aim_printf(&uc->pvs,
                   "%-4d:%-8d %-5d %-3d 0x%-12x\n",
                   pipe,
                   phy_pipe,
                   stage,
                   column,
                   seed_db->hash_seed[column]);
      }  // column
      aim_printf(&uc->pvs, "Hash seed configs: \n");
      for (bit = 0; bit < 8; bit++) {
        aim_printf(&uc->pvs,
                   "%-4d:%-8d %-5d %-3d 0x%-16" PRIx64 "\n",
                   pipe,
                   phy_pipe,
                   stage,
                   bit,
                   hash_seed_out_grp[bit]);
      }
      aim_printf(&uc->pvs, "\n");
    }  // stage
    aim_printf(&uc->pvs, "\n");
  }  // pipe

  return;
}
