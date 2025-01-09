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
#include "pipe_mgr_tof3_db.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_drv.h"
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
extern pipe_mgr_ctx_t *pipe_mgr_ctx;
pipe_status_t pipe_mgr_tof3_interrupt_db_init(rmt_dev_info_t *dev_info) {
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t mirr_addr;
  for (log_pipe = 0; log_pipe < dev_info->num_active_pipes; log_pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    for (unsigned int s_idx = 0; s_idx < dev_info->num_active_mau; s_idx++) {
      for (int isz = 0; isz < PIPE_MGR_TOF3_IMEM_COUNT; ++isz) {
        uint32_t base = 0;
        uint32_t len = 0;
        switch (isz) {
          case PIPE_MGR_TOF3_IMEM32:
            base = offsetof(
                tof3_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword32[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_subword32[1][1][15][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM32_DARK:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword32[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_dark_subword32[1][1][3][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM32_MOCHA:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword32[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_mocha_subword32[1][1][3][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM16:
            base = offsetof(
                tof3_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword16[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_subword16[1][3][15][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM16_DARK:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword16[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_dark_subword16[1][3][3][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM16_MOCHA:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword16[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_mocha_subword16[1][3][3][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM8:
            base = offsetof(
                tof3_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword8[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_subword8[1][1][15][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM8_DARK:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword8[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_dark_subword8[1][1][3][31]) -
                  base + 4;
            break;
          case PIPE_MGR_TOF3_IMEM8_MOCHA:
            base = offsetof(tof3_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword8[0][0][0][0]);
            len = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .mau[s_idx]
                               .dp.imem.imem_mocha_subword8[1][1][3][31]) -
                  base + 4;
            break;
        }
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof3.imem[isz].base_addr =
            base;
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof3.imem[isz].data_len = len;
      }
      /* GFM: Cache the base address and data len */
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof3.base_addr = offsetof(
          tof3_reg,
          pipes[phy_pipe].mau[s_idx].dp.xbar_hash.hash.galois_field_matrix[0]
                                                                          [0]);
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof3.data_len =
          sizeof(uint32_t) * PIPE_MGR_TOF3_MAX_GFM_ROWS *
          PIPE_MGR_TOF3_MAX_GFM_COLS;
    }

    // mirror table
    for (int entry = 0; entry < PIPE_MGR_TOF3_MIRRTBL_ENTRY_NUMB; entry++) {
      mirr_addr = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .pardereg.dprsrreg.dprsrreg.ho_i[0]
                               .him.mirr_hdr_tbl.entry[entry]
                               .entry_0_16);
      PIPE_DB_DATA(dev).mirrtbl[log_pipe].tof3[0][entry].base_address =
          mirr_addr;
      //      PIPE_INTR_MIRR_DATA(dev, log_pipe).tof3[0][entry].base_address =
      //      mirr_addr;
      mirr_addr = offsetof(tof3_reg,
                           pipes[phy_pipe]
                               .pardereg.dprsrreg.dprsrreg.ho_e[0]
                               .hem.mirr_hdr_tbl.entry[entry]
                               .entry_0_16);
      PIPE_DB_DATA(dev).mirrtbl[log_pipe].tof3[1][entry].base_address =
          mirr_addr;
      //      PIPE_INTR_MIRR_DATA(dev, log_pipe).tof3[1][entry].base_address =
      //      mirr_addr;
    }
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_tof3_prsr_db_init(bf_dev_id_t dev) {
  for (int dir = 0; dir < PIPE_DIR_MAX; dir++) {
    uint64_t po_pipe_base =
        (dir ? tof3_mem_pipes_parde_e_prsr_mem_po_action_row_address
             : tof3_mem_pipes_parde_i_prsr_mem_po_action_row_address);
    uint64_t tcam_base =
        (dir ? tof3_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
             : tof3_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address);
    uint64_t ea_row_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_ml_ea_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_ml_ea_row_address;
    uint64_t ctr_init_ram_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_ml_ctr_init_ram_address
            : tof3_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram_address;
    uint64_t po_csum_ctr0_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_0_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row_address;
    uint64_t po_csum_ctr1_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_1_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row_address;
    uint64_t po_csum_ctr2_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_2_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row_address;
    uint64_t po_csum_ctr3_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_3_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row_address;
    uint64_t po_csum_ctr4_base =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_4_row_address
            : tof3_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row_address;
    uint64_t prsr_step =
        dir ? tof3_mem_pipes_parde_e_prsr_mem_array_element_size
            : tof3_mem_pipes_parde_i_prsr_mem_array_element_size;
    /* Po action ram - Array of 256 elements and each element is 32 bytes */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_action_addr = po_pipe_base >> 4;
    /* tcam  */
    pipe_db[dev]->prsr_base_addr[dir].tof3.tcam_addr = tcam_base >> 4;
    /* ea row */
    pipe_db[dev]->prsr_base_addr[dir].tof3.ea_row_addr = ea_row_base >> 4;
    /* ctr init ram */
    pipe_db[dev]->prsr_base_addr[dir].tof3.ctr_init_ram_addr =
        ctr_init_ram_base >> 4;
    /* po_csum_ctr0 */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_csum_ctr0_addr =
        po_csum_ctr0_base >> 4;
    /* po_csum_ctr1 */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_csum_ctr1_addr =
        po_csum_ctr1_base >> 4;
    /* po_csum_ctr2 */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_csum_ctr2_addr =
        po_csum_ctr2_base >> 4;
    /* po_csum_ctr3 */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_csum_ctr3_addr =
        po_csum_ctr3_base >> 4;
    /* po_csum_ctr4 */
    pipe_db[dev]->prsr_base_addr[dir].tof3.po_csum_ctr4_addr =
        po_csum_ctr4_base >> 4;
    pipe_db[dev]->prsr_base_addr[dir].tof3.prsr_step = prsr_step >> 4;
  }
}
void pipe_mgr_tof3_interrupt_db_cleanup(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  int dev_pipes = dev_info->num_active_pipes;
  int dev_stages = dev_info->num_active_mau;
  for (int p = 0; p < dev_pipes; ++p) {
    if (pipe_db[dev]->imem_db && pipe_db[dev]->imem_db[p]) {
      for (int s = 0; s < dev_stages; ++s) {
        for (int i = 0; i < PIPE_MGR_TOF3_IMEM_COUNT; ++i) {
          if (pipe_db[dev]->imem_db[p][s].tof3.imem[i].data) {
            PIPE_MGR_FREE(pipe_db[dev]->imem_db[p][s].tof3.imem[i].data);
          }
        }
      }
      PIPE_MGR_FREE(pipe_db[dev]->imem_db[p]);
    }
  }
}

pipe_status_t pipe_mgr_tof3_interrupt_cache_imem_val(rmt_dev_info_t *dev_info,
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

  for (i = 0; i < PIPE_MGR_TOF3_IMEM_COUNT; ++i) {
    if (base_address ==
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].base_addr) {
      for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
        if (~log_pipe_mask & (1u << logical_pipe)) continue;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data =
            PIPE_MGR_MALLOC(data_len);
        if (!PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data) {
          return PIPE_NO_SYS_RESOURCES;
        }
        if (data_len !=
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data_len) {
          LOG_ERROR(
              "Dev %d pipe %d stage %d, expected imem length %d but got %d",
              dev_info->dev_id,
              logical_pipe,
              stage,
              PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data_len,
              data_len);
        }
        PIPE_MGR_MEMCPY(
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof3.imem[i].data,
            data,
            data_len);
        /*LOG_TRACE("Dev %d got imem %d for pipe %d stage %d",
                  dev,
                  i,
                  logical_pipe,
                  stage);*/
      }
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_cache_prsr_reg_val(
    rmt_dev_info_t *dev_info,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    profile_id_t prof_id,
    uint32_t address,
    uint32_t data,
    bool *shadowed) {
  (void)dev_info;
  (void)prsr_instance_hdl;
  (void)prof_id;
  (void)data;
  (void)address;
  (void)shadowed;
  // FIXME
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_tof3_cache_prsr_val(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    pipe_prsr_instance_hdl_t prsr_instance_hdl,
    uint64_t address,
    uint8_t *data,
    int data_len,
    bool *shadowed) {
  int data_struct_size = 0;
  bool po_action_found = false;
  bool tcam_word_found = false;
  bool ea_found = false, ctr_init_found = false;
  bool csum_ctrl0_found = false, csum_ctrl1_found = false;
  bool csum_ctrl2_found = false, csum_ctrl3_found = false,
       csum_ctrl4_found = false;
  struct pipe_mgr_prsr_instance_t *instance;
  struct pipe_mgr_tof3_prsr_bin_config *bin_cfg;
  uint8_t gress;
  uint64_t prsr_tmp;
  bf_map_sts_t sts;

  /* First check if this is a parser address. */
  uint64_t iprsr_base = tof3_mem_pipes_parde_i_prsr_mem_address >> 4;
  uint64_t iprsr_size = tof3_mem_pipes_parde_i_prsr_mem_array_element_size *
                            tof3_mem_pipes_parde_i_prsr_mem_array_count >>
                        4;
  uint64_t eprsr_base = tof3_mem_pipes_parde_e_prsr_mem_address >> 4;
  uint64_t eprsr_size = tof3_mem_pipes_parde_e_prsr_mem_array_element_size *
                            tof3_mem_pipes_parde_e_prsr_mem_array_count >>
                        4;
  if ((address >= iprsr_base && address < (iprsr_base + iprsr_size)) ||
      (address >= eprsr_base && address < (eprsr_base + eprsr_size))) {
    *shadowed = true;
  } else {
    *shadowed = false;
    return PIPE_SUCCESS;
  }

  for (gress = 0; gress < PIPE_DIR_MAX; gress++) {
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_action_addr) {
      po_action_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.tcam_addr) {
      tcam_word_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.ea_row_addr) {
      ea_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.ctr_init_ram_addr) {
      ctr_init_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_csum_ctr0_addr) {
      csum_ctrl0_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_csum_ctr1_addr) {
      csum_ctrl1_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_csum_ctr2_addr) {
      csum_ctrl2_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_csum_ctr3_addr) {
      csum_ctrl3_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof3.po_csum_ctr4_addr) {
      csum_ctrl4_found = true;
      break;
    }
  }
  if ((!po_action_found) && (!tcam_word_found) && (!ea_found) &&
      (!ctr_init_found) && (!csum_ctrl0_found) && (!csum_ctrl1_found) &&
      (!csum_ctrl2_found) && (!csum_ctrl3_found) && (!csum_ctrl4_found)) {
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
  bin_cfg = &(instance->bin_cfg.tof3);
  prsr_tmp = instance->prsr_map;
  if (po_action_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->po_action_data[0]) * TOF3_PARSER_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR(
          "Dev %d Invalid parser po action entry, dir %d prsr_map %" PRIu64
          " len %d",
          dev_info->dev_id,
          gress,
          prsr_tmp,
          data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_action_data, data, data_len);
    /*LOG_TRACE("Dev %d Got PrsrPO pipe %d dir %d prsr %d",
              dev,
              logical_pipe,
              gress,
              parser);*/

  } else if (tcam_word_found) {
    /* Make sure dst has enough space */
    if ((PIPE_MGR_TOF3_TCAM_WORD_WIDTH * TOF3_PARSER_DEPTH) != data_len) {
      LOG_ERROR(
          "Dev %d Invalid parser tcam word entry, dir %d prsr_map %" PRIu64
          " len %d",
          dev_info->dev_id,
          gress,
          prsr_tmp,
          data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    for (int i = 0; i < TOF3_NUM_PARSERS / 4; i++) {
      if ((prsr_tmp & (1u << i)) == 0) continue;
      uint32_t log_pipe_id, phy_pipe_id;
      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
        PIPE_MGR_MEMCPY(
            PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, i)
                .tof3[gress]
                .tcam_data,
            data,
            data_len);
      }
    }
  } else if (ea_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->ea_row_data[0]) * TOF3_PARSER_DEPTH;
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
        sizeof(bin_cfg->ctr_init_ram_data[0]) * TOF3_PARSER_INIT_RAM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr0_data[0]) * TOF3_PARSER_CSUM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr1_data[0]) * TOF3_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl1 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr1_data, data, data_len);
  } else if (csum_ctrl2_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->po_csum_ctr2_data[0]) * TOF3_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl0 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr2_data, data, data_len);
  } else if (csum_ctrl3_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->po_csum_ctr3_data[0]) * TOF3_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl1 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr3_data, data, data_len);
  } else if (csum_ctrl4_found) {
    /* Make sure dst has enough space */
    data_struct_size =
        sizeof(bin_cfg->po_csum_ctr4_data[0]) * TOF3_PARSER_CSUM_DEPTH;
    if (data_struct_size != data_len) {
      LOG_ERROR("Dev %d Invalid parser po csum ctrl1 entry, dir %d len %d",
                dev_info->dev_id,
                gress,
                data_len);
      return PIPE_INVALID_ARG;
    }

    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(bin_cfg->po_csum_ctr4_data, data, data_len);
  }
  *shadowed = true;
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_interrupt_set_parser_tcam_shadow(
    rmt_dev_info_t *dev_info,
    bf_dev_pipe_t pipe,
    bool ing0_egr1,
    int prsr_id,
    int tcam_index,
    uint8_t data_len,
    uint8_t *word0,
    uint8_t *word1) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t phy_pipe_id;
  if (prsr_id < 0 || prsr_id >= TOF3_NUM_PARSERS) {
    PIPE_MGR_DBGCHK(prsr_id >= 0);
    PIPE_MGR_DBGCHK(prsr_id < TOF3_NUM_PARSERS);
    return PIPE_INVALID_ARG;
  }
  if (tcam_index < 0 || tcam_index >= TOF3_PARSER_DEPTH) {
    PIPE_MGR_DBGCHK(tcam_index >= 0);
    PIPE_MGR_DBGCHK(tcam_index < TOF3_PARSER_DEPTH);
    return PIPE_INVALID_ARG;
  }
  if (data_len > PIPE_MGR_TOF3_TCAM_WORD_WIDTH / 2) return PIPE_INVALID_ARG;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe_id);
  PIPE_MGR_MEMCPY(PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id / 4)
                      .tof3[ing0_egr1]
                      .tcam_data[tcam_index],
                  word0,
                  data_len);
  PIPE_MGR_MEMCPY(
      &PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id / 4)
           .tof3[ing0_egr1]
           .tcam_data[tcam_index][PIPE_MGR_TOF3_TCAM_WORD_WIDTH / 2],
      word1,
      data_len);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof3_interrupt_cache_mirrtbl_val(
    rmt_dev_info_t *dev_info,
    uint32_t log_pipe_mask,
    uint32_t address,
    uint8_t *data,
    int data_len) {
  bf_dev_id_t dev = dev_info->dev_id;
  int entry, reg_numb;
  bf_dev_pipe_t logical_pipe, phy_pipe;
  uint32_t num_pipes = dev_info->num_active_pipes;
  if (dev >= PIPE_MGR_NUM_DEVICES) {
    return PIPE_INVALID_ARG;
  }

  /* Make sure the address from tofino.bin is well formed pipe address. */
  address = dev_info->dev_cfg.dir_addr_set_pipe_type(address);

  for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
    /* Skip pipes which are NOT part of this profile. */
    if (~log_pipe_mask & (1u << logical_pipe)) continue;
    /* Update the address for the current pipe; the DB is indexed by logical
     * pipe but stores the physical address so set the physical pipe into the
     * address. */
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
    phy_pipe = phy_pipe % BF_SUBDEV_PIPE_COUNT;
    address = dev_info->dev_cfg.dir_addr_set_pipe_id(address, phy_pipe);
    for (entry = 0; entry < PIPE_MGR_TOF3_MIRRTBL_ENTRY_NUMB; entry++) {
      if (PIPE_DB_DATA(dev).mirrtbl[logical_pipe].tof3[0][entry].base_address ==
          (address & 0xFFFFFFC0)) {
        reg_numb = (address & 0x3C) >> 2;
        if (data_len > 4) {
          LOG_ERROR(
              "Dev %d Invalid mirror table entry %d register[%d], phy_pipe %d "
              "ingress, len %d",
              dev,
              entry,
              reg_numb,
              phy_pipe,
              data_len);
          return PIPE_INVALID_ARG;
        }
        PIPE_MGR_MEMCPY(&(PIPE_INTR_MIRR_DATA(dev, logical_pipe)
                              .tof3[0][entry]
                              .data[reg_numb]),
                        data,
                        data_len);
        break;
      } else if (PIPE_DB_DATA(dev)
                     .mirrtbl[logical_pipe]
                     .tof3[1][entry]
                     .base_address == (address & 0xFFFFFFC0)) {
        reg_numb = (address & 0x3C) >> 2;
        if (data_len > 4) {
          LOG_ERROR(
              "Dev %d Invalid mirror table entry %d register[%d], phy_pipe %d "
              "egress, len %d",
              dev,
              entry,
              reg_numb,
              phy_pipe,
              data_len);
          return PIPE_INVALID_ARG;
        }
        PIPE_MGR_MEMCPY(&(PIPE_INTR_MIRR_DATA(dev, logical_pipe)
                              .tof3[1][entry]
                              .data[reg_numb]),
                        data,
                        data_len);
        break;
      }
    }
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_tof3_cache_gfm(rmt_dev_info_t *dev_info,
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

  if (base_address != PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof3.base_addr) {
    return PIPE_SUCCESS;
  }
  if (PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof3.data_len != data_len) {
    LOG_ERROR(
        "Invalid GFM entry length, dev %d, phy-pipe %d stage %d len %d (exp "
        "%d)",
        dev,
        phy_pipe,
        stage,
        data_len,
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof3.data_len);
    return PIPE_INVALID_ARG;
  }

  uint32_t num_pipes = dev_info->num_active_pipes;
  for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
    if (~log_pipe_mask & (1u << logical_pipe)) continue;

    pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof3.data, data, data_len);
    /*LOG_TRACE("Cached GFM entry for dev %d, phy-pipe %d stage %d ",
              dev,
              phy_pipe,
              stage);*/
  }
  return PIPE_SUCCESS;
}

/* Rewrite the GFM parity */
static pipe_status_t pipe_mgr_tof3_rewrite_gfm_parity(pipe_sess_hdl_t sess_hdl,
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

    for (row = 0; row < PIPE_MGR_TOF3_MAX_GFM_ROWS; row++) {
      addr =
          offsetof(tof3_reg,
                   pipes[0]
                       .mau[stage]
                       .dp.xbar_hash.hash
                       .galois_field_matrix[row][PIPE_MGR_TOF3_GFM_PARITY_COL]);

      value = PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
                  .tof3.data[row][PIPE_MGR_TOF3_GFM_PARITY_COL];

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

pipe_status_t pipe_mgr_tof3_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
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

    for (row = 0; row < PIPE_MGR_TOF3_MAX_GFM_ROWS; row++) {
      /* byte0 - 8 bits, byte1 - 8 bits */
      for (bitpos = 0; bitpos < PIPE_MGR_TOF3_SINGLE_GFM_ENTRY_SZ; bitpos++) {
        num_ones = 0;

        for (col = 0; col < PIPE_MGR_TOF3_GFM_PARITY_COL; col++) {
          num_ones +=
              ((PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof3.data[row][col] >>
                bitpos) &
               1u);
        }
        if ((num_ones % 2) == 1) {
          /* Set the parity to 1 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof3.data[row][PIPE_MGR_TOF3_GFM_PARITY_COL] |= (1u << bitpos);
        } else {
          /* Set the parity to 0 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof3.data[row][PIPE_MGR_TOF3_GFM_PARITY_COL] &=
              ~((uint32_t)1u << bitpos);
        }
      }
    }
  }

  if (!skip_write) {
    status =
        pipe_mgr_tof3_rewrite_gfm_parity(sess_hdl, dev_info, pipe_bmp, stage);
  }

  return status;
}

pipe_status_t pipe_mgr_tof3_mirrtbl_write(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  /* Loop over all profiles on this device. */
  for (unsigned int prof = 0; prof < dev_info->num_pipeline_profiles; ++prof) {
    /* Each profile has configuration for a set of pipes and the mirroring
     * configuration is the same across all of these pipes in a profile. */
    pipe_bitmap_t *pbm = &dev_info->profile_info[prof]->pipe_bmp;
    /* We can use any pipe from the profile to look up the mirroring config from
     * our shadow, use the first for simplicity. */
    bf_dev_pipe_t log_pipe = dev_info->profile_info[prof]->lowest_pipe;
    /* Config is kept per direction (ingress=0, egress=1). */
    for (int dir = 0; dir < 2; ++dir) {
      uint32_t slice_step;
      if (dir == 0)
        slice_step =
            offsetof(tof3_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1]) -
            offsetof(tof3_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0]);
      else
        slice_step =
            offsetof(tof3_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1]) -
            offsetof(tof3_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0]);
      /* For each row in the mirroring table. */
      for (int entry = 0; entry < PIPE_MGR_TOF3_MIRRTBL_ENTRY_NUMB; ++entry) {
        uint32_t *data =
            PIPE_INTR_MIRR_DATA(dev_id, log_pipe).tof3[dir][entry].data;
        uint32_t base_addr =
            PIPE_INTR_MIRR_DATA(dev_id, log_pipe).tof3[dir][entry].base_address;
        /* Program it on all four deparser slices. */
        const int dprsr_slice_cnt = 4;
        for (int slice = 0; slice < dprsr_slice_cnt; ++slice) {
          /* The entry is read/written through a wide CSR so write each register
           * in the wide register. */
          for (int r = 0; r < PIPE_MGR_TOF3_MIRRTBL_WORD_WIDTH; ++r) {
            uint32_t addr = base_addr + slice_step * slice + 4 * r;
            uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
            pipe_instr_write_reg_t instr;
            construct_instr_reg_write(dev_id, &instr, addr, data[r]);
            pipe_status_t status = pipe_mgr_drv_ilist_add(
                &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
            if (status != PIPE_SUCCESS) {
              return status;
            }
          }
        }
      }
    }
  }
  return PIPE_SUCCESS;
}
