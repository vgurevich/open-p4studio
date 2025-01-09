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
 * @file pipe_mgr_dkm.c
 * @date
 *
 * Code to implement exact match field key mask at run time.
 */

#include <stdint.h>
#include <bf_types/bf_types.h>
#include <lld/lld_reg_if.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv_intf.h"
#include "pipe_mgr_dkm.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_tof_dkm.h"
#include "pipe_mgr_tof2_dkm.h"
#include "pipe_mgr_tof3_dkm.h"
#include "pipe_mgr_exm_tbl_mgr_int.h"

/*
 * To change exact match table field key mask at run time, it is neccessary
 * to update following hardware configurations. HW config can be
 * changed / applied due to change in match key mask only when exact match
 * table is empty.
 *  - Unit RAM match mask should be programmed to exclude masked out bits
 *  - GFM should be to not include match key bits when computing hash
 */

#define PIPE_MGR_MAX_TOFINO_GFM_ROWS (64)
#define PIPE_MGR_MAX_TOFINO_GFM_COLUMNS (52)

static uint32_t dkm_match_mask_addr_get(rmt_dev_info_t *dev_info,
                                        mem_id_t mem_id) {
  int row = dev_info->dev_cfg.mem_id_to_row(mem_id, pipe_mem_type_unit_ram);
  int col = dev_info->dev_cfg.mem_id_to_col(mem_id, pipe_mem_type_unit_ram);
  /* Configuration will be written via instruction list so pipe and stage are
   * not required here (the instruction list specifies those address bits).
   * The index is set to zero to return the address of the first match-mask
   * register. */
  int pipe = 0, stage = 0, idx = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_dkm_match_mask_addr_get(pipe, stage, row, col, idx);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_dkm_match_mask_addr_get(pipe, stage, row, col, idx);
    default:
      PIPE_MGR_DBGCHK(0);
      return 0;
  }
}
static uint32_t dkm_gfm_addr_get(rmt_dev_info_t *dev_info, int row, int col) {
  /* Configuration will be written via instruction list so pipe and stage are
   * not required here (the instruction list specifies those address bits). */
  int pipe = 0, stage = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_dkm_galios_field_matrix_addr_get(
          pipe, stage, row, col);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_dkm_galios_field_matrix_addr_get(
          pipe, stage, row, col);
    default:
      PIPE_MGR_DBGCHK(0);
      return 0;
  }
}

static pipe_status_t build_unit_ram_match_mask(
    pipe_sess_hdl_t shdl,
    rmt_dev_info_t *dev_info,
    pipe_bitmap_t *pbm,
    pipe_mgr_exm_tbl_t *exm_tbl,
    pipe_mgr_exm_stage_info_t *exm_stage_info,
    uint8_t **exm_tbl_words) {
  pipe_status_t rc = PIPE_SUCCESS;

  /* Convert the encode wide word into 128-bit match masks, one mask per unit
   * ram in the wide word.  Each mask is represented by four 32-bit chunks. */
  int num_rams = exm_stage_info->pack_format->num_rams_in_wide_word;
  uint32_t mask[num_rams][4];
  for (int i = 0; i < num_rams; ++i) {
    PIPE_MGR_MEMCPY(&mask[i][0], &exm_tbl_words[i][0], 4);
    PIPE_MGR_MEMCPY(&mask[i][1], &exm_tbl_words[i][4], 4);
    PIPE_MGR_MEMCPY(&mask[i][2], &exm_tbl_words[i][8], 4);
    PIPE_MGR_MEMCPY(&mask[i][3], &exm_tbl_words[i][12], 4);
  }

  /* Program the mask for each RAM in each wide word of each hash way. */
  for (uint8_t way = 0; way < exm_stage_info->num_hash_ways; ++way) {
    pipe_mgr_exm_ram_alloc_info_t *ram_info;
    ram_info = exm_stage_info->hashway_data[way].ram_alloc_info;
    for (uint8_t word = 0; word < ram_info->num_wide_word_blks; ++word) {
      for (int ram = 0; ram < num_rams; ++ram) {
        mem_id_t mem_id = ram_info->tbl_word_blk[word].mem_id[ram];
        uint32_t addr = dkm_match_mask_addr_get(dev_info, mem_id);
        /* Four mask registers per RAM. */
        for (int i = 0; i < 4; ++i) {
          pipe_instr_write_reg_t instr;
          /* Hardware uses 1 to ignore a bit and 0 to match a bit which is the
           * inverse of how our mask has been built so we invert it. */
          uint32_t hw_val = ~mask[ram][i];
          construct_instr_reg_write(
              dev_info->dev_id, &instr, addr + 4 * i, hw_val);
          rc = pipe_mgr_drv_ilist_add(&shdl,
                                      dev_info,
                                      pbm,
                                      exm_stage_info->stage_id,
                                      (uint8_t *)&instr,
                                      sizeof instr);
          if (PIPE_SUCCESS != rc) {
            LOG_ERROR(
                "Dev %d, error %s setting DKM match mask config on table %s "
                "(0x%x) stage %d",
                dev_info->dev_id,
                pipe_str_err(rc),
                exm_tbl->name,
                exm_tbl->mat_tbl_hdl,
                exm_stage_info->stage_id);
            return rc;
          }
        }
      }
    }
  }
  return rc;
}

static pipe_status_t build_dkm_gfm_config(pipe_sess_hdl_t shdl,
                                          rmt_dev_info_t *dev_info,
                                          pipe_bitmap_t *pbm,
                                          pipe_mgr_exm_tbl_t *exm_tbl,
                                          pipe_tbl_match_spec_t *mspec,
                                          dev_stage_t stage_id,
                                          pipe_mgr_dkm_gfm_cfg_t *stage_hash,
                                          pipe_mgr_chip_gfm_t **rv) {
  pipe_status_t rc = PIPE_SUCCESS;
  int gfm_sz = PIPE_MGR_MAX_TOFINO_GFM_ROWS * PIPE_MGR_MAX_TOFINO_GFM_COLUMNS;
  pipe_mgr_chip_gfm_t *gfm = PIPE_MGR_CALLOC(gfm_sz, sizeof *gfm);
  *rv = gfm;
  if (!gfm) {
    return PIPE_NO_SYS_RESOURCES;
  }

  /* Loop over the provided mask, any bit set to zero should be removed from the
   * GFM and any bit set to one should be included. */
  for (uint32_t ms_bit = 0; ms_bit < mspec->num_match_bytes * 8; ++ms_bit) {
    if (ms_bit >= exm_tbl->num_match_spec_bytes * 8) {
      PIPE_MGR_DBGCHK(ms_bit < exm_tbl->num_match_spec_bytes * 8);
      return PIPE_UNEXPECTED;
    }

    /* Not all match spec bits will participate in the hash since some of them
     * are padding the fields to be byte aligned or some of them may be from a
     * portion of the field which is not part of the key (e.g. P4 mask) */
    if (!stage_hash[ms_bit].bits) {
      continue;
    }

    /* The requested mask is set in the match_value_bits.  Check each bit in
     * that array to see if it is included (set means masked in, clear means
     * masked out). */
    bool include = mspec->match_value_bits[ms_bit / 8] & (1u << (ms_bit % 8));
    for (int j = 0; j < stage_hash[ms_bit].num_bits; ++j) {
      pipe_mgr_dkm_gfm_bit_t *bit_info = stage_hash[ms_bit].bits + j;

      uint8_t col = bit_info->col;
      /* Calculate the input bit position, use 128 bits per group since that is
       * what TF2 has and even though TF1 has more, the extra bits are container
       * validity bits which are not used for hashing. */
      uint16_t input_bit = bit_info->grp;
      input_bit *= 128;
      input_bit += bit_info->grp_bit;
      /* There are 16 input bits within each config register, use that as the
       * "row" */
      int row = input_bit / 16;
      int offset = input_bit % 16;

      gfm[col * PIPE_MGR_MAX_TOFINO_GFM_ROWS + row].updated = true;
      if (include) {
        gfm[col * PIPE_MGR_MAX_TOFINO_GFM_ROWS + row].byte_pair |= 1u << offset;
      } else {
        /* Leave the bit as a zero since the key bit does not participate in the
         * hash. */
      }
    }
  }

  /* Go over the newly computed GFM state and post instructions/update shadow
   * for the changed sections. */
  for (int row = 0; row < PIPE_MGR_MAX_TOFINO_GFM_ROWS; ++row) {
    for (int col = 0; col < PIPE_MGR_MAX_TOFINO_GFM_COLUMNS; ++col) {
      pipe_mgr_chip_gfm_t *ent = &gfm[col * PIPE_MGR_MAX_TOFINO_GFM_ROWS + row];
      if (!ent->updated) continue;
      /* Grab the current value from the shadow incase we need to roll back. */
      rc = pipe_mgr_gfm_shadow_entry_get(
          dev_info, exm_tbl->lowest_pipe_id, stage_id, row, col, &ent->backup);
      if (rc != PIPE_SUCCESS) return rc;
      /* Set the new hardware value. */
      uint32_t hw_val = 0;
      switch (dev_info->dev_family) {
        case BF_DEV_FAMILY_TOFINO:
          hw_val = ((ent->byte_pair & 0xFF00) << 1) | (ent->byte_pair & 0xFF);
          break;
        case BF_DEV_FAMILY_TOFINO2:
          hw_val = ent->byte_pair;
          break;
        default:
          PIPE_MGR_DBGCHK(0);
          return PIPE_UNEXPECTED;
      }
      uint32_t addr = dkm_gfm_addr_get(dev_info, row, col);
      pipe_instr_write_reg_t instr;
      construct_instr_reg_write(dev_info->dev_id, &instr, addr, hw_val);

      rc = pipe_mgr_drv_ilist_add(
          &shdl, dev_info, pbm, stage_id, (uint8_t *)&instr, sizeof instr);
      if (PIPE_SUCCESS != rc) {
        LOG_ERROR(
            "Dev %d, error %s setting DKM GFM config on table %s (0x%x) stage "
            "%d",
            dev_info->dev_id,
            pipe_str_err(rc),
            exm_tbl->name,
            exm_tbl->mat_tbl_hdl,
            stage_id);
        return rc;
      }

      /* Update the shadow */
      rc =
          pipe_mgr_update_gfm_shadow(dev_info, pbm, stage_id, row, col, hw_val);
      if (rc != PIPE_SUCCESS) return rc;
    }
  }
  /* Since the GFM has been changed we need to recalculate parity for it. */
  rc = pipe_mgr_recalc_write_gfm_parity(shdl, dev_info, pbm, stage_id, false);
  return rc;
}

/* Use this function to program target with match-key-mask or
 * to reset match-key-mask to default or power on value.
 * Default match key mask is to include all match bits.  */
static pipe_status_t pipe_mgr_dkm_update(pipe_sess_hdl_t shdl,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                         pipe_tbl_match_spec_t *match_spec) {
  pipe_status_t rc = PIPE_INVALID_ARG;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) return PIPE_INVALID_ARG;

  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    /* We've already validated this is a DKM table in the caller so the exm
     * table must exist. */
    PIPE_MGR_DBGCHK(exm_tbl);
    return PIPE_UNEXPECTED;
  }

  pipemgr_dkm_lut_t *dkm_lut_ptr = NULL;
  pipe_mgr_dkm_tbl_map_get(dev_id, mat_tbl_hdl, (void **)&dkm_lut_ptr);
  if (dkm_lut_ptr == NULL || dkm_lut_ptr->dkm_cfg == NULL) {
    LOG_ERROR(
        "%s: Unable to find dynamic key mask details for match-tbl %s 0x%x",
        __func__,
        exm_tbl->name,
        mat_tbl_hdl);
    return PIPE_INVALID_ARG;
  }

  pipe_bitmap_t *pbm = &dev_info->profile_info[exm_tbl->profile_id]->pipe_bmp;
  int num_stages = exm_tbl->exm_tbl_data[0].num_stages;

  /* Variable to hold the updated GFM config per stage. */
  pipe_mgr_chip_gfm_t *per_stage_gfm[num_stages];
  for (int i = 0; i < num_stages; ++i) {
    per_stage_gfm[i] = NULL;
  }

  /* Scratch space representing the EXM table's wide word.  This will hold the
   * encoded entry and be used to derive the unit RAM match-mask config. */
  uint8_t tbl_words[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD][TOF_BYTES_IN_RAM_WORD];
  uint8_t *exm_tbl_words[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD];
  for (int i = 0; i < TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD; i++)
    exm_tbl_words[i] = tbl_words[i];

  for (int stg_idx = 0; stg_idx < num_stages; ++stg_idx) {
    pipe_mgr_exm_stage_info_t *exm_stage_info =
        &exm_tbl->exm_tbl_data[0].exm_stage_info[stg_idx];
    /* Clear state from previous loops. */
    for (int j = 0; j < TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD; ++j) {
      PIPE_MGR_MEMSET(exm_tbl_words[j], 0, TOF_BYTES_IN_RAM_WORD);
    }

    /* Encode the provided mask into each sub entry in the wide word.  This will
     * give us a wide word where only the included (masked-in) field bits and
     * the version/valid bits (which are never masked-out) are set.  Any match
     * overhead bits and masked-out key bits are left at zero.
     * This encoded wide-word is then used to derive the match masks. */
    int num_sub_entries =
        exm_stage_info->pack_format->num_entries_per_wide_word;
    for (int sub_entry = 0; sub_entry < num_sub_entries; ++sub_entry) {
      rc = pipe_mgr_entry_format_tof_dkm_tbl_keymask_encode(
          dev_id,
          exm_tbl->profile_id,
          mat_tbl_hdl,
          exm_stage_info->stage_id,
          exm_stage_info->stage_table_handle,
          sub_entry,
          match_spec,
          exm_tbl_words);
      if (rc != PIPE_SUCCESS) {
        LOG_ERROR(
            "Dev %d table %s (0x%x): Error %s encoding sub-entry %d for DKM "
            "match mask in stage %d",
            dev_id,
            exm_tbl->name,
            mat_tbl_hdl,
            pipe_str_err(rc),
            sub_entry,
            exm_stage_info->stage_id);
        goto cleanup;
      }
    }

    rc = build_unit_ram_match_mask(
        shdl, dev_info, pbm, exm_tbl, exm_stage_info, exm_tbl_words);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "Dev %d table %s (0x%x): Error %s building DKM match mask for stage "
          "%d",
          dev_id,
          exm_tbl->name,
          mat_tbl_hdl,
          pipe_str_err(rc),
          exm_stage_info->stage_id);
      goto cleanup;
    }

    rc = build_dkm_gfm_config(
        shdl,
        dev_info,
        pbm,
        exm_tbl,
        match_spec,
        exm_stage_info->stage_id,
        dkm_lut_ptr->dkm_cfg->hash[exm_stage_info->stage_id],
        &per_stage_gfm[stg_idx]);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR(
          "Dev %d table %s (0x%x): Error %s calculating GFM update for stage "
          "%d",
          dev_id,
          exm_tbl->name,
          mat_tbl_hdl,
          pipe_str_err(rc),
          exm_stage_info->stage_id);
      goto cleanup;
    }
  } /* each stage */

  for (int i = 0; i < num_stages; ++i) {
    if (per_stage_gfm[i]) {
      PIPE_MGR_FREE(per_stage_gfm[i]);
    }
  }
  return rc;

cleanup:
  /* Loop over the per-stage GFM config and restore the original values to the
   * GFM shadow. */
  for (int stg_idx = 0; stg_idx < num_stages; ++stg_idx) {
    if (!per_stage_gfm[stg_idx]) {
      break;
    }
    dev_stage_t stage_id =
        exm_tbl->exm_tbl_data[0].exm_stage_info[stg_idx].stage_id;
    for (int row = 0; row < PIPE_MGR_MAX_TOFINO_GFM_ROWS; ++row) {
      for (int col = 0; col < PIPE_MGR_MAX_TOFINO_GFM_COLUMNS; ++col) {
        int i = col * PIPE_MGR_MAX_TOFINO_GFM_ROWS + row;
        if (!per_stage_gfm[stg_idx][i].updated) continue;
        pipe_mgr_update_gfm_shadow(dev_info,
                                   pbm,
                                   stage_id,
                                   row,
                                   col,
                                   per_stage_gfm[stg_idx][i].backup);
      }
    }
    pipe_mgr_recalc_write_gfm_parity(shdl, dev_info, pbm, stage_id, true);
    PIPE_MGR_FREE(per_stage_gfm[stg_idx]);
  }
  return rc;
}

bool pipe_mgr_is_dkm_table(bf_dev_id_t dev_id, pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  void *unused = NULL;
  return BF_MAP_OK == pipe_mgr_dkm_tbl_map_get(dev_id, mat_tbl_hdl, &unused);
}

static pipe_status_t pipe_mgr_match_key_mask_spec_update_checks(
    bf_dev_id_t dev_id, pipe_mat_tbl_info_t *mat_tbl_info) {
  uint32_t count = 0;
  dev_target_t dev_tgt = {.device_id = dev_id, .dev_pipe_id = BF_DEV_PIPE_ALL};
  pipe_status_t ret = PIPE_SUCCESS;

  if (!mat_tbl_info->dynamic_key_mask_table) {
    LOG_ERROR(
        "Error in applying dynamic key mask for table %s (0x%x) device id %d. "
        "The table cannot accept key field mask",
        mat_tbl_info->name,
        mat_tbl_info->handle,
        dev_id);
    ret = PIPE_INVALID_ARG;
    goto done;
  }

  // Check if table is empty or not.
  if (pipe_mgr_is_device_virtual(dev_id)) {
    ret = pipe_mgr_exm_tbl_get_placed_entry_count(
        dev_tgt, mat_tbl_info->handle, &count);
  } else {
    ret = pipe_mgr_exm_tbl_get_programmed_entry_count(
        dev_tgt, mat_tbl_info->handle, &count);
  }
  if (ret != PIPE_SUCCESS) {
    LOG_ERROR(
        "Error in getting table occupancy for table %s (0x%x) device id %d",
        mat_tbl_info->name,
        mat_tbl_info->handle,
        dev_id);
    goto done;
  }
  if (count) {
    LOG_ERROR(
        "Table key mask cannot be changed while the table is populated. Delete "
        "all entries of the table before changing key mask.  Table %s (0x%x), "
        "device id %d",
        mat_tbl_info->name,
        mat_tbl_info->handle,
        dev_id);
    ret = PIPE_ENTRY_REFERENCES_EXIST;
    goto done;
  }
done:
  return ret;
}

static pipe_status_t pipe_mgr_dkm_cfg_set(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t dev_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          pipe_tbl_match_spec_t *match_spec) {
  if (pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_SUCCESS;
  }

  return pipe_mgr_dkm_update(sess_hdl, dev_id, mat_tbl_hdl, match_spec);
}

/* API exposed through pipe_mgr_intf layer to program match-key-mask */
pipe_status_t pipe_mgr_dkm_set(pipe_sess_hdl_t sess_hdl,
                               bf_dev_id_t dev_id,
                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                               pipe_tbl_match_spec_t *match_spec) {
  pipe_status_t ret;

  if (!match_spec) return PIPE_INVALID_ARG;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ret = pipe_mgr_match_key_mask_spec_update_checks(dev_id, mat_tbl_info);
  if (ret != PIPE_SUCCESS) {
    return ret;
  }

  uint8_t *new_mask = PIPE_MGR_MALLOC(match_spec->num_match_bytes);
  if (!new_mask) return PIPE_NO_SYS_RESOURCES;
  PIPE_MGR_MEMCPY(
      new_mask, match_spec->match_mask_bits, match_spec->num_match_bytes);

  ret = pipe_mgr_dkm_cfg_set(sess_hdl, dev_id, mat_tbl_hdl, match_spec);
  if (ret == PIPE_SUCCESS) {
    // Save mask in table struct.
    if (mat_tbl_info->match_key_mask) {
      PIPE_MGR_FREE(mat_tbl_info->match_key_mask);
    }
    mat_tbl_info->match_key_mask = new_mask;
    mat_tbl_info->match_key_mask_width = match_spec->num_match_bytes;
  } else {
    PIPE_MGR_FREE(new_mask);
  }

  return ret;
}

static pipe_status_t pipe_mgr_dkm_cfg_reset(pipe_sess_hdl_t sess_hdl,
                                            bf_dev_id_t dev_id,
                                            pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  if (pipe_mgr_is_device_virtual(dev_id)) {
    return PIPE_SUCCESS;
  }

  pipe_mgr_exm_tbl_t *exm_tbl = pipe_mgr_exm_tbl_get(dev_id, mat_tbl_hdl);
  if (exm_tbl == NULL) {
    /* We've already validated this is a DKM table in the caller so the exm
     * table must exist. */
    PIPE_MGR_DBGCHK(exm_tbl);
    return PIPE_UNEXPECTED;
  }

  /* Construct a partial match spec which has all bits set indicating all key
   * fields should be included in the match.
   * Note that we are blindly setting the key field array to all ones which
   * may be setting non-key fields (i.e. padding) but that will not affect the
   * DKM update. */
  pipe_tbl_match_spec_t mspec = {0};
  mspec.num_match_bytes = exm_tbl->num_match_spec_bytes;
  mspec.match_value_bits = PIPE_MGR_MALLOC(exm_tbl->num_match_spec_bytes);
  PIPE_MGR_MEMSET(mspec.match_value_bits, 0xFF, exm_tbl->num_match_spec_bytes);

  pipe_status_t rc = pipe_mgr_dkm_update(sess_hdl, dev_id, mat_tbl_hdl, &mspec);
  PIPE_MGR_FREE(mspec.match_value_bits);
  return rc;
}

/* API exposed through pipe_mgr_intf layer to reset match-key-mask to default */
pipe_status_t pipe_mgr_dkm_reset(pipe_sess_hdl_t sess_hdl,
                                 bf_dev_id_t dev_id,
                                 pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  pipe_status_t ret;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error in finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  ret = pipe_mgr_match_key_mask_spec_update_checks(dev_id, mat_tbl_info);
  if (ret != PIPE_SUCCESS) {
    return ret;
  }

  ret = pipe_mgr_dkm_cfg_reset(sess_hdl, dev_id, mat_tbl_hdl);

  if (ret == PIPE_SUCCESS) {
    if (mat_tbl_info->match_key_mask) {
      PIPE_MGR_FREE(mat_tbl_info->match_key_mask);
      mat_tbl_info->match_key_mask = NULL;
    }
    mat_tbl_info->match_key_mask_width = 0;
  }

  return ret;
}

/* API exposed through pipe_mgr_intf layer to get the programmed match-key-mask.
 */
pipe_status_t pipe_mgr_dkm_get(pipe_sess_hdl_t sess_hdl,
                               bf_dev_id_t dev_id,
                               pipe_mat_tbl_hdl_t mat_tbl_hdl,
                               pipe_tbl_match_spec_t *match_spec) {
  (void)sess_hdl;
  pipe_status_t ret = PIPE_SUCCESS;

  pipe_mat_tbl_info_t *mat_tbl_info =
      pipe_mgr_get_tbl_info(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (mat_tbl_info == NULL) {
    LOG_ERROR("Error finding the table info for tbl 0x%x device id %d",
              mat_tbl_hdl,
              dev_id);
    return PIPE_OBJ_NOT_FOUND;
  }

  profile_id_t prof_id =
      pipe_mgr_get_tbl_profile(dev_id, mat_tbl_hdl, __func__, __LINE__);
  if (prof_id == -1) {
    LOG_ERROR("Error in finding profile for tbl %s (0x%x) device id %d",
              mat_tbl_info->name,
              mat_tbl_hdl,
              dev_id);
    return (PIPE_OBJ_NOT_FOUND);
  }

  if (mat_tbl_info->match_key_mask) {
    // Get mask from table struct.
    if (match_spec->num_match_bytes != mat_tbl_info->match_key_mask_width) {
      LOG_ERROR(
          "%s:%d Match spec mask has length %d, expected length %d for "
          "dkm tbl 0x%x device id %d",
          __func__,
          __LINE__,
          match_spec->num_match_bytes,
          mat_tbl_info->match_key_mask_width,
          mat_tbl_hdl,
          dev_id);
      PIPE_MGR_DBGCHK(match_spec->num_match_bytes ==
                      mat_tbl_info->match_key_mask_width);
      return PIPE_INVALID_ARG;
    }
    PIPE_MGR_MEMCPY(match_spec->match_mask_bits,
                    mat_tbl_info->match_key_mask,
                    match_spec->num_match_bytes);
  } else {
    // All bits are set in default mask.
    ret = pipe_mgr_entry_format_construct_match_key_mask_spec(
        dev_id, prof_id, mat_tbl_hdl, match_spec);
    if (ret != PIPE_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error constructing default match key mask for dkm "
          "tbl 0x%x device id %d",
          __func__,
          __LINE__,
          mat_tbl_hdl,
          dev_id);
      return ret;
    }
  }

  return ret;
}

/* For hitless warm init mode, it is required to reconstruct
 * key mask used on match-spec prior to restart.
 * This API restores match key mask for every exm table with
 * dynamic key mask attribute
 */
pipe_status_t pipe_mgr_dkm_rebuild_keymask(pipe_sess_hdl_t sess_hdl,
                                           bf_dev_id_t dev_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl) {
  (void)sess_hdl;
  (void)dev_id;
  (void)mat_tbl_hdl;
  return (PIPE_SUCCESS);
}
