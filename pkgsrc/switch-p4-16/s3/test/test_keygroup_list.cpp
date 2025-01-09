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
#include "gen-model/test_keygroup_list.h"
#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "s3/smi.h"

using namespace smi;

static ModelInfo *model_info = NULL;
static switch_object_type_t test_object_bool;
const ObjectInfo *test_object_keygroup_list_bool_info;
static switch_attr_id_t key_uint32_bool_attr;
static switch_attr_id_t key_list_bool_attr;

static switch_object_type_t test_object_u32;
const ObjectInfo *test_object_keygroup_list_u32_info;
static switch_attr_id_t key_uint32_u32_attr;
static switch_attr_id_t key_list_u32_attr;

static switch_object_type_t test_object_u16;
const ObjectInfo *test_object_keygroup_list_u16_info;
static switch_attr_id_t key_uint32_u16_attr;
static switch_attr_id_t key_list_u16_attr;

static switch_object_type_t test_object_u8;
const ObjectInfo *test_object_keygroup_list_u8_info;
static switch_attr_id_t key_uint32_u8_attr;
static switch_attr_id_t key_list_u8_attr;

static switch_object_type_t test_object_u64;
const ObjectInfo *test_object_keygroup_list_u64_info;
static switch_attr_id_t key_uint32_u64_attr;
static switch_attr_id_t key_list_u64_attr;

static switch_object_type_t test_object_mac;
const ObjectInfo *test_object_keygroup_list_mac_info;
static switch_attr_id_t key_uint32_mac_attr;
static switch_attr_id_t key_list_mac_attr;

static switch_object_type_t test_object_ip_address;
const ObjectInfo *test_object_keygroup_list_ip_address_info;
static switch_attr_id_t key_uint32_ip_address_attr;
static switch_attr_id_t key_list_ip_address_attr;

static switch_object_type_t test_object_ip_prefix;
const ObjectInfo *test_object_keygroup_list_ip_prefix_info;
static switch_attr_id_t key_uint32_ip_prefix_attr;
static switch_attr_id_t key_list_ip_prefix_attr;

void init_objects() {
  test_object_keygroup_list_bool_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_bool");
  test_object_bool = test_object_keygroup_list_bool_info->object_type;
  assert(test_object_bool != 0);
  key_uint32_bool_attr =
      test_object_keygroup_list_bool_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_bool_attr != 0);
  key_list_bool_attr =
      test_object_keygroup_list_bool_info->get_attr_id_from_name(
          "test_attribute_list");
  assert(key_list_bool_attr != 0);

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

  test_object_keygroup_list_u16_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_uint16");
  test_object_u16 = test_object_keygroup_list_u16_info->object_type;
  assert(test_object_u16 != 0);
  key_uint32_u16_attr =
      test_object_keygroup_list_u16_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_u16_attr != 0);
  key_list_u16_attr = test_object_keygroup_list_u16_info->get_attr_id_from_name(
      "test_attribute_list");
  assert(key_list_u16_attr != 0);

  test_object_keygroup_list_u8_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_uint8");
  test_object_u8 = test_object_keygroup_list_u8_info->object_type;
  assert(test_object_u8 != 0);
  key_uint32_u8_attr = test_object_keygroup_list_u8_info->get_attr_id_from_name(
      "test_attribute_uint32");
  assert(key_uint32_u8_attr != 0);
  key_list_u8_attr = test_object_keygroup_list_u8_info->get_attr_id_from_name(
      "test_attribute_list");
  assert(key_list_u8_attr != 0);

  test_object_keygroup_list_u64_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_uint64");
  test_object_u64 = test_object_keygroup_list_u64_info->object_type;
  assert(test_object_u64 != 0);
  key_uint32_u64_attr =
      test_object_keygroup_list_u64_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_u64_attr != 0);
  key_list_u64_attr = test_object_keygroup_list_u64_info->get_attr_id_from_name(
      "test_attribute_list");
  assert(key_list_u64_attr != 0);

  test_object_keygroup_list_mac_info =
      model_info->get_object_info_from_name("test_object_keygroup_list_mac");
  test_object_mac = test_object_keygroup_list_mac_info->object_type;
  assert(test_object_mac != 0);
  key_uint32_mac_attr =
      test_object_keygroup_list_mac_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_mac_attr != 0);
  key_list_mac_attr = test_object_keygroup_list_mac_info->get_attr_id_from_name(
      "test_attribute_list");
  assert(key_list_mac_attr != 0);

  test_object_keygroup_list_ip_address_info =
      model_info->get_object_info_from_name(
          "test_object_keygroup_list_ip_address");
  test_object_ip_address =
      test_object_keygroup_list_ip_address_info->object_type;
  assert(test_object_ip_address != 0);
  key_uint32_ip_address_attr =
      test_object_keygroup_list_ip_address_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_ip_address_attr != 0);
  key_list_ip_address_attr =
      test_object_keygroup_list_ip_address_info->get_attr_id_from_name(
          "test_attribute_list");
  assert(key_list_ip_address_attr != 0);

  test_object_keygroup_list_ip_prefix_info =
      model_info->get_object_info_from_name(
          "test_object_keygroup_list_ip_prefix");
  test_object_ip_prefix = test_object_keygroup_list_ip_prefix_info->object_type;
  assert(test_object_ip_prefix != 0);
  key_uint32_ip_prefix_attr =
      test_object_keygroup_list_ip_prefix_info->get_attr_id_from_name(
          "test_attribute_uint32");
  assert(key_uint32_ip_prefix_attr != 0);
  key_list_ip_prefix_attr =
      test_object_keygroup_list_ip_prefix_info->get_attr_id_from_name(
          "test_attribute_list");
  assert(key_list_ip_prefix_attr != 0);
}

void test_object_list_bool() {
  switch_status_t status;
  uint32_t test_key = 100;
  const std::vector<bool> test_key_bool_1 = {true};
  const std::vector<bool> test_key_bool_2 = {true, true};
  const std::set<attr_w> key_1_attrs{
      attr_w(key_uint32_bool_attr, test_key),
      attr_w(key_list_bool_attr, test_key_bool_1)};
  const std::set<attr_w> key_2_attrs{
      attr_w(key_uint32_bool_attr, test_key),
      attr_w(key_list_bool_attr, test_key_bool_2)};
  switch_object_id_t bool1_oid = {}, bool2_oid = {};
  std::cout << "**** Tesing objects with bool list keygroups****" << std::endl;
  // Create two objects with same uint32 key, different bool list keys.
  status =
      switch_store::object_create(test_object_bool, key_1_attrs, bool1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status =
      switch_store::object_create(test_object_bool, key_2_attrs, bool2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of bool and uint32 key
  switch_object_id_t bool3_oid = {};
  status =
      switch_store::object_create(test_object_bool, key_2_attrs, bool3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(bool1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(bool2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_u32() {
  switch_status_t status;
  uint32_t test_key = 100;
  const std::vector<uint32_t> test_key_u32_1 = {200};
  const std::vector<uint32_t> test_key_u32_2 = {200, 300};
  const std::set<attr_w> key_1_attrs{attr_w(key_uint32_u32_attr, test_key),
                                     attr_w(key_list_u32_attr, test_key_u32_1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_uint32_u32_attr, test_key),
                                     attr_w(key_list_u32_attr, test_key_u32_2)};
  switch_object_id_t u321_oid = {}, u322_oid = {};
  std::cout << "**** Tesing objects with uint32_t list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different u32 list keys.
  status = switch_store::object_create(test_object_u32, key_1_attrs, u321_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_u32, key_2_attrs, u322_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of u32 and uint32 key
  switch_object_id_t u323_oid = {};
  status = switch_store::object_create(test_object_u32, key_2_attrs, u323_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(u321_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(u322_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_u16() {
  switch_status_t status;
  uint32_t test_key = 100;
  const std::vector<uint16_t> test_key_u16_1 = {200};
  const std::vector<uint16_t> test_key_u16_2 = {200, 300};
  const std::set<attr_w> key_1_attrs{attr_w(key_uint32_u16_attr, test_key),
                                     attr_w(key_list_u16_attr, test_key_u16_1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_uint32_u16_attr, test_key),
                                     attr_w(key_list_u16_attr, test_key_u16_2)};
  switch_object_id_t u161_oid = {}, u162_oid = {};
  std::cout << "**** Tesing objects with uint16_t list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different u16 list keys.
  status = switch_store::object_create(test_object_u16, key_1_attrs, u161_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_u16, key_2_attrs, u162_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of u16 and uint32 key
  switch_object_id_t u163_oid = {};
  status = switch_store::object_create(test_object_u16, key_2_attrs, u163_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(u161_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(u162_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_u8() {
  switch_status_t status;
  uint32_t test_key = 100;
  const std::vector<uint8_t> test_key_u8_1 = {100};
  const std::vector<uint8_t> test_key_u8_2 = {100, 200};
  const std::set<attr_w> key_1_attrs{attr_w(key_uint32_u8_attr, test_key),
                                     attr_w(key_list_u8_attr, test_key_u8_1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_uint32_u8_attr, test_key),
                                     attr_w(key_list_u8_attr, test_key_u8_2)};
  switch_object_id_t u81_oid = {}, u82_oid = {};
  std::cout << "**** Tesing objects with uint8_t list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different u8 list keys.
  status = switch_store::object_create(test_object_u8, key_1_attrs, u81_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_u8, key_2_attrs, u82_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of u8 and uint32 key
  switch_object_id_t u83_oid = {};
  status = switch_store::object_create(test_object_u8, key_2_attrs, u83_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(u81_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(u82_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_u64() {
  switch_status_t status;
  uint32_t test_key = 100;
  const std::vector<uint64_t> test_key_u64_1 = {100};
  const std::vector<uint64_t> test_key_u64_2 = {100, 200};
  const std::set<attr_w> key_1_attrs{attr_w(key_uint32_u64_attr, test_key),
                                     attr_w(key_list_u64_attr, test_key_u64_1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_uint32_u64_attr, test_key),
                                     attr_w(key_list_u64_attr, test_key_u64_2)};
  switch_object_id_t u641_oid = {}, u642_oid = {};
  std::cout << "**** Tesing objects with uint64_t list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different u64 list keys.
  status = switch_store::object_create(test_object_u64, key_1_attrs, u641_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_u64, key_2_attrs, u642_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of u64 and uint32 key
  switch_object_id_t u643_oid = {};
  status = switch_store::object_create(test_object_u64, key_2_attrs, u643_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(u641_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(u642_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_mac() {
  switch_status_t status;
  uint32_t test_key = 100;
  switch_mac_addr_t mac1 = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
  switch_mac_addr_t mac2 = {0x00, 0x01, 0x02, 0x03, 0x04, 0x06};
  const std::vector<switch_mac_addr_t> test_key_mac_1 = {mac1};
  const std::vector<switch_mac_addr_t> test_key_mac_2 = {mac1, mac2};
  const std::set<attr_w> key_1_attrs{attr_w(key_uint32_mac_attr, test_key),
                                     attr_w(key_list_mac_attr, test_key_mac_1)};
  const std::set<attr_w> key_2_attrs{attr_w(key_uint32_mac_attr, test_key),
                                     attr_w(key_list_mac_attr, test_key_mac_2)};
  switch_object_id_t mac1_oid = {}, mac2_oid = {};
  std::cout << "**** Tesing objects with MAC list keygroups****" << std::endl;
  // Create two objects with same uint32 key, different mac list keys.
  status = switch_store::object_create(test_object_mac, key_1_attrs, mac1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(test_object_mac, key_2_attrs, mac2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of mac and uint32 key
  switch_object_id_t mac3_oid = {};
  status = switch_store::object_create(test_object_mac, key_2_attrs, mac3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(mac1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(mac2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_ip4_address() {
  switch_status_t status;
  uint32_t test_key = 100;
  switch_ip_address_t ip_address1;
  switch_ip_address_t ip_address2;

  ip_address1.addr_family = ip_address2.addr_family =
      SWITCH_IP_ADDR_FAMILY_IPV4;
  ip_address1.ip4 = 168430081;
  ip_address1.ip4 = 168430082;
  const std::vector<switch_ip_address_t> test_key_ip_address_1 = {ip_address1};
  const std::vector<switch_ip_address_t> test_key_ip_address_2 = {ip_address1,
                                                                  ip_address2};
  const std::set<attr_w> key_1_attrs{
      attr_w(key_uint32_ip_address_attr, test_key),
      attr_w(key_list_ip_address_attr, test_key_ip_address_1)};
  const std::set<attr_w> key_2_attrs{
      attr_w(key_uint32_ip_address_attr, test_key),
      attr_w(key_list_ip_address_attr, test_key_ip_address_2)};
  switch_object_id_t ip_address1_oid = {}, ip_address2_oid = {};
  std::cout << "**** Tesing objects with ip_address list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different ip_address list keys.
  status = switch_store::object_create(
      test_object_ip_address, key_1_attrs, ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(
      test_object_ip_address, key_2_attrs, ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of ip_address and uint32 key
  switch_object_id_t ip_address3_oid = {};
  status = switch_store::object_create(
      test_object_ip_address, key_2_attrs, ip_address3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_ip6_address() {
  switch_status_t status;
  uint32_t test_key = 100;
  switch_ip_address_t ipaddress1;
  switch_ip_address_t ipaddress2;

  ipaddress1.addr_family = ipaddress2.addr_family = SWITCH_IP_ADDR_FAMILY_IPV6;
  for (int i = 0; i < IPV6_LEN; i++) {
    ipaddress1.ip6[i] = i + 1;
  }
  for (int i = 0; i < IPV6_LEN; i++) {
    ipaddress2.ip6[i] = i + 12;
  }
  const std::vector<switch_ip_address_t> test_key_ip_address_1 = {ipaddress1};
  const std::vector<switch_ip_address_t> test_key_ip_address_2 = {ipaddress1,
                                                                  ipaddress2};
  const std::set<attr_w> key_1_attrs{
      attr_w(key_uint32_ip_address_attr, test_key),
      attr_w(key_list_ip_address_attr, test_key_ip_address_1)};
  const std::set<attr_w> key_2_attrs{
      attr_w(key_uint32_ip_address_attr, test_key),
      attr_w(key_list_ip_address_attr, test_key_ip_address_2)};
  switch_object_id_t ip_address1_oid = {}, ip_address2_oid = {};
  std::cout << "**** Tesing objects with ip_address list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different ip_address list keys.
  status = switch_store::object_create(
      test_object_ip_address, key_1_attrs, ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(
      test_object_ip_address, key_2_attrs, ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of ip_address and uint32 key
  switch_object_id_t ip_address3_oid = {};
  status = switch_store::object_create(
      test_object_ip_address, key_2_attrs, ip_address3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_ip4_prefix() {
  switch_status_t status;
  uint32_t test_key = 100;
  switch_ip_prefix_t ip_address1;
  switch_ip_prefix_t ip_address2;

  ip_address1.len = ip_address2.len = 24;
  ip_address1.addr.addr_family = ip_address2.addr.addr_family =
      SWITCH_IP_ADDR_FAMILY_IPV4;
  ip_address1.addr.ip4 = 168430081;
  ip_address1.addr.ip4 = 185273089;
  const std::vector<switch_ip_prefix_t> test_key_ip_address_1 = {ip_address1};
  const std::vector<switch_ip_prefix_t> test_key_ip_address_2 = {ip_address1,
                                                                 ip_address2};
  const std::set<attr_w> key_1_attrs{
      attr_w(key_uint32_ip_prefix_attr, test_key),
      attr_w(key_list_ip_prefix_attr, test_key_ip_address_1)};
  const std::set<attr_w> key_2_attrs{
      attr_w(key_uint32_ip_prefix_attr, test_key),
      attr_w(key_list_ip_prefix_attr, test_key_ip_address_2)};
  switch_object_id_t ip_address1_oid = {}, ip_address2_oid = {};
  std::cout << "**** Tesing objects with ip prefixes list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different ip_address list keys.
  status = switch_store::object_create(
      test_object_ip_prefix, key_1_attrs, ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(
      test_object_ip_prefix, key_2_attrs, ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of ip_prefix and uint32 key
  switch_object_id_t ip_address3_oid = {};
  status = switch_store::object_create(
      test_object_ip_prefix, key_2_attrs, ip_address3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}

void test_object_list_ip6_prefix() {
  switch_status_t status;
  uint32_t test_key = 100;
  switch_ip_prefix_t ipaddress1;
  switch_ip_prefix_t ipaddress2;

  ipaddress1.len = ipaddress2.len = 128;
  ipaddress1.addr.addr_family = ipaddress2.addr.addr_family =
      SWITCH_IP_ADDR_FAMILY_IPV6;
  for (int i = 0; i < IPV6_LEN; i++) {
    ipaddress1.addr.ip6[i] = i + 1;
  }
  for (int i = 0; i < IPV6_LEN; i++) {
    ipaddress2.addr.ip6[i] = i + 12;
  }
  const std::vector<switch_ip_prefix_t> test_key_ip_address_1 = {ipaddress1};
  const std::vector<switch_ip_prefix_t> test_key_ip_address_2 = {ipaddress1,
                                                                 ipaddress2};
  const std::set<attr_w> key_1_attrs{
      attr_w(key_uint32_ip_prefix_attr, test_key),
      attr_w(key_list_ip_prefix_attr, test_key_ip_address_1)};
  const std::set<attr_w> key_2_attrs{
      attr_w(key_uint32_ip_prefix_attr, test_key),
      attr_w(key_list_ip_prefix_attr, test_key_ip_address_2)};
  switch_object_id_t ip_address1_oid = {}, ip_address2_oid = {};
  std::cout << "**** Tesing objects with ip_prefix list keygroups****"
            << std::endl;
  // Create two objects with same uint32 key, different ip_address list keys.
  status = switch_store::object_create(
      test_object_ip_prefix, key_1_attrs, ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_create(
      test_object_ip_prefix, key_2_attrs, ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);

  // Try to create object with same list of ip_prefix and uint32 key
  switch_object_id_t ip_address3_oid = {};
  status = switch_store::object_create(
      test_object_ip_prefix, key_2_attrs, ip_address3_oid);
  assert(status == SWITCH_STATUS_ITEM_ALREADY_EXISTS);

  // Delete all objects
  status = switch_store::object_delete(ip_address1_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
  status = switch_store::object_delete(ip_address2_oid);
  assert(status == SWITCH_STATUS_SUCCESS);
}
int main(void) {
  switch_status_t status = SWITCH_STATUS_SUCCESS;
  const char *test_model_name = TESTDATADIR "/test/test_keygroup_list.json";
  status = switch_store::object_info_init(test_model_name, false, NULL);
  assert(status == SWITCH_STATUS_SUCCESS);
  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  init_objects();
  test_object_list_bool();
  test_object_list_u32();
  test_object_list_u16();
  test_object_list_u8();
  test_object_list_u64();
  test_object_list_mac();
  test_object_list_ip4_address();
  test_object_list_ip6_address();
  test_object_list_ip4_prefix();
  test_object_list_ip6_prefix();
}
