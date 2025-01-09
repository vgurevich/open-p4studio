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


#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_tof_db.h"
#include "pipe_mgr_db.h"
#include <tofino_regs/tofino.h>
#include <tofino_regs/pipe_top_level.h>
extern pipe_mgr_ctx_t *pipe_mgr_ctx;
pipe_status_t pipe_mgr_tof_interrupt_db_init(rmt_dev_info_t *dev_info) {
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  for (log_pipe = 0; log_pipe < dev_info->num_active_pipes; log_pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    for (unsigned int s_idx = 0; s_idx < dev_info->num_active_mau; s_idx++) {
      for (int isz = 0; isz < PIPE_MGR_TOF_IMEM_COUNT; ++isz) {
        uint32_t base =
            isz == PIPE_MGR_TOF_IMEM32
                ? offsetof(
                      Tofino,
                      pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword32[0][0])
                : isz == PIPE_MGR_TOF_IMEM16
                      ? offsetof(Tofino,
                                 pipes[phy_pipe]
                                     .mau[s_idx]
                                     .dp.imem.imem_subword16[0][0])
                      : offsetof(Tofino,
                                 pipes[phy_pipe]
                                     .mau[s_idx]
                                     .dp.imem.imem_subword8[0][0]);
        uint32_t len =
            (isz == PIPE_MGR_TOF_IMEM32
                 ? offsetof(Tofino,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_subword32[63][31])
                 : isz == PIPE_MGR_TOF_IMEM16
                       ? offsetof(Tofino,
                                  pipes[phy_pipe]
                                      .mau[s_idx]
                                      .dp.imem.imem_subword16[95][31])
                       : offsetof(Tofino,
                                  pipes[phy_pipe]
                                      .mau[s_idx]
                                      .dp.imem.imem_subword8[63][31])) -
            base + 4;
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof.imem[isz].base_addr =
            base;
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof.imem[isz].data_len = len;
      }
      /* GFM: Cache the base address and data len */
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof.base_addr = offsetof(
          Tofino,
          pipes[phy_pipe].mau[s_idx].dp.xbar_hash.hash.galois_field_matrix[0]
                                                                          [0]);
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof.data_len =
          sizeof(uint32_t) * PIPE_MGR_TOF_MAX_GFM_ROWS *
          PIPE_MGR_TOF_MAX_GFM_COLS;
    }
  }
  return PIPE_SUCCESS;
}
void pipe_mgr_tof_prsr_reg_init(bf_dev_id_t dev,
                                int dir,
                                uint32_t prsr_reg_base) {
  uint32_t *des = pipe_db[dev]->prsr_base_addr[dir].tof.prsr_reg_addr;
  /* tofino parser registers list.
     pipes[].pmarb.ibp18_reg.ibp_reg[].prsr_reg/
     pipes[].pmarb.ebp18_reg.ebp_reg[].prsr_reg/
     max_iter, pri_thresh,
     phv_owner, no_multi_wr
  */
  const uint32_t offset[TOF_PRSR_REG_DEPTH] = {
      0x10, 0x1c, 0x6c, 0x68, 0x64, 0x60, 0x5c, 0x58, 0x54,
      0x50, 0x4c, 0x48, 0x44, 0x40, 0xac, 0xa8, 0xa4, 0xa0,
      0x9c, 0x98, 0x94, 0x90, 0x8c, 0x88, 0x84, 0x80};
  for (int i = 0; i < TOF_PRSR_REG_DEPTH; i++) {
    des[i] = prsr_reg_base + offset[i];
  }
}

void pipe_mgr_tof_prsr_db_init(bf_dev_id_t dev) {
  for (int dir = 0; dir < PIPE_DIR_MAX; dir++) {
    uint64_t po_pipe_base =
        (dir ? pipe_top_level_pipes_e_prsr_po_action_row_address
             : pipe_top_level_pipes_i_prsr_po_action_row_address);
    uint64_t tcam_0_base =
        (dir ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word0_address
             : pipe_top_level_pipes_i_prsr_ml_tcam_row_word0_address);
    uint64_t tcam_1_base =
        (dir ? pipe_top_level_pipes_e_prsr_ml_tcam_row_word1_address
             : pipe_top_level_pipes_i_prsr_ml_tcam_row_word1_address);
    uint64_t ea_row_base =
        (dir ? pipe_top_level_pipes_e_prsr_ml_ea_row_address
             : pipe_top_level_pipes_i_prsr_ml_ea_row_address);
    uint64_t ctr_init_ram_base =
        (dir ? pipe_top_level_pipes_e_prsr_ml_ctr_init_ram_address
             : pipe_top_level_pipes_i_prsr_ml_ctr_init_ram_address);
    uint64_t po_csum_ctr0_base =
        (dir ? pipe_top_level_pipes_e_prsr_po_csum_ctrl_0_row_address
             : pipe_top_level_pipes_i_prsr_po_csum_ctrl_0_row_address);
    uint64_t po_csum_ctr1_base =
        (dir ? pipe_top_level_pipes_e_prsr_po_csum_ctrl_1_row_address
             : pipe_top_level_pipes_i_prsr_po_csum_ctrl_1_row_address);
    uint32_t prsr_reg_base =
        (dir ? tofino_pipes_pmarb_ebp18_reg_ebp_reg_prsr_reg_address
             : tofino_pipes_pmarb_ibp18_reg_ibp_reg_prsr_reg_address);
    uint64_t prsr_step = dir ? pipe_top_level_pipes_e_prsr_array_element_size
                             : pipe_top_level_pipes_i_prsr_array_element_size;
    uint32_t prsr_reg_step =
        dir ? tofino_pipes_pmarb_ebp18_reg_ebp_reg_array_element_size
            : tofino_pipes_pmarb_ibp18_reg_ibp_reg_array_element_size;
    /* Po action ram - Array of 256 elements and each element is 32 bytes */
    pipe_db[dev]->prsr_base_addr[dir].tof.po_action_addr = po_pipe_base >> 4;
    /* tcam word 0 */
    pipe_db[dev]->prsr_base_addr[dir].tof.word0_addr = tcam_0_base >> 4;
    /* tcam word 1 */
    pipe_db[dev]->prsr_base_addr[dir].tof.word1_addr = tcam_1_base >> 4;
    /* ea row */
    pipe_db[dev]->prsr_base_addr[dir].tof.ea_row_addr = ea_row_base >> 4;
    /* ctr init ram */
    pipe_db[dev]->prsr_base_addr[dir].tof.ctr_init_ram_addr =
        ctr_init_ram_base >> 4;
    /* po_csum_ctr0 */
    pipe_db[dev]->prsr_base_addr[dir].tof.po_csum_ctr0_addr =
        po_csum_ctr0_base >> 4;
    /* po_csum_ctr1 */
    pipe_db[dev]->prsr_base_addr[dir].tof.po_csum_ctr1_addr =
        po_csum_ctr1_base >> 4;
    pipe_db[dev]->prsr_base_addr[dir].tof.prsr_step = prsr_step >> 4;
    /* reg */
    pipe_mgr_tof_prsr_reg_init(dev, dir, prsr_reg_base);
    pipe_db[dev]->prsr_base_addr[dir].tof.prsr_reg_step = prsr_reg_step;
  }
}

void pipe_mgr_tof_interrupt_db_cleanup(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  int dev_pipes = dev_info->num_active_pipes;
  int dev_stages = dev_info->num_active_mau;
  for (int p = 0; p < dev_pipes; ++p) {
    if (pipe_db[dev]->imem_db && pipe_db[dev]->imem_db[p]) {
      for (int s = 0; s < dev_stages; ++s) {
        for (int i = 0; i < PIPE_MGR_TOF_IMEM_COUNT; ++i) {
          if (pipe_db[dev]->imem_db[p][s].tof.imem[i].data) {
            PIPE_MGR_FREE(pipe_db[dev]->imem_db[p][s].tof.imem[i].data);
          }
        }
      }
      PIPE_MGR_FREE(pipe_db[dev]->imem_db[p]);
    }
  }
}

/* Rewrite the seed */
static pipe_status_t pipe_mgr_tof_rewrite_seed(pipe_sess_hdl_t sess_hdl,
                                               rmt_dev_info_t *dev_info,
                                               pipe_bitmap_t *pipe_bmp,
                                               dev_stage_t stage) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t addr = 0, value = 0;
  pipe_instr_write_reg_t instr;
  bf_dev_id_t dev = dev_info->dev_id;
  /* Get lowest pipe of profile to update shadow DB */
  bf_dev_pipe_t log_pipe = PIPE_BITMAP_GET_FIRST_SET(pipe_bmp);

  addr =
      offsetof(Tofino,
               pipes[0]
                   .mau[stage]
                   .dp.xbar_hash.hash.hash_seed[PIPE_MGR_TOF_SEED_PARITY_COL]);

  value = PIPE_SEL_SHADOW_DB(dev)
              ->seed_db[log_pipe][stage]
              .hash_seed[PIPE_MGR_TOF_SEED_PARITY_COL];

  construct_instr_reg_write(dev, &instr, addr, value);

  status = pipe_mgr_drv_ilist_add(
      &sess_hdl, dev_info, pipe_bmp, stage, (uint8_t *)&instr, sizeof instr);

  if (status != PIPE_SUCCESS) {
    LOG_ERROR(
        "Failed to add seed parity to instruction list, stage %d, "
        "rc = (%d), dev_id %d",
        stage,
        status,
        dev);
    return status;
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_recalc_write_seed(pipe_sess_hdl_t sess_hdl,
                                             rmt_dev_info_t *dev_info,
                                             pipe_bitmap_t *pipe_bmp,
                                             dev_stage_t stage) {
  uint32_t col = 0, bitpos = 0, num_ones = 0;
  bf_dev_id_t dev = dev_info->dev_id;
  /* Get lowest pipe of profile to update shadow DB */
  bf_dev_pipe_t log_pipe = PIPE_BITMAP_GET_FIRST_SET(pipe_bmp);

  /* Each hash seed register is 8 bits, each corresponding to its output
   * hash group */
  for (bitpos = 0; bitpos < PIPE_MGR_TOF_OUTPUT_PARITY_GROUPS; bitpos++) {
    num_ones = 0;

    for (col = 0; col < PIPE_MGR_TOF_SEED_PARITY_COL; col++) {
      num_ones +=
          ((PIPE_SEL_SHADOW_DB(dev)->seed_db[log_pipe][stage].hash_seed[col] >>
            bitpos) &
           1u);
    }
    if ((num_ones % 2) == 1) {
      /* Set the parity to 1 */
      PIPE_SEL_SHADOW_DB(dev)
          ->seed_db[log_pipe][stage]
          .hash_seed[PIPE_MGR_TOF_SEED_PARITY_COL] |= (1u << bitpos);
    } else {
      /* Set the parity to 0 */
      PIPE_SEL_SHADOW_DB(dev)
          ->seed_db[log_pipe][stage]
          .hash_seed[PIPE_MGR_TOF_SEED_PARITY_COL] &= ~((uint8_t)1u << bitpos);
    }
  }

  return pipe_mgr_tof_rewrite_seed(sess_hdl, dev_info, pipe_bmp, stage);
}

/* Rewrite the GFM parity */
static pipe_status_t pipe_mgr_tof_rewrite_gfm_parity(pipe_sess_hdl_t sess_hdl,
                                                     rmt_dev_info_t *dev_info,
                                                     pipe_bitmap_t *pipe_bmp,
                                                     dev_stage_t stage) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t row = 0;
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  uint32_t addr = 0, value = 0;
  pipe_instr_write_reg_t instr;
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_bitmap_t local_pipe_bmp;

  PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);

  /* We do not know if features using GFM are symmetric or not,
     so rewrite all pipes one by one
  */
  PIPE_BITMAP_ITER(pipe_bmp, log_pipe) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    PIPE_BITMAP_CLR_ALL(&local_pipe_bmp);
    PIPE_BITMAP_SET(&local_pipe_bmp, log_pipe);

    for (row = 0; row < PIPE_MGR_TOF_MAX_GFM_ROWS; row++) {
      addr =
          offsetof(Tofino,
                   pipes[0]
                       .mau[stage]
                       .dp.xbar_hash.hash
                       .galois_field_matrix[row][PIPE_MGR_TOF_GFM_PARITY_COL]);

      value = PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
                  .tof.data[row][PIPE_MGR_TOF_GFM_PARITY_COL];

      construct_instr_reg_write(dev, &instr, addr, value);

      status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                      dev_info,
                                      &local_pipe_bmp,
                                      stage,
                                      (uint8_t *)&instr,
                                      sizeof instr);

      if (status != PIPE_SUCCESS) {
        LOG_ERROR(
            "Failed to add GFM parity to instruction list, row = %d, stage %d, "
            "rc = (%d), dev_id %d",
            row,
            stage,
            status,
            dev);
        return status;
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
                                                   rmt_dev_info_t *dev_info,
                                                   pipe_bitmap_t *pipe_bmp,
                                                   dev_stage_t stage,
                                                   bool skip_write) {
  pipe_status_t status = PIPE_SUCCESS;
  uint32_t row = 0, col = 0, bitpos = 0, num_ones = 0;
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  PIPE_BITMAP_ITER(pipe_bmp, log_pipe) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    for (row = 0; row < PIPE_MGR_TOF_MAX_GFM_ROWS; row++) {
      /* byte0 - 8 bits, byte1 - 8 bits, valid0 - 1 bit, valid1 - 1 bit */
      for (bitpos = 0; bitpos < PIPE_MGR_TOF_SINGLE_GFM_ENTRY_SZ; bitpos++) {
        num_ones = 0;

        for (col = 0; col < PIPE_MGR_TOF_GFM_PARITY_COL; col++) {
          num_ones +=
              ((PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data[row][col] >>
                bitpos) &
               1u);
        }
        if ((num_ones % 2) == 1) {
          /* Set the parity to 1 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof.data[row][PIPE_MGR_TOF_GFM_PARITY_COL] |= (1u << bitpos);
        } else {
          /* Set the parity to 0 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof.data[row][PIPE_MGR_TOF_GFM_PARITY_COL] &=
              ~((uint32_t)1u << bitpos);
        }
      }
    }
  }

  if (!skip_write) {
    status =
        pipe_mgr_tof_rewrite_gfm_parity(sess_hdl, dev_info, pipe_bmp, stage);
  }

  return status;
}

pipe_status_t pipe_mgr_tof_interrupt_cache_imem_val(rmt_dev_info_t *dev_info,
                                                    uint32_t log_pipe_mask,
                                                    dev_stage_t stage,
                                                    uint32_t base_address,
                                                    uint8_t *data,
                                                    int data_len) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t num_pipes = dev_info->num_active_pipes;
  bf_dev_pipe_t logical_pipe = 0, phy_pipe;
  int i;

  /* Map logical pipe 0 to the physical pipe and use that for comparision. */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  base_address = dev_info->dev_cfg.dir_addr_set_pipe_id(base_address, phy_pipe);

  for (i = 0; i < PIPE_MGR_TOF_IMEM_COUNT; ++i) {
    if (base_address ==
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].base_addr) {
      for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
        if (~log_pipe_mask & (1u << logical_pipe)) continue;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data =
            PIPE_MGR_MALLOC(data_len);
        if (!PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data) {
          return PIPE_NO_SYS_RESOURCES;
        }
        if (data_len !=
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data_len) {
          LOG_ERROR(
              "Dev %d pipe %d stage %d, expected imem length %d but got %d",
              dev_info->dev_id,
              logical_pipe,
              stage,
              PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data_len,
              data_len);
        }
        PIPE_MGR_MEMCPY(
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof.imem[i].data,
            data,
            data_len);
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_cache_prsr_reg_val(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint32_t address,
    uint32_t data,
    bool *shadowed) {
  struct pipe_mgr_prsr_instance_t *instance;
  struct pipe_mgr_tof_prsr_bin_config *bin_cfg;
  uint32_t *base_addr;
  uint8_t gress;
  bool found_reg = false;
  int i;
  /* Bin files generated by glass may not have the top address bit indicating
   * a "pipe" address type, account for that by adding it into the address. */
  uint32_t address_add_pipe_base_addr = (address < tofino_pipes_address)
                                            ? (address + tofino_pipes_address)
                                            : address;
  bf_map_sts_t sts;
  for (gress = 0; gress < PIPE_DIR_MAX; gress++) {
    base_addr = PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.prsr_reg_addr;
    for (i = 0; i < TOF_PRSR_REG_DEPTH; i++) {
      if (address_add_pipe_base_addr == base_addr[i]) {
        address = address_add_pipe_base_addr;
        found_reg = true;
        break;
      }
    }
    if (found_reg) break;
  }
  if (!found_reg) return PIPE_SUCCESS;
  sts = bf_map_get(
      &PIPE_PRSR_DATA(
          dev_info->dev_id,
          PIPE_BITMAP_GET_FIRST_SET(&dev_info->profile_info[prof_id]->pipe_bmp),
          gress),
      (unsigned long)prsr_instance_hdl,
      (void **)&instance);
  if (sts != BF_MAP_OK) {
    LOG_TRACE(
        "Fail in getting prsr instance to set parser bin, hdl 0x%x, dev_id "
        "%d, prof_id %d",
        prsr_instance_hdl,
        dev_info->dev_id,
        prof_id);
    return PIPE_UNEXPECTED;
  }
  bin_cfg = &(instance->bin_cfg.tof);

  // shadow
  bin_cfg->prsr_reg_data[i] = data;
  *shadowed = true;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_cache_prsr_val(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t address,
    uint8_t *data,
    int data_len,
    bool *shadowed) {
  /* store into prsr instance shadow */
  int data_struct_size = 0;
  bool po_action_found = false;
  bool tcam_word0_found = false, tcam_word1_found = false;
  bool ea_found = false, ctr_init_found = false;
  bool csum_ctrl0_found = false, csum_ctrl1_found = false;
  struct pipe_mgr_prsr_instance_t *instance;
  struct pipe_mgr_tof_prsr_bin_config *bin_cfg;
  uint8_t gress;
  bf_map_sts_t sts;

  /* First check if this is a parser address. */
  uint64_t iprsr_base = pipe_top_level_pipes_i_prsr_address >> 4;
  uint64_t iprsr_size = pipe_top_level_pipes_i_prsr_array_element_size *
                            pipe_top_level_pipes_i_prsr_array_count >>
                        4;
  uint64_t eprsr_base = pipe_top_level_pipes_e_prsr_address >> 4;
  uint64_t eprsr_size = pipe_top_level_pipes_e_prsr_array_element_size *
                            pipe_top_level_pipes_e_prsr_array_count >>
                        4;
  if ((address >= iprsr_base && address < (iprsr_base + iprsr_size)) ||
      (address >= eprsr_base && address < (eprsr_base + eprsr_size))) {
    *shadowed = true;
  } else {
    *shadowed = false;
    return PIPE_SUCCESS;
  }

  // after initial, all info for every pipe in the prof_id would be exact the
  // same
  for (gress = 0; gress < PIPE_DIR_MAX; gress++) {
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.po_action_addr) {
      po_action_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.word0_addr) {
      tcam_word0_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.word1_addr) {
      tcam_word1_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.ea_row_addr) {
      ea_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.ctr_init_ram_addr) {
      ctr_init_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.po_csum_ctr0_addr) {
      csum_ctrl0_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof.po_csum_ctr1_addr) {
      csum_ctrl1_found = true;
      break;
    }
  }
  if ((!po_action_found) && (!tcam_word0_found) && (!tcam_word1_found) &&
      (!ea_found) && (!ctr_init_found) && (!csum_ctrl0_found) &&
      (!csum_ctrl1_found)) {
    return PIPE_SUCCESS;
  }
  sts = bf_map_get(
      &PIPE_PRSR_DATA(
          dev_info->dev_id,
          PIPE_BITMAP_GET_FIRST_SET(&dev_info->profile_info[prof_id]->pipe_bmp),
          gress),
      (unsigned long)prsr_instance_hdl,
      (void **)&instance);
  if (sts != BF_MAP_OK) {
    LOG_TRACE(
        "Fail in getting prsr instance to set parser bin, hdl 0x%x, dev_id "
        "%d, prof_id %d",
        prsr_instance_hdl,
        dev_info->dev_id,
        prof_id);
    return PIPE_UNEXPECTED;
  }
  bin_cfg = &(instance->bin_cfg.tof);
  if (po_action_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->po_action_data[0]) * TOF_PARSER_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR(
          "Dev %d Invalid parser po action entry, dir %d prsr_map %" PRIu64
          " len %d",
          dev_info->dev_id,
          gress,
          instance->prsr_map,
          data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in prsr instance cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_action_data, data, data_len);

  } else if (tcam_word0_found) {
    /* Make sure dst has enough space */
    if ((PIPE_MGR_TOF_TCAM_WORD_WIDTH * TOF_PARSER_DEPTH) != data_len) {
      LOG_ERROR("Dev %d Invalid parser word0 entry, dir %d prsr_map %" PRIu64
                " len %d",
                dev_info->dev_id,
                gress,
                instance->prsr_map,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    for (int i = 0; i < TOF_NUM_PARSERS; i++) {
      if ((instance->prsr_map & (1u << i)) == 0) continue;
      bf_dev_pipe_t log_pipe_id = 0;
      bf_dev_pipe_t phy_pipe_id = 0;
      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
        PIPE_MGR_MEMCPY(
            PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, i)
                .tof[gress]
                .word0_data,
            data,
            data_len);
      }
    }

  } else if (tcam_word1_found) {
    /* Make sure dst has enough space */
    if ((PIPE_MGR_TOF_TCAM_WORD_WIDTH * TOF_PARSER_DEPTH) != data_len) {
      LOG_ERROR("Dev %d Invalid parser word1 entry, dir %d prsr_map %" PRIu64
                " len %d",
                dev_info->dev_id,
                gress,
                instance->prsr_map,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    for (int i = 0; i < TOF_NUM_PARSERS; i++) {
      if ((instance->prsr_map & (1u << i)) == 0) continue;
      bf_dev_pipe_t log_pipe_id = 0;
      bf_dev_pipe_t phy_pipe_id = 0;
      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
        PIPE_MGR_MEMCPY(
            PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, i)
                .tof[gress]
                .word1_data,
            data,
            data_len);
      }
    }
  } else if (ea_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->ea_row_data[0]) * TOF_PARSER_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser ea entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->ea_row_data, data, data_len);
  } else if (ctr_init_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->ctr_init_ram_data[0]) * TOF_PARSER_INIT_RAM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser ctr init entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->ctr_init_ram_data, data, data_len);
  } else if (csum_ctrl0_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->po_csum_ctr0_data[0]) * TOF_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl0 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr0_data, data, data_len);
  } else if (csum_ctrl1_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->po_csum_ctr1_data[0]) * TOF_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl1 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr1_data, data, data_len);
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_interrupt_set_parser_tcam_shadow(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    bool ing0_egr1,
    int prsr_id,
    int tcam_index,
    uint8_t data_len,
    uint8_t *word0,
    uint8_t *word1) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t phy_pipe_id = 0;
  if (prsr_id < 0 || prsr_id >= TOF_NUM_PARSERS) {
    PIPE_MGR_DBGCHK(prsr_id >= 0);
    PIPE_MGR_DBGCHK(prsr_id < TOF_NUM_PARSERS);
    return PIPE_INVALID_ARG;
  }
  if (tcam_index < 0 || tcam_index >= TOF_PARSER_DEPTH) {
    PIPE_MGR_DBGCHK(tcam_index >= 0);
    PIPE_MGR_DBGCHK(tcam_index < TOF_PARSER_DEPTH);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe_id);
  PIPE_MGR_MEMCPY(PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id)
                      .tof[ing0_egr1]
                      .word0_data[tcam_index],
                  word0,
                  data_len);
  PIPE_MGR_MEMCPY(PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id)
                      .tof[ing0_egr1]
                      .word1_data[tcam_index],
                  word1,
                  data_len);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof_cache_gfm(rmt_dev_info_t *dev_info,
                                     uint32_t log_pipe_mask,
                                     dev_stage_t stage,
                                     uint32_t address,
                                     uint8_t *data,
                                     int data_len) {
  bf_dev_id_t dev = dev_info->dev_id;
  bf_dev_pipe_t logical_pipe = 0, phy_pipe;
  uint32_t base_address = 0;

  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  if (stage >= dev_info->num_active_mau) {
    return PIPE_SUCCESS;
  }
  /* Use logical pipe 0 to the physical pipe and use that for comparision. */
  pipe_status_t sts =
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  if (PIPE_SUCCESS != sts) {
    PIPE_MGR_DBGCHK(0);
    return sts;
  }
  base_address = dev_info->dev_cfg.dir_addr_set_pipe_id(address, phy_pipe);

  if (base_address != PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.base_addr) {
    return PIPE_SUCCESS;
  }
  if (PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data_len != data_len) {
    LOG_ERROR(
        "Invalid GFM entry length, dev %d, phy-pipe %d stage %d len %d (exp "
        "%d)",
        dev,
        phy_pipe,
        stage,
        data_len,
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data_len);
    return PIPE_INVALID_ARG;
  }

  uint32_t num_pipes = dev_info->num_active_pipes;
  for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
    if (~log_pipe_mask & (1u << logical_pipe)) continue;

    pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof.data, data, data_len);
  }
  return PIPE_SUCCESS;
}
