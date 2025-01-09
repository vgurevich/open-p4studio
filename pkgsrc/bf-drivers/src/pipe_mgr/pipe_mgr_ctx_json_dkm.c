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


/**
 * @file pipe_mgr_ctx_json_dkm.c
 *
 * Parser routines for populating dynamic key masks format information from the
 * ContextJSON file.
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <target-utils/third-party/cJSON/cJSON.h>

#include <bf_types/bf_types.h>
#include "pipe_mgr_dkm.h"
#include "pipe_mgr_log.h"

#include "pipe_mgr_ctx_json.h"

static void free_dkm_cfg(pipemgr_dkm_ms_cfg_t *ms_cfg) {
  if (!ms_cfg) return;
  if (ms_cfg->hash) {
    for (unsigned int stage = 0; stage < ms_cfg->num_stages; ++stage) {
      if (ms_cfg->hash[stage]) {
        for (unsigned int ms_bit = 0; ms_bit < ms_cfg->num_ms_bits; ++ms_bit) {
          if (ms_cfg->hash[stage][ms_bit].bits) {
            PIPE_MGR_FREE(ms_cfg->hash[stage][ms_bit].bits);
            ms_cfg->hash[stage][ms_bit].bits = NULL;
          }
        }
        PIPE_MGR_FREE(ms_cfg->hash[stage]);
        ms_cfg->hash[stage] = NULL;
      }
    }
    PIPE_MGR_FREE(ms_cfg->hash);
    ms_cfg->hash = NULL;
  }
  PIPE_MGR_FREE(ms_cfg);
}

/**
 * Parses the dynamic key mask information for a given table.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to this table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_dkm_for_table(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    cJSON *table_cjson,
    pipe_mat_tbl_info_t *mat_tbl_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipemgr_dkm_lut_t *dkm_lut = NULL;
  pipemgr_dkm_ms_cfg_t *ms_cfg = NULL;
  bf_dev_id_t devid = dev_info->dev_id;

  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];
  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);

  char *name = NULL;
  int table_handle = 0;
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing dynamic key masks format for table handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  int number_stages = dev_info->num_active_mau;

  dkm_lut = PIPE_MGR_CALLOC(1, sizeof *dkm_lut);
  if (dkm_lut == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for dynamic key mask tbl.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  dkm_lut->exm_tbl_hdl = table_handle;

  ms_cfg = PIPE_MGR_CALLOC(1, sizeof *ms_cfg);
  if (ms_cfg == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for DKM information.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  dkm_lut->dkm_cfg = ms_cfg;

  ms_cfg->num_stages = number_stages;
  ms_cfg->num_ms_bits = mat_tbl_info->num_match_bytes * 8;

  ms_cfg->hash = PIPE_MGR_CALLOC(number_stages, sizeof *ms_cfg->hash);
  if (!ms_cfg->hash) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  /* For each stage table, parse the hash functions. */
  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_tables_cjson = NULL;
  err = bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    int stage_number = 0;
    err = bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage_number);
    CHECK_ERR(err, rc, cleanup);
    if (stage_number < 0 || stage_number >= number_stages) {
      LOG_ERROR(
          "Unexpected stage id, %d, while parsing DKM table %s (0x%x), dev %d "
          "has %d stages",
          stage_number,
          name,
          table_handle,
          dev_info->dev_id,
          dev_info->num_active_mau);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    cJSON *hash_functions_cjson = NULL;
    err = bf_cjson_get_object(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_HASH_FUNCTIONS,
                              &hash_functions_cjson);
    CHECK_ERR(err, rc, cleanup);

    // For each hash function...
    cJSON *hash_function_cjson = NULL;
    CTX_JSON_FOR_EACH(hash_function_cjson, hash_functions_cjson) {
      cJSON *hash_bits_cjson = NULL;
      err = bf_cjson_get_object(hash_function_cjson,
                                CTX_JSON_HASH_FUNCTION_HASH_BITS,
                                &hash_bits_cjson);
      CHECK_ERR(err, rc, cleanup);

      cJSON *hash_bit_cjson = NULL;
      CTX_JSON_FOR_EACH(hash_bit_cjson, hash_bits_cjson) {
        /* If we do not yet have state for this stage set it up. */
        if (!ms_cfg->hash[stage_number]) {
          ms_cfg->hash[stage_number] =
              PIPE_MGR_CALLOC(ms_cfg->num_ms_bits, sizeof **ms_cfg->hash);
          if (!ms_cfg->hash[stage_number]) {
            LOG_ERROR("Failed to allocate memory for DKM table %s, 0x%x",
                      name,
                      table_handle);
            rc = PIPE_NO_SYS_RESOURCES;
            goto cleanup;
          }
        }
        pipe_mgr_dkm_gfm_cfg_t *gfm_cfg = ms_cfg->hash[stage_number];

        /* Get the hash bit the "bits_to_xor" will map to, this is the GFM
         * column. */
        int hash_bit = 0;
        err = bf_cjson_get_int(
            hash_bit_cjson, CTX_JSON_HASH_BITS_HASH_BIT, &hash_bit);
        CHECK_ERR(err, rc, cleanup);

        /* Get all the field bits (i.e. match spec bits) which contribute to
         * this hash bit. */
        cJSON *bits_to_xor_cjson = NULL;
        err = bf_cjson_get_object(
            hash_bit_cjson, CTX_JSON_HASH_BIT_BITS_TO_XOR, &bits_to_xor_cjson);
        CHECK_ERR(err, rc, cleanup);
        cJSON *bit_to_xor_cjson = NULL;
        CTX_JSON_FOR_EACH(bit_to_xor_cjson, bits_to_xor_cjson) {
          /* Each entry in the "bits_to_xor" list has a few pieces of
          information we use to build state saved against the DKM table to allow
          us to reprogram the Galois Field Matrix (GFM) to include or exclude a
          key bit from the exact match table's hash.
          We need to know the offset of the key bit in the match spec, this is
          computed using the field's name and bit position within the field.  We
          then need to know where that key bit appears in the input to the GFM
          so we know which config register to update to enable or disable that
          bit. */
          char *field_name_to_xor = NULL;
          bf_cjson_try_get_string(bit_to_xor_cjson,
                                  CTX_JSON_BIT_TO_XOR_GLOBAL_NAME,
                                  &field_name_to_xor);
          if (!field_name_to_xor || *field_name_to_xor == '\0') {
            /* Was not able to find a "global_name" entry to identify this
             * field.  Fall back to using "field_name" instead.  Schema versions
             * before 1.12.0 will not have "global_name" and this allows us to
             * be compatible with those earlier versions. */
            err = bf_cjson_get_string(bit_to_xor_cjson,
                                      CTX_JSON_BIT_TO_XOR_FIELD_NAME,
                                      &field_name_to_xor);
            CHECK_ERR(err, rc, cleanup);
          }

          int field_bit = 0;
          err = bf_cjson_get_int(
              bit_to_xor_cjson, CTX_JSON_BIT_TO_XOR_FIELD_BIT, &field_bit);
          CHECK_ERR(err, rc, cleanup);

          int ms_bit = 0;
          err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                      match_key_fields_cjson,
                                                      field_name_to_xor,
                                                      field_bit,
                                                      1,
                                                      NULL,
                                                      NULL,
                                                      &ms_bit,
                                                      NULL);
          CHECK_ERR(err, rc, cleanup);
          if (ms_bit < 0 || (unsigned int)ms_bit >= ms_cfg->num_ms_bits) {
            LOG_ERROR(
                "Unexpected match spec bit while parsing %s (0x%x) on dev %d: "
                "field %s, field_bit %d, ms_bit %d",
                name,
                table_handle,
                devid,
                field_name_to_xor,
                field_bit,
                ms_bit);
            rc = PIPE_UNEXPECTED;
            goto cleanup;
          }

          /* Parse the hash match group and bit within group to determine the
           * key bit's position in the GFM input and therefore which GFM
           * register to reprogram. */
          int hash_match_group = 0;
          int hash_match_group_bit = 0;
          err = bf_cjson_get_int(bit_to_xor_cjson,
                                 CTX_JSON_BIT_TO_XOR_HASH_MATCH_GROUP,
                                 &hash_match_group);
          CHECK_ERR(err, rc, cleanup);
          err = bf_cjson_get_int(bit_to_xor_cjson,
                                 CTX_JSON_BIT_TO_XOR_HASH_MATCH_GROUP_BIT,
                                 &hash_match_group_bit);
          CHECK_ERR(err, rc, cleanup);

          pipe_mgr_dkm_gfm_bit_t *temp = PIPE_MGR_REALLOC(
              gfm_cfg[ms_bit].bits,
              sizeof(pipe_mgr_dkm_gfm_bit_t) * (gfm_cfg[ms_bit].num_bits + 1));
          if (!temp) {
            rc = PIPE_NO_SYS_RESOURCES;
            goto cleanup;
          }
          gfm_cfg[ms_bit].bits = temp;
          int idx = gfm_cfg[ms_bit].num_bits;
          gfm_cfg[ms_bit].bits[idx].col = hash_bit;
          gfm_cfg[ms_bit].bits[idx].grp = hash_match_group;
          gfm_cfg[ms_bit].bits[idx].grp_bit = hash_match_group_bit;
          gfm_cfg[ms_bit].num_bits++;
        }
      }
    }
  }

  /* Save the DKM state. */
  bf_map_sts_t msts = pipe_mgr_dkm_tbl_map_add(devid, table_handle, dkm_lut);
  if (msts != BF_MAP_OK) {
    rc = PIPE_UNEXPECTED;
    LOG_ERROR("Dev %d: Error storing DKM table state for %s (0x%x)",
              devid,
              name,
              table_handle);
    goto cleanup;
  }

  return rc;

cleanup:
  if (dkm_lut) {
    PIPE_MGR_FREE(dkm_lut);
  }
  if (ms_cfg) {
    free_dkm_cfg(ms_cfg);
  }
  return rc;
}

void pipemgr_dkm_cleanup(bf_dev_id_t dev_id) {
  pipemgr_dkm_lut_t *dkm_lut = NULL;

  while (BF_MAP_OK ==
         pipe_mgr_dkm_tbl_map_get_first_rmv(dev_id, (void **)&dkm_lut)) {
    free_dkm_cfg(dkm_lut->dkm_cfg);
    PIPE_MGR_FREE(dkm_lut);
    dkm_lut = NULL;
  }
}

/**
 * Parse the dynamic key mask information for all tables that use this feature,
 * allowing key-masks to be modified at run-time.
 *
 * @param devid The current device's id.
 * @param tables_cjson The cJSON structure corresponding to the entire
 * ContextJSON.
 *
 * @return A pipe_status_t containing the return code.
 */
pipe_status_t ctx_json_parse_dkm(rmt_dev_info_t *dev_info,
                                 profile_id_t prof_id,
                                 cJSON *root,
                                 rmt_dev_tbl_info_t *tbl_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_DBG("%s:%d: Started parsing dynamic key masks from ContextJSON.",
          __func__,
          __LINE__);

  cJSON *tables_cjson = NULL;
  err = bf_cjson_get_object(root, CTX_JSON_TABLES_NODE, &tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err = bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
    CHECK_ERR(err, rc, cleanup);

    if (strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      continue;
    }

    int handle = 0;
    err = bf_cjson_get_handle(
        dev_info->dev_id, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
    CHECK_ERR(err, rc, cleanup);
    pipe_mat_tbl_hdl_t table_handle = handle;

    // Get match type: only look for dynamic key masks in exact match tables.
    cJSON *match_attributes_cjson = NULL;
    err = bf_cjson_get_object(table_cjson,
                              CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                              &match_attributes_cjson);
    CHECK_ERR(err, rc, cleanup);

    char *match_type = NULL;
    err = bf_cjson_get_string(match_attributes_cjson,
                              CTX_JSON_MATCH_ATTRIBUTES_MATCH_TYPE,
                              &match_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT)) {
      bool uses_dynamic_key_masks = false;
      err = bf_cjson_get_bool(match_attributes_cjson,
                              CTX_JSON_MATCH_ATTRIBUTES_USES_DYNAMIC_KEY_MASKS,
                              &uses_dynamic_key_masks);
      CHECK_ERR(err, rc, cleanup);

      if (uses_dynamic_key_masks) {
        /* Find the info already parsed for the match table so the DKM parsing
         * can reuse it. */
        pipe_mat_tbl_info_t *mat_tbl_info = NULL;
        for (unsigned i = 0; i < tbl_info->num_mat_tbls; ++i) {
          if (tbl_info->mat_tbl_list[i].handle == table_handle) {
            mat_tbl_info = &tbl_info->mat_tbl_list[i];
            break;
          }
        }
        if (!mat_tbl_info) {
          LOG_ERROR(
              "Cannot find MAT info for table 0x%x while parsing its DKM info",
              table_handle);
          rc = PIPE_INTERNAL_ERROR;
          goto cleanup;
        }
        rc = ctx_json_parse_dkm_for_table(
            dev_info, prof_id, table_cjson, mat_tbl_info);
        CHECK_RC(rc, cleanup);
      }
    }
  }

  return rc;

cleanup:
  pipemgr_dkm_cleanup(dev_info->dev_id);
  return rc;
}
