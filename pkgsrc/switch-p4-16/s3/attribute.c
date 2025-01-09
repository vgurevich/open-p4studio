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


#include "s3/attribute.h"
#include "log.h"

/* no nesting allowed */
switch_status_t attr_copy(switch_attribute_t *const attr_dst,
                          const switch_attribute_t *const attr_src) {
  CHECK_RET(attr_src == NULL, SWITCH_STATUS_FAILURE);
  CHECK_RET(attr_dst == NULL, SWITCH_STATUS_FAILURE);
  *attr_dst = *attr_src;
  return SWITCH_STATUS_SUCCESS;
}

const char *switch_attr_type_str(switch_attr_type_t t) {
  const char *type_str;
  switch (t) {
    case SWITCH_TYPE_NONE:
      type_str = "SWITCH_TYPE_NONE";
      break;
    case SWITCH_TYPE_BOOL:
      type_str = "SWITCH_TYPE_BOOL";
      break;
    case SWITCH_TYPE_UINT8:
      type_str = "SWITCH_TYPE_UINT8:";
      break;
    case SWITCH_TYPE_UINT16:
      type_str = "SWITCH_TYPE_UINT16";
      break;
    case SWITCH_TYPE_UINT32:
      type_str = "SWITCH_TYPE_UINT32";
      break;
    case SWITCH_TYPE_UINT64:
      type_str = "SWITCH_TYPE_UINT64";
      break;
    case SWITCH_TYPE_INT64:
      type_str = "SWITCH_TYPE_INT64";
      break;
    case SWITCH_TYPE_ENUM:
      type_str = "SWITCH_TYPE_ENUM";
      break;
    case SWITCH_TYPE_MAC:
      type_str = "SWITCH_TYPE_MAC";
      break;
    case SWITCH_TYPE_STRING:
      type_str = "SWITCH_TYPE_STRING";
      break;
    case SWITCH_TYPE_RANGE:
      type_str = "SWITCH_TYPE_RANGE";
      break;
    case SWITCH_TYPE_IP_ADDRESS:
      type_str = "SWITCH_TYPE_IP_ADDRESS";
      break;
    case SWITCH_TYPE_IP_PREFIX:
      type_str = "SWITCH_TYPE_IP_PREFIX";
      break;
    case SWITCH_TYPE_OBJECT_ID:
      type_str = "SWITCH_TYPE_OBJECT_ID";
      break;
    case SWITCH_TYPE_LIST:
      type_str = "SWITCH_TYPE_LIST";
      break;
    case SWITCH_TYPE_MAX:
      type_str = "SWITCH_TYPE_MAX";
      break;
    default:
      type_str = "Unknown type";
  }

  return type_str;
}
