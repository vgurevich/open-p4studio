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


/*
 *  Hash compute API used during Table entry add/delete.
 *
 *  BareFoot Networks Inc.
 */

#include <stdbool.h>
#include <unistd.h>

#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include "pipe_mgr_hash_compute_json.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_table_packing.h"

bf_hash_comp_t *g_hash_comp[BF_MAX_DEV_COUNT];
#define PIPE_MGR_HASH_COMP_CTX(_dev, _prof) (g_hash_comp[_dev]->profiles[_prof])

pipe_status_t bf_hash_compute_init(bf_dev_id_t devid, uint32_t num_profiles) {
  uint32_t prof_id = 0;
  if (g_hash_comp[devid]) {
    LOG_ERROR("Multiple inits during hash json parsing.");
    for (prof_id = 0; prof_id < g_hash_comp[devid]->num_profiles; prof_id++) {
      if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base) {
        PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base);
      }
      if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut) {
        PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut);
      }
    }
    if (g_hash_comp[devid]->profiles) {
      PIPE_MGR_FREE(g_hash_comp[devid]->profiles);
    }
    PIPE_MGR_FREE(g_hash_comp[devid]);
    g_hash_comp[devid] = NULL;
  }
  g_hash_comp[devid] = PIPE_MGR_CALLOC(1, sizeof(bf_hash_comp_t));
  if (g_hash_comp[devid] == NULL) {
    return (PIPE_NO_SYS_RESOURCES);
  }
  g_hash_comp[devid]->profiles =
      PIPE_MGR_CALLOC(num_profiles, sizeof(bf_hash_comp_profile_t));
  if (g_hash_comp[devid]->profiles == NULL) {
    PIPE_MGR_FREE(g_hash_comp[devid]);
    g_hash_comp[devid] = NULL;
    return (PIPE_NO_SYS_RESOURCES);
  }

  g_hash_comp[devid]->num_profiles = num_profiles;

  for (prof_id = 0; prof_id < g_hash_comp[devid]->num_profiles; prof_id++) {
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_offset_allocated = 0;
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_table_count = 0;
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list = 0;
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base = NULL;
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut = NULL;
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).hash_collison_count = 0;
  }

  return (PIPE_SUCCESS);
}

void bf_hash_compute_destroy(bf_dev_id_t devid) {
  uint32_t prof_id = 0;
  if (g_hash_comp[devid]) {
    for (prof_id = 0; prof_id < g_hash_comp[devid]->num_profiles; prof_id++) {
      if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base) {
        unsigned int i, j;
        for (i = 0;
             i < PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list;
             ++i) {
          if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                  .field_base[i]
                  .key_field_offset) {
            PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                              .field_base[i]
                              .key_field_offset);
          }
          if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                  .field_base[i]
                  .hash_values) {
            PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                              .field_base[i]
                              .hash_values);
          }
          if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                  .field_base[i]
                  .hash_combinations) {
            PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                              .field_base[i]
                              .hash_combinations);
          }
          for (j = 0; j < BF_HASH_COMP_MAX_HASH_BITS; ++j) {
            if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                    .field_base[i]
                    .hash_bit[j]
                    .xtract_list) {
              PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                                .field_base[i]
                                .hash_bit[j]
                                .xtract_list);
            }
          }
          if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                  .field_base[i]
                  .ghost_bit_info) {
            PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                              .field_base[i]
                              .ghost_bit_info);
          }
          if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                  .field_base[i]
                  .ghost_bit_to_hash_bits) {
            for (j = 0; j < PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                                .field_base[i]
                                .num_ghost_bits;
                 ++j) {
              if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                      .field_base[i]
                      .ghost_bit_to_hash_bits[j]
                      .mapping) {
                PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                                  .field_base[i]
                                  .ghost_bit_to_hash_bits[j]
                                  .mapping);
              }
            }
            PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id)
                              .field_base[i]
                              .ghost_bit_to_hash_bits);
          }
        }
        PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base);
      }
      if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut) {
        PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut);
      }
    }
    if (g_hash_comp[devid]->profiles) {
      PIPE_MGR_FREE(g_hash_comp[devid]->profiles);
    }
    PIPE_MGR_FREE(g_hash_comp[devid]);
    g_hash_comp[devid] = NULL;
  }
}

bf_hash_field_t *bf_hash_comp_get_field_base(bf_dev_id_t devid,
                                             profile_id_t prof_id) {
  return (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base);
}

bf_hash_comp_t *bf_hash_comp_get_hash_ctx(bf_dev_id_t devid) {
  return (g_hash_comp[devid]);
}

static inline uint32_t get_word_from_match_spec(uint16_t word_number,
                                                uint8_t *byte_array,
                                                uint16_t num_bytes) {
  /* We always need to form the 32-bit number by putting the lower byte of the
   * match-spec in the LSB of the 32-bit number
   */
  if (((word_number + 1) << 2) <= num_bytes) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    /* For little-endian, to achieve the above mentioned byte order within the
     * 32-bit int, just cast the base address as a uint32
     */
    return (*(((uint32_t *)byte_array) + word_number));
#else
    /* For Big-endian, to achieve the above mentioned byte order within the
     * 32-bit int
     * So, we need to build the 32-bit number in the fashion below.
     */
    uint8_t byte_idx = word_number << 2;
    return ((byte_array[byte_idx]) | (byte_array[byte_idx + 1] << 8) |
            (byte_array[byte_idx + 2] << 16) |
            (byte_array[byte_idx + 3] << 24));
#endif
  } else {
    if ((num_bytes & 0x3) == 0x3) {
      uint8_t byte_idx = word_number << 2;
      return ((byte_array[byte_idx] | (byte_array[byte_idx + 1] << 8) |
               (byte_array[byte_idx + 2] << 16)));
    } else if (num_bytes & 0x1) {
      return (*((uint8_t *)byte_array + (word_number << 2)) & 0xff);
    } else {
#if __BYTE_ORDER == __LITTLE_ENDIAN
      return (*((uint16_t *)byte_array + (word_number << 1)) & 0xffff);
#else
      uint8_t byte_idx = word_number << 2;
      return ((byte_array[byte_idx]) | (byte_array[byte_idx + 1] << 8));
#endif
    }
  }
}

static bf_hash_tbl_lut *bf_hash_comp_get_lut_entry(uint32_t bj_lut_index,
                                                   bf_dev_id_t devid,
                                                   profile_id_t prof_id,
                                                   uint32_t hndl,
                                                   dev_stage_t stage,
                                                   bool proxy) {
  bf_hash_tbl_lut *lut_ptr =
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut + bj_lut_index;
  int k = 0;

  while ((lut_ptr->tbl_hndl != hndl) || (lut_ptr->stage != stage) ||
         lut_ptr->is_proxy_hash != proxy) {
    if (++k >= PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR) {
      break;
    }
    // Handle hash collison...
    lut_ptr += PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth;
  }

  if (k >= PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR) {
    // Key Doesn't exist
    LOG_ERROR("Hash Compute lookup error");
    return (NULL);
  }

  return (lut_ptr);
}

static bf_hash_field_t *bf_hash_comp_get_hash_field(bf_hash_tbl_lut *lut_ptr,
                                                    bf_dev_id_t devid,
                                                    profile_id_t prof_id,
                                                    uint8_t index) {
  if (lut_ptr) {
    return (bf_hash_comp_get_field_base(devid, prof_id) +
            lut_ptr->hash_field_offset[index]);
  }
  return (NULL);
}

#define BF_RMT_HASH_WIDTH \
  (52)  // Move it to right place if more than
        // one file needs it.

pipe_status_t bf_hash_mat_entry_hash_compute(bf_dev_id_t devid,
                                             profile_id_t prof_id,
                                             dev_stage_t stage_id,
                                             pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                             pipe_tbl_match_spec_t *match_spec,
                                             bool proxy_hash,
                                             pipe_exm_hash_t *hash_bits) {
  uint32_t bj_hash;
  uint64_t hash_value;
  bf_hash_field_t *field_ptr;
  bf_hash_tbl_lut *lut_ptr;
  uint8_t bit_value, i, k, w;
  uint32_t word_value;

  bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  for (w = 0; w < lut_ptr->wide_hash_len; w++) {
    field_ptr = bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, w);
    if (field_ptr) {
      hash_value = 0;
      for (i = 0; i < BF_RMT_HASH_WIDTH; i++) {
        // TODO for loop can be iterated for less than 52 times
        if (!(field_ptr->hash_bit[i].seed_xtractwords & 0x7fff)) {
          bit_value = 0;
        } else {
          // get seed value
          bit_value = (field_ptr->hash_bit[i].seed_xtractwords >> 15);
          for (k = 0; k < (field_ptr->hash_bit[i].seed_xtractwords & 0x7fff);
               k++) {
            if (((field_ptr->hash_bit[i].xtract_list + k)->hashtype &
                 BF_HASH_COMP_HASHTYPE_MATCH) == BF_HASH_COMP_HASHTYPE_MATCH) {
              word_value = get_word_from_match_spec(
                  (field_ptr->hash_bit[i].xtract_list + k)->match_word,
                  match_spec->match_value_bits,
                  match_spec->num_match_bytes);
              word_value &=
                  (field_ptr->hash_bit[i].xtract_list + k)->bitmask_match;

              bit_value ^= __builtin_parity(word_value);
            }
          }
        }
        hash_value |= (((uint64_t)bit_value) << i);
      }
      hash_bits[w].num_bits = BF_RMT_HASH_WIDTH;
      hash_bits[w].hash_value = hash_value;
    } else {
      PIPE_MGR_DBGCHK(0);
      return PIPE_OBJ_NOT_FOUND;
    }
  }
  return (PIPE_SUCCESS);
}

// Hash Computation function with second hash algorithm
pipe_status_t bf_hash_mat_entry_hash2_compute(bf_dev_id_t devid,
                                              profile_id_t prof_id,
                                              dev_stage_t stage_id,
                                              pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                              pipe_tbl_match_spec_t *match_spec,
                                              bool proxy_hash,
                                              pipe_exm_hash_t *hash_bits) {
  uint32_t bj_hash;
  uint64_t hash_value;
  bf_hash_field_t *field_ptr;
  bf_hash_tbl_lut *lut_ptr;

  bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_ASSERT(0);
    return PIPE_UNEXPECTED;
  }

  for (int w = 0; w < lut_ptr->wide_hash_len; w++) {
    field_ptr = bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, w);
    if (field_ptr) {
      hash_value = field_ptr->hash_seed;
      for (int i = 0; i < match_spec->num_match_bytes; i++) {
        uint8_t key_byte =
            match_spec->match_value_bits[i] & match_spec->match_mask_bits[i];
        for (int j = 0; j < 8; j++) {
          if (key_byte & (1 << j)) {
            hash_value ^= field_ptr->hash_values[i * 8 + j];
          }
        }
      }
      hash_bits[w].num_bits = BF_RMT_HASH_WIDTH;
      hash_bits[w].hash_value = hash_value;
    }

    else {
      PIPE_MGR_ASSERT(0);
    }
  }

  return (PIPE_SUCCESS);
}

// Hash Computation function for radix based hash algorithm
pipe_status_t bf_hash_mat_entry_radix_hash_compute(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    dev_stage_t stage_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    bool proxy_hash,
    pipe_exm_hash_t *hash_bits) {
  uint32_t bj_hash;
  uint64_t hash_value;
  bf_hash_field_t *field_ptr;
  bf_hash_tbl_lut *lut_ptr;

  if (!hash_bits) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_ASSERT(0);
    return PIPE_UNEXPECTED;
  }

  for (int w = 0; w < lut_ptr->wide_hash_len; w++) {
    field_ptr = bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, w);
    if (field_ptr) {
      hash_value = field_ptr->hash_seed;
      uint8_t radix_val = field_ptr->radix_value;
      uint32_t num_combs_per_fragment = (1 << radix_val);
      uint8_t radix_mask = (1 << radix_val) - 1;
      uint8_t fragments_per_byte = 8 / radix_val;
      uint32_t table_offset = 0;
      uint32_t table_depth_per_byte =
          fragments_per_byte * num_combs_per_fragment;
      for (int i = 0; i < field_ptr->key_length / 8; i++) {
        table_offset = i * table_depth_per_byte;
        for (int j = 0; j < fragments_per_byte; j++) {
          // Read the key bits of this fragment
          uint8_t shifted_radix_mask = radix_mask << (j * radix_val);
          uint8_t key = match_spec->match_value_bits[i] &
                        match_spec->match_mask_bits[i] & shifted_radix_mask;
          key = key >> (j * radix_val);
          // Compute the index of the hash combinations table
          uint32_t array_idx = table_offset + j * num_combs_per_fragment + key;
          hash_value ^= field_ptr->hash_combinations[array_idx];
        }
      }
      hash_bits[w].num_bits = BF_RMT_HASH_WIDTH;
      hash_bits[w].hash_value = hash_value;
    }

    else {
      PIPE_MGR_ASSERT(0);
    }
  }

  return (PIPE_SUCCESS);
}

pipe_status_t bf_hash_mat_entry_hash_action_match_spec_decode_from_hash(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    dev_stage_t stage_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint64_t hash_value,
    bool proxy_hash,
    pipe_tbl_match_spec_t *match_spec) {
  uint32_t bj_hash;
  bf_hash_field_t *field_ptr;
  bf_hash_tbl_lut *lut_ptr;
  uint8_t i;
  uint16_t seed_xtractwords;
  uint8_t *match_word, *bit_mask_ptr;
  uint32_t bit_mask;
  uint8_t bit_value;
  uint8_t idx;
  uint16_t num_valid_match_bits = 0;

  bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }

  if (lut_ptr->wide_hash_len != 1) {
    // Wide hash length has to be 1 for hash action
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  field_ptr = bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, 0);
  if (!field_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  for (i = 0; i < BF_RMT_HASH_WIDTH; i++) {
    bit_value = (hash_value >> i) & 0x01;
    seed_xtractwords = field_ptr->hash_bit[i].seed_xtractwords & 0x7fff;
    if (seed_xtractwords > 1) {
      // For hash action this has to be 1
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
    if (seed_xtractwords == 0) {
      continue;
    }
    if ((field_ptr->hash_bit[i].xtract_list->hashtype &
         BF_HASH_COMP_HASHTYPE_MATCH) == BF_HASH_COMP_HASHTYPE_MATCH) {
      num_valid_match_bits++;
      if (!bit_value) {
        // Nothing to do if the bit value is 0. Since match spec is already
        // initialized to zero
        continue;
      }
      // Get the word in the match value bits corresponding to the xtractword
      match_word = match_spec->match_value_bits;
      bit_mask = field_ptr->hash_bit[i].xtract_list->bitmask_match;
      bit_mask_ptr = (uint8_t *)&bit_mask;
      if ((bit_mask & (bit_mask - 1)) != 0) {
        // The hash for hash action tables is a unity hash (meaning only one
        // bit from the match spec can participate in the hash for an output
        // hash bit). Hence the Bit mask has to be power of 2
        PIPE_MGR_DBGCHK(0);
        return PIPE_INVALID_ARG;
      }
      for (idx = 0; idx < sizeof(bit_mask); idx++) {
        if (bit_mask_ptr[idx]) {
          uint32_t global_idx = field_ptr->key_field_offset[i] / 8;
          global_idx = global_idx < sizeof(bit_mask) ? idx : global_idx + idx;
          match_word[global_idx] |= bit_mask_ptr[idx];
          break;
        }
      }
    } else {
      PIPE_MGR_DBGCHK(0);
      return PIPE_INVALID_ARG;
    }
  }
  match_spec->num_valid_match_bits = num_valid_match_bits;
  return PIPE_SUCCESS;
}

pipe_status_t bf_hash_mat_entry_ghost_bits_compute(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    dev_stage_t stage_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_tbl_match_spec_t *match_spec,
    pipe_mgr_exm_hash_info_for_decode_t *hash_info) {
  uint32_t bj_hash;
  bf_hash_field_t *field_ptr;
  bf_hash_tbl_lut *lut_ptr;
  uint8_t ghost_bit_value, i, j, k = 0;
  uint32_t word_value;
  bool proxy_hash = false;
  uint16_t num_hash_bits = 0;
  uint8_t match_word = 0;
  uint8_t hash_bit = 0;
  uint8_t hash_bit_value = 0;
  int byte_in_match_spec = 0;
  uint8_t bit_in_byte = 0;
  uint64_t hash = hash_info->hash;
  uint8_t hash_bit_lo = hash_info->hash_bit_lo;
  uint8_t hash_bit_hi = hash_info->hash_bit_hi;
  uint8_t ram_select_hi = hash_info->ram_select_hi;
  uint8_t ram_select_lo = hash_info->ram_select_lo;
  uint8_t num_ram_select_bits = hash_info->num_ram_select_bits;
  int match_spec_byte = 0;

  bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  field_ptr = bf_hash_comp_get_hash_field(
      lut_ptr, devid, prof_id, hash_info->wide_hash_idx);
  if (!field_ptr) {
    LOG_ERROR(
        "%s:%d Hash field info not found for tbl 0x%x, device id %d, stage id "
        "%d",
        __func__,
        __LINE__,
        mat_tbl_hdl,
        devid,
        stage_id);
    PIPE_MGR_DBGCHK(0);
    return PIPE_OBJ_NOT_FOUND;
  }
  for (i = 0; i < field_ptr->num_ghost_bits; i++) {
    num_hash_bits = field_ptr->ghost_bit_to_hash_bits[i].num_hash_bits;
    byte_in_match_spec = field_ptr->ghost_bit_info[i].byte_in_match_spec;
    bit_in_byte = field_ptr->ghost_bit_info[i].bit_in_byte;
    for (j = 0; j < num_hash_bits; j++) {
      hash_bit = field_ptr->ghost_bit_to_hash_bits[i].mapping[j];
      if (hash_bit < hash_bit_lo) {
        continue;
      }
      if (hash_bit > hash_bit_hi) {
        if ((hash_bit < ram_select_lo) || !num_ram_select_bits) {
          continue;
        }
        if ((hash_bit > ram_select_hi) || !num_ram_select_bits) {
          continue;
        }
      }
      if (!(field_ptr->hash_bit[hash_bit].seed_xtractwords & 0x7fff)) {
      } else {
        // get seed value
        ghost_bit_value =
            (field_ptr->hash_bit[hash_bit].seed_xtractwords >> 15) & 0x1;
        hash_bit_value = (hash >> hash_bit) & 0x1;
        /* Set the hash bit in the match spec */
        if (byte_in_match_spec != -1) {
          match_spec_byte = match_spec->match_value_bits[byte_in_match_spec];
          match_spec->match_value_bits[byte_in_match_spec] |=
              (hash_bit_value << bit_in_byte);
        } else {
          PIPE_MGR_DBGCHK(0);
        }
        for (k = 0;
             k < (field_ptr->hash_bit[hash_bit].seed_xtractwords & 0x7fff);
             k++) {
          if (((field_ptr->hash_bit[hash_bit].xtract_list + k)->hashtype &
               BF_HASH_COMP_HASHTYPE_MATCH) == BF_HASH_COMP_HASHTYPE_MATCH) {
            match_word =
                (field_ptr->hash_bit[hash_bit].xtract_list + k)->match_word;
            word_value = get_word_from_match_spec(match_word,
                                                  match_spec->match_value_bits,
                                                  match_spec->num_match_bytes);
            word_value &=
                (field_ptr->hash_bit[hash_bit].xtract_list + k)->bitmask_match;

            ghost_bit_value ^= __builtin_parity(word_value);
          }
        }
        /* Now put the ghost bit back in the match spec */
        if (byte_in_match_spec != -1) {
          match_spec->match_value_bits[byte_in_match_spec] = match_spec_byte;
          match_spec->match_value_bits[byte_in_match_spec] |=
              (ghost_bit_value << bit_in_byte);
        }
      }
    }
  }
  return PIPE_SUCCESS;
}

pipe_status_t bf_hash_get_hash_bits_for_key_bit(bf_dev_id_t devid,
                                                profile_id_t prof_id,
                                                dev_stage_t stage_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                int key_bit,
                                                bool proxy_hash,
                                                uint64_t *hash_bits) {
  if (!hash_bits) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  uint32_t bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  bf_hash_tbl_lut *lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bf_hash_field_t *field_ptr =
      bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, 0);
  if (!field_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  if (key_bit >= field_ptr->key_length) {
    return PIPE_INVALID_ARG;
  }
  *hash_bits = field_ptr->hash_values[key_bit];
  return PIPE_SUCCESS;
}

pipe_status_t bf_hash_get_hash_seed(bf_dev_id_t devid,
                                    profile_id_t prof_id,
                                    dev_stage_t stage_id,
                                    pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                    bool proxy_hash,
                                    uint64_t *seed) {
  if (!seed) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  uint32_t bj_hash = bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      mat_tbl_hdl,
      stage_id,
      BF_HASHTYPE(proxy_hash));
  bf_hash_tbl_lut *lut_ptr = bf_hash_comp_get_lut_entry(
      bj_hash, devid, prof_id, mat_tbl_hdl, stage_id, proxy_hash);
  if (!lut_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  bf_hash_field_t *field_ptr =
      bf_hash_comp_get_hash_field(lut_ptr, devid, prof_id, 0);
  if (!field_ptr) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }
  *seed = field_ptr->hash_seed;
  return PIPE_SUCCESS;
}
