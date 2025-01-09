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


/* Standard header includes */
#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <math.h>

#include "pipe_mgr_ctx_json.h"
#include "pipe_mgr_int.h"
#include "pipe_mgr_db.h"
#include "pipe_mgr_rmt_cfg.h"
#include <bfutils/dynamic_hash/bfn_hash_algorithm.h>

#define PIPE_MGR_PARSE_CFG_NAME(cfg_name, out, out_max)                    \
  {                                                                        \
    char *save_ptr = NULL;                                                 \
    strtok_r(cfg_name, "[", &save_ptr);                                    \
    char *token = strtok_r(NULL, "]", &save_ptr);                          \
    if (!token) {                                                          \
      LOG_ERROR("%s:%d Failed to parse %s", __func__, __LINE__, cfg_name); \
      return PIPE_INVALID_ARG;                                             \
    }                                                                      \
    out = atoi(token);                                                     \
    if ((out < 0) || (out >= out_max)) {                                   \
      LOG_ERROR(                                                           \
          "%s:%d Invalid " #out " %d provided", __func__, __LINE__, out);  \
      return PIPE_INVALID_ARG;                                             \
    }                                                                      \
  }

static rmt_dev_tbl_info_t *g_table_info = NULL;

/**
 * Allocates a new match table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated match table.
 */
static pipe_mat_tbl_info_t *allocate_match_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_mat_tbls++;
  g_table_info->mat_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->mat_tbl_list,
      (g_table_info->num_mat_tbls * sizeof(pipe_mat_tbl_info_t)));
  if (g_table_info->mat_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for match table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_mat_tbl_info_t *tbl =
      &(g_table_info->mat_tbl_list[(g_table_info->num_mat_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));

  // Disable duplicate entry check by default.
  tbl->duplicate_entry_check = false;
  tbl->symmetric = true;
  return tbl;
}

/**
 * Allocates a new action data table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated action data table.
 */
static pipe_adt_tbl_info_t *allocate_action_data_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_adt_tbls++;
  g_table_info->adt_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->adt_tbl_list,
      (g_table_info->num_adt_tbls * sizeof(pipe_adt_tbl_info_t)));
  if (g_table_info->adt_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for action data table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_adt_tbl_info_t *tbl =
      &(g_table_info->adt_tbl_list[(g_table_info->num_adt_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));
  tbl->symmetric = true;

  return tbl;
}

/**
 * Allocates a new selection table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated selection table.
 */
static pipe_select_tbl_info_t *allocate_selection_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_select_tbls++;
  g_table_info->select_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->select_tbl_list,
      (g_table_info->num_select_tbls * sizeof(pipe_select_tbl_info_t)));
  if (g_table_info->select_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for selection table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_select_tbl_info_t *tbl =
      &(g_table_info->select_tbl_list[(g_table_info->num_select_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));

  tbl->symmetric = true;
  return tbl;
}

/**
 * Allocates a new statistics table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated statistics table.
 */
static pipe_stat_tbl_info_t *allocate_statistics_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_stat_tbls++;
  g_table_info->stat_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->stat_tbl_list,
      (g_table_info->num_stat_tbls * sizeof(pipe_stat_tbl_info_t)));
  if (g_table_info->stat_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for statistics table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_stat_tbl_info_t *tbl =
      &(g_table_info->stat_tbl_list[(g_table_info->num_stat_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));

  tbl->symmetric = true;
  return tbl;
}

/**
 * Allocates a new meter table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated meter table.
 */
static pipe_meter_tbl_info_t *allocate_meter_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_meter_tbls++;
  g_table_info->meter_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->meter_tbl_list,
      (g_table_info->num_meter_tbls * sizeof(pipe_meter_tbl_info_t)));
  if (g_table_info->meter_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for meter table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_meter_tbl_info_t *tbl =
      &(g_table_info->meter_tbl_list[(g_table_info->num_meter_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));

  tbl->symmetric = true;
  return tbl;
}

/**
 * Allocates a new stateful table in the global g_table_info structure and
 * returns it. It reallocs the global structure every time.
 *
 * @return A pointer to the newly allocated stateful table.
 */
static pipe_stful_tbl_info_t *allocate_stateful_table() {
  // Reallocate list of tables and update metadata.
  g_table_info->num_sful_tbls++;
  g_table_info->stful_tbl_list = PIPE_MGR_REALLOC(
      g_table_info->stful_tbl_list,
      (g_table_info->num_sful_tbls * sizeof(pipe_stful_tbl_info_t)));
  if (g_table_info->stful_tbl_list == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for stateful table.",
              __func__,
              __LINE__);
    return NULL;
  }

  pipe_stful_tbl_info_t *tbl =
      &(g_table_info->stful_tbl_list[(g_table_info->num_sful_tbls) - 1]);
  PIPE_MGR_ASSERT(tbl != NULL);
  PIPE_MGR_MEMSET(tbl, 0, sizeof(*tbl));

  tbl->symmetric = true;
  return tbl;
}

/**
 * Allocates a new rmt table in a given table's rmt_info structure, and returns
 * it. It reallocs the rmt_info structure every time.
 *
 * @return Index of the newly allocated table.
 */
static int allocate_rmt_table(uint32_t *num_rmt_info,
                              rmt_tbl_info_t **rmt_info) {
  ++*num_rmt_info;
  *rmt_info =
      PIPE_MGR_REALLOC(*rmt_info, ((*num_rmt_info) * sizeof(rmt_tbl_info_t)));
  if (*rmt_info == NULL) {
    LOG_ERROR(
        "%s:%d: Failed to allocate memory for rmt table.", __func__, __LINE__);
    return -1;
  }

  int index = (*num_rmt_info) - 1;
  rmt_tbl_info_t *rmt = &((*rmt_info)[index]);
  PIPE_MGR_ASSERT((rmt != NULL));
  PIPE_MGR_MEMSET(rmt, 0, sizeof(*rmt));

  return index;
}

/**
 * Parses the memory banks within a memory resource allocation structure.
 *
 * @param refs_cjson The references cJSON structure.
 * @param ref_type A pointer that will be set to the correct reference type.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_reference_type(
    cJSON *cjson, pipe_tbl_ref_type_t *ref_type) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *how_referenced = NULL;
  err |= bf_cjson_get_string(
      cjson, CTX_JSON_TABLE_HOW_REFERENCED, &how_referenced);
  CHECK_ERR(err, rc, cleanup);

  if (!strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_DIRECT)) {
    *ref_type = PIPE_TBL_REF_TYPE_DIRECT;
  } else if (!strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_INDIRECT)) {
    *ref_type = PIPE_TBL_REF_TYPE_INDIRECT;
  } else {
    LOG_ERROR("%s:%d: Invalid ContextJSON format: property %s with value %s.",
              __func__,
              __LINE__,
              CTX_JSON_TABLE_HOW_REFERENCED,
              how_referenced);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the memory banks within a memory resource allocation structure.
 *
 * @param refs_cjson The references cJSON structure.
 * @param refs_ret A pointer that will be set to the correct reference type.
 * @param number_refs_ret A pointer that will be set to the number of
 * references.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_match_references(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *refs_cjson,
    pipe_tbl_ref_t **refs_ret,
    uint32_t *number_refs_ret) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  PIPE_MGR_ASSERT(refs_cjson != NULL);
  PIPE_MGR_ASSERT(number_refs_ret != NULL);

  *number_refs_ret = cJSON_GetArraySize(refs_cjson);
  if (*number_refs_ret == 0) {
    *refs_ret = NULL;
    return rc;
  } else if (*number_refs_ret > 1) {
    LOG_ERROR(
        "%s:%d: Unexpected reference count %d found in match table, which only "
        "supports at most one such reference",
        __func__,
        __LINE__,
        *number_refs_ret);
    return PIPE_UNEXPECTED;
  }

  *refs_ret = PIPE_MGR_CALLOC(*number_refs_ret, sizeof(pipe_tbl_ref_t));
  if (*refs_ret == NULL) {
    LOG_ERROR("%s:%d: Failed to allocate memory for match table references.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  cJSON *ref_cjson = NULL;
  int index = 0;
  CTX_JSON_FOR_EACH(ref_cjson, refs_cjson) {
    int handle = 0;
    pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;

    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               ref_cjson,
                               CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE,
                               &handle);
    CHECK_ERR(err, rc, cleanup);

    rc |= ctx_json_parse_rmt_cfg_reference_type(ref_cjson, &ref_type);
    CHECK_RC(rc, cleanup);

    (*refs_ret)[index].tbl_hdl = handle;
    (*refs_ret)[index].ref_type = ref_type;
    char *color_mapram_addr_type = NULL;
    rc |= bf_cjson_try_get_string(
        ref_cjson, CTX_JSON_COLOR_MAPRAM_ADDR_TYPE, &color_mapram_addr_type);
    if (color_mapram_addr_type) {
      if (!strcmp(color_mapram_addr_type,
                  CTX_JSON_COLOR_MAPRAM_ADDR_TYPE_IDLE)) {
        (*refs_ret)[index].color_mapram_addr_type = COLOR_MAPRAM_ADDR_TYPE_IDLE;
      } else if (!strcmp(color_mapram_addr_type,
                         CTX_JSON_COLOR_MAPRAM_ADDR_TYPE_STATS)) {
        (*refs_ret)[index].color_mapram_addr_type =
            COLOR_MAPRAM_ADDR_TYPE_STATS;
      } else {
        LOG_ERROR(
            "%s:%d: Invalid ContextJSON format : property %s with value %s.",
            __func__,
            __LINE__,
            CTX_JSON_COLOR_MAPRAM_ADDR_TYPE,
            color_mapram_addr_type);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
    }
    ++index;
  }
  return rc;

cleanup:
  PIPE_MGR_FREE(*refs_ret);
  *refs_ret = NULL;
cleanup_no_free:
  return rc;
}

/**
 * Parses the memory banks within a memory resource allocation structure.
 *
 * @param many_memory_units_and_vpns_cjson A cJSON array of memory_units and
 * vpns arrays.
 * @param bank A pointer to the internal bank RMT configuration to be populated.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_memory_units(
    cJSON *many_memory_units_and_vpns_cjson, rmt_tbl_bank_map_t *bank) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int number_memory_units =
      cJSON_GetArraySize(many_memory_units_and_vpns_cjson);
  rmt_tbl_word_blk_t *tbl_word_block =
      PIPE_MGR_CALLOC(number_memory_units, sizeof(rmt_tbl_word_blk_t));
  if (tbl_word_block == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for stage table memory banks.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  int memory_unit = 0;
  cJSON *memory_units_and_vpns_cjson = NULL;
  CTX_JSON_FOR_EACH(memory_units_and_vpns_cjson,
                    many_memory_units_and_vpns_cjson) {
    cJSON *memory_units_cjson = NULL;
    err |= bf_cjson_get_object(memory_units_and_vpns_cjson,
                               CTX_JSON_MEMORY_UNITS_AND_VPNS_MEMORY_UNITS,
                               &memory_units_cjson);
    CHECK_ERR(err, rc, cleanup);

    int memory_id = 0;
    cJSON *memory_unit_cjson = NULL;
    CTX_JSON_FOR_EACH(memory_unit_cjson, memory_units_cjson) {
      tbl_word_block[memory_unit].mem_id[memory_id] =
          memory_unit_cjson->valueint;
      ++memory_id;
    }

    cJSON *vpns_cjson = NULL;
    err |= bf_cjson_get_object(memory_units_and_vpns_cjson,
                               CTX_JSON_MEMORY_UNITS_AND_VPNS_VPNS,
                               &vpns_cjson);
    CHECK_ERR(err, rc, cleanup);

    int vpn_id = 0;
    cJSON *vpn_cjson = NULL;
    CTX_JSON_FOR_EACH(vpn_cjson, vpns_cjson) {
      tbl_word_block[memory_unit].vpn_id[vpn_id] = vpn_cjson->valueint;
      ++vpn_id;
    }

    ++memory_unit;
  }

  bank->tbl_word_blk = tbl_word_block;
  bank->num_tbl_word_blks = number_memory_units;
  return rc;

cleanup:
  PIPE_MGR_FREE(tbl_word_block);
cleanup_no_free:
  return rc;
}

/**
 * Parses common memory resource allocation fields. These should be present
 * in all stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_common_memory_resource_allocation(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // Parse the memory resource allocation object.
  cJSON *mra_cjson = NULL;
  bf_cjson_try_get_object(stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                          &mra_cjson);

  // Some types of tables won't have any memory associated to them. We default
  // to these values.
  if (mra_cjson == NULL) {
    rmt_table->mem_type = RMT_MEM_SRAM;
    rmt_table->num_spare_rams = 0;
    return rc;
  }

  char *memory_type = NULL;
  err |= bf_cjson_get_string(
      mra_cjson, CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_TYPE, &memory_type);
  CHECK_ERR(err, rc, cleanup);

  if (!strcmp(memory_type, "tcam")) {
    rmt_table->mem_type = RMT_MEM_TCAM;
  } else if (!strcmp(memory_type, "sram")) {
    rmt_table->mem_type = RMT_MEM_SRAM;
  } else if (!strcmp(memory_type, "map_ram")) {
    rmt_table->mem_type = RMT_MEM_MAP_RAM;
  } else if (!strcmp(memory_type, "gateway")) {
    rmt_table->mem_type = RMT_MEM_MAP_RAM;
  } else if (!strcmp(memory_type, "ingress_buffer")) {
    rmt_table->mem_type = RMT_MEM_INGRESS_BUFFER;










  } else {
    LOG_ERROR("%s:%d: Invalid memory type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              memory_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  int spare_ram = 0;
  rmt_table->num_spare_rams = 0;
  // Parse the spare bank memory unit object.
  cJSON *spare_banks_cjson = NULL;
  bf_cjson_try_get_object(
      mra_cjson,
      CTX_JSON_MEMORY_RESOURCE_ALLOCATION_SPARE_BANK_MEMORY_UNIT,
      &spare_banks_cjson);
  if (spare_banks_cjson == NULL) return rc;
  if (spare_banks_cjson->type == cJSON_Array) {
    cJSON *spare_bank_cjson = NULL;
    CTX_JSON_FOR_EACH(spare_bank_cjson, spare_banks_cjson) {
      if (rmt_table->num_spare_rams >= RMT_MAX_S2P_SECTIONS) {
        LOG_ERROR(
            "%s:%d: Invalid ContextJSON format: property \"%s\" too many array "
            "items.",
            __FILE__,
            __LINE__,
            CTX_JSON_MEMORY_RESOURCE_ALLOCATION_SPARE_BANK_MEMORY_UNIT);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      if (spare_bank_cjson->type != cJSON_Number) {
        LOG_ERROR(
            "%s:%d: Invalid ContextJSON format: property \"%s\" array item is "
            "not a number.",
            __FILE__,
            __LINE__,
            CTX_JSON_MEMORY_RESOURCE_ALLOCATION_SPARE_BANK_MEMORY_UNIT);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      rmt_table->spare_rams[rmt_table->num_spare_rams++] =
          spare_bank_cjson->valueint;
    }
  } else {
    err |= bf_cjson_try_get_int(
        mra_cjson,
        CTX_JSON_MEMORY_RESOURCE_ALLOCATION_SPARE_BANK_MEMORY_UNIT,
        &spare_ram);
    CHECK_ERR(err, rc, cleanup);
    rmt_table->spare_rams[0] = spare_ram;
    rmt_table->num_spare_rams = 1;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses the memory resource allocation fields. These should be used for
 * most types of stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_atcam_memory_resource_allocation(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_table->mem_type = RMT_MEM_SRAM;
  rmt_table->num_spare_rams = 0;

  // Notice that ATCAMs have an *array* of memory resource allocation objects,
  // one correspoding to each partition.
  cJSON *mras_cjson = NULL;
  err |= bf_cjson_get_object(stage_table_cjson,
                             CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                             &mras_cjson);

  rmt_table->num_tbl_banks = cJSON_GetArraySize(mras_cjson);
  rmt_tbl_bank_map_t *banks =
      PIPE_MGR_CALLOC(rmt_table->num_tbl_banks, sizeof(rmt_tbl_bank_map_t));
  if (banks == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for stage table memory banks.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  int column_index = 0;
  cJSON *mra_cjson = NULL;
  CTX_JSON_FOR_EACH(mra_cjson, mras_cjson) {
    cJSON *many_memory_units_and_vpns_cjson = NULL;
    err |= bf_cjson_get_int(mra_cjson,
                            CTX_JSON_MEMORY_RESOURCE_ALLOCATION_COLUMN_PRIORITY,
                            &column_index);
    err |= bf_cjson_get_object(
        mra_cjson,
        CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
        &many_memory_units_and_vpns_cjson);
    CHECK_ERR(err, rc, cleanup);

    if (column_index >= rmt_table->num_tbl_banks) {
      LOG_ERROR("%s:%d: Column priority exceeds total number of columns.",
                __func__,
                __LINE__);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    // Parse the memory units.
    rc |= ctx_json_parse_rmt_cfg_memory_units(many_memory_units_and_vpns_cjson,
                                              &banks[column_index]);
    CHECK_RC(rc, cleanup);
  }

  rmt_table->bank_map = banks;
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the memory resource allocation fields. These should be used for
 * most types of stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_memory_resource_allocation(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // Some types of tables won't have a memory resource allocation object. We
  // default the values for those cases.
  cJSON *mra_cjson = NULL;
  bf_cjson_try_get_object(stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                          &mra_cjson);
  if (mra_cjson == NULL) {
    rmt_table->num_tbl_banks = 0;
    rmt_table->mem_type = RMT_MEM_SRAM;
    rmt_table->num_spare_rams = 0;
    return rc;
  }

  // Parse common fields.
  rc |= ctx_json_parse_rmt_cfg_common_memory_resource_allocation(
      stage_table_cjson, rmt_table);
  CHECK_RC(rc, cleanup_no_free);

  rmt_table->num_tbl_banks = 1;
  rmt_tbl_bank_map_t *banks =
      PIPE_MGR_CALLOC(rmt_table->num_tbl_banks, sizeof(rmt_tbl_bank_map_t));
  if (banks == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for stage table memory banks.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  cJSON *many_memory_units_and_vpns_cjson = NULL;
  err |= bf_cjson_get_object(
      mra_cjson,
      CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
      &many_memory_units_and_vpns_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Parse the memory units.
  rc |= ctx_json_parse_rmt_cfg_memory_units(many_memory_units_and_vpns_cjson,
                                            banks);
  CHECK_RC(rc, cleanup);
  rmt_table->bank_map = banks;

  // Parse color memory resource allocation - will be present only for meter
  // tables
  cJSON *cmra_cjson = NULL;
  bf_cjson_try_get_object(stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_COLOR_MEMORY_RESOURCE_ALLOCATION,
                          &cmra_cjson);
  if (cmra_cjson != NULL) {
    cJSON *color_memory_units_and_vpns_cjson = NULL;
    err |= bf_cjson_get_object(
        cmra_cjson,
        CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
        &color_memory_units_and_vpns_cjson);
    CHECK_ERR(err, rc, cleanup);
    rmt_tbl_bank_map_t *color_banks =
        PIPE_MGR_CALLOC(1, sizeof(rmt_tbl_bank_map_t));
    if (color_banks == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for stage table color memory "
          "banks.",
          __FILE__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup_no_free;
    }
    // Parse the memory units.
    rc |= ctx_json_parse_rmt_cfg_memory_units(color_memory_units_and_vpns_cjson,
                                              color_banks);
    CHECK_RC(rc, cleanup);
    rmt_table->color_bank_map = color_banks;
  }

  return rc;

cleanup:
  PIPE_MGR_FREE(banks);
cleanup_no_free:
  return rc;
}

/**
 * Parses the hash match memory resource allocation fields. These are only
 * used in stage tables that have ways, such as hash match and most
 * exact match stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t
ctx_json_parse_rmt_cfg_hash_match_memory_resource_allocation(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table, int *schema_version) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // The hash match RMT itself won't have any spare RAM, and the memory type
  // is SRAM. The memory resource allocation object should be null.

  rmt_table->num_spare_rams = 0;

  cJSON *ways_cjson = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_WAYS, &ways_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  rmt_table->num_tbl_banks = cJSON_GetArraySize(ways_cjson);
  if (rmt_table->num_tbl_banks == 0) {
    // Nothing else to do.
    rmt_table->bank_map = NULL;
    return rc;
  }

  rmt_tbl_bank_map_t *banks =
      PIPE_MGR_CALLOC(rmt_table->num_tbl_banks, sizeof(rmt_tbl_bank_map_t));
  if (banks == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for stage table memory bank.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  int way = 0;
  cJSON *way_cjson = NULL;
  CTX_JSON_FOR_EACH(way_cjson, ways_cjson) {
    cJSON *mra_cjson = NULL;
    err |= bf_cjson_get_object(
        way_cjson, CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION, &mra_cjson);
    CHECK_ERR(err, rc, cleanup);
    char *mem_type = NULL;
    /* Schema version 1.12.3 or later has memory type info. */
    if (schema_version[0] <= 1 && schema_version[1] <= 12 &&
        schema_version[2] < 3) {
      rmt_table->mem_type = RMT_MEM_SRAM;
    } else {
      err |=
          bf_cjson_get_string(mra_cjson,
                              CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_TYPE,
                              &mem_type);
      CHECK_ERR(err, rc, cleanup);
      if (!strcmp(mem_type, "sram")) {
        rmt_table->mem_type = RMT_MEM_SRAM;






      }
    }
    cJSON *many_memory_units_and_vpns_cjson = NULL;
    err |= bf_cjson_get_object(
        mra_cjson,
        CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
        &many_memory_units_and_vpns_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Parse the memory units.
    rc |= ctx_json_parse_rmt_cfg_memory_units(many_memory_units_and_vpns_cjson,
                                              &banks[way]);
    CHECK_RC(rc, cleanup);

    // Parse hash function and bits information.
    int hash_function_id = 0;
    int hash_select_bit_lo = 0;
    int hash_select_bit_hi = 0;
    int hash_entry_bit_lo = 0;
    int hash_entry_bit_hi = 0;
    int number_select_bits = 0;
    int number_entry_bits = 0;
    int number_subword_bits = 0;
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_HASH_FUNCTION_ID,
        &hash_function_id);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_HASH_SELECT_BIT_LO,
        &hash_select_bit_lo);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_HASH_SELECT_BIT_HI,
        &hash_select_bit_hi);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_HASH_ENTRY_BIT_LO,
        &hash_entry_bit_lo);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_HASH_ENTRY_BIT_HI,
        &hash_entry_bit_hi);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_NUMBER_SELECT_BITS,
        &number_select_bits);
    err |= bf_cjson_get_int(
        mra_cjson,
        CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_NUMBER_ENTRY_BITS,
        &number_entry_bits);
    if (schema_version[0] <= 1 && schema_version[1] <= 12 &&
        schema_version[2] < 3) {
      number_subword_bits = 0;
    } else {
      err |= bf_cjson_get_int(
          mra_cjson,
          CTX_JSON_HASH_MATCH_MEMORY_RESOURCE_ALLOCATION_NUMBER_SUB_WORD_BITS,
          &number_subword_bits);
    }
    CHECK_ERR(err, rc, cleanup);

    banks[way].hash_bits.function = hash_function_id;
    banks[way].hash_bits.ram_unit_select_bit_lo = hash_select_bit_lo;
    banks[way].hash_bits.ram_unit_select_bit_hi = hash_select_bit_hi;
    banks[way].hash_bits.ram_line_select_bit_lo = hash_entry_bit_lo;
    banks[way].hash_bits.ram_line_select_bit_hi = hash_entry_bit_hi;
    banks[way].hash_bits.num_ram_select_bits = number_select_bits;
    banks[way].hash_bits.num_ram_line_bits = number_entry_bits;
    banks[way].hash_bits.num_subword_bits = number_subword_bits;
    ++way;
  }

  rmt_table->bank_map = banks;
  return rc;

cleanup:
  PIPE_MGR_FREE(banks);
cleanup_no_free:
  return rc;
}

/*
 * Functions that parse the specific properties of the different types of
 * stage tables. Notice that some of them are no-ops for now, but they could
 * be further implemented in the future if necessary.
 */

/*
static pipe_status_t ctx_json_parse_rmt_cfg_phase0_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  CHECK_RC(rc, cleanup);

cleanup:
  return rc;
}
*/

/**
 * Parses fields specific to action data stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_action_data_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;

  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to hash match stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_hash_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table, int *schema_version) {
  int rc = PIPE_SUCCESS;

  rc |= ctx_json_parse_rmt_cfg_hash_match_memory_resource_allocation(
      stage_table_cjson, rmt_table, schema_version);
  CHECK_RC(rc, cleanup);

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_rmt_cfg_hash_action_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  CHECK_RC(rc, cleanup);

  (void)stage_table_cjson;
  (void)rmt_table;

cleanup:
  return rc;
}

/*
static pipe_status_t ctx_json_parse_rmt_cfg_direct_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  CHECK_RC(rc, cleanup);

cleanup:
  return rc;
}
*/

/**
 * Parses fields specific to match with no key stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_match_with_no_key_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);

  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to ATCAM match stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_atcam_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;

  rc |= ctx_json_parse_rmt_cfg_atcam_memory_resource_allocation(
      stage_table_cjson, rmt_table);
  CHECK_RC(rc, cleanup);

cleanup:
  return rc;
}

/**
 * Parses fields specific to proxy hash match stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_proxy_hash_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table, int *schema_version) {
  int rc = PIPE_SUCCESS;

  // Proxy hash match stage table is parsed exactly the same as a hash match,
  // as far as RMT cfg is concerned.
  rc |= ctx_json_parse_rmt_cfg_hash_match_stage_table_json(
      stage_table_cjson, rmt_table, schema_version);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/* This function finds the highest and lowest mem_ids used by the query
 * range field, and sets the mrd_terminate flag of all tcams within this
 * range except the final one to be true.
 *
 * Note that this method makes 2 assumptions:
 *     - Mem ids used by different range fields should never overlap.
 *     - Mem ids used by each range field should always stay in the same column.
 * Range match cannot work without the above constraints.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_find_mrd_terminate_mem_id(
    rmt_tbl_info_t *rmt_table, cJSON *fields_cjson, char *field_to_find) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_tbl_word_blk_t *tbl_word_block = NULL;
  int i = 0, j = 0;
  uint32_t max_mem_id = 0, min_mem_id = 0;
  bool mem_id_set = false;
  bool *used_mem_units = PIPE_MGR_CALLOC(
      rmt_table->pack_format.mem_units_per_tbl_word, sizeof(bool));
  uint32_t fold_row = TOF_TCAM_NUM_ROWS / 2;

  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
    char *source = NULL;
    char *field_name = NULL;
    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_FIELD_NAME, &field_name);
    CHECK_ERR(err, rc, cleanup);

    // If the field name does not match or field is not a range, skip it.
    if (strcmp(field_name, field_to_find) ||
        strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
      continue;
    }

    // Mark all mem indices that this range field is written into.
    int mem_idx = 0;
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_LSB_MEM_WORD_INDEX, &mem_idx);
    CHECK_ERR(err, rc, cleanup);
    used_mem_units[rmt_table->pack_format.mem_units_per_tbl_word - 1 -
                   mem_idx] = true;
  }

  for (i = 0; i < rmt_table->bank_map->num_tbl_word_blks; i++) {
    tbl_word_block = &(rmt_table->bank_map->tbl_word_blk[i]);
    // Find the highest and lowest mem ids that this range field uses.
    mem_id_set = false;
    for (j = 0; j < rmt_table->pack_format.mem_units_per_tbl_word; j++) {
      if (!used_mem_units[j]) {
        continue;
      }
      if (!mem_id_set) {
        max_mem_id = tbl_word_block->mem_id[j];
        min_mem_id = tbl_word_block->mem_id[j];
        mem_id_set = true;
        continue;
      }
      if (tbl_word_block->mem_id[j] > max_mem_id) {
        max_mem_id = tbl_word_block->mem_id[j];
      }
      if (tbl_word_block->mem_id[j] < min_mem_id) {
        min_mem_id = tbl_word_block->mem_id[j];
      }
    }

    if (max_mem_id % TOF_TCAM_NUM_ROWS >= fold_row &&
        min_mem_id % TOF_TCAM_NUM_ROWS <= fold_row) {
      /* If this tcam group includes row 6, set the mrd_terminate flags of all
       * tcams not in row 6 to true.
       */
      for (j = 0; j < rmt_table->pack_format.mem_units_per_tbl_word; j++) {
        if (tbl_word_block->mem_id[j] % TOF_TCAM_NUM_ROWS != fold_row &&
            tbl_word_block->mem_id[j] >= min_mem_id &&
            tbl_word_block->mem_id[j] <= max_mem_id) {
          tbl_word_block->mrd_terminate[j] = true;
        }
      }
    } else if (max_mem_id % TOF_TCAM_NUM_ROWS < fold_row) {
      /* If this tcam group lives under row 6, set the mrd_terminate flags of
       * all tcams except the top one to true.
       */
      for (j = 0; j < rmt_table->pack_format.mem_units_per_tbl_word; j++) {
        if (tbl_word_block->mem_id[j] < max_mem_id &&
            tbl_word_block->mem_id[j] >= min_mem_id) {
          tbl_word_block->mrd_terminate[j] = true;
        }
      }
    } else if (min_mem_id % TOF_TCAM_NUM_ROWS > fold_row) {
      /* If this tcam group lives above row 6, set the mrd_terminate flags of
       * all tcams except the bottom one to true.
       */
      for (j = 0; j < rmt_table->pack_format.mem_units_per_tbl_word; j++) {
        if (tbl_word_block->mem_id[j] > min_mem_id &&
            tbl_word_block->mem_id[j] <= max_mem_id) {
          tbl_word_block->mrd_terminate[j] = true;
        }
      }
    }
  }

  PIPE_MGR_FREE(used_mem_units);
  return rc;

cleanup:
  PIPE_MGR_FREE(used_mem_units);
  return rc;
}

/* Parses the tcam mem-ids that require MRD to terminate. This can happen
 * when a tcam is part of a group that matches the same range field. In this
 * case, all tcams in the group except the last should MRD terminate to ensure
 * these words are properly AND-ed together. The "last" tcam in the group is
 * the one closest to row 6, because the hardware folds tcam matches towards
 * that row. Note that this function is logically equivalent to a no-op if
 * the table does not use range fields.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_memory_units_for_range(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *pack_formats_cjson = NULL;
  err = bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  // There should only be a single pack format.
  cJSON *pack_format_cjson = NULL;
  err = bf_cjson_get_first(pack_formats_cjson, &pack_format_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *entries_cjson = NULL;
  err = bf_cjson_get_object(
      pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *entry_cjson = NULL;
  err = bf_cjson_get_first(entries_cjson, &entry_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *fields_cjson = NULL;
  err = bf_cjson_get_object(
      entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
    char *source = NULL;
    char *field_name = NULL;

    // If the field is not a range, skip it.
    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
    CHECK_ERR(err, rc, cleanup);
    if (strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
      continue;
    }

    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_FIELD_NAME, &field_name);
    CHECK_ERR(err, rc, cleanup);

    rc = ctx_json_parse_rmt_cfg_find_mrd_terminate_mem_id(
        rmt_table, fields_cjson, field_name);
    CHECK_RC(rc, cleanup);
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to ternary match stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_ternary_match_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);

  /* Parse mrd termination for each memory unit for range tables */
  rc |= ctx_json_parse_rmt_cfg_memory_units_for_range(stage_table_cjson,
                                                      rmt_table);
  CHECK_RC(rc, cleanup);

  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to ternary indirection stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t
ctx_json_parse_rmt_cfg_ternary_indirection_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses meter ALU indexes in stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param num_alu_indexes A pointer to the number of ALU indexes.
 * @param alu_indexes A pointer to the RMT meter ALU indexes for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_meter_alu_indexes_json(
    cJSON *stage_table_cjson, uint8_t *num_alu_indexes, uint8_t *alu_indexes) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  *num_alu_indexes = 0;
  // Parse the meter ALU index object.
  cJSON *meter_alu_indexes_cjson = NULL;

  err = bf_cjson_get_object(stage_table_cjson,
                            CTX_JSON_METER_STAGE_TABLE_ALU_INDEX,
                            &meter_alu_indexes_cjson);
  CHECK_ERR(err, rc, cleanup);

  if (meter_alu_indexes_cjson->type == cJSON_Array) {
    cJSON *meter_alu_index_cjson = NULL;
    CTX_JSON_FOR_EACH(meter_alu_index_cjson, meter_alu_indexes_cjson) {
      if (*num_alu_indexes >= RMT_MAX_S2P_SECTIONS) {
        LOG_ERROR(
            "%s:%d: Invalid ContextJSON format: property \"%s\" too many array "
            "items.",
            __FILE__,
            __LINE__,
            CTX_JSON_METER_STAGE_TABLE_ALU_INDEX);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      if (meter_alu_index_cjson->type != cJSON_Number) {
        LOG_ERROR(
            "%s:%d: Invalid ContextJSON format: property \"%s\" array item is "
            "not a number.",
            __FILE__,
            __LINE__,
            CTX_JSON_METER_STAGE_TABLE_ALU_INDEX);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      alu_indexes[(*num_alu_indexes)++] = meter_alu_index_cjson->valueint;
    }
  } else {
    int alu_index = 0;
    err = bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_METER_STAGE_TABLE_ALU_INDEX, &alu_index);
    CHECK_ERR(err, rc, cleanup);
    *num_alu_indexes = 1;
    alu_indexes[0] = alu_index;
  }

cleanup:
  return rc;
}

/**
 * Parses fields specific to meter stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_meter_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int default_lower_huffman_bits_included = 0;
  err |= bf_cjson_get_int(
      stage_table_cjson,
      CTX_JSON_METER_STAGE_TABLE_DEFAULT_LOWER_HUFFMAN_BITS_INCLUDED,
      &default_lower_huffman_bits_included);
  CHECK_ERR(err, rc, cleanup);
  rmt_table->params.meter_params.default_lower_huffman_bits =
      default_lower_huffman_bits_included;
  rc = ctx_json_parse_rmt_cfg_meter_alu_indexes_json(
      stage_table_cjson,
      &rmt_table->params.meter_params.num_alu_indexes,
      rmt_table->params.meter_params.alu_indexes);
  CHECK_RC(rc, cleanup);

  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to selection stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_selection_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int rc = PIPE_SUCCESS;
  rc = ctx_json_parse_rmt_cfg_meter_alu_indexes_json(
      stage_table_cjson,
      &rmt_table->params.sel_params.num_alu_indexes,
      rmt_table->params.sel_params.alu_indexes);
  CHECK_RC(rc, cleanup);
  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to statistics stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_statistics_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  pipe_status_t rc = PIPE_SUCCESS;
  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to stateful stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_stateful_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  pipe_status_t rc = PIPE_SUCCESS;

  rc = ctx_json_parse_rmt_cfg_meter_alu_indexes_json(
      stage_table_cjson,
      &rmt_table->params.stful_params.num_alu_indexes,
      rmt_table->params.stful_params.alu_indexes);
  CHECK_RC(rc, cleanup);

  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses fields specific to idletime stage tables.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_table A pointer to the internal RMT configuration for this stage
 * table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_idletime_stage_table_json(
    cJSON *stage_table_cjson, rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int precision = 0;
  bool disable_notification = false;
  bool two_way_notification = false;
  bool enable_pfe = false;

  err |= bf_cjson_get_int(
      stage_table_cjson, CTX_JSON_IDLETIME_STAGE_TABLE_PRECISION, &precision);
  err |= bf_cjson_get_bool(stage_table_cjson,
                           CTX_JSON_IDLETIME_STAGE_TABLE_DISABLE_NOTIFICATION,
                           &disable_notification);
  err |= bf_cjson_get_bool(stage_table_cjson,
                           CTX_JSON_IDLETIME_STAGE_TABLE_TWO_WAY_NOTIFICATION,
                           &two_way_notification);
  err |= bf_cjson_get_bool(
      stage_table_cjson, CTX_JSON_IDLETIME_STAGE_TABLE_ENABLE_PFE, &enable_pfe);
  CHECK_ERR(err, rc, cleanup);

  rmt_table->params.idle.bit_width = precision;
  rmt_table->params.idle.notify_disable = disable_notification;
  rmt_table->params.idle.two_way_notify_enable = two_way_notification;
  rmt_table->params.idle.per_flow_enable = enable_pfe;

  rc |= ctx_json_parse_rmt_cfg_memory_resource_allocation(stage_table_cjson,
                                                          rmt_table);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_stash_json(cJSON *stash_cjson,
                                               rmt_tbl_info_t *rmt_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  /* Stash pack format */
  cJSON *pack_format = NULL;
  err |= bf_cjson_get_object(
      stash_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_format);
  CHECK_ERR(err, rc, cleanup);

  pack_format = pack_format->child;
  PIPE_MGR_ASSERT(pack_format != NULL);

  int table_word_width = 0;
  int memory_word_width = 0;
  int entries_per_table_word = 0;
  int number_memory_units_per_table_word = 0;

  err |= bf_cjson_get_int(
      pack_format, CTX_JSON_PACK_FORMAT_TABLE_WORD_WIDTH, &table_word_width);
  err |= bf_cjson_get_int(
      pack_format, CTX_JSON_PACK_FORMAT_MEMORY_WORD_WIDTH, &memory_word_width);
  err |= bf_cjson_get_int(pack_format,
                          CTX_JSON_PACK_FORMAT_ENTRIES_PER_TABLE_WORD,
                          &entries_per_table_word);
  err |=
      bf_cjson_get_int(pack_format,
                       CTX_JSON_PACK_FORMAT_NUMBER_MEMORY_UNITS_PER_TABLE_WORD,
                       &number_memory_units_per_table_word);
  CHECK_ERR(err, rc, cleanup);

  rmt_table->stash = PIPE_MGR_CALLOC(1, sizeof(rmt_tbl_stash_t));
  if (!rmt_table->stash) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  rmt_table->stash->pack_format.tbl_word_width = table_word_width;
  rmt_table->stash->pack_format.mem_word_width = memory_word_width;
  rmt_table->stash->pack_format.entries_per_tbl_word = entries_per_table_word;
  rmt_table->stash->pack_format.mem_units_per_tbl_word =
      number_memory_units_per_table_word;
  rmt_table->stash->number_memory_units_per_table_word =
      number_memory_units_per_table_word;

  /* Stash entries */
  int num_entries = 0;
  err = bf_cjson_get_int(
      stash_cjson, CTX_JSON_STAGE_TABLE_STASH_NUM_ENTRY, &num_entries);
  CHECK_ERR(err, rc, cleanup);
  rmt_table->stash->num_stash_entries = num_entries;

  rmt_table->stash->stash_entries =
      PIPE_MGR_CALLOC(num_entries, sizeof(*rmt_table->stash->stash_entries));
  if (!rmt_table->stash->stash_entries) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  cJSON *entries_json = NULL;
  err |= bf_cjson_get_object(
      stash_cjson, CTX_JSON_STAGE_TABLE_STASH_ENTRIES, &entries_json);
  CHECK_ERR(err, rc, cleanup);

  uint32_t count = GETARRSZ(entries_json);
  uint32_t i = 0, j = 0;
  /* Go over all stash entries */
  for (i = 0; i < count; i++) {
    rmt_table->stash->stash_entries[i] =
        PIPE_MGR_CALLOC(number_memory_units_per_table_word,
                        sizeof(*rmt_table->stash->stash_entries[i]));
    /* Get one stash entry */
    cJSON *item = GETARRITEM(entries_json, i);
    /* Go over all stashes in the wide-word */
    for (j = 0; (j < (uint32_t)GETARRSZ(item)) &&
                (j < (uint32_t)number_memory_units_per_table_word);
         j++) {
      int stash_id = 0, stash_hashbank_select = 0;
      int hash_function_id = 0, stash_match_data_select = 0;

      cJSON *subitem = GETARRITEM(item, j);

      /* Get all the fields */
      err = bf_cjson_get_int(
          subitem, CTX_JSON_STAGE_TABLE_STASH_ENTRY_ID, &stash_id);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_int(subitem,
                             CTX_JSON_STAGE_TABLE_STASH_MATCH_DATA_SELECT,
                             &stash_match_data_select);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_int(subitem,
                             CTX_JSON_STAGE_TABLE_STASH_HASHBANK_SELECT,
                             &stash_hashbank_select);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_int(subitem,
                             CTX_JSON_STAGE_TABLE_STASH_HASH_FUNCTION_ID,
                             &hash_function_id);
      CHECK_ERR(err, rc, cleanup);

      rmt_table->stash->stash_entries[i][j].stash_id = stash_id;
      rmt_table->stash->stash_entries[i][j].stash_match_data_select =
          stash_match_data_select;
      rmt_table->stash->stash_entries[i][j].stash_hashbank_select =
          stash_hashbank_select;
      rmt_table->stash->stash_entries[i][j].hash_function_id = hash_function_id;
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parse fields common to all RMT (stage) tables, and invoke callbacks for
 * each specific stage table.
 *
 * @param stage_table_cjson The cJSON structure corresponding to this stage
 * table.
 * @param rmt_infos A pointer to an array of rmt_infos corresponding to the
 * table's RMT tables.
 * @param num_rmt_infos A pointer to an integer, which will be set to the
 * number of RMT tables.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_stage_table_json(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    cJSON *stage_table_cjson,
    rmt_tbl_info_t **rmt_infos,
    uint32_t *num_rmt_infos) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int stage_number = 0;
  int logical_table_id = 0;

  err |= bf_cjson_get_int(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage_number);
  err |= bf_cjson_try_get_int(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID,
                              &logical_table_id);





  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for stage table on stage %d and "
      "logical table id %d.",
      __func__,
      __LINE__,
      stage_number,
      logical_table_id);

  char *direction = NULL;
  err |= bf_cjson_try_get_string(
      stage_table_cjson, CTX_JSON_TABLE_DIRECTION, &direction);
  int direction_number = 0;
  if (direction) {
    if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_INGRESS)) {
      direction_number = BF_TBL_DIR_INGRESS;
    } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_EGRESS)) {
      direction_number = BF_TBL_DIR_EGRESS;
    } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_GHOST)) {
      direction_number = BF_TBL_DIR_GHOST;
    }
  }

  int size = 0;
  err |= bf_cjson_get_int(stage_table_cjson, CTX_JSON_STAGE_TABLE_SIZE, &size);
  CHECK_ERR(err, rc, cleanup);

  // Do not allocate an RMT structure for a table with size 0.
  if (size == 0) {
    LOG_DBG("%s:%d: Stage table has size 0. Not generating RMT config for it.",
            __func__,
            __LINE__);
    goto cleanup;
  }

  // Allocate a new rmt table.
  int tbl_idx = allocate_rmt_table(num_rmt_infos, rmt_infos);
  if (tbl_idx == -1) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  rmt_tbl_info_t *rmt_table = &(*rmt_infos)[tbl_idx];

  rmt_table->stage_id = stage_number;
  rmt_table->direction = direction_number;
  rmt_table->handle = logical_table_id;

  rmt_table->num_entries = size;

  // Take the first entry in the stage table's pack format, and consider that
  // for generating RMT information. For a stage table with ways, such as a
  // HashMatch stage table, then the pack format will be equal to all pack
  // formats in the ways. If we ever want to have different formats, this code
  // needs to take that into consideration.
  cJSON *pack_format = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_format);
  CHECK_ERR(err, rc, cleanup);

  pack_format = pack_format->child;
  PIPE_MGR_ASSERT(pack_format != NULL);

  char *stage_table_type = NULL;
  int table_word_width = 0;
  int memory_word_width = 0;
  int entries_per_table_word = 0;
  int number_memory_units_per_table_word = 0;

  err |= bf_cjson_get_string(stage_table_cjson,
                             CTX_JSON_STAGE_TABLE_STAGE_TABLE_TYPE,
                             &stage_table_type);
  err |= bf_cjson_get_int(
      pack_format, CTX_JSON_PACK_FORMAT_TABLE_WORD_WIDTH, &table_word_width);
  err |= bf_cjson_get_int(
      pack_format, CTX_JSON_PACK_FORMAT_MEMORY_WORD_WIDTH, &memory_word_width);
  err |= bf_cjson_get_int(pack_format,
                          CTX_JSON_PACK_FORMAT_ENTRIES_PER_TABLE_WORD,
                          &entries_per_table_word);
  err |=
      bf_cjson_get_int(pack_format,
                       CTX_JSON_PACK_FORMAT_NUMBER_MEMORY_UNITS_PER_TABLE_WORD,
                       &number_memory_units_per_table_word);
  CHECK_ERR(err, rc, cleanup);

  rmt_table->pack_format.tbl_word_width = table_word_width;
  rmt_table->pack_format.mem_word_width = memory_word_width;
  rmt_table->pack_format.entries_per_tbl_word = entries_per_table_word;
  rmt_table->pack_format.mem_units_per_tbl_word =
      number_memory_units_per_table_word;

  if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_TERNARY_MATCH)) {
    rmt_table->type = RMT_TBL_TYPE_TERN_MATCH;
    rc |= ctx_json_parse_rmt_cfg_ternary_match_stage_table_json(
        stage_table_cjson, rmt_table);

    // Parse ternary indirection stage table, if any.
    // The TIND does not have the logical table id published against it but it
    // must be the same as the TCAM table's.
    int log_tbl_id = rmt_table->handle;
    cJSON *ternary_indirection_stage_table_cjson = NULL;
    err |= bf_cjson_get_object(
        stage_table_cjson,
        CTX_JSON_STAGE_TABLE_TERNARY_INDIRECTION_STAGE_TABLE,
        &ternary_indirection_stage_table_cjson);
    CHECK_ERR(err, rc, cleanup);

    if (ternary_indirection_stage_table_cjson) {
      /* Parse the action fn handle used for match memory if tind has no mra */
      cJSON *mra_cjson = NULL;
      bf_cjson_try_get_object(ternary_indirection_stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                              &mra_cjson);
      if (mra_cjson == NULL) {
        cJSON *action_formats = NULL, *action_format = NULL;
        bf_cjson_try_get_object(ternary_indirection_stage_table_cjson,
                                CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                                &action_formats);
        if (action_formats) {
          CTX_JSON_FOR_EACH(action_format, action_formats) {
            int vliw = 0;
            err = bf_cjson_get_int(
                action_format, CTX_JSON_ACTION_FORMAT_VLIW_INSTRUCTION, &vliw);
            CHECK_ERR(err, rc, cleanup);
            /* Find the valid match vliw instruction */
            if (vliw != -1) {
              int act_fn_hdl = 0;
              err = bf_cjson_get_handle(dev_info->dev_id,
                                        prof_id,
                                        action_format,
                                        CTX_JSON_ACTION_FORMAT_ACTION_HANDLE,
                                        &act_fn_hdl);
              CHECK_ERR(err, rc, cleanup);
              rmt_table->tind_act_fn_hdl = (pipe_act_fn_hdl_t)act_fn_hdl;
              break;
            }
          }
        }
      }

      rc |= ctx_json_parse_rmt_cfg_stage_table_json(
          dev_info,
          prof_id,
          ternary_indirection_stage_table_cjson,
          rmt_infos,
          num_rmt_infos);
      CHECK_RC(rc, cleanup);
      (*rmt_infos)[*num_rmt_infos - 1].handle = log_tbl_id;
      rmt_table = &(*rmt_infos)[tbl_idx];
    }
  } else if (!strcmp(stage_table_type,
                     CTX_JSON_STAGE_TABLE_TYPE_PHASE0_MATCH)) {
    rmt_table->type = RMT_TBL_TYPE_PHASE0_MATCH;
  } else if (!strcmp(stage_table_type,
                     CTX_JSON_STAGE_TABLE_TYPE_DIRECT_MATCH)) {
    /*rmt_table->type = RMT_TBL_TYPE_DIRECT_MATCH;*/
    /*rc |=
     * ctx_json_parse_rmt_cfg_direct_match_stage_table_json(stage_table_cjson,*/
    /*rmt_table);*/
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_HASH_MATCH)) {
    rmt_table->type = RMT_TBL_TYPE_HASH_MATCH;
    rc |= ctx_json_parse_rmt_cfg_hash_match_stage_table_json(
        stage_table_cjson,
        rmt_table,
        dev_info->profile_info[prof_id]->schema_version);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_ATCAM_MATCH)) {
    rmt_table->type = RMT_TBL_TYPE_ATCAM_MATCH;
    rc |= ctx_json_parse_rmt_cfg_atcam_match_stage_table_json(stage_table_cjson,
                                                              rmt_table);
  } else if (!strcmp(stage_table_type,
                     CTX_JSON_STAGE_TABLE_TYPE_PROXY_HASH_MATCH)) {
    rmt_table->type = RMT_TBL_TYPE_PROXY_HASH;
    rc |= ctx_json_parse_rmt_cfg_proxy_hash_match_stage_table_json(
        stage_table_cjson,
        rmt_table,
        dev_info->profile_info[prof_id]->schema_version);
  } else if (!strcmp(stage_table_type,
                     CTX_JSON_STAGE_TABLE_TYPE_MATCH_WITH_NO_KEY)) {
    rmt_table->type = RMT_TBL_TYPE_NO_KEY;
    rc |= ctx_json_parse_rmt_cfg_match_with_no_key_stage_table_json(
        stage_table_cjson, rmt_table);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_HASH_ACTION)) {
    rmt_table->type = RMT_TBL_TYPE_HASH_ACTION;
    rc |= ctx_json_parse_rmt_cfg_hash_action_stage_table_json(stage_table_cjson,
                                                              rmt_table);
  } else if (!strcmp(stage_table_type,
                     CTX_JSON_STAGE_TABLE_TYPE_TERNARY_INDIRECTION)) {
    rmt_table->type = RMT_TBL_TYPE_TERN_INDIR;
    rc |= ctx_json_parse_rmt_cfg_ternary_indirection_stage_table_json(
        stage_table_cjson, rmt_table);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_ACTION_DATA)) {
    rmt_table->type = RMT_TBL_TYPE_ACTION_DATA;
    rc |= ctx_json_parse_rmt_cfg_action_data_stage_table_json(stage_table_cjson,
                                                              rmt_table);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_METER)) {
    rmt_table->type = RMT_TBL_TYPE_METER;
    rc |= ctx_json_parse_rmt_cfg_meter_stage_table_json(stage_table_cjson,
                                                        rmt_table);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_SELECTION)) {
    rmt_table->type = RMT_TBL_TYPE_SEL_PORT_VEC;
    rc |= ctx_json_parse_rmt_cfg_selection_stage_table_json(stage_table_cjson,
                                                            rmt_table);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_STATISTICS)) {
    rmt_table->type = RMT_TBL_TYPE_STATS;
    rc |= ctx_json_parse_rmt_cfg_statistics_stage_table_json(stage_table_cjson,
                                                             rmt_table);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_STATEFUL)) {
    // FIXME: Why is this type of stage table not one of the types on this enum?
    /*rmt_table->type = RMT_TBL_TYPE_STFUL;*/
    rc |= ctx_json_parse_rmt_cfg_stateful_stage_table_json(stage_table_cjson,
                                                           rmt_table);
    CHECK_RC(rc, cleanup);
  } else if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_IDLETIME)) {
    rmt_table->type = RMT_TBL_TYPE_IDLE_TMO;
    rc |= ctx_json_parse_rmt_cfg_idletime_stage_table_json(stage_table_cjson,
                                                           rmt_table);
    CHECK_RC(rc, cleanup);
  } else {
    LOG_ERROR("%s:%d: Invalid stage table type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              stage_table_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Parse the idletime stage table, if any.
  cJSON *idletime_stage_table_cjson = NULL;
  bf_cjson_try_get_object(stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_IDLETIME_STAGE_TABLE,
                          &idletime_stage_table_cjson);

  if (idletime_stage_table_cjson != NULL) {
    rc |= ctx_json_parse_rmt_cfg_stage_table_json(dev_info,
                                                  prof_id,
                                                  idletime_stage_table_cjson,
                                                  rmt_infos,
                                                  num_rmt_infos);
    CHECK_RC(rc, cleanup);
    rmt_table = &(*rmt_infos)[tbl_idx];
  }

  // Parse the stash info for tofino, if any.
  if (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) {
    cJSON *stash_cjson = NULL;
    bf_cjson_try_get_object(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STASH_ALLOCATION, &stash_cjson);
    if (stash_cjson != NULL) {
      rc |= ctx_json_parse_stash_json(stash_cjson, rmt_table);
      CHECK_RC(rc, cleanup);
    }
  }

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_fields(
    cJSON *cjson_fields, pipe_dhash_field_list_t *field_list) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_field_t *field = NULL;
  uint32_t i = 0;

  field_list->num_fields = GETARRSZ(cjson_fields);
  if (field_list->num_fields == 0) {
    return PIPE_SUCCESS;
  }
  field_list->fields =
      PIPE_MGR_CALLOC(field_list->num_fields, sizeof(pipe_dhash_field_t));
  if (field_list->fields == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  cJSON *cjson_field = NULL;
  CTX_JSON_FOR_EACH(cjson_field, cjson_fields) {
    field = &field_list->fields[i];
    char *name = NULL;
    int start_bit = 0;
    int bit_width = 0;
    bool optional = false;
    err |= bf_cjson_get_string_dup(
        cjson_field,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_FIELD_NAME,
        &name);
    err |= bf_cjson_get_int(
        cjson_field,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_FIELD_START_BIT,
        &start_bit);
    err |= bf_cjson_get_int(
        cjson_field,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_FIELD_BIT_WIDTH,
        &bit_width);
    err |= bf_cjson_get_bool(
        cjson_field,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_FIELD_OPTIONAL,
        &optional);
    CHECK_ERR(err, rc, cleanup);

    field->name = name;
    field->start_bit = start_bit;
    field->bit_width = bit_width;
    field->optional = optional;

    i++;
  }

  return rc;

cleanup:
  if (field_list->fields) {
    for (i = 0; i < field_list->num_fields; i++) {
      if (field_list->fields[i].name) {
        PIPE_MGR_FREE(field_list->fields[i].name);
      }
    }
    PIPE_MGR_FREE(field_list->fields);
    field_list->fields = NULL;
  }
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_crossbars(
    cJSON *cjson_crossbars,
    pipe_dhash_crossbar_t **crossbars,
    uint32_t *num_crossbars,
    ixbar_init_t *ixbar_init) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_crossbar_t *crossbar = NULL;
  ixbar_input_t *ixbar_input = NULL;
  uint32_t i = 0;

  *num_crossbars = GETARRSZ(cjson_crossbars);
  if (*num_crossbars == 0) {
    return PIPE_SUCCESS;
  }
  *crossbars = PIPE_MGR_CALLOC(*num_crossbars, sizeof(pipe_dhash_crossbar_t));
  if (*crossbars == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  ixbar_init->ixbar_inputs =
      PIPE_MGR_CALLOC(*num_crossbars, sizeof(ixbar_input_t));
  if (ixbar_init->ixbar_inputs == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  ixbar_init->inputs_sz = *num_crossbars;

  cJSON *cjson_crossbar = NULL;
  CTX_JSON_FOR_EACH(cjson_crossbar, cjson_crossbars) {
    crossbar = &((*crossbars)[i]);
    char *name = NULL;
    int byte_number = 0;
    int bit_in_byte = 0;
    int field_bit = false;
    err |= bf_cjson_get_string_dup(
        cjson_crossbar,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_NAME,
        &name);
    err |= bf_cjson_get_int(
        cjson_crossbar,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_BYTE,
        &byte_number);
    err |= bf_cjson_get_int(
        cjson_crossbar,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_BIT,
        &bit_in_byte);
    err |= bf_cjson_get_int(
        cjson_crossbar,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_FIELD_BIT,
        &field_bit);
    CHECK_ERR(err, rc, cleanup);

    /* Update crossbar */
    crossbar->name = name;
    crossbar->byte_number = byte_number;
    crossbar->bit_in_byte = bit_in_byte;
    crossbar->field_bit = field_bit;

    /* Update ixbar_init */
    ixbar_input = &(ixbar_init->ixbar_inputs[i]);
    ixbar_input->type = tPHV;
    ixbar_input->ixbar_bit_position = byte_number * 8 + bit_in_byte;
    ixbar_input->bit_size = 1;
    ixbar_input->u.valid = true;

    i++;
  }

  return rc;

cleanup:
  if (*crossbars) {
    for (i = 0; i < *num_crossbars; i++) {
      if ((*crossbars)[i].name) {
        PIPE_MGR_FREE((*crossbars)[i].name);
      }
    }
    PIPE_MGR_FREE(*crossbars);
    *crossbars = NULL;
  }
  if (ixbar_init->ixbar_inputs) {
    PIPE_MGR_FREE(ixbar_init->ixbar_inputs);
    ixbar_init->ixbar_inputs = NULL;
  }
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_crossbar_configs(
    cJSON *cjson_crossbar_configs, pipe_dhash_field_list_t *field_list) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_crossbar_config_t *crossbar_config = NULL;
  uint32_t i = 0;

  field_list->num_crossbar_configs = GETARRSZ(cjson_crossbar_configs);
  if (field_list->num_crossbar_configs == 0) {
    return PIPE_SUCCESS;
  }
  field_list->crossbar_configs = PIPE_MGR_CALLOC(
      field_list->num_crossbar_configs, sizeof(pipe_dhash_crossbar_config_t));
  if (field_list->crossbar_configs == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  cJSON *cjson_crossbar_config = NULL;
  CTX_JSON_FOR_EACH(cjson_crossbar_config, cjson_crossbar_configs) {
    crossbar_config = &field_list->crossbar_configs[i];
    int stage_id = 0;
    err |= bf_cjson_get_int(
        cjson_crossbar_config,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_STAGE_ID,
        &stage_id);
    CHECK_ERR(err, rc, cleanup);
    crossbar_config->stage_id = stage_id;

    /* Parse crossbar bits */
    cJSON *cjson_crossbars = NULL;
    err |= bf_cjson_get_object(
        cjson_crossbar_config,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR,
        &cjson_crossbars);
    CHECK_ERR(err, rc, cleanup);
    rc =
        ctx_json_parse_dynamic_hash_crossbars(cjson_crossbars,
                                              &(crossbar_config->crossbars),
                                              &(crossbar_config->num_crossbars),
                                              &(crossbar_config->ixbar_init));
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    /* Parse crossbar mod bits */
    err |= bf_cjson_get_object(
        cjson_crossbar_config,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_MOD,
        &cjson_crossbars);
    CHECK_ERR(err, rc, cleanup);
    rc = ctx_json_parse_dynamic_hash_crossbars(
        cjson_crossbars,
        &(crossbar_config->crossbar_mods),
        &(crossbar_config->num_crossbar_mods),
        &(crossbar_config->ixbar_mod_init));
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    i++;
  }

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_field_lists(
    cJSON *cjson_field_lists, pipe_dhash_info_t *dhash_node) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  char *name = NULL;
  int handle = 0;
  pipe_dhash_field_list_t *field_list = NULL;
  uint32_t i = 0;

  dhash_node->num_field_lists = GETARRSZ(cjson_field_lists);
  if (dhash_node->num_field_lists == 0) {
    return PIPE_SUCCESS;
  }
  dhash_node->field_lists = PIPE_MGR_CALLOC(dhash_node->num_field_lists,
                                            sizeof(pipe_dhash_field_list_t));
  if (dhash_node->field_lists == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  i = 0;
  cJSON *cjson_field_list = NULL;
  CTX_JSON_FOR_EACH(cjson_field_list, cjson_field_lists) {
    field_list = &dhash_node->field_lists[i];
    err |= bf_cjson_get_string_dup(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_NAME,
        &name);
    CHECK_ERR(err, rc, cleanup);
    err |=
        bf_cjson_get_int(cjson_field_list,
                         CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_HANDLE,
                         &handle);
    CHECK_ERR(err, rc, cleanup);
    field_list->name = name;
    field_list->handle = handle;

    bool is_default = false;
    bool can_permute = false;
    bool can_rotate = false;
    err |= bf_cjson_get_bool(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_IS_DEFAULT,
        &is_default);
    err |= bf_cjson_try_get_bool(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CAN_PERMUTE,
        &can_permute);
    err |= bf_cjson_try_get_bool(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CAN_ROTATE,
        &can_rotate);
    CHECK_ERR(err, rc, cleanup);
    if (is_default) {
      dhash_node->def_field_list_hdl = handle;
      dhash_node->curr_field_list_hdl = handle;
    }
    field_list->can_permute = can_permute;
    field_list->can_rotate = can_rotate;

    /* Parse fields */
    cJSON *cjson_fields = NULL;
    err |= bf_cjson_get_object(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_FIELDS,
        &cjson_fields);
    CHECK_ERR(err, rc, cleanup);
    rc = ctx_json_parse_dynamic_hash_fields(cjson_fields, field_list);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    /* Parse crossbar configuration */
    cJSON *cjson_crossbar_configs = NULL;
    err |= bf_cjson_get_object(
        cjson_field_list,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LIST_CROSSBAR_CONFIG,
        &cjson_crossbar_configs);
    CHECK_ERR(err, rc, cleanup);
    rc = ctx_json_parse_dynamic_hash_crossbar_configs(cjson_crossbar_configs,
                                                      field_list);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    i++;
  }

  return rc;

cleanup:
  return rc;
}

static void initialize_nonp4_dynamic_algorithm(pipe_dhash_info_t *dhash_node) {
  uint32_t non_p4_algo_index = dhash_node->num_algorithms - 1;
  pipe_dhash_alg_t *alg = &dhash_node->algorithms[non_p4_algo_index];

  alg->name = NULL;
  alg->handle = -1;
  alg->hash_alg.crc_matrix = NULL;
  return;
}

static pipe_status_t ctx_json_parse_dynamic_hash_algorithms(
    cJSON *cjson_algorithms, pipe_dhash_info_t *dhash_node) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_alg_t *alg = NULL;
  uint32_t i = 0, j = 0;

  /* In dhash_node->algorithms, we will keep one extra algorithm
   * for dynamic changing to non-p4 algo
   */
  dhash_node->num_algorithms = GETARRSZ(cjson_algorithms) + 1;
  dhash_node->algorithms =
      PIPE_MGR_CALLOC(dhash_node->num_algorithms, sizeof(pipe_dhash_alg_t));
  if (dhash_node->algorithms == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  if (dhash_node->num_algorithms == 1) {
    /* If no P4 defined algos are present, still initialize
     * one space for non-p4 and return success
     */
    initialize_nonp4_dynamic_algorithm(dhash_node);
    return PIPE_SUCCESS;
  }

  cJSON *cjson_algorithm = NULL;
  CTX_JSON_FOR_EACH(cjson_algorithm, cjson_algorithms) {
    alg = &dhash_node->algorithms[i];
    char *name = NULL;
    int handle = 0;
    bool is_default = false;
    char *alg_type = NULL;
    bool msb = false;
    bool extend = false;
    char *polynomial = NULL;
    char *init = NULL;
    bool reverse = false;
    char *final_xor = NULL;
    err |= bf_cjson_get_string_dup(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_NAME,
        &name);
    err |= bf_cjson_get_int(cjson_algorithm,
                            CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_HANDLE,
                            &handle);
    err |= bf_cjson_get_bool(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_IS_DEFAULT,
        &is_default);
    err |=
        bf_cjson_get_string(cjson_algorithm,
                            CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_TYPE,
                            &alg_type);
    err |= bf_cjson_get_bool(cjson_algorithm,
                             CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_MSB,
                             &msb);
    err |=
        bf_cjson_get_bool(cjson_algorithm,
                          CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_EXTEND,
                          &extend);
    err |= bf_cjson_try_get_string(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_POLYNOMIAL,
        &polynomial);
    err |= bf_cjson_try_get_string(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_INIT,
        &init);
    err |= bf_cjson_try_get_bool(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_REVERSE,
        &reverse);
    err |= bf_cjson_try_get_string(
        cjson_algorithm,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHM_FINAL_XOR,
        &final_xor);
    CHECK_ERR(err, rc, cleanup);

    alg->name = name;
    alg->handle = handle;
    alg->hash_alg.hash_alg = hash_alg_str_to_type(alg_type);
    alg->hash_alg.msb = msb;
    alg->hash_alg.extend = extend;
    alg->hash_alg.hash_bit_width = dhash_node->hash_bit_width;
    alg->hash_alg.reverse = reverse;
    if (polynomial) {
      /* crc polynomials have a leading 1, so shift the leading "0x"
       * right one character.
       */
      char *adjusted_poly = polynomial + 1;
      adjusted_poly[0] = '0';
      adjusted_poly[1] = 'x';
      alg->hash_alg.hash_bit_width = (strlen(adjusted_poly) - 2) * 4;
      alg->hash_alg.poly = strtoull(adjusted_poly, NULL, 0);
      alg->hash_alg.crc_type = crc_alg_str_to_type(name);
      if (init) {
        alg->hash_alg.init = strtoull(init, NULL, 0);
      }
      if (final_xor) {
        alg->hash_alg.final_xor = strtoull(final_xor, NULL, 0);
      }
      alg->hash_alg.crc_matrix = PIPE_MGR_CALLOC(256, sizeof(uint8_t *));
      for (j = 0; j < 256; j++) {
        alg->hash_alg.crc_matrix[j] = PIPE_MGR_CALLOC(
            (alg->hash_alg.hash_bit_width + 7) / 8, sizeof(uint8_t));
      }
      initialize_crc_matrix(&alg->hash_alg);
    }

    if (is_default) {
      dhash_node->def_algo_hdl = handle;
      dhash_node->curr_algo_hdl = handle;
    }

    i++;
  }
  /* Initialize the extra array member for non-p4
   * dynamic algo change
   */
  initialize_nonp4_dynamic_algorithm(dhash_node);

  return rc;

cleanup:
  if (dhash_node->algorithms) {
    for (i = 0; i < dhash_node->num_algorithms; i++) {
      if (dhash_node->algorithms[i].name) {
        PIPE_MGR_FREE(dhash_node->algorithms[i].name);
      }
    }
    PIPE_MGR_FREE(dhash_node->algorithms);
    dhash_node->algorithms = NULL;
  }
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_config_hash_node(
    cJSON *cjson_hash, pipe_dhash_hash_t *hash) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  hash_matrix_output_t *hash_bit = NULL;
  uint32_t i = 0;

  int hash_id = 0;
  err |= bf_cjson_get_int(
      cjson_hash, CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_ID, &hash_id);
  CHECK_ERR(err, rc, cleanup);
  hash->hash_id = hash_id;

  cJSON *cjson_hash_bits = NULL;
  err |= bf_cjson_get_object(cjson_hash,
                             CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_BITS,
                             &cjson_hash_bits);
  CHECK_ERR(err, rc, cleanup);
  hash->num_hash_bits = GETARRSZ(cjson_hash_bits);
  if (hash->num_hash_bits == 0) {
    return PIPE_SUCCESS;
  }
  hash->hash_bits =
      PIPE_MGR_CALLOC(hash->num_hash_bits, sizeof(hash_matrix_output_t));
  if (hash->hash_bits == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  cJSON *cjson_hash_bit = NULL;
  CTX_JSON_FOR_EACH(cjson_hash_bit, cjson_hash_bits) {
    hash_bit = &hash->hash_bits[i];
    int gfm_hash_bit = 0;
    int p4_hash_bit = 0;
    err |= bf_cjson_get_int(cjson_hash_bit,
                            CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_BIT_GFM,
                            &gfm_hash_bit);
    err |= bf_cjson_get_int(cjson_hash_bit,
                            CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_BIT_P4,
                            &p4_hash_bit);
    CHECK_ERR(err, rc, cleanup);

    hash_bit->p4_hash_output_bit = p4_hash_bit;
    hash_bit->gfm_start_bit = gfm_hash_bit;
    hash_bit->bit_size = 1;

    i++;
  }

  return rc;

cleanup:
  if (hash->hash_bits) {
    PIPE_MGR_FREE(hash->hash_bits);
    hash->hash_bits = NULL;
  }
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_config(
    cJSON *cjson_hash_configs, pipe_dhash_info_t *dhash_node) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_hash_config_t *hash_config = NULL;
  uint32_t i = 0;

  dhash_node->num_hash_configs = GETARRSZ(cjson_hash_configs);
  if (dhash_node->num_hash_configs == 0) {
    return PIPE_SUCCESS;
  }
  dhash_node->hash_configs = PIPE_MGR_CALLOC(dhash_node->num_hash_configs,
                                             sizeof(pipe_dhash_hash_config_t));
  if (dhash_node->hash_configs == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for dynamic hash calculations "
        "info.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  /* Parse hash */
  cJSON *cjson_hash_config = NULL;
  cJSON *cjson_hash = NULL;
  CTX_JSON_FOR_EACH(cjson_hash_config, cjson_hash_configs) {
    hash_config = &dhash_node->hash_configs[i];
    int stage_id = 0;
    err |= bf_cjson_get_int(
        cjson_hash_config,
        CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_CONFIG_STAGE_ID,
        &stage_id);
    CHECK_ERR(err, rc, cleanup);
    hash_config->stage_id = stage_id;

    /* Parse hash bits */
    err |= bf_cjson_get_object(cjson_hash_config,
                               CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH,
                               &cjson_hash);
    CHECK_ERR(err, rc, cleanup);
    rc = ctx_json_parse_dynamic_hash_config_hash_node(cjson_hash,
                                                      &hash_config->hash);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    /* Parse hash mod bits */
    err |= bf_cjson_get_object(cjson_hash_config,
                               CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_MOD,
                               &cjson_hash);
    CHECK_ERR(err, rc, cleanup);
    rc = ctx_json_parse_dynamic_hash_config_hash_node(cjson_hash,
                                                      &hash_config->hash_mod);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }

    i++;
  }

  return rc;

cleanup:
  if (dhash_node->hash_configs) {
    for (i = 0; i < dhash_node->num_hash_configs; i++) {
      if (dhash_node->hash_configs[i].hash.hash_bits) {
        PIPE_MGR_FREE(dhash_node->hash_configs[i].hash.hash_bits);
      }
      if (dhash_node->hash_configs[i].hash_mod.hash_bits) {
        PIPE_MGR_FREE(dhash_node->hash_configs[i].hash_mod.hash_bits);
      }
    }
    PIPE_MGR_FREE(dhash_node->hash_configs);
    dhash_node->hash_configs = NULL;
  }
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_node(
    cJSON *dyn_hash_calculations_item, pipe_dhash_info_t *dhash_node) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  /* Parse field lists */
  cJSON *cjson_field_lists = NULL;
  err |= bf_cjson_get_object(dyn_hash_calculations_item,
                             CTX_JSON_DYNAMIC_HASH_CALCULATIONS_FIELD_LISTS,
                             &cjson_field_lists);
  CHECK_ERR(err, rc, cleanup);
  rc = ctx_json_parse_dynamic_hash_field_lists(cjson_field_lists, dhash_node);
  if (rc != PIPE_SUCCESS) {
    goto cleanup;
  }

  /* Parse hash algorithms */
  bool any_hash_alg_allowed = false;
  int hash_bit_width = 0;
  err |= bf_cjson_try_get_bool(
      dyn_hash_calculations_item,
      CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ANY_HASH_ALGORITHM_ALLOWED,
      &any_hash_alg_allowed);
  err |= bf_cjson_get_int(dyn_hash_calculations_item,
                          CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_BIT_WIDTH,
                          &hash_bit_width);
  CHECK_ERR(err, rc, cleanup);
  dhash_node->any_hash_algorithm_allowed = any_hash_alg_allowed;
  dhash_node->hash_bit_width = hash_bit_width;

  cJSON *cjson_algorithms = NULL;
  err |= bf_cjson_get_object(dyn_hash_calculations_item,
                             CTX_JSON_DYNAMIC_HASH_CALCULATIONS_ALGORITHMS,
                             &cjson_algorithms);
  CHECK_ERR(err, rc, cleanup);
  rc = ctx_json_parse_dynamic_hash_algorithms(cjson_algorithms, dhash_node);
  if (rc != PIPE_SUCCESS) {
    goto cleanup;
  }

  /* Parse hash configuration */
  cJSON *cjson_hash_configs = NULL;
  err |= bf_cjson_get_object(dyn_hash_calculations_item,
                             CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HASH_CONFIG,
                             &cjson_hash_configs);
  CHECK_ERR(err, rc, cleanup);
  rc = ctx_json_parse_dynamic_hash_config(cjson_hash_configs, dhash_node);
  if (rc != PIPE_SUCCESS) {
    goto cleanup;
  }

  /* Link ixbar_init_t structs to hash_matrix_output */
  uint32_t i, j;
  pipe_dhash_crossbar_config_t *crossbar_config = NULL;
  for (i = 0; i < dhash_node->num_field_lists; i++) {
    if (dhash_node->field_lists[i].num_crossbar_configs !=
        dhash_node->num_hash_configs) {
      LOG_ERROR(
          "%s:%d Inconsistent number of stages for crossbar and hash nodes "
          "in hash calc object 0x%x",
          __func__,
          __LINE__,
          dhash_node->handle);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
    for (j = 0; j < dhash_node->num_hash_configs; j++) {
      crossbar_config = &(dhash_node->field_lists[i].crossbar_configs[j]);
      if (crossbar_config->stage_id != dhash_node->hash_configs[j].stage_id) {
        LOG_ERROR(
            "%s:%d Inconsistent crossbar stage id %d with hash config stage id "
            "%d in hash calc object 0x%x",
            __func__,
            __LINE__,
            crossbar_config->stage_id,
            dhash_node->hash_configs[j].stage_id,
            dhash_node->handle);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      crossbar_config->ixbar_init.hash_matrix_outputs =
          dhash_node->hash_configs[j].hash.hash_bits;
      crossbar_config->ixbar_init.outputs_sz =
          dhash_node->hash_configs[j].hash.num_hash_bits;
      crossbar_config->ixbar_init.parity_group =
          dhash_node->hash_configs[j].hash.hash_id;

      crossbar_config->ixbar_mod_init.hash_matrix_outputs =
          dhash_node->hash_configs[j].hash_mod.hash_bits;
      crossbar_config->ixbar_mod_init.outputs_sz =
          dhash_node->hash_configs[j].hash_mod.num_hash_bits;
      crossbar_config->ixbar_mod_init.parity_group =
          dhash_node->hash_configs[j].hash_mod.hash_id;
    }
  }

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_dynamic_hash_calc(bf_dev_id_t devid,
                                                      profile_id_t prof_id,
                                                      bf_map_t *dhash_info,
                                                      cJSON *root) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  pipe_dhash_info_t *dhash_node = NULL;
  char *name = NULL;
  int handle = 0;
  unsigned long key = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  bf_map_init(dhash_info);

  cJSON *dyn_hash_calculations_cjson = NULL;
  bf_cjson_try_get_object(
      root, CTX_JSON_DYNAMIC_HASH_CALCULATIONS, &dyn_hash_calculations_cjson);

  if (dyn_hash_calculations_cjson == NULL) {
    return PIPE_SUCCESS;
  }
  int num_hash_calc = cJSON_GetArraySize(dyn_hash_calculations_cjson);
  if (num_hash_calc == 0) {
    return PIPE_SUCCESS;
  }

  cJSON *dyn_hash_calculations_item = NULL;
  CTX_JSON_FOR_EACH(dyn_hash_calculations_item, dyn_hash_calculations_cjson) {
    /* Get name and handle */
    err |= bf_cjson_get_string_dup(dyn_hash_calculations_item,
                                   CTX_JSON_DYNAMIC_HASH_CALCULATIONS_NAME,
                                   &name);
    CHECK_ERR(err, rc, cleanup);

    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               dyn_hash_calculations_item,
                               CTX_JSON_DYNAMIC_HASH_CALCULATIONS_HANDLE,
                               &handle);
    CHECK_ERR(err, rc, cleanup);

    dhash_node = PIPE_MGR_CALLOC(1, sizeof(pipe_dhash_info_t));
    if (dhash_node == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for dynamic hash calculations "
          "info.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    dhash_node->name = name;
    dhash_node->handle = handle;
    key = dhash_node->handle;
    bf_map_add(dhash_info, key, (void *)dhash_node);

    rc = ctx_json_parse_dynamic_hash_node(dyn_hash_calculations_item,
                                          dhash_node);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }
  }

  return rc;

cleanup:
  pipe_mgr_rmt_dynamic_selector_free(dhash_info);
  return rc;
}

static pipe_status_t ctx_json_parse_driver_options(
    bf_dev_id_t devid, pipe_driver_options_t *options, cJSON *root) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  cJSON *driver_options_cjson = NULL;
  bf_cjson_try_get_object(root, CTX_JSON_DRIVER_OPTIONS, &driver_options_cjson);

  if (driver_options_cjson == NULL) {
    return PIPE_SUCCESS;
  }

  bool hash_parity = false;
  err |= bf_cjson_get_bool(
      driver_options_cjson, CTX_JSON_DRIVER_OPTIONS_HASH_PARITY, &hash_parity);
  CHECK_ERR(err, rc, cleanup);
  options->hash_parity_enabled = hash_parity;

  cJSON *prsr_multi_threading_val = cJSON_GetObjectItem(
      driver_options_cjson, CTX_JSON_DRIVER_OPTIONS_PRSR_MULTI_THREADING);

  if (prsr_multi_threading_val) {
    bool prsr_multi_threading_disable = false;

    err = bf_cjson_get_bool(driver_options_cjson,
                            CTX_JSON_DRIVER_OPTIONS_PRSR_MULTI_THREADING,
                            &prsr_multi_threading_disable);
    CHECK_ERR(err, rc, cleanup);

    options->prsr_multi_threading_enable =
        (prsr_multi_threading_disable ? PRSR_MULTI_THREADING_MODE_DISABLE
                                      : PRSR_MULTI_THREADING_MODE_ENABLE);
  } else {
    options->prsr_multi_threading_enable = PRSR_MULTI_THREADING_MODE_DEFAULT;
  }

  return rc;

cleanup:
  return rc;
}

static int parse_match_key_field_value(cJSON *match_key_field_val,
                                       char *name,
                                       int match_type,
                                       int bit_width_full,
                                       uint8_t **k,
                                       uint8_t **m) {
  int err = 0;
  char *field_name;
  err = bf_cjson_get_string(match_key_field_val,
                            CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_NAME,
                            &field_name);
  if (err) return err;
  if (strcmp(name, field_name)) {
    LOG_ERROR("%s:%d Error, expecting field %s, found field %s",
              __func__,
              __LINE__,
              name,
              field_name);
    return -1;
  }

  uint8_t byte_width = (bit_width_full + 7) / 8;
  uint8_t *value = PIPE_MGR_CALLOC(byte_width, sizeof(uint8_t));
  if (!value) {
    LOG_ERROR("%s:%d: Memory allocation failure for %d bytes",
              __func__,
              __LINE__,
              byte_width);
    return -1;
  }
  uint8_t *mask = PIPE_MGR_CALLOC(byte_width, sizeof(uint8_t));
  if (!mask) {
    PIPE_MGR_FREE(value);
    LOG_ERROR("%s:%d: Memory allocation failure for %d bytes",
              __func__,
              __LINE__,
              byte_width);
    return -1;
  }

  if (match_type == CTX_JSON_MATCH_TYPE_RANGE) {
    /* Range values are generally start and end but if start equals end it may
     * come as a single value instead.  So first get "value" as both start and
     * end then get the range-start and range-end so we are covered no matter
     * what is published. */
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_VALUE,
                         value,
                         byte_width);
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_VALUE,
                         mask,
                         byte_width);
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_RANGE_START,
                         value,
                         byte_width);
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_RANGE_END,
                         mask,
                         byte_width);
  } else {
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_VALUE,
                         value,
                         byte_width);
    bf_cjson_try_get_hex(match_key_field_val,
                         CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELD_MASK,
                         mask,
                         byte_width);

    if (match_type == CTX_JSON_MATCH_TYPE_EXACT) {
      /* Exact match entries don't have a mask implicitly set it to full. */
      PIPE_MGR_MEMSET(mask, 0xFF, byte_width);
    }
  }
  *k = value;
  *m = mask;

  return err;
}

/**
 * Parse the static entries for a given match table.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @param static_entries_cjson The cJSON structure containing the static entries
 * values for this table.
 * @param match_table The internal match table structure to be populated.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_static_entries(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    cJSON *table_cjson,
    cJSON *static_entries_cjson,
    pipe_mat_tbl_info_t *match_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  bf_dev_id_t devid = dev_info->dev_id;

  match_table->num_static_entries = cJSON_GetArraySize(static_entries_cjson);
  if (match_table->num_static_entries == 0) {
    match_table->static_entries = NULL;
    return PIPE_SUCCESS;
  }

  cJSON *match_key_fields = NULL;
  cJSON *actions_cjson = NULL;
  err = bf_cjson_get_object(
      table_cjson, CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS, &match_key_fields);
  if (err) return err;
  err =
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  if (err) return err;

  /* Parse the match_key_fields array and collect some information for later
   * use.  We'll keep the size of each field, the position of each field, and
   * the offset of each field into the key/mask byte arrays.
   * It is possible that multiple match_key_fields entries refer to the same
   * field in the key/mask byte array, the position field indicates which field
   * in the key/mask byte array the match_key_fields entry corresponds to. */
  int num_match_key_fields = cJSON_GetArraySize(match_key_fields);
  int mkf_to_pos[num_match_key_fields];
  int mkf_to_bit_sz[num_match_key_fields];
  int mkf_to_offset[num_match_key_fields];
  PIPE_MGR_MEMSET(mkf_to_offset, 0, sizeof(mkf_to_offset));
  int max_pos = -1;
  int key_msk_byte_sz = 0;
  int key_msk_bit_sz = 0;
  for (int i = 0; i < num_match_key_fields; ++i) {
    cJSON *mkf = NULL;
    err = bf_cjson_get_array_item(match_key_fields, i, &mkf);
    CHECK_ERR(err, rc, cleanup);

    /* Record the position of this match_key_field in the key/mask array. */
    int field_pos = 0;
    err = bf_cjson_get_int(mkf, CTX_JSON_MATCH_KEY_FIELDS_POSITION, &field_pos);
    CHECK_ERR(err, rc, cleanup);
    mkf_to_pos[i] = field_pos;
    /* Keep track of how many fields are in the key/mask byte array, it will be
     * equal to the maximum position found plus one. */
    if (max_pos < field_pos) max_pos = field_pos;

    /* Record the size of the field as well. */
    int field_sz = 0;
    err = bf_cjson_get_int(
        mkf, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL, &field_sz);
    CHECK_ERR(err, rc, cleanup);
    mkf_to_bit_sz[i] = field_sz;
  }
  /* Find the size of the field at each position, sum them all up to get the
   * total size of the key/mask byte array. */
  for (int i = 0; i <= max_pos; ++i) {
    for (int j = 0; j < num_match_key_fields; ++j) {
      if (mkf_to_pos[j] == i) {
        key_msk_byte_sz += (mkf_to_bit_sz[j] + 7) / 8;
        key_msk_bit_sz += mkf_to_bit_sz[j];
        break;
      }
    }
  }
  /* Find the offset of each match_key_field in the key/mask byte array. */
  for (int i = 0, off = 0; i <= max_pos; ++i) {
    int sz_of_field_at_pos_i = 0;
    for (int j = 0; j < num_match_key_fields; ++j) {
      if (mkf_to_pos[j] == i) {
        sz_of_field_at_pos_i = (mkf_to_bit_sz[i] + 7) / 8;
        mkf_to_offset[j] = off;
      }
    }
    off += sz_of_field_at_pos_i;
  }

  match_table->static_entries = PIPE_MGR_CALLOC(
      match_table->num_static_entries, sizeof(pipe_mgr_static_entry_info_t));
  if (match_table->static_entries == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for match table's static entries.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  int static_entry_index = 0;
  cJSON *static_entry_cjson = NULL;
  CTX_JSON_FOR_EACH(static_entry_cjson, static_entries_cjson) {
    pipe_mgr_static_entry_info_t *entry =
        &match_table->static_entries[static_entry_index];

    int action_handle = 0;
    bool is_default_entry = false;
    int priority = 0;
    err = bf_cjson_get_bool(static_entry_cjson,
                            CTX_JSON_STATIC_ENTRY_IS_DEFAULT_ENTRY,
                            &is_default_entry);
    err |= bf_cjson_get_int(
        static_entry_cjson, CTX_JSON_STATIC_ENTRY_PRIORITY, &priority);
    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               static_entry_cjson,
                               CTX_JSON_STATIC_ENTRY_ACTION_HANDLE,
                               &action_handle);
    CHECK_ERR(err, rc, cleanup);

    // Get the action cJSON structure for this action handle.
    cJSON *action_cjson = NULL;
    err = ctx_json_parse_action_for_action_handle(
        devid, prof_id, actions_cjson, action_handle, &action_cjson);
    CHECK_ERR(err, rc, cleanup);

    char *action_name = NULL;
    err = bf_cjson_get_string_dup(
        action_cjson, CTX_JSON_ACTION_NAME, &action_name);
    CHECK_ERR(err, rc, cleanup);

    // Fill in the fields parsed.
    entry->default_entry = is_default_entry;
    entry->priority = priority;
    entry->action_entry.act_fn_hdl = action_handle;
    entry->action_entry.name = action_name;

    /* Get the match key values json object for this static entry. */
    cJSON *match_key_fields_values = NULL;
    err = bf_cjson_get_object(static_entry_cjson,
                              CTX_JSON_STATIC_ENTRY_MATCH_KEY_FIELDS_VALUES,
                              &match_key_fields_values);
    CHECK_ERR(err, rc, cleanup);

    int number_match_key_fields_values =
        cJSON_GetArraySize(match_key_fields_values);

    if (number_match_key_fields_values > 0) {
      PIPE_MGR_ASSERT(!is_default_entry);
      PIPE_MGR_ASSERT(number_match_key_fields_values == num_match_key_fields);

      entry->len_bytes = key_msk_byte_sz;
      entry->len_bits = key_msk_bit_sz;
      entry->key = PIPE_MGR_CALLOC(entry->len_bytes, sizeof(uint8_t));
      entry->msk = PIPE_MGR_CALLOC(entry->len_bytes, sizeof(uint8_t));
      if (!entry->key || !entry->msk) {
        LOG_ERROR(
            "%s:%d: Could not allocate memory for match table's static "
            "entries.",
            __func__,
            __LINE__);
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
    } else {
      // If there are no match keys, this is the default entry.
      PIPE_MGR_ASSERT(is_default_entry);
      entry->len_bytes = 0;
      entry->len_bits = 0;
      entry->key = NULL;
      entry->msk = NULL;
    }

    for (int i = 0; i < number_match_key_fields_values; ++i) {
      /* Get the match_key_field and corresponding value. */
      cJSON *mkf = NULL, *mkfv = NULL;
      err = bf_cjson_get_array_item(match_key_fields, i, &mkf);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_array_item(match_key_fields_values, i, &mkfv);
      CHECK_ERR(err, rc, cleanup);

      /* Parse a few details from the match key field. */
      int bit_width_full, match_type, start_bit, bit_width, pos;
      char *name = NULL;
      err = ctx_json_parse_match_key_field(mkf,
                                           &bit_width_full,
                                           &match_type,
                                           &start_bit,
                                           &bit_width,
                                           &pos,
                                           &name);
      CHECK_ERR(err, rc, cleanup);

      /* Parse the match key field value as well; k and m will be allocated and
       * populated by the parsing function.  They are allocated to a size to
       * hold bit_width_full number of bits even if the match_key_field_value is
       * narrower. */
      uint8_t *k = NULL, *m = NULL;
      err = parse_match_key_field_value(
          mkfv, name, match_type, bit_width_full, &k, &m);
      CHECK_ERR(err, rc, cleanup);

      if (!k || !m) {
        PIPE_MGR_DBGCHK(0);
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }

      /* Copy the key and mask from the match_key_fields_value entry into the
       * byte array for the entire key/mask.  Only populate from bit_width
       * number of bits starting at start_bit.  The value of the key and mask
       * from the match_key_fields_value is byte aligned but the destination
       * may not be aligned.
       * Note the the values stored in k and m are big endian, where the LSB is
       * in the highest array index and MSB is in the lowest array index. */
      int src_byte = (bit_width_full + 7) / 8 - 1;
      int dst_byte = mkf_to_offset[i] + src_byte - (start_bit / 8);
      int bits_to_write = bit_width;
      int start_bit_offset = start_bit % 8;
      for (int j = 0; j < bit_width; j += 8) {
        uint16_t msk = bits_to_write < 8 ? (1 << bits_to_write) - 1 : 0xFF;
        uint16_t x = k[src_byte];
        uint16_t y = m[src_byte];
        msk <<= start_bit_offset;
        x <<= start_bit_offset;
        y <<= start_bit_offset;
        entry->key[dst_byte] |= x & msk;
        entry->msk[dst_byte] |= y & msk;
        bits_to_write -= 8 - start_bit_offset;
        if (bits_to_write > 0 && start_bit_offset) {
          /* We have more data to write and the field is not byte aligned, put
           * the bits shifted out into the next byte. */
          x >>= 8;
          y >>= 8;
          msk >>= 8;
          entry->key[dst_byte - 1] |= x & msk;
          entry->msk[dst_byte - 1] |= y & msk;
          bits_to_write -= start_bit_offset;
        }
        --src_byte;
        --dst_byte;
      }

      /* For exact match tables the entire mask should be set to 1s. */
      if (match_table->match_type == EXACT_MATCH) {
        for (int j = 0; j < (bit_width_full + 7) / 8; ++j) {
          entry->msk[mkf_to_offset[i] + j] = 0xFF;
        }
      }
      PIPE_MGR_FREE(k);
      PIPE_MGR_FREE(m);
      PIPE_MGR_FREE(name);
    }

    // Parse the action parameter values for this static entry.
    cJSON *action_parameters_values_cjson = NULL;
    err = bf_cjson_get_object(static_entry_cjson,
                              CTX_JSON_STATIC_ENTRY_ACTION_PARAMETERS_VALUES,
                              &action_parameters_values_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *p4_parameters_cjson = NULL;
    err = bf_cjson_get_object(
        action_cjson, CTX_JSON_ACTION_P4_PARAMETERS, &p4_parameters_cjson);
    CHECK_ERR(err, rc, cleanup);

    int number_action_parameters_values =
        cJSON_GetArraySize(action_parameters_values_cjson);
    entry->action_entry.num_act_data = number_action_parameters_values;
    PIPE_MGR_ASSERT(number_action_parameters_values ==
                    cJSON_GetArraySize(p4_parameters_cjson));

    if (number_action_parameters_values > 0) {
      entry->action_entry.act_data = PIPE_MGR_CALLOC(
          number_action_parameters_values, sizeof(pipe_mgr_field_info_t));
      if (entry->action_entry.act_data == NULL) {
        LOG_ERROR(
            "%s:%d: Couldn't alloc memory for match table %s static entries.",
            __func__,
            __LINE__,
            match_table->name);
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
    }

    int parameter_index = 0;
    cJSON *parameter_cjson = NULL;
    CTX_JSON_FOR_EACH(parameter_cjson, action_parameters_values_cjson) {
      pipe_mgr_field_info_t *parameter_ptr =
          &entry->action_entry.act_data[parameter_index];

      char *parameter_name = NULL;
      err = bf_cjson_get_string_dup(parameter_cjson,
                                    CTX_JSON_STATIC_ENTRY_ACTION_PARAMETER_NAME,
                                    &parameter_name);
      CHECK_ERR(err, rc, cleanup);

      int bit_width = 0;
      err = ctx_json_parse_action_parameter_for_parameter(
          action_cjson, parameter_name, &bit_width);
      CHECK_ERR(err, rc, cleanup);

      uint8_t byte_width = (bit_width + 7) / 8;
      uint8_t *value = PIPE_MGR_CALLOC(byte_width, sizeof(uint8_t));
      err = bf_cjson_get_hex(parameter_cjson,
                             CTX_JSON_STATIC_ENTRY_ACTION_PARAMETER_VALUE,
                             value,
                             byte_width);
      if (err) {
        PIPE_MGR_FREE(value);
      }
      CHECK_ERR(err, rc, cleanup);

      parameter_ptr->name = parameter_name;
      parameter_ptr->bit_width = bit_width;
      parameter_ptr->value = value;
      ++parameter_index;
    }

    /* For each indirect resource get the index. */
    cJSON *resources_cjson = NULL;
    err = bf_cjson_get_object(
        action_cjson, CTX_JSON_ACTION_INDIRECT_RESOURCES, &resources_cjson);
    CHECK_ERR(err, rc, cleanup);
    cJSON *res_cjson = NULL;
    CTX_JSON_FOR_EACH(res_cjson, resources_cjson) {
      /* Get the index or, if it isn't a constant index, the index of the
       * action parameter holding the index. */
      char *mode = NULL;
      int res_index = 0;
      err = bf_cjson_get_string(
          res_cjson, CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE, &mode);
      CHECK_ERR(err, rc, cleanup);
      if (!strncmp(mode,
                   CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_INDEX,
                   strlen(CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_INDEX))) {
        int param_index = 0;
        char *param_name = NULL;
        err = bf_cjson_get_int(res_cjson,
                               CTX_JSON_INDIRECT_RESOURCE_PARAMETER_INDEX,
                               &param_index);
        err |= bf_cjson_get_string(
            res_cjson, CTX_JSON_INDIRECT_RESOURCE_PARAMETER_NAME, &param_name);
        CHECK_ERR(err, rc, cleanup);
        if (param_index >= cJSON_GetArraySize(p4_parameters_cjson)) {
          LOG_ERROR(
              "Param index %d out of range (%d), cannot parse static entries "
              "for %s",
              param_index,
              cJSON_GetArraySize(p4_parameters_cjson),
              match_table->name);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
        }
        if ((unsigned)param_index >= entry->action_entry.num_act_data) {
          LOG_ERROR(
              "Param index %d out of range (%d), cannot parse static entries "
              "for %s",
              param_index,
              entry->action_entry.num_act_data,
              match_table->name);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
        }
        if (0 != strcmp(param_name,
                        entry->action_entry.act_data[param_index].name)) {
          LOG_ERROR(
              "Param index %d mismatch expected field %s got field %s, cannot "
              "parse static entries for %s",
              param_index,
              param_name,
              entry->action_entry.act_data[param_index].name,
              match_table->name);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
        }
        for (unsigned i = 0;
             i < (entry->action_entry.act_data[param_index].bit_width + 7) / 8;
             ++i) {
          res_index = (res_index << 8) |
                      entry->action_entry.act_data[param_index].value[i];
        }
      } else if (!strncmp(
                     mode,
                     CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_CONSTANT,
                     strlen(CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_CONSTANT))) {
        err = bf_cjson_get_int(
            res_cjson, CTX_JSON_INDIRECT_RESOURCE_VALUE, &res_index);
        CHECK_ERR(err, rc, cleanup);
      } else {
        PIPE_MGR_DBGCHK(0);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }

      /* Next, get the handle of the resource. */
      int res_hdl = -1;
      err = bf_cjson_get_handle(devid,
                                prof_id,
                                res_cjson,
                                CTX_JSON_INDIRECT_RESOURCE_HANDLE,
                                &res_hdl);
      CHECK_ERR(err, rc, cleanup);

      pipe_mgr_ind_res_info_t *x =
          PIPE_MGR_REALLOC(entry->action_entry.ind_res,
                           (sizeof *x) * (entry->action_entry.num_ind_res + 1));
      if (!x) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      entry->action_entry.ind_res = x;
      entry->action_entry.ind_res[entry->action_entry.num_ind_res].handle =
          res_hdl;
      entry->action_entry.ind_res[entry->action_entry.num_ind_res].idx =
          res_index;
      ++entry->action_entry.num_ind_res;
    }

    /* Get the direct resources this action is using.  Skip this step if
     * p4c-tofino was used since it doesn't support static entries with direct
     * resources. */
    if (dev_info->profile_info[prof_id]->compiler_version[0] >= 6) {
      err = bf_cjson_get_object(
          action_cjson, CTX_JSON_ACTION_DIRECT_RESOURCES, &resources_cjson);
      CHECK_ERR(err, rc, cleanup);
      res_cjson = NULL;
      CTX_JSON_FOR_EACH(res_cjson, resources_cjson) {
        int res_hdl = -1;
        err = bf_cjson_get_handle(devid,
                                  prof_id,
                                  res_cjson,
                                  CTX_JSON_DIRECT_RESOURCE_HANDLE,
                                  &res_hdl);
        CHECK_ERR(err, rc, cleanup);
        pipe_mgr_dir_res_info_t *x = PIPE_MGR_REALLOC(
            entry->action_entry.dir_res,
            (sizeof *x) * (entry->action_entry.num_dir_res + 1));
        if (!x) {
          rc = PIPE_NO_SYS_RESOURCES;
          goto cleanup;
        }
        entry->action_entry.dir_res = x;
        entry->action_entry.dir_res[entry->action_entry.num_dir_res].handle =
            res_hdl;
        ++entry->action_entry.num_dir_res;
      }
    }

    ++static_entry_index;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses the hash action default entries for a given match table.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @param match_table The internal match table information struct.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_hash_action_spec(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *table_cjson,
    pipe_mat_tbl_info_t *match_table) {
  int err = 0, i = 0, number_match_parameters = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  if (!table_cjson || !match_table) {
    LOG_ERROR("%s:%d Null pointer arguments passed", __func__, __LINE__);
    return PIPE_INVALID_ARG;
  }

  int default_action_handle = -1;
  bf_cjson_try_get_handle(devid,
                          prof_id,
                          table_cjson,
                          CTX_JSON_MATCH_TABLE_DEFAULT_ACTION_HANDLE,
                          &default_action_handle);

  // This hash action table does not have a default action associated to it.
  // There is nothing else to do.
  if (default_action_handle == -1) {
    return rc;
  }

  match_table->hash_action_info =
      PIPE_MGR_CALLOC(1, sizeof(hash_action_tbl_info_t));
  if (match_table->hash_action_info == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for hash action information.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  /*
   * Parse hash action's match parameters.
   */
  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  number_match_parameters = cJSON_GetArraySize(match_key_fields_cjson);
  match_table->hash_action_info->num_match_params = number_match_parameters;
  match_table->hash_action_info->match_param_list =
      PIPE_MGR_CALLOC(number_match_parameters, sizeof(pipe_mgr_field_info_t));
  if (!match_table->hash_action_info->match_param_list) {
    LOG_ERROR("%s:%d: Could not allocate memory for hash action information.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  i = 0;
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    // NOTE: Here, the driver is *not* expecting the padded values, as happens
    // in the entry format case! The values are simply the same as those
    // in the match key fields entry in the context JSON.
    char *name = NULL;
    int bit_width = 0;

    err = bf_cjson_get_string_dup(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_NAME, &name);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &bit_width);
    CHECK_ERR(err, rc, cleanup);

    pipe_mgr_field_info_t *match_fields =
        match_table->hash_action_info->match_param_list;

    match_fields[i].name = name;
    match_fields[i].bit_width = bit_width;
    ++i;
  }

  return rc;

cleanup:
  if (match_table->hash_action_info) {
    if (match_table->hash_action_info->match_param_list) {
      for (i = 0; i < number_match_parameters; ++i) {
        if (match_table->hash_action_info->match_param_list[i].name)
          PIPE_MGR_FREE(
              match_table->hash_action_info->match_param_list[i].name);
      }
      PIPE_MGR_FREE(match_table->hash_action_info->match_param_list);
    }
    PIPE_MGR_FREE(match_table->hash_action_info);
    match_table->hash_action_info = NULL;
  }

  return rc;
}

/**
 * Given the cJSON structure for a match table, parse the default action
 * blacklist. Actions may be blacklisted because they involve random number
 * generation, or hash computations.
 *
 * @param table_cjson The cJSON structure corresponding to the match table.
 * @param match_table The internal structure representing the match table in
 * the RMT configuration.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_default_action_blacklist(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *table_cjson,
    pipe_mat_tbl_info_t *match_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  int number_actions_blacklisted = 0;
  cJSON *action_cjson = NULL;
  CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
    bool allowed_as_default_action = false;
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_ALLOWED_AS_DEFAULT_ACTION,
                             &allowed_as_default_action);
    CHECK_ERR(err, rc, cleanup);

    if (!allowed_as_default_action) {
      ++number_actions_blacklisted;
    }
  }

  if (number_actions_blacklisted == 0) {
    // Nothing to do; default these values.
    match_table->def_act_blacklist_size = 0;
    match_table->def_act_blacklist = NULL;
  } else {
    match_table->def_act_blacklist_size = number_actions_blacklisted;
    match_table->def_act_blacklist =
        PIPE_MGR_CALLOC(number_actions_blacklisted, sizeof(pipe_act_fn_hdl_t));
    if (match_table->def_act_blacklist == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for table's default action "
          "blacklist.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    int index = 0;
    CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
      bool allowed_as_default_action = false;
      err |= bf_cjson_get_bool(action_cjson,
                               CTX_JSON_ACTION_ALLOWED_AS_DEFAULT_ACTION,
                               &allowed_as_default_action);
      CHECK_ERR(err, rc, cleanup);

      if (!allowed_as_default_action) {
        int action_handle = 0;
        err |= bf_cjson_get_handle(devid,
                                   prof_id,
                                   action_cjson,
                                   CTX_JSON_ACTION_HANDLE,
                                   &action_handle);
        CHECK_ERR(err, rc, cleanup);

        match_table->def_act_blacklist[index] = action_handle;
        ++index;
      }
    }
  }
  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_rmt_cfg_alpm_trie_spec(
    alpm_mat_tbl_info_t *alpm_info,
    cJSON *match_key_cjson,
    cJSON *excluded_fields_cjson) {
  int spec_offset = 0, bit_width = 0;
  uint32_t i = 0;
  cJSON *match_key_item = NULL;

  uint32_t num_match_fields = GETARRSZ(match_key_cjson);
  alpm_info->field_info =
      PIPE_MGR_CALLOC(num_match_fields, sizeof(alpm_field_info_t));
  alpm_field_info_t *field_info = NULL;
  alpm_info->num_excluded_bits = 0;

  char *field_names[num_match_fields];
  char *excluded_field_names[num_match_fields];
  uint32_t field_names_size = 0;
  uint32_t field_index = 0;
  uint32_t num_excluded_fields = 0;
  PIPE_MGR_MEMSET(field_names, 0, num_match_fields * sizeof(char *));
  PIPE_MGR_MEMSET(excluded_field_names, 0, num_match_fields * sizeof(char *));

  /* Find LPM field */
  char *lpm_field_name = NULL;
  CTX_JSON_FOR_EACH(match_key_item, match_key_cjson) {
    char *match_type = NULL;
    bf_cjson_get_string(
        match_key_item, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type);
    if (!strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_LPM)) {
      bf_cjson_get_string(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_NAME, &lpm_field_name);
      break;
    }
  }

  /* Populate the excluded fields first */
  if (excluded_fields_cjson) {
    num_excluded_fields = GETARRSZ(excluded_fields_cjson);
    if (num_excluded_fields > 0) {
      int half_excluded_spec_offset = 0, half_excluded_bit_width = 0;
      int half_excluded_slice_offset = 0, half_excluded_slice_width = 0;
      uint32_t excluded_bits[num_excluded_fields];
      PIPE_MGR_MEMSET(excluded_bits, 0, num_excluded_fields * sizeof(uint32_t));

      cJSON *excluded_field = NULL;
      CTX_JSON_FOR_EACH(excluded_field, excluded_fields_cjson) {
        char *field_name = NULL;
        int excluded_field_bits = 0;
        bf_cjson_get_string(excluded_field,
                            CTX_JSON_MATCH_ATTRIBUTES_EXCLUDED_FIELD_NAME,
                            &field_name);
        bf_cjson_get_int(excluded_field,
                         CTX_JSON_MATCH_ATTRIBUTES_EXCLUDED_FIELD_BITS,
                         &excluded_field_bits);
        excluded_field_names[field_index] = field_name;
        excluded_bits[field_index] = excluded_field_bits;
        field_index++;
        alpm_info->num_excluded_bits += excluded_field_bits;
      }

      field_index = 0;
      CTX_JSON_FOR_EACH(match_key_item, match_key_cjson) {
        char *field_name = NULL;
        int slice_width = 0, slice_offset = 0;
        bf_cjson_get_string(
            match_key_item, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
        bf_cjson_get_int(
            match_key_item, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &slice_offset);
        bf_cjson_get_int(
            match_key_item, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &slice_width);
        bf_cjson_get_int(match_key_item,
                         CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL,
                         &bit_width);

        /* Save the key width for LPM field if present */
        if (lpm_field_name && strcmp(lpm_field_name, field_name) == 0) {
          alpm_info->lpm_field_key_width = slice_width;
        }

        /* Skip duplicate fields */
        bool found = false;
        for (i = 0; i < field_names_size; i++) {
          if (!strcmp(field_names[i], field_name)) {
            found = true;
            break;
          }
        }
        if (found) {
          continue;
        }
        field_names[field_names_size] = field_name;
        field_names_size++;

        /* Only handle excluded fields on first pass */
        for (i = 0; i < num_excluded_fields; i++) {
          if (!strcmp(excluded_field_names[i], field_name)) {
            found = true;
            break;
          }
        }
        if (found) {
          if ((uint32_t)bit_width > excluded_bits[i]) {
            /* Skip half-excluded field for now, since it should come after the
             * fully excluded fields.
             */
            half_excluded_spec_offset = spec_offset;
            half_excluded_bit_width = bit_width;
            half_excluded_slice_offset = slice_offset;
            half_excluded_slice_width = slice_width;
          } else {
            field_info = &alpm_info->field_info[field_index];
            field_info->byte_offset = spec_offset;
            field_info->bit_width = bit_width;
            field_info->slice_offset = PIPE_MGR_CALLOC(1, sizeof(uint32_t));
            field_info->slice_width = PIPE_MGR_CALLOC(1, sizeof(uint32_t));
            field_info->slice_offset[0] = slice_offset;
            field_info->slice_width[0] = slice_width;
            field_info->num_slices = 1;
            field_index++;
          }
        }
        spec_offset += (bit_width + 7) / 8;
      }

      if (half_excluded_bit_width) {
        field_info = &alpm_info->field_info[field_index];
        field_info->byte_offset = half_excluded_spec_offset;
        field_info->bit_width = half_excluded_bit_width;
        field_info->slice_offset = PIPE_MGR_CALLOC(1, sizeof(uint32_t));
        field_info->slice_width = PIPE_MGR_CALLOC(1, sizeof(uint32_t));
        field_info->slice_offset[0] = half_excluded_slice_offset;
        field_info->slice_width[0] = half_excluded_slice_width;
        field_info->num_slices = 1;
        field_index++;
      }
    }
  }

  PIPE_MGR_MEMSET(field_names, 0, num_match_fields * sizeof(char *));
  if (!lpm_field_name) {
    LOG_ERROR("%s:%d Failed to find lpm field while parsing alpm spec",
              __func__,
              __LINE__);
    return PIPE_OBJ_NOT_FOUND;
  }

  /* Parse any remaining fields as non-excluded. */
  uint32_t non_excluded_index = field_index;
  uint32_t curr_field_index = field_index;
  if (!field_names_size || field_index < field_names_size) {
    spec_offset = 0;
    field_names_size = 0;
    CTX_JSON_FOR_EACH(match_key_item, match_key_cjson) {
      char *field_name = NULL;
      char *match_type = NULL;
      int slice_width = 0;
      int slice_offset = 0;
      bf_cjson_get_string(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
      bf_cjson_get_string(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type);
      bf_cjson_get_int(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &slice_offset);
      bf_cjson_get_int(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &slice_width);
      bf_cjson_get_int(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL, &bit_width);

      /* Skip excluded fields on second pass */
      bool found = false;
      for (i = 0; i < num_excluded_fields; i++) {
        if (!strcmp(excluded_field_names[i], field_name)) {
          found = true;
          break;
        }
      }
      if (found) {
        spec_offset += (bit_width + 7) / 8;
        continue;
      }

      /* Update slice information for duplicate fields */
      for (i = non_excluded_index; i < num_match_fields; i++) {
        if (field_names[i] && !strcmp(field_names[i], field_name)) {
          found = true;
          break;
        }
      }
      if (found) {
        curr_field_index = i;
      } else {
        if (!strcmp(field_name, lpm_field_name)) {
          field_names[num_match_fields - 1] = field_name;
          curr_field_index = num_match_fields - 1;
        } else {
          field_names[field_index] = field_name;
          curr_field_index = field_index;
          field_index++;
        }
        field_info = &alpm_info->field_info[curr_field_index];
        field_info->byte_offset = spec_offset;
        field_info->bit_width = bit_width;
        spec_offset += (bit_width + 7) / 8;
      }

      /* Add slice information */
      field_info = &alpm_info->field_info[curr_field_index];
      field_info->slice_offset =
          PIPE_MGR_REALLOC(field_info->slice_offset,
                           (field_info->num_slices + 1) * sizeof(uint32_t));
      field_info->slice_width =
          PIPE_MGR_REALLOC(field_info->slice_width,
                           (field_info->num_slices + 1) * sizeof(uint32_t));
      /* For the slices of the lpm field, keep the lpm slice last */
      if (curr_field_index == num_match_fields - 1 &&
          strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_LPM) != 0) {
        field_info->slice_offset[field_info->num_slices] =
            field_info->slice_offset[field_info->num_slices - 1];
        field_info->slice_width[field_info->num_slices] =
            field_info->slice_width[field_info->num_slices - 1];
        field_info->slice_offset[field_info->num_slices - 1] = slice_offset;
        field_info->slice_width[field_info->num_slices - 1] = slice_width;
      } else {
        field_info->slice_offset[field_info->num_slices] = slice_offset;
        field_info->slice_width[field_info->num_slices] = slice_width;
      }
      /* Save the key width for LPM field */
      if (strcmp(lpm_field_name, field_name) == 0) {
        alpm_info->lpm_field_key_width = slice_width;
      }
      field_info->num_slices++;
    }

    /* Add one more field index for lpm field and shift if needed */
    field_index++;
    if (field_index < num_match_fields) {
      PIPE_MGR_MEMCPY(&alpm_info->field_info[field_index - 1],
                      &alpm_info->field_info[num_match_fields - 1],
                      sizeof(alpm_field_info_t));
    }
  }

  /* Reallocate if match_key_fields had duplicates */
  if (field_index < num_match_fields) {
    alpm_info->field_info = PIPE_MGR_REALLOC(
        alpm_info->field_info, field_index * sizeof(alpm_field_info_t));
  }
  alpm_info->num_fields = field_index;

  return PIPE_SUCCESS;
}

static pipe_status_t ctx_json_parse_rmt_cfg_alpm_spec(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    alpm_mat_tbl_info_t *alpm_info,
    cJSON *table_cjson,
    cJSON *pre_classifier_cjson,
    cJSON *atcam_table_cjson,
    cJSON *match_attributes_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int pre_classifier_handle = 0;
  int atcam_handle = 0;
  int bins_per_partition = 0;
  int max_subtrees_per_partition = 0;

  err |= bf_cjson_get_handle(devid,
                             prof_id,
                             pre_classifier_cjson,
                             CTX_JSON_TABLE_HANDLE,
                             &pre_classifier_handle);
  err |= bf_cjson_get_handle(
      devid, prof_id, atcam_table_cjson, CTX_JSON_TABLE_HANDLE, &atcam_handle);
  err |= bf_cjson_get_int(match_attributes_cjson,
                          CTX_JSON_MATCH_ATTRIBUTES_BINS_PER_PARTITION,
                          &bins_per_partition);
  err |= bf_cjson_get_int(match_attributes_cjson,
                          CTX_JSON_MATCH_ATTRIBUTES_MAX_SUBTREES_PER_PARTITION,
                          &max_subtrees_per_partition);

  alpm_info->preclass_handle = pre_classifier_handle;
  alpm_info->atcam_handle = atcam_handle;
  alpm_info->partition_depth = bins_per_partition;
  alpm_info->max_subtrees_per_partition = max_subtrees_per_partition;

  char *partition_field_name = NULL;
  int action_handle = 0;

  err |= bf_cjson_get_string(match_attributes_cjson,
                             CTX_JSON_MATCH_ATTRIBUTES_PARTITION_FIELD_NAME,
                             &partition_field_name);

  /* Check if atcam key width optimization is used */
  cJSON *atcam_subset_width_json = NULL;
  int atcam_subset_width = 0;
  bf_cjson_try_get_object(match_attributes_cjson,
                          CTX_JSON_MATCH_ATTRIBUTES_ATCAM_SUBSET_WIDTH,
                          &atcam_subset_width_json);
  if (atcam_subset_width_json != NULL) {
    err |= bf_cjson_get_int(match_attributes_cjson,
                            CTX_JSON_MATCH_ATTRIBUTES_ATCAM_SUBSET_WIDTH,
                            &atcam_subset_width);
    CHECK_ERR(err, rc, cleanup);
    alpm_info->atcam_subset_key_width = atcam_subset_width;
  } else {
    alpm_info->atcam_subset_key_width = 0;
  }

  /* Check if shift_granularity is used */
  int shift_granularity = 0;
  err |= bf_cjson_try_get_int(match_attributes_cjson,
                              CTX_JSON_MATCH_ATTRIBUTES_SHIFT_GRANULARITY,
                              &shift_granularity);
  CHECK_ERR(err, rc, cleanup);
  alpm_info->shift_granularity = shift_granularity;

  cJSON *set_partition_actions_cjson = NULL;
  cJSON *set_partition_action_cjson = NULL;
  int set_partition_action_handle = 0;
  err |=
      bf_cjson_get_object(match_attributes_cjson,
                          CTX_JSON_MATCH_ATTRIBUTES_SET_PARTITION_ACTION_HANDLE,
                          &set_partition_actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  if (set_partition_actions_cjson->type != cJSON_Array) {
    err |= bf_cjson_get_handle(
        devid,
        prof_id,
        match_attributes_cjson,
        CTX_JSON_MATCH_ATTRIBUTES_SET_PARTITION_ACTION_HANDLE,
        &action_handle);
    CHECK_ERR(err, rc, cleanup);

    alpm_info->num_act_fn_hdls = 1;
    alpm_info->act_fn_hdls = PIPE_MGR_CALLOC(1, sizeof(pipe_act_fn_hdl_t));
    if (alpm_info->act_fn_hdls == NULL) {
      LOG_ERROR("%s:%d: Memory allocation failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    alpm_info->act_fn_hdls[0] = action_handle;
  } else {
    int num_act_fn_hdls = cJSON_GetArraySize(set_partition_actions_cjson);
    alpm_info->num_act_fn_hdls = num_act_fn_hdls;
    alpm_info->act_fn_hdls =
        PIPE_MGR_CALLOC(num_act_fn_hdls, sizeof(pipe_act_fn_hdl_t));
    if (alpm_info->act_fn_hdls == NULL) {
      LOG_ERROR("%s:%d: Memory allocation failure", __func__, __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    int index = 0;
    CTX_JSON_FOR_EACH(set_partition_action_cjson, set_partition_actions_cjson) {
      set_partition_action_handle = set_partition_action_cjson->valueint;
      err |= pipe_mgr_tbl_hdl_set_pipe(devid,
                                       prof_id,
                                       set_partition_action_handle,
                                       (pipe_tbl_hdl_t *)&action_handle);
      CHECK_ERR(err, rc, cleanup);

      alpm_info->act_fn_hdls[index++] = action_handle;
    }

    // Assign the first action handle to get the P4 parameters for the action
    action_handle = alpm_info->act_fn_hdls[0];
  }

  // Get the actions from the pre classifier, and match key fields for the
  // top-level ALPM table.
  cJSON *match_key_fields_cjson = NULL;
  cJSON *actions_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                             &match_key_fields_cjson);
  err |= bf_cjson_get_object(
      pre_classifier_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *action_cjson = NULL;
  CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
    int item_handle = 0;
    err |= bf_cjson_get_handle(
        devid, prof_id, action_cjson, CTX_JSON_ACTION_HANDLE, &item_handle);
    CHECK_ERR(err, rc, cleanup);

    if (item_handle == action_handle) {
      break;
    }
  }
  if (action_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not find action with handle 0x%x in pre classifier.",
        __func__,
        __LINE__,
        action_handle);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  int partition_spec_length = 0;
  // These values are *NOT* the padded-to-the-byte values that action spec
  // contains; instead, it is simply the value on the p4 parameters field.
  rc = ctx_json_parse_action_parameter_for_parameter(
      action_cjson, partition_field_name, &partition_spec_length);
  CHECK_RC(rc, cleanup);

  alpm_info->partition_idx_field_width = partition_spec_length;
  alpm_info->partition_idx_start_bit = 0;

  cJSON *excluded_fields_cjson = NULL;
  bf_cjson_try_get_object(match_attributes_cjson,
                          CTX_JSON_MATCH_ATTRIBUTES_EXCLUDED_FIELDS,
                          &excluded_fields_cjson);

  rc = ctx_json_parse_rmt_cfg_alpm_trie_spec(
      alpm_info, match_key_fields_cjson, excluded_fields_cjson);

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_rmt_cfg_default_spec_ind_res(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *table_cjson,
    char *resource_name,
    pipe_tbl_hdl_t *res_tbl_hdl_p) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  int res_tbl_hdl = 0;
  char *name = NULL;
  char *how_referenced = NULL;

  cJSON *res_tbl_refs_cjson = NULL, *res_tbl_ref_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_STATISTICS_TABLE_REFS,
                             &res_tbl_refs_cjson);
  CHECK_ERR(err, rc, cleanup);

  CTX_JSON_FOR_EACH(res_tbl_ref_cjson, res_tbl_refs_cjson) {
    name = NULL;
    err |= bf_cjson_get_string(res_tbl_ref_cjson, CTX_JSON_TABLE_NAME, &name);
    err |= bf_cjson_get_string(
        res_tbl_ref_cjson, CTX_JSON_TABLE_HOW_REFERENCED, &how_referenced);
    CHECK_ERR(err, rc, cleanup);
    if (!strcmp(name, resource_name) &&
        !strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_INDIRECT)) {
      err |= bf_cjson_get_handle(devid,
                                 prof_id,
                                 res_tbl_ref_cjson,
                                 CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE,
                                 &res_tbl_hdl);
      CHECK_ERR(err, rc, cleanup);
      *res_tbl_hdl_p = res_tbl_hdl;
      return PIPE_SUCCESS;
    }
  }

  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_MATCH_TABLE_METER_TABLE_REFS, &res_tbl_refs_cjson);
  CHECK_ERR(err, rc, cleanup);

  CTX_JSON_FOR_EACH(res_tbl_ref_cjson, res_tbl_refs_cjson) {
    name = NULL;
    err |= bf_cjson_get_string(res_tbl_ref_cjson, CTX_JSON_TABLE_NAME, &name);
    err |= bf_cjson_get_string(
        res_tbl_ref_cjson, CTX_JSON_TABLE_HOW_REFERENCED, &how_referenced);
    CHECK_ERR(err, rc, cleanup);
    if (!strcmp(name, resource_name) &&
        !strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_INDIRECT)) {
      err |= bf_cjson_get_handle(devid,
                                 prof_id,
                                 res_tbl_ref_cjson,
                                 CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE,
                                 &res_tbl_hdl);
      CHECK_ERR(err, rc, cleanup);
      *res_tbl_hdl_p = res_tbl_hdl;
      return PIPE_SUCCESS;
    }
  }

  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_STATEFUL_TABLE_REFS,
                             &res_tbl_refs_cjson);
  CHECK_ERR(err, rc, cleanup);

  CTX_JSON_FOR_EACH(res_tbl_ref_cjson, res_tbl_refs_cjson) {
    name = NULL;
    err |= bf_cjson_get_string(res_tbl_ref_cjson, CTX_JSON_TABLE_NAME, &name);
    err |= bf_cjson_get_string(
        res_tbl_ref_cjson, CTX_JSON_TABLE_HOW_REFERENCED, &how_referenced);
    CHECK_ERR(err, rc, cleanup);
    if (!strcmp(name, resource_name) &&
        !strcmp(how_referenced, CTX_JSON_TABLE_HOW_REFERENCED_INDIRECT)) {
      err |= bf_cjson_get_handle(devid,
                                 prof_id,
                                 res_tbl_ref_cjson,
                                 CTX_JSON_MATCH_TABLE_REFERENCES_HANDLE,
                                 &res_tbl_hdl);
      CHECK_ERR(err, rc, cleanup);
      *res_tbl_hdl_p = res_tbl_hdl;
      return PIPE_SUCCESS;
    }
  }

  return rc;

cleanup:
  return rc;
}

static uint32_t parse_stream(uint8_t *stream, uint8_t bit_width) {
  PIPE_MGR_DBGCHK(bit_width > 0 && bit_width <= 32);
  uint32_t ret = 0;
  uint8_t byte_width = (bit_width + 7) / 8;
  uint8_t byte_offset = 4 - byte_width;

  PIPE_MGR_MEMCPY((char *)(&ret) + byte_offset, stream, byte_width);
  return ntohl(ret);
}

/**
 * Parses the default entry for a given match table.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @param match_table The internal match table information struct.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_default_spec(
    rmt_dev_info_t *dev_info,
    profile_id_t prof_id,
    cJSON *table_cjson,
    pipe_mat_tbl_info_t *match_table,
    pipe_act_fn_hdl_t def_act_fn_hdl) {
  bf_dev_id_t devid = dev_info->dev_id;
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *action_cjson = NULL;
  CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
    int item_handle = 0;
    err |= bf_cjson_get_handle(
        devid, prof_id, action_cjson, CTX_JSON_ACTION_HANDLE, &item_handle);
    CHECK_ERR(err, rc, cleanup);

    if ((pipe_act_fn_hdl_t)item_handle == def_act_fn_hdl) {
      break;
    }
  }
  if (action_cjson == NULL) {
    LOG_ERROR("%s:%d: Could not find default action with handle 0x%x on table.",
              __func__,
              __LINE__,
              def_act_fn_hdl);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  bool is_compiler_added_action = false;
  bf_cjson_try_get_bool(action_cjson,
                        CTX_JSON_ACTION_IS_COMPILER_ADDED_ACTION,
                        &is_compiler_added_action);
  bool is_constant_action = false;
  bf_cjson_try_get_bool(
      action_cjson, CTX_JSON_ACTION_IS_CONSTANT_ACTION, &is_constant_action);

  cJSON *p4_parameters_cjson = NULL;
  err = bf_cjson_get_object(
      action_cjson, CTX_JSON_ACTION_P4_PARAMETERS, &p4_parameters_cjson);
  CHECK_ERR(err, rc, cleanup);
  int number_parameters = cJSON_GetArraySize(p4_parameters_cjson);

  match_table->default_info = PIPE_MGR_CALLOC(1, sizeof(pipe_default_info_t));
  if (match_table->default_info == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for default action info.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  pipe_mgr_action_entry_t *action_entry;
  action_entry = &match_table->default_info->action_entry;
  match_table->default_info->p4_default = !is_compiler_added_action;
  match_table->default_info->is_const = is_constant_action;
  action_entry->num_act_data = number_parameters;
  action_entry->act_fn_hdl = def_act_fn_hdl;
  if (number_parameters) {
    action_entry->act_data =
        PIPE_MGR_CALLOC(number_parameters, sizeof(pipe_mgr_field_info_t));
    if (!action_entry->act_data) {
      LOG_ERROR("%s:%d: Could not allocate memory for default action params.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  uint32_t i = 0;
  cJSON *p4_parameter_cjson = NULL;
  CTX_JSON_FOR_EACH(p4_parameter_cjson, p4_parameters_cjson) {
    // NOTE: Here, the driver is *not* expecting the padded values, as happens
    // in the entry format case! The values are simply the same as those
    // in the actions entry in the context JSON.
    char *name = NULL;
    int bit_width = 0;

    err = bf_cjson_get_string_dup(
        p4_parameter_cjson, CTX_JSON_P4_PARAMETER_NAME, &name);
    err |= bf_cjson_get_int(
        p4_parameter_cjson, CTX_JSON_P4_PARAMETER_BIT_WIDTH, &bit_width);
    CHECK_ERR(err, rc, cleanup);

    uint8_t byte_width = (bit_width + 7) / 8;
    uint8_t *default_value = PIPE_MGR_CALLOC(byte_width, sizeof(uint8_t));
    if (!default_value) {
      LOG_ERROR("%s:%d: Could not allocate memory for default action value.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    err = bf_cjson_try_get_hex(p4_parameter_cjson,
                               CTX_JSON_P4_PARAMETER_DEFAULT_VALUE,
                               default_value,
                               byte_width);
    CHECK_ERR(err, rc, cleanup);

    pipe_mgr_field_info_t *action_fields = action_entry->act_data;
    action_fields[i].name = name;
    action_fields[i].bit_width = bit_width;
    action_fields[i].value = default_value;
    ++i;
  }

  /* Parse the indirect resources used by this action. */
  cJSON *resources_cjson = NULL;
  err = bf_cjson_get_object(
      action_cjson, CTX_JSON_ACTION_INDIRECT_RESOURCES, &resources_cjson);
  CHECK_ERR(err, rc, cleanup);
  int ind_res_cnt = cJSON_GetArraySize(resources_cjson);
  if (ind_res_cnt) {
    action_entry->ind_res =
        PIPE_MGR_CALLOC(ind_res_cnt, sizeof(pipe_def_ind_res_t));
    if (action_entry->ind_res == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for default action info "
          "indirect resource list",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  i = 0;
  cJSON *p4_indirect_resource_cjson = NULL;
  CTX_JSON_FOR_EACH(p4_indirect_resource_cjson, resources_cjson) {
    char *access_mode = NULL;
    char *resource_name = NULL;
    pipe_tbl_hdl_t res_tbl_hdl = 0;

    err = bf_cjson_get_string(p4_indirect_resource_cjson,
                              CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE,
                              &access_mode);
    err |= bf_cjson_get_string(p4_indirect_resource_cjson,
                               CTX_JSON_INDIRECT_RESOURCE_RESOURCE_NAME,
                               &resource_name);
    CHECK_ERR(err, rc, cleanup);

    rc = ctx_json_parse_rmt_cfg_default_spec_ind_res(
        devid, prof_id, table_cjson, resource_name, &res_tbl_hdl);
    if (rc != PIPE_SUCCESS) {
      goto cleanup;
    }
    if (!strcmp(access_mode, CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_INDEX)) {
      int param_index = 0;
      err = bf_cjson_get_int(p4_indirect_resource_cjson,
                             CTX_JSON_INDIRECT_RESOURCE_PARAMETER_INDEX,
                             &param_index);
      CHECK_ERR(err, rc, cleanup);

      if (res_tbl_hdl > 0) {
        if ((unsigned)param_index >= action_entry->num_act_data) {
          LOG_ERROR(
              "Param index %d out of range (%d), cannot parse default entry",
              param_index,
              action_entry->num_act_data);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
        }
        action_entry->ind_res[i].handle = res_tbl_hdl;
        /* Convert the indirect resource index bytestream to a primitive */
        if (action_entry->act_data[param_index].bit_width > 32) {
          LOG_ERROR("%s:%d Cannot support indirect resource width %d (max 32)",
                    __func__,
                    __LINE__,
                    action_entry->act_data[param_index].bit_width);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
        }
        action_entry->ind_res[i].idx =
            parse_stream(action_entry->act_data[param_index].value,
                         action_entry->act_data[param_index].bit_width);
        ++i;
      }
    } else if (!strcmp(access_mode,
                       CTX_JSON_INDIRECT_RESOURCE_ACCESS_MODE_CONSTANT)) {
      int value = 0;
      err |= bf_cjson_get_int(
          p4_indirect_resource_cjson, CTX_JSON_INDIRECT_RESOURCE_VALUE, &value);
      CHECK_ERR(err, rc, cleanup);

      if (res_tbl_hdl > 0) {
        action_entry->ind_res[i].handle = res_tbl_hdl;
        action_entry->ind_res[i].idx = value;
        ++i;
      }
    } else {
      PIPE_MGR_DBGCHK(0);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  }
  action_entry->num_ind_res = i;

  /* Finally get the direct resources used by the default entry.  Note this is
   * only available in compiler version 6 and later. */
  if (dev_info->profile_info[prof_id]->compiler_version[0] >= 6) {
    err = bf_cjson_get_object(
        action_cjson, CTX_JSON_ACTION_DIRECT_RESOURCES, &resources_cjson);
    CHECK_ERR(err, rc, cleanup);
    cJSON *res_cjson = NULL;
    CTX_JSON_FOR_EACH(res_cjson, resources_cjson) {
      int res_hdl = -1;
      err = bf_cjson_get_handle(
          devid, prof_id, res_cjson, CTX_JSON_DIRECT_RESOURCE_HANDLE, &res_hdl);
      CHECK_ERR(err, rc, cleanup);
      pipe_mgr_dir_res_info_t *x = PIPE_MGR_REALLOC(
          action_entry->dir_res, (sizeof *x) * (action_entry->num_dir_res + 1));
      if (!x) {
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      action_entry->dir_res = x;
      action_entry->dir_res[action_entry->num_dir_res].handle = res_hdl;
      ++action_entry->num_dir_res;
    }
  }

  return rc;

cleanup:
  return rc;
}

static bool is_ternary_table_lpm_or_exact(cJSON *table_cjson) {
  cJSON *match_key_fields_cjson = NULL;
  if (bf_cjson_get_object(table_cjson,
                          CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                          &match_key_fields_cjson)) {
    return false;
  }

  bool is_lpm = false, is_exact = false;
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *match_type = NULL;
    if (bf_cjson_get_string(
            field_cjson, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type)) {
      return false;
    }

    if (!match_type ||
        !strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_TERNARY) ||
        !strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_RANGE)) {
      return false;
    }

    if (!strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_LPM)) {
      is_lpm = true;
    }
    if (!strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_EXACT)) {
      is_exact = true;
    }
  }

  return (is_lpm || is_exact);
}

static pipe_status_t ctx_json_parse_rmt_cfg_partition_idx_info(
    cJSON *match_attributes_cjson,
    cJSON *match_key_fields_cjson,
    pipe_mat_tbl_info_t *match_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  match_table->partition_idx_info = NULL;
  match_table->partition_idx_info =
      PIPE_MGR_CALLOC(1, sizeof(pipe_partition_idx_info_t));
  if (match_table->partition_idx_info == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for partition idx info.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  err = bf_cjson_get_string_dup(
      match_attributes_cjson,
      CTX_JSON_MATCH_ATTRIBUTES_PARTITION_FIELD_NAME,
      &match_table->partition_idx_info->partition_field_name);
  CHECK_ERR(err, rc, cleanup);

  int spec_length = 0;
  int spec_offset = 0;

  /* Find match type, spec offset and spec length from match_key_fields.
   * This assumes the partition_field_name only appears a single time in the
   * match_key_fields section of the context.json */
  bool use_global_name = false; /* Today the partition index is published with
                                   the field's "name", not "global_name". */
  err = ctx_json_parse_spec_details_for_field(
      use_global_name,
      match_key_fields_cjson,
      match_table->partition_idx_info->partition_field_name,
      0,
      1, /* Any width will do. */
      &spec_length,
      &spec_offset,
      NULL,
      NULL);
  CHECK_ERR(err, rc, cleanup);

  // These are bit size values, example, if size/spec_length is 10 bits, and the
  // position in the spec is at the beginning, then spec_offset will be 6 bits.
  match_table->partition_idx_info->partition_idx_field_width = spec_length;
  match_table->partition_idx_info->partition_idx_start_bit = spec_offset;
  return rc;

cleanup:
  if (match_table->partition_idx_info != NULL) {
    if (match_table->partition_idx_info->partition_field_name != NULL) {
      PIPE_MGR_FREE(match_table->partition_idx_info->partition_field_name);
    }
    PIPE_MGR_FREE(match_table->partition_idx_info);
  }
  return rc;
}
/**
 * Parses a match table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_match_table_json(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *table_cjson,
    bool virtual_device,
    bool is_alpm_internal_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *name = NULL;
  char *direction = NULL;
  int handle = 0;
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_DIRECTION, &direction);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for match table handle 0x%x and "
      "name \"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  // Allocate metadata information.
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }










  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  bool *stages_seen = PIPE_MGR_CALLOC(max_number_stages, sizeof(bool));
  if (all_stage_tables_cjson == NULL || stages_seen == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse exact match entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  // Allocate a new match table.
  pipe_mat_tbl_info_t *match_table = allocate_match_table();
  if (match_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new match table.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  int size = 0;
  int num_partitions = 1;
  int dynamic_key_mask_width = 0;
  bool uses_range = false;
  bool disable_atomic_modify = false;
  cJSON *action_data_refs_cjson = NULL;
  cJSON *meter_refs_cjson = NULL;
  cJSON *selection_refs_cjson = NULL;
  cJSON *stateful_refs_cjson = NULL;
  cJSON *statistics_refs_cjson = NULL;

  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= bf_cjson_get_bool(
      table_cjson, CTX_JSON_MATCH_TABLE_USES_RANGE, &uses_range);
  err |= bf_cjson_try_get_bool(table_cjson,
                               CTX_JSON_MATCH_TABLE_DISABLE_ATOMIC_MODIFY,
                               &disable_atomic_modify);
  err |= bf_cjson_try_get_int(table_cjson,
                              CTX_JSON_MATCH_TABLE_DYNAMIC_MATCH_KEY_MASKS,
                              &dynamic_key_mask_width);
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_ACTION_DATA_TABLE_REFS,
                             &action_data_refs_cjson);
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_STATISTICS_TABLE_REFS,
                             &statistics_refs_cjson);
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_MATCH_TABLE_METER_TABLE_REFS, &meter_refs_cjson);
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_STATEFUL_TABLE_REFS,
                             &stateful_refs_cjson);
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_SELECTION_TABLE_REFS,
                             &selection_refs_cjson);
  CHECK_ERR(err, rc, cleanup);

  match_table->name = name;
  match_table->handle = handle;
  match_table->size = size;
  match_table->num_partitions = num_partitions;
  match_table->match_key_mask_width = dynamic_key_mask_width;
  match_table->uses_range = uses_range;
  match_table->disable_atomic_modify = disable_atomic_modify;

  if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_INGRESS)) {
    match_table->direction = BF_TBL_DIR_INGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_EGRESS)) {
    match_table->direction = BF_TBL_DIR_EGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_GHOST)) {
    match_table->direction = BF_TBL_DIR_GHOST;
  } else {
    LOG_ERROR(
        "%s:%d: Table \"%s\" with handle 0x%x has invalid direction \"%s\".",
        __func__,
        __LINE__,
        name,
        handle,
        direction);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Parse match table references.
  rc = ctx_json_parse_rmt_cfg_match_references(devid,
                                               prof_id,
                                               action_data_refs_cjson,
                                               &match_table->adt_tbl_ref,
                                               &match_table->num_adt_tbl_refs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to parse action reference for match table %s",
              __func__,
              __LINE__,
              name);
    CHECK_RC(rc, cleanup);
  }
  rc = ctx_json_parse_rmt_cfg_match_references(devid,
                                               prof_id,
                                               statistics_refs_cjson,
                                               &match_table->stat_tbl_ref,
                                               &match_table->num_stat_tbl_refs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to parse counter reference for match table %s",
              __func__,
              __LINE__,
              name);
    CHECK_RC(rc, cleanup);
  }
  rc =
      ctx_json_parse_rmt_cfg_match_references(devid,
                                              prof_id,
                                              meter_refs_cjson,
                                              &match_table->meter_tbl_ref,
                                              &match_table->num_meter_tbl_refs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to parse meter reference for match table %s",
              __func__,
              __LINE__,
              name);
    CHECK_RC(rc, cleanup);
  }
  rc = ctx_json_parse_rmt_cfg_match_references(devid,
                                               prof_id,
                                               selection_refs_cjson,
                                               &match_table->sel_tbl_ref,
                                               &match_table->num_sel_tbl_refs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to parse selector reference for match table %s",
              __func__,
              __LINE__,
              name);
    CHECK_RC(rc, cleanup);
  }
  rc =
      ctx_json_parse_rmt_cfg_match_references(devid,
                                              prof_id,
                                              stateful_refs_cjson,
                                              &match_table->stful_tbl_ref,
                                              &match_table->num_stful_tbl_refs);
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR("%s:%d Failed to parse register reference for match table %s",
              __func__,
              __LINE__,
              name);
    CHECK_RC(rc, cleanup);
  }

  // Fetch all stage tables related to this table. This includes ATCAM units,
  // since those are treated as stage tables by the driver.
  int number_stage_tables = 0;
  err |= ctx_json_parse_all_match_stage_tables_for_table(
      table_cjson,
      max_number_stages * max_number_logical_tables,
      all_stage_tables_cjson,
      &number_stage_tables);
  CHECK_ERR(err, rc, cleanup);

  // For ALPM, number_stage_tables will be 0.
  int stage_number = 0;
  int log_tbl_id = 0;

  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];
    PIPE_MGR_ASSERT(stage_table_cjson != NULL);

    char *stage_table_type = NULL;
    int prsr_handle_tmp = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage_number);
    err |= bf_cjson_try_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID, &log_tbl_id);




    err |= bf_cjson_get_string(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_STAGE_TABLE_TYPE,
                               &stage_table_type);
    CHECK_ERR(err, rc, cleanup);
    bf_cjson_try_get_handle(devid,
                            prof_id,
                            stage_table_cjson,
                            CTX_JSON_PARSER_INSTANCE_HDL,
                            &prsr_handle_tmp);
    if (stage_number >= max_number_stages) {
      LOG_ERROR("%s:%d Stage number %d exceeds max number of allowed stages %d",
                __func__,
                __LINE__,
                stage_number,
                max_number_stages);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    // If this stage table is a hash action, then we must update some
    // information in the match table structure itself.
    if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_HASH_ACTION)) {
      rc |= ctx_json_parse_rmt_cfg_hash_action_spec(
          devid, prof_id, table_cjson, match_table);
      CHECK_RC(rc, cleanup);
    }

    // Actually parse the stage table.
    rc |= ctx_json_parse_rmt_cfg_stage_table_json(dev_info,
                                                  prof_id,
                                                  stage_table_cjson,
                                                  &match_table->rmt_info,
                                                  &match_table->num_rmt_info);
    CHECK_ERR(err, rc, cleanup);

    if (stage_number != -1) {
      stages_seen[stage_number] = true;
    } else if (!virtual_device) {
      // Phase0 tables will have a -1 stage number.
      PIPE_MGR_ASSERT(
          !strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_PHASE0_MATCH));
      pipe_prsr_instance_hdl_t prsr_instance_hdl;
      uint32_t log_pipe_id;
      if (prsr_handle_tmp == 0)
        prsr_instance_hdl = DEFAULT_PRSR_INSTANCE_HDL;
      else
        prsr_instance_hdl = prsr_handle_tmp;
      PIPE_BITMAP_ITER(&dev_info->profile_info[prof_id]->pipe_bmp,
                       log_pipe_id) {
        rc = pipe_mgr_set_prsr_instance_phase0_hdl(devid,
                                                   log_pipe_id,
                                                   match_table->direction,
                                                   prsr_instance_hdl,
                                                   match_table->handle);
        if (rc != PIPE_SUCCESS) {
          LOG_ERROR(
              "%s:%d Fail in adding phase0 table 0x%x information into parser "
              "instance 0x%x, dev %d",
              __func__,
              __LINE__,
              match_table->handle,
              prsr_instance_hdl,
              devid);
          goto cleanup;
        }
      }
      match_table->prsr_instance_hdl = prsr_instance_hdl;
    }
  }

  // We have this bit array to make sure we don't double count some stage.
  for (int i = 0; i < max_number_stages; ++i) {
    if (stages_seen[i]) {
      ++match_table->num_stages;
    }
  }

  // Parse the default action blacklist for this table.
  rc = ctx_json_parse_rmt_cfg_default_action_blacklist(
      devid, prof_id, table_cjson, match_table);
  CHECK_RC(rc, cleanup);

  // Parse specific fields to each match table type. This includes handling
  // ALPM cases, which are particularly special.
  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_key_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS, &match_key_cjson);
  CHECK_ERR(err, rc, cleanup);

  char *match_type = NULL;
  err = bf_cjson_get_string(match_attributes_cjson,
                            CTX_JSON_MATCH_ATTRIBUTES_MATCH_TYPE,
                            &match_type);
  CHECK_ERR(err, rc, cleanup);

  if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT)) {
    match_table->match_type = EXACT_MATCH;
    match_table->duplicate_entry_check = true;

    bool uses_dynamic_key_masks = false;
    err = bf_cjson_get_bool(match_attributes_cjson,
                            CTX_JSON_MATCH_ATTRIBUTES_USES_DYNAMIC_KEY_MASKS,
                            &uses_dynamic_key_masks);
    CHECK_ERR(err, rc, cleanup);

    match_table->dynamic_key_mask_table = uses_dynamic_key_masks;
  } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_NO_KEY) ||
             !strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_HASH_ACTION)) {
    match_table->match_type = EXACT_MATCH;
    match_table->duplicate_entry_check = true;
  } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_TERNARY)) {
    match_table->match_type = TERNARY_MATCH;
    if (is_ternary_table_lpm_or_exact(table_cjson)) {
      match_table->duplicate_entry_check = true;
    }
  } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ATCAM)) {
    match_table->match_type = ATCAM_MATCH;
    err |= bf_cjson_get_int(match_attributes_cjson,
                            CTX_JSON_MATCH_ATTRIBUTES_NUMBER_PARTITIONS,
                            &num_partitions);
    match_table->num_partitions = num_partitions;
    // parse the partition_idx info
    if (!is_alpm_internal_table) {
      rc = ctx_json_parse_rmt_cfg_partition_idx_info(
          match_attributes_cjson, match_key_cjson, match_table);
    }
    CHECK_RC(rc, cleanup);

    // Sanity check of logical table size
    for (uint32_t i = 0; i < match_table->num_rmt_info; i++) {
      rmt_tbl_info_t *rmt_info = &match_table->rmt_info[i];
      if (rmt_info->type == RMT_TBL_TYPE_ATCAM_MATCH &&
          rmt_info->num_entries <
              (match_table->num_partitions * rmt_info->num_tbl_banks *
               rmt_info->pack_format.entries_per_tbl_word)) {
        LOG_ERROR(
            "%s:%d Invalid logical table size for atcam table %s (0x%x) device "
            "%d. Received %d Expected at least %d (%d partitions, %d wide "
            "words, %d packing)",
            __func__,
            __LINE__,
            match_table->name,
            match_table->handle,
            devid,
            rmt_info->num_entries,
            (match_table->num_partitions * rmt_info->num_tbl_banks *
             rmt_info->pack_format.entries_per_tbl_word),
            match_table->num_partitions,
            rmt_info->num_tbl_banks,
            rmt_info->pack_format.entries_per_tbl_word);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
    }
  } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
    match_table->match_type = ALPM_MATCH;
    match_table->duplicate_entry_check = true;

    // Get ALPM's pre classifier and ATCAM.
    cJSON *pre_classifier_cjson = NULL;
    err |= bf_cjson_get_object(match_attributes_cjson,
                               CTX_JSON_MATCH_ATTRIBUTES_PRE_CLASSIFIER,
                               &pre_classifier_cjson);
    CHECK_ERR(err, rc, cleanup);

    // ALPM considers the first ATCAM unit as its "handle". Notice that the
    // ALPM will have all of its stage tables parsed from all ATCAM units.
    cJSON *atcam_table_cjson = NULL;
    err |= bf_cjson_get_object(match_attributes_cjson,
                               CTX_JSON_MATCH_ATTRIBUTES_ATCAM_TABLE,
                               &atcam_table_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Parse the pre classifier and the ATCAM. This will shift the pointers
    // to match tables (due to realloc's), so we save the index.
    int current_table_index = g_table_info->num_mat_tbls - 1;

    rc |= ctx_json_parse_rmt_cfg_match_table_json(
        devid, prof_id, pre_classifier_cjson, virtual_device, true);
    rc |= ctx_json_parse_rmt_cfg_match_table_json(
        devid, prof_id, atcam_table_cjson, virtual_device, true);
    CHECK_RC(rc, cleanup);

    // Get the pointers to those tables.
    int pre_classifier_table_index = current_table_index + 1;
    int atcam_table_index = current_table_index + 2;

    pipe_mat_tbl_info_t *pre_classifier_table =
        &g_table_info->mat_tbl_list[pre_classifier_table_index];
    pipe_mat_tbl_info_t *atcam_table =
        &g_table_info->mat_tbl_list[atcam_table_index];
    match_table = &g_table_info->mat_tbl_list[current_table_index];

    PIPE_MGR_ASSERT(pre_classifier_table != NULL);
    PIPE_MGR_ASSERT(atcam_table != NULL);
    PIPE_MGR_ASSERT(match_table != NULL);

    pre_classifier_table->alpm_info =
        PIPE_MGR_CALLOC(1, sizeof(alpm_mat_tbl_info_t));
    if (pre_classifier_table->alpm_info == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for metadata for ALPM table.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pre_classifier_table->default_info->p4_default = true;

    match_table->alpm_info = PIPE_MGR_CALLOC(1, sizeof(alpm_mat_tbl_info_t));
    if (match_table->alpm_info == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for ALPM table info.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    rc = ctx_json_parse_rmt_cfg_alpm_spec(devid,
                                          prof_id,
                                          match_table->alpm_info,
                                          table_cjson,
                                          pre_classifier_cjson,
                                          atcam_table_cjson,
                                          match_attributes_cjson);
    if (rc != PIPE_SUCCESS) {
      LOG_ERROR("%s:%d Failed to parse ALPM details for table 0x%x device %d",
                __func__,
                __LINE__,
                match_table->handle,
                devid);
      goto cleanup;
    }

    // Set the ATCAM's alpm_info to this ALPM table's, and the pre classifier
    // handle.
    atcam_table->alpm_info = match_table->alpm_info;
    pre_classifier_table->alpm_info->preclass_handle =
        match_table->alpm_info->preclass_handle;

    // The ALPM table needs the same RMT tables as the ATCAM. Since we have
    // already parsed the ATCAM table, we copy the pointers here. It also
    // needs
    // the same number of partitions than the ATCAM on the wrapper level.
    match_table->rmt_info = atcam_table->rmt_info;
    match_table->num_rmt_info = atcam_table->num_rmt_info;
    match_table->num_partitions = atcam_table->num_partitions;

    // Propagate the atomic modify disable flag
    pre_classifier_table->disable_atomic_modify =
        match_table->disable_atomic_modify;
    atcam_table->disable_atomic_modify = match_table->disable_atomic_modify;
  }

  /* Calculate the match-spec size */

  uint32_t num_bits = 0, num_bytes = 0;
  int curr_num_bits = 0;
  int curr_num_bytes = 0;
  unsigned i = 0, j = 0;
  uint8_t *mask_ptr;
  uint16_t key_field_mask_len = 0;
  bool mask_defined = false;
  {
    uint32_t num_match_fields = GETARRSZ(match_key_cjson);
    int pos_to_bitwidth[num_match_fields];
    PIPE_MGR_MEMSET(pos_to_bitwidth, 0, num_match_fields * sizeof(int));
    for (i = 0; i < num_match_fields; i++) {
      cJSON *match_key_item = GETARRITEM(match_key_cjson, i);
      int pos = 0;

      bf_cjson_get_int(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_POSITION, &pos);
      bf_cjson_get_int(match_key_item,
                       CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL,
                       &curr_num_bits);
      pos_to_bitwidth[pos] = curr_num_bits;

      char *mask_string = NULL;
      bf_cjson_try_get_string(
          match_key_item, CTX_JSON_MATCH_KEY_FIELDS_MASK, &mask_string);

      // If mask_string not NULL, there is at least one mask defined
      if (mask_string) {
        mask_defined = true;
      }
    }
    for (i = 0; i < num_match_fields; i++) {
      // Not used positions should have value  of 0.
      num_bits += pos_to_bitwidth[i];
      num_bytes += (pos_to_bitwidth[i] + 7) / 8;
    }
    // Allocate memory for table mask, if needed
    if (mask_defined) {
      match_table->tbl_global_key_mask_bits = PIPE_MGR_MALLOC(num_bytes);
      if (!match_table->tbl_global_key_mask_bits) {
        LOG_ERROR("%s:%d: Could not allocate memory for table mask.",
                  __func__,
                  __LINE__);
        match_table->tbl_global_key_mask_len = 0;
        match_table->tbl_global_key_mask_valid = false;
        rc = PIPE_NO_SYS_RESOURCES;
        goto cleanup;
      }
      match_table->tbl_global_key_mask_valid = true;
      match_table->tbl_global_key_mask_len = num_bytes;
      mask_ptr = match_table->tbl_global_key_mask_bits;

      // Pre-initialize mask with 0xFF
      PIPE_MGR_MEMSET(mask_ptr, 0xFF, num_bytes);

      curr_num_bits = 0;
      curr_num_bytes = 0;

      // Second pass, generate mask
      for (i = 0; i < num_match_fields; i++) {
        cJSON *match_key_item = GETARRITEM(match_key_cjson, i);
        char *field_name = NULL;
        bf_cjson_get_string(
            match_key_item, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);

        bf_cjson_get_int(match_key_item,
                         CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL,
                         &curr_num_bits);

        curr_num_bytes = (curr_num_bits + 7) / 8;
        char *mask_string = NULL;

        bf_cjson_try_get_string(
            match_key_item, CTX_JSON_MATCH_KEY_FIELDS_MASK, &mask_string);

        // If mask_string not NULL, copy the mask in appropriate place,
        // otherwise leave 0xFFs and skip the key field
        if (mask_string) {
          // convert the string to stream of bytes
          // round up to full bytes. " - 2" to remove "0x", "+ 1" to round up,
          // e.g. 0x3FF has 2 bytes, not 1
          key_field_mask_len = (strlen(mask_string) - 2 + 1) / 2;

          if (key_field_mask_len > curr_num_bytes) {
            mask_ptr += curr_num_bytes;
            LOG_ERROR(
                "%s:%d: Invalid mask length (%d) for field %s in match table "
                "%s.",
                __func__,
                __LINE__,
                key_field_mask_len,
                field_name,
                match_table->name);
            match_table->tbl_global_key_mask_len = 0;
            match_table->tbl_global_key_mask_valid = false;
            PIPE_MGR_FREE(match_table->tbl_global_key_mask_bits);
            rc = PIPE_INVALID_ARG;
            goto cleanup;
          }

          PIPE_MGR_MEMSET(mask_ptr, 0, curr_num_bytes);
          match_table->tbl_global_key_mask_valid = true;

          // advance the pointer if the mask doesn't contain leading zeros
          // and is shorter than the field length in bytes
          if (key_field_mask_len < curr_num_bytes)
            mask_ptr += (curr_num_bytes - key_field_mask_len);
          ctx_json_hex_to_stream(mask_string, mask_ptr, key_field_mask_len);
          // The mask is stored at match_table->tbl_global_key_mask_bits in
          // the order: MSB @ match_table->tbl_global_key_mask_bits[0]
          mask_ptr += key_field_mask_len;
        } else
          mask_ptr += curr_num_bytes;
      }
    }
  }

  match_table->num_match_bits = num_bits;
  match_table->num_match_bytes = num_bytes;

  /* Populate action fn hdl info */
  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  match_table->num_actions = (uint32_t)GETARRSZ(actions_cjson);
  match_table->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
      match_table->num_actions, sizeof(pipe_act_fn_info_t));
  if (match_table->act_fn_hdl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }
  for (i = 0; i < match_table->num_actions; i++) {
    cJSON *actions_item = GETARRITEM(actions_cjson, i);
    pipe_act_fn_hdl_t act_fn_hdl = 0;
    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               actions_item,
                               CTX_JSON_ACTION_HANDLE,
                               (int *)&act_fn_hdl);
    CHECK_ERR(err, rc, cleanup);

    cJSON *act_param = NULL;
    err |= bf_cjson_get_object(actions_item, "p4_parameters", &act_param);
    CHECK_ERR(err, rc, cleanup);
    int curr_bit_width = 0;
    num_bits = 0;
    num_bytes = 0;
    uint32_t num_act_params = (uint32_t)GETARRSZ(act_param);
    for (j = 0; j < num_act_params; j++) {
      cJSON *act_param_item = GETARRITEM(act_param, j);
      err |= bf_cjson_get_int(act_param_item, "bit_width", &curr_bit_width);
      CHECK_ERR(err, rc, cleanup);

      num_bits += curr_bit_width;
      num_bytes += (curr_bit_width + 7) / 8;
    }
    match_table->act_fn_hdl_info[i].act_fn_hdl = act_fn_hdl;
    match_table->act_fn_hdl_info[i].num_bits = num_bits;
    match_table->act_fn_hdl_info[i].num_bytes = num_bytes;

    /* Schema version 1.11.0 or later has direct resource info. */
    if (dev_info->profile_info[prof_id]->schema_version[0] > 1 ||
        (dev_info->profile_info[prof_id]->schema_version[0] == 1 &&
         dev_info->profile_info[prof_id]->schema_version[1] >= 11)) {
      cJSON *dir_rsrcs = NULL, *dir_rsrc = NULL;
      err = bf_cjson_get_object(
          actions_item, CTX_JSON_ACTION_DIRECT_RESOURCES, &dir_rsrcs);
      CHECK_ERR(err, rc, cleanup);
      CTX_JSON_FOR_EACH(dir_rsrc, dir_rsrcs) {
        int res_hdl = -1;
        err = bf_cjson_get_handle(devid,
                                  prof_id,
                                  dir_rsrc,
                                  CTX_JSON_DIRECT_RESOURCE_HANDLE,
                                  &res_hdl);
        CHECK_ERR(err, rc, cleanup);
        if (PIPE_HDL_TYPE_STAT_TBL == PIPE_GET_HDL_TYPE(res_hdl))
          match_table->act_fn_hdl_info[i].dir_stat_hdl = res_hdl;
        if (PIPE_HDL_TYPE_METER_TBL == PIPE_GET_HDL_TYPE(res_hdl))
          match_table->act_fn_hdl_info[i].dir_meter_hdl = res_hdl;
        if (PIPE_HDL_TYPE_STFUL_TBL == PIPE_GET_HDL_TYPE(res_hdl))
          match_table->act_fn_hdl_info[i].dir_stful_hdl = res_hdl;
      }
    }
  }

  // Check for keyless tables
  cJSON *match_key_fields_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                             &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  int number_match_parameters = cJSON_GetArraySize(match_key_fields_cjson);
  if (number_match_parameters == 0) {
    match_table->keyless_info = PIPE_MGR_CALLOC(1, sizeof(pipe_keyless_info_t));
    if (match_table->keyless_info == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for keyless information.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    match_table->keyless_info->stage_id = stage_number;
    match_table->keyless_info->log_tbl_id = log_tbl_id;

  }

  /* Read the initial (P4 specified) default entry. */
  int default_action_handle = 0;
  err = bf_cjson_try_get_handle(devid,
                                prof_id,
                                table_cjson,
                                CTX_JSON_MATCH_TABLE_DEFAULT_ACTION_HANDLE,
                                &default_action_handle);
  CHECK_ERR(err, rc, cleanup);

  /* Validate that Brig will always have a default action for non-phase0
   * tables.
   */
  if (dev_info->profile_info[prof_id]->compiler_version[0] >= 6 &&
      strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_PHASE0) &&
      default_action_handle == 0) {
    LOG_ERROR(
        "%s:%d Invalid or missing default action handle for match table 0x%x",
        __func__,
        __LINE__,
        handle);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  /* Cache the def action function handle */
  if (PIPE_GET_HDL_VAL(default_action_handle) != 0) {
    rc = ctx_json_parse_rmt_cfg_default_spec(
        dev_info, prof_id, table_cjson, match_table, default_action_handle);
    CHECK_RC(rc, cleanup);
  }

  // Check for static entries. If so, we parse them.
  cJSON *static_entries_cjson = NULL;
  bf_cjson_try_get_object(
      table_cjson, CTX_JSON_MATCH_TABLE_STATIC_ENTRIES, &static_entries_cjson);
  if (static_entries_cjson != NULL) {
    rc = ctx_json_parse_rmt_cfg_static_entries(
        dev_info, prof_id, table_cjson, static_entries_cjson, match_table);
    if (rc) {
      LOG_ERROR(
          "Failed to parse static entries for dev %d profile %d table %s "
          "(0x%x)",
          dev_info->dev_id,
          prof_id,
          match_table->name,
          match_table->handle);
    }
    CHECK_RC(rc, cleanup);
    // Turn on duplicate checking for static tables for iteration purposes
    if (match_table->static_entries) {
      match_table->duplicate_entry_check = true;
    }
  }

  PIPE_MGR_FREE(all_stage_tables_cjson);
  PIPE_MGR_FREE(stages_seen);
  return rc;

cleanup:
  if (stages_seen != NULL) {
    PIPE_MGR_FREE(stages_seen);
  }
  if (all_stage_tables_cjson != NULL) {
    PIPE_MGR_FREE(all_stage_tables_cjson);
  }
cleanup_no_free:
  if (name) {
    LOG_ERROR(
        "Dev %d Failed to parse profile %d table %s", devid, prof_id, name);
  }
  return rc;
}

/**
 * Parses a action data table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_action_data_table_json(
    bf_dev_id_t devid, profile_id_t prof_id, cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *name = NULL;
  char *direction = NULL;
  int handle = 0;
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_DIRECTION, &direction);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for action data table handle 0x%x "
      "and "
      "name \"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  // Allocate metadata information.
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    if (name) PIPE_MGR_FREE(name);
    return PIPE_UNEXPECTED;
  }

  int max_number_stages = dev_info->num_active_mau;

  bool *stages_seen = PIPE_MGR_CALLOC(max_number_stages, sizeof(bool));
  if (stages_seen == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for metadata.", __func__, __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    if (name) PIPE_MGR_FREE(name);
    name = NULL;
    goto cleanup_no_free;
  }

  int size = 0;
  pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;

  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= ctx_json_parse_rmt_cfg_reference_type(table_cjson, &ref_type);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_tables_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = stage_tables_cjson->child;
  if (stage_table_cjson == NULL) {
    LOG_DBG(
        "%s:%d: Action data table with handle 0x%x with no stage tables. "
        "Continuing.",
        __func__,
        __LINE__,
        handle);
    if (name) PIPE_MGR_FREE(name);
    if (stages_seen) PIPE_MGR_FREE(stages_seen);
    return rc;
  }

  cJSON *mra_cjson = NULL;
  bf_cjson_try_get_object(stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                          &mra_cjson);

  if (mra_cjson == NULL) {
    LOG_DBG(
        "%s:%d: Action data table with handle 0x%x with size 0 found. "
        "Continuing.",
        __func__,
        __LINE__,
        handle);
    if (name) PIPE_MGR_FREE(name);
    if (stages_seen) PIPE_MGR_FREE(stages_seen);
    return rc;
  }

  // Allocate a new action_data table.
  pipe_adt_tbl_info_t *action_data_table = allocate_action_data_table();
  if (action_data_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new action data table.",
              __func__,
              __LINE__);
    if (name) PIPE_MGR_FREE(name);
    if (stages_seen) PIPE_MGR_FREE(stages_seen);
    return PIPE_NO_SYS_RESOURCES;
  }

  action_data_table->name = name;
  action_data_table->handle = handle;
  action_data_table->size = size;
  action_data_table->ref_type = ref_type;

  if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_INGRESS)) {
    action_data_table->direction = BF_TBL_DIR_INGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_EGRESS)) {
    action_data_table->direction = BF_TBL_DIR_EGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_GHOST)) {
    action_data_table->direction = BF_TBL_DIR_GHOST;
  } else {
    LOG_ERROR(
        "%s:%d: Action data table \"%s\" with handle 0x%x has invalid "
        "direction \"%s\".",
        __func__,
        __LINE__,
        name,
        handle,
        direction);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    rc |= ctx_json_parse_rmt_cfg_stage_table_json(
        dev_info,
        prof_id,
        stage_table_cjson,
        &action_data_table->rmt_info,
        &action_data_table->num_rmt_info);
    CHECK_RC(rc, cleanup);

    int stage_number = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage_number);
    CHECK_ERR(err, rc, cleanup);

    stages_seen[stage_number] = true;
  }

  // Check how many stages were used.
  for (int i = 0; i < max_number_stages; ++i) {
    if (stages_seen[i]) {
      ++action_data_table->num_stages;
    }
  }

  /* Parse action function info */
  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);
  unsigned i = 0;
  action_data_table->num_actions = (uint32_t)GETARRSZ(actions_cjson);
  action_data_table->act_fn_hdl_info = (pipe_act_fn_info_t *)PIPE_MGR_CALLOC(
      action_data_table->num_actions, sizeof(pipe_act_fn_info_t));
  if (action_data_table->act_fn_hdl_info == NULL) {
    LOG_ERROR("%s:%d Malloc failure", __func__, __LINE__);
    if (stages_seen) PIPE_MGR_FREE(stages_seen);
    if (name) PIPE_MGR_FREE(name);
    return PIPE_NO_SYS_RESOURCES;
  }
  for (i = 0; i < action_data_table->num_actions; i++) {
    cJSON *actions_item = GETARRITEM(actions_cjson, i);

    pipe_act_fn_hdl_t act_fn_hdl = 0;
    bf_cjson_get_int(actions_item, "handle", (int *)&act_fn_hdl);
    cJSON *act_param = NULL;
    bf_cjson_get_object(actions_item, "p4_parameters", &act_param);
    unsigned j = 0;
    uint32_t num_bits = 0;
    int curr_bit_width = 0;
    uint32_t num_bytes = 0;
    uint32_t num_act_params = (uint32_t)GETARRSZ(act_param);
    for (j = 0; j < num_act_params; j++) {
      cJSON *act_param_item = GETARRITEM(act_param, j);
      bf_cjson_get_int(act_param_item, "bit_width", &curr_bit_width);
      num_bits += curr_bit_width;
      num_bytes += (curr_bit_width + 7) / 8;
    }
    action_data_table->act_fn_hdl_info[i].act_fn_hdl = act_fn_hdl;
    action_data_table->act_fn_hdl_info[i].num_bits = num_bits;
    action_data_table->act_fn_hdl_info[i].num_bytes = num_bytes;
  }

  PIPE_MGR_FREE(stages_seen);
  return rc;

cleanup:
  PIPE_MGR_FREE(stages_seen);
cleanup_no_free:
  if (name) {
    LOG_ERROR(
        "Dev %d Failed to parse profile %d table %s", devid, prof_id, name);
  }
  return rc;
}

/**
 * Parses a selection table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_selection_table_json(
    bf_dev_id_t devid, profile_id_t prof_id, cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  char *name = NULL;
  int handle = 0;
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for selection table handle 0x%x and "
      "name \"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  // Allocate a new selection table.
  pipe_select_tbl_info_t *selection_table = allocate_selection_table();
  if (selection_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new selection table.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  char *selection_type = NULL;
  int size = 0;
  pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;
  int max_group_size = 0;
  int bound_to_stateful_table_handle = 0;
  int bound_to_action_data_table_handle = 0;

  err |= bf_cjson_get_string(
      table_cjson, CTX_JSON_SELECTION_TABLE_SELECTION_TYPE, &selection_type);
  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= ctx_json_parse_rmt_cfg_reference_type(table_cjson, &ref_type);
  err |= bf_cjson_get_int(table_cjson,
                          CTX_JSON_SELECTION_TABLE_MAX_PORT_POOL_SIZE,
                          &max_group_size);
  err |= bf_cjson_try_get_handle(
      devid,
      prof_id,
      table_cjson,
      CTX_JSON_SELECTION_TABLE_BOUND_TO_STATEFUL_TABLE_HANDLE,
      &bound_to_stateful_table_handle);
  err |= bf_cjson_get_handle(
      devid,
      prof_id,
      table_cjson,
      CTX_JSON_SELECTION_TABLE_BOUND_TO_ACTION_DATA_TABLE_HANDLE,
      &bound_to_action_data_table_handle);
  CHECK_ERR(err, rc, cleanup);

  selection_table->name = name;
  selection_table->handle = handle;
  selection_table->max_groups = size;
  selection_table->ref_type = ref_type;
  selection_table->max_group_size = max_group_size;
  selection_table->stful_tbl_hdl = bound_to_stateful_table_handle;
  selection_table->adt_tbl_hdl = bound_to_action_data_table_handle;

  if (!strcmp(selection_type, "resilient")) {
    selection_table->mode = RESILIENT;
  } else if (!strcmp(selection_type, "fair")) {
    selection_table->mode = FAIR;
  } else {
    LOG_ERROR("%s:%d: Invalid selection type type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              selection_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Parse stage_tables field.
  cJSON *stage_tables_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  if (GETARRSZ(stage_tables_cjson) > 0) {
    stage_table_cjson = GETARRITEM(stage_tables_cjson, 0);
    bool sps_enable = true;
    err = bf_cjson_try_get_bool(stage_table_cjson,
                                CTX_JSON_SELECTION_TABLE_SPS_SCRAMBLE_ENABLE,
                                &sps_enable);
    CHECK_ERR(err, rc, cleanup);
    selection_table->sps_enable = sps_enable;
  }

  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    rc |=
        ctx_json_parse_rmt_cfg_stage_table_json(dev_info,
                                                prof_id,
                                                stage_table_cjson,
                                                &selection_table->rmt_info,
                                                &selection_table->num_rmt_info);
    CHECK_RC(rc, cleanup);
    ++selection_table->num_stages;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses a statistics table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_statistics_table_json(
    bf_dev_id_t devid, profile_id_t prof_id, cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  char *name = NULL;
  int handle = 0;
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for statistics table handle 0x%x and "
      "name \"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  // Allocate a new statistics table.
  pipe_stat_tbl_info_t *statistics_table = allocate_statistics_table();
  if (statistics_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new statistics table.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  char *statistics_type = NULL;
  int size = 0;
  pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;
  bool enable_pfe = false;
  int pfe_bit_position = 0;
  int packet_counter_resolution = 0;
  int byte_counter_resolution = 0;

  err |= bf_cjson_get_string(
      table_cjson, CTX_JSON_STATISTICS_TABLE_STATISTICS_TYPE, &statistics_type);
  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= ctx_json_parse_rmt_cfg_reference_type(table_cjson, &ref_type);
  err |= bf_cjson_get_bool(
      table_cjson, CTX_JSON_STATISTICS_TABLE_ENABLE_PFE, &enable_pfe);
  err |= bf_cjson_get_int(table_cjson,
                          CTX_JSON_STATISTICS_TABLE_ENABLE_PFE_BIT_POSITION,
                          &pfe_bit_position);
  err |= bf_cjson_get_int(table_cjson,
                          CTX_JSON_STATISTICS_TABLE_PACKET_COUNTER_RESOLUTION,
                          &packet_counter_resolution);
  err |= bf_cjson_get_int(table_cjson,
                          CTX_JSON_STATISTICS_TABLE_BYTE_COUNTER_RESOLUTION,
                          &byte_counter_resolution);
  CHECK_ERR(err, rc, cleanup);

  statistics_table->name = name;
  statistics_table->handle = handle;
  statistics_table->size = size;
  statistics_table->ref_type = ref_type;
  statistics_table->enable_per_flow_enable = enable_pfe;
  statistics_table->per_flow_enable_bit_position = pfe_bit_position;
  statistics_table->packet_counter_resolution = packet_counter_resolution;
  statistics_table->byte_counter_resolution = byte_counter_resolution;

  if (packet_counter_resolution != 0 && packet_counter_resolution != 64) {
    statistics_table->lrt_enabled = true;
  }
  if (byte_counter_resolution != 0 && byte_counter_resolution != 64) {
    statistics_table->lrt_enabled = true;
  }

  if (!strcmp(statistics_type, "packets_and_bytes")) {
    statistics_table->stat_type = PACKET_AND_BYTE_COUNT;
  } else if (!strcmp(statistics_type, "packets")) {
    statistics_table->stat_type = PACKET_COUNT;
  } else if (!strcmp(statistics_type, "bytes")) {
    statistics_table->stat_type = BYTE_COUNT;
  } else {
    LOG_ERROR("%s:%d: Invalid statistics type type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              statistics_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Parse stage_tables field.
  cJSON *stage_tables_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    rc |= ctx_json_parse_rmt_cfg_stage_table_json(
        dev_info,
        prof_id,
        stage_table_cjson,
        &statistics_table->rmt_info,
        &statistics_table->num_rmt_info);
    CHECK_RC(rc, cleanup);
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Reverse calculate max burst size from
 * pipe_mgr_meter_drv_get_burstsize_params
 *
 * @param granularity byte or packet style meter
 * @return max burst size
 */
static inline uint64_t pipe_mgr_meter_get_max_burst_size(
    pipe_meter_granularity_e granularity) {
  uint32_t max_mantissa = 255;
  uint32_t max_exponent = 31;
  uint64_t max_rate = max_mantissa * pow(2, max_exponent);
  if (granularity == PIPE_METER_GRANULARITY_BYTES) {
    max_rate = (max_rate * 8) / 1000;
  }
  return max_rate;
}

/**
 * Reverse calculate max rate from
 * pipe_mgr_meter_drv_get_rate_params
 *
 * @param dev_id device id to get clock speed
 * @param granularity byte or packet style meter
 * @return max rate
 */
static inline uint64_t pipe_mgr_meter_get_max_rate(
    bf_dev_id_t dev_id, pipe_meter_granularity_e granularity) {
  uint64_t clock_speed = pipe_mgr_get_sp_clock_speed(dev_id);
  uint32_t max_mantissa = 511;
  float per_second = (max_mantissa - 1) * clock_speed;
  uint64_t max_rate = ~0ull;
  if (granularity == PIPE_METER_GRANULARITY_BYTES) {
    max_rate = (per_second * 8) / 1000;
  } else {
    max_rate = (uint64_t)per_second;
  }
  return max_rate;
}

/**
 * Parses a meter table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_meter_table_json(
    bf_dev_id_t devid, profile_id_t prof_id, cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  char *name = NULL;
  int handle = 0;
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  CHECK_ERR(err, rc, cleanup);

  char *direction_str = NULL;
  pipe_tbl_dir_t direction = BF_TBL_DIR_INGRESS;
  err = bf_cjson_get_string(
      table_cjson, CTX_JSON_TABLE_DIRECTION, &direction_str);
  CHECK_ERR(err, rc, cleanup);
  if (!strcmp(direction_str, CTX_JSON_TABLE_DIRECTION_INGRESS)) {
    direction = BF_TBL_DIR_INGRESS;
  } else if (!strcmp(direction_str, CTX_JSON_TABLE_DIRECTION_EGRESS)) {
    direction = BF_TBL_DIR_EGRESS;
  } else if (!strcmp(direction_str, CTX_JSON_TABLE_DIRECTION_GHOST)) {
    direction = BF_TBL_DIR_GHOST;
  } else {
    LOG_ERROR(
        "%s:%d: Table \"%s\" with handle 0x%x has invalid direction \"%s\".",
        __func__,
        __LINE__,
        name,
        handle,
        direction_str);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for meter table handle 0x%x and name "
      "\"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  // Allocate a new meter table.
  pipe_meter_tbl_info_t *meter_table = allocate_meter_table();
  if (meter_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new meter table.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  char *meter_type = NULL;
  char *meter_granularity = NULL;
  int size = 0;
  pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;
  char *pre_color_field_name = NULL;
  bool enable_pfe = false;
  int pfe_bit_position = 0;
  bool enable_color_aware_pfe = false;
  int color_aware_pfe_address_type_bit_position = 0;

  err |= bf_cjson_get_string(
      table_cjson, CTX_JSON_METER_TABLE_METER_TYPE, &meter_type);
  err |= bf_cjson_get_string(
      table_cjson, CTX_JSON_METER_TABLE_METER_GRANULARITY, &meter_granularity);
  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= ctx_json_parse_rmt_cfg_reference_type(table_cjson, &ref_type);
  err |= bf_cjson_try_get_string(table_cjson,
                                 CTX_JSON_METER_TABLE_PRE_COLOR_FIELD_NAME,
                                 &pre_color_field_name);
  err |= bf_cjson_get_bool(
      table_cjson, CTX_JSON_METER_TABLE_ENABLE_PFE, &enable_pfe);
  err |= bf_cjson_get_int(table_cjson,
                          CTX_JSON_METER_TABLE_ENABLE_PFE_BIT_POSITION,
                          &pfe_bit_position);
  err |= bf_cjson_get_bool(table_cjson,
                           CTX_JSON_METER_TABLE_ENABLE_COLOR_AWARE_PFE,
                           &enable_color_aware_pfe);
  err |= bf_cjson_get_int(
      table_cjson,
      CTX_JSON_METER_TABLE_COLOR_AWARE_PFE_ADDRESS_TYPE_BIT_POSITION,
      &color_aware_pfe_address_type_bit_position);
  CHECK_ERR(err, rc, cleanup);

  meter_table->name = name;
  meter_table->direction = direction;
  meter_table->handle = handle;
  meter_table->size = size;
  meter_table->ref_type = ref_type;
  meter_table->enable_color_aware = (pre_color_field_name != NULL);
  meter_table->enable_per_flow_enable = enable_pfe;
  meter_table->per_flow_enable_bit_position = pfe_bit_position;
  meter_table->enable_color_aware_per_flow_enable = enable_color_aware_pfe;
  meter_table->color_aware_per_flow_enable_address_type_bit_position =
      color_aware_pfe_address_type_bit_position;

  if (!strcmp(meter_type, CTX_JSON_METER_TABLE_METER_TYPE_STANDARD)) {
    meter_table->meter_type = PIPE_METER_TYPE_STANDARD;
  } else if (!strcmp(meter_type, "lpf")) {
    meter_table->meter_type = PIPE_METER_TYPE_LPF;
  } else if (!strcmp(meter_type, "red")) {
    meter_table->meter_type = PIPE_METER_TYPE_WRED;
  } else {
    LOG_ERROR("%s:%d: Invalid meter type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              meter_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  if (!strcmp(meter_granularity, "bytes")) {
    meter_table->meter_granularity = PIPE_METER_GRANULARITY_BYTES;
  } else if (!strcmp(meter_granularity, "packets")) {
    meter_table->meter_granularity = PIPE_METER_GRANULARITY_PACKETS;
  } else {
    LOG_ERROR("%s:%d: Invalid meter granularity type in ContextJSON: %s.\n",
              __func__,
              __LINE__,
              meter_granularity);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  meter_table->max_rate =
      pipe_mgr_meter_get_max_rate(devid, meter_table->meter_granularity);
  meter_table->max_burst_size =
      pipe_mgr_meter_get_max_burst_size(meter_table->meter_granularity);

  // Parse stage_tables field.
  cJSON *stage_tables_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    rc |= ctx_json_parse_rmt_cfg_stage_table_json(dev_info,
                                                  prof_id,
                                                  stage_table_cjson,
                                                  &meter_table->rmt_info,
                                                  &meter_table->num_rmt_info);
    CHECK_RC(rc, cleanup);
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses a stateful table RMT configuration from the cJSON structure.
 *
 * @param table_cjson The cJSON structure corresponding to this table.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_stateful_table_json(
    bf_dev_id_t devid, profile_id_t prof_id, cJSON *table_cjson) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  // Allocate a new stateful table.
  pipe_stful_tbl_info_t *stateful_table = allocate_stateful_table();
  if (stateful_table == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for a new stateful table.",
              __func__,
              __LINE__);
    return PIPE_NO_SYS_RESOURCES;
  }

  int handle = 0;
  char *name = NULL;
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &handle);
  err |= bf_cjson_get_string_dup(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing RMT configuration for stateful table handle 0x%x and "
      "name \"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  int size = 0;
  pipe_tbl_ref_type_t ref_type = PIPE_TBL_REF_TYPE_INVALID;
  bool dual_width_mode = 0;
  int alu_width = 0;
  int set_instr_adjust_total = 0;
  int set_instr = 0;
  int clr_instr_adjust_total = 0;
  int clr_instr = 0;
  double initial_value_lo = 0;
  double initial_value_hi = 0;
  int bound_to_selection_table_handle = 0;
  int cntr_index = 0;
  char *table_type = NULL;
  char *fifo_direction = NULL;
  pipe_stful_register_param_t *reg_params = NULL;
  int num_reg_params = 0;

  err |= bf_cjson_get_int(table_cjson, CTX_JSON_TABLE_SIZE, &size);
  err |= ctx_json_parse_rmt_cfg_reference_type(table_cjson, &ref_type);
  err |= bf_cjson_get_bool(
      table_cjson, CTX_JSON_TABLE_STATEFUL_DUAL_WIDTH_MODE, &dual_width_mode);
  err |= bf_cjson_get_int(
      table_cjson, CTX_JSON_TABLE_STATEFUL_ALU_WIDTH, &alu_width);
  bf_cjson_try_get_int(table_cjson,
                       CTX_JSON_TABLE_STATEFUL_SET_INSTR_ADJUST_TOTAL,
                       &set_instr_adjust_total);
  bf_cjson_try_get_int(
      table_cjson, CTX_JSON_TABLE_STATEFUL_SET_INSTR, &set_instr);
  bf_cjson_try_get_int(table_cjson,
                       CTX_JSON_TABLE_STATEFUL_CLR_INSTR_ADJUST_TOTAL,
                       &clr_instr_adjust_total);
  bf_cjson_try_get_int(
      table_cjson, CTX_JSON_TABLE_STATEFUL_CLR_INSTR, &clr_instr);
  bf_cjson_try_get_double(
      table_cjson, CTX_JSON_TABLE_STATEFUL_INITIAL_VALUE_LO, &initial_value_lo);
  bf_cjson_try_get_double(
      table_cjson, CTX_JSON_TABLE_STATEFUL_INITIAL_VALUE_HI, &initial_value_hi);
  err |= bf_cjson_try_get_handle(
      devid,
      prof_id,
      table_cjson,
      CTX_JSON_TABLE_STATEFUL_BOUND_TO_SELECTION_TABLE_HANDLE,
      &bound_to_selection_table_handle);
  bf_cjson_try_get_string(
      table_cjson, CTX_JSON_TABLE_STATEFUL_TYPE, &table_type);
  if (table_type && !strcmp(table_type, "fifo")) {
    stateful_table->is_fifo = true;
    bf_cjson_try_get_string(
        table_cjson, CTX_JSON_TABLE_STATEFUL_DIRECTION, &fifo_direction);
    if (fifo_direction && !strcmp(fifo_direction, "in")) {
      stateful_table->fifo_can_cpu_push = false;
      stateful_table->fifo_can_cpu_pop = true;
    } else if (fifo_direction && !strcmp(fifo_direction, "out")) {
      stateful_table->fifo_can_cpu_push = true;
      stateful_table->fifo_can_cpu_pop = false;
    } else if (fifo_direction && !strcmp(fifo_direction, "inout")) {
      stateful_table->fifo_can_cpu_push = false;
      stateful_table->fifo_can_cpu_pop = false;
    } else {
      LOG_ERROR("Table %s (0x%x) has FIFO type but no direction", name, handle);
      rc = PIPE_UNEXPECTED;
      goto cleanup_no_free;
    }
    err |= bf_cjson_get_int(
        table_cjson, CTX_JSON_TABLE_STATEFUL_CNTR_INDEX, &cntr_index);
    CHECK_ERR(err, rc, cleanup_no_free);
  }

  CHECK_ERR(err, rc, cleanup_no_free);

  stateful_table->name = name;
  stateful_table->handle = handle;
  stateful_table->size = size;
  stateful_table->ref_type = ref_type;
  stateful_table->dbl_width = dual_width_mode;
  stateful_table->width = alu_width;
  if (alu_width == 128) {
    /* 128b stateful tables are represented as 64x2. */
    stateful_table->dbl_width = true;
    stateful_table->width = 64;
  }
  stateful_table->set_instr_at = set_instr_adjust_total;
  stateful_table->set_instr = set_instr;
  stateful_table->clr_instr_at = clr_instr_adjust_total;
  stateful_table->clr_instr = clr_instr;
  stateful_table->initial_val_lo = (uint32_t)initial_value_lo;
  stateful_table->initial_val_hi = (uint32_t)initial_value_hi;
  stateful_table->sel_tbl_hdl = bound_to_selection_table_handle;
  stateful_table->cntr_index = cntr_index;

  // Parse action_to_stateful_instruction_slot fields.
  cJSON *action_to_stateful_slot_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson,
      CTX_JSON_STATEFUL_TABLE_ACTION_TO_STATEFUL_INSTRUCTION_SLOT,
      &action_to_stateful_slot_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  int num_actions = cJSON_GetArraySize(action_to_stateful_slot_cjson);
  pipe_stful_action_instr_t *actions =
      PIPE_MGR_CALLOC(num_actions, sizeof(pipe_stful_action_instr_t));

  if (actions == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for stateful table's actions.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  cJSON *item = NULL;
  int arr_index = 0;
  CTX_JSON_FOR_EACH(item, action_to_stateful_slot_cjson) {
    int instruction_number = 0;
    int action_handle = 0;

    err |= bf_cjson_get_int(
        item,
        CTX_JSON_ACTION_TO_STATEFUL_INSTRUCTION_SLOT_INSTRUCTION_SLOT,
        &instruction_number);
    err |= bf_cjson_get_handle(
        devid,
        prof_id,
        item,
        CTX_JSON_ACTION_TO_STATEFUL_INSTRUCTION_SLOT_ACTION_HANDLE,
        &action_handle);
    CHECK_ERR(err, rc, cleanup);

    actions[arr_index].instr_number = instruction_number;
    actions[arr_index].act_hdl = action_handle;
    ++arr_index;
  }
  stateful_table->actions = actions;
  stateful_table->num_actions = arr_index;

  // Parser register_params fields
  cJSON *reg_params_cjson = NULL;
  bf_cjson_try_get_object(
      table_cjson, CTX_JSON_TABLE_STATEFUL_REGISTER_PARAMS, &reg_params_cjson);
  if (reg_params_cjson != NULL) {
    num_reg_params = cJSON_GetArraySize(reg_params_cjson);
    reg_params =
        PIPE_MGR_CALLOC(num_reg_params, sizeof(pipe_stful_register_param_t));

    if (reg_params == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for stateful table's register "
          "params.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    item = NULL;
    arr_index = 0;
    CTX_JSON_FOR_EACH(item, reg_params_cjson) {
      int reg_file_index = 0;
      int init_value = 0;
      int rphandle = 0;
      char *rpname = NULL;

      err = bf_cjson_get_int(
          item, CTX_JSON_REGISTER_PARAM_INDEX, &reg_file_index);
      CHECK_ERR(err, rc, cleanup);

      err = bf_cjson_get_int(
          item, CTX_JSON_REGISTER_PARAM_INIT_VALUE, &init_value);
      CHECK_ERR(err, rc, cleanup);

      /* Nested handle, should not have profile/pipe id set */
      err = bf_cjson_get_int(item, CTX_JSON_REGISTER_PARAM_HANDLE, &rphandle);
      CHECK_ERR(err, rc, cleanup);

      err =
          bf_cjson_get_string_dup(item, CTX_JSON_REGISTER_PARAM_NAME, &rpname);
      CHECK_ERR(err, rc, cleanup);

      reg_params[arr_index].reg_file_index = reg_file_index;
      reg_params[arr_index].init_value = init_value;
      reg_params[arr_index].handle = rphandle;
      reg_params[arr_index].name = rpname;
      ++arr_index;
    }
    stateful_table->reg_params = reg_params;
    stateful_table->num_reg_params = arr_index;
  }

  // Parse stage_tables fields.
  cJSON *stage_tables_cjson = NULL;
  err = bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    rc |=
        ctx_json_parse_rmt_cfg_stage_table_json(dev_info,
                                                prof_id,
                                                stage_table_cjson,
                                                &stateful_table->rmt_info,
                                                &stateful_table->num_rmt_info);
    CHECK_RC(rc, cleanup);
  }
  return rc;

cleanup:
  if (actions != NULL) {
    PIPE_MGR_FREE(actions);
  }
  if (reg_params != NULL) {
    for (int i = 0; i < num_reg_params; i++) {
      if (reg_params[i].name) {
        PIPE_MGR_FREE(reg_params[i].name);
      }
    }
    PIPE_MGR_FREE(reg_params);
  }

cleanup_no_free:
  return rc;
}

static pipe_status_t ctx_json_check_references() {
  pipe_mat_tbl_info_t *mat_tbl = NULL;
  pipe_adt_tbl_info_t *adt_tbl = NULL;
  pipe_stat_tbl_info_t *stat_tbl = NULL;
  pipe_meter_tbl_info_t *meter_tbl = NULL;
  pipe_stful_tbl_info_t *stful_tbl = NULL;
  pipe_select_tbl_info_t *sel_tbl = NULL;
  uint32_t i = 0, j = 0, k = 0;

  for (i = 0; i < g_table_info->num_mat_tbls; i++) {
    mat_tbl = &(g_table_info->mat_tbl_list[i]);

    for (j = 0; j < mat_tbl->num_adt_tbl_refs; j++) {
      for (k = 0; k < g_table_info->num_adt_tbls; k++) {
        adt_tbl = &(g_table_info->adt_tbl_list[k]);
        if (adt_tbl->handle == mat_tbl->adt_tbl_ref[j].tbl_hdl &&
            adt_tbl->ref_type != mat_tbl->adt_tbl_ref[j].ref_type) {
          LOG_ERROR(
              "%s:%d Match table 0x%x expects adt ref %d, but adt table 0x%x "
              "has ref %d",
              __func__,
              __LINE__,
              mat_tbl->handle,
              mat_tbl->adt_tbl_ref[j].ref_type,
              adt_tbl->handle,
              adt_tbl->ref_type);
          return PIPE_INVALID_ARG;
        }
      }
    }

    for (j = 0; j < mat_tbl->num_sel_tbl_refs; j++) {
      for (k = 0; k < g_table_info->num_select_tbls; k++) {
        sel_tbl = &(g_table_info->select_tbl_list[k]);
        if (sel_tbl->handle == mat_tbl->sel_tbl_ref[j].tbl_hdl &&
            sel_tbl->ref_type != mat_tbl->sel_tbl_ref[j].ref_type) {
          LOG_ERROR(
              "%s:%d Match table 0x%x expects sel ref %d, but sel table 0x%x "
              "has ref %d",
              __func__,
              __LINE__,
              mat_tbl->handle,
              mat_tbl->sel_tbl_ref[j].ref_type,
              sel_tbl->handle,
              sel_tbl->ref_type);
          return PIPE_INVALID_ARG;
        }
      }
    }

    for (j = 0; j < mat_tbl->num_stat_tbl_refs; j++) {
      for (k = 0; k < g_table_info->num_stat_tbls; k++) {
        stat_tbl = &(g_table_info->stat_tbl_list[k]);
        if (stat_tbl->handle == mat_tbl->stat_tbl_ref[j].tbl_hdl &&
            stat_tbl->ref_type != mat_tbl->stat_tbl_ref[j].ref_type) {
          LOG_ERROR(
              "%s:%d Match table 0x%x expects stat ref %d, but stat table 0x%x "
              "has ref %d",
              __func__,
              __LINE__,
              mat_tbl->handle,
              mat_tbl->stat_tbl_ref[j].ref_type,
              stat_tbl->handle,
              stat_tbl->ref_type);
          return PIPE_INVALID_ARG;
        }
      }
    }

    for (j = 0; j < mat_tbl->num_meter_tbl_refs; j++) {
      for (k = 0; k < g_table_info->num_meter_tbls; k++) {
        meter_tbl = &(g_table_info->meter_tbl_list[k]);
        if (meter_tbl->handle == mat_tbl->meter_tbl_ref[j].tbl_hdl &&
            meter_tbl->ref_type != mat_tbl->meter_tbl_ref[j].ref_type) {
          LOG_ERROR(
              "%s:%d Match table 0x%x expects meter ref %d, but meter table "
              "0x%x has ref %d",
              __func__,
              __LINE__,
              mat_tbl->handle,
              mat_tbl->meter_tbl_ref[j].ref_type,
              meter_tbl->handle,
              meter_tbl->ref_type);
          return PIPE_INVALID_ARG;
        }
      }
    }

    for (j = 0; j < mat_tbl->num_stful_tbl_refs; j++) {
      for (k = 0; k < g_table_info->num_sful_tbls; k++) {
        stful_tbl = &(g_table_info->stful_tbl_list[k]);
        if (stful_tbl->handle == mat_tbl->stful_tbl_ref[j].tbl_hdl &&
            stful_tbl->ref_type != mat_tbl->stful_tbl_ref[j].ref_type) {
          LOG_ERROR(
              "%s:%d Match table 0x%x expects stful ref %d, but stful table "
              "0x%x has ref %d",
              __func__,
              __LINE__,
              mat_tbl->handle,
              mat_tbl->stful_tbl_ref[j].ref_type,
              stful_tbl->handle,
              stful_tbl->ref_type);
          return PIPE_INVALID_ARG;
        }
      }
    }
  }

  return PIPE_SUCCESS;
}

/**
 * Given a cJSON object representing the entire Context JSON, this function
 * parses the RMT configuration from it. This consists of basically iterating
 * over all tables and taking values from the cJSON structure.
 *
 * @param root The cJSON structure corresponding to the Context JSON file.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_rmt_cfg_tables_json(bf_dev_id_t devid,
                                                        profile_id_t prof_id,
                                                        cJSON *root,
                                                        bool virtual_device) {
  int err = 0;
  int rc = PIPE_SUCCESS;

  cJSON *tables_cjson = NULL;
  err |= bf_cjson_get_object(root, CTX_JSON_TABLES_NODE, &tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      rc |= ctx_json_parse_rmt_cfg_match_table_json(
          devid, prof_id, table_cjson, virtual_device, false);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_ACTION_DATA)) {
      rc |= ctx_json_parse_rmt_cfg_action_data_table_json(
          devid, prof_id, table_cjson);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_METER)) {
      rc |=
          ctx_json_parse_rmt_cfg_meter_table_json(devid, prof_id, table_cjson);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_STATEFUL)) {
      rc |= ctx_json_parse_rmt_cfg_stateful_table_json(
          devid, prof_id, table_cjson);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_STATISTICS)) {
      rc |= ctx_json_parse_rmt_cfg_statistics_table_json(
          devid, prof_id, table_cjson);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_SELECTION)) {
      rc |= ctx_json_parse_rmt_cfg_selection_table_json(
          devid, prof_id, table_cjson);
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_CONDITION)) {
      // Nothing to do.
    } else {
      LOG_ERROR("%s:%d: Invalid table type in ContextJSON: %s.",
                __func__,
                __LINE__,
                table_type);
      return PIPE_INVALID_ARG;
    }
    CHECK_RC(rc, cleanup);
  }

  rc = ctx_json_check_references();
  return rc;

cleanup:
  return rc;
}

static void str_to_bytes(uint8_t *dst, const char *src) {
  int len = strlen(src);
  for (int i = 0; i < len / 2; ++i) {
    char byte_str[3] = {src[2 * i], src[2 * i + 1], '\0'};
    dst[i] = strtoul(byte_str, NULL, 16);
  }
}
/**
 * Given a cJSON object representing the entire Context JSON, this function
 * parses the shared (between driver and compiler) register values from it.
 *
 * @param devid The chip id the Context JSON is for.
 * @param config_cache Where to store the values read out.
 * @param root The cJSON structure corresponding to the Context JSON file.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_config_cache(bf_dev_id_t devid,
                                                 bf_map_t *config_cache,
                                                 cJSON *root) {
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  int num_stages = dev_info->num_active_mau;
  int epb_meta_opt_num = 0;

  cJSON *cfg_cjson = NULL;
  int err = bf_cjson_get_object(root, CTX_JSON_CONFIG_CACHE_NODE, &cfg_cjson);
  CHECK_ERR(err, rc, cleanup);

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      epb_meta_opt_num = 1;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      epb_meta_opt_num = TOF2_NUM_EPB;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      epb_meta_opt_num = TOF3_NUM_EPB;
      break;



    default:
      LOG_ERROR("%s:%d Invalid device family %d specified for device %d",
                __func__,
                __LINE__,
                dev_info->dev_family,
                devid);
      return PIPE_UNEXPECTED;
  }

  /* We expect the following to be found in this block of the context json,
   * allocate them now and fill them in as we go through all the entries in the
   * json. */
  struct pipe_config_cache_reg_t *epb_meta_opt = NULL;
  struct pipe_config_cache_reg_t *prsr_m_thread =
      PIPE_MGR_CALLOC(1, sizeof *prsr_m_thread);
  struct pipe_config_cache_meter_sweep_t *meter_sweep =
      PIPE_MGR_MALLOC(sizeof *meter_sweep);
  struct pipe_config_cache_meter_ctl_t *meter_ctl =
      PIPE_MGR_MALLOC(sizeof *meter_ctl);
  struct pipe_config_cache_2d_byte_array_t *power_ctrl =
      PIPE_MGR_MALLOC(sizeof *power_ctrl);
  struct pipe_config_cache_2d_byte_array_t *hash_seed =
      PIPE_MGR_MALLOC(sizeof *hash_seed);
  struct pipe_config_cache_2d_byte_array_t *hash_mask =
      PIPE_MGR_MALLOC(sizeof *hash_mask);

  epb_meta_opt = PIPE_MGR_CALLOC(epb_meta_opt_num, sizeof *epb_meta_opt);
  meter_sweep->val = PIPE_MGR_CALLOC(num_stages, sizeof *meter_sweep->val);
  meter_ctl->val = PIPE_MGR_CALLOC(num_stages, sizeof *meter_ctl->val);
  power_ctrl->val = PIPE_MGR_CALLOC(num_stages, sizeof *power_ctrl->val);
  hash_seed->val = PIPE_MGR_CALLOC(num_stages, sizeof *hash_seed->val);
  hash_mask->val = PIPE_MGR_CALLOC(num_stages, sizeof *hash_mask->val);
  power_ctrl->size1 = num_stages;
  hash_seed->size1 = num_stages;
  hash_mask->size1 = num_stages;
  for (int i = 0; i < num_stages; ++i) {
    meter_sweep->val[i] =
        PIPE_MGR_CALLOC(dev_info->dev_cfg.stage_cfg.num_meter_alus,
                        sizeof *meter_sweep->val[i]);
    meter_ctl->val[i] = PIPE_MGR_CALLOC(
        dev_info->dev_cfg.stage_cfg.num_meter_alus, sizeof *meter_ctl->val[i]);
    power_ctrl->size2 = 128;
    hash_seed->size2 = 208;
    hash_mask->size2 = 64;
    power_ctrl->val[i] = PIPE_MGR_CALLOC(power_ctrl->size2, sizeof(uint8_t));
    hash_seed->val[i] = PIPE_MGR_CALLOC(hash_seed->size2, sizeof(uint8_t));
    hash_mask->val[i] = PIPE_MGR_CALLOC(hash_mask->size2, sizeof(uint8_t));
  }
  bf_map_init(config_cache);
  bf_map_add(config_cache, pipe_cck_meta_opt_ctrl, epb_meta_opt);
  bf_map_add(config_cache, pipe_cck_parser_multi_threading, prsr_m_thread);
  bf_map_add(config_cache, pipe_cck_meter_sweep_ctrl, meter_sweep);
  bf_map_add(config_cache, pipe_cck_meter_ctrl, meter_ctl);
  bf_map_add(config_cache, pipe_cck_xbar_din_power_ctrl, power_ctrl);
  bf_map_add(config_cache, pipe_cck_hash_seed, hash_seed);
  bf_map_add(config_cache, pipe_cck_hash_parity_group_mask, hash_mask);
  cJSON *cfg_item;
  CTX_JSON_FOR_EACH(cfg_item, cfg_cjson) {
    char *cfg_name;
    err = bf_cjson_get_string(cfg_item, CTX_JSON_CONFIG_CACHE_NAME, &cfg_name);
    CHECK_ERR(err, rc, cleanup);
    char *value;
    err = bf_cjson_get_string(cfg_item, CTX_JSON_CONFIG_CACHE_VALUE, &value);
    CHECK_ERR(err, rc, cleanup);
    if (strstr(cfg_name, "epb_prsr_port_regs.chnl_ctrl[0]") && epb_meta_opt) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      epb_meta_opt->val = strtoul(value, NULL, 16);
    } else if (strstr(cfg_name, "epb_prsr_port_regs.multi_threading")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      prsr_m_thread->val = strtoul(value, NULL, 16);
    } else if (strstr(cfg_name, "meter_sweep_ctl")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      /* Pick out the mau and meter ALU ids out of a string with the following
       * format:
       * mau[0].rams.match.adrdist.meter_sweep_ctl.meter_sweep_ctl[0]
       * First break it with '[' and ']' to get the stage, which could be one or
       * two characters.  Then pick the ALU id from the end of the string; we
       * know that is a single character. */
      int alu = cfg_name[strlen(cfg_name) - 2] - '0';
      int stage = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, stage, num_stages);
      meter_sweep->val[stage][alu] = strtoul(value, NULL, 16);
    } else if (strstr(cfg_name, "meter.meter_ctl")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      /* find stage id and meter alu id out of a string with the following
       * format:
       * mau[stage_id].rams.map_alu.meter_group[alu_id].meter.meter_ctl*/
      int alu_id =
          cfg_name[strlen(cfg_name) - strlen("].meter.meter_ctl") - 1] - '0';
      int stage = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, stage, num_stages);
      meter_ctl->val[stage][alu_id] = strtoul(value, NULL, 16);
    } else if (strstr(cfg_name, "match_input_xbar_din_power_ctl")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      /* Similar to above, pick out the stage id from the string. */
      int stage = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, stage, num_stages);
      str_to_bytes(power_ctrl->val[stage], value);
    } else if (strstr(cfg_name, "hash_seed")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      /* Similar to above, pick out the stage id from the string. */
      int stage = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, stage, num_stages);
      str_to_bytes(hash_seed->val[stage], value);
    } else if (strstr(cfg_name, "parity_group_mask")) {
      LOG_TRACE("Dev %d: Parsed %s as %s", devid, cfg_name, value);
      /* Similar to above, pick out the stage id from the string. */
      int stage = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, stage, num_stages);
      str_to_bytes(hash_mask->val[stage], value);
    } else if (strstr(cfg_name, "epbreg.chan0_group.chnl_ctrl.meta_opt") &&
               epb_meta_opt) {
      /* Similar to above, pick out the EPB from the string. */
      int epb_num = -1;
      PIPE_MGR_PARSE_CFG_NAME(cfg_name, epb_num, epb_meta_opt_num);
      /* Just pick the lower 13 bits. */
      epb_meta_opt[epb_num].val = strtoul(&value[9], NULL, 16);
    }
  }
cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_stage_extension(rmt_dev_info_t *dev_info,
                                                    profile_id_t prof_id,
                                                    cJSON *root) {
  pipe_status_t rc = PIPE_SUCCESS;

  /* The MAU Stage Extension node is optional, if it is not present we can
   * return now. */
  cJSON *cfg_cjson = NULL;
  bf_cjson_try_get_object(root, CTX_JSON_MAU_STAGE_EXT_NODE, &cfg_cjson);
  if (!cfg_cjson) {
    /* If there is no stage extension assume all stages are programmed. */
    dev_info->profile_info[prof_id]->num_stages = dev_info->num_active_mau;
    return PIPE_SUCCESS;
  }

  struct pipe_config_cache_bypass_stage_t *cfg = NULL;
  cfg = PIPE_MGR_CALLOC(1, sizeof *cfg);
  if (!cfg) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  /* Get the stage id of the last stage programmed by the bin file. */
  int sts = bf_cjson_get_int(
      cfg_cjson, CTX_JSON_MAU_STAGE_EXT_LEN, &cfg->last_stage_programmed);
  if (sts) {
    rc = PIPE_UNEXPECTED;
    goto cleanup;
  }
  dev_info->profile_info[prof_id]->num_stages = cfg->last_stage_programmed + 1;

  cJSON *regs = NULL;
  sts = bf_cjson_get_object(cfg_cjson, CTX_JSON_MAU_STAGE_EXT_REG, &regs);
  if (sts || !regs) {
    rc = PIPE_UNEXPECTED;
    goto cleanup;
  }
  /* There should be at least one register to describe the bypass stage
   * configuration. */
  cfg->reg_cnt = cJSON_GetArraySize(regs);
  if (cfg->reg_cnt < 1) {
    rc = PIPE_UNEXPECTED;
    goto cleanup;
  }
  cfg->regs = PIPE_MGR_CALLOC(cfg->reg_cnt, sizeof *cfg->regs);
  if (!cfg->regs) {
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  for (int i = 0; i < cfg->reg_cnt; ++i) {
    struct pipe_config_cache_bypass_stage_reg_t *r = cfg->regs + i;
    cJSON *reg = cJSON_GetArrayItem(regs, i);
    if (!reg) {
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
    int offset;
    sts = bf_cjson_get_int(reg, CTX_JSON_MAU_STAGE_EXT_REG_OFF, &offset);
    if (sts) {
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
    if ((dev_info->dev_family == BF_DEV_FAMILY_TOFINO2) ||
        (dev_info->dev_family == BF_DEV_FAMILY_TOFINO3)) {
      uint32_t addr = dev_info->dev_cfg.dir_addr_set_pipe_type(offset);
      r->num_offsets = 1;
      r->offset = PIPE_MGR_CALLOC(r->num_offsets, sizeof *r->offset);
      if (!r->offset) return PIPE_NO_SYS_RESOURCES;
      r->offset[0] = addr;
    } else {
      LOG_ERROR("Bypass Stage Parsing not implemented for dev_family %d",
                dev_info->dev_family);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
    }

    cJSON *vals = NULL, *msks = NULL;
    sts = bf_cjson_get_object(reg, CTX_JSON_MAU_STAGE_EXT_REG_MSK, &msks);
    if (sts || !msks) {
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
    sts = bf_cjson_get_object(reg, CTX_JSON_MAU_STAGE_EXT_REG_VAL, &vals);
    if (sts || !vals) {
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }
    /* There should be exactly 3 mask values (old-last, intermediate, new-last)
     * and at least one value. */
    int msk_count = cJSON_GetArraySize(msks);
    int val_count = cJSON_GetArraySize(vals);
    if (val_count < 1 || msk_count != 3) {
      rc = PIPE_UNEXPECTED;
      goto cleanup;
    }

    for (int j = 0; j < msk_count; ++j)
      r->mask[j] = cJSON_GetArrayItem(msks, j)->valueint;

    r->num_vals = val_count;
    r->vals = PIPE_MGR_CALLOC(r->num_vals, sizeof *r->vals);
    if (!r->vals) {
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    for (int j = 0; j < r->num_vals; ++j)
      r->vals[j] = cJSON_GetArrayItem(vals, j)->valueint;
  }

  /* Now that the stage extension information has been parsed, save it in our
   * config_cache. */
  bf_map_add(&dev_info->profile_info[prof_id]->config_cache,
             pipe_cck_mau_stage_ext,
             cfg);
  return PIPE_SUCCESS;

cleanup:
  if (cfg) {
    if (cfg->regs) {
      for (int i = 0; i < cfg->reg_cnt; ++i) {
        if (cfg->regs[i].offset) PIPE_MGR_FREE(cfg->regs[i].offset);
        if (cfg->regs[i].vals) PIPE_MGR_FREE(cfg->regs[i].vals);
      }
      PIPE_MGR_FREE(cfg->regs);
    }
    PIPE_MGR_FREE(cfg);
  }

  return rc;
}

static pipe_status_t ctx_json_parse_version_str(char *version,
                                                int *version_arr) {
  /* The required format of the version string is "X.Y.Z" where X, Y, and Z are
   * positive integers.  There may optionally be a "-pr.#" appended to the
   * version string indicating a pre-release version. */
  if (!version) return PIPE_INVALID_ARG;

  /*
   * First locate the two "." seperators in the version string and perform a few
   * sanity checks to make sure that we are not looking at malformed strings
   * such as "X..Y", "..", "X.Y.", etc.
   */
  char *seperator[3] = {NULL, NULL, NULL};
  seperator[0] = strchr(version, '.');
  if (!seperator[0]) {
    /* There were no "." characters in the string at all! */
    return PIPE_INVALID_ARG;
  }
  if (seperator[0][1] == '\0') {
    /* There was a "." but it was at the end of the string! */
    return PIPE_INVALID_ARG;
  }
  if (version == seperator[0]) {
    /* The string started with a "."! */
    return PIPE_INVALID_ARG;
  }
  seperator[1] = strchr(seperator[0] + 1, '.');
  if (!seperator[1]) {
    /* The second "." was not present in the string! */
    return PIPE_INVALID_ARG;
  }
  if (seperator[1][1] == '\0') {
    /* The second "." was at the end of the string! */
    return PIPE_INVALID_ARG;
  }
  if (seperator[0] + 1 == seperator[1]) {
    /* The string contained ".."! */
    return PIPE_INVALID_ARG;
  }
  /* This last seperator is optional so it can be NULL. */
  seperator[2] = strchr(seperator[1] + 1, '-');

  /*
   * Now that the seperators have been located convert the string upto the
   * first "." into an integer.
   */

  /* Terminate the string at the first seperator. */
  char original = seperator[0][0];
  seperator[0][0] = '\0';
  /* Convert the string to an integer. */
  char *end_ptr = NULL;
  version_arr[0] = strtoul(version, &end_ptr, 10);
  if (version == end_ptr || *end_ptr != '\0') {
    /* Restore the string to the orignal value. */
    seperator[0][0] = original;
    /* There were no characters or extra characters in the string! */
    return PIPE_INVALID_ARG;
  }
  /* Restore the string to the orignal value. */
  seperator[0][0] = original;

  /*
   * Convert the string between the first and second seperators into an integer.
   */
  original = seperator[1][0];
  seperator[1][0] = '\0';
  /* Convert the string to an integer. */
  end_ptr = NULL;
  version_arr[1] = strtoul(seperator[0] + 1, &end_ptr, 10);
  if ((seperator[0] + 1) == end_ptr || *end_ptr != '\0') {
    /* Restore the string to the orignal value. */
    seperator[1][0] = original;
    /* There were no characters or extra characters in the string! */
    return PIPE_INVALID_ARG;
  }
  /* Restore the string to the orignal value. */
  seperator[1][0] = original;

  /*
   * Convert the remaining string into an integer ignoring the trailing "-pr-#"
   * if it exists.
   */
  char *space_ptr = NULL;
  if (seperator[2]) {
    original = seperator[2][0];
    seperator[2][0] = '\0';
  } else if ((space_ptr = strchr(version, ' '))) {
    /* A space separates the version from the git SHA, which should be
     * ignored. */
    *space_ptr = '\0';
  }
  /* Convert the string to an integer. */
  end_ptr = NULL;
  version_arr[2] = strtoul(seperator[1] + 1, &end_ptr, 10);
  if ((seperator[1] + 1) == end_ptr) {
    /* Restore the string to the orignal value. */
    if (seperator[2]) seperator[2][0] = original;
    /* There were no characters in the string! */
    return PIPE_INVALID_ARG;
  }
  if ((seperator[2] && *end_ptr != seperator[2][0]) ||
      (!seperator[2] && *end_ptr != '\0')) {
    /* Restore the string to the orignal value. */
    if (seperator[2]) seperator[2][0] = original;
    /* There were invalid characters in the string! */
    return PIPE_INVALID_ARG;
  }
  /* Restore the string to the orignal value. */
  if (seperator[2]) seperator[2][0] = original;
  if (space_ptr) *space_ptr = ' ';
  return PIPE_SUCCESS;
}

static pipe_status_t ctx_json_parse_version(uint8_t devid,
                                            profile_id_t prof_id,
                                            cJSON *root) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }

  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  pipe_status_t sts;
  char *version = NULL;
  int err = 0;

  err = bf_cjson_get_string(root, CTX_JSON_COMPILER_VERSION, &version);
  if (err) {
    return PIPE_OBJ_NOT_FOUND;
  }
  sts = ctx_json_parse_version_str(version, prof_info->compiler_version);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Error %s parsing compiler version string \"%s\" for dev %d profile "
        "%d, program name %s pipeline name %s config file %s",
        pipe_str_err(sts),
        version,
        devid,
        prof_id,
        prof_info->prog_name,
        prof_info->pipeline_name,
        prof_info->cfg_file);
    return sts;
  }

  err = bf_cjson_get_string(root, CTX_JSON_SCHEMA_VERSION, &version);
  if (err) {
    return PIPE_OBJ_NOT_FOUND;
  }
  sts = ctx_json_parse_version_str(version, prof_info->schema_version);
  if (sts != PIPE_SUCCESS) {
    LOG_ERROR(
        "Error %s parsing schema version string \"%s\" for dev %d profile "
        "%d, program name %s pipeline name %s config file %s",
        pipe_str_err(sts),
        version,
        devid,
        prof_id,
        prof_info->prog_name,
        prof_info->pipeline_name,
        prof_info->cfg_file);
    return sts;
  }

  return PIPE_SUCCESS;
}

/**
 * Parse the stage characteristics entry format for a given ContextJson.
 */
static pipe_status_t ctx_json_parse_stage_characteristics_entry(
    cJSON *stage_dp_cjson_item,
    int *stage,
    bool *dir,
    pipe_mgr_stage_char_t *result) {
  if (stage == NULL || dir == NULL || result == NULL) return PIPE_INVALID_ARG;
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  char *gress;

  err |= bf_cjson_get_int(
      stage_dp_cjson_item, CTX_JSON_STAGE_DEPENDENCY_STAGE, stage);
  CHECK_ERR(err, rc, cleanup);

  err |= bf_cjson_get_string(
      stage_dp_cjson_item, CTX_JSON_STAGE_DEPENDENCY_GRESS, &gress);
  CHECK_ERR(err, rc, cleanup);
  if (strstr(gress, "ingress"))
    *dir = 0;
  else if (strstr(gress, "egress"))
    *dir = 1;

  err |= bf_cjson_get_bool(stage_dp_cjson_item,
                           CTX_JSON_STAGE_DEPENDENCY_MATCH,
                           &(result->match_dp));
  CHECK_ERR(err, rc, cleanup);

  err |= bf_cjson_get_int(stage_dp_cjson_item,
                          CTX_JSON_STAGE_CHARACTERISTICS_CLOCK_CYCLES,
                          &(result->clock_cycles));
  CHECK_ERR(err, rc, cleanup);

  return rc;
cleanup:
  return PIPE_INVALID_ARG;
}

static pipe_status_t ctx_json_parse_stage_characteristics(
    bf_dev_id_t devid, bf_map_t *stage_characteristics, cJSON *root) {
  /* if not Tof2, return PIPE_SUCCESS*/
  pipe_status_t rc = PIPE_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  bf_map_init(stage_characteristics);
  if ((dev_info->dev_family != BF_DEV_FAMILY_TOFINO2) &&
      (dev_info->dev_family != BF_DEV_FAMILY_TOFINO3))
    return rc;
  cJSON *stage_dependency_cjson = NULL;

  /* Try to get the mau_stage_characteristics node. */
  bf_cjson_try_get_object(
      root, CTX_JSON_STAGE_CHARACTERISTICS_NODE, &stage_dependency_cjson);
  /* If we didn't find it we may be looking at a contex.json generated by an
   * older compiler, schema version less than 1.10.0, so look for the
   * stage_dependency node instead. */
  if (!stage_dependency_cjson) {
    bf_cjson_try_get_object(
        root, CTX_JSON_STAGE_DEPENDENCY_NODE, &stage_dependency_cjson);
  }

  /* We should have found one of them...  If not return an error. */
  if (!stage_dependency_cjson) return PIPE_INVALID_ARG;

  // Parse stage characteristics entry format.
  cJSON *stage_characteristics_item = NULL;
  pipe_mgr_stage_char_t *entry;
  int stage;
  bool dir;
  CTX_JSON_FOR_EACH(stage_characteristics_item, stage_dependency_cjson) {
    dir = 0;
    entry = PIPE_MGR_CALLOC(1, sizeof(pipe_mgr_stage_char_t));
    if (entry == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for pipe_mgr_stage_char_t object.",
          __func__,
          __LINE__);
      return PIPE_NO_SYS_RESOURCES;
    }
    rc = ctx_json_parse_stage_characteristics_entry(
        stage_characteristics_item, &stage, &dir, entry);
    if (rc != PIPE_SUCCESS) {
      PIPE_MGR_FREE(entry);
      return rc;
    }
    unsigned long k = (stage << 1) | dir;
    bf_map_add(stage_characteristics, k, (void *)entry);
  }
  return rc;
}

static pipe_status_t parse_and_check_target(const rmt_dev_info_t *dev_info,
                                            cJSON *root) {
  char *trgt = NULL;
  bf_cjson_try_get_string(root, CTX_JSON_CHIP_TARGET, &trgt);
  if (!trgt) return PIPE_OBJ_NOT_FOUND;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (!strcmp(trgt, "tofino")) {
        return PIPE_SUCCESS;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      if (!strcmp(trgt, "tofino2") || !strcmp(trgt, "tofino2u") ||
          !strcmp(trgt, "tofino2m") || !strcmp(trgt, "tofino2h")) {
        return PIPE_SUCCESS;
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if (!strcmp(trgt, "tofino3")) {
        return PIPE_SUCCESS;
      }
      break;





    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      break;
  }

  LOG_ERROR(
      "%s: Chip type mismatch, device is type %s but the context.json is for "
      "type %s",
      __func__,
      bf_dev_family_str(dev_info->dev_family),
      trgt);
  return PIPE_INVALID_ARG;
}

/**
 * Main routine for parsing the ContextJSON file. The file is assumed to be in
 * the base directory, named "context.json", but the path can be specified
 * to this function.
 *
 * @param dev_info The current device's dev_info.
 * @param prof_id The id of the profile the context json is for.
 * @param config_file_path The path to the Context JSON file.
 *
 * @return A pointer to a rmt_dev_tbl_info_t structure, containing the parsed
 * table information. As a side effect, this function populates entry format,
 * hash bits, p4 parser and dynamic key masking global structures.
 */
rmt_dev_tbl_info_t *pipe_mgr_ctx_json_parse(rmt_dev_info_t *dev_info,
                                            profile_id_t prof_id,
                                            const char *config_file_path,
                                            bool virtual_device) {
  pipe_status_t rc = PIPE_SUCCESS;
  LOG_TRACE("Parsing table configuration file: %s.", config_file_path);
  bf_dev_id_t devid = dev_info->dev_id;

  /* Allocate table info structure. */
  g_table_info = PIPE_MGR_CALLOC(1, sizeof(rmt_dev_tbl_info_t));
  if (g_table_info == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for rmt_dev_tbl_info_t object.",
              __func__,
              __LINE__);
    goto table_info_alloc_err;
  }

  /* Open context json file. */
  FILE *file = fopen(config_file_path, "r");
  if (file == NULL) {
    LOG_ERROR("%s:%d: Could not open configuration file: %s.\n",
              __func__,
              __LINE__,
              config_file_path);
    goto config_file_fopen_err;
  }

  /* Get size of json file. */
  int fd = fileno(file);
  struct stat stat_b;
  if (fstat(fd, &stat_b)) {
    LOG_ERROR("%s:%d: Could not get config file information %s",
              __func__,
              __LINE__,
              config_file_path);
    goto config_file_buffer_alloc_err;
  }
  size_t to_allocate = stat_b.st_size + 1;

  /* Allocate buffer to hold entire file. */
  char *config_file_buffer = PIPE_MGR_CALLOC(1, to_allocate);
  if (config_file_buffer == NULL) {
    LOG_ERROR("%s:%d: Could not allocate memory for configuration file buffer.",
              __func__,
              __LINE__);
    goto config_file_buffer_alloc_err;
  }

  /* Read file into buffer. */
  size_t num_items = fread(config_file_buffer, stat_b.st_size, 1, file);
  if (num_items != 1) {
    if (ferror(file)) {
      LOG_ERROR(
          "%s:%d: An error occurred while reading configuration file buffer.",
          __func__,
          __LINE__);
      goto config_file_fread_err;
    }

    PIPE_MGR_ASSERT(feof(file));
    LOG_DBG("%s:%d: End of file reached before expected. Continuing.",
            __func__,
            __LINE__);
  }

  /* Create JSON parse object. */
  cJSON *root = cJSON_Parse(config_file_buffer);
  if (root == NULL) {
    LOG_ERROR("%s:%d: cJSON error while parsing configuration file.",
              __func__,
              __LINE__);
    goto cjson_parse_err;
  }

  /* Validate the chip type in the context.json against the type of chip it is
   * being loaded against. */
  rc = parse_and_check_target(dev_info, root);
  if (rc != PIPE_SUCCESS) {
    if (rc == PIPE_OBJ_NOT_FOUND) {
      /* The object not found indicates we couldn't parse the information from
       * the context.json.  Other non-success error codes mean we parsed the
       * json correctly but the data was not correct for this chip. */
      LOG_ERROR(
          "%s:%d: Failed to parse target from ContextJSON", __func__, __LINE__);
    }
    goto target_parse_err;
  }
  // compiler_version, schema_version
  rc = ctx_json_parse_version(devid, prof_id, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse version from ContextJSON", __func__, __LINE__);
    goto version_parse_err;
  }

  rc = ctx_json_parse_parser(devid, prof_id, root, virtual_device);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse PVS information from ContextJSON for %s "
        "device.",
        __func__,
        __LINE__,
        (virtual_device ? "virtual" : "non-virtual"));
    goto parse_parser_err;
  }

  rc = ctx_json_parse_rmt_cfg_tables_json(devid, prof_id, root, virtual_device);
  if (rc) {
    LOG_ERROR("%s:%d: Failed to RMT table information from ContextJSON.",
              __func__,
              __LINE__);
    goto ctx_json_parse_rmt_cfg_tables_err;
  }

  rc = ctx_json_parse_entry_format(devid, prof_id, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse entry format information from ContextJSON.",
        __func__,
        __LINE__);
    goto parse_entry_format_err;
  }

  rc = ctx_json_parse_hashes(devid, prof_id, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse hash information from tables in ContextJSON.",
        __func__,
        __LINE__);
    goto parse_hash_bits_err;
  }

  rc = ctx_json_parse_dkm(dev_info, prof_id, root, g_table_info);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse dynamic key masks information from "
        "ContextJSON.",
        __func__,
        __LINE__);
    goto parse_dkm_err;
  }

  rc = ctx_json_parse_stage_characteristics(
      devid, &dev_info->profile_info[prof_id]->stage_characteristics, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse stage characteristics information from "
        "ContextJSON.",
        __func__,
        __LINE__);
    goto parse_cc_err;
  }

  rc = ctx_json_parse_config_cache(
      devid, &dev_info->profile_info[prof_id]->config_cache, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse config cache information from ContextJSON.",
        __func__,
        __LINE__);
    goto parse_cc_err;
  }

  rc = ctx_json_parse_dynamic_hash_calc(
      devid, prof_id, &dev_info->profile_info[prof_id]->dhash_info, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse dynamic hash calculations information from "
        "ContextJSON.",
        __func__,
        __LINE__);
    goto parse_cc_err;
  }

  rc = ctx_json_parse_stage_extension(dev_info, prof_id, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse MAU stage extension information from "
        "ContextJSON.",
        __func__,
        __LINE__);
    goto parse_cc_err;
  }

  rc = ctx_json_parse_driver_options(
      devid, &dev_info->profile_info[prof_id]->driver_options, root);
  if (rc) {
    LOG_ERROR(
        "%s:%d: Failed to parse driver options from "
        "ContextJSON.",
        __func__,
        __LINE__);
    goto parse_cc_err;
  }

  LOG_TRACE("Successfully parsed table configuration file.");

  fclose(file);
  PIPE_MGR_FREE(config_file_buffer);
  cJSON_Delete(root);
  return g_table_info;

parse_cc_err:
parse_dkm_err:
parse_parser_err:
parse_entry_format_err:
parse_hash_bits_err:
ctx_json_parse_rmt_cfg_tables_err:
version_parse_err:
target_parse_err:
  cJSON_Delete(root);
cjson_parse_err:
config_file_fread_err:
  PIPE_MGR_FREE(config_file_buffer);
config_file_buffer_alloc_err:
  fclose(file);
config_file_fopen_err:
  PIPE_MGR_FREE(g_table_info);
table_info_alloc_err:
  return NULL;
}
