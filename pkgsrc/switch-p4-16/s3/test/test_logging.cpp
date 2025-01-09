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
#include <iomanip>
#include <sstream>
#include <vector>
#include <set>
#include <regex>

#include "gen-model/test_model.h"
#include "bf_switch/bf_switch_types.h"

#include "s3/attribute.h"
#include "s3/attribute_util.h"
#include "bf_switch/bf_event.h"
#include "s3/switch_store.h"
#include "s3/factory.h"
#include "s3/event.h"
#include "../log.h"

using namespace smi;
using namespace std;

ModelInfo *model_info = NULL;
using ::smi::logging::switch_log;

static std::map<smi::attr_w, const char *> attributes_map = ([] {
  std::map<smi::attr_w, const char *> map{
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_BOOL, true),
       "test_attribute_bool=true"},
      {smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_BOOL,
                   std::vector<bool>{true, false, true}),
       "test_attribute_list_bool=[true, false, true]"},
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_UINT8,
                   (uint8_t)0xaa),
       "test_attribute_uint8=170"},
      {smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_UINT8,
                   std::vector<uint8_t>{0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}),
       "test_attribute_list_uint8=[170, 187, 204, 221, 238, 255]"},
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_UINT16,
                   (uint16_t)0xaaaa),
       "test_attribute_uint16=43690"},
      {smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_UINT16,
                   std::vector<uint16_t>{
                       0xaaaa, 0xbbbb, 0xcccc, 0xdddd, 0xeeee, 0xffff}),
       "test_attribute_list_uint16=[43690, 48059, 52428, 56797, "
       "61166, 65535]"},
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_UINT32, 0xaaaaaaaa),
       "test_attribute_uint32=2863311530"},
      {smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_UINT32,
                   std::vector<uint32_t>{0xaaaaaaaa,
                                         0xbbbbbbbb,
                                         0xcccccccc,
                                         0xdddddddd,
                                         0xeeeeeeee,
                                         0xffffffff}),
       "test_attribute_list_uint32=[2863311530, 3149642683, "
       "3435973836, 3722304989, 4008636142, 4294967295]"},
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_INT64,
                   0x7fffffffffffffff),
       "test_attribute_int64=9223372036854775807"},
      {smi::attr_w(
           SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_INT64,
           std::vector<int64_t>{
               0x1111111111111111, 0x7aaaaaaaaaaaaaaa, 0x7fffffffffffffff}),
       "test_attribute_list_int64=[1229782938247303441, "
       "8839064868652493482, 9223372036854775807]"},
      {smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_UINT64,
                   0xffffffffffffffff),
       "test_attribute_uint64=18446744073709551615"},
      {smi::attr_w(
           SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_UINT64,
           std::vector<uint64_t>{
               0x1111111111111111, 0xaaaaaaaaaaaaaaaa, 0xffffffffffffffff}),
       "test_attribute_list_uint64=[1229782938247303441, "
       "12297829382473034410, 18446744073709551615]"},

  };
  switch_string_t text1;
  strcpy(text1.text, "this is a test 1");
  switch_string_t text2;
  strcpy(text2.text, "this is a test 2");
  switch_string_t text3;
  strcpy(text3.text, "this is a test 3");

  switch_object_id_t oid1{.data = 0xf0000000000000ff};
  switch_object_id_t oid2{.data = 0xe0000000000000ee};
  switch_object_id_t oid3{.data = 0xc0000000000000cc};

  switch_mac_addr_t mac1{.mac = {1, 2, 3, 4, 5, 6}};
  switch_mac_addr_t mac2{.mac = {7, 8, 9, 10, 11, 12}};
  switch_mac_addr_t mac3{.mac = {13, 14, 15, 16, 17, 18}};

  switch_ip_address_t ip4_1{.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
                            .ip4 = 0x01020304};
  switch_ip_address_t ip4_2{.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
                            .ip4 = 0x05060708};
  switch_ip_address_t ip4_3{.addr_family = SWITCH_IP_ADDR_FAMILY_IPV4,
                            .ip4 = 0x090a0b0c};

  switch_ip_address_t ip6_1{
      .addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
      .ip6 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}};
  switch_ip_address_t ip6_2{
      .addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
      .ip6 = {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}};
  switch_ip_address_t ip6_3{
      .addr_family = SWITCH_IP_ADDR_FAMILY_IPV6,
      .ip6 = {33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48}};

  switch_ip_prefix_t ip4_prefix1{.len = 0xff, .addr = ip4_1};
  switch_ip_prefix_t ip4_prefix2{.len = 0xee, .addr = ip4_2};
  switch_ip_prefix_t ip4_prefix3{.len = 0xcc, .addr = ip4_3};

  switch_ip_prefix_t ip6_prefix1{.len = 0xff, .addr = ip6_1};
  switch_ip_prefix_t ip6_prefix2{.len = 0xee, .addr = ip6_2};
  switch_ip_prefix_t ip6_prefix3{.len = 0xcc, .addr = ip6_3};

  switch_enum_t enum1{.enumdata = 1};
  switch_enum_t enum2{.enumdata = 2};
  switch_enum_t enum3{.enumdata = 3};

  switch_range_t range1{.min = 0, .max = 10};

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_STRING,
                  std::vector<switch_string_t>{text1, text2, text3})] =
      "test_attribute_list_string=[\"this is a test 1\", \"this is "
      "a test 2\", \"this is a test 3\"]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_STRING, text1)] =
      "test_attribute_string=\"this is a test 1\"";

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_OID,
                  std::vector<switch_object_id_t>{oid1, oid2, oid3})] =
      "test_attribute_list_oid=[0xf0000000000000ff, "
      "0xe0000000000000ee, 0xc0000000000000cc]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_OID, oid1)] =
      "test_attribute_oid=0xf0000000000000ff";

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_MAC,
                  std::vector<switch_mac_addr_t>{mac1, mac2, mac3})] =
      "test_attribute_list_mac=[01:02:03:04:05:06, "
      "07:08:09:0a:0b:0c, 0d:0e:0f:10:11:12]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_MAC, mac1)] =
      "test_attribute_mac=01:02:03:04:05:06";

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_IP4,
                  std::vector<switch_ip_address_t>{ip4_1, ip4_2, ip4_3})] =
      "test_attribute_list_ip4=[1.2.3.4, 5.6.7.8, 9.10.11.12]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_IP4, ip4_1)] =
      "test_attribute_ip4=1.2.3.4";

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_IP6,
                  std::vector<switch_ip_address_t>{ip6_1, ip6_2, ip6_3})] =
      "test_attribute_list_ip6=[102:304:506:708:90a:b0c:d0e:f10, "
      "1112:1314:1516:1718:191a:1b1c:1d1e:1f20, "
      "2122:2324:2526:2728:292a:2b2c:2d2e:2f30]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_IP6, ip6_1)] =
      "test_attribute_ip6=102:304:506:708:90a:b0c:d0e:f10";

  map[smi::attr_w(
      SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_IP4_PREFIX,
      std::vector<switch_ip_prefix_t>{ip4_prefix1, ip4_prefix2, ip4_prefix3})] =
      "test_attribute_list_ip4_prefix=[1.2.3.4/255, 5.6.7.8/238, "
      "9.10.11.12/204]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_IP4_PREFIX,
                  ip4_prefix1)] = "test_attribute_ip4_prefix=1.2.3.4/255";

  map[smi::attr_w(
      SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_IP6_PREFIX,
      std::vector<switch_ip_prefix_t>{ip6_prefix1, ip6_prefix2, ip6_prefix3})] =
      "test_attribute_list_ip6_prefix="
      "[102:304:506:708:90a:b0c:d0e:f10/255, "
      "1112:1314:1516:1718:191a:1b1c:1d1e:1f20/238, "
      "2122:2324:2526:2728:292a:2b2c:2d2e:2f30/204]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_IP6_PREFIX,
                  ip6_prefix1)] =
      "test_attribute_ip6_prefix="
      "102:304:506:708:90a:b0c:d0e:f10/255";

  map[smi::attr_w(SWITCH_TEST_OBJECT_LIST_ATTR_TEST_ATTRIBUTE_LIST_ENUM,
                  std::vector<switch_enum_t>{enum1, enum2, enum3})] =
      "test_attribute_list_enum=[enum_val_2(1), enum_val_3(2), 3]";
  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_ENUM, enum1)] =
      "test_attribute_enum=enum_val_2(1)";

  map[smi::attr_w(SWITCH_TEST_OBJECT_1_ATTR_TEST_ATTRIBUTE_RANGE, range1)] =
      "test_attribute_range=0-10";

  return map;
})();

static std::list<std::string> failed;

void printTest(const std::string &name,
               const bool status,
               const std::string &func) {
  static uint32_t counter = 1;
  if (!status) {
    failed.push_back(func + "::" + name);
  }
  std::cout << std::right << "[ " << std::setfill('0') << std::setw(3)
            << counter++ << " ] "
            << "\033[1;33m" << std::left << std::setfill(' ') << std::setw(50)
            << name << "\033[0m" << std::right << std::setfill('.')
            << std::setw(20) << ".\033[0m "
            << (status ? "[ \033[1;32mPASS\033[0m ]"
                       : "[ \033[1;31mFAIL\033[0m ]")
            << std::endl;
}

void summary() {
  if (failed.size()) {
    std::cerr << std::endl
              << std::endl
              << "Some tests did not pass:" << std::endl;
    for (auto test : failed) {
      std::cerr << "\t- " << test << std::endl;
    }
    std::cerr << std::endl;
    assert(false);
  }
  std::cout << std::endl << std::endl << "All tests passed!!!" << std::endl;
}

std::string getAttrName(const smi::attr_w &attr) {
  auto attr_id = attr.id_get();
  switch_object_type_t ot = model_info->get_object_type_from_attr_id(attr_id);
  const smi::ObjectInfo *object_info = model_info->get_object_info(ot);
  const smi::AttributeMetadata *attr_md =
      object_info->get_attr_metadata(attr_id);
  return attr_md->get_attr_name();
}

void checkAttr(const smi::attr_w &attr, const std::string &value) {
  auto it = attributes_map.find(attr);
  std::string test_name = "smi::attr_w ";
  test_name += getAttrName(attr).substr(15);
  if (it == attributes_map.end()) {
    printTest(test_name, false, "testAttributeOutput");
    std::cerr << getAttrName(attr) << " not found in map with expected results."
              << std::endl;
  } else if (value != it->second) {
    printTest(test_name, false, "testAttributeOutput");
    std::cerr << "Expected: " << it->second << std::endl
              << "Received: " << value << std::endl;
  }
  printTest(test_name, true, "testAttributeOutput");
}

void checkAttrValue(const attr_w &attr, const std::string &value) {
  auto it = attributes_map.find(attr);
  std::string test_name = "switch_attribute_value_t ";
  test_name += getAttrName(attr).substr(15);
  if (it == attributes_map.end()) {
    printTest(test_name, false, "testAttributeValueOutput");
    std::cerr << getAttrName(attr) << " not found in map with expected results."
              << std::endl;
  }

  std::string expected = it->second;
  expected = expected.substr(expected.find("=") + 1);
  if (attr.type_get() == SWITCH_TYPE_ENUM ||
      attr.list_type_get() == SWITCH_TYPE_ENUM) {
    expected =
        std::regex_replace(expected, std::regex("[^,\\[\\]\\s\\d\\(\\)]+"), "");
    expected =
        std::regex_replace(expected, std::regex("(\\d*\\((\\d+)\\))"), "$2");
  }
  if (value != expected) {
    printTest(test_name, false, "testAttributeValueOutput");
    std::cerr << "Expected: " << expected << std::endl
              << "Received: " << value << std::endl;
  }
  printTest(test_name, true, "testAttributeValueOutput");
}

void testAttributeValue() {
  for (auto &i : attributes_map) {
    std::stringstream ss;
    if (i.first.type_get() != SWITCH_TYPE_LIST) {
      ss << i.first.value_get();
      checkAttrValue(i.first, ss.str());
    } else {
      auto list = i.first.getattr_list();
      std::vector<switch_attribute_value_t> values(list.begin(), list.end());
      ss << switch_attribute_value_t{
          .list =
              {
                  .list_type = i.first.list_type_get(),
                  .count = values.size(),
                  .list = values.data(),
              },
          .type = SWITCH_TYPE_LIST};
      checkAttrValue(i.first, ss.str());
    }
  }
}

void testAttribute() {
  for (auto &i : attributes_map) {
    std::stringstream ss;
    ss << i.first;
    checkAttr(i.first, ss.str());
  }
}

void testVectorOfAttributes() {
  std::vector<smi::attr_w> vec;
  std::stringstream expected;
  std::stringstream real;

  for (auto &i : attributes_map) {
    vec.push_back(i.first);
    expected << i.second << std::endl;
  }
  real << vec;
  bool result = real.str() == expected.str();
  printTest("std::vector<smi::attr_w>", result, __func__);
  if (!result) {
    std::cerr << "Expected: " << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << expected.str() << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << "Received: " << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << real.str() << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl;
  }
}

void testSetOfAttributes() {
  std::set<smi::attr_w> test_set;
  std::stringstream expected;
  std::stringstream real;

  for (auto &i : attributes_map) {
    test_set.insert(i.first);
  }
  for (auto &i : test_set) {
    expected << i << std::endl;
  }
  real << test_set;
  bool result = real.str() == expected.str();
  printTest("std::set<smi::attr_w>", result, __func__);
  if (!result) {
    std::cerr << "Expected: " << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << expected.str() << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << "Received: " << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl
              << real.str() << std::endl
              << std::setfill('-') << std::setw(66) << "-" << std::endl;
  }
}

void testSwitchOperation() {
  std::map<switch_operation_t, const char *> op_map{
      {SMI_CREATE_OPERATION, "create"},
      {SMI_GET_OPERATION, "get"},
      {SMI_SET_OPERATION, "set"},
      {SMI_DELETE_OPERATION, "delete"}};
  for (auto &op : op_map) {
    std::stringstream ss;
    ss << op.first;
    bool result = ss.str() == op.second;
    printTest(std::string("switch_operation_t ") + op.second, result, __func__);
    if (!result) {
      std::cerr << "Expected: " << op.second << std::endl
                << "Received: " << ss.str() << std::endl;
    }
  }
}

void testOperations() {
  bool logSucceeded;
  toggle_all_operations(true);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR, "test message");
  assert(logSucceeded);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_DELETE_OPERATION,
                            "test message");
  printTest("testOperations delete", logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_GET_OPERATION,
                            "test message");
  printTest("testOperations get", logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_SET_OPERATION,
                            "test message");
  printTest("testOperations set", logSucceeded, __func__);
  toggle_all_operations(false);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_CREATE_OPERATION,
                            "test message");
  printTest("testOperations create off", !logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_DELETE_OPERATION,
                            "test message");
  printTest("testOperations delete off", !logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_GET_OPERATION,
                            "test message");
  printTest("testOperations get off", !logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_SET_OPERATION,
                            "test messgae");
  printTest("testOperations set off", !logSucceeded, __func__);
  toggle_operation(SMI_CREATE_OPERATION, true);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_CREATE_OPERATION,
                            "test message");
  printTest("testOperations create on", logSucceeded, __func__);
  toggle_operation(SMI_GET_OPERATION, true);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_GET_OPERATION,
                            "test message");
  printTest("testOperations get on", logSucceeded, __func__);
  toggle_operation(SMI_DELETE_OPERATION, true);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_DELETE_OPERATION,
                            "test messgae");
  printTest("testOperations delete on", logSucceeded, __func__);
}

void testGeneralVerbosity() {
  set_log_level(SWITCH_API_LEVEL_DEBUG);
  bool logSucceeded;
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR, "test message");
  printTest("log error on debug", logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DEBUG, "test message");
  printTest("log debug on debug", logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DETAIL, "test message");
  printTest("log detail on debug", !logSucceeded, __func__);
}

void testVerbosityOperationCombination() {
  set_log_level(SWITCH_API_LEVEL_DEBUG);
  toggle_all_operations(true);
  toggle_operation(SMI_CREATE_OPERATION, false);
  bool logSucceeded;
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_GET_OPERATION,
                            "test message");
  printTest("log error on debug operation get", logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_ERROR,
                            SWITCH_OT_NONE,
                            SMI_CREATE_OPERATION,
                            "test message");
  printTest("log error on debug operation create", !logSucceeded, __func__);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DETAIL,
                            SWITCH_OT_NONE,
                            SMI_GET_OPERATION,
                            "test message");
  printTest("log detail on debug operation get", !logSucceeded, __func__);
}

void testDifferentTypesDontCrash() {
  switch_operation_t operation;
  switch_log(SWITCH_API_LEVEL_ERROR, SWITCH_OT_NONE, "{}", operation);
  switch_object_id_t objectId;
  switch_log(SWITCH_API_LEVEL_ERROR, SWITCH_OT_NONE, "{}", objectId);
  printTest("different operations no crash", true, __func__);
}

void testObjectOptions() {
  switch_object_type_t object_type_5 = SWITCH_OBJECT_TYPE_TEST_OBJECT_5;
  switch_object_type_t object_type_6 = SWITCH_OBJECT_TYPE_TEST_OBJECT_6;
  bool logSucceeded;

  set_log_level(SWITCH_API_LEVEL_WARN);
  set_log_level_object(object_type_5, SWITCH_API_LEVEL_ERROR);
  set_log_level_object(object_type_6, SWITCH_API_LEVEL_WARN);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_ERROR, object_type_5, "sample message");
  printTest("log error on error for object type", logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_ERROR, object_type_6, "sample message");
  printTest("log error on warn for object type", logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_WARN, object_type_5, "sample message");
  printTest("log warn on error for object type", !logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_WARN, object_type_6, "sample message");
  printTest("log warn on warn for object type", logSucceeded, __func__);

  set_log_level_object(object_type_5, SWITCH_API_LEVEL_DETAIL);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_5, "sample message");
  printTest("log detail on detail for object type", !logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_6, "sample message");
  printTest("log detail on warn for object type", !logSucceeded, __func__);
  set_log_level(SWITCH_API_LEVEL_DETAIL);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_5, "sample message");
  printTest("log detail on detail for object type", logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_6, "sample message");
  printTest("log detail on warn for object type", !logSucceeded, __func__);
  set_log_level_object(object_type_6, SWITCH_API_LEVEL_DEBUG);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DEBUG, object_type_6, "sample message");
  printTest("log debug on warn for object type", logSucceeded, __func__);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_6, "sample message");
  printTest("log detail on warn for object type", !logSucceeded, __func__);
  set_log_level_object(object_type_6, SWITCH_API_LEVEL_DETAIL);
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DETAIL, object_type_6, "sample message");
  printTest("log detail on detail for object type", logSucceeded, __func__);

  toggle_all_operations(true);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DETAIL,
                            object_type_5,
                            SMI_GET_OPERATION,
                            "sample message");
  printTest(
      "log detail on detail for object type get on", logSucceeded, __func__);
  toggle_all_operations(false);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DETAIL,
                            object_type_5,
                            SMI_GET_OPERATION,
                            "sample message");
  printTest(
      "log detail on detail for object type get off", !logSucceeded, __func__);
  set_log_level_all_objects(SWITCH_API_LEVEL_DETAIL);
  logSucceeded = switch_log(SWITCH_API_LEVEL_DEBUG,
                            object_type_5,
                            SMI_GET_OPERATION,
                            "{}",
                            "sample message");
  printTest(
      "log debug on detail for object type get off", !logSucceeded, __func__);
  object_type_5 = 0;
  logSucceeded =
      switch_log(SWITCH_API_LEVEL_DEBUG, object_type_5, "{}", "sample message");
  printTest("log debug on detail for null object type", logSucceeded, __func__);
}

int main(void) {
  const char *const test_model_name = TESTDATADIR "/test/test_model.json";
  switch_store::object_info_init(test_model_name, false, NULL);

  testGeneralVerbosity();
  testOperations();
  testVerbosityOperationCombination();
  testDifferentTypesDontCrash();

  model_info = switch_store::switch_model_info_get();
  assert(model_info != NULL);

  set_log_level_all_objects(SWITCH_API_LEVEL_ERROR);
  testObjectOptions();

  testSwitchOperation();
  testAttributeValue();
  testAttribute();
  testVectorOfAttributes();
  testSetOfAttributes();

  summary();
  printf("\n\nAll tests passed!\n");
  return 0;
}
