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
#include <stdio.h>

#include "bf_switch/bf_switch_types.h"
#include "s3/attribute.h"

void test_attribute_base() {
  const size_t size_of_attr = sizeof(switch_attribute_t);
  union {
    switch_attribute_t attr;
    char v[size_of_attr];
  } attr_char_u;
  // fill with some stuff;
  for (size_t i = 0; i < size_of_attr; i++) {
    attr_char_u.v[i] = 0xFF - i;
  }
  switch_attribute_t attr_from = attr_char_u.attr;
  switch_attribute_t attr_to = {0};
  switch_attribute_value_t value_out;

  switch_status_t status = 0;

  // v_u32_set(&attr_to.value, attr_from.value.u32);

  // status |= v_u32_get(&attr_to.value, &value_out.u32);

  assert(status == 0);
  // assert(attr_to.value.u32 == value_out.u32);

  attr_id_set(&attr_to, attr_from.id);
  for (unsigned i = SWITCH_TYPE_FIRST; i < SWITCH_TYPE_MAX; i++) {
    const switch_attr_type_t t = (switch_attr_type_t)(i);
    printf("testing %s\n", switch_attr_type_str(t));

    if (is_composite_type(t)) continue;
    switch (t) {
      case SWITCH_TYPE_BOOL:
        v_bool_set(&attr_to.value, attr_from.value.booldata);
        break;

      case SWITCH_TYPE_UINT8:
        v_u8_set(&attr_to.value, attr_from.value.u8);
        break;

      case SWITCH_TYPE_UINT16:
        v_u16_set(&attr_to.value, attr_from.value.u16);
        break;

      case SWITCH_TYPE_UINT32:
        v_u32_set(&attr_to.value, attr_from.value.u32);
        break;

      case SWITCH_TYPE_UINT64:
        v_u64_set(&attr_to.value, attr_from.value.u64);

        break;

      case SWITCH_TYPE_INT64:
        v_s64_set(&attr_to.value, attr_from.value.s64);

        break;

      case SWITCH_TYPE_ENUM:
        v_enum_set(&attr_to.value, attr_from.value.enumdata);
        break;

      case SWITCH_TYPE_MAC:
        v_mac_set(&attr_to.value, attr_from.value.mac);

        break;

      case SWITCH_TYPE_STRING:
        v_string_set(&attr_to.value, attr_from.value.text);

        break;

      case SWITCH_TYPE_IP_ADDRESS:
        v_ipaddr_set(&attr_to.value, attr_from.value.ipaddr);

        break;

      case SWITCH_TYPE_IP_PREFIX:
        v_ipprefix_set(&attr_to.value, attr_from.value.ipprefix);

        break;

      case SWITCH_TYPE_OBJECT_ID:
        v_oid_set(&attr_to.value, attr_from.value.oid);
        break;

      case SWITCH_TYPE_RANGE:
        v_range_set(&attr_to.value, attr_from.value.range);
        break;

      default:
        printf("missing test for type %s : %u\n", switch_attr_type_str(t), i);
        assert(0);
    }

    bool equal = 0;
    for (unsigned j = SWITCH_TYPE_FIRST; j < SWITCH_TYPE_MAX; j++) {
      const switch_attr_type_t t_get = (switch_attr_type_t)(j);

      switch (t_get) {
        case SWITCH_TYPE_BOOL:
          status = v_bool_get(&attr_to.value, &value_out.booldata);
          equal = attr_from.value.booldata == value_out.booldata;
          break;
        case SWITCH_TYPE_UINT8:
          status = v_u8_get(&attr_to.value, &value_out.u8);
          equal = attr_from.value.u8 == value_out.u8;
          break;
        case SWITCH_TYPE_UINT16:
          status = v_u16_get(&attr_to.value, &value_out.u16);
          equal = attr_from.value.u16 == value_out.u16;
          break;
        case SWITCH_TYPE_UINT32:
          status = v_u32_get(&attr_to.value, &value_out.u32);
          equal = attr_from.value.u32 == value_out.u32;
          break;
        case SWITCH_TYPE_UINT64:
          status = v_u64_get(&attr_to.value, &value_out.u64);
          equal = attr_from.value.u64 == value_out.u64;
          break;

        case SWITCH_TYPE_INT64:
          status = v_s64_get(&attr_to.value, &value_out.s64);
          equal = attr_from.value.s64 == value_out.s64;
          break;

        case SWITCH_TYPE_ENUM:
          status = v_enum_get(&attr_to.value, &value_out.enumdata);
          equal =
              attr_from.value.enumdata.enumdata == value_out.enumdata.enumdata;
          break;

        case SWITCH_TYPE_MAC:
          status = v_mac_get(&attr_to.value, &value_out.mac);
          equal = (memcmp(&attr_from.value.mac,
                          &value_out.mac,
                          sizeof(value_out.mac)) == 0);
          break;

        case SWITCH_TYPE_STRING:
          status = v_string_get(&attr_to.value, &value_out.text);
          equal = (memcmp(&attr_from.value.text,
                          &value_out.text,
                          sizeof(value_out.text)) == 0);
          break;

        case SWITCH_TYPE_IP_ADDRESS:
          status = v_ipaddr_get(&attr_to.value, &value_out.ipaddr);
          equal = memcmp(&attr_from.value.ipaddr,
                         &value_out.ipaddr,
                         sizeof(value_out.ipaddr)) == 0;
          break;

        case SWITCH_TYPE_IP_PREFIX:
          status = v_ipprefix_get(&attr_to.value, &value_out.ipprefix);
          equal = memcmp(&attr_from.value.ipprefix,
                         &value_out.ipprefix,
                         sizeof(value_out.ipprefix)) == 0;
          break;

        case SWITCH_TYPE_OBJECT_ID:
          status = v_oid_get(&attr_to.value, &value_out.oid);
          equal = (attr_from.value.oid.data == value_out.oid.data);

          break;

        case SWITCH_TYPE_RANGE:
          status = v_range_get(&attr_to.value, &value_out.range);
          equal = (attr_from.value.range.min == value_out.range.min);
          break;

        default:
          continue;
          break;
      }
      if (t == t_get) {
        assert(status == SWITCH_STATUS_SUCCESS);
        assert(equal);
      } else {
        assert(status != SWITCH_STATUS_SUCCESS);
      }
    }
  }
}

int main(void) {
  test_attribute_base();

  printf("\n\nAll tests passed!\n");
  return 0;
}
