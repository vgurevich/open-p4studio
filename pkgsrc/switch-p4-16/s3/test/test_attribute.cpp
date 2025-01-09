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

#include <cassert>
#include <iostream>

#include "bf_switch/bf_switch_types.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"

using namespace smi;

static ModelInfo *model_info = NULL;
static switch_object_type_t test_object_2;
const ObjectInfo *test_object_2_info;
static switch_object_type_t test_object_list;
const ObjectInfo *list_info;
static switch_attr_id_t b_list_attr;
static switch_attr_id_t u8_list_attr;
static switch_attr_id_t u16_list_attr;
static switch_attr_id_t u32_list_attr;
static switch_attr_id_t u64_list_attr;
static switch_attr_id_t s64_list_attr;
static switch_attr_id_t str_list_attr;
static switch_attr_id_t oid_list_attr;
static switch_attr_id_t mac_list_attr;
static switch_attr_id_t ip_list_attr;
static switch_attr_id_t prefix_list_attr;

void init_objects() {
  test_object_2_info = model_info->get_object_info_from_name("test_object_2");
  test_object_2 = test_object_2_info->object_type;
  assert(test_object_2 != 0);
  list_info = model_info->get_object_info_from_name("test_object_list");
  test_object_list = list_info->object_type;
  assert(test_object_list != 0);
  b_list_attr = list_info->get_attr_id_from_name("test_attribute_list_bool");
  assert(b_list_attr != 0);
  u8_list_attr = list_info->get_attr_id_from_name("test_attribute_list_uint8");
  assert(u8_list_attr != 0);
  u16_list_attr =
      list_info->get_attr_id_from_name("test_attribute_list_uint16");
  assert(u16_list_attr != 0);
  u32_list_attr =
      list_info->get_attr_id_from_name("test_attribute_list_uint32");
  assert(u32_list_attr != 0);
  u64_list_attr =
      list_info->get_attr_id_from_name("test_attribute_list_uint64");
  assert(u64_list_attr != 0);
  s64_list_attr = list_info->get_attr_id_from_name("test_attribute_list_int64");
  assert(s64_list_attr != 0);
  str_list_attr =
      list_info->get_attr_id_from_name("test_attribute_list_string");
  assert(str_list_attr != 0);
  oid_list_attr = list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(oid_list_attr != 0);
  mac_list_attr = list_info->get_attr_id_from_name("test_attribute_list_mac");
  assert(mac_list_attr != 0);
  ip_list_attr = list_info->get_attr_id_from_name("test_attribute_list_ip");
  assert(ip_list_attr != 0);
  prefix_list_attr =
      list_info->get_attr_id_from_name("test_attribute_list_prefix");
  assert(prefix_list_attr != 0);
}

void test_attribute() {
  std::cout << "**** Testing attribute util set get ****" << std::endl;
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    bool value = true, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    uint8_t value = 0xff, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    uint16_t value = 0xffff, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    uint32_t value = 0xffffffff, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    uint64_t value = 0xffffffffffffffff, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    int64_t value = -128, value_get = 0;
    ;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get == value);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_enum_t value = {.enumdata = 0xffff}, value_get = {.enumdata = 0};
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.enumdata == value.enumdata);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_enum_t value = {.enumdata = 0xffff}, value_get = {.enumdata = 0};
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.enumdata == value.enumdata);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_mac_addr_t a = {.mac = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6}};
    switch_mac_addr_t b = {.mac = {0, 0, 0, 0, 0, 0}};
    attr_set.v_set(a);
    attr_set.v_get(b);
    for (int i = 0; i < ETH_LEN; i++) {
      assert(a.mac[i] == b.mac[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    std::string mac_str = "01:02:03:04:05:06";
    switch_mac_addr_t a = {};
    switch_mac_addr_t aa = {.mac = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6}};
    attr_util::parse_mac(mac_str, a);
    for (int i = 0; i < ETH_LEN; i++) {
      assert(a.mac[i] == aa.mac[i]);
    }
    switch_mac_addr_t b = {.mac = {0, 0, 0, 0, 0, 0}};
    attr_set.v_set(a);
    attr_set.v_get(b);
    for (int i = 0; i < ETH_LEN; i++) {
      assert(a.mac[i] == b.mac[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_address_t value, value_get;
    value.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    value.ip4 = 168430081;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.addr_family == value.addr_family);
    assert(value_get.ip4 == value.ip4);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_address_t value, value_get;
    value.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    for (int i = 0; i < IPV6_LEN; i++) {
      value.ip6[i] = i + 1;
    }
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.addr_family == value.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(value_get.ip6[i] == value.ip6[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    switch_ip_address_t a, value, value_get;
    attr_w attr_set(attr_id);
    std::string ip_addr_str = "10.10.10.1";
    attr_util::parse_ip_address(ip_addr_str, a);
    value.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    value.ip4 = 168430081;
    assert(value.addr_family == a.addr_family);
    assert(value.ip4 == a.ip4);
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.addr_family == value.addr_family);
    assert(value_get.ip4 == value.ip4);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    switch_ip_address_t a, value, value_get;
    attr_w attr_set(attr_id);
    std::string ip_addr_str = "0102:0304:0506:0708:090a:0b0c:0d0e:0f10";
    attr_util::parse_ip_address(ip_addr_str, a);
    value.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    for (int i = 0; i < IPV6_LEN; i++) {
      value.ip6[i] = i + 1;
    }
    assert(value.addr_family == a.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(value.ip6[i] == a.ip6[i]);
    }
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.addr_family == value.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(value_get.ip6[i] == value.ip6[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_prefix_t value, value_get;
    value.len = 24;
    value.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    value.addr.ip4 = 168430080;
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.len == value.len);
    assert(value_get.addr.addr_family == value.addr.addr_family);
    assert(value_get.addr.ip4 == value.addr.ip4);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_prefix_t a, value = {}, value_get = {};
    std::string ip_addr_str = "10.10.10.0/24";
    attr_util::parse_ip_prefix(ip_addr_str, a);
    value.len = 24;
    value.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    value.addr.ip4 = 168430080;
    assert(a.len == value.len);
    assert(a.addr.addr_family == value.addr.addr_family);
    assert(a.addr.ip4 == value.addr.ip4);
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.len == value.len);
    assert(value_get.addr.addr_family == value.addr.addr_family);
    assert(value_get.addr.ip4 == value.addr.ip4);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_prefix_t value = {}, value_get = {};
    value.len = 96;
    value.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    for (int i = 0; i < IPV6_LEN - 4; i++) {
      value.addr.ip6[i] = i + 1;
    }
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.len == value.len);
    assert(value_get.addr.addr_family == value.addr.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(value_get.addr.ip6[i] == value.addr.ip6[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_ip_prefix_t a, value = {}, value_get = {};
    std::string ip_addr_str = "0102:0304:0506:0708:090a:0b0c:0000:0000/96";
    attr_util::parse_ip_prefix(ip_addr_str, a);
    value.len = 96;
    value.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    for (int i = 0; i < IPV6_LEN - 4; i++) {
      value.addr.ip6[i] = i + 1;
    }
    assert(a.len == value.len);
    assert(a.addr.addr_family == value.addr.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(a.addr.ip6[i] == value.addr.ip6[i]);
    }
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.len == value.len);
    assert(value_get.addr.addr_family == value.addr.addr_family);
    for (int i = 0; i < IPV6_LEN; i++) {
      assert(value_get.addr.ip6[i] == value.addr.ip6[i]);
    }
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_string_t value = {}, value_get = {};
    memcpy(value.text, "hello", 5);
    value.text[5] = '\0';
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(strcmp(value.text, value_get.text) == 0);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
  {
    switch_attr_id_t attr_id = 10;
    attr_w attr_set(attr_id);
    switch_range_t value = {.min = 0xff, .max = 0xffff}, value_get = {};
    attr_set.v_set(value);
    attr_set.v_get(value_get);
    assert(value_get.min == value.min);
    assert(value_get.max == value.max);
    std::string str;
    attr_set.attr_to_string(str);
    std::cout << str;
  }
}
void test_list() {
  std::cout << "**** Testing lists ****" << std::endl;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> attrs;
  switch_object_id_t oid1 = {}, oid2 = {}, oid3 = {}, oid4 = {};

  switch_store::object_create(test_object_2, attrs, oid1);
  switch_store::object_create(test_object_2, attrs, oid2);
  switch_store::object_create(test_object_2, attrs, oid3);
  switch_store::object_create(test_object_2, attrs, oid4);
  std::cout << "**** Bool ****" << std::endl;
  {
    bool a = true, b = false, c = false, d = true;
    std::vector<bool> lset = {a, b, c, d};
    attr_w list_attr(b_list_attr);
    list_attr.v_set(lset);
    std::vector<bool> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** uint8_t ****" << std::endl;
  {
    uint8_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint8_t> lset = {a, b, c, d};
    attr_w list_attr(u8_list_attr);
    list_attr.v_set(lset);
    std::vector<uint8_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** uint16_t ****" << std::endl;
  {
    uint16_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint16_t> lset = {a, b, c, d};
    attr_w list_attr(u16_list_attr);
    list_attr.v_set(lset);
    std::vector<uint16_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** uint32_t ****" << std::endl;
  {
    uint32_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint32_t> lset = {a, b, c, d};
    attr_w list_attr(u32_list_attr);
    list_attr.v_set(lset);
    std::vector<uint32_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** uint64_t ****" << std::endl;
  {
    uint64_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint64_t> lset = {a, b, c, d};
    attr_w list_attr(u64_list_attr);
    list_attr.v_set(lset);
    std::vector<uint64_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** int64_t ****" << std::endl;
  {
    int64_t a = -1, b = -2, c = -3, d = -4;
    std::vector<int64_t> lset = {a, b, c, d};
    attr_w list_attr(s64_list_attr);
    list_attr.v_set(lset);
    std::vector<int64_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** string ****" << std::endl;
  {
    switch_string_t a = {}, b = {}, c = {}, d = {};
    memcpy(a.text, "hello1", 6);
    a.text[6] = '\0';
    memcpy(b.text, "hello2", 6);
    b.text[6] = '\0';
    memcpy(c.text, "hello3", 6);
    c.text[6] = '\0';
    memcpy(d.text, "hello4", 6);
    d.text[6] = '\0';
    std::vector<switch_string_t> lset = {a, b, c, d};
    attr_w list_attr(str_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_string_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(strcmp(lset[i].text, lget[i].text) == 0);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** oid ****" << std::endl;
  {
    std::vector<switch_object_id_t> lset = {oid1, oid2, oid3, oid4};
    attr_w list_attr(oid_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_object_id_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].data == lget[i].data);
    }
    attrs.insert(list_attr);
  }
  std::cout << "**** ip ****" << std::endl;
  {
    switch_ip_address_t a;
    a.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    a.ip4 = 168430081;
    switch_ip_address_t b;
    b.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    b.ip6[0] = 1;
    b.ip6[15] = 15;
    switch_ip_address_t c;
    c.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    c.ip6[0] = 11;
    c.ip6[15] = 115;
    switch_ip_address_t d;
    d.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    d.ip4 = 185273089;
    std::vector<switch_ip_address_t> lset = {a, b, c, d};
    attr_w list_attr(ip_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_ip_address_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].addr_family == lget[i].addr_family);
    }
    assert(lset[0].ip4 == lget[0].ip4);
    assert(lset[1].ip6[0] == lget[1].ip6[0]);
    assert(lset[1].ip6[15] == lget[1].ip6[15]);
    assert(lset[2].ip6[0] == lget[2].ip6[0]);
    assert(lset[2].ip6[15] == lget[2].ip6[15]);
    assert(lset[3].ip4 == lget[3].ip4);
    attrs.insert(list_attr);
  }
  std::cout << "**** ip prefix ****" << std::endl;
  {
    switch_ip_prefix_t a, b;
    a.len = 32;
    a.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    a.addr.ip4 = 168430081;
    b.len = 120;
    b.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    b.addr.ip6[0] = 11;
    b.addr.ip6[15] = 115;
    std::vector<switch_ip_prefix_t> lset = {a, b};
    attr_w list_attr(prefix_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_ip_prefix_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].len == lget[i].len);
      assert(lset[i].addr.addr_family == lget[i].addr.addr_family);
    }
    assert(lset[0].addr.ip4 == lget[0].addr.ip4);
    assert(lset[1].addr.ip6[0] == lget[1].addr.ip6[0]);
    assert(lset[1].addr.ip6[15] == lget[1].addr.ip6[15]);
    attrs.insert(list_attr);
  }
  std::cout << "**** mac ****" << std::endl;
  {
    switch_mac_addr_t a = {.mac = {0x1, 0, 0, 0, 0, 0}};
    switch_mac_addr_t b = {.mac = {0, 0x1, 0, 0, 0, 0}};
    switch_mac_addr_t c = {.mac = {0, 0, 0x1, 0, 0, 0}};
    switch_mac_addr_t d = {.mac = {0, 0, 0, 0x1, 0, 0}};
    switch_mac_addr_t e = {.mac = {0, 0, 0, 0, 0x1, 0}};
    switch_mac_addr_t f = {.mac = {0, 0, 0, 0, 0, 0x1}};
    std::vector<switch_mac_addr_t> lset = {a, b, c, d, e, f};
    attr_w list_attr(mac_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_mac_addr_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].mac[i] == lget[i].mac[i]);
    }
    attrs.insert(list_attr);
  }
  switch_object_id_t oid = {};
  status = switch_store::object_create(test_object_list, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  {
    attr_w list_attr(b_list_attr);
    status = switch_store::attribute_get(oid, b_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_BOOL);
    bool a = true, b = false, c = false, d = true;
    std::vector<bool> lset = {a, b, c, d};
    std::vector<bool> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "bool " << list_string << "\n";
  }
  {
    attr_w list_attr(u8_list_attr);
    status = switch_store::attribute_get(oid, u8_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_UINT8);
    uint8_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint8_t> lset = {a, b, c, d};
    std::vector<uint8_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u8 " << list_string << "\n";
  }
  {
    attr_w list_attr(u16_list_attr);
    status = switch_store::attribute_get(oid, u16_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_UINT16);
    uint16_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint16_t> lset = {a, b, c, d};
    std::vector<uint16_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u16 " << list_string << "\n";
  }
  {
    attr_w list_attr(u32_list_attr);
    status = switch_store::attribute_get(oid, u32_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_UINT32);
    uint32_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint32_t> lset = {a, b, c, d};
    std::vector<uint32_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u32 " << list_string << "\n";
  }
  {
    attr_w list_attr(u64_list_attr);
    status = switch_store::attribute_get(oid, u64_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_UINT64);
    uint64_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint64_t> lset = {a, b, c, d};
    std::vector<uint64_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u64 " << list_string << "\n";
  }
  {
    attr_w list_attr(s64_list_attr);
    status = switch_store::attribute_get(oid, s64_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_INT64);
    int64_t a = -1, b = -2, c = -3, d = -4;
    std::vector<int64_t> lset = {a, b, c, d};
    std::vector<int64_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "s64 " << list_string << "\n";
  }
  {
    attr_w list_attr(str_list_attr);
    status = switch_store::attribute_get(oid, str_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_STRING);
    switch_string_t a = {}, b = {}, c = {}, d = {};
    memcpy(a.text, "hello1", 6);
    a.text[6] = '\0';
    memcpy(b.text, "hello2", 6);
    b.text[6] = '\0';
    memcpy(c.text, "hello3", 6);
    c.text[6] = '\0';
    memcpy(d.text, "hello4", 6);
    d.text[6] = '\0';
    std::vector<switch_string_t> lset = {a, b, c, d};
    std::vector<switch_string_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(strcmp(lset[i].text, lget[i].text) == 0);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "string " << list_string << "\n";
  }
  {
    attr_w list_attr(oid_list_attr);
    status = switch_store::attribute_get(oid, oid_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_OBJECT_ID);
    std::vector<switch_object_id_t> lset = {oid1, oid2, oid3, oid4};
    std::vector<switch_object_id_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].data == lget[i].data);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "oid " << list_string << "\n";
  }
  {
    attr_w list_attr(ip_list_attr);
    status = switch_store::attribute_get(oid, ip_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_IP_ADDRESS);
    switch_ip_address_t a;
    a.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    a.ip4 = 168430081;
    switch_ip_address_t b;
    b.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    b.ip6[0] = 1;
    b.ip6[15] = 15;
    switch_ip_address_t c;
    c.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    c.ip6[0] = 11;
    c.ip6[15] = 115;
    switch_ip_address_t d;
    d.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    d.ip4 = 185273089;
    std::vector<switch_ip_address_t> lset = {a, b, c, d};
    std::vector<switch_ip_address_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].addr_family == lget[i].addr_family);
    }
    assert(lset[0].ip4 == lget[0].ip4);
    assert(lset[1].ip6[0] == lget[1].ip6[0]);
    assert(lset[1].ip6[15] == lget[1].ip6[15]);
    assert(lset[2].ip6[0] == lget[2].ip6[0]);
    assert(lset[2].ip6[15] == lget[2].ip6[15]);
    assert(lset[3].ip4 == lget[3].ip4);
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "ip " << list_string << "\n";
  }
  {
    attr_w list_attr(prefix_list_attr);
    status = switch_store::attribute_get(oid, prefix_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_IP_PREFIX);
    switch_ip_prefix_t a, b;
    a.len = 32;
    a.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
    a.addr.ip4 = 168430081;
    b.len = 120;
    b.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
    b.addr.ip6[0] = 11;
    b.addr.ip6[15] = 115;
    std::vector<switch_ip_prefix_t> lset = {a, b};
    std::vector<switch_ip_prefix_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].len == lget[i].len);
      assert(lset[i].addr.addr_family == lget[i].addr.addr_family);
    }
    assert(lset[0].addr.ip4 == lget[0].addr.ip4);
    assert(lset[1].addr.ip6[0] == lget[1].addr.ip6[0]);
    assert(lset[1].addr.ip6[15] == lget[1].addr.ip6[15]);
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "ip prefix " << list_string << "\n";
  }
  {
    attr_w list_attr(mac_list_attr);
    status = switch_store::attribute_get(oid, mac_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_MAC);
    switch_mac_addr_t a = {.mac = {0x1, 0, 0, 0, 0, 0}};
    switch_mac_addr_t b = {.mac = {0, 0x1, 0, 0, 0, 0}};
    switch_mac_addr_t c = {.mac = {0, 0, 0x1, 0, 0, 0}};
    switch_mac_addr_t d = {.mac = {0, 0, 0, 0x1, 0, 0}};
    switch_mac_addr_t e = {.mac = {0, 0, 0, 0, 0x1, 0}};
    switch_mac_addr_t f = {.mac = {0, 0, 0, 0, 0, 0x1}};
    std::vector<switch_mac_addr_t> lset = {a, b, c, d, e, f};
    std::vector<switch_mac_addr_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].mac[i] == lget[i].mac[i]);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "mac " << list_string << "\n";
  }
  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_attribute_set() {
  std::cout << "**** Testing lists attribute uint8 set ****" << std::endl;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> attrs;
  {
    uint8_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint8_t> lset = {a, b, c, d};
    attr_w list_attr(u8_list_attr);
    list_attr.v_set(lset);
    std::vector<uint8_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
    attrs.insert(list_attr);
  }
  switch_object_id_t oid = {};
  status = switch_store::object_create(test_object_list, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  {
    attr_w list_attr(u8_list_attr);
    status = switch_store::attribute_get(oid, u8_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_UINT8);
    uint8_t a = 1, b = 2, c = 3, d = 4;
    std::vector<uint8_t> lset = {a, b, c, d};
    std::vector<uint8_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
  }
  {
    uint8_t a = 5, b = 6, c = 7, d = 8;
    std::vector<uint8_t> lset = {a, b, c, d};
    attr_w list_attr(u8_list_attr, lset);
    status = switch_store::attribute_set(oid, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr_w new_list_attr(u8_list_attr);
    status = switch_store::attribute_get(oid, u8_list_attr, new_list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(new_list_attr.list_type_get() == SWITCH_TYPE_UINT8);
    std::vector<uint8_t> lget;
    new_list_attr.v_get(lget);
    assert(lget.size() == 4);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i] == lget[i]);
    }
  }
  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_attribute_mac_set() {
  std::cout << "**** Testing lists attribute mac set ****" << std::endl;
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  std::set<attr_w> attrs;
  {
    switch_mac_addr_t a = {.mac = {0x1, 0, 0, 0, 0, 0}};
    switch_mac_addr_t b = {.mac = {0, 0x1, 0, 0, 0, 0}};
    switch_mac_addr_t c = {.mac = {0, 0, 0x1, 0, 0, 0}};
    switch_mac_addr_t d = {.mac = {0, 0, 0, 0x1, 0, 0}};
    switch_mac_addr_t e = {.mac = {0, 0, 0, 0, 0x1, 0}};
    switch_mac_addr_t f = {.mac = {0, 0, 0, 0, 0, 0x1}};
    std::vector<switch_mac_addr_t> lset = {a, b, c, d, e, f};
    attr_w list_attr(mac_list_attr);
    list_attr.v_set(lset);
    std::vector<switch_mac_addr_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].mac[i] == lget[i].mac[i]);
    }
    attrs.insert(list_attr);
  }
  switch_object_id_t oid = {};
  status = switch_store::object_create(test_object_list, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  {
    attr_w list_attr(mac_list_attr);
    status = switch_store::attribute_get(oid, mac_list_attr, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(list_attr.list_type_get() == SWITCH_TYPE_MAC);
    switch_mac_addr_t a = {.mac = {0x1, 0, 0, 0, 0, 0}};
    switch_mac_addr_t b = {.mac = {0, 0x1, 0, 0, 0, 0}};
    switch_mac_addr_t c = {.mac = {0, 0, 0x1, 0, 0, 0}};
    switch_mac_addr_t d = {.mac = {0, 0, 0, 0x1, 0, 0}};
    switch_mac_addr_t e = {.mac = {0, 0, 0, 0, 0x1, 0}};
    switch_mac_addr_t f = {.mac = {0, 0, 0, 0, 0, 0x1}};
    std::vector<switch_mac_addr_t> lset = {a, b, c, d, e, f};
    std::vector<switch_mac_addr_t> lget;
    list_attr.v_get(lget);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].mac[i] == lget[i].mac[i]);
    }
  }
  {
    switch_mac_addr_t a = {.mac = {0x4, 0, 0, 0, 0, 0}};
    switch_mac_addr_t b = {.mac = {0, 0x4, 0, 0, 0, 0}};
    switch_mac_addr_t c = {.mac = {0, 0, 0x4, 0, 0, 0}};
    switch_mac_addr_t d = {.mac = {0, 0, 0, 0x4, 0, 0}};
    std::vector<switch_mac_addr_t> lset = {a, b, c, d};
    attr_w list_attr(mac_list_attr, lset);
    status = switch_store::attribute_set(oid, list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr_w new_list_attr(mac_list_attr);
    status = switch_store::attribute_get(oid, mac_list_attr, new_list_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(new_list_attr.list_type_get() == SWITCH_TYPE_MAC);
    std::vector<switch_mac_addr_t> lget;
    new_list_attr.v_get(lget);
    assert(lget.size() == 4);
    for (uint32_t i = 0; i < lset.size(); i++) {
      assert(lset[i].mac[i] == lget[i].mac[i]);
    }
  }
  status = switch_store::object_delete(oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_attribute_build() {
  std::cout << "**** Testing attribute build ****" << std::endl;
  switch_attribute_t attr;
  size_t i = 0;

  // verify import/export for bool data type
  memset(&attr, 0, sizeof(switch_attribute_t));
  attr.id = u8_list_attr;
  attr.value.type = SWITCH_TYPE_LIST;
  attr.value.list.list_type = SWITCH_TYPE_BOOL;
  attr.value.list.count = 4;
  attr.value.list.list =
      (switch_attribute_value_t *)calloc(4, sizeof(switch_attribute_value_t));
  switch_attribute_value_t *list = attr.value.list.list;
  while (i < attr.value.list.count) {
    list[i].booldata = true;
    i++;
  }
  {
    // import
    attr_w list_attr(b_list_attr);
    list_attr.attr_import(attr);
    std::vector<bool> lget;
    list_attr.v_get(lget);
    for (auto b : lget) {
      assert(b == true);
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "bool " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_BOOL);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].booldata);
      std::cout << new_attr.value.list.list[j].booldata << " ";
      j++;
    }
    std::cout << "\n";
  }

  // verify import/export for u8 data type
  memset(attr.value.list.list, 0, 4 * sizeof(switch_attribute_value_t));
  attr.value.list.list_type = SWITCH_TYPE_UINT8;
  list = attr.value.list.list;
  i = 0;
  while (i < attr.value.list.count) {
    list[i].u8 = i;
    i++;
  }
  {
    // import
    attr_w list_attr(u8_list_attr);
    list_attr.attr_import(attr);
    std::vector<uint8_t> lget;
    list_attr.v_get(lget);
    uint8_t v = 0;
    for (auto b : lget) {
      assert(b == v);
      v++;
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u8 " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_UINT8);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].u8);
      std::cout << new_attr.value.list.list[j].u8 << " ";
      j++;
    }
    std::cout << "\n";
  }

  // verify import/export for u16 data type
  memset(attr.value.list.list, 0, 4 * sizeof(switch_attribute_value_t));
  attr.value.list.list_type = SWITCH_TYPE_UINT16;
  list = attr.value.list.list;
  i = 0;
  while (i < attr.value.list.count) {
    list[i].u16 = i;
    i++;
  }
  {
    // import
    attr_w list_attr(u16_list_attr);
    list_attr.attr_import(attr);
    std::vector<uint16_t> lget;
    list_attr.v_get(lget);
    uint16_t v = 0;
    for (auto b : lget) {
      assert(b == v);
      v++;
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u16 " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_UINT16);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].u16);
      std::cout << new_attr.value.list.list[j].u16 << " ";
      j++;
    }
    std::cout << "\n";
  }

  // verify import/export for u32 data type
  memset(attr.value.list.list, 0, 4 * sizeof(switch_attribute_value_t));
  attr.value.list.list_type = SWITCH_TYPE_UINT32;
  list = attr.value.list.list;
  i = 0;
  while (i < attr.value.list.count) {
    list[i].u32 = i;
    i++;
  }
  {
    // import
    attr_w list_attr(u32_list_attr);
    list_attr.attr_import(attr);
    std::vector<uint32_t> lget;
    list_attr.v_get(lget);
    uint32_t v = 0;
    for (auto b : lget) {
      assert(b == v);
      v++;
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u32 " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_UINT32);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].u32);
      std::cout << new_attr.value.list.list[j].u32 << " ";
      j++;
    }
    std::cout << "\n";
  }

  // verify import/export for u64 data type
  memset(attr.value.list.list, 0, 4 * sizeof(switch_attribute_value_t));
  attr.value.list.list_type = SWITCH_TYPE_UINT64;
  list = attr.value.list.list;
  i = 0;
  while (i < attr.value.list.count) {
    list[i].u64 = i;
    i++;
  }
  {
    // import
    attr_w list_attr(u64_list_attr);
    list_attr.attr_import(attr);
    std::vector<uint64_t> lget;
    list_attr.v_get(lget);
    uint64_t v = 0;
    for (auto b : lget) {
      assert(b == v);
      v++;
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "u64 " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_UINT64);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].u64);
      std::cout << new_attr.value.list.list[j].u64 << " ";
      j++;
    }
    std::cout << "\n";
  }

  // verify import/export for i64 data type
  memset(attr.value.list.list, 0, 4 * sizeof(switch_attribute_value_t));
  attr.value.list.list_type = SWITCH_TYPE_INT64;
  list = attr.value.list.list;
  i = 0;
  while (i < attr.value.list.count) {
    list[i].s64 = i;
    i++;
  }
  {
    // import
    attr_w list_attr(s64_list_attr);
    list_attr.attr_import(attr);
    std::vector<int64_t> lget;
    list_attr.v_get(lget);
    int64_t v = 0;
    for (auto b : lget) {
      assert(b == v);
      v++;
    }
    std::string list_string;
    list_attr.attr_to_string(list_string);
    std::cout << "s64 " << list_string << "\n";

    // export
    switch_attribute_t new_attr;
    memset(&new_attr, 0, sizeof(switch_attribute_t));
    list_attr.attr_export(&new_attr);
    assert(new_attr.value.type == SWITCH_TYPE_LIST);
    assert(new_attr.value.list.list_type == SWITCH_TYPE_INT64);
    assert(new_attr.value.list.count == 4);
    uint32_t j = 0;
    while (j < new_attr.value.list.count) {
      assert(lget[j] == new_attr.value.list.list[j].s64);
      std::cout << new_attr.value.list.list[j].s64 << " ";
      j++;
    }
    std::cout << "\n";
  }
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  status = switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  init_objects();
  test_attribute();
  test_list();
  test_attribute_set();
  test_attribute_mac_set();
  test_attribute_build();

  std::cout << "All tests passed!\n";
  return 0;
}
