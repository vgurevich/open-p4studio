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
 * @file ctx_json_utils.h
 *
 * Utility functions used by the driver to facilitate parsing cJSON structures,
 * as well as macro definitions for the Context JSON's fields.
 */

#include <bf_types/bf_types.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <stdio.h>
#include <string.h>

#include <ctx_json/ctx_json_utils.h>
#include "ctx_json_log.h"

#define PAD_TO_BYTE(x) (8 * (((x) + 7) / 8))

#define CHECK_ERR_INTERNAL(err, label, file, line, func)                \
  if ((err)) {                                                          \
    LOG_ERROR(                                                          \
        "%s:%d: An error has occurred while parsing the Context JSON. " \
        "Function %s cannot continue.",                                 \
        (file),                                                         \
        (line),                                                         \
        (func));                                                        \
    goto label;                                                         \
  }

#define CHECK_ERR(err, label) \
  CHECK_ERR_INTERNAL(err, label, __FILE__, __LINE__, __func__)

int bf_cjson_get_string_dup(cJSON *cjson, char *property, char **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  *ret = bf_sys_strdup(tmp->valuestring);
  if (*ret == NULL) {
    return -1;
  }
  return 0;
}

/**
 * Fetch a string with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the string which will be set to the value in the
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_string(cJSON *cjson, char *property, char **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }
  if (tmp->type != cJSON_String) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a string.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  *ret = tmp->valuestring;
  return 0;
}

/**
 * Same as bf_cjson_get_int, except that it fails softly: the ret pointer will
 * be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the string which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_string(cJSON *cjson, char *property, char **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type == cJSON_String) {
    *ret = tmp->valuestring;
  }
  return 0;
}

/**
 * Same as bf_cjson_get_string_dup, except that it fails softly: the ret
 * pointer will be unchanged, but the return code is still successful. Should
 * be used only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the string which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_string_dup(cJSON *cjson, char *property, char **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type == cJSON_String) {
    *ret = bf_sys_strdup(tmp->valuestring);
  }
  return 0;
}

void ctx_json_hex_to_stream(char *hex, uint8_t *stream, uint8_t len) {
  /* The first two characters of the string are always '0x', so we can ignore
   * them. Change the 'x' to a '0' if the length is odd so we can scan
   * two chars at a time.
   */
  if (strlen(hex) % 2 != 0) {
    hex[1] = '0';
  }

  /* Scan from lsb since the hex string isn't zero-padded */
  char *hex_iter = hex + strlen(hex) - 2;
  uint8_t *byte_iter = stream + len - 1;
  for (; hex_iter > hex && byte_iter >= stream; hex_iter -= 2, byte_iter--) {
    sscanf(hex_iter, "%2hhx", byte_iter);
  }

  if (strlen(hex) % 2 != 0) {
    hex[1] = 'x';
  }
  return;
}

/**
 * Fetch a hex string with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the bytestream to be updated.
 * @param len Length of the input bytestream.
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_hex(cJSON *cjson, char *property, uint8_t *ret, uint8_t len) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }
  if (tmp->type != cJSON_String) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a string.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  ctx_json_hex_to_stream(tmp->valuestring, ret, len);
  return 0;
}

/**
 * Same as bf_cjson_get_hex, except that it fails softly: the ret
 * pointer will be unchanged, but the return code is still successful. Should
 * be used only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the bytestream to be updated.
 * @param len Length of the input bytestream.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_hex(cJSON *cjson,
                         char *property,
                         uint8_t *ret,
                         uint8_t len) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp && tmp->type == cJSON_String) {
    ctx_json_hex_to_stream(tmp->valuestring, ret, len);
  }
  return 0;
}

/**
 * Checks whether the specified property is present and if it is a string.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 *
 * @return true if present, otherwise false.
 */
int bf_cjson_has_hex(cJSON *cjson, char *property) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  return tmp && tmp->type == cJSON_String;
}

/**
 * Fetch an integer with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 * Because cJSON always parses numbers as int's, the return pointer must be
 * an int*.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the integer which will be set to the value in the
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_int(cJSON *cjson, char *property, int *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }
  if (tmp->type != cJSON_Number) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a number.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  *ret = tmp->valueint;
  return 0;
}

/**
 * Same as bf_cjson_get_int, except that it fails softly: the ret pointer will
 * be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the integer which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_int(cJSON *cjson, char *property, int *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type == cJSON_Number) {
    *ret = tmp->valueint;
  }
  return 0;
}

/**
 * Checks if the specified properity is present and is a number.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 *
 * @return true if found, otherwise false.
 */
bool bf_cjson_has_int(cJSON *cjson, const char *property) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  return tmp != NULL && tmp->type == cJSON_Number;
}

/**
 * Fetch a double with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the double which will be set to the value in the
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_double(cJSON *cjson, char *property, double *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }
  if (tmp->type != cJSON_Number) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a number.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  *ret = tmp->valuedouble;
  return 0;
}

/**
 * Same as bf_cjson_get_double, except that it fails softly: the ret pointer
 * will be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the double which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_double(cJSON *cjson, char *property, double *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type == cJSON_Number) {
    *ret = tmp->valuedouble;
  }
  return 0;
}

/**
 * Fetch a boolean with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the boolean which will be set to the value in the
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_bool(cJSON *cjson, char *property, bool *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  if (tmp->type == cJSON_False) {
    *ret = false;
  } else if (tmp->type == cJSON_True) {
    *ret = true;
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a boolean.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  return 0;
}

/**
 * Same as bf_cjson_get_bool, except that it fails softly: the ret pointer will
 * be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the boolean which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_bool(cJSON *cjson, char *property, bool *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    return 0;
  }

  if (tmp->type == cJSON_False) {
    *ret = false;
  } else if (tmp->type == cJSON_True) {
    *ret = true;
  }

  return 0;
}

/**
 * Fetch the first cJSON object with a cJSON array. This function will return
 * an error if the passed in cJSON is not an array, or if it is empty.
 *
 * @param cjson The parent cJSON structure.
 * @param ret A pointer to the cJSON* object which will be set to the object
 * corresponding to the property name given.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_first(cJSON *cjson, cJSON **ret) {
  if (cjson->type != cJSON_Array) {
    LOG_ERROR("%s:%d: Invalid ContextJSON format: given cJSON is not an array.",
              __FILE__,
              __LINE__);
    return -1;
  }

  cJSON *tmp = cjson->child;
  if (tmp == NULL) {
    LOG_ERROR("%s:%d: Invalid ContextJSON format: given cJSON array is empty.",
              __FILE__,
              __LINE__);
    return -1;
  }

  *ret = tmp;
  return 0;
}

/**
 * Same as bf_cjson_get_first, except that it fails softly: the return value
 * will be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param ret A pointer to the cJSON* object which will be set to the object
 * corresponding to the property name given.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_first(cJSON *cjson, cJSON **ret) {
  if (cjson->type != cJSON_Array || cjson->child == NULL) {
    return 0;
  }

  *ret = cjson->child;
  return 0;
}

/**
 * Fetch the ith cJSON object from a cJSON array. This function will return
 * an error if the passed in cJSON is not an array, or if it is empty.
 *
 * @param cjson The cJSON array structure.
 * @param i The index of the array element to get.
 * @param ret A pointer to the cJSON* object which will be set to the ith
 *            entry in the array.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_array_item(cJSON *cjson, int i, cJSON **ret) {
  if (!cjson || i < 0 || !ret) {
    LOG_ERROR("%s:%d: Illegal arguments %p %d %p",
              __func__,
              __LINE__,
              (void *)cjson,
              i,
              (void *)ret);
    return -1;
  }
  if (cjson->type != cJSON_Array) {
    LOG_ERROR("%s:%d: Invalid ContextJSON format: given cJSON is not an array.",
              __FILE__,
              __LINE__);
    return -1;
  }
  int sz = cJSON_GetArraySize(cjson);
  if (i >= sz) {
    LOG_ERROR("%s:%d: Invalid request for item %d, size is %d",
              __func__,
              __LINE__,
              i,
              sz);
    return -1;
  }

  *ret = cJSON_GetArrayItem(cjson, i);
  if (*ret == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: given cJSON array item is null.",
        __FILE__,
        __LINE__);
    return -1;
  }

  return 0;
}

/**
 * Fetch a cJSON object with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist, or if the
 * property maps to a cJSON_NULL object.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the cJSON* object which will be set to the object
 * corresponding to the property name given.
 *
 * @return 0 if successful, -1 on error.
 */
int bf_cjson_get_object(cJSON *cjson, char *property, cJSON **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL || tmp->type == cJSON_NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  *ret = tmp;
  return 0;
}

/**
 * Same as bf_cjson_get_object, except that it fails softly: the return value
 * will be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the cJSON* object which will be set to the object
 * corresponding to the property name given.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_object(cJSON *cjson, char *property, cJSON **ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type != cJSON_NULL) {
    *ret = tmp;
  }
  return 0;
}

/**
 * Fetch a Table Handle with a given name within a cJSON structure. This
 * function will return an error if the property name does not exist.
 * Because cJSON always parses numbers as int's, the return pointer must be
 * an int*.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the integer which will be set to the value in the
 * property.
 *
 * @return 0 if successful, -1 on error.
 */
extern int pipe_mgr_tbl_hdl_set_pipe(bf_dev_id_t devid,
                                     profile_id_t prof_id,
                                     pipe_tbl_hdl_t handle,
                                     pipe_tbl_hdl_t *ret_handle);
int bf_cjson_get_handle(bf_dev_id_t devid,
                        profile_id_t prof_id,
                        cJSON *cjson,
                        char *property,
                        int *ret) {
  pipe_tbl_hdl_t ret_handle = 0;
  int rc = 0;
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp == NULL) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: could not find cJSON property "
        "\"%s\".",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }
  if (tmp->type != cJSON_Number) {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: property \"%s\" is not a number.",
        __FILE__,
        __LINE__,
        property);
    return -1;
  }

  rc = pipe_mgr_tbl_hdl_set_pipe(devid, prof_id, tmp->valueint, &ret_handle);
  if (rc != 0) {
    return rc;
  }
  *ret = ret_handle;
  return 0;
}

/**
 * Same as bf_cjson_get_handle, except that it fails softly: the ret pointer
 *will
 * be unchanged, but the return code is still successful. Should be used
 * only for optional properties.
 *
 * @param cjson The parent cJSON structure.
 * @param property The name of the property searched for in the parent.
 * @param ret A pointer to the integer which will be set to the value in the
 * property.
 *
 * @return 0, it always succeeds.
 */
int bf_cjson_try_get_handle(bf_dev_id_t devid,
                            profile_id_t prof_id,
                            cJSON *cjson,
                            char *property,
                            int *ret) {
  cJSON *tmp = cJSON_GetObjectItem(cjson, property);
  if (tmp != NULL && tmp->type == cJSON_Number) {
    pipe_tbl_hdl_t ret_handle = 0;
    int rc = 0;
    rc = pipe_mgr_tbl_hdl_set_pipe(devid, prof_id, tmp->valueint, &ret_handle);
    if (rc != 0) {
      return rc;
    }
    *ret = ret_handle;
  }
  return 0;
}

/**
 * Given an action and a field name, this function finds the length of the field
 * and its offset in the action spec. The return pointers may be NULL, in which
 * case the function does not try to change its value.
 *
 * @param action_cjson The cJSON structure corresponding to the action.
 * @param field_name The name of the field to be queried.
 * @param spec_length_ret A pointer which will be set to the field's length.
 * @param spec_offset_ret A pointer which will be set to the field's offset.
 *
 * @return 0 on success, -1 on failure.
 */
int ctx_json_parse_action_spec_details_for_field(cJSON *action_cjson,
                                                 char *field_to_find,
                                                 int *spec_length_ret,
                                                 int *spec_offset_ret) {
  int err = 0;
  bf_sys_assert(field_to_find != NULL);

  // Special fields that should not be looked for in the action spec.
  if (!strncmp(field_to_find, "--", 2) || !strncmp(field_to_find, "$", 1)) {
    if (spec_length_ret != NULL) {
      *spec_length_ret = 0;
    }
    if (spec_offset_ret != NULL) {
      *spec_offset_ret = 0;
    }
    return 0;
  }

  cJSON *p4_parameters_cjson = NULL;
  err = bf_cjson_get_object(
      action_cjson, CTX_JSON_ACTION_P4_PARAMETERS, &p4_parameters_cjson);
  CHECK_ERR(err, cleanup);

  int spec_offset = 0;
  cJSON *p4_parameter_cjson = NULL;

  CTX_JSON_FOR_EACH(p4_parameter_cjson, p4_parameters_cjson) {
    int param_width = 0;
    char *param_name = NULL;
    err = bf_cjson_get_int(
        p4_parameter_cjson, CTX_JSON_P4_PARAMETER_BIT_WIDTH, &param_width);
    CHECK_ERR(err, cleanup);
    err = bf_cjson_get_string(
        p4_parameter_cjson, CTX_JSON_P4_PARAMETER_NAME, &param_name);
    CHECK_ERR(err, cleanup);

    // Found a match.
    if (!strcmp(param_name, field_to_find)) {
      if (spec_offset_ret != NULL) {
        int padding = PAD_TO_BYTE(param_width) - param_width;
        *spec_offset_ret = spec_offset + padding;
      }
      if (spec_length_ret != NULL) {
        *spec_length_ret = param_width;
      }
      break;
    }
    spec_offset += PAD_TO_BYTE(param_width);
  }

  // Make sure we have found the field.
  if (p4_parameter_cjson == NULL) {
    LOG_ERROR("%s:%d: Could not find field %s in action spec.",
              __FILE__,
              __LINE__,
              field_to_find);
    return -1;
  }
  return err;

cleanup:
  return err;
}

/* Parse the "match_key_fields" section of a table's context.json entry to
 * determine the size of the key in bytes. */
int ctx_json_parse_spec_details_for_key_length(cJSON *match_key_fields_cjson,
                                               int *key_length) {
  int err = 0;
  cJSON *field_cjson = NULL;
  int num_match_fields = GETARRSZ(match_key_fields_cjson);
  int position_to_size[num_match_fields];
  int max_position = -1;

  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    int field_pos;
    int field_width;
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_POSITION, &field_pos);
    CHECK_ERR(err, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL, &field_width);
    CHECK_ERR(err, cleanup);
    if (field_pos < 0 || field_pos >= num_match_fields || field_width < 0) {
      return -1;
    }
    if (field_pos > max_position) {
      max_position = field_pos;
    }
    position_to_size[field_pos] = field_width;
  }
  int byte_count = 0;
  for (int i = 0; i <= max_position; ++i) {
    byte_count += PAD_TO_BYTE(position_to_size[i]) / 8;
  }

  *key_length = byte_count;

cleanup:
  return err;
}

/**
 * Given the context.json's schema version, return true if it is equal to or
 * greater than the specified version.
 * @param schema_version [IN] The current context.json's version.
 * @param major [IN] The specified major version to compare against.
 * @param minor [IN] The specified minor version to compare against.
 * @param patch [IN] The specified patch version to compare against.
 * return True if the specified version (major.minor.patch) is greater than or
 * equal to schema_version. */
bool ctx_json_schema_version_at_least(const int *schema_version,
                                      int major,
                                      int minor,
                                      int patch) {
  if (schema_version[0] > major) {
    return true;
  }
  if (schema_version[0] == major) {
    if (schema_version[1] > minor) {
      return true;
    }
    if (schema_version[1] == minor) {
      return schema_version[2] >= patch;
    }
  }
  return false;
}

/**
 * Given match_key_fields and a field name, this function finds the length of
 * the field, its offset and match type in the match spec. The return pointers
 * may be NULL, in which case the function does not try to change its value.
 *
 * @param use_global_name [IN] Compare field_to_find to "global_name" or "name",
 *                             the caller should set it based on the source of
 *                             the field_to_find argument.
 * @param match_key_fields_cjson [IN] The cJSON structure corresponding to the
 *                                    table's "match_key_fields".
 * @param field_to_find [IN] The global name of the field to be queried.
 * @param field_start [IN] The starting bit of this field.
 * @param field_width [IN] The length in bits of the field to find.
 * @param spec_length_ret [OUT] A pointer which will be set to the field's
 *                              length in the match spec.
 * @param spec_offset_ret [OUT] A pointer which will be set to the field's
 *                              offset in the match spec.
 * @param ms_bit          [OUT] A pointer which will be set to the field bit's
 *                              offset in the match spec.  Note that the offset
 *                              of field_start is always returned, even if
 *                              field_width is more than one.
 * @param is_range [OUT] A pointer to a bool which will be set to true if the
 *                       field is a range match and false otherwise.
 *
 * @return 0 on success, -1 on failure.
 */
int ctx_json_parse_spec_details_for_field(bool use_global_name,
                                          cJSON *match_key_fields_cjson,
                                          const char *field_to_find,
                                          int field_start,
                                          int field_width,
                                          int *spec_length_ret,
                                          int *spec_offset_ret,
                                          int *ms_bit,
                                          bool *is_range) {
  int err = 0;
  bf_sys_assert(field_to_find != NULL);

  // Special fields that should not be looked for in the match spec.
  if (!strncmp(field_to_find, "--", 2) || !strncmp(field_to_find, "$", 1)) {
    if (spec_length_ret != NULL) {
      *spec_length_ret = 0;
    }
    if (spec_offset_ret != NULL) {
      *spec_offset_ret = 0;
    }
    if (ms_bit) *ms_bit = 0;
    if (is_range) *is_range = false;
    return 0;
  }

  /* Go over all entries of the match_key_fields.  Populate a position-to-size
   * map and find the position of the field (or field slice) which has been
   * requested. */
  int num_match_fields = GETARRSZ(match_key_fields_cjson);
  int position_to_size[num_match_fields];
  int slice_pos = -1;
  cJSON *field_cjson = NULL;
  CTX_JSON_FOR_EACH(field_cjson, match_key_fields_cjson) {
    char *field_name = NULL;
    int slice_start = 0;
    int slice_width = 0;
    int slice_end = 0;
    int field_pos;
    int full_width;

    /* Start with the "name", if we are using global names attempt to get that
     * instead.  The "global_name" will only be present in the match_key_fields
     * if it is different than "name".  Note that with older context.json schema
     * versions we do NOT use "global_name" because the caller does not have
     * access to the field's global-name, they only have name.  Only with schema
     * versions 1.12.0 and later will the callers have a global name. */
    err = bf_cjson_get_string(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_NAME, &field_name);
    CHECK_ERR(err, cleanup);
    if (use_global_name) {
      bf_cjson_try_get_string(
          field_cjson, CTX_JSON_MATCH_KEY_FIELDS_GLOBAL_NAME, &field_name);
    }

    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_POSITION, &field_pos);
    CHECK_ERR(err, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL, &full_width);
    CHECK_ERR(err, cleanup);
    if (field_pos < 0 || field_pos >= num_match_fields || full_width < 0) {
      return -1;
    }
    position_to_size[field_pos] = full_width;

    /* The current field from the match_key_fields section of the json may have
     * the same name as our requested field but we also need to validate the
     * field is covering the same bit range as our requested field.  The
     * match_key_fields section may have entries with duplicate names for
     * various slices of the field  so we check if this slice contains our
     * requested field. */
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, &slice_start);
    CHECK_ERR(err, cleanup);
    err = bf_cjson_get_int(
        field_cjson, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, &slice_width);
    CHECK_ERR(err, cleanup);
    slice_end = slice_start + slice_width - 1;

    if (!strcmp(field_name, field_to_find) &&
        (slice_start <= field_start && field_start <= slice_end)) {
      /* We've found the requested field/field-slice! */
      slice_pos = field_pos;

      /* Pick up the match type if it was requested. */
      if (is_range) {
        char *match_type = NULL;
        err = bf_cjson_get_string(
            field_cjson, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type);
        CHECK_ERR(err, cleanup);
        if (!strcmp(match_type, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_RANGE)) {
          *is_range = true;
        } else {
          *is_range = false;
        }
      }
    }
  }

  if (slice_pos == -1) {
    LOG_ERROR(
        "%s:%d: Could not find field %s start_bit %d length %d in match spec.",
        __FILE__,
        __LINE__,
        field_to_find,
        field_start,
        field_width);
    return -1;
  }
  if (spec_offset_ret) {
    int ms_offset = 0;
    for (int i = 0; i < slice_pos; ++i) {
      ms_offset += PAD_TO_BYTE(position_to_size[i]);
    }
    int spec_field_width = position_to_size[slice_pos];
    int padding = PAD_TO_BYTE(spec_field_width) - spec_field_width;
    *spec_offset_ret = spec_field_width > 8 ? (ms_offset + padding) : ms_offset;
  }
  if (spec_length_ret != NULL) {
    int spec_field_width = position_to_size[slice_pos];
    *spec_length_ret = spec_field_width;
  }
  if (ms_bit) {
    /* Convert the field and field bit to a bit in the match spec.  For example
     * if the match spec has a 6-bit field A and a 9-bit field B:
     *
     *    MS Bit Number:                        1 1 1 1 1 1     2 2 2 2 1 1 1 1
     *    7-0,15-8,23-16:       7 6 5 4 3 2 1 0 5 4 3 2 1 0 9 8 3 2 1 0 9 8 7 6
     *                         +---------------+---------------+---------------+
     *    Match Spec:          |    A A A A A A|              B|B B B B B B B B|
     *                         +---------------+---------------+---------------+
     *    Bit-in-Byte:          7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
     *    Byte Offset:             Byte 0          Byte 1          Byte 2
     *
     * Field A has field bits 5..0 located at match spec bits 5..0.
     * Field B has field b its 8..0 located at:
     *   B[8] @ MS[8]
     *   B[7] @ MS[23]
     *   B[6] @ MS[22]
     *   B[5] @ MS[21]
     *   B[4] @ MS[20]
     *   B[3] @ MS[19]
     *   B[2] @ MS[18]
     *   B[1] @ MS[17]
     *   B[0] @ MS[16]
     */

    /* Find the byte offset of this field's LSB in the match spec, field bit
     * zero is bit zero of this byte. Next, find the byte which contains the
     * requested field bit.  Finally find the bit within the byte. */
    int ms_lsb = 0;
    for (int i = 0; i < slice_pos; ++i) {
      ms_lsb += PAD_TO_BYTE(position_to_size[i]) / 8;
    }
    ms_lsb += PAD_TO_BYTE(position_to_size[slice_pos]) / 8 - 1;
    int ms_byte = ms_lsb - field_start / 8;
    *ms_bit = ms_byte * 8 + (field_start % 8);
  }
  return err;

cleanup:
  return err;
}

/**
 * Given a table cJSON, parses all of its match stage tables. This does not
 * include ternary indirection or idletime, but it does include units for
 * tables who have them.
 *
 * @param table_cjson The cJSON structure corresponding to the table.
 * @param max_number_stage_tables The size of all_stage_tables_cjson array.
 * @param all_stage_tables_cjson An array of cJSON* objects, which will be
 * set to this table's match stage tables.
 * @param number_stage_tables A pointer which will be set to the number of
 * stage tables found.
 *
 * @return 0 on success, -1 on failure.
 */
int ctx_json_parse_all_match_stage_tables_for_table(
    cJSON *table_cjson,
    int max_number_stage_tables,
    cJSON **all_stage_tables_cjson,
    int *number_stage_tables) {
  int err = 0;

  cJSON *match_attributes_cjson = NULL;
  err = bf_cjson_get_object(table_cjson,
                            CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                            &match_attributes_cjson);
  CHECK_ERR(err, cleanup);

  /*
   * Fetch all stage tables and put them into one list. This is necessary to
   * handle the different types of exact match, which may have units.
   *
   * NOTE:
   * - ALPM tables don't have any stage tables;
   * - ATCAM tables take their stage tables from the units.
   */
  int index = 0;
  cJSON *units_cjson = NULL;
  bf_cjson_try_get_object(
      match_attributes_cjson, CTX_JSON_MATCH_ATTRIBUTES_UNITS, &units_cjson);

  if (units_cjson != NULL) {
    cJSON *unit_cjson = NULL;
    CTX_JSON_FOR_EACH(unit_cjson, units_cjson) {
      cJSON *unit_match_attributes_cjson = NULL;
      err = bf_cjson_get_object(unit_cjson,
                                CTX_JSON_MATCH_TABLE_MATCH_ATTRIBUTES,
                                &unit_match_attributes_cjson);
      CHECK_ERR(err, cleanup);

      cJSON *stage_tables_cjson = NULL;
      err = bf_cjson_get_object(unit_match_attributes_cjson,
                                CTX_JSON_TABLE_STAGE_TABLES,
                                &stage_tables_cjson);
      CHECK_ERR(err, cleanup);

      // Algorithmic units only have one stage table.
      cJSON *stage_table_cjson = NULL;
      CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
        if (index >= max_number_stage_tables) {
          LOG_ERROR("%s:%d: Table has more stage tables than expected.",
                    __FILE__,
                    __LINE__);
          goto cleanup;
        }
        all_stage_tables_cjson[index] = stage_table_cjson;
        ++index;
      }
    }
  }
  cJSON *stage_tables_cjson = NULL;
  err = bf_cjson_get_object(
      match_attributes_cjson, CTX_JSON_TABLE_STAGE_TABLES, &stage_tables_cjson);
  CHECK_ERR(err, cleanup);

  cJSON *stage_table_cjson = NULL;
  CTX_JSON_FOR_EACH(stage_table_cjson, stage_tables_cjson) {
    if (index >= max_number_stage_tables) {
      LOG_ERROR("%s:%d: Table has more stage tables than expected.",
                __FILE__,
                __LINE__);
      goto cleanup;
    }
    all_stage_tables_cjson[index] = stage_table_cjson;
    ++index;
  }

  *number_stage_tables = index;
  return 0;

cleanup:
  return -1;
}

/**
 * Finds an action in the actions cJSON array, given an action handle.
 *
 * @param actions_cjson The cJSON structure corresponding to the actions.
 * @param action_handle The action handle to look for.
 * @param ret A pointer to a cJSON * that will be set to the cJSON structure
 * corresponding to the action with the given handle.
 *
 * @return 0 if successful, -1 on error.
 */
int ctx_json_parse_action_for_action_handle(bf_dev_id_t devid,
                                            profile_id_t prof_id,
                                            cJSON *actions_cjson,
                                            int action_handle,
                                            cJSON **ret) {
  int err = 0;

  cJSON *action_cjson = NULL;
  CTX_JSON_FOR_EACH(action_cjson, actions_cjson) {
    int item_handle = 0;
    err = bf_cjson_get_handle(
        devid, prof_id, action_cjson, CTX_JSON_ACTION_HANDLE, &item_handle);
    CHECK_ERR(err, cleanup);

    if (item_handle == action_handle) {
      break;
    }
  }
  if (action_cjson == NULL) {
    LOG_ERROR("%s:%d: Could not find default action with handle 0x%x on table.",
              __FILE__,
              __LINE__,
              action_handle);
    err = -1;
    goto cleanup;
  }

  *ret = action_cjson;
  return err;

cleanup:
  return err;
}

/**
 * Parses one entry from a table's match_key_fields list.
 *
 * @return 0 if successful, -1 on error.
 */
int ctx_json_parse_match_key_field(cJSON *match_key_field,
                                   int *bit_width_full,
                                   int *match_type,
                                   int *start_bit,
                                   int *bit_width,
                                   int *position,
                                   char **name) {
  int err = 0;
  char *match_type_str;
  err = bf_cjson_get_string(
      match_key_field, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE, &match_type_str);
  CHECK_ERR(err, cleanup);
  err = bf_cjson_get_int(match_key_field,
                         CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH_FULL,
                         bit_width_full);
  CHECK_ERR(err, cleanup);
  err = bf_cjson_get_int(
      match_key_field, CTX_JSON_MATCH_KEY_FIELDS_START_BIT, start_bit);
  CHECK_ERR(err, cleanup);
  err = bf_cjson_get_int(
      match_key_field, CTX_JSON_MATCH_KEY_FIELDS_BIT_WIDTH, bit_width);
  CHECK_ERR(err, cleanup);
  err = bf_cjson_get_int(
      match_key_field, CTX_JSON_MATCH_KEY_FIELDS_POSITION, position);
  CHECK_ERR(err, cleanup);
  err = bf_cjson_get_string_dup(
      match_key_field, CTX_JSON_MATCH_KEY_FIELDS_NAME, name);
  CHECK_ERR(err, cleanup);

  if (!strcmp(match_type_str, CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_EXACT)) {
    *match_type = CTX_JSON_MATCH_TYPE_EXACT;
  } else if (!strcmp(match_type_str,
                     CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_LPM)) {
    *match_type = CTX_JSON_MATCH_TYPE_LPM;
  } else if (!strcmp(match_type_str,
                     CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_TERNARY)) {
    *match_type = CTX_JSON_MATCH_TYPE_TERNARY;
  } else if (!strcmp(match_type_str,
                     CTX_JSON_MATCH_KEY_FIELDS_MATCH_TYPE_RANGE)) {
    *match_type = CTX_JSON_MATCH_TYPE_RANGE;
  } else {
    LOG_ERROR(
        "%s:%d: Invalid ContextJSON format: match type \"%s\" is not "
        "valid.",
        __FILE__,
        __LINE__,
        match_type_str);
    err = -1;
    goto cleanup;
  }

cleanup:
  return err;
}

/**
 * Parses the action information for a given parameter name.
 *
 * @param action_cjson The cJSON structure corresponding to the action.
 * @param parameter_to_find The name of the parameter to look for.
 * @param bit_width_ret A pointer to an integer that will be set to the bit
 * width of the field.
 * @param start_bit_ret A pointer to an integer that will be set to the start
 * bit of the field.
 *
 * @return 0 if successful, -1 on error.
 */
int ctx_json_parse_action_parameter_for_parameter(cJSON *action_cjson,
                                                  char *parameter_to_find,
                                                  int *bit_width_ret) {
  int err = 0;

  cJSON *p4_parameters_cjson = NULL;
  err = bf_cjson_get_object(
      action_cjson, CTX_JSON_ACTION_P4_PARAMETERS, &p4_parameters_cjson);
  CHECK_ERR(err, cleanup);

  cJSON *parameter_cjson = NULL;
  CTX_JSON_FOR_EACH(parameter_cjson, p4_parameters_cjson) {
    char *item_name = NULL;

    err = bf_cjson_get_string(
        parameter_cjson, CTX_JSON_P4_PARAMETER_NAME, &item_name);
    CHECK_ERR(err, cleanup);

    if (!strcmp(item_name, parameter_to_find)) {
      break;
    }
  }
  if (parameter_cjson == NULL) {
    LOG_ERROR("%s:%d: Could not find field \"%s\" in the action's parameters.",
              __FILE__,
              __LINE__,
              parameter_to_find);
    err = -1;
    goto cleanup;
  }

  int bit_width = 0;
  err = bf_cjson_get_int(
      parameter_cjson, CTX_JSON_P4_PARAMETER_BIT_WIDTH, &bit_width);
  CHECK_ERR(err, cleanup);

  if (bit_width_ret != NULL) {
    *bit_width_ret = bit_width;
  }

  return err;

cleanup:
  return err;
}

/**
 * Parses a "global_name" from the cJSON object, if it is not found or is empty
 * it falls back to using "name".  Schema versions prior to 1.12.0 will not have
 * "global_name" in several places, but even post 1.12.0 "global_name" may be
 * empty for special fields like version/valid.
 *
 * @param cjson [IN] The cJSON structure to read from.
 * @param gbl_name [IN] The identifier for "global_name".
 * @param name [IN] The identifier for "name".
 * @param field_name [OUT] The string parsed from the cJSON is returned here.
 *
 * @return 0 on success, -1 on failure.
 */
int ctx_json_parse_global_name(cJSON *cjson,
                               char *gbl_name,
                               char *name,
                               char **field_name) {
  int rv = 0;
  *field_name = NULL;
  bf_cjson_try_get_string(cjson, gbl_name, field_name);
  if (!*field_name || **field_name == '\0')
    rv = bf_cjson_get_string(cjson, name, field_name);
  return rv;
}
