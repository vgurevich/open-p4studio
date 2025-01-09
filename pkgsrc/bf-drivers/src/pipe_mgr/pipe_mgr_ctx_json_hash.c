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
 *  Hash computation support for P4 match_spec.
 *
 *  BareFoot Networks Inc.
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <target-utils/third-party/cJSON/cJSON.h>

#include <bf_types/bf_types.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_porting.h>
#include <pipe_mgr/pipe_mgr_intf.h>
#include "pipe_mgr_hash_compute_json.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_table_packing.h"

#include "pipe_mgr_ctx_json.h"

#define BYTE_WIDTH (8)
#define BF_HASH_MAX_MATCH_SPEC_BYTE_WIDTH (128)
#define DEFAULT_HASH_RADIX_VALUE (4)

extern bf_hash_comp_t *g_hash_comp[BF_MAX_DEV_COUNT];
#define PIPE_MGR_HASH_COMP_CTX(_dev, _prof) (g_hash_comp[_dev]->profiles[_prof])

static uint16_t bf_hash_comp_get_next_unused_field_offset(
    bf_dev_id_t devid, profile_id_t prof_id) {
  uint16_t field_offset;

  if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_offset_allocated >=
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list) {
    LOG_ERROR("Invalid field offset %u >= %u",
              PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_offset_allocated,
              PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list);
    bf_sys_assert(0);
    return (BF_HASH_COMP_FIELD_OFFSET_INVALID);
  }
  field_offset = PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_offset_allocated;

  // For now its not atomic increment because bf_hash_compute_init()
  // is expected to be called only once during driver init time
  // and within a single thread. No need for MT safe here.
  PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_offset_allocated++;
  return (field_offset);
}

static bf_hash_tbl_lut *bf_hash_comp_update_lut_entry(uint16_t bj_lut_index,
                                                      bf_dev_id_t devid,
                                                      profile_id_t prof_id,
                                                      uint32_t hndl,
                                                      uint8_t stage,
                                                      uint8_t wide_hash_len,
                                                      bool proxy) {
  bf_hash_tbl_lut *lut_ptr =
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut + bj_lut_index;
  int k = 0;

  while (k < PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR && lut_ptr->tbl_hndl != 0) {
    // Handle hash collison...
    lut_ptr += PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth;
    k++;
  }

  if (k >= PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR) {
    // Fatal...
    // Hashing is poor... Not able to handle collison.
    // Collison probability > 1/BF_HASH_COMP_COLLISON_HANDLE_FACTOR.
    // Improve hashing or increase collison factor.
    LOG_ERROR("Cannot handle hash collision.");
    return (NULL);
  }

  if (k) {
    // For debug/building confidence in hash purpose
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).hash_collison_count++;
    // For validating Hash entropy..Ideally this counter should be 0.
    // If positive, expectation is less than BF_HASH_COLLISON_HANDLE_FACTOR.
    // When more than BF_HASH_COLLISON_HANDLE_FACTOR and no fatal error
    // (that is same bucket is not indexed more than COLLISON_HANDLE_FACTOR)
    // then use judgement call here. If this counter is greater than
    // 40, then look for different hash !!
  }
  lut_ptr->tbl_hndl = hndl;
  lut_ptr->stage = stage;
  lut_ptr->wide_hash_len = wide_hash_len;
  lut_ptr->is_proxy_hash = proxy;

  return (lut_ptr);
}

static uint16_t bf_hash_comp_compute_lut_index(bf_dev_id_t devid,
                                               profile_id_t prof_id,
                                               uint32_t table_handle,
                                               uint8_t stage,
                                               bool hash_type) {
  return (bob_jenkin_hash_one_at_a_time(
      PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth,
      table_handle,
      stage,
      BF_HASHTYPE(hash_type)));
}

static bf_hash_field_t *bf_hash_comp_get_hash_field_ptr(bf_dev_id_t devid,
                                                        profile_id_t prof_id,
                                                        uint16_t field_offset) {
  if (field_offset == BF_HASH_COMP_FIELD_OFFSET_INVALID) {
    return (NULL);
  }
  if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base == NULL) {
    LOG_ERROR("%s dev %d NULL field_base", __func__, devid);
    return NULL;
  }
  return (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base + field_offset);
}

/**
 * Parses a particular hash bit information from the cJSON structure and
 * the match spec and updates the hash values of each key bit participating
 * in this hash bit
 *
 * @param hash_bit_cjson The cJSON corresponding to this hash bit.
 * @param match_key_fields_cjson The match spec JSON data required to
 * parse the field bit
 * @param hash_field The internal structure with the hash values to update.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_update_hash_values(
    rmt_dev_profile_info_t *prof_info,
    cJSON *hash_bit_cjson,
    cJSON *match_key_fields_cjson,
    bf_hash_field_t *hash_field) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int hash_bit = 0;
  int seed_value = 0;
  cJSON *bits_to_xor = NULL;
  cJSON *bit_to_xor = NULL;
  err =
      bf_cjson_get_int(hash_bit_cjson, CTX_JSON_HASH_BITS_HASH_BIT, &hash_bit);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_object(
      hash_bit_cjson, CTX_JSON_HASH_BIT_BITS_TO_XOR, &bits_to_xor);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(hash_bit_cjson, CTX_JSON_HASH_BIT_SEED, &seed_value);
  CHECK_ERR(err, rc, cleanup);

  CTX_JSON_FOR_EACH(bit_to_xor, bits_to_xor) {
    // Parse field name and field bit from ContextJSON.
    char *field_name = NULL;
    int field_bit = 0;

    err = ctx_json_parse_global_name(bit_to_xor,
                                     CTX_JSON_BIT_TO_XOR_GLOBAL_NAME,
                                     CTX_JSON_BIT_TO_XOR_FIELD_NAME,
                                     &field_name);
    CHECK_ERR(err, rc, cleanup);
    err =
        bf_cjson_get_int(bit_to_xor, CTX_JSON_BIT_TO_XOR_FIELD_BIT, &field_bit);
    CHECK_ERR(err, rc, cleanup);

    // Get the location of the field_bit in the Key
    int ms_bit = 0;
    bool use_global_name =
        ctx_json_schema_has_global_name(prof_info->schema_version);
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_bit,
                                                1,
                                                NULL,
                                                NULL,
                                                &ms_bit,
                                                NULL);

    CHECK_ERR(err, rc, cleanup);

    hash_field->hash_values[ms_bit] |= (1ULL << hash_bit);
  }

  /* Note the seed_value is expected to always be either 0 or 1. */
  hash_field->hash_seed |= ((uint64_t)seed_value << hash_bit);

  return rc;

cleanup:
  return rc;
}

/**
 * Compute all the possible hash combinations for a given radix value
 */
static pipe_status_t ctx_json_generate_hash_combinations(
    bf_hash_field_t *hash_field) {
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t array_idx = 0;

  int num_key_fragments = (hash_field->key_length) / (hash_field->radix_value);
  if (num_key_fragments < 1) {
    LOG_ERROR(
        "%s : Error in generating hash combinations for given radix value:"
        "Key Length 0x%x is less than radix value 0x%x",
        __func__,
        hash_field->key_length,
        hash_field->radix_value);
    return PIPE_INVALID_ARG;
  }

  int hash_combs_per_fragment = 1 << hash_field->radix_value;

  // The hash combinations are computed by xoring the hash_values of the
  // key_bits that are "1"
  for (int i = 0; i < num_key_fragments; i++) {
    for (uint8_t j = 0; j < hash_field->radix_value; j++) {
      int hash_value_idx = i * hash_field->radix_value + j;
      int increment = 1 << (j + 1);
      for (int k = 0; k < hash_combs_per_fragment; k += increment) {
        for (int l = increment / 2; l < increment; l++) {
          array_idx = i * hash_combs_per_fragment + k + l;
          hash_field->hash_combinations[array_idx] ^=
              hash_field->hash_values[hash_value_idx];
        }
      }
    }
  }

  return rc;
}
/**
 * Parses a particular hash bit from the cJSON structure and the match spec
 * into the appropriate internal representation, given by hash_field.
 *
 * @param hash_bit_cjson The cJSON corresponding to this hash bit.
 * @param match_key_fields_cjson The match spec information required to parse
 * the hash bit.
 * @param hash_field The internal structure to be populated.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_hash_bit(rmt_dev_profile_info_t *prof_info,
                                             cJSON *hash_bit_cjson,
                                             cJSON *match_key_fields_cjson,
                                             bf_hash_field_t *hash_field) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int hash_bit = 0;
  cJSON *bits_to_xor = NULL;
  err =
      bf_cjson_get_int(hash_bit_cjson, CTX_JSON_HASH_BITS_HASH_BIT, &hash_bit);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_object(
      hash_bit_cjson, CTX_JSON_HASH_BIT_BITS_TO_XOR, &bits_to_xor);
  CHECK_ERR(err, rc, cleanup);
  int match_bit_mask[BF_HASH_MAX_MATCH_SPEC_BYTE_WIDTH];
  PIPE_MGR_MEMSET(
      match_bit_mask, 0, BF_HASH_MAX_MATCH_SPEC_BYTE_WIDTH * sizeof(int));

  cJSON *bit_to_xor = NULL;

  CTX_JSON_FOR_EACH(bit_to_xor, bits_to_xor) {
    // Parse field name and field bit from ContextJSON.
    char *field_name = NULL;
    int field_bit = 0;

    err = ctx_json_parse_global_name(bit_to_xor,
                                     CTX_JSON_BIT_TO_XOR_GLOBAL_NAME,
                                     CTX_JSON_BIT_TO_XOR_FIELD_NAME,
                                     &field_name);
    CHECK_ERR(err, rc, cleanup);

    err =
        bf_cjson_get_int(bit_to_xor, CTX_JSON_BIT_TO_XOR_FIELD_BIT, &field_bit);
    CHECK_ERR(err, rc, cleanup);

    // Get the location of the field in the Key
    int spec_length = 0;
    int spec_offset = 0;
    bool use_global_name =
        ctx_json_schema_has_global_name(prof_info->schema_version);
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_bit,
                                                1,
                                                &spec_length,
                                                &spec_offset,
                                                NULL,
                                                NULL);
    CHECK_ERR(err, rc, cleanup);
    if (spec_length < 8) spec_length = 8;
    int bit_offset = spec_offset + spec_length - field_bit - 1;
    int byte = bit_offset / BYTE_WIDTH;
    int bit_in_byte = BYTE_WIDTH - (bit_offset % BYTE_WIDTH) - 1;
    match_bit_mask[byte] |= 1 << bit_in_byte;
  }

  int word_count = 0;
  for (int i = 0; i < BF_HASH_MAX_MATCH_SPEC_BYTE_WIDTH; i++) {
    if (match_bit_mask[i]) {
      word_count++;
    }
  }

  if (word_count == 0) {
    return rc;
  }

  hash_field->hash_bit[hash_bit].xtract_list =
      PIPE_MGR_CALLOC(word_count, sizeof(hash_comp_xtract_t));

  hash_field->hash_bit[hash_bit].seed_xtractwords = word_count;

  int seed = 0;
  err = bf_cjson_get_int(hash_bit_cjson, CTX_JSON_HASH_BIT_SEED, &seed);
  CHECK_ERR(err, rc, cleanup);

  hash_field->hash_bit[hash_bit].seed_xtractwords |= seed << 15;
  hash_comp_xtract_t *xtract_list = hash_field->hash_bit[hash_bit].xtract_list;
  for (int i = 0; i < BF_HASH_MAX_MATCH_SPEC_BYTE_WIDTH; i += 4) {
    if (match_bit_mask[i] || match_bit_mask[i + 1] || match_bit_mask[i + 2] ||
        match_bit_mask[i + 3]) {
      xtract_list->match_word = i >> 2;
      uint32_t a = match_bit_mask[i];
      uint32_t b = match_bit_mask[i + 1];
      uint32_t c = match_bit_mask[i + 2];
      uint32_t d = match_bit_mask[i + 3];
      xtract_list->bitmask_match = (a) | (b << 8) | (c << 16) | (d << 24);
      xtract_list->hashtype = BF_HASH_COMP_HASHTYPE_MATCH;
      xtract_list++;
    }
  }
  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_ghost_bit_info(
    rmt_dev_profile_info_t *prof_info,
    cJSON *ghost_bit_info_cjson,
    bf_hash_field_t *hash_field,
    cJSON *match_key_fields_cjson) {
  uint16_t num_ghost_bits = cJSON_GetArraySize(ghost_bit_info_cjson);
  hash_field->num_ghost_bits = num_ghost_bits;
  hash_field->ghost_bit_info = (bf_hash_ghost_bit_info_t *)PIPE_MGR_CALLOC(
      num_ghost_bits, sizeof(bf_hash_ghost_bit_info_t));
  if (hash_field->ghost_bit_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  pipe_status_t rc = PIPE_SUCCESS;
  uint32_t i = 0;
  cJSON *ghost_bit_cjson;

  CTX_JSON_FOR_EACH(ghost_bit_cjson, ghost_bit_info_cjson) {
    char *field_name = NULL;
    int err = 0, field_bit = 0;

    err = ctx_json_parse_global_name(ghost_bit_cjson,
                                     CTX_JSON_GHOST_BIT_INFO_GLOBAL_NAME,
                                     CTX_JSON_GHOST_BIT_INFO_FIELD_NAME,
                                     &field_name);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        ghost_bit_cjson, CTX_JSON_GHOST_BIT_INFO_POSITION, &field_bit);
    CHECK_ERR(err, rc, cleanup);
    int ms_bit = 0;
    bool use_global_name =
        ctx_json_schema_has_global_name(prof_info->schema_version);
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_bit,
                                                1,
                                                NULL,
                                                NULL,
                                                &ms_bit,
                                                NULL);
    CHECK_ERR(err, rc, cleanup);

    hash_field->ghost_bit_info[i].byte_in_match_spec = ms_bit / 8;
    hash_field->ghost_bit_info[i].bit_in_byte = ms_bit % 8;
    i++;
  }
cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_ghost_bits_to_hash_bit(
    cJSON *ghost_bits_to_hash_bit_cjson, bf_hash_field_t *hash_field) {
  cJSON *item;
  hash_field->ghost_bit_to_hash_bits =
      (bf_hash_ghost_bit_to_hash_bit_t *)PIPE_MGR_CALLOC(
          hash_field->num_ghost_bits, sizeof(bf_hash_ghost_bit_to_hash_bit_t));
  if (hash_field->ghost_bit_to_hash_bits == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  uint32_t i = 0;
  cJSON *ghost_bit_to_hash_bits_cjson;
  unsigned idx = 0;
  CTX_JSON_FOR_EACH(ghost_bit_to_hash_bits_cjson,
                    ghost_bits_to_hash_bit_cjson) {
    uint32_t num_hash_bits = cJSON_GetArraySize(ghost_bit_to_hash_bits_cjson);
    hash_field->ghost_bit_to_hash_bits[idx].num_hash_bits = num_hash_bits;
    hash_field->ghost_bit_to_hash_bits[idx].mapping =
        (uint8_t *)PIPE_MGR_CALLOC(num_hash_bits, sizeof(uint8_t));
    if (hash_field->ghost_bit_to_hash_bits[idx].mapping == NULL) {
      LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    for (i = 0; i < num_hash_bits; i++) {
      item = cJSON_GetArrayItem(ghost_bit_to_hash_bits_cjson, i);
      hash_field->ghost_bit_to_hash_bits[idx].mapping[i] = item->valueint;
    }
    idx++;
  }
  return PIPE_SUCCESS;
}

/**
 * Parses the hash bits for a given table from the Context JSON.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure containing the table's information.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_hash_for_table(bf_dev_id_t devid,
                                                   profile_id_t prof_id,
                                                   cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int table_handle = 0;
  char *name = NULL;
  cJSON **all_stage_tables_cjson = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG("%s:%d: Parsing hash bits for table handle 0x%x named \"%s\".",
          __func__,
          __LINE__,
          table_handle,
          name);

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse exact match entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  int number_stage_tables = 0;
  err = ctx_json_parse_all_match_stage_tables_for_table(
      table_cjson,
      max_number_stages * max_number_logical_tables,
      all_stage_tables_cjson,
      &number_stage_tables);
  CHECK_ERR(err, rc, cleanup);

  // Length of the Key for this table in Bytes
  int key_length;
  err = ctx_json_parse_spec_details_for_key_length(match_key_fields_cjson,
                                                   &key_length);
  CHECK_ERR(err, rc, cleanup);

  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);
  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];
    PIPE_MGR_ASSERT(stage_table_cjson != NULL);

    cJSON *hash_functions_cjson = NULL;
    err = bf_cjson_get_object(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_HASH_FUNCTIONS,
                              &hash_functions_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Check if the table has any hash functions.
    int number_hash_functions = cJSON_GetArraySize(hash_functions_cjson);
    if (number_hash_functions == 0) {
      continue;
    }

    int wide_hash_len = cJSON_GetArraySize(hash_functions_cjson);
    int stage = 0;
    err = bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);
    CHECK_ERR(err, rc, cleanup);

    // Compute hash index.
    uint16_t bj_index = bf_hash_comp_compute_lut_index(
        devid, prof_id, table_handle, stage, false);

    // Update the look-up table entry with this stage, handle, size, and
    // hash type.
    bf_hash_tbl_lut *lut_ptr = bf_hash_comp_update_lut_entry(
        bj_index, devid, prof_id, table_handle, stage, wide_hash_len, false);
    if (lut_ptr == NULL) {
      LOG_ERROR("%s:%d: Could not populate hash LUT.", __func__, __LINE__);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }

    // Go through every hash function in this stage, and add it.
    int index = 0;
    cJSON *hash_function_cjson = NULL;
    CTX_JSON_FOR_EACH(hash_function_cjson, hash_functions_cjson) {
      uint16_t field_offset =
          bf_hash_comp_get_next_unused_field_offset(devid, prof_id);
      if (field_offset >=
          PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list) {
        LOG_ERROR("%s:%d: Could not find unused field offset for hash bits.",
                  __func__,
                  __LINE__);
        rc = PIPE_INIT_ERROR;
        goto cleanup;
      }

      lut_ptr->hash_field_offset[index] = field_offset;

      // Buffers used to store the bit masks for each bit to xor. These will
      // be zeroed out for each new bit.
      bf_hash_field_t *hash_field =
          bf_hash_comp_get_hash_field_ptr(devid, prof_id, field_offset);

      // Make sure the hash field has been allocated elsewhere.
      PIPE_MGR_ASSERT(hash_field != NULL);
      if (hash_field == NULL) {
        LOG_ERROR("%s:%d: NULL hash_field.", __func__, __LINE__);
        rc = PIPE_INIT_ERROR;
        goto cleanup;
      }
      PIPE_MGR_ASSERT(hash_field->hash_width == 0);

      cJSON *hash_bits_cjson = NULL;
      cJSON *hash_bit_cjson = NULL;
      err = bf_cjson_get_object(hash_function_cjson,
                                CTX_JSON_HASH_FUNCTION_HASH_BITS,
                                &hash_bits_cjson);
      CHECK_ERR(err, rc, cleanup);

      /*Parsing the JSON for generating information for new hash Computation*/
      hash_field->key_length = key_length * 8;
      // Allocate memory for hash values
      hash_field->hash_values =
          PIPE_MGR_CALLOC(hash_field->key_length, sizeof(uint64_t));
      hash_field->hash_seed = 0;

      // Allocate memory for key fields length info
      hash_field->key_field_offset =
          PIPE_MGR_CALLOC(GETARRSZ(hash_bits_cjson), sizeof(uint16_t));

      // Compute the hash values by iterating over each hash bit
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        rc = ctx_json_parse_update_hash_values(
            prof_info, hash_bit_cjson, match_key_fields_cjson, hash_field);
        CHECK_RC(rc, cleanup);
      }

      // Compute all the possible hash combination values for a given radix
      hash_field->radix_value =
          DEFAULT_HASH_RADIX_VALUE;  // ASSIGNING THE RADIX VALUE HERE
      hash_field->hash_combs_size = (key_length * 8 / hash_field->radix_value) *
                                    (1 << hash_field->radix_value);
      // Allocate memory for hash combinations
      hash_field->hash_combinations =
          PIPE_MGR_CALLOC(hash_field->hash_combs_size, sizeof(uint64_t));
      // Generate the hash combinations
      rc = ctx_json_generate_hash_combinations(hash_field);
      CHECK_RC(rc, cleanup);

      /**********Parsing JSON for hash field information*********/
      // Parse all hash bits.
      hash_bit_cjson = NULL;
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        rc = ctx_json_parse_hash_bit(
            prof_info, hash_bit_cjson, match_key_fields_cjson, hash_field);
        CHECK_RC(rc, cleanup);
      }

      /* Calculate size key fields */
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        cJSON *bits_to_xor = NULL;
        cJSON *bit_to_xor = NULL;
        char *field_key_name = NULL;
        int field_key_offset = 0;
        int key_iter = 0;
        int field_bit = 0;
        err = bf_cjson_get_object(
            hash_bit_cjson, CTX_JSON_HASH_BIT_BITS_TO_XOR, &bits_to_xor);
        CHECK_ERR(err, rc, cleanup);
        /* Get the first item in the "bits_to_xor" list.  This is to populate
         * the key_field_offset used to decode hash action cases.  Non-hash
         * action tables may have more than one array entry but the
         * key_field_offset is only used for hash action tables so we only need
         * to use the first array entry. */
        bit_to_xor = GETARRITEM(bits_to_xor, 0);

        err = ctx_json_parse_global_name(bit_to_xor,
                                         CTX_JSON_BIT_TO_XOR_GLOBAL_NAME,
                                         CTX_JSON_BIT_TO_XOR_FIELD_NAME,
                                         &field_key_name);
        CHECK_ERR(err, rc, cleanup);
        err = bf_cjson_get_int(
            bit_to_xor, CTX_JSON_BIT_TO_XOR_FIELD_BIT, &field_bit);
        CHECK_ERR(err, rc, cleanup);
        err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                    match_key_fields_cjson,
                                                    field_key_name,
                                                    field_bit,
                                                    1,
                                                    NULL,
                                                    &field_key_offset,
                                                    NULL,
                                                    NULL);
        CHECK_ERR(err, rc, cleanup);
        hash_field->key_field_offset[key_iter++] = field_key_offset;
      }

      /* Parse the ghost_bit_info */
      cJSON *ghost_bit_info_cjson = NULL;
      bf_cjson_try_get_object(hash_function_cjson,
                              CTX_JSON_STAGE_TABLE_GHOST_BIT_INFO,
                              &ghost_bit_info_cjson);
      if (ghost_bit_info_cjson) {
        /* For now ignore if the ghost bit info cjson is not present.
         * This is so that the compiler tests don't error out.
         */
        rc = ctx_json_parse_ghost_bit_info(prof_info,
                                           ghost_bit_info_cjson,
                                           hash_field,
                                           match_key_fields_cjson);
        CHECK_RC(rc, cleanup);
        /* Parse the ghost_bits_to_hash_bit info */
        cJSON *ghost_bits_to_hash_bit_cjson = NULL;
        int new_err =
            bf_cjson_get_object(hash_function_cjson,
                                CTX_JSON_STAGE_TABLE_GHOST_BITS_TO_HASH_BIT,
                                &ghost_bits_to_hash_bit_cjson);
        if (new_err == 0) {
          /* For now ignore if the ghost bits to hash bit info is not present.
           * This is so that the compiler tests don't error out.
           */
          rc = ctx_json_parse_ghost_bits_to_hash_bit(
              ghost_bits_to_hash_bit_cjson, hash_field);
          CHECK_RC(rc, cleanup);
        }
      }

      ++index;
    }

    // Figure out whether this stage table is a proxy hash. If it is, parse
    // the proxy hash function.
    char *stage_table_type = NULL;
    err = bf_cjson_get_string(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_STAGE_TABLE_TYPE,
                              &stage_table_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_PROXY_HASH_MATCH)) {
      // Compute hash index.
      bj_index = bf_hash_comp_compute_lut_index(
          devid, prof_id, table_handle, stage, true);

      // There is only one hash function here.
      wide_hash_len = 1;

      // Update the look-up table entry with this stage, handle, size, and
      // hash type.
      lut_ptr = bf_hash_comp_update_lut_entry(
          bj_index, devid, prof_id, table_handle, stage, wide_hash_len, true);
      if (lut_ptr == NULL) {
        LOG_ERROR("%s:%d: Could not populate hash LUT.", __func__, __LINE__);
        rc = PIPE_INIT_ERROR;
        goto cleanup;
      }

      cJSON *proxy_hash_function_cjson = NULL;
      err = bf_cjson_get_object(stage_table_cjson,
                                CTX_JSON_STAGE_TABLE_PROXY_HASH_FUNCTION,
                                &proxy_hash_function_cjson);
      CHECK_ERR(err, rc, cleanup);

      uint16_t field_offset =
          bf_hash_comp_get_next_unused_field_offset(devid, prof_id);
      if (field_offset >=
          PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list) {
        LOG_ERROR("%s:%d: Could not find unused field offset for hash bits.",
                  __func__,
                  __LINE__);
        rc = PIPE_INIT_ERROR;
        goto cleanup;
      }

      // Only one field offset will be needed.
      lut_ptr->hash_field_offset[0] = field_offset;

      // Buffers used to store the bit masks for each bit to xor. These will
      // be zeroed out for each new bit.
      bf_hash_field_t *hash_field =
          bf_hash_comp_get_hash_field_ptr(devid, prof_id, field_offset);

      // Make sure the hash field has been allocated elsewhere.
      PIPE_MGR_ASSERT(hash_field != NULL);
      if (hash_field == NULL) {
        LOG_ERROR("%s:%d: NULL hash_field.", __func__, __LINE__);
        rc = PIPE_INIT_ERROR;
        goto cleanup;
      }
      PIPE_MGR_ASSERT(hash_field->hash_width == 0);

      cJSON *hash_bits_cjson = NULL;
      err = bf_cjson_get_object(proxy_hash_function_cjson,
                                CTX_JSON_HASH_FUNCTION_HASH_BITS,
                                &hash_bits_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Parse all hash bits.
      cJSON *hash_bit_cjson = NULL;
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        rc = ctx_json_parse_hash_bit(
            prof_info, hash_bit_cjson, match_key_fields_cjson, hash_field);
        CHECK_RC(rc, cleanup);
      }

      /*Parsing the JSON for generating information for new hash
       * Computation*/
      hash_field->key_length = key_length * 8;
      // Allocate memory for hash values
      hash_field->hash_values =
          PIPE_MGR_CALLOC(hash_field->key_length, sizeof(uint64_t));
      hash_field->hash_seed = 0;

      // Generate Bit wise hash values
      hash_bit_cjson = NULL;
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        rc = ctx_json_parse_update_hash_values(
            prof_info, hash_bit_cjson, match_key_fields_cjson, hash_field);
        CHECK_RC(rc, cleanup);
      }

      /*Generating the hash combination values for radix based hash
       * computation*/
      hash_field->radix_value =
          DEFAULT_HASH_RADIX_VALUE;  // ASSIGNING THE RADIX VALUE HERE
      hash_field->hash_combs_size = (key_length * 8 / hash_field->radix_value) *
                                    (1 << hash_field->radix_value);
      // Allocate memory for hash combinations
      hash_field->hash_combinations =
          PIPE_MGR_CALLOC(hash_field->hash_combs_size, sizeof(uint64_t));

      // Generate the hash combination values for a given radix
      rc = ctx_json_generate_hash_combinations(hash_field);
      CHECK_RC(rc, cleanup);
    }
  }

  PIPE_MGR_FREE(all_stage_tables_cjson);
  return rc;

cleanup:
  if (all_stage_tables_cjson) {
    PIPE_MGR_FREE(all_stage_tables_cjson);
  }
  LOG_ERROR("%s - Dev %d profile %d: Failed to parse table %s (0x%x)",
            __func__,
            devid,
            prof_id,
            name,
            table_handle);
  return rc;
}

/**
 * Given a list of tables, computes some metadata and allocates the global
 * look-up tables that will be populated later.
 *
 * @param devid The current device's id.
 * @param tables_cjson A cJSON array of tables.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_allocate_hash_luts(bf_dev_id_t devid,
                                                 profile_id_t prof_id,
                                                 cJSON *tables_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_DBG("%s:%d: Parsing metadata to allocate look-up tables.",
          __func__,
          __LINE__);

  // Compute a few details from the general ContextJSON structure.
  int table_count = 0;
  int total_hash_field_lists = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;
  int table_size = 0;
  int table_zero_size_count = 0;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse exact match entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto all_stage_tables_alloc_err;
  }

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);

    // Skip non-match tables
    if (strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      continue;
    }

    cJSON *match_attributes_cjson = NULL;
    err |= bf_cjson_get_object(table_cjson,
                               CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                               &match_attributes_cjson);
    CHECK_ERR(err, rc, parse_tables_err);

    char *match_type = NULL;
    rc |= bf_cjson_get_string(match_attributes_cjson,
                              CTX_JSON_MATCH_ATTRIBUTES_MATCH_TYPE,
                              &match_type);

    if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT) ||
        !strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_HASH_ACTION)) {
      // Fetch all stage tables for this table, and parse the hash bits.
      int number_stage_tables = 0;
      err |= ctx_json_parse_all_match_stage_tables_for_table(
          table_cjson,
          max_number_stages * max_number_logical_tables,
          all_stage_tables_cjson,
          &number_stage_tables);
      CHECK_ERR(err, rc, parse_tables_err);

      for (int i = 0; i < number_stage_tables; ++i) {
        cJSON *stage_table_cjson = all_stage_tables_cjson[i];
        PIPE_MGR_ASSERT(stage_table_cjson != NULL);

        cJSON *hash_functions_cjson = NULL;
        err |= bf_cjson_get_object(stage_table_cjson,
                                   CTX_JSON_STAGE_TABLE_HASH_FUNCTIONS,
                                   &hash_functions_cjson);
        CHECK_ERR(err, rc, parse_tables_err);

        total_hash_field_lists += cJSON_GetArraySize(hash_functions_cjson);

        char *stage_table_type = NULL;
        err |= bf_cjson_get_string(stage_table_cjson,
                                   CTX_JSON_STAGE_TABLE_STAGE_TABLE_TYPE,
                                   &stage_table_type);
        CHECK_ERR(err, rc, parse_tables_err);

        // Effectively, a proxy hash match adds a hash field list.
        if (!strcmp(stage_table_type,
                    CTX_JSON_STAGE_TABLE_TYPE_PROXY_HASH_MATCH)) {
          ++total_hash_field_lists;
        }
        /* Each stage table will be represented in the LUT. */
        ++table_count;
      }
      err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &table_size);
      CHECK_ERR(err, rc, parse_tables_err);
      /* Count tables with zero table size */
      if (table_size == 0) ++table_zero_size_count;
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ATCAM)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_TERNARY)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_NO_KEY)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_PHASE0)) {
      // Nothing to do.
    } else {
      LOG_ERROR("%s:%d: Invalid match_type found in ContextJSON: %s.",
                __func__,
                __LINE__,
                match_type);
      rc = PIPE_INVALID_ARG;
      goto parse_tables_err;
    }
  }

  PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_table_count = table_count;
  PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth =
      PIPEMGR_TBL_PKG_2POWER(table_count);
  PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list =
      total_hash_field_lists;

  // No hash bits to play with; simply return, we are done here.
  if (table_count == 0 && total_hash_field_lists == 0) {
    PIPE_MGR_FREE(all_stage_tables_cjson);
    return rc;
    /* Display a warning for tables which are expected to have hash fields but
     * found none while parsing. */
  } else if (table_count && !total_hash_field_lists &&
             table_zero_size_count < table_count) {
    LOG_WARN(
        "Dev %d, %d hash based tables but no hash fields!", devid, table_count);
  }
  LOG_TRACE("Dev %d, %d hash based tables %d hash fields",
            devid,
            table_count,
            total_hash_field_lists);
  if (total_hash_field_lists) {
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base = PIPE_MGR_CALLOC(
        PIPE_MGR_HASH_COMP_CTX(devid, prof_id).total_hash_field_list,
        sizeof(bf_hash_field_t));
    if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base == NULL) {
      rc = PIPE_NO_SYS_RESOURCES;
      goto field_base_alloc_err;
    }
  } else {
    PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base = NULL;
  }

  PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut =
      PIPE_MGR_CALLOC(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut_depth *
                          PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                      sizeof(bf_hash_tbl_lut));
  if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).lut == NULL) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto lut_alloc_err;
  }

  PIPE_MGR_FREE(all_stage_tables_cjson);
  return rc;

lut_alloc_err:
  if (PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base)
    PIPE_MGR_FREE(PIPE_MGR_HASH_COMP_CTX(devid, prof_id).field_base);
field_base_alloc_err:
parse_tables_err:
  PIPE_MGR_FREE(all_stage_tables_cjson);
all_stage_tables_alloc_err:
  return rc;
}

/**
 * Traverses a given list of tables and calls the necessary callbacks to
 * populate hash information, if any, according to table type and match type.
 * Assumes that look-up tables have already been allocated (by calling
 * ctx_json_allocate_hash_luts).
 *
 * @param devid The current device's id.
 * @param tables_cjson A cJSON array of tables.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_traverse_and_callback_hash(bf_dev_id_t devid,
                                                         profile_id_t prof_id,
                                                         cJSON *tables_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_DBG("%s:%d: Traversing tables to populate entry format structures.",
          __func__,
          __LINE__);

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
    CHECK_ERR(err, rc, cleanup);

    // Skip non-match tables
    if (strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      continue;
    }

    cJSON *match_attributes_cjson = NULL;
    err |= bf_cjson_get_object(table_cjson,
                               CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                               &match_attributes_cjson);
    CHECK_ERR(err, rc, cleanup);

    char *match_type = NULL;
    err |= bf_cjson_get_string(match_attributes_cjson,
                               CTX_JSON_MATCH_ATTRIBUTES_MATCH_TYPE,
                               &match_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT) ||
        !strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_HASH_ACTION)) {
      rc |= ctx_json_parse_hash_for_table(devid, prof_id, table_cjson);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ATCAM)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_TERNARY)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_NO_KEY)) {
      // Nothing to do.
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_PHASE0)) {
      // Nothing to do.
    } else {
      LOG_ERROR("%s:%d: Invalid match_type found in ContextJSON: %s.",
                __func__,
                __LINE__,
                match_type);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Main routine that is publicly exposed. Given a cJSON object representing
 * the entire ContextJSON object, this function parses its contents into the
 * driver's internal hash formatting. The first pass consists of allocating the
 * global context lookup table (LUT) structures. The second pass looks at each
 * particular table type and calls the appropriate callback to parse it.
 *
 * @param devid The device id in question.
 * @param root A cJSON object representing the ContextJSON.
 * @return A pipe_status_t object with the return code.
 */
pipe_status_t ctx_json_parse_hashes(bf_dev_id_t devid,
                                    profile_id_t prof_id,
                                    cJSON *json) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  (void)prof_id;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR("Invalid device id. Unable to complete hash module init");
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  LOG_DBG(
      "%s:%d: Parsing hash bit format from Context JSON.", __func__, __LINE__);

  cJSON *tables_cjson = NULL;
  err |= bf_cjson_get_object(json, CTX_JSON_TABLES_NODE, &tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Parse all the hash bit information from the tables.
  rc |= ctx_json_allocate_hash_luts(devid, prof_id, tables_cjson);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d: Could not allocate hash bit LUTs.", __func__, __LINE__);
    goto cleanup;
  }

  // Parse all the hash bit information from the tables.
  rc |= ctx_json_traverse_and_callback_hash(devid, prof_id, tables_cjson);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d: Could not populate hash bit LUTs.", __func__, __LINE__);
    goto cleanup;
  }

  LOG_DBG("%s:%d: Successfully parsed hash bit format from Context JSON.",
          __func__,
          __LINE__);
  return rc;

cleanup:
  return rc;
}
