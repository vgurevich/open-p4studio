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
#include <sstream>

#include "gen-model/replay.h"
#include "bf_switch/bf_switch_types.h"
#include "s3/attribute.h"
#include "s3/factory.h"
#include "s3/switch_store.h"

using namespace smi;
using namespace std;

const char *const test_model_name = TESTDATADIR "/test/replay.json";
ModelInfo *model_info = NULL;
const ObjectInfo *test_object_1_info;
static switch_object_type_t test_object_1;
static switch_attr_id_t test_attribute_bool;
static switch_attr_id_t test_attribute_uint8;
static switch_attr_id_t test_attribute_uint16;
static switch_attr_id_t test_attribute_uint32;
static switch_attr_id_t test_attribute_uint64;
static switch_attr_id_t test_attribute_int64;
static switch_attr_id_t test_attribute_string;
static switch_attr_id_t test_attribute_range;

const ObjectInfo *test_object_2_info;
static switch_object_type_t test_object_2;
static switch_attr_id_t test_attribute_enum;

const ObjectInfo *test_object_3_info;
static switch_object_type_t test_object_3;
static switch_attr_id_t test_attribute_ip_addr;
static switch_attr_id_t test_attribute_ip_prefix;

const ObjectInfo *test_object_4_info;
static switch_object_type_t test_object_4;
static switch_attr_id_t test_attribute_mac;

const ObjectInfo *test_object_5_info;
static switch_object_type_t test_object_5;

const ObjectInfo *test_object_6_info;
static switch_object_type_t test_object_6;

const ObjectInfo *test_object_7_info;
static switch_object_type_t test_object_7;
static switch_attr_id_t test_attribute_oid;

const ObjectInfo *test_object_8_info;
static switch_object_type_t test_object_8;

const ObjectInfo *test_auto_81_info;
static switch_object_type_t test_auto_81;
static switch_attr_id_t test_auto_81_parent;

const ObjectInfo *test_object_list_info;
static switch_object_type_t test_object_list;
static switch_attr_id_t test_attribute_list_oid;
static switch_attr_id_t test_attribute_list_uint64;

const ObjectInfo *device_info;
static switch_object_type_t device;
static switch_attr_id_t device_port;

const ObjectInfo *port_info;
static switch_object_type_t port;
static switch_attr_id_t port_device;
static switch_attr_id_t port_mirror;

const ObjectInfo *mirror_info;
static switch_object_type_t mirror;
static switch_attr_id_t mirror_device;
static switch_attr_id_t mirror_port;

static switch_object_type_t test_object_9;
const ObjectInfo *test_object_9_info;
static switch_object_type_t test_key_1;
const ObjectInfo *test_key_1_info;
static switch_object_type_t test_key_2;
const ObjectInfo *test_key_2_info;
static switch_object_type_t test_key_3;
const ObjectInfo *test_key_3_info;
static switch_attr_id_t key_1_attr;
static switch_attr_id_t key_2_attr;
static switch_attr_id_t key_3_attr;
static switch_attr_id_t obj3_key1;
static switch_attr_id_t obj3_key2;
static switch_attr_id_t obj3_key3;

const ObjectInfo *test_object_10_info;
static switch_object_type_t test_object_10;
static switch_attr_id_t mem_list;
const ObjectInfo *member_ot_info;
static switch_object_type_t member_ot;
static switch_attr_id_t group_oid;

static switch_object_type_t test_object_u32;
const ObjectInfo *test_object_keygroup_list_u32_info;
static switch_attr_id_t key_uint32_u32_attr;
static switch_attr_id_t key_list_u32_attr;

void init_objects() {
  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  test_object_1_info = model_info->get_object_info_from_name("test_object_1");
  test_object_1 = test_object_1_info->object_type;
  assert(test_object_1 != 0);
  test_attribute_bool =
      test_object_1_info->get_attr_id_from_name("test_attribute_bool");
  assert(test_attribute_bool != 0);
  test_attribute_uint8 =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint8");
  assert(test_attribute_uint8 != 0);
  test_attribute_uint16 =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint16");
  assert(test_attribute_uint16 != 0);
  test_attribute_uint32 =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint32");
  assert(test_attribute_uint32 != 0);
  test_attribute_uint64 =
      test_object_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(test_attribute_uint64 != 0);
  test_attribute_int64 =
      test_object_1_info->get_attr_id_from_name("test_attribute_int64");
  assert(test_attribute_int64 != 0);
  test_attribute_string =
      test_object_1_info->get_attr_id_from_name("test_attribute_string");
  assert(test_attribute_string != 0);
  test_attribute_range =
      test_object_1_info->get_attr_id_from_name("test_attribute_range");
  assert(test_attribute_range != 0);

  test_object_2_info = model_info->get_object_info_from_name("test_object_2");
  test_object_2 = test_object_2_info->object_type;
  assert(test_object_2 != 0);
  test_attribute_enum =
      test_object_2_info->get_attr_id_from_name("test_attribute_enum");
  assert(test_attribute_enum != 0);

  test_object_3_info = model_info->get_object_info_from_name("test_object_3");
  test_object_3 = test_object_3_info->object_type;
  assert(test_object_3 != 0);
  test_attribute_ip_addr =
      test_object_3_info->get_attr_id_from_name("test_attribute_ip_addr");
  assert(test_attribute_ip_addr != 0);
  test_attribute_ip_prefix =
      test_object_3_info->get_attr_id_from_name("test_attribute_ip_prefix");
  assert(test_attribute_ip_prefix != 0);

  test_object_4_info = model_info->get_object_info_from_name("test_object_4");
  test_object_4 = test_object_4_info->object_type;
  assert(test_object_4 != 0);
  test_attribute_mac =
      test_object_4_info->get_attr_id_from_name("test_attribute_mac");
  assert(test_attribute_mac != 0);

  test_object_list_info =
      model_info->get_object_info_from_name("test_object_list");
  test_object_list = test_object_list_info->object_type;
  assert(test_object_list != 0);
  test_attribute_list_oid =
      test_object_list_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(test_attribute_list_oid != 0);
  test_attribute_list_uint64 = test_object_list_info->get_attr_id_from_name(
      "test_attribute_list_uint64");
  assert(test_attribute_list_uint64 != 0);

  test_object_5_info = model_info->get_object_info_from_name("test_object_5");
  test_object_5 = test_object_5_info->object_type;
  assert(test_object_5 != 0);

  test_object_6_info = model_info->get_object_info_from_name("test_object_6");
  test_object_6 = test_object_6_info->object_type;
  assert(test_object_6 != 0);

  test_object_7_info = model_info->get_object_info_from_name("test_object_7");
  test_object_7 = test_object_7_info->object_type;
  assert(test_object_7 != 0);
  test_attribute_oid =
      test_object_7_info->get_attr_id_from_name("test_attribute_oid");
  assert(test_attribute_oid != 0);

  test_object_8_info = model_info->get_object_info_from_name("test_object_8");
  test_object_8 = test_object_8_info->object_type;
  assert(test_object_8 != 0);

  test_auto_81_info = model_info->get_object_info_from_name("test_auto_81");
  test_auto_81 = test_auto_81_info->object_type;
  assert(test_auto_81 != 0);
  test_auto_81_parent =
      test_auto_81_info->get_attr_id_from_name("test_auto_81_parent");
  assert(test_auto_81_parent != 0);

  device_info = model_info->get_object_info_from_name("device");
  device = device_info->object_type;
  assert(device != 0);
  device_port = device_info->get_attr_id_from_name("device_port");
  assert(device_port != 0);

  port_info = model_info->get_object_info_from_name("port");
  port = port_info->object_type;
  assert(port != 0);
  port_device = port_info->get_attr_id_from_name("port_device");
  assert(port_device != 0);
  port_mirror = port_info->get_attr_id_from_name("port_mirror");
  assert(port_mirror != 0);

  mirror_info = model_info->get_object_info_from_name("mirror");
  mirror = mirror_info->object_type;
  assert(mirror != 0);
  mirror_device = mirror_info->get_attr_id_from_name("mirror_device");
  assert(mirror_device != 0);
  mirror_port = mirror_info->get_attr_id_from_name("mirror_port");
  assert(mirror_port != 0);

  test_object_9_info = model_info->get_object_info_from_name("test_object_9");
  test_object_9 = test_object_9_info->object_type;
  assert(test_object_3 != 0);
  test_key_1_info = model_info->get_object_info_from_name("test_key_1");
  test_key_1 = test_key_1_info->object_type;
  assert(test_key_1 != 0);
  test_key_2_info = model_info->get_object_info_from_name("test_key_2");
  test_key_2 = test_key_2_info->object_type;
  assert(test_key_2 != 0);
  test_key_3_info = model_info->get_object_info_from_name("test_key_3");
  test_key_3 = test_key_3_info->object_type;
  assert(test_key_3 != 0);
  key_1_attr = test_key_1_info->get_attr_id_from_name("test_attribute_uint64");
  assert(key_1_attr != 0);
  key_2_attr = test_key_2_info->get_attr_id_from_name("test_attribute_uint64");
  assert(key_2_attr != 0);
  key_3_attr = test_key_3_info->get_attr_id_from_name("test_attribute_uint64");
  assert(key_3_attr != 0);
  obj3_key1 = test_object_9_info->get_attr_id_from_name("test_key1");
  assert(obj3_key1 != 0);
  obj3_key2 = test_object_9_info->get_attr_id_from_name("test_key2");
  assert(obj3_key2 != 0);
  obj3_key3 = test_object_9_info->get_attr_id_from_name("test_key3");
  assert(obj3_key3 != 0);

  test_object_10_info = model_info->get_object_info_from_name("test_object_10");
  test_object_10 = test_object_10_info->object_type;
  assert(test_object_10 != 0);
  mem_list =
      test_object_10_info->get_attr_id_from_name("test_attribute_list_oid");
  assert(mem_list != 0);
  member_ot_info =
      model_info->get_object_info_from_name("test_object_membership");
  member_ot = member_ot_info->object_type;
  assert(member_ot != 0);
  group_oid = member_ot_info->get_attr_id_from_name("test_attribute_oid");
  assert(group_oid != 0);

  test_object_keygroup_list_u32_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_uint32");
  test_object_u32 = test_object_keygroup_list_u32_info->object_type;
  assert(test_object_u32 != 0);
  key_uint32_u32_attr =
      test_object_keygroup_list_u32_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_u32_attr != 0);
  key_list_u32_attr = test_object_keygroup_list_u32_info->get_attr_id_from_name(
      "test_attribute_list");
  assert(key_list_u32_attr != 0);
}

void test_simple_types() {
  std::cout << "**** Tesing objects creation for simple types ****"
            << std::endl;
  switch_status_t status;

  std::set<attr_w> attrs;
  switch_object_id_t oid = {};
  bool booldata = true, booldata_new = false;
  uint8_t uint8 = 0xff, uint8_new = 0;
  uint16_t uint16 = 0xffff, uint16_new = 0;
  uint32_t uint32 = 0xffffffff, uint32_new = 0;
  uint64_t uint64 = 0xffffffffffffffff, uint64_new = 0;
  int64_t int64 = -128, int64_new = 0;
  switch_range_t range = {.min = 0xfffff0, .max = 0xffffff};
  switch_range_t range_new = {.min = 0, .max = 0};
  std::string ctext = "hello";
  switch_string_t text = {}, text_new = {};
  strncpy(text.text, ctext.c_str(), sizeof(text.text) - 1);

  attrs.insert(attr_w(test_attribute_bool, booldata));
  attrs.insert(attr_w(test_attribute_uint8, uint8));
  attrs.insert(attr_w(test_attribute_uint16, uint16));
  attrs.insert(attr_w(test_attribute_uint32, uint32));
  attrs.insert(attr_w(test_attribute_uint64, uint64));
  attrs.insert(attr_w(test_attribute_int64, int64));
  attrs.insert(attr_w(test_attribute_string, text));
  attrs.insert(attr_w(test_attribute_range, range));
  status = switch_store::object_create(test_object_1, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_1, first_handle);
  std::cout << first_handle.data << " " << oid.data << std::endl;
  assert(first_handle.data == oid.data);

  // test bool attribute is set correctly in DB
  attr_w bool_attr(test_attribute_bool);
  status =
      switch_store::attribute_get(first_handle, test_attribute_bool, bool_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  bool_attr.v_get(booldata_new);
  std::cout << booldata_new << " " << booldata << std::endl;
  assert(booldata == booldata_new);

  // test uint8 attribute is set correctly in DB
  attr_w uint8_attr(test_attribute_uint8);
  status = switch_store::attribute_get(
      first_handle, test_attribute_uint8, uint8_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  uint8_attr.v_get(uint8_new);
  std::cout << unsigned(uint8_new) << " " << unsigned(uint8) << std::endl;
  assert(uint8 == uint8_new);

  // test uint16 attribute is set correctly in DB
  attr_w uint16_attr(test_attribute_uint16);
  status = switch_store::attribute_get(
      first_handle, test_attribute_uint16, uint16_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  uint16_attr.v_get(uint16_new);
  std::cout << uint16_new << " " << uint16 << std::endl;
  assert(uint16 == uint16_new);

  // test uint32 attribute is set correctly in DB
  attr_w uint32_attr(test_attribute_uint32);
  status = switch_store::attribute_get(
      first_handle, test_attribute_uint32, uint32_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  uint32_attr.v_get(uint32_new);
  std::cout << uint32_new << " " << uint32 << std::endl;
  assert(uint32 == uint32_new);

  // test uint64 attribute is set correctly in DB
  attr_w uint64_attr(test_attribute_uint64);
  status = switch_store::attribute_get(
      first_handle, test_attribute_uint64, uint64_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  uint64_attr.v_get(uint64_new);
  std::cout << uint64_new << " " << uint64 << std::endl;
  assert(uint64 == uint64_new);

  // test int64 attribute is set correctly in DB
  attr_w int64_attr(test_attribute_int64);
  status = switch_store::attribute_get(
      first_handle, test_attribute_int64, int64_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  int64_attr.v_get(int64_new);
  std::cout << int64_new << " " << int64 << std::endl;
  assert(int64 == int64_new);

  // test string attribute is set correctly in DB
  attr_w string_attr(test_attribute_string);
  status = switch_store::attribute_get(
      first_handle, test_attribute_string, string_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  string_attr.v_get(text_new);
  std::string ctext_new = text_new.text;
  std::cout << ctext_new << " " << ctext << std::endl;
  assert(ctext_new == ctext);

  // test string attribute is set correctly in DB
  attr_w range_attr(test_attribute_range);
  status = switch_store::attribute_get(
      first_handle, test_attribute_range, range_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  range_attr.v_get(range_new);
  std::cout << range_new.min << " " << range.min << std::endl;
  std::cout << range_new.max << " " << range.max << std::endl;
  assert(range_new.min == range.min);
  assert(range_new.max == range.max);

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_enums() {
  std::cout << "**** Tesing objects creation with enum types ****" << std::endl;
  switch_status_t status;

  switch_enum_t e = {.enumdata = 1}, e_new = {.enumdata = 0};
  std::set<attr_w> attrs;
  switch_object_id_t oid = {};
  attrs.insert(attr_w(test_attribute_enum, e));
  status = switch_store::object_create(test_object_2, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_2, first_handle);
  std::cout << first_handle.data << " " << oid.data << std::endl;
  assert(first_handle.data == oid.data);

  // test enum attribute is set correctly in DB
  attr_w enum_attr(test_attribute_enum);
  status =
      switch_store::attribute_get(first_handle, test_attribute_enum, enum_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  enum_attr.v_get(e_new);
  std::cout << e_new.enumdata << " " << e.enumdata << std::endl;
  assert(e_new.enumdata == e.enumdata);

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

#include "arpa/inet.h"
std::stringstream &operator<<(std::stringstream &os,
                              const switch_ip6_t &ipv6_address) {
  static char ipv6_str[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, ipv6_address, ipv6_str, INET6_ADDRSTRLEN);
  os << ipv6_str;
  return os;
}

void test_ip_address_host() {
  std::cout << "**** Tesing objects creation with ip host types ****"
            << std::endl;
  switch_status_t status;

  switch_ip_address_t ip4_addr_host;
  ip4_addr_host.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4;
  ip4_addr_host.ip4 = 168427521;

  std::string ip_str_host = "1234:5678:9abc:def0:4422:1131:1111:2221";
  struct sockaddr_in6 sa6 = {};
  inet_pton(
      AF_INET6, ip_str_host.c_str(), static_cast<void *>(&(sa6.sin6_addr)));
  switch_ip_address_t ip6_addr_host;
  ip6_addr_host.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
  for (size_t i = 0; i < sizeof(struct in6_addr); i++) {
    ip6_addr_host.ip6[i] = sa6.sin6_addr.s6_addr[i];
  }

  std::set<attr_w> v4_attrs;
  switch_object_id_t v4_oid = {};
  v4_attrs.insert(attr_w(test_attribute_ip_addr, ip4_addr_host));
  status = switch_store::object_create(test_object_3, v4_attrs, v4_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_3, first_handle);
  std::cout << "oid " << first_handle.data << " " << v4_oid.data << std::endl;
  assert(first_handle.data == v4_oid.data);

  // test ipv4 addr attribute is set correctly in DB
  {
    switch_ip_address_t ip4_addr_host_new;
    attr_w v4_attr(test_attribute_ip_addr);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_addr, v4_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    v4_attr.v_get(ip4_addr_host_new);
    std::cout << "v4 " << ip4_addr_host_new.ip4 << " " << ip4_addr_host.ip4
              << std::endl;
    assert(ip4_addr_host_new.ip4 == ip4_addr_host.ip4);
    assert(ip4_addr_host_new.addr_family == SWITCH_IP_ADDR_FAMILY_IPV4);
    switch_ip_prefix_t prefix_lpm_new;
    attr_w attr(test_attribute_ip_prefix);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_prefix, attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr.v_get(prefix_lpm_new);
    assert(prefix_lpm_new.addr.ip4 == 0);
    assert(prefix_lpm_new.len == 0);
    assert(prefix_lpm_new.addr.addr_family == SWITCH_IP_ADDR_FAMILY_NONE);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::set<attr_w> v6_attrs;
  switch_object_id_t v6_oid = {};
  v6_attrs.insert(attr_w(test_attribute_ip_addr, ip6_addr_host));
  status = switch_store::object_create(test_object_3, v6_attrs, v6_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  first_handle = {};
  switch_store::object_get_first_handle(test_object_3, first_handle);
  std::cout << "oid " << first_handle.data << " " << v6_oid.data << std::endl;
  assert(first_handle.data == v6_oid.data);

  // test ipv6 addr attribute is set correctly in DB
  {
    switch_ip_address_t ip6_addr_host_new;
    attr_w v6_attr(test_attribute_ip_addr);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_addr, v6_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    v6_attr.v_get(ip6_addr_host_new);
    std::stringstream out;
    out << "v6 ";
    out << ip6_addr_host_new.ip6 << " ";
    out << ip6_addr_host.ip6 << std::endl;
    std::cout << out.str();
    for (int i = 0; i < 16; i++) {
      if (ip6_addr_host_new.ip6[i] != ip6_addr_host.ip6[i]) assert(0);
    }
    assert(ip6_addr_host_new.addr_family == SWITCH_IP_ADDR_FAMILY_IPV6);
    switch_ip_prefix_t prefix_lpm_new;
    attr_w attr(test_attribute_ip_prefix);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_prefix, attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr.v_get(prefix_lpm_new);
    assert(prefix_lpm_new.addr.ip4 == 0);
    assert(prefix_lpm_new.len == 0);
    assert(prefix_lpm_new.addr.addr_family == SWITCH_IP_ADDR_FAMILY_NONE);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_ip_address_lpm() {
  std::cout << "**** Tesing objects creation with ip lpm types ****"
            << std::endl;
  switch_status_t status;

  switch_ip_prefix_t ip4_prefix_lpm;
  ip4_prefix_lpm.len = 16;
  ip4_prefix_lpm.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
  ip4_prefix_lpm.addr.ip4 = 168427520;

  std::string ip_str_lpm = "1234:5678:9abc:def0:4422:1131:0000:0000";
  struct sockaddr_in6 sa6 = {};
  inet_pton(
      AF_INET6, ip_str_lpm.c_str(), static_cast<void *>(&(sa6.sin6_addr)));
  switch_ip_prefix_t ip6_prefix_lpm;
  ip6_prefix_lpm.len = 96;
  ip6_prefix_lpm.addr.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
  for (size_t i = 0; i < sizeof(struct in6_addr); i++) {
    ip6_prefix_lpm.addr.ip6[i] = sa6.sin6_addr.s6_addr[i];
  }

  std::set<attr_w> v4_attrs;
  switch_object_id_t v4_oid = {};
  v4_attrs.insert(attr_w(test_attribute_ip_prefix, ip4_prefix_lpm));
  status = switch_store::object_create(test_object_3, v4_attrs, v4_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_3, first_handle);
  std::cout << first_handle.data << " " << v4_oid.data << std::endl;
  assert(first_handle.data == v4_oid.data);

  // test ipv4 addr attribute is set correctly in DB
  {
    switch_ip_prefix_t ip4_prefix_lpm_new;
    attr_w v4_attr(test_attribute_ip_prefix);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_prefix, v4_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    v4_attr.v_get(ip4_prefix_lpm_new);
    std::cout << ip4_prefix_lpm_new.addr.ip4 << " " << ip4_prefix_lpm.addr.ip4
              << std::endl;
    assert(ip4_prefix_lpm_new.addr.ip4 == ip4_prefix_lpm.addr.ip4);
    assert(ip4_prefix_lpm_new.len == ip4_prefix_lpm.len);
    assert(ip4_prefix_lpm_new.addr.addr_family ==
           ip4_prefix_lpm.addr.addr_family);
    switch_ip_address_t addr_host;
    attr_w attr(test_attribute_ip_addr);
    status =
        switch_store::attribute_get(first_handle, test_attribute_ip_addr, attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr.v_get(addr_host);
    assert(addr_host.addr_family == SWITCH_IP_ADDR_FAMILY_NONE);
    assert(addr_host.ip4 == 0);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::set<attr_w> v6_attrs;
  switch_object_id_t v6_oid = {};
  v6_attrs.insert(attr_w(test_attribute_ip_prefix, ip6_prefix_lpm));
  status = switch_store::object_create(test_object_3, v6_attrs, v6_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  first_handle = {};
  switch_store::object_get_first_handle(test_object_3, first_handle);
  std::cout << first_handle.data << " " << v6_oid.data << std::endl;
  assert(first_handle.data == v6_oid.data);

  // test ipv6 addr attribute is set correctly in DB
  {
    switch_ip_prefix_t ip6_prefix_lpm_new;
    attr_w v6_attr(test_attribute_ip_prefix);
    status = switch_store::attribute_get(
        first_handle, test_attribute_ip_prefix, v6_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    v6_attr.v_get(ip6_prefix_lpm_new);
    std::stringstream out;
    out << ip6_prefix_lpm_new.addr.ip6 << " ";
    out << ip6_prefix_lpm.addr.ip6 << std::endl;
    std::cout << out.str();
    for (int i = 0; i < 16; i++) {
      if (ip6_prefix_lpm_new.addr.ip6[i] != ip6_prefix_lpm.addr.ip6[i])
        assert(0);
    }
    switch_ip_address_t addr_host;
    attr_w attr(test_attribute_ip_addr);
    status =
        switch_store::attribute_get(first_handle, test_attribute_ip_addr, attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    attr.v_get(addr_host);
    assert(addr_host.addr_family == SWITCH_IP_ADDR_FAMILY_NONE);
    assert(addr_host.ip4 == 0);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_mac() {
  std::cout << "**** Tesing objects creation with mac types ****" << std::endl;
  switch_status_t status;

  switch_mac_addr_t mac = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x00}, mac_new = {};
  std::set<attr_w> attrs;
  switch_object_id_t oid = {};
  attrs.insert(attr_w(test_attribute_mac, mac));
  status = switch_store::object_create(test_object_4, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_4, first_handle);
  std::cout << first_handle.data << " " << oid.data << std::endl;
  assert(first_handle.data == oid.data);

  // test enum attribute is set correctly in DB
  attr_w mac_attr(test_attribute_mac);
  status =
      switch_store::attribute_get(first_handle, test_attribute_mac, mac_attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  mac_attr.v_get(mac_new);
  for (int i = 0; i < 6; i++) {
    if (mac.mac[i] != mac_new.mac[i]) assert(0);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

/*
 * Test object graph cycles and joins
 * device -> port1
 * port1  -> device
 * port2  -> device
 *        -> mirror
 * port3  -> device
 * mirror -> device
 *        -> port3
 */
void test_oid() {
  std::cout << "**** Tesing objects creation with oid types ****" << std::endl;
  switch_status_t status;
  uint32_t count = 0;
  std::set<uint64_t> ports;

  // create device
  std::set<attr_w> device_attrs;
  switch_object_id_t device_oid = {};
  status = switch_store::object_create(device, device_attrs, device_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // create ports
  std::set<attr_w> port_attrs;
  switch_object_id_t port1_oid = {}, port2_oid = {}, port3_oid = {};
  port_attrs.insert(attr_w(port_device, device_oid));
  status = switch_store::object_create(port, port_attrs, port1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port1_oid.data);
  status = switch_store::object_create(port, port_attrs, port2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port2_oid.data);
  status = switch_store::object_create(port, port_attrs, port3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port3_oid.data);

  // set port1 as attr of device
  attr_w device_port_attr(device_port, port1_oid);
  status = switch_store::attribute_set(device_oid, device_port_attr);
  assert(status == SWITCH_STATUS_SUCCESS);

  // create mirror obj with port3 as mirror port
  std::set<attr_w> mirror_attrs;
  switch_object_id_t mirror_oid = {};
  mirror_attrs.insert(attr_w(mirror_device, device_oid));
  mirror_attrs.insert(attr_w(mirror_port, port3_oid));
  status = switch_store::object_create(mirror, mirror_attrs, mirror_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // set port2 to use mirror session obj
  attr_w port_mirror_attr(port_mirror, mirror_oid);
  status = switch_store::attribute_set(port2_oid, port_mirror_attr);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_device_handle = {};
  count = 0;
  switch_store::object_get_first_handle(device, first_device_handle);
  assert(first_device_handle.data == device_oid.data);
  // test port attribute is set correctly in DB
  attr_w attr(device_port);
  switch_object_id_t device_port_attr_handle = {};
  status = switch_store::attribute_get(first_device_handle, device_port, attr);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr.v_get(device_port_attr_handle);
  assert(device_port_attr_handle.data == port1_oid.data);

  // now read back mirrors from DB
  switch_object_id_t first_mirror_handle = {};
  count = 0;
  switch_store::object_get_first_handle(mirror, first_mirror_handle);
  assert(first_mirror_handle.data == mirror_oid.data);
  // test port attribute is set correctly in DB
  attr_w attr1(mirror_port);
  switch_object_id_t mirror_port_attr_handle = {};
  status = switch_store::attribute_get(first_mirror_handle, mirror_port, attr1);
  assert(status == SWITCH_STATUS_SUCCESS);
  attr1.v_get(mirror_port_attr_handle);
  assert(mirror_port_attr_handle.data == port3_oid.data);

  // now read back ports from DB
  switch_object_id_t first_port_handle = {};
  count = 0;
  std::vector<switch_object_id_t> next_port_handles;
  switch_store::object_get_first_handle(port, first_port_handle);
  switch_store::object_get_next_handles(
      first_port_handle, 2, next_port_handles, count);
  ports.erase(first_port_handle.data);
  for (auto hdl : next_port_handles) {
    ports.erase(hdl.data);
  }
  assert(ports.size() == 0);

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_graph() {
  std::cout << "**** Tesing object graph ****" << std::endl;
  switch_status_t status;
  uint32_t count = 0;
  std::set<uint64_t> ports;

  // create device
  std::set<attr_w> device_attrs;
  switch_object_id_t device_oid = {};
  status = switch_store::object_create(device, device_attrs, device_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // create ports
  std::set<attr_w> port_attrs;
  switch_object_id_t port1_oid = {}, port2_oid = {}, port3_oid = {};
  port_attrs.insert(attr_w(port_device, device_oid));
  status = switch_store::object_create(port, port_attrs, port1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port1_oid.data);
  status = switch_store::object_create(port, port_attrs, port2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port2_oid.data);
  status = switch_store::object_create(port, port_attrs, port3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  ports.insert(port3_oid.data);

  // set port1 as attr of device
  attr_w device_port_attr(device_port, port1_oid);
  status = switch_store::attribute_set(device_oid, device_port_attr);
  assert(status == SWITCH_STATUS_SUCCESS);

  // create mirror obj with port3 as mirror port
  std::set<attr_w> mirror_attrs;
  switch_object_id_t mirror_oid = {};
  mirror_attrs.insert(attr_w(mirror_device, device_oid));
  mirror_attrs.insert(attr_w(mirror_port, port3_oid));
  status = switch_store::object_create(mirror, mirror_attrs, mirror_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // set port2 to use mirror session obj
  attr_w port_mirror_attr(port_mirror, mirror_oid);
  status = switch_store::attribute_set(port2_oid, port_mirror_attr);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_device_handle = {};
  count = 0;
  switch_store::object_get_first_handle(device, first_device_handle);
  assert(first_device_handle.data == device_oid.data);

  // now read back mirrors from DB
  switch_object_id_t first_mirror_handle = {};
  count = 0;
  switch_store::object_get_first_handle(mirror, first_mirror_handle);
  assert(first_mirror_handle.data == mirror_oid.data);

  // now read back ports from DB
  switch_object_id_t first_port_handle = {};
  count = 0;
  std::vector<switch_object_id_t> next_port_handles;
  switch_store::object_get_first_handle(port, first_port_handle);
  switch_store::object_get_next_handles(
      first_port_handle, 2, next_port_handles, count);
  assert(next_port_handles.size() == 2);

  // verify object graph
  /*
   * device -> port1
   * port1  -> device
   * port2  -> device
   *        -> mirror
   * port3  -> device
   * mirror -> device
   *        -> port3
   */
  std::set<switch_object_id_t> mirror_handles, port_handles, device_handles;
  switch_store::referencing_set_get(first_device_handle, port, port_handles);
  assert(port_handles.size() == 3);
  for (const auto t_port : port_handles) {
    assert(t_port.data == port1_oid.data || t_port.data == port2_oid.data ||
           t_port.data == port3_oid.data);
  }
  switch_store::referencing_set_get(
      first_device_handle, mirror, mirror_handles);
  assert(mirror_handles.size() == 1);
  for (const auto t_mirror : mirror_handles) {
    assert(t_mirror.data == mirror_oid.data);
  }
  switch_store::referencing_set_get(port1_oid, device, device_handles);
  assert(device_handles.size() == 1);
  for (const auto t_device : device_handles) {
    assert(t_device.data == device_oid.data);
  }
  port_handles.clear();
  switch_store::referencing_set_get(mirror_oid, port, port_handles);
  assert(port_handles.size() == 1);
  for (const auto t_port : port_handles) {
    assert(t_port.data == port2_oid.data);
  }
  mirror_handles.clear();
  switch_store::referencing_set_get(port3_oid, mirror, mirror_handles);
  assert(mirror_handles.size() == 1);
  for (const auto t_mirror : mirror_handles) {
    assert(t_mirror.data == mirror_oid.data);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

class test_auto_81_obj : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_81;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_81_ATTR_TEST_AUTO_81_PARENT;

 public:
  test_auto_81_obj(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
    std::cout << "In test_auto_81_obj" << std::endl;
  }
};

class test_auto_811_obj : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_811;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_811_ATTR_TEST_AUTO_811_PARENT;

 public:
  test_auto_811_obj(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
    std::cout << "In test_auto_811_obj" << std::endl;
  }
};

class test_auto_812_obj : public auto_object {
 private:
  static const switch_object_type_t auto_ot = SWITCH_OBJECT_TYPE_TEST_AUTO_812;
  static const switch_attr_id_t parent_attr_id =
      SWITCH_TEST_AUTO_812_ATTR_TEST_AUTO_812_PARENT;

 public:
  test_auto_812_obj(const switch_object_id_t parent, switch_status_t &status)
      : auto_object(auto_ot, parent_attr_id, parent) {
    status = SWITCH_STATUS_SUCCESS;
    std::cout << "In test_auto_812_obj" << std::endl;
  }
};

void test_auto_objects() {
  std::cout << "**** Tesing objects creation with children objects ****"
            << std::endl;
  switch_status_t status;

  REGISTER_OBJECT(test_auto_81_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_81);
  REGISTER_OBJECT(test_auto_811_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_811);
  REGISTER_OBJECT(test_auto_812_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_812);
  std::set<attr_w> attrs;
  switch_object_id_t oid = {}, oid_81, oid_811, oid_812;
  status = switch_store::object_create(test_object_8, attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  find_auto_oid(oid, SWITCH_OBJECT_TYPE_TEST_AUTO_81, oid_81);
  find_auto_oid(oid, SWITCH_OBJECT_TYPE_TEST_AUTO_811, oid_811);
  find_auto_oid(oid, SWITCH_OBJECT_TYPE_TEST_AUTO_812, oid_812);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  REGISTER_OBJECT(test_auto_81_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_81);
  REGISTER_OBJECT(test_auto_811_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_811);
  REGISTER_OBJECT(test_auto_812_obj, SWITCH_OBJECT_TYPE_TEST_AUTO_812);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle, oid_81_new, oid_811_new, oid_812_new;
  switch_store::object_get_first_handle(test_object_8, first_handle);
  std::cout << first_handle.data << " " << oid.data << std::endl;
  assert(first_handle.data == oid.data);

  find_auto_oid(first_handle, SWITCH_OBJECT_TYPE_TEST_AUTO_81, oid_81_new);
  find_auto_oid(first_handle, SWITCH_OBJECT_TYPE_TEST_AUTO_811, oid_811_new);
  find_auto_oid(first_handle, SWITCH_OBJECT_TYPE_TEST_AUTO_812, oid_812_new);
  assert(oid_81.data == oid_81_new.data);
  assert(oid_811.data == oid_811_new.data);
  assert(oid_812.data == oid_812_new.data);

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_lists() {
  std::cout << "**** Tesing objects creation with list types ****" << std::endl;
  switch_status_t status;

  std::set<attr_w> attrs;
  switch_object_id_t oid1_5 = {}, oid2_5 = {}, oid3_5 = {}, oid4_5 = {};
  status = switch_store::object_create(test_object_5, attrs, oid1_5);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_5, attrs, oid2_5);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_5, attrs, oid3_5);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_5, attrs, oid4_5);
  assert(status == SWITCH_STATUS_SUCCESS);

  std::set<attr_w> attrs_list;
  switch_object_id_t oid = {};
  uint64_t val = 1111;
  attr_w oid_list(test_attribute_list_oid);
  std::vector<switch_object_id_t> oids = {oid1_5, oid2_5, oid3_5, oid4_5};
  oid_list.v_set(oids);
  attrs_list.insert(oid_list);
  attr_w uint64_list(test_attribute_list_uint64);
  std::vector<uint64_t> u64s = {val, val * 2, val * 3};
  uint64_list.v_set(u64s);
  attrs_list.insert(uint64_list);
  status = switch_store::object_create(test_object_list, attrs_list, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  // now read back device from DB
  switch_object_id_t first_handle = {};
  switch_store::object_get_first_handle(test_object_list, first_handle);
  std::cout << first_handle.data << " " << oid.data << std::endl;
  assert(first_handle.data == oid.data);

  attr_w new_oid_list(test_attribute_list_oid);
  status = switch_store::attribute_get(
      first_handle, test_attribute_list_oid, new_oid_list);
  std::vector<switch_object_id_t> new_oids;
  new_oid_list.v_get(new_oids);
  assert(4 == new_oids.size());
  for (auto noid : new_oids) {
    assert(noid.data == oid1_5.data || noid.data == oid2_5.data ||
           noid.data == oid3_5.data || noid.data == oid4_5.data);
  }

  attr_w new_u64_list(test_attribute_list_uint64);
  status = switch_store::attribute_get(
      first_handle, test_attribute_list_uint64, new_u64_list);
  std::vector<uint64_t> new_u64s;
  new_u64_list.v_get(new_u64s);
  assert(3 == new_u64s.size());
  for (auto u64 : new_u64s) {
    assert(u64 == 1111 || u64 == 2222 || u64 == 3333);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_key_groups() {
  std::cout << "**** Testing key groups ****" << std::endl;
  switch_status_t status;

  /* create key attribute objects */
  uint64_t key1 = 1;
  uint64_t key2 = 2;
  uint64_t key3 = 3;
  const std::set<attr_w> key_1_attrs{attr_w(key_1_attr, key1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_2_attr, key2)};
  const std::set<attr_w> key_3_attrs{attr_w(key_3_attr, key3)};
  switch_object_id_t key_1_oid = {}, key_2_oid = {}, key_3_oid = {};
  status = switch_store::object_create(test_key_1, key_1_attrs, key_1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_key_2, key_2_attrs, key_2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_key_3, key_3_attrs, key_3_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  /* create objects using above keys */
  switch_object_id_t oid_1 = {};
  const std::set<attr_w> oid_1_attrs{attr_w(obj3_key1, key_1_oid),
                                     attr_w(obj3_key2, key_2_oid)};
  status = switch_store::object_create(test_object_9, oid_1_attrs, oid_1);
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_object_id_t oid_2 = {};
  const std::set<attr_w> oid_2_attrs{attr_w(obj3_key2, key_2_oid),
                                     attr_w(obj3_key3, key_3_oid)};
  status = switch_store::object_create(test_object_9, oid_2_attrs, oid_2);
  assert(status == SWITCH_STATUS_SUCCESS);

  uint32_t test_key1 = 100;
  const std::vector<uint32_t> test_key_u32_1 = {200, 300};
  const std::set<attr_w> key_u32_list_attrs{
      attr_w(key_uint32_u32_attr, test_key1),
      attr_w(key_list_u32_attr, test_key_u32_1)};
  switch_object_id_t u321_oid = {};
  status = switch_store::object_create(
      test_object_u32, key_u32_list_attrs, u321_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  /* validate basic get */
  {
    switch_object_id_t oid_out1 = {0};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_1_attrs, oid_out1);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out1.data == oid_1.data);
    switch_object_id_t oid_out2 = {0};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_2_attrs, oid_out2);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out2.data == oid_2.data);
    switch_object_id_t oid_out3 = {0};
    const std::set<attr_w> oid_attrs{attr_w(obj3_key3, key_3_oid),
                                     attr_w(obj3_key2, key_2_oid)};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_attrs, oid_out3);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out3.data == oid_2.data);

    switch_object_id_t u321_oid_2;
    status = switch_store::object_id_get_wkey(
        test_object_u32, key_u32_list_attrs, u321_oid_2);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(u321_oid.data == u321_oid_2.data);
  }

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  /* validate basic get again */
  {
    switch_object_id_t oid_out1 = {0};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_1_attrs, oid_out1);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out1.data == oid_1.data);
    switch_object_id_t oid_out2 = {0};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_2_attrs, oid_out2);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out2.data == oid_2.data);
    switch_object_id_t oid_out3 = {0};
    const std::set<attr_w> oid_attrs{attr_w(obj3_key3, key_3_oid),
                                     attr_w(obj3_key2, key_2_oid)};
    status =
        switch_store::object_id_get_wkey(test_object_9, oid_attrs, oid_out3);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(oid_out3.data == oid_2.data);

    switch_object_id_t u321_oid_2;
    status = switch_store::object_id_get_wkey(
        test_object_u32, key_u32_list_attrs, u321_oid_2);
    assert(status == SWITCH_STATUS_SUCCESS);
    assert(u321_oid.data == u321_oid_2.data);
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_membership() {
  std::cout << "**** Testing membership ****" << std::endl;
  switch_status_t status;
  std::set<attr_w> no_attrs;
  switch_object_id_t oid = {}, moid1 = {}, moid2 = {}, moid3 = {};

  status = switch_store::object_create(test_object_10, no_attrs, oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  const std::set<attr_w> attr1{attr_w(group_oid, oid)};
  const std::set<attr_w> attr2{attr_w(group_oid, oid)};
  const std::set<attr_w> attr3{attr_w(group_oid, oid)};

  status = switch_store::object_create(member_ot, attr1, moid1);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_create(member_ot, attr2, moid2);
  assert(status == SWITCH_STATUS_SUCCESS);

  status = switch_store::object_create(member_ot, attr3, moid3);
  assert(status == SWITCH_STATUS_SUCCESS);

  {
    attr_w get_attr(mem_list);
    std::vector<switch_object_id_t> moids;
    status = switch_store::attribute_get(oid, mem_list, get_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    moids.clear();
    get_attr.v_get(moids);
    assert(moids.size() == 3);
    for (auto temp_oid : moids) {
      std::cout << std::hex << temp_oid.data << std::endl;
    }
  }

  // simulate warm init
  switch_store::object_info_dump("/tmp/db.txt");
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, true, "/tmp/db.txt");
  assert(status == SWITCH_STATUS_SUCCESS);
  switch_store::object_replay(true);

  {
    attr_w get_attr(mem_list);
    std::vector<switch_object_id_t> moids;
    status = switch_store::attribute_get(oid, mem_list, get_attr);
    assert(status == SWITCH_STATUS_SUCCESS);
    moids.clear();
    get_attr.v_get(moids);
    assert(moids.size() == 3);
    for (auto temp_oid : moids) {
      std::cout << std::hex << temp_oid.data << std::endl;
    }
  }

  // simulate cold init
  switch_store::object_info_clean();
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
}

int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
  init_objects();

  test_simple_types();
  test_enums();
  test_ip_address_host();
  test_ip_address_lpm();
  test_mac();
  test_oid();
  test_lists();
  test_auto_objects();
  test_object_graph();
  test_object_key_groups();
  test_membership();

  printf("\n\nAll tests passed!\n");
  return 0;
}
