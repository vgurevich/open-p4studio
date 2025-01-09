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
 * @file pipe_mgr_ctx_json_entry_format.c
 *
 * Parser routines for populating entry format information from the Context
 * JSON file.
 */

#include <stdbool.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#include <bf_types/bf_types.h>
#include <target-utils/third-party/cJSON/cJSON.h>
#include <pipe_mgr/pipe_mgr_config.h>
#include <pipe_mgr/pipe_mgr_err.h>
#include <pipe_mgr/pipe_mgr_porting.h>

#include "pipe_mgr_ctx_json.h"
#include "pipe_mgr_entry_format_json.h"
#include "pipe_mgr_table_packing.h"

#define DEFAULT_PFE_POSITION 0
#define IDLETIME_PFE_POSITION 20
#define ACTION_DATA_PTR_PFE_POSITION 22
#define METER_PTR_PFE_POSITION 23
#define STATEFUL_PTR_PFE_POSITION 23
#define SELECTION_PTR_PFE_POSITION 23
#define STATISTICS_PTR_PFE_POSITION 19
#define METER_COLOR_AWARE_BIT_POSITION 24

#define BYTE_WIDTH 8

extern pipemgr_tbl_pkg_ctx_t *g_tbl_pkg_ctx[BF_MAX_DEV_COUNT];
#define PIPE_MGR_TBL_PKG_CTX(_dev, _prof) (g_tbl_pkg_ctx[_dev]->profiles[_prof])

/**
 * Converts from enum ctx_json_match_type to the entry format internal enum.
 * This function really should be a no-op: the order of the enums is the same.
 * But, for future maintenance, it's easier to have this helper converter,
 * so that if a new match_type arises, it forces people to add it here rather
 * than rely on the order of enums.
 *
 * @param match_type The value of the ctx_json_match_type enum, casted to an
 * int.
 *
 * @return The corresponding internal entry_format match type.
 */
static inline int entry_format_match_type_from_ctx_json(int match_type) {
  switch (match_type) {
    case CTX_JSON_MATCH_TYPE_EXACT:
      return TBL_PKG_FIELD_MATCHTYPE_EXACT;
    case CTX_JSON_MATCH_TYPE_TERNARY:
      return TBL_PKG_FIELD_MATCHTYPE_TERNARY;
    case CTX_JSON_MATCH_TYPE_LPM:
      return TBL_PKG_FIELD_MATCHTYPE_LPM;
    case CTX_JSON_MATCH_TYPE_RANGE:
      return TBL_PKG_FIELD_MATCHTYPE_RANGE;
  }
  return TBL_PKG_FIELD_MATCHTYPE_INVALID;
}

static char *normalize_name(char *str) {
  size_t len = strlen(str);
  for (unsigned int i = 0; i < len; i++) {
    if (str[i] == '.' || str[i] == '[' || str[i] == ']' || str[i] == '$') {
      str[i] = '_';
    }
  }
  return str;
}

static uint32_t pipemgr_tbl_pkg_gen_str_index_for_field_name(
    bf_dev_id_t devid, char *string, profile_id_t prof_id) {
  uint32_t field_name_str_index;
  if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).field_name_str_index >=
      PIPEMGR_TBL_PKG_MAX_P4_FIELD_NAMES) {
    LOG_ERROR("Insufficient p4 field name table entries.");
    PIPE_MGR_ASSERT(0);
  }
  field_name_str_index =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).field_name_str_index;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).p4_strings[field_name_str_index] =
      normalize_name(bf_sys_strdup(string));
  if (!PIPE_MGR_TBL_PKG_CTX(devid, prof_id).p4_strings[field_name_str_index]) {
    LOG_ERROR("Unable to allocate memory for p4 string.");
    PIPE_MGR_ASSERT(0);
  }
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).field_name_str_index++;
  return field_name_str_index;
}

static pipemgr_tbl_pkg_lut_t *pipemgr_tbl_pkg_update_lut_entry(
    pipemgr_tbl_pkg_lut_t *tbl_lut_ptr,
    uint16_t table_depth,
    int *hash_collision,
    uint16_t bj_lut_index,
    uint32_t hndl,
    uint8_t stage,
    uint16_t field_mem_size) {
  pipemgr_tbl_pkg_lut_t *lut_ptr = tbl_lut_ptr + bj_lut_index;
  int k = 0;

  while ((k < PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR) &&
         ((lut_ptr->tbl_hndl != 0) || (lut_ptr->stage != 0))) {
    // Handle hash collision...
    lut_ptr += table_depth;
    k++;
  }

  if (k >= PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR) {
    // Fatal...
    // Hashing is poor... Not able to handle collision.
    // Collison probability > 1/PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR.
    // Improve hashing or increase collision factor.
    LOG_ERROR("Cannot handle hash collision.");
    return (NULL);
  }

  if (k) {
    // For debug/building confidence in bob hash
    (*hash_collision)++;
    // For validating Hash entropy..Ideally this counter should be 0.
    // If positive, expectation is less than
    // PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR.
    // When more than PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR and no fatal error
    // (that is same bucket is not indexed more than COLLISON_HANDLE_FACTOR)
    // then use judgement call here. If this counter is greater than
    // 40, then look for different hash !!
  }
  lut_ptr->tbl_hndl = hndl;
  lut_ptr->stage = stage;
  lut_ptr->match_field_mem_size = field_mem_size;

  return (lut_ptr);
}

// #### Parse details from specs helper functions ####

static pipe_status_t ctx_json_parse_action_immediate(
    bf_dev_id_t devid,
    cJSON *action_cjson,
    cJSON *immediate_field_cjson,
    pipemgr_tbl_pkg_action_parameter_field_t *immediate_ptr) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  // Immediate names are currently not tracked. If they must be, then devid will
  // be used.
  (void)devid;

  char *immediate_type = NULL;
  char *immediate_name = NULL;
  int dest_start = 0;
  int dest_width = 0;

  err |= bf_cjson_get_string(immediate_field_cjson,
                             CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_TYPE,
                             &immediate_type);
  err |= bf_cjson_get_string(immediate_field_cjson,
                             CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_NAME,
                             &immediate_name);
  err |= bf_cjson_get_int(immediate_field_cjson,
                          CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_DEST_START,
                          &dest_start);
  err |= bf_cjson_get_int(immediate_field_cjson,
                          CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_DEST_WIDTH,
                          &dest_width);
  CHECK_ERR(err, rc, cleanup);

  immediate_ptr->start_offset = dest_start;
  immediate_ptr->bit_width = dest_width;

  if (!strcmp(immediate_type,
              CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_TYPE_CONSTANT)) {
    double const_value = 0;
    err |=
        bf_cjson_get_double(immediate_field_cjson,
                            CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_CONST_VALUE,
                            &const_value);
    CHECK_ERR(err, rc, cleanup);

    immediate_ptr->is_parameter = 0;
    immediate_ptr->constant_value = (uint32_t)const_value;
    immediate_ptr->param_shift = 0;
  } else if (
      !strcmp(immediate_type,
              CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_TYPE_PARAMETER)) {
    int param_shift = 0;
    int param_width = 0;
    int param_offset = 0;
    err |= bf_cjson_get_int(immediate_field_cjson,
                            CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS_PARAM_SHIFT,
                            &param_shift);
    CHECK_ERR(err, rc, cleanup);

    immediate_ptr->param_shift = param_shift;
    immediate_ptr->is_parameter = 1;
    immediate_ptr->constant_value = 0;

    // Find param_start and param_width from action spec.
    err |= ctx_json_parse_action_spec_details_for_field(
        action_cjson, immediate_name, &param_width, &param_offset);
    CHECK_ERR(err, rc, cleanup);

    immediate_ptr->param_width = param_width;
    immediate_ptr->param_start = param_offset;

    bool is_mod_field_conditionally_mask = false;
    bool is_mod_field_conditionally_value = false;
    char *mod_field_conditionally_mask_field_name = NULL;

    bf_cjson_try_get_bool(
        immediate_field_cjson,
        CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_MASK,
        &is_mod_field_conditionally_mask);
    bf_cjson_try_get_bool(
        immediate_field_cjson,
        CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_VALUE,
        &is_mod_field_conditionally_value);
    bf_cjson_try_get_string(
        immediate_field_cjson,
        CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_MASK_FIELD_NAME,
        &mod_field_conditionally_mask_field_name);

    immediate_ptr->is_mod_field_conditionally_mask =
        is_mod_field_conditionally_mask;
    immediate_ptr->is_mod_field_conditionally_value =
        is_mod_field_conditionally_value;
    if (immediate_ptr->is_mod_field_conditionally_mask) {
      /* For conditionally modified fields, the width of the field has to be one
       * byte or less
       */
      if (param_width > 8) {
        LOG_ERROR("%s:%d Invalid width %d for conditional mask \"%s\"",
                  __func__,
                  __LINE__,
                  param_width,
                  immediate_name);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
    }
    if (immediate_ptr->is_mod_field_conditionally_value) {
      if (!mod_field_conditionally_mask_field_name) {
        LOG_ERROR(
            "%s:%d Invalid ContextJSON format: missing mask field name for "
            "conditional field \"%s\"",
            __func__,
            __LINE__,
            immediate_name);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
      err |= ctx_json_parse_action_spec_details_for_field(
          action_cjson,
          mod_field_conditionally_mask_field_name,
          &param_width,
          &param_offset);
      CHECK_ERR(err, rc, cleanup);
      immediate_ptr->mask_offset = param_offset;
      immediate_ptr->mask_width = param_width;
    }
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: immediate type \"%s\" is not "
        "valid.",
        __func__,
        __LINE__,
        immediate_type);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  CHECK_ERR(err, rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parse the action formats cJSON into the internal structure, given by the
 * action_handles_ptr struct.
 *
 * @param devid The current device's id.
 * @param action_formats_cjson A cJSON array of action formats.
 * @param actions_cjson The action spec for the current table.
 * @param action_handles_ptr A pointer to the internal structure to represent
 * the action handles..
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_action_handle_format(
    bf_dev_id_t devid,
    cJSON *action_formats_cjson,
    cJSON *actions_cjson,
    pipemgr_tbl_pkg_action_handles_t *action_handles_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int number_action_handles = 0;
  int number_immediate_fields = 0;

  // First action handle location in memory.
  action_handles_ptr->action_hdl =
      (pipemgr_tbl_pkg_action_entry_field_t *)(action_handles_ptr + 1);

  cJSON *action_format_cjson = NULL;
  CTX_JSON_FOR_EACH(action_format_cjson, action_formats_cjson) {
    // Next action handle: starting from the first entry, skip all action
    // handles and immediate fields so far.
    void *ptr =
        (uint8_t *)action_handles_ptr->action_hdl +
        number_action_handles * sizeof(pipemgr_tbl_pkg_action_entry_field_t) +
        number_immediate_fields *
            sizeof(pipemgr_tbl_pkg_action_parameter_field_t);
    pipemgr_tbl_pkg_action_entry_field_t *action_entry_ptr = ptr;

    int action_handle = 0;
    char *action_name = NULL;
    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_ACTION_HANDLE,
                               &action_handle);
    err |= bf_cjson_get_string(
        action_format_cjson, CTX_JSON_ACTION_FORMAT_ACTION_NAME, &action_name);
    CHECK_ERR(err, rc, cleanup);

    action_entry_ptr->action_handle = action_handle;
    action_entry_ptr->stringindex =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, action_name, prof_id);

    // Find the action in the actions list passed in.
    cJSON *action_cjson = NULL;
    CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
      char *name = NULL;
      err |= bf_cjson_get_string(action_cjson, CTX_JSON_ACTION_NAME, &name);
      CHECK_ERR(err, rc, cleanup);

      // Found the match.
      if (!strcmp(name, action_name)) {
        break;
      }
    }
    if (action_cjson == NULL) {
      LOG_ERROR("%s:%d Could not find action \"%s\" in actions list.",
                __func__,
                __LINE__,
                action_name);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    // Parse override fields.
    int full_stats_addr = 0;
    int full_meter_addr = 0;
    int full_stful_addr = 0;
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_PFE_SET,
                             &action_entry_ptr->force_stats_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_PFE_SET,
                             &action_entry_ptr->force_meter_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_PFE_SET,
                             &action_entry_ptr->force_stful_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_ADDR,
                             &action_entry_ptr->force_stats_addr);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_ADDR,
                             &action_entry_ptr->force_meter_addr);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_ADDR,
                             &action_entry_ptr->force_stful_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_FULL_ADDR,
                            &full_stats_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_FULL_ADDR,
                            &full_meter_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_FULL_ADDR,
                            &full_stful_addr);
    CHECK_ERR(err, rc, cleanup);
    action_entry_ptr->forced_full_stats_addr = full_stats_addr;
    action_entry_ptr->forced_full_meter_addr = full_meter_addr;
    action_entry_ptr->forced_full_stful_addr = full_stful_addr;

    int vliw_instruction = 0;
    int next_table = 0;
    bool default_action = false;
    err |= bf_cjson_get_int(action_format_cjson,
                            CTX_JSON_ACTION_FORMAT_VLIW_INSTRUCTION,
                            &vliw_instruction);
    err |= bf_cjson_get_int(
        action_format_cjson, CTX_JSON_ACTION_FORMAT_NEXT_TABLE, &next_table);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_ALLOWED_AS_DEFAULT_ACTION,
                             &default_action);
    CHECK_ERR(err, rc, cleanup);

    action_entry_ptr->instr = vliw_instruction;
    action_entry_ptr->next_tbl = next_table;
    action_entry_ptr->default_action = default_action;

    cJSON *immediate_fields_cjson = NULL;
    err |= bf_cjson_get_object(action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
                               &immediate_fields_cjson);
    CHECK_ERR(err, rc, cleanup);

    int number_immediate_fields_in_handle = 0;
    action_entry_ptr->act_param =
        (pipemgr_tbl_pkg_action_parameter_field_t *)(action_entry_ptr + 1);
    pipemgr_tbl_pkg_action_parameter_field_t *immediate_ptr =
        action_entry_ptr->act_param;

    cJSON *immediate_field_cjson = NULL;
    CTX_JSON_FOR_EACH(immediate_field_cjson, immediate_fields_cjson) {
      rc |= ctx_json_parse_action_immediate(
          devid, action_cjson, immediate_field_cjson, immediate_ptr);
      ++immediate_ptr;
      ++number_immediate_fields;
      ++number_immediate_fields_in_handle;
    }

    action_entry_ptr->param_count = number_immediate_fields_in_handle;
    ++number_action_handles;
  }

  action_handles_ptr->action_hdl_count = number_action_handles;
  return rc;

cleanup:
  return rc;
}

// #### Ternary Indirection Entyr Format ####

/**
 * Parses some metadata for a given ternary indirection stage table cJSON
 * structure.
 *
 * @param stage_tables_cjson The stage table's cJSON structure.
 * @param number_fields_per_stage A pointer to (max_number_stages) integers
 * to be populated with the number of fields for each action.
 * @param number_entries_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of entries for each action.
 * @param number_ways_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of ways for each action.
 * @param number_action_formats_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of action_formats for each action.
 * @param number_immediate_fields_per_stage A pointer to
 * (max_number_stages) integers to be populated with the number of
 * immediate_fields for each action.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_tind_entry_format_metadata(
    cJSON *stage_table_cjson,
    int *number_fields_per_stage,
    int *number_entries_per_stage,
    int *number_action_formats_per_stage,
    int *number_immediate_fields_per_stage) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *pack_formats_cjson = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all pack formats in this stage table.
  cJSON *pack_format_cjson = NULL;
  CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
    cJSON *entries_cjson = NULL;
    err |= bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Look at all entries in this pack_format, and parse them.
    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      cJSON *fields_cjson = NULL;
      err |= bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      *number_fields_per_stage += cJSON_GetArraySize(fields_cjson);
      ++*number_entries_per_stage;
    }
  }

  // Iterate over all action formats in this stage table.
  cJSON *action_formats_cjson = NULL;
  err |= bf_cjson_get_object(stage_table_cjson,
                             CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                             &action_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *action_format_cjson = NULL;
  CTX_JSON_FOR_EACH(action_format_cjson, action_formats_cjson) {
    cJSON *immediate_fields_cjson = NULL;
    err |= bf_cjson_get_object(action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
                               &immediate_fields_cjson);
    CHECK_ERR(err, rc, cleanup);

    *number_immediate_fields_per_stage +=
        cJSON_GetArraySize(immediate_fields_cjson);
    ++*number_action_formats_per_stage;
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the entry format for an ternary indirection field, from the cJSON
 * object representing the field in the stage table's pack format object.
 *
 * @param field_cjson The cJSON object describing the field.
 * @param field_ptr A pointer to the internal struct representing this field's
 *  entry format.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_tind_entry_format_field(
    bf_dev_id_t devid,
    cJSON *field_cjson,
    pipemgr_tbl_pkg_match_entry_field_t *field_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *field_name = NULL;
  char *source = NULL;
  char *immediate_name = NULL;
  int field_width = 0;
  int start_bit = 0;
  int lsb_mem_word_offset = 0;
  int lsb_mem_word_index = 0;
  int msb_mem_word_index = 0;
  bool enable_pfe = false;

  err = ctx_json_parse_global_name(field_cjson,
                                   CTX_JSON_TIND_ENTRY_FORMAT_GLOBAL_NAME,
                                   CTX_JSON_TIND_ENTRY_FORMAT_FIELD_NAME,
                                   &field_name);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(
      field_cjson, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE, &source);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_try_get_string(
      field_cjson, CTX_JSON_TIND_ENTRY_FORMAT_IMMEDIATE_NAME, &immediate_name);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_TIND_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_TIND_ENTRY_FORMAT_START_BIT, &start_bit);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_TIND_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                         &lsb_mem_word_offset);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_TIND_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                         &lsb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_EXACT_ENTRY_FORMAT_MSB_MEM_WORD_INDEX,
                         &msb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_bool(
      field_cjson, CTX_JSON_TIND_ENTRY_FORMAT_ENABLE_PFE, &enable_pfe);
  CHECK_ERR(err, rc, cleanup);

  field_ptr->stringindex =
      pipemgr_tbl_pkg_gen_str_index_for_field_name(devid, field_name, prof_id);
  field_ptr->field_width = field_width;
  field_ptr->fieldsb = start_bit;
  field_ptr->field_offset = lsb_mem_word_offset;
  field_ptr->memword_index[0] = lsb_mem_word_index;
  field_ptr->memword_index[1] = msb_mem_word_index;
  field_ptr->msbit = DEFAULT_PFE_POSITION;
  field_ptr->perflow = enable_pfe;

  /* This field is not applicable to ternary indirection (TIND) and should be
   * defaulted. */
  field_ptr->match_mode = TBL_PKG_FIELD_MATCHMODE_INVALID;

  if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_VERSION)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_VERSION;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_ZERO)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_ZERO;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_SPEC)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SPEC;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_SEL_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELPTR;
    /* DRV-991 : The msbit is used to figure out the bit position of the PFE
     * when encoding. PFE bit position is the MSB of the address if applicable
     */
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_ADT_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_ADTPTR;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_INSTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_INSTR;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_NEXT_TABLE)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_NXT_TBL;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_IMMEDIATE)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_IMMEDIATE;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_METER_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_METERPTR;
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_STATS_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_STATSPTR;
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_STATEFUL_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_STFLPTR;
  } else if (!strcmp(source,
                     CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_SELECTION_LENGTH)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELLEN;
  } else if (!strcmp(
                 source,
                 CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_SELECTION_LENGTH_SHIFT)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELLENSHIFT;
  } else if (!strcmp(source, CTX_JSON_TIND_ENTRY_FORMAT_SOURCE_PROXY_HASH)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_PROXYHASH;
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: source type \"%s\" is not valid.",
        __func__,
        __LINE__,
        source);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Don't actually care about immediate name.
  if (immediate_name != NULL) {
    field_ptr->immediate = 1;
  } else {
    field_ptr->immediate = 0;
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the stage table entry format from the cJSON object representing a
 * ternary indirection stage table.
 *
 * @param stage_table_cjson The cJSON object describing the ternary indirection
 * stage table.
 * @param actions_cjson The cJSON object describing the action spec in the
 * ternary indirection table. This is used to compute information that the
 * entry format requires, but ContextJSON only provides at a table level.
 * @param stage_handle_ptr A pointer to the internal struct representing this
 * stage table entry in the ternary indirection table. Notice that there can be
 * multiple stage tables per stage, differently from other tables.
 * @param action_handles_ptr A pointer to the internal struct representing
 * this stage table's action handles in the ternary indirection match table.
 *
 * FIXME: there should be only one of these per stage. Figure out how to fix
 * this for ALPM.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_tind_stage_table_entry_format(
    bf_dev_id_t devid,
    cJSON *stage_table_cjson,
    cJSON *actions_cjson,
    pipemgr_tbl_pkg_match_entry_format_t *format_ptr,
    pipemgr_tbl_pkg_action_handles_t *action_handles_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int number_fields = 0;
  int number_entries = 0;

  // Compute the memory location for the entry: it starts right after the
  // match entry format struct.
  format_ptr->entry_pkg =
      (pipemgr_tbl_pkg_match_entry_line_t *)(format_ptr + 1);

  // Parse match entry format from each way.
  cJSON *pack_formats_cjson = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all pack formats in this stage table.
  cJSON *pack_format_cjson = NULL;
  CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
    cJSON *entries_cjson = NULL;
    err |= bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Finally, look at all entries in this pack_format, and parse them.
    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      int number_fields_in_entry = 0;

      // Compute the memory location for the next entry format: from the first
      // one, skip all entries and fields seen so far.
      void *ptr = (uint8_t *)format_ptr->entry_pkg +
                  number_entries * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
                  number_fields * sizeof(pipemgr_tbl_pkg_match_entry_field_t);
      pipemgr_tbl_pkg_match_entry_line_t *entry_ptr = ptr;

      int entry_number = 0;
      err |= bf_cjson_get_int(
          entry_cjson, CTX_JSON_TIND_ENTRY_FORMAT_ENTRY_NUMBER, &entry_number);
      CHECK_ERR(err, rc, cleanup);

      entry_ptr->entry = entry_number;

      cJSON *fields_cjson = NULL;
      err |= bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Compute the memory location for the field: it starts right after the
      // entry struct.
      entry_ptr->fields =
          (pipemgr_tbl_pkg_match_entry_field_t *)(entry_ptr + 1);

      // Iterator used to go over each field in this entry.
      pipemgr_tbl_pkg_match_entry_field_t *field_ptr = entry_ptr->fields;

      cJSON *field_cjson = NULL;
      CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
        rc |= ctx_json_parse_tind_entry_format_field(
            devid, field_cjson, field_ptr, prof_id);
        CHECK_RC(rc, cleanup);

        ++number_fields;
        ++number_fields_in_entry;
        ++field_ptr;
      }

      entry_ptr->field_count = number_fields_in_entry;

      ++number_entries;
    }
  }

  format_ptr->total_fields_in_all_entries = number_fields;
  format_ptr->entry_count = number_entries;

  // Parse action format from stage table.
  cJSON *action_formats_cjson = NULL;
  err |= bf_cjson_get_object(stage_table_cjson,
                             CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                             &action_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  rc |= ctx_json_parse_action_handle_format(
      devid, action_formats_cjson, actions_cjson, action_handles_ptr, prof_id);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses a single ternary indirection table from a cJSON object.
 *
 * @param devid The device id. This is used to fetch pointers to LUTs.
 * @param table_cjosn The cJSON object describing a ternary indirection table.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_tind_entry_format(bf_dev_id_t devid,
                                                      cJSON *table_cjson,
                                                      cJSON *stage_table_cjson,
                                                      profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing ternary indirection entry format for table handle 0x%x "
      "named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *tind_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut_collision_count);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be recomputed
  // here by hand.
  int number_fields_per_stage = 0;
  int number_entries_per_stage = 0;
  int number_action_formats_per_stage = 0;
  int number_immediate_fields_per_stage = 0;

  // First pass: compute metadata about the entry formats from the context.
  rc = ctx_json_parse_tind_entry_format_metadata(
      stage_table_cjson,
      &number_fields_per_stage,
      &number_entries_per_stage,
      &number_action_formats_per_stage,
      &number_immediate_fields_per_stage);
  CHECK_RC(rc, cleanup);

  // Second pass: allocate LUT from metadata, and parse fields.
  int match_entry_mem_size = 0;
  int action_data_mem_size = 0;
  int stage = 0;
  err = bf_cjson_get_int(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);
  CHECK_ERR(err, rc, cleanup);

  // Memory required for each field, entry, way, logical table and the
  // overall match entry format structure.
  match_entry_mem_size +=
      sizeof(pipemgr_tbl_pkg_match_entry_field_t) * number_fields_per_stage;
  match_entry_mem_size +=
      sizeof(pipemgr_tbl_pkg_match_entry_line_t) * number_entries_per_stage;
  match_entry_mem_size += sizeof(pipemgr_tbl_pkg_exm_format_t);

  // Memory required for the action formats, immediate fields and the
  // overall structure for action handles.
  action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_parameter_field_t) *
                          number_immediate_fields_per_stage;
  action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_entry_field_t) *
                          number_action_formats_per_stage;
  action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_handles_t);

  // Obtain lut_ptr for this table and stage.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, stage, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr =
      pipemgr_tbl_pkg_update_lut_entry(tind_lut,
                                       lut_depth,
                                       collision_count_ptr,
                                       bj_index,
                                       table_handle,
                                       stage,
                                       match_entry_mem_size);
  if (lut_ptr == NULL) {
    LOG_ERROR("%s:%d: Could not populate ternary indirection LUT.",
              __func__,
              __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate LUT with the appropriate calculated size.
  lut_ptr->u.tind_ptr =
      PIPE_MGR_CALLOC(1, match_entry_mem_size + action_data_mem_size);
  if (lut_ptr->u.tind_ptr == NULL) {
    LOG_ERROR("%s:%d: Could not allocate ternary indirection table structure.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  pipemgr_tbl_pkg_match_entry_format_t *format_ptr = lut_ptr->u.tind_ptr;

  void *ptr = (uint8_t *)format_ptr + match_entry_mem_size;
  pipemgr_tbl_pkg_action_handles_t *action_handles_ptr = ptr;

  cJSON *actions_cjson = NULL;
  err =
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  rc = ctx_json_parse_tind_stage_table_entry_format(devid,
                                                    stage_table_cjson,
                                                    actions_cjson,
                                                    format_ptr,
                                                    action_handles_ptr,
                                                    prof_id);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

// #### Exact Match Entry Format #####

/**
 * Parses some metadata for a given exact match stage table cJSON structure.
 *
 * @param table_cjson The table's cJSON structure.
 * @param number_fields_per_stage A pointer to (max_number_stages) integers
 * to be populated with the number of fields for each action.
 * @param number_entries_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of entries for each action.
 * @param number_ways_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of ways for each action.
 * @param number_action_formats_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of action_formats for each action.
 * @param number_immediate_fields_per_stage A pointer to
 * (max_number_stages) integers to be populated with the number of
 * immediate_fields for each action.
 * @param stage_tables_for_stage A pointer to (max_number_stages) pointers,
 * each of which points to (max_number_logical_tables) cJSON* structures to
 * be populated. If this stage table has a given stage table, it must appear in
 * this array.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_exact_match_entry_format_metadata(
    bf_dev_id_t devid,
    cJSON *table_cjson,
    int *number_logical_tables_per_stage,
    int *number_fields_per_stage,
    int *number_entries_per_stage,
    int *number_ways_per_stage,
    int *number_action_formats_per_stage,
    int *number_immediate_fields_per_stage,
    int *number_stash_fields_per_stage,
    int *number_stash_entries_per_stage,
    int *number_stash_ways_per_stage,
    cJSON ***stage_tables_for_stage) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse exact match entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  int number_stage_tables = 0;
  err |= ctx_json_parse_all_match_stage_tables_for_table(
      table_cjson,
      max_number_stages * max_number_logical_tables,
      all_stage_tables_cjson,
      &number_stage_tables);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all stage tables.
  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];
    PIPE_MGR_ASSERT(stage_table_cjson != NULL);

    int stage = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);
    CHECK_ERR(err, rc, cleanup);

    // Add this logical table to the stage.
    int logical_table_id = 0;
    err |= bf_cjson_get_int(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID,
                            &logical_table_id);
    CHECK_ERR(err, rc, cleanup);

    stage_tables_for_stage[stage][logical_table_id] = stage_table_cjson;
    ++number_logical_tables_per_stage[stage];

    /*
     * Iterate over all ways in this stage table. Notice that for ATCAM,
     * we don't directly have a way; but the driver expects to parse the single
     * stage table in an ATCAM unit as a "way". If there is no "ways" in the
     * current stage table, then just make "ways" be the list of stage tables
     * in this table.
     */
    cJSON *ways_cjson = NULL;
    err |= bf_cjson_try_get_object(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_WAYS, &ways_cjson);

    cJSON all_ways_cjson;
    if (ways_cjson == NULL) {
      all_ways_cjson.next = NULL;
      all_ways_cjson.prev = NULL;
      all_ways_cjson.child = stage_table_cjson;
    } else {
      PIPE_MGR_MEMCPY(&all_ways_cjson, ways_cjson, sizeof(cJSON));
    }
    CHECK_ERR(err, rc, cleanup);

    // Iterate over all ways in this stage table.
    cJSON *way_cjson = NULL;
    CTX_JSON_FOR_EACH(way_cjson, &all_ways_cjson) {
      // Update the number of ways in this stage.
      ++number_ways_per_stage[stage];

      cJSON *pack_formats_cjson = NULL;
      err |= bf_cjson_get_object(
          way_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Iterate over all pack formats in this stage table.
      cJSON *pack_format_cjson = NULL;
      CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
        cJSON *entries_cjson = NULL;
        err |= bf_cjson_get_object(
            pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Finally, look at all entries in this pack_format, and parse them.
        cJSON *entry_cjson = NULL;
        CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
          cJSON *fields_cjson = NULL;
          err |= bf_cjson_get_object(
              entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
          CHECK_ERR(err, rc, cleanup);

          number_fields_per_stage[stage] += cJSON_GetArraySize(fields_cjson);
          ++number_entries_per_stage[stage];
        }
      }
    }

    // Iterate over all action formats in this stage table.
    cJSON *action_formats_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                               &action_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *action_format_cjson = NULL;
    CTX_JSON_FOR_EACH(action_format_cjson, action_formats_cjson) {
      cJSON *immediate_fields_cjson = NULL;
      err |= bf_cjson_get_object(action_format_cjson,
                                 CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
                                 &immediate_fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      number_immediate_fields_per_stage[stage] +=
          cJSON_GetArraySize(immediate_fields_cjson);
      ++number_action_formats_per_stage[stage];
    }

    // Get stash allocation
    cJSON *stash_cjson = NULL;
    err |= bf_cjson_try_get_object(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STASH_ALLOCATION, &stash_cjson);
    if (stash_cjson != NULL) {
      ++number_stash_ways_per_stage[stage];
      cJSON *pack_formats_cjson = NULL;
      err |= bf_cjson_get_object(
          stash_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
      CHECK_ERR(err, rc, cleanup);
      // Iterate over all pack formats in this stage table.
      cJSON *pack_format_cjson = NULL;
      CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
        cJSON *entries_cjson = NULL;
        err |= bf_cjson_get_object(
            pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Finally, look at all entries in this pack_format, and parse them.
        cJSON *entry_cjson = NULL;
        CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
          cJSON *fields_cjson = NULL;
          err |= bf_cjson_get_object(
              entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
          CHECK_ERR(err, rc, cleanup);

          number_stash_fields_per_stage[stage] +=
              cJSON_GetArraySize(fields_cjson);
          ++number_stash_entries_per_stage[stage];
        }
      }
    }
  }

cleanup:
  PIPE_MGR_FREE(all_stage_tables_cjson);
cleanup_no_free:
  return rc;
}

/**
 * Parses the entry format for an exact match field, from the cJSON object
 * representing the field in the stage table's pack format object.
 *
 * @param field_cjson The cJSON object describing the field.
 * @param match_key_fields_cjson The cJSON object describing the
 *  match key fields object in the exact match table. This is used to compute
 *  match spec information that the entry format requires, but ContextJSON
 *  only provides at a table level.
 * @param field_ptr A pointer to the internal struct representing this field's
 *  entry format.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_exact_match_entry_format_field(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *field_cjson,
    cJSON *match_key_fields_cjson,
    pipemgr_tbl_pkg_match_entry_field_t *field_ptr) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *field_name = NULL;
  char *source = NULL;
  char *immediate_name = NULL;
  char *match_mode = NULL;
  int field_width = 0;
  int start_bit = 0;
  int lsb_mem_word_offset = 0;
  int lsb_mem_word_index = 0;
  int msb_mem_word_index = 0;
  bool enable_pfe = false;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  err = ctx_json_parse_global_name(field_cjson,
                                   CTX_JSON_EXACT_ENTRY_FORMAT_GLOBAL_NAME,
                                   CTX_JSON_EXACT_ENTRY_FORMAT_FIELD_NAME,
                                   &field_name);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE, &source);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_try_get_string(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_IMMEDIATE_NAME, &immediate_name);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_MATCH_MODE, &match_mode);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_START_BIT, &start_bit);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_EXACT_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                         &lsb_mem_word_offset);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_EXACT_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                         &lsb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_EXACT_ENTRY_FORMAT_MSB_MEM_WORD_INDEX,
                         &msb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_bool(
      field_cjson, CTX_JSON_EXACT_ENTRY_FORMAT_ENABLE_PFE, &enable_pfe);
  CHECK_ERR(err, rc, cleanup);

  int spec_length = 0;
  int spec_offset = 0;

  // Find match type, spec offset and spec length from match_key_fields.
  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);
  if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_SPEC)) {
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                start_bit,
                                                field_width,
                                                &spec_length,
                                                &spec_offset,
                                                NULL,
                                                NULL);
    if (err != 0) {
      LOG_ERROR(
          "%s:%d Error processing pack_format field %s (source %s, width %d, "
          "start_bit %d) details from match_key_fields",
          __func__,
          __LINE__,
          field_name,
          source,
          field_width,
          start_bit);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  }

  field_ptr->stringindex =
      pipemgr_tbl_pkg_gen_str_index_for_field_name(devid, field_name, prof_id);
  field_ptr->field_width = field_width;
  field_ptr->fieldsb = start_bit;
  field_ptr->field_offset = lsb_mem_word_offset;
  field_ptr->memword_index[0] = lsb_mem_word_index;
  field_ptr->memword_index[1] = msb_mem_word_index;
  field_ptr->spec_len = spec_length < 8 ? 8 : spec_length;
  field_ptr->spec_start_bit = spec_offset;
  field_ptr->perflow = enable_pfe;
  field_ptr->msbit = DEFAULT_PFE_POSITION;

  if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_VERSION)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_VERSION;






  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_ZERO)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_ZERO;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_SPEC)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SPEC;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_SEL_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELPTR;
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_ADT_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_ADTPTR;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_INSTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_INSTR;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_NEXT_TABLE)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_NXT_TBL;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_IMMEDIATE)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_IMMEDIATE;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_METER_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_METERPTR;
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_STATS_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_STATSPTR;
    field_ptr->msbit = field_width - 1;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_STATEFUL_PTR)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_STFLPTR;
  } else if (!strcmp(source,
                     CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_SELECTION_LENGTH)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELLEN;
  } else if (!strcmp(
                 source,
                 CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_SELECTION_LENGTH_SHIFT)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SELLENSHIFT;
  } else if (!strcmp(source, CTX_JSON_EXACT_ENTRY_FORMAT_SOURCE_PROXY_HASH)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_PROXYHASH;
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: source type \"%s\" is not valid.",
        __func__,
        __LINE__,
        source);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  // Don't actually care about immediate name.
  if (immediate_name != NULL) {
    field_ptr->immediate = 1;
  } else {
    field_ptr->immediate = 0;
  }

  if (!strcmp(match_mode, CTX_JSON_EXACT_ENTRY_FORMAT_MATCH_MODE_S1Q0)) {
    field_ptr->match_mode = TBL_PKG_FIELD_MATCHMODE_S1Q0;
  } else if (!strcmp(match_mode, CTX_JSON_EXACT_ENTRY_FORMAT_MATCH_MODE_S0Q1)) {
    field_ptr->match_mode = TBL_PKG_FIELD_MATCHMODE_S0Q1;
  } else if (!strcmp(match_mode,
                     CTX_JSON_EXACT_ENTRY_FORMAT_MATCH_MODE_UNUSED) ||
             !strcmp(match_mode,
                     CTX_JSON_EXACT_ENTRY_FORMAT_MATCH_MODE_EXACT)) {
    // FIXME: The driver does not have an enum for these match modes yet.
    field_ptr->match_mode = TBL_PKG_FIELD_MATCHMODE_INVALID;
  } else {
    field_ptr->match_mode = TBL_PKG_FIELD_MATCHMODE_INVALID;
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: match mode \"%s\" is not valid.",
        __func__,
        __LINE__,
        match_mode);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the stage table entry format from the cJSON object representing a
 * exact stage table.
 *
 * @param stage_table_cjson The cJSON object describing the exact stage table.
 * @param match_key_fields_cjson The cJSON object describing the
 * match key fields object in the exact table. This is used to compute
 * match spec information that the entry format requires, but ContextJSON
 * only provides at a table level.
 * @param actions_cjson The cJSON object describing the
 * action spec in the exact table. This is used to compute information that
 * the entry format requires, but ContextJSON only provides at a table level.
 * @param stage_handle_ptr A pointer to the internal struct representing this
 * stage table entry in the exact table. Notice that there can be multiple
 * stage tables per stage, differently from other tables.
 * @param action_handles_ptr A pointer to the internal struct representing
 * this stage table's action handles in the exact match table.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_exact_match_stage_table_entry_format(
    bf_dev_id_t devid,
    const char *table_name,
    int stage,
    cJSON *stage_table_cjson,
    cJSON *match_key_fields_cjson,
    cJSON *actions_cjson,
    pipemgr_tbl_pkg_exm_stage_hdl_format_t *stage_handle_ptr,
    pipemgr_tbl_pkg_action_handles_t *action_handles_ptr,
    uint8_t logical_table_id,
    void **end_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  stage_handle_ptr->stage_handle = logical_table_id;
  stage_handle_ptr->way_pkg =
      (pipemgr_tbl_pkg_way_format_t *)(stage_handle_ptr + 1);

  /*
   * Iterate over all ways in this stage table. Notice that for ATCAM,
   * we don't directly have a way; but the driver expects to parse the single
   * stage table in an ATCAM unit as a "way". If there is no "ways" in the
   * current stage table, then just make "ways" be the list of stage tables
   * in this table.
   */
  cJSON *ways_cjson = NULL;
  bf_cjson_try_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_WAYS, &ways_cjson);

  cJSON all_ways_cjson;
  if (ways_cjson == NULL) {
    all_ways_cjson.next = NULL;
    all_ways_cjson.prev = NULL;
    all_ways_cjson.child = stage_table_cjson;
  } else {
    PIPE_MGR_MEMCPY(&all_ways_cjson, ways_cjson, sizeof(cJSON));
  }

  int number_fields = 0;
  int number_entries = 0;
  int number_ways = 0;

  // Parse match entry format from each way.
  cJSON *way_cjson = NULL;
  CTX_JSON_FOR_EACH(way_cjson, &all_ways_cjson) {
    int number_entries_in_way = 0;
    int number_fields_in_way = 0;

    // Compute the pointer to this way: from the first one, skip all ways,
    // entries and fields structs seen so far.
    void *ptr = (uint8_t *)stage_handle_ptr->way_pkg +
                number_ways * (sizeof(pipemgr_tbl_pkg_way_format_t) +
                               sizeof(pipemgr_tbl_pkg_match_entry_format_t)) +
                number_entries * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
                number_fields * sizeof(pipemgr_tbl_pkg_match_entry_field_t);
    pipemgr_tbl_pkg_way_format_t *way_ptr = ptr;

    // Compute the entry format pointer: it is right after the way struct in
    // memory.
    way_ptr->entry_format =
        (pipemgr_tbl_pkg_match_entry_format_t *)(way_ptr + 1);

    int way = 0;
    bf_cjson_try_get_int(way_cjson, CTX_JSON_WAY_WAY_NUMBER, &way);
    way_ptr->way = way;

    pipemgr_tbl_pkg_match_entry_format_t *entry_format_ptr =
        way_ptr->entry_format;

    // Compute the entry memory location within this entry format: it is right
    // after the entry format struct.
    entry_format_ptr->entry_pkg =
        (pipemgr_tbl_pkg_match_entry_line_t *)(entry_format_ptr + 1);

    // TODO: If multiple pack formats are ever supported, this code must
    // change.
    cJSON *pack_formats_cjson = NULL;
    err = bf_cjson_get_object(
        way_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Iterate over all pack formats in this stage table.
    cJSON *pack_format_cjson = NULL;
    CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
      cJSON *entries_cjson = NULL;
      err = bf_cjson_get_object(
          pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Finally, look at all entries in this pack_format, and parse them.
      cJSON *entry_cjson = NULL;
      CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
        uint8_t number_fields_in_entry = 0;

        // Next entry pointer: from the first entry in this entry format,
        // skip all entries and fields seen in this entry.
        ptr =
            (uint8_t *)entry_format_ptr->entry_pkg +
            number_entries_in_way * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
            number_fields_in_way * sizeof(pipemgr_tbl_pkg_match_entry_field_t);
        pipemgr_tbl_pkg_match_entry_line_t *entry_ptr = ptr;

        // Compute the field pointer: it is right after the entry struct.
        entry_ptr->fields =
            (pipemgr_tbl_pkg_match_entry_field_t *)(entry_ptr + 1);

        int entry_number = 0;
        err = bf_cjson_get_int(entry_cjson,
                               CTX_JSON_EXACT_ENTRY_FORMAT_ENTRY_NUMBER,
                               &entry_number);
        CHECK_ERR(err, rc, cleanup);
        entry_ptr->entry = entry_number;

        cJSON *fields_cjson = NULL;
        err = bf_cjson_get_object(
            entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Iterator to go over all fields in the entry.
        pipemgr_tbl_pkg_match_entry_field_t *field_ptr = entry_ptr->fields;

        // Finally, iterate through fields and populate them.
        cJSON *field_cjson = NULL;
        CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
          rc = ctx_json_parse_exact_match_entry_format_field(
              devid, prof_id, field_cjson, match_key_fields_cjson, field_ptr);
          if (rc != PIPE_SUCCESS) {
            LOG_ERROR(
                "Dev %d table %s profile %d stage %d lt %d, way %d entry %d: "
                "error parsing entry format fields %s",
                devid,
                table_name,
                prof_id,
                stage,
                logical_table_id,
                way,
                entry_number,
                pipe_str_err(rc));
          }
          CHECK_RC(rc, cleanup);

          ++number_fields;
          ++number_fields_in_way;
          ++number_fields_in_entry;
          ++field_ptr;
        }

        entry_ptr->field_count = number_fields_in_entry;
        ++number_entries;
        ++number_entries_in_way;
      }
    }
    entry_format_ptr->entry_count = number_entries_in_way;
    entry_format_ptr->total_fields_in_all_entries = number_fields_in_way;
    ++number_ways;
  }

  // Update metadata for this stage handle (logical table).
  stage_handle_ptr->way_count = number_ways;
  stage_handle_ptr->total_fields_in_all_ways = number_fields;
  stage_handle_ptr->total_entries_in_all_ways = number_entries;

  // Parse action format from stage table.
  cJSON *action_formats_cjson = NULL;
  err = bf_cjson_get_object(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                            &action_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  rc = ctx_json_parse_action_handle_format(
      devid, action_formats_cjson, actions_cjson, action_handles_ptr, prof_id);
  CHECK_RC(rc, cleanup);

  // Stash
  cJSON *stash_cjson = NULL;
  bf_cjson_try_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_STASH_ALLOCATION, &stash_cjson);

  int number_stash_fields = 0;
  int number_stash_entries = 0;
  int number_stash_ways = 0;

  if (stash_cjson != NULL) {
    ++number_stash_ways;
    int number_stash_entries_in_way = 0;
    int number_stash_fields_in_way = 0;

    /* Stash way pkg is right after the exm tbl way pkg */
    stage_handle_ptr->stash_way_pkg =
        (pipemgr_tbl_pkg_way_format_t
             *)((uint8_t *)stage_handle_ptr->way_pkg +
                number_ways * (sizeof(pipemgr_tbl_pkg_way_format_t) +
                               sizeof(pipemgr_tbl_pkg_match_entry_format_t)) +
                number_entries * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
                number_fields * sizeof(pipemgr_tbl_pkg_match_entry_field_t));
    pipemgr_tbl_pkg_way_format_t *stash_way_ptr =
        stage_handle_ptr->stash_way_pkg;

    stash_way_ptr->entry_format =
        (pipemgr_tbl_pkg_match_entry_format_t *)(stash_way_ptr + 1);

    int way = 0;
    stash_way_ptr->way = way;

    pipemgr_tbl_pkg_match_entry_format_t *stash_entry_format_ptr =
        stash_way_ptr->entry_format;

    // Compute the entry memory location within this entry format: it is right
    // after the entry format struct.
    stash_entry_format_ptr->entry_pkg =
        (pipemgr_tbl_pkg_match_entry_line_t *)(stash_entry_format_ptr + 1);

    cJSON *pack_formats_cjson = NULL;
    err = bf_cjson_get_object(
        stash_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Iterate over all pack formats in this stage table.
    cJSON *pack_format_cjson = NULL;
    CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
      cJSON *entries_cjson = NULL;
      err = bf_cjson_get_object(
          pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Finally, look at all entries in this pack_format, and parse them.
      cJSON *entry_cjson = NULL;
      CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
        uint8_t number_stash_fields_in_entry = 0;

        // Next entry pointer: from the first entry in this entry format,
        // skip all entries and fields seen in this entry.
        void *ptr = (uint8_t *)stash_entry_format_ptr->entry_pkg +
                    number_stash_entries_in_way *
                        sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
                    number_stash_fields_in_way *
                        sizeof(pipemgr_tbl_pkg_match_entry_field_t);
        pipemgr_tbl_pkg_match_entry_line_t *stash_entry_ptr = ptr;

        // Compute the field pointer: it is right after the entry struct.
        stash_entry_ptr->fields =
            (pipemgr_tbl_pkg_match_entry_field_t *)(stash_entry_ptr + 1);

        int entry_number = 0;
        err = bf_cjson_get_int(entry_cjson,
                               CTX_JSON_EXACT_ENTRY_FORMAT_ENTRY_NUMBER,
                               &entry_number);
        CHECK_ERR(err, rc, cleanup);
        stash_entry_ptr->entry = entry_number;

        cJSON *fields_cjson = NULL;
        err = bf_cjson_get_object(
            entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Iterator to go over all fields in the entry.
        pipemgr_tbl_pkg_match_entry_field_t *field_ptr =
            stash_entry_ptr->fields;

        // Finally, iterate through fields and populate them.
        cJSON *field_cjson = NULL;
        CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
          rc = ctx_json_parse_exact_match_entry_format_field(
              devid, prof_id, field_cjson, match_key_fields_cjson, field_ptr);
          CHECK_RC(rc, cleanup);

          ++number_stash_fields;
          ++number_stash_fields_in_way;
          ++number_stash_fields_in_entry;
          ++field_ptr;
        }

        stash_entry_ptr->field_count = number_stash_fields_in_entry;
        ++number_stash_entries;
        ++number_stash_entries_in_way;
      }
    }
    stash_entry_format_ptr->entry_count = number_stash_entries_in_way;
    stash_entry_format_ptr->total_fields_in_all_entries =
        number_stash_fields_in_way;
  }
  // Update stash metadata for this stage handle (logical table).
  stage_handle_ptr->stash_way_count = number_stash_ways;
  stage_handle_ptr->stash_total_fields_in_all_ways = number_stash_fields;
  stage_handle_ptr->stash_total_entries_in_all_ways = number_stash_entries;

  *end_ptr =
      (uint8_t *)stage_handle_ptr->way_pkg +
      number_ways * (sizeof(pipemgr_tbl_pkg_way_format_t) +
                     sizeof(pipemgr_tbl_pkg_match_entry_format_t)) +
      number_entries * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
      number_fields * sizeof(pipemgr_tbl_pkg_match_entry_field_t) +
      number_stash_ways * (sizeof(pipemgr_tbl_pkg_way_format_t) +
                           sizeof(pipemgr_tbl_pkg_match_entry_format_t)) +
      number_stash_entries * sizeof(pipemgr_tbl_pkg_match_entry_line_t) +
      number_stash_fields * sizeof(pipemgr_tbl_pkg_match_entry_field_t);
  return rc;

cleanup:
  return rc;
}

/**
 * Parses a single exact match table from a cJSON object.
 *
 * @param devid The device id. This is used to fetch pointers to LUTs.
 * @param table_cjosn The cJSON object describing a exact table.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_exact_match_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err;
  pipe_status_t rc = PIPE_SUCCESS;

  int stage = -1;
  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup_no_free);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing exact match entry format for table handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *exm_match_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut_collision_count);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be recomputed
  // here by hand.
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  // Allocate metadata.
  int *number_logical_tables_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_fields_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_entries_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_ways_per_stage = PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_action_formats_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_immediate_fields_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_stash_fields_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_stash_entries_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_stash_ways_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  cJSON ***stage_tables_for_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(cJSON **));

  if (number_logical_tables_per_stage == NULL ||
      number_fields_per_stage == NULL || number_entries_per_stage == NULL ||
      number_ways_per_stage == NULL ||
      number_action_formats_per_stage == NULL ||
      number_immediate_fields_per_stage == NULL ||
      stage_tables_for_stage == NULL || number_stash_fields_per_stage == NULL ||
      number_stash_entries_per_stage == NULL ||
      number_stash_ways_per_stage == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for metadata to parse exact match "
        "table entry format.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }
  for (int i = 0; i < max_number_stages; ++i) {
    stage_tables_for_stage[i] =
        PIPE_MGR_CALLOC(max_number_logical_tables, sizeof(cJSON *));
    if (stage_tables_for_stage[i] == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for metadata to parse exact match "
          "table entry format.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  // First pass: compute metadata about the entry formats from the context.
  rc = ctx_json_parse_exact_match_entry_format_metadata(
      devid,
      table_cjson,
      number_logical_tables_per_stage,
      number_fields_per_stage,
      number_entries_per_stage,
      number_ways_per_stage,
      number_action_formats_per_stage,
      number_immediate_fields_per_stage,
      number_stash_fields_per_stage,
      number_stash_entries_per_stage,
      number_stash_ways_per_stage,
      stage_tables_for_stage);
  CHECK_RC(rc, cleanup);

  pipemgr_tbl_pkg_lut_t *lut_ptr = NULL;

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);
  cJSON *actions_cjson = NULL;
  err =
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Second pass: allocate LUT from metadata, and parse fields.
  for (stage = 0; stage < max_number_stages; ++stage) {
    if (number_logical_tables_per_stage[stage] == 0) {
      continue;
    }

    int match_entry_mem_size = 0;
    int stash_entry_mem_size = 0;
    int action_data_mem_size = 0;

    // Memory required for each field, entry, way, logical table and the
    // overall match entry format structure.
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_match_entry_field_t) *
                            number_fields_per_stage[stage];
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_match_entry_line_t) *
                            number_entries_per_stage[stage];
    match_entry_mem_size += (sizeof(pipemgr_tbl_pkg_way_format_t) +
                             sizeof(pipemgr_tbl_pkg_match_entry_format_t)) *
                            number_ways_per_stage[stage];
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_exm_stage_hdl_format_t) *
                            number_logical_tables_per_stage[stage];
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_exm_format_t);

    // Add stash
    stash_entry_mem_size += sizeof(pipemgr_tbl_pkg_match_entry_field_t) *
                            number_stash_fields_per_stage[stage];
    stash_entry_mem_size += sizeof(pipemgr_tbl_pkg_match_entry_line_t) *
                            number_stash_entries_per_stage[stage];
    match_entry_mem_size += (sizeof(pipemgr_tbl_pkg_way_format_t) +
                             sizeof(pipemgr_tbl_pkg_match_entry_format_t)) *
                            number_stash_ways_per_stage[stage];

    // Memory required for the action formats, immediate fields and the
    // overall structure for action handles.
    action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_parameter_field_t) *
                            number_immediate_fields_per_stage[stage];
    action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_entry_field_t) *
                            number_action_formats_per_stage[stage];
    action_data_mem_size += sizeof(pipemgr_tbl_pkg_action_handles_t);

    // Obtain lut_ptr for this table and stage.
    uint16_t bj_index =
        bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, stage, 0);

    lut_ptr = pipemgr_tbl_pkg_update_lut_entry(
        exm_match_lut,
        lut_depth,
        collision_count_ptr,
        bj_index,
        table_handle,
        stage,
        match_entry_mem_size + stash_entry_mem_size);
    if (lut_ptr == NULL) {
      LOG_ERROR("%s:%d: Could not populate exact match table LUT.",
                __func__,
                __LINE__);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }

    // Allocate LUT with the appropriate calculated size.
    lut_ptr->u.exm_ptr = PIPE_MGR_CALLOC(
        1, match_entry_mem_size + stash_entry_mem_size + action_data_mem_size);
    if (lut_ptr->u.exm_ptr == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for exact match table structure.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    pipemgr_tbl_pkg_exm_format_t *format_ptr = lut_ptr->u.exm_ptr;
    format_ptr->stg_hdl =
        (pipemgr_tbl_pkg_exm_stage_hdl_format_t *)(format_ptr + 1);

    format_ptr->stage_handle_count = number_logical_tables_per_stage[stage];
    pipemgr_tbl_pkg_exm_stage_hdl_format_t *stage_handle_ptr =
        format_ptr->stg_hdl;

    void *ptr =
        (uint8_t *)format_ptr + match_entry_mem_size + stash_entry_mem_size;
    pipemgr_tbl_pkg_action_handles_t *action_handle_ptr = ptr;

    for (int logical_table_id = 0; logical_table_id < max_number_logical_tables;
         ++logical_table_id) {
      if (stage_tables_for_stage[stage][logical_table_id] == NULL) {
        continue;
      }

      cJSON *stage_table_cjson =
          stage_tables_for_stage[stage][logical_table_id];

      rc = ctx_json_parse_exact_match_stage_table_entry_format(
          devid,
          name,
          stage,
          stage_table_cjson,
          match_key_fields_cjson,
          actions_cjson,
          stage_handle_ptr,
          action_handle_ptr,
          logical_table_id,
          &ptr,
          prof_id);
      CHECK_RC(rc, cleanup);

      stage_handle_ptr = ptr;
    }
  }

cleanup:
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Dev %d Failed to parse table %s (0x%x) stage %d in profile %d err %s",
        devid,
        name,
        table_handle,
        stage,
        prof_id,
        pipe_str_err(rc));
  }
  if (number_logical_tables_per_stage != NULL) {
    PIPE_MGR_FREE(number_logical_tables_per_stage);
  }
  if (number_fields_per_stage != NULL) {
    PIPE_MGR_FREE(number_fields_per_stage);
  }
  if (number_entries_per_stage != NULL) {
    PIPE_MGR_FREE(number_entries_per_stage);
  }
  if (number_ways_per_stage != NULL) {
    PIPE_MGR_FREE(number_ways_per_stage);
  }
  if (number_action_formats_per_stage != NULL) {
    PIPE_MGR_FREE(number_action_formats_per_stage);
  }
  if (number_immediate_fields_per_stage != NULL) {
    PIPE_MGR_FREE(number_immediate_fields_per_stage);
  }
  if (number_stash_fields_per_stage != NULL) {
    PIPE_MGR_FREE(number_stash_fields_per_stage);
  }
  if (number_stash_entries_per_stage != NULL) {
    PIPE_MGR_FREE(number_stash_entries_per_stage);
  }
  if (number_stash_ways_per_stage != NULL) {
    PIPE_MGR_FREE(number_stash_ways_per_stage);
  }
  if (stage_tables_for_stage != NULL) {
    for (int i = 0; i < max_number_stages; ++i) {
      if (stage_tables_for_stage[i] != NULL) {
        PIPE_MGR_FREE(stage_tables_for_stage[i]);
      }
    }
    PIPE_MGR_FREE(stage_tables_for_stage);
  }
cleanup_no_free:
  return rc;
}

// #### Ternary Table Entry Format ####

static int mau_range_field_compare(const void *a, const void *b) {
  const pipemgr_tbl_pkg_rangetbl_mau_field_t *field_a = a;
  const pipemgr_tbl_pkg_rangetbl_mau_field_t *field_b = b;

  if (field_a->startbit < field_b->startbit) {
    return -1;
  } else if (field_a->startbit > field_b->startbit) {
    return 1;
  }
  return 0;
}

static pipe_status_t ctx_json_parse_range_table_entry_format_field(
    cJSON *stage_tables_cjson,
    char *field_to_find,
    int field_start_bit,
    int field_bit_width,
    pipemgr_tbl_pkg_rangetbl_mau_field_t *mau_field_ptr,
    int *number_mau_range_fields_in_field) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  pipemgr_tbl_pkg_rangetbl_mau_field_t *first_mau_field_ptr = mau_field_ptr;

  // Only read range expansion info from the first stage
  cJSON *stage_table_cjson = NULL;
  err = bf_cjson_get_first(stage_tables_cjson, &stage_table_cjson);
  CHECK_ERR(err, rc, cleanup);

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
  CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
    cJSON *fields_cjson = NULL;
    err = bf_cjson_get_object(
        entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *field_cjson = NULL;
    CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
      char *source = NULL;
      char *field_name = NULL;
      err = bf_cjson_get_string(
          field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
      CHECK_ERR(err, rc, cleanup);
      err = ctx_json_parse_global_name(field_cjson,
                                       CTX_JSON_TERN_ENTRY_FORMAT_GLOBAL_NAME,
                                       CTX_JSON_TERN_ENTRY_FORMAT_FIELD_NAME,
                                       &field_name);
      CHECK_ERR(err, rc, cleanup);

      // If the field name does not match or field is not a range, skip it.
      if (strcmp(field_name, field_to_find) ||
          strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
        continue;
      }

      /* 4-bit range uses 8 bits of Word0 and 8 bits of Word1 to encode four
       * bits of range key.  These 8 bits are consecutive in Word0/1 but are
       * published in the context json as two separate fields, the start-bit
       * would be the same since it is using the same section of the key but the
       * lsb_mem_word_offset would be different.  The upper nibble is published
       * with "is_duplicate" set to true and can be skipped here since we are
       * counting the number of key sections and both fields refer to the same
       * key section. */
      cJSON *range_cjson = NULL;
      bool is_duplicate = false;
      err = bf_cjson_get_object(
          field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE, &range_cjson);
      CHECK_ERR(err, rc, cleanup);

      err = bf_cjson_get_bool(range_cjson,
                              CTX_JSON_TERN_ENTRY_FORMAT_RANGE_IS_DUPLICATE,
                              &is_duplicate);
      CHECK_ERR(err, rc, cleanup);

      if (is_duplicate) {
        continue;
      }

      int start_bit = 0;
      int field_width = 0;
      err = bf_cjson_get_int(
          field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_START_BIT, &start_bit);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_int(
          field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
      CHECK_ERR(err, rc, cleanup);

      /* Even if the field name matched we could still be talking about a
       * different key field, compare the start-bit to the range passed in by
       * the caller.  One example of multiple key fields with the same name is
       * range matching two slices of the same field. */
      if (start_bit < field_start_bit ||
          start_bit >= (field_start_bit + field_bit_width))
        continue;

      mau_field_ptr->startbit = start_bit;
      mau_field_ptr->fieldwidth = field_width;
      ++*number_mau_range_fields_in_field;
      ++mau_field_ptr;
    }
  }

  // NOTE: The driver expect the mau fields to be sorted by start_bit, but
  // this is not guaranteed in the ContextJSON (and it shouldn't be - it is
  // JSON!). We sort those here.
  qsort(first_mau_field_ptr,
        *number_mau_range_fields_in_field,
        sizeof(pipemgr_tbl_pkg_rangetbl_mau_field_t),
        mau_range_field_compare);

  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_range_tables_entry_format_metadata(
    cJSON *table_cjson,
    int *total_number_range_fields,
    int *total_number_mau_range_fields) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *match_attributes_cjson = NULL;
  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Parse number range fields from match key fields.
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *match_type = NULL;
    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_RANGE)) {
      ++*total_number_range_fields;
    }
  }

  // Parse number mau range fields from pack format in the stage tables.
  cJSON *stage_tables_cjson = NULL;
  err = bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    cJSON *pack_formats_cjson = NULL;
    err = bf_cjson_get_object(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                              &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    // There should be only one pack format.
    cJSON *pack_format_cjson = NULL;
    err = bf_cjson_get_first(pack_formats_cjson, &pack_format_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *entries_cjson = NULL;
    err = bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Go through all entries and fields to find mau fields.
    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      cJSON *fields_cjson = NULL;
      err = bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      field_cjson = NULL;
      CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
        char *source = NULL;
        err = bf_cjson_get_string(
            field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
        CHECK_ERR(err, rc, cleanup);

        // Field has to be a range field, and we don't count the duplicates.
        if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
          cJSON *range_cjson = NULL;
          bool is_duplicate = false;
          err = bf_cjson_get_object(
              field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE, &range_cjson);
          CHECK_ERR(err, rc, cleanup);

          err = bf_cjson_get_bool(range_cjson,
                                  CTX_JSON_TERN_ENTRY_FORMAT_RANGE_IS_DUPLICATE,
                                  &is_duplicate);
          CHECK_ERR(err, rc, cleanup);

          if (!is_duplicate) {
            ++*total_number_mau_range_fields;
          }
        }
      }
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses a single range table from a ternary table cJSON object.
 *
 * @param devid The device id. This is used to fetch pointers to LUTs.
 * @param table_cjson The cJSON object describing a ternary table.
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_range_table_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing range tables entry format for table handle 0x%x "
      "named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *rangetbl_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut_collision_count);

  /* Number of entries in match_key_fields which are range match. */
  int total_number_range_fields = 0;
  /* Number of non-duplicate range entries in the pack_format-entry-fields. */
  int total_number_mau_range_fields = 0;
  rc = ctx_json_parse_range_tables_entry_format_metadata(
      table_cjson, &total_number_range_fields, &total_number_mau_range_fields);
  CHECK_RC(rc, cleanup);

  // Memory required for range fields and range mau fields.
  int mem_size = 0;
  mem_size += sizeof(pipemgr_tbl_pkg_rangetbl_t);
  mem_size +=
      sizeof(pipemgr_tbl_pkg_rangetbl_field_t) * total_number_range_fields;
  mem_size += sizeof(pipemgr_tbl_pkg_rangetbl_mau_field_t) *
              total_number_mau_range_fields;

  // Obtain lut_ptr for this stage.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr =
      pipemgr_tbl_pkg_update_lut_entry(rangetbl_lut,
                                       lut_depth,
                                       collision_count_ptr,
                                       bj_index,
                                       table_handle,
                                       0,
                                       0);
  if (lut_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not populate range table match LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate the internal structs with the appropriate calculated size.
  lut_ptr->u.rangetbl_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.rangetbl_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for range tables internal "
        "structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  // Start parsing JSON contents.
  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_tables_cjson = NULL;
  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  pipemgr_tbl_pkg_rangetbl_t *format_ptr = lut_ptr->u.rangetbl_ptr;
  format_ptr->fieldcount = total_number_range_fields;
  format_ptr->fields = (pipemgr_tbl_pkg_rangetbl_field_t *)(format_ptr + 1);

  pipemgr_tbl_pkg_rangetbl_field_t *field_ptr = format_ptr->fields;
  field_ptr->maufields =
      (pipemgr_tbl_pkg_rangetbl_mau_field_t *)(field_ptr + 1);

  int number_range_fields = 0;
  int number_mau_range_fields = 0;

  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *field_name = NULL;
    int field_width = 0;
    int field_start_bit = 0;
    if (use_global_name) {
      bf_cjson_try_get_string(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_GLOBAL_NAME, &field_name);
    }
    if (!field_name || *field_name == '\0') {
      err = bf_cjson_get_string(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
      CHECK_ERR(err, rc, cleanup);
    }
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &field_width);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &field_start_bit);
    CHECK_ERR(err, rc, cleanup);

    int spec_length = 0;
    int spec_offset = 0;
    bool is_range = false;
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_start_bit,
                                                field_width,
                                                &spec_length,
                                                &spec_offset,
                                                NULL,
                                                &is_range);
    CHECK_ERR(err, rc, cleanup);

    if (is_range) {
      int range_start_bit = 0;
      int range_bit_width = 0;
      err = bf_cjson_get_int(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &range_start_bit);
      CHECK_ERR(err, rc, cleanup);
      err = bf_cjson_get_int(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &range_bit_width);
      CHECK_ERR(err, rc, cleanup);
      void *ptr =
          (uint8_t *)format_ptr->fields +
          number_range_fields * sizeof(pipemgr_tbl_pkg_rangetbl_field_t) +
          number_mau_range_fields *
              sizeof(pipemgr_tbl_pkg_rangetbl_mau_field_t);
      field_ptr = ptr;
      field_ptr->maufields =
          (pipemgr_tbl_pkg_rangetbl_mau_field_t *)(field_ptr + 1);
      pipemgr_tbl_pkg_rangetbl_mau_field_t *mau_field_ptr =
          field_ptr->maufields;

      int number_mau_range_fields_in_field = 0;
      rc = ctx_json_parse_range_table_entry_format_field(
          stage_tables_cjson,
          field_name,
          range_start_bit,
          range_bit_width,
          mau_field_ptr,
          &number_mau_range_fields_in_field);
      CHECK_RC(rc, cleanup);

      field_ptr->maufieldcount = number_mau_range_fields_in_field;

      field_ptr->param_startbit = spec_offset;
      field_ptr->param_fieldwidth = spec_length < 8 ? 8 : spec_length;
      field_ptr->fieldname_str_index =
          pipemgr_tbl_pkg_gen_str_index_for_field_name(
              devid, field_name, prof_id);

      ++number_range_fields;
      number_mau_range_fields += number_mau_range_fields_in_field;
    }
  }
  return rc;
cleanup:
  return rc;
}

/**
 * Parses some metadata for a given ternary match stage table cJSON structure.
 *
 * @param stage_tables_cjson The stage table's cJSON structure.
 * @param number_fields_per_stage A pointer to (max_number_stages) integers
 * to be populated with the number of fields for each action.
 * @param number_entries_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of entries for each action.
 * @param number_range_masks_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of const_tuples for each action.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_ternary_match_entry_format_metadata(
    cJSON *stage_tables_cjson,
    int *number_fields_per_stage,
    int *number_entries_per_stage,
    int *number_range_masks_per_stage,
    int max_number_stages) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // Parse number of fields and entries in this stage.
  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    int stage = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);

    cJSON *pack_formats_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                               &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);
    if (stage >= max_number_stages) {
      LOG_ERROR("%s:%d Stage number %d exceeds max number of allowed stages %d",
                __func__,
                __LINE__,
                stage,
                max_number_stages);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }

    // Iterate over all pack formats in this stage table.
    cJSON *pack_format_cjson = NULL;
    CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
      cJSON *entries_cjson = NULL;
      err |= bf_cjson_get_object(
          pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Look at all entries in this pack_format.
      cJSON *entry_cjson = NULL;
      CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
        cJSON *fields_cjson = NULL;
        err |= bf_cjson_get_object(
            entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
        CHECK_ERR(err, rc, cleanup);

        number_fields_per_stage[stage] += cJSON_GetArraySize(fields_cjson);
        ++number_entries_per_stage[stage];
      }
    }

    // Now, parse the number of range masks per stage.
    cJSON *memory_resource_allocation_cjson = NULL;
    err |=
        bf_cjson_try_get_object(stage_table_cjson,
                                CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                                &memory_resource_allocation_cjson);
    if (!memory_resource_allocation_cjson) {
      continue;
    }

    cJSON *many_memory_units_and_vpns_cjson = NULL;
    err |= bf_cjson_get_object(
        memory_resource_allocation_cjson,
        CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
        &many_memory_units_and_vpns_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *memory_units_and_vpns_cjson =
        many_memory_units_and_vpns_cjson->child;
    cJSON *memory_units_cjson = NULL;
    err |= bf_cjson_get_object(memory_units_and_vpns_cjson,
                               CTX_JSON_MEMORY_UNITS_AND_VPNS_MEMORY_UNITS,
                               &memory_units_cjson);
    CHECK_ERR(err, rc, cleanup);
    number_range_masks_per_stage[stage] +=
        cJSON_GetArraySize(memory_units_cjson);
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the entry format for a ternary match field, from the cJSON object
 * representing the field in the ternary stage table's pack format object.
 *
 * @param field_cjson The cJSON object describing the field.
 * @param match_key_fields_cjson The cJSON object describing the
 *  match key fields object in the ternary table. This is used to compute
 *  match spec information that the entry format requires, but ContextJSON
 *  only provides at a table level.
 * @param field_ptr A pointer to the internal struct representing this field's
 *  entry format.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_ternary_match_entry_format_field(
    bf_dev_id_t devid,
    cJSON *field_cjson,
    cJSON *match_key_fields_cjson,
    pipemgr_tbl_pkg_tern_entry_field_t *field_ptr,
    rmt_dev_profile_info_t *prof_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *field_name = NULL;
  char *source = NULL;
  int field_width = 0;
  int start_bit = 0;
  int lsb_mem_word_offset = 0;
  int lsb_mem_word_index = 0;
  int msb_mem_word_index = 0;
  cJSON *range_cjson = NULL;

  // These values are ignored by any field that is not range, so we default
  // them.
  int range_type = 0, range_nibble_offset = 0;
  bool range_hi_byte = false;

  err = ctx_json_parse_global_name(field_cjson,
                                   CTX_JSON_TERN_ENTRY_FORMAT_GLOBAL_NAME,
                                   CTX_JSON_TERN_ENTRY_FORMAT_FIELD_NAME,
                                   &field_name);
  CHECK_ERR(err, rc, cleanup);

  err = bf_cjson_get_string(
      field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(
      field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_START_BIT, &start_bit);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_TERN_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                         &lsb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_TERN_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                         &lsb_mem_word_offset);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_int(field_cjson,
                         CTX_JSON_TERN_ENTRY_FORMAT_MSB_MEM_WORD_INDEX,
                         &msb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);

  int spec_length = 0;
  int spec_offset = 0;

  // Find match type, spec offset and spec length from match_key_fields.
  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);
  err = ctx_json_parse_spec_details_for_field(use_global_name,
                                              match_key_fields_cjson,
                                              field_name,
                                              start_bit,
                                              field_width,
                                              &spec_length,
                                              &spec_offset,
                                              NULL,
                                              NULL);
  if (err != 0) {
    LOG_ERROR(
        "%s:%d Error processing pack_format field %s (source %s, width %d, "
        "start_bit %d) details from match_key_fields",
        __func__,
        __LINE__,
        field_name,
        source,
        field_width,
        start_bit);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  field_ptr->stringindex = pipemgr_tbl_pkg_gen_str_index_for_field_name(
      devid, field_name, prof_info->profile_id);
  field_ptr->bitwidth = field_width;
  field_ptr->startbit = start_bit;
  field_ptr->lsbmemwordoffset = lsb_mem_word_offset;
  field_ptr->memword_index[0] = lsb_mem_word_index;
  field_ptr->memword_index[1] = msb_mem_word_index;
  field_ptr->src_len = spec_length < 8 ? 8 : spec_length;
  field_ptr->srcoffset = spec_offset;
  field_ptr->range_type = range_type;
  field_ptr->range_hi_byte = range_hi_byte;
  field_ptr->range_nibble_offset = range_nibble_offset;

  if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_VERSION)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_VERSION;
  } else if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_ZERO)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_ZERO;
  } else if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_SPEC)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_SPEC;
  } else if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_PARITY)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_PARITY;
  } else if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_PAYLOAD)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_PAYLOAD;
  } else if (!strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
    field_ptr->location = TBL_PKG_TERN_FIELD_LOCATION_RANGE;

    // Parse range type and range hi byte.
    err = bf_cjson_get_object(
        field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE, &range_cjson);
    CHECK_ERR(err, rc, cleanup);

    err = bf_cjson_get_int(
        range_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE_TYPE, &range_type);
    CHECK_ERR(err, rc, cleanup);
    bf_cjson_try_get_int(range_cjson,
                         CTX_JSON_TERN_ENTRY_FORMAT_RANGE_NIBBLE_OFFSET,
                         &range_nibble_offset);

    field_ptr->range_type = range_type;
    field_ptr->range_nibble_offset = range_nibble_offset;
    field_ptr->range_hi_byte =
        ((lsb_mem_word_offset - 1) % BYTE_WIDTH) >= range_type;
  } else {
    field_ptr->location = TBL_PKG_FIELD_SOURCE_INVALID;
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: source type \"%s\" is not valid.",
        __func__,
        __LINE__,
        source);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses the stage table entry format from the cJSON object representing a
 * ternary stage table.
 *
 * @param stage_table_cjson The cJSON object describing the ternary stage
 * table.
 * @param match_key_fields_cjson The cJSON object describing the
 *  match key fields object in the ternary table. This is used to compute
 *  match spec information that the entry format requires, but ContextJSON
 *  only provides at a table level.
 * @param format_ptr A pointer to the internal struct representing this stage
 *  table entry in the ternary table.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_ternary_match_stage_table_entry_format(
    bf_dev_id_t devid,
    cJSON *stage_table_cjson,
    cJSON *match_key_fields_cjson,
    pipemgr_tbl_pkg_tern_tbl_t *format_ptr,
    rmt_dev_profile_info_t *prof_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // Iterator that will go over the entries to be populated.
  pipemgr_tbl_pkg_tern_entry_t *entry_format_ptr = format_ptr->entry;

  cJSON *pack_formats_cjson = NULL;
  err = bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all pack formats in this stage table.
  cJSON *pack_format_cjson = NULL;
  CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
    cJSON *entries_cjson = NULL;
    err = bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Look at all entries in this pack_format, and parse them.
    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      uint8_t number_fields_in_entry = 0;

      int entry_number = 0;
      err = bf_cjson_get_int(
          entry_cjson, CTX_JSON_TERN_ENTRY_FORMAT_ENTRY_NUMBER, &entry_number);
      CHECK_ERR(err, rc, cleanup);

      entry_format_ptr->entry = entry_number;
      entry_format_ptr->field =
          (pipemgr_tbl_pkg_tern_entry_field_t *)(entry_format_ptr + 1);

      pipemgr_tbl_pkg_tern_entry_field_t *field_ptr = entry_format_ptr->field;

      cJSON *fields_cjson = NULL;
      err = bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Finally, parse each particular field in this entry.
      cJSON *field_cjson = NULL;
      CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
        rc = ctx_json_parse_ternary_match_entry_format_field(
            devid, field_cjson, match_key_fields_cjson, field_ptr, prof_info);
        CHECK_RC(rc, cleanup);

        ++number_fields_in_entry;
        ++field_ptr;
      }

      entry_format_ptr->entryfieldcount = number_fields_in_entry;

      void *ptr =
          (uint8_t *)entry_format_ptr->field +
          number_fields_in_entry * sizeof(pipemgr_tbl_pkg_tern_entry_field_t);
      entry_format_ptr = ptr;
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the range mask entry format from a cJSON object representing a
 * ternary
 * stage table.
 *
 * @param stage_table_cjson The cJSON object describing the ternary stage
 * table.
 * @param format_ptr A pointer to the internal struct representing this stage
 *  table entry in the ternary table.
 *
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_ternary_match_range_masks_entry_format(
    cJSON *stage_table_cjson, pipemgr_tbl_pkg_tern_tbl_t *format_ptr) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // To parse range masks, we must look at the memory resource allocation
  // object.
  cJSON *memory_resource_allocation_cjson = NULL;
  err |=
      bf_cjson_try_get_object(stage_table_cjson,
                              CTX_JSON_STAGE_TABLE_MEMORY_RESOURCE_ALLOCATION,
                              &memory_resource_allocation_cjson);
  if (!memory_resource_allocation_cjson) {
    goto cleanup_no_free;
  }

  pipemgr_tbl_pkg_tern_range_mask_t *range_format = format_ptr->rangeformat;

  cJSON *many_memory_units_and_vpns_cjson = NULL;
  err |= bf_cjson_get_object(
      memory_resource_allocation_cjson,
      CTX_JSON_MEMORY_RESOURCE_ALLOCATION_MEMORY_UNITS_AND_VPNS,
      &many_memory_units_and_vpns_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  int depth_memory_units = cJSON_GetArraySize(many_memory_units_and_vpns_cjson);

  cJSON *memory_units_and_vpns_cjson = many_memory_units_and_vpns_cjson->child;
  cJSON *memory_units_cjson = NULL;
  err |= bf_cjson_get_object(memory_units_and_vpns_cjson,
                             CTX_JSON_MEMORY_UNITS_AND_VPNS_MEMORY_UNITS,
                             &memory_units_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  int width_memory_units = cJSON_GetArraySize(memory_units_cjson);

  int **memory_units = PIPE_MGR_CALLOC(depth_memory_units, sizeof(int *));
  if (memory_units == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for metadata to parse range masks.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  for (int i = 0; i < depth_memory_units; ++i) {
    memory_units[i] = PIPE_MGR_CALLOC(width_memory_units, sizeof(int));
    if (memory_units[i] == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for metadata to parse range "
          "masks.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  int row = 0;
  int col = 0;
  CTX_JSON_FOR_EACH(memory_units_and_vpns_cjson,
                    many_memory_units_and_vpns_cjson) {
    col = 0;
    err |= bf_cjson_get_object(memory_units_and_vpns_cjson,
                               CTX_JSON_MEMORY_UNITS_AND_VPNS_MEMORY_UNITS,
                               &memory_units_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *memory_unit_cjson = NULL;
    CTX_JSON_FOR_EACH(memory_unit_cjson, memory_units_cjson) {
      memory_units[row][col] = memory_unit_cjson->valueint;
      ++col;
    }

    // Makes sure that all rows and columns have all entries. If this
    // ever triggers, need to fill them up with 0xff in memids.
    PIPE_MGR_ASSERT(col == width_memory_units);
    ++row;
  }
  PIPE_MGR_ASSERT(row == depth_memory_units);

  for (col = 0; col < width_memory_units; ++col) {
    int current_word_index = width_memory_units - col - 1;

    // Parsing the range mask is tricky. For this particular word index,
    // we iterate over all fields in the entry format. For the ones that
    // are ranges, we bitwise OR them their masks together.
    cJSON *pack_formats_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                               &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *pack_format_cjson = NULL;
    err |= bf_cjson_get_first(pack_formats_cjson, &pack_format_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *entries_cjson = NULL;
    err |= bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    uint64_t range_mask = 0;

    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      cJSON *fields_cjson = NULL;
      err |= bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      cJSON *field_cjson = NULL;
      CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
        char *source = NULL;
        err |= bf_cjson_get_string(
            field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE, &source);
        CHECK_ERR(err, rc, cleanup);

        // If this field is not a range, skip it.
        if (strcmp(source, CTX_JSON_TERN_ENTRY_FORMAT_SOURCE_RANGE)) {
          continue;
        }

        cJSON *range_cjson = NULL;
        int lsb_mem_word_index = 0;
        int lsb_mem_word_offset = 0;
        int range_type = 0;

        err |= bf_cjson_get_object(
            field_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE, &range_cjson);
        err |= bf_cjson_get_int(field_cjson,
                                CTX_JSON_TERN_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                                &lsb_mem_word_index);
        err |= bf_cjson_get_int(field_cjson,
                                CTX_JSON_TERN_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                                &lsb_mem_word_offset);
        CHECK_ERR(err, rc, cleanup);

        err |= bf_cjson_get_int(
            range_cjson, CTX_JSON_TERN_ENTRY_FORMAT_RANGE_TYPE, &range_type);
        CHECK_ERR(err, rc, cleanup);

        // If the word index matches, update the range mask accordingly.
        if (lsb_mem_word_index == current_word_index) {
          int lsb_range_offset =
              lsb_mem_word_offset - (lsb_mem_word_offset - 1) % range_type;
          range_mask |= (uint64_t)((1 << range_type) - 1) << lsb_range_offset;
        }
      }
    }

    // Update the range mask format fields with the computed info.
    range_format->word_index = current_word_index;
    range_format->mask = range_mask;

    for (row = 0; row < depth_memory_units; ++row) {
      range_format->memids[row] = memory_units[row][col];
    }

    // Mark other TCAMs as invalid.
    for (row = depth_memory_units; row < TBL_PKG_MAX_DEPTH_TCAM_UNITS; ++row) {
      range_format->memids[row] = 0xff;
    }
    ++range_format;
  }

cleanup:
  if (memory_units != NULL) {
    for (int i = 0; i < depth_memory_units; ++i) {
      if (memory_units[i] != NULL) {
        PIPE_MGR_FREE(memory_units[i]);
      }
    }
    PIPE_MGR_FREE(memory_units);
  }
cleanup_no_free:
  return rc;
}

/**
 * Parses a single ternary match table from a cJSON object.
 *
 * While parsing a ternary stage table, if a ternary indirection stage table
 * is indicated, it is also parsed accordingly.
 *
 * @param devid The device id. This is used to fetch pointers to LUTs.
 * @param table_cjosn The cJSON object describing a ternary table.
 * @return A pipe_status_t integer with return code.
 */
static pipe_status_t ctx_json_parse_ternary_match_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup_no_free);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing ternary match entry format for table handle 0x%x "
      "named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *tern_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut_collision_count);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be recomputed
  // here by hand.
  int max_number_stages = dev_info->num_active_mau;

  // Allocate metadata.
  int *number_fields_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_entries_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  int *number_range_masks_per_stage =
      PIPE_MGR_CALLOC(max_number_stages, sizeof(int));
  if (number_fields_per_stage == NULL || number_entries_per_stage == NULL ||
      number_range_masks_per_stage == NULL) {
    goto cleanup;
  }

  // Start parsing JSON contents.
  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *stage_tables_cjson = NULL;
  err = bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  // First pass: compute metadata about the entry formats from the context.
  rc = ctx_json_parse_ternary_match_entry_format_metadata(
      stage_tables_cjson,
      number_fields_per_stage,
      number_entries_per_stage,
      number_range_masks_per_stage,
      max_number_stages);
  CHECK_RC(rc, cleanup);

  // Second pass: build LUT from metadata, and parse fields.
  cJSON *stage_table_cjson = NULL;
  int stage = -1;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    int match_entry_mem_size = 0;
    int range_masks_mem_size = 0;
    stage = -1;
    err = bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);
    CHECK_ERR(err, rc, cleanup);

    // Memory required for each field, entry, way, logical table and the
    // overall match entry format structure.
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_tern_entry_field_t) *
                            number_fields_per_stage[stage];
    match_entry_mem_size +=
        sizeof(pipemgr_tbl_pkg_tern_entry_t) * number_entries_per_stage[stage];
    match_entry_mem_size += sizeof(pipemgr_tbl_pkg_tern_tbl_t);

    range_masks_mem_size += sizeof(pipemgr_tbl_pkg_tern_range_mask_t) *
                            number_range_masks_per_stage[stage];

    // Obtain lut_ptr for this stage.
    uint16_t bj_index =
        bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, stage, 0);

    pipemgr_tbl_pkg_lut_t *lut_ptr =
        pipemgr_tbl_pkg_update_lut_entry(tern_lut,
                                         lut_depth,
                                         collision_count_ptr,
                                         bj_index,
                                         table_handle,
                                         stage,
                                         match_entry_mem_size);
    if (lut_ptr == NULL) {
      LOG_ERROR(
          "%s:%d: Could not populate ternary match LUT.", __func__, __LINE__);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }

    // Allocate the internal structs with the appropriate calculated size.
    lut_ptr->u.tern_ptr =
        PIPE_MGR_CALLOC(1, match_entry_mem_size + range_masks_mem_size);
    if (lut_ptr->u.tern_ptr == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for ternary table internal "
          "structure.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    // Populate common fields with metadata already computed.
    pipemgr_tbl_pkg_tern_tbl_t *format_ptr = lut_ptr->u.tern_ptr;

    format_ptr->entrycount = number_entries_per_stage[stage];
    format_ptr->totalfields = number_fields_per_stage[stage];
    format_ptr->rangemaskcount = number_range_masks_per_stage[stage];
    void *ptr = (uint8_t *)format_ptr + match_entry_mem_size;
    format_ptr->rangeformat = ptr;
    format_ptr->entry = (pipemgr_tbl_pkg_tern_entry_t *)(format_ptr + 1);

    // Parse the ternary entry format and range masks.
    rc = ctx_json_parse_ternary_match_stage_table_entry_format(
        devid,
        stage_table_cjson,
        match_key_fields_cjson,
        format_ptr,
        prof_info);
    CHECK_RC(rc, cleanup);
    rc = ctx_json_parse_ternary_match_range_masks_entry_format(
        stage_table_cjson, format_ptr);
    CHECK_RC(rc, cleanup);

    // Parse the ternary indirection stage table, if any.
    cJSON *ternary_indirection_stage_table_cjson = NULL;
    bf_cjson_try_get_object(
        stage_table_cjson,
        CTX_JSON_STAGE_TABLE_TERNARY_INDIRECTION_STAGE_TABLE,
        &ternary_indirection_stage_table_cjson);

    if (ternary_indirection_stage_table_cjson != NULL) {
      rc |= ctx_json_parse_tind_entry_format(
          devid, table_cjson, ternary_indirection_stage_table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    }
  }

cleanup:
  if (number_fields_per_stage != NULL) {
    PIPE_MGR_FREE(number_fields_per_stage);
  }
  if (number_entries_per_stage != NULL) {
    PIPE_MGR_FREE(number_entries_per_stage);
  }
  if (number_range_masks_per_stage != NULL) {
    PIPE_MGR_FREE(number_range_masks_per_stage);
  }
  if (rc != PIPE_SUCCESS) {
    LOG_ERROR(
        "Dev %d, failed to parse table %s (0x%x) stage %d in profile %d from "
        "context.json, status %s",
        devid,
        name,
        table_handle,
        prof_id,
        stage,
        pipe_str_err(rc));
  }
cleanup_no_free:
  return rc;
}

// #### Action Data Entry Format ####

/**
 * Given an action handle, finds the index in the action_handles array that
 * corresponds to that handle. If there is none, associates a handle to a new
 * index. This function does not fail. It asserts if there are no more free
 * indices in the array (which means we are trying to parse more actions than
 * the maximum number of actions in this action data table).
 *
 * @param number_action The number of actions in the action data table.
 * @param action_handles The array that maps indices to action handles.
 * @param action_handle The action handle that we are looking for in the
 * action_handles array.
 *
 * @return The index of the action handle in the array. Does not fail.
 */
static inline int get_action_index_for_action_handle(int number_actions,
                                                     int *action_handles,
                                                     int action_handle) {
  int first_free_index = -1;

  for (int i = 0; i < number_actions; ++i) {
    // Action handle had already been set.
    if (action_handles[i] == action_handle) {
      return i;
    }

    if (action_handles[i] == 0 && first_free_index == -1) {
      first_free_index = i;
    }
  }

  // Must have found some free index; otherwise, this is an internal problem,
  // or the ContextJSON has the wrong number of actions in the top-level table
  // structure.
  PIPE_MGR_ASSERT(first_free_index != -1);
  action_handles[first_free_index] = action_handle;
  return first_free_index;
}

/**
 * Parses some metadata for a given action data stage table cJSON structure.
 *
 * @param stage_tables_cjson The stage table's cJSON structure.
 * @param number_action The number of actions in this action data table.
 * @param action_handles A pointer to (number_action) integers to be populated
 * with action handles for each action. This serves as a mapping from indices
 * to action handles.
 * @param number_stages_per_action A pointer to (number_action) integers to be
 * populated with the number of stages for each action.
 * @param number_fields_per_action A pointer to (number_action) integers to be
 * populated with the number of fields for each action.
 * @param number_entries_per_action A pointer to (number_action) integers to
 * be
 * populated with the number of entries for each action.
 * @param number_const_tuples_per_action A pointer to (number_action) integers
 * to be populated with the number of const_tuples for each action.
 * @param stage_tables_for_action A pointer to (number_action) pointers, each
 * of
 * which points to (max_number_stages) cJSON* structures to be populated.
 * If this stage table has a given action, it must appear in that array.
 *
 * TODO: Notice that this assumes that each stage can only have one stage
 * table. This is true as of now, but may change in the future.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_action_data_entry_format_metadata(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *stage_tables_cjson,
    int number_actions,
    int *action_handles,
    int *number_stages_per_action,
    int *number_fields_per_action,
    int *number_entries_per_action,
    int *number_const_tuples_per_action,
    cJSON ***stage_tables_for_action) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    int stage = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);

    cJSON *pack_formats_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                               &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Iterate over all pack formats in this stage table.
    cJSON *pack_format_cjson = NULL;
    CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
      int action_handle = 0;
      err |= bf_cjson_get_handle(devid,
                                 prof_id,
                                 pack_format_cjson,
                                 CTX_JSON_PACK_FORMAT_ACTION_HANDLE,
                                 &action_handle);
      CHECK_ERR(err, rc, cleanup);

      // This function will set action_handles appropriately if needed.
      int action_index = get_action_index_for_action_handle(
          number_actions, action_handles, action_handle);
      PIPE_MGR_ASSERT(action_index < number_actions);

      cJSON *entries_cjson = NULL;
      err |= bf_cjson_get_object(
          pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Finally, look at all entries in this pack_format, and parse them.
      cJSON *entry_cjson = NULL;
      CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
        stage_tables_for_action[action_index][stage] = stage_table_cjson;
        ++number_stages_per_action[action_index];

        cJSON *fields_cjson = NULL;
        err |= bf_cjson_get_object(
            entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
        CHECK_ERR(err, rc, cleanup);

        cJSON *field_cjson = NULL;
        CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
          cJSON *const_tuples_cjson = NULL;
          err |= bf_cjson_try_get_object(
              field_cjson,
              CTX_JSON_ACTION_DATA_ENTRY_FORMAT_CONST_TUPLES,
              &const_tuples_cjson);
          if (const_tuples_cjson != NULL) {
            number_const_tuples_per_action[action_index] +=
                cJSON_GetArraySize(const_tuples_cjson);
          }

          ++number_fields_per_action[action_index];
        }
        ++number_entries_per_action[action_index];
      }
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parses the entry format for a given field's cJSON structure and the action
 * spec.
 *
 * @param devid The current device's id.
 * @param field_cjson This field's cJSON structure.
 * @param action_cjson The action spec for the current action.
 * @param field_ptr The pointer to the internal structure corresponding to
 * this field.
 * @param number_const_tuples_in_field_ret A pointer to an integer that will
 * be set to the number of constant tuples in this field.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_action_data_entry_format_field(
    bf_dev_id_t devid,
    cJSON *field_cjson,
    cJSON *action_cjson,
    pipemgr_tbl_pkg_act_fn_entry_field_t *field_ptr,
    int *number_const_tuples_in_field_ret,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // General fields that will be parsed.
  char *field_name = NULL;
  char *source = NULL;
  bool is_mod_field_conditionally_mask = false;
  bool is_mod_field_conditionally_value = false;
  char *mod_field_conditionally_mask_field_name = NULL;
  int field_width = 0;
  int start_bit = 0;
  int lsb_mem_word_offset = 0;
  int lsb_mem_word_index = 0;

  err |= bf_cjson_get_string(
      field_cjson, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_FIELD_NAME, &field_name);
  err |= bf_cjson_try_get_bool(
      field_cjson,
      CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_MASK,
      &is_mod_field_conditionally_mask);
  err |= bf_cjson_try_get_bool(
      field_cjson,
      CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_VALUE,
      &is_mod_field_conditionally_value);
  err |= bf_cjson_try_get_string(
      field_cjson,
      CTX_JSON_ACTION_DATA_ENTRY_FORMAT_MOD_FIELD_CONDITIONALLY_MASK_FIELD_NAME,
      &mod_field_conditionally_mask_field_name);
  err |= bf_cjson_get_string(
      field_cjson, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_SOURCE, &source);
  err |= bf_cjson_get_int(
      field_cjson, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
  err |= bf_cjson_get_int(
      field_cjson, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_START_BIT, &start_bit);
  err |= bf_cjson_get_int(field_cjson,
                          CTX_JSON_ACTION_DATA_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                          &lsb_mem_word_offset);
  err |= bf_cjson_get_int(field_cjson,
                          CTX_JSON_ACTION_DATA_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                          &lsb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);

  // Find spec offset and spec length from the actions list.
  int spec_width = 0;
  int spec_offset = 0;
  err |= ctx_json_parse_action_spec_details_for_field(
      action_cjson, field_name, &spec_width, &spec_offset);
  CHECK_ERR(err, rc, cleanup);
  field_ptr->source_offset = spec_offset;
  field_ptr->source_width = spec_width;

  field_ptr->stringindex =
      pipemgr_tbl_pkg_gen_str_index_for_field_name(devid, field_name, prof_id);
  field_ptr->field_width = field_width;
  field_ptr->shift = start_bit;
  field_ptr->field_offset = lsb_mem_word_offset;
  field_ptr->word_index = lsb_mem_word_index;
  field_ptr->is_mod_field_conditionally_mask = is_mod_field_conditionally_mask;
  field_ptr->is_mod_field_conditionally_value =
      is_mod_field_conditionally_value;
  if (field_ptr->is_mod_field_conditionally_mask) {
    /* For conditionally modified fields, the width of the field has to be one
     * byte or less
     */
    PIPE_MGR_ASSERT(spec_width <= 8);
  }
  if (field_ptr->is_mod_field_conditionally_value) {
    if (!mod_field_conditionally_mask_field_name) {
      LOG_ERROR(
          "%s:%d Invalid ContextJSON format: missing mask field name for "
          "conditional field \"%s\"",
          __func__,
          __LINE__,
          field_name);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
    err |= ctx_json_parse_action_spec_details_for_field(
        action_cjson,
        mod_field_conditionally_mask_field_name,
        &spec_width,
        &spec_offset);
    CHECK_ERR(err, rc, cleanup);
    field_ptr->mask_offset = spec_offset;
    field_ptr->mask_width = spec_width;
  }

  if (!strcmp(source, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_SOURCE_VERSION)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_VERSION;
  } else if (!strcmp(source, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_SOURCE_ZERO)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_ZERO;
  } else if (!strcmp(source, CTX_JSON_ACTION_DATA_ENTRY_FORMAT_SOURCE_SPEC)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_SPEC;
  } else if (!strcmp(source,
                     CTX_JSON_ACTION_DATA_ENTRY_FORMAT_SOURCE_CONSTANT)) {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_CONSTANT;
  } else {
    field_ptr->source = TBL_PKG_FIELD_SOURCE_INVALID;
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: source type \"%s\" is not valid.",
        __func__,
        __LINE__,
        source);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  int number_const_tuples_in_field = 0;

  // The constant tuples will show up in memory right after this field's
  // struct.
  field_ptr->constvalues =
      (pipemgr_tbl_pkg_act_fn_field_constvalues_t *)(field_ptr + 1);

  cJSON *const_tuples_cjson = NULL;
  err |= bf_cjson_try_get_object(field_cjson,
                                 CTX_JSON_ACTION_DATA_ENTRY_FORMAT_CONST_TUPLES,
                                 &const_tuples_cjson);

  if (const_tuples_cjson != NULL) {
    // Iterator used to go over the constant tuples in the internal structure.
    pipemgr_tbl_pkg_act_fn_field_constvalues_t *const_values_ptr =
        field_ptr->constvalues;

    cJSON *const_tuple_cjson = NULL;
    CTX_JSON_FOR_EACH(const_tuple_cjson, const_tuples_cjson) {
      int dest_start = 0;
      int dest_width = 0;
      double const_value = 0;
      err |= bf_cjson_get_int(
          const_tuple_cjson, CTX_JSON_CONST_TUPLE_DEST_START, &dest_start);
      err |= bf_cjson_get_int(
          const_tuple_cjson, CTX_JSON_CONST_TUPLE_DEST_WIDTH, &dest_width);
      err |= bf_cjson_get_double(
          const_tuple_cjson, CTX_JSON_CONST_TUPLE_VALUE, &const_value);

      const_values_ptr->dststart = dest_start;
      const_values_ptr->dstwidth = dest_width;
      const_values_ptr->constvalue = (uint32_t)const_value;

      ++const_values_ptr;
      ++number_const_tuples_in_field;
      ++field_ptr->constvalue_count;
    }
  }

  *number_const_tuples_in_field_ret = number_const_tuples_in_field;
  return rc;

cleanup:
  return rc;
}

static pipe_status_t ctx_json_parse_action_data_action_entry_format(
    bf_dev_id_t devid,
    int action_handle,
    cJSON **stage_tables,
    cJSON *actions_cjson,
    pipemgr_tbl_pkg_act_fn_handle_t *action_function_handle_ptr,
    void **end_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

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
    LOG_ERROR("%s:%d: Could not find action handle 0x%x in action spec.",
              __func__,
              __LINE__,
              action_handle);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  int number_entries = 0;
  int number_fields = 0;
  int number_const_tuples = 0;
  int number_stages = 0;

  // The stage format will show up in memory right after this action function
  // handle's struct.
  action_function_handle_ptr->stageformat =
      (pipemgr_tbl_pkg_act_fn_stage_format_t *)(action_function_handle_ptr + 1);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;

  for (int stage = 0; stage < max_number_stages; ++stage) {
    // This stage does not have a stage table for this action; skip it.
    if (stage_tables[stage] == NULL) {
      continue;
    }

    // Compute the next stage format: starting from the first one, skip all
    // stage formats, entries, fields and const tuples used so far.
    pipemgr_tbl_pkg_act_fn_stage_format_t *stage_format_ptr =
        (pipemgr_tbl_pkg_act_fn_stage_format_t
             *)((uint8_t *)action_function_handle_ptr->stageformat +
                number_stages * sizeof(pipemgr_tbl_pkg_act_fn_stage_format_t) +
                number_entries * sizeof(pipemgr_tbl_pkg_act_fn_entry_t) +
                number_fields * sizeof(pipemgr_tbl_pkg_act_fn_entry_field_t) +
                number_const_tuples *
                    sizeof(pipemgr_tbl_pkg_act_fn_field_constvalues_t));

    int number_fields_in_stage = 0;
    int number_entries_in_stage = 0;
    int number_const_tuples_in_stage = 0;

    stage_format_ptr->stage = stage;
    stage_format_ptr->entry =
        (pipemgr_tbl_pkg_act_fn_entry_t *)(stage_format_ptr + 1);

    cJSON *stage_table_cjson = stage_tables[stage];
    cJSON *pack_formats_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                               &pack_formats_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Iterate over all pack formats in this stage table.
    cJSON *pack_format_cjson = NULL;
    CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
      int pack_format_action_handle = 0;
      err |= bf_cjson_get_handle(devid,
                                 prof_id,
                                 pack_format_cjson,
                                 CTX_JSON_PACK_FORMAT_ACTION_HANDLE,
                                 &pack_format_action_handle);
      CHECK_ERR(err, rc, cleanup);

      if (pack_format_action_handle != action_handle) {
        // Not the right pack format; try the next one.
        continue;
      }

      cJSON *entries_cjson = NULL;
      err |= bf_cjson_get_object(
          pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
      CHECK_ERR(err, rc, cleanup);

      // Finally, look at all entries in this pack_format, and parse them.
      cJSON *entry_cjson = NULL;
      CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
        int number_fields_in_entry = 0;
        int number_const_tuples_in_entry = 0;

        // Next entry format: skip all entries, fields and const tuples seen
        // so far.
        void *ptr =
            (uint8_t *)stage_format_ptr->entry +
            number_entries_in_stage * sizeof(pipemgr_tbl_pkg_act_fn_entry_t) +
            number_fields_in_stage *
                sizeof(pipemgr_tbl_pkg_act_fn_entry_field_t) +
            number_const_tuples_in_stage *
                sizeof(pipemgr_tbl_pkg_act_fn_field_constvalues_t);
        pipemgr_tbl_pkg_act_fn_entry_t *entry_format_ptr = ptr;

        int entry_number = 0;
        err |= bf_cjson_get_int(entry_cjson,
                                CTX_JSON_ACTION_DATA_ENTRY_FORMAT_ENTRY_NUMBER,
                                &entry_number);
        CHECK_ERR(err, rc, cleanup);
        entry_format_ptr->entry = entry_number;

        entry_format_ptr->field =
            (pipemgr_tbl_pkg_act_fn_entry_field_t *)(entry_format_ptr + 1);

        cJSON *fields_cjson = NULL;
        err |= bf_cjson_get_object(
            entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
        CHECK_ERR(err, rc, cleanup);

        cJSON *field_cjson = NULL;
        CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
          int number_const_tuples_in_field = 0;
          pipemgr_tbl_pkg_act_fn_entry_field_t *field_ptr;

          // Compute the current field ptr.
          ptr = (uint8_t *)entry_format_ptr->field +
                number_fields_in_entry *
                    sizeof(pipemgr_tbl_pkg_act_fn_entry_field_t) +
                number_const_tuples_in_entry *
                    sizeof(pipemgr_tbl_pkg_act_fn_field_constvalues_t);
          field_ptr = ptr;

          rc |= ctx_json_parse_action_data_entry_format_field(
              devid,
              field_cjson,
              action_cjson,
              field_ptr,
              &number_const_tuples_in_field,
              prof_id);
          CHECK_RC(rc, cleanup);

          number_const_tuples += number_const_tuples_in_field;
          number_const_tuples_in_stage += number_const_tuples_in_field;
          number_const_tuples_in_entry += number_const_tuples_in_field;

          ++number_fields;
          ++number_fields_in_stage;
          ++number_fields_in_entry;
        }

        // Update metadata.
        entry_format_ptr->entryfieldcount = number_fields_in_entry;
        entry_format_ptr->totalconsttuples = number_const_tuples_in_entry;

        ++number_entries;
        ++number_entries_in_stage;
      }

      // Do not loop over pack formats: there is only one for each action.
      break;
    }

    stage_format_ptr->totalfields = number_fields_in_stage;
    stage_format_ptr->entrycount = number_entries_in_stage;
    stage_format_ptr->totalconsttuples = number_const_tuples_in_stage;

    ++number_stages;
  }

  action_function_handle_ptr->actfunchandle = action_handle;
  action_function_handle_ptr->stagecount = number_stages;
  action_function_handle_ptr->totalconsttuples = number_const_tuples;
  action_function_handle_ptr->totalfields = number_fields;
  action_function_handle_ptr->totalentries = number_entries;

  // Mark the end of the memory used by this action.
  *end_ptr =
      (uint8_t *)action_function_handle_ptr->stageformat +
      number_stages * sizeof(pipemgr_tbl_pkg_act_fn_stage_format_t) +
      number_entries * sizeof(pipemgr_tbl_pkg_act_fn_entry_t) +
      number_fields * sizeof(pipemgr_tbl_pkg_act_fn_entry_field_t) +
      number_const_tuples * sizeof(pipemgr_tbl_pkg_act_fn_field_constvalues_t);
  return rc;

cleanup:
  return rc;
}

/**
 * Parse the action data entry format for a given cJSON table. This function
 * popules the adt LUT, one action at a time. The Context JSON publishes this
 * information per stage, and the internal structure requires it per action;
 * this function makes sure that the mapping is correctly done.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_action_data_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int table_handle = 0;
  char *name = NULL;
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup_no_free);

  LOG_DBG(
      "%s:%d: Parsing action data entry format for table handle 0x%x "
      "named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *adt_lut = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut_collision_count);

  cJSON *stage_tables_cjson = NULL;
  err |= bf_cjson_get_object(
      table_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be recomputed
  // here by hand.
  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup_no_free);

  int number_actions = cJSON_GetArraySize(actions_cjson);

  cJSON ***stage_tables_for_action =
      PIPE_MGR_CALLOC(number_actions, sizeof(cJSON **));
  if (stage_tables_for_action == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for metadata used to parse action "
        "data tables.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;

  for (int i = 0; i < number_actions; ++i) {
    stage_tables_for_action[i] =
        PIPE_MGR_CALLOC(max_number_stages, sizeof(cJSON *));
    if (stage_tables_for_action[i] == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for metadata used to parse "
          "action "
          "data tables.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup_stage_tables_for_action;
    }
  }

  int *action_handles = PIPE_MGR_CALLOC(number_actions, sizeof(int));
  int *number_stages_per_action = PIPE_MGR_CALLOC(number_actions, sizeof(int));
  int *number_entries_per_action = PIPE_MGR_CALLOC(number_actions, sizeof(int));
  int *number_fields_per_action = PIPE_MGR_CALLOC(number_actions, sizeof(int));
  int *number_const_tuples_per_action =
      PIPE_MGR_CALLOC(number_actions, sizeof(int));
  if (action_handles == NULL || number_stages_per_action == NULL ||
      number_entries_per_action == NULL || number_fields_per_action == NULL ||
      number_const_tuples_per_action == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for metadata used to parse action "
        "data tables.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  rc |= ctx_json_parse_action_data_entry_format_metadata(
      devid,
      prof_id,
      stage_tables_cjson,
      number_actions,
      action_handles,
      number_stages_per_action,
      number_fields_per_action,
      number_entries_per_action,
      number_const_tuples_per_action,
      stage_tables_for_action);
  CHECK_RC(rc, cleanup);

  // Compute size to allocate.
  int total_number_stages = 0;
  int total_number_entries = 0;
  int total_number_fields = 0;
  int total_number_const_tuples = 0;

  for (int i = 0; i < number_actions; ++i) {
    total_number_stages += number_stages_per_action[i];
    total_number_entries += number_entries_per_action[i];
    total_number_fields += number_fields_per_action[i];
    total_number_const_tuples += number_const_tuples_per_action[i];
  }

  int mem_size = 0;
  mem_size += sizeof(pipemgr_tbl_pkg_adt_t);
  mem_size += sizeof(pipemgr_tbl_pkg_act_fn_handle_t) * number_actions;
  mem_size +=
      sizeof(pipemgr_tbl_pkg_act_fn_stage_format_t) * total_number_stages;
  mem_size += sizeof(pipemgr_tbl_pkg_act_fn_entry_t) * total_number_entries;
  mem_size +=
      sizeof(pipemgr_tbl_pkg_act_fn_entry_field_t) * total_number_fields;
  mem_size += sizeof(pipemgr_tbl_pkg_act_fn_field_constvalues_t) *
              total_number_const_tuples;

  // Obtain lut_ptr for this table and stage.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr = pipemgr_tbl_pkg_update_lut_entry(
      adt_lut, lut_depth, collision_count_ptr, bj_index, table_handle, 0, 0);
  if (lut_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not populate action data table LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate LUT with the appropriate calculated size.
  lut_ptr->u.adt_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.adt_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for action data table structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  pipemgr_tbl_pkg_adt_t *adt_ptr = lut_ptr->u.adt_ptr;
  adt_ptr->act_fn_hdl_count = number_actions;

  adt_ptr->hdl = (pipemgr_tbl_pkg_act_fn_handle_t *)(adt_ptr + 1);
  pipemgr_tbl_pkg_act_fn_handle_t *action_function_handle_ptr = adt_ptr->hdl;

  // Loop through actions and their stage tables, parsing each action.
  for (int i = 0; i < number_actions; ++i) {
    // An action handle of 0 indicates this action was not used.
    if (action_handles[i] == 0) {
      continue;
    }

    void *ptr = NULL;
    rc |= ctx_json_parse_action_data_action_entry_format(
        devid,
        action_handles[i],
        stage_tables_for_action[i],
        actions_cjson,
        action_function_handle_ptr,
        &ptr,
        prof_id);
    CHECK_RC(rc, cleanup);

    // FIXME: Remove ptr.
    // Make the next action function handle start wherever the last entry
    // finished.
    action_function_handle_ptr = ptr;
  }

cleanup:
  if (action_handles != NULL) {
    PIPE_MGR_FREE(action_handles);
  }
  if (number_entries_per_action != NULL) {
    PIPE_MGR_FREE(number_entries_per_action);
  }
  if (number_stages_per_action != NULL) {
    PIPE_MGR_FREE(number_stages_per_action);
  }
  if (number_fields_per_action != NULL) {
    PIPE_MGR_FREE(number_fields_per_action);
  }
  if (number_const_tuples_per_action != NULL) {
    PIPE_MGR_FREE(number_const_tuples_per_action);
  }
cleanup_stage_tables_for_action:
  if (stage_tables_for_action != NULL) {
    for (int i = 0; i < number_actions; ++i) {
      if (stage_tables_for_action[i] != NULL) {
        PIPE_MGR_FREE(stage_tables_for_action[i]);
      }
    }
    PIPE_MGR_FREE(stage_tables_for_action);
  }
cleanup_no_free:
  return rc;
}

/* --Match Spec and Action Spec formatting details for Logging / Printing-- */

/**
 * Parse the match spec for a given cJSON table. This function populates an
 * entry in the match spec LUT.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_match_spec_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, rmt_dev_profile_info_t *prof_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  profile_id_t prof_id = prof_info->profile_id;

  // Match spec LUT.
  pipemgr_tbl_pkg_lut_t *match_spec_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut_collision_count);

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing match spec format for table handle 0x%x named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  // Use only table handle for producing the hash.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);
  pipemgr_tbl_pkg_lut_t *lut_ptr =
      pipemgr_tbl_pkg_update_lut_entry(match_spec_lut,
                                       lut_depth,
                                       collision_count_ptr,
                                       bj_index,
                                       table_handle,
                                       0,
                                       0);
  if (lut_ptr == NULL) {
    LOG_ERROR("%s:%d: Could not populate match spec LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Compute amount of memory needed for LUT entry.
  int total_fields = cJSON_GetArraySize(match_key_fields_cjson);
  int to_allocate = sizeof(pipemgr_tbl_pkg_spec_t) +
                    (sizeof(pipemgr_tbl_pkg_spec_field_t) * total_fields);

  lut_ptr->u.matchspec_ptr = PIPE_MGR_CALLOC(1, to_allocate);
  if (!lut_ptr->u.matchspec_ptr) {
    LOG_ERROR("%s:%d: Could not allocate memory for match spec structure.",
              __func__,
              __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  pipemgr_tbl_pkg_spec_t *match_spec_ptr = lut_ptr->u.matchspec_ptr;
  match_spec_ptr->fieldcount = total_fields;
  match_spec_ptr->fields = (pipemgr_tbl_pkg_spec_field_t *)(match_spec_ptr + 1);

  // Iterator on the match spec internal structure.
  pipemgr_tbl_pkg_spec_field_t *field_ptr = match_spec_ptr->fields;

  bool use_global_name =
      ctx_json_schema_has_global_name(prof_info->schema_version);
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *field_name = NULL;
    int field_width = 0;
    int field_start_bit = 0;
    if (use_global_name) {
      bf_cjson_try_get_string(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_GLOBAL_NAME, &field_name);
    }
    if (!field_name || *field_name == '\0') {
      err = bf_cjson_get_string(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
      CHECK_ERR(err, rc, cleanup);
    }
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &field_width);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &field_start_bit);
    CHECK_ERR(err, rc, cleanup);

    int spec_length = 0;
    int spec_offset = 0;

    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_start_bit,
                                                field_width,
                                                &spec_length,
                                                &spec_offset,
                                                NULL,
                                                NULL);
    CHECK_ERR(err, rc, cleanup);

    field_ptr->fieldname_str_index =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, field_name, prof_id);
    field_ptr->fieldwidth = spec_length;
    field_ptr->startbit = spec_offset;

    ++field_ptr;
  }
  return rc;

cleanup:
  LOG_ERROR("%s: Cannot parse match spec for table %s (0x%x)",
            __func__,
            name,
            table_handle);
  return rc;
}

/**
 * Parse the action spec for a given cJSON table. This function populates an
 * entry in the action spec LUT.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_action_spec_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  // Action spec LUT.
  pipemgr_tbl_pkg_lut_t *action_spec_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut;
  uint32_t lut_depth =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut_collision_count);

  int table_handle = 0;
  char *name = NULL;
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing action spec format for table handle 0x%x named \"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  cJSON *actions_cjson = NULL;
  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all actions in this table.
  cJSON *action_cjson = NULL;
  CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
    int action_handle = 0;
    err |= bf_cjson_get_handle(
        devid, prof_id, action_cjson, CTX_JSON_ACTION_HANDLE, &action_handle);
    CHECK_ERR(err, rc, cleanup);

    // Use only action handle for producing the hash for this LUT.
    uint16_t bj_index =
        bob_jenkin_hash_one_at_a_time(lut_depth, action_handle, 0, 0);
    pipemgr_tbl_pkg_lut_t *lut_ptr =
        pipemgr_tbl_pkg_update_lut_entry(action_spec_lut,
                                         lut_depth,
                                         collision_count_ptr,
                                         bj_index,
                                         action_handle,
                                         0,
                                         0);
    if (lut_ptr == NULL) {
      LOG_ERROR(
          "%s:%d: Could not populate action spec LUT.", __func__, __LINE__);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }

    cJSON *p4_parameters_cjson = NULL;
    err |= bf_cjson_get_object(
        action_cjson, CTX_JSON_ACTION_P4_PARAMETERS, &p4_parameters_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Compute amount of memory needed for LUT entry.
    int total_fields = cJSON_GetArraySize(p4_parameters_cjson);
    int to_allocate = sizeof(pipemgr_tbl_pkg_action_spec_t) +
                      (sizeof(pipemgr_tbl_pkg_spec_field_t) * total_fields);

    // Allocate memory for the contents in the LUT.
    lut_ptr->u.actionspec_ptr = PIPE_MGR_CALLOC(1, to_allocate);
    if (lut_ptr->u.actionspec_ptr == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for action spec structure.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    pipemgr_tbl_pkg_action_spec_t *action_spec_ptr = lut_ptr->u.actionspec_ptr;
    action_spec_ptr->fieldcount = total_fields;
    action_spec_ptr->fields =
        (pipemgr_tbl_pkg_spec_field_t *)(action_spec_ptr + 1);

    // Parse this action's information.
    char *action_name = NULL;
    err |=
        bf_cjson_get_string(action_cjson, CTX_JSON_ACTION_NAME, &action_name);
    CHECK_ERR(err, rc, cleanup);

    action_spec_ptr->actionfunc_name_str_index =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, action_name, prof_id);

    // Iterator used to populate internal structures.
    pipemgr_tbl_pkg_spec_field_t *field_ptr = action_spec_ptr->fields;

    // Iterate over all p4 parameters in this action, and parse them.
    cJSON *p4_parameter_cjson = NULL;
    CTX_JSON_FOR_EACH(p4_parameter_cjson, p4_parameters_cjson) {
      char *param_name = NULL;
      err |= bf_cjson_get_string(
          p4_parameter_cjson, CTX_JSON_ACTION_NAME, &param_name);

      int spec_length = 0;
      int spec_offset = 0;
      err |= ctx_json_parse_action_spec_details_for_field(
          action_cjson, param_name, &spec_length, &spec_offset);
      CHECK_ERR(err, rc, cleanup);

      field_ptr->fieldname_str_index =
          pipemgr_tbl_pkg_gen_str_index_for_field_name(
              devid, param_name, prof_id);
      field_ptr->startbit = spec_offset;
      field_ptr->fieldwidth = spec_length;

      ++field_ptr;
    }
  }
  return rc;

cleanup:
  return rc;
}

// #### Phase0 Entry Format ####

/**
 * Parse the phase0 match entry format for a given cJSON table. This function
 * populates an entry in the phase0 match LUT.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_phase0_match_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, rmt_dev_profile_info_t *prof_info) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;
  profile_id_t prof_id = prof_info->profile_id;

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing phase0 match format for table handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *phase0_match_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut;
  uint32_t lut_depth =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut_collision_count);

  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_key_fields_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                            &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  int number_fields = cJSON_GetArraySize(match_key_fields_cjson);
  int mem_size = 0;
  mem_size += sizeof(pipemgr_tbl_pkg_phase0_match_t);
  mem_size += number_fields * sizeof(pipemgr_tbl_pkg_phase0_match_field_t);

  // Obtain lut_ptr.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr =
      pipemgr_tbl_pkg_update_lut_entry(phase0_match_lut,
                                       lut_depth,
                                       collision_count_ptr,
                                       bj_index,
                                       table_handle,
                                       0,
                                       0);
  if (lut_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not populate phase0 match LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate the internal structs with the appropriate calculated size.
  lut_ptr->u.phase0_match_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.phase0_match_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for phase0 match internal "
        "structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  // Populate common fields with metadata already computed.
  pipemgr_tbl_pkg_phase0_match_t *format_ptr = lut_ptr->u.phase0_match_ptr;
  format_ptr->fieldcount = number_fields;
  format_ptr->matchfields =
      (pipemgr_tbl_pkg_phase0_match_field_t *)(format_ptr + 1);

  // Iterator to go over internal structure.
  pipemgr_tbl_pkg_phase0_match_field_t *field_ptr = format_ptr->matchfields;
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *field_name = NULL;
    int spec_offset = 0;
    int spec_length = 0;
    int field_start_bit = 0;
    int field_width = 0;

    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &field_width);
    CHECK_ERR(err, rc, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &field_start_bit);
    CHECK_ERR(err, rc, cleanup);

    bool use_global_name =
        ctx_json_schema_has_global_name(prof_info->schema_version);
    err = ctx_json_parse_spec_details_for_field(use_global_name,
                                                match_key_fields_cjson,
                                                field_name,
                                                field_start_bit,
                                                field_width,
                                                &spec_length,
                                                &spec_offset,
                                                NULL,
                                                NULL);
    CHECK_ERR(err, rc, cleanup);

    field_ptr->fieldname_str_index =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, field_name, prof_id);
    field_ptr->startbit = spec_offset;
    field_ptr->fieldwidth = spec_length < 8 ? 8 : spec_length;

    ++field_ptr;
  }

cleanup:
  return rc;
}

/**
 * Parses the phase0 entry format for a given field's cJSON structure and the
 * action spec.
 *
 * @param devid The current device's id.
 * @param field_cjson This field's cJSON structure.
 * @param action_cjson The action spec for the current action.
 * @param field_ptr The pointer to the internal structure corresponding to
 * this field.
 * @param number_const_tuples_in_field_ret A pointer to an integer that will
 * be set to the number of constant tuples in this field.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_phase0_action_entry_format_field(
    bf_dev_id_t devid,
    cJSON *field_cjson,
    cJSON *action_cjson,
    pipemgr_tbl_pkg_phase0_action_field_t *field_ptr,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  char *field_name = NULL;
  char *source = NULL;
  int field_width = 0;
  int lsb_mem_word_offset = 0;
  int lsb_mem_word_index = 0;

  err |= bf_cjson_get_string(
      field_cjson, CTX_JSON_PHASE0_ENTRY_FORMAT_FIELD_NAME, &field_name);
  err |= bf_cjson_get_string(
      field_cjson, CTX_JSON_PHASE0_ENTRY_FORMAT_SOURCE, &source);
  err |= bf_cjson_get_int(
      field_cjson, CTX_JSON_PHASE0_ENTRY_FORMAT_FIELD_WIDTH, &field_width);
  err |= bf_cjson_get_int(field_cjson,
                          CTX_JSON_PHASE0_ENTRY_FORMAT_LSB_MEM_WORD_OFFSET,
                          &lsb_mem_word_offset);
  err |= bf_cjson_get_int(field_cjson,
                          CTX_JSON_PHASE0_ENTRY_FORMAT_LSB_MEM_WORD_INDEX,
                          &lsb_mem_word_index);
  CHECK_ERR(err, rc, cleanup);

  field_ptr->fieldname_str_index =
      pipemgr_tbl_pkg_gen_str_index_for_field_name(devid, field_name, prof_id);
  field_ptr->fieldwidth = field_width;
  field_ptr->lsbmemwordoffset = lsb_mem_word_offset;
  field_ptr->memword_index[0] = lsb_mem_word_index;

  if (!strcmp(source, CTX_JSON_PHASE0_ENTRY_FORMAT_SOURCE_IMMEDIATE)) {
    field_ptr->location = TBL_PKG_PHASE0_FIELD_LOCATION_CONSTANT;
    double const_value = 0;
    err |= bf_cjson_get_double(
        field_cjson, CTX_JSON_PHASE0_ENTRY_FORMAT_CONST_VALUE, &const_value);
    CHECK_ERR(err, rc, cleanup);
    field_ptr->constant_value = (uint32_t)const_value;
  } else if (!strcmp(source, CTX_JSON_PHASE0_ENTRY_FORMAT_SOURCE_SPEC)) {
    field_ptr->location = TBL_PKG_PHASE0_FIELD_LOCATION_SPEC;
    int start_bit = 0;
    int spec_offset = 0;
    int spec_length = 0;
    err |= bf_cjson_get_int(
        field_cjson, CTX_JSON_PHASE0_ENTRY_FORMAT_START_BIT, &start_bit);
    CHECK_ERR(err, rc, cleanup);

    rc |= ctx_json_parse_action_spec_details_for_field(
        action_cjson, field_name, &spec_length, &spec_offset);
    CHECK_RC(rc, cleanup);

    field_ptr->param_shift = start_bit;
    field_ptr->param_width = spec_length;
    field_ptr->param_start = spec_offset;
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: source type \"%s\" is not valid.",
        __func__,
        __LINE__,
        source);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses some metadata for a given ternary indirection stage table cJSON
 * structure.
 *
 * @param stage_tables_cjson The stage table's cJSON structure.
 * @param number_fields_per_stage A pointer to (max_number_stages) integers
 * to be populated with the number of fields for each action.
 * @param number_entries_per_stage A pointer to (max_number_stages)
 * integers to be populated with the number of entries for each action.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_phase0_action_entry_format_metadata(
    cJSON *stage_table_cjson, int *number_fields, int *number_entries) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *pack_formats_cjson = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all pack formats in this stage table.
  cJSON *pack_format_cjson = NULL;
  CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
    cJSON *entries_cjson = NULL;
    err |= bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Look at all entries in this pack_format, and parse them.
    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      cJSON *fields_cjson = NULL;
      err |= bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      *number_fields += cJSON_GetArraySize(fields_cjson);
      ++*number_entries;
    }
  }
  return rc;

cleanup:
  return rc;
}

/**
 * Parse the phase0 action entry format for a given cJSON table. This function
 * populates an entry in the phase0 action LUT.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_phase0_action_entry_format(
    bf_dev_id_t devid, cJSON *table_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int table_handle = 0;
  char *name = NULL;
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing phase0 action format for table handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *phase0_action_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut;
  uint32_t lut_depth =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut_collision_count);

  cJSON *match_attributes_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                             &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_key_fields_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                             &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *actions_cjson = NULL;
  cJSON *stage_tables_cjson = NULL;

  err |=
      bf_cjson_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  err |= bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Phase0 is a special case: it has one and only one stage table, and only
  // one action.
  cJSON *stage_table_cjson = NULL;
  cJSON *action_cjson = NULL;
  err |= bf_cjson_get_first(stage_tables_cjson, &stage_table_cjson);
  err |= bf_cjson_get_first(actions_cjson, &action_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be recomputed
  // here by hand.
  int total_number_fields = 0;
  int total_number_entries = 0;

  rc |= ctx_json_parse_phase0_action_entry_format_metadata(
      stage_table_cjson, &total_number_fields, &total_number_entries);
  CHECK_RC(rc, cleanup);

  int mem_size = 0;
  mem_size += sizeof(pipemgr_tbl_pkg_phase0_action_t);
  mem_size +=
      total_number_entries * sizeof(pipemgr_tbl_pkg_phase0_action_entry_t);
  mem_size +=
      total_number_fields * sizeof(pipemgr_tbl_pkg_phase0_action_field_t);

  // Obtain lut_ptr.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr =
      pipemgr_tbl_pkg_update_lut_entry(phase0_action_lut,
                                       lut_depth,
                                       collision_count_ptr,
                                       bj_index,
                                       table_handle,
                                       0,
                                       0);
  if (lut_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not populate phase0 action LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate the internal structs with the appropriate calculated size.
  lut_ptr->u.phase0_action_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.phase0_action_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for phase0 action internal "
        "structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  // Populate common fields with metadata already computed.
  pipemgr_tbl_pkg_phase0_action_t *format_ptr = lut_ptr->u.phase0_action_ptr;
  format_ptr->totalfields = total_number_fields;
  format_ptr->totalentries = total_number_entries;
  format_ptr->entry = (pipemgr_tbl_pkg_phase0_action_entry_t *)(format_ptr + 1);

  // Finally parse the entry format.
  cJSON *pack_formats_cjson = NULL;
  err |= bf_cjson_get_object(
      stage_table_cjson, CTX_JSON_STAGE_TABLE_PACK_FORMAT, &pack_formats_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *pack_format_cjson = NULL;
  CTX_JSON_FOR_EACH(pack_format_cjson, pack_formats_cjson) {
    int number_entries = 0;
    int number_fields = 0;

    cJSON *entries_cjson = NULL;
    err |= bf_cjson_get_object(
        pack_format_cjson, CTX_JSON_PACK_FORMAT_ENTRIES, &entries_cjson);
    CHECK_ERR(err, rc, cleanup);

    // Iterator to go over internal structure.
    void *ptr = (uint8_t *)format_ptr->entry +
                number_entries * sizeof(pipemgr_tbl_pkg_phase0_action_entry_t) +
                number_fields * sizeof(pipemgr_tbl_pkg_phase0_action_field_t);
    pipemgr_tbl_pkg_phase0_action_entry_t *entry_ptr = ptr;

    entry_ptr->actionfields =
        (pipemgr_tbl_pkg_phase0_action_field_t *)(entry_ptr + 1);

    cJSON *entry_cjson = NULL;
    CTX_JSON_FOR_EACH(entry_cjson, entries_cjson) {
      int number_fields_in_entry = 0;
      int entry_number = 0;
      err |= bf_cjson_get_int(entry_cjson,
                              CTX_JSON_PHASE0_ENTRY_FORMAT_ENTRY_NUMBER,
                              &entry_number);
      CHECK_ERR(err, rc, cleanup);

      pipemgr_tbl_pkg_phase0_action_field_t *field_ptr =
          entry_ptr->actionfields;

      cJSON *fields_cjson = NULL;
      err |= bf_cjson_get_object(
          entry_cjson, CTX_JSON_PACK_FORMAT_ENTRY_FIELDS, &fields_cjson);
      CHECK_ERR(err, rc, cleanup);

      cJSON *field_cjson = NULL;
      CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
        rc |= ctx_json_parse_phase0_action_entry_format_field(
            devid, field_cjson, action_cjson, field_ptr, prof_id);
        CHECK_RC(rc, cleanup);

        ++field_ptr;
        ++number_fields;
        ++number_fields_in_entry;
      }

      ++number_entries;
      entry_ptr->fieldcount = number_fields_in_entry;
      entry_ptr->entry = entry_number;
    }
  }

cleanup:
  return rc;
}

// #### Default Entry Format ####

/**
 * Parses some metadata for a given action data stage table cJSON structure.
 *
 * @param table_cjson The table's cJSON structure.
 * @param last_stage_table_cjson A pointer to a cJSON * that will be set
 * to this table's last stage table, which is the one used to fill up the
 * default entry format.
 * @param number_action_formats A pointer to an integer that will be set to
 * the number of action formats in the last stage table.
 * @param number_selectors A pointer to an integer that will be set to the
 * number of selectors in the last stage table.
 * @param number_immediate_fields A pointer to an integer that will be set to
 * the number of immediate fields in the last stage table.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_default_entry_format_metadata(
    bf_dev_id_t devid,
    cJSON *table_cjson,
    cJSON **last_stage_table_cjson,
    int *number_action_formats,
    int *number_selectors,
    int *number_immediate_fields) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to default  entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  int number_stage_tables = 0;
  err |= ctx_json_parse_all_match_stage_tables_for_table(
      table_cjson,
      max_number_stages * max_number_logical_tables,
      all_stage_tables_cjson,
      &number_stage_tables);
  CHECK_ERR(err, rc, cleanup);

  // If table has no stages, there is nothing to parse
  if (number_stage_tables == 0) {
    rc = PIPE_SUCCESS;
    goto cleanup;
  }

  // Find the stage with highest number: it is the last one.
  int max_stage = -1;
  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];
    PIPE_MGR_ASSERT(stage_table_cjson != NULL);

    int stage_number = 0;
    int logical_table_id = 0;
    err |= bf_cjson_get_int(
        stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage_number);
    err |= bf_cjson_get_int(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID,
                            &logical_table_id);
    CHECK_ERR(err, rc, cleanup);

    int key_to_sort = (stage_number << 16) | logical_table_id;
    if (key_to_sort > max_stage) {
      max_stage = key_to_sort;
      *last_stage_table_cjson = stage_table_cjson;
    }
  }
  PIPE_MGR_ASSERT(*last_stage_table_cjson != NULL);

  /*
   * Parse the number of action formats and immediate fields.
   */
  cJSON *action_formats_cjson = NULL;
  err |= bf_cjson_try_get_object(*last_stage_table_cjson,
                                 CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                                 &action_formats_cjson);
  if (action_formats_cjson == NULL) {
    cJSON *ternary_indirection_stage_table = NULL;
    err |= bf_cjson_get_object(
        *last_stage_table_cjson,
        CTX_JSON_STAGE_TABLE_TERNARY_INDIRECTION_STAGE_TABLE,
        &ternary_indirection_stage_table);
    CHECK_ERR(err, rc, cleanup);

    err |= bf_cjson_get_object(ternary_indirection_stage_table,
                               CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                               &action_formats_cjson);
  }
  CHECK_ERR(err, rc, cleanup);

  *number_action_formats = cJSON_GetArraySize(action_formats_cjson);

  // We add one action format for the reset_default_action action format.
  *number_action_formats += 1;

  cJSON *action_format_cjson = NULL;
  CTX_JSON_FOR_EACH(action_format_cjson, action_formats_cjson) {
    cJSON *immediate_fields_cjson = NULL;
    err |= bf_cjson_get_object(action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
                               &immediate_fields_cjson);
    CHECK_ERR(err, rc, cleanup);

    *number_immediate_fields += cJSON_GetArraySize(immediate_fields_cjson);
  }

  /*
   * Currently we support only one selector. Thus, we simply look to see if
   * we reference any selectors.
   */
  cJSON *selection_table_refs_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_SELECTION_TABLE_REFS,
                             &selection_table_refs_cjson);
  CHECK_ERR(err, rc, cleanup);
  *number_selectors = cJSON_GetArraySize(selection_table_refs_cjson);

cleanup:
  PIPE_MGR_FREE(all_stage_tables_cjson);
cleanup_no_free:
  return rc;
}

/**
 * Parse the action formats cJSON into the internal structure, given by the
 * action_handles_ptr struct.
 *
 * @param devid The current device's id.
 * @param table_cjson A cJSON representing the current table.
 * @param last_stage_table_cjson The last stage table in this table.
 * @param action_entry_ptr A pointer to the internal structure to represent
 * the action entry.
 * @param action_param_ptr A pointer to the internal structure to represent
 * the action parameter fields.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_default_entry_format_action_format(
    bf_dev_id_t devid,
    profile_id_t prof_id,
    cJSON *table_cjson,
    cJSON *last_stage_table_cjson,
    pipemgr_tbl_pkg_default_action_fn_t *action_entry_ptr,
    pipemgr_tbl_pkg_default_action_param_t *action_param_ptr) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  cJSON *action_formats_cjson = NULL;
  bf_cjson_try_get_object(last_stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                          &action_formats_cjson);

  // Check to see if we have a ternary table: if so, action format is on the
  // ternary indirection.
  if (action_formats_cjson == NULL) {
    cJSON *ternary_indirection_stage_table = NULL;
    err |= bf_cjson_get_object(
        last_stage_table_cjson,
        CTX_JSON_STAGE_TABLE_TERNARY_INDIRECTION_STAGE_TABLE,
        &ternary_indirection_stage_table);
    err |= bf_cjson_get_object(ternary_indirection_stage_table,
                               CTX_JSON_STAGE_TABLE_ACTION_FORMAT,
                               &action_formats_cjson);
    CHECK_ERR(err, rc, cleanup);
  }

  int default_next_table_mask = 0;
  int default_next_table_default = 0;
  bf_cjson_try_get_int(table_cjson,
                       CTX_JSON_MATCH_TABLE_DEFAULT_NEXT_TABLE_DEFAULT,
                       &default_next_table_default);
  err = bf_cjson_get_int(table_cjson,
                         CTX_JSON_MATCH_TABLE_DEFAULT_NEXT_TABLE_MASK,
                         &default_next_table_mask);
  CHECK_ERR(err, rc, cleanup);

  cJSON *actions_cjson = NULL;
  bf_cjson_try_get_object(table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
  cJSON *action_format_cjson = NULL;
  CTX_JSON_FOR_EACH(action_format_cjson, action_formats_cjson) {
    char *action_name = NULL;
    int action_handle = 0;
    int next_table_full = 0;
    int vliw_instruction_full = 0;
    int full_stats_addr = 0;
    int full_meter_addr = 0;
    int full_stful_addr = 0;
    int next_table_exec = 0;
    int next_table_long_branch = 0;


    err |= bf_cjson_get_handle(devid,
                               prof_id,
                               action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_ACTION_HANDLE,
                               &action_handle);
    err |= bf_cjson_get_string(
        action_format_cjson, CTX_JSON_ACTION_FORMAT_ACTION_NAME, &action_name);
    err |= bf_cjson_get_int(action_format_cjson,
                            CTX_JSON_ACTION_FORMAT_VLIW_INSTRUCTION_FULL,
                            &vliw_instruction_full);
    err |= bf_cjson_get_int(action_format_cjson,
                            CTX_JSON_ACTION_FORMAT_NEXT_TABLE_FULL,
                            &next_table_full);
    CHECK_ERR(err, rc, cleanup);
    bf_cjson_try_get_int(action_format_cjson,
                         CTX_JSON_ACTION_FORMAT_NEXT_TABLE_EXEC,
                         &next_table_exec);
    bf_cjson_try_get_int(action_format_cjson,
                         CTX_JSON_ACTION_FORMAT_NEXT_TABLE_LONG_BRANCH,
                         &next_table_long_branch);






    // Find the action in the actions list passed in.
    cJSON *action_cjson = NULL;
    err |= ctx_json_parse_action_for_action_handle(
        devid, prof_id, actions_cjson, action_handle, &action_cjson);
    CHECK_ERR(err, rc, cleanup);

    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_PFE_SET,
                             &action_entry_ptr->force_stats_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_PFE_SET,
                             &action_entry_ptr->force_meter_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_PFE_SET,
                             &action_entry_ptr->force_stful_pfe_set);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_ADDR,
                             &action_entry_ptr->force_stats_addr);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_ADDR,
                             &action_entry_ptr->force_meter_addr);
    err |= bf_cjson_get_bool(action_cjson,
                             CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_ADDR,
                             &action_entry_ptr->force_stful_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_STATS_FULL_ADDR,
                            &full_stats_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_METER_FULL_ADDR,
                            &full_meter_addr);
    err |= bf_cjson_get_int(action_cjson,
                            CTX_JSON_ACTION_FORMAT_OVERRIDE_STFUL_FULL_ADDR,
                            &full_stful_addr);
    CHECK_ERR(err, rc, cleanup);

    action_entry_ptr->action_fn_handle = action_handle;
    action_entry_ptr->instr = vliw_instruction_full;
    action_entry_ptr->next_tbl = next_table_full;
    action_entry_ptr->next_tbl_default = default_next_table_default;
    action_entry_ptr->next_tbl_exec = next_table_exec;
    action_entry_ptr->next_tbl_long_branch = next_table_long_branch;


    action_entry_ptr->tbl_mask = default_next_table_mask;
    action_entry_ptr->forced_full_stats_addr = full_stats_addr;
    action_entry_ptr->forced_full_meter_addr = full_meter_addr;
    action_entry_ptr->forced_full_stful_addr = full_stful_addr;

    ++action_entry_ptr;

    action_param_ptr->action_fn_handle = action_handle;
    action_param_ptr->act_field =
        (pipemgr_tbl_pkg_action_parameter_field_t *)(action_param_ptr + 1);

    pipemgr_tbl_pkg_action_parameter_field_t *immediate_ptr =
        action_param_ptr->act_field;

    cJSON *immediate_fields_cjson = NULL;
    err |= bf_cjson_get_object(action_format_cjson,
                               CTX_JSON_ACTION_FORMAT_IMMEDIATE_FIELDS,
                               &immediate_fields_cjson);
    CHECK_ERR(err, rc, cleanup);

    int number_immediate_fields = 0;
    cJSON *immediate_field_cjson = NULL;
    CTX_JSON_FOR_EACH(immediate_field_cjson, immediate_fields_cjson) {
      rc |= ctx_json_parse_action_immediate(
          devid, action_cjson, immediate_field_cjson, immediate_ptr);
      ++number_immediate_fields;
      ++immediate_ptr;
    }
    action_param_ptr->act_param_count = number_immediate_fields;
    action_param_ptr = (pipemgr_tbl_pkg_default_action_param_t *)immediate_ptr;
  }

  /* Finally, we add reset_default_action. Notice that memory has already
   * been allocated for it (by the calculation in the metadata function).
   *
   * Note that this is only relevant for glass, since brig will always have
   * a default action and thus will not require default reset values.
   * We set the default to be 255 to signify end-of-pipe for Tofino.
   */
  int default_next_table = 255;
  bf_cjson_try_get_int(last_stage_table_cjson,
                       CTX_JSON_STAGE_TABLE_DEFAULT_NEXT_TABLE,
                       &default_next_table);

  action_entry_ptr->action_fn_handle = 0;
  action_entry_ptr->instr = 0;
  action_entry_ptr->next_tbl = default_next_table;
  action_entry_ptr->tbl_mask = default_next_table_mask;
  action_entry_ptr->next_tbl_default = 0;
  action_entry_ptr->forced_full_stats_addr = 0;
  action_entry_ptr->forced_full_meter_addr = 0;
  action_entry_ptr->forced_full_stful_addr = 0;
  action_entry_ptr->force_stats_addr = false;
  action_entry_ptr->force_meter_addr = false;
  action_entry_ptr->force_stful_addr = false;
  action_entry_ptr->force_stats_pfe_set = false;
  action_entry_ptr->force_meter_pfe_set = false;
  action_entry_ptr->force_stful_pfe_set = false;

  return rc;

cleanup:
  return rc;
}

/**
 * Parse the default entry format for a given cJSON table. This function
 * populates an entry in the default entries LUT.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to a table.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_default_entry_format(bf_dev_id_t devid,
                                                         cJSON *table_cjson,
                                                         profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int table_handle = 0;
  char *name = NULL;
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing default entry format for table handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      table_handle,
      name);

  pipemgr_tbl_pkg_lut_t *dft_lut = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut_collision_count);

  // Metadata necessary for building the LUT for each stage table.
  // These were removed from the old ContextJSON, so they must be
  // recomputed
  // here by hand.
  cJSON *last_stage_table_cjson = NULL;
  int number_action_formats = 0;
  int number_selectors = 0;
  int number_immediate_fields = 0;

  // First pass: compute metadata about the entry formats from the
  // context.
  rc |= ctx_json_parse_default_entry_format_metadata(devid,
                                                     table_cjson,
                                                     &last_stage_table_cjson,
                                                     &number_action_formats,
                                                     &number_selectors,
                                                     &number_immediate_fields);
  CHECK_RC(rc, cleanup);
  // If rc was success and last_stage_table_cjson is NULL, then
  // this table has no stages.
  if (!last_stage_table_cjson) {
    goto cleanup;
  }

  cJSON *match_attributes_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                             &match_attributes_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *match_key_fields_cjson = NULL;
  err |= bf_cjson_get_object(table_cjson,
                             CTX_JSON_MATCH_TABLE_MATCH_KEY_FIELDS,
                             &match_key_fields_cjson);
  CHECK_ERR(err, rc, cleanup);

  int mem_size = 0;
  int stage = 0;
  int logical_table_id = 0;





  err |= bf_cjson_get_int(
      last_stage_table_cjson, CTX_JSON_STAGE_TABLE_STAGE_NUMBER, &stage);
  err |= bf_cjson_get_int(last_stage_table_cjson,
                          CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID,
                          &logical_table_id);
  CHECK_ERR(err, rc, cleanup);

  // Memory required for each field, entry, way, logical table and the
  // overall match entry format structure.
  mem_size += sizeof(pipemgr_tbl_pkg_default_entry_t);
  mem_size +=
      number_action_formats * (sizeof(pipemgr_tbl_pkg_default_action_fn_t) +
                               sizeof(pipemgr_tbl_pkg_default_action_param_t));
  mem_size += number_immediate_fields *
              sizeof(pipemgr_tbl_pkg_action_parameter_field_t);

  // Obtain lut_ptr for this stage.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, table_handle, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr = pipemgr_tbl_pkg_update_lut_entry(
      dft_lut, lut_depth, collision_count_ptr, bj_index, table_handle, 0, 0);
  if (lut_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not populate default entry LUT.", __func__, __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate the internal structs with the appropriate calculated size.
  lut_ptr->u.dft_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.dft_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for default entry internal "
        "structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  // Populate common fields with metadata already computed.
  pipemgr_tbl_pkg_default_entry_t *format_ptr = lut_ptr->u.dft_ptr;

  format_ptr->totalactionhandles = number_action_formats;
  format_ptr->stage = stage;
  format_ptr->selectorcount = number_selectors;
  format_ptr->logicalid = logical_table_id;

  PIPE_MGR_ASSERT(number_selectors <= 1);

  // Setup pointers for actions.
  format_ptr->actions = (pipemgr_tbl_pkg_default_action_fn_t *)(format_ptr + 1);

  // Setup pointers for immediates.
  format_ptr->act_param =
      (pipemgr_tbl_pkg_default_action_param_t *)(format_ptr->actions +
                                                 number_action_formats);

  // Parse action formats and immediates for this default entry.

  rc |=
      ctx_json_parse_default_entry_format_action_format(devid,
                                                        prof_id,
                                                        table_cjson,
                                                        last_stage_table_cjson,
                                                        format_ptr->actions,
                                                        format_ptr->act_param);
  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

// #### Learn Quanta Entry Format ####

/**
 * Parse the learn quanta entry format for a given ContextJSON. This function
 * populates an entry in the learn quanta LUT.
 *
 * @param devid The current device's id.
 * @param learn_quantum_cjson The cJSON structure corresponding to this learn
 * quantum.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_learn_quantum_entry_format(
    bf_dev_id_t devid, cJSON *learn_quantum_cjson, profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  int handle = 0;
  char *name = NULL;
  cJSON *fields_cjson = NULL;
  err |= bf_cjson_get_handle(devid,
                             prof_id,
                             learn_quantum_cjson,
                             CTX_JSON_LEARN_QUANTUM_HANDLE,
                             &handle);
  err |= bf_cjson_get_string(
      learn_quantum_cjson, CTX_JSON_LEARN_QUANTUM_NAME, &name);
  err |= bf_cjson_get_object(
      learn_quantum_cjson, CTX_JSON_LEARN_QUANTUM_FIELDS, &fields_cjson);
  int number_fields = cJSON_GetArraySize(fields_cjson);
  char *field_names[number_fields];
  PIPE_MGR_MEMSET(field_names, 0, number_fields * sizeof(char *));
  CHECK_ERR(err, rc, cleanup);

  LOG_DBG(
      "%s:%d: Parsing learn quantum entry format for handle 0x%x named "
      "\"%s\".",
      __func__,
      __LINE__,
      handle,
      name);

  pipemgr_tbl_pkg_lut_t *lq_lut = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut_collision_count);

  int lq_cfg_type = 0;
  err |= bf_cjson_get_int(
      learn_quantum_cjson, CTX_JSON_LEARN_QUANTUM_LQ_CFG_TYPE, &lq_cfg_type);
  CHECK_ERR(err, rc, cleanup);

  // Memory required for the entry format.
  int mem_size = 0;
  mem_size += sizeof(pipemgr_tbl_pkg_lq_t);
  mem_size += number_fields * sizeof(pipemgr_tbl_pkg_lq_field_t);

  // Obtain lut_ptr for this stage.
  uint16_t bj_index =
      bob_jenkin_hash_one_at_a_time(lut_depth, lq_cfg_type + 1, 0, 0);

  pipemgr_tbl_pkg_lut_t *lut_ptr = pipemgr_tbl_pkg_update_lut_entry(
      lq_lut, lut_depth, collision_count_ptr, bj_index, lq_cfg_type + 1, 0, 0);
  if (lut_ptr == NULL) {
    LOG_ERROR("%s:%d: Could not populate learn quanta entry LUT.",
              __func__,
              __LINE__);
    rc = PIPE_INIT_ERROR;
    goto cleanup;
  }

  // Allocate the internal structs with the appropriate calculated size.
  lut_ptr->u.lq_ptr = PIPE_MGR_CALLOC(1, mem_size);
  if (lut_ptr->u.lq_ptr == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory for learn quanta entry internal "
        "structure.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup;
  }

  // Populate common fields with metadata already computed.
  pipemgr_tbl_pkg_lq_t *format_ptr = lut_ptr->u.lq_ptr;

  // Parse information we already have.
  format_ptr->handle = handle;
  format_ptr->lq_cfg_type = lq_cfg_type;
  format_ptr->lrn_cfg_type_sz = 0;
  format_ptr->lq_name_str_index =
      pipemgr_tbl_pkg_gen_str_index_for_field_name(devid, name, prof_id);
  format_ptr->fieldcount = number_fields;
  format_ptr->fields = (pipemgr_tbl_pkg_lq_field_t *)(format_ptr + 1);

  // Parse the learn quantum's fields.
  pipemgr_tbl_pkg_lq_field_t *field_ptr = format_ptr->fields;
  int total_fields_in_bytes = 0;
  uint32_t field_arr_size = 0, field_idx = 0;

  cJSON *field_cjson = NULL, *phv_field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, fields_cjson) {
    char *field_name = NULL;
    int str_index = 0;
    int start_bit = 0;
    int start_byte = 0;
    int field_width = 0;
    int phv_offset = 0;
    int total_field_width = 0;
    int field_ptr_idx = 0;
    int num_slices = 0;

    err |= bf_cjson_get_string(
        field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_NAME, &field_name);
    err |= bf_cjson_get_int(
        field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_START_BIT, &start_bit);
    err |= bf_cjson_get_int(
        field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_START_BYTE, &start_byte);
    err |= bf_cjson_get_int(
        field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_WIDTH, &field_width);
    err |= bf_cjson_try_get_int(
        field_cjson, CTX_JSON_LEARN_QUANTUM_PHV_OFFSET, &phv_offset);
    CHECK_ERR(err, rc, cleanup);

    for (field_idx = 0; field_idx < field_arr_size; field_idx++) {
      if (!strcmp(field_names[field_idx], field_name)) {
        break;
      }
    }
    if (field_idx < field_arr_size) {
      continue;
    }
    field_names[field_idx] = field_name;
    field_arr_size++;

    str_index = pipemgr_tbl_pkg_gen_str_index_for_field_name(
        devid, field_name, prof_id);
    field_ptr->fieldname_str_index = str_index;
    field_ptr->startbit = start_bit;
    field_ptr->byteoffset = start_byte;
    field_ptr->fieldwidth = field_width;
    field_ptr->phvoffset = phv_offset;
    total_field_width = field_width;

    // ++field_ptr;

    /* Find all other phv slices for this field to store all slices of a field
     * contiguously. */
    field_ptr_idx = 1;
    for (phv_field_cjson = field_cjson->next; phv_field_cjson;
         phv_field_cjson = phv_field_cjson->next) {
      char *phv_field_name = NULL;
      err |= bf_cjson_get_string(
          phv_field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_NAME, &phv_field_name);
      CHECK_ERR(err, rc, cleanup);
      if (strcmp(phv_field_name, field_name)) {
        continue;
      }

      err |= bf_cjson_get_int(
          phv_field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_START_BIT, &start_bit);
      err |= bf_cjson_get_int(phv_field_cjson,
                              CTX_JSON_LEARN_QUANTUM_FIELD_START_BYTE,
                              &start_byte);
      err |= bf_cjson_get_int(
          phv_field_cjson, CTX_JSON_LEARN_QUANTUM_FIELD_WIDTH, &field_width);
      err |= bf_cjson_try_get_int(
          phv_field_cjson, CTX_JSON_LEARN_QUANTUM_PHV_OFFSET, &phv_offset);
      CHECK_ERR(err, rc, cleanup);

      (field_ptr + field_ptr_idx)->fieldname_str_index = str_index;
      (field_ptr + field_ptr_idx)->startbit = start_bit;
      (field_ptr + field_ptr_idx)->byteoffset = start_byte;
      (field_ptr + field_ptr_idx)->fieldwidth = field_width;
      (field_ptr + field_ptr_idx)->phvoffset = phv_offset;

      total_field_width += field_width;
      field_ptr_idx++;
    }
    num_slices = field_ptr_idx;

    /* Update the total width of the field for each slice */
    for (field_ptr_idx = 0; field_ptr_idx < num_slices; field_ptr_idx++) {
      (field_ptr + field_ptr_idx)->totalfieldwidth = total_field_width;
    }

    /*
     * 3 byte fields are represented with uint32_t, so we must add a byte.
     */
    int field_width_in_bytes = (total_field_width + 7) / 8;
    if (field_width_in_bytes == 3) {
      field_width_in_bytes = 4;
    }
    total_fields_in_bytes += field_width_in_bytes;

    field_ptr += num_slices;
  }
  format_ptr->lrn_cfg_type_sz = total_fields_in_bytes;

  CHECK_RC(rc, cleanup);
  return rc;

cleanup:
  return rc;
}

// #### Snapshot Entry Format ####

/**
 * Parses a PHV container into the internal structure for snapshot entry
 * format.
 *
 * @param devid The current device's id.
 * @param phv_container_cjson The cJSON structure corresponding to a PHV
 * container.
 * @param direction A number representing a direction. This is 0 for ingress
 * and 1 for egress.
 * @param phv_record_ptr A pointer to the internal structure corresponding to
 * the first record in the PHV container.
 * @param pov_header_ptr A pointer to the internal structure corresponding to
 * the first POV header in the PHV container.
 * @param number_phv_records A pointer to an integer that will store the number
 * of PHV records within this container.
 * @param number_pov_headers A pointer to an integer that will store the number
 * of POV headers within this container.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_snapshot_entry_format_phv(
    bf_dev_id_t devid,
    cJSON *phv_container_cjson,
    int direction,
    pipemgr_tbl_pkg_snapshot_phv_t *phv_record_ptr,
    pipemgr_tbl_pkg_snapshot_pov_t *pov_header_ptr,
    int *number_phv_records,
    int *number_pov_headers,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  *number_phv_records = 0;
  *number_pov_headers = 0;

  int phv_number = 0;
  err |= bf_cjson_get_int(
      phv_container_cjson, CTX_JSON_PHV_CONTAINER_PHV_NUMBER, &phv_number);
  CHECK_ERR(err, rc, cleanup);

  int word_bit_width = 0;
  err |= bf_cjson_get_int(phv_container_cjson,
                          CTX_JSON_PHV_CONTAINER_WORD_BIT_WIDTH,
                          &word_bit_width);
  CHECK_ERR(err, rc, cleanup);

  char *container_type = NULL;
  err |= bf_cjson_try_get_string(
      phv_container_cjson, CTX_JSON_PHV_CONTAINER_TYPE, &container_type);
  CHECK_ERR(err, rc, cleanup);

  cJSON *records_cjson = NULL;
  err |= bf_cjson_get_object(
      phv_container_cjson, CTX_JSON_PHV_CONTAINER_RECORDS, &records_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *record_cjson = NULL;
  CTX_JSON_FOR_EACH(record_cjson, records_cjson) {
    phv_record_ptr->phvnumber = phv_number;
    phv_record_ptr->container_width = word_bit_width;
    phv_record_ptr->direction = direction;
    if (strcmp(container_type, "normal") == 0)
      phv_record_ptr->container_type = PIPE_MGR_NORMAL_PHV;
    else if (strcmp(container_type, "mocha") == 0)
      phv_record_ptr->container_type = PIPE_MGR_MOCHA_PHV;
    else if (strcmp(container_type, "dark") == 0)
      phv_record_ptr->container_type = PIPE_MGR_DARK_PHV;
    else
      phv_record_ptr->container_type = PIPE_MGR_UNKNOWN_PHV;

    char *field_name = NULL;
    int position_offset = 0;
    int field_lsb = 0;
    int field_msb = 0;
    int field_width = 0;
    int phv_lsb = 0;
    int phv_msb = 0;
    bool is_compiler_generated = false;
    bool is_pov = false;

    err |= bf_cjson_get_string(
        record_cjson, CTX_JSON_PHV_RECORD_FIELD_NAME, &field_name);
    err |= bf_cjson_get_int(
        record_cjson, CTX_JSON_PHV_RECORD_POSITION_OFFSET, &position_offset);
    err |= bf_cjson_get_int(
        record_cjson, CTX_JSON_PHV_RECORD_FIELD_LSB, &field_lsb);
    err |= bf_cjson_get_int(
        record_cjson, CTX_JSON_PHV_RECORD_FIELD_MSB, &field_msb);
    err |= bf_cjson_get_int(
        record_cjson, CTX_JSON_PHV_RECORD_FIELD_WIDTH, &field_width);
    err |=
        bf_cjson_get_int(record_cjson, CTX_JSON_PHV_RECORD_PHV_LSB, &phv_lsb);
    err |=
        bf_cjson_get_int(record_cjson, CTX_JSON_PHV_RECORD_PHV_MSB, &phv_msb);
    err |= bf_cjson_get_bool(record_cjson,
                             CTX_JSON_PHV_RECORD_IS_COMPILER_GENERATED,
                             &is_compiler_generated);
    err |= bf_cjson_get_bool(record_cjson, CTX_JSON_PHV_RECORD_IS_POV, &is_pov);
    CHECK_ERR(err, rc, cleanup);

    phv_record_ptr->phvname_str_index =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, field_name, prof_id);
    phv_record_ptr->position_offset = position_offset;
    phv_record_ptr->fieldlsb = field_lsb;
    phv_record_ptr->fieldmsb = field_msb;
    // The field width may be spread across multiple PHVs, so it's not simply
    // field_msb - field_lsb + 1.
    phv_record_ptr->fieldwidth = field_width;
    phv_record_ptr->phvlsb = phv_lsb;
    phv_record_ptr->phvmsb = phv_msb;
    phv_record_ptr->pov_hdrs = NULL;

    if (is_pov) {
      cJSON *pov_headers_cjson = NULL;
      err |= bf_cjson_get_object(
          record_cjson, CTX_JSON_PHV_RECORD_POV_HEADERS, &pov_headers_cjson);
      CHECK_ERR(err, rc, cleanup);

      int i = 0;
      cJSON *pov_header_cjson = NULL;
      CTX_JSON_FOR_EACH(pov_header_cjson, pov_headers_cjson) {
        char *header_name = NULL;
        int header_position_offset = 0;
        int bit_index = 0;
        bool hidden = false;

        err |= bf_cjson_get_string(
            pov_header_cjson, CTX_JSON_POV_HEADER_HEADER_NAME, &header_name);
        err |= bf_cjson_get_int(
            pov_header_cjson, CTX_JSON_POV_HEADER_BIT_INDEX, &bit_index);
        err |= bf_cjson_get_int(pov_header_cjson,
                                CTX_JSON_POV_HEADER_POSITION_OFFSET,
                                &header_position_offset);
        err |= bf_cjson_get_bool(
            pov_header_cjson, CTX_JSON_POV_HEADER_HIDDEN, &hidden);
        CHECK_ERR(err, rc, cleanup);

        if (strlen(header_name) > 7 &&
            !strcmp(header_name + strlen(header_name) - 7, ".$valid")) {
          header_name[strlen(header_name) - 7] = '\0';
        }

        pov_header_ptr->pov_hdr_str_index[i] =
            pipemgr_tbl_pkg_gen_str_index_for_field_name(
                devid, header_name, prof_id);
        pov_header_ptr->pov_bit[i] = bit_index;
        pov_header_ptr->position_offset[i] = header_position_offset;
        pov_header_ptr->hidden[i] = hidden;

        ++i;
      }
      // Make sure that there is at least one header here. The ContextJSON
      // should not output empty PHVs.
      PIPE_MGR_ASSERT(i > 0);

      pov_header_ptr->pov_bit_count = i;
      phv_record_ptr->pov_hdrs = pov_header_ptr;
      ++*number_pov_headers;
      ++pov_header_ptr;
    }

    ++phv_record_ptr;
    ++*number_phv_records;
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses snapshot metadata for a specific table. In particular, this function
 * returns the number of stage tables within the specified table that are
 * in the specified stage.
 *
 * @param devid The current device's id.
 * @param table_cjson The cJSON structure corresponding to this table.
 * @param stage_number The stage in which we are looking for stage tables.
 * @param number_stage_tables_in_stage_in_table A pointer to an int that will
 * be set to the number of stage tables within the given table that are also
 * in the given stage.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_snapshot_entry_format_metadata_table(
    bf_dev_id_t devid,
    cJSON *table_cjson,
    int stage_number,
    int *number_stage_tables_in_stage_in_table) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse exact match entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  // Fetch all stage tables for this table.
  int number_stage_tables = 0;
  err |= ctx_json_parse_all_match_stage_tables_for_table(
      table_cjson,
      max_number_stages * max_number_logical_tables,
      all_stage_tables_cjson,
      &number_stage_tables);
  CHECK_ERR(err, rc, cleanup);

  // Iterate over all stage tables to find the number of stage tables in this
  // stage.
  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];

    int current_stage_number = 0;
    err |= bf_cjson_get_int(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_STAGE_NUMBER,
                            &current_stage_number);
    CHECK_ERR(err, rc, cleanup);

    if (stage_number == current_stage_number) {
      ++*number_stage_tables_in_stage_in_table;
    }
  }

  goto cleanup;

cleanup:
  PIPE_MGR_FREE(all_stage_tables_cjson);
cleanup_no_free:
  return rc;
}

/**
 * Parses some metadata for the snapshot entry format LUT.
 *
 * @param all_stage_tables_cjson An array of all match stage tables in the
 * program.
 * @param number_stage_tables_in_stage The number of elements in the
 * all_stage_tables_cjson array.
 * @param stage_phv_allocation_cjson The cJSON structure corresponding to
 * a stage's PHV allocation.
 * @param number_stage_tables_in_stage A pointer to an integer that will be set
 * to the number of stage tables in this stage.
 * @param number_phv_records_in_stage A pointer to an integer that will be set
 * to the number of PHV records in this stage.
 * @param number_pov_headers_in_stage  A pointer to an integer that will be set
 * to the number of POV headers in this stage.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_snapshot_entry_format_metadata(
    bf_dev_id_t devid,
    cJSON *tables_cjson,
    int stage_number,
    cJSON *stage_phv_allocation_cjson,
    int *number_stage_tables_in_stage,
    int *number_phv_records_in_stage,
    int *number_pov_headers_in_stage) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  *number_stage_tables_in_stage = 0;
  *number_phv_records_in_stage = 0;
  *number_pov_headers_in_stage = 0;

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
    CHECK_ERR(err, rc, cleanup);

    // Skip non-match tables.
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

    int number_stage_tables_in_stage_in_table = 0;
    if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
      cJSON *pre_classifier_cjson = NULL;
      cJSON *atcam_cjson = NULL;

      err |= bf_cjson_get_object(match_attributes_cjson,
                                 CTX_JSON_MATCH_ATTRIBUTES_PRE_CLASSIFIER,
                                 &pre_classifier_cjson);
      err |= bf_cjson_get_object(match_attributes_cjson,
                                 CTX_JSON_MATCH_ATTRIBUTES_ATCAM_TABLE,
                                 &atcam_cjson);
      CHECK_ERR(err, rc, cleanup);

      rc |= ctx_json_parse_snapshot_entry_format_metadata_table(
          devid,
          pre_classifier_cjson,
          stage_number,
          &number_stage_tables_in_stage_in_table);
      *number_stage_tables_in_stage += number_stage_tables_in_stage_in_table;

      number_stage_tables_in_stage_in_table = 0;
      rc |= ctx_json_parse_snapshot_entry_format_metadata_table(
          devid,
          atcam_cjson,
          stage_number,
          &number_stage_tables_in_stage_in_table);
      *number_stage_tables_in_stage += number_stage_tables_in_stage_in_table;
      CHECK_RC(rc, cleanup);
    } else {
      rc |= ctx_json_parse_snapshot_entry_format_metadata_table(
          devid,
          table_cjson,
          stage_number,
          &number_stage_tables_in_stage_in_table);
      *number_stage_tables_in_stage += number_stage_tables_in_stage_in_table;
    }
  }

  // Iterate over all ingress PHVs to compute number of PHV/POVs.
  cJSON *ingress_stage_phv_allocation_cjson = NULL;
  err |= bf_cjson_get_object(stage_phv_allocation_cjson,
                             CTX_JSON_STAGE_PHV_ALLOCATION_INGRESS,
                             &ingress_stage_phv_allocation_cjson);
  CHECK_ERR(err, rc, cleanup);

  cJSON *phv_container_cjson = NULL;
  CTX_JSON_FOR_EACH(phv_container_cjson, ingress_stage_phv_allocation_cjson) {
    CHECK_ERR(err, rc, cleanup);

    cJSON *records_cjson = NULL;
    err |= bf_cjson_get_object(
        phv_container_cjson, CTX_JSON_PHV_CONTAINER_RECORDS, &records_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *record_cjson = NULL;
    CTX_JSON_FOR_EACH(record_cjson, records_cjson) {
      bool is_pov = false;
      err |=
          bf_cjson_get_bool(record_cjson, CTX_JSON_PHV_RECORD_IS_POV, &is_pov);
      CHECK_ERR(err, rc, cleanup);

      if (is_pov) {
        ++*number_pov_headers_in_stage;
      }

      ++*number_phv_records_in_stage;
    }
  }

  // Iterate over all egress PHVs to compute number of PHV/POVs.
  cJSON *egress_stage_phv_allocation_cjson = NULL;
  err |= bf_cjson_get_object(stage_phv_allocation_cjson,
                             CTX_JSON_STAGE_PHV_ALLOCATION_EGRESS,
                             &egress_stage_phv_allocation_cjson);
  CHECK_ERR(err, rc, cleanup);

  phv_container_cjson = NULL;
  CTX_JSON_FOR_EACH(phv_container_cjson, egress_stage_phv_allocation_cjson) {
    CHECK_ERR(err, rc, cleanup);

    cJSON *records_cjson = NULL;
    err |= bf_cjson_get_object(
        phv_container_cjson, CTX_JSON_PHV_CONTAINER_RECORDS, &records_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *record_cjson = NULL;
    CTX_JSON_FOR_EACH(record_cjson, records_cjson) {
      bool is_pov = false;
      err |=
          bf_cjson_get_bool(record_cjson, CTX_JSON_PHV_RECORD_IS_POV, &is_pov);
      CHECK_ERR(err, rc, cleanup);

      if (is_pov) {
        ++*number_pov_headers_in_stage;
      }
      ++*number_phv_records_in_stage;
    }
  }
  goto cleanup;

cleanup:
  return rc;
}

/**
 * Parses the snapshot entry format information for logical tables. Iterates
 * over the tables node for a specific stage number.
 *
 * @param devid The current device's id.
 * @param tables_cjson The cJSON structure corresponding to the tables node.
 * @param stage_number The stage number we are querying.
 * @param table_ptr A pointer to the internal structure corresponding to the
 * first of the logical tables.
 *
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_snapshot_entry_format_table(
    bf_dev_id_t devid,
    cJSON *table_cjson,
    int stage_number,
    pipemgr_tbl_pkg_logical_table_details_t *logical_table_ptr,
    int *number_stage_tables_in_stage,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  *number_stage_tables_in_stage = 0;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  int max_number_stages = dev_info->num_active_mau;
  int max_number_logical_tables =
      dev_info->dev_cfg.stage_cfg.num_logical_tables;

  cJSON **all_stage_tables_cjson = PIPE_MGR_CALLOC(
      max_number_stages * max_number_logical_tables, sizeof(cJSON *));
  if (all_stage_tables_cjson == NULL) {
    LOG_ERROR(
        "%s:%d: Could not allocate memory to parse snapshot entry format "
        "metadata.",
        __func__,
        __LINE__);
    rc = PIPE_NO_SYS_RESOURCES;
    goto cleanup_no_free;
  }

  char *table_name = NULL;
  char *direction = NULL;
  int table_handle = 0;

  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &table_name);
  err |= bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_DIRECTION, &direction);
  err |= bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);

  // Internally, direction is a number, so we explicitly parse it.
  int direction_number = 0;
  if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_INGRESS)) {
    direction_number = BF_TBL_DIR_INGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_EGRESS)) {
    direction_number = BF_TBL_DIR_EGRESS;
  } else if (!strcmp(direction, CTX_JSON_TABLE_DIRECTION_GHOST)) {
    direction_number = BF_TBL_DIR_GHOST;
  } else {
    LOG_ERROR(
        "%s:%d: Table \"%s\" with handle 0x%x has invalid direction \"%s\".",
        __func__,
        __LINE__,
        table_name,
        table_handle,
        direction);
    rc = PIPE_INVALID_ARG;
    goto cleanup;
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

  // Fetch all the stage tables associated to the current table.
  int number_stage_tables = 0;
  if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
    /*
     * For ALPM, we parse the pre classifier completely separately, but we
     * parse the ATCAM tables as if they were the ALPM's.
     * This must happen because Snapshot expects to find the ATCAM units
     * with the ALPM handle.
     */
    cJSON *pre_classifier_cjson = NULL;
    cJSON *atcam_cjson = NULL;

    err |= bf_cjson_get_object(match_attributes_cjson,
                               CTX_JSON_MATCH_ATTRIBUTES_PRE_CLASSIFIER,
                               &pre_classifier_cjson);
    err |= bf_cjson_get_object(match_attributes_cjson,
                               CTX_JSON_MATCH_ATTRIBUTES_ATCAM_TABLE,
                               &atcam_cjson);
    CHECK_ERR(err, rc, cleanup);

    int number_stage_tables_in_pre_classifier = 0;
    rc |= ctx_json_parse_snapshot_entry_format_table(
        devid,
        pre_classifier_cjson,
        stage_number,
        logical_table_ptr,
        &number_stage_tables_in_pre_classifier,
        prof_id);
    CHECK_RC(rc, cleanup);

    logical_table_ptr += number_stage_tables_in_pre_classifier;
    *number_stage_tables_in_stage += number_stage_tables_in_pre_classifier;

    // Fetch all stage tables from the ATCAM table.
    err |= ctx_json_parse_all_match_stage_tables_for_table(
        atcam_cjson,
        max_number_stages * max_number_logical_tables,
        all_stage_tables_cjson,
        &number_stage_tables);
    CHECK_ERR(err, rc, cleanup);
  } else {
    /*
     * The regular case is simple: the stage tables associated to the table
     * are simply the "common ones": either ATCAM units, or stage tables
     * themselves.
     */
    err |= ctx_json_parse_all_match_stage_tables_for_table(
        table_cjson,
        max_number_stages * max_number_logical_tables,
        all_stage_tables_cjson,
        &number_stage_tables);
    CHECK_ERR(err, rc, cleanup);
  }

  // Go over all stage tables associated to this table, and parse the
  // logical table details required for snapshot.
  for (int i = 0; i < number_stage_tables; ++i) {
    cJSON *stage_table_cjson = all_stage_tables_cjson[i];

    int current_stage_number = 0;
    err |= bf_cjson_get_int(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_STAGE_NUMBER,
                            &current_stage_number);
    CHECK_ERR(err, rc, cleanup);

    if (stage_number != current_stage_number) {
      continue;
    }

    char *stage_table_type = NULL;
    int logical_table_id = 0;
    err |= bf_cjson_get_string(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_STAGE_TABLE_TYPE,
                               &stage_table_type);
    err |= bf_cjson_get_int(stage_table_cjson,
                            CTX_JSON_STAGE_TABLE_LOGICAL_TABLE_ID,
                            &logical_table_id);
    CHECK_ERR(err, rc, cleanup);

    logical_table_ptr->logical_id = (stage_number << 4) + logical_table_id;
    logical_table_ptr->table_handle = table_handle;
    logical_table_ptr->direction = direction_number;
    logical_table_ptr->tablename_str_index =
        pipemgr_tbl_pkg_gen_str_index_for_field_name(
            devid, table_name, prof_id);
    logical_table_ptr->stage = stage_number;

    err |= bf_cjson_get_bool(stage_table_cjson,
                             CTX_JSON_STAGE_TABLE_HAS_ATTACHED_GATEWAY,
                             &logical_table_ptr->has_attached_gateway);
    CHECK_ERR(err, rc, cleanup);

    cJSON *result_physical_buses_cjson = NULL;
    err |= bf_cjson_get_object(stage_table_cjson,
                               CTX_JSON_STAGE_TABLE_RESULT_PHYSICAL_BUSES,
                               &result_physical_buses_cjson);
    CHECK_ERR(err, rc, cleanup);

    int number_buses = 0;
    cJSON *bus_cjson = NULL;
    CTX_JSON_FOR_EACH(bus_cjson, result_physical_buses_cjson) {
      logical_table_ptr->physical_buses[number_buses] = bus_cjson->valueint;
      ++number_buses;
    }
    logical_table_ptr->inuse_physical_buses = number_buses;

    logical_table_ptr->table_type_tcam = false;
    logical_table_ptr->tcam_addr_shift = 0;

    // Ternary tables need a little bit more information for shifting the
    // address of the tind.
    if (!strcmp(stage_table_type, CTX_JSON_STAGE_TABLE_TYPE_TERNARY_MATCH)) {
      logical_table_ptr->table_type_tcam = true;

      cJSON *ternary_indirection_stage_table_cjson = NULL;
      err |= bf_cjson_get_object(
          stage_table_cjson,
          CTX_JSON_STAGE_TABLE_TERNARY_INDIRECTION_STAGE_TABLE,
          &ternary_indirection_stage_table_cjson);
      CHECK_ERR(err, rc, cleanup);

      cJSON *pack_formats_cjson = NULL;
      err |= bf_cjson_get_object(ternary_indirection_stage_table_cjson,
                                 CTX_JSON_STAGE_TABLE_PACK_FORMAT,
                                 &pack_formats_cjson);

      cJSON *pack_format_cjson = NULL;
      err |= bf_cjson_get_first(pack_formats_cjson, &pack_format_cjson);
      CHECK_ERR(err, rc, cleanup);

      int table_word_width = 0;
      int entries_per_table_word = 0;
      err |= bf_cjson_get_int(pack_format_cjson,
                              CTX_JSON_PACK_FORMAT_TABLE_WORD_WIDTH,
                              &table_word_width);
      err |= bf_cjson_get_int(pack_format_cjson,
                              CTX_JSON_PACK_FORMAT_ENTRIES_PER_TABLE_WORD,
                              &entries_per_table_word);
      CHECK_ERR(err, rc, cleanup);

      int entry_bit_width = table_word_width / entries_per_table_word;
      switch (entry_bit_width) {
        case 0:
          logical_table_ptr->tcam_addr_shift = 0;
          break;
        case 4:
          logical_table_ptr->tcam_addr_shift = 0;
          break;
        case 8:
          logical_table_ptr->tcam_addr_shift = 1;
          break;
        case 16:
          logical_table_ptr->tcam_addr_shift = 2;
          break;
        case 32:
          logical_table_ptr->tcam_addr_shift = 3;
          break;
        case 64:
          logical_table_ptr->tcam_addr_shift = 4;
          break;
        default:
          LOG_ERROR(
              "%s:%d: Invalid entry bit width %d in the ternary indirection "
              "table within ternary table \"%s\" with handle 0x%x.",
              __func__,
              __LINE__,
              entry_bit_width,
              table_name,
              table_handle);
          rc = PIPE_INVALID_ARG;
          goto cleanup;
      }
    }

    ++*number_stage_tables_in_stage;
    ++logical_table_ptr;
  }
  goto cleanup;

cleanup:
  PIPE_MGR_FREE(all_stage_tables_cjson);
cleanup_no_free:
  return rc;
}

/**
 * Parse the snapshot entry format for a given ContextJSON. This function
 * populates an entry in the snapshot LUT.
 *
 * @param devid The current device's id.
 * @param tables_cjson The cJSON structure corresponding to the tables node.
 * @param phv_allocation_cjson The cJSON structure corresponding to the
 * PHV allocation node.
 * @return A pipe_status_t with the return code.
 */
static pipe_status_t ctx_json_parse_snapshot_entry_format(
    bf_dev_id_t devid,
    cJSON *tables_cjson,
    cJSON *phv_allocation_cjson,
    profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_DBG(
      "%s:%d: Parsing snapshot entry format information.", __func__, __LINE__);

  pipemgr_tbl_pkg_lut_t *snapshot_lut =
      PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut;
  uint32_t lut_depth = PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut_depth;
  int *collision_count_ptr =
      &(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut_collision_count);

  cJSON *stage_phv_allocation_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_phv_allocation_cjson, phv_allocation_cjson) {
    int stage_number = 0;
    err |= bf_cjson_get_int(stage_phv_allocation_cjson,
                            CTX_JSON_STAGE_PHV_ALLOCATION_STAGE_NUMBER,
                            &stage_number);
    CHECK_ERR(err, rc, cleanup);

    // Metadata necessary for building the LUT for each snapshot stage.
    // These were removed from the old ContextJSON, so they must be recomputed
    // here by hand.
    int number_stage_tables_in_stage = 0;
    int number_phv_records_in_stage = 0;
    int number_pov_headers_in_stage = 0;

    rc |= ctx_json_parse_snapshot_entry_format_metadata(
        devid,
        tables_cjson,
        stage_number,
        stage_phv_allocation_cjson,
        &number_stage_tables_in_stage,
        &number_phv_records_in_stage,
        &number_pov_headers_in_stage);
    CHECK_RC(rc, cleanup);

    // Memory required for the entry format.
    int mem_size = 0;
    mem_size += sizeof(pipemgr_tbl_pkg_snapshot_t);
    mem_size +=
        number_phv_records_in_stage * sizeof(pipemgr_tbl_pkg_snapshot_phv_t);
    mem_size +=
        number_pov_headers_in_stage * sizeof(pipemgr_tbl_pkg_snapshot_pov_t);
    mem_size += number_stage_tables_in_stage *
                sizeof(pipemgr_tbl_pkg_logical_table_details_t);

    // Obtain lut_ptr for this stage.
    uint16_t bj_index =
        bob_jenkin_hash_one_at_a_time(lut_depth, 0, stage_number + 1, 0);

    pipemgr_tbl_pkg_lut_t *lut_ptr =
        pipemgr_tbl_pkg_update_lut_entry(snapshot_lut,
                                         lut_depth,
                                         collision_count_ptr,
                                         bj_index,
                                         0,
                                         stage_number + 1,
                                         0);
    if (lut_ptr == NULL) {
      LOG_ERROR("%s:%d: Could not populate snapshot LUT.", __func__, __LINE__);
      rc = PIPE_INIT_ERROR;
      goto cleanup;
    }

    // Allocate the internal structs with the appropriate calculated size.
    lut_ptr->u.snapshot_ptr = PIPE_MGR_CALLOC(1, mem_size);
    if (lut_ptr->u.snapshot_ptr == NULL) {
      LOG_ERROR(
          "%s:%d: Could not allocate memory for snapshot entry internal "
          "structure.",
          __func__,
          __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }

    // Populate common fields with metadata already computed.
    pipemgr_tbl_pkg_snapshot_t *format_ptr = lut_ptr->u.snapshot_ptr;
    format_ptr->total_phv_recs = number_phv_records_in_stage;
    format_ptr->total_pov_hdrs = number_pov_headers_in_stage;
    format_ptr->total_tables = number_stage_tables_in_stage;

    format_ptr->phv_recs = (pipemgr_tbl_pkg_snapshot_phv_t *)(format_ptr + 1);
    format_ptr->pov_hdrs =
        (pipemgr_tbl_pkg_snapshot_pov_t *)(format_ptr->phv_recs +
                                           number_phv_records_in_stage);
    format_ptr->tables =
        (pipemgr_tbl_pkg_logical_table_details_t
             *)(format_ptr->pov_hdrs + number_pov_headers_in_stage);

    pipemgr_tbl_pkg_snapshot_phv_t *phv_record_ptr = format_ptr->phv_recs;
    pipemgr_tbl_pkg_snapshot_pov_t *pov_header_ptr = format_ptr->pov_hdrs;

    // Go through each ingress PHV container, and parse the PHV records.
    cJSON *ingress_stage_phv_allocation_cjson = NULL;
    err |= bf_cjson_get_object(stage_phv_allocation_cjson,
                               CTX_JSON_STAGE_PHV_ALLOCATION_INGRESS,
                               &ingress_stage_phv_allocation_cjson);
    CHECK_ERR(err, rc, cleanup);

    cJSON *phv_container_cjson = NULL;
    CTX_JSON_FOR_EACH(phv_container_cjson, ingress_stage_phv_allocation_cjson) {
      int number_phv_records = 0;
      int number_pov_headers = 0;

      // Direction is 0 for ingress.
      int direction = 0;
      rc |= ctx_json_parse_snapshot_entry_format_phv(devid,
                                                     phv_container_cjson,
                                                     direction,
                                                     phv_record_ptr,
                                                     pov_header_ptr,
                                                     &number_phv_records,
                                                     &number_pov_headers,
                                                     prof_id);
      CHECK_RC(rc, cleanup);

      phv_record_ptr += number_phv_records;
      pov_header_ptr += number_pov_headers;
    }

    // Do the same thing for egress.
    cJSON *egress_stage_phv_allocation_cjson = NULL;
    err |= bf_cjson_get_object(stage_phv_allocation_cjson,
                               CTX_JSON_STAGE_PHV_ALLOCATION_EGRESS,
                               &egress_stage_phv_allocation_cjson);
    CHECK_ERR(err, rc, cleanup);

    phv_container_cjson = NULL;
    CTX_JSON_FOR_EACH(phv_container_cjson, egress_stage_phv_allocation_cjson) {
      int number_phv_records = 0;
      int number_pov_headers = 0;

      // Direction is 1 for egress.
      int direction = 1;
      rc |= ctx_json_parse_snapshot_entry_format_phv(devid,
                                                     phv_container_cjson,
                                                     direction,
                                                     phv_record_ptr,
                                                     pov_header_ptr,
                                                     &number_phv_records,
                                                     &number_pov_headers,
                                                     prof_id);
      CHECK_RC(rc, cleanup);

      phv_record_ptr += number_phv_records;
      pov_header_ptr += number_pov_headers;
    }

    // Parse the stage tables for this stage. For this, we must iterate
    // over all tables and find the stage tables that work.
    pipemgr_tbl_pkg_logical_table_details_t *table_ptr = format_ptr->tables;

    cJSON *table_cjson = NULL;
    CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
      char *table_type = NULL;
      err |= bf_cjson_get_string(
          table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
      CHECK_ERR(err, rc, cleanup);

      // Skip non-match tables.
      if (strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
        continue;
      }

      int number_stage_tables_in_stage_in_table = 0;
      rc |= ctx_json_parse_snapshot_entry_format_table(
          devid,
          table_cjson,
          stage_number,
          table_ptr,
          &number_stage_tables_in_stage_in_table,
          prof_id);
      table_ptr += number_stage_tables_in_stage_in_table;
      CHECK_RC(rc, cleanup);
    }
  }

  return rc;

cleanup:
  return rc;
}

// #### Main Routines ####

/**
 * Given a list of tables, computes some metadata and allocates the global
 * look-up tables that will be populated later.
 *
 * @param devid The current device's id.
 * @param tables_cjson A cJSON array of tables.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_allocate_entry_format_luts(bf_dev_id_t devid,
                                                         cJSON *root,
                                                         profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  LOG_DBG("%s:%d: Parsing ContextJSON metadata to allocate look-up tables.",
          __func__,
          __LINE__);

  cJSON *tables_cjson = NULL;
  cJSON *learn_quanta_cjson = NULL;
  cJSON *phv_allocation_cjson = NULL;
  err |= bf_cjson_get_object(root, CTX_JSON_TABLES_NODE, &tables_cjson);
  err |= bf_cjson_get_object(
      root, CTX_JSON_LEARN_QUANTA_NODE, &learn_quanta_cjson);
  err |= bf_cjson_get_object(
      root, CTX_JSON_PHV_ALLOCATION_NODE, &phv_allocation_cjson);
  CHECK_ERR(err, rc, cleanup);

  int total_matchspec_tables = 0;
  int total_actionspec_tables = 0;
  int total_exact_match_tables = 0;
  int total_tern_match_tables = 0;
  int total_action_data_tables = 0;
  int total_tind_tables = 0;
  int total_default_tables = 0;
  int total_phase0_tables = 0;
  int total_range_tables = 0;
  int total_learn_quanta = cJSON_GetArraySize(learn_quanta_cjson);
  int total_snapshot_stages = cJSON_GetArraySize(phv_allocation_cjson);

  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    char *table_type = NULL;
    err |= bf_cjson_get_string(
        table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
    CHECK_ERR(err, rc, cleanup);

    if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
      bool uses_range = false;
      err |= bf_cjson_get_bool(
          table_cjson, CTX_JSON_MATCH_TABLE_USES_RANGE, &uses_range);
      CHECK_ERR(err, rc, cleanup);

      if (uses_range) {
        ++total_range_tables;
      }

      cJSON *actions_cjson = NULL;
      err |= bf_cjson_get_object(
          table_cjson, CTX_JSON_TABLE_ACTIONS, &actions_cjson);
      CHECK_ERR(err, rc, cleanup);

      total_actionspec_tables += cJSON_GetArraySize(actions_cjson);

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

      if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT)) {
        ++total_default_tables;
        ++total_matchspec_tables;

        cJSON *stage_tables_cjson = NULL;
        err |= bf_cjson_get_object(match_attributes_cjson,
                                   CTX_JSON_MATCH_ATTRIBUTES_STAGE_TABLES,
                                   &stage_tables_cjson);
        CHECK_ERR(err, rc, cleanup);
        total_exact_match_tables += cJSON_GetArraySize(stage_tables_cjson);
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ATCAM)) {
        ++total_default_tables;
        ++total_matchspec_tables;
        ++total_exact_match_tables;
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
        // Count the ATCAM physical table.
        ++total_exact_match_tables;
        ++total_default_tables;
        ++total_matchspec_tables;

        // Also count the pre classifier.
        ++total_default_tables;
        ++total_matchspec_tables;
        ++total_tern_match_tables;

        cJSON *pre_classifier_cjson = NULL;
        err |= bf_cjson_get_object(match_attributes_cjson,
                                   CTX_JSON_MATCH_ATTRIBUTES_PRE_CLASSIFIER,
                                   &pre_classifier_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Pre classifier actions.
        cJSON *pre_classifier_actions_cjson = NULL;
        err |= bf_cjson_get_object(pre_classifier_cjson,
                                   CTX_JSON_TABLE_ACTIONS,
                                   &pre_classifier_actions_cjson);
        CHECK_ERR(err, rc, cleanup);
        total_actionspec_tables +=
            cJSON_GetArraySize(pre_classifier_actions_cjson);

        cJSON *pre_classifier_match_attributes_cjson = NULL;
        err |= bf_cjson_get_object(pre_classifier_cjson,
                                   CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                                   &pre_classifier_match_attributes_cjson);
        CHECK_ERR(err, rc, cleanup);

        // Pre classifier tinds.
        cJSON *stage_tables_cjson = NULL;
        err |= bf_cjson_get_object(pre_classifier_match_attributes_cjson,
                                   CTX_JSON_MATCH_ATTRIBUTES_STAGE_TABLES,
                                   &stage_tables_cjson);
        CHECK_ERR(err, rc, cleanup);
        total_tind_tables += cJSON_GetArraySize(stage_tables_cjson);
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_HASH_ACTION)) {
        ++total_default_tables;
        ++total_matchspec_tables;
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_TERNARY)) {
        ++total_matchspec_tables;

        cJSON *stage_tables_cjson = NULL;
        err |= bf_cjson_get_object(match_attributes_cjson,
                                   CTX_JSON_MATCH_ATTRIBUTES_STAGE_TABLES,
                                   &stage_tables_cjson);
        CHECK_ERR(err, rc, cleanup);
        int stage_tables = cJSON_GetArraySize(stage_tables_cjson);
        total_tind_tables += stage_tables;
        total_tern_match_tables += stage_tables;

        ++total_default_tables;
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_NO_KEY)) {
        ++total_default_tables;
        ++total_matchspec_tables;
      } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_PHASE0)) {
        ++total_phase0_tables;
        ++total_matchspec_tables;
      } else {
        LOG_ERROR("%s:%d: Invalid match_type \"%s\" found in ContextJSON.",
                  __func__,
                  __LINE__,
                  match_type);
        rc = PIPE_INVALID_ARG;
        goto cleanup;
      }
    } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_ACTION_DATA)) {
      ++total_action_data_tables;
    }
  }

  // Allocate LUTs.
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_adt_tbl = total_action_data_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_action_data_tables);
  if (total_action_data_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).adt_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for action data LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_tind_tbl = total_tind_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_tind_tables);
  if (total_tind_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tind_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for ternary indirection LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_exm_tbl = total_exact_match_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_exact_match_tables);
  if (total_exact_match_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).exm_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for exact match LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_tern_tbl = total_tern_match_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_tern_match_tables);
  if (total_tern_match_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).tern_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for ternary match LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_matchspec_tbls =
      total_matchspec_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_matchspec_tables);
  if (total_matchspec_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).matchspec_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for match spec LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_actionspec_tbls =
      total_actionspec_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_actionspec_tables);
  if (total_actionspec_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).actionspec_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for action spec LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_rangetbls = total_range_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_range_tables);
  if (total_range_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).rangetbl_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for range tables LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_phase0_tbls = total_phase0_tables;

  // There aren't many Phase0 tables, so this doubles to increase hashing
  // into LUT entropy.
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut_depth =
      2 * PIPEMGR_TBL_PKG_2POWER(total_phase0_tables);
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut_depth =
      2 * PIPEMGR_TBL_PKG_2POWER(total_phase0_tables);

  if (total_phase0_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_match_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for phase0 match LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).phase0_action_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for phase0 action LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_dft_entries = total_default_tables;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_default_tables);
  if (total_default_tables > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).dft_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for default entry LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_lqs = total_learn_quanta;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut_depth =
      2 * PIPEMGR_TBL_PKG_2POWER(total_learn_quanta);
  if (total_learn_quanta > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut =
        PIPE_MGR_CALLOC(PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut_depth *
                            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
                        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).lq_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for learn quanta LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).total_snapshot_stages =
      total_snapshot_stages;
  PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut_depth =
      PIPEMGR_TBL_PKG_2POWER(total_snapshot_stages);
  if (total_snapshot_stages > 0) {
    PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut = PIPE_MGR_CALLOC(
        PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut_depth *
            PIPEMGR_TBL_PKG_HASH_COLLISION_FACTOR,
        sizeof(pipemgr_tbl_pkg_lut_t));
    if (PIPE_MGR_TBL_PKG_CTX(devid, prof_id).snapshot_lut == NULL) {
      LOG_ERROR("%s:%d: Could not allocate memory for snapshot LUT.",
                __func__,
                __LINE__);
      rc = PIPE_NO_SYS_RESOURCES;
      goto cleanup;
    }
  }

  return rc;

cleanup:
  return rc;
}

/**
 * Parses a given table cJSON into its appropriate entry format LUTs.
 *
 * Assumes that look-up tables have already been allocated (by calling
 * ctx_json_allocate_entry_format_luts).
 *
 * @param devid The current device's id.
 * @param tables_cjson A cJSON array of tables.
 * @return A pipe_status_t containing the return code.
 */
static pipe_status_t ctx_json_parse_table_entry_format(bf_dev_id_t devid,
                                                       cJSON *table_cjson,
                                                       profile_id_t prof_id) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(devid);
  PIPE_MGR_ASSERT(dev_info != NULL);
  if (!dev_info) {
    LOG_ERROR("%s: Unable to use dev id %d", __func__, devid);
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  rmt_dev_profile_info_t *prof_info = dev_info->profile_info[prof_id];

  int table_handle = 0;
  char *name = NULL;
  err = bf_cjson_get_handle(
      devid, prof_id, table_cjson, CTX_JSON_TABLE_HANDLE, &table_handle);
  CHECK_ERR(err, rc, cleanup);
  err = bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_NAME, &name);
  CHECK_ERR(err, rc, cleanup);

  char *table_type = NULL;
  err =
      bf_cjson_get_string(table_cjson, CTX_JSON_TABLE_TABLE_TYPE, &table_type);
  CHECK_ERR(err, rc, cleanup);

  if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_MATCH)) {
    // If this table uses ranges, then parse the range entry format.
    bool uses_range = false;
    err = bf_cjson_get_bool(
        table_cjson, CTX_JSON_MATCH_TABLE_USES_RANGE, &uses_range);
    CHECK_ERR(err, rc, cleanup);

    if (uses_range) {
      rc = ctx_json_parse_range_table_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    }

    // Parse the appropriate entry formats. Notice that ternary indirection
    // parsing will be called within ternary parsing.
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

    if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_EXACT) ||
        !strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ATCAM)) {
      rc |=
          ctx_json_parse_exact_match_entry_format(devid, table_cjson, prof_id);
      rc |= ctx_json_parse_default_entry_format(devid, table_cjson, prof_id);
      rc |=
          ctx_json_parse_match_spec_entry_format(devid, table_cjson, prof_info);
      rc |=
          ctx_json_parse_action_spec_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_ALPM)) {
      /*
       * Differently from other types of tables, ALPM is simply just a wrapper
       * to a ternary table and an ATCAM table. Here, we call routines to parse
       * the ternary (pre classifier) and the ATCAM itself.
       */

      // Parse the pre classifier entry format.
      cJSON *pre_classifier_cjson = NULL;
      err |= bf_cjson_get_object(match_attributes_cjson,
                                 CTX_JSON_MATCH_ATTRIBUTES_PRE_CLASSIFIER,
                                 &pre_classifier_cjson);
      CHECK_ERR(err, rc, cleanup);

      rc |= ctx_json_parse_ternary_match_entry_format(
          devid, pre_classifier_cjson, prof_id);
      rc |= ctx_json_parse_default_entry_format(
          devid, pre_classifier_cjson, prof_id);
      rc |= ctx_json_parse_match_spec_entry_format(
          devid, pre_classifier_cjson, prof_info);
      rc |= ctx_json_parse_action_spec_entry_format(
          devid, pre_classifier_cjson, prof_id);
      CHECK_RC(rc, cleanup);

      // Parse the ATCAM table.
      cJSON *atcam_table_cjson = NULL;

      err |= bf_cjson_get_object(match_attributes_cjson,
                                 CTX_JSON_MATCH_ATTRIBUTES_ATCAM_TABLE,
                                 &atcam_table_cjson);
      CHECK_ERR(err, rc, cleanup);

      rc |= ctx_json_parse_default_entry_format(
          devid, atcam_table_cjson, prof_id);
      rc |= ctx_json_parse_exact_match_entry_format(
          devid, atcam_table_cjson, prof_id);
      rc |= ctx_json_parse_match_spec_entry_format(
          devid, atcam_table_cjson, prof_info);
      rc |= ctx_json_parse_action_spec_entry_format(
          devid, atcam_table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_HASH_ACTION)) {
      rc |= ctx_json_parse_default_entry_format(devid, table_cjson, prof_id);
      rc |=
          ctx_json_parse_match_spec_entry_format(devid, table_cjson, prof_info);
      rc |=
          ctx_json_parse_action_spec_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_NO_KEY)) {
      rc |= ctx_json_parse_default_entry_format(devid, table_cjson, prof_id);
      rc |=
          ctx_json_parse_match_spec_entry_format(devid, table_cjson, prof_info);
      rc |=
          ctx_json_parse_action_spec_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_PHASE0)) {
      rc |= ctx_json_parse_phase0_match_entry_format(
          devid, table_cjson, prof_info);
      rc |= ctx_json_parse_phase0_action_entry_format(
          devid, table_cjson, prof_id);
      rc |=
          ctx_json_parse_match_spec_entry_format(devid, table_cjson, prof_info);
      rc |=
          ctx_json_parse_action_spec_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else if (!strcmp(match_type, CTX_JSON_MATCH_TABLE_TYPE_TERNARY)) {
      rc |= ctx_json_parse_ternary_match_entry_format(
          devid, table_cjson, prof_id);
      rc |= ctx_json_parse_default_entry_format(devid, table_cjson, prof_id);
      rc |=
          ctx_json_parse_match_spec_entry_format(devid, table_cjson, prof_info);
      rc |=
          ctx_json_parse_action_spec_entry_format(devid, table_cjson, prof_id);
      CHECK_RC(rc, cleanup);
    } else {
      LOG_ERROR("%s:%d: Invalid match type \"%s\" found in ContextJSON.",
                __func__,
                __LINE__,
                match_type);
      rc = PIPE_INVALID_ARG;
      goto cleanup;
    }
  } else if (!strcmp(table_type, CTX_JSON_TABLE_TYPE_ACTION_DATA)) {
    rc |= ctx_json_parse_action_data_entry_format(devid, table_cjson, prof_id);
  }

  return rc;

cleanup:
  LOG_ERROR("%s - Dev %d Profile %d: Failed to parse table %s (0x%x)",
            __func__,
            devid,
            prof_id,
            name,
            table_handle);
  return rc;
}

/**
 * Main routine that is publically exposed. Given a cJSON object representing
 * the entire ContextJSON object, this function parses its contents into the
 * driver's internal entry format. The first pass consists of allocating the
 * global context lookup table (LUT) structures. The second pass looks at each
 * particular table type and calls the appropriate callback to parse it.
 *
 * @param devid The device id in question.
 * @param root A cJSON object representing the ContextJSON.
 * @return A pipe_status_t object with the return code.
 */
pipe_status_t ctx_json_parse_entry_format(bf_dev_id_t devid,
                                          profile_id_t prof_id,
                                          cJSON *root) {
  int err = 0;
  pipe_status_t rc = PIPE_SUCCESS;

  if (devid >= BF_MAX_DEV_COUNT) {
    LOG_ERROR(
        "Invalid device id. Unable to complete table packing module init");
    rc = PIPE_INVALID_ARG;
    goto cleanup;
  }

  LOG_DBG("%s:%d: Started parsing entry format from Context JSON.",
          __func__,
          __LINE__);

  cJSON *tables_cjson = NULL;
  cJSON *learn_quanta_cjson = NULL;
  cJSON *phv_allocation_cjson = NULL;
  err |= bf_cjson_get_object(root, CTX_JSON_TABLES_NODE, &tables_cjson);
  err |= bf_cjson_get_object(
      root, CTX_JSON_LEARN_QUANTA_NODE, &learn_quanta_cjson);
  err |= bf_cjson_get_object(
      root, CTX_JSON_PHV_ALLOCATION_NODE, &phv_allocation_cjson);
  CHECK_ERR(err, rc, cleanup);

  // Allocate the LUTs for all types of tables.
  rc |= ctx_json_allocate_entry_format_luts(devid, root, prof_id);

  // Traverse all tables, calling the appropriate callbacks for each table
  // type.
  LOG_DBG("%s:%d: Traversing tables to populate entry format structures.",
          __func__,
          __LINE__);
  cJSON *table_cjson = NULL;
  CTX_JSON_FOR_EACH(table_cjson, tables_cjson) {
    rc |= ctx_json_parse_table_entry_format(devid, table_cjson, prof_id);
    CHECK_RC(rc, cleanup);
  }

  // Parse the learn quanta entry format.
  LOG_DBG("%s:%d: Parsing learn quanta entry format structures.",
          __func__,
          __LINE__);
  cJSON *learn_quantum_cjson = NULL;
  CTX_JSON_FOR_EACH(learn_quantum_cjson, learn_quanta_cjson) {
    rc |= ctx_json_parse_learn_quantum_entry_format(
        devid, learn_quantum_cjson, prof_id);
    CHECK_RC(rc, cleanup);
  }

  // Parse snapshot entry format.
  rc |= ctx_json_parse_snapshot_entry_format(
      devid, tables_cjson, phv_allocation_cjson, prof_id);
  CHECK_RC(rc, cleanup);

  LOG_DBG("%s:%d: Successfully populated entry format structures.",
          __func__,
          __LINE__);
  return rc;

cleanup:
  // FIXME: if rc is unsuccessful, clean up.
  return rc;
}
