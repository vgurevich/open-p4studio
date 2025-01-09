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


#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

extern "C" {
#include <target-utils/third-party/cJSON/cJSON.h>
}

#include <memory>
#include <algorithm>
#include <string>

#include "s3/attribute.h"
#include "s3/meta/meta.h"
#include "bf_switch/bf_switch_types.h"

namespace smi {

bool parser_obj_type[UINT16_MAX] = {0};
bool parser_attr_ids[UINT16_MAX] = {0};

#define CHECK_RET(x, ret)                                                      \
  do {                                                                         \
    if (x) {                                                                   \
      fprintf(                                                                 \
          stderr, "\n%s:%d error %lu at (%s)\n", __func__, __LINE__, ret, #x); \
      return nullptr;                                                          \
    }                                                                          \
  } while (0)

#define CHECK_CLEAN(x, ret)                                                    \
  do {                                                                         \
    if (x) {                                                                   \
      fprintf(                                                                 \
          stderr, "\n%s:%d error %lu at (%s)\n", __func__, __LINE__, ret, #x); \
      goto end;                                                                \
    }                                                                          \
  } while (0)

static uint16_t current_parser_object_id = 0;
static uint16_t current_parser_attr_id = 0;

extern "C" {
char *read_file_to_buffer(const char *const file) {
  char *buffer = 0;
  size_t length = 0;
  FILE *f = fopen(file, "rb");
  size_t result = 0;

  if (f) {
    fseek(f, 0, SEEK_END);
    long int len = ftell(f);
    if (len == -1) {
      fprintf(stderr, "Failed to read JSON, invalid size");
      fclose(f);
      return NULL;
    }
    length = len;
    fseek(f, 0, SEEK_SET);
    buffer = reinterpret_cast<char *>(malloc(length + 1));
    if (buffer) {
      result = fread(buffer, 1, length, f);
      if (result != length) {
        fprintf(stderr, "Failed to read JSON");
      }
      buffer[length] = '\0';  // Null terminate
    }
    fclose(f);
  } else {
    return NULL;
  }

  return buffer;
}
}

static switch_status_t set_object_class_from_string(const char *const str,
                                                    ObjectInfo *object_info) {
  if (str == NULL) return SWITCH_STATUS_FAILURE;

  if (strcmp(str, "user") == 0) {
    object_info->set_object_class(OBJECT_CLASS_USER);
  } else if (strcmp(str, "auto") == 0) {
    object_info->set_object_class(OBJECT_CLASS_AUTO);
  } else {
    return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

static switch_status_t attribute_type_from_string(const char *const str,
                                                  switch_attr_type_t *type) {
  if (str == NULL) return SWITCH_STATUS_FAILURE;

  if (strcmp(str, "SWITCH_TYPE_BOOL") == 0) {
    *type = SWITCH_TYPE_BOOL;
  } else if (strcmp(str, "SWITCH_TYPE_UINT8") == 0) {
    *type = SWITCH_TYPE_UINT8;
  } else if (strcmp(str, "SWITCH_TYPE_UINT16") == 0) {
    *type = SWITCH_TYPE_UINT16;
  } else if (strcmp(str, "SWITCH_TYPE_UINT32") == 0) {
    *type = SWITCH_TYPE_UINT32;
  } else if (strcmp(str, "SWITCH_TYPE_UINT64") == 0) {
    *type = SWITCH_TYPE_UINT64;
  } else if (strcmp(str, "SWITCH_TYPE_INT64") == 0) {
    *type = SWITCH_TYPE_INT64;
  } else if (strcmp(str, "SWITCH_TYPE_MAC") == 0) {
    *type = SWITCH_TYPE_MAC;
  } else if (strcmp(str, "SWITCH_TYPE_STRING") == 0) {
    *type = SWITCH_TYPE_STRING;
  } else if (strcmp(str, "SWITCH_TYPE_IP_ADDRESS") == 0) {
    *type = SWITCH_TYPE_IP_ADDRESS;
  } else if (strcmp(str, "SWITCH_TYPE_IP_PREFIX") == 0) {
    *type = SWITCH_TYPE_IP_PREFIX;
  } else if (strcmp(str, "SWITCH_TYPE_OBJECT_ID") == 0) {
    *type = SWITCH_TYPE_OBJECT_ID;
  } else if (strcmp(str, "SWITCH_TYPE_RANGE") == 0) {
    *type = SWITCH_TYPE_RANGE;
  } else if (strcmp(str, "SWITCH_TYPE_LIST") == 0) {
    *type = SWITCH_TYPE_LIST;
  } else if (strcmp(str, "SWITCH_TYPE_ENUM") == 0) {
    *type = SWITCH_TYPE_ENUM;
  } else {
    return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

static switch_status_t attribute_default_val_from_type(
    const switch_attr_type_t attr_type,
    switch_attribute_value_t *const attr_val_out) {
  if (attr_val_out == NULL) return SWITCH_STATUS_FAILURE;

  switch_attribute_value_t val = {};
  val.type = attr_type;
  switch (attr_type) {
    case SWITCH_TYPE_BOOL:
    case SWITCH_TYPE_UINT8:
    case SWITCH_TYPE_UINT16:
    case SWITCH_TYPE_UINT32:
    case SWITCH_TYPE_UINT64:
    case SWITCH_TYPE_INT64:
    case SWITCH_TYPE_ENUM:
    case SWITCH_TYPE_MAC:
    case SWITCH_TYPE_STRING:
    case SWITCH_TYPE_IP_ADDRESS:
    case SWITCH_TYPE_IP_PREFIX:
    case SWITCH_TYPE_OBJECT_ID:
    case SWITCH_TYPE_RANGE:
    /* composite types */
    case SWITCH_TYPE_LIST:
      break;
    default:
      fprintf(stderr, "Unexpected type!");
      return SWITCH_STATUS_FAILURE;
  }
  *attr_val_out = val;
  return SWITCH_STATUS_SUCCESS;
}

switch_status_t attribute_default_val_get(
    const cJSON *const default_value_cjson,
    const AttributeMetadata &attr_md,
    switch_attribute_value_t *const default_value) {
  const switch_attr_type_t type = attr_md.type;
  // https://github.com/DaveGamble/cJSON/blob/master/cJSON.h#L118
  const uint8_t UINT8 = static_cast<uint8_t>(default_value_cjson->valuedouble);
  const uint16_t UINT16 =
      static_cast<uint16_t>(default_value_cjson->valuedouble);
  const uint32_t UINT32 =
      static_cast<uint32_t>(default_value_cjson->valuedouble);
  const uint64_t UINT64 =
      static_cast<uint64_t>(default_value_cjson->valuedouble);
  const int64_t INT64 = static_cast<int64_t>(default_value_cjson->valuedouble);
  switch_string_t STRING = {};
  const int cjson_type = default_value_cjson->type;
  switch (type) {
    case SWITCH_TYPE_BOOL:
      assert(cjson_type == cJSON_True || cjson_type == cJSON_False);
      if (cjson_type == cJSON_True) {
        v_bool_set(default_value, 1);
      } else if (cjson_type == cJSON_False) {
        v_bool_set(default_value, 0);
      }
      break;
    case SWITCH_TYPE_UINT8:
      assert(cjson_type == cJSON_Number);
      v_u8_set(default_value, UINT8);
      break;
    case SWITCH_TYPE_UINT16:
      assert(cjson_type == cJSON_Number);
      v_u16_set(default_value, UINT16);
      break;
    case SWITCH_TYPE_UINT32:
      assert(cjson_type == cJSON_Number);
      v_u32_set(default_value, UINT32);
      break;

    case SWITCH_TYPE_UINT64:
      assert(cjson_type == cJSON_Number);
      v_u64_set(default_value, UINT64);
      break;

    case SWITCH_TYPE_INT64:
      assert(cjson_type == cJSON_Number);
      v_s64_set(default_value, INT64);
      break;
    case SWITCH_TYPE_ENUM: {
      const ValueMetadata *value_md = attr_md.get_value_metadata();
      switch_enum_t val = {};
      assert(cjson_type == cJSON_String);
      for (const auto &enums : value_md->get_enum_metadata()) {
        if (!strcmp(default_value_cjson->valuestring,
                    enums.enum_name.c_str())) {
          val.enumdata = enums.enum_value;
          break;
        }
      }
      v_enum_set(default_value, val);
    } break;

    case SWITCH_TYPE_STRING:
      assert(cjson_type == cJSON_String);
      strncpy(STRING.text,
              default_value_cjson->valuestring,
              SWITCH_MAX_STRING_LEN - 1),
          STRING.text[SWITCH_MAX_STRING_LEN - 1] = '\0';
      v_string_set(default_value, STRING);
      break;

    default:
      assert(0 && "Implement parsing default values for this type!");
      return SWITCH_STATUS_FAILURE;
  }
  return SWITCH_STATUS_SUCCESS;
}

uint16_t resolve_collision(uint16_t hash, bool *ids) {
  if (ids[hash]) {
    while (ids[hash]) hash++;
  }
  ids[hash] = 1;
  return hash;
}

uint16_t allocate_obj_type(const char *const name) {
  (void)name;
  return current_parser_object_id++;
}

uint16_t allocate_attr_ids(const char *const name) {
  (void)name;
  return ++current_parser_attr_id;
}

switch_status_t model_info_check_id_collision(ModelInfo *model_info) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  bool ids[UINT16_MAX] = {false};
  for (auto it = model_info->begin(); it != model_info->end(); it++) {
    auto obj_info = *it;
    if (ids[obj_info.object_type]) {
      fprintf(stderr,
              "Object id:%" PRIu16 " name:%s\n",
              obj_info.object_type,
              obj_info.get_object_name().c_str());
      return SWITCH_STATUS_FAILURE;
    }

    ids[obj_info.object_type] = 1;
  }
  return status;
}

std::string get_object_fqn(std::string idl_prefix, std::string object_name) {
  std::string fqn = idl_prefix + "OBJECT_TYPE_" + object_name;
  std::transform(fqn.begin(), fqn.end(), fqn.begin(), ::toupper);
  return fqn;
}
std::string get_attr_fqn(std::string idl_prefix,
                         std::string object_name,
                         std::string attr_name) {
  std::string fqn = idl_prefix + object_name + "_ATTR_" + attr_name;
  std::transform(fqn.begin(), fqn.end(), fqn.begin(), ::toupper);
  return fqn;
}

std::string get_cntr_fqn(std::string idl_prefix,
                         std::string object_name,
                         std::string attr_name) {
  std::string fqn = idl_prefix + object_name + "_COUNTER_" + attr_name;
  std::transform(fqn.begin(), fqn.end(), fqn.begin(), ::toupper);
  return fqn;
}

std::string get_cntr_enum_fqn(std::string idl_prefix,
                              const std::string object_name,
                              const std::string attr_name,
                              const std::string enum_name) {
  std::string fqn =
      idl_prefix + object_name + "_COUNTER_" + attr_name + "_" + enum_name;
  std::transform(fqn.begin(), fqn.end(), fqn.begin(), ::toupper);
  return fqn;
}

std::string get_enum_fqn(std::string idl_prefix,
                         const std::string object_name,
                         const std::string attr_name,
                         const std::string val_name) {
  std::string fqn =
      idl_prefix + object_name + "_ATTR_" + attr_name + "_" + val_name;
  std::transform(fqn.begin(), fqn.end(), fqn.begin(), ::toupper);
  return fqn;
}

std::unique_ptr<ModelInfo> build_model_info_from_string(const char *config,
                                                        bool log_objects) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::unique_ptr<ModelInfo> model_info = nullptr;
  cJSON *object_cjson = NULL;
  cJSON *root_cjson = NULL;
  cJSON *version_cjson = NULL;
  cJSON *objects_cjson = NULL;
  cJSON *idl_prefix_cjson = NULL;
  char none_obj_name[] = "none";
  char internal_object[] = "internal_object";

  CHECK_RET(config == NULL, SWITCH_STATUS_FAILURE);

  root_cjson = cJSON_Parse(config);
  CHECK_RET(root_cjson == NULL, SWITCH_STATUS_FAILURE);

  model_info = std::unique_ptr<ModelInfo>(new ModelInfo);
  CHECK_CLEAN(model_info == nullptr, SWITCH_STATUS_FAILURE);

  version_cjson = cJSON_GetObjectItem(root_cjson, "version");
  CHECK_CLEAN(version_cjson == NULL, SWITCH_STATUS_FAILURE);
  model_info->set_version(version_cjson->valueint);
  model_info->object_name_max_len = 0;
  model_info->attr_name_max_len = 0;

  objects_cjson = cJSON_GetObjectItem(root_cjson, "objects");
  CHECK_CLEAN(objects_cjson == NULL, SWITCH_STATUS_FAILURE);

  idl_prefix_cjson = cJSON_GetObjectItem(root_cjson, "idlprefix");
  CHECK_CLEAN(idl_prefix_cjson == NULL, SWITCH_STATUS_FAILURE);

  {
    // Add none object type
    ObjectInfo none_object_info = {};
    none_object_info.set_object_class(OBJECT_CLASS_NONE);
    none_object_info.set_object_name(none_obj_name);
    none_object_info.set_counter(false);
    none_object_info.set_counter_attr_id(0);
    none_object_info.set_object_name_fqn(
        get_object_fqn(idl_prefix_cjson->valuestring, none_obj_name));
    none_object_info.object_type =
        allocate_obj_type(none_object_info.get_object_name_fqn().c_str());
    model_info->add_id_name_mapping(
        none_object_info.get_object_name_fqn(),
        static_cast<uint64_t>(none_object_info.object_type));
    model_info->add_object_info_entry(none_object_info);
  }

  /* first pass*/
  /* For each object, parse all attributes and counters if any */
  cJSON_ArrayForEach(object_cjson, objects_cjson) {
    ObjectInfo object_info = {};
    cJSON *obj_class_cjson = cJSON_GetObjectItem(object_cjson, "class");
    CHECK_CLEAN(obj_class_cjson == NULL, SWITCH_STATUS_FAILURE);

    status = set_object_class_from_string(obj_class_cjson->valuestring,
                                          &object_info);
    CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);

    object_info.set_object_name(object_cjson->string);
    object_info.set_object_name_fqn(
        get_object_fqn(idl_prefix_cjson->valuestring, object_cjson->string));
    object_info.object_type =
        allocate_obj_type(object_info.get_object_name_fqn().c_str());
    model_info->add_id_name_mapping(
        object_info.get_object_name_fqn(),
        static_cast<uint64_t>(object_info.object_type));

    size_t len = (object_info.get_object_name()).size();
    if (len > model_info->object_name_max_len)
      model_info->object_name_max_len = len;

    object_info.auto_obj_prio = 0;
    cJSON *obj_prio_cjson = cJSON_GetObjectItem(object_cjson, "priority");
    if (obj_prio_cjson) object_info.auto_obj_prio = obj_prio_cjson->valueint;

    if (object_info.get_object_class() == OBJECT_CLASS_USER) {
      cJSON *obj_desc_cjson = cJSON_GetObjectItem(object_cjson, "description");
      CHECK_CLEAN(obj_desc_cjson == NULL, SWITCH_STATUS_FAILURE);
      object_info.set_object_desc(obj_desc_cjson->valuestring);
    }

    cJSON *attrs_cjson = cJSON_GetObjectItem(object_cjson, "attributes");
    CHECK_CLEAN(attrs_cjson == NULL, SWITCH_STATUS_FAILURE);
    size_t num_attr = cJSON_GetArraySize(attrs_cjson);

    cJSON *cntrs_cjson = cJSON_GetObjectItem(object_cjson, "counter");
    if (cntrs_cjson) {
      num_attr += cJSON_GetArraySize(cntrs_cjson);
    }
    // object_info->attr_count = num_attr;
    if (log_objects) {
      fprintf(stdout,
              "Object id:%" PRIu16 " name:%s\n",
              object_info.object_type,
              object_cjson->string);
    }

    cJSON *attr_cjson;
    // Before Adding User defined attributes for an Object, we inject internal
    // attribute internal_object. The default
    // value for this attribute is false. This needs to be set true internally
    // when an object is created internally
    // without user initiation.
    {
      AttributeMetadata attr_md = {};
      attr_md.set_parent_object_type(object_info.object_type);
      attr_md.set_attr_name(internal_object);
      attr_md.set_attr_name_fqn(get_attr_fqn(idl_prefix_cjson->valuestring,
                                             object_info.get_object_name(),
                                             internal_object));
      attr_md.attr_id = allocate_attr_ids(attr_md.get_attr_name_fqn().c_str());
      model_info->add_id_name_mapping(attr_md.get_attr_name_fqn(),
                                      static_cast<uint64_t>(attr_md.attr_id));
      if (log_objects) {
        fprintf(stdout,
                "Attr id:%" PRIu16 " name:%s\n",
                attr_md.attr_id,
                internal_object);
      }
      attr_md.type = SWITCH_TYPE_BOOL;
      switch_attribute_value_t default_value = {};
      default_value.type = attr_md.type;
      default_value.booldata = false;
      ValueMetadata *value_md = attr_md.get_value_metadata_for_update();
      value_md->set_default_value(default_value);
      switch_attr_flags_t flags = {};
      flags.is_internal = true;
      flags.is_immutable = true;
      attr_md.set_flags(flags);
      model_info->update_attr_to_ot_map(attr_md.attr_id,
                                        object_info.object_type);
      object_info.add_to_attribute_metadata_list(attr_md);
    }

    // Parse attributes
    cJSON_ArrayForEach(attr_cjson, attrs_cjson) {
      AttributeMetadata attr_md = {};
      attr_md.set_parent_object_type(object_info.object_type);
      attr_md.set_attr_name(attr_cjson->string);
      attr_md.set_attr_name_fqn(get_attr_fqn(idl_prefix_cjson->valuestring,
                                             object_info.get_object_name(),
                                             attr_cjson->string));
      attr_md.attr_id = allocate_attr_ids(attr_md.get_attr_name_fqn().c_str());
      model_info->add_id_name_mapping(attr_md.get_attr_name_fqn(),
                                      static_cast<uint64_t>(attr_md.attr_id));

      if (log_objects) {
        fprintf(stdout,
                "Attr id:%" PRIu16 " name:%s\n",
                attr_md.attr_id,
                attr_cjson->string);
      }

      len = (attr_md.get_attr_name()).size();
      if (len > model_info->attr_name_max_len)
        model_info->attr_name_max_len = len;

      cJSON *type_info_cjson = cJSON_GetObjectItem(attr_cjson, "type_info");
      CHECK_CLEAN(type_info_cjson == NULL, SWITCH_STATUS_FAILURE);

      cJSON *type_cjson = cJSON_GetObjectItem(type_info_cjson, "type");
      CHECK_CLEAN(type_cjson == NULL, SWITCH_STATUS_FAILURE);

      switch_attr_type_t type = SWITCH_TYPE_NONE;
      status = attribute_type_from_string(type_cjson->valuestring, &type);
      CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
      attr_md.type = type;

      ValueMetadata *value_md = attr_md.get_value_metadata_for_update();
      value_md->type = type;
      if (attr_md.type == SWITCH_TYPE_LIST) {
        cJSON *list_cjson = cJSON_GetObjectItem(type_info_cjson, "list");
        CHECK_CLEAN(list_cjson == NULL, SWITCH_STATUS_FAILURE);

        cJSON *list_type_cjson = cJSON_GetObjectItem(list_cjson, "type");
        CHECK_CLEAN(list_type_cjson == NULL, SWITCH_STATUS_FAILURE);

        type = SWITCH_TYPE_NONE;
        status =
            attribute_type_from_string(list_type_cjson->valuestring, &type);
        CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
        value_md->type = type;

        if (type == SWITCH_TYPE_ENUM) {
          cJSON *enum_names = cJSON_GetObjectItem(list_cjson, "enum");
          CHECK_CLEAN(enum_names == NULL, SWITCH_STATUS_FAILURE);

          cJSON *enum_name = NULL;
          uint32_t enum_val = 0;
          cJSON_ArrayForEach(enum_name, enum_names) {
            EnumMetadata em = {};
            em.enum_value = enum_val;
            em.enum_name = enum_name->valuestring;
            em.enum_name_fqn =
                get_enum_fqn(idl_prefix_cjson->valuestring,
                             object_info.get_object_name().c_str(),
                             attr_md.get_attr_name().c_str(),
                             enum_name->valuestring);
            enum_val++;  // assigning sequential enum values to names
            value_md->add_enum_metadata(em);
          }
        }

      } else if (is_composite_type(attr_md.type)) {
        // TODO(all): "ther composite type"
        assert(0 && "schema parsing for this type not implemented!");
      } else if (attr_md.type == SWITCH_TYPE_ENUM) {
        /* enum values and names */
        cJSON *enum_names = cJSON_GetObjectItem(type_info_cjson, "enum");
        CHECK_CLEAN(enum_names == NULL, SWITCH_STATUS_FAILURE);

        cJSON *enum_name = NULL;
        uint32_t enum_val = 0;
        cJSON_ArrayForEach(enum_name, enum_names) {
          EnumMetadata em = {};
          em.enum_value = enum_val;
          em.enum_name = enum_name->valuestring;
          em.enum_name_fqn = get_enum_fqn(idl_prefix_cjson->valuestring,
                                          object_info.get_object_name().c_str(),
                                          attr_md.get_attr_name().c_str(),
                                          enum_name->valuestring);
          enum_val++;  // assigning sequential enum values to names
          value_md->add_enum_metadata(em);
        }
      }

      // flags
      switch_attr_flags_t flags = {};
      cJSON *const is_mandatory_cjson =
          cJSON_GetObjectItem(attr_cjson, "is_mandatory");
      if (is_mandatory_cjson != NULL) {
        flags.is_mandatory = (is_mandatory_cjson->type == cJSON_True);
      }
      cJSON *const is_immutable_cjson =
          cJSON_GetObjectItem(attr_cjson, "is_immutable");
      if (is_immutable_cjson != NULL) {
        flags.is_immutable = (is_immutable_cjson->type == cJSON_True);
      }
      cJSON *const is_read_only_cjson =
          cJSON_GetObjectItem(attr_cjson, "is_read_only");
      if (is_read_only_cjson != NULL) {
        flags.is_read_only = (is_read_only_cjson->type == cJSON_True);
      }
      cJSON *const is_internal_cjson =
          cJSON_GetObjectItem(attr_cjson, "is_internal");
      if (is_internal_cjson != NULL) {
        flags.is_internal = (is_internal_cjson->type == cJSON_True);
      }
      cJSON *const is_create_only_cjson =
          cJSON_GetObjectItem(attr_cjson, "is_create_only");
      if (is_create_only_cjson != NULL) {
        flags.is_create_only = (is_create_only_cjson->type == cJSON_True);
      }
      cJSON *const re_evaluate_cjson =
          cJSON_GetObjectItem(attr_cjson, "re_evaluate");
      if (re_evaluate_cjson != NULL) {
        flags.re_evaluate = (re_evaluate_cjson->type == cJSON_True);
      }
      if ((flags.is_internal || flags.is_read_only || flags.is_immutable) &&
          (flags.is_mandatory || flags.is_create_only)) {
        fprintf(stderr,
                "Invalid flags. Attr id:%" PRIu16 " name:%s\n",
                attr_md.attr_id,
                attr_cjson->string);
        goto end;
      }
      if (strcmp(attr_cjson->string, "status") == 0 &&
          object_info.get_object_class() == OBJECT_CLASS_AUTO) {
        flags.is_status = true;
      }
      // Object re-evaluation of referred object is permitted only from
      // referring User Objects
      if (flags.re_evaluate &&
          (object_info.get_object_class() != OBJECT_CLASS_USER)) {
        fprintf(stderr,
                "Invalid flag %s for object class %s Attr id:%" PRIu16 "\n",
                attr_cjson->string,
                object_cjson->string,
                attr_md.attr_id);
        goto end;
      }
      attr_md.set_flags(flags);

      // set default value
      switch_attribute_value_t default_value = {};
      status = attribute_default_val_from_type(attr_md.type, &default_value);
      CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);

      cJSON *default_value_cjson =
          cJSON_GetObjectItem(type_info_cjson, "default_value");
      if (default_value_cjson != NULL) {
        status = attribute_default_val_get(
            default_value_cjson, attr_md, &default_value);
        CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
      }
      value_md->set_default_value(default_value);

      if (object_info.get_object_class() == OBJECT_CLASS_USER &&
          flags.is_internal == false) {
        cJSON *attr_desc_cjson = cJSON_GetObjectItem(attr_cjson, "description");
        CHECK_CLEAN(attr_desc_cjson == NULL, SWITCH_STATUS_FAILURE);
        attr_md.set_description(attr_desc_cjson->valuestring);
      }

      // mark all stat-objects that requires store-caching as part of Warm-Init
      if (strstr(attr_cjson->string, "stats_cache") != NULL &&
          object_info.get_object_class() == OBJECT_CLASS_AUTO) {
        object_info.set_stats_cache(true);
      } else {
        object_info.set_stats_cache(false);
      }

      // Finally add to the object_info
      model_info->update_attr_to_ot_map(attr_md.attr_id,
                                        object_info.object_type);
      object_info.add_to_attribute_metadata_list(attr_md);
    }

    /* check for counter field */
    if (cntrs_cjson) {
      object_info.set_counter(true);
    } else {
      object_info.set_counter(false);
      model_info->add_object_info_entry(object_info);
      continue;
    }

    /* Parse counters */
    cJSON *cntr_cjson;
    cJSON_ArrayForEach(cntr_cjson, cntrs_cjson) {
      AttributeMetadata attr_md = {};
      attr_md.set_parent_object_type(object_info.object_type);
      attr_md.set_attr_name(cntr_cjson->string);
      attr_md.set_attr_name_fqn(
          get_cntr_fqn(idl_prefix_cjson->valuestring,
                       object_info.get_object_name().c_str(),
                       cntr_cjson->string));
      attr_md.attr_id = allocate_attr_ids(attr_md.get_attr_name_fqn().c_str());
      model_info->add_id_name_mapping(attr_md.get_attr_name_fqn(),
                                      static_cast<uint64_t>(attr_md.attr_id));

      if (log_objects) {
        fprintf(stdout,
                "Cntr id:%" PRIu16 " name:%s\n",
                attr_md.attr_id,
                cntr_cjson->string);
      }

      cJSON *type_info_cjson = cJSON_GetObjectItem(cntr_cjson, "type_info");
      CHECK_CLEAN(type_info_cjson == NULL, SWITCH_STATUS_FAILURE);

      cJSON *type_cjson = cJSON_GetObjectItem(type_info_cjson, "type");
      CHECK_CLEAN(type_cjson == NULL, SWITCH_STATUS_FAILURE);

      switch_attr_type_t type = SWITCH_TYPE_NONE;
      status = attribute_type_from_string(type_cjson->valuestring, &type);
      CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
      attr_md.type = type;

      ValueMetadata *value_md = attr_md.get_value_metadata_for_update();
      value_md->type = type;
      if (attr_md.type == SWITCH_TYPE_ENUM) {
        /* enum values and names */
        cJSON *enum_names = cJSON_GetObjectItem(type_info_cjson, "enum");
        CHECK_CLEAN(enum_names == NULL, SWITCH_STATUS_FAILURE);

        cJSON *enum_name = NULL;
        uint32_t enum_val = 0;
        cJSON_ArrayForEach(enum_name, enum_names) {
          EnumMetadata em = {};
          em.enum_value = enum_val;
          em.enum_name = enum_name->valuestring;
          em.enum_name_fqn =
              get_cntr_enum_fqn(idl_prefix_cjson->valuestring,
                                object_info.get_object_name().c_str(),
                                attr_md.get_attr_name().c_str(),
                                em.enum_name.c_str());
          enum_val++;  // assigning sequential enum values to names
          value_md->add_enum_metadata(em);
        }
        object_info.set_counter_attr_id(attr_md.attr_id);
      }

      switch_attr_flags_t flags = {};
      flags.is_counter = true;
      attr_md.set_flags(flags);

      switch_attribute_value_t default_value = {};
      status = attribute_default_val_from_type(attr_md.type, &default_value);
      CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);

      cJSON *default_value_cjson =
          cJSON_GetObjectItem(type_info_cjson, "default_value");
      if (default_value_cjson != NULL) {
        status = attribute_default_val_get(
            default_value_cjson, attr_md, &default_value);
        CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);
      }
      value_md->set_default_value(default_value);

      // Finally add to the object_info
      model_info->update_attr_to_ot_map(attr_md.attr_id,
                                        object_info.object_type);
      object_info.add_to_attribute_metadata_list(attr_md);
    }
    model_info->add_object_info_entry(object_info);
  }

  /*
   * Check for ID collisions before using them
   */
  status = model_info_check_id_collision(model_info.get());
  CHECK_CLEAN(status != SWITCH_STATUS_SUCCESS, SWITCH_STATUS_FAILURE);

  /* second pass,
   * where we need to reference other objects_cjson or attributes. Here we use
   * the IDs
   * generated in the first pass */

  cJSON_ArrayForEach(object_cjson, objects_cjson) {
    ObjectInfo *object_info =
        model_info->get_object_info_from_name_for_update(object_cjson->string);
    CHECK_CLEAN(object_info == nullptr, SWITCH_STATUS_FAILURE);

    // Get dependency list
    cJSON *dependencies_cjson =
        cJSON_GetObjectItem(object_cjson, "dependencies");
    cJSON *dep_cjson = NULL;
    if (dependencies_cjson != NULL) {
      cJSON_ArrayForEach(dep_cjson, dependencies_cjson) {
        switch_attr_id_t dep_attr_id = 0;
        cJSON *dep_object_cjson = cJSON_GetObjectItem(dep_cjson, "object");
        cJSON *dep_attr_cjson = cJSON_GetObjectItem(dep_cjson, "attribute");

        CHECK_CLEAN(dep_object_cjson == NULL, SWITCH_STATUS_FAILURE);
        CHECK_CLEAN(dep_attr_cjson == NULL, SWITCH_STATUS_FAILURE);

        ObjectAttrPair dep_pair = {};
        const ObjectInfo *dep_object_info =
            model_info->get_object_info_from_name(
                dep_object_cjson->valuestring);
        CHECK_CLEAN(dep_object_info == nullptr, SWITCH_STATUS_FAILURE);

        dep_attr_id =
            dep_object_info->get_attr_id_from_name(dep_attr_cjson->valuestring);
        if (dep_attr_id == 0) {
          fprintf(stderr,
                  "dep_attr_id not found .. ot=%s dep attr=%s.%s",
                  object_cjson->string,
                  dep_object_cjson->valuestring,
                  dep_attr_cjson->valuestring);
        }
        CHECK_CLEAN(dep_attr_id == 0, SWITCH_STATUS_FAILURE);

        if (log_objects) {
          fprintf(stdout,
                  "Deps ot:%s id:%" PRIu16 " obj:%s attr:%s\n",
                  object_cjson->string,
                  dep_attr_id,
                  dep_object_cjson->valuestring,
                  dep_attr_cjson->valuestring);
        }
        dep_pair.object_type = dep_object_info->object_type;
        dep_pair.attr_id = dep_attr_id;

        object_info->add_to_dependency_list(dep_pair);
      }
    }

    // Get membership list
    cJSON *membership_cjson = cJSON_GetObjectItem(object_cjson, "membership");
    cJSON *mem_cjson = NULL;
    if (membership_cjson != NULL) {
      cJSON_ArrayForEach(mem_cjson, membership_cjson) {
        switch_attr_id_t mem_attr_id = 0;
        cJSON *mem_object_cjson = cJSON_GetObjectItem(mem_cjson, "object");
        cJSON *mem_attr_cjson = cJSON_GetObjectItem(mem_cjson, "attribute");

        CHECK_CLEAN(mem_object_cjson == NULL, SWITCH_STATUS_FAILURE);
        CHECK_CLEAN(mem_attr_cjson == NULL, SWITCH_STATUS_FAILURE);

        ObjectAttrPair mem_pair = {};
        const ObjectInfo *mem_object_info =
            model_info->get_object_info_from_name(
                mem_object_cjson->valuestring);
        CHECK_CLEAN(mem_object_info == nullptr, SWITCH_STATUS_FAILURE);

        mem_attr_id =
            mem_object_info->get_attr_id_from_name(mem_attr_cjson->valuestring);
        if (mem_attr_id == 0) {
          fprintf(stderr,
                  "ot=%s: membership attr_id not found mem_ot=%s,  attr=%s",
                  object_cjson->string,
                  mem_object_cjson->valuestring,
                  mem_attr_cjson->valuestring);
        }
        CHECK_CLEAN(mem_attr_id == 0, SWITCH_STATUS_FAILURE);

        auto mem_attr_md = mem_object_info->get_attr_metadata(mem_attr_id);
        CHECK_CLEAN(mem_attr_md == NULL, SWITCH_STATUS_FAILURE);
        auto const mem_attr_flags = mem_attr_md->get_flags();
        if (!mem_attr_flags.is_internal && !mem_attr_flags.is_read_only) {
          fprintf(stderr,
                  "Invalid flags for Object id:%" PRIu16
                  " name:%s,  Attr id:%" PRIu16
                  " name:%s, Attribute of Group Object containing members must "
                  "be read_only and/or internal\n",
                  mem_object_info->object_type,
                  (mem_object_info->get_object_name()).c_str(),
                  mem_attr_id,
                  (mem_attr_md->get_attr_name()).c_str());
          goto end;
        }

        if (log_objects) {
          fprintf(stdout,
                  "Mems ot:%s id:%" PRIu16 " obj:%s attr:%s\n",
                  object_cjson->string,
                  mem_attr_id,
                  mem_object_cjson->valuestring,
                  mem_attr_cjson->valuestring);
        }
        mem_pair.object_type = mem_object_info->object_type;
        mem_pair.attr_id = mem_attr_id;

        object_info->add_to_membership_list(mem_pair);
      }
    }

    // Get key groups
    cJSON *kgs_cjson = cJSON_GetObjectItem(object_cjson, "key_groups");
    cJSON *kg_cjson = NULL;
    cJSON *kg_element_cjson = NULL;
    if (kgs_cjson != NULL) {
      cJSON_ArrayForEach(kg_cjson, kgs_cjson) {
        KeyGroup kg = {};

        cJSON_ArrayForEach(kg_element_cjson, kg_cjson) {
          const ObjectInfo *temp_info =
              model_info->get_object_info(object_info->object_type);
          switch_attr_id_t kg_element_attr_id =
              temp_info->get_attr_id_from_name(kg_element_cjson->valuestring);
          CHECK_CLEAN(kg_element_attr_id == 0, SWITCH_STATUS_FAILURE);

          kg.attr_list.push_back(kg_element_attr_id);
        }

        object_info->add_to_key_groups(kg);
      }
    }

    cJSON *attrs_cjson = cJSON_GetObjectItem(object_cjson, "attributes");
    CHECK_CLEAN(attrs_cjson == NULL, SWITCH_STATUS_FAILURE);

    cJSON *attr_cjson;
    cJSON_ArrayForEach(attr_cjson, attrs_cjson) {
      AttributeMetadata *attr_md =
          object_info->get_attr_metadata_by_name(attr_cjson->string);
      CHECK_CLEAN(attr_md == nullptr, SWITCH_STATUS_FAILURE);
      ValueMetadata *value_md = attr_md->get_value_metadata_for_update();

      cJSON *type_info_cjson = cJSON_GetObjectItem(attr_cjson, "type_info");
      if (attr_md->type == SWITCH_TYPE_OBJECT_ID) {
        cJSON *allowed_object_type_cjson =
            cJSON_GetObjectItem(type_info_cjson, "allowed_object_types");
        CHECK_CLEAN(allowed_object_type_cjson == NULL, SWITCH_STATUS_FAILURE);
        cJSON *allowed_type_cjson = NULL;
        cJSON_ArrayForEach(allowed_type_cjson, allowed_object_type_cjson) {
          const ObjectInfo *allowed_object_info =
              model_info->get_object_info_from_name(
                  allowed_type_cjson->valuestring);
          if (allowed_object_info == nullptr) {
            fprintf(
                stderr,
                "allowed_object_type not found .. ot=%s allowed obj type=%s",
                object_cjson->string,
                allowed_type_cjson->valuestring);
          }
          CHECK_CLEAN(allowed_object_info == nullptr, SWITCH_STATUS_FAILURE);
          value_md->type = SWITCH_TYPE_OBJECT_ID;
          value_md->add_allowed_object_types(allowed_object_info->object_type);
        }
      } else if (attr_md->type == SWITCH_TYPE_LIST) {
        if (value_md->type == SWITCH_TYPE_OBJECT_ID) {
          cJSON *type_info_list_cjson =
              cJSON_GetObjectItem(type_info_cjson, "list");
          CHECK_CLEAN(type_info_list_cjson == NULL, SWITCH_STATUS_FAILURE);
          cJSON *allowed_object_type_cjson =
              cJSON_GetObjectItem(type_info_list_cjson, "allowed_object_types");
          CHECK_CLEAN(allowed_object_type_cjson == NULL, SWITCH_STATUS_FAILURE);
          cJSON *allowed_type_cjson = NULL;
          cJSON_ArrayForEach(allowed_type_cjson, allowed_object_type_cjson) {
            const ObjectInfo *allowed_object_info =
                model_info->get_object_info_from_name(
                    allowed_type_cjson->valuestring);
            if (allowed_object_info == nullptr) {
              fprintf(
                  stderr,
                  "allowed_object_type not found .. ot=%s allowed obj type=%s",
                  object_cjson->string,
                  allowed_type_cjson->valuestring);
            }
            CHECK_CLEAN(allowed_object_info == nullptr, SWITCH_STATUS_FAILURE);
            value_md->add_allowed_object_types(
                allowed_object_info->object_type);
          }
        }

      } else if (is_composite_type(attr_md->type)) {
        fprintf(stderr, "unknown type");
        goto end;
      }
    }

    // parse object's cli-info
    cJSON *cli_info_cjson = cJSON_GetObjectItem(object_cjson, "cli_info");
    CliMetadata *cli_md = object_info->get_cli_metadata_mutable();
    if (cli_info_cjson != NULL) {
      // parse "format"
      cJSON *format_cjson = cJSON_GetObjectItem(cli_info_cjson, "format");
      if (format_cjson != NULL) {
        // parse key_attrs
        cJSON *key_attrs_cjson = cJSON_GetObjectItem(format_cjson, "key_attrs");
        if (key_attrs_cjson) {
          cJSON *key_cjson;
          cJSON_ArrayForEach(key_cjson, key_attrs_cjson) {
            ObjectAttrPair obj_attr_pair = {};
            cJSON *key_object_cjson = cJSON_GetObjectItem(key_cjson, "object");
            CHECK_RET(key_object_cjson == NULL, SWITCH_STATUS_FAILURE);
            cJSON *key_attr_cjson = cJSON_GetObjectItem(key_cjson, "attribute");
            CHECK_RET(key_attr_cjson == NULL, SWITCH_STATUS_FAILURE);

            obj_attr_pair.object_type = object_info->object_type;
            // key_object_type = model_utils_get_object_type_from_name(
            //     model_info, key_object_cjson->valuestring);
            // CHECK_RET(key_object_type == 0, SWITCH_STATUS_FAILURE);

            obj_attr_pair.attr_id =
                object_info->get_attr_id_from_name(key_attr_cjson->valuestring);
            if (obj_attr_pair.attr_id == 0) {
              fprintf(stderr,
                      "cli key_attr_id not found .. ot=%s key attr=%s",
                      key_object_cjson->valuestring,
                      key_attr_cjson->valuestring);
            }
            CHECK_RET(obj_attr_pair.attr_id == 0, SWITCH_STATUS_FAILURE);
            cli_md->add_to_key_attrs(obj_attr_pair);
          }
        }
      }

      cJSON *table_view_attrs_cjson =
          cJSON_GetObjectItem(cli_info_cjson, "table_view_attrs");
      if (table_view_attrs_cjson) {
        cJSON *table_attr_cjson;
        cJSON_ArrayForEach(table_attr_cjson, table_view_attrs_cjson) {
          switch_attr_id_t attr_id =
              object_info->get_attr_id_from_name(table_attr_cjson->valuestring);
          if (attr_id == 0) {
            fprintf(stderr,
                    "table view cli attr_id not found .. key attr=%s",
                    table_attr_cjson->valuestring);
          }
          CHECK_RET(attr_id == 0, SWITCH_STATUS_FAILURE);
          cli_md->add_to_table_view_attr_list(attr_id);
        }
      }
    }

    if (!object_info->get_counter()) continue;

    cJSON *cntrs_cjson = cJSON_GetObjectItem(object_cjson, "counter");
    CHECK_CLEAN(cntrs_cjson == NULL, SWITCH_STATUS_FAILURE);
    cJSON *cntr_cjson;
    cJSON_ArrayForEach(cntr_cjson, cntrs_cjson) {
      AttributeMetadata *attr_md =
          object_info->get_attr_metadata_by_name(cntr_cjson->string);
      CHECK_CLEAN(attr_md == nullptr, SWITCH_STATUS_FAILURE);
      ValueMetadata *value_md = attr_md->get_value_metadata_for_update();

      cJSON *type_info_cjson = cJSON_GetObjectItem(cntr_cjson, "type_info");

      if (attr_md->type == SWITCH_TYPE_OBJECT_ID) {
        cJSON *allowed_object_type_cjson =
            cJSON_GetObjectItem(type_info_cjson, "allowed_object_types");
        CHECK_CLEAN(allowed_object_type_cjson == NULL, SWITCH_STATUS_FAILURE);
        cJSON *allowed_type_cjson = NULL;
        cJSON_ArrayForEach(allowed_type_cjson, allowed_object_type_cjson) {
          const ObjectInfo *allowed_object_info =
              model_info->get_object_info_from_name(
                  allowed_type_cjson->valuestring);
          if (allowed_object_info == nullptr) {
            fprintf(
                stderr,
                "allowed_object_type not found .. ot=%s allowed obj type=%s",
                object_cjson->string,
                allowed_type_cjson->valuestring);
          }
          CHECK_CLEAN(allowed_object_info == nullptr, SWITCH_STATUS_FAILURE);
          value_md->add_allowed_object_types(allowed_object_info->object_type);
        }
        object_info->set_counter_attr_stats(attr_md->attr_id);
      }
    }
  }

  if (root_cjson) cJSON_Delete(root_cjson);

  model_info->compute_directed_object_graph();

  return model_info;

end:
  if (root_cjson) cJSON_Delete(root_cjson);
  return nullptr;
}

std::unique_ptr<ModelInfo> build_model_info_from_file(
    const char *const config_file, bool verbose) {
  if (config_file == NULL) return nullptr;
  memset(parser_obj_type, 0, sizeof(parser_obj_type));
  memset(parser_attr_ids, 0, sizeof(parser_attr_ids));
  current_parser_object_id = 0;
  current_parser_attr_id = 0;

  char *buffer = read_file_to_buffer(config_file);
  if (buffer == NULL) return nullptr;

  std::unique_ptr<ModelInfo> info =
      build_model_info_from_string(buffer, verbose);

  free(buffer);
  return info;
}

}  // namespace smi
