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
#include "pipe_mgr_tof2_db.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_drv.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof2_regs/tof2_mem_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <tof3_regs/tof3_mem_drv.h>
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

#define sizeof_member(type, member) sizeof(((type *)0)->member)

pipe_status_t pipe_mgr_tof2_interrupt_db_init(rmt_dev_info_t *dev_info) {
  bf_dev_pipe_t phy_pipe = 0, log_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t mirr_addr;
  for (log_pipe = 0; log_pipe < dev_info->num_active_pipes; log_pipe++) {
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);

    for (unsigned int s_idx = 0; s_idx < dev_info->num_active_mau; s_idx++) {
      for (int isz = 0; isz < PIPE_MGR_TOF2_IMEM_COUNT; ++isz) {
        uint32_t base = 0;
        uint32_t len = 0;
        switch (isz) {
          case PIPE_MGR_TOF2_IMEM32:
            base = offsetof(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword32[0][0][0][0]);
            len = sizeof_member(
                tof2_reg, pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword32);
            break;
          case PIPE_MGR_TOF2_IMEM32_DARK:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword32[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_dark_subword32);
            break;
          case PIPE_MGR_TOF2_IMEM32_MOCHA:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword32[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_mocha_subword32);
            break;
          case PIPE_MGR_TOF2_IMEM16:
            base = offsetof(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword16[0][0][0][0]);
            len = sizeof_member(
                tof2_reg, pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword16);
            break;
          case PIPE_MGR_TOF2_IMEM16_DARK:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword16[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_dark_subword16);
            break;
          case PIPE_MGR_TOF2_IMEM16_MOCHA:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword16[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_mocha_subword16);
            break;
          case PIPE_MGR_TOF2_IMEM8:
            base = offsetof(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword8[0][0][0][0]);
            len = sizeof_member(
                tof2_reg, pipes[phy_pipe].mau[s_idx].dp.imem.imem_subword8);
            break;
          case PIPE_MGR_TOF2_IMEM8_DARK:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_dark_subword8[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_dark_subword8);
            break;
          case PIPE_MGR_TOF2_IMEM8_MOCHA:
            base = offsetof(tof2_reg,
                            pipes[phy_pipe]
                                .mau[s_idx]
                                .dp.imem.imem_mocha_subword8[0][0][0][0]);
            len = sizeof_member(
                tof2_reg,
                pipes[phy_pipe].mau[s_idx].dp.imem.imem_mocha_subword8);
            break;
        }
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof2.imem[isz].base_addr =
            base;
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof2.imem[isz].data_len = len;
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof2.imem[isz].data =
            PIPE_MGR_CALLOC(1, len);
        if (!PIPE_INTR_IMEM_DATA(dev, phy_pipe, s_idx).tof2.imem[isz].data) {
          return PIPE_NO_SYS_RESOURCES;
        }
      }
      /* GFM: Cache the base address and data len */
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof2.base_addr = offsetof(
          tof2_reg,
          pipes[phy_pipe].mau[s_idx].dp.xbar_hash.hash.galois_field_matrix[0]
                                                                          [0]);
      PIPE_INTR_GFM_DATA(dev, phy_pipe, s_idx).tof2.data_len =
          sizeof(uint32_t) * PIPE_MGR_TOF2_MAX_GFM_ROWS *
          PIPE_MGR_TOF2_MAX_GFM_COLS;
    }

    // mirror table
    for (int entry = 0; entry < PIPE_MGR_TOF2_MIRRTBL_ENTRY_NUMB; entry++) {
      mirr_addr = offsetof(tof2_reg,
                           pipes[phy_pipe]
                               .pardereg.dprsrreg.dprsrreg.ho_i[0]
                               .him.mirr_hdr_tbl.entry[entry]
                               .entry_0_16);
      PIPE_DB_DATA(dev).mirrtbl[log_pipe].tof2[0][entry].base_address =
          mirr_addr;
      //      PIPE_INTR_MIRR_DATA(dev, log_pipe).tof2[0][entry].base_address =
      //      mirr_addr;
      mirr_addr = offsetof(tof2_reg,
                           pipes[phy_pipe]
                               .pardereg.dprsrreg.dprsrreg.ho_e[0]
                               .hem.mirr_hdr_tbl.entry[entry]
                               .entry_0_16);
      PIPE_DB_DATA(dev).mirrtbl[log_pipe].tof2[1][entry].base_address =
          mirr_addr;
      //      PIPE_INTR_MIRR_DATA(dev, log_pipe).tof2[1][entry].base_address =
      //      mirr_addr;
    }
  }
  return PIPE_SUCCESS;
}

void pipe_mgr_tof2_prsr_db_init(bf_dev_id_t dev) {
  for (int dir = 0; dir < PIPE_DIR_MAX; dir++) {
    uint64_t po_pipe_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_action_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_action_row_address) >>
        4;
    uint64_t tcam_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_tcam_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_ml_tcam_row_address) >>
        4;
    uint64_t ea_row_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_ea_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_ml_ea_row_address) >>
        4;
    uint64_t ctr_init_ram_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_ml_ctr_init_ram_address
             : tof2_mem_pipes_parde_i_prsr_mem_ml_ctr_init_ram_address) >>
        4;
    uint64_t po_csum_ctr0_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_0_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_0_row_address) >>
        4;
    uint64_t po_csum_ctr1_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_1_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_1_row_address) >>
        4;
    uint64_t po_csum_ctr2_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_2_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_2_row_address) >>
        4;
    uint64_t po_csum_ctr3_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_3_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_3_row_address) >>
        4;
    uint64_t po_csum_ctr4_base =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_po_csum_ctrl_4_row_address
             : tof2_mem_pipes_parde_i_prsr_mem_po_csum_ctrl_4_row_address) >>
        4;
    uint64_t prsr_step =
        (dir ? tof2_mem_pipes_parde_e_prsr_mem_array_element_size
             : tof2_mem_pipes_parde_i_prsr_mem_array_element_size) >>
        4;
    /* Po action ram - Array of 256 elements and each element is 32 bytes */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_action_addr = po_pipe_base;
    /* tcam  */
    pipe_db[dev]->prsr_base_addr[dir].tof2.tcam_addr = tcam_base;
    /* ea row */
    pipe_db[dev]->prsr_base_addr[dir].tof2.ea_row_addr = ea_row_base;
    /* ctr init ram */
    pipe_db[dev]->prsr_base_addr[dir].tof2.ctr_init_ram_addr =
        ctr_init_ram_base;
    /* po_csum_ctr0 */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_csum_ctr0_addr =
        po_csum_ctr0_base;
    /* po_csum_ctr1 */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_csum_ctr1_addr =
        po_csum_ctr1_base;
    /* po_csum_ctr2 */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_csum_ctr2_addr =
        po_csum_ctr2_base;
    /* po_csum_ctr3 */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_csum_ctr3_addr =
        po_csum_ctr3_base;
    /* po_csum_ctr4 */
    pipe_db[dev]->prsr_base_addr[dir].tof2.po_csum_ctr4_addr =
        po_csum_ctr4_base;
    pipe_db[dev]->prsr_base_addr[dir].tof2.prsr_step = prsr_step;
  }
}
void pipe_mgr_tof2_interrupt_db_cleanup(rmt_dev_info_t *dev_info) {
  bf_dev_id_t dev = dev_info->dev_id;
  int dev_pipes = dev_info->dev_cfg.num_pipelines;
  int dev_stages = dev_info->num_active_mau;
  for (int p = 0; p < dev_pipes; ++p) {
    if (pipe_db[dev]->imem_db && pipe_db[dev]->imem_db[p]) {
      for (int s = 0; s < dev_stages; ++s) {
        for (int i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
          if (pipe_db[dev]->imem_db[p][s].tof2.imem[i].data) {
            PIPE_MGR_FREE(pipe_db[dev]->imem_db[p][s].tof2.imem[i].data);
          }
        }
      }
      PIPE_MGR_FREE(pipe_db[dev]->imem_db[p]);
    }
  }
}

pipe_status_t pipe_mgr_tof2_interrupt_cache_imem_val(rmt_dev_info_t *dev_info,
                                                     uint32_t log_pipe_mask,
                                                     dev_stage_t stage,
                                                     uint32_t base_address,
                                                     uint8_t *data,
                                                     int data_len,
                                                     bool *shadowed) {
  bf_dev_id_t dev = dev_info->dev_id;
  uint32_t num_pipes = dev_info->num_active_pipes;
  bf_dev_pipe_t logical_pipe = 0, phy_pipe;
  int i;

  /* Map logical pipe 0 to the physical pipe and use that for comparision. */
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
  base_address = dev_info->dev_cfg.dir_addr_set_pipe_id(base_address, phy_pipe);

  *shadowed = false;
  for (i = 0; i < PIPE_MGR_TOF2_IMEM_COUNT; ++i) {
    if (base_address ==
        PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof2.imem[i].base_addr) {
      *shadowed = true;
      for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
        if (~log_pipe_mask & (1u << logical_pipe)) continue;
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
        if (data_len !=
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof2.imem[i].data_len) {
          LOG_ERROR(
              "Dev %d pipe %d stage %d, expected imem length %d but got %d",
              dev_info->dev_id,
              logical_pipe,
              stage,
              PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof2.imem[i].data_len,
              data_len);
        }
        PIPE_MGR_MEMCPY(
            PIPE_INTR_IMEM_DATA(dev, phy_pipe, stage).tof2.imem[i].data,
            data,
            data_len);
      }
    }
  }

  return PIPE_SUCCESS;
}

/* Describes one "chunk" of imem.  A chunk is a section defined by a start
 * offset within a single imem type and a length.  A chunk can be the entire
 * imem type or as small as four bytes.  Note that the start index and the
 * length are always multiples of four since the imem is implemented in 32 bit
 * registers. */
struct imem_chunk {
  enum pipe_mgr_tof2_imem_idx imem_type;
  int start_idx;
  int data_len;
};
/* Describes one "block" of imem.  A block is a collection of chunks that have
 * consecutive chip addresses. */
struct imem_block {
  uint32_t base_addr;
  int chunk_cnt;
  /* Pointer to chunk_cnt number of chunks. */
  struct imem_chunk *chunks;
};
/* Helper to get a uint32_t value out of the imem shadow to use in a CSR write
 * (either direct register write or ilist register write.  This helper is needed
 * because the shadow is a byte array so four elements must be combined in the
 * correct order to get the u32 to write.  Note, this can also be used to get a
 * u32 value to compare to the result of a HW read to imem regsiters. */
static inline uint32_t tof2_imem_get_one(uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
         ((uint32_t)p[3] << 24);
}
/* Helper function to write imem with ilist DMA.
 * Writes all data pointed to by the imem_block pointer to the requested pipe
 * bitmap. */
static pipe_status_t tof2_imem_ilist_wr_one_block(pipe_sess_hdl_t shdl,
                                                  rmt_dev_info_t *dev_info,
                                                  pipe_bitmap_t *pbm,
                                                  struct imem_block *block,
                                                  pipe_imem_data_db_t *imem) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  uint32_t addr = block->base_addr;
  uint32_t stage = dev_info->dev_cfg.pcie_pipe_addr_get_stage(addr);
  for (int chunk_idx = 0; chunk_idx < block->chunk_cnt; ++chunk_idx) {
    struct imem_chunk *chk = block->chunks + chunk_idx;
    for (int i = 0; i < chk->data_len; i += 4) {
      int chk_offset = chk->start_idx + i;
      uint8_t *datap = imem[chk->imem_type].data + chk_offset;
      uint32_t data = tof2_imem_get_one(datap);

      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(dev_id, &instr, addr + i, data);
      pipe_status_t rc = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Dev %d: Failed to post imem instr (%s)", dev_id, pipe_str_err(rc));
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}

/* Helper struct to capture one imem register.  The side, group, alu and word
 * are the four array indexes in the imem CSRs.  Note that word may have a
 * special value of -1 indicating that all words for the side/group/alu are
 * represented. */
struct tof2_one_phv_imem {
  enum pipe_mgr_tof2_imem_idx imem_type;
  int side;
  int group;
  int alu;
  int word;
};
static pipe_status_t tof2_imem_ilist_wr(pipe_sess_hdl_t shdl,
                                        rmt_dev_info_t *dev_info,
                                        pipe_bitmap_t *pbm,
                                        bf_dev_pipe_t phy_pipe,
                                        int stage,
                                        struct tof2_one_phv_imem *reg,
                                        bool dbl_write,
                                        bool atomic,
                                        pipe_imem_data_db_t *imem) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_status_t rc;
  int instr_cnt = dbl_write ? 2 : 1;
  enum pipe_mgr_tof2_imem_idx imem_type = reg->imem_type;
  int side = reg->side;
  int group = reg->group;
  int alu = reg->alu;
  int word = reg->word;
  int word_start = word == -1 ? 0 : word;
  int word_end = word == -1 ? 31 : word;
  int word_cnt = word_end - word_start + 1;
  uint32_t base_addr, imem_base;
  switch (imem_type) {
    case PIPE_MGR_TOF2_IMEM32:
      base_addr = offsetof(
          tof2_reg,
          pipes[0].mau[0].dp.imem.imem_subword32[side][group][alu][word_start]);
      imem_base = offsetof(tof2_reg,
                           pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM32_DARK:
      base_addr =
          offsetof(tof2_reg,
                   pipes[0].mau[0].dp.imem.imem_dark_subword32[side][group][alu]
                                                              [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_dark_subword32[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM32_MOCHA:
      base_addr = offsetof(
          tof2_reg,
          pipes[0].mau[0].dp.imem.imem_mocha_subword32[side][group][alu]
                                                      [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_mocha_subword32[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM16:
      base_addr = offsetof(
          tof2_reg,
          pipes[0].mau[0].dp.imem.imem_subword16[side][group][alu][word_start]);
      imem_base = offsetof(tof2_reg,
                           pipes[0].mau[0].dp.imem.imem_subword16[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM16_DARK:
      base_addr =
          offsetof(tof2_reg,
                   pipes[0].mau[0].dp.imem.imem_dark_subword16[side][group][alu]
                                                              [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_dark_subword16[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM16_MOCHA:
      base_addr = offsetof(
          tof2_reg,
          pipes[0].mau[0].dp.imem.imem_mocha_subword16[side][group][alu]
                                                      [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_mocha_subword16[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM8:
      base_addr = offsetof(
          tof2_reg,
          pipes[0].mau[0].dp.imem.imem_subword8[side][group][alu][word_start]);
      imem_base =
          offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM8_DARK:
      base_addr =
          offsetof(tof2_reg,
                   pipes[0].mau[0].dp.imem.imem_dark_subword8[side][group][alu]
                                                             [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_dark_subword8[0][0][0][0]);
      break;
    case PIPE_MGR_TOF2_IMEM8_MOCHA:
      base_addr =
          offsetof(tof2_reg,
                   pipes[0].mau[0].dp.imem.imem_mocha_subword8[side][group][alu]
                                                              [word_start]);
      imem_base = offsetof(
          tof2_reg, pipes[0].mau[0].dp.imem.imem_mocha_subword8[0][0][0][0]);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  if (atomic) {
    (void)phy_pipe;
    /* Setup the wide bubble. */
    pipe_atomic_mod_csr_instr_t instr;
    int gress = 0; /* Ingress */
    construct_instr_atomic_mod_csr(dev_id, &instr, gress, true, true);
    rc = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
    if (rc != PIPE_SUCCESS) goto err_cleanup;
  }

  for (int i = 0; i < word_cnt; ++i) {
    uint32_t addr = base_addr + 4 * i;
    uint8_t *datap = imem[imem_type].data + (addr - imem_base);
    uint32_t data = tof2_imem_get_one(datap);
    pipe_instr_write_reg_t instr;
    construct_instr_reg_write(dev_id, &instr, addr, data);
    for (int j = 0; j < instr_cnt; ++j) {
      rc = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
      if (rc != PIPE_SUCCESS) goto err_cleanup;
    }
  }

  if (atomic) {
    /* Terminate the atomic-mod-csr. */
    pipe_atomic_mod_csr_instr_t instr;
    int gress = 0; /* Ingress */
    construct_instr_atomic_mod_csr(dev_id, &instr, gress, false, true);
    rc = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, pbm, stage, (uint8_t *)&instr, sizeof instr);
    if (rc != PIPE_SUCCESS) goto err_cleanup;
  }
  return PIPE_SUCCESS;
err_cleanup:
  pipe_mgr_drv_ilist_abort(&shdl);
  return rc;
}
/* Helper function to write imem with block write DMA.
 * Combines the data pointed to be a struct imem_block into a series of DMA
 * write block operations.  The pipe_imem_data_db_t is a pointer to the imem
 * shadow where the data will be sourced using the offsets and lengths from the
 * struct imem_block.  The dma_wait_okay should be false if we cannot service
 * DRs when out of DMA memory such as during init time.  */
static pipe_status_t tof2_imem_blk_wr_one_block(pipe_sess_hdl_t shdl,
                                                rmt_dev_info_t *dev_info,
                                                int log_mask,
                                                struct imem_block *block,
                                                pipe_imem_data_db_t *imem,
                                                bool dma_wait_okay) {
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_mgr_drv_ses_state_t *st =
      pipe_mgr_drv_get_ses_state(&shdl, __func__, __LINE__);
  if (!st) {
    LOG_ERROR("%s : Failed to get ses state", __func__);
    return PIPE_UNEXPECTED;
  }

  pipe_mgr_drv_buf_t *b = NULL;
  int dma_offset = 0;
  int bwr_size = pipe_mgr_drv_buf_size(dev_id, PIPE_MGR_DRV_BUF_BWR);
  int chunk_idx = 0;
  int chnk_offset = block->chunks[chunk_idx].start_idx;
  int chnk_len = block->chunks[chunk_idx].data_len;
  uint32_t base_addr = block->base_addr;
  while (chunk_idx < block->chunk_cnt) {
    struct imem_chunk *chk = block->chunks + chunk_idx;
    /* Allocate a buffer if we don't already have one. */
    if (!b) {
      b = pipe_mgr_drv_buf_alloc(
          st->sid, dev_id, bwr_size, PIPE_MGR_DRV_BUF_BWR, dma_wait_okay);
      if (!b) {
        return PIPE_NO_SYS_RESOURCES;
      }
      dma_offset = 0;
    }
    /* Copy min of DMA size remaining and block size remaining */
    uint8_t *blk_data = imem[chk->imem_type].data + chnk_offset;
    int dma_size_remaining = bwr_size - dma_offset;
    if (chnk_len <= dma_size_remaining) {
      /* Copy until end of block. */
      LOG_TRACE("%s: Adding %s[%d..%d]",
                __func__,
                pipe_mgr_tof2_imem_name(chk->imem_type),
                chnk_offset,
                chnk_offset + chnk_len - 4);
      PIPE_MGR_MEMCPY(b->addr + dma_offset, blk_data, chnk_len);
      dma_offset += chnk_len;
      ++chunk_idx;
      /* Reset block offset if we have another valid chunk */
      if (chunk_idx < block->chunk_cnt) {
        chnk_offset = block->chunks[chunk_idx].start_idx;
        chnk_len = block->chunks[chunk_idx].data_len;
      }
    } else {
      /* Copy as much as we can to fill the DMA buffer. */
      PIPE_MGR_MEMCPY(b->addr + dma_offset, blk_data, dma_size_remaining);
      dma_offset += dma_size_remaining;
      chnk_offset += dma_size_remaining;
      chnk_len -= dma_size_remaining;
      LOG_TRACE("%s: Write to 0x%x..%x (len 0x%x), pipe mask %x, stage %d",
                __func__,
                base_addr,
                base_addr + dma_offset - 4,
                dma_offset,
                log_mask,
                dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr));
      pipe_status_t rc = pipe_mgr_drv_blk_wr(
          &shdl,
          4,
          dma_offset / 4,
          4,
          dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(base_addr),
          log_mask,
          b);
      if (rc != PIPE_SUCCESS) return rc;
      base_addr += dma_offset;
      b = NULL;
      dma_offset = 0;
    }
  }

  /* Push any partially full DMA buffer. */
  if (b) {
    LOG_TRACE("%s: Write to 0x%x..%x (len 0x%x), pipe mask %x, stage %d",
              __func__,
              base_addr,
              base_addr + dma_offset - 4,
              dma_offset,
              log_mask,
              dev_info->dev_cfg.pcie_pipe_addr_get_stage(base_addr));
    pipe_status_t rc = pipe_mgr_drv_blk_wr(
        &shdl,
        4,
        dma_offset / 4,
        4,
        dev_info->dev_cfg.pcie_pipe_addr_to_full_addr(base_addr),
        log_mask,
        b);
    if (rc != PIPE_SUCCESS) return rc;
  }
  return PIPE_SUCCESS;
}

/* Helper to write blocks of imem in a single stage to a set of pipes.
 * This writes the portions of imem which can be safely written with normal
 * DMA write-block or ilist operations. */
static pipe_status_t tof2_imem_wr_blocks(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info,
                                         pipe_bitmap_t *pbm,
                                         int log_mask,
                                         pipe_imem_data_db_t *imem,
                                         bool chip_init) {
  pipe_status_t rc;
  /* How imem is written depends on whether this is at init time or run-time.
   * During run-time there may be traffic and other operations to the MAU using
   * atomic-mod-csr.  At init time we can safely send write-block DMAs as they
   * are more efficient, however at runtime we must keep everything in the ilist
   * path to prevent imem writes through block write DMA from intermixing with
   * an ilist operation on another session doing atomic-mod-csr to the same
   * stage.  Mixing the two may result in the imem writes being added to the
   * atomic-mod-csr operation in the MAU and overflow the MAU's 64 entry amod
   * fifo. */
  if (chip_init) {
    /* Since traffic is not flowing, the following imem sections can be
     * written with block writes as they have consecutive addresses:
     *  0: mocha16[0][0][0][1]...mocha16[1][3][3][31] (0x4000004..0x4000FFC)
     *  1: dark16 (all of it)                         (0x4001000..0x4001FFC)
     *  2: mocha32 (all of it)                        (0x4002000..0x40027FC)
     *  3: mocha8 (all of it)                         (0x4002800..0x4002FFC)
     *  4: dark32 (all of it)                         (0x4003000..0x40037FC)
     *  5: dark8 (all of it)                          (0x4003800..0x4003FFC)
     * There is an address hole, so imem16 needs a new write-block
     *  0: norm16 (all of it)                         (0x4008000..0x400BFFC)
     *  1: norm32[0][0][0][0]...norm32[0][0][5][31]   (0x400C000..0x400C2FC)
     * Then norm32[0][0][6][0] must be written with a double-write ilist.
     * Next block write section is:
     *  0: norm32[0][0][6][1]...norm32[0][1][7][31]   (0x400C304..0x400CBFC)
     * Then norm32[0][1][8][0] must be written with a double-write ilist.
     * Next block write section is:
     *  0: norm32[0][1][8][1]...norm32[1][1][15][31]  (0x400CC04..0x400DFFC)
     *  1: norm8 (all of it)                          (0x400E000..0x400FFFC)
     */
    struct imem_chunk chnks_0[6];
    struct imem_chunk chnks_1[2];
    struct imem_chunk chnks_2[1];
    struct imem_chunk chnks_3[2];
    chnks_0[0].imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA;
    chnks_0[0].start_idx = 4;
    chnks_0[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[1][3][3][31]) -
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[0][0][0][1]) +
        4;
    chnks_0[1].imem_type = PIPE_MGR_TOF2_IMEM16_DARK;
    chnks_0[1].start_idx = 0;
    chnks_0[1].data_len = imem[PIPE_MGR_TOF2_IMEM16_DARK].data_len;
    chnks_0[2].imem_type = PIPE_MGR_TOF2_IMEM32_MOCHA;
    chnks_0[2].start_idx = 0;
    chnks_0[2].data_len = imem[PIPE_MGR_TOF2_IMEM32_MOCHA].data_len;
    chnks_0[3].imem_type = PIPE_MGR_TOF2_IMEM8_MOCHA;
    chnks_0[3].start_idx = 0;
    chnks_0[3].data_len = imem[PIPE_MGR_TOF2_IMEM8_MOCHA].data_len;
    chnks_0[4].imem_type = PIPE_MGR_TOF2_IMEM32_DARK;
    chnks_0[4].start_idx = 0;
    chnks_0[4].data_len = imem[PIPE_MGR_TOF2_IMEM32_DARK].data_len;
    chnks_0[5].imem_type = PIPE_MGR_TOF2_IMEM8_DARK;
    chnks_0[5].start_idx = 0;
    chnks_0[5].data_len = imem[PIPE_MGR_TOF2_IMEM8_DARK].data_len;

    chnks_1[0].imem_type = PIPE_MGR_TOF2_IMEM16;
    chnks_1[0].start_idx = 0;
    chnks_1[0].data_len = imem[PIPE_MGR_TOF2_IMEM16].data_len;
    chnks_1[1].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_1[1].start_idx = 0;
    chnks_1[1].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[0][0][5][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]) +
        4;

    chnks_2[0].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_2[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][6][1]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]);
    chnks_2[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[0][1][7][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][6][1]) +
        4;

    chnks_3[0].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_3[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][1][8][1]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]);
    chnks_3[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[1][1][15][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][1][8][1]) +
        4;
    chnks_3[1].imem_type = PIPE_MGR_TOF2_IMEM8;
    chnks_3[1].start_idx = 0;
    chnks_3[1].data_len = imem[PIPE_MGR_TOF2_IMEM8].data_len;

    struct imem_block b0;
    b0.base_addr =
        imem[PIPE_MGR_TOF2_IMEM16_MOCHA].base_addr + chnks_0[0].start_idx;
    b0.chunk_cnt = 6;
    b0.chunks = chnks_0;
    struct imem_block b1;
    b1.base_addr = imem[PIPE_MGR_TOF2_IMEM16].base_addr + chnks_1[0].start_idx;
    b1.chunk_cnt = 2;
    b1.chunks = chnks_1;
    struct imem_block b2;
    b2.base_addr = imem[PIPE_MGR_TOF2_IMEM32].base_addr + chnks_2[0].start_idx;
    b2.chunk_cnt = 1;
    b2.chunks = chnks_2;
    struct imem_block b3;
    b3.base_addr = imem[PIPE_MGR_TOF2_IMEM32].base_addr + chnks_3[0].start_idx;
    b3.chunk_cnt = 2;
    b3.chunks = chnks_3;
    rc = tof2_imem_blk_wr_one_block(
        shdl, dev_info, log_mask, &b0, imem, !chip_init);
    if (rc != PIPE_SUCCESS) return rc;
    rc = tof2_imem_blk_wr_one_block(
        shdl, dev_info, log_mask, &b1, imem, !chip_init);
    if (rc != PIPE_SUCCESS) return rc;
    rc = tof2_imem_blk_wr_one_block(
        shdl, dev_info, log_mask, &b2, imem, !chip_init);
    if (rc != PIPE_SUCCESS) return rc;
    rc = tof2_imem_blk_wr_one_block(
        shdl, dev_info, log_mask, &b3, imem, !chip_init);
    if (rc != PIPE_SUCCESS) return rc;
  } else {
    /* Since traffic is flowing, the following imem sections can be
     * written as blocks:
     *   0: mocha16[0][0][0][1]...mocha16[1][2][0][31]
     * Then mocha16[1][2][1][0..31] must be written with an atomic ilist.
     *   0: mocha16[1][2][2][0]...mocha16[1][3][3][31]
     *   1: dark16 (all of it)
     *   2: mocha32 (all of it)
     *   3: mocha8 (all of it)
     *   4: dark32 (all of it)
     *   5: dark8 (all of it)
     * There is an address hole, so imem16 needs a new block.
     *   0: norm16 (all of it)
     *   1: norm32[0][0][0][0] ... norm32[0][0][5][31]
     * Then norm32[0][0][6][0] must be written with a double-write atomic ilist.
     * Next block write section is:
     *   0: norm32[0][0][6][1] ... norm32[0][1][7][31]
     * Then norm32[0][1][8][0] must be written with a double-write atomic ilist.
     * Next block write section is:
     *   0: norm32[0][1][8][1] ... norm32[1][1][15][31]
     *   1: norm8[0][0][0][0] ... norm8[1][0][4][31]
     * Then norm8[1][0][5][0..31] must be written with an atomic ilist.
     * Next block write section is:
     *   0: norm8[1][0][6][0] ... norm8[1][0][8][31]
     * Then norm8[1][0][9][0..31] must be written with an atomic ilist.
     * Next block write section is:
     *   0: norm8[1][0][10][0] ... norm8[1][1][15][31]
     */
    struct imem_chunk chnks_0[1];
    struct imem_chunk chnks_1[6];
    struct imem_chunk chnks_2[2];
    struct imem_chunk chnks_3[1];
    struct imem_chunk chnks_4[2];
    struct imem_chunk chnks_5[1];
    struct imem_chunk chnks_6[1];
    chnks_0[0].imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA;
    chnks_0[0].start_idx = 4;
    chnks_0[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[1][2][0][31]) -
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[0][0][0][1]) +
        4;

    chnks_1[0].imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA;
    chnks_1[0].start_idx =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[1][2][2][0]) -
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[0][0][0][0]);
    chnks_1[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[1][3][3][31]) -
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_mocha_subword16[1][2][2][0]) +
        4;
    chnks_1[1].imem_type = PIPE_MGR_TOF2_IMEM16_DARK;
    chnks_1[1].start_idx = 0;
    chnks_1[1].data_len = imem[PIPE_MGR_TOF2_IMEM16_DARK].data_len;
    chnks_1[2].imem_type = PIPE_MGR_TOF2_IMEM32_MOCHA;
    chnks_1[2].start_idx = 0;
    chnks_1[2].data_len = imem[PIPE_MGR_TOF2_IMEM32_MOCHA].data_len;
    chnks_1[3].imem_type = PIPE_MGR_TOF2_IMEM8_MOCHA;
    chnks_1[3].start_idx = 0;
    chnks_1[3].data_len = imem[PIPE_MGR_TOF2_IMEM8_MOCHA].data_len;
    chnks_1[4].imem_type = PIPE_MGR_TOF2_IMEM32_DARK;
    chnks_1[4].start_idx = 0;
    chnks_1[4].data_len = imem[PIPE_MGR_TOF2_IMEM32_DARK].data_len;
    chnks_1[5].imem_type = PIPE_MGR_TOF2_IMEM8_DARK;
    chnks_1[5].start_idx = 0;
    chnks_1[5].data_len = imem[PIPE_MGR_TOF2_IMEM8_DARK].data_len;

    chnks_2[0].imem_type = PIPE_MGR_TOF2_IMEM16;
    chnks_2[0].start_idx = 0;
    chnks_2[0].data_len = imem[PIPE_MGR_TOF2_IMEM16].data_len;
    chnks_2[1].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_2[1].start_idx = 0;
    chnks_2[1].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[0][0][5][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]) +
        4;

    chnks_3[0].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_3[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][6][1]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]);
    chnks_3[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[0][1][7][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][6][1]) +
        4;

    chnks_4[0].imem_type = PIPE_MGR_TOF2_IMEM32;
    chnks_4[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][1][8][1]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][0][0][0]);
    chnks_4[0].data_len =
        offsetof(tof2_reg,
                 pipes[0].mau[0].dp.imem.imem_subword32[1][1][15][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword32[0][1][8][1]) +
        4;
    chnks_4[1].imem_type = PIPE_MGR_TOF2_IMEM8;
    chnks_4[1].start_idx = 0;
    chnks_4[1].data_len = imem[PIPE_MGR_TOF2_IMEM8].data_len;
    offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][4][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[0][0][0][0]) +
        4;

    chnks_5[0].imem_type = PIPE_MGR_TOF2_IMEM8;
    chnks_5[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][6][0]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[0][0][0][0]);
    chnks_5[0].data_len = imem[PIPE_MGR_TOF2_IMEM8].data_len;
    offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][8][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][6][0]) +
        4;

    chnks_6[0].imem_type = PIPE_MGR_TOF2_IMEM8;
    chnks_6[0].start_idx =
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][10][0]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[0][0][0][0]);
    chnks_6[0].data_len = imem[PIPE_MGR_TOF2_IMEM8].data_len;
    offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][1][15][31]) -
        offsetof(tof2_reg, pipes[0].mau[0].dp.imem.imem_subword8[1][0][10][0]) +
        4;

    struct imem_block b[7];
    b[0].base_addr =
        imem[PIPE_MGR_TOF2_IMEM16_MOCHA].base_addr + chnks_0[0].start_idx;
    b[0].chunk_cnt = 1;
    b[0].chunks = chnks_0;
    b[1].base_addr =
        imem[PIPE_MGR_TOF2_IMEM16_MOCHA].base_addr + chnks_1[0].start_idx;
    b[1].chunk_cnt = 6;
    b[1].chunks = chnks_1;
    b[2].base_addr =
        imem[PIPE_MGR_TOF2_IMEM16].base_addr + chnks_2[0].start_idx;
    b[2].chunk_cnt = 2;
    b[2].chunks = chnks_2;
    b[3].base_addr =
        imem[PIPE_MGR_TOF2_IMEM32].base_addr + chnks_3[0].start_idx;
    b[3].chunk_cnt = 1;
    b[3].chunks = chnks_3;
    b[4].base_addr =
        imem[PIPE_MGR_TOF2_IMEM32].base_addr + chnks_4[0].start_idx;
    b[4].chunk_cnt = 2;
    b[4].chunks = chnks_4;
    b[5].base_addr = imem[PIPE_MGR_TOF2_IMEM8].base_addr + chnks_5[0].start_idx;
    b[5].chunk_cnt = 1;
    b[5].chunks = chnks_5;
    b[6].base_addr = imem[PIPE_MGR_TOF2_IMEM8].base_addr + chnks_6[0].start_idx;
    b[6].chunk_cnt = 1;
    b[6].chunks = chnks_6;
    for (size_t i = 0; i < sizeof b / sizeof b[0]; ++i) {
      rc = tof2_imem_ilist_wr_one_block(shdl, dev_info, pbm, &b[i], imem);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Dev %d failed to write block %d, pipe-mask %x, %s",
                  __func__,
                  __LINE__,
                  dev_info->dev_id,
                  (int)i,
                  log_mask,
                  pipe_str_err(rc));
        pipe_mgr_drv_ilist_abort(&shdl);
        return rc;
      }
    }
  }
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_tof2_imem_write(pipe_sess_hdl_t shdl,
                                       rmt_dev_info_t *dev_info,
                                       bf_dev_pipe_t phy_pipe_filter,
                                       int stage_filter,
                                       bool chip_init) {
  pipe_status_t rc;
  bf_dev_id_t dev_id = dev_info->dev_id;

  /* For each pipeline profile load the imem data to the pipes belonging to that
   * profile.  We can use the data from any one of the pipes since imem is
   * identical across all pipes in the same profile. */
  for (uint32_t prof = 0; prof < dev_info->num_pipeline_profiles; ++prof) {
    bf_dev_pipe_t log_pipe = dev_info->profile_info[prof]->lowest_pipe;
    bf_dev_pipe_t phy_pipe = log_pipe;
    pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe, &phy_pipe);
    pipe_bitmap_t pipe_bmp;
    int log_pipe_mask = 0;
    unsigned int p = 0;
    PIPE_BITMAP_INIT(&pipe_bmp, PIPE_BMP_SIZE);
    PIPE_BITMAP_ITER(&dev_info->profile_info[prof]->pipe_bmp, p) {
      bf_dev_pipe_t tmp_phy_pipe = p;
      pipe_mgr_map_pipe_id_log_to_phy(dev_info, p, &tmp_phy_pipe);
      if (phy_pipe_filter == BF_DEV_PIPE_ALL ||
          phy_pipe_filter == tmp_phy_pipe) {
        log_pipe_mask |= 1u << p;
        PIPE_BITMAP_SET(&pipe_bmp, p);
      }
    }
    if (!log_pipe_mask) continue;

    for (uint8_t stg = 0; stg < dev_info->num_active_mau; ++stg) {
      /* If the caller requested only one stage (for example correcting an imem
       * parity error in a specific pipe and stage) then skip all other stages.
       */
      if (stage_filter != -1) {
        if (stage_filter != (int)stg) {
          continue;
        }
      }
      LOG_TRACE(
          "%s: Dev %d profile %d pipe_mask %x (first %d) stage %d, loading "
          "imem for %s",
          __func__,
          dev_id,
          prof,
          log_pipe_mask,
          log_pipe,
          stg,
          chip_init ? "init" : "runtime");
      pipe_imem_data_db_t *imem =
          PIPE_INTR_IMEM_DATA(dev_id, phy_pipe, stg).tof2.imem;
      /* Block write as much of imem as we can first then finish up with ilist
       * writes afterwards.  Note the sections which can be block written depend
       * on whether traffic is flowing or not, hence the check on chip_init. */
      rc = tof2_imem_wr_blocks(
          shdl, dev_info, &pipe_bmp, log_pipe_mask, imem, chip_init);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s:%d Dev %d pipe mask %x stage %d error %s loading imem",
                  __func__,
                  __LINE__,
                  dev_id,
                  log_pipe_mask,
                  stg,
                  pipe_str_err(rc));
        return rc;
      }

      /* The following three imem registers are always written with ilist as
       * double writes:
       *  - mocha16[0][0][0][0]
       *  - norm32[0][0][6][0]
       *  - norm32[0][1][8][0] */
      struct tof2_one_phv_imem one_off_wrs[3];
      one_off_wrs[0].imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA;
      one_off_wrs[0].side = 0;
      one_off_wrs[0].group = 0;
      one_off_wrs[0].alu = 0;
      one_off_wrs[0].word = 0;
      one_off_wrs[1].imem_type = PIPE_MGR_TOF2_IMEM32;
      one_off_wrs[1].side = 0;
      one_off_wrs[1].group = 0;
      one_off_wrs[1].alu = 6;
      one_off_wrs[1].word = 0;
      one_off_wrs[2].imem_type = PIPE_MGR_TOF2_IMEM32;
      one_off_wrs[2].side = 0;
      one_off_wrs[2].group = 1;
      one_off_wrs[2].alu = 8;
      one_off_wrs[2].word = 0;

      bool atomic = !chip_init;
      for (size_t i = 0; i < sizeof one_off_wrs / sizeof one_off_wrs[0]; ++i) {
        rc = tof2_imem_ilist_wr(shdl,
                                dev_info,
                                &pipe_bmp,
                                phy_pipe,
                                stg,
                                &one_off_wrs[i],
                                true, /* Double write */
                                atomic,
                                imem);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR("%s:%d Dev %d pipe mask %x stage %d error %s loading imem",
                    __func__,
                    __LINE__,
                    dev_id,
                    log_pipe_mask,
                    stg,
                    pipe_str_err(rc));
        }
      }
      /* Furthermore, if traffic is running the following sections must be
       * written with wide bubbles to ensure there are no reads to these
       * registers in the cycle immediately following the write to the register:
       *  - norm8[1][0][5][0..31]
       *  - norm8[1][0][9][0..31]
       *  - mocha16[1][2][1][0..31]
       */
      if (!chip_init) {
        struct tof2_one_phv_imem phv_grps[3];
        phv_grps[0].imem_type = PIPE_MGR_TOF2_IMEM8;
        phv_grps[0].side = 1;
        phv_grps[0].group = 0;
        phv_grps[0].alu = 5;
        phv_grps[0].word = -1;
        phv_grps[1].imem_type = PIPE_MGR_TOF2_IMEM8;
        phv_grps[1].side = 1;
        phv_grps[1].group = 0;
        phv_grps[1].alu = 9;
        phv_grps[1].word = -1;
        phv_grps[1].imem_type = PIPE_MGR_TOF2_IMEM16_MOCHA;
        phv_grps[1].side = 1;
        phv_grps[1].group = 2;
        phv_grps[1].alu = 1;
        phv_grps[1].word = -1;
        for (int i = 0; i < 3; ++i) {
          rc = tof2_imem_ilist_wr(shdl,
                                  dev_info,
                                  &pipe_bmp,
                                  phy_pipe,
                                  stg,
                                  &phv_grps[i],
                                  false, /* Single write */
                                  atomic,
                                  imem);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "%s:%d Dev %d pipe mask %x stage %d error %s loading imem",
                __func__,
                __LINE__,
                dev_id,
                log_pipe_mask,
                stg,
                pipe_str_err(rc));
          }
        }
      }
    } /* For each stage */
  }   /* For each profile */

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_cache_prsr_reg_val(
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
  /* No need to cache parser register values as tof2 multi-parser is not yet
   * supported. */
  return PIPE_SUCCESS;
}
pipe_status_t pipe_mgr_tof2_cache_prsr_val(
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
  struct pipe_mgr_tof2_prsr_bin_config *bin_cfg;
  uint8_t gress;
  uint64_t prsr_tmp;
  bf_map_sts_t sts;

  /* First check if this is a parser address. */
  uint64_t iprsr_base = tof2_mem_pipes_parde_i_prsr_mem_address >> 4;
  uint64_t iprsr_size = tof2_mem_pipes_parde_i_prsr_mem_array_element_size *
                            tof2_mem_pipes_parde_i_prsr_mem_array_count >>
                        4;
  uint64_t eprsr_base = tof2_mem_pipes_parde_e_prsr_mem_address >> 4;
  uint64_t eprsr_size = tof2_mem_pipes_parde_e_prsr_mem_array_element_size *
                            tof2_mem_pipes_parde_e_prsr_mem_array_count >>
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
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_action_addr) {
      po_action_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.tcam_addr) {
      tcam_word_found = true;
      break;
    }
    if (address == PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.ea_row_addr) {
      ea_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.ctr_init_ram_addr) {
      ctr_init_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_csum_ctr0_addr) {
      csum_ctrl0_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_csum_ctr1_addr) {
      csum_ctrl1_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_csum_ctr2_addr) {
      csum_ctrl2_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_csum_ctr3_addr) {
      csum_ctrl3_found = true;
      break;
    }
    if (address ==
        PIPE_PRSR_ADDR(dev_info->dev_id, gress).tof2.po_csum_ctr4_addr) {
      csum_ctrl4_found = true;
      break;
    }
  }
  if ((!po_action_found) && (!tcam_word_found) && (!ea_found) &&
      (!ctr_init_found) && (!csum_ctrl0_found) && (!csum_ctrl1_found) &&
      (!csum_ctrl2_found) && (!csum_ctrl3_found) && (!csum_ctrl4_found)) {
    return PIPE_SUCCESS;
  }
  bf_dev_pipe_t prof_first_pipe = dev_info->profile_info[prof_id]->lowest_pipe;
  bf_map_t *prsr_inst_map =
      &PIPE_PRSR_DATA(dev_info->dev_id, prof_first_pipe, gress);
  sts = bf_map_get(
      prsr_inst_map, (unsigned long)prsr_instance_hdl, (void **)&instance);
  if (sts != BF_MAP_OK) {
    LOG_ERROR(
        "Fail in getting prsr instance to set parser bin, hdl 0x%x, dev_id "
        "%d, prof_id %d",
        prsr_instance_hdl,
        dev_info->dev_id,
        prof_id);
    return PIPE_UNEXPECTED;
  }
  bin_cfg = &(instance->bin_cfg.tof2);
  prsr_tmp = instance->prsr_map;
  if (po_action_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->po_action_data[0]) * TOF2_PARSER_DEPTH;
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
    if ((PIPE_MGR_TOF2_TCAM_WORD_WIDTH * TOF2_PARSER_DEPTH) != data_len) {
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
    for (int i = 0; i < TOF2_NUM_PARSERS / 4; i++) {
      if ((prsr_tmp & (1u << i)) == 0) continue;
      bf_dev_pipe_t log_pipe_id = 0;
      bf_dev_pipe_t phy_pipe_id = 0;
      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        pipe_mgr_map_pipe_id_log_to_phy(dev_info, log_pipe_id, &phy_pipe_id);
        PIPE_MGR_MEMCPY(
            PIPE_INTR_PRSR_TCAM_DATA(dev_info->dev_id, phy_pipe_id, i)
                .tof2[gress]
                .tcam_data,
            data,
            data_len);
      }
    }
  } else if (ea_found) {
    /* Make sure dst has enough space */
    data_struct_size = sizeof(bin_cfg->ea_row_data[0]) * TOF2_PARSER_DEPTH;
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
        sizeof(bin_cfg->ctr_init_ram_data[0]) * TOF2_PARSER_INIT_RAM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr0_data[0]) * TOF2_PARSER_CSUM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr1_data[0]) * TOF2_PARSER_CSUM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr2_data[0]) * TOF2_PARSER_CSUM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr3_data[0]) * TOF2_PARSER_CSUM_DEPTH;
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
        sizeof(bin_cfg->po_csum_ctr4_data[0]) * TOF2_PARSER_CSUM_DEPTH;
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

pipe_status_t pipe_mgr_tof2_interrupt_set_parser_tcam_shadow(
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
  if (prsr_id < 0 || prsr_id >= TOF2_NUM_PARSERS) {
    PIPE_MGR_DBGCHK(prsr_id >= 0);
    PIPE_MGR_DBGCHK(prsr_id < TOF2_NUM_PARSERS);
    return PIPE_INVALID_ARG;
  }
  if (tcam_index < 0 || tcam_index >= TOF2_PARSER_DEPTH) {
    PIPE_MGR_DBGCHK(tcam_index >= 0);
    PIPE_MGR_DBGCHK(tcam_index < TOF2_PARSER_DEPTH);
    return PIPE_INVALID_ARG;
  }
  if (data_len > PIPE_MGR_TOF2_TCAM_WORD_WIDTH / 2) return PIPE_INVALID_ARG;
  pipe_mgr_map_pipe_id_log_to_phy(dev_info, pipe, &phy_pipe_id);
  PIPE_MGR_MEMCPY(PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id / 4)
                      .tof2[ing0_egr1]
                      .tcam_data[tcam_index],
                  word0,
                  data_len);
  PIPE_MGR_MEMCPY(
      &PIPE_INTR_PRSR_TCAM_DATA(dev, phy_pipe_id, prsr_id / 4)
           .tof2[ing0_egr1]
           .tcam_data[tcam_index][PIPE_MGR_TOF2_TCAM_WORD_WIDTH / 2],
      word1,
      data_len);
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_interrupt_cache_mirrtbl_val(
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
    address = dev_info->dev_cfg.dir_addr_set_pipe_id(address, phy_pipe);
    for (entry = 0; entry < PIPE_MGR_TOF2_MIRRTBL_ENTRY_NUMB; entry++) {
      if (PIPE_DB_DATA(dev).mirrtbl[logical_pipe].tof2[0][entry].base_address ==
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
                              .tof2[0][entry]
                              .data[reg_numb]),
                        data,
                        data_len);
        break;
      } else if (PIPE_DB_DATA(dev)
                     .mirrtbl[logical_pipe]
                     .tof2[1][entry]
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
                              .tof2[1][entry]
                              .data[reg_numb]),
                        data,
                        data_len);
        break;
      }
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_mirrtbl_write(pipe_sess_hdl_t shdl,
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
            offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[1]) -
            offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_i[0]);
      else
        slice_step =
            offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[1]) -
            offsetof(tof2_reg, pipes[0].pardereg.dprsrreg.dprsrreg.ho_e[0]);
      /* For each row in the mirroring table. */
      for (int entry = 0; entry < PIPE_MGR_TOF2_MIRRTBL_ENTRY_NUMB; ++entry) {
        uint32_t *data =
            PIPE_INTR_MIRR_DATA(dev_id, log_pipe).tof2[dir][entry].data;
        uint32_t base_addr =
            PIPE_INTR_MIRR_DATA(dev_id, log_pipe).tof2[dir][entry].base_address;
        /* Program it on all four deparser slices. */
        const int dprsr_slice_cnt = 4;
        for (int slice = 0; slice < dprsr_slice_cnt; ++slice) {
          /* The entry is read/written through a wide CSR so write each register
           * in the wide register. */
          for (int r = 0; r < PIPE_MGR_TOF2_MIRRTBL_WORD_WIDTH; ++r) {
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

pipe_status_t pipe_mgr_tof2_cache_gfm(rmt_dev_info_t *dev_info,
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

  if (base_address != PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof2.base_addr) {
    return PIPE_SUCCESS;
  }
  if (PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof2.data_len != data_len) {
    LOG_ERROR(
        "Invalid GFM entry length, dev %d, phy-pipe %d stage %d len %d (exp "
        "%d)",
        dev,
        phy_pipe,
        stage,
        data_len,
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof2.data_len);
    return PIPE_INVALID_ARG;
  }

  uint32_t num_pipes = dev_info->num_active_pipes;
  for (logical_pipe = 0; logical_pipe < num_pipes; ++logical_pipe) {
    if (~log_pipe_mask & (1u << logical_pipe)) continue;

    pipe_mgr_map_pipe_id_log_to_phy(dev_info, logical_pipe, &phy_pipe);
    /* Copy over the data in cache */
    PIPE_MGR_MEMCPY(
        PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof2.data, data, data_len);
    /*LOG_TRACE("Cached GFM entry for dev %d, phy-pipe %d stage %d ",
              dev,
              phy_pipe,
              stage);*/
  }
  return PIPE_SUCCESS;
}

/* Rewrite the seed */
static pipe_status_t pipe_mgr_tof2_rewrite_seed(pipe_sess_hdl_t sess_hdl,
                                                rmt_dev_info_t *dev_info,
                                                pipe_bitmap_t *pipe_bmp,
                                                dev_stage_t stage) {
  pipe_status_t status = PIPE_SUCCESS;
  bf_dev_pipe_t log_pipe = 0;
  uint32_t addr = 0, value = 0;
  pipe_instr_write_reg_t instr;
  bf_dev_id_t dev = dev_info->dev_id;
  pipe_bitmap_t local_pipe_bmp;

  PIPE_BITMAP_INIT(&local_pipe_bmp, PIPE_BMP_SIZE);

  /* We do not know if features using GFM are symmetric or not,
     so rewrite all pipes one by one
  */
  PIPE_BITMAP_ITER(pipe_bmp, log_pipe) {
    PIPE_BITMAP_CLR_ALL(&local_pipe_bmp);
    PIPE_BITMAP_SET(&local_pipe_bmp, log_pipe);

    addr = offsetof(
        tof2_reg,
        pipes[0]
            .mau[stage]
            .dp.xbar_hash.hash.hash_seed[PIPE_MGR_TOF2_SEED_PARITY_COL]);

    value = PIPE_SEL_SHADOW_DB(dev)
                ->seed_db[log_pipe][stage]
                .hash_seed[PIPE_MGR_TOF2_SEED_PARITY_COL];

    construct_instr_reg_write(dev, &instr, addr, value);

    status = pipe_mgr_drv_ilist_add(&sess_hdl,
                                    dev_info,
                                    &local_pipe_bmp,
                                    stage,
                                    (uint8_t *)&instr,
                                    sizeof instr);

    if (status != PIPE_SUCCESS) {
      LOG_ERROR(
          "Failed to add seed parity to instruction list, stage %d, "
          "rc = (%d), dev_id %d",
          stage,
          status,
          dev);
      return status;
    }
  }

  return PIPE_SUCCESS;
}

pipe_status_t pipe_mgr_tof2_recalc_write_seed(pipe_sess_hdl_t sess_hdl,
                                              rmt_dev_info_t *dev_info,
                                              pipe_bitmap_t *pipe_bmp,
                                              dev_stage_t stage) {
  uint32_t col = 0, bitpos = 0, num_ones = 0;
  bf_dev_pipe_t log_pipe = 0;
  bf_dev_id_t dev = dev_info->dev_id;

  PIPE_BITMAP_ITER(pipe_bmp, log_pipe) {
    /* Each hash seed register is 8 bits, each bit corresponding to its output
     * hash group */
    for (bitpos = 0; bitpos < PIPE_MGR_TOF2_OUTPUT_PARITY_GROUPS; bitpos++) {
      num_ones = 0;

      for (col = 0; col < PIPE_MGR_TOF2_SEED_PARITY_COL; col++) {
        num_ones += ((PIPE_SEL_SHADOW_DB(dev)
                          ->seed_db[log_pipe][stage]
                          .hash_seed[col] >>
                      bitpos) &
                     1u);
      }
      if ((num_ones % 2) == 1) {
        /* Set the parity to 1 */
        PIPE_SEL_SHADOW_DB(dev)
            ->seed_db[log_pipe][stage]
            .hash_seed[PIPE_MGR_TOF2_SEED_PARITY_COL] |= (1u << bitpos);
      } else {
        /* Set the parity to 0 */
        PIPE_SEL_SHADOW_DB(dev)
            ->seed_db[log_pipe][stage]
            .hash_seed[PIPE_MGR_TOF2_SEED_PARITY_COL] &=
            ~((uint8_t)1u << bitpos);
      }
    }
  }

  return pipe_mgr_tof2_rewrite_seed(sess_hdl, dev_info, pipe_bmp, stage);
}

/* Rewrite the GFM parity */
static pipe_status_t pipe_mgr_tof2_rewrite_gfm_parity(pipe_sess_hdl_t sess_hdl,
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

    for (row = 0; row < PIPE_MGR_TOF2_MAX_GFM_ROWS; row++) {
      addr =
          offsetof(tof2_reg,
                   pipes[0]
                       .mau[stage]
                       .dp.xbar_hash.hash
                       .galois_field_matrix[row][PIPE_MGR_TOF2_GFM_PARITY_COL]);

      value = PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
                  .tof2.data[row][PIPE_MGR_TOF2_GFM_PARITY_COL];

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

pipe_status_t pipe_mgr_tof2_recalc_write_gfm_parity(pipe_sess_hdl_t sess_hdl,
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

    for (row = 0; row < PIPE_MGR_TOF2_MAX_GFM_ROWS; row++) {
      /* byte0 - 8 bits, byte1 - 8 bits */
      for (bitpos = 0; bitpos < PIPE_MGR_TOF2_SINGLE_GFM_ENTRY_SZ; bitpos++) {
        num_ones = 0;

        for (col = 0; col < PIPE_MGR_TOF2_GFM_PARITY_COL; col++) {
          num_ones +=
              ((PIPE_INTR_GFM_DATA(dev, phy_pipe, stage).tof2.data[row][col] >>
                bitpos) &
               1u);
        }
        if ((num_ones % 2) == 1) {
          /* Set the parity to 1 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof2.data[row][PIPE_MGR_TOF2_GFM_PARITY_COL] |= (1u << bitpos);
        } else {
          /* Set the parity to 0 */
          PIPE_INTR_GFM_DATA(dev, phy_pipe, stage)
              .tof2.data[row][PIPE_MGR_TOF2_GFM_PARITY_COL] &=
              ~((uint32_t)1u << bitpos);
        }
      }
    }
  }

  if (!skip_write) {
    status =
        pipe_mgr_tof2_rewrite_gfm_parity(sess_hdl, dev_info, pipe_bmp, stage);
  }

  return status;
}

pipe_status_t pipe_mgr_tof2_gfm_test_pattern(pipe_sess_hdl_t shdl,
                                             rmt_dev_info_t *dev_info,
                                             bf_dev_pipe_t pipe_tgt,
                                             bf_dev_direction_t gress,
                                             dev_stage_t stage_id,
                                             int num_patterns,
                                             uint64_t *row_patterns,
                                             uint64_t *row_bad_parity) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_bitmap_t pbm;
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  if (pipe_tgt == BF_DEV_PIPE_ALL) {
    for (unsigned i = 0; i < dev_info->num_active_pipes; ++i) {
      PIPE_BITMAP_SET(&pbm, i);
    }
  } else {
    PIPE_BITMAP_SET(&pbm, pipe_tgt);
  }

  /* To atomically update a GFM row the 52 CSR writes must be bracketed by the
   * atomic-mod-CSR and atomic-mod-CSR-go instructions.  We can prepare these
   * two operations here and re-use them for each udpate. */
  int g = gress == BF_DEV_DIR_INGRESS ? 0 : 1;
  pipe_atomic_mod_csr_instr_t start_instr, go_instr;
  construct_instr_atomic_mod_csr(dev_id, &start_instr, g, true, true);
  construct_instr_atomic_mod_csr(dev_id, &go_instr, g, false, true);

  for (dev_stage_t stage = 0; stage < dev_info->num_active_mau; ++stage) {
    if (stage_id != 0xFF && stage != stage_id) continue;
    /* Program all 1024 input rows.  These rows are programmed together in
     * groups of 16 rows, each group using one config register per column.
     * This results in 64 (1024/16) "row groups" to program.
     * Use a 65th iteration to program the eight seed values. */
    for (int row_reg = 0; row_reg < 65; ++row_reg) {
      bool is_seed = row_reg == 64;
      /* Post the atomic-mod-CSR begin command. */
      rc = pipe_mgr_drv_ilist_add(&shdl,
                                  dev_info,
                                  &pbm,
                                  stage,
                                  (uint8_t *)&start_instr,
                                  sizeof start_instr);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s: Error %s posting atomic-begin to stage %d, pipe %X",
                  __func__,
                  pipe_str_err(rc),
                  stage,
                  pipe_tgt);
        return rc;
      }

      /* Program the first 51 columns (first 51 hash output bits) with the
       * requested data patterns.  The last (52nd) column will hold the parity
       * value for the row. */
      uint32_t parity = 0;
      for (int col = 0; col < 51; ++col) {
        uint32_t addr =
            is_seed
                ? offsetof(tof2_reg,
                           pipes[0].mau[stage].dp.xbar_hash.hash.hash_seed[col])
                : offsetof(
                      tof2_reg,
                      pipes[0]
                          .mau[stage]
                          .dp.xbar_hash.hash.galois_field_matrix[row_reg][col]);
        pipe_instr_write_reg_t instr;
        uint32_t data = 0;
        /* For each of the 16 rows in this group, setup the correct data to
         * program in the column.  The patterns requested by the caller will
         * be programmed over groups of rows.  For example, if there are three
         * patterns (A, B, & C) then row 0 will get A, row 1 will get B, row 2
         * will get C, row 3 will get A, row 4 will get B, etc. */
        int num_data_bits = is_seed ? 8 : 16;
        for (int i = 0; i < num_data_bits; ++i) {
          /* Rows 0..1023 are for the GFM while rows 1024..1031 are for the
           * eight seeds. */
          int row = 64 * row_reg + i;
          uint32_t val = (row_patterns[row % num_patterns] >> col) & 1;
          data |= val << i;
        }
        construct_instr_reg_write(dev_id, &instr, addr, data);
        rc = pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s: Error %s posting GFM[%d][%d] write to stage %d, pipe %X",
              __func__,
              pipe_str_err(rc),
              row_reg,
              col,
              stage,
              pipe_tgt);
          return rc;
        }
        /* The parity variable simply tracks whether we have an even or odd
         * number of ones on each of the 16 rows in the group. */
        parity ^= data;
      }

      {
        /* We've computed the correct (even) parity value for the 16 of 1024
         * rows we are currently modifying.  However, the caller may have
         * requested certain rows have incorrect (odd) parity.  This is
         * communicated by setting bits in the 1024 bits of row_bad_parity,
         * each bit corresponds to a row. */
        uint32_t addr =
            is_seed
                ? offsetof(tof2_reg,
                           pipes[0].mau[stage].dp.xbar_hash.hash.hash_seed[51])
                : offsetof(
                      tof2_reg,
                      pipes[0]
                          .mau[stage]
                          .dp.xbar_hash.hash.galois_field_matrix[row_reg][51]);
        pipe_instr_write_reg_t instr;
        uint32_t data = parity;
        if (!is_seed && row_bad_parity) {
          /* Find the 16 bits in row_bad_parity which correspond to this set of
           * rows. */
          int row = row_reg * 16;
          int row_parity_idx = row / 64;
          int row_parity_offset = row % 64;
          uint32_t parity_flip =
              (row_bad_parity[row_parity_idx] >> row_parity_offset) & 0xFFFF;
          /* XOR those bits into our good-parity value to flip the parity on
           * any row requiring bad parity. */
          data = data ^ parity_flip;
        }
        construct_instr_reg_write(dev_id, &instr, addr, data);
        rc = pipe_mgr_drv_ilist_add(
            &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s: Error %s posting GFM[%d][51] write to stage %d, pipe %X",
              __func__,
              pipe_str_err(rc),
              row_reg,
              stage,
              pipe_tgt);
          return rc;
        }
      }

      /* Post the atomic-mod-CSR go command. */
      rc = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pbm, stage, (uint8_t *)&go_instr, sizeof go_instr);
    }
  }
  return rc;
}

pipe_status_t pipe_mgr_tof2_gfm_test_col(pipe_sess_hdl_t shdl,
                                         rmt_dev_info_t *dev_info,
                                         bf_dev_pipe_t pipe_tgt,
                                         bf_dev_direction_t gress,
                                         dev_stage_t stage_id,
                                         int column,
                                         uint16_t col_data[64]) {
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_id_t dev_id = dev_info->dev_id;
  pipe_bitmap_t pbm;
  PIPE_BITMAP_INIT(&pbm, PIPE_BMP_SIZE);
  if (pipe_tgt == BF_DEV_PIPE_ALL) {
    for (unsigned i = 0; i < dev_info->num_active_pipes; ++i) {
      PIPE_BITMAP_SET(&pbm, i);
    }
  } else {
    PIPE_BITMAP_SET(&pbm, pipe_tgt);
  }

  /* To atomically update a GFM column the 64 CSR writes must be bracketed by
   * the atomic-mod-CSR and atomic-mod-CSR-go instructions.  We can prepare
   * these two operations here and re-use them for each udpate. */
  int g = gress == BF_DEV_DIR_INGRESS ? 0 : 1;
  pipe_atomic_mod_csr_instr_t start_instr, go_instr;
  construct_instr_atomic_mod_csr(dev_id, &start_instr, g, true, true);
  construct_instr_atomic_mod_csr(dev_id, &go_instr, g, false, true);

  for (dev_stage_t stage = 0; stage < dev_info->num_active_mau; ++stage) {
    if (stage_id != 0xFF && stage != stage_id) continue;

    /* Post the atomic-mod-CSR begin command. */
    rc = pipe_mgr_drv_ilist_add(&shdl,
                                dev_info,
                                &pbm,
                                stage,
                                (uint8_t *)&start_instr,
                                sizeof start_instr);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s: Error %s posting atomic-begin to stage %d, pipe %X",
                __func__,
                pipe_str_err(rc),
                stage,
                pipe_tgt);
      return rc;
    }

    /* Program every row in the stage to cover all 1024 input bits (64 regs,
     * each register covers 16 input bits, that is 16 GFM rows). */
    for (int row_reg = 0; row_reg < 64; ++row_reg) {
      uint32_t addr = offsetof(
          tof2_reg,
          pipes[0].mau[stage].dp.xbar_hash.hash.galois_field_matrix[row_reg]
                                                                   [column]);
      pipe_instr_write_reg_t instr;
      uint32_t data = col_data[row_reg];
      construct_instr_reg_write(dev_id, &instr, addr, data);
      rc = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, &pbm, stage, (uint8_t *)&instr, sizeof instr);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR("%s: Error %s posting GFM[%d][%d] write to stage %d, pipe %X",
                  __func__,
                  pipe_str_err(rc),
                  row_reg,
                  column,
                  stage,
                  pipe_tgt);
        return rc;
      }
    }

    /* Post the atomic-mod-CSR go command. */
    rc = pipe_mgr_drv_ilist_add(
        &shdl, dev_info, &pbm, stage, (uint8_t *)&go_instr, sizeof go_instr);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s: Error %s posting atomic-go to stage %d, pipe %X",
                __func__,
                pipe_str_err(rc),
                stage,
                pipe_tgt);
      return rc;
    }
  }
  return rc;
}
