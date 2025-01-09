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


#ifndef INCLUDE_S3_ATTRIBUTE_H__
#define INCLUDE_S3_ATTRIBUTE_H__

#include <string.h>

#include "bf_switch/bf_switch_types.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *switch_attr_type_str(switch_attr_type_t t);

static inline bool is_composite_type(switch_attr_type_t attr_type) {
  switch (attr_type) {
    case SWITCH_TYPE_LIST:
      // case SWITCH_TYPE_MAP:
      // case SWITCH_TYPE_ACL_FIELD:
      // case SWITCH_TYPE_ACL_ACTION:
      return true;
    default:
      return false;
  }
}
/*
 *  TODO: This form of data structure isn't exactly memory efficient.
 *  instead of union, we could have the following from, along with helper
 *  access functions. The attribute ID can be used to validate.
 *
 *
 *  This form shouldn't be the one used to store, due to memory in efficiency.
 *  some serialized binary format will be more efficient.
 *
 *  It also might not be most efficient as an interface. Helper functions
 *  are used to manipulate attribute struct to allow future optimizations.
 *
 *	This structure of attribute type does not have implicit restrictions.
 *	A user may setup a list of list of map of uint32_t or so. switch_store
 * does validation based on schema.
 */

typedef struct _switch_attr_kv_t switch_attr_kv_t;

typedef struct _switch_acl_field_data_t {
  bool enable;
  switch_attr_type_t type;
  switch_attribute_value_t *mask;
  switch_attribute_value_t *data;
} switch_acl_field_data_t;

typedef struct _switch_acl_action_data_t {
  bool enable;
  switch_attr_type_t type;
  switch_attribute_value_t *parameter;
} switch_acl_action_data_t;

typedef struct _switch_attr_map_t {
  switch_attr_type_t k_type;
  switch_attr_type_t v_type;
  size_t count;
  switch_attr_kv_t *list;
} switch_attr_map_t;

typedef struct _switch_attr_kv_t {
  switch_attribute_value_t k, v;
} switch_attr_kv_t;

inline static switch_status_t v_bool_get(
    const switch_attribute_value_t *const value, bool *const val_out) {
  if (value->type != SWITCH_TYPE_BOOL) return SWITCH_STATUS_FAILURE;
  *val_out = value->booldata;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_u8_get(
    const switch_attribute_value_t *const value, uint8_t *const val_out) {
  if (value->type != SWITCH_TYPE_UINT8) return SWITCH_STATUS_FAILURE;
  *val_out = value->u8;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_u16_get(
    const switch_attribute_value_t *const value, uint16_t *const val_out) {
  if (value->type != SWITCH_TYPE_UINT16) return SWITCH_STATUS_FAILURE;
  *val_out = value->u16;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_u32_get(
    const switch_attribute_value_t *const value, uint32_t *const val_out) {
  if (value->type != SWITCH_TYPE_UINT32) return SWITCH_STATUS_FAILURE;
  *val_out = value->u32;
  return SWITCH_STATUS_SUCCESS;
}
/*

inline static switch_status_t v_s32_get(
    const switch_attribute_value_t *const value, int32_t *const val_out) {
  if (value->type != SWITCH_TYPE_INT32) return SWITCH_STATUS_FAILURE;
  *val_out = value->s32;
  return SWITCH_STATUS_SUCCESS;
}
*/

inline static switch_status_t v_enum_get(
    const switch_attribute_value_t *const value, switch_enum_t *const val_out) {
  if (value->type != SWITCH_TYPE_ENUM) return SWITCH_STATUS_FAILURE;
  *val_out = value->enumdata;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_u64_get(
    const switch_attribute_value_t *const value, uint64_t *const val_out) {
  if (value->type != SWITCH_TYPE_UINT64) return SWITCH_STATUS_FAILURE;
  *val_out = value->u64;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_s64_get(
    const switch_attribute_value_t *const value, int64_t *const val_out) {
  if (value->type != SWITCH_TYPE_INT64) return SWITCH_STATUS_FAILURE;
  *val_out = value->s64;
  return SWITCH_STATUS_SUCCESS;
}
inline static switch_status_t v_oid_get(
    const switch_attribute_value_t *const value,
    switch_object_id_t *const val_out) {
  if (value->type != SWITCH_TYPE_OBJECT_ID) return SWITCH_STATUS_FAILURE;
  *val_out = value->oid;
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_mac_get(
    const switch_attribute_value_t *const value,
    switch_mac_addr_t *const val_out) {
  if (value->type != SWITCH_TYPE_MAC) return SWITCH_STATUS_FAILURE;
  memcpy(val_out, &value->mac, sizeof(switch_mac_addr_t));
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_string_get(
    const switch_attribute_value_t *const value,
    switch_string_t *const val_out) {
  if (value->type != SWITCH_TYPE_STRING) return SWITCH_STATUS_FAILURE;
  memcpy(val_out, &value->text, sizeof(switch_string_t));
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_ipaddr_get(
    const switch_attribute_value_t *const value,
    switch_ip_address_t *const val_out) {
  if (value->type != SWITCH_TYPE_IP_ADDRESS) return SWITCH_STATUS_FAILURE;
  memcpy(val_out, &value->ipaddr, sizeof(switch_ip_address_t));
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_ipprefix_get(
    const switch_attribute_value_t *const value,
    switch_ip_prefix_t *const val_out) {
  if (value->type != SWITCH_TYPE_IP_PREFIX) return SWITCH_STATUS_FAILURE;
  memcpy(val_out, &value->ipprefix, sizeof(switch_ip_prefix_t));
  return SWITCH_STATUS_SUCCESS;
}

inline static switch_status_t v_range_get(
    const switch_attribute_value_t *const value,
    switch_range_t *const val_out) {
  if (value->type != SWITCH_TYPE_RANGE) return SWITCH_STATUS_FAILURE;
  memcpy(val_out, &value->range, sizeof(switch_range_t));
  return SWITCH_STATUS_SUCCESS;
}

inline static void attr_id_set(switch_attribute_t *const attr,
                               switch_attr_id_t id) {
  attr->id = id;
}

inline static void v_bool_set(switch_attribute_value_t *const value,
                              const bool val) {
  value->type = SWITCH_TYPE_BOOL;
  value->booldata = val;
}

inline static void v_u16_set(switch_attribute_value_t *const value,
                             uint16_t val) {
  value->type = SWITCH_TYPE_UINT16;
  value->u16 = val;
}

inline static void v_u8_set(switch_attribute_value_t *const value,
                            uint8_t val) {
  value->type = SWITCH_TYPE_UINT8;
  value->u8 = val;
}

inline static void v_u32_set(switch_attribute_value_t *const value,
                             const uint32_t val) {
  value->type = SWITCH_TYPE_UINT32;
  value->u32 = val;
}
/*

inline static void v_s32_set(switch_attribute_value_t *const value,
                             const int32_t val) {
  value->type = SWITCH_TYPE_INT32;
  value->s32 = val;
}
*/

inline static void v_enum_set(switch_attribute_value_t *const value,
                              const switch_enum_t val) {
  value->type = SWITCH_TYPE_ENUM;
  value->enumdata = val;
}

inline static void v_u64_set(switch_attribute_value_t *const value,
                             const uint64_t val) {
  value->type = SWITCH_TYPE_UINT64;
  value->u64 = val;
}

inline static void v_s64_set(switch_attribute_value_t *const value,
                             const int64_t val) {
  value->type = SWITCH_TYPE_INT64;
  value->s64 = val;
}

inline static void v_oid_set(switch_attribute_value_t *const value,
                             const switch_object_id_t val

) {
  value->type = SWITCH_TYPE_OBJECT_ID;
  value->oid = val;
}

inline static void v_mac_set(switch_attribute_value_t *const value,
                             const switch_mac_addr_t val) {
  value->type = SWITCH_TYPE_MAC;
  memcpy(&value->mac, &val, sizeof(val));
}

inline static void v_string_set(switch_attribute_value_t *const value,
                                const switch_string_t val) {
  value->type = SWITCH_TYPE_STRING;
  memcpy(&value->text, &val, sizeof(val));
}

inline static void v_ipaddr_set(switch_attribute_value_t *const value,
                                const switch_ip_address_t val) {
  value->type = SWITCH_TYPE_IP_ADDRESS;
  memcpy(&value->ipaddr, &val, sizeof(switch_ip_address_t));
}
inline static void v_ipprefix_set(switch_attribute_value_t *const value,
                                  const switch_ip_prefix_t val) {
  value->type = SWITCH_TYPE_IP_PREFIX;
  memcpy(&value->ipprefix, &val, sizeof(switch_ip_prefix_t));
}
inline static void v_range_set(switch_attribute_value_t *const value,
                               const switch_range_t val) {
  value->type = SWITCH_TYPE_RANGE;
  memcpy(&value->range, &val, sizeof(switch_range_t));
}

switch_status_t attr_copy(switch_attribute_t *const attr_dst,
                          const switch_attribute_t *const attr_src);

#ifdef __cplusplus
}
#endif
#endif  // INCLUDE_S3_ATTRIBUTE_H__
